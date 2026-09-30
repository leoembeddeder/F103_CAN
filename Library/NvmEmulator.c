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
    if (hdr->iniDone != NVM_SCHEMA_MAGIC)
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

typedef struct {
    uint8_t high;
    uint8_t mid;
    uint8_t low;
    uint8_t prio;
    const char *name;
    const char *desc;
} OEM_DTC_Entry_t;

static const OEM_DTC_Entry_t s_oemDtcList[AMOUNT_OF_DTC] = {
    /*  1 | U300614 */ { 0xF0, 0x06, 0x14, 2, "ASW_DTC_BatteryVoltHighWarn", "Battery voltage too high warning" },
    /*  2 | U300615 */ { 0xF0, 0x06, 0x15, 3, "ASW_DTC_BatteryVoltLowWarn", "Battery voltage too low warning" },
    /*  3 | C100616 */ { 0xD0, 0x06, 0x16, 2, "ASW_DTC_BatteryVoltHigh", "Battery voltage high" },
    /*  4 | C100617 */ { 0xD0, 0x06, 0x17, 3, "ASW_DTC_BatteryVoltLow", "Battery voltage low" },
    /*  5 | C100618 */ { 0xD0, 0x06, 0x18, 2, "ASW_DTC_CanBusOff", "CAN Busoff" },
    /*  6 | C100619 */ { 0xD0, 0x06, 0x19, 3, "ASW_DTC_SysPrs_SigLost", "SysPrs Signal Lost" },
    /*  7 | C100620 */ { 0xD0, 0x06, 0x20, 2, "ASW_DTC_SysPrs_OverLimit", "SysPrs Over Limit" },
    /*  8 | C100621 */ { 0xD0, 0x06, 0x21, 3, "ASW_DTC_SysPrs_JumpErr", "SysPrs Jump Error" },
    /*  9 | C100622 */ { 0xD0, 0x06, 0x22, 2, "ASW_DTC_AccuPrs_SigLost", "AccuPrs Signal Lost" },
    /* 10 | C100623 */ { 0xD0, 0x06, 0x23, 3, "ASW_DTC_AccuPrs_OverLimit", "AccuPrs Over Limit" },
    /* 11 | C100624 */ { 0xD0, 0x06, 0x24, 2, "ASW_DTC_AccuPrs_JumpErr", "AccuPrs Jump Error" },
    /* 12 | C100625 */ { 0xD0, 0x06, 0x25, 3, "ASW_DTC_PfsPrs_SigLost", "PfsPrs Signal Lost" },
    /* 13 | C100626 */ { 0xD0, 0x06, 0x26, 2, "ASW_DTC_PfsPrs_OverLimit", "PfsPrs Over Limit" },
    /* 14 | C100627 */ { 0xD0, 0x06, 0x27, 3, "ASW_DTC_PfsPrs_JumpErr", "PfsPrs Jump Error" },
    /* 15 | C100628 */ { 0xD0, 0x06, 0x28, 2, "ASW_DTC_PipeLineErrFL", "PipeLine Error FL" },
    /* 16 | C100629 */ { 0xD0, 0x06, 0x29, 3, "ASW_DTC_PipeLineErrFR", "PipeLine Error FR" },
    /* 17 | C100630 */ { 0xD0, 0x06, 0x30, 2, "ASW_DTC_PipeLineErrRL", "PipeLine Error RL" },
    /* 18 | C100631 */ { 0xD0, 0x06, 0x31, 3, "ASW_DTC_PipeLineErrRR", "PipeLine Error RR" },
    /* 19 | C100632 */ { 0xD0, 0x06, 0x32, 2, "ASW_DTC_WssFL_Err", "Wheel Speed Sensor FL Error" },
    /* 20 | C100633 */ { 0xD0, 0x06, 0x33, 3, "ASW_DTC_WssFR_Err", "Wheel Speed Sensor FR Error" },
    /* 21 | C100634 */ { 0xD0, 0x06, 0x34, 2, "ASW_DTC_WssRL_Err", "Wheel Speed Sensor RL Error" },
    /* 22 | C100635 */ { 0xD0, 0x06, 0x35, 3, "ASW_DTC_WssRR_Err", "Wheel Speed Sensor RR Error" },
    /* 23 | C100636 */ { 0xD0, 0x06, 0x36, 2, "ASW_DTC_AccuLowPrs_Err", "Accumulator Low Pressure" },
    /* 24 | C100637 */ { 0xD0, 0x06, 0x37, 3, "ASW_DTC_AccuPrsHoldErr", "Accumulator Pressure Hold Error" },
    /* 25 | C100638 */ { 0xD0, 0x06, 0x38, 2, "ASW_DTC_PconErr", "Pressure Control Error" },
    /* 26 | C100639 */ { 0xD0, 0x06, 0x39, 3, "ASW_DTC_AccuPump_DG", "Accumulator Pump Degraded" },
    /* 27 | C100640 */ { 0xD0, 0x06, 0x40, 2, "ASW_DTC_PumpMotLockedRotFault", "Pump Motor Locked Rotor Fault" },
    /* 28 | C100641 */ { 0xD0, 0x06, 0x41, 3, "ASW_DTC_AccuPump_Err", "Accumulator Pump Error" },
    /* 29 | C100642 */ { 0xD0, 0x06, 0x42, 2, "ASW_DTC_PTS_Err", "Pedal Travel Sensor Error" },
    /* 30 | C100643 */ { 0xD0, 0x06, 0x43, 3, "ASW_DTC_PTS_Chanl_1Err", "PTS Channel 1 Error" },
    /* 31 | C100644 */ { 0xD0, 0x06, 0x44, 2, "ASW_DTC_PTS_Chanl_2Err", "PTS Channel 2 Error" },
    /* 32 | C100645 */ { 0xD0, 0x06, 0x45, 3, "ASW_DTC_MC_OutleakFault", "Master Cylinder Out-Leak Fault" },
    /* 33 | C100646 */ { 0xD0, 0x06, 0x46, 2, "ASW_DTC_MC_InleakFault", "Master Cylinder In-Leak Fault" },
    /* 34 | C100647 */ { 0xD0, 0x06, 0x47, 3, "ASW_DTC_IMUyawrateSignalErrLevel", "IMU Yaw Rate Signal Error" },
    /* 35 | C100648 */ { 0xD0, 0x06, 0x48, 2, "ASW_DTC_IMUaySignalErrLevel", "IMU Ay Signal Error" },
    /* 36 | C100649 */ { 0xD0, 0x06, 0x49, 3, "ASW_DTC_IMUaxSignalErrLevel", "IMU Ax Signal Error" },
    /* 37 | C100650 */ { 0xD0, 0x06, 0x50, 2, "ASW_DTC_SASSignalErrLevel", "SAS Signal Error" },
    /* 38 | C100651 */ { 0xD0, 0x06, 0x51, 3, "ASW_DTC_BrkConValveDriverFault", "Brake Control Valve Driver Fault" },
    /* 39 | C100652 */ { 0xD0, 0x06, 0x52, 2, "ASW_DTC_WhelPrsConValveDriverFault", "Wheel Pressure Valve Driver Fault" },
    /* 40 | C100653 */ { 0xD0, 0x06, 0x53, 3, "ASW_DTC_ISV1_Fault", "ISV1 Valve Fault" },
    /* 41 | C100654 */ { 0xD0, 0x06, 0x54, 2, "ASW_DTC_ISV1_OverTempWarn", "ISV1 Over Temperature Warning" },
    /* 42 | C100655 */ { 0xD0, 0x06, 0x55, 3, "ASW_DTC_ISV2_Fault", "ISV2 Valve Fault" },
    /* 43 | C100656 */ { 0xD0, 0x06, 0x56, 2, "ASW_DTC_ISV2_OverTempWarn", "ISV2 Over Temperature Warning" },
    /* 44 | C100657 */ { 0xD0, 0x06, 0x57, 3, "ASW_DTC_ISV3_Fault", "ISV3 Valve Fault" },
    /* 45 | C100658 */ { 0xD0, 0x06, 0x58, 2, "ASW_DTC_ISV3_OverTempWarn", "ISV3 Over Temperature Warning" },
    /* 46 | C100659 */ { 0xD0, 0x06, 0x59, 3, "ASW_DTC_ISV4_Fault", "ISV4 Valve Fault" },
    /* 47 | C100660 */ { 0xD0, 0x06, 0x60, 2, "ASW_DTC_ISV4_OverTempWarn", "ISV4 Over Temperature Warning" },
    /* 48 | C100661 */ { 0xD0, 0x06, 0x61, 3, "ASW_DTC_PAV_Fault", "PAV Valve Fault" },
    /* 49 | C100662 */ { 0xD0, 0x06, 0x62, 2, "ASW_DTC_PAV_OverTempWarn", "PAV Over Temperature Warning" },
    /* 50 | C100663 */ { 0xD0, 0x06, 0x63, 3, "ASW_DTC_PRV_Fault", "PRV Valve Fault" },
    /* 51 | C100664 */ { 0xD0, 0x06, 0x64, 2, "ASW_DTC_PRV_OverTempWarn", "PRV Over Temperature Warning" },
    /* 52 | C100665 */ { 0xD0, 0x06, 0x65, 3, "ASW_DTC_BAV_Fault", "BAV Valve Fault" },
    /* 53 | C100666 */ { 0xD0, 0x06, 0x66, 2, "ASW_DTC_CSV_Fault", "CSV Valve Fault" },
    /* 54 | C100667 */ { 0xD0, 0x06, 0x67, 3, "ASW_DTC_SSV_Fault", "SSV Valve Fault" },
    /* 55 | C100668 */ { 0xD0, 0x06, 0x68, 2, "ASW_DTC_USV_Fault", "USV Valve Fault" },
    /* 56 | C100669 */ { 0xD0, 0x06, 0x70, 2, "ASW_DTC_SASangErr", "Steering Angle Sensor Error" },
    /* 57 | C100670 */ { 0xD0, 0x06, 0x71, 3, "ASW_DTC_BrkSwitchErr", "Brake Switch Error" },
    /* 58 | C100671 */ { 0xD0, 0x06, 0x72, 2, "ASW_DTC_PowerSysErr", "Power System Error" },
    /* 59 | C100672 */ { 0xD0, 0x06, 0x73, 3, "ASW_DTC_AccPosErr", "Accelerator Position Error" },
    /* 60 | C100673 */ { 0xD0, 0x06, 0x74, 2, "ASW_DTC_GearPosErr", "Gear Position Error" },
    /* 61 | C100674 */ { 0xD0, 0x06, 0x75, 3, "ASW_DTC_BtrErr", "Battery Pack Invalid" },
    /* 62 | C100675 */ { 0xD0, 0x06, 0x76, 2, "ASW_DTC_BrkFluidLevel_Low", "Brake Fluid Level Low" },
    /* 63 | C100676 */ { 0xD0, 0x06, 0x77, 3, "ASW_DTC_SeatBltErr", "Seat Belt Signal Error" },
    /* 64 | C100677 */ { 0xD0, 0x06, 0x78, 2, "ASW_DTC_DoorsErr", "Doors Signal Error" },
    /* 65 | C100678 */ { 0xD0, 0x06, 0x79, 3, "ASW_DTC_AdasConnectErr", "ADAS Communication Error" },
    /* 66 | C100679 */ { 0xD0, 0x06, 0x80, 2, "ASW_DTC_ExternalEPBErr", "External EPB Control Error" },
    /* 67 | C100680 */ { 0xD0, 0x06, 0x81, 3, "ASW_DTC_FrontCameraErr", "Front Camera Error" },
    /* 68 | C100681 */ { 0xD0, 0x06, 0x82, 2, "ASW_DTC_Reserved1", "Reserved OEM DTC 1" },
    /* 69 | C100682 */ { 0xD0, 0x06, 0x83, 3, "ASW_DTC_Reserved2", "Reserved OEM DTC 2" },
};

