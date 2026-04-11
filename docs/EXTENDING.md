# Extending the TESAIoT Dev Kit Master Template

คู่มือเจาะลึกสำหรับ developer ที่ต้องการ **ต่อยอด** master template:
- เพิ่ม episode ใหม่
- เพิ่ม sensor / peripheral ที่ master level
- เพิ่ม / แก้ display variant
- Hook เข้า init chain ใน `main.c`

> สำหรับภาพรวม + quick start ให้ดู ../README.md ก่อน
> เอกสารนี้เน้น **internal API reference + extension recipes** ที่ไม่มีใน README

---

## 1. Master Init Timeline (ใช้อ้างอิงเมื่อจะ hook เข้า chain)

ลำดับการ initialize ใน `proj_cm55/main.c` (file:line reference ตรงไปยังโค้ดจริง):

### `int main(void)` — main.c:877

| Order | Call | File:Line | หน้าที่ |
| --- | --- | --- | --- |
| 1 | `cybsp_init()` | main.c:881 | Board clock, pin mux, PMU, device trim (จาก design.modus) |
| 2 | `setup_clib_support()` | main.c:890 | CLIB + RTC HAL binding → `time()`, `printf(%f)` พร้อมใช้ |
| 3 | `setup_tickless_idle_timer()` | main.c:892 | LPTimer_1 + MCWDT → FreeRTOS tickless idle |
| 4 | `init_retarget_io()` | main.c:895 | Debug UART 115200 baud → `printf` |
| 5 | `__enable_irq()` | main.c:898 | Enable NVIC global interrupts |
| 6 | `xTaskCreate(cm55_gfx_task, ...)` | main.c:901 | Spawn single graphics task (stack `configMINIMAL_STACK_SIZE * 16`, priority `configMAX_PRIORITIES - 1`) |
| 7 | `vTaskStartScheduler()` | main.c:917 | Transfer control to FreeRTOS |

### `cm55_gfx_task()` — main.c:445

| Order | Call | File:Line | หน้าที่ |
| --- | --- | --- | --- |
| 1 | `Cy_GFXSS_Init(GFXSS, &GFXSS_config, &gfx_context)` | main.c:618 | Display controller + GPU hardware init |
| 2 | `Cy_SysInt_Init(&dc_irq_cfg, dc_irq_handler)` | ~main.c:624 | Display controller IRQ (frame buffer ready) |
| 3 | `Cy_SysInt_Init(&gpu_irq_cfg, gpu_irq_handler)` | ~main.c:636 | GPU IRQ (VGLite completion) |
| 4 | `Cy_SCB_I2C_Init(DISPLAY_I2C_CONTROLLER_HW, ...)` | ~main.c:651 | Display touch I2C bus init |
| 5 | Display panel init (ตาม `CONFIG_DISPLAY`) | ~main.c:680 | Waveshare 4.3"/7" หรือ EK79007AD3 10.1" |
| 6 | `sensor_i2c_controller_init()` | main.c:753 | Dedicated sensor I2C on SCB0 (1.8V) — best-effort |
| 7 | `i3c_controller_init()` | main.c:760 | I3C bus for BMM350 — best-effort |
| 8 | `vg_lite_init_mem(&vg_params)` | main.c:775 | Allocate VGLite GPU heap ใน `.cy_gpu_buf` |
| 9 | `vg_lite_init(W/4, H/4)` | main.c:780 | Initialize VGLite draw context |
| 10 | `lv_init()` | main.c:786 | LVGL kernel init |
| 11 | `lv_port_disp_init()` | main.c:787 | Bind LVGL display driver → GFXSS + VGLite |
| 12 | `lv_port_indev_init()` | main.c:789 | Bind LVGL input device → touch controller |
| 13 | **`example_main(lv_scr_act())`** | main.c:803 | 🎯 **HANDOFF to episode** |
| 14 | `lv_timer_handler()` loop | main.c:815 | Run LVGL refresh + animations forever |

