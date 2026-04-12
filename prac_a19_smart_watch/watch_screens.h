/**
 * watch_screens.h — Forward declarations for smart watch screen creators
 *
 * Each screen function creates its UI inside the given parent container.
 * The parent is a 280x280 circular watch face area.
 */

#ifndef WATCH_SCREENS_H
#define WATCH_SCREENS_H

#include "pse84_common.h"
#include "app_interface.h"

/** Create the digital clock screen (HH:MM:SS + date). */
lv_obj_t *watch_clock_create(lv_obj_t *parent);

/** Create the sensor data screen (IMU, temp, humidity). */
lv_obj_t *watch_sensors_create(lv_obj_t *parent);

/** Create the step counter screen with arc gauge. */
lv_obj_t *watch_steps_create(lv_obj_t *parent);

/** Create the weather info screen (temp, humidity, altitude). */
lv_obj_t *watch_weather_create(lv_obj_t *parent);

#endif /* WATCH_SCREENS_H */
