# WiFi Connect via IPC

ตัวอย่างนี้แสดงการเชื่อมต่อ WiFi แบบ Cross-Core: CM55 (หน้าจอ) ส่งคำสั่ง IPC ไปยัง CM33_NS (ซึ่งควบคุม CYW55513 ผ่าน SDHC0) เพื่อสั่ง scan, connect และ disconnect โดย CM33_NS จะส่งสถานะกลับมาแบบ asynchronous ผ่าน IPC_CMD_WIFI_STATE_PUSH (0xD9)

หลักการสำคัญคือ **Deferred Flag Pattern**: เมื่อ IPC callback ทำงานใน ISR context จะเซ็ตเฉพาะ volatile flag เท่านั้น แล้ว LVGL timer ที่ทำงานใน task context จะอ่าน flag และอัปเดต UI อย่างปลอดภัย การเรียก `Cy_IPC_Pipe_SendMessage()` จาก ISR callback โดยตรงจะทำให้เกิด deadlock ถาวร

UI ประกอบด้วยการ์ดสถานะ (สี: เขียว=เชื่อมต่อ, เหลือง=กำลังเชื่อมต่อ, แดง=ตัดการเชื่อมต่อ) ช่องกรอก SSID และ Password พร้อมคีย์บอร์ดบนหน้าจอ ปุ่ม Connect/Disconnect และแสดง RSSI + IP address เมื่อเชื่อมต่อสำเร็จ

IPC payload ของคำสั่ง WIFI_CONNECT จัด format เป็น: data[0]=SSID length, data[1..32]=SSID, data[33]=password length, data[34..96]=password โดยบัฟเฟอร์ TX ต้องอยู่ใน shared SRAM (`CY_SECTION_SHAREDMEM`) เพื่อให้ทั้งสอง core เข้าถึงได้

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
