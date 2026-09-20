#ifndef CHARON_SERVICEANDSESSIONCONTROL_H_
#define CHARON_SERVICEANDSESSIONCONTROL_H_

/* Includes ******************************************************************/

#include <stdint.h>
#include "isotp_socket.h"
#include "main.h"
/* Constants *****************************************************************/

/* Macros ********************************************************************/

/* Types *********************************************************************/

/** @brief Session Types */
typedef enum
{
    charon_sscType_invalid,
    charon_sscType_default,         /**< charon_sscType_default */
    charon_sscType_programming,     /**< charon_sscType_programming */
    charon_sscType_extended,        /**< charon_sscType_extended */
    charon_sscType_secured,         /**< charon_sscType_secured */

    charon_sscType_amount,          /**< The amount of valid session IDs */
    charon_sscType_notStarted,      /**< There is no active session with a client */
    charon_sscType_timedOut         /**< A session has timed out, a library wide reset is necessary */
} charon_sessionTypes_t;

/* Interfaces ****************************************************************/

/** @brief The function resets all Session and Service Control involved Parameters to default. */
void charon_sscReset (void);

/** @brief The function initializes the System given Communication Socket.
 * 
 * @param sscComSocket Abstract Hardware socked that should be used to initializes Communication Socket.
 */
void charon_sscInit (ISocket_t sscComSocket);

/** @brief This function checks if current session is timed out and checks if the current session needs to be exceeded. */
void charon_sscCyclic(void);

/** @brief The function handles incoming messages and process them. */
void charon_sscRcvMessage (void);

/** @brief Function is used to send data to client. 
 * 
 * @param pBuffer Contains data to send from Server to Client.
 * @param length Size of pBuffer in bytes
 */
void charon_sscTxMessage (uint8_t const * const pBuffer, uint32_t length);

/** @brief This function is used to change the current Session.  
 * 
 * @param sessionType Session to set to @ref charon_sessionTypes_t.
 * @param timeoutP2 Timeout for the initial answer @ref ComTimeoutLimits_t.
 * @param timeoutP2extended Timeout for the extended answer @ref ComTimeoutLimits_t.
 */
void charon_sscSetSession (charon_sessionTypes_t sessionType, uint32_t timeoutP2, uint32_t timeoutP2extended);

/** @brief Function to get current Session. 
 * 
 * @return @ref charon_sessionTypes_t. 
 */
charon_sessionTypes_t charon_sscGetSession (void);

/** @brief Function checks timestamp for tester present heartbeat. */
void charon_sscTesterPresentHeartbeat(void);

void on_ecu_reset(uint8_t reset_type);

/** @brief Set current addressing mode (ADDRESS_PHYSICAL, ADDRESS_FUNCTIONAL). */
void charon_sscSetAddressingMode (uint32_t addressingMode);

/** @brief Get current addressing mode. */
uint32_t charon_sscGetAddressingMode (void);

/** @brief Set active security level bitmask (SECURITY_LOCKED, SECURITY_LEVEL_1, SECURITY_LEVEL_2). */
void charon_sscSetSecurityLevel (uint32_t securityLevel);

/** @brief Get active security level bitmask. */
uint32_t charon_sscGetSecurityLevel (void);

/** @brief Reset security level to SECURITY_LOCKED. */
void charon_sscResetSecurityLevel (void);

/** @brief Set active encryption level mask. */
void charon_sscSetEncryptionLevel (uint32_t encryptionLevel);

/** @brief Get active encryption level mask. */
uint32_t charon_sscGetEncryptionLevel (void);

#endif 