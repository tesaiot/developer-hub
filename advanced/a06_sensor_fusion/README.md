# A05 — Sensor Fusion

9-DOF Attitude and Heading Reference System combining BMI270 accelerometer/gyroscope with BMM350 magnetometer using a complementary filter for real-time orientation display.

## What it demonstrates

- Complementary filter for sensor fusion (alpha=0.96)
- Artificial horizon with pitch/roll response
- Compass needle driven by magnetometer heading
- 2D tilt indicator ball
- Raw sensor data display for all 9 axes
- Fusion quality estimation
- BSP guards for sensor availability

## Hardware

- Board: KIT_PSE84_AI (all 9 DOF), KIT_PSE84_EVAL_EPC2 (IMU + Mag)
- Sensors: BMI270 (accel + gyro), BMM350 (magnetometer)

## How to use

1. Copy `main_example.c` to `proj_cm55/app/`
2. Build: `make build -j && make program`
3. Tilt and rotate the board to see orientation changes
4. Compass heading updates from magnetometer data
