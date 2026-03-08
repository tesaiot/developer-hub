/*******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for PSE84 Trust M End-to-End Trust Management
*              via TESAIoT Platform using MQTT with Mutual TLS authentication.
*
* Related Document: See README.md
*
*******************************************************************************
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
* TESAIoT Platform Integration
*******************************************************************************
* SPDX-FileCopyrightText: 2024-2025 Assoc. Prof. Wiroon Sriborrirux (TESAIoT Platform Creator)
*
* This application has been modified by the TESAIoT Platform Developer Team to add:
* - Integration with TESAIoT AIoT Foundation Platform
* - Certificate Lifecycle Management (Protected Update workflow)
* - Protected Update workflow support (manifest + fragments, v3.0.0)
* - OPTIGA Trust M hardware security integration
* - Interactive menu for provisioning and testing
* Security by Design: IoT Cybersecurity compliance (ETSI EN 303 645, NIST IR 8259A, IEC 62443, IEEE 802.1AR)
* Original work Copyright 2024-2025 Cypress Semiconductor Corporation (Infineon)
* Modifications Copyright 2024-2025 Assoc. Prof. Wiroon Sriborrirux (TESAIoT Platform Creator)
*
* This application is part of the TESAIoT AIoT Foundation Platform, developed in
* collaboration with Infineon Technologies AG for PSoC Edge E84 + OPTIGA Trust M.
*
* Contact: Wiroon Sriborrirux <sriborrirux@gmail.com>
*******************************************************************************/

/* Header file includes */
#include <stdio.h>
#include "cybsp.h"
#include "optiga_trust_helpers.h"
#include "retarget_io_init.h"
#include "mqtt_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"
#include "cyabs_rtos_impl.h"
#include "cy_time.h"
#include "cycfg_peripherals.h"
#include "cy_scb_uart.h"         /* For Cy_SCB_UART_GetNumInRxFifo() - FreeRTOS-friendly getchar */
#include "cy_tls_optiga_key.h"
#include "optiga_psa_se.h"
#include "cy_wcm.h"
#include "mqtt_client_config.h"
#include "../cryptography/cryptography_examples.h"
#include "subscriber_task.h"
#include "publisher_task.h"
#include "semphr.h"
#include "event_groups.h"



/*----------------------------------------------------------------------------
 * TESAIoT Library v3.0.0 - Umbrella Headers (5 Domain Groups)
 *
 * Instead of including 12+ individual headers, use these 6 umbrella headers:
 *---------------------------------------------------------------------------*/
#include "tesaiot.h"                   // Group 6: License Management
#include "tesaiot_config.h"            // Configuration (debug, OID settings)
#include "tesaiot_optiga_core.h"       // Group 1: Core OPTIGA instance
#include "tesaiot_protected_update.h"  // Group 2: Protected Update Workflow
#include "tesaiot_optiga.h"            // Group 3: TESAIoT OPTIGA Trust M
#include "tesaiot_platform.h"          // Group 4: Standard Libs (MQTT, SNTP)

/******************************************************************************
 * Macros
 ******************************************************************************/
/* The timeout value in microsecond used to wait for core to be booted */
#define CM55_BOOT_WAIT_TIME_US            (10U)
/* Enabling or disabling a MCWDT requires a wait time of upto 2 CLK_LF cycles
 * to come into effect. This wait time value will depend on the actual CLK_LF
 * frequency set by the BSP.
 */
#define LPTIMER_0_WAIT_TIME_USEC           (62U)
/* Define the LPTimer interrupt priority number. '1' implies highest priority.
 */
#define APP_LPTIMER_INTERRUPT_PRIORITY      (1U)



/*******************************************************************************
 * Global Variables
 ******************************************************************************/
/* LPTimer HAL object */
static mtb_hal_lptimer_t lptimer_obj;
typedef mtb_hal_rtc_t rtc_type;
extern EventGroupHandle_t data_received_event_group;
#define BUFFER_SIZE 512
uint8_t uart_buffer[BUFFER_SIZE];
hash_data_from_host_t tesa_hash_pwd;
uint8_t digest[32];   
uint8_t message[] ={"TESA7abcd546FF711"};


/*****************************************************************************
 * Function Definitions
 *****************************************************************************/

