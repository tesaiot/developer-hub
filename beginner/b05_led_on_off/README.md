# เปิดปิด LED ฮาร์ดแวร์ (Hardware LED On/Off)

ตัวอย่างนี้สร้างปุ่มสองตัวบนหน้าจอ: ON และ OFF การแตะ ON จะเปิด LED ฮาร์ดแวร์ที่ต่อกับขา P13_7 และการแตะ OFF จะปิด Virtual LED ของ LVGL สะท้อนสถานะฮาร์ดแวร์เพื่อ feedback ทางภาพ

GPIO ถูกเริ่มต้นด้วย `cyhal_gpio_init` กำหนดเป็น `CYHAL_GPIO_DIR_OUTPUT` พร้อม strong drive ฟังก์ชัน `cyhal_gpio_write` ตั้งค่าขาเป็น high หรือ low ตามปุ่มที่ถูกกด

ตัวอย่างนี้ผสมผสานการควบคุมฮาร์ดแวร์กับ LVGL UI feedback Virtual LED บนหน้าจออัปเดตพร้อมกับ LED จริง ผู้ใช้จึงรู้สถานะปัจจุบันเสมอแม้มองไม่เห็นบอร์ดจริง

ขา P13_7 ตรงกับ user LED บนทั้ง AI Kit และ Eva Kit ตรวจสอบว่า LED ไม่ถูกใช้โดยโมดูลอื่นก่อนรันตัวอย่างนี้ รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
