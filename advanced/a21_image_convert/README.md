# จับภาพจากกล้อง (Camera Frame Capture)

ตัวอย่างนี้สาธิตการจับเฟรมภาพจากเซ็นเซอร์กล้อง, เข้ารหัสเป็น JPEG และแสดงผลใน LVGL image widget ฮาร์ดแวร์กล้องทำงานบน CM55 core และแชร์เฟรมไปยัง CM33 application core ผ่านกลไก IPC frame sharing

pipeline การจับภาพทำงาน 3 ขั้นตอน: ขั้นแรก ร้องขอเฟรมจาก CM55 camera task ผ่าน IPC frame share API ซึ่งคืนค่า pointer ไปยังข้อมูลพิกเซล RGB565 ดิบใน shared PSRAM ขั้นที่สอง เฟรมดิบถูกส่งผ่าน JPEG encoder wrapper ซึ่งบีบอัดเป็น JPEG byte buffer ที่เหมาะสำหรับแสดงผลหรือส่งต่อ ขั้นที่สาม ข้อมูล JPEG ถูกตั้งเป็น source สำหรับ LVGL image widget โดยใช้ image descriptor ที่สร้างแบบ dynamic

UI แสดงพื้นที่ preview ที่เฟรมที่จับได้ถูกแสดง พร้อมตัวควบคุมการจับภาพ (ปุ่ม capture), metadata ของเฟรม (ความละเอียด, ขนาด, เวลาเข้ารหัส) และตัวบ่งชี้สถานะ preview อัปเดตทุกครั้งที่ผู้ใช้กดปุ่ม capture แสดงเฟรมล่าสุด สถิติเฟรมรวมถึงขนาดดิบ, ขนาด JPEG, อัตราส่วนการบีบอัด และเวลาเข้ารหัสเป็นมิลลิวินาทีแสดงใต้ preview

ตัวอย่างนี้สาธิตการแชร์เฟรมผ่าน IPC ระหว่าง core, การผสาน JPEG encoding, การจัดการ LVGL image source แบบ dynamic และขั้นตอนการจับภาพจากกล้อง เป็นพื้นฐานสำหรับแอปพลิเคชันขั้นสูงกว่า เช่น การสตรีมวิดีโอ (RTSP) และการตรวจจับใบหน้า

รองรับ TESAIoT Dev Kit และ PSoC Edge AI Kit