const uint8_t trust_anchor1[] = {
 0x30, 0x82, 0x02, 0x58, 0x30, 0x82, 0x01, 0xFF, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x2F,
 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x56, 0x31, 0x0B,
 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x49, 0x4E, 0x31, 0x0D, 0x30, 0x0B, 0x06,
 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x04, 0x49, 0x46, 0x49, 0x4E, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03,
 0x55, 0x04, 0x0B, 0x0C, 0x03, 0x43, 0x43, 0x53, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04,
 0x03, 0x0C, 0x0A, 0x49, 0x6E, 0x74, 0x43, 0x41, 0x20, 0x50, 0x32, 0x35, 0x36, 0x31, 0x15, 0x30,
 0x13, 0x06, 0x03, 0x55, 0x04, 0x2E, 0x13, 0x0C, 0x54, 0x72, 0x75, 0x73, 0x74, 0x20, 0x41, 0x6E,
 0x63, 0x68, 0x6F, 0x72, 0x30, 0x1E, 0x17, 0x0D, 0x31, 0x36, 0x30, 0x35, 0x32, 0x36, 0x30, 0x38,
 0x30, 0x31, 0x33, 0x37, 0x5A, 0x17, 0x0D, 0x31, 0x37, 0x30, 0x36, 0x30, 0x35, 0x30, 0x38, 0x30,
 0x31, 0x33, 0x37, 0x5A, 0x30, 0x5A, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13,
 0x02, 0x49, 0x4E, 0x31, 0x0D, 0x30, 0x0B, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x04, 0x49, 0x46,
 0x49, 0x4E, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x0C, 0x03, 0x43, 0x43, 0x53,
 0x31, 0x17, 0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0E, 0x65, 0x6E, 0x64, 0x45, 0x6E,
 0x74, 0x69, 0x74, 0x79, 0x20, 0x50, 0x32, 0x35, 0x36, 0x31, 0x15, 0x30, 0x13, 0x06, 0x03, 0x55,
 0x04, 0x2E, 0x13, 0x0C, 0x54, 0x72, 0x75, 0x73, 0x74, 0x20, 0x41, 0x6E, 0x63, 0x68, 0x6F, 0x72,
 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A,
 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x19, 0xB5, 0xB2, 0x17, 0x0D,
 0xF5, 0x98, 0x5E, 0xD4, 0xD9, 0x72, 0x16, 0xEF, 0x61, 0x39, 0x3F, 0x14, 0x58, 0xAF, 0x5C, 0x02,
 0x78, 0x07, 0xCA, 0x48, 0x8F, 0x2A, 0xE3, 0x90, 0xB9, 0x03, 0xA1, 0xD2, 0x46, 0x20, 0x09, 0x21,
 0x52, 0x98, 0xDC, 0x8E, 0x88, 0x84, 0x67, 0x8E, 0x83, 0xD1, 0xDE, 0x0F, 0x1C, 0xE5, 0x19, 0x1D,
 0x0C, 0x74, 0x60, 0x41, 0x58, 0x5B, 0x36, 0x55, 0xF8, 0x3D, 0xAB, 0xA3, 0x81, 0xB9, 0x30, 0x81,
 0xB6, 0x30, 0x09, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x02, 0x30, 0x00, 0x30, 0x1D, 0x06, 0x03,
 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0xB5, 0x97, 0xFD, 0xAB, 0x36, 0x1A, 0xA0, 0xA2, 0x23,
 0xA7, 0x68, 0x25, 0x25, 0xFB, 0x82, 0x55, 0xD0, 0x4F, 0xCF, 0xB8, 0x30, 0x7A, 0x06, 0x03, 0x55,
 0x1D, 0x23, 0x04, 0x73, 0x30, 0x71, 0x80, 0x14, 0x1A, 0xBB, 0x56, 0x44, 0x65, 0x8C, 0x4D, 0x4F,
 0xCD, 0x29, 0xA2, 0x3F, 0x4C, 0xC6, 0xBC, 0xA8, 0x8B, 0xA4, 0x0A, 0xDA, 0xA1, 0x56, 0xA4, 0x54,
 0x30, 0x52, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x49, 0x4E, 0x31,
 0x0D, 0x30, 0x0B, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x04, 0x49, 0x46, 0x49, 0x4E, 0x31, 0x0C,
 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x0C, 0x03, 0x43, 0x43, 0x53, 0x31, 0x0F, 0x30, 0x0D,
 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x06, 0x52, 0x6F, 0x6F, 0x74, 0x43, 0x41, 0x31, 0x15, 0x30,
 0x13, 0x06, 0x03, 0x55, 0x04, 0x2E, 0x13, 0x0C, 0x54, 0x72, 0x75, 0x73, 0x74, 0x20, 0x41, 0x6E,
 0x63, 0x68, 0x6F, 0x72, 0x82, 0x01, 0x2E, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x1D, 0x0F, 0x01, 0x01,
 0xFF, 0x04, 0x04, 0x03, 0x02, 0x00, 0x81, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
 0x04, 0x03, 0x02, 0x03, 0x47, 0x00, 0x30, 0x44, 0x02, 0x20, 0x68, 0xFD, 0x9C, 0x8F, 0x35, 0x33,
 0x0B, 0xB8, 0x32, 0x8C, 0xAF, 0x1C, 0x81, 0x4E, 0x41, 0x29, 0x26, 0xCB, 0xB7, 0x10, 0xA0, 0x75,
 0xFC, 0x89, 0xAE, 0xC5, 0x1D, 0x92, 0x8E, 0x72, 0xEF, 0x5C, 0x02, 0x20, 0x7D, 0xC1, 0xEB, 0x58,
 0x21, 0xF1, 0xFD, 0xFB, 0x5E, 0xD7, 0xDE, 0x06, 0xC9, 0xB4, 0xFF, 0x59, 0x8D, 0x37, 0x8C, 0x7A,
 0x48, 0xCD, 0x2D, 0x99, 0x74, 0x77, 0x58, 0x9D, 0x95, 0x51, 0x8F, 0x5D
   };


