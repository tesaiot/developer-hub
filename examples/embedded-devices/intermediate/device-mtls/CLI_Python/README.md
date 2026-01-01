# CLI — MQTTS mTLS Quick Sender / ตัวส่งข้อความ MQTTS mTLS แบบ CLI

> 🇹🇭 สคริปต์ Bash ตัวอย่างที่ใช้ `mosquitto_pub` ส่งเทเลเมทรีไปยัง TESAIoT ผ่าน MQTT over TLS (Mutual Authentication)
>
> 🇬🇧 Bash helper that wraps `mosquitto_pub` to publish schema-aligned telemetry to TESAIoT over MQTT with mutual TLS.

---

## 1. Files / ไฟล์ในโฟลเดอร์

| File | 🇹🇭 รายละเอียด | 🇬🇧 Description |
| --- | --- | --- |
| `publish_mqtt_sample.sh` | สคริปต์ Bash ที่เตรียม payload, เซ็นด้วย key ใน bundle และส่งไปยัง broker ของ TESAIoT | Bash script that synthesises telemetry, signs it with the bundle key, and publishes to the TESAIoT broker |

> 🇹🇭 สคริปต์วางอยู่ข้างตัวอย่างภาษา C เพื่อใช้ bundle credentials จาก `../certs_credentials`
>
> 🇬🇧 The script sits next to the C example so both reuse the same credential bundle in `../certs_credentials`.

---

## 2. Prerequisites / สิ่งที่ต้องเตรียม

- 🇬🇧 `mosquitto_pub` (install via `sudo apt install mosquitto-clients` or `brew install mosquitto`)
  🇹🇭 ติดตั้ง `mosquitto_pub` สำหรับส่ง MQTT
- 🇬🇧 `python3` (standard library only) for generating the JSON payload
  🇹🇭 ใช้ `python3` เพื่อสร้าง payload ที่ตรงกับ schema
- 🇬🇧 TESAIoT mTLS credential bundle synced into `../certs_credentials/`
  🇹🇭 ต้องมี credential bundle (client key/cert + CA) อยู่ใน `../certs_credentials/`

---

## 3. Quick Start / เริ่มต้นอย่างรวดเร็ว

1. 🇬🇧 Sync credentials: run `make prepare` from the parent directory (or copy the bundle manually).
   🇹🇭 ซิงค์ credentials ด้วย `make prepare` หรือคัดลอกไฟล์เข้ามือ
2. 🇬🇧 Verify the Mosquitto CLI: `mosquitto_pub --help`
   🇹🇭 ตรวจสอบ `mosquitto_pub --help` ว่าพร้อมใช้งาน
3. 🇬🇧 Publish telemetry with one of the recipes below.
   🇹🇭 ส่งเทเลเมทรีด้วยคำสั่งตัวอย่างด้านล่าง

```bash
cd tutorial/examples/device-to-platform/mTLS/CLI_Python

# 🇬🇧 Send a single payload to production broker (mqtt.tesaiot.com:8883)
# 🇹🇭 ส่งข้อความเดียวไปยัง broker จริงที่ mqtt.tesaiot.com:8883
./publish_mqtt_sample.sh --once

# 🇬🇧 Force TLS 1.3 during the handshake (TLS 1.2 remains the default)
# 🇹🇭 บังคับใช้ TLS 1.3 ขณะเชื่อมต่อ (ค่าเริ่มต้นยังเป็น TLS 1.2)
./publish_mqtt_sample.sh --tls-version tlsv1.3 --once

# 🇬🇧 Keep sending every 5 seconds for 2 minutes
# 🇹🇭 ส่งทุก 5 วินาทีเป็นเวลา 2 นาที
./publish_mqtt_sample.sh --interval 5 --period 2

# 🇬🇧 Dry-run mode to view the payload without publishing
# 🇹🇭 โหมด dry-run เพื่อดู payload โดยไม่ส่งออก
./publish_mqtt_sample.sh --dry-run
```

---

## 4. Script Flags / พารามิเตอร์ของสคริปต์

| Flag | 🇹🇭 ความหมาย | 🇬🇧 Description |
| --- | --- | --- |
| `--certs-dir` | โฟลเดอร์ที่เก็บ bundle (`../certs_credentials` ตามค่าเริ่มต้น) | Directory containing the credential bundle (defaults to `../certs_credentials`) |
| `--broker-host`, `--broker-port` | กำหนดปลายทาง broker | Override broker host/port |
| `--device-id` | ใช้ device ID จากไฟล์ หรือกำหนดเอง | Device identifier to embed in the payload |
| `--interval`, `--period` | ระยะเวลาส่งซ้ำ (วินาที / นาที) | Publish cadence (seconds) and overall duration (minutes) |
| `--dry-run` | แสดง payload แล้วออก ไม่ส่งจริง | Print payload only, skip publishing |
| `--tls-version` | กำหนดเวอร์ชัน TLS (`tlsv1.2` หรือ `tlsv1.3`) | Select TLS version (`tlsv1.2` or `tlsv1.3`) |

---

## 5. Notes / หมายเหตุ

- 🇹🇭 สคริปต์เคารพ Apache 2.0 license เหมือนไฟล์อื่น ๆ ในตัวอย่างนี้ คุณสามารถคัดลอกไปใช้ต่อได้โดยให้เครดิต
  🇬🇧 Licensed under Apache 2.0—feel free to adapt inside your lab projects while preserving the header.
- 🇹🇭 หากต้องการส่งค่าข้อมูลอื่น ๆ ปรับส่วน Python ที่สร้าง JSON ได้โดยตรง
  🇬🇧 Adjust the Python snippet that builds the JSON payload to match your device schema.
- 🇹🇭 ตรวจสอบให้แน่ใจว่าเวลาของเครื่องถูกต้อง เพราะ TLS broker จะปฏิเสธหากเวลาคลาดเคลื่อนมาก
  🇬🇧 Ensure system time is accurate—TLS handshakes will fail if the clock drifts too far.
