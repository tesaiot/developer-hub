# Multi-Page Navigation App

## เกี่ยวกับตัวอย่างนี้
ตัวอย่างนี้สาธิตรูปแบบการนำทางหลายหน้าแบบ production-ready ประกอบด้วย navigation stack สำหรับปุ่ม Back, แถบแท็บด้านล่างสำหรับสลับหน้า และสามหน้าหลัก (Home, Sensors, Settings) ข้อมูลเซ็นเซอร์ใช้ค่าจำลองเพื่อให้ UI ทำงานได้โดยไม่ต้องพึ่ง IPC

## สิ่งที่จะได้เรียนรู้
- การสร้าง page manager แบบ self-contained ด้วย navigation stack
- การสร้างแถบแท็บ (tab bar) ด้านล่างพร้อม highlight หน้าที่เลือก
- การจัดการ page lifecycle (สร้าง/ทำลาย content เมื่อสลับหน้า)
- การใช้ `lv_obj_clean()` ล้าง content area ก่อนสร้างหน้าใหม่
- การใช้ slider, switch และ button ใน Settings page
- การเก็บ navigation stack เพื่อรองรับปุ่ม Back

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ตามปกติ

## สิ่งที่จะเห็นบนหน้าจอ
ด้านล่างจะมีแถบแท็บ 3 ปุ่ม (Home, Sensors, Settings) หน้า Home แสดงการ์ดต้อนรับและปุ่มนำทาง หน้า Sensors แสดงข้อมูล IMU (Accelerometer + Gyroscope) แบบเรียลไทม์ หน้า Settings มีสไลเดอร์ความสว่างและสวิตช์ Dark Theme

## ลองปรับแต่ง
- เพิ่มหน้าที่ 4 เช่น หน้า About หรือ Help
- เปลี่ยนข้อมูลจำลองเป็นค่าเซ็นเซอร์จริงผ่าน `app_sensor`
- เพิ่ม animation เมื่อสลับหน้า (slide left/right)
- เพิ่มฟีเจอร์บันทึกค่า Settings ลง NVM
- เพิ่ม badge/notification บนแท็บเพื่อแจ้งเตือนข้อมูลใหม่
