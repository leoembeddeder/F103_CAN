#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "DiagnosticAndCommunicationManagementFunctionalUnit.h"
#include "SessionAndServiceControl.h"
#include "interface_clock.h"
#include "interface_debug.h"
#include "negativeResponse.h"
#include "ServiceLookupTable.h"
#include "StoredDataTransmissionFunctionalUnit.h"
#include "aes.h"
#include "aes_cmac.h"


/* Imports *******************************************************************/

/* Macros ********************************************************************/
#define SECURITY_LEVEL1_MASK_BYTE0 0xA5U
#define SECURITY_LEVEL1_MASK_BYTE1 0x5AU
#define SECURITY_LEVEL1_MASK_BYTE2 0xC3U
#define SECURITY_LEVEL1_MASK_BYTE3 0x3CU

#define SECURITY_MAX_FAILED_ATTEMPTS 3U
#define SECURITY_LOCKOUT_DELAY_MS    10000U /* 10 seconds */

/* Types *********************************************************************/
uds_reset_t ecu_reset;

/**
 * Container Type for Timings
 */
typedef struct DefaultSessionTimings_t_private
{
    uint32_t p2;            /**< Response Timeout */
    uint32_t p2star;        /**< Extended Response Timing */
} DefaultSessionTimings_t;

/**
 * @brief Container Type for states of DTCSetting
 */
typedef enum DTCSettingType_t_private
{
    ON              = 0x01,
    OFF             = 0x02,
    // Vehicle manufacturer specific: 0x40 – 0x5F.


    // System supplier specific: 0x60 – 0x7E.


    amountOfDTCSettingType

}DTCSettingType_t;


/* Constants *****************************************************************/

/**
 * Default timings for each session.
 * Sorted for their session id. Values are in milliseconds, for both parameters.
 *
 * @todo: these timing values are example values from iso 14229-2 chapter 7.2 table 4
 * change these as necessary 
 */
static const DefaultSessionTimings_t defaultTimings[charon_sscType_amount] =
{
        {50,5000},      /* Default */
        {50,5000},      /* Programming */
        {50,5000},      /* Extended */
        {50,5000}       /* Security */
};


/* Variables *****************************************************************/
static uint8_t s_masterKey[16] = {
    0x2BU, 0x7EU, 0x15U, 0x16U, 0x28U, 0xAEU, 0xD2U, 0xA6U,
    0xABU, 0xF7U, 0x15U, 0x88U, 0x09U, 0xCFU, 0x4FU, 0x3CU
};

static uint8_t s_level1Seed[4] = {0u};
static bool s_level1SeedValid = false;

static uint8_t s_level2Seed[16] = {0u};
static bool s_level2SeedValid = false;

static uint8_t s_failedAttemptCount = 0u;
static uint32_t s_lockoutStartTime = 0u;
static bool s_lockoutActive = false;

/* Private Function Definitions **********************************************/
void charon_SecurityAccess_SetMasterKey(const uint8_t key[16])
{
    if (key != NULL)
    {
        (void)memcpy(s_masterKey, key, 16u);
    }
}

void charon_SecurityAccess_InvalidateSeeds(void)
{
    s_level1SeedValid = false;
    s_level2SeedValid = false;
    (void)memset(s_level1Seed, 0, sizeof(s_level1Seed));
    (void)memset(s_level2Seed, 0, sizeof(s_level2Seed));
}

static bool isSecurityLockedOut(void)
{
    if (s_lockoutActive)
    {
        if (charon_interface_clock_getTimeElapsed(s_lockoutStartTime) >= SECURITY_LOCKOUT_DELAY_MS)
        {
            s_lockoutActive = false;
            s_failedAttemptCount = 0u;
            return false;
        }
        return true;
    }
    return false;
}

static void generatePseudorandomBytes(uint8_t *pOutput, uint32_t length)
{
    static uint32_t s_seedCounter = 0x12345678u;
    uint32_t now = charon_interface_clock_getTime();
    s_seedCounter ^= (now + 0x9E3779B9u);

    for (uint32_t i = 0u; i < length; ++i)
    {
        s_seedCounter = s_seedCounter * 1103515245u + 12345u;
        pOutput[i] = (uint8_t)(s_seedCounter >> 16u) ^ (uint8_t)(now >> (i & 0x07u));
    }
    bool allZero = true;
    for (uint32_t i = 0u; i < length; ++i)
    {
        if (pOutput[i] != 0u)
        {
            allZero = false;
            break;
        }
    }
    if (allZero)
    {
        pOutput[0] = 0x5Au;
        pOutput[length - 1u] = 0xA5u;
    }
}

