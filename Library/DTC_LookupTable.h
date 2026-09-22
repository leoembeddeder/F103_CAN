#ifndef CHARON_DTC_LookupTable_H_
#define CHARON_DTC_LookupTable_H_

/* Includes ******************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "StoredDataTransmissionFunctionalUnit.h"


/* Constants *****************************************************************/

/* Macros ********************************************************************/

/** @brief This Bit defines if the "Test failed" Bit of the DTC status Mask is available for requests. */
#define DTC_Enable_testFailed_Status_Bit                            ((uint8_t)0x01) 
/** @brief This Bit defines if the "Test failed this operation Cycle" Bit of the DTC status Mask is available for requests. */
#define DTC_Enable_testFailedThisOperationCycle_Status_Bit          ((uint8_t)0x01) 
/** @brief This Bit defines if the "Pending DTC" Bit of the DTC status Mask is available for requests. */
#define DTC_Enable_pendingDTC_Status_Bit                            ((uint8_t)0x01) 
/** @brief This Bit defines if the "confirmed DTC" Bit of the DTC status Mask is available for requests. */
#define DTC_Enable_confirmedDTC_Status_Bit                          ((uint8_t)0x01) 
/** @brief This Bit defines if the "test Not Completed Since Last Clear" Bit of the DTC status Mask is available for requests. */
#define DTC_Enable_testNotCompletedSinceLastClear_Status_Bit        ((uint8_t)0x01) 
/** @brief This Bit defines if the "Test Failed Since Last Clear" Bit of the DTC status Mask is available for requests. */
#define DTC_Enable_testFailedSinceLastClear_Status_Bit              ((uint8_t)0x01) 
/** @brief This Bit defines if the "test Not Completed This Operation Cycle" Bit of the DTC status Mask is available for requests. */
#define DTC_Enable_testNotCompletedThisOperationCycle_Status_Bit    ((uint8_t)0x01) 
/** @brief This Bit defines if the "Warning Indicator Requested" Bit of the DTC status Mask is available for requests. */
#define DTC_Enable_warningIndicatorRequested_Status_Bit             ((uint8_t)0x01) 

/** @brief Defines the Format Identifier for the ISO 14229-1 DTC_Format */
#define DTC_Format_Identifier                                       ((uint8_t)0x01)


/* Types *********************************************************************/

/* Interfaces ****************************************************************/

/**
 * @brief Returns DTC status availability mask.
 * 
 * @return uint8_t          Contains the DTC status availability Mask.
 */
uint8_t charon_getDTCStatusAvailabilityMask(void);

/**
 * @brief Searches the NVM for a matching DTCnumber und will return this DTC address.
 * 
 * @param DTCHighByte       Part of the 24 bit DTCnumber.
 * @param DTCMiddleByte     Part of the 24 bit DTCnumber.
 * @param DTCLowByte        Part of the 24 bit DTCnumber.
 * @param mirror            Only used if a mirror subfunction was requested by the user.
 * @param userDefMemory     Only used if a userDefMemory subfunction was requested by the user.
 * @param MemorySelection   Only needed by userDefMemory.
 * @return DTC_t*           Pointer to the DTC requested address.
 */
DTC_t* charon_getDTCLookupByDTCNumber (uint8_t DTCHighByte, uint8_t DTCMiddleByte, uint8_t DTCLowByte, bool mirror, bool userDefMemory, uint8_t MemorySelection);

/**
 * @brief Searches the NVM for a matching StatusMask and will return this DTC address.
 * 
 * @param StatusMask        The statusmask put in by the user.
 * @param offset            When the first DTC is given back the next is picked using this value.
 * @param mirror            Only used if a mirror subfunction was requested by the user.
 * @return DTC_t*           Pointer to the DTC requested address.
 */
DTC_t* charon_getDTCLookupByStatusMask (uint8_t StatusMask, uint16_t offset, bool mirror, bool userDefMemory, uint8_t MemorySelection);

/**
 * @brief Searches the NVM for all matching DTC and will return the amount.
 * 
 * @param StatusMask        The statusmask put in by the user.
 * @param mirror            Only used if a mirror subfunction was requested by the user.
 * @param userDefMemory     Only used if a userDefMemory subfunction was requested by the user.
 * @param MemorySelection   Only needed by userDefMemory
 * @return uint32_t         Amount of DTC.
 */
