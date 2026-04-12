# Practice Examples — TESAIoT Dev Kit

ตัวอย่างสำหรับฝึกฝนนอกเวลาและต่อยอดสร้าง IoT Application บน TESAIoT Dev Kit
รวม 24 ตัวอย่าง แบ่งเป็น 3 กลุ่มตามลักษณะการใช้งาน

## รายการตัวอย่าง

### กลุ่ม A: Pure UI — 10 ตัวอย่าง

ตัวอย่างที่ใช้ LVGL ล้วน ไม่ต้องต่อ sensor

| # | โฟลเดอร์ | ชื่อ | ระดับ | คำอธิบาย |
|---|---|---|---|---|
| 1 | prac_b18_thai_text | Thai Text Display | Beginner | แสดงข้อความภาษาไทย Noto Sans Thai 4 ขนาด |
| 2 | prac_i19_color_mixer | RGB Color Mixer | Intermediate | 3 sliders ผสมสี RGB พร้อม preview และ hex code |
| 3 | prac_i33_lcd_console | Rich Text LCD Console | Intermediate | console แสดง log แบบ color-coded พร้อม auto-scroll |
| 4 | prac_i15_tile_navigation | Tile Navigation | Intermediate | swipe ซ้าย-ขวาสลับ 4 หน้า ด้วย TileView |
| 5 | prac_i28_multi_page_app | Multi-Page App | Intermediate | app 3 หน้า (Home/Sensors/Settings) พร้อม tab bar |
| 6 | prac_a10_flappy_bird | Flappy Bird | Advanced | เกม touch tap, 50fps physics, Game Boy palette |
| 7 | prac_a12_snake_game | Snake Game | Advanced | เกม touch D-pad, grid 30x24, 140 segments |
| 8 | prac_a13_pong_game | Pong Game | Advanced | เกม touch, AI opponent, progressive speed-up |
| 9 | prac_a14_game_shooter | Space Shooter | Advanced | เกม D-pad+fire, entity pool, AABB collision |
| 10 | prac_a15_game_framework | Game Framework | Advanced | framework สำหรับสร้างเกม: palette, CRT overlay, input |

### กลุ่ม B: Sensor Visualization — 10 ตัวอย่าง

ตัวอย่างที่อ่านค่า sensor โดยตรงจาก CM55 แล้ว visualize บนจอ

| # | โฟลเดอร์ | ชื่อ | Sensors | คำอธิบาย |
|---|---|---|---|---|
| 1 | prac_i02_line_chart_accel | Line Chart | BMI270 | กราฟเส้น scrolling 3 แกน XYZ |
| 2 | prac_i07_motion_detector | Motion Detector | BMI270 | ตรวจจับการเคลื่อนไหว + event log |
| 3 | prac_i08_level_bubble | Bubble Level | BMI270 | ระดับน้ำ 2D จาก accelerometer tilt |
| 4 | prac_i18_chart_statistics | Chart Statistics | BMI270 | กราฟพร้อมสถิติ min/max/avg แบบ rolling |
| 5 | prac_i10_gauge_cluster | Gauge Cluster | DPS368 + SHT4x | 3 arc gauges: อุณหภูมิ ความชื้น ความดัน |
| 6 | prac_i03_bar_chart_multi | Multi-Series Bar | ทั้ง 4 sensors | กราฟแท่งเปรียบเทียบ sensor ทุกตัว |
| 7 | prac_i09_data_logger | Data Logger | BMI270 + DPS368 + SHT4x | บันทึกค่า sensor พร้อม timestamp |
| 8 | prac_i06_environ_monitor | Environment Monitor | DPS368 + SHT4x | ตรวจวัดสิ่งแวดล้อม พร้อม trend arrows |
| 9 | prac_a06_sensor_fusion | 9-DOF Sensor Fusion | BMI270 + BMM350 | AHRS orientation จาก accel+gyro+mag |
| 10 | prac_i23_automation_rules | Automation Rules | BMI270 + DPS368 + SHT4x | กฎ IF-THEN 4 ข้อ ตั้งเงื่อนไขจาก sensor |

### กลุ่ม C: Production Patterns — 4 ตัวอย่าง

ตัวอย่างรูปแบบ app จริง สำหรับต่อยอดเป็น IoT product

| # | โฟลเดอร์ | ชื่อ | คำอธิบาย |
|---|---|---|---|
| 1 | prac_a20_production_dashboard | Production Dashboard | dashboard อุตสาหกรรม: 4 sensor cards + live chart |
| 2 | prac_a19_smart_watch | Smart Watch | หน้าปัดนาฬิกาหลายหน้า swipe สลับ (Clock/Sensors/Steps/Weather) |
| 3 | prac_i11_status_panel | System Status Panel | แสดง CPU, heap, uptime, WiFi state จาก FreeRTOS |
| 4 | prac_a02_wifi_connect | WiFi Connect Flow | scan AP → เลือก → ใส่ password → connect (demo mode) |

## โครงสร้างไฟล์

ทุกตัวอย่างมีโครงสร้างเดียวกัน:

```
prac_<id>_<name>/
  main_example.c      entry point: void example_main(lv_obj_t *parent)
  metadata.json        ข้อมูลตัวอย่าง (title, difficulty, tags, boards)
  README.md            คำอธิบายภาษาไทย
  *.c / *.h            ไฟล์เสริม (ถ้ามี)
```

## License

MIT License — TESAIoT Foundation Platform (www.tesaiot.dev)