uint8_t aes_manifest_data[234] = 
	{
	0x84, 0x43, 0xA1, 0x01, 0x26, 0xA1, 0x04, 0x42, 0xE0, 0xE3, 0x58, 0x9C, 0x86, 0x01, 0xF6, 0xF6, 
	0x84, 0x22, 0x13, 0x03, 0x82, 0x18, 0x81, 0x02, 0x82, 0x82, 0x20, 0x58, 0x25, 0x82, 0x18, 0x29, 
	0x58, 0x20, 0x95, 0x00, 0xC0, 0x9F, 0xEB, 0xD1, 0x63, 0x3F, 0x03, 0xFF, 0x7B, 0xB3, 0x62, 0x99, 
	0x58, 0xD8, 0x64, 0x8C, 0x50, 0xD4, 0x85, 0x59, 0x0E, 0x5B, 0x62, 0x49, 0xB1, 0x81, 0x5B, 0x38, 
	0x48, 0x2B, 0x82, 0x01, 0x83, 0x43, 0xA1, 0x01, 0x0A, 0x81, 0x82, 0x58, 0x54, 0xA3, 0x04, 0x42, 
	0xF1, 0xD1, 0x01, 0x3A, 0x00, 0x01, 0x00, 0xB7, 0x05, 0x82, 0x44, 0x74, 0x65, 0x73, 0x74, 0x58, 
	0x40, 0x34, 0x62, 0xCC, 0x89, 0x5C, 0xE9, 0xBC, 0x1C, 0xB3, 0x74, 0x48, 0xE8, 0xF6, 0x09, 0x73, 
	0xB6, 0x92, 0xCA, 0xD1, 0xA3, 0x05, 0x4D, 0x8A, 0xD5, 0x42, 0xD5, 0xCE, 0xAF, 0x63, 0x4B, 0x97, 
	0x58, 0x81, 0x52, 0x90, 0x6F, 0x27, 0x37, 0xE6, 0xBE, 0xC6, 0x00, 0xE5, 0x3F, 0x4A, 0x0C, 0x1E, 
	0xCD, 0x3B, 0x9C, 0x11, 0x93, 0x30, 0x1F, 0x61, 0xAA, 0xBB, 0x36, 0xA7, 0x44, 0x85, 0x32, 0xBC, 
	0xFA, 0xF6, 0xF6, 0x82, 0x40, 0x42, 0xE2, 0x00, 0x58, 0x40, 0xF0, 0x42, 0xAD, 0xB8, 0x3A, 0x69, 
	0xCE, 0x53, 0xC0, 0xB3, 0xA9, 0xCF, 0xF6, 0x62, 0x41, 0xBC, 0x67, 0x70, 0x3E, 0x51, 0xCF, 0x31, 
	0xDA, 0xC8, 0x17, 0x55, 0xE5, 0xA3, 0x61, 0x5D, 0x97, 0x59, 0x7E, 0xC5, 0x23, 0xAE, 0xF7, 0xFD, 
	0x49, 0xE2, 0xE8, 0x8D, 0xA9, 0xBD, 0xD1, 0xAD, 0xBD, 0x3D, 0x63, 0xEC, 0x96, 0xE7, 0x2C, 0x72, 
	0xFA, 0xE2, 0x2B, 0xB0, 0x83, 0xA9, 0x6A, 0x25, 0xE6, 0x5F,
	};

	uint8_t aes_key_final_fragment_array[27] = 
	{
	0xB1, 0x37, 0xD4, 0x08, 0xF4, 0x0F, 0x3D, 0x16, 0x46, 0x6C, 0x2D, 0x2C, 0x68, 0x3D, 0x0E, 0xFA, 
	0x3D, 0xF5, 0xEE, 0x2B, 0x59, 0xCA, 0x5B, 0x90, 0x27, 0x4E, 0x35, 

	};

#include "optiga_util.h"
#include "optiga_crypt.h"
#include "optiga_lib_common.h"
#include <stdio.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


