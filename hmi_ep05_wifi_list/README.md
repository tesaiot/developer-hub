# EP05 — WiFi List (สแกน WiFi แล้วแสดงผลเป็นรายการ)

> **Series:** HMI Menu & Setting • **Episode:** 5 / 7 • **ระดับ:** intermediate

## Why — ทำไมต้องเรียนตัวอย่างนี้?

ก่อนจะ "เชื่อมต่อ" WiFi ได้ (ep07) ผู้ใช้ต้องมองเห็นก่อนว่ามีเครือข่ายอะไรบ้างให้เลือก
ep05 คือ "หน้า scan" — กดแล้วรอสักครู่ แล้วมี list ของ AP รอบตัวโผล่มาให้เลือก
พร้อม signal strength และ security type (Open / WPA2 / WPA3)

การสแกน WiFi บน PSoC Edge ใช้ stack ซ้อนกันหลายชั้น: `whd` driver → `cy_wcm`
(Connection Manager) → application callback ที่เรียกเมื่อแต่ละ AP ถูกค้นพบ
ep05 สอนวิธี wrap โครงนี้เป็น **service layer** ที่ UI เรียกง่าย ๆ โดยไม่ต้องรู้
รายละเอียดของ WHD

เราจะได้เรียนรู้:

- วิธีแยก **service** (scan logic) ออกจาก **UI page** แบบสะอาด
- Async pattern: UI เรียก `start_scan` → service run ใน background → callback แจ้ง
  UI เมื่อมีผลลัพธ์ใหม่
- การใช้ LVGL `lv_async_call()` เพื่อส่งงานจาก non-LVGL thread กลับเข้า LVGL thread
  อย่างปลอดภัย (LVGL ไม่ thread-safe ถ้าเรียกจากหลาย thread พร้อมกัน)
- Pre-initialization pattern: ถ้า init `cy_wcm` ใน callback ปุ่มตรง ๆ ผู้ใช้จะเห็น UI
  ค้างยาว 1-3 วินาที — ดังนั้นเรา `wifi_scan_service_preinit()` ตั้งแต่ boot

## What — ตัวอย่างนี้แสดงอะไร?

- Navigation shell จาก ep04 (header / nav / stage / footer)
- **Page WiFi List** (หน้าหลักของ episode) แทนที่ "Home" — ประกอบด้วย:
  - ปุ่ม **"Scan"** ที่ header → ยิง `wifi_scan_service_start()`
  - Status label ("Scanning…", "Found N networks", "Scan failed")
  - **Scrollable list** ของ AP; แต่ละแถวเป็น row container ที่แสดง:
    - SSID (ชื่อ WiFi) — clip ถ้ายาวเกิน
    - RSSI (ความแรงสัญญาณ) — แสดงเป็นตัวเลข dBm + ไอคอน bar
    - Security — icon ล็อคถ้า encrypted, เปิดถ้า open

### ไฟล์ที่มีใน episode นี้

| File | บทบาท |
| --- | --- |
| `main_example.c` | เรียก `wifi_scan_service_preinit()` ตอน boot แล้ว forward เข้า UI |
| `nav/ui_menu_navigation.c` / `.h` | shell จาก ep04 (ปรับให้ตั้ง WiFi List เป็นหน้า default) |
| `nav/ui_menu_layout.h` | layout constants |
| `nav/menu_nav_logic.c` / `.h` | page switch logic |
| `wifi_list/ui_wifi_list_page.c` / `.h` | สร้าง list + bind ต่อ scan service callback |
| `wifi_list/wifi_scan_service.c` / `.h` | wrapper เหนือ `cy_wcm` — start/stop/preinit + result callback |
| `wifi_list/wifi_scan_types.h` | struct `wifi_scan_result_t` (ssid, rssi, security, bssid) |
| `assets/app_logo.*` | โลโก้ |

## How — ทำงานอย่างไร?

### ขั้นที่ 1: `example_main()` pre-init WiFi ก่อน UI

```c
void example_main(lv_obj_t *parent)
{
    (void)parent;
    cy_rslt_t r = wifi_scan_service_preinit();
    if(r != CY_RSLT_SUCCESS) {
        printf("[WIFI_LIST] STARTUP_WIFI_INIT_FAIL rslt=0x%08lx\r\n", ...);
    }
    ui_wifi_list_create();
}
```

`wifi_scan_service_preinit()` ภายในจะ:

1. `cy_wcm_init()` ด้วย interface STA
2. ไม่ start scan จริง — แค่ "warm up" driver stack

ถ้า init fail จะ log แต่ UI ยังเปิดได้ (แค่ scan ใช้ไม่ได้)

### ขั้นที่ 2: `ui_wifi_list_create()` build shell + stage

เรียก nav shell จาก ep04 แล้วใช้ `render_wifi_list_page()` เป็น default stage builder
แทน home page เดิม

### ขั้นที่ 3: เมื่อผู้ใช้กด Scan

```c
static void on_scan_click(lv_event_t *e)
{
    lv_label_set_text(s_status_label, "Scanning...");
    lv_obj_clean(s_list_container);   /* เคลียร์ผล list เดิม */
    wifi_scan_service_start(on_scan_result_cb, on_scan_done_cb);
}
```

