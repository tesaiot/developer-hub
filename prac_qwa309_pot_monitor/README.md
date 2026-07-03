# QWA309 — Potentiometer Monitor

## เกี่ยวกับตัวอย่างนี้
ตัวอย่างนี้อ่านค่าโพเทนชิโอมิเตอร์ 4 ตัว (VR1–VR4) บน TESAIoT Dev Kit ผ่าน AUTANALOG SAR ADC แบบ 12-bit ด้วยแรงดันอ้างอิง 1.8V แล้วนำค่าที่ได้มาแสดงบนจอ LVGL เป็นการ์ด 4 ใบ พร้อม bar, ค่าแรงดัน (V), เปอร์เซ็นต์ และค่าดิบ (raw) แบบเรียลไทม์ อัปเดตทุก 100 ms เป็น practise แรกของชุด QWA309 ที่ใช้ ADC จริงบนฮาร์ดแวร์ โดย UI module จะเริ่มต้น SAR ADC ด้วยตัวเอง (เฟรมเวิร์กหลักไม่ได้ยก SAR ให้)

## ฮาร์ดแวร์ที่ใช้
- **AUTANALOG SAR ADC** — โมดูล ADC ในตัวของ PSoC Edge อ่านสัญญาณอนาล็อกจากขาโพเทนชิโอมิเตอร์
- **VR1** — `P15.5` (ADC channel 5, index 1) เส้นสี teal
- **VR2** — `P15.4` (ADC channel 4, index 0) เส้นสีเขียว
- **VR3** — `P15.6` (ADC channel 6, index 2) เส้นสีเหลือง/ส้ม
- **VR4** — `P15.7` (ADC channel 7, index 3) เส้นสีแดง/ชมพู
- ทั้ง 4 ขาถูกตั้งเป็นโหมด `CY_GPIO_DM_ANALOG` ด้วย `Cy_GPIO_Pin_FastInit()` ก่อนเปิด SAR

## สิ่งที่จะได้เรียนรู้
- การเริ่มต้น AUTANALOG SAR ADC ด้วย `Cy_AutAnalog_Init()`, `Cy_AutAnalog_Enable()` และ `Cy_AutAnalog_StartAutonomousControl()` โดยใช้ config `autonomous_analog_init` ที่ BSP สร้างให้
- การตั้งขา GPIO ให้เป็น analog input ด้วย `Cy_GPIO_Pin_FastInit()` โหมด `CY_GPIO_DM_ANALOG`
- การอ่านผลลัพธ์ ADC ด้วย `Cy_AutAnalog_SAR_ReadResult()` และตรวจสถานะช่องด้วย `Cy_AutAnalog_SAR_GetHSchanResultStatus()`
- การแปลงค่าดิบ 12-bit (0–4095) เป็นแรงดัน mV และเปอร์เซ็นต์แบบ integer math (ไม่ใช้ float)
- การใช้ `lv_timer_create()` เพื่อ poll ADC ตามคาบเวลาแล้วอัปเดต UI
- การสร้าง `lv_bar` และ label หลายค่าในการ์ดด้วย LVGL flexbox layout

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ด้วย `BOARD=TESAIOT_DEV_KIT` (ตัวอย่าง QWA309 ใช้ base board — ลงได้เฉพาะ TESAIoT Dev Kit)

## สิ่งที่จะเห็นบนหน้าจอ
พื้นหลังจอสีเข้ม ด้านบนเป็น header ชื่อ "Pot Monitor" พร้อมสถานะ ADC ทางขวา (เปลี่ยนระหว่าง "Live" สี teal เมื่อทั้ง 4 ช่องพร้อม หรือ "ADC settling" สีเหลืองระหว่างรอค่านิ่ง) ถัดลงมาเป็นแถบสรุปแสดงช่วงแรงดัน "0.000-1.800 V" และคาบสุ่ม "100 ms" ส่วนกลางเป็นการ์ด 4 ใบจัดเป็นตาราง 2x2 แต่ละใบมีขอบสีประจำช่อง แสดงชื่อ VR/ขา, ค่าแรงดันตัวใหญ่, bar สีที่ยาวตามค่า, เปอร์เซ็นต์ และค่า raw เมื่อหมุนโพเทนชิโอมิเตอร์ ทุกค่าจะขยับตามทันที

## ลองปรับแต่ง
- ปรับ `POT_REFRESH_PERIOD_MS` (ค่าเริ่มต้น 100) ให้อัปเดตเร็วหรือช้าลง
- เปลี่ยนสีประจำช่องในตาราง `pot_channels[]` ผ่านฟิลด์ `accent`
- แก้ `POT_ADC_VREF_MV` (1800) หากใช้แรงดันอ้างอิงอื่น หรือ `POT_ADC_FULL_SCALE` (4095) หากเปลี่ยนความละเอียด ADC
- เพิ่มการเฉลี่ยค่า (moving average) ใน `update_channel()` เพื่อลด noise ของค่า raw
- เพิ่มการแจ้งเตือน/เปลี่ยนสี bar เมื่อค่าเกิน threshold ที่กำหนด
