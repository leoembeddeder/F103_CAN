#include "NvmEmulator.h"
#include "StoredDataTransmissionFunctionalUnit.h"
#include "DTC_LookupTable.h"
#include "uds_config.h"
#include "syn_param.h"
#include "flash.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>



/* Imports *******************************************************************/
#define ALIGN4(x) (((uint32_t)(x) + 3u) & ~3u)

/* Constants *****************************************************************/
/** @brief Size of the DTC header area. */
#define STORAGE_HEADER ((uint8_t) sizeof(DTC_header_t)) 

/* Macros ********************************************************************/
/** @brief Start and End boundaries for memory areas */
#define START_OF_RESERVED_SPACE_FOR_DTC             (0u)
#define END_OF_RESERVED_SPACE_FOR_DTC               (STORAGE_HEADER + ((uint32_t)sizeof(DTC_t) * (uint32_t)AMOUNT_OF_DTC))
#define START_OF_RESERVED_SPACE_FOR_SNAPSHOT        ALIGN4(END_OF_RESERVED_SPACE_FOR_DTC)
#define END_OF_RESERVED_SPACE_FOR_SNAPSHOT          (START_OF_RESERVED_SPACE_FOR_SNAPSHOT + ((uint32_t)sizeof(DTC_SnapshotData_t) * (uint32_t)AMOUNT_OF_SNAPSHOT))

#define START_OF_RESERVED_SPACE_FOR_STOREDDATA      ALIGN4(END_OF_RESERVED_SPACE_FOR_SNAPSHOT)
#define END_OF_RESERVED_SPACE_FOR_STOREDDATA        (START_OF_RESERVED_SPACE_FOR_STOREDDATA + ((uint32_t)sizeof(DTC_StoredData_t) * (uint32_t)AMOUNT_OF_STOREDDATA))

#define START_OF_RESERVED_SPACE_FOR_EXTENDEDDATA    ALIGN4(END_OF_RESERVED_SPACE_FOR_STOREDDATA)
#define END_OF_RESERVED_SPACE_FOR_EXTENDEDDATA      (START_OF_RESERVED_SPACE_FOR_EXTENDEDDATA + ((uint32_t)sizeof(DTC_ExtendedData_t) * (uint32_t)AMOUNT_OF_EXTENDEDDATA))

#define NVM_STORAGE_TOTAL_SIZE                      ALIGN4(END_OF_RESERVED_SPACE_FOR_EXTENDEDDATA)



/* Variables *****************************************************************/
/** @brief RAM working shadow buffer */
static uint8_t NvmEmulator_MemorySpace[NVM_STORAGE_TOTAL_SIZE];

/** @brief Wear-leveling parameter store handle */
static SYN_ParamStore s_nvmStore;
static bool s_nvmInitialized = false;

static void charon_NvmDriver_reanchorPointers(void)
{
    DTC_header_t *hdr = (DTC_header_t *)charon_NvmDriver_getNvmAddress_for_DTC(0, true);
    if (hdr->iniDone != 0xDEADBEEF)
    {
        return;
    }

    /* Re-anchor pointers in RAM for DTCs and snapshots */
    for (uint16_t i = 0; i < AMOUNT_OF_DTC; i++)
    {
        DTC_t *dtc = (DTC_t *)charon_NvmDriver_getNvmAddress_for_DTC(i, false);
        for (uint8_t s = 0; s < NVM_AMOUNT_OF_SNAPSHOTS; s++)
        {
            if (dtc->DTCSnapshotLength[s] > 0)
            {
                uint8_t snapRecNum = dtc->DTCSnapshotRecordNumber[s];
                dtc->DTCSnapshotAddress[s] = (DTC_SnapshotData_t *)charon_NvmDriver_getNvmAddress_for_Snapshot(snapRecNum);
            }
            else
            {
                dtc->DTCSnapshotAddress[s] = NULL;
            }
        }
        for (uint8_t d = 0; d < NVM_AMOUNT_OF_DATARECORDS; d++)
        {
            if (dtc->DTCStoredDataLength[d] > 0)
            {
                uint8_t recNum = dtc->DTCStoredDataRecordNumber[d];
                dtc->DTCStoredDataAddress[d] = (DTC_StoredData_t *)charon_NvmDriver_getNvmAddress_for_StoredData(recNum);
            }
            else
            {
                dtc->DTCStoredDataAddress[d] = NULL;
            }
        }
        for (uint8_t e = 0; e < NVM_AMOUNT_OF_EXTENDED; e++)
        {
            if (dtc->DTCExtendedDataLength[e] > 0)
            {
                uint8_t recNum = dtc->DTCExtDataRecordNumber[e];
                dtc->DTCExtendedDataAddress[e] = (DTC_ExtendedData_t *)charon_NvmDriver_getNvmAddress_for_ExtendedData(recNum);
            }
            else
            {
                dtc->DTCExtendedDataAddress[e] = NULL;
            }
        }
    }
}


