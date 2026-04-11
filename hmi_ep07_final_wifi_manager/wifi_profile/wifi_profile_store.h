#ifndef WIFI_PROFILE_STORE_H
#define WIFI_PROFILE_STORE_H

#include "wifi_profile_types.h"

#include <stdbool.h>

bool wifi_profile_store_load(wifi_profile_data_t *out_profile);
bool wifi_profile_store_save(const wifi_profile_data_t *profile);
bool wifi_profile_store_clear(void);

#endif /* WIFI_PROFILE_STORE_H */