/**
 * @file bootloader_jump.c
 * @brief Cortex-M3 Application Jump Routine implementation for STM32F103
 */

#include "bootloader_jump.h"
#include "ota_metadata.h"

#if defined(STM32F103xE) || defined(STM32F1xx) || defined(USE_HAL_DRIVER)
#include "stm32f1xx_hal.h"
#endif

#define RAM_START_ADDR  0x20000000UL
#define RAM_END_ADDR    0x20010000UL /* 64 KB RAM for STM32F103xE */

bool bootloader_is_slot_bootable(uint32_t app_addr) {
    uint32_t msp_val = *(volatile uint32_t *)app_addr;
    uint32_t reset_handler = *(volatile uint32_t *)(app_addr + 4U);

    /* 1. MSP must point within valid SRAM bounds and be 8-byte aligned */
    if ((msp_val < RAM_START_ADDR) || (msp_val > RAM_END_ADDR) || ((msp_val & 0x07U) != 0U)) {
        return false;
    }

    /* 2. Reset handler address must reside within application bounds and have Thumb bit set */
    if ((reset_handler < app_addr) || 
        (reset_handler > (app_addr + SLOT_SIZE)) || 
        ((reset_handler & 0x01U) == 0U)) {
        return false;
    }

    return true;
}

void bootloader_jump_to_app(uint32_t app_addr) {
    if (!bootloader_is_slot_bootable(app_addr)) {
        return;
    }

    uint32_t msp_val = *(volatile uint32_t *)app_addr;
    uint32_t reset_handler = *(volatile uint32_t *)(app_addr + 4U);

    /* 1. Disable all interrupts */
    __disable_irq();

    /* 2. Disable SysTick timer */
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    /* 3. Clear all NVIC interrupt enable bits and pending flags */
    for (uint8_t i = 0U; i < 8U; ++i) {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }

    /* 4. De-initialize HAL and clock settings */
#if defined(HAL_MODULE_ENABLED)
    HAL_DeInit();
#endif

    /* 5. Relocate Vector Table to target application base address */
    SCB->VTOR = app_addr;

    /* 6. Set Main Stack Pointer to application's initial stack pointer */
    __set_MSP(msp_val);

    /* 7. Jump to Application Reset Handler */
    void (*app_entry)(void) = (void (*)(void))(uintptr_t)reset_handler;
    app_entry();

    /* Unreachable */
    while (1) {
    }
}
