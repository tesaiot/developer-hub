# TESAIoT Bento Kit — Master Project Template

<p align="center">
  <strong>ฐานรากสำหรับการพัฒนา AIoT บน PSoC Edge E84</strong><br>
  โปรเจกต์ตั้งต้นสำหรับ <strong>Practise Examples</strong> ของหลักสูตร <strong>TESAIoT Firmware Stack</strong><br>
  ภายใต้ <a href="https://www.tesaiot.dev"><strong>TESAIoT Foundation Platform</strong></a><br>
  โดย <strong>สมาคมสมองกลฝังตัวไทย (TESA)</strong>
</p>

---

## 🎓 เกี่ยวกับ Template นี้

`tesaiot_bento_kit_master` คือ **โปรเจกต์ multi-core ตั้งต้นที่ตั้งค่าทุกอย่างให้เรียบร้อยแล้ว**
สำหรับรันชุดตัวอย่าง **Practise Examples** บนบอร์ด **Infineon PSoC Edge E84 (KIT_PSE84_AI)**
พร้อมจอสัมผัส 4.3 นิ้ว — microcontroller สถาปัตยกรรม Arm Cortex-M55 + Cortex-M33 คู่
ที่มาพร้อม GPU แบบ VGLite สำหรับ UI graphics ความเร็วสูง

นักพัฒนาเพียงแค่:

1. Clone โปรเจกต์นี้ครั้งเดียว แล้ว `make getlibs`
2. วางไฟล์ตัวอย่าง (practise example) ลงในโฟลเดอร์ `proj_cm55/apps/`
3. `make build` → ได้ **combined hex รวม 3 cores** ในไฟล์เดียว
4. `make program` flash ลงบอร์ด

**ไม่ต้องแก้ `main.c`, `Makefile` หรือ config ใด ๆ** — ทุก subsystem ของ Core Firmware Stack
(จอแสดงผล, ระบบสัมผัส, sensor buses, WiFi stack, FreeRTOS, LVGL) ถูก initialize เตรียมไว้แล้ว

