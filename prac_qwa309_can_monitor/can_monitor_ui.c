/*******************************************************************************
* File Name        : can_monitor_ui.c
*
* Description      : CANFD0 Classic CAN 2.0A @ 500 kbps monitor on CM55 with an
*                    LVGL frame table. Ported from qwa309-training-base
*                    canbus_test (originally CM33-NS baremetal) to the CM55
*                    LVGL episode model. Fully POLLED — no NVIC/ISR — so it
*                    runs entirely inside the LVGL gfx task and avoids CM55
*                    interrupt-mux setup.
*
*                    TX: one frame/second, ID 0x123, DLC 8, counter payload.
*                    RX: polls RX FIFO 0 each tick; shows the last frame.
*                    Pins: P16.2 = RX, P16.3 = TX -> SN65HVD230 (U13).
*                    Pure-code setup — no Device Configurator changes.
*******************************************************************************/
#include "can_monitor_ui.h"

#include "cybsp.h"
#include "cy_pdl.h"
#include "cy_canfd.h"
#include "gpio_pse84_bga_220.h"
#include "lvgl.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* ---- CANFD target ---------------------------------------------------------- */
#define CANBUS_HW              CANFD0
#define CANBUS_CHANNEL         (1U)
#define CANBUS_CHANNEL_MASK    (1UL << CANBUS_CHANNEL)
#define CANBUS_PCLK_DST        (PCLK_CANFD0_CLOCK_CAN_EN1)
#define CANBUS_RX_PORT         GPIO_PRT16
#define CANBUS_RX_PIN          (2U)
#define CANBUS_RX_HSIOM        P16_2_CANFD0_TTCAN_RX1
#define CANBUS_TX_PORT         GPIO_PRT16
#define CANBUS_TX_PIN          (3U)
#define CANBUS_TX_HSIOM        P16_3_CANFD0_TTCAN_TX1
#define CANBUS_DIV_TYPE        (CY_SYSCLK_DIV_8_BIT)
#define CANBUS_DIV_NUM         (4U)
#define CANBUS_DIV_VALUE       (0U)
/* 500 kbps from 100 MHz: prescaler 10, TS1 15, TS2 4, SJW 4 (register = n-1) */
#define CANBUS_BITRATE_PRESCALER   (10U - 1U)
#define CANBUS_BITRATE_TS1         (15U - 1U)
#define CANBUS_BITRATE_TS2         (4U - 1U)
#define CANBUS_BITRATE_SJW         (4U - 1U)
#define CAN_TX_ID              (0x123U)
#define CAN_TX_DLC             (8U)
#define CANBUS_MRAM_ADDRESS    (CY_CAN0MRAM_BASE + 0U)
#define CANBUS_MRAM_SIZE       (4096U)
#define CAN_REFRESH_PERIOD_MS  (250U)
#define CAN_TX_EVERY_TICKS     (4U)     /* 4 x 250 ms = 1 s */

