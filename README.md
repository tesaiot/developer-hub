# TESAIoT Dev Kit — Episodes Catalogue

<p align="center">
  <strong>ชุดตัวอย่างการพัฒนา AIoT บน PSoC Edge E84</strong><br>
  ส่วนหนึ่งของหลักสูตร <strong>TESAIoT Firmware Stack</strong> ภายใต้<br>
  <a href="https://www.tesaiot.dev"><strong>TESAIoT Foundation Platform</strong></a><br>
  โดย <strong>สมาคมสมองกลฝังตัวไทย (TESA)</strong>
</p>

---

## 📖 เกี่ยวกับชุดตัวอย่างนี้

Episodes Catalogue เป็นคอลเลกชันของ **ตัวอย่างโค้ดพร้อมใช้งานจริง** จำนวน 14 ตัวอย่าง
ที่ถูกออกแบบมาเพื่อถ่ายทอดเทคนิคการพัฒนา AIoT ทีละหัวข้ออย่างเป็นลำดับ —
ตั้งแต่การสร้าง UI พื้นฐานจนถึงการรวมระบบ sensor + WiFi + audio เข้าด้วยกัน

แต่ละ episode เป็น **ชุดไฟล์ drop-in** ที่นักพัฒนาสามารถวางลงใน
`tesaiot_dev_kit_master` แล้ว build + flash ได้ทันที
โดยไม่ต้องแก้ไข main project หรือ build configuration ใด ๆ

**เอกสารประกอบของแต่ละ episode ครอบคลุม 3 ด้านสำคัญ:**

- **Why** — เหตุผลและเป้าหมายของบทเรียน: ทำไมต้องศึกษาหัวข้อนี้ และจะนำไปประยุกต์ใช้ได้อย่างไร
- **What** — สิ่งที่ตัวอย่างแสดง: โครงสร้าง UI, data flow, widgets และ behaviors ที่เกิดขึ้นบนหน้าจอ
- **How** — วิธีการทำงานในระดับโค้ด: การเรียก API, sequence ของการ initialize, event handling

---

## 📚 Series 1 — HMI Menu & Setting

เน้นการสร้าง **Human-Machine Interface** ด้วย LVGL พร้อมการเชื่อมต่อ WiFi และการจัดเก็บค่าใน NVM

| # | Episode | ระดับ | หัวข้อ |
| :---: | --- | :---: | --- |
| 01 | hmi_ep01_basic_label | 🟢 Beginner | LVGL label + image — หน้าจอ LVGL ตัวแรก |
| 02 | hmi_ep02_button_event | 🟢 Beginner | Button widget + event callback + counter logic |
| 03 | hmi_ep03_text_input_keyboard | 🟢 Beginner | Textarea + keyboard + input mode switching |
| 04 | hmi_ep04_menu_navigation | 🟢 Beginner | Header + tabs + sidebar + page routing |
| 05 | hmi_ep05_wifi_list | 🟡 Intermediate | WiFi scan + dynamic AP list rendering |
| 06 | hmi_ep06_wifi_profile_nvm | 🟡 Intermediate | WiFi profile form + NVM persistence (save/load/clear) |
| 07 | hmi_ep07_final_wifi_manager | 🟡 Intermediate | Full WiFi Manager — connect / disconnect / retry / auto-connect |

## 📚 Series 2 — Interactive with TESAIoT Dev Kit

เน้นการอ่านและนำเสนอข้อมูลจาก **sensors + audio** บน dev kit

