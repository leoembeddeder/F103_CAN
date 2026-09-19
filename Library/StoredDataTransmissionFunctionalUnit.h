#ifndef CHARON_STOREDDATATRANSMISSIONFUNCTIONALUNIT_H_
#define CHARON_STOREDDATATRANSMISSIONFUNCTIONALUNIT_H_

/* Includes ******************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "uds_types.h"
#include "uds_config.h"

/* Imports *******************************************************************/

/* Macros ********************************************************************/

/** @brief Calc for pad size (windows will always pad an uneven uint8 counter to full uint16 without making sure its clean (crc16 errors)). */
#define PAD_SIZE(x) (4 - (x % 4))


/* Types *********************************************************************/

/** @brief Used to compare which function is requested. */
typedef enum DTCSubfunction_t_public
{
    ISOSAEReserved                                      = 0x00,
    reportNumberOfDTCByStatusMask                       = 0x01,
    reportDTCByStatusMask                               = 0x02,
    reportDTCSnapshotIdentification                     = 0x03,
    reportDTCSnapshotRecordByDTCNumber                  = 0x04,
    reportDTCStoredDataByRecordNumber                   = 0x05,
    reportDTCExtDataRecordByDTCNumber                   = 0x06,
    reportNumberOfDTCBySeverityMaskRecord               = 0x07,
    reportDTCBySeverityMaskRecord                       = 0x08,
    reportSeverityInformationOfDTC                      = 0x09,
    reportSupportedDTC                                  = 0x0A,
    reportFirstTestFailedDTC                            = 0x0B,
    reportFirstConfirmedDTC                             = 0x0C,
    reportMostRecentTestFailedDTC                       = 0x0D,
    reportMostRecentConfirmedDTC                        = 0x0E,
    reportMirrorMemoryDTCByStatusMask                   = 0x0F,
    reportMirrorMemoryDTCExtDataRecordByDTCNumber       = 0x10,
    reportNumberOfMirrorMemoryDTCByStatusMask           = 0x11,
    reportNumberOfEmissionsOBDDTCByStatusMask           = 0x12,
    reportEmissionsOBDDTCByStatusMask                   = 0x13,
    reportDTCFaultDetectionCounter                      = 0x14,
    reportDTCWithPermanentStatus                        = 0x15,
    reportDTCExtDataRecordByRecordNumber                = 0x16,
    reportUserDefMemoryDTCByStatusMask                  = 0x17,
    reportUserDefMemoryDTCSnapshotRecordByDTCNumber     = 0x18,
    reportUserDefMemoryDTCExtDataRecordByDTCNumber      = 0x19,
    reportWWHOBDDTCByMaskRecord                         = 0x42,
    reportWWHOBDDTCWithPermanentStatus                  = 0x55,
    amountOFSubfunctions
}DTCSubfunction_t;

/* OBD Extended Data Format per Issue #58 & #66 */
typedef struct {
    uint8_t fault_occur_counter;
    uint8_t fault_pending_counter;
    uint8_t aged_counter;
    uint8_t ageing_counter;
} OBD_Extended_Data_Format;

/* Snapshot Time & Date Structure Definition */
typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} OBD_Global_Snapshot_DataTime_Format;

/* DTC Global Snapshot Data Structure Definition */
typedef struct {
    uint8_t voltage;           /* Battery supply voltage in 0.1V units (e.g. 120 = 12.0V) */
    uint8_t global_power_mode; /* Global power mode (e.g. 0x01=OFF, 0x02=ACC, 0x03=ON, 0x04=START) */
    OBD_Global_Snapshot_DataTime_Format st_global_snapshot_datatime;
} OBD_Global_Snapshot_Format;


/** @brief Used to determine the save location in userDef functions. */
typedef enum userDefMemoryList       /** @todo USER: For future implementation of DTC 0x17-0x19. Just a dummy atm. */
{
    reserved = 0x00,
    SD_card  = 0x01,

    amountOfDefAddress
}userDefMemoryList_t; 


/** @todo USER: Define your range here. */
/** @brief Used to define the range of these DTCs, used in 0x14 ClearDiagnosticInformation. */
typedef enum manufacturerDefDTC_group_t_private
{
    powertrainGroup                = 0x000100,     /**< min value (ISO 14229-1 S:353). */
    powertrainGroupSTART              = 0x000101,
    powertrainGroupEND                = 0xAFFFFF,

    chassisGroup                   = 0xB00000,
    chassisGroupSTART                 = 0xB00001,
    chassisGroupEND                   = 0xCFFFFF,

    bodyGroup                      = 0xD00000,
    bodyGroupSTART                    = 0xD00001,
    bodyGroupEND                      = 0xEFFFFF,

    networkCommunicationGroup     = 0xF00000,
    networkCommunicationGroupSTART   = 0xF00001,
    networkCommunicationGroupEND     = 0xFFFEFF, /**< max value (ISO 14229-1 S:353). */
}manufacturerDefDTC_group_t;