const char* optiga_status_to_str(uint16_t status)
{
    switch (status)
    {
        case 0x0000: return "OPTIGA_LIB_SUCCESS";
        case 0x8001: return "Encryption/Decryption failed";
        case 0x8002: return "Invalid input parameters";
        case 0x8003: return "Memory allocation error";
        case 0x8004: return "Busy / Command already running";
        case 0x8005: return "Invalid command or option";
        case 0x8006: return "Communication error (I2C/CRC)";
        case 0x8007: return "Access condition not satisfied";
        case 0x8008: return "Protection error / Locked object";
        case 0x8009: return "Data object not found";
        case 0x800A: return "Invalid metadata / parsing error";
        case 0x800B: return "Session not established / secure channel required";
        case 0x800C: return "Command execution timeout";
        default:     return "Unknown OPTIGA status";
    }
}
void decode_optiga_metadata(const uint8_t *meta, uint16_t len)
{
    bool found_write = false;
    bool found_exec  = false;
    bool locked_write = false;
    bool locked_exec  = false;
    bool found_keytype = false;
    bool found_usage = false;

    printf("---- OPTIGA Metadata Decode ----\n");
    printf("Raw (%u bytes): ", len);
    for (int i = 0; i < len; i++)
        printf("%02X", meta[i]);
    printf("\n");

    for (int i = 0; i + 2 < len; i++)
    {
        // D0 = Write condition
        if (meta[i] == 0xD0 && meta[i+1] == 0x01)
        {
            found_write = true;
            if (meta[i+2] == 0x00) printf("Write  : Allowed (ALW)\n");
            else if (meta[i+2] == 0x01) { printf("Write  : Locked (NEV)\n"); locked_write = true; }
            else printf("Write  : Restricted (0x%02X)\n", meta[i+2]);
        }

        // D3 = Execute condition
        if (meta[i] == 0xD3 && meta[i+1] == 0x01)
        {
            found_exec = true;
            if (meta[i+2] == 0x00) printf("Execute: Allowed (ALW)\n");
            else if (meta[i+2] == 0x01) { printf("Execute: Denied (NEV)\n"); locked_exec = true; }
            else printf("Execute: Restricted (0x%02X)\n", meta[i+2]);
        }

        // E0 = Key Type
        if (meta[i] == 0xE0 && meta[i+1] == 0x01)
        {
            found_keytype = true;
            uint8_t key_type = meta[i+2];
            const char *type_str = "Unknown";
            switch (key_type)
            {
                case 0x01: type_str = "RSA"; break;
                case 0x02: type_str = "ECC"; break;
                case 0x03: type_str = "AES"; break;
                case 0x04: type_str = "HMAC"; break;
                default: break;
            }
            printf("KeyType: 0x%02X (%s)\n", key_type, type_str);
        }

        // E1 = Key Usage
        if (meta[i] == 0xE1 && meta[i+1] == 0x01)
        {
            found_usage = true;
            uint8_t usage = meta[i+2];
            printf("Usage  : 0x%02X ", usage);
            switch (usage)
            {
                case 0x00: printf("(None)\n"); break;
                case 0x01: printf("(Encrypt/Decrypt)\n"); break;
                case 0x02: printf("(Sign/Verify)\n"); break;
                case 0x03: printf("(Key Agreement)\n"); break;
                default: printf("(Custom/Reserved)\n"); break;
            }
        }
    }

    if (!found_write && !found_exec)
    {
        printf("⚠️  No D0/D3 fields found — factory or user minimal metadata\n");
    }

    // Quick overall summary
    printf("--------------------------------\n");
    printf("Summary: %s / %s",
        locked_write ? "Write=Locked" : "Write=Allowed",
        locked_exec  ? " / Exec=Locked" : " / Exec=Allowed");
    if (found_keytype)
        printf(" / KeyType=Found");
    if (found_usage)
        printf(" / Usage=Found");
    printf("\n--------------------------------\n");
}



    uint8_t plain[16];
    uint8_t cipher[16];
    uint32_t cipher_len = sizeof(cipher);
    uint8_t decrypted[16];
    uint32_t decrypted_len = sizeof(decrypted);

static void optiga_callback(void *context, optiga_lib_status_t return_status)
{
    (void)context;
    optiga_lib_status = return_status;
}

void clear_terminal(void)
{
    // ESC [2J = clear screen, ESC [H = cursor to top-left
    printf("\033[2J\033[H");
}

uint8_t random_data[16];


#define RUNTIME 0 
#define PROTECT_UPDATED 1

