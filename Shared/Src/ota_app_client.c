/**
 * @file ota_app_client.c
 * @brief Application-side A/B Dual Slot OTA Client & Self-Confirmation implementation
 */

#include "ota_app_client.h"
#include "ota_metadata_mgr.h"
#include "ota_boot_request.h"
#include "ota_crc32.h"

#if defined(STM32F103xE) || defined(STM32F1xx) || defined(USE_HAL_DRIVER)
#include "stm32f1xx_hal.h"
#endif

uint8_t ota_app_get_current_slot(void) {
    /* SCB->VTOR holds the active vector table base */
#if defined(SCB)
    uint32_t vtor = SCB->VTOR;
    if ((vtor >= SLOT_B_ADDR) && (vtor < (SLOT_B_ADDR + SLOT_SIZE))) {
        return SLOT_B;
    }
#endif
    return SLOT_A;
}

bool ota_app_verify_self(void) {
    metadata_t meta;
    if (ota_metadata_read(&meta) != META_OK) {
        return false;
    }

    uint8_t self_slot = ota_app_get_current_slot();
    return ota_metadata_image_is_intact(&meta, self_slot);
}

bool ota_app_confirm_startup(void) {
    metadata_t meta;
    if (ota_metadata_read(&meta) != META_OK) {
        return false;
    }

    uint8_t self_slot = ota_app_get_current_slot();

    /* If already confirmed and boot failure count is cleared, no write needed */
    if ((meta.slot[self_slot].state == STATE_VALID) && (meta.boot_fail_count == 0U)) {
        return true;
    }

    /* Transition self slot to STATE_VALID and reset boot failure counter */
    meta.slot[self_slot].state = STATE_VALID;
    meta.active_slot           = self_slot;
    meta.boot_fail_count       = 0U;

    return (ota_metadata_write(&meta) == META_OK);
}

void ota_app_request_bootloader_update(void) {
    ota_boot_request_set();
#if defined(NVIC_SystemReset)
    NVIC_SystemReset();
#endif
}
