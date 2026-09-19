
#include "interface_clock.h"

#include "main.h"


uint32_t charon_interface_clock_getTime(void)
{
    return GetTickMS();
}

uint32_t charon_interface_clock_getTimeElapsed(uint32_t timestamp)
{
    return GetTickMS() - timestamp;
}