static bool constantTimeEqual4(const uint8_t left[4], const uint8_t right[4])
{
    uint8_t diff = 0u;
    for (uint8_t i = 0u; i < 4u; ++i)
    {
        diff |= (uint8_t)(left[i] ^ right[i]);
    }
    return (diff == 0u);
}

static bool isValidSessionTransition(charon_sessionTypes_t current, uint8_t target)
{
    if ((uint8_t)current == target)
    {
        return true;
    }

    switch (current)
    {
    case charon_sscType_default:
        if ((target == (uint8_t)charon_sscType_extended) ||
            (target == (uint8_t)charon_sscType_secured))
        {
            return true;
        }
        /* Direct transition Default -> Programming is forbidden (NRC 0x22) */
        return false;

    case charon_sscType_extended:
        if ((target == (uint8_t)charon_sscType_default) ||
            (target == (uint8_t)charon_sscType_programming) ||
            (target == (uint8_t)charon_sscType_secured))
        {
            return true;
        }
        return false;

    case charon_sscType_programming:
        if (target == (uint8_t)charon_sscType_default)
        {
            return true;
        }
        /* Programming directly to Extended is forbidden */
        return false;

    case charon_sscType_secured:
        if ((target == (uint8_t)charon_sscType_default) ||
            (target == (uint8_t)charon_sscType_extended))
        {
            return true;
        }
        return false;

    default:
        return (target == (uint8_t)charon_sscType_default);
    }
}

/* Interfaces  ***************************************************************/

void charon_DiagnosticAndCommunicationManagementFunctionalUnit_reset (void)
{
    charon_SecurityAccess_InvalidateSeeds();
    s_failedAttemptCount = 0u;
    s_lockoutActive = false;
    s_lockoutStartTime = 0u;
}


uds_responseCode_t charon_DiagnosticAndCommunicationManagementFunctionalUnit_DiagnosticSessionControl (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    uds_responseCode_t result = uds_responseCode_PositiveResponse;
    uint8_t transmitBuffer[6];

    if (receiveBufferSize != 2u)
    {
        CHARON_ERROR("Unexpected message length.");
        result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
    }
    else
    {
        uint8_t session = receiveBuffer[1] & 0x7Fu;
        uint8_t responseSuppress = receiveBuffer[1] & 0x80u;
        if ( (session >= (uint8_t)charon_sscType_amount) || (session == (uint8_t)charon_sscType_invalid) )
        {
            CHARON_WARNING("Session 0x%x unknown or invalid!", session);
            result = uds_responseCode_SubfunctionNotSupported;
        }
		else if (!isValidSessionTransition(charon_sscGetSession(), session))
        {
            CHARON_WARNING("Invalid session transition from 0x%x to 0x%x!", (uint8_t)charon_sscGetSession(), session);
            result = uds_responseCode_ConditionsNotCorrect;
        }
        else
        {
            CHARON_INFO("Changing Session to 0x%x.", session);

            if (responseSuppress == 0u)
            {
				transmitBuffer[0] = (uint8_t)uds_sid_DiagnosticSessionControl | (uint8_t)uds_sid_PositiveResponseMask;
                transmitBuffer[1] = session;
                transmitBuffer[2] = (uint8_t)(defaultTimings[session].p2 >> 8u);
                transmitBuffer[3] = (uint8_t)(defaultTimings[session].p2 & 0xFFu);
                transmitBuffer[4] = (uint8_t)((defaultTimings[session].p2star / 10u) >> 8u);
                transmitBuffer[5] = (uint8_t)((defaultTimings[session].p2star / 10u) & 0xFFu);
                charon_sscTxMessage(transmitBuffer, sizeof(transmitBuffer));
            }
            charon_sscSetSession((charon_sessionTypes_t)session, defaultTimings[session].p2, defaultTimings[session].p2star);
			charon_SecurityAccess_InvalidateSeeds();
            charon_sscResetSecurityLevel();
        }
    }
    if (result != uds_responseCode_PositiveResponse)
    {
        charon_sendNegativeResponse(result, uds_sid_DiagnosticSessionControl);
    }
    return result;
}

