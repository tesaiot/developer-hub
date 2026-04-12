# Space Shooter

## เกี่ยวกับตัวอย่างนี้
เกมยิงอวกาศที่ทำงานบนจอทัชสกรีนของ TESAIoT Dev Kit ยานขนาด 44x20 พิกเซลพร้อมปีกและห้องนักบิน กระสุน pool 6 ลูก ศัตรู pool 8 ตัวพร้อมตาคู่ มีระบบชีวิต 3 หัวใจ ความยากเพิ่มขึ้นเรื่อยๆ ใช้ AABB collision detection

## สิ่งที่จะได้เรียนรู้
- Entity pool pattern (pre-allocated bullets[6] + enemies[8])
- ระบบ spawn cooldown และ progressive difficulty
- AABB collision detection หลายชั้น (bullet-enemy, ship-enemy, enemy-past-bottom)
- การสร้าง sprite จากหลาย LVGL objects (ยาน: ลำตัว+ปีก+ห้องนักบิน)
- ระบบ lives/health และ game over condition
- การใช้ D-pad L/R + Action button สำหรับ move + fire

## วิธีติดตั้ง
1. คัดลอกไฟล์ทั้งหมดในโฟลเดอร์นี้ไปที่ `proj_cm55/apps/`
2. Build และ flash ตามปกติ

## สิ่งที่จะเห็นบนหน้าจอ
สนามเกมสีเขียว Game Boy พร้อมยานอวกาศด้านล่าง ศัตรูตกลงมาจากด้านบนด้วยความเร็วต่างกัน ใช้ D-pad ซ้าย-ขวาเพื่อขยับยาน กดปุ่ม A เพื่อยิงกระสุน ทำลายศัตรูให้มากที่สุด

## ลองปรับแต่ง
- เพิ่มจำนวนกระสุนพร้อมกันด้วย `SHOOT_BULLET_MAX`
- ปรับความเร็วศัตรู range `lv_rand(18, 34)` ใน `shooter_spawn_enemy()`
- เปลี่ยน spawn cooldown `18U` เพื่อปรับความถี่ศัตรู (ค่าน้อย = ศัตรูมากขึ้น)
- เพิ่มศัตรูหลายแบบที่มีขนาดและ HP ต่างกัน
- เพิ่มระบบ power-up เช่น กระสุนกว้างขึ้น หรือยิงหลายลูกพร้อมกัน
