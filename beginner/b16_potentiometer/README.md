# เกจ Potentiometer (Potentiometer Gauge)

ตัวอย่างนี้อ่านค่า analog potentiometer บนบอร์ด Eva Kit และแสดงค่าใน Arc gauge Potentiometer ให้ค่า 0-4095 (ADC 12-bit) ที่ map กับตำแหน่งการหมุนจริง

Arc gauge ครอบคลุมช่วงค่า potentiometer ทั้งหมด โดย label ตรงกลางแสดงทั้งค่า ADC ดิบและเปอร์เซ็นต์ที่คำนวณ timer 100ms ให้การติดตามตำแหน่งปุ่มหมุนที่ลื่นไหล

Potentiometer เป็นอุปกรณ์ input แบบ analog ที่เข้าใจง่ายที่สุด ให้การ mapping ตรงจากกายภาพเป็นดิจิทัล เหมาะสำหรับควบคุมระดับเสียง ปรับความสว่าง และป้อนค่า set-point ด้วยมือในแอปพลิเคชันอุตสาหกรรม

ตัวอย่างนี้รองรับ TESAIoT Dev Kit และ PSoC Edge Eva Kit เท่านั้น (`#if BSP_HAS_POTENTIOMETER`) เนื่องจาก AI Kit ไม่มี potentiometer