uds_responseCode_t charon_DiagnosticAndCommunicationManagementFunctionalUnit_EcuReset (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    static uint8_t s_buffer[2];
    uint8_t sub;

    if ((receiveBuffer == NULL) || (receiveBufferSize != 2u))
    {
        CHARON_ERROR("Unexpected message length.");
        charon_sendNegativeResponse(
            uds_responseCode_IncorrectMessageLengthOrInvalidFormat,
            uds_sid_EcuReset);
        return uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
    }

    sub = receiveBuffer[1] & 0x7Fu;
    if ((sub < RESET_HARD) ||
        (sub > RESET_DISABLE_RAPID_POWER_SHUTDOWN))
    {
        charon_sendNegativeResponse(
            uds_responseCode_SubfunctionNotSupported,
            uds_sid_EcuReset);
        return uds_responseCode_SubfunctionNotSupported;
    }

    ecu_reset.reset_type_requested = sub;
    ecu_reset.reset_wait_elapsed_ms = charon_interface_clock_getTime();

    if ((receiveBuffer[1] & 0x80u) == 0u)
    {
        s_buffer[0] = (uint8_t)uds_sid_EcuReset | (uint8_t)uds_sid_PositiveResponseMask;
        s_buffer[1] = sub;
        charon_sscTxMessage(s_buffer, sizeof(s_buffer));
    }

    CHARON_INFO("ECU Reset Service SID:0x11 Triggered");
    return uds_responseCode_ServiceNotSupported;
}