**จุดสำคัญ**: `example_main()` ถูกเรียก **ครั้งเดียว** หลัง LVGL พร้อมใช้. Episode จะ return หลัง setup widget หรือ spawn task เสริมแล้วก็ได้ — LVGL event loop ทำงานต่อให้

---

## 2. Public Resources (API ที่ episode เรียกใช้ได้)

### 2.1 Platform Headers (ใน `proj_cm55/platform/`)

**`sensor_bus.h`** — Extern handles to pre-initialized sensor buses
```c
#include "sensor_bus.h"
/* จะได้: */
extern cy_stc_scb_i2c_context_t sensor_i2c_controller_context;
extern mtb_hal_i2c_t             sensor_i2c_controller_hal_obj;  /* ใช้กับ Bosch / Infineon I2C drivers */
extern cy_stc_i3c_context_t      CYBSP_I3C_CONTROLLER_context;    /* ใช้กับ BMM350 */
/* + cybsp.h ถูก include ให้อัตโนมัติ → CYBSP_I3C_CONTROLLER_HW, _IRQ, _config ใช้ได้เลย */
```

**`retarget_io_init.h`** — Debug UART
```c
#include "retarget_io_init.h"
/* printf ใช้ได้หลัง init_retarget_io() (master เรียกให้แล้ว) */
```

### 2.2 LVGL Port Headers (ใน `proj_cm55/lvgl_port/`)

**`lv_port_disp.h`** — Display resolution macros
```c
#include "lv_port_disp.h"
/* ใช้ในการคำนวณ layout */
#define MY_DISP_HOR_RES  /* 480 สำหรับ 4.3", 1024 สำหรับ 7", 1280 สำหรับ 10.1" */
#define MY_DISP_VER_RES  /* 800 สำหรับ 4.3", 600 สำหรับ 7", 800 สำหรับ 10.1" */
```

**`lv_port_indev.h`** — Touch input device handle (ใน episode ส่วนมากไม่ต้องใช้โดยตรง)

### 2.3 Shared Assets (ใน `proj_cm55/app_assets/`)

**`app_logo.h`** — TESAIoT branding logo
```c
#include "app_logo.h"
extern const lv_image_dsc_t APP_LOGO;   /* use with lv_image_set_src(img, &APP_LOGO) */
```

### 2.4 App Interface Contract (ใน `proj_cm55/apps/`)

**`app_interface.h`** — Required by every episode
```c
#include "app_interface.h"

/* Episode MUST provide a strong definition: */
void example_main(lv_obj_t *parent);
```

---

## 3. Extension Recipes

### Recipe A — เพิ่ม Episode ใหม่ (simple case, 3 ไฟล์)

**Target**: สร้าง episode ที่ใช้ sensor หรือ UI ที่มีอยู่แล้วใน master

1. สร้าง folder `episodes/<epNN_name>/`
2. เขียน `main_example.c`:
   ```c
   #include "app_interface.h"
   #include "sensor_bus.h"         /* ถ้าใช้ sensor */
   #include "lvgl.h"

   void example_main(lv_obj_t *parent)
   {
       /* เขียน UI / logic ที่นี่ */
       lv_obj_t *label = lv_label_create(parent);
       lv_label_set_text(label, "My new lesson");
       lv_obj_center(label);
   }
   ```
3. เขียน `metadata.json` (ดู schema ใน `../README.md` section "Extension")
4. เขียน `README.md` (Thai: Why / What / How)
5. ติดตั้งและทดสอบ:
   ```sh
   find proj_cm55/apps -mindepth 1 -maxdepth 1 \
        ! -name 'app_interface.h' ! -name 'README.md' ! -name '_default' \
        -exec rm -rf {} +
   rsync -a ../episodes/<epNN_name>/ proj_cm55/apps/
   gmake build
   ```

**No Makefile edits needed.** INCLUDES ของ master ใช้ `$(shell find ./apps -type d)` ซึ่ง auto-discover ทุก subfolder ที่ episode สร้างขึ้น

### Recipe B — Episode ที่มี subfolder (complex case)

