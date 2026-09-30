# IPC พื้นฐาน (Basic IPC)

ตัวอย่างนี้สาธิตกลไกการสื่อสารระหว่างโปรเซสเซอร์พื้นฐานระหว่างคอร์ CM33 และ CM55 โดยใช้ `Cy_IPC_Pipe_SendMessage` และ `Cy_IPC_Pipe_RegisterCallback` ตัวนับ ping-pong แบบง่ายเพิ่มค่าทุกครั้งที่มีการแลกเปลี่ยนข้อความ

ฝั่ง CM55 ลงทะเบียน callback ที่ทำงานเมื่อข้อความมาถึงจาก CM33 callback แยกค่าตัวนับจากข้อความ เพิ่มค่า และตั้ง flag สำหรับส่งตอบกลับ การตอบกลับถูกส่งจาก main task loop ไม่ใช่จาก callback เพื่อหลีกเลี่ยง IPC pipe deadlock

นี่คือรูปแบบ IPC ที่สำคัญที่สุดที่ต้องเข้าใจ: ห้ามเรียก `Cy_IPC_Pipe_SendMessage` จากภายใน `RegisterCallback` handler เด็ดขาด IPC pipe เป็นทรัพยากรแบบ single-owner และการเรียก SendMessage ขณะที่ handler ถือ pipe lock จะทำให้เกิด deadlock ถาวรที่ทำให้จอแสดงผลค้าง

UI แสดงจำนวน ping-pong ปัจจุบันและทิศทางข้อความล่าสุด (CM33-ไป-CM55 หรือ CM55-ไป-CM33) ปุ่ม "Send Ping" เริ่มข้อความแรก ตัวอย่างนี้เป็นพื้นฐานของตัวอย่าง IPC อื่นๆ ทั้งหมดในชุดนี้

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