uds_responseCode_t charon_DiagnosticAndCommunicationManagementFunctionalUnit_SecurityAccess (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
	uds_responseCode_t result = uds_responseCode_PositiveResponse;

    if ((receiveBuffer == NULL) || (receiveBufferSize < 2u))
    {
        CHARON_ERROR("Security Access invalid length.");
        result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
    }
    else
    {
        uint8_t subfunction = receiveBuffer[1] & 0x7Fu;
        uint8_t responseSuppress = receiveBuffer[1] & 0x80u;

        if (isSecurityLockedOut())
        {
            CHARON_WARNING("Security Access locked out, delay not expired.");
            result = uds_responseCode_RequiredTimeDelayNotExpired;
        }
        else
        {
            switch (subfunction)
            {
            case 0x01: /* Request Seed Level 1 */
            {
                if (receiveBufferSize != 6u)
                {
                    result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
                }
                else
                {
                    uint8_t txBuf[6];
                    txBuf[0] = (uint8_t)uds_sid_SecurityAccess | (uint8_t)uds_sid_PositiveResponseMask;
                    txBuf[1] = 0x01u;

                    if ((charon_sscGetSecurityLevel() & SECURITY_LEVEL_1) != 0u)
                    {
                        /* Already unlocked: return 4 zero bytes */
                        txBuf[2] = 0u;
                        txBuf[3] = 0u;
                        txBuf[4] = 0u;
                        txBuf[5] = 0u;
                        s_level1SeedValid = false;
                    }
                    else
                    {
                        generatePseudorandomBytes(s_level1Seed, 4u);
                        s_level1SeedValid = true;
                        (void)memcpy(&txBuf[2], s_level1Seed, 4u);
                    }
                    if (responseSuppress == 0u)
                    {
                        charon_sscTxMessage(txBuf, sizeof(txBuf));
                    }
                }
                break;
            }

            case 0x02: /* Send Key Level 1 */
            {
                if (receiveBufferSize != 6u)
                {
                    result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
                }
                else if (!s_level1SeedValid)
                {
                    CHARON_WARNING("Security Level 1 sequence error.");
                    result = uds_responseCode_RequestSequenceError;
                }
                else
                {
                    uint8_t expectedKey[4];
                    expectedKey[0] = (uint8_t)(s_level1Seed[0] ^ SECURITY_LEVEL1_MASK_BYTE0);
                    expectedKey[1] = (uint8_t)(s_level1Seed[1] ^ SECURITY_LEVEL1_MASK_BYTE1);
                    expectedKey[2] = (uint8_t)(s_level1Seed[2] ^ SECURITY_LEVEL1_MASK_BYTE2);
                    expectedKey[3] = (uint8_t)(s_level1Seed[3] ^ SECURITY_LEVEL1_MASK_BYTE3);
                    s_level1SeedValid = false;

                    if (constantTimeEqual4(&receiveBuffer[2], expectedKey))
                    {
                        CHARON_INFO("Security Level 1 Unlocked.");
                        s_failedAttemptCount = 0u;
                        charon_sscSetSecurityLevel(charon_sscGetSecurityLevel() | SECURITY_LEVEL_1);
                        if (responseSuppress == 0u)
                        {
                            uint8_t txBuf[2] = {
                                (uint8_t)uds_sid_SecurityAccess | (uint8_t)uds_sid_PositiveResponseMask,
                                0x02u
                            };
                            charon_sscTxMessage(txBuf, sizeof(txBuf));
                        }
                    }
                    else
                    {
                        s_failedAttemptCount++;
                        CHARON_WARNING("Security Level 1 invalid key! Attempt %u/%u", s_failedAttemptCount, SECURITY_MAX_FAILED_ATTEMPTS);
                        if (s_failedAttemptCount >= SECURITY_MAX_FAILED_ATTEMPTS)
                        {
                            s_lockoutActive = true;
                            s_lockoutStartTime = charon_interface_clock_getTime();
                            result = uds_responseCode_ExceededNumberOfAttempts;
                        }
                        else
                        {
                            result = uds_responseCode_InvalidKey;
                        }
                    }
                }
                break;
            }

            case 0x03: /* Request Seed Level 2 */
            {
                if (receiveBufferSize != 18u)
                {
                    result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
                }
                else
                {
                    uint8_t txBuf[18];
                    txBuf[0] = (uint8_t)uds_sid_SecurityAccess | (uint8_t)uds_sid_PositiveResponseMask;
                    txBuf[1] = 0x03u;

                    if ((charon_sscGetSecurityLevel() & SECURITY_LEVEL_2) != 0u)
                    {
                        /* Already unlocked: return 16 zero bytes */
                        (void)memset(&txBuf[2], 0, 16u);
                        s_level2SeedValid = false;
                    }
                    else
                    {
                        generatePseudorandomBytes(s_level2Seed, 16u);
                        s_level2SeedValid = true;
                        (void)memcpy(&txBuf[2], s_level2Seed, 16u);
                    }
                    if (responseSuppress == 0u)
                    {
                        charon_sscTxMessage(txBuf, sizeof(txBuf));
                    }
                }
                break;
            }

            case 0x04: /* Send Key Level 2 (AES-CMAC128) */
            {
                if (receiveBufferSize != 18u)
                {
                    result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
                }
                else if (!s_level2SeedValid)
                {
                    CHARON_WARNING("Security Level 2 sequence error.");
                    result = uds_responseCode_RequestSequenceError;
                }
                else
                {
                    uint8_t expectedKey[16];
                    bool cmacOk = aes_cmac_128(s_masterKey, s_level2Seed, 16u, expectedKey);
                    s_level2SeedValid = false;

                    if (cmacOk && aes_cmac_constant_time_equal(&receiveBuffer[2], expectedKey))
                    {
                        CHARON_INFO("Security Level 2 Unlocked.");
                        s_failedAttemptCount = 0u;
                        charon_sscSetSecurityLevel(charon_sscGetSecurityLevel() | SECURITY_LEVEL_2);
                        if (responseSuppress == 0u)
                        {
                            uint8_t txBuf[2] = {
                                (uint8_t)uds_sid_SecurityAccess | (uint8_t)uds_sid_PositiveResponseMask,
                                0x04u
                            };
                            charon_sscTxMessage(txBuf, sizeof(txBuf));
                        }
                    }
                    else
                    {
                        s_failedAttemptCount++;
                        CHARON_WARNING("Security Level 2 invalid key! Attempt %u/%u", s_failedAttemptCount, SECURITY_MAX_FAILED_ATTEMPTS);
                        if (s_failedAttemptCount >= SECURITY_MAX_FAILED_ATTEMPTS)
                        {
                            s_lockoutActive = true;
                            s_lockoutStartTime = charon_interface_clock_getTime();
                            result = uds_responseCode_ExceededNumberOfAttempts;
                        }
                        else
                        {
                            result = uds_responseCode_InvalidKey;
                        }
                    }
                }
                break;
            }

            default:
                CHARON_WARNING("Unsupported Security Access subfunction: 0x%x", subfunction);
                result = uds_responseCode_SubfunctionNotSupported;
                break;
            }
        }
    }

    if (result != uds_responseCode_PositiveResponse)
    {
        charon_sendNegativeResponse(result, uds_sid_SecurityAccess);
    }
    return result;
}

uds_responseCode_t charon_DiagnosticAndCommunicationManagementFunctionalUnit_CommunicationControl (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    (void)receiveBuffer;
    (void)receiveBufferSize;
    CHARON_INFO("Com Control Service SID:0x28 Triggered");
    return uds_responseCode_ServiceNotSupported;
}

