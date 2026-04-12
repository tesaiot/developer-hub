# Rich Text LCD Console

## เกี่ยวกับตัวอย่างนี้
ตัวอย่างนี้สร้างหน้าจอคอนโซลแบบเทอร์มินัลบนจอ LVGL สำหรับแสดง log ระบบแบบเรียลไทม์ รองรับ 4 ระดับ (INFO, WARN, ERROR, DEBUG) พร้อม timestamp จาก FreeRTOS, auto-scroll ไปบรรทัดล่างสุดอัตโนมัติ และ ring buffer ขนาด 50 บรรทัดเพื่อจำกัดการใช้หน่วยความจำ

## สิ่งที่จะได้เรียนรู้
- การใช้ `lv_textarea_create()` สร้าง textarea แบบ read-only สำหรับ console output
- การสร้าง ring buffer เพื่อจำกัดจำนวนบรรทัดที่แสดง
- การใช้ `lv_timer_create()` สร้าง timer สำหรับ periodic task
- การจัดรูปแบบ log ด้วย timestamp จาก `xTaskGetTickCount()`
- การใช้ variadic function (`va_list`) สำหรับ printf-style API

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ตามปกติ

## สิ่งที่จะเห็นบนหน้าจอ
หน้าจอพื้นหลังสีเข้มแสดง title bar ด้านบน ตามด้วย legend แสดงระดับ log แต่ละสี กล่องคอนโซลตรงกลางจะแสดงข้อความ log ที่วิ่งขึ้นเรื่อยๆ ทุก 500ms พร้อม timestamp และแถบสถิติด้านล่างแสดงจำนวนบรรทัด

## ลองปรับแต่ง
- เปลี่ยนขนาด ring buffer (CONSOLE_MAX_LINES) ให้มากขึ้นหรือน้อยลง
- ปรับ DEMO_INTERVAL_MS ให้เร็วขึ้น (100ms) หรือช้าลง (1000ms)
- เพิ่มปุ่ม Clear สำหรับล้างหน้าจอคอนโซล
- เพิ่มปุ่ม Pause/Resume สำหรับหยุดและเริ่ม log ใหม่
- ลองเพิ่มสีพื้นหลังที่แตกต่างกันตามระดับ log