> 💡 ตัวอย่างที่ build แล้วพร้อม flash (combined hex) ดาวน์โหลดได้จาก
> [GitHub Releases](https://github.com/tesaiot/developer-hub/releases) — ไม่ต้อง build เองก็ได้

### Episode Entry Contract

Template เรียก `example_main()` หนึ่งครั้งหลัง initialize ระบบเสร็จ — ตัวอย่างแต่ละตัว
implement ฟังก์ชันนี้ใน `main_example.c`:

```c
// proj_cm55/apps/main_example.c
#include "pse84_common.h"     /* UI palette + helper inlines ที่ practise ใช้ร่วมกัน */

void example_main(lv_obj_t *parent)
{
    /* สร้าง UI ของตัวอย่างบน parent ที่ template ส่งเข้ามา */
}
```

เมื่อ `apps/` ว่าง template จะใช้ **weak fallback** (`app_core/example_main_default.c`)
แสดงหน้า splash แทน

---

## 🧭 สิ่งที่ Core TESAIoT Firmware Stack เตรียมให้พร้อมใช้

เมื่อบอร์ด boot ขึ้นมา ระบบจะเตรียมทรัพยากรทั้งหมดนี้ให้ทำงานแล้ว — ตัวอย่างของนักพัฒนา
เพียงแค่ **เรียกใช้ API** โดยไม่ต้อง init hardware เอง

### 🖼️ ระบบแสดงผล (Graphics + Display)

| Capability | รายละเอียดที่นักพัฒนาใช้ได้ |
| --- | --- |
| **LVGL 9** graphics library | Widget ครบชุด (label, button, textarea, keyboard, chart, slider, arc, meter, tabview, list, image, ...) พร้อม flex/grid layout, animations, events, styles |
| **VGLite GPU** acceleration | การวาดทุกอย่างถูกเร่งความเร็วด้วย GPU ฮาร์ดแวร์ของ PSoC Edge — ไม่ต้องเขียน GPU code เอง |
| **Display drivers** 3 แบบ | เลือกได้ตามบอร์ด (4.3", 7", หรือ 10.1") — ดู section "Hardware ที่รองรับ" |
| **Fonts** | Montserrat หลายขนาด + **Noto Sans Thai** (14/16/20/28) พร้อมใช้ — รองรับข้อความไทย |

**นักพัฒนาเขียนแค่:**
```c
lv_obj_t *label = lv_label_create(parent);
lv_label_set_text(label, "สวัสดี TESAIoT");
lv_obj_center(label);
```

### 👆 ระบบสัมผัส (Touch Input)

Touch controller (GT911 / FT5406 / ILI2511) ถูกติดตั้งและเชื่อมกับ LVGL ให้แล้ว
event จากการแตะหน้าจอจะไปถึง widget โดยอัตโนมัติ

```c
lv_obj_add_event_cb(my_button, on_clicked, LV_EVENT_CLICKED, NULL);
```

### 🌡️ Sensor Bus (I2C + I3C) + Direct Readers

| Bus | รองรับ Sensor | Reader API ที่พร้อมใช้ |
| --- | --- | --- |
| **Sensor I2C** (1.8V) | DPS368 (ความดัน), SHT4x (อุณหภูมิ/ความชื้น), BMI270 (IMU 6 แกน) | `dps368_reader`, `sht4x_reader`, `bmi270_reader` |
| **Sensor I3C** | BMM350 (magnetometer/เข็มทิศ) | `bmm350_reader` |

ตัวอย่าง Practise อ่านเซนเซอร์ผ่าน **direct reader** (อยู่ใน `proj_cm55/app_sensor/`) บน CM55
โดยใช้ bus handle ที่ template เตรียมไว้ใน `sensor_bus.h`:

```c
#include "pse84_common.h"
#include "sensor_bus.h"
#include "bmi270/bmi270_reader.h"

void example_main(lv_obj_t *parent) {
    bmi270_reader_init(&sensor_i2c_controller_hal_obj);   /* bus เตรียมไว้แล้ว */
    bmi270_sample_t s;
    if (bmi270_reader_poll(&s)) {
        /* ใช้ s.ax, s.gy, ... วาดบน parent */
    }
}
```

### 📡 ระบบเครือข่ายไร้สาย (WiFi + TCP/IP + TLS)

ชุด middleware ของ Infineon WiFi Connection Manager (cy_wcm) ถูก link เข้า master พร้อมใช้:
- **WiFi radio driver** (WHD + CYW55500) · **cy_wcm API** (scan/connect/disconnect)
- **lwIP** (TCP/IP) · **mbedTLS** (hardware-accelerated) · **WPA3**

```c
cy_wcm_init(&config);
cy_wcm_connect_ap(&params, &ip_info);
```

### ⚙️ FreeRTOS + 🔍 Debug

- **FreeRTOS** kernel พร้อมใช้: task, mutex, semaphore, queue, timer + tickless idle
- **`printf()`** ไปยัง debug UART (115200 baud) เปิดใช้งานแล้ว — ดู log ผ่าน serial monitor ได้ทันที

---

## 🛠️ Hardware ที่รองรับ

- **Board:** Infineon **KIT_PSE84_AI** (PSoC Edge E84 AI Development Kit) ⭐

เลือกจอด้วย `CONFIG_DISPLAY` ใน `common.mk`:

| `CONFIG_DISPLAY` | Panel | ขนาด | Touch |
| --- | --- | --- | --- |
| `W4P3INCH_DISP` ⭐ **default** | Waveshare 4.3" DSI | 480×800 | FT5406 |
| `WS7P0DSI_RPI_DISP` | Waveshare 7" DSI | 1024×600 | GT911 |
| `WF101JTYAHMNB0_DISP` | EK79007AD3 10.1" DSI | 1280×800 | ILI2511 |

**Toolchain:** ModusToolbox 3.6+ · GCC ARM Embedded 14.2.1 · KitProg3 (flash/debug)

---

## 🚀 Quick Start

### 1. Clone และเตรียม dependencies

```sh
git clone -b tesaiot_bento_kit_master https://github.com/tesaiot/developer-hub.git tesaiot_bento_kit_master
cd tesaiot_bento_kit_master

# ดึง libraries (ทำครั้งเดียว)
make getlibs
```

### 2. วาง Practise Example ที่ต้องการ

`apps/` คือช่อง drop-in (ไฟล์ระบบอยู่ใน `app_core/` นอก `apps/` จึงล้าง `apps/` ได้ปลอดภัย):

```sh
# ล้างตัวอย่างเดิม แล้ววางตัวอย่างใหม่
rm -rf proj_cm55/apps/*
rsync -a /path/to/prac_b18_thai_text/ proj_cm55/apps/
```

> ซอร์สของตัวอย่างทั้งหมดอยู่ที่ branch
> [`tesaiot_dev_kit_practise_codes`](https://github.com/tesaiot/developer-hub/tree/tesaiot_dev_kit_practise_codes)

### 3. Build แล้ว Flash

```sh
make build      # ได้ build/app_combined.hex (รวม 3 cores)
make program    # flash ลงบอร์ดผ่าน KitProg3
```

### 4. ดู serial log

เปิด serial monitor ที่ 115200 baud:
- macOS: `screen /dev/cu.usbmodem* 115200`
- Linux: `screen /dev/ttyACM0 115200`
- Windows: PuTTY / TeraTerm

---

## 📚 Practise Examples (19 ตัวพร้อมใช้บน firmware)

ชุดตัวอย่างฝึกฝน แบ่งเป็น 3 กลุ่ม — แต่ละตัวมี combined hex ใน
[GitHub Releases](https://github.com/tesaiot/developer-hub/releases) ดาวน์โหลดแล้ว flash ได้ทันที

| กลุ่ม | จำนวน | ตัวอย่าง |
| --- | :---: | --- |
| **A — Pure UI** | 5 | thai_text, color_mixer, lcd_console, tile_navigation, multi_page_app |
| **B — Sensor Visualization** | 10 | line_chart_accel, motion_detector, level_bubble, gauge_cluster, bar_chart_multi, data_logger, environ_monitor, sensor_fusion, automation_rules, chart_statistics |
| **C — Production Patterns** | 4 | production_dashboard, smart_watch, status_panel, wifi_connect |

รายการเต็ม + เอกสารภาษาไทย: branch
[`tesaiot_dev_kit_practise_codes`](https://github.com/tesaiot/developer-hub/tree/tesaiot_dev_kit_practise_codes)

> หมายเหตุ: ตัวอย่างชุดเกม (รหัส a10–a15) อยู่ระหว่างพัฒนา game framework บน firmware
> จึงยังไม่รวมอยู่ในชุดนี้

---

## 📂 โครงสร้างของ Template

```
tesaiot_bento_kit_master/
├── proj_cm55/              ← โปรเจกต์หลัก (Cortex-M55, UI + sensors + WiFi)
│   ├── main.c              ← เตรียมทุก subsystem แล้วเรียก example_main() (ไม่ต้องแก้)
│   ├── apps/               ← ⭐ ตำแหน่งวาง practise example (เริ่มต้นว่าง)
│   ├── app_core/           ← ระบบของ template:
│   │   ├── app_interface.h          (สัญญา example_main)
│   │   ├── example_main_default.c   (weak fallback / splash)
│   │   └── pse84_common.h           (UI palette + helper inlines ของ practise)
│   ├── app_sensor/         ← direct readers: bmi270 / bmm350 / dps368 / sht4x
│   ├── app_assets/         ← โลโก้ + ฟอนต์ Noto Sans Thai
│   ├── platform/           ← sensor_bus.h, retarget_io
│   ├── lvgl_cfg/           ← LVGL configuration
│   ├── lvgl_port/          ← LVGL display + touch bindings
│   ├── lvgl_override/      ← VGLite GPU draw path
│   ├── core_cfg/           ← FreeRTOS + display I2C config
│   └── deps/               ← middleware packages (*.mtb)
│
├── proj_cm33_s/            ← Secure CM33 boot project
├── proj_cm33_ns/           ← Non-secure CM33 project
├── bsps/TARGET_APP_KIT_PSE84_AI/   ← Board Support Package
├── common.mk               ← TARGET, CONFIG, TOOLCHAIN, CONFIG_DISPLAY
├── common_app.mk
├── Makefile                ← top-level multi-core orchestration
└── docs/EXTENDING.md       ← คู่มือการต่อยอด master ระดับลึก
```

> `build/` และ `libs/` ไม่ได้ commit ไว้ใน branch — รัน `make getlibs` เพื่อดึง dependencies
> และ `make build` เพื่อสร้าง artifacts

---

## 🔗 ทรัพยากรเพิ่มเติม

- **🌐 TESAIoT Foundation Platform**: <https://www.tesaiot.dev>
- **📦 Combined hex releases**: <https://github.com/tesaiot/developer-hub/releases>
- **📘 Practise source**: branch [`tesaiot_dev_kit_practise_codes`](https://github.com/tesaiot/developer-hub/tree/tesaiot_dev_kit_practise_codes)
- **🔧 ต่อยอดระดับลึก**: `docs/EXTENDING.md`
- **📖 LVGL Docs**: <https://docs.lvgl.io/9.x/> · **ModusToolbox**: <https://www.infineon.com/modustoolbox>

---

## 🏛️ Credit & License

**TESAIoT Firmware Stack** เป็นส่วนหนึ่งของ **TESAIoT Foundation Platform** พัฒนาและดูแลโดย
**สมาคมสมองกลฝังตัวไทย (Thai Embedded Systems Association — TESA)** · <https://www.tesaiot.dev>

**MIT License** (ดู `LICENSE`) — รวมส่วนประกอบ open-source จาก Infineon ModusToolbox BSP +
middleware, LVGL, FreeRTOS, Bosch Sensortec BMI270/BMM350 SensorAPI, Sensirion SHT4x,
Infineon WiFi Host Driver + mbedTLS โดย license ของแต่ละส่วนเป็นไปตามต้นทาง