uint32_t charon_getDTCCountByStatusMask (uint8_t StatusMask, bool mirror, bool userDefMemory, uint8_t MemorySelection);

/**
 * @brief Searches the NVM for saved Snapshot and will return found DTC address.
 * 
 * @param offset            When the first DTC is given back the next is picked using this value.
 * @return DTC_t*           Pointer to the DTC requested address.
 */
DTC_t* charon_getDTCSnapshotAddress (uint16_t offset);

/**
 * @brief Searches the NVM for DTCStoredDataRecordNumber and will return found DTC address.
 * 
 * @param RecordNumber      The User inputted requested number.
 * @return DTC_t*           Pointer to the DTC requested address.
 */
DTC_t* charon_getDTCLookupByRecordNumber (uint8_t RecordNumber);

/**
 * @brief Prints all DataRecords, only used when 0xFF was input by user.
 * 
 * @param receiveBuffer     Payload.
 * @param receiveBufferSize Payload size.
 */
uds_responseCode_t charon_getDTCLookup_reportAllDataRecords (const uint8_t * receiveBuffer, uint32_t receiveBufferSize);

/**
 * @brief Searches the NVM for all matching DTC and will return the amount.
 * 
 * @param DTCSeverityMask   The severity mask put in by the user.
 * @param DTCStatusMask     The status mask put in by the user.
 * @return uint32_t         Amount of DTCAmount of DTC.
 */
uint32_t charon_getDTCCountBySeverityMask (uint8_t DTCSeverityMask, uint8_t DTCStatusMask);

/**
 * @brief Searches the NVM for a matching StatusMask AND SeverityMask, will return this DTC address.
 * 
 * @param DTCSeverityMask   The severity mask put in by the user.
 * @param DTCStatusMask     The status mask put in by the user.
 * @param offset            When the first DTC is given back the next is picked using this value.
 * @return DTC_t*           Pointer to the DTC requested address.
 */
DTC_t* charon_getDTCSeverityMaskAddress (uint8_t DTCSeverityMask, uint8_t DTCStatusMask, uint16_t offset);

/**
 * @brief Searches the NVM for ExtData with a matching RecordNumber and returns found DTC address.
 * 
 * @param RecordNumber      The User inputted requested number.
 * @param offset            When the first DTC is given back the next is picked using this value.
 * @param p_index_found_at  Index is needed for response building.
 * @return DTC_t*           Pointer to the DTC requested address.
 */
DTC_t* charon_getDTCExtDataAddressByRecordNumber (uint8_t RecordNumber, uint16_t offset, uint32_t * p_index_found_at);

/**
 * @brief Searches the NVM for all ExtData with a matching RecordNumber and returns the amount.
 * 
 * @param RecordNumber      The User inputted requested number.
 * @return uint8_t          Amount of found DTC.
 */
uint8_t charon_getDTCExtDataCountByRecordNumber (uint8_t RecordNumber);

/**
 * @brief Use by subfunction 0x14, will delete DTC based on input and save the changes.
 * 
 * @param DTCHighByte       Part of the 24 bit DTCnumber.
 * @param DTCMiddleByte     Part of the 24 bit DTCnumber.
 * @param DTCLowByte        Part of the 24 bit DTCnumber.
 * @param start             of defined range.
 * @param end               of defined range.
 * @param vobd              Flag for processing.
 * @param safety            Flag for processing.
 * @param emission          Flag for processing.
 * @param delAll           Flag for processing.
 */
void charon_deleteDTC (uint8_t DTCHighByte, uint8_t DTCMiddleByte, uint8_t DTCLowByte, uint32_t start, uint32_t end, bool vobd, bool safety, bool emission, bool delAll);

/** @brief Safety function for Nvm to not read non valid data. USER: NEEDS TO BE CALLED ONCE WHEN EVER THE NVM IS RESET! */
void charon_DTC_LookupTable_header_SET (void);

/* ===========================================================================
 * AUTOSAR Dem / ISO 14229-1 Fault Debounce & Application Logic Handlers
 * (Issue #71 & #66)
 * =========================================================================== */

