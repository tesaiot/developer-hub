#include "mic_presenter.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"

#include "mic_view.h"

#define MIC_UI_TIMER_PERIOD_MS    (50U)

static lv_timer_t *s_ui_timer = NULL;
static bool s_view_ready = false;
static bool s_has_sample = false;
static mic_presenter_sample_t s_latest_sample = {0};

/* Run on LVGL thread cadence: copy latest sample and refresh view. */
static void mic_presenter_ui_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    mic_presenter_sample_t local = {0};
    bool has_sample = false;

    /* Producer runs in another task, so protect shared sample snapshot. */
    taskENTER_CRITICAL();
    has_sample = s_has_sample;
    if (has_sample)
    {
        local = s_latest_sample;
    }
    taskEXIT_CRITICAL();

    if (has_sample && s_view_ready)
    {
        mic_view_apply(&local);
    }
}

cy_rslt_t mic_presenter_start(void)
{
    if (s_view_ready)
    {
        return CY_RSLT_SUCCESS;
    }

    cy_rslt_t rslt = mic_view_create();
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    s_ui_timer = lv_timer_create(mic_presenter_ui_timer_cb, MIC_UI_TIMER_PERIOD_MS, NULL);
    if (NULL == s_ui_timer)
    {
        return CY_RSLT_TYPE_ERROR;
    }

    s_view_ready = true;
    return CY_RSLT_SUCCESS;
}

void mic_presenter_publish_sample(const mic_presenter_sample_t *sample)
{
    if (NULL == sample)
    {
        return;
    }

    /* Latest-sample wins policy to keep UI responsive and non-blocking. */
    taskENTER_CRITICAL();
    s_latest_sample = *sample;
    s_has_sample = true;
    taskEXIT_CRITICAL();
}