
#include "interface_clock.h"

#include "main.h"


uint32_t drvClocks_getRuntime(void)
{
    return HAL_GetTick();
}


uint32_t charon_interface_clock_getTime(void)
{
    return drvClocks_getRuntime();
}

uint32_t charon_interface_clock_getTimeElapsed(uint32_t timestamp)
{
    return drvClocks_getRuntime() - timestamp;
}
