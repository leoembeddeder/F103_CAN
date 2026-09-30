#include "NvmEmulator.h"
#include "RoutineFunctionalUnit.h"
#include "negativeResponse.h"
#include "SessionAndServiceControl.h"
#include "ota_metadata_mgr.h"
#include "ota_crc32.h"
#include <string.h>
#include <stdbool.h>

#if defined(STM32F103xE) || defined(STM32F1xx) || defined(USE_HAL_DRIVER)
#include "stm32f1xx_hal.h"
#endif

/* Imports *******************************************************************/

/* Variables *****************************************************************/

static metadata_t s_cachedMeta;
static bool s_metaLoaded = false;

/* Private Function Definitions **********************************************/

static bool eraseSlot(uint8_t slot_id)
{
    if (slot_id > SLOT_B)
    {
        return false;
    }

    uint32_t slot_addr = SLOT_ADDR(slot_id);
    uint32_t nb_pages  = SLOT_SIZE / FLASH_PAGE_SIZE_F103; /* 105 pages */

#if defined(HAL_FLASH_MODULE_ENABLED) || defined(STM32F103xE) || defined(STM32F1xx)
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

    FLASH_EraseInitTypeDef eraseInit;
    memset(&eraseInit, 0, sizeof(eraseInit));
    eraseInit.TypeErase   = FLASH_TYPEERASE_PAGES;
    eraseInit.PageAddress = slot_addr;
    eraseInit.NbPages     = nb_pages;

    uint32_t pageError = 0u;
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&eraseInit, &pageError);
    HAL_FLASH_Lock();

    if (status != HAL_OK)
    {
        return false;
    }
#endif

    /* Mark target slot IN_PROGRESS in metadata */
    metadata_t meta;
    if (ota_metadata_read(&meta) != META_OK)
    {
        (void)ota_metadata_init_default(&meta);
    }
    meta.slot[slot_id].state = STATE_IN_PROGRESS;
    meta.slot[slot_id].size  = 0U;
    (void)ota_metadata_write(&meta);

    return true;
}

/* Interfaces  ***************************************************************/

const metadata_t* charon_RoutineFunctionalUnit_GetMetadata (void)
{
    if (!s_metaLoaded)
    {
        if (ota_metadata_read(&s_cachedMeta) != META_OK)
        {
            (void)ota_metadata_init_default(&s_cachedMeta);
        }
        s_metaLoaded = true;
    }
    return &s_cachedMeta;
}

