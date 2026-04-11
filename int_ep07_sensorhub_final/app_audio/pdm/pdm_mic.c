#include "pdm_mic.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "cy_pdl.h"
#include "cybsp.h"

#include "cyabs_rtos.h"

#define PDM_PCM_ISR_PRIORITY            (2U)
#define PDM_PCM_GAIN_DB                 (CY_PDM_PCM_SEL_GAIN_11DB)

#define PDM_LEFT_CH_INDEX               (2U)
#define PDM_RIGHT_CH_INDEX              (3U)
#define PDM_LEFT_CH_CONFIG              channel_2_config
#define PDM_RIGHT_CH_CONFIG             channel_3_config

#define PDM_HW_FIFO_SIZE                (64U)
#define PDM_RX_FIFO_TRIG_LEVEL          (PDM_HW_FIFO_SIZE / 2U)
#define PDM_INTERRUPTS_PER_FRAME        ((PDM_MIC_FRAME_SAMPLES_PER_CHANNEL + PDM_RX_FIFO_TRIG_LEVEL - 1U) / PDM_RX_FIFO_TRIG_LEVEL)

#define PDM_READY_LEFT                  (1U << 0)
#define PDM_READY_RIGHT                 (1U << 1)
#define PDM_READY_BOTH                  (PDM_READY_LEFT | PDM_READY_RIGHT)

typedef struct
{
    int16_t buffer0[PDM_MIC_FRAME_SAMPLES_PER_CHANNEL];
    int16_t buffer1[PDM_MIC_FRAME_SAMPLES_PER_CHANNEL];
    int16_t *active;
    int16_t *full;
    uint16_t block_count;
} pdm_channel_state_t;

static pdm_channel_state_t s_left_state = {0};
static pdm_channel_state_t s_right_state = {0};

static cy_semaphore_t s_pdm_mic_sema;
static volatile bool s_pdm_initialized = false;
static volatile uint8_t s_ready_mask = 0U;

static const cy_stc_sysint_t s_pdm_irq_left_cfg =
{
    .intrSrc = (IRQn_Type)CYBSP_PDM_CHANNEL_2_IRQ,
    .intrPriority = PDM_PCM_ISR_PRIORITY,
};

static const cy_stc_sysint_t s_pdm_irq_right_cfg =
{
    .intrSrc = (IRQn_Type)CYBSP_PDM_CHANNEL_3_IRQ,
    .intrPriority = PDM_PCM_ISR_PRIORITY,
};

/* ISR helper for each stereo channel: fill active buffer and flip on frame boundary. */
static void process_channel_irq(pdm_channel_state_t *state, uint8_t channel_index, uint8_t ready_bit)
{
    uint32_t intr_status = Cy_PDM_PCM_Channel_GetInterruptStatusMasked(CYBSP_PDM_HW, channel_index);

    if (0U != (CY_PDM_PCM_INTR_RX_TRIGGER & intr_status))
    {
        uint32_t offset = (uint32_t)state->block_count * PDM_RX_FIFO_TRIG_LEVEL;

        for (uint32_t index = 0U; index < PDM_RX_FIFO_TRIG_LEVEL; index++)
        {
            int16_t sample = (int16_t)Cy_PDM_PCM_Channel_ReadFifo(CYBSP_PDM_HW, channel_index);
            uint32_t out_idx = offset + index;

            if (out_idx < PDM_MIC_FRAME_SAMPLES_PER_CHANNEL)
            {
                state->active[out_idx] = sample;
            }
        }

        Cy_PDM_PCM_Channel_ClearInterrupt(CYBSP_PDM_HW, channel_index, CY_PDM_PCM_INTR_RX_TRIGGER);

        state->block_count++;
        if (state->block_count >= PDM_INTERRUPTS_PER_FRAME)
        {
            int16_t *temp = state->active;
            state->active = state->full;
            state->full = temp;
            state->block_count = 0U;

            s_ready_mask |= ready_bit;
            if ((s_ready_mask & PDM_READY_BOTH) == PDM_READY_BOTH)
            {
                /* Unblock consumer only when both L and R frame buffers are ready. */
                s_ready_mask &= (uint8_t)(~PDM_READY_BOTH);
                (void)cy_rtos_semaphore_set(&s_pdm_mic_sema);
            }
        }
    }

    if (0U != ((CY_PDM_PCM_INTR_RX_FIR_OVERFLOW |
                CY_PDM_PCM_INTR_RX_OVERFLOW |
                CY_PDM_PCM_INTR_RX_IF_OVERFLOW |
                CY_PDM_PCM_INTR_RX_UNDERFLOW) & intr_status))
    {
        Cy_PDM_PCM_Channel_ClearInterrupt(CYBSP_PDM_HW, channel_index, CY_PDM_PCM_INTR_MASK);
    }
}

static void pdm_pcm_left_event_handler(void)
{
    process_channel_irq(&s_left_state, PDM_LEFT_CH_INDEX, PDM_READY_LEFT);
}

static void pdm_pcm_right_event_handler(void)
{
    process_channel_irq(&s_right_state, PDM_RIGHT_CH_INDEX, PDM_READY_RIGHT);
}

