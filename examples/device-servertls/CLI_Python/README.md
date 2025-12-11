# CLI — HTTPS Server-TLS Sample / ตัวอย่าง CLI ส่งข้อมูลผ่าน HTTPS Server-TLS

> 🇹🇭 เครื่องมือ Python แบบ standalone สำหรับทดสอบการส่งเทเลเมทรีเข้าระบบ TESAIoT โดยใช้ Server-TLS bundle (CA + API key)
>
> 🇬🇧 Standalone Python helper that posts telemetry to TESAIoT over HTTPS using the Server-TLS bundle (CA + API key).

---

## 1. Files / ไฟล์ในโฟลเดอร์

| File | 🇹🇭 รายละเอียด | 🇬🇧 Description |
| --- | --- | --- |
| `post_https_sample.py` | สคริปต์ Python 3 อ่าน bundle, สร้างเทเลเมทรีตัวอย่าง และส่ง `POST` ไปยัง `/api/v1/telemetry` | Python 3 script that loads the bundle, generates schema-aligned telemetry, and POSTs it to `/api/v1/telemetry` |

---

## 2. Quick Start / วิธีใช้งานอย่างรวดเร็ว

1. 🇬🇧 Sync credentials: from the parent folder run `make prepare` or copy the Server-TLS bundle into `../certs_credentials/`.
   🇹🇭 ซิงค์ bundle ด้วย `make prepare` หรือคัดลอกไฟล์เข้าสู่ `../certs_credentials/`
2. 🇬🇧 Ensure Python ≥ 3.8 is available (`python3 --version`).
   🇹🇭 ตรวจสอบว่า Python เวอร์ชัน 3.8 ขึ้นไปพร้อมใช้งาน
3. 🇬🇧 Run one of the recipes below.
   🇹🇭 เลือกคำสั่งด้านล่างเพื่อส่งเทเลเมทรี

```bash
cd tutorial/examples/device-to-platform/serverTLS/CLI_Python

# 🇬🇧 Single telemetry document to production (https://tesaiot.com)
# 🇹🇭 ส่งข้อความเดียวไปยังระบบจริง (https://tesaiot.com)
./post_https_sample.py

# 🇬🇧 Override base URL (e.g., staging) and preview payload only
# 🇹🇭 สลับไปยัง staging และดู payload โดยไม่ส่งจริง
./post_https_sample.py --base-url https://staging.tesaiot.com --dry-run

# 🇬🇧 Send every 5 seconds for two minutes
# 🇹🇭 ส่งทุก 5 วินาที นาน 2 นาที
./post_https_sample.py --period 2 --interval 5
```

---

## 3. Flags & Environment Variables / พารามิเตอร์สำคัญ

| Flag / env | 🇹🇭 ความหมาย | 🇬🇧 Description |
| --- | --- | --- |
| `--certs-dir`, `CERTS_DIR` | โฟลเดอร์เก็บ bundle (ค่าเริ่มต้น `../certs_credentials`) | Directory containing the credential bundle |
| `--base-url`, `BASE_URL` | เปลี่ยนฐาน URL หากไม่ได้ใช้ production | Override ingest base URL (falls back to `endpoints.json`) |
| `--endpoint`, `ENDPOINT` | ระบุปลายทาง API (ค่าเริ่มต้น `/api/v1/telemetry`) | API endpoint path to POST to |
| `--period`, `--interval` | ระยะเวลาส่งซ้ำเป็นนาที / วินาที | Duration in minutes and send interval in seconds |
| `--dry-run` | แสดง payload แล้วออก | Print payload and exit without sending |

---

## 4. Additional Notes / หมายเหตุเพิ่มเติม

- 🇹🇭 สคริปต์ยึดตาม Apache 2.0 license เหมือนไฟล์อื่น สามารถคัดลอกไปทดลองได้โดยคง header เอาไว้
  🇬🇧 Licensed under Apache 2.0—copy and adapt as long as the header is preserved.
- 🇹🇭 ฟังก์ชัน `resolve_base_url` จะพยายามอ่านข้อความจาก `endpoints.json` หากไม่ระบุ `--base-url`
  🇬🇧 `resolve_base_url` inspects `endpoints.json` when `--base-url` is omitted, mirroring device onboarding.
- 🇹🇭 หาก TLS handshake ล้มเหลว ให้ตรวจสอบเวลาของเครื่องและสิทธิ์ไฟล์ CA (`600`)
  🇬🇧 TLS handshake errors usually mean the system clock is wrong or the CA file lacks `0600` permissions.
