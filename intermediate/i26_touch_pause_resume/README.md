# IPC หยุด/เริ่มสัมผัส (Touch Pause/Resume IPC)

ตัวอย่างนี้สาธิตโปรโตคอลการแชร์ I2C bus ระหว่าง CM33 และ CM55 โดยใช้ `IPC_CMD_TOUCH_PAUSE` (0xD6) และ `IPC_CMD_TOUCH_RESUME` (0xD7) รูปแบบนี้พัฒนาขึ้นสำหรับการเข้าถึง OPTIGA Trust M ซึ่งแชร์ SCB0 (I2C) กับตัวควบคุมสัมผัส CM55

ก่อนที่ CM33 จะเข้าถึงอุปกรณ์ I2C ที่ใช้ร่วม ต้องส่ง TOUCH_PAUSE ไปยัง CM55 จากนั้น CM55 จะหยุดการสำรวจสัมผัส ปล่อย I2C bus หลังจาก CM33 ทำ I2C transaction เสร็จ จะส่ง TOUCH_RESUME จากนั้น CM55 จะเริ่มต้น touch driver ใหม่ผ่าน `lv_port_indev_request_reinit()` ซึ่งปลอดภัยจาก ISR และทำงานใน GFX task

การ pause/resume ต้องจับคู่อย่างเคร่งครัด: ทุก PAUSE ต้องมี RESUME ตรงกัน RESUME ที่ขาดหายจะปิดการ input สัมผัสถาวร ตัวอย่างนี้มี safety timeout ที่ auto-resume หากคอร์ที่ร้องขอไม่ส่ง RESUME ภายใน 5 วินาที

UI แสดงสถานะสัมผัสปัจจุบัน (Active/Paused) ตัวนับ pause และปุ่มจำลองการเข้าถึง I2C เมื่อกดปุ่ม ลำดับ PAUSE-ACCESS-RESUME ทำงานพร้อม delay ที่เหมาะสม และ label สถานะสัมผัสอัปเดตแบบเรียลไทม์

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
