#include "NvmEmulator.h"
#include "flash.h"
#include "interface_debug.h"
#include "UploadDownloadFunctionalUnit.h"
#include "SessionAndServiceControl.h"
#include "negativeResponse.h"
#include "ota_metadata.h"
#include "ota_metadata_mgr.h"
#include "DTC_LookupTable.h"
#include <string.h>


/* Imports *******************************************************************/

/* Constants *****************************************************************/

/* Macros ********************************************************************/

/**
 * UDS Payload Size, in this case defined by ISO TP
 * @{
 */
#define UDS_MAX_INPUT_FRAME_SIZE    4095u
#define UDS_MAX_OUTPUT_FRAME_SIZE   4095u
/**
 * @}
 */

/* Types *********************************************************************/

/**
 * Enumeration to describe the Transfer States
 */
typedef enum TransferDirection_t_private
{
    transfer_idle,           /**< No Transfer Ongoing */
    transfer_download,       /**< Transfer Client -> Server */
    transfer_upload          /**< Transfer Server -> Client */
} TransferDirection_t;


/* Variables *****************************************************************/

/** Current Transfer State */
static TransferDirection_t s_transferDirection = transfer_idle;
/** Stores the Memory Address used to send Data in Chunks */
static uint32_t s_currentMemoryAddress = 0uL;
/** Stores Transfer Amount remaining */
static uint32_t s_remainingMemoryLength = 0uL;
/** Stores Counter to Check Transfer amount and count */
static uint8_t s_nextSequenceCounter = 0uL;


/* Private Function Definitions **********************************************/

/**
 * Handle Transfers
 *
 * Checks on Transfer direction and initiates the Transfer of Data in
 * given direction.
 *
 * @param direction
 *      @see @ref TransferDirection_t
 * @param receiveBuffer
 *      Payload
 * @param receiveBufferSize
 *      Payload Length
 * @return @see @ref uds_responseCode_t
 */
static uds_responseCode_t requestTransfer(TransferDirection_t direction, const uint8_t * receiveBuffer, uint32_t receiveBufferSize);


/* Interfaces  ***************************************************************/

void charon_UploadDownloadFunctionalUnit_reset (void)
{
    s_transferDirection = transfer_idle;
    s_currentMemoryAddress = 0;
    s_remainingMemoryLength = 0;
    s_nextSequenceCounter = 0;
}

uds_responseCode_t charon_UploadDownloadFunctionalUnit_RequestDownload (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    return requestTransfer(transfer_download, receiveBuffer, receiveBufferSize);
}

uds_responseCode_t charon_UploadDownloadFunctionalUnit_RequestUpload (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    return requestTransfer(transfer_upload, receiveBuffer, receiveBufferSize);
}

