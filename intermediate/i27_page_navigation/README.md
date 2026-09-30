# การนำทางระหว่างหน้า (Page Navigation)

ตัวอย่างนี้สาธิตการนำทางไปข้างหน้าและย้อนกลับระหว่าง 2 หน้าโดยใช้ `pm_navigate` และ `pm_back` หน้า A มีปุ่ม "Go to B" และหน้า B มีปุ่ม "Back to A" แสดงพฤติกรรม navigation stack มาตรฐาน

เมื่อเรียก `pm_navigate` destroy callback ของหน้าปัจจุบันจะทำงาน จากนั้น create callback ของหน้าเป้าหมายจะรัน page manager รักษา navigation stack ไว้เพื่อให้ `pm_back` กลับไปยังหน้าก่อนหน้าโดยไม่ต้องระบุ ID อย่างชัดเจน

ทั้งสองหน้ามีสไตล์ภาพที่ต่างกัน (สีพื้นหลังและชื่อต่างกัน) เพื่อให้เห็นการเปลี่ยนผ่านการนำทางชัดเจน แต่ละหน้ายังติดตามและแสดงจำนวนครั้งที่ถูกเยี่ยมชม โดยเก็บตัวนับใน static variable ข้ามรอบ create/destroy

ตัวอย่างนี้จำลองการนำทางภายใน parent widget แบบ self-contained เนื่องจากรันเป็นตัวอย่างแบบ standalone ใน firmware จริง `pm_navigate` และ `pm_back` ถูกเรียกกับ global page manager instance `s_pm` และ page manager จัดการ widget tree ทั้งหมด

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
