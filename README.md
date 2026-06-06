# TESAIoT Dev Kit — Master Project Template

<p align="center">
  <strong>ฐานรากสำหรับการพัฒนา AIoT บน PSoC Edge E84</strong><br>
  ส่วนหนึ่งของหลักสูตร <strong>TESAIoT Firmware Stack</strong> ภายใต้<br>
  <a href="https://www.tesaiot.dev"><strong>TESAIoT Foundation Platform</strong></a><br>
  โดย <strong>สมาคมสมองกลฝังตัวไทย (TESA)</strong>
</p>

---

## 🎓 เกี่ยวกับหลักสูตรนี้

**สมาคมสมองกลฝังตัวไทย (TESA)** จัดทำ **TESAIoT Firmware Stack** ขึ้นเพื่อถ่ายทอดองค์ความรู้
ในการพัฒนา Artificial Intelligence of Things (AIoT) ให้กับนักพัฒนาชาวไทย โดยใช้บอร์ด
**Infineon PSoC Edge E84** — microcontroller สถาปัตยกรรม Arm Cortex-M55 + Cortex-M33
ที่มาพร้อม GPU แบบ VGLite สำหรับ UI graphics ความเร็วสูง

หลักสูตรนี้เป็นส่วนหนึ่งของ **TESAIoT Foundation Platform** (https://www.tesaiot.dev)
ซึ่งรวมแหล่งเรียนรู้ catalog ตัวอย่าง เอกสารประกอบ และช่องทางดาวน์โหลดโค้ดสำหรับ
นักพัฒนาทุกระดับ

**นักพัฒนาจะได้เรียนรู้ผ่านตัวอย่างที่ใช้งานได้จริงบนบอร์ด:**
- การสร้าง HMI (Human-Machine Interface) ด้วย LVGL บนจอสัมผัส
- การอ่าน sensor หลายชนิด (pressure, humidity, motion, magnetometer, microphone)
- การเชื่อมต่อ WiFi พร้อม reconnect logic และ credential persistence
- การออกแบบ application แบบ task-based ด้วย FreeRTOS

## 🧰 เกี่ยวกับ Master Project Template

Master Project Template นี้คือ **โปรเจกต์ตั้งต้นที่ตั้งค่าทุกอย่างให้เรียบร้อยแล้ว** นักพัฒนาเพียงแค่:

1. Clone โปรเจกต์นี้ครั้งเดียว
2. ดาวน์โหลดไฟล์ตัวอย่าง (episode) จาก https://www.tesaiot.dev
3. วางไฟล์ของ episode ลงในโฟลเดอร์ `proj_cm55/apps/`
4. Build แล้ว flash ลงบอร์ด

**ไม่ต้องแก้ไฟล์หลัก ไม่ต้อง config อะไรเพิ่ม** — ทุก subsystem ของ Core Firmware Stack
(จอแสดงผล, ระบบสัมผัส, sensor buses, WiFi stack, audio, FreeRTOS, LVGL graphics engine)
ถูก initialize เตรียมให้พร้อมใช้ในทุก episode

---

## 🧭 สิ่งที่ Core TESAIoT Firmware Stack เตรียมให้พร้อมใช้

เมื่อบอร์ด boot ขึ้นมา ระบบจะเตรียมทรัพยากรทั้งหมดนี้ให้ทำงานแล้ว — episode ของนักพัฒนา
เพียงแค่ **เรียกใช้ API** โดยไม่ต้อง init hardware เอง

### 🖼️ ระบบแสดงผล (Graphics + Display)

| Capability | รายละเอียดที่นักพัฒนาใช้ได้ |
| --- | --- |
| **LVGL 9** graphics library | Widget ครบชุด (label, button, textarea, keyboard, chart, slider, arc, meter, tabview, list, image, ...) พร้อม flex/grid layout, animations, events, styles |
| **VGLite GPU** acceleration | การวาดทุกอย่างถูกเร่งความเร็วด้วย GPU ฮาร์ดแวร์ของ PSoC Edge — นักพัฒนาไม่ต้องเขียน GPU code เอง |
| **Display drivers** 3 แบบ | เลือกได้ตามบอร์ดที่ใช้ (4.3", 7", หรือ 10.1") — ดู section "Hardware ที่รองรับ" |
| **Fonts** | Montserrat ขนาด 12, 14, 16, 18, 20, 22, 24, 28, 30, 40 พร้อมใช้ (ไม่ต้องเปิดใน config เอง) |

**นักพัฒนาเขียนแค่:**
```c
lv_obj_t *label = lv_label_create(parent);
lv_label_set_text(label, "สวัสดี TESAIoT");
lv_obj_center(label);
```

### 👆 ระบบสัมผัส (Touch Input)

Touch controller (GT911 / FT5406 / ILI2511) ถูกติดตั้งและเชื่อมกับ LVGL ให้แล้ว
event จากการแตะหน้าจอจะไปถึง widget โดยอัตโนมัติ

**นักพัฒนาเขียนแค่:**
```c
lv_obj_add_event_cb(my_button, on_clicked, LV_EVENT_CLICKED, NULL);
```

### 🌡️ Sensor Bus (I2C + I3C)

| Bus | รองรับ Sensor | Middleware ที่พร้อมใช้ |
| --- | --- | --- |
| **Sensor I2C** (1.8V domain) | DPS368 (ความดัน), SHT4x (อุณหภูมิ/ความชื้น), BMI270 (IMU 6 แกน) | Infineon DPS3xx, Sensirion SHT4x, Bosch BMI270 SensorAPI |
| **Sensor I3C** | BMM350 (magnetometer/เข็มทิศ) | Bosch BMM350 SensorAPI |

**นักพัฒนาเขียนแค่:**
```c
#include "sensor_bus.h"
#include "dps368/dps368_presenter.h"

void example_main(lv_obj_t *parent) {
    dps368_presenter_start(&sensor_i2c_controller_hal_obj);
}
```
Sensor bus ถูก init ไว้แล้ว — แค่ส่ง handle ให้ presenter ของ episode

### 🎤 ระบบเสียง (Digital Microphone)

PDM stereo microphone เชื่อมต่อผ่าน Cypress PDL — นักพัฒนาสามารถอ่าน audio frames
แบบ stereo (left/right channel) ได้ทันที

### 📡 ระบบเครือข่ายไร้สาย (WiFi + TCP/IP + TLS)

ชุด middleware ของ Infineon WiFi Connection Manager (cy_wcm) ถูก link เข้า master
พร้อมใช้งาน ครอบคลุม:
- **WiFi radio driver** (WHD + CYW55500)
- **cy_wcm API** สำหรับ scan / connect / disconnect
- **lwIP** — TCP/IP stack
- **mbedTLS** (with hardware acceleration) — TLS/SSL/crypto
- **WPA3** authentication

**นักพัฒนาเขียนแค่:**
```c
cy_wcm_init(&config);
cy_wcm_scan(&scan_filter, on_scan_result, NULL);
cy_wcm_connect_ap(&params, &ip_info);
```

### ⚙️ ระบบจัดการงาน (FreeRTOS)

FreeRTOS kernel ถูกเตรียมให้พร้อมใช้: task scheduling, mutex, semaphore, queue, timer
พร้อม **tickless idle** สำหรับประหยัดพลังงาน — นักพัฒนาสามารถสร้าง task เสริมได้ตามต้องการ

### 🔍 Debug & Logging

`printf()` ไปยัง debug UART (115200 baud) ถูกเปิดใช้งาน — นักพัฒนาสามารถดู log
ผ่าน serial monitor ได้ทันทีเพื่อ debug และตรวจสอบการทำงานของโปรแกรม

---

## 🛠️ Hardware ที่รองรับ

### Board

- **Infineon KIT_PSE84_AI** (PSoC Edge E84 AI Development Kit) — recommended ⭐

### Display

เลือกได้ 3 แบบโดยการตั้งค่า `CONFIG_DISPLAY` ใน `common.mk`:

| `CONFIG_DISPLAY` | Panel | ขนาด | Touch Controller |
| --- | --- | --- | --- |
| `W4P3INCH_DISP` ⭐ **default** | Waveshare 4.3" DSI | 480×800 | FT5406 |
| `WS7P0DSI_RPI_DISP` | Waveshare 7" DSI | 1024×600 | GT911 |
| `WF101JTYAHMNB0_DISP` | EK79007AD3 10.1" DSI | 1280×800 | ILI2511 |

### Tool chain

- **ModusToolbox 3.6** ขึ้นไป (https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
- **GCC ARM Embedded** 14.2.1 (มากับ ModusToolbox)
- **KitProg3** สำหรับ flash และ debug (มากับบอร์ด)

---

## 🚀 Quick Start

### 1. Clone และเตรียม dependencies

```sh
git clone https://github.com/TESA-AIoT-Platform/tesaiot_dev_kit_master.git
cd tesaiot_dev_kit_master

# ดึง libraries (ประมาณ 1 นาที, ขนาด ~469 MB — ทำครั้งเดียว)
make getlibs
```

### 2. เลือกและติดตั้ง Episode ที่ต้องการศึกษา

เข้าไปที่ https://www.tesaiot.dev เลือก episode ที่ต้องการ แล้วดาวน์โหลดหรือใช้
folder ที่ checkout ไว้แล้ว. ใช้ helper script `tools/install_episode.sh`
ซึ่งจะทำ **clean + install + invalidate build cache** ให้อัตโนมัติในคำสั่งเดียว:

```sh
# ติดตั้ง episode จาก folder (เช่น sibling episodes/ repo)
tools/install_episode.sh ../episodes/hmi_ep01_basic_label

# หรือจาก path ที่ unzip ไว้
unzip ~/Downloads/hmi_ep01_basic_label.zip -d /tmp/
tools/install_episode.sh /tmp/hmi_ep01_basic_label
```

สิ่งที่ script ทำให้อัตโนมัติ:
1. ลบไฟล์ episode เก่าใน `proj_cm55/apps/` (เก็บ `app_interface.h` + `_default/` ไว้)
2. rsync ไฟล์ของ episode ใหม่เข้าไป
3. ล้าง build cache ของ `apps/` เพื่อ rebuild สะอาด

> ⚠️ **อย่าลบ `proj_cm55/apps/app_interface.h` หรือ `proj_cm55/apps/_default/`** —
> เป็นไฟล์ system ของ master template ที่ทุก episode ต้องพึ่ง. รายละเอียดอยู่ใน
> comment block ของ `app_interface.h`

### 3. Build แล้ว Flash

```sh
make build      # ประมาณ 30 วินาที
make program    # flash ลงบอร์ดผ่าน KitProg3
```

### 4. เปลี่ยน / ย้อน Episode

```sh
# เปลี่ยนเป็น episode อื่น — script จัดการ clean + install ให้
tools/install_episode.sh ../episodes/int_ep07_sensorhub_final
make build && make program

# หรือล้าง episode กลับเป็น default splash screen
tools/clean_episode.sh
make build && make program
```

### 5. ดูผลบนจอของบอร์ดและ serial log

เปิด serial monitor ที่ 115200 baud เพื่อดู debug output:
- macOS: `screen /dev/cu.usbmodem* 115200`
- Linux: `screen /dev/ttyACM0 115200`
- Windows: ใช้ PuTTY หรือ TeraTerm เปิด COM port

---

## 📚 Curriculum — 14 Episodes ที่มีให้ศึกษา

หลักสูตรแบ่งเป็น 2 ซีรีส์หลัก ศึกษาคู่ขนานกันได้ตามความสนใจ:

### 🔵 Series 1 — HMI Menu & Setting (UI + WiFi)

เน้นการสร้าง Graphical User Interface และการเชื่อมต่อ WiFi

| # | Episode | ระดับ | สิ่งที่จะได้เรียนรู้ |
| --- | --- | --- | --- |
| 01 | `hmi_ep01_basic_label` | 🟢 Beginner | LVGL label + image — หน้าจอ LVGL ตัวแรก |
| 02 | `hmi_ep02_button_event` | 🟢 Beginner | Button widget + event callback + counter logic |
| 03 | `hmi_ep03_text_input_keyboard` | 🟢 Beginner | Textarea + keyboard + input mode switching |
| 04 | `hmi_ep04_menu_navigation` | 🟢 Beginner | Menu shell + tabs + page routing |
| 05 | `hmi_ep05_wifi_list` | 🟡 Intermediate | WiFi scan + AP list + signal strength |
| 06 | `hmi_ep06_wifi_profile_nvm` | 🟡 Intermediate | WiFi profile form + NVM persistence (save/load/clear) |
| 07 | `hmi_ep07_final_wifi_manager` | 🟡 Intermediate | Full WiFi Manager (connect/disconnect/retry/auto-connect) |

### 🟠 Series 2 — Interactive with TESAIoT Dev Kit (Sensors + Audio)

เน้นการอ่านและนำเสนอข้อมูลจากเซ็นเซอร์ + ไมโครโฟนดิจิทัล

| # | Episode | ระดับ | สิ่งที่จะได้เรียนรู้ |
| --- | --- | --- | --- |
| 01 | `int_ep01_dps368_monitor` | 🟢 Beginner | DPS368 — อ่านความดันและอุณหภูมิ |
| 02 | `int_ep02_bmi270_motion_visual` | 🟢 Beginner | BMI270 — accel/gyro + trend chart |
| 03 | `int_ep03_sht40_indicator` | 🟢 Beginner | SHT4x — อุณหภูมิ/ความชื้น + comfort zone |
| 04 | `int_ep04_bmm350_compass` | 🟡 Intermediate | BMM350 — เข็มทิศ + manual calibration |
| 05 | `int_ep05_bmi270_radar_view` | 🟡 Intermediate | BMI270 — radar-style motion visualization |
| 06 | `int_ep06_digital_mic_probe` | 🟡 Intermediate | PDM stereo mic + level meter + peak hold |
| 07 | `int_ep07_sensorhub_final` | 🔴 Advanced | Multi-sensor dashboard — รวม 4 sensors + PDM mic |

### เส้นทางการเรียนรู้แนะนำ

- **เริ่มต้น** → HMI ep01 → ep02 → ep03 → ep04 (ศึกษาพื้นฐาน LVGL)
- **เซ็นเซอร์** → INT ep01 → ep02 → ep03 → ep04 → ep05 (ศึกษาการอ่าน sensor ทีละตัว)
- **การเชื่อมต่อ** → HMI ep05 → ep06 → ep07 (ศึกษา WiFi ทีละขั้น)
- **เสียง** → INT ep06 (PDM mic)
- **รวมทุกอย่าง** → INT ep07 (sensor dashboard) หรือ HMI ep07 (WiFi Manager)

---

## 🔄 การสลับระหว่าง Episode

เมื่อศึกษา episode หนึ่งเสร็จแล้วและอยากศึกษา episode ถัดไป:

```sh
# 1. ลบ episode เก่าออก (เก็บไฟล์ระบบไว้)
find proj_cm55/apps -mindepth 1 -maxdepth 1 \
     ! -name 'app_interface.h' ! -name 'README.md' ! -name '_default' \
     -exec rm -rf {} +

# 2. วาง episode ใหม่
unzip ~/Downloads/<next_episode>.zip -d proj_cm55/apps/

# 3. Build ใหม่ (incremental — ใช้เวลาแค่ไม่กี่วินาที)
make build
make program
```

**ไม่ต้องแก้ `main.c` หรือ `Makefile`** — ระบบของ Master Template จะรับรู้ episode ใหม่โดยอัตโนมัติ

---

## 📂 โครงสร้างของ Master Template

```
tesaiot_dev_kit_master/
├── proj_cm55/              ← โปรเจกต์หลัก (Cortex-M55, UI + sensors + WiFi)
│   ├── main.c              ← ไฟล์หลัก (ไม่ต้องแก้) — เตรียมทุก subsystem แล้วเรียก episode
│   ├── apps/               ← ⭐ ตำแหน่งสำหรับวาง episode
│   │   ├── app_interface.h      (สัญญาของ episode)
│   │   ├── README.md
│   │   └── _default/            (หน้าจอเริ่มต้นก่อนติดตั้ง episode)
│   ├── app_assets/         ← โลโก้ + asset ที่ทุก episode ใช้ร่วมกัน
│   ├── platform/           ← retarget_io, sensor_bus
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
└── docs/
    └── EXTENDING.md        ← สำหรับนักพัฒนาที่ต้องการต่อยอดหรือปรับแต่ง master ระดับลึก
```

---

## 🔗 ทรัพยากรเพิ่มเติม

- **🌐 TESAIoT Foundation Platform**: <https://www.tesaiot.dev>
  แหล่งรวม catalog ของ episodes, เอกสารประกอบ, วิดีโอสอน, และชุมชนนักพัฒนา
- **📘 Episodes Catalogue**: `../episodes/README.md`
  รายละเอียดของทั้ง 14 episodes ภาษาไทย (Why / What / How)
- **🔧 สำหรับการต่อยอดระดับลึก**: `docs/EXTENDING.md`
  คู่มือการเพิ่ม episode ใหม่ เพิ่ม sensor/peripheral ใน master และการปรับแต่ง build system
- **📖 LVGL Documentation**: <https://docs.lvgl.io/9.x/>
- **📖 ModusToolbox User Guide**: <https://www.infineon.com/modustoolbox>

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

ดูรายละเอียดใน `LICENSE`

Master template นี้รวมส่วนประกอบ open-source จาก:
- Infineon ModusToolbox BSP + middleware
- LVGL Project
- FreeRTOS
- Bosch Sensortec BMI270 + BMM350 SensorAPI
- Sensirion SHT4x driver
- Infineon WiFi Host Driver + mbedTLS

โดย license ของแต่ละส่วนเป็นไปตามต้นทาง
