#ifndef CHARON_INTERFACE_NVMDRIVER_H_
#define CHARON_INTERFACE_NVMDRIVER_H_

/* Includes ******************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"

/* Constants *****************************************************************/
#define NVM_SCHEMA_MAGIC                                (0xDEADBEE4U)

/* Macros ********************************************************************/

/* Types *********************************************************************/

/* Variables *****************************************************************/
/**
 * @brief Initializes the NVM storage engine.
 * Scans flash for the latest valid wear-leveled slot and loads it into the
 * RAM shadow buffer. If flash is blank or uninitialized, it formats default
 * tables and commits them to flash.
 */
void charon_NvmDriver_init(void);

/**
 * @brief Commits the current RAM shadow buffer to Flash with wear leveling.
 * Calculates CRC16 and writes to the next available flash slot.
 *
 * @retval uds_responseCode_PositiveResponse          Flash written successfully
 * @retval uds_responseCode_GeneralProgrammingFailure Flash write failed
 */
uds_responseCode_t charon_NvmDriver_flush(void);
/* Interfaces  ***************************************************************/

/**
 * @brief Checks given memory address for validity.
 * This function checks the address range, if it is available in
 * memory map for reading or writing.
 * 
 * @param address      The start address of the memory range
 * @param length       The length of the range
 * @retval true        Memory range is valid
 * @retval false       Memory range is not valid
 */
bool charon_NvmDriver_checkAddressRange(uint32_t address, uint32_t length);

/**
 * @brief Writes data to non volatile memory.
 * This function writes given data to the desired memory location.
 * 
 * @param address      The start address of write operation
 * @param data         The data to write
 * @param size         The amount of bytes to write
 * @retval uds_responseCode_PositiveResponse          Memory written successfully
 * @retval uds_responseCode_GeneralProgrammingFailure Error while programming
*/
uds_responseCode_t charon_NvmDriver_write (uint32_t address, const uint8_t* data, uint32_t size);

/**
 * @brief Reads data from non volatile memory.
 * This function reads data to given buffer from desired memory location.
 * 
 * @param address      The start address of read operation
 * @param data         The buffer to fill with data
 * @param size         The number of bytes to read
 */
void charon_NvmDriver_read (uint32_t address, uint8_t* data, uint32_t size);

/**
 * @brief Erases non volatile memory.
 */
void charon_NvmDriver_erase (void);

/**
 * @brief Gives back the Nvm start address.
 * 
 * @return uint32_t    The Nvm start address
 */
uint32_t charon_NvmDriver_getNvmAddress (void);

/**
 * @brief Gives back the Mirror Nvm start address.
 * 
 * @param input        Is for the requested DTC, 0 for the first and so on
 * @param header       Option to request the header of the memory block 
 * @return uint32_t    The Mirror Nvm start address
 */
uint32_t charon_NvmDriver_getMirrorNvmAddress (uint16_t input, bool header);

/**
 * @brief Easy Nvm handling, TRUE = DTC header / FALSE = start of DTC storage.
 * 
 * @param input        Is for the requested DTC, 0 for the first and so on
 * @param header       Option to request the header of the memory block 
 * @return uint32_t    The Nvm start address for DTC
 */
uint32_t charon_NvmDriver_getNvmAddress_for_DTC (uint16_t input, bool header);

/**
 * @brief Easy Nvm handling.
 * 
 * @param input        Is for the requested Snapshot, 0 for the first and so on
 * @return uint32_t    The Nvm start address for Snapshot
 */
uint32_t charon_NvmDriver_getNvmAddress_for_Snapshot (uint16_t input);

/**
 * @brief Easy Nvm handling. 
 * 
 * @param input        Is for the requested StoredData, 0 for the first and so on
 * @return uint32_t    The Nvm start address for StoredData
 */
uint32_t charon_NvmDriver_getNvmAddress_for_StoredData (uint16_t input);

/**
 * @brief Easy Nvm handling.
 * 
 * @param input        Is for the requested ExtendedData, 0 for the first and so on
 * @return uint32_t    The Nvm start address for ExtendedData
 */
uint32_t charon_NvmDriver_getNvmAddress_for_ExtendedData (uint16_t input);

#endif 