# QWA309 — DFR0522 RGB Dot Matrix

## เกี่ยวกับตัวอย่างนี้
ตัวอย่างนี้ควบคุมจอ RGB dot-matrix ของ DFRobot DFR0522 (ขนาด 16x8 พิกเซล) ผ่านบัส I2C ที่ปลายทาง address `0x10` โดยใช้บัส I2C 3.3V ที่ framework เตรียมไว้ให้ร่วมกับจอ display อยู่แล้ว (`DISPLAY_I2C_CONTROLLER_HW`) จึงไม่ต้องตั้งค่า SCB เพิ่มเอง หน้าจอ LVGL จะสร้างปุ่มควบคุมให้กดสั่งงาน matrix ได้ทั้งการล้างจอ ติดพิกเซลเดี่ยว เติมสีทั้งแผง และรูปแบบหลายสี พร้อมแสดงสถานะการสื่อสาร I2C กลับมาแบบเรียลไทม์

## ฮาร์ดแวร์ที่ใช้
- **DFRobot DFR0522 RGB Matrix (16x8)** — จอ dot-matrix RGB ที่ควบคุมผ่าน I2C address `0x10` (`RGB_PANEL_I2C_ADDRESS`)
- **บัส I2C ร่วม 3.3V (`DISPLAY_I2C_CONTROLLER_HW`)** — SCB I2C controller ตัวเดียวกับที่ใช้กับจอ display/touch โดย framework init ให้แล้ว ใช้ context `disp_touch_i2c_controller_context`
- **จอ LVGL + touch ของ TESAIoT Dev Kit** — ใช้แสดงหน้าปุ่มควบคุมและรับการกดจากผู้ใช้

## สิ่งที่จะได้เรียนรู้
- การสื่อสาร I2C ระดับ low-level ด้วย `Cy_SCB_I2C_MasterSendStart()` / `MasterWriteByte()` / `MasterSendStop()`
- การประกอบ frame คำสั่งของ DFR0522 (command register `0x02`, function byte, color, x, y) ลง tx buffer ขนาด 51 ไบต์ (`RGB_PANEL_TX_SIZE`)
- ฟังก์ชันของ panel: clear (`0x01`), fill (`0x09`), pixel (`0x08`) ตามค่าใน `rgb_panel.c`
- การทำ device presence check ด้วย start/stop condition แล้วอ่านค่า ACK/NACK
- การแปลง `cy_en_scb_i2c_status_t` เป็นข้อความสถานะ (ACK, TIMEOUT, ADDRESS NACK ฯลฯ)
- การจัด layout ปุ่มด้วย LVGL flexbox และผูก event callback (`LV_EVENT_CLICKED`)

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ด้วย `BOARD=TESAIOT_DEV_KIT` (ตัวอย่าง QWA309 ใช้ base board — ลงได้เฉพาะ TESAIoT Dev Kit) และต้องต่อจอ DFR0522 บนบัส I2C 3.3V

## สิ่งที่จะเห็นบนหน้าจอ
พื้นหลังสีเข้ม (`0x101418`) มี header ด้านบนแสดงชื่อ "DFRobot DFR0522 RGB Matrix" พร้อมป้ายสถานะ "Device 0x10" และปุ่ม **Check 0x10** / **Clear** ถัดลงมาแบ่งเป็น 3 กลุ่ม:
- **Single LED (center pixel)** — ปุ่มสี Red/Green/Blue/Yellow/Purple/Cyan/White ติดพิกเซลเดี่ยวที่ตำแหน่งกลางจอ (x=7, y=3)
- **Whole panel** — ปุ่มสีชุดเดียวกันสำหรับเติมสีเต็มทั้งแผง
- **Multi-color patterns** — ปุ่ม RGB Pixels, All Colors และ Corners (ติด 4 มุมจอด้วยสีแดง/เขียว/น้ำเงิน/ขาว)

เมื่อกดปุ่ม ปุ่มทั้งหมดจะถูก disable ชั่วคราว ป้ายสถานะขึ้น "Sending..." แล้วเปลี่ยนเป็นสีเขียวถ้าสำเร็จ หรือแดงถ้าเกิดข้อผิดพลาด พร้อมข้อความผลลัพธ์

## ลองปรับแต่ง
- เปลี่ยนตำแหน่งพิกเซลกลางจอในโหมด PIXEL จาก (7, 3) เป็นตำแหน่งอื่นภายในขอบเขต 16x8 (`RGB_PANEL_WIDTH` x `RGB_PANEL_HEIGHT`)
- เพิ่มสีใหม่ใน `pixel_button_configs` / `fill_button_configs` โดยอ้างค่าใน `rgb_panel_color_t` (OFF..WHITE = 0..7)
- ปรับ `DEVICE_CHECK_TIMEOUT_MS` / `RGB_PANEL_BYTE_TIMEOUT_MS` เพื่อทดสอบความไวของ timeout บนบัส I2C
- สร้าง pattern ใหม่ (เช่น เส้นทแยง หรือ animation) ใน `run_panel_action()` โดยเรียก `rgb_panel_pixel()` เป็นลูป
- เพิ่ม delay ระหว่างพิกเซลใน pattern เพื่อดูลำดับการวาดทีละจุด
