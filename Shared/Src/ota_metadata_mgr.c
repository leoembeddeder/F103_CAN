/**
 * @file ota_metadata_mgr.c
 * @brief Redundant Ping-Pong Metadata Manager implementation for STM32F103
 */

#include "ota_metadata_mgr.h"
#include "ota_crc32.h"
#include <string.h>

#if defined(STM32F103xE) || defined(STM32F1xx) || defined(USE_HAL_DRIVER)
#include "stm32f1xx_hal.h"
#endif

#define META_CRC_COVERAGE (sizeof(metadata_t) - sizeof(uint32_t)) /* 44 bytes */

static const uint32_t s_meta_pages[2] = {
    META_PAGE_A_ADDR,
    META_PAGE_B_ADDR
};

/* ----------------------------------------------------------------
 * Internal Flash Driver Helpers
 * ---------------------------------------------------------------- */

static int flash_erase_metadata_page(uint32_t page_addr) {
#if defined(HAL_FLASH_MODULE_ENABLED) || defined(STM32F103xE) || defined(STM32F1xx)
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

    FLASH_EraseInitTypeDef erase_init;
    memset(&erase_init, 0, sizeof(erase_init));
    erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = page_addr;
    erase_init.NbPages     = 1U;

    uint32_t page_error = 0U;
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    HAL_FLASH_Lock();

    return (status == HAL_OK) ? 0 : -1;
#else
    (void)page_addr;
    return 0;
#endif
}

static int flash_write_metadata(uint32_t page_addr, const void *data, size_t len) {
#if defined(HAL_FLASH_MODULE_ENABLED) || defined(STM32F103xE) || defined(STM32F1xx)
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

    const uint8_t *src = (const uint8_t *)data;
    size_t i = 0U;

    while ((i + 2U) <= len) {
        uint16_t halfword = (uint16_t)(src[i] | ((uint16_t)src[i + 1U] << 8U));
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, page_addr + i, (uint64_t)halfword) != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }
        i += 2U;
    }

    if (i < len) {
        uint16_t halfword = (uint16_t)(src[i] | 0xFF00U);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, page_addr + i, (uint64_t)halfword) != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }
    }

    HAL_FLASH_Lock();
    return 0;
#else
    (void)page_addr;
    (void)data;
    (void)len;
    return 0;
#endif
}

/* ----------------------------------------------------------------
 * Validation & Loading
 * ---------------------------------------------------------------- */

bool ota_metadata_is_valid(const metadata_t *meta) {
    if (meta == NULL) {
        return false;
    }

    if (meta->magic != METADATA_MAGIC) {
        return false;
    }

    uint32_t computed_crc = ota_crc32_compute(meta, META_CRC_COVERAGE);
    if (computed_crc != meta->meta_crc32) {
        return false;
    }

    if (meta->active_slot > SLOT_B) {
        return false;
    }

    for (unsigned s = 0U; s < 2U; ++s) {
        if (meta->slot[s].state > STATE_VALID) {
            return false;
        }
        if (meta->slot[s].size > SLOT_SIZE) {
            return false;
        }
    }

    return true;
}

static bool load_page(unsigned index, metadata_t *dest) {
    if (dest == NULL || index > 1U) {
        return false;
    }

    const metadata_t *flash_ptr = (const metadata_t *)s_meta_pages[index];
    memcpy(dest, flash_ptr, sizeof(metadata_t));

    return ota_metadata_is_valid(dest);
}

metadata_status_t ota_metadata_read(metadata_t *dest) {
    if (dest == NULL) {
        return META_ERR_ARG;
    }

    metadata_t pageA;
    metadata_t pageB;

    bool a_ok = load_page(0U, &pageA);
    bool b_ok = load_page(1U, &pageB);

    if (!a_ok && !b_ok) {
        return META_ERR_NONE; /* Blank or corrupted flash */
    }

    const metadata_t *chosen;
    if (a_ok && b_ok) {
        chosen = (pageB.counter > pageA.counter) ? &pageB : &pageA;
    } else {
        chosen = a_ok ? &pageA : &pageB;
    }

    memcpy(dest, chosen, sizeof(metadata_t));
    return META_OK;
}

/* ----------------------------------------------------------------
 * Atomic Ping-Pong Write
 * ---------------------------------------------------------------- */

static void select_write_target(unsigned *page, uint32_t *counter) {
    metadata_t pageA;
    metadata_t pageB;

    bool a_ok = load_page(0U, &pageA);
    bool b_ok = load_page(1U, &pageB);

    if (!a_ok && !b_ok) {
        *page = 0U;
        *counter = 1U;
        return;
    }

    if (a_ok && b_ok) {
        if (pageB.counter > pageA.counter) {
            *page = 0U; /* Page B is latest -> overwrite Page A */
            *counter = pageB.counter + 1U;
        } else {
            *page = 1U; /* Page A is latest -> overwrite Page B */
            *counter = pageA.counter + 1U;
        }
        return;
    }

    if (a_ok) {
        *page = 1U;
        *counter = pageA.counter + 1U;
    } else {
        *page = 0U;
        *counter = pageB.counter + 1U;
    }
}

