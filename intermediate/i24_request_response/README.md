# IPC แบบ Request-Response (IPC Request-Response)

ตัวอย่างนี้สาธิตรูปแบบ IPC แบบ synchronous request-response พร้อม timeout คอร์ที่ร้องขอส่งคำสั่ง จากนั้นสำรวจ volatile flag จนกว่าการตอบกลับจะมาถึงหรือ timeout หมดอายุ นี่คือรูปแบบที่ `wifi_manager_is_connected()` และฟังก์ชัน query ที่คล้ายกันใช้

request ถูกส่งผ่าน `Cy_IPC_Pipe_SendMessage` และ response handler (registered callback) ตั้ง volatile flag และเก็บข้อมูลตอบกลับ task ที่ร้องขอจะวนตรวจ flag ในลูปแน่น พร้อม timeout ตาม FreeRTOS tick เพื่อป้องกันการบล็อกแบบไม่มีที่สิ้นสุด

timeout สำคัญสำหรับความทนทาน หากคอร์ปลายทางยุ่ง ค้าง หรือ IPC pipe ติดขัด timeout ทำให้ task ที่ร้องขอกู้คืนได้แทนที่จะค้างตลอด firmware จริงใช้ timeout 5 วินาทีสำหรับการ query สถานะ WiFi

ปุ่มทริกเกอร์ request และ UI แสดงค่าตอบกลับ เวลา round-trip เป็นมิลลิวินาที และว่า request timeout หรือไม่ รูปแบบนี้ควรใช้อย่างจำกัดเพราะบล็อก calling task รูปแบบ deferred flag (I21) เหมาะกว่าสำหรับสถานการณ์แบบ non-blocking

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
