#include <stdint.h>
#include "uds_types.h"
#include "DataLookupTable.h"
#include "ServiceLookupTable.h"
#include "SessionAndServiceControl.h"

/* Imports *******************************************************************/

/* Constants *****************************************************************/

/* Static storage buffers for DIDs */
static uint8_t s_did_test_did[10]          = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
static uint8_t s_did_test_did2[10]         = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA};
static uint8_t s_did_test_periodic[10]     = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A};
static uint8_t s_did_test_periodic1[10]    = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A};
static uint8_t s_did_test_dynamic[10]      = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A};
static uint8_t s_did_f181_spare_part[16]   = "STM32F103HD-PART";
static uint8_t s_did_f182_boot_sw[16]      = "BOOT-AB-CRC32-V1";
static uint8_t s_did_f183_ecu_sw[16]       = "SW-CHARON-02.01 ";
static uint8_t s_did_f184_app_sw[16]       = "APP-FW-V2.1.000 ";
static uint8_t s_did_f185_ecu_hw[16]       = "HW-STM32F103RC-1";
static uint8_t s_did_f186_session[1]       = {0x01};
static uint8_t s_did_f18a_supplier[16]     = "LEOEMBEDDED-SYS ";
static uint8_t s_did_f190_vin[17]          = "1HGCR2F83HA000001";

/**
 * @brief the lookup table for the Data Identifiers to store and handle all the needed Data
 * @ref charon_dataIdentifierObject_t
 */
static charon_dataIdentifierObject_t charonDIDLookupTable[] =
{
    /* Data Identifier */               /* Allowed Sessions */                                                           /* length */   /* Start Address of Data */         /* Has Scaling Data */      /* Scaling Data Length */       /* Scaling Data Address */  /* DID is Read only*/      /* DynamicallyDefineDID */
    {uds_data_test_did,                 (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      10u,         (uint32_t)&s_did_test_did[0],          false,                      0,                              0x00000000,                 false,                     false},
    {uds_data_test_did2,                (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      10u,         (uint32_t)&s_did_test_did2[0],         false,                      0,                              0x00000000,                 true,                      false},
    {uds_data_test_periodic,            (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      10u,         (uint32_t)&s_did_test_periodic[0],     false,                      0,                              0x00000000,                 false,                     false},
    {uds_data_test_periodic1,           (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      10u,         (uint32_t)&s_did_test_periodic1[0],    false,                      0,                              0x00000000,                 false,                     false},
    {uds_data_test_dynamic,             (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      10u,         (uint32_t)&s_did_test_dynamic[0],      false,                      0,                              0x00000000,                 false,                     true},
    {uds_did_hib_spare_part_number,     (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      16u,         (uint32_t)&s_did_f181_spare_part[0],   false,                      0,                              0x00000000,                 true,                      false},
    {uds_did_boot_sw_identifier,        (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      16u,         (uint32_t)&s_did_f182_boot_sw[0],      false,                      0,                              0x00000000,                 true,                      false},
    {uds_did_ecu_software_number,       (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      16u,         (uint32_t)&s_did_f183_ecu_sw[0],       false,                      0,                              0x00000000,                 true,                      false},
    {uds_did_ecu_app_software_number,   (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      16u,         (uint32_t)&s_did_f184_app_sw[0],       false,                      0,                              0x00000000,                 true,                      false},
    {uds_did_ecu_hardware_number,       (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      16u,         (uint32_t)&s_did_f185_ecu_hw[0],       false,                      0,                              0x00000000,                 true,                      false},
    {uds_did_active_diagnostic_session, (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      1u,          (uint32_t)&s_did_f186_session[0],      false,                      0,                              0x00000000,                 true,                      false},
    {uds_did_system_supplier_identifier,(SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      16u,         (uint32_t)&s_did_f18a_supplier[0],     false,                      0,                              0x00000000,                 true,                      false},
    {uds_did_vin,                       (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      17u,         (uint32_t)&s_did_f190_vin[0],          false,                      0,                              0x00000000,                 false,                     false},
};


/* Macros ********************************************************************/

/* Types *********************************************************************/

/* Variables *****************************************************************/

/* Private Function Definitions **********************************************/

/* Interfaces  ***************************************************************/

void charonDIDLookUpInit (void)
{
    /* DIDs have statically assigned RAM data pointers */
}

charon_dataIdentifierObject_t* charon_getDataLookupTableByDID (uint16_t DID )
{
    charon_dataIdentifierObject_t* pDidEntry = NULL;

    if (DID == uds_did_active_diagnostic_session)
    {
        s_did_f186_session[0] = (uint8_t)charon_sscGetSession();
    }

    for(uint32_t i=0; i < ARRAY_SIZE(charonDIDLookupTable); i++)
    {
        if(DID == charonDIDLookupTable[i].DID)
        {
            pDidEntry = &charonDIDLookupTable[i];
            break;
        }
    }
    return pDidEntry;        
}

charon_dataIdentifierObject_t* charon_getDataLookupTableByAddress (uint32_t dataAddress)
{
    charon_dataIdentifierObject_t* pAddressEntry = NULL;

    for(uint32_t i=0; i < ARRAY_SIZE(charonDIDLookupTable); i++)
    {
        if(dataAddress == charonDIDLookupTable[i].addressOfData)
        {
            pAddressEntry = &charonDIDLookupTable[i];
        }
    }
    return pAddressEntry;
}


