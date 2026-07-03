/*******************************************************************************
* File Name        : header_tester_ui.c
*
* Description      : LVGL console and controls for QWA309 header hardware tests.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "header_tester_ui.h"

#include "cybsp.h"
#include "cy_scb_i2c.h"
#include "cy_scb_uart.h"
#include "display_i2c_config.h"
#include "lv_port_indev.h"
#include "lvgl.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*******************************************************************************
* Macros
*******************************************************************************/
#define I2C_SCAN_MIN_ADDR       (0x08U)
#define I2C_SCAN_MAX_ADDR       (0x77U)
#define I2C_SCAN_ADDR_COUNT     (I2C_SCAN_MAX_ADDR - I2C_SCAN_MIN_ADDR + 1U)
#define I2C_SCAN_PROBE_TIMEOUT  (5U)
#define ESP32_SIM_ADDR          (0x30U)
#define ESP32_SIM_TIMEOUT       (50U)
#define ESP32_SIM_REQ_MAGIC     (0xA5U)
#define ESP32_SIM_RSP_MAGIC     (0x5AU)
#define ESP32_SIM_CMD_VERSION   (0x01U)
#define ESP32_SIM_CMD_GPIO_DRIVE (0x20U)
#define ESP32_SIM_REQ_SIZE      (4U)
#define ESP32_SIM_RSP_SIZE      (5U)
#define ESP32_SIM_READ_RETRIES  (4U)
#define ESP32_SIM_RETRY_DELAY_MS (10U)
#define UART_TEST_CMD_ECHO      (0x11U)
#define UART_TEST_TIMEOUT_MS    (250U)
#define UART_TEST_RETRIES       (4U)
#define UART_TEST_BAUD_TEXT     "115200 8N1"
#define SPI_TEST_CMD_ECHO       (0x31U)
#define SPI_TEST_REQ_SIZE       (8U)
#define SPI_TEST_RSP_SIZE       (5U)
#define SPI_TEST_FRAME_SIZE     (8U)
#define SPI_TEST_TIMEOUT_MS     (250U)
#define CONSOLE_BUFFER_SIZE     (4096U)
#define GPIO_WATCH_SAMPLE_COUNT (80U)
#define GPIO_WATCH_SAMPLE_MS    (100U)
#define GPIO_OUTPUT_STEP_MS     (700U)
#define PWM_TEST_CYCLES         (50U)
#define PWM_TEST_HALF_PERIOD_MS (20U)
#define ADC_LEVEL_SAMPLE_COUNT  (80U)
#define ADC_LEVEL_SAMPLE_MS     (100U)

#define HEADER_UART_HW          (SCB9)
#define HEADER_UART_RX_PORT     (GPIO_PRT15)
#define HEADER_UART_RX_PIN      (0U)
#define HEADER_UART_RX_HSIOM    (P15_0_SCB9_UART_RX)
#define HEADER_UART_TX_PORT     (GPIO_PRT15)
#define HEADER_UART_TX_PIN      (1U)
#define HEADER_UART_TX_HSIOM    (P15_1_SCB9_UART_TX)
#define HEADER_SPI_CLK_PORT     (GPIO_PRT9)
#define HEADER_SPI_CLK_PIN      (3U)
#define HEADER_SPI_MOSI_PORT    (GPIO_PRT9)
#define HEADER_SPI_MOSI_PIN     (2U)
#define HEADER_SPI_MISO_PORT    (GPIO_PRT9)
#define HEADER_SPI_MISO_PIN     (1U)
#define HEADER_SPI_SS_PORT      (GPIO_PRT9)
#define HEADER_SPI_SS_PIN       (0U)
#define HEADER_ADC0_PORT        (GPIO_PRT15)
#define HEADER_ADC0_PIN         (2U)
#define HEADER_ADC1_PORT        (GPIO_PRT15)
#define HEADER_ADC1_PIN         (3U)
#define HEADER_GPIO_COUNT       (6U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
static lv_obj_t *console_view;
static lv_obj_t *scan_button;
static lv_obj_t *esp32_button;
static lv_obj_t *uart_button;
static lv_obj_t *spi_button;
static lv_obj_t *gpio_in_button;
static lv_obj_t *gpio_out_button;
static lv_obj_t *pwm_button;
static lv_obj_t *adc_button;
static lv_obj_t *pwm3_button;
static lv_obj_t *status_label;
static char console_buffer[CONSOLE_BUFFER_SIZE];
static bool scan_in_progress;
static uint8_t esp32_test_counter;
static uint8_t uart_test_counter;
static uint8_t spi_test_counter;
static bool header_uart_initialized;
static bool header_spi_initialized;
static cy_stc_scb_uart_context_t header_uart_context;

static GPIO_PRT_Type *const header_gpio_ports[HEADER_GPIO_COUNT] =
{
    P13_0_PORT,
    P13_3_PORT,
    P13_4_PORT,
    P13_5_PORT,
    P13_6_PORT,
    P13_7_PORT,
};

static const uint8_t header_gpio_pins[HEADER_GPIO_COUNT] =
{
    P13_0_PIN,
    P13_3_PIN,
    P13_4_PIN,
    P13_5_PIN,
    P13_6_PIN,
    P13_7_PIN,
};

static const en_hsiom_sel_t header_gpio_hsiom[HEADER_GPIO_COUNT] =
{
    P13_0_GPIO,
    P13_3_GPIO,
    P13_4_GPIO,
    P13_5_GPIO,
    P13_6_GPIO,
    P13_7_GPIO,
};

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void scan_button_event_cb(lv_event_t *event);
static void esp32_button_event_cb(lv_event_t *event);
static void uart_button_event_cb(lv_event_t *event);
static void spi_button_event_cb(lv_event_t *event);
static void gpio_in_button_event_cb(lv_event_t *event);
static void gpio_out_button_event_cb(lv_event_t *event);
static void pwm_button_event_cb(lv_event_t *event);
static void adc_button_event_cb(lv_event_t *event);
static void pwm3_button_event_cb(lv_event_t *event);
static void clear_button_event_cb(lv_event_t *event);
static void scan_timer_cb(lv_timer_t *timer);
static void esp32_test_timer_cb(lv_timer_t *timer);
static void uart_test_timer_cb(lv_timer_t *timer);
static void spi_test_timer_cb(lv_timer_t *timer);
static void gpio_in_test_timer_cb(lv_timer_t *timer);
static void gpio_out_test_timer_cb(lv_timer_t *timer);
static void pwm_test_timer_cb(lv_timer_t *timer);
static void adc_level_test_timer_cb(lv_timer_t *timer);
static void pwm3_test_timer_cb(lv_timer_t *timer);
static void run_i2c_scan(void);
static void run_esp32_i2c_test(void);
static void run_uart_echo_test(void);
static void run_spi_echo_test(void);
static void run_gpio_input_test(void);
static void run_gpio_output_test(void);
static void run_pwm_output_test(void);
static void run_adc_level_input_test(void);
static void run_pwm3_output_test(void);
static bool probe_i2c_address(uint8_t address, cy_en_scb_i2c_status_t *status);
static cy_en_scb_i2c_status_t write_i2c_packet(uint8_t address,
                                               const uint8_t *data,
                                               uint32_t size);
static cy_en_scb_i2c_status_t read_i2c_packet(uint8_t address,
                                              uint8_t *data,
                                              uint32_t size);
static bool initialize_header_uart(void);
static bool uart_transfer_packet(const uint8_t *tx,
                                 uint8_t *rx,
                                 uint32_t rx_size,
                                 uint32_t *received_out);
static bool initialize_header_spi(void);
static bool spi_transfer_frame(const uint8_t *tx,
                               uint8_t *rx,
                               uint32_t size,
                               uint32_t timeout_ms);
static void configure_header_gpio_inputs(void);
static void configure_header_gpio_outputs(void);
static uint8_t read_header_gpio_mask(void);
static uint8_t read_header_gpio_output_mask(void);
static void drive_header_gpio_mask(uint8_t mask);
static void log_port13_registers(const char *tag);
static void configure_adc_level_inputs(void);
static uint8_t read_adc_level_mask(void);
static uint8_t xor_checksum(const uint8_t *data, uint32_t size);
static void console_clear(void);
static void console_append(const char *format, ...);
static void console_append_text(const char *text);
static void console_refresh(void);
static void set_status_text(const char *text);
static const char *i2c_status_to_text(cy_en_scb_i2c_status_t status);