/* ---- CANFD config (Classic CAN 2.0A) --------------------------------------- */
static cy_stc_canfd_context_t g_ctx;
static const cy_stc_canfd_bitrate_t g_bitrate = {
    .prescaler = CANBUS_BITRATE_PRESCALER, .timeSegment1 = CANBUS_BITRATE_TS1,
    .timeSegment2 = CANBUS_BITRATE_TS2, .syncJumpWidth = CANBUS_BITRATE_SJW,
};
static const cy_stc_canfd_transceiver_delay_compensation_t g_tdc = {
    .tdcEnabled = false, .tdcOffset = 0U, .tdcFilterWindow = 0U,
};
static const cy_stc_canfd_sid_filter_config_t g_sid_cfg = {
    .numberOfSIDFilters = 0U, .sidFilter = NULL,
};
static const cy_stc_canfd_extid_filter_config_t g_xid_cfg = {
    .numberOfEXTIDFilters = 0U, .extidFilter = NULL, .extIDANDMask = 0x1FFFFFFFUL,
};
static const cy_stc_canfd_global_filter_config_t g_global_cfg = {
    .nonMatchingFramesStandard = CY_CANFD_ACCEPT_IN_RXFIFO_0,
    .nonMatchingFramesExtended = CY_CANFD_ACCEPT_IN_RXFIFO_0,
    .rejectRemoteFramesStandard = false, .rejectRemoteFramesExtended = false,
};
static const cy_en_canfd_fifo_config_t g_rx_fifo0_cfg = {
    .mode = CY_CANFD_FIFO_MODE_BLOCKING, .watermark = 0U,
    .numberOfFIFOElements = 4U, .topPointerLogicEnabled = false,
};
static const cy_en_canfd_fifo_config_t g_rx_fifo1_cfg = {
    .mode = CY_CANFD_FIFO_MODE_BLOCKING, .watermark = 0U,
    .numberOfFIFOElements = 1U, .topPointerLogicEnabled = false,
};
static cy_stc_canfd_config_t g_cfg = {
    .txCallback = NULL, .rxCallback = NULL, .errorCallback = NULL,   /* polled */
    .canFDMode = false, .bitrate = &g_bitrate, .fastBitrate = &g_bitrate,
    .tdcConfig = &g_tdc, .sidFilterConfig = &g_sid_cfg,
    .extidFilterConfig = &g_xid_cfg, .globalFilterConfig = &g_global_cfg,
    .rxBufferDataSize = CY_CANFD_BUFFER_DATA_SIZE_8,
    .rxFIFO1DataSize = CY_CANFD_BUFFER_DATA_SIZE_8,
    .rxFIFO0DataSize = CY_CANFD_BUFFER_DATA_SIZE_8,
    .txBufferDataSize = CY_CANFD_BUFFER_DATA_SIZE_8,
    .rxFIFO0Config = &g_rx_fifo0_cfg, .rxFIFO1Config = &g_rx_fifo1_cfg,
    .noOfRxBuffers = 1U, .noOfTxBuffers = 1U,
    .messageRAMaddress = CANBUS_MRAM_ADDRESS, .messageRAMsize = CANBUS_MRAM_SIZE,
};

/* TX/RX element storage */
static cy_stc_canfd_t0_t g_t0;
static cy_stc_canfd_t1_t g_t1;
static uint32_t g_tx_words[2];
static cy_stc_canfd_tx_buffer_t g_tx_buf = { .t0_f = &g_t0, .t1_f = &g_t1, .data_area_f = g_tx_words };
static cy_stc_canfd_r0_t g_r0;
static cy_stc_canfd_r1_t g_r1;
static uint32_t g_rx_words[2];
static cy_stc_canfd_rx_buffer_t g_rx_buf = { .r0_f = &g_r0, .r1_f = &g_r1, .data_area_f = g_rx_words };

/* Runtime counters + last frames */
static bool     g_online;
static uint32_t g_tx_count, g_rx_count, g_tx_seq;
static uint8_t  g_last_tx[8];
static uint32_t g_last_rx_id, g_last_rx_dlc;
static uint8_t  g_last_rx[8];

/* UI handles */
static lv_obj_t *status_label, *tx_count_label, *tx_data_label, *rx_count_label, *rx_frame_label;

static void can_pins_init(void)
{
    cy_stc_gpio_pin_config_t tx_cfg = { .outVal = 1U, .driveMode = CY_GPIO_DM_STRONG_IN_OFF, .hsiom = CANBUS_TX_HSIOM };
    (void)Cy_GPIO_Pin_Init(CANBUS_TX_PORT, CANBUS_TX_PIN, &tx_cfg);
    cy_stc_gpio_pin_config_t rx_cfg = { .outVal = 0U, .driveMode = CY_GPIO_DM_HIGHZ, .hsiom = CANBUS_RX_HSIOM };
    (void)Cy_GPIO_Pin_Init(CANBUS_RX_PORT, CANBUS_RX_PIN, &rx_cfg);
}

static void can_clock_init(void)
{
    Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_CANFD0_PERI_NR, CY_MMIO_CANFD0_GROUP_NR,
                                 CY_MMIO_CANFD0_SLAVE_NR, CY_MMIO_CANFD0_CLK_HF_NR);
    Cy_SysClk_PeriPclkSetDivider(CANBUS_PCLK_DST, CANBUS_DIV_TYPE, CANBUS_DIV_NUM, CANBUS_DIV_VALUE);
    Cy_SysClk_PeriPclkAssignDivider(CANBUS_PCLK_DST, CANBUS_DIV_TYPE, CANBUS_DIV_NUM);
    Cy_SysClk_PeriPclkEnableDivider(CANBUS_PCLK_DST, CANBUS_DIV_TYPE, CANBUS_DIV_NUM);
}

