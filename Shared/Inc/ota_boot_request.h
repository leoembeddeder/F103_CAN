/**
 * @file ota_boot_request.h
 * @brief Boot Request Flag Mechanism for Soft-Reset OTA Activation
 *
 * For STM32F103 High-Density (STM32F103xE) with 64 KB SRAM (0x20000000 - 0x20010000):
 * - Last 32-bit word in RAM: 0x2000FFFC
 * - Stack pointer (_estack) is lowered to 0x2000FFF8 (8-byte AAPCS alignment)
 *
 * Soft reset via NVIC_SystemReset() preserves SRAM contents across reset cycles.
 */

#ifndef OTA_BOOT_REQUEST_H
#define OTA_BOOT_REQUEST_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_REQUEST_MAGIC       0xB007C0DEUL
#define BOOT_REQUEST_RAM_ADDR    0x2000FFFCUL

/* Hardware pointer to the reserved RAM word */
#define BOOT_REQUEST_FLAG_PTR    ((volatile uint32_t *)BOOT_REQUEST_RAM_ADDR)

/**
 * @brief Checks if a bootloader request was posted before the last reset.
 */
static inline bool ota_boot_request_is_pending(void) {
    return (*BOOT_REQUEST_FLAG_PTR == BOOT_REQUEST_MAGIC);
}

/**
 * @brief Sets the bootloader request flag in RAM.
 */
static inline void ota_boot_request_set(void) {
    *BOOT_REQUEST_FLAG_PTR = BOOT_REQUEST_MAGIC;
}

/**
 * @brief Clears the bootloader request flag in RAM.
 */
static inline void ota_boot_request_clear(void) {
    *BOOT_REQUEST_FLAG_PTR = 0U;
}

#ifdef __cplusplus
}
#endif

#endif /* OTA_BOOT_REQUEST_H */
