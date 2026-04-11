/*******************************************************************************
 * @file    main_example.c
 * @brief   Episode entry wrapper — INT ep04: BMM350 Magnetometer Compass
 *
 *  The Bosch BMM350 magnetometer lives on the I3C bus (CYBSP_I3C_CONTROLLER),
 *  not the I2C sensor bus. The master template initializes the I3C controller
 *  in main.c and exposes both the hardware block and the PDL context via
 *  sensor_bus.h. This wrapper hands them to the vendored BMM350 presenter,
 *  which drives a compass view with an on-screen hard-iron calibration flow.
 *
 *  NOTE: The BMM350 vendor SDK ships a patch script (bmm350_fix.bash) that
 *  rewrites the upstream bmm350.c so it builds under GCC. The master Makefile
 *  applies that patch as a one-shot step the first time you build the episode.
 *  See the episode README for details.
 ******************************************************************************/
#include "app_interface.h"
#include "tesaiot_thai.h"   /* Thai font support badge + helpers */
#include "sensor_bus.h"

#include "bmm350/bmm350_presenter.h"

void example_main(lv_obj_t *parent)
{
    /* Master template bundles Noto Sans Thai fonts — this badge
     * confirms to the developer that Thai rendering is available. */
    tesaiot_add_thai_support_badge();

    (void)parent;

    bmm350_presenter_start(CYBSP_I3C_CONTROLLER_HW, &CYBSP_I3C_CONTROLLER_context);
}
