#include "isotp_user.h"
#include "isotp_defines.h"
#include "isotp_socket.h"
#include "can.h"


void isotp_user_debug(const char* message, ...)
{
    (void) message;
    return;
}



int isotp_user_send_can(const uint32_t arbitration_id, const uint8_t* data, const uint8_t size
#ifdef ISO_TP_USER_SEND_CAN_FLAGS
                        , const uint8_t flags
#endif
#ifdef ISO_TP_USER_SEND_CAN_ARG
                        , void* arg
#endif
){ 
#ifdef ISO_TP_USER_SEND_CAN_FLAGS
    (void)flags;
#endif
#ifdef ISO_TP_USER_SEND_CAN_ARG
    (void)arg;
#endif

    if (CAN_TX(arbitration_id, (uint8_t*)data, size) != HAL_OK)
    {
        TpRecordCanTxError();
        return ISOTP_RET_ERROR;
    }

    return ISOTP_RET_OK;
}

uint32_t isotp_user_get_us(void)
{
    return GetTickMS() * 1000;
}

