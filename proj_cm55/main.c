/*******************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for
*                    application running on CM55 CPU.
*
* Related Document : See README.md
*
********************************************************************************
 * (c) 2025-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "retarget_io_init.h"
#include "vg_lite.h"
#include "vg_lite_platform.h"

#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"
#include "cyabs_rtos_impl.h"
#include "cy_time.h"
#include "mtb_hal.h"

#include "lvgl.h"

#if defined(MTB_DISPLAY_WS7P0DSI_RPI)
#include "mtb_disp_ws7p0dsi_drv.h"
#elif defined(MTB_DISPLAY_EK79007AD3)
#include "mtb_display_ek79007ad3.h"
#elif defined(MTB_DISPLAY_W4P3INCH_RPI)
#include "mtb_disp_dsi_waveshare_4p3.h"
#endif

#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "display_i2c_config.h"

/* TESAIoT Dev Kit Master — episode entry contract.
 * The active episode dropped into proj_cm55/apps/ provides a strong definition
 * of example_main(). When apps/ is empty, the weak stub in
 * apps/_default/example_main_default.c is used instead.
 */
#include "app_interface.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define GPU_INT_PRIORITY                    (3U)
#define DC_INT_PRIORITY                     (3U)

#define GFX_TASK_NAME                       ("CM55 Gfx Task")
/* stack size in words */
#define GFX_TASK_STACK_SIZE                 (configMINIMAL_STACK_SIZE * 16)

#define GFX_TASK_PRIORITY                   (configMAX_PRIORITIES - 1)

#define APP_BUFFER_COUNT                    (2U)
/* 64 KB */
#define DEFAULT_GPU_CMD_BUFFER_SIZE         ((64U) * (1024U))


#define GPU_TESSELLATION_BUFFER_SIZE        ((MY_DISP_VER_RES) * 128U)

#define VGLITE_HEAP_SIZE                    (((DEFAULT_GPU_CMD_BUFFER_SIZE) * \
                                              (APP_BUFFER_COUNT)) + \
                                             ((GPU_TESSELLATION_BUFFER_SIZE) * \
                                              (APP_BUFFER_COUNT)))

#define GPU_MEM_BASE                        (0x0U)
#define I2C_CONTROLLER_IRQ_PRIORITY         (2UL)
#define I3C_CONTROLLER_IRQ_PRIORITY         (2UL)
#define VG_PARAMS_POS                       (0UL)
#define LVGL_LOOP_MAX_DELAY_MS              (5U)

/* Enabling or disabling a MCWDT requires a wait time of upto 2 CLK_LF cycles
 * to come into effect. This wait time value will depend on the actual CLK_LF
 * frequency set by the BSP.
 */
#define LPTIMER_1_WAIT_TIME_USEC            (62U)

/* Define the LPTimer interrupt priority number. '1' implies highest priority.*/
#define APP_LPTIMER_INTERRUPT_PRIORITY      (1U)

#if ( configGENERATE_RUN_TIME_STATS == 1 )
#define TCPWM_TIMER_INT_PRIORITY            (1U)
#endif


/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Heap memory for VGLite to allocate memory for buffers, command, and
 * tessellation buffers
 */
CY_SECTION(".cy_gpu_buf") uint8_t contiguous_mem[VGLITE_HEAP_SIZE] = {0xFF};

volatile void *vglite_heap_base = &contiguous_mem;

TaskHandle_t rtos_cm55_gfx_task_handle = NULL;

/* DC IRQ Config */
cy_stc_sysint_t dc_irq_cfg =
{
    .intrSrc      = GFXSS_DC_IRQ,
    .intrPriority = DC_INT_PRIORITY
};

/* GPU IRQ Config */
cy_stc_sysint_t gpu_irq_cfg =
{
    .intrSrc      = GFXSS_GPU_IRQ,
    .intrPriority = GPU_INT_PRIORITY
};

cy_stc_scb_i2c_context_t disp_touch_i2c_controller_context;
mtb_hal_i2c_t disp_touch_i2c_controller_hal_obj;

