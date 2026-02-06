/**
 * SPDX-FileCopyrightText: 2024-2025 Assoc. Prof. Wiroon Sriborrirux (TESAIoT Platform Creator)
 *
 * \file optiga_oid_config.h
 *
 * \brief DEPRECATED - Wrapper for backward compatibility
 *
 * This file now redirects to tesaiot_oid_config.h for proper TESAIoT namespace.
 *
 * MIGRATION GUIDE:
 * Replace: #include "optiga_oid_config.h"
 * With:    #include "tesaiot_oid_config.h"
 *
 * This wrapper will be removed in v1.0
 *
 * \ingroup TESAIoT
 */

#ifndef OPTIGA_OID_CONFIG_H_
#define OPTIGA_OID_CONFIG_H_

/* Deprecation warning - shown once per compilation unit */
#if defined(__GNUC__) || defined(__clang__)
#warning "optiga_oid_config.h is deprecated. Use tesaiot_oid_config.h instead. This wrapper will be removed in v1.0"
#elif defined(_MSC_VER)
#pragma message("Warning: optiga_oid_config.h is deprecated. Use tesaiot_oid_config.h instead. This wrapper will be removed in v1.0")
#endif

/* Redirect to canonical header - tesaiot_config.h now contains OID config */
#include "tesaiot_config.h"

#endif /* OPTIGA_OID_CONFIG_H_ */
