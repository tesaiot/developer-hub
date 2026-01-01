# ตัวอย่าง Platform-to-Application (Python)

> **ภาษาไทย** - สำหรับเวอร์ชันภาษาอังกฤษ ดูที่ [README.md](README.md)

---

## 1. ภาพรวม

ตัวอย่างนี้สาธิตการเชื่อมต่อ TESAIoT Platform ผ่าน API Gateway (APISIX) ด้วย API Key เพียงค่าเดียว สามารถดูรายชื่ออุปกรณ์, โครงสร้างสคีมา, เทเลเมตรี (ทั้งย้อนหลังด้วย `--start`/`--since` และ real-time ผ่าน WebSocket) และสถิติรวมได้ภายในไม่กี่คำสั่ง

**ใช้ได้สำหรับ:** Beginner → Intermediate Python developers, DevOps engineers, หรือ Solution Architects ที่ต้องการทดสอบ API gateway อย่างรวดเร็ว


## 2. โครงสร้างโปรเจ็กต์

```
platform-to-application/
├── .env.example              # แม่แบบไฟล์ environment (อย่า commit .env จริง)
├── .gitignore                # บล็อก .env, __pycache__, .venv
├── README.md                 # คู่มือภาษาอังกฤษ
├── README-TH.md              # คู่มือฉบับนี้ (ไทย)
├── app.py                    # Typer CLI ที่เรียก REST API ผ่าน requests
├── config.yaml               # ค่าตั้งต้น: base_url, timeout, schema type
├── requirements.txt          # Dependencies สำหรับ virtualenv
├── run.sh                    # สคริปต์ช่วย bootstrap + run (Linux/macOS)
└── __pycache__/ .venv/ ...   # ถูก ignore โดยอัตโนมัติ
```


## 3. ความต้องการระบบ

| รายการ | รายละเอียด |
| ------ | ---- |
| Python | เวอร์ชัน 3.9 ขึ้นไป (แนะนำ 3.11) |
| Pip / venv | ติดตั้ง `pip` และ `python3 -m venv` |
| ระบบปฏิบัติการ | Linux, macOS, หรือ Windows พร้อม WSL / Git Bash |
| API Key | ค่า API Key จาก APISIX (เช่น `tesa_ak_…`) |
| Internet | ต้องเชื่อมต่ออินเทอร์เน็ตภายในเครือข่ายที่ไปถึง TESAIoT Platform |

> หากใช้ Windows PowerShell ให้แทน `export` ด้วย `setx` หรือใช้ไฟล์ `.env`


## 4. เตรียม Environment

### 4.1 ขั้นตอนด่วนสำหรับ Linux/macOS (แนะนำ)

1. เข้าโฟลเดอร์ตัวอย่าง
   ```bash
   cd tutorial/examples/application-to-platform
   ```
2. สร้าง virtualenv และติดตั้ง dependencies (ใช้ `run.sh` หรือสั่งเอง)
   ```bash
   ./run.sh bootstrap
   ```
   หากสคริปต์เตือนว่า `python3-venv` ไม่มี ให้ติดตั้งด้วย `sudo apt install python3-venv`
3. คัดลอกไฟล์ environment ตัวอย่างแล้วเติม API Key
   ```bash
   cp .env.example .env
   # แก้ไขไฟล์ .env แล้วใส่ TESAIOT_API_KEY=tesa_ak_xxx
   ```
4. รันคำสั่ง
   ```bash
   ./run.sh run devices list
   ```

> **หมายเหตุ Windows**: ใช้ PowerShell `python -m venv .venv ; .\.venv\Scripts\Activate.ps1` แล้วรัน `pip install -r requirements.txt` จากนั้นใช้ `python app.py ...`


## 5. การตั้งค่า Environment Variables

ไฟล์ `.env` รองรับค่าต่อไปนี้:

| Key | คำอธิบาย |
| --- | ---------- |
| `TESAIOT_API_KEY` (จำเป็น) | API Key จาก APISIX |
| `TESAIOT_ORGANIZATION_ID` (ไม่บังคับ) | ใช้กรณีต้องเจาะจงองค์กร |
| `TESAIOT_USER_EMAIL` (ไม่บังคับ) | สำหรับ header เพิ่มเติม |
| `TESAIOT_REALTIME_WS_URL` (ไม่บังคับ) | URL สำหรับ real-time telemetry (ค่าเริ่มต้นชี้ไป `wss://admin.tesaiot.com/ws/telemetry`) |