**Target**: Episode ที่มีโค้ดแยกหลายโมดูล (เช่น `hmi_ep07_final_wifi_manager` ที่มี `nav/`, `wifi_conn/`, `wifi_list/`, `wifi_profile/`)

โครงสร้างที่ master รองรับ:
```
episodes/my_complex_ep/
├── main_example.c
├── metadata.json
├── README.md
├── service/
│   ├── foo_service.c
│   └── foo_service.h
├── pages/
│   ├── page_home.c
│   └── page_home.h
└── widgets/
    ├── my_widget.c
    └── my_widget.h
```

**Episode ใช้ `#include "foo_service.h"`** โดยไม่ต้องใส่ path prefix — เพราะ master's Makefile ทำ `find ./apps -type d` แล้วใส่ทุก subdir ลง `INCLUDES`

**ข้อควรระวัง:**
- ทุก `.c` ใน `apps/**` จะถูก compile อัตโนมัติ → ห้ามทิ้งไฟล์เก่าค้างไว้
- ไฟล์ที่ไม่ต้องการ compile ให้ตั้งชื่อนามสกุลอื่น เช่น `.c.bak` หรือย้ายออกจาก `apps/`
- Header names ควร unique เพื่อหลีกเลี่ยง ambiguity (เช่น `wifi_page.h` อาจชนกันข้ามโฟลเดอร์)

### Recipe C — เพิ่ม Sensor / Peripheral ใหม่ที่ Master Level

**When**: Hardware resource ที่ episode หลายตัวจะใช้ร่วมกัน (เช่น เพิ่ม MAX30102 PPG sensor เพื่อรองรับ health monitoring episodes)

**Step 1 — เพิ่ม middleware package**

หาชื่อ `.mtb` package ของ sensor จาก https://github.com/Infineon/, เช่น:
```sh
# proj_cm55/deps/max30102.mtb
https://github.com/Example/sensor-max30102/#release-v1.0#$$ASSET_REPO$$/sensor-max30102/release-v1.0
```

แล้วรัน:
```sh
gmake getlibs
```

**Step 2 — Init ใน `main.c` cm55_gfx_task**

เพิ่มหลัง `sensor_i2c_controller_init()` (เพราะ MAX30102 ใช้ I2C):
```c
// main.c, ~line 756
cy_rslt_t max_rslt = max30102_init(&sensor_i2c_controller_hal_obj);
if (CY_RSLT_SUCCESS != max_rslt) {
    printf("[MASTER] MAX30102 init failed (0x%08lx)\r\n", (unsigned long)max_rslt);
}
```

**Step 3 — Expose API ผ่าน `platform/sensor_bus.h`**

```c
// platform/sensor_bus.h — เพิ่มบรรทัดนี้
extern max30102_handle_t max30102_handle;

// main.c — declare non-static
max30102_handle_t max30102_handle;
```

**Step 4 — Rebuild + verify**

```sh
gmake build
```

**Step 5 — Episode ใหม่ใช้งานได้ทันที**

```c
// episodes/new_ppg_monitor/main_example.c
#include "sensor_bus.h"
#include "max30102.h"

void example_main(lv_obj_t *parent)
{
    uint32_t hr, spo2;
    max30102_read(&max30102_handle, &hr, &spo2);
    /* ... draw UI ... */
}
```

### Recipe D — เพิ่ม Display Variant ใหม่

