#include "flash.h"


/**
 * @brief Helper to map flash memory address to STM32F103 Sector number.
 */
uint32_t GetPage(uint32_t Addr)
{
  return (Addr - STM32F103_FLASH_BASE) / STM32F103_PAGE_SIZE;;
}


/* ── Sector / Page Size ─────────────────────────────────────────────────── */

uint32_t syn_port_flash_sector_size(uint32_t addr)
{
    (void)addr;
    return STM32F103_PAGE_SIZE; /* 2048 bytes */
}


/**
 * @brief Write Flash using 32-bit Word Mode (FLASH_TYPEPROGRAM_WORD).
 */
SYN_Status syn_port_flash_write_word(uint32_t addr, const void *buf, size_t len)
{
    if (buf == NULL) return SYN_INVALID_PARAM;

    HAL_FLASH_Unlock();
    const uint8_t *data = (const uint8_t *)buf;
    size_t i = 0U;

    /* Write 32-bit word chunks */
    while (i + 4U <= len) {
        uint32_t word_val;
        memcpy(&word_val, &data[i], 4U);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, (uint64_t)word_val) != HAL_OK) {
            HAL_FLASH_Lock();
            return SYN_ERROR;
        }
        i += 4U;
    }

    /* Write trailing bytes */
    if (i < len) {
        uint32_t word_val = 0xFFFFFFFFU;
        memcpy(&word_val, &data[i], len - i);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, (uint64_t)word_val) != HAL_OK) {
            HAL_FLASH_Lock();
            return SYN_ERROR;
        }
    }

    HAL_FLASH_Lock();
    return SYN_OK;
}



/* ── Page Erase ─────────────────────────────────────────────────────────── */

SYN_Status syn_port_flash_erase(uint32_t addr)
{
    HAL_FLASH_Unlock();
	
    FLASH_EraseInitTypeDef erase_init;
    memset(&erase_init, 0, sizeof(erase_init));
	erase_init.Banks       = 1;
    erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = addr;
    erase_init.NbPages     = 1U;

    uint32_t page_error = 0U;
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_init, &page_error);

    HAL_FLASH_Lock();

    return (status == HAL_OK) ? SYN_OK : SYN_ERROR;
}



SYN_Status syn_port_flash_write(uint32_t addr, const void *buf, size_t len)
{
    return (syn_port_flash_write_word(addr, buf, len) == SYN_OK) ? SYN_OK : SYN_ERROR;
}


/* ── Flash Read ─────────────────────────────────────────────────────────── */

SYN_Status syn_port_flash_read(uint32_t addr, void *buf, size_t len)
{
    if (buf == NULL) return SYN_INVALID_PARAM;
    memcpy(buf, (const void *)(uintptr_t)addr, len);
    return SYN_OK;
}







/* ── STM32 Flash demo test ─ */
typedef struct {
	uint16_t brightness;
	int16_t  offset;
	uint8_t  mode;
} MyParams;
static SYN_ParamStore store;
static MyParams params;

//#define FLASH_PARAM_START (STM32F103_FLASH_BASE + 10U * STM32F103_PAGE_SIZE) 




void stm32f103_flash_test(void)
{
    /* 1. Initialize with 2 pages (2 KB each = 4 KB total) */
    syn_param_init(&store, FLASH_PARAM_START, 2, sizeof(MyParams));

    /* 2. Load latest parameter record or defaults */
    syn_param_load(&store, &params);
    printf("Loaded -> brightness: %d, offset: %d, mode: %d\r\n",
           params.brightness, params.offset, params.mode);

    params.brightness++;
    params.offset++;
    params.mode++;

    /* 3. Save to next slot */
    SYN_Status status = syn_param_save(&store, &params);
    if (status == SYN_OK) {
        print_flash_status(&store);
    } else {
        printf("Save failed with status: %d\r\n", status);
    }
}