void charon_NvmDriver_init(void)
{
    /* Initialize parameter store with 2 flash sectors (4KB total wear-leveling pool) */
    SYN_Status status = syn_param_init(&s_nvmStore, FLASH_PARAM_START, 2, sizeof(NvmEmulator_MemorySpace));

    if (status == SYN_OK)
    {
        /* Load latest persistent record */
        if (syn_param_load(&s_nvmStore, NvmEmulator_MemorySpace) == SYN_OK)
        {
            DTC_header_t *hdr = (DTC_header_t *)charon_NvmDriver_getNvmAddress_for_DTC(0, true);
            if (hdr->iniDone == 0xDEADBEEF)
            {
                charon_NvmDriver_reanchorPointers();
                s_nvmInitialized = true;
                return;
            }
        }
    }
    /* First boot or blank flash: format default tables */
    memset(NvmEmulator_MemorySpace, 0, sizeof(NvmEmulator_MemorySpace));
    s_nvmInitialized = true;
    charon_DTC_LookupTable_header_SET();
    charon_NvmDriver_flush();
}

uds_responseCode_t charon_NvmDriver_flush(void)
{
    if (!s_nvmInitialized)
    {
        charon_NvmDriver_init();
    }
    /* Ensure header CRC16 is updated */
    charon_StoredDataTransmissionFunctionalUnit_CRC16_update();
    SYN_Status status = syn_param_save(&s_nvmStore, NvmEmulator_MemorySpace);
    if (status == SYN_OK)
    {
        return uds_responseCode_PositiveResponse;
    }
    return uds_responseCode_GeneralProgrammingFailure;
} 




/* Private Function Definitions **********************************************/

/* Interfaces  ***************************************************************/

bool charon_NvmDriver_checkAddressRange(uint32_t address, uint32_t length)
{
    return (address + length) <= sizeof(NvmEmulator_MemorySpace);
}

uds_responseCode_t charon_NvmDriver_write(uint32_t address, const uint8_t* data, uint32_t size)
{
	 if (!charon_NvmDriver_checkAddressRange(address, size))
	 {
		 return uds_responseCode_RequestOutOfRange;
	 }
	 memcpy(&NvmEmulator_MemorySpace[address], data, size);
	 return charon_NvmDriver_flush();
}

void charon_NvmDriver_read (uint32_t address, uint8_t* data, uint32_t size)
{
	if (charon_NvmDriver_checkAddressRange(address, size))
	{
	 memcpy(data, &NvmEmulator_MemorySpace[address], size);
	}
}

void charon_NvmDriver_erase (void)
{
	syn_param_erase_all(&s_nvmStore);
	memset(NvmEmulator_MemorySpace, 0, sizeof(NvmEmulator_MemorySpace));
	charon_DTC_LookupTable_header_SET();
	charon_NvmDriver_flush();
}

uint32_t charon_NvmDriver_getNvmAddress (void)
{
    return (uint32_t)&NvmEmulator_MemorySpace[0];
}

//###########################################################################################################
// DTC related 
//###########################################################################################################

uint32_t charon_NvmDriver_getMirrorNvmAddress (uint16_t input, bool header)
{
	return charon_NvmDriver_getNvmAddress_for_DTC(input, header);
}

uint32_t charon_NvmDriver_getNvmAddress_for_DTC (uint16_t input, bool header)
{
	uint32_t offset = START_OF_RESERVED_SPACE_FOR_DTC;

	if (header)
	{
	 return (uint32_t)&NvmEmulator_MemorySpace[offset];
	}
	 offset += STORAGE_HEADER + ((uint32_t)sizeof(DTC_t) * (uint32_t)input);
	 return (uint32_t)&NvmEmulator_MemorySpace[offset];
}

uint32_t charon_NvmDriver_getNvmAddress_for_Snapshot (uint16_t input)
{
	uint32_t offset = START_OF_RESERVED_SPACE_FOR_SNAPSHOT + ((uint32_t)sizeof(DTC_SnapshotData_t) * (uint32_t)input);
	return (uint32_t)&NvmEmulator_MemorySpace[offset];
}

uint32_t charon_NvmDriver_getNvmAddress_for_StoredData (uint16_t input)
{
	uint32_t offset = START_OF_RESERVED_SPACE_FOR_STOREDDATA + ((uint32_t)sizeof(DTC_StoredData_t) * (uint32_t)input);
	return (uint32_t)&NvmEmulator_MemorySpace[offset];
}

uint32_t charon_NvmDriver_getNvmAddress_for_ExtendedData (uint16_t input)
{
	uint32_t offset = START_OF_RESERVED_SPACE_FOR_EXTENDEDDATA + ((uint32_t)sizeof(DTC_ExtendedData_t) * (uint32_t)input);
	return (uint32_t)&NvmEmulator_MemorySpace[offset];
}

//###########################################################################################################
// DID related 
//###########################################################################################################