static bool can_init(void)
{
    can_pins_init();
    can_clock_init();
    Cy_CANFD_EnableMRAM(CANBUS_HW, CANBUS_CHANNEL_MASK, 6U);
    if (CY_CANFD_SUCCESS != Cy_CANFD_Init(CANBUS_HW, CANBUS_CHANNEL, &g_cfg, &g_ctx)) {
        return false;
    }
    Cy_CANFD_ConfigChangesEnable(CANBUS_HW, CANBUS_CHANNEL);
    /* One-shot TX: disable automatic retransmission so a frame is not held
     * pending an ACK when no peer node is on the bus. Lets the TX counter
     * advance during a single-node self-test; a real bus still ACKs normally. */
    CANBUS_HW->CH[CANBUS_CHANNEL].M_TTCAN.CCCR |= CANFD_CH_M_TTCAN_CCCR_DAR_Msk;
    Cy_CANFD_TestModeConfig(CANBUS_HW, CANBUS_CHANNEL, CY_CANFD_TEST_MODE_DISABLE);
    Cy_CANFD_ConfigChangesDisable(CANBUS_HW, CANBUS_CHANNEL);
    printf("[CAN] CANFD0 ch%u online @ 500 kbps Classic CAN (P16.2 RX / P16.3 TX)\r\n",
           (unsigned)CANBUS_CHANNEL);
    return true;
}

static void can_send(uint32_t seq)
{
    g_t0.id = CAN_TX_ID; g_t0.rtr = CY_CANFD_RTR_DATA_FRAME;
    g_t0.xtd = CY_CANFD_XTD_STANDARD_ID; g_t0.esi = CY_CANFD_ESI_ERROR_ACTIVE;
    g_t1.dlc = CAN_TX_DLC; g_t1.brs = false;
    g_t1.fdf = CY_CANFD_FDF_STANDARD_FRAME; g_t1.efc = false; g_t1.mm = 0U;
    uint8_t *p = (uint8_t *)g_tx_words;
    for (uint32_t i = 0U; i < CAN_TX_DLC; i++) { p[i] = (uint8_t)(seq + i); g_last_tx[i] = p[i]; }
    if (CY_CANFD_SUCCESS == Cy_CANFD_UpdateAndTransmitMsgBuffer(CANBUS_HW, CANBUS_CHANNEL, &g_tx_buf, 0U, &g_ctx)) {
        g_tx_count++;
    }
}

static void can_poll_rx(void)
{
    uint32_t irq = Cy_CANFD_GetInterruptStatus(CANBUS_HW, CANBUS_CHANNEL);
    if (0U != (irq & CY_CANFD_RX_FIFO_0_NEW_MESSAGE)) {
        if (CY_CANFD_SUCCESS == Cy_CANFD_GetFIFOTop(CANBUS_HW, CANBUS_CHANNEL, 0U, &g_rx_buf)) {
            g_last_rx_id  = g_r0.id;
            g_last_rx_dlc = g_r1.dlc;
            uint8_t *d = (uint8_t *)g_rx_words;
            for (uint32_t i = 0U; i < 8U; i++) { g_last_rx[i] = d[i]; }
            g_rx_count++;
        }
        Cy_CANFD_AckRxFifo(CANBUS_HW, CANBUS_CHANNEL, 0U);
        Cy_CANFD_ClearInterrupt(CANBUS_HW, CANBUS_CHANNEL, CY_CANFD_RX_FIFO_0_NEW_MESSAGE);
    }
}

static void style_label(lv_obj_t *l, uint32_t c, const lv_font_t *f)
{
    lv_obj_set_style_text_color(l, lv_color_hex(c), 0);
    lv_obj_set_style_text_font(l, f, 0);
}

