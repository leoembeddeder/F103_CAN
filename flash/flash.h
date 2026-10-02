#ifndef __FLASH_H__
#define __FLASH_H__


#include "main.h"
#include "ota_metadata.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "syntropic.h"

#define STM32F103_FLASH_BASE      0x08000000U
#define STM32F103_PAGE_SIZE       0x00000800U       /* FLASH Page Size, 2 KBytes */

/* Safe DTC / NVM parameter storage region at top of flash (24 KB, Pages 244-255) */
#define FLASH_PARAM_START         DTC_STORAGE_ADDR
#define FLASH_PARAM_SIZE          DTC_STORAGE_SIZE



uint32_t GetPage(uint32_t Addr);

SYN_Status syn_port_flash_write_word(uint32_t addr, const void *buf, size_t len);


/**
 * @brief Erase a flash sector.
 *
 * @param addr  Start address of the sector (must be sector-aligned).
 * @return SYN_OK on success.
 */
SYN_Status syn_port_flash_erase(uint32_t addr);

/**
 * @brief Read from flash.
 *
 * @param addr  Source address in flash.
 * @param buf   Destination buffer.
 * @param len   Number of bytes to read.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_flash_read(uint32_t addr, void *buf, size_t len);

/**
 * @brief Write to flash.
 *
 * Flash must be erased before writing (writes can only clear bits).
 * The implementation should handle any alignment requirements.
 *
 * @param addr  Destination address in flash.
 * @param buf   Source data.
 * @param len   Number of bytes to write.
 * @return SYN_OK on success.
 */
SYN_Status syn_port_flash_write(uint32_t addr, const void *buf, size_t len);

/**
 * @brief Get the sector size for the given address.
 *
 * @param addr  Address within the sector.
 * @return Sector size in bytes.
 */
uint32_t syn_port_flash_sector_size(uint32_t addr);



void stm32f103_flash_test(void);



#endif
