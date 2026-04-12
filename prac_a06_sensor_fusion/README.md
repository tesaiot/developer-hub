# Sensor Fusion - 9-DOF AHRS

## เกี่ยวกับตัวอย่างนี้
ระบบรวมข้อมูลเซ็นเซอร์ 9 แกน (Accelerometer 3 แกน + Gyroscope 3 แกน + Magnetometer 3 แกน) เพื่อคำนวณ orientation ของบอร์ด ใช้ complementary filter ผสานข้อมูลจาก BMI270 และ BMM350 แสดงผล compass, artificial horizon, tilt indicator และค่าดิบทั้ง 9 แกน

## เซ็นเซอร์ที่ใช้
- **BMI270** — Accelerometer (acc_g_x/y/z) และ Gyroscope (gyr_dps_x/y/z)
- **BMM350** — Magnetometer (x_ut/y_ut/z_ut) และ heading_deg

## สิ่งที่จะได้เรียนรู้
- หลักการ Complementary Filter สำหรับ sensor fusion
- การคำนวณ Roll/Pitch จาก accelerometer + gyroscope
- การใช้ magnetometer สำหรับ heading/yaw
- การสร้าง compass dial ด้วย LVGL objects
- การสร้าง artificial horizon แบบ pitch/roll responsive
- การแสดงค่า raw sensor data แบบ 3 คอลัมน์

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ตามปกติ

## สิ่งที่จะเห็นบนหน้าจอ
- ซ้าย: compass dial พร้อมจุดแดงแสดงทิศเหนือ และค่า heading ด้านล่าง
- กลาง: artificial horizon แสดงระดับเอียงของบอร์ด (สีฟ้า=ฟ้า สีน้ำตาล=ดิน)
- ขวา: ค่า Roll/Pitch/Yaw/Heading เป็นตัวเลข พร้อม fusion quality bar
- ล่าง: ค่าดิบ 9 แกน (Accel, Gyro, Mag) และ tilt indicator วงกลม

## ลองปรับแต่ง
- ปรับค่า `ALPHA` (0.0-1.0) เพื่อเปลี่ยนน้ำหนักระหว่าง gyro กับ accel
- เปลี่ยน `REFRESH_MS` เพื่อปรับอัตราอัปเดต (ค่าน้อย = smooth กว่า)
- เพิ่ม Kalman filter แทน complementary filter
- เพิ่ม magnetometer calibration routine
