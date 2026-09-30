/**
 * @file ota_crc32.c
 * @brief Standard IEEE 802.3 CRC32 implementation
 */

#include "ota_crc32.h"

#define CRC32_POLY 0xEDB88320UL

uint32_t ota_crc32_update(uint32_t current_crc, const void *data, size_t length) {
    if (data == NULL) {
        return current_crc;
    }

    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = current_crc;

    for (size_t i = 0; i < length; ++i) {
        crc ^= p[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 1U) ? ((crc >> 1U) ^ CRC32_POLY) : (crc >> 1U);
        }
    }

    return crc;
}

uint32_t ota_crc32_compute(const void *data, size_t length) {
    uint32_t crc = ota_crc32_update(OTA_CRC32_INIT, data, length);
    return (crc ^ 0xFFFFFFFFUL);
}
