#include "NvmEmulator.h"
#include "RoutineFunctionalUnit.h"
#include "negativeResponse.h"
#include "SessionAndServiceControl.h"
#include <string.h>
#include <stdbool.h>

#if defined(STM32F103xE) || defined(STM32F1xx)
#include "stm32f1xx_hal.h"
#endif

/* Imports *******************************************************************/

/* Macros ********************************************************************/

#define UDS_BL_MAGIC_UDSM 0x5544534DUL

/* Types *********************************************************************/

/* Variables *****************************************************************/

static UdsBootloaderMetadata_t s_bootloaderMeta = {
    .magic = UDS_BL_MAGIC_UDSM,
    .version = 1u,
    .image_size = 0u,
    .crc32 = 0u,
    .status = (uint8_t)UDS_BL_SLOT_CONFIRMED,
    .boot_attempts = 0u,
    .max_attempts = 3u,
    .active_slot = 0u
};

/* Private Function Definitions **********************************************/

static uint32_t calculateCrc32(const uint8_t *pData, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    if (pData == NULL)
    {
        return 0u;
    }
    for (uint32_t i = 0u; i < length; ++i)
    {
        crc ^= pData[i];
        for (uint8_t bit = 0u; bit < 8u; ++bit)
        {
            crc = (crc & 1u) ? ((crc >> 1u) ^ 0xEDB88320UL) : (crc >> 1u);
        }
    }
    return (crc ^ 0xFFFFFFFFUL);
}

static bool eraseSlotB(void)
{
#if defined(HAL_FLASH_MODULE_ENABLED) || defined(STM32F103xE)
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
    FLASH_EraseInitTypeDef eraseInit;
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.PageAddress = UDS_BL_F103_APP_SLOT_B_START;
    eraseInit.NbPages = UDS_BL_F103_APP_SLOT_B_SIZE / 2048u; /* 120 pages */
    uint32_t pageError = 0u;
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&eraseInit, &pageError);
    HAL_FLASH_Lock();
    return (status == HAL_OK);
#else
    return true;
#endif
}

/* Interfaces  ***************************************************************/

const UdsBootloaderMetadata_t* charon_RoutineFunctionalUnit_GetMetadata (void)
{
    return &s_bootloaderMeta;
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
    else if ( (receiveBuffer[1] == 0u) || (receiveBuffer[1] > 3u) )
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
                if (receiveBufferSize == 4u)
                {
                    /* Erase Candidate Firmware Slot B partition directly in Flash */
                    if (!eraseSlotB())
                    {
                        result = uds_responseCode_GeneralProgrammingFailure;
                    }
                }
                else if (receiveBufferSize >= 12u)
                {
                    /* Memory erase targeting flash slot */
                    uint32_t eraseAddr = ((uint32_t)receiveBuffer[4] << 24) |
                                         ((uint32_t)receiveBuffer[5] << 16) |
                                         ((uint32_t)receiveBuffer[6] << 8)  |
                                          (uint32_t)receiveBuffer[7];
                    uint32_t eraseLen  = ((uint32_t)receiveBuffer[8] << 24) |
                                         ((uint32_t)receiveBuffer[9] << 16) |
                                         ((uint32_t)receiveBuffer[10] << 8) |
                                          (uint32_t)receiveBuffer[11];

                    if ((eraseAddr >= UDS_BL_F103_APP_SLOT_B_START) &&
                        ((eraseAddr + eraseLen) <= (UDS_BL_F103_APP_SLOT_B_START + UDS_BL_F103_APP_SLOT_B_SIZE)))
                    {
                        if (!eraseSlotB())
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
                uint32_t checkAddr = UDS_BL_F103_APP_SLOT_B_START;
                uint32_t checkLen  = UDS_BL_F103_APP_SLOT_B_SIZE;
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
                }
                else if (receiveBufferSize == 8u)
                {
                    /* Format: 31 01 02 02 [CRC32 4B] defaulting to Slot B */
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
                    if ((checkAddr < UDS_BL_F103_APP_SLOT_B_START) ||
                        ((checkAddr + checkLen) > (UDS_BL_F103_APP_SLOT_B_START + UDS_BL_F103_APP_SLOT_B_SIZE)))
                    {
                        result = uds_responseCode_RequestOutOfRange;
                    }
                    else
                    {
                        uint32_t computedCrc = calculateCrc32((const uint8_t *)(uintptr_t)checkAddr, checkLen);
                        hasStatusRecord = true;
                        if (computedCrc == expectedCrc)
                        {
                            statusRecord = 0x00u; /* 0x00: Verification Successful */
                            s_bootloaderMeta.crc32 = computedCrc;
                            s_bootloaderMeta.image_size = checkLen;
                            s_bootloaderMeta.status = (uint8_t)UDS_BL_SLOT_CANDIDATE;
                            s_bootloaderMeta.active_slot = 1u; /* Stage candidate in Slot B */
                        }
                        else
                        {
                            statusRecord = 0x01u; /* 0x01: Verification Failed */
                            s_bootloaderMeta.status = (uint8_t)UDS_BL_SLOT_INVALID;
                        }
                    }
                }
            }
            else if (subfunction == 0x03u) /* RequestRoutineResults */
            {
                hasStatusRecord = true;
                statusRecord = (s_bootloaderMeta.status == (uint8_t)UDS_BL_SLOT_CANDIDATE) ? 0x00u : 0x01u;
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



