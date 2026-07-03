# QWA309 — Push Button Monitor

## เกี่ยวกับตัวอย่างนี้
ตัวอย่างนี้อ่านสถานะปุ่มกดภายนอกสองปุ่มบนบอร์ด QWA309 Dev Kit ผ่าน GPIO แบบ active-low (P17.5 และ P17.7) โดยเปิด internal pull-up ให้แต่ละขา แล้วนำสถานะกด/ปล่อยไปแสดงบนหน้าจอ LVGL แบบเรียลไทม์ พร้อมนับจำนวนครั้งที่กดและจับเวลาที่กดค้าง ปุ่มทั้งสองเป็น GPIO ล้วน โมดูล UI จะ init ขาเองโดยไม่ต้องพึ่ง design.modus

## ฮาร์ดแวร์ที่ใช้
- **ปุ่มกด SW5 — P17.7** — อ่านผ่าน `Cy_GPIO_Read()` แบบ active-low, สีเน้น (accent) เขียว `0x22C55E`
- **ปุ่มกด SW6 — P17.5** — อ่านผ่าน `Cy_GPIO_Read()` แบบ active-low, สีเน้น (accent) ฟ้า `0x38BDF8`
- ทั้งสองขาตั้งเป็น input pull-up ด้วย `Cy_GPIO_Pin_FastInit(..., CY_GPIO_DM_PULLUP, 1UL, HSIOM_SEL_GPIO)` ค่าปกติเป็น HIGH และกลายเป็น LOW เมื่อกดปุ่ม

## สิ่งที่จะได้เรียนรู้
- การตั้งค่า GPIO เป็น input pull-up ด้วย `Cy_GPIO_Pin_FastInit()`
- การอ่านสถานะปุ่มแบบ active-low ด้วย `Cy_GPIO_Read()` (ค่า `0` = กด)
- เทคนิค debounce แบบนับ tick (`BUTTON_DEBOUNCE_TICKS = 2`) เพื่อกรองสัญญาณเด้ง
- การใช้ `lv_timer_create()` สุ่มอ่านปุ่มทุก `BUTTON_REFRESH_PERIOD_MS = 25` ms
- การนับจำนวนครั้งที่กด (`press_count`) และสะสมเวลาที่กดค้าง (`hold_time_ms`)
- การอัปเดตสี card/indicator/label ตามสถานะปุ่มด้วย LVGL

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ด้วย `BOARD=TESAIOT_DEV_KIT` (ตัวอย่าง QWA309 ใช้ base board — ลงได้เฉพาะ TESAIoT Dev Kit) และต้องต่อปุ่มภายนอกเข้าที่ P17.5 และ P17.7

## สิ่งที่จะเห็นบนหน้าจอ
พื้นหลังจอสีเข้ม (`0x0B0F14`) มี title "Button Monitor" ด้านบน ตามด้วยบรรทัดสรุปสถานะ "SW5 / SW6 released" ถัดลงมาเป็นการ์ดสองใบเรียงแนวนอน แต่ละใบแทนหนึ่งปุ่ม แสดงชื่อปุ่ม, เลขขา, วงกลม indicator กลางการ์ด และข้อความ "RELEASED"/"PRESSED" เมื่อกดปุ่ม การ์ดจะเปลี่ยนสีขอบและ indicator เป็นสี accent ของปุ่มนั้น พร้อมอัปเดต "Level LOW active", "Presses N" และ "Hold N ms" ด้านล่างบรรทัดสรุปจะเปลี่ยนเป็น "Both buttons pressed" (เหลือง) เมื่อกดทั้งคู่ หรือ "SWx pressed" เมื่อกดปุ่มเดียว

## ลองปรับแต่ง
- ปรับ `BUTTON_REFRESH_PERIOD_MS` (25 ms) ให้อ่านถี่ขึ้นหรือช้าลง — มีผลต่อความละเอียดของ hold time ด้วย
- ปรับ `BUTTON_DEBOUNCE_TICKS` (2) เพื่อเพิ่ม/ลดความเข้มงวดในการกรองสัญญาณเด้ง
- เปลี่ยนสี accent ของแต่ละปุ่ม (`0x22C55E`, `0x38BDF8`) ให้เข้ากับธีมที่ต้องการ
- เพิ่มปุ่มที่สามโดยขยาย `BUTTON_COUNT` และเพิ่มสมาชิกในอาร์เรย์ `buttons[]` พร้อมกำหนด port/pin ใหม่
- เพิ่มการตรวจจับ long-press โดยเช็ก `hold_time_ms` เกิน threshold แล้วทำ action พิเศษ
