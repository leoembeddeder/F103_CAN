#include <stdint.h>
#include "uds_types.h"
#include "DataLookupTable.h"
#include "ServiceLookupTable.h"
#include "SessionAndServiceControl.h"

/* Imports *******************************************************************/

/* Constants *****************************************************************/

/**
 * @brief the lookup table for the Data Identifiers to store and handle all the needed Data
 * @ref charon_dataIdentifierObject_t
 */
static charon_dataIdentifierObject_t charonDIDLookupTable[] =
{
    /* Data Identifier */           /* Allowed Sessions */                                                           /* length */   /* Start Address of Data */      /* Has Scaling Data */      /* Scaling Data Length */       /* Scaling Data Address */  /* DID is Read only*/      /* DynamicallyDefineDID */
    {uds_data_test_did,             (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      10u,         0x100000,                        true,                       10,                             0x00000000,                 false,                     false},
    {uds_data_test_did2,            (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      10u,         0x200000,                        false,                      10,                             0x00000000,                 true,                      false},
    {uds_data_test_periodic,        (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      10u,         0x150000,                        false,                      10,                             0x00000000,                 false,                     false},
    {uds_data_test_periodic1,       (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      10u,         0x160000,                        false,                      10,                             0x00000000,                 false,                     false},
    {uds_data_test_dynamic,         (SESSION_DEFAULT | SESSION_PROGRAMMING | SESSION_EXTENDED | SESSION_SECURED),      10u,         0x140000,                        false,                      10,                             0x00000000,                 false,                     true},
};


/* Macros ********************************************************************/

/* Types *********************************************************************/

/* Variables *****************************************************************/

/* Private Function Definitions **********************************************/

/* Interfaces  ***************************************************************/

void charonDIDLookUpInit (void)
{
    for(uint32_t counter = 0; counter < ARRAY_SIZE(charonDIDLookupTable); counter ++)
    {
        charon_dataIdentifierObject_t * pTableEntryBase = &charonDIDLookupTable[counter];
        charon_dataIdentifierObject_t * pTableEntry = &charonDIDLookupTable[counter + 1];
        if(pTableEntryBase->lengthOfData == 0u)
        {
            pTableEntry->addressOfData = pTableEntryBase->addressOfData + MAX_VARIABLE_DATA_LENGTH;
        }
        else
        {
            pTableEntry->addressOfData = pTableEntryBase->addressOfData + pTableEntryBase->lengthOfData;
        }
    }  
}

charon_dataIdentifierObject_t* charon_getDataLookupTableByDID (uint16_t DID )
{
    charon_dataIdentifierObject_t* pDidEntry = NULL;

    for(uint32_t i=0; i < ARRAY_SIZE(charonDIDLookupTable); i++)
    {
        if(DID == charonDIDLookupTable[i].DID)
        {
            pDidEntry = &charonDIDLookupTable[i];
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