`wifi_scan_service_start()` เรียก `cy_wcm_start_scan()` ซึ่งจะ spawn worker ของมันเอง

### ขั้นที่ 4: Callback จาก WHD → UI thread

`cy_wcm` callback ถูกเรียกจาก **WCM internal task** ไม่ใช่ LVGL task นี่คือ
trap ที่คนเจอบ่อยที่สุด — ถ้าเราเรียก `lv_label_set_text()` ตรงนั้นเลย จะเกิด race
กับ `lv_timer_handler()` ที่กำลังวาดอยู่

แก้ด้วย `lv_async_call()`:

```c
static void scan_cb_from_whd(cy_wcm_scan_result_t *r, void *ud, cy_wcm_scan_status_t s)
{
    /* Copy ข้อมูลจาก r เข้า global buffer (heap-safe) */
    queue_result(r);

    /* Schedule update บน LVGL thread */
    lv_async_call(flush_results_into_list, NULL);
}
```

`flush_results_into_list` จะรันในรอบ LVGL timer ต่อไป ซึ่ง thread-safe

### ขั้นที่ 5: วาด list row

```c
for(i = 0; i < scan_count; i++) {
    lv_obj_t *row = lv_obj_create(list_container);
    lv_obj_set_size(row, lv_pct(100), 56);

    lv_obj_t *ssid_lbl = lv_label_create(row);
    lv_label_set_text(ssid_lbl, results[i].ssid);

    lv_obj_t *rssi_lbl = lv_label_create(row);
    lv_label_set_text_fmt(rssi_lbl, "%d dBm", results[i].rssi);

    lv_obj_t *sec_icon = lv_label_create(row);
    lv_label_set_text(sec_icon,
        results[i].security == CY_WCM_SECURITY_OPEN ? "" : LV_SYMBOL_CLOSE);
}
```

List container ใช้ `lv_obj_set_scroll_dir(LV_DIR_VER)` เพื่อให้เลื่อนแนวตั้งได้

## วิธีติดตั้งและรัน

```sh
cd tesaiot_dev_kit_master

find proj_cm55/apps -mindepth 1 -maxdepth 1 \
     ! -name 'app_interface.h' ! -name 'README.md' ! -name '_default' \
     -exec rm -rf {} +

rsync -a ../episodes/hmi_ep05_wifi_list/ proj_cm55/apps/

make clean
make program TARGET=APP_KIT_PSE84_AI CONFIG_DISPLAY=WS7P0DSI_RPI_DISP
```

> **หมายเหตุ:** ต้องแน่ใจว่า module WiFi (M2M/CYW55513) บน kit ทำงานได้ และ
> `deps/*.mtb` ของ master template มี `cy_wcm`, `whd-bsp-integration`, `lwip`,
> `mbedtls` ครบ (master template นี้เตรียมไว้แล้ว)

## สิ่งที่จะเห็นบนหน้าจอ

- Header พร้อมปุ่ม "Scan"
- Status label "Ready to scan"
- เมื่อกด Scan → label เปลี่ยนเป็น "Scanning..."
- หลัง 2-5 วิ → list โผล่พร้อม SSID, RSSI, lock icon (ถ้า encrypted)
- เลื่อน list ขึ้น-ลงได้

## อะไรที่คุณสามารถทดลองเปลี่ยนได้?

1. **Filter โดย RSSI** — แสดงเฉพาะ AP ที่ RSSI > -80
2. **Sort list** — เรียงจากแรงสุดไปอ่อนสุด
3. **เพิ่ม refresh ไอคอน animation** — หมุน icon ตอน scan
4. **Auto-rescan** — ใช้ `lv_timer_create()` เรียก scan ทุก 30 วิ
5. **Show bar เป็น icon** — แปลง RSSI เป็น 1-4 ขีด แทนตัวเลข

## ศัพท์ที่ต้องรู้

- **`cy_wcm`** — Cypress Connection Manager, API เรียก scan/connect/disconnect
- **`whd`** — Cypress WiFi Host Driver, อยู่ใต้ `cy_wcm`
- **`cy_wcm_scan_result_t`** — struct ที่ `whd` ส่งมาต่อ 1 AP
- **`lv_async_call(fn, data)`** — schedule ให้ `fn(data)` ทำงานบน LVGL thread
- **Service layer** — โมดูลที่ wrap API ซับซ้อนให้ UI เรียกง่าย
- **Pre-init pattern** — init เตรียมไว้ตอน boot เพื่อไม่ให้ user ต้องรอ
- **SSID / BSSID / RSSI** — Service Set ID (ชื่อ), MAC ของ AP, ความแรงสัญญาณ dBm
- **`LV_SYMBOL_CLOSE`** — glyph ล็อค/กากบาทใน LVGL built-in font

## ขั้นต่อไป

**EP06 — WiFi Profile NVM** จะเพิ่มหน้าที่เก็บ SSID + password ลง non-volatile
memory (NVM / flash) เพื่อให้บอร์ดจำ credential ได้ข้ามการ reboot และจะสอน
`lv_textarea` + password mode จาก ep03 มาประยุกต์กับ form กรอก profile