static lv_obj_t *make_card(lv_obj_t *row, const char *title, uint32_t accent)
{
    lv_obj_t *card = lv_obj_create(row);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_height(card, lv_pct(100));
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x111827), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(accent), 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_style_pad_gap(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_t *t = lv_label_create(card);
    lv_label_set_text(t, title);
    style_label(t, accent, &lv_font_montserrat_16);
    return card;
}

static void can_timer_cb(lv_timer_t *timer)
{
    static uint32_t tick = 0U;
    (void)timer;

    if (g_online && ((tick % CAN_TX_EVERY_TICKS) == 0U)) {
        can_send(g_tx_seq++);
    }
    if (g_online) {
        can_poll_rx();
    }
    tick++;

    lv_label_set_text_fmt(tx_count_label, "TX frames: %lu", (unsigned long)g_tx_count);
    lv_label_set_text_fmt(tx_data_label, "ID 0x%03X  %02X %02X %02X %02X %02X %02X %02X %02X",
                          (unsigned)CAN_TX_ID, g_last_tx[0], g_last_tx[1], g_last_tx[2], g_last_tx[3],
                          g_last_tx[4], g_last_tx[5], g_last_tx[6], g_last_tx[7]);
    lv_label_set_text_fmt(rx_count_label, "RX frames: %lu", (unsigned long)g_rx_count);
    if (g_rx_count > 0U) {
        lv_label_set_text_fmt(rx_frame_label, "ID 0x%03lX dlc %lu  %02X %02X %02X %02X %02X %02X %02X %02X",
                              (unsigned long)g_last_rx_id, (unsigned long)g_last_rx_dlc,
                              g_last_rx[0], g_last_rx[1], g_last_rx[2], g_last_rx[3],
                              g_last_rx[4], g_last_rx[5], g_last_rx[6], g_last_rx[7]);
    } else {
        lv_label_set_text(rx_frame_label, "waiting for a peer node...");
    }

    if ((tick % 4U) == 0U) {
        printf("[CAN] tx=%lu rx=%lu\r\n", (unsigned long)g_tx_count, (unsigned long)g_rx_count);
    }
}

void can_monitor_ui_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    g_online = can_init();

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B0F14), 0);
    lv_obj_set_style_pad_all(screen, 16, 0);
    lv_obj_set_style_pad_gap(screen, 12, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_remove_style_all(header);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(header, 4, 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "CAN Monitor  (CANFD0 ch1, 500 kbps)");
    style_label(title, 0xF8FAFC, &lv_font_montserrat_24);
    status_label = lv_label_create(header);
    lv_label_set_text(status_label, g_online ? "Online  P16.2 RX / P16.3 TX  SN65HVD230"
                                             : "CANFD0 init FAILED");
    style_label(status_label, g_online ? 0x5EEAD4 : 0xFCA5A5, &lv_font_montserrat_16);

    lv_obj_t *row = lv_obj_create(screen);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_flex_grow(row, 1);
    lv_obj_set_style_pad_gap(row, 12, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);

    lv_obj_t *tx_card = make_card(row, "Transmit (1 Hz)", 0x22C55E);
    tx_count_label = lv_label_create(tx_card);
    lv_label_set_text(tx_count_label, "TX frames: 0");
    style_label(tx_count_label, 0xF8FAFC, &lv_font_montserrat_20);
    tx_data_label = lv_label_create(tx_card);
    lv_label_set_text(tx_data_label, "ID 0x123");
    style_label(tx_data_label, 0x94A3B8, &lv_font_montserrat_14);

    lv_obj_t *rx_card = make_card(row, "Receive (FIFO0)", 0x38BDF8);
    rx_count_label = lv_label_create(rx_card);
    lv_label_set_text(rx_count_label, "RX frames: 0");
    style_label(rx_count_label, 0xF8FAFC, &lv_font_montserrat_20);
    rx_frame_label = lv_label_create(rx_card);
    lv_label_set_text(rx_frame_label, "waiting for a peer node...");
    style_label(rx_frame_label, 0x94A3B8, &lv_font_montserrat_14);

    lv_timer_t *timer = lv_timer_create(can_timer_cb, CAN_REFRESH_PERIOD_MS, NULL);
    lv_timer_ready(timer);
}

/* [] END OF FILE */