metadata_status_t ota_metadata_write(const metadata_t *src) {
    if (src == NULL) {
        return META_ERR_ARG;
    }

    unsigned page = 0U;
    uint32_t counter = 1U;
    select_write_target(&page, &counter);

    metadata_t out;
    memset(&out, 0, sizeof(out));

    out.magic           = METADATA_MAGIC;
    out.counter         = counter;
    out.active_slot     = src->active_slot;
    out.boot_fail_count = src->boot_fail_count;

    for (unsigned s = 0U; s < 2U; ++s) {
        out.slot[s].size    = src->slot[s].size;
        out.slot[s].crc32   = src->slot[s].crc32;
        out.slot[s].version = src->slot[s].version;
        out.slot[s].state   = src->slot[s].state;
    }

    out.meta_crc32 = ota_crc32_compute(&out, META_CRC_COVERAGE);

    uint32_t target_addr = s_meta_pages[page];

    if (flash_erase_metadata_page(target_addr) != 0) {
        return META_ERR_ERASE;
    }

    if (flash_write_metadata(target_addr, &out, sizeof(metadata_t)) != 0) {
        return META_ERR_WRITE;
    }

    /* Verification Readback */
    metadata_t verify;
    if (!load_page(page, &verify) || (verify.counter != counter)) {
        return META_ERR_VERIFY;
    }

    return META_OK;
}

/* ----------------------------------------------------------------
 * State Management & Transitions
 * ---------------------------------------------------------------- */

metadata_status_t ota_metadata_init_default(metadata_t *dest) {
    metadata_t init_meta;
    memset(&init_meta, 0, sizeof(init_meta));

    init_meta.magic           = METADATA_MAGIC;
    init_meta.counter         = 1U;
    init_meta.active_slot     = SLOT_A;
    init_meta.boot_fail_count = 0U;

    /* Slot A configured as default initial valid image */
    init_meta.slot[SLOT_A].size    = 0U;
    init_meta.slot[SLOT_A].crc32   = 0U;
    init_meta.slot[SLOT_A].version = 1U;
    init_meta.slot[SLOT_A].state   = STATE_VALID;

    /* Slot B starts empty */
    init_meta.slot[SLOT_B].size    = 0U;
    init_meta.slot[SLOT_B].crc32   = 0U;
    init_meta.slot[SLOT_B].version = 0U;
    init_meta.slot[SLOT_B].state   = STATE_EMPTY;

    metadata_status_t status = ota_metadata_write(&init_meta);
    if ((status == META_OK) && (dest != NULL)) {
        memcpy(dest, &init_meta, sizeof(metadata_t));
    }
    return status;
}

metadata_status_t ota_metadata_set_candidate(uint8_t target_slot, uint32_t size, uint32_t crc32, uint32_t version) {
    if (target_slot > SLOT_B) {
        return META_ERR_ARG;
    }

    metadata_t meta;
    metadata_status_t status = ota_metadata_read(&meta);
    if (status != META_OK) {
        (void)ota_metadata_init_default(&meta);
    }

    meta.slot[target_slot].size    = size;
    meta.slot[target_slot].crc32   = crc32;
    meta.slot[target_slot].version = version;
    meta.slot[target_slot].state   = STATE_TESTING;

    meta.active_slot     = target_slot;
    meta.boot_fail_count = 0U;

    return ota_metadata_write(&meta);
}

metadata_status_t ota_metadata_confirm(void) {
    metadata_t meta;
    metadata_status_t status = ota_metadata_read(&meta);
    if (status != META_OK) {
        return status;
    }

    uint8_t active = meta.active_slot;
    if ((meta.slot[active].state == STATE_VALID) && (meta.boot_fail_count == 0U)) {
        return META_OK; /* Already confirmed */
    }

    meta.slot[active].state = STATE_VALID;
    meta.boot_fail_count = 0U;

    return ota_metadata_write(&meta);
}

metadata_status_t ota_metadata_rollback(void) {
    metadata_t meta;
    metadata_status_t status = ota_metadata_read(&meta);
    if (status != META_OK) {
        return status;
    }

    uint8_t failed_slot   = meta.active_slot;
    uint8_t fallback_slot = OTHER_SLOT(failed_slot);

    meta.slot[failed_slot].state = STATE_EMPTY;
    meta.active_slot             = fallback_slot;
    meta.boot_fail_count         = 0U;

    return ota_metadata_write(&meta);
}

bool ota_metadata_image_is_intact(const metadata_t *meta, uint8_t slot) {
    if ((meta == NULL) || (slot > SLOT_B)) {
        return false;
    }

    uint32_t image_size = meta->slot[slot].size;
    if ((image_size == 0U) || (image_size > SLOT_SIZE)) {
        return false;
    }

    const void *flash_base = (const void *)(uintptr_t)SLOT_ADDR(slot);
    uint32_t computed = ota_crc32_compute(flash_base, image_size);

    return (computed == meta->slot[slot].crc32);
}

uint8_t ota_metadata_get_active_slot(void) {
    metadata_t meta;
    if (ota_metadata_read(&meta) == META_OK) {
        return meta.active_slot;
    }
    return SLOT_A;
}

uint8_t ota_metadata_get_target_slot(void) {
    return OTHER_SLOT(ota_metadata_get_active_slot());
}