static void charon_NvmDriver_seedDefaultDTCs(void)
{
    charon_DTC_LookupTable_header_SET();

    for (uint16_t i = 0; i < AMOUNT_OF_DTC; i++)
    {
        DTC_t dtc;
        memset(&dtc, 0, sizeof(dtc));
        dtc.DTCHighByte = s_oemDtcList[i].high;
        dtc.DTCMiddleByte = s_oemDtcList[i].mid;
        dtc.DTCLowByte = s_oemDtcList[i].low;
        /* ISO 14229-1 cleared default state: testNotCompletedSinceLastClear | testNotCompletedThisOperationCycle */
        dtc.DTCStatusMask = (UDS_DTC_STATUS_TEST_NOT_COMPLETED_SLC | UDS_DTC_STATUS_TEST_NOT_COMPLETED_TOC);
        dtc.statusOfDTC = dtc.DTCStatusMask;
        dtc.DTCSeverityMask = (s_oemDtcList[i].prio == 2) ? 0x60 : 0x40;
        dtc.DTCSeverityMaskRecordHigh = dtc.DTCSeverityMask;
        dtc.DTCSeverityMaskRecordLow = dtc.DTCStatusMask;
        dtc.FunctionalGroupIdentifier = (dtc.DTCHighByte == 0xF0) ? 0xFE : 0xD0;
        dtc.DTCSettingType = 0x01; /* ControlDTCSetting ON */

        DTC_SnapshotData_t snap;
        memset(&snap, 0, sizeof(snap));

        DTC_StoredData_t stored;
        memset(&stored, 0, sizeof(stored));

        DTC_ExtendedData_t ext;
        memset(&ext, 0, sizeof(ext));

        charon_StoredDataTransmissionFunctionalUnit_writeDTCToNvm(dtc, snap, stored, ext);
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
            if (hdr->iniDone == NVM_SCHEMA_MAGIC && hdr->totalDTCCounter == AMOUNT_OF_DTC)
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
    if ((address >= 0x20000000u) && ((address + size) <= 0x20010000u))
    {
        memcpy((void*)address, data, size);
        return uds_responseCode_PositiveResponse;
    }
	 if (!charon_NvmDriver_checkAddressRange(address, size))
	 {
		 return uds_responseCode_RequestOutOfRange;
	 }
	 memcpy(&NvmEmulator_MemorySpace[address], data, size);
	 return charon_NvmDriver_flush();
}

void charon_NvmDriver_read (uint32_t address, uint8_t* data, uint32_t size)
{
    if ((address >= STM32F103_FLASH_BASE) && ((address + size) <= (STM32F103_FLASH_BASE + (256u * STM32F103_PAGE_SIZE))))
    {
        syn_port_flash_read(address, data, size);
    }
    else if (charon_NvmDriver_checkAddressRange(address, size))
    {
        memcpy(data, &NvmEmulator_MemorySpace[address], size);
    }
    else if ((address >= 0x08000000u && address < 0x08080000u) ||
             (address >= 0x20000000u && address < 0x20010000u))
    {
        memcpy(data, (const void*)address, size);
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