cy_rslt_t pdm_mic_init(void)
{
    cy_rslt_t rslt = cy_rtos_semaphore_init(&s_pdm_mic_sema, 4U, 0U);
    if (CY_RSLT_SUCCESS != rslt)
    {
        printf("[EP07][MIC] SEMA_INIT_FAIL rslt=0x%08lx\r\n", (unsigned long)rslt);
        return rslt;
    }

    cy_en_pdm_pcm_status_t pdm_rslt = Cy_PDM_PCM_Init(CYBSP_PDM_HW, &CYBSP_PDM_config);
    if (CY_PDM_PCM_SUCCESS != pdm_rslt)
    {
        printf("[EP07][MIC] PDM_INIT_FAIL rslt=0x%08lx\r\n", (unsigned long)pdm_rslt);
        return (cy_rslt_t)pdm_rslt;
    }

    s_left_state.active = s_left_state.buffer0;
    s_left_state.full = s_left_state.buffer1;
    s_left_state.block_count = 0U;

    s_right_state.active = s_right_state.buffer0;
    s_right_state.full = s_right_state.buffer1;
    s_right_state.block_count = 0U;

    Cy_PDM_PCM_Channel_Init(CYBSP_PDM_HW, &PDM_LEFT_CH_CONFIG, (uint8_t)PDM_LEFT_CH_INDEX);
    Cy_PDM_PCM_Channel_Init(CYBSP_PDM_HW, &PDM_RIGHT_CH_CONFIG, (uint8_t)PDM_RIGHT_CH_INDEX);

    Cy_PDM_PCM_Channel_Enable(CYBSP_PDM_HW, PDM_LEFT_CH_INDEX);
    Cy_PDM_PCM_Channel_Enable(CYBSP_PDM_HW, PDM_RIGHT_CH_INDEX);

    Cy_PDM_PCM_Channel_ClearInterrupt(CYBSP_PDM_HW, PDM_LEFT_CH_INDEX, CY_PDM_PCM_INTR_MASK);
    Cy_PDM_PCM_Channel_ClearInterrupt(CYBSP_PDM_HW, PDM_RIGHT_CH_INDEX, CY_PDM_PCM_INTR_MASK);

    Cy_PDM_PCM_Channel_SetInterruptMask(CYBSP_PDM_HW, PDM_LEFT_CH_INDEX, CY_PDM_PCM_INTR_MASK);
    Cy_PDM_PCM_Channel_SetInterruptMask(CYBSP_PDM_HW, PDM_RIGHT_CH_INDEX, CY_PDM_PCM_INTR_MASK);

    cy_en_sysint_status_t irq_left_rslt = Cy_SysInt_Init(&s_pdm_irq_left_cfg, &pdm_pcm_left_event_handler);
    if (CY_SYSINT_SUCCESS != irq_left_rslt)
    {
        printf("[EP07][MIC] IRQ_LEFT_INIT_FAIL rslt=0x%08lx\r\n", (unsigned long)irq_left_rslt);
        return (cy_rslt_t)irq_left_rslt;
    }

    cy_en_sysint_status_t irq_right_rslt = Cy_SysInt_Init(&s_pdm_irq_right_cfg, &pdm_pcm_right_event_handler);
    if (CY_SYSINT_SUCCESS != irq_right_rslt)
    {
        printf("[EP07][MIC] IRQ_RIGHT_INIT_FAIL rslt=0x%08lx\r\n", (unsigned long)irq_right_rslt);
        return (cy_rslt_t)irq_right_rslt;
    }

    NVIC_ClearPendingIRQ(s_pdm_irq_left_cfg.intrSrc);
    NVIC_ClearPendingIRQ(s_pdm_irq_right_cfg.intrSrc);
    NVIC_EnableIRQ(s_pdm_irq_left_cfg.intrSrc);
    NVIC_EnableIRQ(s_pdm_irq_right_cfg.intrSrc);

    Cy_PDM_PCM_SetGain(CYBSP_PDM_HW, PDM_LEFT_CH_INDEX, PDM_PCM_GAIN_DB);
    Cy_PDM_PCM_SetGain(CYBSP_PDM_HW, PDM_RIGHT_CH_INDEX, PDM_PCM_GAIN_DB);

    Cy_PDM_PCM_Activate_Channel(CYBSP_PDM_HW, PDM_LEFT_CH_INDEX);
    Cy_PDM_PCM_Activate_Channel(CYBSP_PDM_HW, PDM_RIGHT_CH_INDEX);

    s_ready_mask = 0U;
    s_pdm_initialized = true;

    printf("[EP07][MIC] INIT_OK fs=%lu frame_samples=%u ch=L:2,R:3\r\n",
           (unsigned long)PDM_MIC_SAMPLE_RATE_HZ,
           (unsigned int)PDM_MIC_FRAME_SAMPLES_PER_CHANNEL);

    return CY_RSLT_SUCCESS;
}

cy_rslt_t pdm_mic_get_frame(pdm_mic_frame_t *frame)
{
    if ((NULL == frame) || (!s_pdm_initialized))
    {
        return CY_RSLT_TYPE_ERROR;
    }

    cy_rslt_t rslt = cy_rtos_semaphore_get(&s_pdm_mic_sema, CY_RTOS_NEVER_TIMEOUT);
    if (CY_RSLT_SUCCESS == rslt)
    {
        frame->left = s_left_state.full;
        frame->right = s_right_state.full;
        frame->sample_count = PDM_MIC_FRAME_SAMPLES_PER_CHANNEL;
    }

    return rslt;
}

cy_rslt_t pdm_mic_deinit(void)
{
    NVIC_DisableIRQ(s_pdm_irq_left_cfg.intrSrc);
    NVIC_DisableIRQ(s_pdm_irq_right_cfg.intrSrc);

    Cy_PDM_PCM_DeInit(CYBSP_PDM_HW);
    s_pdm_initialized = false;

    return CY_RSLT_SUCCESS;
}


