# QWA309 — Header I/O Test

## เกี่ยวกับตัวอย่างนี้
ตัวอย่างนี้เป็นเครื่องมือ diagnostic สำหรับตรวจสอบขา I/O บน Arduino header ของบอร์ด QWA309 Dev Kit ให้ครบทุกเส้นทาง ทั้ง I2C, UART, SPI, GPIO, PWM และเน็ต ADC โดยสร้างหน้าจอ console แบบเทอร์มินัลด้วย LVGL พร้อมปุ่มทดสอบแยกแต่ละบัส ผู้เรียนกดปุ่มเพื่อรันการทดสอบทีละอย่าง แล้วอ่านผล PASS / PARTIAL / FAIL พร้อม log รายละเอียดบนจอ การทดสอบบางส่วน (UART/SPI/I2C echo, GPIO, PWM) ต้องต่อสายกับบอร์ด ESP32-S3 companion ที่รันเฟิร์มแวร์ simulator เป็นคู่ทดสอบ

## ฮาร์ดแวร์ที่ใช้
- **I2C controller (SCB ของจอ/ทัชสกรีน)** — ใช้ `DISPLAY_I2C_CONTROLLER_HW` สแกนที่อยู่ `0x08`–`0x77` และคุยกับ ESP32-S3 simulator ที่ address `0x30`
- **UART (SCB9)** — `P15.1` = TX, `P15.0` = RX ที่ 115200 8N1 (init ผ่าน `Cy_SCB_UART_Init`)
- **SPI แบบ bit-bang (mode 0)** — `P9.3` = SCK, `P9.2` = MOSI, `P9.1` = MISO, `P9.0` = CS
- **GPIO 6 เส้น** — `P13.0`, `P13.3`, `P13.4`, `P13.5`, `P13.6`, `P13.7` ใช้ทั้ง input-watch และ output-drive
- **PWM5 complementary** — `P13.3` (PWM5+) / `P13.4` (PWM5-) เป็นคู่สัญญาณดิจิทัลกลับเฟส
- **เน็ต ADC / PWM3** — `P15.2` (ADC2 / PWM3+) / `P15.3` (ADC3 / PWM3-) ทดสอบเป็น digital level input และ complementary output

## สิ่งที่จะได้เรียนรู้
- การใช้ PDL SCB I2C แบบ manual (`Cy_SCB_I2C_MasterSendStart` / `MasterWriteByte` / `MasterReadByte` / `MasterSendStop`) เพื่อ probe และ scan บัส
- การ init และรับ-ส่งข้อมูล UART ด้วย `Cy_SCB_UART_PutArrayBlocking` พร้อม timeout และ retry
- การเขียน SPI แบบ bit-bang ด้วย `Cy_GPIO_Write` / `Cy_GPIO_Read` และหน่วงเวลา `Cy_SysLib_DelayUs`
- การคุมขา GPIO ด้วย `Cy_GPIO_Pin_FastInit` (HIGHZ / STRONG) และการอ่าน mask แบบ one-hot walking pattern
- การสร้างสัญญาณ PWM complementary ด้วยการสลับ logic บนคู่ขา และการตรวจ both-high fault
- การทำ protocol เล็ก ๆ ด้วย magic byte + counter + XOR checksum เพื่อยืนยันคำตอบจาก ESP32
- การสร้าง console บนจอด้วย `lv_textarea_create()` + ring buffer และรัน test แบบ non-blocking ผ่าน `lv_timer_create()`

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ด้วย `BOARD=TESAIOT_DEV_KIT` (ตัวอย่าง QWA309 ใช้ base board — ลงได้เฉพาะ TESAIoT Dev Kit)
3. ต่อสายจาก header ไปยังบอร์ด ESP32-S3 companion ตามที่ log บนจอบอกในแต่ละการทดสอบ

## สิ่งที่จะเห็นบนหน้าจอ
หน้าจอพื้นหลังสีเข้ม (`0x101418`) มีแถบหัวด้านบนแสดงชื่อ "Header Tester" และป้ายสถานะ (Ready / Scanning… / PASS / FAIL) ทางขวา ถัดมาเป็นแถวปุ่มสีต่าง ๆ ได้แก่ Clear, Scan, I2C ESP32, UART Echo, SPI ESP32, GPIO In, GPIO Out, PWM Out, ADC In และ PWM3 Out ส่วนล่างเป็นกล่อง console สีเขียวอ่อนบนพื้นดำที่พิมพ์ log แบบมี timestamp (`[xxxx ms]`) และเลื่อนลงไปบรรทัดล่างสุดอัตโนมัติ กดปุ่มใดจะ disable ปุ่มอื่นชั่วคราวระหว่างรัน แล้วสรุปผลเป็น PASS/PARTIAL/FAIL ที่ท้าย log

## ลองปรับแต่ง
- ปรับช่วงสแกน I2C ด้วย `I2C_SCAN_MIN_ADDR` / `I2C_SCAN_MAX_ADDR` หรือเปลี่ยน address จำลองด้วย `ESP32_SIM_ADDR` (0x30)
- เปลี่ยนจำนวนรอบและอัตราสุ่มของ GPIO/ADC watch ผ่าน `GPIO_WATCH_SAMPLE_COUNT`, `GPIO_WATCH_SAMPLE_MS`, `ADC_LEVEL_SAMPLE_COUNT`
- ปรับความถี่ PWM ด้วย `PWM_TEST_HALF_PERIOD_MS` และจำนวนรอบด้วย `PWM_TEST_CYCLES` (ค่าเริ่มต้น ~25 Hz, 50 รอบ)
- ปรับจำนวน retry/timeout ของ UART/SPI/I2C echo (`UART_TEST_RETRIES`, `UART_TEST_TIMEOUT_MS`, `ESP32_SIM_READ_RETRIES`)
- ขยายขนาด `CONSOLE_BUFFER_SIZE` (4096) หากต้องการเก็บ log ย้อนหลังมากขึ้น