#define UDS_DTC_STATUS_TEST_FAILED                  (0x01U)
#define UDS_DTC_STATUS_TEST_FAILED_THIS_CYCLE       (0x02U)
#define UDS_DTC_STATUS_PENDING                      (0x04U)
#define UDS_DTC_STATUS_CONFIRMED                    (0x08U)
#define UDS_DTC_STATUS_TEST_NOT_COMPLETED_SLC       (0x10U)
#define UDS_DTC_STATUS_TEST_FAILED_SLC              (0x20U)
#define UDS_DTC_STATUS_TEST_NOT_COMPLETED_TOC       (0x40U)
#define UDS_DTC_STATUS_WARNING_INDICATOR_REQ        (0x80U)
#define UDS_DTC_STATUS_CLEARED                      (0x50U) /* 0x10 | 0x40 */

/**
 * @brief ISO 14229-1 / AUTOSAR Dem counter-based fault debouncing engine.
 * Steps fault detection counter (+16 on failed up to +127, -16 on passed down to -128).
 * Updates status bits, aging counter, aged counter, and commits to wear-leveled flash.
 *
 * @param dtc_number 24-bit DTC identifier (e.g. 0x010016, 0xD00616).
 * @param failed     true if fault condition detected, false if test passed.
 * @return true if DTC was found and processed, false otherwise.
 */
bool uds_dtc_app_report_event(uint32_t dtc_number, bool failed);

/**
 * @brief Directly sets the status byte and fault state of a DTC.
 *
 * @param dtc_number 24-bit DTC identifier.
 * @param status     DTC status mask byte to set.
 * @return true if successful, false otherwise.
 */
bool uds_dtc_app_set_fault(uint32_t dtc_number, uint8_t status);

/**
 * @brief Clears the fault state of a DTC, resetting status to cleared state (0x50).
 *
 * @param dtc_number 24-bit DTC identifier.
 * @return true if successful, false otherwise.
 */
bool uds_dtc_app_clear_fault(uint32_t dtc_number);

/**
 * @brief Sets the global snapshot record for a specified DTC.
 *
 * @param dtc_number 24-bit DTC identifier.
 * @param snapshot   Pointer to OBD_Global_Snapshot_Format structure.
 * @return true if successful, false otherwise.
 */
bool uds_dtc_app_set_global_snapshot(uint32_t dtc_number, const OBD_Global_Snapshot_Format *snapshot);

/**
 * @brief Reads the global snapshot record of a specified DTC.
 *
 * @param dtc_number 24-bit DTC identifier.
 * @param snapshot   Pointer to OBD_Global_Snapshot_Format buffer to receive data.
 * @return true if successful, false otherwise.
 */
bool uds_dtc_app_get_global_snapshot(uint32_t dtc_number, OBD_Global_Snapshot_Format *snapshot);

/**
 * @brief Sets the extended data record (occurrence, pending, aged, ageing) for a specified DTC.
 *
 * @param dtc_number 24-bit DTC identifier.
 * @param ext_data   Pointer to OBD_Extended_Data_Format structure.
 * @return true if successful, false otherwise.
 */
bool uds_dtc_app_set_extended_data(uint32_t dtc_number, const OBD_Extended_Data_Format *ext_data);

/**
 * @brief Reads the extended data record of a specified DTC.
 *
 * @param dtc_number 24-bit DTC identifier.
 * @param ext_data   Pointer to OBD_Extended_Data_Format buffer to receive data.
 * @return true if successful, false otherwise.
 */
bool uds_dtc_app_get_extended_data(uint32_t dtc_number, OBD_Extended_Data_Format *ext_data);

/* Charon naming aliases for convenience */
#define charon_DTC_reportEvent                      uds_dtc_app_report_event
#define charon_DTC_setFault                         uds_dtc_app_set_fault
#define charon_DTC_clearFault                       uds_dtc_app_clear_fault
#define charon_DTC_setGlobalSnapshot                uds_dtc_app_set_global_snapshot
#define charon_DTC_getGlobalSnapshot                uds_dtc_app_get_global_snapshot
#define charon_DTC_setExtendedData                  uds_dtc_app_set_extended_data
#define charon_DTC_getExtendedData                  uds_dtc_app_get_extended_data

#endif 