/** @brief Store snapshot identifier and payload. */
typedef struct
{
    uint8_t DTCSnapshotDataRecordNumberOfIdentifiers;               /**< Used to determine how many identifier are saved */
    uint8_t DTCSnapshotDataPayload[NVM_RESERVED_SIZE_SNAPSHOT];     /**< @todo USER: determine how much memory space should be reserved */
}DTC_SnapshotData_t;                                                /**< @brief Store snapshot identifier and payload. */

/** @brief Store storedData identifier and payload. */
typedef struct
{
    uint8_t DTCStoredDataRecordNumberOfIdentifiers;                 /**< Used to determine how many identifier are saved */
    uint8_t DTCStoredDataPayload[NVM_RESERVED_SIZE_DATARECORD];     /**< @todo USER: determine how much memory space should be reserved */
}DTC_StoredData_t;                                                  /**< @brief Store storedData identifier and payload. */

/** @brief Store extData identifier and payload. */
typedef struct
{
    uint8_t DTCExtendedDataRecordNumberOfIdentifiers;               /**< Used to determine how many identifier are saved */
    uint8_t DTCExtendedDataPayload[NVM_RESERVED_SIZE_EXTENDED];     /**< @todo USER: determine how much memory space should be reserved */
}DTC_ExtendedData_t;                                                /**< @brief Store extData identifier and payload. */

/** @brief DTC type used to store DTC data. */
typedef struct                                                                                                                                 // source:
{
    uint8_t  DTCStatusMask;                                                 /**< Bit 0-8 def:    0 =testFailed, 1 =testFailedThisOperationCycle, 2 =pendingDTC, 3=confirmedDTC,         // [ISO D.2.3 S=356]
                                                                                 4 =testNotCompletedSinceLastClear, 5 =TestFailedSinceLastClear, 6 =testNotCompletedThisOperationCycle
                                                                                 7 =warningIndicatorRequested */
    uint8_t  DTCHighByte;                                                   /**< H + M + L = DTCMaskRecord  */                                                                          // [ISO 11.3.2.3 S=198]
    uint8_t  DTCMiddleByte;                                                 /**< 3-byte value, together represent a unique identification number for a specific diagnostic trouble code */
    uint8_t  DTCLowByte;                                 
    uint8_t  DTCSnapshotRecordNumber[NVM_AMOUNT_OF_SNAPSHOTS];              /**< 0x01-0xFE manufacturer specific usage, 0xFF = report all stored records at once. */          // [ISO 11.3.2.3 S=198]
    uint8_t  DTCStoredDataRecordNumber[NVM_AMOUNT_OF_DATARECORDS];          /**< 0x01-0xFE manufacturer specific usage, 0xFF = report all stored records at once. */          // [ISO 11.3.2.3 S=198]
    uint8_t  DTCExtDataRecordNumber[NVM_AMOUNT_OF_EXTENDED];                /**< 0x01-0x8F manufacturer specific records, 0x90-0xEF legislated OBD records,                   // [ISO 11.3.2.3 S=199]
                                                                                 0xFE report all legislated OBD in a single message, 0xFF report all records. */
    uint8_t  DTCSeverityMaskRecordHigh;                                     /**< = DTCSeverityMask */
    uint8_t  DTCSeverityMaskRecordLow;                                      /**< = DTCStatusMask */
    uint8_t  DTCSeverityMask;                                               /**< Bit 0-4 def: DTC class information, Bit 5-7 def: DTC severity information */                  // [ISO D.3 S=366]
    uint8_t  FunctionalGroupIdentifier;                                     /**< 0x33 = Emission, 0xD0 = Safety, 0xFE = VOBD, 0xFF = ALL */                                    // [ISO D.5 S=369]
    uint32_t MemorySelection;                                               /**< implemented for future use */  

    uint8_t  NumberOfSavedSnapshots;                                        /**< For faster lookup times */
    uint16_t DTCSnapshotLength[NVM_AMOUNT_OF_SNAPSHOTS];                    /**< "0" for default and needs to be written */
    DTC_SnapshotData_t* DTCSnapshotAddress[NVM_AMOUNT_OF_SNAPSHOTS];        /**< Address location is defined by USER */

    uint8_t  NumberOfSavedDataRecords;                                      /**< For faster lookup times */
    uint16_t DTCStoredDataLength[NVM_AMOUNT_OF_DATARECORDS];                /**< "0" for default and needs to be written */
    DTC_StoredData_t* DTCStoredDataAddress[NVM_AMOUNT_OF_DATARECORDS];      /**< Address location is defined by USER */

    uint8_t  NumberOfSavedExtendedData;                                     /**< For faster lookup times */
    uint16_t DTCExtendedDataLength[NVM_AMOUNT_OF_EXTENDED];                 /**< "0" for default and needs to be written */
    DTC_ExtendedData_t* DTCExtendedDataAddress[NVM_AMOUNT_OF_EXTENDED];     /**< Address location is defined by USER */

    uint8_t statusOfDTC;                                                    /**< Should be the same as "DTCStatusMask" but nor clearly defined in ISO */
    uint8_t DTCSettingType;                                                 /**< Used by ControlDTCSetting (0x85) to prevent writing to DTCStatusMask:  0x01 = on, 0x02 = off or used specific */
}DTC_t;                                                                     /**< @brief DTC type used to store DTC data. */