cy_stc_sysint_t disp_touch_i2c_controller_irq_cfg =
{
    .intrSrc      = DISPLAY_I2C_CONTROLLER_IRQ,
    .intrPriority = I2C_CONTROLLER_IRQ_PRIORITY,
};

/* Dedicated sensor I2C bus on 1.8V domain (SCB0, P8.0/P8.1).
 * These are exposed (non-static) so episodes dropped into apps/ can read
 * sensors through the already-initialized bus — see platform/sensor_bus.h. */
cy_stc_scb_i2c_context_t sensor_i2c_controller_context;
mtb_hal_i2c_t sensor_i2c_controller_hal_obj;

static const cy_stc_scb_i2c_config_t sensor_i2c_controller_config =
{
    .i2cMode = CY_SCB_I2C_MASTER,
    .useRxFifo = true,
    .useTxFifo = true,
    .slaveAddress = 0U,
    .slaveAddressMask = 0U,
    .acceptAddrInFifo = false,
    .ackGeneralAddr = false,
    .enableWakeFromSleep = false,
    .enableDigitalFilter = false,
    .lowPhaseDutyCycle = 16,
    .highPhaseDutyCycle = 16,
};

#if defined (COMPONENT_MTB_HAL) && (MTB_HAL_DRIVER_AVAILABLE_I2C)
static const mtb_hal_i2c_configurator_t sensor_i2c_controller_hal_config =
{
    .base = SCB0,
    .config = &sensor_i2c_controller_config,
    .clock = NULL,
};
#endif
/* Dedicated sensor I3C controller context for BMM350 (single-core CM55). */
cy_stc_i3c_context_t CYBSP_I3C_CONTROLLER_context;

cy_stc_sysint_t i3c_controller_irq_cfg =
{
    .intrSrc      = CYBSP_I3C_CONTROLLER_IRQ,
    .intrPriority = I3C_CONTROLLER_IRQ_PRIORITY,
};

#if defined(MTB_DISPLAY_EK79007AD3)
mtb_display_ek79007ad3_pin_config_t ek79007ad3_pin_cfg =
{
    .reset_port = CYBSP_DISP_RST_PORT,
    .reset_pin  = CYBSP_DISP_RST_PIN,
};
#endif

/* LPTimer HAL object */
static mtb_hal_lptimer_t lptimer_obj;

/* RTC HAL object */
static mtb_hal_rtc_t rtc_obj;

#if (configGENERATE_RUN_TIME_STATS == 1)
/*******************************************************************************
* Function Name: setup_run_time_stats_timer
********************************************************************************
* Summary:
*  This function configuresTCPWM 0 GRP 0 Counter 0 as the timer source for
*  FreeRTOS runtime statistics.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void setup_run_time_stats_timer(void)
{
    /* Initialze TCPWM block with required timer configuration */
    if (CY_TCPWM_SUCCESS != Cy_TCPWM_Counter_Init(CYBSP_GENERAL_PURPOSE_TIMER_HW,
                                                CYBSP_GENERAL_PURPOSE_TIMER_NUM,
                                           &CYBSP_GENERAL_PURPOSE_TIMER_config))
    {
        handle_app_error();
    }

    /* Enable the initialized counter */
    Cy_TCPWM_Counter_Enable(CYBSP_GENERAL_PURPOSE_TIMER_HW,
                            CYBSP_GENERAL_PURPOSE_TIMER_NUM);

    /* Start the counter */
    Cy_TCPWM_TriggerStart_Single(CYBSP_GENERAL_PURPOSE_TIMER_HW,
                                 CYBSP_GENERAL_PURPOSE_TIMER_NUM);
}