void crypt_authen_key_test(int mode){
   
    //clear_terminal();
    optiga_util_t *optiga_util = NULL;
    optiga_crypt_t *crypt = NULL;
   
    // 2️⃣ Create instances
    do
 {
    optiga_util = optiga_util_create(0, optiga_callback, NULL);
    if(optiga_util!=NULL){
        break;
    }
 } while (optiga_lib_status == OPTIGA_LIB_BUSY);

      crypt = optiga_crypt_create(0, optiga_callback, NULL);
  

    if (NULL == crypt || NULL == optiga_util)
    {
        printf("Failed to create OPTIGA instances\n");
        return;
    }

    // 2️⃣ Create instances
    optiga_util  = optiga_util_create(0, optiga_callback, NULL);
    
    // 3️⃣ Open application
    optiga_lib_status = OPTIGA_LIB_BUSY;
    optiga_util_open_application(optiga_util, 0);
    while (optiga_lib_status == OPTIGA_LIB_BUSY) {}
    if (optiga_lib_status != OPTIGA_LIB_SUCCESS)
    {
        printf("Open application failed\n");
        return;
    }

   // write key
  
   if (mode == PROTECT_UPDATED){
    uint8_t read_meta[100];
    memset(read_meta,0,sizeof(read_meta));
    uint16_t read_meta_length = sizeof(read_meta);
    optiga_lib_status = OPTIGA_LIB_BUSY;
    optiga_key_id_t key_oid[5] ={OPTIGA_KEY_ID_E0F0,OPTIGA_KEY_ID_E0F1,OPTIGA_KEY_ID_E0F2,OPTIGA_KEY_ID_E0F3,OPTIGA_KEY_ID_SECRET_BASED};
     int  i;
     for ( i=0 ;  i < 5;  i++)
    {
    printf("Checking keyoid 0x%4X .......\r\n",key_oid[i]);
    optiga_util_read_metadata(optiga_util,
   key_oid[i],
                                                  read_meta,
                                                  &read_meta_length);

    while (optiga_lib_status == OPTIGA_LIB_BUSY) {}

    if (optiga_lib_status == OPTIGA_LIB_SUCCESS)
    {
        printf("meta_data: ");
        for (int i = 0; i < 17; i++)
            printf("%02X", read_meta[i]);
        printf("\n");
        decode_optiga_metadata(read_meta,18);

    }
}
   }
   
// 4️⃣ Generate random data
    optiga_lib_status = OPTIGA_LIB_BUSY;
    optiga_crypt_random(crypt, OPTIGA_RNG_TYPE_TRNG, random_data, sizeof(random_data));
    while (OPTIGA_LIB_BUSY == optiga_lib_status) {

    }

    if (OPTIGA_LIB_SUCCESS == optiga_lib_status)
    {
       // printf("Random number: ");
       // for (int i = 0; i < sizeof(random_data); i++)
       //     printf("%02X", random_data[i]);
       // printf("\n");
    }
    else
    {
        printf("Random generation failed (%s)\n", optiga_status_to_str(optiga_lib_status));
    }


    uint8_t iv[16] = {           // Initialization Vector
        'T','E','S','A',
        '_','A','I','O',
        'T','1',0xAA,0xAB,
        0xAC,0xAD,0xAE,0xAF
    };
   
     memcpy(&iv[10],&random_data[10],6);

if (mode == PROTECT_UPDATED){


uint8_t trust_anchor_metadata[] = {0x20, 0x06, 0xD3, 0x01, 0x00, 0xE8, 0x01, 0x11};
uint8_t confidentiality_oid_metadata[] = {0x20, 0x06, 0xD3, 0x01, 0x00, 0xE8, 0x01, 0x23};
const unsigned char shared_secret[] = {
    0x49, 0xC9, 0xF4, 0x92, 0xA9, 0x92, 0xF6, 0xD4, 0xC5, 0x4F, 0x5B, 0x12, 0xC5, 0x7E, 0xDB, 0x27,
    0xCE, 0xD2, 0x24, 0x04, 0x8F, 0x25, 0x48, 0x2A, 0xA1, 0x49, 0xC9, 0xF4, 0x92, 0xA9, 0x92, 0xF6,
    0x49, 0xC9, 0xF4, 0x92, 0xA9, 0x92, 0xF6, 0xD4, 0xC5, 0x4F, 0x5B, 0x12, 0xC5, 0x7E, 0xDB, 0x27,
    0xCE, 0xD2, 0x24, 0x04, 0x8F, 0x25, 0x48, 0x2A, 0xA1, 0x49, 0xC9, 0xF4, 0x92, 0xA9, 0x92, 0xF6};	

const uint8_t target_key_oid_metadata[] = 
{
    0x20,
    0x05,
    0xD0,
    0x03,
    0x21,
    0xE0,
    0xE3,
};


   printf ("write meta data Trust anchor\r\n");

    optiga_lib_status = OPTIGA_LIB_BUSY;
   optiga_util_write_metadata(optiga_util,0xe0e3, trust_anchor_metadata ,sizeof(trust_anchor_metadata));

    while (optiga_lib_status == OPTIGA_LIB_BUSY) {
        
    }

  if (optiga_lib_status == OPTIGA_LIB_SUCCESS) {
        printf("trust anchor meta data OK\r\n");
    }else{
        printf("trust anchor meta data  NG :%s  \r\n", optiga_status_to_str(optiga_lib_status));

    }


    optiga_lib_status = OPTIGA_LIB_BUSY;
    optiga_util_write_data(
            optiga_util,
            0xe0e3,
            OPTIGA_UTIL_ERASE_AND_WRITE,
            0,
            trust_anchor1,
            sizeof(trust_anchor1)
        );
    
    while (optiga_lib_status == OPTIGA_LIB_BUSY) {
        
    }

    if (optiga_lib_status == OPTIGA_LIB_SUCCESS) {
        printf("trust auchor data OK\r\n");
    }else{
        printf("trust auchor data NG :%s \r\n",  optiga_status_to_str(optiga_lib_status));

    }

    printf ("write meta data confidentiality_oid_metadata\r\n");

    optiga_lib_status = OPTIGA_LIB_BUSY;
    optiga_util_write_metadata(optiga_util,0xf1d1, confidentiality_oid_metadata ,sizeof(confidentiality_oid_metadata));

    while (optiga_lib_status == OPTIGA_LIB_BUSY) {
        
    }

  if (optiga_lib_status == OPTIGA_LIB_SUCCESS) {
        printf("confidentiality_oid_metadata OK\r\n");
    }else{
        printf("confidentiality_oid_metadata NG :%s\r\n",  optiga_status_to_str(optiga_lib_status));

    }

     optiga_lib_status = OPTIGA_LIB_BUSY;
      optiga_util_write_data(
            optiga_util,
            0xF1D1,
            OPTIGA_UTIL_ERASE_AND_WRITE,
            0,
            shared_secret,
            sizeof(shared_secret)
        );


        while (OPTIGA_LIB_BUSY == optiga_lib_status) {
            // Wait until the optiga_util_write_data operation is completed
        }

    if (optiga_lib_status == OPTIGA_LIB_SUCCESS) {
        printf("shared secret OK\r\n");
    }else{
        printf("shared secret NG :%s \r\n",  optiga_status_to_str(optiga_lib_status));

    }

  
    optiga_protected_update_manifest_fragment_configuration_t data_aes_key_configuration = {
    0x01,
    aes_manifest_data,
    sizeof(aes_manifest_data),
    NULL,
    0,
    aes_key_final_fragment_array,
    sizeof(aes_key_final_fragment_array)};

        const optiga_protected_update_data_configuration_t  optiga_protected_update_data_set[] =
    {
        {
            0xE200,
            target_key_oid_metadata,
            sizeof(target_key_oid_metadata),
            &data_aes_key_configuration, 
            "Protected Update - AES Key"
        },
    };

 optiga_lib_status = OPTIGA_LIB_BUSY;
  optiga_util_write_metadata(
                optiga_util,
                optiga_protected_update_data_set[0].target_oid,
                (uint8_t *)optiga_protected_update_data_set[0].target_oid_metadata,
                (uint8_t)optiga_protected_update_data_set[0].target_oid_metadata_length
            );

 while (optiga_lib_status == OPTIGA_LIB_BUSY) {
        
    }

 if (optiga_lib_status == OPTIGA_LIB_SUCCESS) {
      printf("target oid meta data OK\r\n");
 }else{
     printf("target oid meta data NG :%s \r\n",  optiga_status_to_str(optiga_lib_status));

 }
            
 printf("%s\r\n",optiga_protected_update_data_set->set_prot_example_string);

 optiga_lib_status = OPTIGA_LIB_BUSY;

     optiga_util_protected_update_start(optiga_util, optiga_protected_update_data_set[0].data_config->manifest_version, optiga_protected_update_data_set[0].data_config->manifest_data,optiga_protected_update_data_set[0].data_config->manifest_length );

 while (optiga_lib_status == OPTIGA_LIB_BUSY) {
        
    }

 if (optiga_lib_status == OPTIGA_LIB_SUCCESS) {
        printf("start load manifest data OK\r\n");
    }else{
        printf("start load manifest data NG :%s %04X \r\n",  optiga_status_to_str(optiga_lib_status),optiga_lib_status);

    }
   
    optiga_lib_status = OPTIGA_LIB_BUSY;
    optiga_util_protected_update_final(optiga_util,optiga_protected_update_data_set[0].data_config->final_fragment_data,optiga_protected_update_data_set[0].data_config->final_fragment_length);

    while (optiga_lib_status == OPTIGA_LIB_BUSY) {
        
    }
    if (optiga_lib_status == OPTIGA_LIB_SUCCESS) {
        printf("start load fragment data OK\r\n");
    }else{
        printf("start load fragment data NG :%s %04X \r\n",  optiga_status_to_str(optiga_lib_status),optiga_lib_status);

    }


}else{

 
tesa_hash_pwd.buffer =message;
tesa_hash_pwd.length =17;


optiga_lib_status = OPTIGA_LIB_BUSY;
optiga_crypt_hash(crypt,
                   OPTIGA_HASH_TYPE_SHA_256,
                    OPTIGA_CRYPT_HOST_DATA,
                    &tesa_hash_pwd,
                    digest);


 while (optiga_lib_status == OPTIGA_LIB_BUSY) {
    
 }
 if (optiga_lib_status == OPTIGA_LIB_SUCCESS)
    {
    //printf("digest: ");
     //   for (int i = 0; i < 32; i++)
     //       printf("%02X",digest[i]);
     //   printf("\n");
    }else{
    
        printf("Encryption failed : %s\n",optiga_status_to_str(optiga_lib_status));
    }

    uint8_t ch;
    int index;
    int rc;
    index =0;
    while (1)
    {
        ch = getchar();   // read one character from UART

        if (index < BUFFER_SIZE)
            uart_buffer[index++] = ch;

        /* Detect CRLF */
        if (index >= 2 &&
            uart_buffer[index-2] == '\r' &&
            uart_buffer[index-1] == '\n')
        {
           

            //printf("Received: %s\n", uart_buffer);
            rc = memcmp(digest,uart_buffer,32);
            if (rc !=0){
                printf("FAILED\r\n");
                
            }else{
                printf("OK\r\n");
                break;
            }
             uart_buffer[index] = '\0';
             index = 0;   // reset buffer
        }
    }
  



// enable the encryptedv ...
     memcpy(plain,random_data,16);
       // printf("plaintext: ");
        for (int i = 0; i < 16; i++)
            //printf("%02X",plain[i]);
            putchar(plain[i]);
        putchar(0xd);
        putchar(0xa);    
            //printf("\n");

       memset(uart_buffer,0,sizeof(uart_buffer));     
       index =0;
       while(1){
            
        ch = getchar();   // read one character from UART

        if (index < BUFFER_SIZE)
            uart_buffer[index++] = ch;

        /* Detect CRLF */
        if (index >= 2 &&
            uart_buffer[index-2] == '\r' &&
            uart_buffer[index-1] == '\n')
        {
           

            //printf("Received: %s\n", uart_buffer);
  

            
            uart_buffer[index] = '\0';
            index = 0;   // reset buffer
            break;
        }


        }          

 #if 1
 // 4️⃣ Encrypt
    optiga_lib_status = OPTIGA_LIB_BUSY;
    optiga_crypt_symmetric_encrypt(
        crypt,
        OPTIGA_SYMMETRIC_CBC,        // AES-CBC mode
        0xE200,     
        plain, 
        sizeof(plain),        // Input
        iv, 
        sizeof(iv),              // IV
        NULL,
        0,
        cipher ,&cipher_len                    // Output
    );
    while (optiga_lib_status == OPTIGA_LIB_BUSY) {
        
    }

    if (optiga_lib_status == OPTIGA_LIB_SUCCESS)
    {
        //printf("Ciphertext: ");
        //for (int i = 0; i < sizeof(cipher); i++)
         //   printf("%02X", cipher[i]);
        //printf("\n");
           rc = memcmp(cipher,uart_buffer,16);
        if (rc !=0){
                printf("FAILED\r\n");
            
            }else{
                printf("PSOC_AI_E64>\r\n");
               
            }



    }
    else
    {
        printf("Encryption failed : %s\n",optiga_status_to_str(optiga_lib_status));
    }


 
  printf("end\r\n");
 /* printf("Ciphertext: ");
    for (int i = 0; i < sizeof(cipher); i++)
           printf("%02X", cipher[i]);
        printf("\n");
  printf("IV :");
  for (int i = 0; i < sizeof(cipher); i++)
           printf("%02X", iv[i]);
        printf("\n");  
  printf("uart buff :");
  for (int i = 0; i < 16; i++)
           printf("%02X", uart_buffer[i]);
        printf("\n");*/
#endif

    optiga_util_close_application(optiga_util, 0);
    optiga_util_destroy(optiga_util);
    optiga_crypt_destroy(crypt);

}

}
/***************************************************************************
 * Function Name: optiga_client_task
 ******************************************************************************
 * Summary:
 *  A task to properly initialize the chip and use the certificate installed
 *  to the OPTIGA Secure Element. After this start the MQTT task
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 */
void optiga_client_task(void *pvParameters) {
	CY_UNUSED_PARAMETER(pvParameters);
	
   crypt_authen_key_test(RUNTIME);


}

 
/*******************************************************************************
* Function Name: lptimer_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for LPTimer instance.
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
* 1. This function first configures and initializes an interrupt for LPTimer.
* 2. Then it initializes the LPTimer HAL object to be used in the RTOS
*    tickless idle mode implementation to allow the device enter deep sleep
*    when idle task runs. LPTIMER_0 instance is configured for CM33 CPU.
* 3. It then passes the LPTimer object to abstraction RTOS library that
*    implements tickless idle mode
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
        .intrSrc = CYBSP_CM33_LPTIMER_0_IRQ,
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
                                    Cy_MCWDT_Init(CYBSP_CM33_LPTIMER_0_HW,
                                                &CYBSP_CM33_LPTIMER_0_config);

    /* MCWDT initialization failed. Stop program execution. */
    if(CY_MCWDT_SUCCESS != mcwdt_init_status)
    {
        handle_app_error();
    }

    /* Enable MCWDT instance */
    Cy_MCWDT_Enable(CYBSP_CM33_LPTIMER_0_HW,
                    CY_MCWDT_CTR_Msk,
                    LPTIMER_0_WAIT_TIME_USEC);

    /* Setup LPTimer using the HAL object and desired configuration as defined
     * in the device configurator. */
    cy_rslt_t result = mtb_hal_lptimer_setup(&lptimer_obj,
                                            &CYBSP_CM33_LPTIMER_0_hal_config);

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

