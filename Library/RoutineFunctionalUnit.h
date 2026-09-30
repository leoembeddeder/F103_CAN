#ifndef CHARON_ROUTINEFUNCTIONALUNIT_H_
#define CHARON_ROUTINEFUNCTIONALUNIT_H_

/* Includes ******************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"
#include "ota_metadata.h"
#include "ota_metadata_mgr.h"

/* Constants *****************************************************************/

#define UDS_BL_ROUTINE_ERASE_MEMORY     0xFF00U
#define UDS_BL_ROUTINE_CHECK_MEMORY     0x0202U

/* STM32F103 High-Density 512KB Flash Memory Map (256 pages of 2 KB each) */
#define UDS_BL_F103_BOOTLOADER_START    BOOTLOADER_ADDR   /* 0x08000000 (64 KB,  Pages 0-31)   */
#define UDS_BL_F103_BOOTLOADER_SIZE     BOOTLOADER_SIZE
#define UDS_BL_F103_APP_SLOT_A_START    SLOT_A_ADDR       /* 0x08010000 (210 KB, Pages 32-136) */
#define UDS_BL_F103_APP_SLOT_A_SIZE     SLOT_SIZE
#define UDS_BL_F103_APP_SLOT_B_START    SLOT_B_ADDR       /* 0x08044800 (210 KB, Pages 137-241)*/
#define UDS_BL_F103_APP_SLOT_B_SIZE     SLOT_SIZE
#define UDS_BL_F103_METADATA_A_START    META_PAGE_A_ADDR  /* 0x08079000 (2 KB,   Page 242)     */
#define UDS_BL_F103_METADATA_B_START    META_PAGE_B_ADDR  /* 0x08079800 (2 KB,   Page 243)     */
#define UDS_BL_F103_DTC_START           DTC_STORAGE_ADDR  /* 0x0807A000 (24 KB,  Pages 244-255)*/
#define UDS_BL_F103_DTC_SIZE            DTC_STORAGE_SIZE

/* Types *********************************************************************/

typedef enum {
    UDS_BL_SLOT_INVALID   = STATE_EMPTY,
    UDS_BL_SLOT_IN_PROG   = STATE_IN_PROGRESS,
    UDS_BL_SLOT_CANDIDATE = STATE_TESTING,
    UDS_BL_SLOT_ACTIVE    = STATE_VALID,
    UDS_BL_SLOT_CONFIRMED = STATE_VALID,
    UDS_BL_SLOT_ROLLBACK  = 4
} UdsBootloaderSlotStatus;

/* Interfaces ****************************************************************/

/**
 * UDS ISO 14229-1 SE 2013-03-15
 * SID: 0x31 RoutineControl
 *
 * @param receiveBuffer Payload
 * @param receiveBufferSize Payload Size
 * @return @see @ref uds_responseCode_t
 */
uds_responseCode_t charon_RoutineFunctionalUnit_RoutineControl (const uint8_t * receiveBuffer, uint32_t receiveBufferSize);

/** @brief Returns pointer to current active bootloader metadata structure. */
const metadata_t* charon_RoutineFunctionalUnit_GetMetadata (void);

#endif /* CHARON_ROUTINEFUNCTIONALUNIT_H_ */