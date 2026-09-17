#ifndef ISOTP_SOCKET_H_
#define ISOTP_SOCKET_H_

#include <stdint.h>



typedef struct ISocket_t_public
{
    int32_t (*numAvailableBytes)(void);                     /**< User should implement this function for hardware abstraction. */
    int32_t (*receive)(uint8_t *buf, uint32_t len);         /**< User should implement this function for hardware abstraction. */
    int32_t (*transmit)(const uint8_t *buf, uint32_t len);  /**< The User should implement this function if it is used for hardware abstraction. */
} ISocket_t;

/**
 * @brief Callback functions for CHARON UDS Stack.
 * Pass this struct to charon uds during initialization.
 */
extern const ISocket_t isotp_socket;

/**
 * @brief Initializes ISO-TP and CAN peripheral.
 * Call this function upon main init procedure.
 * ISO-TP and CAN driver are initialized by this
 * function.
 */
void isotp_socket_init (void);

/**
 * @brief Cyclic handling of ISO-TP stuff.
 * CAN bus is polled for new messages and
 * ISO-TP is handled in this function.
 * It must be called on a regular basis.
 */
void isotp_socket_task (void);

#endif 
