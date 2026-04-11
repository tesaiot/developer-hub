# Mic UI Module

โฟลเดอร์นี้เก็บโมดูล UI ของ EP06

- `mic_presenter.*` รับข้อมูลจาก logger แล้วอัปเดต view ผ่าน LVGL timer
- `mic_view.*` สร้าง/อัปเดตหน้าจอแบบ `Header + Content + Footer`

หมายเหตุ: UI ตัวอย่างนี้เป็น read-only monitor (ไม่มี input event)
