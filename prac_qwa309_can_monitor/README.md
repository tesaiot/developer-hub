# QWA309 — CAN Bus Monitor

## เกี่ยวกับตัวอย่างนี้
ตัวอย่างนี้ตั้งค่า CANFD0 ให้ทำงานในโหมด Classic CAN 2.0A ที่ความเร็ว 500 kbps บนแกน CM55 แบบ polled ล้วน (ไม่ใช้ NVIC/ISR) จึงรันอยู่ภายใน gfx task ของ LVGL ได้เลย โปรแกรมจะส่งเฟรม heartbeat ออกไป 1 เฟรมต่อวินาที (ID `0x123`, DLC 8, payload เป็นตัวนับ) เพื่อพิสูจน์ว่า CANFD0 ทำงานได้ พร้อมกับ poll RX FIFO 0 ทุก tick แล้วนำเฟรมล่าสุดมาแสดงบนตาราง LVGL สัญญาณต่อออกไปที่ทรานซีฟเวอร์ SN65HVD230 (U13) ผ่านขา P16.2 (RX) และ P16.3 (TX)

## ฮาร์ดแวร์ที่ใช้
- **CANFD0 channel 1** — ตัวคอนโทรลเลอร์ CAN บน PSoC Edge ตั้งค่าเป็น Classic CAN 2.0A ที่ 500 kbps (prescaler 10, TS1 15, TS2 4, SJW 4 จาก clock 100 MHz)
- **P16.2** — สาย RX (HSIOM `P16_2_CANFD0_TTCAN_RX1`, drive mode HIGHZ)
- **P16.3** — สาย TX (HSIOM `P16_3_CANFD0_TTCAN_TX1`, drive mode STRONG)
- **SN65HVD230 (U13)** — CAN transceiver แปลงเป็นคู่สาย CANH/CANL ต้องมี terminator 120Ω (jumper ที่ P9) เมื่อต่อเข้าบัสจริง

## สิ่งที่จะได้เรียนรู้
- การตั้งค่า CANFD0 ด้วยโค้ดล้วนผ่าน PDL (`Cy_CANFD_Init`) โดยไม่แตะ Device Configurator
- การกำหนด bit timing ของ CAN (prescaler / timeSegment1 / timeSegment2 / syncJumpWidth) ให้ได้ 500 kbps
- การจ่าย peripheral clock ให้ CANFD ด้วย `Cy_SysClk_PeriPclkSetDivider` / `AssignDivider` / `EnableDivider`
- การตั้ง GPIO HSIOM ให้ขา P16.2/P16.3 ทำหน้าที่เป็น CAN RX/TX
- การส่งเฟรมด้วย `Cy_CANFD_UpdateAndTransmitMsgBuffer` และการ poll เฟรมเข้าด้วย `Cy_CANFD_GetFIFOTop` + `Cy_CANFD_AckRxFifo`
- เทคนิค one-shot TX โดยเซ็ตบิต DAR (Disable Automatic Retransmission) ให้ตัวนับ TX เดินหน้าได้แม้ยังไม่มี node อื่นบนบัสมา ACK
- การผูก logic CAN เข้ากับ `lv_timer_create()` เพื่ออัปเดตหน้าจอเป็นระยะ

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ด้วย `BOARD=TESAIOT_DEV_KIT` (ตัวอย่าง QWA309 ใช้ base board — ลงได้เฉพาะ TESAIoT Dev Kit)

## สิ่งที่จะเห็นบนหน้าจอ
พื้นหลังสีเข้ม (`0x0B0F14`) มีหัวข้อ "CAN Monitor (CANFD0 ch1, 500 kbps)" ด้านบน ตามด้วยบรรทัดสถานะ — ถ้า init สำเร็จจะขึ้น "Online P16.2 RX / P16.3 TX SN65HVD230" เป็นสีเขียวมิ้นต์ ถ้าล้มเหลวจะขึ้น "CANFD0 init FAILED" สีแดง ถัดลงมาเป็นการ์ด 2 ใบวางเรียงกัน การ์ดซ้าย "Transmit (1 Hz)" ขอบเขียวแสดงจำนวนเฟรมที่ส่ง (TX frames) และ payload ล่าสุดในรูป `ID 0x123 XX XX ...` การ์ดขวา "Receive (FIFO0)" ขอบฟ้าแสดงจำนวนเฟรมที่รับ และเฟรมล่าสุด หากยังไม่มี node อื่นบนบัสจะขึ้น "waiting for a peer node..." ตัวนับ TX จะเพิ่มขึ้นทีละ 1 ทุกวินาที ส่วน RX จะอัปเดตเมื่อมี CAN peer หรือ USB-CAN analyzer ต่อเข้ามา

## ลองปรับแต่ง
- เปลี่ยน `CAN_TX_ID` (0x123) หรือ `CAN_TX_DLC` (8) เพื่อลองส่ง ID / ความยาว payload อื่น
- ปรับ `CAN_REFRESH_PERIOD_MS` (250 ms) และ `CAN_TX_EVERY_TICKS` (4 tick = 1 วินาที) เพื่อเปลี่ยนอัตรา TX heartbeat
- ปรับกลุ่ม `CANBUS_BITRATE_*` (prescaler/TS1/TS2/SJW) เพื่อทดลอง baud rate อื่น เช่น 250 kbps หรือ 1 Mbps
- ลองปิดบิต DAR (บรรทัด `CCCR |= ... DAR_Msk`) เพื่อดูพฤติกรรม auto-retransmission เมื่อไม่มี ACK บนบัส
- เพิ่ม SID filter (`numberOfSIDFilters`) เพื่อรับเฉพาะบาง message ID แทนที่จะรับทุกเฟรมเข้า RX FIFO 0