| # | Episode | ระดับ | หัวข้อ |
| :---: | --- | :---: | --- |
| 01 | int_ep01_dps368_monitor | 🟢 Beginner | DPS368 — barometric pressure + temperature monitor |
| 02 | int_ep02_bmi270_motion_visual | 🟢 Beginner | BMI270 — accelerometer/gyroscope + trend chart |
| 03 | int_ep03_sht40_indicator | 🟢 Beginner | SHT4x — temperature/humidity + comfort zone indicator |
| 04 | int_ep04_bmm350_compass | 🟡 Intermediate | BMM350 — compass dial + XYZ visualization + calibration |
| 05 | int_ep05_bmi270_radar_view | 🟡 Intermediate | BMI270 — radar-style motion visualization + intensity rings |
| 06 | int_ep06_digital_mic_probe | 🟡 Intermediate | Digital PDM stereo microphone + level meter + peak hold |
| 07 | int_ep07_sensorhub_final | 🔴 Advanced | Multi-sensor dashboard — DPS368 + SHT4x + BMI270 + BMM350 + PDM mic |

---

## 🧱 โครงสร้างมาตรฐานของ Episode

ทุก episode มีไฟล์พื้นฐานอย่างน้อย 3 ไฟล์ พร้อมโค้ดเฉพาะตัวอย่างนั้น:

```
<episode_name>/
├── main_example.c          ← implement void example_main(lv_obj_t *parent)
├── metadata.json           ← metadata สำหรับ TESAIoT Foundation Platform
├── README.md               ← เอกสารภาษาไทย (Why / What / How)
└── <source files>          ← โค้ดเฉพาะของ episode (.c / .h / subfolders)
```

### Episode Entry Contract

Master project template จะเรียก `example_main()` เพียงครั้งเดียวหลังจาก initialize
ระบบทุกอย่างเรียบร้อยแล้ว:

```c
// ใน tesaiot_dev_kit_master/proj_cm55/main.c
lv_init();
lv_port_disp_init();
lv_port_indev_init();
example_main(lv_scr_act());   /* 🎯 handoff to episode */
```

Episode เขียน `main_example.c` ที่ forward เข้าสู่โค้ดของตัวอย่าง:

```c
#include "app_interface.h"
#include "my_episode_code.h"

void example_main(lv_obj_t *parent)
{
    (void)parent;
    my_episode_create();
}
```

เมื่อ `apps/` ยังว่างเปล่า master จะใช้ **weak fallback** แสดงหน้าจอแนะนำ
ให้ดาวน์โหลด episode จาก https://www.tesaiot.dev

---

## 🛠️ Hardware Resources ที่ใช้ร่วมกัน

Master project template เตรียม hardware resources ให้พร้อมใช้ — episode สามารถเรียกใช้
ได้ทันทีโดยไม่ต้อง init เอง:

| Resource | การเข้าถึงจาก episode |
| --- | --- |
| **LVGL screen + display + touch** | `parent = lv_scr_act()` ที่ master ส่งเข้ามา |
| **Sensor I2C bus** (DPS368, SHT4x, BMI270 — 1.8V domain) | `#include "sensor_bus.h"` → `sensor_i2c_controller_hal_obj` |
| **Sensor I3C bus** (BMM350 magnetometer) | `#include "sensor_bus.h"` → `CYBSP_I3C_CONTROLLER_HW` / `_context` |
| **WiFi middleware** (cy_wcm + lwIP + mbedTLS) | link-ready — เรียก `cy_wcm_*` API ได้ทันที |
| **FreeRTOS** | link-ready — `xTaskCreate`, mutex, semaphore, queue พร้อมใช้ |
| **Debug UART** (`printf`) | link-ready — เรียก `printf()` ได้ทันที |
| **Shared assets** (`app_logo.h`) | `#include "app_logo.h"` — resolve ไปที่ master's copy อัตโนมัติ |

---

## 🚀 วิธีติดตั้ง Episode

### แบบที่ 1 — ดาวน์โหลดผ่าน TESAIoT Foundation Platform (แนะนำ)

1. เข้าไปที่ https://www.tesaiot.dev
2. เลือก episode ที่ต้องการศึกษา
3. กดปุ่ม **Download .zip**
4. แตกไฟล์ลงใน `tesaiot_dev_kit_master/proj_cm55/apps/`

### แบบที่ 2 — Copy จาก local repository

