/*******************************************************************************
 * @file    board_profile.h
 * @brief   TESAIoT Dev Kit Master — compile-time board profile.
 *
 *  Both supported carrier boards share the SAME chip (PSE846GPS2DBZC4A),
 *  the SAME BSP (APP_KIT_PSE84_AI), the SAME 4.3" display and the SAME
 *  3-core boot stack. This header captures the ONLY things that differ:
 *  peripheral wiring and which optional hardware is populated.
 *
 *  Select the profile from the build with:
 *      make build BOARD=PSOC_EDGE_AI_KIT     # legacy (default)
 *      make build BOARD=TESAIOT_DEV_KIT      # QWA309 training board
 *
 *  common.mk emits -DBOARD_$(BOARD); this header keys off that macro.
 *
 *  INSTALL: copy to proj_cm55/platform/board_profile.h
 ******************************************************************************/
#ifndef TESAIOT_BOARD_PROFILE_H
#define TESAIOT_BOARD_PROFILE_H

/* Default to the legacy AI Kit so existing builds are unchanged. */
#if !defined(BOARD_PSOC_EDGE_AI_KIT) && !defined(BOARD_TESAIOT_DEV_KIT)
#define BOARD_PSOC_EDGE_AI_KIT
#endif

#if defined(BOARD_TESAIOT_DEV_KIT)
/* ---------------------------------------------------------------------------
 * TESAIoT Dev Kit (QWA309) = PSoC Edge AI Kit SoM  +  custom base board.
 * The AI Kit SoM is plugged in, so it carries the SAME onboard sensors and the
 * SAME sensor bus wiring as the standalone AI Kit: I2C sensors on SCB0/P8/1.8V,
 * BMM350 on I3C, PDM mic, WiFi — plus a radar. The base board ADDS pots, buttons,
 * CAN, RGB matrix and a CapSense companion. This board is a SUPERSET of the AI Kit.
 * ------------------------------------------------------------------------- */
#define BOARD_NAME_STR                 "TESAIoT Dev Kit (QWA309)"
#define BOARD_SENSOR_I2C_SHARED_DISP   (0)  /* SoM sensor bus = SCB0 / P8 / 1.8 V  */
#define BOARD_HAS_ONBOARD_SENSORS      (1)  /* DPS368, BMI270, SHT4x (via SoM)     */
#define BOARD_HAS_I3C_BMM350           (1)  /* BMM350 on I3C (via SoM)             */
#define BOARD_HAS_PDM_MIC              (1)  /* digital mic (via SoM)               */
#define BOARD_HAS_RADAR                (1)  /* radar (QWA309 addition)             */
#define BOARD_HAS_CAN                  (1)  /* CANFD0 ch1, P16.2/P16.3, SN65HVD230 */
#define BOARD_HAS_POTS                 (1)  /* 4x SAR pots, P15.4-P15.7, Vref 1.8V */
#define BOARD_HAS_EXT_BUTTONS          (1)  /* SW9 P17.5, SW10 P17.7               */
#define BOARD_HAS_RGB_MATRIX           (1)  /* DFR0522, I2C 0x10                   */
#define BOARD_HAS_CAPSENSE_COMPANION   (1)  /* PSoC 4000T, I2C 0x08                */

#else  /* BOARD_PSOC_EDGE_AI_KIT (default / legacy) */
/* ---------------------------------------------------------------------------
 * PSoC Edge AI Kit — SoM only, no custom base board.
 * Dedicated 1.8 V sensor bus on SCB0 (P8.0/P8.1) plus I3C for BMM350.
 * ------------------------------------------------------------------------- */
#define BOARD_NAME_STR                 "PSoC Edge AI Kit"
#define BOARD_SENSOR_I2C_SHARED_DISP   (0)  /* dedicated SCB0 / P8 / 1.8 V bus     */
#define BOARD_HAS_ONBOARD_SENSORS      (1)  /* DPS368, BMI270, SHT4x               */
#define BOARD_HAS_I3C_BMM350           (1)  /* BMM350 on I3C P3.0/P3.1             */
#define BOARD_HAS_PDM_MIC              (1)
#define BOARD_HAS_RADAR                (0)  /* no radar on the bare AI Kit         */
#define BOARD_HAS_CAN                  (0)
#define BOARD_HAS_POTS                 (0)
#define BOARD_HAS_EXT_BUTTONS          (0)
#define BOARD_HAS_RGB_MATRIX           (0)
#define BOARD_HAS_CAPSENSE_COMPANION   (0)

#endif

#endif /* TESAIOT_BOARD_PROFILE_H */
