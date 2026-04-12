# Chart Statistics

## เกี่ยวกับตัวอย่างนี้
ตัวอย่างนี้สาธิตการคำนวณสถิติ (min, max, avg) จากค่าความเร่ง BMI270 แกน X แบบเรียลไทม์ แสดงกราฟเส้นพร้อมค่าสถิติ 4 ตัวในการ์ดแยก มี ring buffer เก็บ 120 ตัวอย่างล่าสุด และปุ่ม Reset เคลียร์ข้อมูล ใช้ `app_sensor/bmi270` อ่านค่าเซ็นเซอร์โดยตรงผ่าน I2C

## สิ่งที่จะได้เรียนรู้
- การทำ ring buffer สำหรับเก็บประวัติข้อมูลเซ็นเซอร์
- การคำนวณ min/max/avg จาก sliding window
- การสร้าง stat cards ด้วย LVGL flex layout
- การใช้ `lv_btn_create()` และ event callback สำหรับปุ่ม Reset
- การอ่านค่า BMI270 ผ่าน `bmi270_reader_poll()` แทน IPC

## ความต้องการ
- บอร์ด KIT_PSE84_AI หรือ TESAIoT_PSE84_AI
- เซ็นเซอร์ BMI270 (onboard)
- ไฟล์ `app_sensor/bmi270/` ในโปรเจค

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. ตรวจสอบว่า `app_sensor/bmi270/` อยู่ใน include path
3. Build และ flash ตามปกติ

## สิ่งที่จะเห็นบนหน้าจอ
ด้านบนมีหัวข้อ "Acceleration Statistics" พร้อมปุ่ม Reset ถัดมาเป็นกราฟเส้นแสดง accel X ด้านล่างมี 4 การ์ดสถิติ (Current, Min, Max, Avg) พร้อมจำนวน samples

## ลองปรับแต่ง
- เปลี่ยนจากแกน X เป็นแกน Y หรือ Z
- เพิ่มค่า standard deviation
- เพิ่ม chart สำหรับแสดง min/max boundary lines
- ปรับ `STATS_WINDOW` เพื่อเปลี่ยนช่วงเวลาสถิติ
