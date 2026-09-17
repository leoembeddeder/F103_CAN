#ifndef CHARON_INTERFACE_DEBUG_H_
#define CHARON_INTERFACE_DEBUG_H_

/* Includes ******************************************************************/

#include "uds.h"

/* Constants *****************************************************************/

/* Macros ********************************************************************/

#if CHARON_CONFIG_LOG_MSG_OUT_AS_MACRO
#include <stdio.h>

// Add your Logging Macro here
#define CHARON_INFO(x, ...)     printf("[CHARON] " x, ##__VA_ARGS__)
#define CHARON_WARNING(x, ...)  printf("[CHARON] " x, ##__VA_ARGS__)
#define CHARON_ERROR(x, ...)    printf("[CHARON] " x, ##__VA_ARGS__)
#endif

#if CHARON_CONFIG_LOG_MSG_OUT_AS_FUNCTION
void CHARON_INFO (char *x, ...);
void CHARON_WARNING (char *x, ...);
void CHARON_ERROR (char *x, ...);
#endif



/* Types *********************************************************************/

/* Interfaces ****************************************************************/


#endif 