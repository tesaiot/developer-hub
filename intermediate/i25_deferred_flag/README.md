# รูปแบบ Deferred Flag (Deferred Flag Pattern)

นี่คือรูปแบบ IPC ที่สำคัญที่สุดใน firmware ของ PSoC Edge สาธิตวิธีตอบกลับ IPC message อย่างปลอดภัยโดยไม่ทำให้เกิด deadlock ถาวร กฎนี้เป็นสิ่งที่ต้องปฏิบัติอย่างเคร่งครัด: ห้ามเรียก `Cy_IPC_Pipe_SendMessage` จากภายใน `RegisterCallback` handler เด็ดขาด

IPC pipe เป็นทรัพยากรแบบ single-owner เมื่อ callback handler ทำงาน pipe lock ถูกถืออยู่ หาก handler เรียก SendMessage มันจะพยายามขอ pipe lock เดียวกัน ทำให้เกิด deadlock ทันทีและถาวร จอแสดงผลค้าง สัมผัสหยุดตอบสนอง และต้อง power cycle เท่านั้นจึงกู้คืนบอร์ดได้

ทางออกคือรูปแบบ deferred flag: ISR callback ตั้ง `volatile bool` flag และเก็บข้อมูลตอบกลับใน volatile structure main task loop (หรือ LVGL timer) ตรวจ flag และเมื่อถูกตั้ง จะเรียก SendMessage จาก task context ที่ไม่ได้ถือ pipe lock

ตัวอย่างนี้แสดงรูปแบบด้วย message log ที่แสดงลำดับ: RECEIVE (ISR ตั้ง flag) จากนั้น REPLY (task ส่งข้อความ) ตัวนับติดตาม round-trip ที่สำเร็จเพื่อยืนยันว่ารูปแบบทำงานได้โดยไม่เกิด deadlock รูปแบบนี้ถูกค้นพบหลังเหตุการณ์ IPC deadlock วันที่ 2026-03-10

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
