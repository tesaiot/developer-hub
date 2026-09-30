# PSoC Edge E84 Reference Examples — C / LVGL

ชุดตัวอย่างโค้ด C สำหรับบอร์ด **PSoC Edge E84** รวม **76 ตัวอย่าง** จัดเป็นสามระดับ
ตั้งแต่วิดเจ็ต LVGL ตัวแรก ไปจนถึง WiFi, OPTIGA Trust M, เกม และแอปพลิเคชันหลายหน้าจอ
ทุกตัวอย่างใช้ LVGL บนคอร์ Cortex-M55 และเรียก API ฮาร์ดแวร์จริงผ่าน `pse84_common.h`

English summary: [below](#english)

| ระดับ | จำนวน | คำนำหน้า |
|-------|-------|----------|
| [Beginner](#beginner-18) | 18 | `b01`–`b18` |
| [Intermediate](#intermediate-34) | 34 | `i01`–`i34` |
| [Advanced](#advanced-24) | 24 | `a01`–`a24` |
| **รวม** | **76** | |

---

## Build ด้วย SDK template

ตัวอย่างเหล่านี้**ไม่ใช่โปรเจกต์ที่ build ได้เดี่ยว ๆ** แต่ละตัวอย่างเป็นชุดไฟล์ drop-in
ที่ต้อง build ภายในโปรเจกต์เฟิร์มแวร์ PSoC Edge E84 ให้ใช้ SDK template ของ BENTO/TESAIoT จาก
[tesaiot/tesaiot-pse84-devkit-sdk](https://github.com/tesaiot/tesaiot-pse84-devkit-sdk)
branch นี้ไม่มี project template และไม่มีไลบรารีที่คอมไพล์ไว้แล้ว

1. **เตรียม SDK template** — clone
   [tesaiot/tesaiot-pse84-devkit-sdk](https://github.com/tesaiot/tesaiot-pse84-devkit-sdk)
   แล้วทำตาม README ของ template จน template ที่ยังไม่แก้ไข build และรันบนบอร์ดได้
   template มีเฮดเดอร์ที่ตัวอย่าง include นอกเหนือจาก `pse84_common.h` เช่น `ipc_communication.h`,
   `ipc_sensorhub.h`, `wifi_manager.h`, `usb_hid_joystick.h`, `radar_task.h` และ `lv_fonts_thai.h`
2. **เพิ่มตัวอย่างทีละหนึ่งตัว** — คัดลอกไฟล์ของตัวอย่างพร้อม `pse84_common.h` ไปไว้ในโฟลเดอร์ซอร์สของโปรเจกต์ CM55
   (README ของแต่ละตัวอย่างใช้ `proj_cm55/app/`) ทุกตัวอย่างประกาศ `example_main()`
   ถ้าใส่สองตัวอย่างใน build เดียวกันจะ link ไม่ผ่าน
3. **เรียกจุดเริ่มต้น** — SDK template ไม่ได้เรียก `example_main()` ให้เอง ให้เรียกหนึ่งครั้งจาก graphics task ของ CM55
   หลังจาก LVGL, จอ และระบบสัมผัสพร้อมแล้ว โดยส่งออบเจกต์ LVGL เต็มจอ (800 × 480)

   ```c
   #include "pse84_common.h"

   example_main(lv_screen_active());
   ```

   เรียก LVGL จาก graphics task เท่านั้น
4. **ตรวจ board flags** — `pse84_common.h` ตั้งค่า `BSP_HAS_*` ที่โปรเจกต์ไม่ได้กำหนดเป็น `0`
   ซึ่งจะตัดโค้ดของฮาร์ดแวร์นั้นออก ตรวจว่าโปรเจกต์กำหนด flags ของบอร์ดไว้ครบ
   (`BSP_HAS_DPS368`, `BSP_HAS_SHT40`, `BSP_HAS_CAPSENSE`, `BSP_HAS_POTENTIOMETER`,
   `BSP_HAS_BMI270`, `BSP_HAS_BMM350`) มิฉะนั้นตัวอย่างเซนเซอร์จะไม่แสดงค่า
5. **Build และ flash** ด้วยคำสั่งตาม README ของ SDK template (`make build` แล้ว `make program`)

ตัวอย่างที่สื่อสารกับคอร์ Cortex-M33 ผ่าน IPC ต้องพึ่ง IPC handler ของเฟิร์มแวร์ที่นำไป build ด้วย

> **Eva Kit:** SDK template ในตอนนี้มี board support package สำหรับ `KIT_PSE84_AI` เท่านั้น
> ไม่มี BSP ของ Eva Kit (`KIT_PSE84_EVAL_EPC2`) ตัวอย่างที่ต้องใช้ Eva Kit จึงต้องใช้ BSP ของ Eva Kit เพิ่มเอง

---

## บอร์ดที่รองรับ

แต่ละตัวอย่างระบุบอร์ดที่รองรับไว้ในฟิลด์ `boards` ของ `metadata.json` คอลัมน์ "บอร์ด" ในตารางด้านล่างแสดงค่านั้น

| บอร์ด | ชื่อใน `metadata.json` | ความแตกต่างของฮาร์ดแวร์ที่มีผล |
|-------|-------------------------|--------------------------------|
| **AI Kit** | `KIT_PSE84_AI`, `ai_kit` | มีเซนเซอร์ DPS368, SHT40, เรดาร์ BGT60TR13C และพอร์ต USB host ไม่มี CapSense และโพเทนชิโอมิเตอร์ |
| **Eva Kit** | `KIT_PSE84_EVAL_EPC2`, `eva_kit` | มีปุ่มและสไลเดอร์ CapSense และโพเทนชิโอมิเตอร์ ไม่มี DPS368 และ SHT40 |
| **Game Console** | `KIT_PSE84_AI_GAME`, `game_console` | เฟิร์มแวร์ Game Console บน AI Kit |

- `b12`–`b14` และ `i31` ต้องใช้ DPS368/SHT40 (AI Kit)
- `b15`, `b16`, `i12`, `i13`, `i29`, `i30` ต้องใช้ CapSense หรือโพเทนชิโอมิเตอร์ (Eva Kit)
- `a18` ต้องใช้เรดาร์ (AI Kit)
- ตัวอย่างที่ชื่อขึ้นต้นด้วย **[UI Concept]** หรือ **[UI Reference]** (`a04`, `a07`, `a09`, `a11`, `a21`, `a22`)
  แสดงเฉพาะเลย์เอาต์หน้าจอ คำอธิบายใน `metadata.json` บอกว่าต้องเชื่อมฮาร์ดแวร์ส่วนใดเพิ่ม

**TESAIoT Dev Kit** ใช้โมดูล PSoC Edge E84 AI ตัวเดียวกับ AI Kit บนบอร์ดฐาน QWA309
README ของบางตัวอย่างกล่าวถึงบอร์ดนี้ แต่ไม่มี `metadata.json` ใดระบุไว้ จึงให้ถือว่ายังไม่ได้ทดสอบกับซอร์สใน branch นี้
ตัวอย่าง 19 ตัวจากชุดนี้ที่ปรับให้ใช้กับ Dev Kit แล้วอยู่ใน branch
[`tesaiot_dev_kit_practise_codes`](https://github.com/tesaiot/developer-hub/tree/tesaiot_dev_kit_practise_codes)

---

## รายการตัวอย่าง

### Beginner (18)

> พื้นฐาน — GPIO, เซนเซอร์ และวิดเจ็ต LVGL

| ตัวอย่าง | ชื่อ | บอร์ด |
|---|---|---|
| [`b01_widget_basics`](./beginner/b01_widget_basics/) | Widget Basics — Interactive Grid Menu | AI Kit, Eva Kit |
| [`b02_widget_applied`](./beginner/b02_widget_applied/) | Widget Applied — Sensors & Interactive Demos | AI Kit, Eva Kit |
| [`b03_layout_and_events`](./beginner/b03_layout_and_events/) | Layout & Events — Grid Menu | AI Kit, Eva Kit |
| [`b04_led_toggle`](./beginner/b04_led_toggle/) | LED Toggle | AI Kit, Eva Kit |
| [`b05_led_on_off`](./beginner/b05_led_on_off/) | Hardware LED On/Off | AI Kit, Eva Kit |
| [`b06_button_read`](./beginner/b06_button_read/) | Hardware Button Read | AI Kit, Eva Kit |
| [`b07_button_led_link`](./beginner/b07_button_led_link/) | Button-LED Link | AI Kit, Eva Kit |
| [`b08_multi_led_panel`](./beginner/b08_multi_led_panel/) | Multi LED Control Panel | AI Kit, Eva Kit |
| [`b09_gpio_status_bar`](./beginner/b09_gpio_status_bar/) | GPIO Status Bar | AI Kit, Eva Kit |
| [`b10_accel_xyz`](./beginner/b10_accel_xyz/) | Accelerometer XYZ | AI Kit, Eva Kit |
| [`b11_gyro_xyz`](./beginner/b11_gyro_xyz/) | Gyroscope XYZ | AI Kit, Eva Kit |
| [`b12_temperature`](./beginner/b12_temperature/) | Temperature Gauge | AI Kit |
| [`b13_pressure`](./beginner/b13_pressure/) | Pressure Bar | AI Kit |
| [`b14_humidity`](./beginner/b14_humidity/) | Humidity Gauge | AI Kit |
| [`b15_capsense_buttons`](./beginner/b15_capsense_buttons/) | CapSense Buttons | Eva Kit |
| [`b16_potentiometer`](./beginner/b16_potentiometer/) | Potentiometer Gauge | Eva Kit |
| [`b17_sensor_labels`](./beginner/b17_sensor_labels/) | All Sensors Dashboard | AI Kit, Eva Kit |
| [`b18_thai_text`](./beginner/b18_thai_text/) | Thai Text Display | AI Kit, Eva Kit |

### Intermediate (34)

> ระดับกลาง — แดชบอร์ด, กราฟ, การนำทางหลายหน้า และ IPC

| ตัวอย่าง | ชื่อ | บอร์ด |
|---|---|---|
| [`i01_sensor_dashboard`](./intermediate/i01_sensor_dashboard/) | Sensor Dashboard | AI Kit, Eva Kit |
| [`i02_line_chart_accel`](./intermediate/i02_line_chart_accel/) | Line Chart — Acceleration | AI Kit, Eva Kit |
| [`i03_bar_chart_multi`](./intermediate/i03_bar_chart_multi/) | Multi-Series Bar Chart | AI Kit, Eva Kit |
| [`i04_compass_rose`](./intermediate/i04_compass_rose/) | Compass Rose | AI Kit, Eva Kit |
| [`i05_wifi_scanner`](./intermediate/i05_wifi_scanner/) | WiFi Scanner | AI Kit, Eva Kit |
| [`i06_environ_monitor`](./intermediate/i06_environ_monitor/) | Environmental Monitor | AI Kit, Eva Kit |
| [`i07_motion_detector`](./intermediate/i07_motion_detector/) | Motion Detector | AI Kit, Eva Kit |
| [`i08_level_bubble`](./intermediate/i08_level_bubble/) | Bubble Level | AI Kit, Eva Kit |
| [`i09_data_logger`](./intermediate/i09_data_logger/) | Data Logger | AI Kit, Eva Kit |
| [`i10_gauge_cluster`](./intermediate/i10_gauge_cluster/) | Gauge Cluster | AI Kit, Eva Kit |
| [`i11_status_panel`](./intermediate/i11_status_panel/) | System Status Panel | AI Kit, Eva Kit |
| [`i12_capsense_viz`](./intermediate/i12_capsense_viz/) | CapSense Visualization | Eva Kit |
| [`i13_pot_control`](./intermediate/i13_pot_control/) | Potentiometer Control | Eva Kit |
| [`i14_auto_scroll_log`](./intermediate/i14_auto_scroll_log/) | Auto-Scrolling Console Log | AI Kit, Eva Kit |
| [`i15_tile_navigation`](./intermediate/i15_tile_navigation/) | Tile Navigation | AI Kit, Eva Kit |
| [`i16_tabbed_sensors`](./intermediate/i16_tabbed_sensors/) | Tabbed Sensors | AI Kit, Eva Kit |
| [`i17_basic_ipc`](./intermediate/i17_basic_ipc/) | Basic IPC | AI Kit, Eva Kit |
| [`i18_chart_statistics`](./intermediate/i18_chart_statistics/) | Chart Statistics | AI Kit, Eva Kit |
| [`i19_color_mixer`](./intermediate/i19_color_mixer/) | RGB Color Mixer | AI Kit, Eva Kit |
| [`i20_sensor_push`](./intermediate/i20_sensor_push/) | Sensor Push via IPC | AI Kit, Eva Kit |
| [`i21_led_ipc`](./intermediate/i21_led_ipc/) | LED Control via IPC | AI Kit, Eva Kit |
| [`i22_wifi_status_bar`](./intermediate/i22_wifi_status_bar/) | WiFi Status Bar | AI Kit, Eva Kit |
| [`i23_automation_rules`](./intermediate/i23_automation_rules/) | Automation Rules | AI Kit, Eva Kit |
| [`i24_request_response`](./intermediate/i24_request_response/) | IPC Request-Response | AI Kit, Eva Kit |
| [`i25_deferred_flag`](./intermediate/i25_deferred_flag/) | Deferred Flag Pattern | AI Kit, Eva Kit |
| [`i26_touch_pause_resume`](./intermediate/i26_touch_pause_resume/) | Touch Pause/Resume IPC | AI Kit, Eva Kit |
| [`i27_page_navigation`](./intermediate/i27_page_navigation/) | Page Navigation | AI Kit, Eva Kit |
| [`i28_multi_page_app`](./intermediate/i28_multi_page_app/) | Multi-Page Navigation App | AI Kit, Eva Kit |
| [`i29_controls_capsense`](./intermediate/i29_controls_capsense/) | CapSense Controls | Eva Kit |
| [`i30_controls_pot`](./intermediate/i30_controls_pot/) | Potentiometer Arc Gauge | Eva Kit |
| [`i31_environ_fusion`](./intermediate/i31_environ_fusion/) | Environment Sensor Fusion | AI Kit |
| [`i32_joystick_display`](./intermediate/i32_joystick_display/) | Joystick Visualization | AI Kit, Eva Kit, Game Console |
| [`i33_lcd_console`](./intermediate/i33_lcd_console/) | Rich Text LCD Console | AI Kit, Eva Kit, Game Console |
| [`i34_motion_tilt`](./intermediate/i34_motion_tilt/) | Motion Tilt Angles | AI Kit, Eva Kit |

### Advanced (24)

> ระดับสูง — WiFi, OPTIGA Trust M, UI กล้อง, เกม และแอปพลิเคชันครบวงจร

| ตัวอย่าง | ชื่อ | บอร์ด |
|---|---|---|
| [`a01_wifi_scan`](./advanced/a01_wifi_scan/) | WiFi Scan + Table Display | AI Kit, Eva Kit |
| [`a02_wifi_connect`](./advanced/a02_wifi_connect/) | WiFi Connect Flow | AI Kit, Eva Kit |
| [`a03_pin_manager`](./advanced/a03_pin_manager/) | PIN Manager | AI Kit, Eva Kit |
| [`a04_cert_viewer`](./advanced/a04_cert_viewer/) | [UI Reference] Certificate Viewer | AI Kit, Eva Kit |
| [`a05_optiga_uid`](./advanced/a05_optiga_uid/) | Security Info Display | AI Kit, Eva Kit |
| [`a06_sensor_fusion`](./advanced/a06_sensor_fusion/) | Sensor Fusion | AI Kit, Eva Kit |
| [`a07_optiga_crypto`](./advanced/a07_optiga_crypto/) | [UI Reference] Crypto Algorithm Visualization | AI Kit, Eva Kit |
| [`a08_hsm_pin_entry`](./advanced/a08_hsm_pin_entry/) | HSM PIN Entry Overlay | AI Kit, Eva Kit |
| [`a09_camera_preview`](./advanced/a09_camera_preview/) | [UI Concept] Camera Preview | AI Kit |
| [`a10_flappy_bird`](./advanced/a10_flappy_bird/) | Flappy Bird (Touch Control) | AI Kit, Eva Kit, Game Console |
| [`a11_face_detection`](./advanced/a11_face_detection/) | [UI Concept] Face Detection with ML | AI Kit |
| [`a12_snake_game`](./advanced/a12_snake_game/) | Snake Game (Touch D-pad) | AI Kit, Eva Kit, Game Console |
| [`a13_pong_game`](./advanced/a13_pong_game/) | Pong Game (Player vs AI) | AI Kit, Eva Kit, Game Console |
| [`a14_game_shooter`](./advanced/a14_game_shooter/) | Space Shooter (Touch Control) | AI Kit, Eva Kit, Game Console |
| [`a15_game_framework`](./advanced/a15_game_framework/) | Game Common Framework Demo | AI Kit, Eva Kit, Game Console |
| [`a16_hsm_health`](./advanced/a16_hsm_health/) | HSM Health Status Dashboard | AI Kit, Eva Kit |
| [`a17_hsm_crypto`](./advanced/a17_hsm_crypto/) | HSM Crypto Benchmark | AI Kit, Eva Kit |
| [`a18_radar_presence`](./advanced/a18_radar_presence/) | Radar Presence Detection | AI Kit |
| [`a19_smart_watch`](./advanced/a19_smart_watch/) | Multi-Screen Smart Watch | AI Kit, Eva Kit |
| [`a20_production_dashboard`](./advanced/a20_production_dashboard/) | Production Dashboard | AI Kit, Eva Kit |
| [`a21_image_convert`](./advanced/a21_image_convert/) | [UI Concept] Camera Frame Capture | AI Kit |
| [`a22_face_database`](./advanced/a22_face_database/) | [UI Concept] Face Database | AI Kit |
| [`a23_wifi_connect_ipc`](./advanced/a23_wifi_connect_ipc/) | WiFi Connect via IPC | AI Kit, Eva Kit |
| [`a24_wifi_settings`](./advanced/a24_wifi_settings/) | WiFi Settings (iOS-style Scan + Connect) | AI Kit, Eva Kit |

---

## โครงสร้างไฟล์

ทุกตัวอย่างมีโครงสร้างเดียวกัน:

```
beginner/bNN_<name>/   intermediate/iNN_<name>/   advanced/aNN_<name>/
  main_example.c      จุดเริ่มต้น: void example_main(lv_obj_t *parent)
  metadata.json       หัวข้อ, ระดับ, โดเมน, บอร์ดที่รองรับ, API ที่ใช้
  README.md           คำอธิบายตัวอย่าง (ภาษาไทยหรืออังกฤษ)
  *.c / *.h           ไฟล์เสริม (ถ้ามี)
pse84_common.h        เฮดเดอร์ร่วม: LVGL, FreeRTOS, BSP feature flags, ค่าสี และตัวช่วย thai_label()
```

---

## ชุดตัวอย่างอื่นใน repository นี้

| Branch | เนื้อหา |
|--------|---------|
| [`main`](https://github.com/tesaiot/developer-hub) | ตัวอย่างการเชื่อมต่อ TESAIoT Platform (MQTT, REST API, Security) |
| [`tesaiot_dev_kit_practise_codes`](https://github.com/tesaiot/developer-hub/tree/tesaiot_dev_kit_practise_codes) | ตัวอย่างฝึกฝนสำหรับ TESAIoT Dev Kit รวมถึงตัวอย่าง 19 ตัวจาก branch นี้ที่ปรับให้ใช้กับ Dev Kit |
| [`tesaiot_dev_kit_episodes`](https://github.com/tesaiot/developer-hub/tree/tesaiot_dev_kit_episodes) | Episodes ทีละขั้น: เมนูและการตั้งค่า HMI, การแสดงผลเซนเซอร์ |

---

## English

76 C/LVGL examples for Infineon PSoC Edge E84 boards (AI Kit, Eva Kit, Game
Console), in `beginner/`, `intermediate/` and `advanced/`. Each example folder
holds `main_example.c` (entry point `void example_main(lv_obj_t *parent)`),
`README.md` and `metadata.json`; the `boards` field of `metadata.json` names the
boards it supports. All examples include `pse84_common.h` from the branch root.

The examples are drop-in source sets, not standalone projects. Build one at a
time inside the BENTO/TESAIoT SDK template from
[tesaiot/tesaiot-pse84-devkit-sdk](https://github.com/tesaiot/tesaiot-pse84-devkit-sdk):
copy the example and `pse84_common.h` into the template's CM55 project, call
`example_main()` once from the CM55 graphics task after LVGL is up (the template
does not do this for you), make sure the `BSP_HAS_*` flags for your board are
defined, then `make build` and `make program`. The SDK template currently ships
a board support package for `KIT_PSE84_AI` only; Eva Kit (`KIT_PSE84_EVAL_EPC2`)
examples need an Eva Kit BSP, which it does not include. This branch contains no
project template and no prebuilt libraries.

---

## License

Apache License 2.0 เช่นเดียวกับ branch `main` ของ repository นี้ ดู [LICENSE](./LICENSE)
