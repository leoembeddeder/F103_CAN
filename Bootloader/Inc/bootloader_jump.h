/**
 * @file bootloader_jump.h
 * @brief Cortex-M3 Application Jump Routine for STM32F103
 */

#ifndef BOOTLOADER_JUMP_H
#define BOOTLOADER_JUMP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Checks if a given flash address contains a valid stack pointer and reset handler.
 * @param app_addr Start address of the application slot.
 * @return true if bootable.
 */
bool bootloader_is_slot_bootable(uint32_t app_addr);

/**
 * @brief De-initializes peripherals, sets VTOR and MSP, and branches to application entry.
 * @param app_addr Start address of the application slot (e.g. 0x08010000 or 0x08044800).
 * @note This function never returns on success.
 */
void bootloader_jump_to_app(uint32_t app_addr);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_JUMP_H */