**When**: รองรับ panel อื่นนอกเหนือจาก 4 ตัวปัจจุบัน (Waveshare 4.3"/7" + EK79007AD3 10.1")

**Step 1 — เพิ่ม `.mtb` package** ของ display driver ลง `proj_cm55/deps/`

**Step 2 — เพิ่ม branch ใน `proj_cm55/Makefile`**:
```makefile
else ifeq ($(CONFIG_DISPLAY), MY_NEW_DISP)
DEFINES += MTB_DISPLAY_MY_NEW MTB_CTP_MY_NEW
# Ignore unused display drivers
CY_IGNORE += $(SEARCH_display-dsi-waveshare-7-0-lcd-c)
CY_IGNORE += $(SEARCH_touch-ctp-gt911)
# ... exclude others ...
endif
```

**Step 3 — เพิ่ม panel init branch ใน `main.c`**:
```c
// main.c ~line 680
#elif defined(MTB_DISPLAY_MY_NEW)
    mtb_display_my_new_init(DISPLAY_I2C_CONTROLLER_HW, &disp_touch_i2c_controller_context);
#endif
```

**Step 4 — Build ทดสอบ**:
```sh
gmake build CONFIG_DISPLAY=MY_NEW_DISP
```

### Recipe E — เปิด LVGL Font / Widget / Feature เพิ่ม

**When**: Episode ใช้ font หรือ feature ที่ `lv_conf.h` ปิดไว้

แก้ `proj_cm55/lvgl_cfg/lv_conf.h`:
```c
#define LV_FONT_MONTSERRAT_32 1   /* was 0 */
#define LV_USE_MSGBOX 1             /* enable widget */
#define LV_USE_METER 1              /* enable analog meter */
```

**ข้อดีของการเพิ่มใน master**: ทุก episode ที่ต้องการ feature นั้นจะใช้งานได้ทันที ไม่ต้องแก้ซ้ำ

**ข้อเสีย**: Binary ใหญ่ขึ้น (~2-20 KB ต่อ font, ~5-50 KB ต่อ widget)

---

## 4. Common Pitfalls (จาก build-test ประสบการณ์จริง)

### 4.1 `'lv_font_montserrat_XX' undeclared`

Font ขนาดที่ episode ใช้ยังไม่เปิดใน `lv_conf.h`. แก้:
```c
// proj_cm55/lvgl_cfg/lv_conf.h
#define LV_FONT_MONTSERRAT_XX 1
```

Master เปิดไว้แล้ว: 12, 14, 16, 18, 20, 22, 24, 28, 30, 40. ขนาดอื่นต้องเปิดเพิ่ม

### 4.2 `'CYBSP_I3C_CONTROLLER_HW' undeclared`

Episode source file ที่ reference `CYBSP_*` macro ต้อง include header ที่มี declaration:
```c
#include "sensor_bus.h"   /* ได้ cybsp.h + extern sensor handles มาด้วย */
```

### 4.3 `main.c` รัน `sensor_i2c_controller_init()` fail เงียบ

Master จัดการเป็น **best-effort** — ถ้า SCB0 init fail จะ `printf` เตือนและทำงานต่อ เพื่อให้ WiFi-only episodes ยัง boot ได้ ถ้า episode ที่ต้องการ sensor ไม่ work ให้เช็ค serial log ว่ามี:
```
[MASTER] Sensor I2C init failed (0x........) — sensor episodes unavailable
```
หรือ
```
[MASTER] I3C init failed (0x........) — BMM350 compass episodes unavailable
```

ปัญหาปกติคือ pin conflict ใน `design.modus` หรือ I/O voltage ไม่ตรง (sensor bus อยู่บน 1.8V domain)

### 4.4 Duplicate symbol `example_main` ตอน link

สาเหตุ: มี **สอง** ไฟล์ใน `apps/**` ที่ประกาศ `void example_main(lv_obj_t *parent)` เป็น strong symbol

แก้: ลบไฟล์ที่ซ้ำออก. มี strong `example_main` **ได้แค่ตัวเดียว** ต่อ build — default weak stub ใน `_default/example_main_default.c` จะถูกใช้เมื่อไม่มี strong เลย

### 4.5 Linker warning `multiple definition of APP_LOGO`

สาเหตุ: Episode วาง `app_logo.c` มาเอง ทั้งที่ master มี `app_assets/app_logo.c` อยู่แล้ว

แก้: ลบ `app_logo.c` / `app_logo.h` ออกจาก episode folder. ใช้ `#include "app_logo.h"` ตามปกติ — จะ resolve ไปที่ master's copy อัตโนมัติ (ชี้ด้วย INCLUDES)

### 4.6 Build ติดที่ `proj_cm33_s` → schema `cydesignfile_v7` not found

สาเหตุ: BSP ถูกบันทึกด้วย ModusToolbox 3.7 แต่เครื่อง dev ใช้ 3.6

แก้: ใช้ BSP ที่มี schema v6 (ซึ่ง master template ใช้อยู่แล้ว). ห้าม edit `design.modus` ด้วย device-configurator ของ 3.7 แล้ว save — จะ upgrade schema เป็น v7 อัตโนมัติ

### 4.7 BMM350 compile ผ่านแต่อ่านค่าเป็นขยะ (Windows)

สาเหตุ: BMM350 SensorAPI มี upstream bug ที่ต้อง patch ก่อน compile. Master Makefile **ไม่มี bash prebuild** เพื่อรองรับ Windows — ต้อง vendor pre-patched source

แก้: copy pre-patched `bmm350.c` ลง `episodes/int_ep04_bmm350_compass/app_sensor/bmm350/` (หรือ edit `mtb_shared/BMM350_SensorAPI/bmm350.c` โดยตรงก่อน build)

---

## 5. Build System Hooks (สำหรับ advanced customization)

### 5.1 CY_IGNORE — ไม่ compile ไฟล์ / package

```makefile
# proj_cm55/Makefile
CY_IGNORE += $(SEARCH_lvgl)/tests
CY_IGNORE += ./legacy_code
CY_IGNORE += ../../episodes              # master's default (episodes catalog ไม่ถูก compile)
CY_IGNORE += $(SEARCH_secure-sockets)    # master's default (ไม่ใช้ TLS socket)
```

### 5.2 PREBUILD / POSTBUILD

```makefile
# proj_cm55/Makefile
PREBUILD=python3 tools/generate_version.py
POSTBUILD=cp build/*/proj_cm55.hex ~/flash_ready/
```

**คำเตือน**: Master ตั้ง `PREBUILD=` (empty) เพื่อรองรับ Windows. Episode ที่ต้องการ prebuild ให้ vendor pre-processed source แทน

### 5.3 DEFINES — compile-time config

```makefile
# proj_cm55/Makefile
DEFINES += MY_FEATURE_ENABLED MY_VERSION=\"1.0\"
```

### 5.4 Per-project override

Master มี 3 projects: `proj_cm33_s`, `proj_cm33_ns`, `proj_cm55`. ส่วนมาก developer แก้แค่ `proj_cm55/Makefile`. การแก้ CM33 Makefile ต้องระวังเรื่อง secure boot chain — ปกติไม่ต้องแตะ

---

## 6. Known-Good Configurations (verified by build test)

| Item | Value |
| --- | --- |
| Host OS | macOS (tested on Darwin 25.5) |
| ModusToolbox | 3.6 |
| GCC ARM EABI | 14.2.1 (`/Applications/mtb-gcc-arm-eabi/14.2.1/`) |
| GNU Make | 4.4.1 (`gmake` from Homebrew — macOS system `make` 3.81 ไม่พอ) |
| TARGET | `APP_KIT_PSE84_AI` (BSP schema `cydesignfile_v6`) |
| CONFIG_DISPLAY | `W4P3INCH_DISP` default (ทดสอบ build ผ่านทั้ง 14 eps) |
| TOOLCHAIN | `GCC_ARM` |
| CONFIG | `Debug` |

**Build metrics** (จาก 14-episode regression test):
- First full build: ~30 วินาที
- Incremental (apps/ swap): 4-36 วินาที
- ELF size range: 5.0-8.3 MB (WiFi-heavy episodes ใหญ่กว่า)

---

## 7. Related Reading

- ../README.md — Master template overview + Quick Start + Capability Matrix
- ../proj_cm55/apps/README.md — Episode slot contract + install/switch instructions
- ../../episodes/README.md — All 14 episode catalogue

## Document Ends

This is a living document. Update เมื่อมี extension recipe ใหม่ที่ใช้ซ้ำได้หรือเจอ pitfall ใหม่ที่ควรบันทึก
