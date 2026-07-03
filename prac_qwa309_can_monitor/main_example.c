/*******************************************************************************
 * @file    main_example.c
 * @brief   Practise entry wrapper — QWA309: CANFD0 Classic-CAN monitor (CM55).
 *
 * Ported from qwa309-training-base canbus_test (CM33 baremetal) to the CM55
 * LVGL episode model, fully polled (no NVIC). TX heartbeat proves CANFD0 works
 * from CM55; the RX table fills once a peer node / USB-CAN analyzer is attached
 * to the SN65HVD230 (CANH/CANL, 120R terminator via P9 jumper).
 *
 * BUILD (tools_3.8):  make build BOARD=TESAIOT_DEV_KIT
 ******************************************************************************/
#include "app_interface.h"
#include "can_monitor_ui.h"

void example_main(lv_obj_t *parent)
{
    (void)parent;
    can_monitor_ui_create();
}
