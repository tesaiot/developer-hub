# WiFi Settings (iOS-style Scan + Connect)

ตัวอย่างนี้สร้างหน้า "ตั้งค่า WiFi" สไตล์ iOS บนหน้าจอ โดยรวมสามส่วนเข้าด้วยกัน: การสแกนหาเครือข่ายแบบไม่บล็อก (non-blocking scan), รายการเครือข่ายพร้อมแท่งสัญญาณ (signal bars) และการเชื่อมต่อผ่านคีย์บอร์ดบนหน้าจอ

การสแกนใช้ `wifi_manager` แบบ non-blocking: เรียก `scan_start` แล้ว poll ด้วย LVGL timer เพื่ออ่าน `scan_ready` และดึงผล `scan_result` มาสร้างแถวรายการ แต่ละแถวแปลงค่า RSSI เป็นแท่งสัญญาณ 0-3 ขีด

หลักการสำคัญคือ **Deferred Timer Pattern**: `wifi_manager_connect()` เป็นการเรียกแบบ **บล็อก** (blocking) ที่ทำให้หน้าจอค้าง 10-35 วินาทีระหว่างเชื่อมต่อ จึงต้องเก็บ SSID และรหัสผ่านไว้ก่อน แล้ว defer การเชื่อมต่อไปยัง one-shot timer 50 มิลลิวินาที เพื่อให้ LVGL ได้ render สถานะ "กำลังเชื่อมต่อ" ก่อนที่ core จะถูกบล็อก รูปแบบเดียวกันนี้ใช้ได้กับทุกการเรียกที่บล็อกนาน: อัปเดตหน้าจอให้ผู้ใช้เห็นสถานะก่อน แล้วค่อยเรียกงานที่บล็อกใน timer รอบถัดไป

UI เป็น state machine หกสถานะ: `SCANNING` → `LIST` → `PASSWORD` → `CONNECTING` → `CONNECTED` หรือ `ERROR` เมื่อเชื่อมต่อล้มเหลวจะ auto-retry หนึ่งครั้ง เพราะ `cy_wcm_connect_ap` มีโอกาสล้มเหลวเป็นครั้งคราว การกรอกรหัสผ่านใช้ `lv_keyboard` ที่ผูกกับ `lv_textarea` (สูงสุด 63 อักขระ)

โครงสร้าง: CM55 ดูแลหน้าจอและ UI ส่วน CM33_NS ดูแล WiFi (CYW55513 ผ่าน SDHC0) โดย `wifi_manager` ห่อการสื่อสารข้ามคอร์ไว้ให้แล้ว

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