/** @brief Header type used for storage of total counters, addresses and CRC16 check. */
typedef struct
{
    uint16_t totalDTCCounter;                         /**< total DTC counter, does not reset! only when ALL DTC where erased. */
    uint16_t totalSnapshotRecordCounter;              /**< Only 1 Byte to safe the DTCSnapshotRecordNumber. */
    uint16_t totalDataRecordCounter;                  /**< Only 1 Byte to safe the DTCStoredDataRecordNumber. */ 
    uint16_t totalExtDataRecordCounter;               /**< Only 1 Byte to safe the DTCExtDataRecordNumber. */

    uint16_t currentDTCCounter;                       /**< Current DTC counter. */
    uint16_t currentSnapshotRecordCounter;            /**< Only 1 Byte to safe the DTCSnapshotRecordNumber. */
    uint16_t currentDataRecordCounter;                /**< Only 1 Byte to safe the DTCStoredDataRecordNumber. */ 
    uint16_t currentExtDataRecordCounter;             /**< Only 1 Byte to safe the DTCExtDataRecordNumber. */

    uint16_t deletedDTCCounter;                       /**< Its possible to delete single or groups of DTC, when this happens its saved here. */
    uint16_t deletedSnapshotRecordCounter;            /**< Its possible to delete single or groups of snapshots, when this happens its saved here. */
    uint16_t deletedDataRecordCounter;                /**< Its possible to delete single or groups of dataRecords, when this happens its saved here. */
    uint16_t deletedExtDataRecordCounter;             /**< Its possible to delete single or groups of extData, when this happens its saved here. */

    uint32_t FirstFailedDTC;                            /**< Address to the ... | "0" as default. */ 
    uint32_t FirstConfirmedDTC;                         /**< Address to the ... | "0" as default. */
    uint32_t MostRecentTestFailed;                      /**< Address to the ... | "0" as default. */
    uint32_t MostRecentConfirmedDTC;                    /**< Address to the ... | "0" as default. */

    uint16_t sizeOfDTCLookupTable;                    /**< For easy read purpose. */
    uint16_t sizeOfSnapshotLookupTable;               /**< For easy read purpose. */
    uint16_t sizeOfStoredDataLookupTable;             /**< For easy read purpose. */
    uint16_t sizeOfExtDataLookupTable;                /**< For easy read purpose. */

    uint8_t  nvmDTCLookupTable[(AMOUNT_OF_DTC / 8) + 1];                  /**< Bitwise Nvm slot handling. */
    uint8_t  selfPaddingDTC[PAD_SIZE(AMOUNT_OF_DTC)];                      /**< Windows will pad even a packed struct on uneven byte count, this prevents random values. */
    uint8_t  nvmSnapshotLookupTable[(AMOUNT_OF_SNAPSHOT / 8) + 1];        /**< Bitwise Nvm slot handling. */
    uint8_t  selfPaddingSNAP[PAD_SIZE(AMOUNT_OF_SNAPSHOT)];                /**< Windows will pad even a packed struct on uneven byte count, this prevents random values. */
    uint8_t  nvmStoredDataLookupTable[(AMOUNT_OF_STOREDDATA / 8) + 1];    /**< Bitwise Nvm slot handling. */
    uint8_t  selfPaddingSTORED[PAD_SIZE(AMOUNT_OF_STOREDDATA)];            /**< Windows will pad even a packed struct on uneven byte count, this prevents random values. */
    uint8_t  nvmExtDataLookupTable[(AMOUNT_OF_EXTENDEDDATA / 8) + 1];     /**< Bitwise Nvm slot handling. */
    uint8_t  selfPaddingEXT[PAD_SIZE(AMOUNT_OF_EXTENDEDDATA)];             /**< Windows will pad even a packed struct on uneven byte count, this prevents random values. */

    uint32_t iniDone;                                /**< When the System is completely reset, the storage needs a clean up. */

    uint16_t CRC16Checksum;                            /**< For CRC16 check. */
}DTC_header_t;                                          /**< @brief Header type used for storage of total counters, addresses and CRC16 check. */


/* Interfaces  ***************************************************************/

