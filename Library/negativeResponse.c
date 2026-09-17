#include "negativeResponse.h"
#include "SessionAndServiceControl.h"
#include "ServiceLookupTable.h"
#include "interface_debug.h"



/* Interfaces  ***************************************************************/

void charon_sendNegativeResponse (uds_responseCode_t ResponseCode, uds_sid_t RequestedSid)
{
    CHARON_INFO("Sending error message %s (0x%x) for SID %s (0x%x)", charon_ServiceLookupTable_getNameForReturnCode(ResponseCode), (uint8_t)ResponseCode, charon_ServiceLookupTable_getNameForServiceSid((uint8_t)RequestedSid), (uint8_t)RequestedSid);
    uint8_t transmitBuffer[3] = {(uint8_t)uds_sid_NegativeResponse, (uint8_t)RequestedSid, (uint8_t)ResponseCode};
    charon_sscTxMessage(transmitBuffer, sizeof(transmitBuffer));
}


