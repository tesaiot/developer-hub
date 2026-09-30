# i04 — Compass Rose

Production-quality compass driven by the BMM350 magnetometer. A 250x250 circular
compass rose rotates smoothly based on the device heading, with cardinal direction
labels (N/S/E/W) rotating on the ring. Heading degrees are displayed in the center
and the cardinal text direction (N, NE, E, SE, etc.) is shown below.

## Features

- Rotating compass ring using `lv_obj_set_style_transform_angle()`
- Cardinal labels rotate with the ring; north is highlighted in red
- Center shows heading in degrees with large font
- Change-gated updates (only redraws when heading changes)
- BSP guard: shows "BMM350 not available" on boards without magnetometer

## Sensor Data

- `ipc_sensorhub_snapshot()` reads `snap.bmm350.heading_x10`
- Heading is in 0.1-degree units (0-3600), divided by 10 for display
- LVGL transform angle uses 0.1-degree units (negative for ring rotation)
- 100ms update interval

## BSP Support

- **AI Kit**: Full compass functionality (BSP_HAS_BMM350 = 1)
- **Eva Kit**: Full compass functionality (BSP_HAS_BMM350 = 1)
- **Game Console**: Shows fallback message (BSP_HAS_BMM350 = 0)