/*******************************************************************************
* Function Name: get_run_time_counter_value
********************************************************************************
* Summary:
*  Function to fetch run time counter value. This will be used by FreeRTOS for
*  run time statistics calculation.
*
* Parameters:
*  void
*
* Return:
*  uint32_t: TCPWM 0 GRP 0 Counter 0 value
*
*******************************************************************************/
uint32_t get_run_time_counter_value(void)
{
   return (Cy_TCPWM_Counter_GetCounter(CYBSP_GENERAL_PURPOSE_TIMER_HW,
                                       CYBSP_GENERAL_PURPOSE_TIMER_NUM));
}


/*******************************************************************************
* Function Name: calculate_idle_percentage
********************************************************************************
* Summary:
*  Function to calculate CPU idle percentage. This function is used by LVGL to
*  showcase CPU usage.
*
* Parameters:
*  void
*
* Return:
*  uint32_t: CPU idle percentage
*
*******************************************************************************/
uint32_t calculate_idle_percentage(void)
{
    static uint32_t previousIdleTime = 0;
    static TickType_t previousTick = 0;
    uint32_t time_diff = 0;
    uint32_t idle_percent = 0;

    uint32_t currentIdleTime = ulTaskGetIdleRunTimeCounter();
    TickType_t currentTick = portGET_RUN_TIME_COUNTER_VALUE();

    time_diff = currentTick - previousTick;

    if((currentIdleTime >= previousIdleTime) && (currentTick > previousTick))
    {
        idle_percent = ((currentIdleTime - previousIdleTime) * 100)/time_diff;
    }

    previousIdleTime = ulTaskGetIdleRunTimeCounter();
    previousTick = portGET_RUN_TIME_COUNTER_VALUE();

    return idle_percent;
}
#endif /* #if configGENERATE_RUN_TIME_STATS == 1 */

/*******************************************************************************
* Function Name: lptimer_interrupt_handler
********************************************************************************
* Summary:
*  Interrupt handler function for LPTimer instance.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void lptimer_interrupt_handler(void)
{
    mtb_hal_lptimer_process_interrupt(&lptimer_obj);
}


/*******************************************************************************
* Function Name: setup_tickless_idle_timer
********************************************************************************
* Summary:
*  1. This function first configures and initializes an interrupt for LPTimer.
*  2. Then it initializes the LPTimer HAL object to be used in the RTOS
*     tickless idle mode implementation to allow the device enter deep sleep
*     when idle task runs. LPTIMER_1 instance is configured for CM55 CPU.
*  3. It then passes the LPTimer object to abstraction RTOS library that
*     implements tickless idle mode
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_tickless_idle_timer(void)
{
    /* Interrupt configuration structure for LPTimer */
    cy_stc_sysint_t lptimer_intr_cfg =
    {
        .intrSrc = CYBSP_CM55_LPTIMER_1_IRQ,
        .intrPriority = APP_LPTIMER_INTERRUPT_PRIORITY
    };

    /* Initialize the LPTimer interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status =
                                    Cy_SysInt_Init(&lptimer_intr_cfg,
                                                    lptimer_interrupt_handler);

    /* LPTimer interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(lptimer_intr_cfg.intrSrc);

    /* Initialize the MCWDT block */
    cy_en_mcwdt_status_t mcwdt_init_status =
                                    Cy_MCWDT_Init(CYBSP_CM55_LPTIMER_1_HW,
                                                &CYBSP_CM55_LPTIMER_1_config);

    /* MCWDT initialization failed. Stop program execution. */
    if(CY_MCWDT_SUCCESS != mcwdt_init_status)
    {
        handle_app_error();
    }

    /* Enable MCWDT instance */
    Cy_MCWDT_Enable(CYBSP_CM55_LPTIMER_1_HW,
                    CY_MCWDT_CTR_Msk,
                    LPTIMER_1_WAIT_TIME_USEC);

    /* Setup LPTimer using the HAL object and desired configuration as defined
     * in the device configurator. */
    cy_rslt_t result = mtb_hal_lptimer_setup(&lptimer_obj,
                                            &CYBSP_CM55_LPTIMER_1_hal_config);

    /* LPTimer setup failed. Stop program execution. */
    if(CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Pass the LPTimer object to abstraction RTOS library that implements
     * tickless idle mode
     */
    cyabs_rtos_set_lptimer(&lptimer_obj);
}


