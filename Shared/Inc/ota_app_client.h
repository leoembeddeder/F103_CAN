/**
 * @file ota_app_client.h
 * @brief Application-side A/B Dual Slot OTA Client & Self-Confirmation Hooks
 */

#ifndef OTA_APP_CLIENT_H
#define OTA_APP_CLIENT_H

#include "ota_metadata.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Detects which slot (SLOT_A or SLOT_B) the application is currently running from.
 */
uint8_t ota_app_get_current_slot(void);

/**
 * @brief Verifies that the application itself matches its metadata CRC32.
 */
bool ota_app_verify_self(void);

/**
 * @brief Confirms successful application boot, transitioning state from TESTING to VALID.
 * @note Call this in main() after peripherals, CAN, and diagnostic stacks are initialized.
 * @return true if successfully confirmed or already VALID.
 */
bool ota_app_confirm_startup(void);

/**
 * @brief Triggers an immediate reboot into the bootloader for OTA update.
 */
void ota_app_request_bootloader_update(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_APP_CLIENT_H */
