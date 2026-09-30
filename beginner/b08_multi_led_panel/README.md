# แผงควบคุม LED หลายตัว (Multi LED Control Panel)

ตัวอย่างนี้สร้างแผงควบคุมแบบ grid สำหรับ LED ทั้งหมดที่มีบนบอร์ด AI Kit มี LED 5 ตัว (Red, Green, Blue, Orange, White) ในขณะที่ Eva Kit มี 2 ตัว BSP guard macro เลือกการกำหนดค่าที่ถูกต้องขณะ compile

แต่ละแถว LED มีชื่อ LED, ตัวบ่งชี้ Virtual LED และสวิตช์สำหรับเปิดปิด grid ใช้ `LV_LAYOUT_GRID` สำหรับการจัดตำแหน่งที่เรียบร้อย สวิตช์ทั้งหมดใช้ event callback เดียวที่ระบุ LED เป้าหมายผ่าน user data pointer

ขา GPIO เฉพาะบอร์ดกำหนดภายใน `#if` preprocessor guard AI Kit ใช้ขา P13_4 ถึง P13_7 และ P14_0 ในขณะที่ Eva Kit ใช้ P13_7 และ P14_0 การเลือกขณะ compile นี้รับรองการ mapping ฮาร์ดแวร์ที่ถูกต้องบนแต่ละบอร์ด

ปุ่ม "All ON" และ "All OFF" ด้านล่างให้การควบคุมทั้งหมดพร้อมกัน รูปแบบแผงนี้ใช้ซ้ำได้สำหรับแอปพลิเคชัน digital output หลายช่องใดๆ รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
