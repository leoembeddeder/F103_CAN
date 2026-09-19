#ifndef CHARON_SERVICELOOKUPTABLE_H_
#define CHARON_SERVICELOOKUPTABLE_H_

/* Includes ******************************************************************/

#include <stdint.h>
#include <uds_types.h>

/* Constants *****************************************************************/

/* Macros ********************************************************************/
/** @brief Cast and shift charon_sscType_default for later usage in serviceLookupTable. */
#define SESSION_DEFAULT			((uint32_t) 1u << (uint8_t)charon_sscType_default)
/** @brief Cast and shift charon_sscType_programming for later usage in serviceLookupTable. */
#define SESSION_PROGRAMMING  	((uint32_t) 1u << (uint8_t)charon_sscType_programming)
/** @brief Cast and shift charon_sscType_extended for later usage in serviceLookupTable. */
#define SESSION_EXTENDED		((uint32_t) 1u << (uint8_t)charon_sscType_extended)
/** @brief Cast and shift charon_sscType_secured for later usage in serviceLookupTable. */
#define SESSION_SECURED			((uint32_t) 1u << (uint8_t)charon_sscType_secured)

/** @name Addressing Types */
#define ADDRESS_PHYSICAL        ((uint32_t) 1u << 0)
#define ADDRESS_FUNCTIONAL      ((uint32_t) 1u << 1)
#define ADDRESS_BOTH            (ADDRESS_PHYSICAL | ADDRESS_FUNCTIONAL)

/** @name Security Access Levels */
#define SECURITY_LOCKED         ((uint32_t) 1u << 0)
#define SECURITY_LEVEL_1        ((uint32_t) 1u << 1)
#define SECURITY_LEVEL_2        ((uint32_t) 1u << 2)
#define SECURITY_ALL            (SECURITY_LOCKED | SECURITY_LEVEL_1 | SECURITY_LEVEL_2)

/* Types *********************************************************************/

/** @brief Service Function Signature */
typedef uds_responseCode_t (*charonUdsFunctionSignature)(const uint8_t * pData, uint32_t length);

/** @brief Service Object Type. This describes all Attributes needed to check execution rights of a UDS Service.*/
typedef struct
{
    uds_sid_t                   sid;                /**< Service ID @ref uds_sid_t. */
    uint32_t                    sessionMask;        /**< Mask for current session. */
    charonUdsFunctionSignature  serviceRunable;     /**< Function called to run Service. */ 
    uint32_t                    emcryptionMask;     /**< Type of used encryption. */
    uint32_t                    addressMask;        /**< Mask for supported addressing modes. */
    uint32_t                    securityMask;       /**< Mask for required security levels. */
} charon_serviceObject_t;

/* Interfaces ****************************************************************/

/** @brief Function is used to get a Service id object out of ServiceLookupTable. 
 * 
 * @param sid Service ID to get from LookupTable.
 * @return @ref charon_serviceObject_t* 
 */
charon_serviceObject_t* charon_ServiceLookupTable_getServiceObject (uint8_t sid);

/** @brief Function is used to get the name of Service ID as string.
 * 
 * @param sid The SID to get the name from.
 * @return Name of SID as a string.
 */
const char * charon_ServiceLookupTable_getNameForServiceSid (uint8_t sid);

/** @brief Function is used to get the name of uds_responseCode_t as a string.
 * 
 * @param returnCode Return code to get the name from.
 * @return Name of ReturnCode as string.
 */
const char * charon_ServiceLookupTable_getNameForReturnCode (uds_responseCode_t returnCode);

#endif 