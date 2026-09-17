#include "uds.h"
#include "DiagnosticAndCommunicationManagementFunctionalUnit.h"
#include "UploadDownloadFunctionalUnit.h"
#include "SessionAndServiceControl.h"
#include "interface_debug.h"


/* Imports *******************************************************************/

/* Constants *****************************************************************/

/* Macros ********************************************************************/

/* Types *********************************************************************/

/* Variables *****************************************************************/

/* Private Function Definitions **********************************************/


/* Interfaces  ***************************************************************/


void charon_reset (void)
{
    charon_sscReset();
    charon_DiagnosticAndCommunicationManagementFunctionalUnit_reset();
    charon_UploadDownloadFunctionalUnit_reset();
}

void charon_init (ISocket_t systemCommunicationSocket)
{
    charon_reset();
    charon_sscInit(systemCommunicationSocket);
}

void charon_task (void)
{
    /** @todo think about putting charon_sscRcvMessage into charon_sscCyclic */

    /* Process Received Data */
    charon_sscRcvMessage();

    /* Process SSC Layer */
    charon_sscCyclic();

    /* Reset library if session has ended */
    if (charon_sscGetSession() == charon_sscType_timedOut)
    {
        charon_reset();
    }

    return;
}


