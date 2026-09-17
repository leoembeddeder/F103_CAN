#ifndef CHARON_UPLOADDOWNLOADFUNCTIONALUNIT_H_
#define CHARON_UPLOADDOWNLOADFUNCTIONALUNIT_H_

/* Includes ******************************************************************/

#include <stdint.h>
#include "uds_types.h"

/* Constants *****************************************************************/

/* Macros ********************************************************************/

/* Types *********************************************************************/

/* Interfaces ****************************************************************/

/** @brief Resets all internal Variables and stops all ongoing Services. */
void charon_UploadDownloadFunctionalUnit_reset (void);

/** @brief The service is used by the client to initiate a data transfer from the client to the server (download).
 * UDS ISO 14229-1 SE 2013-03-15
 * SID: 0x34
 *
 * @param receiveBuffer Payload
 * @param receiveBufferSize Payload Size
 * @return @see @ref uds_responseCode_t
 */
uds_responseCode_t charon_UploadDownloadFunctionalUnit_RequestDownload (const uint8_t * receiveBuffer, uint32_t receiveBufferSize);

/** @brief The service is used by the client to initiate a data transfer from the server to the client (upload).
 * UDS ISO 14229-1 SE 2013-03-15
 * SID: 0x35
 *
 * @param receiveBuffer Payload
 * @param receiveBufferSize Payload Size
 * @return @see @ref uds_responseCode_t
 */
uds_responseCode_t charon_UploadDownloadFunctionalUnit_RequestUpload (const uint8_t * receiveBuffer, uint32_t receiveBufferSize);

/** @brief The service is used by the client to transfer data either from the client to the server (download) or from the server to the client (upload).
 * UDS ISO 14229-1 SE 2013-03-15
 * SID: 0x36
 *
 * @param receiveBuffer Payload
 * @param receiveBufferSize Payload Size
 * @return @see @ref uds_responseCode_t
 */
uds_responseCode_t charon_UploadDownloadFunctionalUnit_TransferData (const uint8_t * receiveBuffer, uint32_t receiveBufferSize);

/** @brief The service is used by the client to terminate a data transfer between client and server (upload or download). 
 * UDS ISO 14229-1 SE 2013-03-15
 * SID: 0x37
 *
 * @param receiveBuffer Payload
 * @param receiveBufferSize Payload Size
 * @return @see @ref uds_responseCode_t
 */
uds_responseCode_t charon_UploadDownloadFunctionalUnit_RequestTransferExit (const uint8_t * receiveBuffer, uint32_t receiveBufferSize);

/** @brief The service is used by the client to initiate a file data transfer from either the client 
 *         to the server or from the server to the client (download or upload).
 * UDS ISO 14229-1 SE 2013-03-15
 * SID: 0x38
 *
 * @param receiveBuffer Payload
 * @param receiveBufferSize Payload Size
 * @return @see @ref uds_responseCode_t
 */
uds_responseCode_t charon_UploadDownloadFunctionalUnit_RequestFileTransfer (const uint8_t * receiveBuffer, uint32_t receiveBufferSize);

#ifdef TEST

void charon_UploadDownloadFunctionalUnit_setCurrentMemoryAddress (uint32_t newAddress);

void charon_UploadDownloadFunctionalUnit_setRemainingMemoryLength (uint32_t newLength);

void charon_UploadDownloadFunctionalUnit_setTransferDirection (uint8_t newDirection);

void charon_UploadDownloadFunctionalUnit_setNextSequenceCounter (uint8_t newCounter);

uint32_t charon_UploadDownloadFunctionalUnit_getCurrentMemoryAddress (void);

uint32_t charon_UploadDownloadFunctionalUnit_getRemainingMemoryLength (void);

uint8_t charon_UploadDownloadFunctionalUnit_getTransferDirection (void);

uint8_t charon_UploadDownloadFunctionalUnit_getNextSequenceCounter (void);

#endif

#endif 