uds_responseCode_t charon_DiagnosticAndCommunicationManagementFunctionalUnit_TesterPresent (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    uds_responseCode_t result = uds_responseCode_PositiveResponse;

    if (receiveBufferSize != 2u)
    {
        CHARON_ERROR("Unexpected message length.");
        result = uds_responseCode_IncorrectMessageLengthOrInvalidFormat;
    }
    else if ( (receiveBuffer[1] & 0x7Fu) != 0u)
    {
        CHARON_ERROR("Subfunction is not 0x00 or 0x80.");
        result = uds_responseCode_SubfunctionNotSupported;
    }
    else
    {
        charon_sscTesterPresentHeartbeat();
        if ( (receiveBuffer[1] & 0x80u) == 0u)
        {
            uint8_t transmitBuffer[2] = {(uint8_t)uds_sid_TesterPresent | (uint8_t)uds_sid_PositiveResponseMask, 0u};
            charon_sscTxMessage(transmitBuffer, sizeof(transmitBuffer));
        }
    }

    if (result != uds_responseCode_PositiveResponse)
    {
        charon_sendNegativeResponse(result, uds_sid_TesterPresent);
    }

    CHARON_INFO("Tester Present Service SID:0x3E Triggered");
    return result;
}

uds_responseCode_t charon_DiagnosticAndCommunicationManagementFunctionalUnit_AccessTimingParameter (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    (void)receiveBuffer;
    (void)receiveBufferSize;
    CHARON_INFO("Access Timing Parameter Service SID:0x83 Triggered");
    return uds_responseCode_ServiceNotSupported;
}

uds_responseCode_t charon_DiagnosticAndCommunicationManagementFunctionalUnit_SecuredDataTransmission (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    (void)receiveBuffer;
    (void)receiveBufferSize;
    CHARON_INFO("Secured Data Transmission Service SID:0x84 Triggered");
    return uds_responseCode_ServiceNotSupported;
}

uds_responseCode_t charon_DiagnosticAndCommunicationManagementFunctionalUnit_ControlDtcSetting (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    static uint8_t s_buffer[2];
    uint32_t length = 2u;
    uint8_t wantedChange;
    DTC_t DTC_update;
    uint16_t amountOfDTCToChange;

    CHARON_INFO("Control DTC Setting Service SID:0x85 Triggered");

    // Session check.
    charon_sessionTypes_t currentSession = charon_sscGetSession();
    if ((currentSession == SESSION_DEFAULT) || (currentSession == SESSION_PROGRAMMING))
    {
        // In current session not allowed.
        charon_sendNegativeResponse(uds_responseCode_ServiceNotSupportedInActiveSession, uds_sid_ControlDtcSetting);
        CHARON_ERROR("Service in current session not allowed.");
        return uds_responseCode_ServiceNotSupportedInActiveSession;
    }
    
    /** @todo check server condition, if server is in critical normal mode = send negative response. (ISO 14229-1 S71)*/
    if (false)
    {
        // Server is in critical normal mode activity.
        charon_sendNegativeResponse(uds_responseCode_ConditionsNotCorrect, uds_sid_ControlDtcSetting);
        CHARON_ERROR("Server is in critical normal mode.");
        return uds_responseCode_ConditionsNotCorrect;
    }


    // Going trough the complete receiveBuffer and building the package to change requested DTC.
    wantedChange = receiveBuffer[1];
    amountOfDTCToChange = (receiveBufferSize - 2u);
    CHARON_INFO("SettingType updating started.");
    for (uint32_t i = 0; i < amountOfDTCToChange; i+=3)
    {
        DTC_update.DTCSettingType = wantedChange;
        DTC_update.DTCHighByte = receiveBuffer[i + 2];
        DTC_update.DTCMiddleByte = receiveBuffer[i + 3];
        DTC_update.DTCLowByte = receiveBuffer[i + 4];
        charon_StoredDataTransmissionFunctionalUnit_DTCSettingTypeUpdater(DTC_update);
    }


    // Building response buffer.
    s_buffer[0] = (uint8_t)uds_sid_ControlDtcSetting | (uint8_t)uds_sid_PositiveResponseMask;
    s_buffer[1] = receiveBuffer[1]; // Even "off" on "off" request a positive response and the requested input shall be given out.

    charon_sscTxMessage(s_buffer,length);
    CHARON_INFO("SettingType's are now updated.");
    return uds_responseCode_PositiveResponse;
}

uds_responseCode_t charon_DiagnosticAndCommunicationManagementFunctionalUnit_ResponseOnEvent (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    (void)receiveBuffer;
    (void)receiveBufferSize;
    CHARON_INFO("Response On Event Service SID:0x86 Triggered");
    return uds_responseCode_ServiceNotSupported;
}

uds_responseCode_t charon_DiagnosticAndCommunicationManagementFunctionalUnit_LinkControl (const uint8_t * receiveBuffer, uint32_t receiveBufferSize)
{
    (void)receiveBuffer;
    (void)receiveBufferSize;
    CHARON_INFO("Link Control Service SID:0x87 Triggered");
    return uds_responseCode_ServiceNotSupported;
}