uds_responseCode_t charon_RoutineFunctionalUnit_RoutineControl (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    uds_responseCode_t result = uds_responseCode_PositiveResponse;
    uint8_t statusRecord = 0x00u;
    bool hasStatusRecord = false;

    if (receiveBufferSize < 4u)
    {
        result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
    }
    else if ((receiveBuffer[1] == 0u) || (receiveBuffer[1] > 3u))
    {
        result = uds_responseCode_SubfunctionNotSupported;
    }
    else
    {
        uint8_t subfunction = receiveBuffer[1];
        uint16_t routineIdentifier = ((uint16_t)receiveBuffer[2] << 8) | receiveBuffer[3];

        if (routineIdentifier == UDS_BL_ROUTINE_ERASE_MEMORY)
        {
            if (subfunction == 0x01u) /* StartRoutine */
            {
                uint8_t target_slot = ota_metadata_get_target_slot();

                if (receiveBufferSize == 4u)
                {
                    /* Erase inactive target slot (Slot A or Slot B) */
                    if (!eraseSlot(target_slot))
                    {
                        result = uds_responseCode_GeneralProgrammingFailure;
                    }
                }
                else if (receiveBufferSize >= 12u)
                {
                    /* Specific erase address supplied */
                    uint32_t eraseAddr = ((uint32_t)receiveBuffer[4] << 24) |
                                         ((uint32_t)receiveBuffer[5] << 16) |
                                         ((uint32_t)receiveBuffer[6] << 8)  |
                                          (uint32_t)receiveBuffer[7];
                    uint32_t eraseLen  = ((uint32_t)receiveBuffer[8] << 24) |
                                         ((uint32_t)receiveBuffer[9] << 16) |
                                         ((uint32_t)receiveBuffer[10] << 8) |
                                          (uint32_t)receiveBuffer[11];

                    if ((eraseAddr == SLOT_A_ADDR) && (eraseLen <= SLOT_SIZE))
                    {
                        target_slot = SLOT_A;
                        if (!eraseSlot(target_slot))
                        {
                            result = uds_responseCode_GeneralProgrammingFailure;
                        }
                    }
                    else if ((eraseAddr == SLOT_B_ADDR) && (eraseLen <= SLOT_SIZE))
                    {
                        target_slot = SLOT_B;
                        if (!eraseSlot(target_slot))
                        {
                            result = uds_responseCode_GeneralProgrammingFailure;
                        }
                    }
                    else
                    {
                        result = uds_responseCode_RequestOutOfRange;
                    }
                }
                else
                {
                    result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
                }
            }
            else
            {
                result = uds_responseCode_SubfunctionNotSupported;
            }
        }
        else if (routineIdentifier == UDS_BL_ROUTINE_CHECK_MEMORY)
        {
            if (subfunction == 0x01u) /* StartRoutine */
            {
                uint8_t target_slot = ota_metadata_get_target_slot();
                uint32_t checkAddr = SLOT_ADDR(target_slot);
                uint32_t checkLen  = SLOT_SIZE;
                uint32_t expectedCrc = 0u;

                if (receiveBufferSize == 16u)
                {
                    /* Format: 31 01 02 02 [Addr 4B] [Length 4B] [CRC32 4B] */
                    checkAddr = ((uint32_t)receiveBuffer[4] << 24) |
                                ((uint32_t)receiveBuffer[5] << 16) |
                                ((uint32_t)receiveBuffer[6] << 8)  |
                                 (uint32_t)receiveBuffer[7];
                    checkLen  = ((uint32_t)receiveBuffer[8] << 24) |
                                ((uint32_t)receiveBuffer[9] << 16) |
                                ((uint32_t)receiveBuffer[10] << 8) |
                                 (uint32_t)receiveBuffer[11];
                    expectedCrc = ((uint32_t)receiveBuffer[12] << 24) |
                                  ((uint32_t)receiveBuffer[13] << 16) |
                                  ((uint32_t)receiveBuffer[14] << 8)  |
                                   (uint32_t)receiveBuffer[15];

                    if (checkAddr == SLOT_A_ADDR)
                    {
                        target_slot = SLOT_A;
                    }
                    else if (checkAddr == SLOT_B_ADDR)
                    {
                        target_slot = SLOT_B;
                    }
                }
                else if (receiveBufferSize == 8u)
                {
                    /* Format: 31 01 02 02 [CRC32 4B] */
                    expectedCrc = ((uint32_t)receiveBuffer[4] << 24) |
                                  ((uint32_t)receiveBuffer[5] << 16) |
                                  ((uint32_t)receiveBuffer[6] << 8)  |
                                   (uint32_t)receiveBuffer[7];
                }
                else
                {
                    result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
                }

                if (result == uds_responseCode_PositiveResponse)
                {
                    if ((checkAddr != SLOT_A_ADDR && checkAddr != SLOT_B_ADDR) ||
                        (checkLen == 0u) || (checkLen > SLOT_SIZE))
                    {
                        result = uds_responseCode_RequestOutOfRange;
                    }
                    else
                    {
                        uint32_t computedCrc = ota_crc32_compute((const void *)(uintptr_t)checkAddr, checkLen);
                        hasStatusRecord = true;
                        if (computedCrc == expectedCrc)
                        {
                            statusRecord = 0x00u; /* 0x00: Verification Successful */
                            /* Update ping-pong metadata: set target slot as candidate in TESTING state */
                            (void)ota_metadata_set_candidate(target_slot, checkLen, computedCrc, 1U);
                            (void)ota_metadata_read(&s_cachedMeta);
                        }
                        else
                        {
                            statusRecord = 0x01u; /* 0x01: Verification Failed */
                        }
                    }
                }
            }
            else if (subfunction == 0x03u) /* RequestRoutineResults */
            {
                hasStatusRecord = true;
                metadata_t meta;
                if (ota_metadata_read(&meta) == META_OK)
                {
                    uint8_t target = ota_metadata_get_target_slot();
                    statusRecord = (meta.slot[target].state == STATE_TESTING) ? 0x00u : 0x01u;
                }
                else
                {
                    statusRecord = 0x01u;
                }
            }
            else
            {
                result = uds_responseCode_SubfunctionNotSupported;
            }
        }
        else
        {
            result = uds_responseCode_RequestOutOfRange;
        }
    }

    if (result != uds_responseCode_PositiveResponse)
    {
        charon_sendNegativeResponse(result, uds_sid_RoutineControl);
    }
    else
    {
        if (hasStatusRecord)
        {
            uint8_t transmitBuffer[5] = {
                receiveBuffer[0] | (uint8_t)uds_sid_PositiveResponseMask,
                receiveBuffer[1],
                receiveBuffer[2],
                receiveBuffer[3],
                statusRecord
            };
            charon_sscTxMessage(transmitBuffer, sizeof(transmitBuffer));
        }
        else
        {
            uint8_t transmitBuffer[4] = {
                receiveBuffer[0] | (uint8_t)uds_sid_PositiveResponseMask,
                receiveBuffer[1],
                receiveBuffer[2],
                receiveBuffer[3]
            };
            charon_sscTxMessage(transmitBuffer, sizeof(transmitBuffer));
        }
    }
    return result;
}
