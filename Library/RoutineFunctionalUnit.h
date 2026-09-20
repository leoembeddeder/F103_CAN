#ifndef CHARON_ROUTINEFUNCTIONALUNIT_H_
#define CHARON_ROUTINEFUNCTIONALUNIT_H_

/* Includes ******************************************************************/

#include <stdint.h>
#include "uds_types.h"

/* Constants *****************************************************************/

#define UDS_BL_ROUTINE_ERASE_MEMORY     0xFF00U
#define UDS_BL_ROUTINE_CHECK_MEMORY     0x0202U

/* STM32F103 High-Density 512KB Flash Memory Map */
#define UDS_BL_F103_BOOTLOADER_START    0x08000000UL
#define UDS_BL_F103_BOOTLOADER_SIZE     0x00008000UL /* 32 KB (Pages 0-15) */
#define UDS_BL_F103_APP_SLOT_A_START    0x08008000UL /* 240 KB (Pages 16-135) */
#define UDS_BL_F103_APP_SLOT_A_SIZE     0x0003C000UL
#define UDS_BL_F103_APP_SLOT_B_START    0x08044000UL /* 240 KB (Pages 136-251) */
#define UDS_BL_F103_APP_SLOT_B_SIZE     0x0003C000UL
#define UDS_BL_F103_NVM_METADATA_START  0x0807E000UL /* 8 KB (Pages 252-255) */
#define UDS_BL_F103_NVM_METADATA_SIZE   0x00002000UL

/* Macros ********************************************************************/

/* Types *********************************************************************/

typedef enum {
    UDS_BL_SLOT_INVALID = 0,
    UDS_BL_SLOT_CANDIDATE = 1,
    UDS_BL_SLOT_ACTIVE = 2,
    UDS_BL_SLOT_CONFIRMED = 3,
    UDS_BL_SLOT_ROLLBACK = 4
} UdsBootloaderSlotStatus;

typedef struct __attribute__((packed)) {
    uint32_t magic;        /* 0x5544534D ("UDSM") */
    uint32_t version;      /* Monotonic firmware version */
    uint32_t image_size;   /* Image size in bytes */
    uint32_t crc32;        /* Transmission CRC-32 */
    uint8_t  status;       /* UdsBootloaderSlotStatus */
    uint8_t  boot_attempts;/* Attempt count */
    uint8_t  max_attempts; /* Max attempts before rollback */
    uint8_t  active_slot;  /* 0 = Slot A, 1 = Slot B */
} UdsBootloaderMetadata_t;

/* Interfaces ****************************************************************/

/**
 * UDS ISO 14229-1 SE 2013-03-15
 * SID: 0x31
 *
 * @param receiveBuffer Payload
 * @param receiveBufferSize Payload Size
 * @return @see @ref uds_responseCode_t
 */
uds_responseCode_t charon_RoutineFunctionalUnit_RoutineControl (const uint8_t * receiveBuffer, uint32_t receiveBufferSize);

/** @brief Returns pointer to current active bootloader metadata. */
const UdsBootloaderMetadata_t* charon_RoutineFunctionalUnit_GetMetadata (void);

#endif 