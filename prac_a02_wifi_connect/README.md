# WiFi Connect Flow

## เกี่ยวกับตัวอย่างนี้
ตัวอย่าง UI สำหรับเชื่อมต่อ WiFi แบบ STA (Station) ครบวงจร ตั้งแต่ scan หาเครือข่าย, เลือก AP จาก dropdown, ใส่รหัสผ่านผ่าน on-screen keyboard, จนถึงแสดงสถานะเชื่อมต่อและ IP address ใช้ state machine ควบคุม UI transitions และ demo data สำหรับแสดงผลบน CM55 (WiFi จริงทำงานบน CM33_NS)

## สิ่งที่จะได้เรียนรู้
- การสร้าง state machine สำหรับ UI flow หลายขั้นตอน
- การใช้ `lv_dropdown_create()` แสดงรายการเครือข่าย WiFi
- การใช้ `lv_keyboard_create()` + `lv_textarea_create()` รับรหัสผ่าน
- การใช้ `lv_spinner_create()` แสดงสถานะ loading
- การซ่อน/แสดง UI elements ตาม state ด้วย `LV_OBJ_FLAG_HIDDEN`
- การใช้ poll timer สำหรับ non-blocking UI updates
- การใช้ color-coded indicator แสดงสถานะเชื่อมต่อ

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ตามปกติ
3. ตัวอย่างนี้ใช้ demo data ไม่ต้องมี WiFi จริง

## สิ่งที่จะเห็นบนหน้าจอ
หน้าจอแสดง "WiFi Connect" พร้อมปุ่ม Scan ด้านล่าง กด Scan จะเห็น spinner หมุนประมาณ 1.5 วินาที จากนั้นแสดง dropdown พร้อมรายชื่อเครือข่ายจำลอง 5 รายการ เลือกเครือข่ายแล้วใส่รหัสผ่าน กด Connect จะจำลองการเชื่อมต่อแล้วแสดง IP address

## หมายเหตุ
ตัวอย่างนี้ใช้ข้อมูลจำลองเพราะ WiFi stack (cy_wcm) ทำงานบน CM33_NS ไม่ใช่ CM55 หากต้องการเชื่อมต่อ WiFi จริง ต้อง deploy บน CM33_NS หรือใช้ IPC bridge

## ลองปรับแต่ง
- เพิ่ม RSSI bar indicator ข้างชื่อเครือข่าย
- เพิ่มฟังก์ชัน disconnect
- เพิ่มหน้าจอแสดงรายละเอียดเครือข่ายที่เชื่อมต่อ
- เปลี่ยน demo data ให้สุ่มค่า RSSI แต่ละครั้งที่ scan
