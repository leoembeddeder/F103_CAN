#include "isotp_socket.h"
#include "can.h"
#include "isotp.h"
#include <stdint.h>


#define ISOTP_RCV_BUFFER_SIZE     512u
#define ISOTP_TX_BUFFER_SIZE      512u
#define CAN_RX_QUEUE_SIZE         16u

typedef struct
{
    uint32_t id;
    uint8_t len;
    uint8_t data[8];
} CanRxFrame_t;

static int32_t numAvailableBytes(void);
static int32_t receive(uint8_t *buf, uint32_t len);
static int32_t transmit(const uint8_t *buf, uint32_t len);
static bool enqueueCanFrame(uint32_t id, const uint8_t *data, uint8_t len);
static bool dequeueCanFrame(uint32_t *id, uint8_t *data, uint8_t *len);

static IsoTpLink g_phylink;
static uint8_t g_isotpPhyRecvBuf[ISOTP_RCV_BUFFER_SIZE];
static uint8_t g_isotpPhySendBuf[ISOTP_TX_BUFFER_SIZE];
static CanRxFrame_t g_canRxQueue[CAN_RX_QUEUE_SIZE];
static volatile uint8_t  g_canRxHead = 0u;
static volatile uint8_t  g_canRxTail = 0u;
static volatile uint32_t g_canRxDropped = 0u;
static volatile uint32_t g_canTxErrors = 0u;

void isotp_socket_init(void)
{
    g_canRxHead = 0u;
    g_canRxTail = 0u;
    g_canRxDropped = 0u;
    g_canTxErrors = 0u;
    isotp_init_link(
        &g_phylink,
        TX_ID,
        g_isotpPhySendBuf,
        sizeof(g_isotpPhySendBuf),
        g_isotpPhyRecvBuf,
        sizeof(g_isotpPhyRecvBuf));
}

void isotp_socket_task(void)
{
    uint32_t id;
    uint8_t len;
    uint8_t data[8];

    while (dequeueCanFrame(&id, data, &len))
    {
        /*
         * Physical requests may be multi-frame. Functional requests are only
         * accepted as single frames because ISO-TP flow control is physical.
         */
        if ((id == RX_ID) ||
            ((id == 0x7DFu) && ((data[0] & 0xF0u) == 0u)))
        {
            isotp_on_can_message(&g_phylink, data, len);
        }
    }

    isotp_poll(&g_phylink);
}

const ISocket_t isotp_socket =
{
    .numAvailableBytes = numAvailableBytes,
    .receive = receive,
    .transmit = transmit
};

static int32_t numAvailableBytes(void)
{
    if (g_phylink.receive_status == ISOTP_RECEIVE_STATUS_FULL)
    {
        return (int32_t)g_phylink.receive_size;
    }
    return 0;
}

static int32_t receive(uint8_t *buf, uint32_t len)
{
    uint32_t outSize = 0u;
    int result = isotp_receive(&g_phylink, buf, len, &outSize);
    return (result == ISOTP_RET_OK) ? (int32_t)outSize : (int32_t)result;
}

static int32_t transmit(const uint8_t *buf, uint32_t len)
{
    int result = isotp_send(&g_phylink, buf, len);
    if (result != ISOTP_RET_OK)
    {
        g_canTxErrors++;
    }
    return (result == ISOTP_RET_OK) ? (int32_t)len : (int32_t)result;
}

static bool enqueueCanFrame(uint32_t id, const uint8_t *data, uint8_t len)
{
    uint8_t nextHead = (uint8_t)((g_canRxHead + 1u) % CAN_RX_QUEUE_SIZE);

    if ((data == NULL) || (len > 8u))
    {
        return false;
    }

    if (nextHead == g_canRxTail)
    {
        g_canRxDropped++;
        return false;
    }

    g_canRxQueue[g_canRxHead].id = id;
    g_canRxQueue[g_canRxHead].len = len;
    memcpy(g_canRxQueue[g_canRxHead].data, data, len);
    __DMB();
    g_canRxHead = nextHead;
    return true;
}

static bool dequeueCanFrame(uint32_t *id, uint8_t *data, uint8_t *len)
{
    uint8_t tail = g_canRxTail;

    if ((tail == g_canRxHead) || (id == NULL) || (data == NULL) || (len == NULL))
    {
        return false;
    }

    *id = g_canRxQueue[tail].id;
    *len = g_canRxQueue[tail].len;
    memcpy(data, g_canRxQueue[tail].data, *len);
    __DMB();
    g_canRxTail = (uint8_t)((tail + 1u) % CAN_RX_QUEUE_SIZE);
    return true;
}

uint32_t TpGetDroppedFrameCount(void)
{
    return g_canRxDropped;
}

uint32_t TpGetTransmitErrorCount(void)
{
    return g_canTxErrors;
}

void TpRecordCanTxError(void)
{
    g_canTxErrors++;
}





/* Receive FIFO 0 message pending interrupt management */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t data[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, data) == HAL_OK)
    {
        if ((rxHeader.StdId == RX_ID) || (rxHeader.StdId == 0x7DFu))
        {
			(void)enqueueCanFrame(rxHeader.StdId, data, (uint8_t)rxHeader.DLC);
		}
    }

}

