# Mic UI Module

โมดูลนี้เก็บชนิดข้อมูลและ presenter interface สำหรับส่งข้อมูล `Digital PDM Mic` ไปยังหน้า `Audio` ของ SensorHub

ไฟล์หลักที่ใช้กับ EP07:

- `mic_presenter.h` : data structure (`mic_presenter_sample_t`) และ publish/get APIs
- `mic_presenter.c` : latest-sample buffer (producer/consumer interface)

หมายเหตุ:

- ใน EP07 หน้าจอแสดงผล Audio อยู่ใน `app_ui/sensorhub/sensorhub_view.c`
- `mic_view.*` ยังเก็บไว้เป็น reference จากตัวอย่างเดิม
