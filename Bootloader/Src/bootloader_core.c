/**
 * @file bootloader_core.c
 * @brief Bootloader Core State Machine & Decision Engine implementation
 */

#include "bootloader_core.h"
#include "bootloader_jump.h"
#include "ota_metadata_mgr.h"
#include "ota_boot_request.h"

boot_action_t bootloader_evaluate(uint32_t *target_app_addr) {
    if (target_app_addr == NULL) {
        return BOOT_ACTION_PANIC;
    }

    /* 1. Check Boot-Request flag in RAM */
    if (ota_boot_request_is_pending()) {
        ota_boot_request_clear();
        return BOOT_ACTION_ENTER_UPDATE_MODE;
    }

    /* 2. Read Metadata from Ping-Pong Flash Pages */
    metadata_t meta;
    if (ota_metadata_read(&meta) != META_OK) {
        /* Blank flash: initialize defaults */
        if (ota_metadata_init_default(&meta) != META_OK) {
            return BOOT_ACTION_ENTER_UPDATE_MODE;
        }
    }

    uint8_t active = meta.active_slot;
    uint8_t other  = OTHER_SLOT(active);

    switch (meta.slot[active].state) {
        case STATE_VALID: {
            uint32_t active_addr = SLOT_ADDR(active);
            if (bootloader_is_slot_bootable(active_addr)) {
                /* If size is registered, verify full image CRC */
                if (meta.slot[active].size > 0U) {
                    if (ota_metadata_image_is_intact(&meta, active)) {
                        *target_app_addr = active_addr;
                        return BOOT_ACTION_JUMP_APP;
                    }
                } else {
                    /* Unrestricted / development image without registered size */
                    *target_app_addr = active_addr;
                    return BOOT_ACTION_JUMP_APP;
                }
            }

            /* Active slot invalid despite VALID state -> check fallback */
            if (bootloader_is_slot_bootable(SLOT_ADDR(other))) {
                (void)ota_metadata_rollback();
                *target_app_addr = SLOT_ADDR(other);
                return BOOT_ACTION_ROLLBACK_AND_JUMP;
            }
            break;
        }

        case STATE_TESTING: {
            /* Check if maximum boot failure attempts exceeded */
            if (meta.boot_fail_count >= MAX_BOOT_FAILURES) {
                /* Automatic Rollback */
                uint32_t fallback_addr = SLOT_ADDR(other);
                if (meta.slot[other].state != STATE_EMPTY &&
                    bootloader_is_slot_bootable(fallback_addr)) {
                    (void)ota_metadata_rollback();
                    *target_app_addr = fallback_addr;
                    return BOOT_ACTION_ROLLBACK_AND_JUMP;
                }
                /* No usable fallback image */
                return BOOT_ACTION_ENTER_UPDATE_MODE;
            }

            uint32_t active_addr = SLOT_ADDR(active);
            if (bootloader_is_slot_bootable(active_addr) &&
                ota_metadata_image_is_intact(&meta, active)) {
                /* Increment failure counter BEFORE jumping so crash during boot counts */
                metadata_t attempt = meta;
                attempt.boot_fail_count = (uint8_t)(meta.boot_fail_count + 1U);
                (void)ota_metadata_write(&attempt);

                *target_app_addr = active_addr;
                return BOOT_ACTION_JUMP_APP;
            }

            /* Image under test is corrupted -> rollback immediately */
            if (bootloader_is_slot_bootable(SLOT_ADDR(other))) {
                (void)ota_metadata_rollback();
                *target_app_addr = SLOT_ADDR(other);
                return BOOT_ACTION_ROLLBACK_AND_JUMP;
            }
            break;
        }

        case STATE_IN_PROGRESS:
        case STATE_EMPTY:
        default: {
            /* Try fallback slot */
            uint32_t fallback_addr = SLOT_ADDR(other);
            if ((meta.slot[other].state == STATE_VALID) && 
                bootloader_is_slot_bootable(fallback_addr)) {
                (void)ota_metadata_rollback();
                *target_app_addr = fallback_addr;
                return BOOT_ACTION_ROLLBACK_AND_JUMP;
            }
            break;
        }
    }

    return BOOT_ACTION_ENTER_UPDATE_MODE;
}

void bootloader_main(void) {
    uint32_t jump_target = 0U;
    boot_action_t action = bootloader_evaluate(&jump_target);

    if ((action == BOOT_ACTION_JUMP_APP) || (action == BOOT_ACTION_ROLLBACK_AND_JUMP)) {
        bootloader_jump_to_app(jump_target);
    }

    /* Update mode: stay in bootloader and await UDS transfer over CAN */
}
