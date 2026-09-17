#include "isotp_socket.h"
#include "can.h"
#include "isotp.h"

#include <stdint.h>

#define BUFF_SIZE   1024

static int32_t numAvailableBytes(void);
static int32_t receive(uint8_t *buf, uint32_t len);
static int32_t transmit(const uint8_t *buf, uint32_t len);

static IsoTpLink link;
static uint8_t sendbuffer[BUFF_SIZE];
static uint8_t receivebuffer[BUFF_SIZE];

const ISocket_t isotp_socket = 
{
    .numAvailableBytes = numAvailableBytes,
    .receive = receive,
    .transmit = transmit
};

static int32_t numAvailableBytes(void)
{
    int32_t numBytes = 0;
    if (link.receive_status == ISOTP_RECEIVE_STATUS_FULL)
    {
        numBytes = link.receive_size;
    }
    return numBytes;
}

static int32_t receive(uint8_t *buf, uint32_t len)
{
    uint16_t outSize = 0;
    isotp_receive(&link, buf, len, &outSize);
    return outSize;
}

static int32_t transmit(const uint8_t *buf, uint32_t len)
{
    isotp_send(&link, buf, len);
    return len;
}



void isotp_socket_init (void)
{
    isotp_init_link(&link, 0x7E8,
                    sendbuffer, sizeof(sendbuffer),
                    receivebuffer, sizeof(receivebuffer)
    );
}

void isotp_socket_task (void)
{
    isotp_poll(&link);
}




/* Receive FIFO 0 message pending interrupt management */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef hdr;
    uint8_t data[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, data) == HAL_OK)
    {
        if(RX_ID == hdr.StdId )
        {
			isotp_on_can_message(&link, data, hdr.DLC);
		}
    }

}

