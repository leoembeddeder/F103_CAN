#ifndef CHARON_CONFIG_H_
#define CHARON_CONFIG_H_

/* Includes ******************************************************************/

/* Imports *******************************************************************/

/* Constants *****************************************************************/
#define CHARON_CONFIG_LOG_MSG_OUT_AS_MACRO 1

/* Macros ********************************************************************/
/** @brief Macro to switch comfortably on or off charon welcome message. */ 
#define CHARON_CONFIG_DO_NOT_PRINT_WELCOME 0
/** @brief Macro to switch comfortably between endianess. */
#define CHARON_CONFIG_IS_BIG_ENDIAN 0
/** @brief Macro to switch comfortably between logging options.*/
#define CHARON_CONFIG_LOG_MSG_OUT_AS_FUNCTION 0
/** @brief Macro to switch comfortably on or off debug option for nvm. */
#define CHARON_CONFIG_OBD_SUPPORT 0


/** @brief For DTC Nvm size, NVm config may be changed depending on targets memory.*/
#define AMOUNT_OF_DTC                                   ((uint16_t)12u)
/** @brief For snapshot Nvm size, NVm config may be changed depending on targets memory. AMOUNT_OF_SNAPSHOT and AMOUNT... are only uint8_t so only 255 can be saved max. */
#define AMOUNT_OF_SNAPSHOT                              ((uint8_t) 8u)
/** @brief For storedData Nvm size, NVm config may be changed depending on targets memory. AMOUNT_OF_SNAPSHOT and AMOUNT... are only uint8_t so only 255 can be saved max. */
#define AMOUNT_OF_STOREDDATA                            ((uint8_t) 8u)
/** @brief For extData Nvm size, NVm config may be changed depending on targets memory. AMOUNT_OF_SNAPSHOT and AMOUNT... are only uint8_t so only 255 can be saved max. */
#define AMOUNT_OF_EXTENDEDDATA                          ((uint8_t) 8u)


/** @brief Used to define how many addresses the USER wants to save per DTC and datatype. */
#define NVM_AMOUNT_OF_SNAPSHOTS                         ((uint8_t)3u)
/** @brief Used to define how many addresses the USER wants to save per DTC and datatype. */
#define NVM_AMOUNT_OF_DATARECORDS                       ((uint8_t)3u)
/** @brief Used to define how many addresses the USER wants to save per DTC and datatype. */
#define NVM_AMOUNT_OF_EXTENDED                          ((uint8_t)3u)


/** @brief Used to define how much memory space the USER wants to reserve, for the payload. */
#define NVM_RESERVED_SIZE_SNAPSHOT                      ((uint16_t)32u)
/** @brief Used to define how much memory space the USER wants to reserve, for the payload. */
#define NVM_RESERVED_SIZE_DATARECORD                    ((uint16_t)32u)
/** @brief Used to define how much memory space the USER wants to reserve, for the payload. */
#define NVM_RESERVED_SIZE_EXTENDED                      ((uint16_t)32u)

/* Types *********************************************************************/

/* Variables *****************************************************************/

/* Private Function Definitions **********************************************/

/* Interfaces  ***************************************************************/

#endif 
