# คอนโซลบนจอ LCD (Rich Text LCD Console)

ตัวอย่างนี้สร้างคอนโซลแบบเทอร์มินัลบนจอ AMOLED สำหรับแสดงข้อความ log แบบเรียลไทม์ เหมาะสำหรับใช้เป็น visual debug output ในระหว่างพัฒนาโดยไม่ต้องพึ่ง serial console

ระบบ log แบ่งเป็น 4 ระดับ: INFO ([I] สีเขียว), WARN ([W] สีส้ม), ERROR ([E] สีแดง) และ DEBUG ([D] สีเทา) ทุกบรรทัดมี timestamp จาก `xTaskGetTickCount()` ในรูปแบบ MM:SS.mmm ข้อมูลเก็บใน ring buffer ขนาด 50 บรรทัด เมื่อเต็มจะเขียนทับบรรทัดเก่าที่สุดโดยอัตโนมัติ

ฟังก์ชัน `console_print(level, fmt, ...)` ใช้ได้เหมือน printf — format ข้อความ เพิ่ม prefix ระดับ log และ timestamp แล้วใส่ลง ring buffer พร้อม auto-scroll ไปบรรทัดล่างสุดเสมอ

ตัวอย่าง demo จะสร้างข้อความ log ทุก 500ms จำลองข้อมูลจากเซ็นเซอร์ (BMI270, DPS368, BMM350), สถานะ IPC, WiFi RSSI, heap usage และ LVGL performance เพื่อแสดงการใช้งานจริงของแต่ละ log level

รองรับทุกบอร์ด: TESAIoT Dev Kit, PSoC Edge AI Kit, Eva Kit และ Game Console
