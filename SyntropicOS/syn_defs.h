#ifndef SYN_DEFS_H
#define SYN_DEFS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



/* ── Status codes ───────────────────────────────────────────────────────── */

/**
 * @brief Return status used by all SyntropicOS functions.
 */
typedef enum {
    SYN_OK = 0,               /**< Operation completed successfully */
    SYN_ERROR = -1,           /**< Generic error */
    SYN_BUSY = -2,            /**< Resource is busy */
    SYN_TIMEOUT = -3,         /**< Operation timed out */
    SYN_INVALID_PARAM = -4,   /**< Invalid parameter passed */
    SYN_NOT_IMPLEMENTED = -5, /**< Function not implemented by port */
    SYN_NOT_FOUND = -6,       /**< Requested item not found */
} SYN_Status;

/* ── GPIO types ─────────────────────────────────────────────────────────── */

/**
 * @brief Abstract GPIO pin identifier.
 *
 * The meaning of this value is platform-specific. It may encode a
 * port+pin pair, a flat pin number, or any scheme the port layer defines.
 */
typedef uint16_t SYN_GPIO_Pin;

/**
 * @brief Logical state of a GPIO pin.
 */
typedef enum {
    SYN_GPIO_LOW = 0,
    SYN_GPIO_HIGH = 1,
} SYN_GPIO_State;

/**
 * @brief GPIO pin mode / direction.
 */
typedef enum {
    SYN_GPIO_INPUT = 0,
    SYN_GPIO_OUTPUT = 1,
    SYN_GPIO_INPUT_PULLUP = 2,
    SYN_GPIO_INPUT_PULLDOWN = 3,
    SYN_GPIO_OUTPUT_OD = 4, /**< Open-drain output */
} SYN_GPIO_Mode;

/* ── UART types ─────────────────────────────────────────────────────────── */

/**
 * @brief Identifies a UART peripheral instance (0, 1, 2, …).
 */
typedef uint8_t SYN_UARTInstance;

/* ── Utility macros ─────────────────────────────────────────────────────── */

/** Number of elements in a statically-allocated array. */
#define SYN_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/** Suppress unused-variable warnings. */
#define SYN_UNUSED_VAR(x) ((void)(x))

/** @brief Minimum of two values. */
#define SYN_MIN(a, b) (((a) < (b)) ? (a) : (b))
/** @brief Maximum of two values. */
#define SYN_MAX(a, b) (((a) > (b)) ? (a) : (b))

/** Absolute value */
#define SYN_ABS(x) (((x) < 0) ? -(x) : (x))

/** Sign: returns -1, 0, or +1 */
#define SYN_SIGN(x) (((x) > 0) - ((x) < 0))

/** Clamp value to [lo, hi] */
#define SYN_CLAMP(val, lo, hi) (((val) < (lo)) ? (lo) : (((val) > (hi)) ? (hi) : (val)))

#ifdef __cplusplus
}
#endif

#endif /* SYN_DEFS_H */
