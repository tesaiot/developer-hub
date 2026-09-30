# ค่า Accelerometer XYZ (Accelerometer XYZ)

ตัวอย่างนี้อ่านข้อมูล accelerometer จากเซ็นเซอร์ IMU BMI270 และแสดงค่าแกน X, Y, Z แบบ real-time LVGL timer ทุก 100ms poll เซ็นเซอร์และอัปเดต label สามตัวที่แสดงความเร่งในหน่วย g-force

BMI270 เป็น IMU 6 แกนที่ให้ทั้งข้อมูล accelerometer และ gyroscope ตัวอย่างนี้ใช้เฉพาะส่วน accelerometer เซ็นเซอร์ถูกเริ่มต้นด้วย `bmi270_init` และอ่านด้วย `bmi270_read_accel` ซึ่งคืนค่า float สามค่า

ค่าแต่ละแกนแสดงด้วย label ที่มีรหัสสี: แดงสำหรับ X เขียวสำหรับ Y และน้ำเงินสำหรับ Z ค่าจัดรูปแบบเป็นทศนิยมสองตำแหน่ง bar chart ด้านล่างให้การแสดงภาพขนาดของแต่ละแกน

BMI270 มีทั้งบน AI Kit และ Eva Kit (BSP_HAS_BMI270=1) ดังนั้นตัวอย่างนี้รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