/*******************************************************************************
* Function Name: header_tester_ui_create
********************************************************************************
* Summary:
*  Creates a minimal LVGL QWA309 header tester UI.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void header_tester_ui_create(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_t *header;
    lv_obj_t *title;
    lv_obj_t *clear_button;
    lv_obj_t *scan_label;
    lv_obj_t *esp32_label;
    lv_obj_t *uart_label;
    lv_obj_t *spi_label;
    lv_obj_t *gpio_in_label;
    lv_obj_t *gpio_out_label;
    lv_obj_t *pwm_label;
    lv_obj_t *adc_label;
    lv_obj_t *pwm3_label;
    lv_obj_t *clear_label;

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101418), 0);
    lv_obj_set_style_pad_all(screen, 14, 0);
    lv_obj_set_style_pad_gap(screen, 10, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    header = lv_obj_create(screen);
    lv_obj_remove_style_all(header);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 62);
    lv_obj_set_style_pad_gap(header, 10, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    title = lv_label_create(header);
    lv_label_set_text(title, "Header Tester");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);

    status_label = lv_label_create(header);
    lv_label_set_text(status_label, "Ready");
    lv_obj_set_flex_grow(status_label, 1);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xA7B0BD), 0);

    clear_button = lv_button_create(header);
    lv_obj_set_size(clear_button, 112, 48);
    lv_obj_set_style_radius(clear_button, 6, 0);
    lv_obj_set_style_bg_color(clear_button, lv_color_hex(0x334155), 0);
    lv_obj_add_event_cb(clear_button, clear_button_event_cb, LV_EVENT_CLICKED, NULL);

    clear_label = lv_label_create(clear_button);
    lv_label_set_text(clear_label, "Clear");
    lv_obj_center(clear_label);

    scan_button = lv_button_create(header);
    lv_obj_set_size(scan_button, 132, 48);
    lv_obj_set_style_radius(scan_button, 6, 0);
    lv_obj_set_style_bg_color(scan_button, lv_color_hex(0x0F766E), 0);
    lv_obj_add_event_cb(scan_button, scan_button_event_cb, LV_EVENT_CLICKED, NULL);

    scan_label = lv_label_create(scan_button);
    lv_label_set_text(scan_label, LV_SYMBOL_REFRESH " Scan");
    lv_obj_center(scan_label);

    esp32_button = lv_button_create(header);
    lv_obj_set_size(esp32_button, 132, 48);
    lv_obj_set_style_radius(esp32_button, 6, 0);
    lv_obj_set_style_bg_color(esp32_button, lv_color_hex(0x1D4ED8), 0);
    lv_obj_add_event_cb(esp32_button, esp32_button_event_cb, LV_EVENT_CLICKED, NULL);

    esp32_label = lv_label_create(esp32_button);
    lv_label_set_text(esp32_label, "I2C ESP32");
    lv_obj_center(esp32_label);

    uart_button = lv_button_create(header);
    lv_obj_set_size(uart_button, 132, 48);
    lv_obj_set_style_radius(uart_button, 6, 0);
    lv_obj_set_style_bg_color(uart_button, lv_color_hex(0x7C3AED), 0);
    lv_obj_add_event_cb(uart_button, uart_button_event_cb, LV_EVENT_CLICKED, NULL);

    uart_label = lv_label_create(uart_button);
    lv_label_set_text(uart_label, "UART Echo");
    lv_obj_center(uart_label);

    spi_button = lv_button_create(header);
    lv_obj_set_size(spi_button, 132, 48);
    lv_obj_set_style_radius(spi_button, 6, 0);
    lv_obj_set_style_bg_color(spi_button, lv_color_hex(0x0E7490), 0);
    lv_obj_add_event_cb(spi_button, spi_button_event_cb, LV_EVENT_CLICKED, NULL);

    spi_label = lv_label_create(spi_button);
    lv_label_set_text(spi_label, "SPI ESP32");
    lv_obj_center(spi_label);

    gpio_in_button = lv_button_create(header);
    lv_obj_set_size(gpio_in_button, 132, 48);
    lv_obj_set_style_radius(gpio_in_button, 6, 0);
    lv_obj_set_style_bg_color(gpio_in_button, lv_color_hex(0xB45309), 0);
    lv_obj_add_event_cb(gpio_in_button, gpio_in_button_event_cb, LV_EVENT_CLICKED, NULL);

    gpio_in_label = lv_label_create(gpio_in_button);
    lv_label_set_text(gpio_in_label, "GPIO In");
    lv_obj_center(gpio_in_label);

    gpio_out_button = lv_button_create(header);
    lv_obj_set_size(gpio_out_button, 132, 48);
    lv_obj_set_style_radius(gpio_out_button, 6, 0);
    lv_obj_set_style_bg_color(gpio_out_button, lv_color_hex(0xA21CAF), 0);
    lv_obj_add_event_cb(gpio_out_button, gpio_out_button_event_cb, LV_EVENT_CLICKED, NULL);

    gpio_out_label = lv_label_create(gpio_out_button);
    lv_label_set_text(gpio_out_label, "GPIO Out");
    lv_obj_center(gpio_out_label);

    pwm_button = lv_button_create(header);
    lv_obj_set_size(pwm_button, 132, 48);
    lv_obj_set_style_radius(pwm_button, 6, 0);
    lv_obj_set_style_bg_color(pwm_button, lv_color_hex(0xBE123C), 0);
    lv_obj_add_event_cb(pwm_button, pwm_button_event_cb, LV_EVENT_CLICKED, NULL);

    pwm_label = lv_label_create(pwm_button);
    lv_label_set_text(pwm_label, "PWM Out");
    lv_obj_center(pwm_label);

    adc_button = lv_button_create(header);
    lv_obj_set_size(adc_button, 132, 48);
    lv_obj_set_style_radius(adc_button, 6, 0);
    lv_obj_set_style_bg_color(adc_button, lv_color_hex(0x15803D), 0);
    lv_obj_add_event_cb(adc_button, adc_button_event_cb, LV_EVENT_CLICKED, NULL);

    adc_label = lv_label_create(adc_button);
    lv_label_set_text(adc_label, "ADC In");
    lv_obj_center(adc_label);

    pwm3_button = lv_button_create(header);
    lv_obj_set_size(pwm3_button, 132, 48);
    lv_obj_set_style_radius(pwm3_button, 6, 0);
    lv_obj_set_style_bg_color(pwm3_button, lv_color_hex(0x0F766E), 0);
    lv_obj_add_event_cb(pwm3_button, pwm3_button_event_cb, LV_EVENT_CLICKED, NULL);

    pwm3_label = lv_label_create(pwm3_button);
    lv_label_set_text(pwm3_label, "PWM3 Out");
    lv_obj_center(pwm3_label);

    console_view = lv_textarea_create(screen);
    lv_obj_set_width(console_view, lv_pct(100));
    lv_obj_set_flex_grow(console_view, 1);
    lv_obj_set_style_radius(console_view, 6, 0);
    lv_obj_set_style_bg_color(console_view, lv_color_hex(0x050A0F), 0);
    lv_obj_set_style_border_color(console_view, lv_color_hex(0x263241), 0);
    lv_obj_set_style_border_width(console_view, 1, 0);
    lv_obj_set_style_pad_all(console_view, 12, 0);
    lv_obj_set_style_text_color(console_view, lv_color_hex(0xD1FAE5), 0);
    lv_obj_set_style_text_font(console_view, &lv_font_montserrat_14, 0);
    lv_textarea_set_cursor_click_pos(console_view, false);
    lv_textarea_set_text(console_view, "");

    console_append("Header tester ready.\r\n");
    console_append("Bus: display/touch I2C controller (SCB5 on APP_KIT_PSE84_AI).\r\n");
    console_append("Press Scan to probe addresses 0x%02X..0x%02X.\r\n",
                   I2C_SCAN_MIN_ADDR,
                   I2C_SCAN_MAX_ADDR);
    console_append("Press I2C ESP32 to test simulator address 0x%02X.\r\n",
                   ESP32_SIM_ADDR);
    console_append("Press UART Echo to test P15.1 TX / P15.0 RX at %s.\r\n",
                   UART_TEST_BAUD_TEXT);
    console_append("Press SPI ESP32 to test P9.3 SCK / P9.2 MOSI / P9.1 MISO / P9.0 CS.\r\n");
    console_append("Press GPIO In to watch P13.0/P13.3/P13.4/P13.5/P13.6/P13.7 without I2C control.\r\n");
    console_append("Press GPIO Out to drive P13.0/P13.3/P13.4/P13.5/P13.6/P13.7; watch ESP32 serial log.\r\n");
    console_append("Press PWM Out to drive PWM5+/PWM5- complementary pulses on P13.3/P13.4.\r\n");
    console_append("Press ADC In to watch P15.2/P15.3 ADC/PWM3 nets as digital level inputs.\r\n");
    console_append("Press PWM3 Out to drive PWM3+/PWM3- complementary pulses on P15.2/P15.3.\r\n");
}

/*******************************************************************************
* Function Name: scan_button_event_cb
********************************************************************************/
static void scan_button_event_cb(lv_event_t *event)
{
    (void)event;

    if (scan_in_progress)
    {
        return;
    }

    scan_in_progress = true;
    set_status_text("Scanning...");
    lv_obj_add_state(scan_button, LV_STATE_DISABLED);
    lv_obj_add_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_add_state(uart_button, LV_STATE_DISABLED);
    lv_obj_add_state(spi_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_add_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_add_state(adc_button, LV_STATE_DISABLED);

    lv_timer_t *timer = lv_timer_create(scan_timer_cb, 1, NULL);
    lv_timer_set_repeat_count(timer, 1);
}

/*******************************************************************************
* Function Name: esp32_button_event_cb
********************************************************************************/
static void esp32_button_event_cb(lv_event_t *event)
{
    (void)event;

    if (scan_in_progress)
    {
        return;
    }

    scan_in_progress = true;
    set_status_text("Testing ESP32...");
    lv_obj_add_state(scan_button, LV_STATE_DISABLED);
    lv_obj_add_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_add_state(uart_button, LV_STATE_DISABLED);
    lv_obj_add_state(spi_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_add_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_add_state(adc_button, LV_STATE_DISABLED);

    lv_timer_t *timer = lv_timer_create(esp32_test_timer_cb, 1, NULL);
    lv_timer_set_repeat_count(timer, 1);
}

/*******************************************************************************
* Function Name: uart_button_event_cb
********************************************************************************/
static void uart_button_event_cb(lv_event_t *event)
{
    (void)event;

    if (scan_in_progress)
    {
        return;
    }

    scan_in_progress = true;
    set_status_text("Testing UART...");
    lv_obj_add_state(scan_button, LV_STATE_DISABLED);
    lv_obj_add_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_add_state(uart_button, LV_STATE_DISABLED);
    lv_obj_add_state(spi_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_add_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_add_state(adc_button, LV_STATE_DISABLED);

    lv_timer_t *timer = lv_timer_create(uart_test_timer_cb, 1, NULL);
    lv_timer_set_repeat_count(timer, 1);
}

/*******************************************************************************
* Function Name: spi_button_event_cb
********************************************************************************/
static void spi_button_event_cb(lv_event_t *event)
{
    (void)event;

    if (scan_in_progress)
    {
        return;
    }

    scan_in_progress = true;
    set_status_text("Testing SPI...");
    lv_obj_add_state(scan_button, LV_STATE_DISABLED);
    lv_obj_add_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_add_state(uart_button, LV_STATE_DISABLED);
    lv_obj_add_state(spi_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_add_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_add_state(adc_button, LV_STATE_DISABLED);

    lv_timer_t *timer = lv_timer_create(spi_test_timer_cb, 1, NULL);
    lv_timer_set_repeat_count(timer, 1);
}

/*******************************************************************************
* Function Name: gpio_in_button_event_cb
********************************************************************************/
static void gpio_in_button_event_cb(lv_event_t *event)
{
    (void)event;

    if (scan_in_progress)
    {
        return;
    }

    scan_in_progress = true;
    set_status_text("Testing GPIO input...");
    lv_obj_add_state(scan_button, LV_STATE_DISABLED);
    lv_obj_add_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_add_state(uart_button, LV_STATE_DISABLED);
    lv_obj_add_state(spi_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_add_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_add_state(adc_button, LV_STATE_DISABLED);

    lv_timer_t *timer = lv_timer_create(gpio_in_test_timer_cb, 1, NULL);
    lv_timer_set_repeat_count(timer, 1);
}

/*******************************************************************************
* Function Name: gpio_out_button_event_cb
********************************************************************************/
static void gpio_out_button_event_cb(lv_event_t *event)
{
    (void)event;

    if (scan_in_progress)
    {
        return;
    }

    scan_in_progress = true;
    set_status_text("Testing GPIO output...");
    lv_obj_add_state(scan_button, LV_STATE_DISABLED);
    lv_obj_add_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_add_state(uart_button, LV_STATE_DISABLED);
    lv_obj_add_state(spi_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_add_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_add_state(adc_button, LV_STATE_DISABLED);

    lv_timer_t *timer = lv_timer_create(gpio_out_test_timer_cb, 1, NULL);
    lv_timer_set_repeat_count(timer, 1);
}

/*******************************************************************************
* Function Name: pwm_button_event_cb
********************************************************************************/
static void pwm_button_event_cb(lv_event_t *event)
{
    (void)event;

    if (scan_in_progress)
    {
        return;
    }

    scan_in_progress = true;
    set_status_text("Testing PWM output...");
    lv_obj_add_state(scan_button, LV_STATE_DISABLED);
    lv_obj_add_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_add_state(uart_button, LV_STATE_DISABLED);
    lv_obj_add_state(spi_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_add_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_add_state(adc_button, LV_STATE_DISABLED);

    lv_timer_t *timer = lv_timer_create(pwm_test_timer_cb, 1, NULL);
    lv_timer_set_repeat_count(timer, 1);
}

/*******************************************************************************
* Function Name: adc_button_event_cb
********************************************************************************/
static void adc_button_event_cb(lv_event_t *event)
{
    (void)event;

    if (scan_in_progress)
    {
        return;
    }

    scan_in_progress = true;
    set_status_text("Testing ADC nets...");
    lv_obj_add_state(scan_button, LV_STATE_DISABLED);
    lv_obj_add_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_add_state(uart_button, LV_STATE_DISABLED);
    lv_obj_add_state(spi_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_add_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_add_state(adc_button, LV_STATE_DISABLED);

    lv_timer_t *timer = lv_timer_create(adc_level_test_timer_cb, 1, NULL);
    lv_timer_set_repeat_count(timer, 1);
}

/*******************************************************************************
* Function Name: pwm3_button_event_cb
********************************************************************************/
static void pwm3_button_event_cb(lv_event_t *event)
{
    (void)event;

    if (scan_in_progress)
    {
        return;
    }

    scan_in_progress = true;
    set_status_text("Testing PWM3 output...");
    lv_obj_add_state(scan_button, LV_STATE_DISABLED);
    lv_obj_add_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_add_state(uart_button, LV_STATE_DISABLED);
    lv_obj_add_state(spi_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_add_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_add_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_add_state(adc_button, LV_STATE_DISABLED);
    lv_obj_add_state(pwm3_button, LV_STATE_DISABLED);

    lv_timer_t *timer = lv_timer_create(pwm3_test_timer_cb, 1, NULL);
    lv_timer_set_repeat_count(timer, 1);
}

/*******************************************************************************
* Function Name: clear_button_event_cb
********************************************************************************/
static void clear_button_event_cb(lv_event_t *event)
{
    (void)event;

    console_clear();
    console_append("Console cleared.\r\n");
    set_status_text(scan_in_progress ? "Scanning..." : "Ready");
}

/*******************************************************************************
* Function Name: scan_timer_cb
********************************************************************************/
static void scan_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    run_i2c_scan();

    scan_in_progress = false;
    lv_obj_remove_state(scan_button, LV_STATE_DISABLED);
    lv_obj_remove_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_remove_state(uart_button, LV_STATE_DISABLED);
    lv_obj_remove_state(spi_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_remove_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_remove_state(adc_button, LV_STATE_DISABLED);
}

/*******************************************************************************
* Function Name: esp32_test_timer_cb
********************************************************************************/
static void esp32_test_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    run_esp32_i2c_test();

    scan_in_progress = false;
    lv_obj_remove_state(scan_button, LV_STATE_DISABLED);
    lv_obj_remove_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_remove_state(uart_button, LV_STATE_DISABLED);
    lv_obj_remove_state(spi_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_remove_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_remove_state(adc_button, LV_STATE_DISABLED);
}

/*******************************************************************************
* Function Name: uart_test_timer_cb
********************************************************************************/
static void uart_test_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    run_uart_echo_test();

    scan_in_progress = false;
    lv_obj_remove_state(scan_button, LV_STATE_DISABLED);
    lv_obj_remove_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_remove_state(uart_button, LV_STATE_DISABLED);
    lv_obj_remove_state(spi_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_remove_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_remove_state(adc_button, LV_STATE_DISABLED);
}

/*******************************************************************************
* Function Name: spi_test_timer_cb
********************************************************************************/
static void spi_test_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    run_spi_echo_test();

    scan_in_progress = false;
    lv_obj_remove_state(scan_button, LV_STATE_DISABLED);
    lv_obj_remove_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_remove_state(uart_button, LV_STATE_DISABLED);
    lv_obj_remove_state(spi_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_remove_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_remove_state(adc_button, LV_STATE_DISABLED);
}

/*******************************************************************************
* Function Name: gpio_in_test_timer_cb
********************************************************************************/
static void gpio_in_test_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    run_gpio_input_test();

    scan_in_progress = false;
    lv_obj_remove_state(scan_button, LV_STATE_DISABLED);
    lv_obj_remove_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_remove_state(uart_button, LV_STATE_DISABLED);
    lv_obj_remove_state(spi_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_remove_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_remove_state(adc_button, LV_STATE_DISABLED);
}

/*******************************************************************************
* Function Name: gpio_out_test_timer_cb
********************************************************************************/
static void gpio_out_test_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    run_gpio_output_test();

    scan_in_progress = false;
    lv_obj_remove_state(scan_button, LV_STATE_DISABLED);
    lv_obj_remove_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_remove_state(uart_button, LV_STATE_DISABLED);
    lv_obj_remove_state(spi_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_remove_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_remove_state(adc_button, LV_STATE_DISABLED);
}

/*******************************************************************************
* Function Name: pwm_test_timer_cb
********************************************************************************/
static void pwm_test_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    run_pwm_output_test();

    scan_in_progress = false;
    lv_obj_remove_state(scan_button, LV_STATE_DISABLED);
    lv_obj_remove_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_remove_state(uart_button, LV_STATE_DISABLED);
    lv_obj_remove_state(spi_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_remove_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_remove_state(adc_button, LV_STATE_DISABLED);
}

/*******************************************************************************
* Function Name: adc_level_test_timer_cb
********************************************************************************/
static void adc_level_test_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    run_adc_level_input_test();

    scan_in_progress = false;
    lv_obj_remove_state(scan_button, LV_STATE_DISABLED);
    lv_obj_remove_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_remove_state(uart_button, LV_STATE_DISABLED);
    lv_obj_remove_state(spi_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_remove_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_remove_state(adc_button, LV_STATE_DISABLED);
}

/*******************************************************************************
* Function Name: pwm3_test_timer_cb
********************************************************************************/
static void pwm3_test_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    run_pwm3_output_test();

    scan_in_progress = false;
    lv_obj_remove_state(scan_button, LV_STATE_DISABLED);
    lv_obj_remove_state(esp32_button, LV_STATE_DISABLED);
    lv_obj_remove_state(uart_button, LV_STATE_DISABLED);
    lv_obj_remove_state(spi_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_in_button, LV_STATE_DISABLED);
    lv_obj_remove_state(gpio_out_button, LV_STATE_DISABLED);
    lv_obj_remove_state(pwm_button, LV_STATE_DISABLED);
    lv_obj_remove_state(adc_button, LV_STATE_DISABLED);
    lv_obj_remove_state(pwm3_button, LV_STATE_DISABLED);
}

/*******************************************************************************
* Function Name: run_i2c_scan
********************************************************************************/
static void run_i2c_scan(void)
{
    uint8_t found_addresses[I2C_SCAN_ADDR_COUNT];
    uint8_t found_count = 0U;
    char row_text[80];
    int offset;

    console_append("\r\n[%lu ms] === I2C Bus Scan ===\r\n",
                   (unsigned long)lv_tick_get());
    console_append("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n");

    for (uint8_t row = 0U; row < 8U; row++)
    {
        offset = snprintf(row_text, sizeof(row_text), "%02X:", (unsigned)(row * 16U));

        for (uint8_t col = 0U; col < 16U; col++)
        {
            uint8_t address = (uint8_t)(row * 16U + col);

            if ((address < I2C_SCAN_MIN_ADDR) || (address > I2C_SCAN_MAX_ADDR))
            {
                offset += snprintf(&row_text[offset],
                                   sizeof(row_text) - (size_t)offset,
                                   "   ");
                continue;
            }

            bool device_found = probe_i2c_address(address, NULL);

            offset += snprintf(&row_text[offset],
                               sizeof(row_text) - (size_t)offset,
                               device_found ? " %02X" : " --",
                               (unsigned)address);

            if (device_found && (found_count < I2C_SCAN_ADDR_COUNT))
            {
                found_addresses[found_count] = address;
                found_count++;
            }
        }

        console_append("%s\r\n", row_text);
    }

    if (found_count > 0U)
    {
        console_append("Found %u device(s):", (unsigned)found_count);

        for (uint8_t i = 0U; i < found_count; i++)
        {
            console_append(" 0x%02X", (unsigned)found_addresses[i]);
        }

        console_append("\r\n");
        set_status_text("Scan complete");
    }
    else
    {
        console_append("No devices found. Check wiring, power, and pull-ups.\r\n");
        set_status_text("No devices found");
    }

    console_append("=== Scan complete ===\r\n");
}

/*******************************************************************************
* Function Name: run_esp32_i2c_test
********************************************************************************/
static void run_esp32_i2c_test(void)
{
    uint8_t tx[ESP32_SIM_REQ_SIZE] =
    {
        ESP32_SIM_REQ_MAGIC,
        ESP32_SIM_CMD_VERSION,
        esp32_test_counter++,
        0U,
    };
    uint8_t rx[ESP32_SIM_RSP_SIZE] = {0U};
    cy_en_scb_i2c_status_t status;
    bool valid;
    bool got_response = false;

    tx[3] = xor_checksum(tx, ESP32_SIM_REQ_SIZE - 1U);

    console_append("\r\n[%lu ms] === ESP32-S3 I2C Simulator 0x%02X ===\r\n",
                   (unsigned long)lv_tick_get(),
                   (unsigned)ESP32_SIM_ADDR);
    console_append("TX: %02X %02X %02X %02X\r\n",
                   (unsigned)tx[0],
                   (unsigned)tx[1],
                   (unsigned)tx[2],
                   (unsigned)tx[3]);

    status = write_i2c_packet(ESP32_SIM_ADDR, tx, ESP32_SIM_REQ_SIZE);
    if (status != CY_SCB_I2C_SUCCESS)
    {
        console_append("WRITE failed: %s\r\n", i2c_status_to_text(status));
        console_append("Result: FAIL - ESP32 did not ACK request\r\n");
        set_status_text("ESP32 I2C FAIL");
        return;
    }

    for (uint32_t attempt = 1U; attempt <= ESP32_SIM_READ_RETRIES; attempt++)
    {
        Cy_SysLib_Delay(ESP32_SIM_RETRY_DELAY_MS);

        status = read_i2c_packet(ESP32_SIM_ADDR, rx, ESP32_SIM_RSP_SIZE);
        if (status != CY_SCB_I2C_SUCCESS)
        {
            console_append("RX try %lu failed: %s\r\n",
                           (unsigned long)attempt,
                           i2c_status_to_text(status));
            continue;
        }

        console_append("RX try %lu: %02X %02X %02X %02X %02X\r\n",
                       (unsigned long)attempt,
                       (unsigned)rx[0],
                       (unsigned)rx[1],
                       (unsigned)rx[2],
                       (unsigned)rx[3],
                       (unsigned)rx[4]);

        valid = (rx[0] == ESP32_SIM_RSP_MAGIC) &&
                (rx[1] == ESP32_SIM_CMD_VERSION) &&
                (rx[2] == tx[2]) &&
                (xor_checksum(rx, ESP32_SIM_RSP_SIZE - 1U) == rx[4]);

        if (valid)
        {
            got_response = true;
            break;
        }
    }

    if (got_response)
    {
        console_append("Result: PASS - ESP32 I2C simulator responded, status=0x%02X\r\n",
                       (unsigned)rx[3]);
        set_status_text("ESP32 I2C PASS");
    }
    else
    {
        console_append("Result: FAIL - bad ESP32 response header/counter/checksum after retries\r\n");
        set_status_text("ESP32 I2C FAIL");
    }
}

/*******************************************************************************
* Function Name: run_uart_echo_test
********************************************************************************/
static void run_uart_echo_test(void)
{
    uint8_t tx[ESP32_SIM_REQ_SIZE] =
    {
        ESP32_SIM_REQ_MAGIC,
        UART_TEST_CMD_ECHO,
        uart_test_counter++,
        0U,
    };
    uint8_t rx[ESP32_SIM_RSP_SIZE] = {0U};
    bool response_valid;
    bool got_response = false;

    tx[3] = xor_checksum(tx, ESP32_SIM_REQ_SIZE - 1U);

    console_append("\r\n[%lu ms] === ESP32-S3 UART Echo ===\r\n",
                   (unsigned long)lv_tick_get());
    console_append("Port: P15.1 TX -> ESP RX, P15.0 RX <- ESP TX, %s\r\n",
                   UART_TEST_BAUD_TEXT);
    console_append("TX: %02X %02X %02X %02X\r\n",
                   (unsigned)tx[0],
                   (unsigned)tx[1],
                   (unsigned)tx[2],
                   (unsigned)tx[3]);

    if (!initialize_header_uart())
    {
        console_append("Result: FAIL - SCB9 UART init failed\r\n");
        set_status_text("UART init FAIL");
        return;
    }

    for (uint32_t attempt = 1U; attempt <= UART_TEST_RETRIES; attempt++)
    {
        uint32_t received = 0U;
        response_valid = false;

        if (uart_transfer_packet(tx, rx, ESP32_SIM_RSP_SIZE, &received))
        {
            console_append("RX try %lu: %02X %02X %02X %02X %02X\r\n",
                           (unsigned long)attempt,
                           (unsigned)rx[0],
                           (unsigned)rx[1],
                           (unsigned)rx[2],
                           (unsigned)rx[3],
                           (unsigned)rx[4]);

            response_valid = (rx[0] == ESP32_SIM_RSP_MAGIC) &&
                             (rx[1] == UART_TEST_CMD_ECHO) &&
                             (rx[2] == tx[2]) &&
                             (rx[3] == UART_TEST_CMD_ECHO) &&
                             (xor_checksum(rx, ESP32_SIM_RSP_SIZE - 1U) == rx[4]);
        }
        else
        {
            console_append("RX try %lu timeout, received=%lu",
                           (unsigned long)attempt,
                           (unsigned long)received);

            for (uint32_t i = 0U; i < received; i++)
            {
                console_append(" %02X", (unsigned)rx[i]);
            }

            console_append("\r\n");
        }

        if (response_valid)
        {
            got_response = true;
            break;
        }

        Cy_SysLib_Delay(10U);
    }

    if (got_response)
    {
        console_append("Result: PASS - ESP32 UART echo responded, status=0x%02X\r\n",
                       (unsigned)rx[3]);
        set_status_text("UART Echo PASS");
    }
    else
    {
        console_append("Result: FAIL - UART response timeout/header/counter/checksum after retries\r\n");
        set_status_text("UART Echo FAIL");
    }
}

/*******************************************************************************
* Function Name: run_spi_echo_test
********************************************************************************/
static void run_spi_echo_test(void)
{
    uint8_t tx_req[SPI_TEST_FRAME_SIZE] =
    {
        ESP32_SIM_REQ_MAGIC,
        SPI_TEST_CMD_ECHO,
        spi_test_counter++,
        0x5AU,
        0x01U,
        0x02U,
        0x03U,
        0U,
    };
    uint8_t tx_dummy[SPI_TEST_FRAME_SIZE] =
    {
        0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
    };
    uint8_t rx_discard[SPI_TEST_FRAME_SIZE] = {0U};
    uint8_t rx_frame[SPI_TEST_FRAME_SIZE] = {0U};
    bool response_valid;

    tx_req[SPI_TEST_FRAME_SIZE - 1U] = xor_checksum(tx_req, SPI_TEST_FRAME_SIZE - 1U);

    console_append("\r\n[%lu ms] === ESP32-S3 SPI Slave ===\r\n",
                   (unsigned long)lv_tick_get());
    console_append("Port: P9.3 SCK -> ESP GPIO4, P9.2 MOSI -> GPIO5, P9.1 MISO <- GPIO6, P9.0 CS -> GPIO7\r\n");
    console_append("Mode: GPIO bit-bang SPI mode 0 for header HW validation\r\n");
    console_append("TX req: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                   (unsigned)tx_req[0],
                   (unsigned)tx_req[1],
                   (unsigned)tx_req[2],
                   (unsigned)tx_req[3],
                   (unsigned)tx_req[4],
                   (unsigned)tx_req[5],
                   (unsigned)tx_req[6],
                   (unsigned)tx_req[7]);

    if (!initialize_header_spi())
    {
        console_append("Result: FAIL - SCB1 SPI init failed\r\n");
        set_status_text("SPI init FAIL");
        return;
    }

    if (!spi_transfer_frame(tx_req, rx_discard, SPI_TEST_FRAME_SIZE, SPI_TEST_TIMEOUT_MS))
    {
        console_append("Result: FAIL - SPI request transaction timed out\r\n");
        set_status_text("SPI FAIL");
        return;
    }

    console_append("RX req-phase: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                   (unsigned)rx_discard[0],
                   (unsigned)rx_discard[1],
                   (unsigned)rx_discard[2],
                   (unsigned)rx_discard[3],
                   (unsigned)rx_discard[4],
                   (unsigned)rx_discard[5],
                   (unsigned)rx_discard[6],
                   (unsigned)rx_discard[7]);

    Cy_SysLib_Delay(10U);

    if (!spi_transfer_frame(tx_dummy, rx_frame, SPI_TEST_FRAME_SIZE, SPI_TEST_TIMEOUT_MS))
    {
        console_append("Result: FAIL - SPI response transaction timed out\r\n");
        set_status_text("SPI FAIL");
        return;
    }

    console_append("RX rsp: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                   (unsigned)rx_frame[0],
                   (unsigned)rx_frame[1],
                   (unsigned)rx_frame[2],
                   (unsigned)rx_frame[3],
                   (unsigned)rx_frame[4],
                   (unsigned)rx_frame[5],
                   (unsigned)rx_frame[6],
                   (unsigned)rx_frame[7]);

    response_valid = (rx_frame[0] == ESP32_SIM_RSP_MAGIC) &&
                     (rx_frame[1] == SPI_TEST_CMD_ECHO) &&
                     (rx_frame[2] == tx_req[2]) &&
                     (rx_frame[3] == SPI_TEST_CMD_ECHO) &&
                     (xor_checksum(rx_frame, SPI_TEST_RSP_SIZE - 1U) == rx_frame[4]);

    if (response_valid)
    {
        console_append("Result: PASS - ESP32 SPI slave responded, status=0x%02X\r\n",
                       (unsigned)rx_frame[3]);
        set_status_text("SPI PASS");
    }
    else
    {
        console_append("Result: FAIL - bad SPI response header/counter/checksum\r\n");
        set_status_text("SPI FAIL");
    }
}

/*******************************************************************************
* Function Name: run_gpio_input_test
********************************************************************************/
static void run_gpio_input_test(void)
{
    uint8_t last_mask = 0xFFU;
    uint8_t seen_high_mask = 0U;
    uint8_t seen_pattern_mask = 0U;
    uint8_t invalid_mask = 0U;
    uint8_t first_onehot = 0U;
    uint8_t previous_onehot = 0U;
    uint32_t transitions = 0U;
    uint32_t order_errors = 0U;

    configure_header_gpio_inputs();

    console_append("\r\n[%lu ms] === GPIO Watch: ESP32 auto-drive -> PSoC Read ===\r\n",
                   (unsigned long)lv_tick_get());
    console_append("Map bit0..5 = P13.0 P13.3 P13.4 P13.5 P13.6 P13.7\r\n");
    console_append("Connect all six GPIO signals together, or use one signal wire and repeat per net.\r\n");

    for (uint32_t sample = 0U; sample < GPIO_WATCH_SAMPLE_COUNT; sample++)
    {
        uint8_t mask = read_header_gpio_mask();

        seen_high_mask |= mask;

        if (mask == 0x00U)
        {
            seen_pattern_mask |= 0x40U;
        }
        else if ((mask & (uint8_t)(mask - 1U)) == 0U)
        {
            seen_pattern_mask |= mask;

            if (mask != previous_onehot)
            {
                if (previous_onehot == 0U)
                {
                    first_onehot = mask;
                }
                else
                {
                    uint8_t expected_next = (previous_onehot < 0x20U) ?
                                            (uint8_t)(previous_onehot << 1U) :
                                            0x01U;
                    if (mask != expected_next)
                    {
                        order_errors++;
                    }
                }

                previous_onehot = mask;
            }
        }
        else
        {
            invalid_mask |= mask;
        }

        if (mask != last_mask)
        {
            console_append("Sample %02lu mask=0x%02X\r\n",
                           (unsigned long)sample,
                           (unsigned)mask);
            last_mask = mask;
            transitions++;
        }

        Cy_SysLib_Delay(GPIO_WATCH_SAMPLE_MS);
    }

    console_append("Seen high bits mask=0x%02X transitions=%lu\r\n",
                   (unsigned)seen_high_mask,
                   (unsigned long)transitions);
    console_append("Seen pattern flags=0x%02X invalid-combined-mask=0x%02X\r\n",
                   (unsigned)seen_pattern_mask,
                   (unsigned)invalid_mask);
    console_append("Sequence first-onehot=0x%02X order-errors=%lu\r\n",
                   (unsigned)first_onehot,
                   (unsigned long)order_errors);

    if ((seen_high_mask == 0x3FU) &&
        ((seen_pattern_mask & 0x7FU) == 0x7FU) &&
        (invalid_mask == 0U) &&
        (order_errors == 0U))
    {
        console_append("Result: PASS - all six GPIO inputs saw the autonomous walking pattern\r\n");
        set_status_text("GPIO In PASS");
    }
    else if (seen_high_mask != 0U)
    {
        console_append("Result: PARTIAL - GPIO activity seen, but not all six clean one-hot states were captured\r\n");
        set_status_text("GPIO In PARTIAL");
    }
    else
    {
        console_append("Result: FAIL - no GPIO activity seen; check selected wire, GND, ESP32 pin, and P13 net\r\n");
        set_status_text("GPIO In FAIL");
    }
}

/*******************************************************************************
* Function Name: run_gpio_output_test
********************************************************************************/
static void run_gpio_output_test(void)
{
    configure_header_gpio_outputs();

    console_append("\r\n[%lu ms] === GPIO Out: PSoC Drive -> ESP32 Read ===\r\n",
                   (unsigned long)lv_tick_get());
    console_append("Map bit0..5 = P13.0 P13.3 P13.4 P13.5 P13.6 P13.7\r\n");
    console_append("ESP32 firmware must be in GPIO input-watch mode. Check ESP32 serial for matching masks.\r\n");
    log_port13_registers("after-config");

    for (uint8_t step = 0U; step <= HEADER_GPIO_COUNT; step++)
    {
        uint8_t mask = (step < HEADER_GPIO_COUNT) ?
                       (uint8_t)(1U << step) :
                       0x00U;

        drive_header_gpio_mask(mask);
        Cy_SysLib_Delay(5U);
        uint8_t readback = read_header_gpio_output_mask();
        console_append("Drive step %u mask=0x%02X readback=0x%02X\r\n",
                       (unsigned)step,
                       (unsigned)mask,
                       (unsigned)readback);
        log_port13_registers("step");
        Cy_SysLib_Delay(GPIO_OUTPUT_STEP_MS);
    }

    for (uint8_t repeat = 0U; repeat < 2U; repeat++)
    {
        for (uint8_t step = 0U; step <= HEADER_GPIO_COUNT; step++)
        {
            uint8_t mask = (step < HEADER_GPIO_COUNT) ?
                           (uint8_t)(1U << step) :
                           0x00U;

            drive_header_gpio_mask(mask);
            Cy_SysLib_Delay(5U);
            uint8_t readback = read_header_gpio_output_mask();
            console_append("Repeat %u step %u mask=0x%02X readback=0x%02X\r\n",
                           (unsigned)repeat,
                           (unsigned)step,
                           (unsigned)mask,
                           (unsigned)readback);
            log_port13_registers("repeat");
            Cy_SysLib_Delay(GPIO_OUTPUT_STEP_MS);
        }
    }

    drive_header_gpio_mask(0x00U);

    console_append("Result: SENT - verify ESP32 serial saw 0x01,0x02,0x04,0x08,0x10,0x20,0x00; PSoC pins remain output-low\r\n");
    set_status_text("GPIO Out sent");
}

/*******************************************************************************
* Function Name: run_pwm_output_test
********************************************************************************/
static void run_pwm_output_test(void)
{
    Cy_GPIO_Pin_FastInit(P13_3_PORT,
                         P13_3_PIN,
                         CY_GPIO_DM_STRONG_IN_OFF,
                         0U,
                         P13_3_GPIO);
    Cy_GPIO_Pin_FastInit(P13_4_PORT,
                         P13_4_PIN,
                         CY_GPIO_DM_STRONG_IN_OFF,
                         0U,
                         P13_4_GPIO);

    console_append("\r\n[%lu ms] === PWM5 Complementary Output ===\r\n",
                   (unsigned long)lv_tick_get());
    console_append("PWM5+ = P13.3_GPIO_PWM5+_3V3, PWM5- = P13.4_GPIO_PWM5-_3V3\r\n");
    console_append("Meaning: digital complementary pair, not analog positive/negative voltage.\r\n");
    console_append("Pattern: %u cycles, half-period=%u ms, expected frequency about %u Hz\r\n",
                   (unsigned)PWM_TEST_CYCLES,
                   (unsigned)PWM_TEST_HALF_PERIOD_MS,
                   (unsigned)(1000U / (PWM_TEST_HALF_PERIOD_MS * 2U)));
    console_append("ESP32 PWM watch wiring: P13.3 -> GPIO5, P13.4 -> GPIO6, GND -> GND\r\n");

    for (uint32_t cycle = 0U; cycle < PWM_TEST_CYCLES; cycle++)
    {
        Cy_GPIO_Write(P13_3_PORT, P13_3_PIN, 1U);
        Cy_GPIO_Write(P13_4_PORT, P13_4_PIN, 0U);
        Cy_SysLib_Delay(PWM_TEST_HALF_PERIOD_MS);

        Cy_GPIO_Write(P13_3_PORT, P13_3_PIN, 0U);
        Cy_GPIO_Write(P13_4_PORT, P13_4_PIN, 1U);
        Cy_SysLib_Delay(PWM_TEST_HALF_PERIOD_MS);
    }

    Cy_GPIO_Write(P13_3_PORT, P13_3_PIN, 0U);
    Cy_GPIO_Write(P13_4_PORT, P13_4_PIN, 0U);

    console_append("Result: SENT - verify ESP32 serial saw PWM5+ and PWM5- edges with no both-high fault\r\n");
    set_status_text("PWM Out sent");
}

/*******************************************************************************
* Function Name: run_adc_level_input_test
********************************************************************************/
static void run_adc_level_input_test(void)
{
    uint8_t seen_mask = 0U;
    uint8_t invalid_mask = 0U;
    uint8_t last_mask = 0xFFU;
    uint32_t transitions = 0U;

    configure_adc_level_inputs();

    console_append("\r\n[%lu ms] === ADC/PWM3 Net Level Input ===\r\n",
                   (unsigned long)lv_tick_get());
    console_append("Map bit0=P15.2_ADC_2_PWM3+_3V3, bit1=P15.3_ADC_3_PWM3-_3V3\r\n");
    console_append("ESP32 ADC level source wiring: P15.2 -> GPIO4, P15.3 -> GPIO5, GND -> GND\r\n");
    console_append("This validates header routing/level threshold first; SAR ADC voltage conversion is a later firmware integration step.\r\n");

    for (uint32_t sample = 0U; sample < ADC_LEVEL_SAMPLE_COUNT; sample++)
    {
        uint8_t mask = read_adc_level_mask();

        if (mask <= 0x03U)
        {
            seen_mask |= (uint8_t)(1U << mask);
        }
        else
        {
            invalid_mask |= mask;
        }

        if (mask != last_mask)
        {
            console_append("Sample %02lu mask=0x%02X\r\n",
                           (unsigned long)sample,
                           (unsigned)mask);
            last_mask = mask;
            transitions++;
        }

        Cy_SysLib_Delay(ADC_LEVEL_SAMPLE_MS);
    }

    console_append("Seen states flags=0x%02X transitions=%lu invalid-mask=0x%02X\r\n",
                   (unsigned)seen_mask,
                   (unsigned long)transitions,
                   (unsigned)invalid_mask);

    if ((seen_mask == 0x0FU) && (invalid_mask == 0U))
    {
        console_append("Result: PASS - P15.2/P15.3 ADC/PWM3 nets saw all 00/01/10/11 level states\r\n");
        set_status_text("ADC nets PASS");
    }
    else if (seen_mask != 0U)
    {
        console_append("Result: PARTIAL - ADC/PWM3 net activity seen, but not all four states were captured\r\n");
        set_status_text("ADC nets PARTIAL");
    }
    else
    {
        console_append("Result: FAIL - no ADC/PWM3 net activity seen; check wiring, GND, and ESP32 source mode\r\n");
        set_status_text("ADC nets FAIL");
    }
}

/*******************************************************************************
* Function Name: run_pwm3_output_test
********************************************************************************/
static void run_pwm3_output_test(void)
{
    Cy_GPIO_Pin_FastInit(HEADER_ADC0_PORT,
                         HEADER_ADC0_PIN,
                         CY_GPIO_DM_STRONG_IN_OFF,
                         0U,
                         P15_2_GPIO);
    Cy_GPIO_Pin_FastInit(HEADER_ADC1_PORT,
                         HEADER_ADC1_PIN,
                         CY_GPIO_DM_STRONG_IN_OFF,
                         0U,
                         P15_3_GPIO);

    console_append("\r\n[%lu ms] === PWM3 Complementary Output ===\r\n",
                   (unsigned long)lv_tick_get());
    console_append("PWM3+ = P15.2_ADC_2_PWM3+_3V3, PWM3- = P15.3_ADC_3_PWM3-_3V3\r\n");
    console_append("Meaning: digital complementary pair on ADC/PWM3 shared nets; this is not ADC output.\r\n");
    console_append("Pattern: %u cycles, half-period=%u ms, expected frequency about %u Hz\r\n",
                   (unsigned)PWM_TEST_CYCLES,
                   (unsigned)PWM_TEST_HALF_PERIOD_MS,
                   (unsigned)(1000U / (PWM_TEST_HALF_PERIOD_MS * 2U)));
    console_append("ESP32 PWM watch wiring: P15.2 -> GPIO4, P15.3 -> GPIO5, GND -> GND\r\n");

    for (uint32_t cycle = 0U; cycle < PWM_TEST_CYCLES; cycle++)
    {
        Cy_GPIO_Write(HEADER_ADC0_PORT, HEADER_ADC0_PIN, 1U);
        Cy_GPIO_Write(HEADER_ADC1_PORT, HEADER_ADC1_PIN, 0U);
        Cy_SysLib_Delay(PWM_TEST_HALF_PERIOD_MS);

        Cy_GPIO_Write(HEADER_ADC0_PORT, HEADER_ADC0_PIN, 0U);
        Cy_GPIO_Write(HEADER_ADC1_PORT, HEADER_ADC1_PIN, 1U);
        Cy_SysLib_Delay(PWM_TEST_HALF_PERIOD_MS);
    }

    Cy_GPIO_Write(HEADER_ADC0_PORT, HEADER_ADC0_PIN, 0U);
    Cy_GPIO_Write(HEADER_ADC1_PORT, HEADER_ADC1_PIN, 0U);

    console_append("Result: SENT - verify ESP32 serial saw PWM3+ and PWM3- edges with no both-high fault\r\n");
    set_status_text("PWM3 Out sent");
}

/*******************************************************************************
* Function Name: probe_i2c_address
********************************************************************************/
static bool probe_i2c_address(uint8_t address, cy_en_scb_i2c_status_t *status)
{
    cy_en_scb_i2c_status_t probe_status;

    probe_status = Cy_SCB_I2C_MasterSendStart(DISPLAY_I2C_CONTROLLER_HW,
                                              address,
                                              CY_SCB_I2C_WRITE_XFER,
                                              I2C_SCAN_PROBE_TIMEOUT,
                                              &disp_touch_i2c_controller_context);

    (void)Cy_SCB_I2C_MasterSendStop(DISPLAY_I2C_CONTROLLER_HW,
                                    I2C_SCAN_PROBE_TIMEOUT,
                                    &disp_touch_i2c_controller_context);

    if (status != NULL)
    {
        *status = probe_status;
    }

    return (probe_status == CY_SCB_I2C_SUCCESS);
}

/*******************************************************************************
* Function Name: write_i2c_packet
********************************************************************************/
static cy_en_scb_i2c_status_t write_i2c_packet(uint8_t address,
                                               const uint8_t *data,
                                               uint32_t size)
{
    cy_en_scb_i2c_status_t status;
    cy_en_scb_i2c_status_t stop_status;

    status = Cy_SCB_I2C_MasterSendStart(DISPLAY_I2C_CONTROLLER_HW,
                                        address,
                                        CY_SCB_I2C_WRITE_XFER,
                                        ESP32_SIM_TIMEOUT,
                                        &disp_touch_i2c_controller_context);

    for (uint32_t i = 0U; (i < size) && (status == CY_SCB_I2C_SUCCESS); i++)
    {
        status = Cy_SCB_I2C_MasterWriteByte(DISPLAY_I2C_CONTROLLER_HW,
                                            data[i],
                                            ESP32_SIM_TIMEOUT,
                                            &disp_touch_i2c_controller_context);
    }

    stop_status = Cy_SCB_I2C_MasterSendStop(DISPLAY_I2C_CONTROLLER_HW,
                                            ESP32_SIM_TIMEOUT,
                                            &disp_touch_i2c_controller_context);

    return (status == CY_SCB_I2C_SUCCESS) ? stop_status : status;
}

/*******************************************************************************
* Function Name: read_i2c_packet
********************************************************************************/
static cy_en_scb_i2c_status_t read_i2c_packet(uint8_t address,
                                              uint8_t *data,
                                              uint32_t size)
{
    cy_en_scb_i2c_status_t status;
    cy_en_scb_i2c_status_t stop_status;

    status = Cy_SCB_I2C_MasterSendStart(DISPLAY_I2C_CONTROLLER_HW,
                                        address,
                                        CY_SCB_I2C_READ_XFER,
                                        ESP32_SIM_TIMEOUT,
                                        &disp_touch_i2c_controller_context);

    for (uint32_t i = 0U; (i < size) && (status == CY_SCB_I2C_SUCCESS); i++)
    {
        cy_en_scb_i2c_command_t ack =
            (i == (size - 1U)) ? CY_SCB_I2C_NAK : CY_SCB_I2C_ACK;

        status = Cy_SCB_I2C_MasterReadByte(DISPLAY_I2C_CONTROLLER_HW,
                                           ack,
                                           &data[i],
                                           ESP32_SIM_TIMEOUT,
                                           &disp_touch_i2c_controller_context);
    }

    stop_status = Cy_SCB_I2C_MasterSendStop(DISPLAY_I2C_CONTROLLER_HW,
                                            ESP32_SIM_TIMEOUT,
                                            &disp_touch_i2c_controller_context);

    return (status == CY_SCB_I2C_SUCCESS) ? stop_status : status;
}

static bool initialize_header_uart(void)
{
    static const cy_stc_scb_uart_config_t header_uart_config =
    {
        .uartMode = CY_SCB_UART_STANDARD,
        .enableMultiProcessorMode = false,
        .smartCardRetryOnNack = false,
        .irdaInvertRx = false,
        .irdaEnableLowPowerReceiver = false,
        .oversample = 10,
        .enableMsbFirst = false,
        .dataWidth = 8UL,
        .parity = CY_SCB_UART_PARITY_NONE,
        .stopBits = CY_SCB_UART_STOP_BITS_1,
        .enableInputFilter = false,
        .breakWidth = 11UL,
        .dropOnFrameError = false,
        .dropOnParityError = false,
        .breaklevel = false,
        .receiverAddress = 0x0UL,
        .receiverAddressMask = 0x0UL,
        .acceptAddrInFifo = false,
        .enableCts = false,
        .ctsPolarity = CY_SCB_UART_ACTIVE_LOW,
        .rtsRxFifoLevel = 0UL,
        .rtsPolarity = CY_SCB_UART_ACTIVE_LOW,
        .rxFifoTriggerLevel = 0UL,
        .rxFifoIntEnableMask = 0UL,
        .txFifoTriggerLevel = 0UL,
        .txFifoIntEnableMask = 0UL,
    };

    if (header_uart_initialized)
    {
        return true;
    }

    Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_SCB9_PERI_NR,
                                 CY_MMIO_SCB9_GROUP_NR,
                                 CY_MMIO_SCB9_SLAVE_NR,
                                 CY_MMIO_SCB9_CLK_HF_NR);
    (void)Cy_SysClk_PeriPclkAssignDivider(PCLK_SCB9_CLOCK_SCB_EN,
                                          CY_SYSCLK_DIV_16_BIT,
                                          1U);

    Cy_GPIO_Pin_FastInit(HEADER_UART_RX_PORT,
                         HEADER_UART_RX_PIN,
                         CY_GPIO_DM_HIGHZ,
                         0U,
                         HEADER_UART_RX_HSIOM);
    Cy_GPIO_Pin_FastInit(HEADER_UART_TX_PORT,
                         HEADER_UART_TX_PIN,
                         CY_GPIO_DM_STRONG_IN_OFF,
                         1U,
                         HEADER_UART_TX_HSIOM);

    if (Cy_SCB_UART_Init(HEADER_UART_HW,
                         &header_uart_config,
                         &header_uart_context) != CY_SCB_UART_SUCCESS)
    {
        return false;
    }

    Cy_SCB_UART_Enable(HEADER_UART_HW);
    header_uart_initialized = true;
    return true;
}

static bool uart_transfer_packet(const uint8_t *tx,
                                 uint8_t *rx,
                                 uint32_t rx_size,
                                 uint32_t *received_out)
{
    uint32_t received = 0U;
    uint32_t start_tick;

    memset(rx, 0, rx_size);
    Cy_SCB_UART_ClearRxFifo(HEADER_UART_HW);
    Cy_SCB_UART_PutArrayBlocking(HEADER_UART_HW, (void *)tx, ESP32_SIM_REQ_SIZE);

    start_tick = lv_tick_get();
    while ((received < rx_size) &&
           ((uint32_t)(lv_tick_get() - start_tick) < UART_TEST_TIMEOUT_MS))
    {
        while ((received < rx_size) && (Cy_SCB_UART_GetNumInRxFifo(HEADER_UART_HW) > 0U))
        {
            rx[received] = (uint8_t)Cy_SCB_UART_Get(HEADER_UART_HW);
            received++;
        }

        if (received < rx_size)
        {
            Cy_SysLib_Delay(1U);
        }
    }

    if (received_out != NULL)
    {
        *received_out = received;
    }

    return (received == rx_size);
}

static bool initialize_header_spi(void)
{
    if (header_spi_initialized)
    {
        return true;
    }

    Cy_GPIO_Pin_FastInit(HEADER_SPI_CLK_PORT,
                         HEADER_SPI_CLK_PIN,
                         CY_GPIO_DM_STRONG_IN_OFF,
                         0U,
                         P9_3_GPIO);
    Cy_GPIO_Pin_FastInit(HEADER_SPI_MOSI_PORT,
                         HEADER_SPI_MOSI_PIN,
                         CY_GPIO_DM_STRONG_IN_OFF,
                         0U,
                         P9_2_GPIO);
    Cy_GPIO_Pin_FastInit(HEADER_SPI_MISO_PORT,
                         HEADER_SPI_MISO_PIN,
                         CY_GPIO_DM_HIGHZ,
                         0U,
                         P9_1_GPIO);
    Cy_GPIO_Pin_FastInit(HEADER_SPI_SS_PORT,
                         HEADER_SPI_SS_PIN,
                         CY_GPIO_DM_STRONG_IN_OFF,
                         1U,
                         P9_0_GPIO);

    header_spi_initialized = true;
    return true;
}

static bool spi_transfer_frame(const uint8_t *tx,
                               uint8_t *rx,
                               uint32_t size,
                               uint32_t timeout_ms)
{
    (void)timeout_ms;
    memset(rx, 0, size);

    Cy_GPIO_Write(HEADER_SPI_CLK_PORT, HEADER_SPI_CLK_PIN, 0U);
    Cy_GPIO_Write(HEADER_SPI_SS_PORT, HEADER_SPI_SS_PIN, 0U);
    Cy_SysLib_DelayUs(20U);

    for (uint32_t byte_index = 0U; byte_index < size; byte_index++)
    {
        uint8_t rx_byte = 0U;

        for (int8_t bit = 7; bit >= 0; bit--)
        {
            uint32_t tx_bit = ((tx[byte_index] >> (uint8_t)bit) & 0x01U);

            Cy_GPIO_Write(HEADER_SPI_MOSI_PORT, HEADER_SPI_MOSI_PIN, tx_bit);
            Cy_SysLib_DelayUs(5U);
            Cy_GPIO_Write(HEADER_SPI_CLK_PORT, HEADER_SPI_CLK_PIN, 1U);
            Cy_SysLib_DelayUs(5U);

            if (Cy_GPIO_Read(HEADER_SPI_MISO_PORT, HEADER_SPI_MISO_PIN) != 0U)
            {
                rx_byte |= (uint8_t)(1U << (uint8_t)bit);
            }

            Cy_GPIO_Write(HEADER_SPI_CLK_PORT, HEADER_SPI_CLK_PIN, 0U);
            Cy_SysLib_DelayUs(5U);
        }

        rx[byte_index] = rx_byte;
    }

    Cy_SysLib_DelayUs(20U);
    Cy_GPIO_Write(HEADER_SPI_SS_PORT, HEADER_SPI_SS_PIN, 1U);
    Cy_GPIO_Write(HEADER_SPI_MOSI_PORT, HEADER_SPI_MOSI_PIN, 0U);
    return true;
}

/*******************************************************************************
* Function Name: configure_header_gpio_inputs
********************************************************************************/
static void configure_header_gpio_inputs(void)
{
    for (uint32_t i = 0U; i < HEADER_GPIO_COUNT; i++)
    {
        Cy_GPIO_Pin_FastInit(header_gpio_ports[i],
                             header_gpio_pins[i],
                             CY_GPIO_DM_HIGHZ,
                             0U,
                             header_gpio_hsiom[i]);
    }
}

/*******************************************************************************
* Function Name: configure_header_gpio_outputs
********************************************************************************/
static void configure_header_gpio_outputs(void)
{
    for (uint32_t i = 0U; i < HEADER_GPIO_COUNT; i++)
    {
        Cy_GPIO_Pin_FastInit(header_gpio_ports[i],
                             header_gpio_pins[i],
                             CY_GPIO_DM_STRONG_IN_OFF,
                             0U,
                             header_gpio_hsiom[i]);
    }
}

/*******************************************************************************
* Function Name: read_header_gpio_mask
********************************************************************************/
static uint8_t read_header_gpio_mask(void)
{
    uint8_t mask = 0U;

    for (uint32_t i = 0U; i < HEADER_GPIO_COUNT; i++)
    {
        if (Cy_GPIO_Read(header_gpio_ports[i], header_gpio_pins[i]) != 0U)
        {
            mask |= (uint8_t)(1U << i);
        }
    }

    return mask;
}

/*******************************************************************************
* Function Name: log_port13_registers
********************************************************************************/
static void log_port13_registers(const char *tag)
{
    console_append("P13 %s OUT=0x%02lX IN=0x%02lX CFG=0x%08lX CFG_OUT3=0x%08lX HSIOM0=0x%08lX HSIOM1=0x%08lX HSIOM_NS=0x%02lX\r\n",
                   tag,
                   (unsigned long)(GPIO_PRT_OUT(GPIO_PRT13) & 0xFFUL),
                   (unsigned long)(GPIO_PRT_IN(GPIO_PRT13) & 0xFFUL),
                   (unsigned long)GPIO_PRT_CFG(GPIO_PRT13),
                   (unsigned long)GPIO_PRT_CFG_OUT3(GPIO_PRT13),
                   (unsigned long)HSIOM_PRT_PORT_SEL0(HSIOM_PRT13),
                   (unsigned long)HSIOM_PRT_PORT_SEL1(HSIOM_PRT13),
                   (unsigned long)(HSIOM_SECURE_PRT13->NONSECURE_MASK & 0xFFUL));
}

/*******************************************************************************
* Function Name: read_header_gpio_output_mask
********************************************************************************/
static uint8_t read_header_gpio_output_mask(void)
{
    uint8_t mask = 0U;

    for (uint32_t i = 0U; i < HEADER_GPIO_COUNT; i++)
    {
        if (Cy_GPIO_ReadOut(header_gpio_ports[i], header_gpio_pins[i]) != 0U)
        {
            mask |= (uint8_t)(1U << i);
        }
    }

    return mask;
}

/*******************************************************************************
* Function Name: configure_adc_level_inputs
********************************************************************************/
static void configure_adc_level_inputs(void)
{
    Cy_GPIO_Pin_FastInit(HEADER_ADC0_PORT,
                         HEADER_ADC0_PIN,
                         CY_GPIO_DM_HIGHZ,
                         0U,
                         P15_2_GPIO);
    Cy_GPIO_Pin_FastInit(HEADER_ADC1_PORT,
                         HEADER_ADC1_PIN,
                         CY_GPIO_DM_HIGHZ,
                         0U,
                         P15_3_GPIO);
}

/*******************************************************************************
* Function Name: read_adc_level_mask
********************************************************************************/
static uint8_t read_adc_level_mask(void)
{
    uint8_t mask = 0U;

    if (Cy_GPIO_Read(HEADER_ADC0_PORT, HEADER_ADC0_PIN) != 0U)
    {
        mask |= 0x01U;
    }

    if (Cy_GPIO_Read(HEADER_ADC1_PORT, HEADER_ADC1_PIN) != 0U)
    {
        mask |= 0x02U;
    }

    return mask;
}

/*******************************************************************************
* Function Name: drive_header_gpio_mask
********************************************************************************/
static void drive_header_gpio_mask(uint8_t mask)
{
    for (uint32_t i = 0U; i < HEADER_GPIO_COUNT; i++)
    {
        Cy_GPIO_Write(header_gpio_ports[i],
                               header_gpio_pins[i],
                               ((mask & (uint8_t)(1U << i)) != 0U) ? 1U : 0U);
    }
}

static uint8_t xor_checksum(const uint8_t *data, uint32_t size)
{
    uint8_t checksum = 0U;

    for (uint32_t i = 0U; i < size; i++)
    {
        checksum ^= data[i];
    }

    return checksum;
}

/*******************************************************************************
* Function Name: console_clear
********************************************************************************/
static void console_clear(void)
{
    console_buffer[0] = '\0';
    console_refresh();
}

/*******************************************************************************
* Function Name: console_append
********************************************************************************/
static void console_append(const char *format, ...)
{
    char text[192];
    va_list args;

    va_start(args, format);
    (void)vsnprintf(text, sizeof(text), format, args);
    va_end(args);

    console_append_text(text);
}

/*******************************************************************************
* Function Name: console_append_text
********************************************************************************/
static void console_append_text(const char *text)
{
    size_t current_length = strlen(console_buffer);
    size_t text_length = strlen(text);

    (void)printf("%s", text);
    (void)fflush(stdout);

    if (text_length >= CONSOLE_BUFFER_SIZE)
    {
        text += text_length - (CONSOLE_BUFFER_SIZE - 1U);
        text_length = CONSOLE_BUFFER_SIZE - 1U;
    }

    if ((current_length + text_length) >= CONSOLE_BUFFER_SIZE)
    {
        size_t overflow = (current_length + text_length) - (CONSOLE_BUFFER_SIZE - 1U);
        size_t remaining = current_length - overflow;

        memmove(console_buffer, &console_buffer[overflow], remaining);
        console_buffer[remaining] = '\0';
        current_length = remaining;
    }

    memcpy(&console_buffer[current_length], text, text_length);
    console_buffer[current_length + text_length] = '\0';
    console_refresh();
}

/*******************************************************************************
* Function Name: console_refresh
********************************************************************************/
static void console_refresh(void)
{
    if (console_view != NULL)
    {
        lv_textarea_set_text(console_view, console_buffer);
        lv_textarea_set_cursor_pos(console_view, LV_TEXTAREA_CURSOR_LAST);
    }
}

/*******************************************************************************
* Function Name: set_status_text
********************************************************************************/
static void set_status_text(const char *text)
{
    if (status_label != NULL)
    {
        lv_label_set_text(status_label, text);
    }
}

/*******************************************************************************
* Function Name: i2c_status_to_text
********************************************************************************/
static const char *i2c_status_to_text(cy_en_scb_i2c_status_t status)
{
    switch (status)
    {
        case CY_SCB_I2C_SUCCESS:
            return "ACK / SUCCESS";
        case CY_SCB_I2C_BAD_PARAM:
            return "BAD PARAMETER";
        case CY_SCB_I2C_MASTER_NOT_READY:
            return "BUS NOT READY";
        case CY_SCB_I2C_MASTER_MANUAL_TIMEOUT:
            return "TIMEOUT";
        case CY_SCB_I2C_MASTER_MANUAL_ADDR_NAK:
            return "ADDRESS NACK";
        case CY_SCB_I2C_MASTER_MANUAL_NAK:
            return "DATA NACK";
        case CY_SCB_I2C_MASTER_MANUAL_ARB_LOST:
            return "ARBITRATION LOST";
        case CY_SCB_I2C_MASTER_MANUAL_BUS_ERR:
            return "BUS ERROR";
        default:
            return "UNKNOWN ERROR";
    }
}

/* [] END OF FILE */
