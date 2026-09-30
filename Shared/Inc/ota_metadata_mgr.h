/**
 * @file ota_metadata_mgr.h
 * @brief Redundant Ping-Pong Metadata Manager for STM32F103 A/B Dual Slot OTA
 */

#ifndef OTA_METADATA_MGR_H
#define OTA_METADATA_MGR_H

#include "ota_metadata.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validates integrity, magic, CRC, and structural bounds of a metadata record.
 */
bool ota_metadata_is_valid(const metadata_t *meta);

/**
 * @brief Reads the authoritative metadata record from the two ping-pong pages.
 * @param dest Output structure to populate with the latest valid metadata.
 * @return META_OK on success, or error code if both pages are invalid/erased.
 */
metadata_status_t ota_metadata_read(metadata_t *dest);

/**
 * @brief Writes updated metadata to the inactive page, increments counter, and computes CRC32.
 * @param src Metadata contents to write.
 * @return META_OK on success.
 */
metadata_status_t ota_metadata_write(const metadata_t *src);

/**
 * @brief Initializes factory/default metadata if the flash pages are empty.
 */
metadata_status_t ota_metadata_init_default(metadata_t *dest);

/**
 * @brief Marks candidate image in target slot, switches active_slot to it, and sets state to TESTING.
 */
metadata_status_t ota_metadata_set_candidate(uint8_t target_slot, uint32_t size, uint32_t crc32, uint32_t version);

/**
 * @brief Application self-confirmation: transitions active slot to VALID and clears boot_fail_count.
 */
metadata_status_t ota_metadata_confirm(void);

/**
 * @brief Reverts active slot to the alternate fallback slot and marks failed slot as EMPTY.
 */
metadata_status_t ota_metadata_rollback(void);

/**
 * @brief Computes CRC32 over the physical firmware image in flash and compares against metadata.
 */
bool ota_metadata_image_is_intact(const metadata_t *meta, uint8_t slot);

/**
 * @brief Returns current active slot (0 = Slot A, 1 = Slot B).
 */
uint8_t ota_metadata_get_active_slot(void);

/**
 * @brief Returns the inactive target slot ready for download (0 = Slot A, 1 = Slot B).
 */
uint8_t ota_metadata_get_target_slot(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_METADATA_MGR_H */
