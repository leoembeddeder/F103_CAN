#ifndef CHARON_ROUTINEFUNCTIONALUNIT_H_
#define CHARON_ROUTINEFUNCTIONALUNIT_H_

/* Includes ******************************************************************/

#include <stdint.h>
#include "uds_types.h"

/* Constants *****************************************************************/

/* Macros ********************************************************************/

/* Types *********************************************************************/

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

#endif 