#ifndef CHARON_INTERFACE_CLOCK_H_
#define CHARON_INTERFACE_CLOCK_H_

/* Includes ******************************************************************/

#include <stdint.h>

/* Constants *****************************************************************/

/* Macros ********************************************************************/

/* Types *********************************************************************/

/* Interfaces ****************************************************************/

/** @brief Get Current System Time (preferable as Timestamp in ms).
 *
 * @return System Time.
 */
uint32_t charon_interface_clock_getTime(void);

/** @brief Get time difference from given System time to moment of call.
 *
 * @param timestamp Value given by charon_interface_clock_getTime.
 *
 * @return time elapsed to given timestamp (preferable in ms).
 */
uint32_t charon_interface_clock_getTimeElapsed(uint32_t timestamp);

#endif 