/*******************************************************************************
* Function Name: dc_irq_handler
********************************************************************************
* Summary:
*  Display Controller interrupt handler which gets invoked when the DC finishes
*  utilizing the current frame buffer.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void dc_irq_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    Cy_GFXSS_Clear_DC_Interrupt(GFXSS, &gfx_context);

    /* Notify the cm55_gfx_task */
    xTaskNotifyFromISR(rtos_cm55_gfx_task_handle, 1, eSetValueWithOverwrite,
                       &xHigherPriorityTaskWoken);

    /* Perform a context switch if a higher-priority task was woken */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/*******************************************************************************
* Function Name: gpu_irq_handler
********************************************************************************
* Summary:
*  GPU interrupt handler which gets invoked when the GPU finishes composing
*  a frame.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void gpu_irq_handler(void)
{
    Cy_GFXSS_Clear_GPU_Interrupt(GFXSS, &gfx_context);
    vg_lite_IRQHandler();
}


/*******************************************************************************
* Function Name: disp_touch_i2c_controller_interrupt
********************************************************************************
* Summary:
*  I2C controller ISR for display/touch bus.
*******************************************************************************/
static void disp_touch_i2c_controller_interrupt(void)
{
    Cy_SCB_I2C_Interrupt(DISPLAY_I2C_CONTROLLER_HW,
                         &disp_touch_i2c_controller_context);
}

/*******************************************************************************
* Function Name: sensor_i2c_controller_init
********************************************************************************
* Summary:
*  Initialize dedicated 1.8V sensor bus (SCB0 on P8.0/P8.1) and bind HAL object.
*******************************************************************************/
static cy_rslt_t sensor_i2c_controller_init(void)
{
    cy_stc_gpio_pin_config_t scl_cfg =
    {
        .outVal = 1,
        .driveMode = CY_GPIO_DM_OD_DRIVESLOW,
        .hsiom = P8_0_SCB0_I2C_SCL,
        .intEdge = CY_GPIO_INTR_DISABLE,
        .intMask = 0UL,
        .vtrip = CY_GPIO_VTRIP_CMOS,
        .slewRate = CY_GPIO_SLEW_FAST,
        .driveSel = CY_GPIO_DRIVE_1_2,
        .vregEn = 0UL,
        .ibufMode = 0UL,
        .vtripSel = 0UL,
        .vrefSel = 0UL,
        .vohSel = 0UL,
        .pullUpRes = CY_GPIO_PULLUP_RES_DISABLE,
        .nonSec = 1,
    };

    cy_stc_gpio_pin_config_t sda_cfg = scl_cfg;
    sda_cfg.hsiom = P8_1_SCB0_I2C_SDA;

    Cy_GPIO_Pin_Init(GPIO_PRT8, 0U, &scl_cfg);
    Cy_GPIO_Pin_Init(GPIO_PRT8, 1U, &sda_cfg);

    Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_SCB0_PERI_NR,
                                 CY_MMIO_SCB0_GROUP_NR,
                                 CY_MMIO_SCB0_SLAVE_NR,
                                 CY_MMIO_SCB0_CLK_HF_NR);
    Cy_SysClk_PeriPclkAssignDivider(PCLK_SCB0_CLOCK_SCB_EN,
                                    CY_SYSCLK_DIV_16_BIT,
                                    CYBSP_I2C_CONTROLLER_CLK_DIV_NUM);
    (void)Cy_SysClk_PeriPclkEnableDivider((en_clk_dst_t)CYBSP_I2C_CONTROLLER_CLK_DIV_GRP_NUM,
                                          CY_SYSCLK_DIV_16_BIT,
                                          CYBSP_I2C_CONTROLLER_CLK_DIV_NUM);

    cy_en_scb_i2c_status_t i2c_rslt = Cy_SCB_I2C_Init(SCB0,
                                                      &sensor_i2c_controller_config,
                                                      &sensor_i2c_controller_context);
    if (CY_SCB_I2C_SUCCESS != i2c_rslt)
    {
        return (cy_rslt_t)i2c_rslt;
    }

    Cy_SCB_I2C_Enable(SCB0);

