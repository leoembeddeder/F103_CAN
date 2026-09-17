#ifndef CHARON_NEGATIVERESPONSE_H_
#define CHARON_NEGATIVERESPONSE_H_

/* Includes ******************************************************************/
#include "uds_types.h"
#include <stdint.h>

/* Constants *****************************************************************/

/* Macros ********************************************************************/

/* Types *********************************************************************/

/* Interfaces ****************************************************************/

/** @brief The function is used to create a negative response of client request.
 * 
 * @param ResponseCode @ref uds_responseCode_t.
 * @param RequestedSid @ref uds_sid_t.
 */
void charon_sendNegativeResponse (uds_responseCode_t ResponseCode, uds_sid_t RequestedSid);

#endif 