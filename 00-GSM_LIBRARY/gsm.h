/**
 * \author  Tilen Majerle
 * \email   tilen@majerle.eu
 * \website http://esp8266at.com
 * \license MIT
 * \brief   GSM Library
 *	
\verbatim
   ----------------------------------------------------------------------
    Copyright (c) 2016 Tilen Majerle

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software, 
    and to permit persons to whom the Software is furnished to do so, 
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
    AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------
\endverbatim
 */
#ifndef GSM_H
#define GSM_H 110

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup GSM
 * \brief    
 * \{
 *
 *
 * \par Dependencies
 *
\verbatim
 - stdlib.h
 - string.h
 - stdint.h
\endverbatim
 */
#include "stdlib.h"
#include "string.h"
#include "stdint.h"

/* Proto threads */
#include "pt/pt.h"
/* GSM configuration */
#include "gsm_config.h"
/* Buffer implementation */
#include "buffer.h"

/* GSM Low-Level drivers */
#include "gsm_ll.h"

#if GSM_RTOS
/* RTOS support */
#include "gsm_sys.h"
#endif

/**
 * @defgroup GSM_Macros
 * @brief    Library defines
 * @{
 */

#define gvol          volatile
#define gstatic       static


/* Configuration defines */

/**
 * \}
 */
 
/**
 * \defgroup GSM_Typedefs
 * \brief    Library Typedefs
 * \{
 */

/**
 * \brief  GSM library result enumeration
 */
typedef enum _GSM_Result_t {
    gsmOK = 0x00,                                           /*!< OK status */
    gsmERROR,                                               /*!< Error status */
    gsmBUSY,                                                /*!< GSM is busy right now */
    gsmPARERR,                                              /*!< Parameter errors */
    gsmTIMEOUT,                                             /*!< Timeout occurred */
    gsmSENDFAIL,                                            /*!< Send data failed on connection */
    
    gsmSYSERROR,                                            /*!< Syscall error */
    gsmLLERROR,                                             /*!< Error in Low-Level processing */
    gsmSIMNOTREADYERROR,                                    /*!< SIM is not ready to operate */
    gsmENTERTEXTMODEERROR,                                  /*!< Error trying to enter text mode for SMS */
    gsmSIMMEMORYERROR,                                      /*!< Error trying to select SMS mode */
    gsmNETWORKERROR
} GSM_Result_t;

/**
 * \brief  Call direction enumeration
 */
typedef enum _GSM_CallDir_t {
    GSM_CallDir_MO = 0x00,                                  /*!< Call direction is Mobile Originated */
    GSM_CallDir_MT = 0x01                                   /*<! Call direction is Mobile Terminated */
} GSM_CallDir_t;

/**
 * \brief  Call type enumeration
 */
typedef enum _GSM_CallType_t {
    GSM_CallType_Voice = 0x00,                              /*!< Call type is voice */
    GSM_CallType_Data = 0x01,                               /*!< Call type is data */
    GSM_CallType_Fax = 0x02                                 /*!< Call type is fax */
} GSM_CallType_t;

/**
 * \brief  Call status enumeration
 */
typedef enum _GSM_CallState_t {
    GSM_CallState_Active = 0x00,                            /*!< Call is active */
    GSM_CallState_Held = 0x01,                              /*<! Call is held */
    GSM_CallState_Dialing = 0x02,                           /*<! Call is dialing */
    GSM_CallState_Alerting = 0x03,                          /*<! Call is alerting */
    GSM_CallState_Incoming = 0x04,                          /*<! Call is incoming */
    GSM_CallState_Waiting = 0x05,                           /*<! Call is waiting */
    GSM_CallState_Disconnect = 0x06                         /*<! Call is disconnected */
} GSM_CallState_t;

/**
 * \brief  SMS memory enumeration
 * \note   This is currently not used and is for future purpose
 */
typedef enum _GSM_SMS_Memory_t {
    GSM_SMS_Memory_SM,
    GSM_SMS_Memory_BM,
    GSM_SMS_Memory_SE,
    GSM_SMS_Memory_ME
} GSM_SMS_Memory_t;

/**
 * \brief  SMS mass delete enumeration
 */
typedef enum _GSM_SMS_MassDelete_t {
    GSM_SMS_MassDelete_Read = 0x00,                         /*!< Delete all read SMS messages */
    GSM_SMS_MassDelete_Unread,                              /*!< Delete all unread SMS messages */
    GSM_SMS_MassDelete_Sent,                                /*!< Delete all sent SMS messages */
    GSM_SMS_MassDelete_Unsent,                              /*!< Delete all unset SMS messages */
    GSM_SMS_MassDelete_Inbox,                               /*!< Delete all received SMS messages */
    GSM_SMS_MassDelete_All                                  /*!< Delete all SMS messages */
} GSM_SMS_MassDelete_t;

/**
 * \brief  SMS read type enumeration
 */
typedef enum _GSM_SMS_ReadType_t {
    GSM_SMS_ReadType_ALL,                                   /*!< List all SMS entries */
    GSM_SMS_ReadType_READ,                                  /*!< List read entries only */
    GSM_SMS_ReadType_UNREAD                                 /*!< List unread entries only */
} GSM_SMS_ReadType_t;

/**
 * \brief  SIM card informations
 */
typedef enum _GSM_CPIN_t {
    GSM_CPIN_Unknown = 0x00,                                /*!< Default value */
    GSM_CPIN_Ready,                                         /*!< SIM is ready to use */
    GSM_CPIN_SIM_PIN,                                       /*!< SIM is waiting for PIN number */
    GSM_CPIN_SIM_PUK,                                       /*!< SIM is blocked and is waiting for PUK number */
    GSM_CPIN_PH_SIM_PIN,
    GSM_CPIN_PH_SIM_PUK,
    GSM_CPIN_SIM_PIN2,
    GSM_CPIN_SIM_PUK2
} GSM_CPIN_t;

/**
 * \brief  Time structure
 */
typedef struct _GSM_Date_t {
    uint8_t Day;                                            /*!< Day in month, from 1 to 31 */
    uint8_t Month;                                          /*!< Month in a year, from 1 to 12 */
    uint16_t Year;                                          /*!< Year with included thousands */
} GSM_Date_t;

/**
 * \brief  Time structure
 */
typedef struct _GSM_Time_t {
    uint8_t Hours;                                          /*!< Hours in format 00-24 */
    uint8_t Minutes;                                        /*!< Minutes */
    uint8_t Seconds;                                        /*!< Seconds */
} GSM_Time_t;

/**
 * \brief  Date and time structure
 */
typedef struct _GSM_DateTime_t {
    GSM_Date_t Date;                                        /*!< Date data */
    GSM_Time_t Time;                                        /*!< Time data */
} GSM_DateTime_t;

/**
 * \brief  SMS structure for send message
 */
typedef struct _GSM_SMS_t {
    const char* Number;                                     /*!< Pointer to constant string for number */
    const char* Data;                                       /*!< Pointer to constant data array */
    uint16_t SentSMSMemNum;                                 /*!< Memory number for sent SMS */
} GSM_SMS_t;

/**
 * \brief  Call information structure
 * \note   Used when call status changes to notify user
 */
typedef struct _GSM_CallInfo_t {
    uint8_t ID;                                             /*!< Call identification number, 0 - 7 */
    GSM_CallDir_t Dir;                                      /*!< Call direction */
    GSM_CallState_t State;                                  /*!< Call state */
    GSM_CallType_t Type;                                    /*!< Call type */
    char Number[16];                                        /*!< Call phone number */
    uint8_t IsMultiparty;                                   /*!< Call multiparty status */
    uint8_t AddressType;                                    /*!< Call address type */
    char Name[20];                                          /*!< Name in phonebook, if exists for selected number */
} GSM_CallInfo_t;

/**
 * \brief  SMS information structure
 * \note   Used when new SMS is received to notify user
 */
typedef struct _GSM_SmsInfo_t {
    GSM_SMS_Memory_t Memory;                                /*!< Memory where received SMS was saved */
    uint16_t Position;                                      /*!< Position where SMS was stored in memory */
    
    union {
        struct {
            uint8_t Received:1;                             /*!< Do we have new SMS info on this structure and was not yet checked */
            uint8_t Used:1;                                 /*!< Memory used indication */
        } F;
        uint8_t Value;
    } Flags;                                                /*!< Structure flags management */
} GSM_SmsInfo_t;

/**
 * \brief  SMS item
 */
typedef struct _GSM_SMS_Entry_t {
    uint16_t Position;                                      /*!< Position number in memory */
    GSM_SMS_Memory_t Memory;                                /*!< Memory type used for SMS */
    char Data[GSM_SMS_MAX_LENGTH];                          /*!< Actual SMS data in text mode */
    uint8_t DataLen;                                        /*!< Length of SMS data */
    char Name[20];                                          /*!< Name of sender if exists in phonebook */
    char Number[20];                                        /*!< Sender phone number */
    GSM_DateTime_t DateTime;                                /*!< SMS date and time sent/received */
} GSM_SMS_Entry_t;

/**
 * \brief  Phone book item
 */
typedef struct _GSM_PB_Entry_t {
    uint16_t Index;                                         /*!< Phonebook index number */
    char Name[20];                                          /*!< Phonebook saved display name */
    char Number[20];                                        /*!< Actual phone number */
} GSM_PB_Entry_t;

/**
 * \brief  Connection type enumeration
 */
typedef enum _GSM_CONN_ConnType_t {
    GSM_CONN_ConnType_TCP,                                  /*!< Create TCP connection  */
    GSM_CONN_ConnType_UPD                                   /*!< Create UDP connection */
} GSM_CONN_ConnType_t;

/**
 * \brief  Connection structure for GSM
 */
typedef struct _GSM_CONN_t {
    uint8_t ID;                                             /*!< Connection identification */
    uint8_t* ReceiveData;                                   /*!< Pointer to data array for receive or send */
    uint16_t BytesToRead;                                   /*!< Bytes to read from receive at a time, selected by user on function call */
    uint16_t BytesRead;                                     /*!< Bytes actually read from device, number of bytes we actually read in packet */
    uint32_t BytesReadTotal;                                /*!< Total number of bytes we already read from response */
    uint16_t BytesReadRemaining;                            /*!< Number of bytes we have to read in current packet */
    uint16_t BytesRemaining;                                /*!< Number of bytes remaining to read in module buffer */
    uint16_t ReadTimeout;                                   /*!< Timeout before checking for new data when reading */
    union {
        struct {
            uint8_t Active:1;                               /*!< Connection active flag */
            uint8_t RxGetReceived:1;                        /*!< RXGET was received waiting to read data! */
        } F;
        uint8_t Value;                                      /*!< Value containing all the flags in single memory */
    } Flags;                                                /*!< Union with all the listed flags */
} GSM_CONN_t;

/**
 * \brief  HTTP suppoerted request methods
 */
typedef enum _GSM_HTTP_Method_t {
    GSM_HTTP_Method_GET = 0x00,                             /*!< HTTP method GET */
    GSM_HTTP_Method_POST = 0x01,                            /*!< HTTP method POST */
    GSM_HTTP_Method_HEAD = 0x02                             /*!< HTTP method HEAD */
} GSM_HTTP_Method_t;

/**
 * \brief  HTTP structure for GSM
 */
typedef struct _GSM_HTTP_t {
    GSM_HTTP_Method_t Method;                               /*!< HTTP method */
    const char* TMP;                                        /*!< Pointer to URL to use/CONTENT to send */
    uint16_t Code;                                          /*!< HTTP response code */
    uint8_t* Data;                                          /*!< Pointer to read/write data array */
    uint32_t DataLength;                                    /*!< Length of data array in units of bytes */
    uint32_t BytesReceived;                                 /*!< Number of total bytes received by HTTP response from server to GSM */
    uint32_t BytesReadTotal;                                /*!< Total number of bytes we already read from GSM module HTTP response */
    uint16_t BytesToRead;                                   /*!< Bytes to read from receive at a time, selected by user on function call */
    uint32_t BytesRead;                                     /*!< Actual number of bytes read in last read procedure */
    uint32_t BytesReadRemaining;                            /*!< Number of bytes remaining to read in current read procedure */
} GSM_HTTP_t;

/**
 * \brief  FTP structure for GSM
 */
typedef struct _GSM_FTP_t {
    uint8_t ID;                                             /*!< Future use purpose */
} GSM_FTP_t;

typedef enum _GSM_NetworkStatus_t {
    GSM_NetworkStatus_Searching,
    GSM_NetworkStatus_RegisteredHome,
    GSM_NetworkStatus_NotRegistered,
    GSM_NetworkStatus_RegistrationDenied,
    GSM_NetworkStatus_Unknown,
    GSM_NetworkStatus_RegisteredRoaming
} GSM_NetworkStatus_t;

/**
 * \brief  GSM structure
 */
typedef struct _GSM_t {
    gvol uint32_t Time;                                     /*!< Current time in units of milliseconds */
    gvol GSM_Result_t RetVal;                               /*!< Return value */
    
    /*! Low-Level management */
    GSM_LL_t LL;                                            /*!< Structure for Low-Level communication */
    
    /*!< Active command informations */
    gvol uint16_t ActiveCmd;                                /*!< Current active command for execution */
    gvol uint16_t ActiveCmdSaved;                           /*!< Value of saved active CMD when necessary to change active command while processing one. */
    const char* gvol ActiveCmdResp;                         /*!< Pointer to active command response we are waiting for */
    gvol uint32_t ActiveCmdStart;                           /*!< Time when new command started with execution */
    gvol GSM_Result_t ActiveResult;                         /*!< Result to return from function */
    gvol uint32_t ActiveCmdTimeout;                         /*!< Timeout in units of MS for active command to finish */
    
    gvol GSM_NetworkStatus_t NetworkStatus;                 /*!< Network status enumeration */
    
    /*!< SMS management */
    GSM_SMS_t SMS;                                          /*!< SMS Send object */
    GSM_SmsInfo_t SmsInfos[GSM_MAX_RECEIVED_SMS_INFO];      /*!< Received SMS info object */
    
    /*!< Call management */
    GSM_CallInfo_t CallInfo;                                /*!< Call info object */
    
    /*!< Pin checks */
    GSM_CPIN_t CPIN;                                        /*!< SIM status */
    
    /*!< Network options */
    uint8_t IP[4];                                          /*!< Device IP address when connected to network */
    
    /*!< Plain connections check */
    GSM_CONN_t* Conns[6];                                   /*!< Array of pointers to connections */
    
    /*!< HTTP */
    GSM_HTTP_t HTTP;                                        /*!< HTTP related structure */
    
    /*!< FTP */
    GSM_FTP_t FTP;                                          /*!< FTP related structure */
    
#if GSM_RTOS
    /*!< RTOS support */
    GSM_RTOS_SYNC_t Sync;                                     /*!< RTOS Synchronization object */
#endif
    
    /*!< Flags management */
    union {
        struct {
            uint8_t IsBlocking:1;                           /*!< Set to 1 when function is in blocking execution */
            uint8_t Call_Idle:1;                            /*!< Set to 1 when we have to call IDLE callback */
            
            uint8_t PIN_Ok:1;                               /*!< Set to 1 when PIN is entered correct */
            uint8_t PIN_Error:1;                            /*!< Set to 1 when PIN is not connect */
            uint8_t PUK_Ok:1;                               /*!< Set to 1 when PUK is entered correct */
            uint8_t PUK_Error:1;                            /*!< Set to 1 when PUK is not correct */
            
            uint8_t SMS_SendOk:1;                           /*!< SMS was successfully sent */
            uint8_t SMS_SendError:1;                        /*!< We got an error trying to send SMS */
            uint8_t SMS_Read_Data:1;                        /*!< Set to 1 when we are reading actual SMS data */
            uint8_t SMS_CMTI_Received:1;                    /*!< Set to 1 when CMTI SMS info is received and callback should be called */
            
            uint8_t CALL_CLCC_Received:1;                   /*!< Set to 1 when CLCC call info is received and callback should be called */
            uint8_t CALL_RING_Received:1;                   /*!< Set to 1 when RING is received and callback should be called */
            
            uint8_t ReadSingleLineDataRespond:1;            /*!< Set to 1 when we have to read response from command like AT+CGMI. Data is returned as plain text without any special command before */
            
            uint8_t CLIENT_Read_Data:1;                     /*!< Set to 1 when we are reading raw data from client response */
            
            uint8_t HTTP_Read_Data:1;                       /*!< Set to 1 when reading data from HTTP response */
            
            uint8_t LastOperationStatus:1;
        } F;
        uint32_t Value;                                     /*!< Value containing all the flags in single memory */
    } Flags;                                                /*!< Union holding all the required flags */
    union {
        struct {
            uint8_t RespOk:1;                               /*!< OK message response */
            uint8_t RespError:1;                            /*!< Error message response */
            uint8_t RespBracket:1;                          /*!< Bracket received (SMS messages) */
            
            uint8_t RespConnectOk:1;                        /*!< n, CONNECT OK was returned from device */
            uint8_t RespConnectFail:1;                      /*!< n, CONNECT FAIL was returned from device */
            uint8_t RespConnectAlready:1;                   /*!< n, ALREADY CONNECTED was returned from device */
            uint8_t RespCloseOk:1;                          /*!< n, CLOSE OK was returned from device */
            uint8_t RespSendOk:1;                           /*!< n, SEND OK was returned from device */
            uint8_t RespSendFail:1;                         /*!< n, SEND FAIL was returned from device */
            
            uint8_t RespHttpAction:1;                       /*!< +HTTPACTION received and already parsed */
            uint8_t RespDownload:1;                         /*!< DOWNLOAD received for download HTTP data */
        } F;
        uint32_t Value;                                     /*!< Value containing all the flags in single memory */
    } Events;                                               /*!< Union holding all the required events for library internal processing */
} GSM_t;

/**
 * @}
 */

/**
 * \defgroup GSM_Functions
 * \brief    Library Functions
 * \{
 */
GSM_Result_t GSM_Init(gvol GSM_t* GSM, const char* pin, uint32_t Baudrate);
GSM_Result_t GSM_Update(gvol GSM_t* GSM);
GSM_Result_t GSM_WaitReady(gvol GSM_t* GSM, uint32_t timeout);
GSM_Result_t GSM_Delay(gvol GSM_t* GSM, uint32_t timeout);
GSM_Result_t GSM_Process(gvol GSM_t* GSM);
GSM_Result_t GSM_ProcessCallbacks(gvol GSM_t* GSM);
GSM_Result_t GSM_UpdateTime(gvol GSM_t* GSM, uint32_t ticksNum);
GSM_Result_t GSM_GetLastReturnStatus(gvol GSM_t* GSM);
uint32_t GSM_DataReceived(uint8_t* ch, uint32_t count);

/**
 * \defgroup INFO_API
 * \brief    Informations based functions
 * \{
 */

/**
 * \brief  Gets manufacturer from SIM device
 * \param  *GSM: Pointer to working \ref GSM_t structure
 * \param  *str: Pointer to string array to save menufacturer name
 * \param  length: Length of string array
 * \param  blocking: Status whether this function should be blocking to check for response
 * \retval Member of \ref GSM_Result_t enumeration
 */
GSM_Result_t GSM_INFO_GetManufacturer(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking);

/**
 * \brief  Gets model name from SIM device
 * \param  *GSM: Pointer to working \ref GSM_t structure
 * \param  *str: Pointer to string array to save model name
 * \param  length: Length of string array
 * \param  blocking: Status whether this function should be blocking to check for response
 * \retval Member of \ref GSM_Result_t enumeration
 */
GSM_Result_t GSM_INFO_GetModel(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking);

/**
 * \brief  Gets revision from SIM device
 * \param  *GSM: Pointer to working \ref GSM_t structure
 * \param  *str: Pointer to string array to save revision name
 * \param  length: Length of string array
 * \param  blocking: Status whether this function should be blocking to check for response
 * \retval Member of \ref GSM_Result_t enumeration
 */
GSM_Result_t GSM_INFO_GetRevision(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking);

/**
 * \brief  Gets serial number from SIM device
 * \param  *GSM: Pointer to working \ref GSM_t structure
 * \param  *str: Pointer to string array to save serial number
 * \param  length: Length of string array
 * \param  blocking: Status whether this function should be blocking to check for response
 * \retval Member of \ref GSM_Result_t enumeration
 */
GSM_Result_t GSM_INFO_GetSerialNumber(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking);
/**
 * \}
 */

/**
 * \defgroup PIN_API
 * \brief    PIN/PUK based functions
 * \{
 */
GSM_Result_t GSM_PIN_Enter(gvol GSM_t* GSM, const char* pin, uint32_t blocking);
GSM_Result_t GSM_PIN_Remove(gvol GSM_t* GSM, const char* current_pin, uint32_t blocking);
GSM_Result_t GSM_PIN_Add(gvol GSM_t* GSM, const char* new_pin, uint32_t blocking);
GSM_Result_t GSM_PUK_Enter(gvol GSM_t* GSM, const char* puk, const char* new_pin, uint32_t blocking);
/**
 * \}
 */

/**
 * \defgroup CALL_API
 * \brief    CALL based functions
 * \{
 */
GSM_Result_t GSM_CALL_Voice(gvol GSM_t* GSM, const char* number, uint32_t blocking);
GSM_Result_t GSM_CALL_VoiceFromSIMPosition(gvol GSM_t* GSM, uint16_t pos, uint32_t blocking);
GSM_Result_t GSM_CALL_Data(gvol GSM_t* GSM, const char* number, uint32_t blocking);
GSM_Result_t GSM_CALL_DataFromSIMPosition(gvol GSM_t* GSM, uint16_t pos, uint32_t blocking);
GSM_Result_t GSM_CALL_Answer(gvol GSM_t* GSM, uint32_t blocking);
GSM_Result_t GSM_CALL_HangUp(gvol GSM_t* GSM, uint32_t blocking);
GSM_CallInfo_t* GSM_CALL_GetInfo(gvol GSM_t* GSM, uint32_t blocking);
GSM_Result_t GSM_CALL_ClearInfo(gvol GSM_t* GSM, GSM_CallInfo_t* info, uint32_t blocking);

/**
 * \}
 */

/**
 * \defgroup SMS_API
 * \brief    SMS based functions
 * \{
 */
GSM_Result_t GSM_SMS_Send(gvol GSM_t* GSM, const char* number, const char* data, uint32_t blocking);
GSM_Result_t GSM_SMS_Read(gvol GSM_t* GSM, GSM_SMS_Entry_t* SMS, uint16_t position, uint32_t blocking);
GSM_Result_t GSM_SMS_List(gvol GSM_t* GSM, GSM_SMS_ReadType_t type, GSM_SMS_Entry_t* entries, uint16_t entries_count, uint16_t* entries_read, uint32_t blocking);
GSM_Result_t GSM_SMS_Delete(gvol GSM_t* GSM, uint16_t position, uint32_t blocking);
GSM_Result_t GSM_SMS_MassDelete(gvol GSM_t* GSM, GSM_SMS_MassDelete_t Type, uint32_t blocking);
GSM_SmsInfo_t* GSM_SMS_GetReceivedInfo(gvol GSM_t* GSM, uint32_t blocking);
GSM_Result_t GSM_SMS_ClearReceivedInfo(gvol GSM_t* GSM, GSM_SmsInfo_t* info, uint32_t blocking);

/**
 * \}
 */
 
/**
 * \defgroup PHONEBOOK_API
 * \brief    PHONEBOOK based functions
 * \{
 */
GSM_Result_t GSM_PB_Add(gvol GSM_t* GSM, const char* name, const char* number, uint32_t blocking);
GSM_Result_t GSM_PB_Edit(gvol GSM_t* GSM, uint32_t index, const char* name, const char* number, uint32_t blocking);
GSM_Result_t GSM_PB_Delete(gvol GSM_t* GSM, uint32_t index, uint32_t blocking);
GSM_Result_t GSM_PB_Get(gvol GSM_t* GSM, GSM_PB_Entry_t* entry, uint32_t index, uint32_t blocking);
GSM_Result_t GSM_PB_List(gvol GSM_t* GSM, GSM_PB_Entry_t* entries, uint16_t start_index, uint16_t entries_count, uint16_t* entries_read, uint32_t blocking);
GSM_Result_t GSM_PB_Search(gvol GSM_t* GSM, const char* search, GSM_PB_Entry_t* entries, uint16_t entries_count, uint16_t* entries_read, uint32_t blocking);
/**
 * \}
 */
 
/**
 * \defgroup DATETIME_API
 * \brief    DATETIME based functions
 * \{
 */
GSM_Result_t GSM_DATETIME_Get(gvol GSM_t* GSM, GSM_DateTime_t* datetime, uint32_t blocking);
/**
 * \}
 */
 
/**
 * \defgroup GPRS_API
 * \brief    GPRS based functions
 * \{
 */
GSM_Result_t GSM_GPRS_Attach(gvol GSM_t* GSM, const char* apn, const char* user, const char* pwd, uint32_t blocking);
GSM_Result_t GSM_GPRS_Detach(gvol GSM_t* GSM, uint32_t blocking);
/**
 * \}
 */

/**
 * \defgroup CONN_API
 * \brief    TCP/UPD connection based functions
 * \{
 */
GSM_Result_t GSM_CONN_ConnStart(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, GSM_CONN_ConnType_t type, const char* host, uint16_t port, uint32_t blocking);
GSM_Result_t GSM_CONN_ConnSend(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, const void* data, uint16_t length, uint32_t blocking);
GSM_Result_t GSM_CONN_ConnReceive(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, void* data, uint16_t length, uint16_t* read, uint16_t timeBeforeRead, uint32_t blocking);
GSM_Result_t GSM_CONN_ConnClose(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, uint32_t blocking);
/**
 * \}
 */

/**
 * \brief  Checks if any data to read from connection response
 * \note   This functions only checks flags and does not inquiry GSM.
 *              It means that when you are not in RTOS or ASYNC mode, 
 *              you have to manually call \ref GSM_Update function to parse any incoming data in a loop for specific timeout
 * \param  *GSM: Pointer to working \ref GSM_t structure
 * \param  *conn: Pointer to active \ref GSM_CONN_t connection structure
 * \param  blocking: Status whether this function should be blocking to check for response.
 *           Currently this param has no effect and is reserved for future use
 * \retval Status indicating if data available:
 *           - 0: Data not available
 *           - > 0: Data available for read operation
 */
uint32_t GSM_CONN_DataAvailable(gvol GSM_t* GSM, const gvol GSM_CONN_t* conn, uint32_t blocking);

/**
 * \defgroup HTTP_API
 * \brief    HTTP based functions
 * \{
 */
GSM_Result_t GSM_HTTP_Begin(gvol GSM_t* GSM, uint32_t blocking);
GSM_Result_t GSM_HTTP_End(gvol GSM_t* GSM, uint32_t blocking);

GSM_Result_t GSM_HTTP_SetContent(gvol GSM_t* GSM, const char* content, uint32_t blocking);
GSM_Result_t GSM_HTTP_SetData(gvol GSM_t* GSM, const void* data, uint32_t length, uint32_t blocking);
GSM_Result_t GSM_HTTP_Execute(gvol GSM_t* GSM, const char* url, GSM_HTTP_Method_t method, uint32_t blocking);
GSM_Result_t GSM_HTTP_Read(gvol GSM_t* GSM, void* data, uint32_t length, uint32_t* read, uint32_t blocking);

/**
 * \brief  Checks if any data to read from HTTP response
 * \note   This functions only checks flags and does not inquiry GSM.
 *              It means that when you are not in RTOS or ASYNC mode, 
 *              you have to manually call \ref GSM_Update function to parse any incoming data in a loop for specific timeout
 * \param  *GSM: Pointer to working \ref GSM_t structure
 * \param  blocking: Status whether this function should be blocking to check for response.
 *           Currently this param has no effect and is reserved for future use
 * \retval Status indicating if data available:
 *           - 0: Data not available
 *           - > 0: Data available for read operation
 */
uint32_t GSM_HTTP_DataAvailable(gvol GSM_t* GSM, uint32_t blocking);
/**
 * \}
 */
 
/**
 * \defgroup FTP_API
 * \brief    FTP based functions
 * \{
 */
GSM_Result_t GSM_FTP_Begin(gvol GSM_t* GSM, uint32_t blocking);
GSM_Result_t GSM_FTP_End(gvol GSM_t* GSM, uint32_t blocking);

GSM_Result_t GSM_FTP_Connect(gvol GSM_t* GSM, const char* server, uint16_t port, const char* user, const char* pass, uint32_t blocking);
GSM_Result_t GSM_FTP_UploadFile(gvol GSM_t* GSM, const char* folder, const char* file, const void* data, uint32_t length, uint32_t blocking);
GSM_Result_t GSM_FTP_Disconnect(gvol GSM_t* GSM, uint32_t blocking);
/**
 * \}
 */

//Callbacks API
//Active command is IDLE just now
void GSM_Callback_Idle(gvol GSM_t* GSM);
//Call info has changed
void GSM_Callback_CALL_Info(gvol GSM_t* GSM);
//Ring word received!
void GSM_Callback_CALL_Ring(gvol GSM_t* GSM);
//SMS received!
void GSM_Callback_SMS_Info(gvol GSM_t* GSM);

/**
 * \}
 */
 
/**
 * \}
 */

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