ค่าจาก `config.yaml` จะถูกใช้เป็นค่าเริ่มต้น:
- `api_gateway.base_url` → `https://admin.tesaiot.com/api/v1/external`
- `verify_tls` → `true`
- `default_schema` → `industrial_device`


## 6. คำสั่งที่มีให้

ทุกคำสั่งทำงานผ่าน Typer CLI (Python) ใช้รูปแบบ `./run.sh run <command>` หรือ `python app.py <command>` เมื่อ activate virtualenv แล้ว:

| Command | ทำอะไร |
| ------- | --------- |
| `devices list --limit 20` | แสดงรายชื่ออุปกรณ์ขององค์กร |
| `devices show <device_id>` | ดูรายละเอียดเจาะจง |
| `devices telemetry <device_id> --since 15m` | ดึง telemetry ช่วงเวลาที่กำหนด (เช่น 15 นาทีล่าสุด หรือระบุ `--start`/`--end`) |
| `devices schema --schema-type industrial_device` | โหลดสคีมาตัวอย่าง |
| `devices stream <device_id> --duration 60` | ต่อ WebSocket เพื่ออ่าน telemetry สด |
| `stats summary` | สรุป Total/Active/Secure/Throughput |

ตัวอย่างการใช้งาน:

```bash
# Linux/macOS
./run.sh run devices list --limit 5
./run.sh run devices telemetry 1ba776fc-1250-4a21-bc7f-fd538cdda083 --since 30m
./run.sh run devices telemetry 1ba776fc-1250-4a21-bc7f-fd538cdda083 \
  --start 2025-09-20T00:00:00Z --end 2025-09-20T01:00:00Z --limit 100
./run.sh run devices stream 1ba776fc-1250-4a21-bc7f-fd538cdda083 --duration 60
./run.sh run stats summary
```

ตัวอย่างผลลัพธ์:

```
Device Inventory
#  Device ID                        Name              Status  Type
1  1ba776fc-1250-4a21-…             Device01-mTLS-CSR  active  sensor
2  e919a2f9-ad08-40be-…             Device02-ServerTLS active  gateway
```

```
Telemetry for 1ba776fc-1250-4a21-bc7f-fd538cdda083 (start: 2025-09-20T00:00:00Z, end: 2025-09-20T01:00:00Z)
Timestamp                Values
2025-09-20T00:42:14...   {"data_heart_rate": 79, ...}
...
```

```
Streaming live telemetry. Press Ctrl+C to stop.
2025-09-20T09:31:44Z 1ba776fc-1250-4a21-bc7f-fd538cdda083 {"temperature": 24.3, "humidity": 58}
• keep-alive ping
2025-09-20T09:31:49Z 1ba776fc-1250-4a21-bc7f-fd538cdda083 {"temperature": 24.1, "humidity": 57}
Received 2 telemetry message(s)
```

```
Device Posture
Metric                Value
Total devices         2
Active devices        2
Secure rate           100.00%
Telemetry avg msg/min 0.0
```


## 7. สิทธิ์ที่ต้องการ

- CLI จะเรียก endpoints: `/devices`, `/devices/<id>`, `/devices/<id>/telemetry`, `/devices/schemas/<type>`, `/devices/stats`, `/dashboard/stats`, และ WebSocket `/ws/telemetry`
- API Key ต้องมี scopes อย่างน้อย: `devices:read`, `telemetry:read`, `organizations:read`, `dashboard:read`, `analytics:read`
- บนระบบล่าสุด สคริปต์ดูแล APISIX จะเพิ่ม scopes ข้างต้นให้อัตโนมัติ ถ้าใช้ key เก่าที่เฉพาะ `devices:read` กรุณาติดต่อผู้ดูแลเพื่ออัปเดต

