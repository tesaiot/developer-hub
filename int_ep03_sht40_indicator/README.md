# INT EP03 — SHT40 Humidity & Temperature Indicator

วัด **ความชื้นสัมพัทธ์ (RH%)** และ **อุณหภูมิ (°C)** ด้วยเซนเซอร์ Sensirion **SHT4x** บน I2C แล้วแสดงผลเป็นตัวบ่งชี้สีบนจอ LVGL

---

## Why — ทำไมต้องเรียนตอนนี้

**SHT4x** series (SHT40 / SHT41 / SHT45) คือเซนเซอร์ humidity + temperature ของ Sensirion ที่ได้ชื่อว่า **แม่นที่สุดในตลาด** ในระดับราคา:

- **±1.8 %RH** (SHT40)
- **±0.2 °C**
- ช่วง RH: 0 – 100 %, temp: -40 – +125 °C
- **Low power** — < 0.4 µA average ที่ 1 Hz
- **Protocol** — I2C address `0x44` (SHT40) / `0x45` / `0x46`

เซนเซอร์ Environmental สำคัญเพราะ:

- **HVAC / smart building** — ควบคุมแอร์ + dehumidifier
- **Agriculture** — ดูแลโรงเรือน, monitor dew point
- **Data center** — ป้องกันการกัดกร่อนและ static discharge
- **Cold chain** — ติดตามการขนส่งยา / วัคซีน
- **ธรณีวิทยา** — ความชื้นในดินคำนวณจาก air RH + soil sensor

ในตอนนี้คุณจะได้เรียนรู้:

1. **Command-based I2C protocol** (ไม่มี register map เหมือน DPS368/BMI270)
2. การจัดการ **conversion delay** (SHT4x ต้องรอ ~10 ms หลังสั่ง measure)
3. **CRC-8 verification** สำหรับทุก response (Sensirion ใช้ polynomial 0x31)
4. การเล่นสีตาม threshold (สีเขียว = สบาย, เหลือง = แห้ง/ชื้นเกิน, แดง = อันตราย)

---

## What — ไฟล์ในตอนนี้

| ไฟล์ | หน้าที่ |
|---|---|
| `main_example.c` | Entry wrapper → `sht4x_presenter_start()` |
| `app_sensor/sht4x/sht4x_driver.{c,h}` | ส่ง command + อ่าน 6 bytes + ตรวจ CRC |
| `app_sensor/sht4x/sht4x_reader.{c,h}` | แปลง raw → RH% / °C ตามสูตร Sensirion |
| `app_sensor/sht4x/sht4x_config.h` | Address + precision mode (high / med / low) |
| `app_sensor/sht4x/sht4x_types.h` | struct `sht4x_sample_t` |
| `app_ui/sht4x/sht4x_presenter.{c,h}` | Wire reader ↔ view |
| `app_ui/sht4x/sht4x_view.{c,h}` | Arc indicators + threshold colors |
| `app_ui/app_logo.{c,h}`, `APP_LOGO.png` | โลโก้ |

รวม **14 ไฟล์**

---

## How — อ่านโค้ดทีละชั้น

### ชั้นที่ 1 — Master เตรียม I2C ให้

```c
#include "sensor_bus.h"
#include "sht4x/sht4x_presenter.h"

void example_main(lv_obj_t *parent)
{
    (void)parent;
    sht4x_presenter_start(&sensor_i2c_controller_hal_obj);
}
```

### ชั้นที่ 2 — Command protocol

SHT4x ไม่มี register map — แค่ส่ง **1-byte command** แล้วรอผล:

| Command | Action | Delay |
|---|---|---|
| `0xFD` | Measure T & RH, high precision | 8.2 ms |
| `0xF6` | Medium precision | 4.5 ms |
| `0xE0` | Low precision | 1.6 ms |
| `0x94` | Soft reset | 1 ms |
| `0x89` | Read serial number | 10 ms |

หลัง delay host อ่าน 6 bytes: `[T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC]`

### ชั้นที่ 3 — CRC-8 (polynomial 0x31)

Sensirion ใช้ CRC-8 Maxim polynomial:

```c
uint8_t crc = 0xFF;
for (each byte) {
    crc ^= byte;
    for (8 bits) crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
}
```

ถ้า CRC ผิดแปลว่า **wire noise** หรือ **pull-up อ่อน** — ต้องทิ้งตัวอย่างทิ้ง

### ชั้นที่ 4 — แปลง raw → physical

สูตรจาก Sensirion datasheet:

```
T_degC  = -45 + 175 * (t_raw / 65535)
RH_pct  = -6  + 125 * (rh_raw / 65535)  /* แล้ว clamp [0, 100] */
```

### ชั้นที่ 5 — View color thresholds

`sht4x_view.c` เปลี่ยนสี arc ตาม RH:

- RH < 30 % → สีส้ม (แห้งเกิน)
- 30 – 60 % → สีเขียว (สบาย)
- 60 – 80 % → สีเหลือง (ชื้น)
- > 80 % → สีแดง (อันตราย)

---

## Install & Run

```bash
cd tesaiot_dev_kit_master
rsync -a ../episodes/int_ep03_sht40_indicator/ proj_cm55/apps/int_ep03_sht40_indicator/
make getlibs
make build -j
make program
```

หายใจรดเซนเซอร์เบาๆ — RH ควรกระโดดขึ้น 10 – 20 % ภายใน 2 วินาที

---

## Experiment Ideas

- **Dew point calculator** — สูตร Magnus: `Td = (bT(RH)/(a - T(RH)))` แสดงเป็นค่าที่ 3
- **Comfort index** — heat index, humidex, หรือ THI
- **Calibration check** — เทียบกับเกลือเปียก NaCl (75 %RH saturated)
- **Logging 24 ชม.** — บันทึก 1 sample/นาที ลง RAM ring buffer

---

## Glossary

- **RH (Relative Humidity)** — % ของไอน้ำจริงเทียบกับ saturation ณ อุณหภูมินั้น
- **Dew point** — อุณหภูมิที่ไอน้ำเริ่มกลั่นตัวเป็นหยดน้ำ
- **CRC** — Cyclic Redundancy Check, checksum ป้องกันข้อมูลเพี้ยน
- **Polynomial 0x31** — `x^8 + x^5 + x^4 + 1` ที่ Sensirion ใช้เป็นมาตรฐาน

---

## Next

ไปตอน **EP04 — BMM350 Compass** เพื่อเรียน I3C + magnetometer + calibration flow
