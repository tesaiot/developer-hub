# CLI — ตัวอย่าง CLI ส่งข้อมูลผ่าน HTTPS Server-TLS

> **ภาษาไทย** - สำหรับเวอร์ชันภาษาอังกฤษ ดูที่ [README.md](README.md)

เครื่องมือ Python แบบ standalone สำหรับทดสอบการส่งเทเลเมทรีเข้าระบบ TESAIoT โดยใช้ Server-TLS bundle (CA + API key)

---

## 1. ไฟล์ในโฟลเดอร์

| File | รายละเอียด |
| --- | --- |
| `post_https_sample.py` | สคริปต์ Python 3 อ่าน bundle, สร้างเทเลเมทรีตัวอย่าง และส่ง `POST` ไปยัง `/api/v1/telemetry` |

---

## 2. วิธีใช้งานอย่างรวดเร็ว

1. ซิงค์ bundle ด้วย `make prepare` หรือคัดลอกไฟล์เข้าสู่ `../certs_credentials/`
2. ตรวจสอบว่า Python เวอร์ชัน 3.8 ขึ้นไปพร้อมใช้งาน (`python3 --version`)
3. เลือกคำสั่งด้านล่างเพื่อส่งเทเลเมทรี

```bash
cd tutorial/examples/device-to-platform/serverTLS/CLI_Python

# ส่งข้อความเดียวไปยังระบบจริง (https://tesaiot.com)
./post_https_sample.py

# สลับไปยัง staging และดู payload โดยไม่ส่งจริง
./post_https_sample.py --base-url https://staging.tesaiot.com --dry-run

# ส่งทุก 5 วินาที นาน 2 นาที
./post_https_sample.py --period 2 --interval 5
```

---

## 3. พารามิเตอร์สำคัญ

| Flag / env | ความหมาย |
| --- | --- |
| `--certs-dir`, `CERTS_DIR` | โฟลเดอร์เก็บ bundle (ค่าเริ่มต้น `../certs_credentials`) |
| `--base-url`, `BASE_URL` | เปลี่ยนฐาน URL หากไม่ได้ใช้ production |
| `--endpoint`, `ENDPOINT` | ระบุปลายทาง API (ค่าเริ่มต้น `/api/v1/telemetry`) |
| `--period`, `--interval` | ระยะเวลาส่งซ้ำเป็นนาที / วินาที |
| `--dry-run` | แสดง payload แล้วออก |

---

## 4. หมายเหตุเพิ่มเติม

- สคริปต์ยึดตาม Apache 2.0 license เหมือนไฟล์อื่น สามารถคัดลอกไปทดลองได้โดยคง header เอาไว้
- ฟังก์ชัน `resolve_base_url` จะพยายามอ่านข้อความจาก `endpoints.json` หากไม่ระบุ `--base-url`
- หาก TLS handshake ล้มเหลว ให้ตรวจสอบเวลาของเครื่องและสิทธิ์ไฟล์ CA (`600`)