uds_responseCode_t charon_UploadDownloadFunctionalUnit_TransferData (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    const PACKED_STRUCT(anonym) {
        uint8_t sid;
        uint8_t blockSequenceCounter;
        uint8_t data[UDS_MAX_INPUT_FRAME_SIZE];
    } * receivedMessage = (const void*)receiveBuffer;

    uds_responseCode_t result = uds_responseCode_PositiveResponse;

    if (receiveBufferSize > UDS_MAX_INPUT_FRAME_SIZE)
    {
        CHARON_ERROR("Frame is too long! Maximum size is %i.", UDS_MAX_INPUT_FRAME_SIZE);
        result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
    }
    else if (s_transferDirection == transfer_idle)
    {
        CHARON_ERROR("Transfer Data was not expected. Forgot to request upload/download?");
        result = uds_responseCode_RequestSequenceError;
    }
    else if (s_remainingMemoryLength < (receiveBufferSize - 2u) )
    {
        CHARON_ERROR("Too much data received!");
        result = uds_responseCode_TransferDataSuspended;
    }
    else if (receivedMessage->blockSequenceCounter != s_nextSequenceCounter)
    {
        CHARON_ERROR("Sequence number not expected! Received: %i, Expected: %i.", receivedMessage->blockSequenceCounter, s_nextSequenceCounter);
        result = uds_responseCode_WrongBlockSequenceCounter;
    }
    else
    {
        if (s_transferDirection == transfer_download)
        {
            uint32_t chunkLen = receiveBufferSize - 2u;
            if ((s_currentMemoryAddress >= STM32F103_FLASH_BASE) && 
                ((s_currentMemoryAddress + chunkLen) <= (STM32F103_FLASH_BASE + (256u * STM32F103_PAGE_SIZE))))
            {
                if (syn_port_flash_write(s_currentMemoryAddress, receivedMessage->data, chunkLen) == SYN_OK)
                {
                    result = uds_responseCode_PositiveResponse;
                }
                else
                {
                    result = uds_responseCode_GeneralProgrammingFailure;
                }
            }
            else
            {
                result = charon_NvmDriver_write(s_currentMemoryAddress, receivedMessage->data, chunkLen);
            }

            if (result == uds_responseCode_PositiveResponse)
            {
                s_currentMemoryAddress += chunkLen;
                s_remainingMemoryLength -= chunkLen;
                s_nextSequenceCounter++;

                uint8_t transmitBuffer[2] = {
                        receivedMessage->sid | (uint8_t)uds_sid_PositiveResponseMask,
                        receivedMessage->blockSequenceCounter
                };
                charon_sscTxMessage(transmitBuffer, sizeof(transmitBuffer));
            }
            else
            {
                CHARON_ERROR("Flash/NVM driver reported error while writing.");
            }
        }
        else
        {
            static uint8_t transmitBuffer[UDS_MAX_OUTPUT_FRAME_SIZE];
            uint32_t transmitBufferSize = sizeof(transmitBuffer);
            if (s_remainingMemoryLength < (transmitBufferSize - 2u) )
            {
                transmitBufferSize = s_remainingMemoryLength + 2u;
            }
            uint32_t readLen = transmitBufferSize - 2u;
            if ((s_currentMemoryAddress >= 0x20000000U) && (s_currentMemoryAddress < 0x20010000U))
            {
                memcpy(&(transmitBuffer[2]), (const void *)(uintptr_t)s_currentMemoryAddress, readLen);
            }
            else if ((s_currentMemoryAddress >= STM32F103_FLASH_BASE) && 
                ((s_currentMemoryAddress + readLen) <= (STM32F103_FLASH_BASE + (256u * STM32F103_PAGE_SIZE))))
            {
                syn_port_flash_read(s_currentMemoryAddress, &(transmitBuffer[2]), readLen);
            }
            else
            {
                charon_NvmDriver_read(s_currentMemoryAddress, &(transmitBuffer[2]), readLen);
            }
            s_currentMemoryAddress += readLen;
            s_remainingMemoryLength -= readLen;
            s_nextSequenceCounter++;

            transmitBuffer[0] = (uint8_t)uds_sid_TransferData | (uint8_t)uds_sid_PositiveResponseMask;
            transmitBuffer[1] = receivedMessage->blockSequenceCounter;
            charon_sscTxMessage(transmitBuffer, transmitBufferSize);
        }
    }
    if (result != uds_responseCode_PositiveResponse)
    {
        charon_sendNegativeResponse(result, uds_sid_TransferData);
    }
    return result;
}

uds_responseCode_t charon_UploadDownloadFunctionalUnit_RequestTransferExit (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    uds_responseCode_t result = uds_responseCode_PositiveResponse;
    (void) receiveBuffer;

    if (receiveBufferSize > 1u)
    {
        CHARON_ERROR("Unexpected message length.");
        result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
    }
    else if (s_remainingMemoryLength != 0u)
    {
        CHARON_ERROR("Transfer Exit received, but not all data was transferred.");
        result = uds_responseCode_RequestSequenceError;
    }
    else if (s_transferDirection == transfer_idle)
    {
        CHARON_ERROR("No transfer ongoing, cannot exit transfer.");
        result = uds_responseCode_RequestSequenceError;
    }
    else
    {
        s_currentMemoryAddress = 0;
        s_remainingMemoryLength = 0;
        s_nextSequenceCounter = 0;
        s_transferDirection = transfer_idle;

        CHARON_INFO("Exiting transfer mode.");
        uint8_t transmitBuffer[1] = {(uint8_t)uds_sid_RequestTransferExit | (uint8_t)uds_sid_PositiveResponseMask};
        charon_sscTxMessage(transmitBuffer, sizeof(transmitBuffer));
    }
    if (result != uds_responseCode_PositiveResponse)
    {
        charon_sendNegativeResponse(result, uds_sid_RequestTransferExit);
    }
    return result;
}

