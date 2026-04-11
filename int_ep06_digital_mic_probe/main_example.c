/*******************************************************************************
 * @file    main_example.c
 * @brief   Episode entry wrapper — INT ep06: Digital Microphone Probe (PDM)
 *
 *  Streams stereo PDM audio from the two on-board digital MEMS microphones,
 *  decimates each channel in hardware, computes peak/average levels, and
 *  renders a live stereo level meter on the LVGL display.
 *
 *  Two services are started:
 *      1. mic_presenter_start()     — builds the UI and consumes sample
 *                                      snapshots for rendering.
 *      2. pdm_probe_logger_start()  — owns the PDM/PCM peripheral, runs the
 *                                      DMA capture task, and publishes
 *                                      samples to the presenter.
 ******************************************************************************/
#include "app_interface.h"
#include "tesaiot_thai.h"   /* Thai font support badge + helpers */

#include "mic/mic_presenter.h"
#include "pdm/pdm_probe_logger.h"

void example_main(lv_obj_t *parent)
{
    /* Master template bundles Noto Sans Thai fonts — this badge
     * confirms to the developer that Thai rendering is available. */
    tesaiot_add_thai_support_badge();

    (void)parent;

    /* Build the UI first so the presenter's LVGL objects exist before the
     * producer task starts pushing samples. */
    (void)mic_presenter_start();

    /* Start the PDM capture + level-meter task. */
    (void)pdm_probe_logger_start();
}
