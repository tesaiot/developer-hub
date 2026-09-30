# แถบสถานะ GPIO (GPIO Status Bar)

ตัวอย่างนี้สร้างแถบสถานะครบถ้วนที่ monitor GPIO input และ output ทั้งหมด สถานะปุ่ม (SW1, SW2) และสถานะ LED แสดงในแถบแนวนอนด้านบนหน้าจอ คล้ายตัวบ่งชี้สถานะระบบ

แต่ละขา GPIO แสดงด้วยจุดสีเล็กและ label ขา input แสดงสถานะ real-time ผ่านการ poll ด้วย timer ทุก 100ms ขา output แสดงสถานะล่าสุดที่เขียน แถบสถานะใช้ flex row layout สำหรับการจัดเรียงแนวนอนที่กะทัดรัด

ด้านล่างแถบสถานะ ปุ่ม toggle ให้เปลี่ยนสถานะ LED ขณะที่แถบสถานะอัปเดตแบบ real-time สร้าง interface ทั้ง monitor และควบคุมแบบสองทิศทาง

รูปแบบนี้ใช้กันทั่วไปสำหรับหน้าจอวินิจฉัยฮาร์ดแวร์และ dashboard สถานะระบบ รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