typedef struct {
    const char *path;
    uint32_t address;
    uint32_t size;
    bool writable;
} VirtualFile_t;

static const char s_dirManifest[] = "/dtc/log.bin\n/cal/params.bin\n/boot/slotA.bin\n/boot/slotB.bin\n";

static const VirtualFile_t s_vfs[] = {
    { "/dtc/log.bin",       0x0807A000U, 12288U,        true  },
    { "/cal/params.bin",    0x0807A000U, FLASH_PARAM_SIZE, true  },
    { "/boot/slotA.bin",    0x08010000U, 210U * 1024U,  true  },
    { "/boot/slotB.bin",    0x08044800U, 210U * 1024U,  true  },
};

static const VirtualFile_t* find_virtual_file(const char *name, uint16_t nameLen)
{
    for (size_t i = 0; i < sizeof(s_vfs)/sizeof(s_vfs[0]); i++)
    {
        if ((strlen(s_vfs[i].path) == nameLen) &&
            (strncmp(s_vfs[i].path, name, nameLen) == 0))
        {
            return &s_vfs[i];
        }
    }
    return NULL;
}

uds_responseCode_t charon_UploadDownloadFunctionalUnit_RequestFileTransfer (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    CHARON_INFO("Request File Transfer Service SID:0x38 Triggered");

    if ((receiveBuffer == NULL) || (receiveBufferSize < 4u))
    {
        charon_sendNegativeResponse(uds_responseCode_IncorrectMessageLengthOrInvalidFormat, uds_sid_RequestFileTransfer);
        return uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
    }

    uint8_t mode = receiveBuffer[1];
    uint16_t pathLen = ((uint16_t)receiveBuffer[2] << 8) | receiveBuffer[3];

    if (receiveBufferSize < (4u + pathLen))
    {
        charon_sendNegativeResponse(uds_responseCode_IncorrectMessageLengthOrInvalidFormat, uds_sid_RequestFileTransfer);
        return uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
    }

    const char *filePath = (const char *)&receiveBuffer[4];
    uds_responseCode_t result = uds_responseCode_PositiveResponse;
    uint8_t txBuffer[16];
    uint32_t txLength = 0u;

    switch (mode)
    {
    case 0x01: /* AddFile */
    case 0x03: /* ReplaceFile */
    {
        const VirtualFile_t *vf = find_virtual_file(filePath, pathLen);
        if ((vf == NULL) || (!vf->writable))
        {
            result = uds_responseCode_RequestOutOfRange;
            break;
        }
        s_currentMemoryAddress = vf->address;
        s_remainingMemoryLength = vf->size;
        s_nextSequenceCounter = 1u;
        s_transferDirection = transfer_download;

        txBuffer[0] = (uint8_t)uds_sid_RequestFileTransfer | (uint8_t)uds_sid_PositiveResponseMask;
        txBuffer[1] = mode;
        txBuffer[2] = 0x20u; /* lengthFormatIdentifier: 2-byte maxNumberOfBlockLength */
        txBuffer[3] = 0x00u;
        txBuffer[4] = 0x80u; /* maxNumberOfBlockLength = 128 bytes */
        txBuffer[5] = 0x00u; /* dataFormatIdentifier = uncompressed / unencrypted */
        txLength = 6u;
        break;
    }
    case 0x02: /* DeleteFile */
    {
        const VirtualFile_t *vf = find_virtual_file(filePath, pathLen);
        if (vf == NULL)
        {
            result = uds_responseCode_RequestOutOfRange;
            break;
        }
        if (strcmp(vf->path, "/dtc/log.bin") == 0)
        {
            charon_deleteDTC(0u, 0u, 0u, 0u, 0u, false, false, false, true);
        }
        txBuffer[0] = (uint8_t)uds_sid_RequestFileTransfer | (uint8_t)uds_sid_PositiveResponseMask;
        txBuffer[1] = mode;
        txLength = 2u;
        break;
    }
    case 0x04: /* ReadFile */
    {
        const VirtualFile_t *vf = find_virtual_file(filePath, pathLen);
        if (vf == NULL)
        {
            result = uds_responseCode_RequestOutOfRange;
            break;
        }
        s_currentMemoryAddress = vf->address;
        s_remainingMemoryLength = vf->size;
        s_nextSequenceCounter = 1u;
        s_transferDirection = transfer_upload;

        txBuffer[0] = (uint8_t)uds_sid_RequestFileTransfer | (uint8_t)uds_sid_PositiveResponseMask;
        txBuffer[1] = mode;
        txBuffer[2] = 0x20u; /* lengthFormatIdentifier */
        txBuffer[3] = 0x00u;
        txBuffer[4] = 0x80u; /* maxNumberOfBlockLength = 128 bytes */
        txBuffer[5] = 0x00u; /* dataFormatIdentifier */
        txBuffer[6] = 0x04u; /* fileSizeParameterLength = 4 bytes */
        txBuffer[7] = (uint8_t)((vf->size >> 24) & 0xFFu);
        txBuffer[8] = (uint8_t)((vf->size >> 16) & 0xFFu);
        txBuffer[9] = (uint8_t)((vf->size >> 8) & 0xFFu);
        txBuffer[10] = (uint8_t)(vf->size & 0xFFu);
        txLength = 11u;
        break;
    }
    case 0x05: /* ReadDir */
    {
        uint32_t manifestLen = (uint32_t)strlen(s_dirManifest);
        s_currentMemoryAddress = (uint32_t)(uintptr_t)s_dirManifest;
        s_remainingMemoryLength = manifestLen;
        s_nextSequenceCounter = 1u;
        s_transferDirection = transfer_upload;

        txBuffer[0] = (uint8_t)uds_sid_RequestFileTransfer | (uint8_t)uds_sid_PositiveResponseMask;
        txBuffer[1] = mode;
        txBuffer[2] = 0x20u;
        txBuffer[3] = 0x00u;
        txBuffer[4] = 0x80u;
        txBuffer[5] = 0x00u;
        txBuffer[6] = 0x02u; /* fileSizeParameterLength = 2 bytes */
        txBuffer[7] = (uint8_t)((manifestLen >> 8) & 0xFFu);
        txBuffer[8] = (uint8_t)(manifestLen & 0xFFu);
        txLength = 9u;
        break;
    }
    default:
        result = uds_responseCode_SubfunctionNotSupported;
        break;
    }

    if (result != uds_responseCode_PositiveResponse)
    {
        charon_sendNegativeResponse(result, uds_sid_RequestFileTransfer);
    }
    else
    {
        charon_sscTxMessage(txBuffer, txLength);
    }
    return result;
}