ตรวจสอบเองได้ด้วย `curl` (เปลี่ยน `<key>`):
```bash
curl -i https://admin.tesaiot.com/api/v1/external/health \
  -H "X-API-Key: <key>"
```
ควรได้ `200` พร้อมข้อมูลสถานะ `{"status":"healthy"}`


## 8. ขยายผล

| ฟีเจอร์ | ทำอย่างไร |
| -------- | ----------- |
| เพิ่มคำสั่ง CLI | เพิ่มฟังก์ชันใหม่ใน `app.py` แล้วเพิ่ม `@cli.command()` |
| พารามิเตอร์เพิ่ม | ขยายฟังก์ชัน `_request` หรือเพิ่ม options ใน Typer |
| รวมกับระบบเดิม | นำ class `TESAIoTClient` ไปใช้ในโปรเจ็กต์อื่น |
| Logging | ปรับ `console = Console()` หรือใช้ `logging` เสริม |


## 9. แนวทางความปลอดภัย

- อย่า commit ไฟล์ `.env` หรือ API Key จริง (`.gitignore` บล็อกให้แล้ว)
- หมุน API Key เมื่อไม่ใช้งาน (APISIX มี endpoint revoke/rotate)
- ควรทดสอบ key ใหม่บน non-production ก่อนเสมอ
- ตรวจสอบให้ `.venv`, `__pycache__` ไม่ถูก commit (ถูก ignore แล้ว)
- บันทึก log การใช้งาน API Key สำหรับ audit


## 10. การแก้ปัญหา

| อาการ | วิธีแก้ |
| ------ | ----------- |
| `401 Authentication required` | ตรวจสอบ API Key หรือ scopes |
| `404 Device not found` | ตรวจสอบ device_id ถูกต้อง / อยู่ในองค์กรเดียวกัน |
| `requests.exceptions.SSLError` | อย่าเปลี่ยน base_url เป็น HTTP, ให้ตั้ง `verify_tls: false` เฉพาะกรณีทดสอบ self-signed |
| `ModuleNotFoundError` | รัน `./run.sh bootstrap` หรือ `pip install -r requirements.txt` |
| `.venv` สร้างไม่ได้ | ติดตั้ง `python3-venv` (Debian/Ubuntu) |


## 11. ตัวอย่างการใช้งานจริง (2025-09-18)

ใช้ API Key `Infineon_TESAIoT` (scopes ถูกบังคับครบชุด)

```
TESAIOT_API_KEY=tesa_ak_<redacted> \
  PYTHONPATH=. python3 app.py devices list --limit 5
```
- พบอุปกรณ์ 2 ตัว: `Device01-mTLS-CSR`, `Device02-ServerTLS`

```
TESAIOT_API_KEY=... python3 app.py devices telemetry 1ba776fc-... --limit 5
```
- ได้ข้อมูล Vital-signs ครบพร้อม timestamp

```
TESAIOT_API_KEY=... python3 app.py stats summary
```
- สรุป Total=2, Active=2, Secure=100%, Telemetry avg=0 msg/min (ในหน้าต่าง 15 นาที)


## 12. ขั้นตอนถัดไป

- เพิ่มคำสั่ง CLI สำหรับส่ง telemetry (POST `/telemetry`) เพื่อทดสอบ end-to-end ingest
- รวมกับ CI/CD (เช่น GitHub Actions) เพื่อตรวจสอบว่า API key ยังใช้งานได้ก่อน deploy
- สร้าง unit test สำหรับ `TESAIoTClient` โดย mock `requests.Session`
- เสริม integration test สำหรับคำสั่ง `devices stream` (เช่น mock WebSocket server)


## 13. ช่องทางช่วยเหลือ

- แจ้ง Incident: ช่องทางเดียวกับ TESAIoT Platform (Ops/Support channel)
- ติดต่อเจ้าหน้าที่ Infineon: eric.seow@infineon.com
- เจ้าของแพลตฟอร์ม: sriborrirux@gmail.com

> เมื่อเปิด ticket ให้แนบผลจาก `stats summary` + `devices telemetry` เพื่อตรวจสอบสิทธิ์และข้อมูลล่าสุด

---

ขอให้สนุกกับการเชื่อมต่อ TESAIoT Platform!
