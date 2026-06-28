#pragma once
// Header included via esphome: includes: in hub_main.yaml
// Adds the ESP-IDF headers needed by custom lambdas.
#include "esp_chip_info.h"   // esp_chip_info_t, CHIP_FEATURE_*
#include "esp_flash.h"       // esp_flash_get_size
#include "esp_heap_caps.h"   // heap_caps_get_total_size/free_size, MALLOC_CAP_*
#include "esp_wifi.h"        // wifi_ap_record_t, esp_wifi_sta_get_ap_info, WIFI_IF_STA
#include "esp_netif.h"       // esp_netif_t, esp_netif_get_default_netif, esp_netif_ip_info_t
