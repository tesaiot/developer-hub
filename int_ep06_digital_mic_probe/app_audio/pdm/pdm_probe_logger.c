#include "pdm_probe_logger.h"

#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "mic_presenter.h"
#include "pdm_mic.h"

#define PDM_LOG_TASK_NAME             ("PDM Log Task")
#define PDM_LOG_TASK_STACK_SIZE       (configMINIMAL_STACK_SIZE * 4U)
#define PDM_LOG_TASK_PRIORITY         (configMAX_PRIORITIES - 2)
#define PDM_LOG_DECIMATE_FRAMES       (10U)  /* 10 frames x 10 ms ~= 100 ms */

/* Tuned for classroom speech level so UI% doesn't saturate too early. */
#define PDM_UI_FLOOR_ABS              (80U)
#define PDM_UI_CEIL_ABS               (8000U)

typedef struct
{
    uint32_t peak_abs;
    uint32_t avg_abs;
    uint32_t peak_tenth_pct_fs;
    uint32_t avg_tenth_pct_fs;
    uint32_t ui_pct;
} mic_level_t;

static TaskHandle_t s_pdm_log_task_handle = NULL;

static uint32_t abs_i16(int16_t val)
{
    return (val < 0) ? (uint32_t)(-(int32_t)val) : (uint32_t)val;
}

static uint32_t to_tenth_pct_fs(uint32_t abs_value)
{
    return (abs_value * 1000U) / 32767U;
}

static uint32_t to_ui_pct(uint32_t avg_abs)
{
    if (avg_abs <= PDM_UI_FLOOR_ABS)
    {
        return 0U;
    }

    if (avg_abs >= PDM_UI_CEIL_ABS)
    {
        return 100U;
    }

    return ((avg_abs - PDM_UI_FLOOR_ABS) * 100U) / (PDM_UI_CEIL_ABS - PDM_UI_FLOOR_ABS);
}

static mic_level_t compute_level(const int16_t *samples, uint32_t sample_count)
{
    mic_level_t level = {0};
    uint64_t sum_abs = 0ULL;

    for (uint32_t i = 0U; i < sample_count; i++)
    {
        uint32_t a = abs_i16(samples[i]);
        if (a > level.peak_abs)
        {
            level.peak_abs = a;
        }

        sum_abs += (uint64_t)a;
    }

    level.avg_abs = (sample_count > 0U) ? (uint32_t)(sum_abs / (uint64_t)sample_count) : 0U;
    level.peak_tenth_pct_fs = to_tenth_pct_fs(level.peak_abs);
    level.avg_tenth_pct_fs = to_tenth_pct_fs(level.avg_abs);
    level.ui_pct = to_ui_pct(level.avg_abs);

    return level;
}

static void pdm_probe_logger_task(void *arg)
{
    (void)arg;

    uint32_t frame_count = 0U;

    for (;;)
    {
        pdm_mic_frame_t frame = {0};
        cy_rslt_t rslt = pdm_mic_get_frame(&frame);

        if ((CY_RSLT_SUCCESS != rslt) || (NULL == frame.left) || (NULL == frame.right))
        {
            printf("[EP06][MIC] GET_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
            continue;
        }

        mic_level_t left = compute_level(frame.left, frame.sample_count);
        mic_level_t right = compute_level(frame.right, frame.sample_count);

        uint32_t sum_avg = left.avg_abs + right.avg_abs;
        int32_t balance_lr = 0;
        if (sum_avg > 0U)
        {
            balance_lr = ((int32_t)left.avg_abs - (int32_t)right.avg_abs) * 100 / (int32_t)sum_avg;
        }

        frame_count++;

        mic_presenter_sample_t sample =
        {
            .frame_count = frame_count,
            .left_peak_abs = left.peak_abs,
            .left_avg_abs = left.avg_abs,
            .right_peak_abs = right.peak_abs,
            .right_avg_abs = right.avg_abs,
            .left_peak_tenth_pct_fs = left.peak_tenth_pct_fs,
            .left_avg_tenth_pct_fs = left.avg_tenth_pct_fs,
            .right_peak_tenth_pct_fs = right.peak_tenth_pct_fs,
            .right_avg_tenth_pct_fs = right.avg_tenth_pct_fs,
            .left_ui_pct = left.ui_pct,
            .right_ui_pct = right.ui_pct,
            .balance_lr = balance_lr,
        };

        mic_presenter_publish_sample(&sample);

        if ((frame_count % PDM_LOG_DECIMATE_FRAMES) == 0U)
        {
            printf("[EP06][MIC] frame=%lu "
                   "L{peak=%lu fs=%lu.%01lu%% avg=%lu fs=%lu.%01lu%% ui=%lu%%} "
                   "R{peak=%lu fs=%lu.%01lu%% avg=%lu fs=%lu.%01lu%% ui=%lu%%} "
                   "bal=%ld\r\n",
                   (unsigned long)frame_count,
                   (unsigned long)left.peak_abs,
                   (unsigned long)(left.peak_tenth_pct_fs / 10U),
                   (unsigned long)(left.peak_tenth_pct_fs % 10U),
                   (unsigned long)left.avg_abs,
                   (unsigned long)(left.avg_tenth_pct_fs / 10U),
                   (unsigned long)(left.avg_tenth_pct_fs % 10U),
                   (unsigned long)left.ui_pct,
                   (unsigned long)right.peak_abs,
                   (unsigned long)(right.peak_tenth_pct_fs / 10U),
                   (unsigned long)(right.peak_tenth_pct_fs % 10U),
                   (unsigned long)right.avg_abs,
                   (unsigned long)(right.avg_tenth_pct_fs / 10U),
                   (unsigned long)(right.avg_tenth_pct_fs % 10U),
                   (unsigned long)right.ui_pct,
                   (long)balance_lr);
        }
    }
}

cy_rslt_t pdm_probe_logger_start(void)
{
    cy_rslt_t rslt = pdm_mic_init();
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    BaseType_t task_rslt = xTaskCreate(pdm_probe_logger_task,
                                       PDM_LOG_TASK_NAME,
                                       PDM_LOG_TASK_STACK_SIZE,
                                       NULL,
                                       PDM_LOG_TASK_PRIORITY,
                                       &s_pdm_log_task_handle);

    if (pdPASS != task_rslt)
    {
        printf("[EP06][MIC] TASK_CREATE_FAIL\r\n");
        return CY_RSLT_TYPE_ERROR;
    }

    printf("[EP06][MIC] LOGGER_START ok log_period_ms=100 scale=raw+ui\r\n");
    return CY_RSLT_SUCCESS;
}