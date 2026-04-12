# Automation Rules

## เกี่ยวกับตัวอย่างนี้
ระบบกฎอัตโนมัติ (Rule Engine) ที่ตรวจสอบค่าเซ็นเซอร์เทียบกับ threshold ที่ตั้งไว้ มี 4 กฎที่ทำงานพร้อมกัน อัปเดตผลการประเมินที่ 5 Hz แสดงสถานะ TRIGGERED (สีแดง) หรือ Normal (สีเขียว) พร้อม global alert

## กฎที่ตั้งไว้
1. **Rule 1** — Accel magnitude > 1.5g (ตรวจจับการสั่น/เขย่า)
2. **Rule 2** — Temperature > 35C (อุณหภูมิสูงเกินไป)
3. **Rule 3** — Humidity > 70% (ความชื้นสูงเกินไป)
4. **Rule 4** — Pressure < 990 hPa (ความดันต่ำ/พายุ)

## เซ็นเซอร์ที่ใช้
- **BMI270** — ความเร่ง 3 แกน คำนวณ magnitude
- **DPS368** — ความดันอากาศ (hPa) และอุณหภูมิสำรอง
- **SHT4x** — อุณหภูมิ (C) และความชื้นสัมพัทธ์ (%)

## สิ่งที่จะได้เรียนรู้
- การออกแบบ rule engine ด้วย struct array
- การใช้ enum สำหรับ source/operator mapping
- การสร้าง status indicator (วงกลมสี) ด้วย LVGL
- การใช้ `lv_flex_flow` แบบ ROW_WRAP สำหรับ grid layout
- การอ่านค่าเซ็นเซอร์หลายตัวและ dispatch ตาม source type

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ตามปกติ

## สิ่งที่จะเห็นบนหน้าจอ
การ์ดกฎ 4 ใบเรียงแบบ 2x2 แต่ละใบแสดงเงื่อนไข ค่าปัจจุบัน และสถานะ (Normal/TRIGGERED) พร้อมไฟ indicator สีเขียว/แดง ด้านบนมี global alert แจ้งจำนวนกฎที่ triggered

## ลองปรับแต่ง
- เปลี่ยนค่า threshold ของแต่ละกฎ
- เพิ่มกฎใหม่ (เพิ่ม `MAX_RULES` และ source type)
- เพิ่ม action เมื่อ triggered เช่น กระพริบหน้าจอ หรือส่ง alert ผ่าน UART
- เพิ่ม hysteresis เพื่อป้องกันการ toggle บ่อยเกินไป
