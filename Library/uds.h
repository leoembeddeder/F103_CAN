#ifndef CHARON_H_
#define CHARON_H_

/* Includes ******************************************************************/

#include <stdint.h>
#include "isotp_socket.h"
#include "uds_config.h"

/* Constants *****************************************************************/

/* Macros ********************************************************************/

/* Types *********************************************************************/

/* Interfaces ****************************************************************/

/** @brief Function resets all dependent charon server processes */
void charon_reset (void);

/** @brief The function initializes charon sever.
 * 
 * @param systemCommunicationSocket Socket to be initialized @ref ISocket_t.
 */
void charon_init (ISocket_t systemCommunicationSocket);

/** @brief Task can be cyclic called to fullfil its duty to respond to client requests. */
void charon_task (void);

#endif 