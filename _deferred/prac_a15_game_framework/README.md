# Game Framework Demo

## เกี่ยวกับตัวอย่างนี้
ตัวอย่างสาธิตโครงสร้างพื้นฐานของ game_common framework ที่ใช้ร่วมกันในเกม Flappy Bird, Snake, Pong และ Space Shooter แสดง Game Boy 4-tone palette, CRT scanline overlay, Touch overlay controls และ live input state display ในหน้าจอเดียว

## สิ่งที่จะได้เรียนรู้
- โครงสร้าง game_common.h / game_common.c ที่เป็น reusable framework
- Game Boy DMG 4-tone palette (DARKEST, DARK, LIGHT, LIGHTEST)
- การสร้าง CRT scanline overlay ด้วย `game_add_lcd_scanlines()`
- ระบบ Touch overlay controls: D-pad, Action button, Restart button
- การอ่าน input state ผ่าน `game_input_read()` แบบ unified
- การใช้ `lv_timer_create()` สำหรับ polling input state

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ตามปกติ

## สิ่งที่จะเห็นบนหน้าจอ
หน้าจอแบ่ง 4 ส่วน: (1) สี่เหลี่ยม 4 สี Game Boy palette (2) แผง CRT scanline พร้อม sprite ตัวอย่าง (3) การ์ดแสดงสถานะ input แบบ live (4) ข้อความอธิบาย API ของ framework มี touch controls ครบชุด (D-pad + A + Restart)

## ลองปรับแต่ง
- เปลี่ยนค่าสี palette ใน `game_common.h` เพื่อสร้างธีมใหม่
- ปรับ scanline spacing จาก 8px เป็นค่าอื่นใน `game_add_lcd_scanlines()`
- เพิ่ม sprite ตัวอย่างใหม่ในแผง CRT (เช่น งู หรือไม้ตีปิงปอง)
- ปรับขนาดปุ่ม touch overlay ที่ `CTRL_BTN_SZ` และ `CTRL_ACT_SZ`
- ใช้ framework นี้เป็นฐานสร้างเกมใหม่ของตัวเอง
