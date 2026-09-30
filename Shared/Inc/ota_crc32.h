/**
 * @file ota_crc32.h
 * @brief Standard IEEE 802.3 CRC32 calculation routines
 */

#ifndef OTA_CRC32_H
#define OTA_CRC32_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_CRC32_INIT 0xFFFFFFFFUL

/**
 * @brief Computes standard IEEE 802.3 CRC32 across a memory buffer.
 */
uint32_t ota_crc32_compute(const void *data, size_t length);

/**
 * @brief Updates a rolling CRC32 value with a new chunk of data.
 */
uint32_t ota_crc32_update(uint32_t current_crc, const void *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* OTA_CRC32_H */
