#include "crc16.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


/* Imports *******************************************************************/

/* Constants *****************************************************************/

/* Macros ********************************************************************/

/* Types *********************************************************************/

/* Variables *****************************************************************/

/* Private Function Definitions **********************************************/

/* Interfaces  ***************************************************************/

crc16_t charon_crc16_reflect(crc16_t data, size_t data_len)
{
    uint32_t i;
    crc16_t ret;

    ret = data & 0x01;
    for (i = 1; i < data_len; i++) 
    {
        data >>= 1;
        ret = (ret << 1) | (data & 0x01);
    }
    return ret;
}

crc16_t charon_crc16_update(crc16_t crc, const void *data, size_t data_len)
{
    const unsigned char *d = (const unsigned char *)data;
    uint32_t i;
    bool bit;
    unsigned char c;

    while (data_len--) 
    {
        c = *d++;
        for (i = 0x01; i & 0xff; i <<= 1)
        {
            bit = crc & 0x8000;
            if (c & i)
            {
                bit = !bit;
            }
            crc <<= 1;
            if (bit) 
            {
                crc ^= 0x8005;
            }
        }
        crc &= 0xffff;
    }
    return crc & 0xffff;
}