```sh
cd tesaiot_dev_kit_master

# ลบ episode เก่าออก (เก็บไฟล์ระบบ app_interface.h, README.md, _default/)
find proj_cm55/apps -mindepth 1 -maxdepth 1 \
     ! -name 'app_interface.h' ! -name 'README.md' ! -name '_default' \
     -exec rm -rf {} +

# วาง episode ที่ต้องการ
rsync -a ../episodes/hmi_ep04_menu_navigation/ proj_cm55/apps/

# Build + flash
make clean
make program
```

**ระหว่างการติดตั้ง episode นักพัฒนาไม่ต้องแก้ไขไฟล์ใด ๆ ใน master project
ทั้ง `main.c`, `Makefile`, BSP config หรือ build configuration**

---

## 🔄 การสลับระหว่าง Episode

การเปลี่ยนจาก episode หนึ่งไปยังอีก episode หนึ่งทำได้เสมอ โดยไม่มี state คงค้าง
จาก episode ก่อนหน้า:

```sh
# 1. ลบ episode ปัจจุบัน
find proj_cm55/apps -mindepth 1 -maxdepth 1 \
     ! -name 'app_interface.h' ! -name 'README.md' ! -name '_default' \
     -exec rm -rf {} +

# 2. วาง episode ใหม่
rsync -a ../episodes/<next_episode>/ proj_cm55/apps/

# 3. Build + flash (incremental — ใช้เวลาไม่กี่วินาที)
make build && make program
```

---

## 📋 Metadata Schema

ทุก episode มีไฟล์ `metadata.json` สำหรับการจัด catalog และ discovery บน
TESAIoT Foundation Platform:

```json
{
  "title": "EP04 — Menu Navigation",
  "description": "คำอธิบายสั้นเป็นภาษาไทย",
  "series": "hmi_menu_setting",
  "episode_order": 4,
  "difficulty": "beginner",
  "domain": "UI Navigation",
  "tags": ["lvgl", "menu", "tabs", "navigation"],
  "boards": ["KIT_PSE84_AI", "KIT_PSE84_EVAL_EPC2"],
  "files": ["main_example.c", "nav/ui_menu_navigation.c", "..."],
  "requires_sensors": false,
  "requires_wifi": false,
  "requires_audio": false,
  "upstream_repo": "tesaiot-platform/...",
  "upstream_branch": "ep04-menu-navigation"
}
```

---

## 🔗 ทรัพยากรเพิ่มเติม

- **🌐 TESAIoT Foundation Platform**: <https://www.tesaiot.dev>
  แหล่งรวม catalog ของ episodes, เอกสารประกอบ, และชุมชนนักพัฒนา
- **📘 Master Project Template**: `../tesaiot_dev_kit_master/README.md`
  รายละเอียดของ master template และ Core TESAIoT Firmware Stack ที่ episode ทั้งหมดใช้
- **🔧 Extension Guide**: `../tesaiot_dev_kit_master/docs/EXTENDING.md`
  คู่มือสำหรับนักพัฒนาที่ต้องการสร้าง episode ใหม่หรือต่อยอด master template

---

## 🏛️ Credit

**TESAIoT Firmware Stack** เป็นส่วนหนึ่งของ **TESAIoT Foundation Platform** พัฒนาและดูแลโดย:

<p align="center">
  <strong>สมาคมสมองกลฝังตัวไทย</strong><br>
  Thai Embedded Systems Association (TESA)<br>
  <a href="https://www.tesaiot.dev">www.tesaiot.dev</a>
</p>

---

## 📝 License

**MIT License** — Contributed by **สมาคมสมองกลฝังตัวไทย (Thai Embedded Systems Association — TESA)**

ดูรายละเอียดใน `../tesaiot_dev_kit_master/LICENSE`

Episodes ในชุดนี้รวมส่วนประกอบ open-source จาก Infineon ModusToolbox, LVGL,
FreeRTOS, Bosch Sensortec (BMI270/BMM350), Sensirion (SHT4x), และ Infineon
WiFi Host Driver — โดย license ของแต่ละส่วนเป็นไปตามต้นทาง
