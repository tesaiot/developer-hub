# CLI — ตัวส่งข้อความ MQTTS mTLS แบบ CLI

> **ภาษาไทย** - สำหรับเวอร์ชันภาษาอังกฤษ ดูที่ [README.md](README.md)

สคริปต์ Bash ตัวอย่างที่ใช้ `mosquitto_pub` ส่งเทเลเมทรีไปยัง TESAIoT ผ่าน MQTT over TLS (Mutual Authentication)

---

## 1. ไฟล์ในโฟลเดอร์

| File | รายละเอียด |
| --- | --- |
| `publish_mqtt_sample.sh` | สคริปต์ Bash ที่เตรียม payload, เซ็นด้วย key ใน bundle และส่งไปยัง broker ของ TESAIoT |

> สคริปต์วางอยู่ข้างตัวอย่างภาษา C เพื่อใช้ bundle credentials จาก `../certs_credentials`

---

## 2. สิ่งที่ต้องเตรียม

- ติดตั้ง `mosquitto_pub` สำหรับส่ง MQTT (`sudo apt install mosquitto-clients` หรือ `brew install mosquitto`)
- ใช้ `python3` เพื่อสร้าง payload ที่ตรงกับ schema (ใช้ standard library เท่านั้น)
- ต้องมี credential bundle (client key/cert + CA) อยู่ใน `../certs_credentials/`

---

## 3. เริ่มต้นอย่างรวดเร็ว

1. ซิงค์ credentials ด้วย `make prepare` หรือคัดลอกไฟล์เข้ามือ
2. ตรวจสอบ `mosquitto_pub --help` ว่าพร้อมใช้งาน
3. ส่งเทเลเมทรีด้วยคำสั่งตัวอย่างด้านล่าง

```bash
cd tutorial/examples/device-to-platform/mTLS/CLI_Python

# ส่งข้อความเดียวไปยัง broker จริงที่ mqtt.tesaiot.com:8883
./publish_mqtt_sample.sh --once

# บังคับใช้ TLS 1.3 ขณะเชื่อมต่อ (ค่าเริ่มต้นยังเป็น TLS 1.2)
./publish_mqtt_sample.sh --tls-version tlsv1.3 --once

# ส่งทุก 5 วินาทีเป็นเวลา 2 นาที
./publish_mqtt_sample.sh --interval 5 --period 2

# โหมด dry-run เพื่อดู payload โดยไม่ส่งออก
./publish_mqtt_sample.sh --dry-run
```

---

## 4. พารามิเตอร์ของสคริปต์

| Flag | ความหมาย |
| --- | --- |
| `--certs-dir` | โฟลเดอร์ที่เก็บ bundle (`../certs_credentials` ตามค่าเริ่มต้น) |
| `--broker-host`, `--broker-port` | กำหนดปลายทาง broker |
| `--device-id` | ใช้ device ID จากไฟล์ หรือกำหนดเอง |
| `--interval`, `--period` | ระยะเวลาส่งซ้ำ (วินาที / นาที) |
| `--dry-run` | แสดง payload แล้วออก ไม่ส่งจริง |
| `--tls-version` | กำหนดเวอร์ชัน TLS (`tlsv1.2` หรือ `tlsv1.3`) |

---

## 5. หมายเหตุ

- สคริปต์เคารพ Apache 2.0 license เหมือนไฟล์อื่น ๆ ในตัวอย่างนี้ คุณสามารถคัดลอกไปใช้ต่อได้โดยให้เครดิต
- หากต้องการส่งค่าข้อมูลอื่น ๆ ปรับส่วน Python ที่สร้าง JSON ได้โดยตรง
- ตรวจสอบให้แน่ใจว่าเวลาของเครื่องถูกต้อง เพราะ TLS broker จะปฏิเสธหากเวลาคลาดเคลื่อนมาก
