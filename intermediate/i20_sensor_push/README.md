# ส่งข้อมูลเซ็นเซอร์ผ่าน IPC (Sensor Push via IPC)

ตัวอย่างนี้สาธิตรูปแบบการส่งข้อมูลเซ็นเซอร์ที่ใช้ทั่ว firmware: CM33 อ่านฮาร์ดแวร์เซ็นเซอร์ แพ็คค่าลงในโครงสร้าง IPC message และส่งไปยัง CM55 เพื่อแสดงผล นี่คือรูปแบบ `ipc_sensorhub` มาตรฐาน

ฝั่ง CM33 (แสดงเชิงแนวคิด) อ่าน BMI270 และแพ็ค accelerometer XYZ ลงใน `ipc_msg_t.data[]` array เป็นจำนวนเต็ม 16-bit แบบมีเครื่องหมายในรูปแบบ little-endian ฝั่ง CM55 ลงทะเบียน callback สำหรับ `IPC_CMD_SENSOR_DATA` และแกะค่าลงใน volatile shared buffer

LVGL timer บน CM55 อ่าน shared buffer เป็นระยะและอัปเดต label ค่า 3 ตัวบนหน้าจอ callback เขียนลง buffer เท่านั้น (เร็ว ไม่บล็อก) LVGL timer จัดการการอัปเดต widget ทั้งหมดจาก GFX task context ที่การเรียก LVGL ปลอดภัย

รูปแบบนี้ทำให้แยกระหว่าง sensor I/O (โดเมน CM33 เข้าถึง bus ได้) และการ render จอแสดงผล (โดเมน CM55 เข้าถึง GPU ได้) ได้สะอาด การข้ามขอบเขตนี้ไม่ถูกต้องจะเกิด BusFault ซึ่งเป็นเหตุผลที่ IPC bridge มีอยู่

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
