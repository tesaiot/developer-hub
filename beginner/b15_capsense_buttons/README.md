# ปุ่ม CapSense (CapSense Buttons)

ตัวอย่างนี้อ่านปุ่มสัมผัส CapSense และ linear slider จากบอร์ด Eva Kit ปุ่มสัมผัสแบบ capacitive ตรวจจับนิ้วที่เข้าใกล้โดยไม่ต้องกดทางกายภาพ และ slider ให้ค่าตำแหน่ง

แต่ละปุ่ม CapSense ถูกแสดงเป็น Virtual LED ที่สว่างเมื่อถูกสัมผัส ตำแหน่ง slider แสดงเป็น LVGL Slider widget และ label ค่าตัวเลข timer 50ms poll ฮาร์ดแวร์ CapSense เพื่อการตอบสนองสัมผัสที่ไว

CapSense เป็นเทคโนโลยีการรับรู้แบบ capacitive โดย Infineon ที่เปิดใช้ interface สัมผัสโดยไม่ต้องใช้สวิตช์เชิงกล Eva Kit มี CapSense widget พร้อมปุ่มหลายตัวและ linear slider

ตัวอย่างนี้รองรับ TESAIoT Dev Kit และ PSoC Edge Eva Kit เท่านั้น (`#if BSP_HAS_CAPSENSE`) เนื่องจาก AI Kit ไม่มีฮาร์ดแวร์ CapSense
