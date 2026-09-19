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

static void charon_NvmDriver_seedDefaultDTCs(void)
{
    charon_DTC_LookupTable_header_SET();

    /* DTC 1: P010016 - Mass Air Flow Sensor Circuit */
    {
        DTC_t dtc;
        memset(&dtc, 0, sizeof(dtc));
        dtc.DTCHighByte = 0x01;
        dtc.DTCMiddleByte = 0x00;
        dtc.DTCLowByte = 0x16;
        dtc.DTCStatusMask = 0x2F;
        dtc.statusOfDTC = 0x2F;
        dtc.DTCSeverityMask = 0x20;
        dtc.DTCSeverityMaskRecordHigh = 0x20;
        dtc.DTCSeverityMaskRecordLow = 0x2F;
        dtc.FunctionalGroupIdentifier = 0x33;
        dtc.DTCSettingType = 0x01;

        DTC_SnapshotData_t snap;
        memset(&snap, 0, sizeof(snap));
        snap.DTCSnapshotDataRecordNumberOfIdentifiers = 0x01;
        snap.DTCSnapshotDataPayload[0] = 0x00;
        snap.DTCSnapshotDataPayload[1] = 0x40;
        snap.DTCSnapshotDataPayload[2] = 0x12;
        dtc.DTCSnapshotLength[0] = 0x03;

        DTC_StoredData_t stored;
        memset(&stored, 0, sizeof(stored));
        stored.DTCStoredDataRecordNumberOfIdentifiers = 0x01;
        stored.DTCStoredDataPayload[0] = 0xEE;
        dtc.DTCStoredDataLength[0] = 0x01;

        DTC_ExtendedData_t ext;
        memset(&ext, 0, sizeof(ext));
        ext.DTCExtendedDataRecordNumberOfIdentifiers = 0x01;
        ext.DTCExtendedDataPayload[0] = 0x05;
        dtc.DTCExtendedDataLength[0] = 0x01;

        charon_StoredDataTransmissionFunctionalUnit_writeDTCToNvm(dtc, snap, stored, ext);
    }

    /* DTC 2: P020017 - Injector Circuit Open */
    {
        DTC_t dtc;
        memset(&dtc, 0, sizeof(dtc));
        dtc.DTCHighByte = 0x02;
        dtc.DTCMiddleByte = 0x00;
        dtc.DTCLowByte = 0x17;
        dtc.DTCStatusMask = 0x2B;
        dtc.statusOfDTC = 0x2B;
        dtc.DTCSeverityMask = 0x40;
        dtc.DTCSeverityMaskRecordHigh = 0x40;
        dtc.DTCSeverityMaskRecordLow = 0x2B;
        dtc.FunctionalGroupIdentifier = 0x33;
        dtc.DTCSettingType = 0x01;

        DTC_SnapshotData_t snap;
        memset(&snap, 0, sizeof(snap));
        snap.DTCSnapshotDataRecordNumberOfIdentifiers = 0x01;
        snap.DTCSnapshotDataPayload[0] = 0x00;
        snap.DTCSnapshotDataPayload[1] = 0x50;
        snap.DTCSnapshotDataPayload[2] = 0x22;
        dtc.DTCSnapshotLength[0] = 0x03;

        DTC_StoredData_t stored;
        memset(&stored, 0, sizeof(stored));
        stored.DTCStoredDataRecordNumberOfIdentifiers = 0x01;
        stored.DTCStoredDataPayload[0] = 0xEE;
        dtc.DTCStoredDataLength[0] = 0x01;

        DTC_ExtendedData_t ext;
        memset(&ext, 0, sizeof(ext));
        ext.DTCExtendedDataRecordNumberOfIdentifiers = 0x01;
        ext.DTCExtendedDataPayload[0] = 0x02;
        dtc.DTCExtendedDataLength[0] = 0x01;

        charon_StoredDataTransmissionFunctionalUnit_writeDTCToNvm(dtc, snap, stored, ext);
    }

    /* DTC 3: P030011 - Cylinder Misfire Detected */
    {
        DTC_t dtc;
        memset(&dtc, 0, sizeof(dtc));
        dtc.DTCHighByte = 0x03;
        dtc.DTCMiddleByte = 0x00;
        dtc.DTCLowByte = 0x11;
        dtc.DTCStatusMask = 0x01;
        dtc.statusOfDTC = 0x01;
        dtc.DTCSeverityMask = 0x60;
        dtc.DTCSeverityMaskRecordHigh = 0x60;
        dtc.DTCSeverityMaskRecordLow = 0x01;
        dtc.FunctionalGroupIdentifier = 0x33;
        dtc.DTCSettingType = 0x01;

        DTC_SnapshotData_t snap;
        memset(&snap, 0, sizeof(snap));
        snap.DTCSnapshotDataRecordNumberOfIdentifiers = 0x01;
        snap.DTCSnapshotDataPayload[0] = 0x00;
        snap.DTCSnapshotDataPayload[1] = 0x60;
        snap.DTCSnapshotDataPayload[2] = 0x33;
        dtc.DTCSnapshotLength[0] = 0x03;

        DTC_StoredData_t stored;
        memset(&stored, 0, sizeof(stored));
        stored.DTCStoredDataRecordNumberOfIdentifiers = 0x01;
        stored.DTCStoredDataPayload[0] = 0xEE;
        dtc.DTCStoredDataLength[0] = 0x01;

        DTC_ExtendedData_t ext;
        memset(&ext, 0, sizeof(ext));
        ext.DTCExtendedDataRecordNumberOfIdentifiers = 0x01;
        ext.DTCExtendedDataPayload[0] = 0x01;
        dtc.DTCExtendedDataLength[0] = 0x01;

        charon_StoredDataTransmissionFunctionalUnit_writeDTCToNvm(dtc, snap, stored, ext);
    }

    /* DTC 4: U010000 - Lost Communication With ECM */
    {
        DTC_t dtc;
        memset(&dtc, 0, sizeof(dtc));
        dtc.DTCHighByte = 0xC1;
        dtc.DTCMiddleByte = 0x00;
        dtc.DTCLowByte = 0x00;
        dtc.DTCStatusMask = 0x08;
        dtc.statusOfDTC = 0x08;
        dtc.DTCSeverityMask = 0x80;
        dtc.DTCSeverityMaskRecordHigh = 0x80;
        dtc.DTCSeverityMaskRecordLow = 0x08;
        dtc.FunctionalGroupIdentifier = 0xD0;
        dtc.DTCSettingType = 0x01;

        DTC_SnapshotData_t snap;
        memset(&snap, 0, sizeof(snap));
        snap.DTCSnapshotDataRecordNumberOfIdentifiers = 0x01;
        snap.DTCSnapshotDataPayload[0] = 0x12;
        snap.DTCSnapshotDataPayload[1] = 0x34;
        dtc.DTCSnapshotLength[0] = 0x02;

        DTC_StoredData_t stored;
        memset(&stored, 0, sizeof(stored));
        stored.DTCStoredDataRecordNumberOfIdentifiers = 0x01;
        stored.DTCStoredDataPayload[0] = 0xEE;
        dtc.DTCStoredDataLength[0] = 0x01;

        DTC_ExtendedData_t ext;
        memset(&ext, 0, sizeof(ext));
        ext.DTCExtendedDataRecordNumberOfIdentifiers = 0x01;
        ext.DTCExtendedDataPayload[0] = 0x03;
        dtc.DTCExtendedDataLength[0] = 0x01;

        charon_StoredDataTransmissionFunctionalUnit_writeDTCToNvm(dtc, snap, stored, ext);
    }

    /* DTC 5: C100616 - Battery Volt High */
    {
        DTC_t dtc;
        memset(&dtc, 0, sizeof(dtc));
        dtc.DTCHighByte = 0xD0;
        dtc.DTCMiddleByte = 0x06;
        dtc.DTCLowByte = 0x16;
        dtc.DTCStatusMask = 0x08;
        dtc.statusOfDTC = 0x08;
        dtc.DTCSeverityMask = 0x80;
        dtc.DTCSeverityMaskRecordHigh = 0x80;
        dtc.DTCSeverityMaskRecordLow = 0x08;
        dtc.FunctionalGroupIdentifier = 0xD0;
        dtc.DTCSettingType = 0x01;

        DTC_SnapshotData_t snap;
        memset(&snap, 0, sizeof(snap));
        snap.DTCSnapshotDataRecordNumberOfIdentifiers = 0x01;
        snap.DTCSnapshotDataPayload[0] = 0x12;
        snap.DTCSnapshotDataPayload[1] = 0x34;
        dtc.DTCSnapshotLength[0] = 0x02;

        DTC_StoredData_t stored;
        memset(&stored, 0, sizeof(stored));
        stored.DTCStoredDataRecordNumberOfIdentifiers = 0x01;
        stored.DTCStoredDataPayload[0] = 0xEE;
        dtc.DTCStoredDataLength[0] = 0x01;

        DTC_ExtendedData_t ext;
        memset(&ext, 0, sizeof(ext));
        ext.DTCExtendedDataRecordNumberOfIdentifiers = 0x01;
        ext.DTCExtendedDataPayload[0] = 0x03;
        dtc.DTCExtendedDataLength[0] = 0x01;

        charon_StoredDataTransmissionFunctionalUnit_writeDTCToNvm(dtc, snap, stored, ext);
    }

    /* DTC 6: C100617 - Battery Volt Low  */
    {
        DTC_t dtc;
        memset(&dtc, 0, sizeof(dtc));
        dtc.DTCHighByte = 0xD0;
        dtc.DTCMiddleByte = 0x06;
        dtc.DTCLowByte = 0x17;
        dtc.DTCStatusMask = 0x08;
        dtc.statusOfDTC = 0x08;
        dtc.DTCSeverityMask = 0x80;
        dtc.DTCSeverityMaskRecordHigh = 0x80;
        dtc.DTCSeverityMaskRecordLow = 0x08;
        dtc.FunctionalGroupIdentifier = 0xD0;
        dtc.DTCSettingType = 0x01;

        DTC_SnapshotData_t snap;
        memset(&snap, 0, sizeof(snap));
        snap.DTCSnapshotDataRecordNumberOfIdentifiers = 0x01;
        snap.DTCSnapshotDataPayload[0] = 0x12;
        snap.DTCSnapshotDataPayload[1] = 0x34;
        dtc.DTCSnapshotLength[0] = 0x02;

        DTC_StoredData_t stored;
        memset(&stored, 0, sizeof(stored));
        stored.DTCStoredDataRecordNumberOfIdentifiers = 0x01;
        stored.DTCStoredDataPayload[0] = 0xEE;
        dtc.DTCStoredDataLength[0] = 0x01;

        DTC_ExtendedData_t ext;
        memset(&ext, 0, sizeof(ext));
        ext.DTCExtendedDataRecordNumberOfIdentifiers = 0x01;
        ext.DTCExtendedDataPayload[0] = 0x03;
        dtc.DTCExtendedDataLength[0] = 0x01;

        charon_StoredDataTransmissionFunctionalUnit_writeDTCToNvm(dtc, snap, stored, ext);
    }

    /* DTC 7: C100618 - Can Bus Off  */
    {
        DTC_t dtc;
        memset(&dtc, 0, sizeof(dtc));
        dtc.DTCHighByte = 0xD0;
        dtc.DTCMiddleByte = 0x06;
        dtc.DTCLowByte = 0x18;
        dtc.DTCStatusMask = 0x08;
        dtc.statusOfDTC = 0x08;
        dtc.DTCSeverityMask = 0x80;
        dtc.DTCSeverityMaskRecordHigh = 0x80;
        dtc.DTCSeverityMaskRecordLow = 0x08;
        dtc.FunctionalGroupIdentifier = 0xD0;
        dtc.DTCSettingType = 0x01;

        DTC_SnapshotData_t snap;
        memset(&snap, 0, sizeof(snap));
        snap.DTCSnapshotDataRecordNumberOfIdentifiers = 0x01;
        snap.DTCSnapshotDataPayload[0] = 0x12;
        snap.DTCSnapshotDataPayload[1] = 0x34;
        dtc.DTCSnapshotLength[0] = 0x02;

        DTC_StoredData_t stored;
        memset(&stored, 0, sizeof(stored));
        stored.DTCStoredDataRecordNumberOfIdentifiers = 0x01;
        stored.DTCStoredDataPayload[0] = 0xEE;
        dtc.DTCStoredDataLength[0] = 0x01;

        DTC_ExtendedData_t ext;
        memset(&ext, 0, sizeof(ext));
        ext.DTCExtendedDataRecordNumberOfIdentifiers = 0x01;
        ext.DTCExtendedDataPayload[0] = 0x03;
        dtc.DTCExtendedDataLength[0] = 0x01;

        charon_StoredDataTransmissionFunctionalUnit_writeDTCToNvm(dtc, snap, stored, ext);
    }

    /* DTC 8: C100619 - SysPrs Sig Lost  */
    {
        DTC_t dtc;
        memset(&dtc, 0, sizeof(dtc));
        dtc.DTCHighByte = 0xD0;
        dtc.DTCMiddleByte = 0x06;
        dtc.DTCLowByte = 0x19;
        dtc.DTCStatusMask = 0x08;
        dtc.statusOfDTC = 0x08;
        dtc.DTCSeverityMask = 0x80;
        dtc.DTCSeverityMaskRecordHigh = 0x80;
        dtc.DTCSeverityMaskRecordLow = 0x08;
        dtc.FunctionalGroupIdentifier = 0xD0;
        dtc.DTCSettingType = 0x01;

        DTC_SnapshotData_t snap;
        memset(&snap, 0, sizeof(snap));
        snap.DTCSnapshotDataRecordNumberOfIdentifiers = 0x01;
        snap.DTCSnapshotDataPayload[0] = 0x12;
        snap.DTCSnapshotDataPayload[1] = 0x34;
        dtc.DTCSnapshotLength[0] = 0x02;

        DTC_StoredData_t stored;
        memset(&stored, 0, sizeof(stored));
        stored.DTCStoredDataRecordNumberOfIdentifiers = 0x01;
        stored.DTCStoredDataPayload[0] = 0xEE;
        dtc.DTCStoredDataLength[0] = 0x01;

        DTC_ExtendedData_t ext;
        memset(&ext, 0, sizeof(ext));
        ext.DTCExtendedDataRecordNumberOfIdentifiers = 0x01;
        ext.DTCExtendedDataPayload[0] = 0x03;
        dtc.DTCExtendedDataLength[0] = 0x01;

        charon_StoredDataTransmissionFunctionalUnit_writeDTCToNvm(dtc, snap, stored, ext);
    }

}

void charon_NvmDriver_init(void)
{
    /* Initialize parameter store with 2 flash sectors (4KB total wear-leveling pool) */
    SYN_Status status = syn_param_init(&s_nvmStore, FLASH_PARAM_START, 2, sizeof(NvmEmulator_MemorySpace));

    //if (status == SYN_OK)
    {
        /* Load latest persistent record */
        if (syn_param_load(&s_nvmStore, NvmEmulator_MemorySpace) == SYN_OK)
        {
            DTC_header_t *hdr = (DTC_header_t *)charon_NvmDriver_getNvmAddress_for_DTC(0, true);
            if (hdr->iniDone == 0xDEADBEEF && hdr->currentDTCCounter > 0)
            {
                charon_NvmDriver_reanchorPointers();
                s_nvmInitialized = true;
                return;
            }
        }
    }
    /* First boot, blank flash, or unseeded table: format default tables and seed initial demonstration DTCs */
    memset(NvmEmulator_MemorySpace, 0, sizeof(NvmEmulator_MemorySpace));
    s_nvmInitialized = true;
    charon_NvmDriver_seedDefaultDTCs();
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

