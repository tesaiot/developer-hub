# HSM PIN Entry Overlay

ตัวอย่างนี้สร้างหน้าจอกด PIN ระดับ production ที่ดัดแปลงจาก `page_hsm.c` ของ firmware จริง ประกอบด้วยแป้นพิมพ์ตัวเลข 3x4, การแสดง PIN แบบจุด, shake animation เมื่อกดผิด, green flash เมื่อถูกต้อง และระบบล็อกหลังป้อนผิด 3 ครั้ง

PIN display ใช้จุดวงกลม 4 จุดที่เปลี่ยนสีเมื่อกรอก: ว่าง=วงแหวนม่วง, กรอกแล้ว=ม่วงทึบ, ถูกต้อง=เขียว, ล็อก=แดง เมื่อกรอกครบ 4 หลักจะ auto-submit ทันที ถ้า PIN ผิดจะเล่น lateral shake animation (lv_anim) เพื่อให้ผู้ใช้เห็นชัดว่าผิด

ระบบล็อก: ป้อนผิด 3 ครั้งจะแสดงการ์ดล็อกสีแดงพร้อมตัวนับถอยหลัง 30 วินาที ปุ่มกดจะถูกซ่อนจนกว่าจะหมดเวลาล็อก ป้องกันการโจมตี brute-force

ใน production จริง PIN hash จะถูกส่งไปยัง CM33_NS ผ่าน IPC_CMD_OPTIGA_VERIFY_PIN (0xE5) เพื่อตรวจสอบด้วย OPTIGA Trust M SHA-256 — PIN เป็น plaintext จะไม่ถูกเก็บใน flash เลย ตัวอย่างนี้ใช้ FNV-1a hash สำหรับ demo แบบ standalone โดยมี code reference ของ production path เป็นคอมเมนต์ไว้

CY_SECTION_SHAREDMEM ใช้สำหรับ IPC TX buffer เพื่อให้ทั้ง CM55 และ CM33_NS เข้าถึง shared SRAM ได้

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