#ifdef TEST
void charon_UploadDownloadFunctionalUnit_setCurrentMemoryAddress (uint32_t newAddress)
{
    s_currentMemoryAddress = newAddress;
}
void charon_UploadDownloadFunctionalUnit_setRemainingMemoryLength (uint32_t newLength)
{
    s_remainingMemoryLength = newLength;
}
void charon_UploadDownloadFunctionalUnit_setTransferDirection (uint8_t newDirection)
{
    s_transferDirection = newDirection;
}
void charon_UploadDownloadFunctionalUnit_setNextSequenceCounter (uint8_t newCounter)
{
    s_nextSequenceCounter = newCounter;
}
uint32_t charon_UploadDownloadFunctionalUnit_getCurrentMemoryAddress (void)
{
    return s_currentMemoryAddress;
}
uint32_t charon_UploadDownloadFunctionalUnit_getRemainingMemoryLength (void)
{
    return s_remainingMemoryLength;
}
uint8_t charon_UploadDownloadFunctionalUnit_getTransferDirection (void)
{
    return s_transferDirection;
}
uint8_t charon_UploadDownloadFunctionalUnit_getNextSequenceCounter (void)
{
    return s_nextSequenceCounter;
}

#endif


/* Private Function **********************************************************/

static uds_responseCode_t requestTransfer(TransferDirection_t direction, const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    const PACKED_STRUCT(anonym) {
        uint8_t sid;
        uint8_t dataFormatIdentifier;
        uint8_t addressAndLengthFormatIdentifier;
        uint8_t AddressInformation[8];
    } * receivedMessage = (const void*)receiveBuffer;

    uds_responseCode_t result = uds_responseCode_PositiveResponse;

    uint8_t lengthOfMemoryLength = receivedMessage->addressAndLengthFormatIdentifier >> 4;
    uint8_t lengthOfMemoryAddress = receivedMessage->addressAndLengthFormatIdentifier & 0xFu;
    if (
            (lengthOfMemoryAddress == 0u) ||
            (lengthOfMemoryLength == 0u) ||
            (lengthOfMemoryAddress > 4u) ||
            (lengthOfMemoryLength > 4u) ||
            (receivedMessage->dataFormatIdentifier != 0x00u)
        )
    {
        CHARON_ERROR("Format Identifier invalid: size of address = %i, size of length = %i.", lengthOfMemoryAddress, lengthOfMemoryLength);
        result = uds_responseCode_RequestOutOfRange;
    }

    else if ( (((uint32_t)lengthOfMemoryAddress + lengthOfMemoryLength) + 3u) != receiveBufferSize )
    {
        CHARON_ERROR("Unexpected message length.");
        result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
    }

    else
    {
        uint32_t memoryAddress = 0;
        uint32_t memoryLength = 0;
        for (uint8_t i = 0; i<lengthOfMemoryAddress; i++)
        {
            memoryAddress |= (uint32_t)(receivedMessage->AddressInformation[lengthOfMemoryAddress - (i+1u)]) << (i*8u);
        }

        for (uint8_t i = 0; i<lengthOfMemoryLength; i++)
        {
            memoryLength |= (uint32_t)receivedMessage->AddressInformation[(lengthOfMemoryAddress + lengthOfMemoryLength) - (i+1u)] << (i*8u);
        }
        CHARON_INFO("Transfer Requested, address 0x%x, length 0x%x, direction %s.", memoryAddress, memoryLength, direction == transfer_download ? "download" : "upload");

        bool isValidRange = false;
        if (direction == transfer_download)
        {
            /* Firmware downloads strictly restricted to Slot A or Slot B */
            if ((memoryAddress >= SLOT_A_ADDR) && 
                ((memoryAddress + memoryLength) <= (SLOT_A_ADDR + SLOT_SIZE)))
            {
                isValidRange = true;
            }
            else if ((memoryAddress >= SLOT_B_ADDR) && 
                     ((memoryAddress + memoryLength) <= (SLOT_B_ADDR + SLOT_SIZE)))
            {
                isValidRange = true;
            }
            else if (charon_NvmDriver_checkAddressRange(memoryAddress, memoryLength))
            {
                isValidRange = true;
            }
        }
        else
        {
            if ((memoryAddress >= STM32F103_FLASH_BASE) && 
                ((memoryAddress + memoryLength) <= (STM32F103_FLASH_BASE + (256u * STM32F103_PAGE_SIZE))))
            {
                isValidRange = true;
            }
            else if (charon_NvmDriver_checkAddressRange(memoryAddress, memoryLength))
            {
                isValidRange = true;
            }
        }

        if (!isValidRange)
        {
            CHARON_ERROR("Requested memory is out of range.");
            result = uds_responseCode_RequestOutOfRange;
        }
        else if (s_transferDirection != transfer_idle)
        {
            CHARON_ERROR("A transfer is already ongoing.");
            result = uds_responseCode_ConditionsNotCorrect;
        }
        else
        {
            s_currentMemoryAddress = memoryAddress;
            s_remainingMemoryLength = memoryLength;
            s_transferDirection = direction;
            s_nextSequenceCounter = 1;
            uint8_t transmitBuffer[4] = {
                    receivedMessage->sid | (uint8_t)uds_sid_PositiveResponseMask,
                    0x20,
                    (uint8_t)((UDS_MAX_INPUT_FRAME_SIZE >> 8u) & 0xFFu),
                    (uint8_t)(UDS_MAX_INPUT_FRAME_SIZE & 0xFFu)
            };
            charon_sscTxMessage(transmitBuffer, sizeof(transmitBuffer));
        }

    }
    if (result != uds_responseCode_PositiveResponse)
    {
        uds_sid_t receivedSid;
        if (direction == transfer_download)
        {
            receivedSid = uds_sid_RequestDownload;
        }
        else
        {
            receivedSid = uds_sid_RequestUpload;
        }
        charon_sendNegativeResponse(result, receivedSid);
    }
    return result;
}


