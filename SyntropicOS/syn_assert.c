#include "syn_assert.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"



/* ── Assert Handler Fallback ────────────────────────────────────────────── */

SYN_WEAK SYN_NORETURN void syn_assert_failed(const char *file, int line)
{
    (void)file;
    (void)line;
    __disable_irq();
    for (;;) {
    }
}


