/**
 * @file bootloader_core.h
 * @brief Bootloader Core State Machine & Decision Engine
 */

#ifndef BOOTLOADER_CORE_H
#define BOOTLOADER_CORE_H

#include "ota_metadata.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOOT_ACTION_JUMP_APP,
    BOOT_ACTION_ENTER_UPDATE_MODE,
    BOOT_ACTION_ROLLBACK_AND_JUMP,
    BOOT_ACTION_PANIC
} boot_action_t;

/**
 * @brief Evaluates boot-request flag, reads ping-pong metadata, checks CRC, and decides boot action.
 * @param target_app_addr Output pointer to receive the address to jump to if action is JUMP.
 * @return boot_action_t Recommended action.
 */
boot_action_t bootloader_evaluate(uint32_t *target_app_addr);

/**
 * @brief Main bootloader entry function. Runs evaluation and jumps to app or enters UDS update mode.
 */
void bootloader_main(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_CORE_H */