#if defined (COMPONENT_MTB_HAL) && (MTB_HAL_DRIVER_AVAILABLE_I2C)
    return mtb_hal_i2c_setup(&sensor_i2c_controller_hal_obj,
                             &sensor_i2c_controller_hal_config,
                             &sensor_i2c_controller_context,
                             NULL);
#else
    return CY_RSLT_SUCCESS;
#endif
}
/*******************************************************************************
* Function Name: i3c_controller_interrupt
********************************************************************************
* Summary:
*  I3C controller ISR.
*******************************************************************************/
static void i3c_controller_interrupt(void)
{
    Cy_I3C_Interrupt(CYBSP_I3C_CONTROLLER_HW, &CYBSP_I3C_CONTROLLER_context);
}

/*******************************************************************************
* Function Name: i3c_controller_init
********************************************************************************
* Summary:
*  Initialize I3C controller and enable NVIC for BMM350 communication.
*******************************************************************************/
static cy_rslt_t i3c_controller_init(void)
{
    cy_en_i3c_status_t i3c_rslt = Cy_I3C_Init(CYBSP_I3C_CONTROLLER_HW,
                                             &CYBSP_I3C_CONTROLLER_config,
                                             &CYBSP_I3C_CONTROLLER_context);
    if (CY_I3C_SUCCESS != i3c_rslt)
    {
        return (cy_rslt_t)i3c_rslt;
    }

    cy_en_sysint_status_t irq_rslt = Cy_SysInt_Init(&i3c_controller_irq_cfg,
                                                   &i3c_controller_interrupt);
    if (CY_SYSINT_SUCCESS != irq_rslt)
    {
        return (cy_rslt_t)irq_rslt;
    }

    NVIC_EnableIRQ((IRQn_Type)i3c_controller_irq_cfg.intrSrc);
    Cy_I3C_Enable(CYBSP_I3C_CONTROLLER_HW, &CYBSP_I3C_CONTROLLER_context);
    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
* Function Name: cm55_gfx_task
********************************************************************************
* Summary:
*   This is the FreeRTOS task callback function.
*   It initializes:
*       - GFX subsystem.
*       - Configure the DC, GPU interrupts.
*       - Initialize I2C interface to be used for touch as well as 7, 4.3-inch 
*         display drivers.
*       - Initializes the display panel selected through Makefile component and
*         vglite driver.
*       - Allocates vglite memory.
*       - Configures LVGL, low level display and touch driver.
*       - Finally invokes the UI application.
*
* Parameters:
*  void *arg: Pointer to the argument passed to the task (not used)
*
* Return:
*  void
*
*******************************************************************************/
static void cm55_gfx_task(void *arg)
{
    CY_UNUSED_PARAMETER(arg);

    uint32_t time_till_next = 0;

    cy_en_sysint_status_t sysint_status = CY_SYSINT_SUCCESS;
    cy_en_gfx_status_t gfx_status = CY_GFX_SUCCESS;
    vg_lite_error_t vglite_status = VG_LITE_SUCCESS;

#if defined(MTB_DISPLAY_WS7P0DSI_RPI)
    cy_rslt_t status = CY_RSLT_SUCCESS;
#elif defined(MTB_DISPLAY_EK79007AD3)
    cy_en_mipidsi_status_t mipi_status = CY_MIPIDSI_SUCCESS;
#endif

    cy_en_scb_i2c_status_t i2c_result = CY_SCB_I2C_SUCCESS;

    /* GFXSS init */
    /* MIPI-DSI Display specific configs */
#if defined(MTB_DISPLAY_WS7P0DSI_RPI)
    GFXSS_config.mipi_dsi_cfg = &mtb_disp_ws7p0dsi_dsi_config;
#elif defined(MTB_DISPLAY_EK79007AD3)
    GFXSS_config.mipi_dsi_cfg = &mtb_display_ek79007ad3_mipidsi_config;
#elif defined(MTB_DISPLAY_W4P3INCH_RPI)
    GFXSS_config.mipi_dsi_cfg = &mtb_disp_waveshare_4p3_dsi_config;
#endif

    GFXSS_config.dc_cfg->gfx_layer_config->width  = MY_DISP_HOR_RES;
    GFXSS_config.dc_cfg->gfx_layer_config->height = MY_DISP_VER_RES;
    GFXSS_config.dc_cfg->display_width            = MY_DISP_HOR_RES;
    GFXSS_config.dc_cfg->display_height           = MY_DISP_VER_RES; 

    /* Set frame buffer address to the GFXSS configuration structure */
    GFXSS_config.dc_cfg->gfx_layer_config->buffer_address    = frame_buffer1;
    GFXSS_config.dc_cfg->gfx_layer_config->uv_buffer_address = frame_buffer1;

    /* Initialize Graphics subsystem as per the configuration */
    gfx_status = Cy_GFXSS_Init(GFXSS, &GFXSS_config, &gfx_context);

    if (CY_GFX_SUCCESS == gfx_status)
    {
        /* Initialize GFXSS DC interrupt */
        sysint_status = Cy_SysInt_Init(&dc_irq_cfg, dc_irq_handler);

        if (CY_SYSINT_SUCCESS != sysint_status)
        {
            printf("Error in registering DC interrupt: %d\r\n", sysint_status);
            handle_app_error();
        }

        /* Enable GFX DC interrupt in NVIC. */
        NVIC_EnableIRQ(GFXSS_DC_IRQ);

        /* Initialize GFX GPU interrupt */
        sysint_status = Cy_SysInt_Init(&gpu_irq_cfg, gpu_irq_handler);

        if (CY_SYSINT_SUCCESS != sysint_status)
        {
            printf("Error in registering GPU interrupt: %d\r\n", sysint_status);
            handle_app_error();
        }

        /* Enable GPU interrupt */
        Cy_GFXSS_Enable_GPU_Interrupt(GFXSS);

        /* Enable GFX GPU interrupt in NVIC. */
        NVIC_EnableIRQ(GFXSS_GPU_IRQ);

        /* Initialize the I2C in controller mode. */
        i2c_result = Cy_SCB_I2C_Init(DISPLAY_I2C_CONTROLLER_HW,
                                     &DISPLAY_I2C_CONTROLLER_config,
                                     &disp_touch_i2c_controller_context);

        if (CY_SCB_I2C_SUCCESS != i2c_result)
        {
            printf("I2C controller initialization failed !!\n");
            handle_app_error();
        }

        /* Initialize the I2C interrupt */
        sysint_status = Cy_SysInt_Init(&disp_touch_i2c_controller_irq_cfg,
                                       &disp_touch_i2c_controller_interrupt);

        if (CY_SYSINT_SUCCESS != sysint_status)
        {
            printf("I2C controller interrupt initialization failed\r\n");
            handle_app_error();
        }

        /* Enable the I2C interrupts. */
        NVIC_EnableIRQ(disp_touch_i2c_controller_irq_cfg.intrSrc);

        /* Enable the I2C */
        Cy_SCB_I2C_Enable(DISPLAY_I2C_CONTROLLER_HW);

        vTaskDelay(pdMS_TO_TICKS(500));

#if defined(MTB_DISPLAY_WS7P0DSI_RPI)
        /* Initialize the RPI display */
        status = mtb_disp_ws7p0dsi_panel_init(DISPLAY_I2C_CONTROLLER_HW,
                                              &disp_touch_i2c_controller_context);

        if (CY_RSLT_SUCCESS != status)
        {
            printf("Waveshare 7-Inch R-Pi display init failed with status = %u\r\n", (unsigned int) status);
            CY_ASSERT(0);
        }

#elif defined(MTB_DISPLAY_EK79007AD3)
        /* Initialize the WF101JTYAHMNB0 display driver. */
        mipi_status = mtb_display_ek79007ad3_init(GFXSS_GFXSS_MIPIDSI,
                                                  &ek79007ad3_pin_cfg);

        if (CY_MIPIDSI_SUCCESS != mipi_status)
        {
            printf("WF101JTYAHMNB0 10-inch display init failed with status = %d\r\n", mipi_status);
            CY_ASSERT(0);
        }

#elif defined(MTB_DISPLAY_W4P3INCH_RPI)

        i2c_result = Cy_SCB_I2C_Init(DISPLAY_I2C_CONTROLLER_HW,
                                     &DISPLAY_I2C_CONTROLLER_config,
                                     &disp_touch_i2c_controller_context);

        if (CY_SCB_I2C_SUCCESS != i2c_result)
        {
            printf("I2C controller initialization failed !!\n");
            CY_ASSERT(0);
        }

        /* Initialize the I2C interrupt */
        sysint_status = Cy_SysInt_Init(&disp_touch_i2c_controller_irq_cfg,
                                       &disp_touch_i2c_controller_interrupt);

        if (CY_SYSINT_SUCCESS != sysint_status)
        {
            printf("I2C controller interrupt initialization failed\r\n");
            CY_ASSERT(0);
        }

        /* Enable the I2C interrupts. */
        NVIC_EnableIRQ(disp_touch_i2c_controller_irq_cfg.intrSrc);

        /* Enable the I2C */
        Cy_SCB_I2C_Enable(DISPLAY_I2C_CONTROLLER_HW);

         /* Initialize the Waveshare 4.3-Inch display */
        i2c_result = mtb_disp_waveshare_4p3_init(DISPLAY_I2C_CONTROLLER_HW,
                                             &disp_touch_i2c_controller_context);
        if (CY_SCB_I2C_SUCCESS != i2c_result)
        {
            printf("Waveshare 4.3-Inch display init failed with status = %u\r\n", (unsigned int) i2c_result);
            CY_ASSERT(0);
        }
#endif
        /* Display I2C HAL context (for touch/display middleware path). */
        cy_rslt_t i2c_hal_rslt = mtb_hal_i2c_setup(&disp_touch_i2c_controller_hal_obj,
                                                 &DISPLAY_I2C_CONTROLLER_hal_config,
                                                 &disp_touch_i2c_controller_context,
                                                 NULL);

        if (CY_RSLT_SUCCESS != i2c_hal_rslt)
        {
            printf("Display I2C HAL setup failed, status = 0x%08lx\r\n",
                   (unsigned long)i2c_hal_rslt);
            handle_app_error();
        }

        /* Initialize dedicated sensor buses. Best-effort — episodes that do not
         * use sensors will simply ignore these handles. Master template always
         * initializes ALL subsystems so any episode dropped in apps/ Just Works. */
        cy_rslt_t sensor_i2c_rslt = sensor_i2c_controller_init();
        if (CY_RSLT_SUCCESS != sensor_i2c_rslt)
        {
            printf("[MASTER] Sensor I2C init failed (0x%08lx) — sensor episodes unavailable\r\n",
                   (unsigned long)sensor_i2c_rslt);
        }

        cy_rslt_t i3c_rslt = i3c_controller_init();
        if (CY_RSLT_SUCCESS != i3c_rslt)
        {
            printf("[MASTER] I3C init failed (0x%08lx) — BMM350 compass episodes unavailable\r\n",
                   (unsigned long)i3c_rslt);
        }

        /* Allocate memory for VGLite from the vglite_heap_base */
        vg_module_parameters_t vg_params;
        vg_params.register_mem_base = (uint32_t)GFXSS_GFXSS_GPU_GCNANO;
        vg_params.gpu_mem_base[VG_PARAMS_POS] = GPU_MEM_BASE;
        vg_params.contiguous_mem_base[VG_PARAMS_POS] = vglite_heap_base;
        vg_params.contiguous_mem_size[VG_PARAMS_POS] = VGLITE_HEAP_SIZE;

        /* Initialize VGlite memory. */
        vg_lite_init_mem(&vg_params);

        /* Initialize the memory and data structures needed for VGLite draw/blit
         * functions
         */
        vglite_status = vg_lite_init((MY_DISP_HOR_RES) / 4,
                             (MY_DISP_VER_RES) / 4);

        if (VG_LITE_SUCCESS == vglite_status)
        {
            /* Initialize LVGL library */
            lv_init();
            lv_port_disp_init();
            /* Initialize touch input for interactive UI controls. */
            lv_port_indev_init();

            /* ========================================================= *
             *   EPISODE ENTRY POINT — master template invariant          *
             * ========================================================= *
             *  main.c NEVER changes per episode. Whatever episode's code *
             *  currently lives in proj_cm55/apps/ provides a strong      *
             *  implementation of example_main(parent).                   *
             *                                                            *
             *  When apps/ is empty, the weak stub in                     *
             *  apps/_default/example_main_default.c takes over and       *
             *  shows instructions for downloading an episode.            *
             * ========================================================= */
            lv_obj_t *parent = lv_scr_act();
            example_main(parent);
        }
        else
        {
            printf("vg_lite_init failed, status: %d\r\n", vglite_status);

            /* Deallocate all the resources and free up all the memory */
            vg_lite_close();
            handle_app_error();
        }
    }
    else
    {
        printf("Graphics subsystem init failed, status: %d\r\n", gfx_status);
        handle_app_error();
    }

    for (;;)
    {
        /* LVGL's timer handler function, to be called periodically to handle
         * LVGL tasks.
         */
        time_till_next = lv_timer_handler();
        if(time_till_next > LVGL_LOOP_MAX_DELAY_MS)
        {
            time_till_next = LVGL_LOOP_MAX_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(time_till_next));
    }
}

/*******************************************************************************
* Function Name: setup_clib_support
********************************************************************************
* Summary:
*    1. This function configures and initializes the Real-Time Clock (RTC)).
*    2. It then initializes the RTC HAL object to enable CLIB support library
*       to work with the provided Real-Time Clock (RTC) module.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_clib_support(void)
{
    /* RTC Initialization is done in CM33 non-secure project */

    /* Initialize the ModusToolbox CLIB support library */
    mtb_clib_support_init(&rtc_obj);
}


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  This is the main function for CM55 non-secure application.
*    1. It initializes the device and board peripherals.
*    2. It sets up the LPtimer instance for CM55 CPU and initializes debug UART.
*    3. It creates the FreeRTOS application task 'cm55_gfx_task'.
*    4. It starts the RTOS task scheduler.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result       = CY_RSLT_SUCCESS;
    BaseType_t task_return = pdFAIL;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Setup CLIB support library. */
    setup_clib_support();
    /* Setup the LPTimer instance for CM55 */
    setup_tickless_idle_timer();

    /* Initialize retarget-io middleware */
    init_retarget_io();

    /* Enable global interrupts */
    __enable_irq();

    /* Create the FreeRTOS Task */
    task_return = xTaskCreate(cm55_gfx_task, GFX_TASK_NAME,
                              GFX_TASK_STACK_SIZE, NULL,
                              GFX_TASK_PRIORITY, &rtos_cm55_gfx_task_handle);

    /* ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    if (pdPASS == task_return)
    {
        printf("****************** "
               "TESAIoT Dev Kit Master — Ready for Episode "
               "****************** \r\n\n");

        /* Start the RTOS Scheduler */
        vTaskStartScheduler();

        /* Should never get here! */
        handle_app_error();
    }
    else
    {
        printf("Error: Failed to create cm55_gfx_task.\r\n");
        handle_app_error();
    }
}


/* [] END OF FILE */








































