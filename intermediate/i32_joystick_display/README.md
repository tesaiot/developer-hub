# แสดงผลจอยสติ๊ก (Joystick Visualization)

ตัวอย่างนี้แสดงผลข้อมูล gamepad แบบ dual-analog-stick บนจอ AMOLED ดัดแปลงจาก production `page_joystick.c` โดยรองรับการแสดงผลครบทุกส่วนของ gamepad

คุณสมบัติหลัก:
- แท่งอนาล็อกซ้ายและขวา: พื้นที่ 180x180 พิกเซลพร้อม crosshair, dead zone ring (30px จากศูนย์กลาง) และจุดแสดงตำแหน่งที่เคลื่อนที่ตามค่าแกน
- ปุ่ม A/B/X/Y: วงกลมสีเขียว/แดง/น้ำเงิน/ส้มที่สว่างขึ้นเมื่อกด
- D-pad (Hat Switch): ลูกศร 4 ทิศที่ highlight ทิศทางที่กด
- ปุ่มไหล่ LB/RB: แถบตัวบ่งชี้สีม่วง
- ค่าแกนดิบ: LX, LY, RX, RY (0-255) แสดงแบบเรียลไทม์

ใน production จริง ข้อมูล joystick มาจาก CM33_NS USB Host ผ่าน IPC_CMD_JOYSTICK_STATE (0xC0) ตัวอย่างนี้ใช้ touch input จำลองการเคลื่อนแท่งอนาล็อก — ลากนิ้วบนพื้นที่ crosshair เพื่อเคลื่อนจุด ส่วนปุ่มและ D-pad จะสลับอัตโนมัติเพื่อแสดงหน้าตา UI

รองรับทุกบอร์ด: TESAIoT Dev Kit, PSoC Edge AI Kit, Eva Kit และ Game Console