/**
 * @brief Updates already in NVM defined DTC. Changes in status or saved packaged data like snapshots.
 * 
 * @param DTCInput              Prepared DTC_t to change a requested DTC.
 */
void charon_StoredDataTransmissionFunctionalUnit_DTCSettingTypeUpdater (DTC_t DTCInput);

/**
 * @brief UDS ISO 14229-1 SE 2013-03-15.
 * SID: 0x14
 *
 * @param receiveBuffer         Payload.
 * @param receiveBufferSize     Payload Size.
 * @return uds_responseCode_t
 */
uds_responseCode_t charon_StoredDataTransmissionFunctionalUnit_ClearDiagnosticInformation (const uint8_t * receiveBuffer, uint32_t receiveBufferSize);

/**
 * @brief Big switch case to change in the requested DTC subfunction.
 * UDS ISO 14229-1 SE 2013-03-15
 * SID: 0x19
 *
 * @param receiveBuffer         Payload.
 * @param receiveBufferSize     Payload Size.
 * @return uds_responseCode_t
 */
uds_responseCode_t charon_StoredDataTransmissionFunctionalUnit_ReadDtcInformation (const uint8_t * receiveBuffer, uint32_t receiveBufferSize);

/**
 * @brief Used to input or change a DTC in the Nvm.
 * 
 * @param DTCInput              Data package to be saved in Nvm, new or changes.
 * @param snapPayload           Snapshot package to be saved in Nvm.
 * @param storedPayload         StoredData package to be saved in Nvm.
 * @param extPayload            ExtData package to be saved in Nvm.
 * @return uds_responseCode_t 
 */
uds_responseCode_t charon_StoredDataTransmissionFunctionalUnit_writeDTCToNvm (DTC_t DTCInput, DTC_SnapshotData_t snapPayload, DTC_StoredData_t storedPayload, DTC_ExtendedData_t extPayload);

/**
 * @brief Pointer to the headers requested mirror address.
 *  
 * @return DTC_header_t*        The requested mirror address.
 */
DTC_header_t* charon_StoredDataTransmissionFunctionalUnit_getMirrorHeaderAddress(void);

/**
 * @brief Pointer to the headers requested user def. address.
 * 
 * @return DTC_header_t*        The requested user def. address.
 */
DTC_header_t* charon_StoredDataTransmissionFunctionalUnit_getUserDefHeaderAddress(uint8_t MemorySelection);

/**
 * @brief Returns the total amount saved in the Mirror Nvm header.
 * 
 * @return uint16_t             DTCCounter.
 */
uint16_t charon_StoredDataTransmissionFunctionalUnit_get_total_mirrorDTCCounter (void);

/**
 * @brief Returns the total amount saved in the Mirror Nvm header.
 * 
 * @return uint8_t              snapshotCounter.
 */
uint8_t charon_StoredDataTransmissionFunctionalUnit_get_total_mirrorSnapshotCounter (void);

/**
 * @brief Returns the total amount saved in the Mirror Nvm header.
 * 
 * @return uint8_t              dataRecordCounter.
 */
uint8_t charon_StoredDataTransmissionFunctionalUnit_get_total_mirrorDataRecordCounter (void);

/**
 * @brief Returns the total amount saved in the Mirror Nvm header. 
 * 
 * @return uint8_t              extDataCounter.
 */
uint8_t charon_StoredDataTransmissionFunctionalUnit_get_total_mirrorExtDataRecordCounter (void);

/**
 * @brief Returns the total amount saved in the user def. Nvm header.
 * 
 * @param MemorySelection       wanted address.
 * @return uint16_t             DTCCounter.
 */
uint16_t charon_StoredDataTransmissionFunctionalUnit_get_total_userDefDTCCounter (uint8_t MemorySelection);

/**
 * @brief Returns the total amount saved in the user def. Nvm header.
 * 
 * @param MemorySelection       wanted address.
 * @return uint8_t              snapshotCounter.
 */
uint8_t charon_StoredDataTransmissionFunctionalUnit_get_total_userDefSnapshotCounter (uint8_t MemorySelection);

/**
 * @brief Returns the total amount saved in the user def. Nvm header.
 * 
 * @param MemorySelection       wanted address.
 * @return uint8_t              dataRecordCounter.
 */
uint8_t charon_StoredDataTransmissionFunctionalUnit_get_total_userDefDataRecordCounter (uint8_t MemorySelection);

/**
 * @brief Returns the total amount saved in the user def. Nvm header. 
 * 
 * @param MemorySelection       wanted address.
 * @return uint8_t              extDataCounter.
 */
uint8_t charon_StoredDataTransmissionFunctionalUnit_get_total_userDefExtDataRecordCounter (uint8_t MemorySelection);

/** @brief Update of the CRC16 check sum. */
void charon_StoredDataTransmissionFunctionalUnit_CRC16_update (void);

#endif 