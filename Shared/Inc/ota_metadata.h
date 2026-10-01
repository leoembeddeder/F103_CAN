/**
 * @file ota_metadata.h
 * @brief Flash Memory Layout & A/B Dual-Slot OTA Metadata Definitions for STM32F103 High-Density (512 KB)
 *
 * Partition Layout (512 KB Flash, 256 physical pages of 2 KB each):
 *   0x08000000 - 0x0800FFFF : BOOTLOADER  (64 KB,  32 pages)
 *   0x08010000 - 0x080447FF : SLOT A       (210 KB, 105 pages)
 *   0x08044800 - 0x08078FFF : SLOT B       (210 KB, 105 pages)
 *   0x08079000 - 0x080797FF : METADATA A   (2 KB,   1 page)
 *   0x08079800 - 0x08079FFF : METADATA B   (2 KB,   1 page)
 *   0x0807A000 - 0x0807FFFF : DTC / NVM    (24 KB,  12 pages)
 */

#ifndef OTA_METADATA_H
#define OTA_METADATA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================
 * Flash Memory Map (STM32F103xE, 512 KB Flash)
 * ========================================================= */
#define FLASH_BASE_ADDR         0x08000000UL
#define FLASH_TOTAL_SIZE        (512U * 1024U)
#define FLASH_PAGE_SIZE_F103    2048U          /* 2 KB per page for High-Density */

#define BOOTLOADER_ADDR         0x08000000UL
#define BOOTLOADER_SIZE         (64U  * 1024U) /* 32 pages */

#define SLOT_A_ADDR             0x08010000UL
#define SLOT_B_ADDR             0x08044800UL
#define SLOT_SIZE               (210U * 1024U) /* 105 pages = 215,040 bytes */

#define META_PAGE_A_ADDR        0x08079000UL   /* Page 242 */
#define META_PAGE_B_ADDR        0x08079800UL   /* Page 243 */
#define META_PAGE_SIZE          2048U

#define DTC_STORAGE_ADDR        0x0807A000UL   /* Page 244 */
#define DTC_STORAGE_SIZE        (24U  * 1024U) /* 12 pages */

/* =========================================================
 * Slot Identifiers
 * ========================================================= */
#define SLOT_A                  0U
#define SLOT_B                  1U

#define SLOT_ADDR(s)            (((s) == SLOT_A) ? SLOT_A_ADDR : SLOT_B_ADDR)
#define OTHER_SLOT(s)           (((s) == SLOT_A) ? SLOT_B : SLOT_A)

/* =========================================================
 * Firmware State Machine
 * ========================================================= */
typedef enum {
    STATE_EMPTY       = 0x00U,  /**< Slot empty or erased */
    STATE_IN_PROGRESS = 0x01U,  /**< Transfer started, write ongoing */
    STATE_TESTING     = 0x02U,  /**< Installed & verified, awaiting app confirmation */
    STATE_VALID       = 0x03U   /**< Fully confirmed and bootable */
} fw_state_t;

/* =========================================================
 * Per-Slot Descriptor (16 bytes, no padding)
 * ========================================================= */
typedef struct {
    uint32_t size;              /**< Firmware image size in bytes */
    uint32_t crc32;             /**< Expected IEEE 802.3 CRC32 */
    uint32_t version;           /**< Monotonic firmware version */
    uint8_t  state;             /**< fw_state_t */
    uint8_t  reserved[3];       /**< Alignment to 16 bytes */
} slot_info_t;

/* =========================================================
 * Dual-Slot Metadata Structure (48 bytes, 8-byte aligned)
 * ========================================================= */
typedef struct {
    uint32_t    magic;          /**< METADATA_MAGIC (0x424C4D44 "BLMD") */
    uint32_t    counter;        /**< Monotonic ping-pong counter */
    slot_info_t slot[2];        /**< Independent descriptors for Slot A and Slot B */
    uint8_t     active_slot;    /**< Currently active boot slot (SLOT_A or SLOT_B) */
    uint8_t     boot_fail_count;/**< Boots without confirmation (resets on VALID) */
    uint8_t     reserved[2];    /**< Alignment padding */
    uint32_t    meta_crc32;     /**< CRC32 covering the first 44 bytes */
} metadata_t;

#define METADATA_MAGIC          0x424C4D44UL   /* "BLMD" */
#define MAX_BOOT_FAILURES       3U

/* Status return codes */
typedef enum {
    META_OK         =  0,
    META_ERR_ARG    = -1,
    META_ERR_ERASE  = -2,
    META_ERR_WRITE  = -3,
    META_ERR_VERIFY = -4,
    META_ERR_NONE   = -5
} metadata_status_t;

#ifdef __cplusplus
}
#endif

#endif /* OTA_METADATA_H */