/******************************************************************************
 * Function Name: main
 ******************************************************************************
 * Summary:
 *  System entrance point. This function initializes retarget IO, RTC, sets up 
 *  the MQTT client task, enables CM55 and then starts the RTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 ******************************************************************************/

int main(void)
{
    cy_rslt_t result;
 
    /* Initialize the board support package. */
    
    result = cybsp_init();
    CY_ASSERT(CY_RSLT_SUCCESS == result);

    /* To avoid compiler warnings. */
    CY_UNUSED_PARAMETER(result);

    /* Enable global interrupts. */
    __enable_irq();

#

    /* Setup the LPTimer instance for CM33 CPU. */
    setup_tickless_idle_timer();

    /* Initialize retarget-io middleware */
    init_retarget_io();
#if 0
    /* Initialize rtc */
    Cy_RTC_Init(&CYBSP_RTC_config);
    Cy_RTC_SetDateAndTime(&CYBSP_RTC_config);
    
    /* Initialize the CLIB support library */
    mtb_clib_support_init(&obj);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
    printf("\x1b[2J\x1b[;H");
    printf("===============================================================\n");
    printf("PSoC Edge E84 + OPTIGA Trust M + TESAIoT Platform\n");
    printf("===============================================================\n");
    #if TESAIOT_DEBUG_INFO_ENABLED
    printf("%s\n", LABEL_FEATURES);
    #endif /* TESAIOT_DEBUG_INFO_ENABLED */
    printf("  - OPTIGA Trust M V3 hardware security\n");
    printf("  - Protected Update workflow with TESAIoT Platform (MQTT)\n");
    printf("  - Certificate Lifecycle Management (Protected Update)\n");
    printf("  - Factory pre-provisioning + secure certificate renewal\n");
    printf("  - Mutual TLS authentication\n");
    printf("===============================================================\n\n");
#endif
    /* NOTE: TESAIoT License Check is performed in optiga_client_task()
     * AFTER OPTIGA Trust M is initialized. This is required because
     * tesaiot_license_init() reads the device UID from OPTIGA.
     */

    /* Enable CM55. CY_CORTEX_M55_APPL_ADDR must be updated if CM55 memory layout is changed. */
    Cy_SysEnableCM55(MXCM55, CY_CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_US);
    // clear_terminal();

    /*Create an OPTIGA Task */
    result = xTaskCreate(optiga_client_task, "OPTIGA", 1024 * 12, NULL, 2, NULL);


    if( pdPASS == result )
    {
        /* Start the FreeRTOS scheduler. */
        vTaskStartScheduler();
        
        /* Should never get here. */
        handle_app_error();
    }
    else
    {
        handle_app_error();
    }
}

/*******************************************************************************
* FreeRTOS Hook Functions for Debugging
*******************************************************************************/

/**
 * @brief Stack overflow hook - called by FreeRTOS when stack overflow detected
 *
 * This hook is called when configCHECK_FOR_STACK_OVERFLOW = 2 detects overflow.
 * Critical for debugging Publisher task stack issues (256 words = 1024 bytes).
 *
 * @param xTask Handle of task with stack overflow
 * @param pcTaskName Name of task with stack overflow
 */

/* [] END OF FILE */
