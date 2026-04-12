# System Status Panel

## เกี่ยวกับตัวอย่างนี้
แผงสถานะระบบระดับ production แสดงข้อมูลระบบ 6 การ์ดในรูปแบบตาราง 2x3 ประกอบด้วย CPU (จำลอง), Heap memory, Uptime, Sensor availability, BMI270 data, และ WiFi status พร้อม color-coded indicator (เขียว=OK, เหลือง=warning, แดง=error) เซ็นเซอร์อ่านโดยตรงจาก app_sensor, WiFi status แสดงเป็น demo (WiFi ทำงานบน CM33_NS)

## สิ่งที่จะได้เรียนรู้
- การสร้างการ์ดสถานะพร้อม indicator dot แบบ color-coded
- การจัดวาง grid layout ด้วย absolute positioning
- การอ่านข้อมูล FreeRTOS (tick count, uptime)
- การตรวจสอบสถานะเซ็นเซอร์หลายตัวพร้อมกัน
- การใช้ `lv_timer_create()` อัพเดทข้อมูลเป็นระยะ

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. ตรวจสอบว่า `app_sensor/` อยู่ใน include path
3. Build และ flash ตามปกติ

## สิ่งที่จะเห็นบนหน้าจอ
หัวข้อ "System Status" พร้อมคำบรรยายภาษาไทย ตามด้วยการ์ดสถานะ 6 ใบจัด 2 คอลัมน์ 3 แถว แต่ละการ์ดมี icon, ชื่อ, ค่า และ dot indicator สีเขียว/เหลืองที่มุมขวาบน ข้อมูลอัพเดททุก 500ms

## ลองปรับแต่ง
- เพิ่มการ์ดแสดง FreeRTOS task count
- เปลี่ยน CPU card ให้ใช้ bar แสดงเปอร์เซ็นต์
- เพิ่ม WiFi status จริงผ่าน IPC bridge ถ้ามี
- เพิ่มการ์ดสำหรับ memory usage จริง
