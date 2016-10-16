/**	
 * |----------------------------------------------------------------------
 * | Copyright (c) 2016 Tilen Majerle
 * |  
 * | Permission is hereby granted, free of charge, to any person
 * | obtaining a copy of this software and associated documentation
 * | files (the "Software"), to deal in the Software without restriction,
 * | including without limitation the rights to use, copy, modify, merge,
 * | publish, distribute, sublicense, and/or sell copies of the Software, 
 * | and to permit persons to whom the Software is furnished to do so, 
 * | subject to the following conditions:
 * | 
 * | The above copyright notice and this permission notice shall be
 * | included in all copies or substantial portions of the Software.
 * | 
 * | THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * | EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * | OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * | AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * | HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * | WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * | FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * | OTHER DEALINGS IN THE SOFTWARE.
 * |----------------------------------------------------------------------
 */
#include "gsm.h"
#include "stdio.h"

/******************************************************************************/
/******************************************************************************/
/***                           Private structures                            **/
/******************************************************************************/
/******************************************************************************/
typedef struct {
    uint8_t Length;
    uint8_t Data[128];
} Received_t;
#define GSM_RECEIVED_ADD(c)             do { Received.Data[Received.Length++] = (c); Received.Data[Received.Length] = 0; } while (0)
#define GSM_RECEIVED_RESET()            do { Received.Length = 0; Received.Data[0] = 0; } while (0)
#define GSM_RECEIVED_LENGTH()           Received.Length

typedef struct {
    gvol const void* CPtr1;
    gvol const void* CPtr2;
    gvol const void* CPtr3;
    gvol void* Ptr1;
    gvol void* Ptr2;
    gvol uint32_t UI;
} Pointers_t;
gstatic
Pointers_t Pointers;

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
#define CHARISNUM(x)                    ((x) >= '0' && (x) <= '9')
#define CHARTONUM(x)                    ((x) - '0')

#define F(x)                            ((const char *)(x))
    
#define GSM_CRLF                        F("\r\n")
    
/* List of active commands */
#define CMD_IDLE                        ((uint16_t)0x0000)
#define CMD_GEN_SMSNOTIFY               ((uint16_t)0x0001)
#define CMD_GEN_ERROR_NUMERIC           ((uint16_t)0x0002)
#define CMD_GEN_CALL_CLCC               ((uint16_t)0x0003)
#define CMD_GEN_FACTORY_SETTINGS        ((uint16_t)0x0004)
#define CMD_IS_ACTIVE_GENERAL(GSM)      ((GSM)->ActiveCmd >= 0x0001 && (GSM)->ActiveCmd < 0x0100)

#define CMD_PIN                         ((uint16_t)0x0101)
#define CMD_PUK                         ((uint16_t)0x0102)
#define CMD_PIN_REMOVE                  ((uint16_t)0x0103)
#define CMD_PIN_ADD                     ((uint16_t)0x0104)
#define CMD_IS_ACTIVE_PIN(GSM)          ((GSM)->ActiveCmd >= 0x0100 && (GSM)->ActiveCmd < 0x0200)

#define CMD_SMS                         ((uint16_t)0x0200)
#define CMD_SMS_SEND                    ((uint16_t)0x0201)
#define CMD_SMS_READ                    ((uint16_t)0x0202)
#define CMD_SMS_DELETE                  ((uint16_t)0x0203)
#define CMD_SMS_MASSDELETE              ((uint16_t)0x0204)
#define CMD_SMS_LIST                    ((uint16_t)0x0205)
#define CMD_SMS_CMGF                    ((uint16_t)0x0210)
#define CMD_SMS_CMGS                    ((uint16_t)0x0211)
#define CMD_SMS_CMGR                    ((uint16_t)0x0212)
#define CMD_SMS_CMGD                    ((uint16_t)0x0213)
#define CMD_IS_ACTIVE_SMS(GSM)          ((GSM)->ActiveCmd >= 0x0200 && (GSM)->ActiveCmd < 0x0300)

#define CMD_CALL                        ((uint16_t)0x0300)
#define CMD_CALL_VOICE                  ((uint16_t)0x0301)
#define CMD_CALL_DATA                   ((uint16_t)0x0302)
#define CMD_CALL_ANSWER                 ((uint16_t)0x0303)
#define CMD_CALL_HANGUP                 ((uint16_t)0x0304)
#define CMD_CALL_VOICE_SIM_POS          ((uint16_t)0x0305)
#define CMD_CALL_DATA_SIM_POS           ((uint16_t)0x0306)
#define CMD_IS_ACTIVE_CALL(GSM)         ((GSM)->ActiveCmd >= 0x0300 && (GSM)->ActiveCmd < 0x0400)

#define CMD_INFO                        ((uint16_t)0x0400)
#define CMD_INFO_CGMM                   ((uint16_t)0x0401)
#define CMD_INFO_CGMI                   ((uint16_t)0x0402)
#define CMD_INFO_CGMR                   ((uint16_t)0x0403)
#define CMD_INFO_CNUM                   ((uint16_t)0x0404)
#define CMD_INFO_CGSN                   ((uint16_t)0x0405)
#define CMD_IS_ACTIVE_INFO(GSM)         ((GSM)->ActiveCmd >= 0x0400 && (GSM)->ActiveCmd < 0x0500)

#define CMD_PB                          ((uint16_t)0x0500)
#define CMD_PB_ADD                      ((uint16_t)0x0501)
#define CMD_PB_EDIT                     ((uint16_t)0x0502)
#define CMD_PB_DELETE                   ((uint16_t)0x0503)
#define CMD_PB_GET                      ((uint16_t)0x0504)
#define CMD_PB_LIST                     ((uint16_t)0x0505)
#define CMD_PB_SEARCH                   ((uint16_t)0x0506)
#define CMD_IS_ACTIVE_PB(GSM)           ((GSM)->ActiveCmd >= 0x0500 && (GSM)->ActiveCmd < 0x0600)

#define CMD_DATETIME                    ((uint16_t)0x0600)
#define CMD_DATETIME_GET                ((uint16_t)0x0601)
#define CMD_IS_ACTIVE_DATETIME(GSM)     ((GSM)->ActiveCmd >= 0x0600 && (GSM)->ActiveCmd < 0x0700)

#define CMD_GPRS                        ((uint16_t)0x0700)
#define CMD_GPRS_SETAPN                 ((uint16_t)0x0701)
#define CMD_GPRS_ATTACH                 ((uint16_t)0x0702)
#define CMD_GPRS_DETACH                 ((uint16_t)0x0703)
#define CMD_GPRS_HTTPBEGIN              ((uint16_t)0x0704)
#define CMD_GPRS_HTTPEND                ((uint16_t)0x0705)
#define CMD_GPRS_HTTPEXECUTE            ((uint16_t)0x0706)
#define CMD_GPRS_HTTPSEND               ((uint16_t)0x0707)
#define CMD_GPRS_HTTPCONTENT            ((uint16_t)0x0708)
#define CMD_GPRS_FTPBEGIN               ((uint16_t)0x0709)
#define CMD_GPRS_FTPEND                 ((uint16_t)0x070A)
#define CMD_GPRS_FTPCONNECT             ((uint16_t)0x070B)

#define CMD_GPRS_CIPSHUT                ((uint16_t)0x0720)
#define CMD_GPRS_CGATT                  ((uint16_t)0x0721)
#define CMD_GPRS_CGACT                  ((uint16_t)0x0722)
#define CMD_GPRS_SAPBR                  ((uint16_t)0x0723)
#define CMD_GPRS_CIICR                  ((uint16_t)0x0724)
#define CMD_GPRS_CIFSR                  ((uint16_t)0x0725)
#define CMD_GPRS_CSTT                   ((uint16_t)0x0726)
#define CMD_GPRS_CIPMUX                 ((uint16_t)0x0727)
#define CMD_GPRS_CIPSTATUS              ((uint16_t)0x0728)
#define CMD_GPRS_CIPSTART               ((uint16_t)0x0729)
#define CMD_GPRS_CIPSEND                ((uint16_t)0x072A)
#define CMD_GPRS_CIPCLOSE               ((uint16_t)0x072B)
#define CMD_GPRS_CIPRXGET               ((uint16_t)0x072C)
#define CMD_GPRS_HTTPINIT               ((uint16_t)0x072D)
#define CMD_GPRS_HTTPPARA               ((uint16_t)0x072E)
#define CMD_GPRS_HTTPDATA               ((uint16_t)0x072F)
#define CMD_GPRS_HTTPACTION             ((uint16_t)0x0730)
#define CMD_GPRS_HTTPREAD               ((uint16_t)0x0731)
#define CMD_GPRS_HTTPTERM               ((uint16_t)0x0732)
#define CMD_GPRS_FTPCID                 ((uint16_t)0x0733)
#define CMD_GPRS_FTPSERV                ((uint16_t)0x0734)
#define CMD_GPRS_FTPUN                  ((uint16_t)0x0735)
#define CMD_GPRS_FTPPW                  ((uint16_t)0x0736)
#define CMD_GPRS_FTPPUTNAME             ((uint16_t)0x0737)
#define CMD_GPRS_FTPPUTPATH             ((uint16_t)0x0738)
#define CMD_GPRS_FTPPORT                ((uint16_t)0x0739)
#define CMD_GPRS_CREG                   ((uint16_t)0x073A)
#define CMD_IS_ACTIVE_GPRS(GSM)         ((GSM)->ActiveCmd >= 0x0700 && (GSM)->ActiveCmd < 0x0800)

#if GSM_RTOS
#define GSM_IS_BUSY(ptr)                ((ptr)->ActiveCmd != CMD_IDLE || (ptr)->Flags.F.Call_Idle != 0)
#else
#define GSM_IS_BUSY(ptr)                ((ptr)->ActiveCmd != CMD_IDLE || (ptr)->Flags.F.Call_Idle != 0)
#endif
#define GSM_IS_READY(ptr)               (!GSM_IS_BUSY(ptr))
#define GSM_CHECK_BUSY(ptr)             do { if (GSM_IS_BUSY(ptr)) { GSM_RETURN(GSM, gsmBUSY); } } while (0)
#define GSM_CHECK_INPUTS(c)             do { if (!(c)) { GSM_RETURN(GSM, gsmPARERR); } } while (0)

#if GSM_RTOS == 1
#define GSM_IDLE(GSM)                   do {    \
    if (GSM_SYS_Release((GSM_RTOS_SYNC_t *)&(GSM)->Sync)) { \
    }                                           \
    (GSM)->ActiveCmd = CMD_IDLE;                \
    GSM_RESET_THREADS(GSM);                     \
    if (!(GSM)->Flags.F.IsBlocking) {           \
        (GSM)->Flags.F.Call_Idle = 1;           \
    }                                           \
} while (0)
#else
#define GSM_IDLE(GSM)                   do {    \
    (GSM)->ActiveCmd = CMD_IDLE;                \
    GSM_RESET_THREADS(GSM);                     \
    if (!(GSM)->Flags.F.IsBlocking) {           \
        (GSM)->Flags.F.Call_Idle = 1;           \
    }                                           \
} while (0)
#endif

#if GSM_RTOS
#define GSM_ACTIVE_CMD(GSM, cmd)        do {    \
    if (GSM_SYS_Request((GSM_RTOS_SYNC_t *)&(GSM)->Sync)) { \
        return gsmTIMEOUT;                      \
    }                                           \
    if ((GSM)->ActiveCmd == CMD_IDLE) {         \
        (GSM)->ActiveCmdStart = (GSM)->Time;    \
    }                                           \
    (GSM)->ActiveCmd = (cmd);                   \
} while (0)
#else
#define GSM_ACTIVE_CMD(GSM, cmd)        do {    \
    if ((GSM)->ActiveCmd == CMD_IDLE) {         \
        (GSM)->ActiveCmdStart = (GSM)->Time;    \
    }                                           \
    (GSM)->ActiveCmd = (cmd);                   \
} while (0)
#endif

#define GSM_CMD_SAVE(GSM)               (GSM)->ActiveCmdSaved = (GSM)->ActiveCmd
#define GSM_CMD_RESTORE(GSM)            (GSM)->ActiveCmd = (GSM)->ActiveCmdSaved

#define GSM_RETURN(GSM, val)            do { (GSM)->RetVal = (val); return (val); } while (0)
#define GSM_RETURN_BLOCKING(GSM, b, mt) do {    \
    GSM_Result_t res;                           \
    if (!(b)) {                                 \
        (GSM)->Flags.F.IsBlocking = 0;          \
        GSM_RETURN(GSM, gsmOK);                 \
    }                                           \
    (GSM)->Flags.F.IsBlocking = 1;              \
    res = GSM_WaitReady(GSM, mt);               \
    if (res == gsmTIMEOUT) {                    \
        return gsmTIMEOUT;                      \
    }                                           \
    res = (GSM)->ActiveResult;                  \
    (GSM)->ActiveResult = gsmOK;                \
    return res;                                 \
} while (0)

#define GSM_RESET_EVENTS_RESP(GSM)      do { (GSM)->Events.Value = 0; } while (0)

#define GSM_EXECUTE_SIM_READY_CHECK(GSM)  \
if ((GSM)->CPIN != GSM_CPIN_Ready) {                        /* SIM must be ready to call */     \
    GSM_RESET_EVENTS_RESP(GSM);                             /* Reset events */                  \
    UART_SEND_STR(F("AT+CPIN?"));                           /* Check again to be sure */        \
    UART_SEND_STR(F(GSM_CRLF));                                                                 \
    PT_WAIT_UNTIL(pt, (GSM)->Events.F.RespOk ||                                                 \
                        (GSM)->Events.F.RespError);         /* Wait for response */             \
    if ((GSM)->CPIN != GSM_CPIN_Ready) {                                                        \
        GSM->ActiveResult = gsmSIMNOTREADYERROR;                 /* SIM is not ready to operate */   \
        GSM_IDLE(GSM);                                      /* Go IDLE mode */                  \
        PT_EXIT(pt);                                        /* Stop execution */                \
    }                                                                                           \
}
#define GSM_EXECUTE_NETWORK_CHECK(GSM)  \
if ((GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredHome && \
    (GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredRoaming) {  /* Check if connected to network */     \
    GSM_RESET_EVENTS_RESP(GSM);                             /* Reset events */                  \
    UART_SEND_STR(F("AT+CREG?"));                           /* Check again to be sure */        \
    UART_SEND_STR(F(GSM_CRLF));                                                                 \
    PT_WAIT_UNTIL(pt, (GSM)->Events.F.RespOk ||                                                 \
                        (GSM)->Events.F.RespError);         /* Wait for response */             \
    if ((GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredHome &&                             \
        (GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredRoaming) {                          \
        GSM->ActiveResult = gsmNETWORKERROR;                /* Network ERROR */                 \
        GSM_IDLE(GSM);                                      /* Go IDLE mode */                  \
        PT_EXIT(pt);                                        /* Stop execution */                \
    }                                                                                           \
}
    
/*!< Implementation required */
#define UART_SEND_STR(str)              GSM_LL_SendData((GSM_LL_t *)&GSM->LL, (const uint8_t *)(str), strlen((const char *)str));
#define UART_SEND(str, len)             GSM_LL_SendData((GSM_LL_t *)&GSM, (const uint8_t *)(str), (len));
#define UART_SEND_CH(ch)                GSM_LL_SendData((GSM_LL_t *)&GSM, (const uint8_t *)(ch), 1);

#define GSM_DEBUG(fmt, ...)             printf(fmt, ##__VA_ARGS__)

#define GSM_OK                          F("OK\r\n")
#define GSM_ERROR                       F("ERROR\r\n")
#define GSM_BUSY                        F("BUSY\r\n")
#define GSM_NO_CARRIER                  F("NO CARRIER\r\n")
#define GSM_RING                        F("RING\r\n")

/******************************************************************************/
/******************************************************************************/
/***                            Private variables                            **/
/******************************************************************************/
/******************************************************************************/
gstatic BUFFER_t Buffer;                                    /* Buffer structure */
uint8_t Buffer_Data[GSM_BUFFER_SIZE + 1];                   /* Buffer data array */
gstatic Received_t Received;                                /* Received data structure */
gstatic gvol GSM_t* GSM;                                    /* Working pointer to GSM_t structure */

gstatic
struct pt pt_GEN, pt_INFO, pt_PIN, pt_CALL, pt_SMS, pt_PB, pt_DATETIME, pt_GPRS;

#define GSM_RESET_THREADS(GSM) \
PT_INIT(&pt_GEN); PT_INIT(&pt_INFO); PT_INIT(&pt_PIN); PT_INIT(&pt_CALL);   \
PT_INIT(&pt_SMS); PT_INIT(&pt_PB); PT_INIT(&pt_DATETIME); PT_INIT(&pt_GPRS);

/******************************************************************************/
/******************************************************************************/
/***                            Private functions                            **/
/******************************************************************************/
/******************************************************************************/

/* Check if needle exists in haystack memory */
//gstatic
void* mem_mem(void* haystack, size_t haystacksize, void* needle, size_t needlesize) {
    unsigned char* hptr = (unsigned char *)haystack;
    unsigned char* nptr = (unsigned char *)needle;
    unsigned int i;

    if (needlesize > haystacksize) {                		/* Check sizes */
        return 0;                                   		/* Needle is greater than haystack = nothing in memory */
    }
    if (haystacksize == needlesize) {                		/* Check if same length */
        if (memcmp(hptr, nptr, needlesize) == 0) {
            return hptr;
        }
        return 0;
    }
    haystacksize -= needlesize;                        		/* Set haystack size pointers */
    for (i = 0; i < haystacksize; i++) {            		/* Go through entire memory */
        if (memcmp(&hptr[i], nptr, needlesize) == 0) {      /* Check memory match */
            return &hptr[i];
        }
    }
    return 0;
}

/* Parses and returns number from string */
gstatic
int32_t ParseNumber(char* ptr, uint8_t* cnt) {
    uint8_t minus = 0, i = 0;
    int32_t sum = 0;
    
    if (*ptr == '-') {                                		/* Check for minus character */
        minus = 1;
        ptr++;
        i++;
    }
    while (CHARISNUM(*ptr)) {                        		/* Parse number */
        sum = 10 * sum + CHARTONUM(*ptr);
        ptr++;
        i++;
    }
    if (cnt != NULL) {                                		/* Save number of characters used for number */
        *cnt = i;
    }
    if (minus) {                                    		/* Minus detected */
        return 0 - sum;
    }
    return sum;                                       		/* Return number */
}

/* Parses date from string and stores to date structure */
gstatic
void ParseDATE(gvol GSM_t* GSM, GSM_Date_t* DateStr, char* str) {
    uint8_t len = 0, i;
    
    DateStr->Year = 2000 + ParseNumber(&str[0], &i);
    len += i + 1;
    DateStr->Month = ParseNumber(&str[len], &i);
    len += i + 1;
    DateStr->Day = ParseNumber(&str[len], &i);
}

/* Parses time from string and stores to time structure */
gstatic
void ParseTIME(gvol GSM_t* GSM, GSM_Time_t* TimeStr, char* str) {
    uint8_t len = 0, i;
    
    TimeStr->Hours = ParseNumber(&str[0], &i);
    len += i + 1;
    TimeStr->Minutes = ParseNumber(&str[len], &i);
    len += i + 1;
    TimeStr->Seconds = ParseNumber(&str[len], &i);
}

/* Parses +CMGR response for reading SMS data */
gstatic
void ParseCMGR(gvol GSM_t* GSM, GSM_SMS_Entry_t* sms, char* str) {
    char *p = str, *saveptr, *token;
    uint8_t i = 0;
    
    token = strtok_r(p, ",", &saveptr);
    while (token != NULL) {
        if (*token == '"') {
            token++;
        }
        if (token[strlen(token) - 1] == '"') {
            token[strlen(token) - 1] = 0;
        }
        switch (i) {
            case 0:
                //"REC READ" and other options
                break;
            case 1:
                strcpy(sms->Number, token);
                break;
            case 2:
                strcpy(sms->Name, token);
                break;
            case 3:
                ParseDATE(GSM, &sms->DateTime.Date, token);
                break;
            case 4:
                ParseTIME(GSM, &sms->DateTime.Time, token);
                break;
        }
        i++;
        token = strtok_r(NULL, ",", &saveptr);
    }
}
/* Parses +CMGL response for reading SMS data */
gstatic
void ParseCMGL(gvol GSM_t* GSM, GSM_SMS_Entry_t* sms, char* str) {
    uint8_t cnt;
    
    sms->Position = ParseNumber(str, &cnt);                 /* Parse number */
    str += cnt + 1;                                         /* Remove offset + comma */
    ParseCMGR(GSM, sms, str);                               /* From here, +CMGL is the same as +CMGR so use the same function as used in +CMGR response */
}
/* Parses +CLCC statement */
gstatic
void ParseCLCC(gvol GSM_t* GSM, gvol GSM_CallInfo_t* CallInfo, char* str) {
    uint8_t cnt = 0;
    
    CallInfo->ID = ParseNumber(str, &cnt);                  /* Get call ID */
    str += ++cnt;
    CallInfo->Dir = (GSM_CallDir_t)ParseNumber(str, &cnt);  /* Get call direction */
    str += ++cnt;
    CallInfo->State = (GSM_CallState_t)ParseNumber(str, &cnt);  /* Get call state */
    str += ++cnt;
    CallInfo->Type = (GSM_CallType_t)ParseNumber(str, &cnt);/* Get call type */
    str += ++cnt;
    CallInfo->IsMultiparty = ParseNumber(str, &cnt);        /* Get multiparty status */
    str += ++cnt;
    
    /* Parse phone number */
    if (*str == '"') {                                      /* Remmove start " character */
        str++;
    }
    CallInfo->Number[0] = 0;                                /* Clear number */
    cnt = 0;
    while (*str && *str != '"') {                           /* Read string until next " character */
        CallInfo->Number[cnt++] = *str++;                   /* Save number as string */
        CallInfo->Number[cnt] = 0;
    }
    if (*str == '"') {                                      /* Ignore end " character */
        str++;
    }
    str++;                                                  /* Ignore , character */
    
    CallInfo->AddressType = ParseNumber(str, &cnt);         /* Get address number */
    str += ++cnt;
    
    /* Parse phone book name if exists */
    if (*str == '"') {
        str++;
    }
    CallInfo->Name[0] = 0;
    cnt = 0;
    while (*str && *str != '"') {
        CallInfo->Name[cnt++] = *str++;                     /* Save number as string */
        CallInfo->Name[cnt] = 0;
    }
}
/* Parses +CMTI statement */
gstatic
void ParseCMTI(gvol GSM_t* GSM, gvol GSM_SmsInfo_t* SmsInfo, char* str) {
    if (*str == '"') {
        str++;
    }
    if (strncmp(str, "ME", 2) == 0) {
        SmsInfo->Memory = GSM_SMS_Memory_ME;
    } else if (strncmp(str, "SM", 2) == 0) {
        SmsInfo->Memory = GSM_SMS_Memory_SM;
    }
    str += 4;
    SmsInfo->Position = ParseNumber(str, NULL);
}
/* Parses +CPIN statement */
gstatic 
void ParseCPIN(gvol GSM_t* GSM, char* str) {
    if (strcmp(str, F("READY\r\n")) == 0) {
        GSM->CPIN = GSM_CPIN_Ready;
    } else if (strcmp(str, F("SIM PIN\r\n")) == 0) {
        GSM->CPIN = GSM_CPIN_SIM_PIN;
    } else if (strcmp(str, F("SIM PUK\r\n")) == 0) {
        GSM->CPIN = GSM_CPIN_SIM_PUK;
    } else if (strcmp(str, F("SIM PIN2\r\n")) == 0) {
        GSM->CPIN = GSM_CPIN_SIM_PIN2;
    } else if (strcmp(str, F("SIM PUK2\r\n")) == 0) {
        GSM->CPIN = GSM_CPIN_SIM_PUK2;
    } else if (strcmp(str, F("PH_SIM PIN\r\n")) == 0) {
        GSM->CPIN = GSM_CPIN_PH_SIM_PIN;
    } else if (strcmp(str, F("PH_SIM PUK\r\n")) == 0) {
        GSM->CPIN = GSM_CPIN_PH_SIM_PUK;
    }
}
/* Parses +CPBR statement */
gstatic
void ParseCPBR(gvol GSM_t* GSM, GSM_PB_Entry_t* entry, char* str) {
    uint8_t cnt;
    
    entry->Index = ParseNumber(str, &cnt);                  /* Get index number */
    str += cnt + 1;                                         /* Ignore comma */
    
    if (*str == '"') {                                      /* Ignore quote */
        str++;
    }
    cnt = 0;
    entry->Number[0] = 0;
    while (*str && *str != '"') {                           /* Parse number */
        entry->Number[cnt++] = *str++;                      /* Character by character */
        entry->Number[cnt] = 0;
    }
    if (*str == '"') {                                      /* Ignore quote */
        str++;
    }
    str++;                                                  /* Ignore comma */
    while (CHARISNUM(*str)) {                               /* Ignore phone number type */
        str++;
    }
    str++;                                                  /* Ignore comma */
    if (*str == '"') {                                      /* Ignore quote */
        str++;
    }
    cnt = 0;
    entry->Name[0] = 0;
    while (*str && *str != '"') {                           /* Parse number */
        entry->Name[cnt++] = *str++;                        /* Character by character */
        entry->Name[cnt] = 0;
    }
    if (*str == '"') {                                      /* Ignore quote */
        str++;
    }
}
/* Parses IP string */
gstatic
void ParseIP(gvol GSM_t* GSM, uint8_t* ip, char* str) {
    uint8_t cnt;
    *ip++ = ParseNumber(str, &cnt);
    str += cnt + 1;
    *ip++ = ParseNumber(str, &cnt);
    str += cnt + 1;
    *ip++ = ParseNumber(str, &cnt);
    str += cnt + 1;
    *ip++ = ParseNumber(str, &cnt);
}
/* Parses IP string */
gstatic
void ParseCREG(gvol GSM_t* GSM, char* str) {
    GSM->NetworkStatus = GSM_NetworkStatus_Unknown;
    if (*str == '0') {
        str += 2;
        GSM->NetworkStatus = (GSM_NetworkStatus_t)ParseNumber(str, NULL);
    }
}
/* Parses CIPRXGET statement */
gstatic 
void ParseCIPRXGET(gvol GSM_t* GSM, GSM_CONN_t* conn, char* str) {
    uint8_t cnt, num, connID;
    
    num = ParseNumber(str, &cnt);                           /* Response number */
    str += cnt + 1;
    connID = ParseNumber(str, &cnt);                        /* Connection ID */
    if (num == 1) {                                         /* Notification about new data received */
        if (conn == NULL) {
            conn = GSM->Conns[connID];                      /* Get connection pointer */
        }
        if (conn != NULL) {
            conn->Flags.F.RxGetReceived = 1;                /* We have new incoming data available in buffer to read for specific connection */
        }
    }
    str += cnt + 1;
    if (num == 2 && conn != NULL) {
        conn->BytesReadRemaining = ParseNumber(str, &cnt);  /* Bytes returned from response */
        str += cnt + 1;
        conn->BytesRemaining = ParseNumber(str, NULL);      /* Bytes remaining in buffer */
    }
}
/* Parse +HTTPACTION statement */
gstatic
void ParseHTTPACTION(gvol GSM_t* GSM, gvol GSM_HTTP_t* http, char* str) {
    uint8_t cnt;
    
    http->Method = (GSM_HTTP_Method_t)ParseNumber(str, &cnt);   /* Parse number for method */
    str += cnt + 1;
    http->Code = ParseNumber(str, &cnt);                    /* HTTP code response */
    str += cnt + 1;
    http->BytesReceived = ParseNumber(str, NULL);           /* Parse number of bytes to read */
}

/* Processes received string from module */
gstatic
void ParseReceived(gvol GSM_t* GSM, Received_t* Received) {
    char* str = (char *)Received->Data;
    uint16_t length = Received->Length;
    
    uint8_t is_ok = 0, is_error = 0;
    
    if (*str == '\r' && *(str + 1) == '\n') {               /* Check empty line */
        return;
    }
    
    /* Check for OK */
    if (GSM->ActiveCmd == CMD_GPRS_CIPSHUT) {               /* CIPSHUT answers with another response as OK */
        is_ok = strcmp(str, F("SHUT OK\r\n")) == 0;         /* Check if OK received */
    } else {
        is_ok = strcmp(str, GSM_OK) == 0;                   /* Check if OK received */
    }
    
    /* Check for error */
    if (!is_ok) {
        if (*str == '+') {
            is_error = strncmp(str, "+CME ERROR:", 11) == 0;    /* Check if error received */
        }
        if (!is_error) {
            is_error = strcmp(str, GSM_ERROR) == 0;         /* Check if error received */
        }
    }
    
    if (GSM->ActiveCmd == CMD_GPRS_CIFSR && CHARISNUM(*str)) {  /* IP received? */
        ParseIP(GSM, (uint8_t *)&GSM->IP, str);             /* Parse IP string */
        is_ok = 1;                                          /* Mark as OK */
    }
    
    if (CMD_IS_ACTIVE_INFO(GSM) && !is_ok && !is_error) {   /* Active command regarding GSM INFO */
        if (GSM->ActiveCmd == CMD_INFO_CGMI) {
            if (strncmp(str, F("AT+CGMI"), 7) != 0) {       /* Device manufacturer */
                strncpy((char *)Pointers.Ptr1, str, length - 2);    /* Copy received string to memory */
            }
        } else if (GSM->ActiveCmd == CMD_INFO_CGMM) {
            if (strncmp(str, F("AT+CGMM"), 7) != 0) {       /* Device number */
                strncpy((char *)Pointers.Ptr1, str, length - 2);    /* Copy received string to memory */
            }
        } else if (GSM->ActiveCmd == CMD_INFO_CGMR) {
            if (strncmp(str, F("AT+CGMR"), 7) != 0) {       /* Device revision */
                if (strncmp(str, "Revision:", 9) == 0) {    /* Remove revision part if exists */
                    str += 9;
                    length -= 9;
                }
                strncpy((char *)Pointers.Ptr1, str, length - 2);    /* Copy received string to memory */
            }
        } else if (GSM->ActiveCmd == CMD_INFO_CGSN) {
            if (strncmp(str, F("AT+CGSN"), 7) != 0) {       /* Device serial number */
                strncpy((char *)Pointers.Ptr1, str, length - 2);    /* Copy received string to memory */
            }
        }
    }
    
    if (GSM->ActiveCmd == CMD_GPRS_CIPSTART && CHARISNUM(*str)) {   /* We are trying to connect as client */
        if (strcmp(&str[1], F(", CONNECT OK\r\n")) == 0) {  /* n, CONNECT OK received */
            GSM->Events.F.RespConnectOk = 1;
        } else if (strcmp(&str[1], F(", CONNECT FAIL\r\n")) == 0) {/* n, CONNECT FAIL received */
            GSM->Events.F.RespConnectFail = 1;
        } else if (strcmp(&str[1], F(", ALREADY CONNECT\r\n")) == 0) {  /* n, ALREADY CONNECT received */
            GSM->Events.F.RespConnectAlready = 1;
        }
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPCLOSE) {
        if (strcmp(&str[1], F(", CLOSE OK\r\n")) == 0) {    /* n, CLOSE OK received */
            GSM->Events.F.RespCloseOk = 1;                  /* Closed OK */
            is_ok = 1;                                      /* Response is OK */
        }
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPSEND) {
        if (strcmp(&str[1], F(", SEND OK\r\n")) == 0) {/* n, SEND OK received */
            GSM->Events.F.RespSendOk = 1;
            is_ok = 1;
        } else if (strcmp(&str[1], F(", SEND FAIL\r\n")) == 0) {  /* n, SEND FAIL received */
            GSM->Events.F.RespSendFail = 1;
            is_error = 1;
        }
    }
    
    /* Some special cases */
    if (strcmp(str, GSM_RING) == 0) {
        GSM->Flags.F.CALL_RING_Received = 1;                /* Set flag for callbacks */
    } else if (strcmp(str, GSM_BUSY) == 0) {
        //GSM_DEBUG("Call BUSY");
    } else if (strcmp(str, GSM_NO_CARRIER) == 0) {
        //GSM_DEBUG("Call NO CARRIER");
    }
    
    if (*str == '+' && !is_error) {                         /* For statements starting with '+' sign */     
        if (GSM->ActiveCmd == CMD_GPRS_CREG && strncmp(str, F("+CREG:"), 6) == 0) {
            ParseCREG(GSM, &str[7]);                        /* Parse CREG response */
        } else if (CMD_IS_ACTIVE_GPRS(GSM)) {
        
        } else if (CMD_IS_ACTIVE_SMS(GSM)) {                /* Currently active command is regarding SMS */
            if (GSM->ActiveCmd == CMD_SMS_SEND && strncmp(str, F("+CMGS:"), 6) == 0) {  /* We just sent SMS and number in memory is returned */
                GSM->SMS.SentSMSMemNum = ParseNumber(&str[7], NULL);/* Parse number and save it */
            } else if (GSM->ActiveCmd == CMD_SMS_READ && strncmp(str, F("+CMGR:"), 6) == 0) { /* When SMS Read instruction is executed */
                ParseCMGR(GSM, (GSM_SMS_Entry_t *)Pointers.Ptr1, str + 7);  /* Parse received command for read SMS */
                GSM->Flags.F.SMS_Read_Data = 1;             /* Next step is to read actual SMS data */
                ((GSM_SMS_Entry_t *)Pointers.Ptr1)->DataLen = 0; /* Reset data length */
            } else if (GSM->ActiveCmd == CMD_SMS_LIST && strncmp(str, F("+CMGL:"), 6) == 0) {  /* When list command is executed */
                if (*(uint16_t *)Pointers.Ptr2 < Pointers.UI) { /* Do we still have empty memory to read data? */
                    ParseCMGL(GSM, (GSM_SMS_Entry_t *)Pointers.Ptr1, str + 7);
                    ((GSM_SMS_Entry_t *)Pointers.Ptr1)->DataLen = 0; /* Reset data length */
                }
                GSM->Flags.F.SMS_Read_Data = 1;             /* Next step is to read actual SMS data 
                                                               Activate this command, even if data won't be saved. 
                                                               Data should be flushed from received buffer and ignored! */
            }
        } else if (CMD_IS_ACTIVE_PB(GSM)) {                 /* Currently active command is regarding PHONEBOOK */
            if (strncmp(str, F("+CPBR:"), 6) == 0) {
                ParseCPBR(GSM, (GSM_PB_Entry_t *)Pointers.Ptr1, str + 7);   /* Parse +CPBR statement */
                if (GSM->ActiveCmd == CMD_PB_LIST) {      /* Check for GETALL or SEARCH commands */
                    Pointers.Ptr1 = ((GSM_PB_Entry_t *)Pointers.Ptr1) + 1;  /* Set new pointer offset in data array */
                    *(uint16_t *)Pointers.Ptr2 = (*(uint16_t *)Pointers.Ptr2) + 1;  /* Increase number of received entries */
                }
            } else if (GSM->ActiveCmd == CMD_PB_SEARCH && strncmp(str, F("+CPBF:"), 6) == 0) {
                if (*(uint16_t *)Pointers.Ptr2 < Pointers.UI) { /* Check for GETALL or SEARCH commands */
                    ParseCPBR(GSM, (GSM_PB_Entry_t *)Pointers.Ptr1, str + 7);   /* Parse +CPBR statement */
                    Pointers.Ptr1 = ((GSM_PB_Entry_t *)Pointers.Ptr1) + 1;  /* Set new pointer offset in data array */
                    *(uint16_t *)Pointers.Ptr2 = (*(uint16_t *)Pointers.Ptr2) + 1;  /* Increase number of received entries */
                }
            }
        }
        
        /* Below statements can be received even if no command active */
        if (strncmp(str, F("+CPIN:"), 6) == 0) {            /* For SIM status response */
            ParseCPIN(GSM, str + 7);                        /* Parse +CPIN statement */
        } else if (strncmp(str, F("+CLCC:"), 6) == 0) {     /* Call informations changes */
            ParseCLCC(GSM, &GSM->CallInfo, str + 7);        /* Parse +CLCC statement */
            GSM->Flags.F.CALL_CLCC_Received = 1;            /* Set flag for callback */
        } else if (strncmp(str, F("+CMTI:"), 6) == 0) {
            uint8_t i = 0;
            for (i = 0; i < GSM_MAX_RECEIVED_SMS_INFO; i++) {
                if (!GSM->SmsInfos[i].Flags.F.Used && !GSM->SmsInfos[i].Flags.F.Received) { /* Check if available memory */
                    GSM->SmsInfos[i].Flags.F.Used = 1;      /* We have used this memory */
                    GSM->SmsInfos[i].Flags.F.Received = 1;  /* We have received memory */
                    
                    ParseCMTI(GSM, (GSM_SmsInfo_t *)&GSM->SmsInfos[i], str + 7); /* Parse +CMTI statement */
                    break;
                }
            }
            GSM->Flags.F.SMS_CMTI_Received = 1;             /* Set flag for callback */
        } else if (strncmp(str, F("+CIPRXGET:"), 10) == 0) {/* +CIPRXGET statement */
            if (strlen(&str[11]) > 5) {                     /* We executed command */
                GSM_CONN_t* conn = (GSM_CONN_t *)Pointers.Ptr1;
                ParseCIPRXGET(GSM, conn, str + 11);         /* Parse statement */
                GSM->Flags.F.CLIENT_Read_Data = 0;          /* Reset flag to read data first */
                if (conn->BytesReadRemaining) {             /* Any bytes to read? */
                    GSM->Flags.F.CLIENT_Read_Data = 1;      /* Read raw data from response */
                }
            } else {                                        /* Notification info */
                ParseCIPRXGET(GSM, NULL, str + 11);         /* Get connection ID from string */
            }
        } else if (strncmp(str, F("+HTTPACTION:"), 12) == 0) {  /* Check HTTPACTION */
            ParseHTTPACTION(GSM, &GSM->HTTP, &str[13]);     /* Parse +HTTPACTION response */
            GSM->Events.F.RespHttpAction = 1;               /* HTTP ACTION */
        }  else if (strncmp(str, F("+HTTPREAD:"), 10) == 0) {   /* Check HTTPACTION */
            GSM->HTTP.BytesRead = 0;                        /* Reset read bytes */
            GSM->HTTP.BytesReadRemaining = ParseNumber(&str[11], NULL); /* Get number of bytes we have to read in this request */
            GSM->Flags.F.HTTP_Read_Data = 1;                /* HTTP read data */
        } 
    }
    
    if (GSM->ActiveCmd == CMD_GPRS_HTTPDATA && strncmp(str, "DOWNLOAD", 8) == 0) {  /* Download received? */
        GSM->Events.F.RespDownload = 1;                     /* Set flag, send data */
    }
    
    if (is_ok) {
        GSM->Events.F.RespOk = 1;
        GSM->Events.F.RespError = 0;
        GSM->Flags.F.LastOperationStatus = 1;
    } else if (is_error) {
        GSM->Events.F.RespOk = 0;
        GSM->Events.F.RespError = 1;
        GSM->Flags.F.LastOperationStatus = 0;
    }
}

/* Starts command and sets pointer for return statement */
gstatic 
GSM_Result_t StartCommand(gvol GSM_t* GSM, uint16_t cmd, const char* cmdResp) {
    GSM->ActiveCmd = cmd;
    GSM->ActiveCmdResp = (char *)cmdResp;
    GSM->ActiveCmdStart = GSM->Time;
    GSM->ActiveResult = gsmOK;
    
    return gsmOK;
}

/* Converts number to string */
gstatic
void NumberToString(char* str, uint32_t number) {
    sprintf(str, "%d", number);
}

/******************************************************************************/
/******************************************************************************/
/***                              Protothreads                               **/
/******************************************************************************/
/******************************************************************************/
gstatic
PT_THREAD(PT_Thread_GEN(struct pt* pt, gvol GSM_t* GSM)) {
    PT_BEGIN(pt);                                           /* Begin thread */
    
    if (GSM->ActiveCmd == CMD_GEN_FACTORY_SETTINGS) {       /* Check active command status */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT&F"));                           /* Factory reset */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GEN_FACTORY_SETTINGS, NULL);  /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);
    } else if (GSM->ActiveCmd == CMD_GEN_SMSNOTIFY) {       /* Check active command status */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CNMI=1,2,0,0,0"));              /* Automatically send notification about new received SMS and SMS itself */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GEN_SMSNOTIFY, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);
    } else if (GSM->ActiveCmd == CMD_GEN_ERROR_NUMERIC) {   /* Enable numeric error reporting */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CMEE=1"));                      /* Error reporting */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GEN_ERROR_NUMERIC, NULL);     /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);
    } else if (GSM->ActiveCmd == CMD_GEN_CALL_CLCC) {       /* Enable auto notifications for call */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CLCC=1"));                      /* Auto notify about new calls */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GEN_CALL_CLCC, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);
    }
    PT_END(pt);                                             /* End thread */
}

gstatic 
PT_THREAD(PT_Thread_INFO(struct pt* pt, gvol GSM_t* GSM)) {
    PT_BEGIN(pt);
    
    if (GSM->ActiveCmd == CMD_INFO_CGMI) {                  /* Check device manufacturer */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CGMI"));                        /* Send data to device */
        UART_SEND_STR(F(GSM_CRLF));
        StartCommand(GSM, CMD_INFO_CGMI, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE state */
    } else if (GSM->ActiveCmd == CMD_INFO_CGMM) {           /* Check device model */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CGMM"));                        /* Send data to device */
        UART_SEND_STR(F(GSM_CRLF));
        StartCommand(GSM, CMD_INFO_CGMM, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        GSM_IDLE(GSM);                                      /* Go IDLE state */
    } else if (GSM->ActiveCmd == CMD_INFO_CGMR) {           /* Check device revision */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CGMR"));                        /* Send data to device */
        UART_SEND_STR(F(GSM_CRLF));
        StartCommand(GSM, CMD_INFO_CGMR, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE state */
    } else if (GSM->ActiveCmd == CMD_INFO_CGSN) {           /* Check device serial number */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CGSN"));                        /* Send data to device */
        UART_SEND_STR(F(GSM_CRLF));
        StartCommand(GSM, CMD_INFO_CGSN, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE state */
    }
    PT_END(pt);                                             /* End thread */
}

gstatic
PT_THREAD(PT_Thread_PIN(struct pt* pt, gvol GSM_t* GSM)) {
    PT_BEGIN(pt);                                           /* Begin thread */

    GSM_RESET_EVENTS_RESP(GSM);                             /* Reset events */
    UART_SEND_STR(F("AT+CPIN?"));                           /* Check SIM status first */
    UART_SEND_STR(F(GSM_CRLF));
    
    PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                        GSM->Events.F.RespError);           /* Wait for response */
    
    if (GSM->Events.F.RespError) {                          /* Check error */
        GSM->ActiveResult = gsmERROR;                       /* Return error */
        GSM_IDLE(GSM);                                      /* Go IDLE state */
        PT_EXIT(pt);                                        /* Exit with thread */
    }
    
    if (GSM->ActiveCmd == CMD_PIN) {
        if (GSM->CPIN != GSM_CPIN_SIM_PIN && GSM->CPIN != GSM_CPIN_SIM_PIN2) {  /* Check if in valid state for PIN insert */
            GSM_IDLE(GSM);                                  /* Process IDLE */
            PT_EXIT(pt);                                    /* Stop thread */
        }
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CPIN="));                       /* Send data to device */
        UART_SEND_STR((char *)Pointers.CPtr1);              /* PIN pointer is in this variable */
        UART_SEND_STR(F(GSM_CRLF));
        StartCommand(GSM, CMD_PIN, NULL);                   /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        if (GSM->Events.F.RespOk) {                         /* Response is OK */
            GSM->Flags.F.PIN_Ok = 1;
            GSM->Flags.F.PIN_Error = 0;
        }
        if (GSM->Events.F.RespError) {                      /* Response is error */
            GSM->Flags.F.PIN_Ok = 0;
            GSM->Flags.F.PIN_Error = 1;
        }
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PUK) {
        if (GSM->CPIN != GSM_CPIN_SIM_PUK && GSM->CPIN != GSM_CPIN_SIM_PUK2) {  /* Check if in valid state for PIN insert */
            GSM_IDLE(GSM);                                  /* Process IDLE */
            PT_EXIT(pt);                                    /* Stop thread */
        }
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CPIN="));                       /* Send data to device */
        UART_SEND_STR((char *)Pointers.CPtr1);              /* PUK pointer is in this variable */
        UART_SEND_STR(F(","));                              /* Separator */
        UART_SEND_STR((char *)Pointers.CPtr2);              /* New pin pointer */
        UART_SEND_STR(F(GSM_CRLF));
        StartCommand(GSM, CMD_PUK, NULL);                   /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        if (GSM->Events.F.RespOk) {                         /* Response is OK */
            GSM->Flags.F.PUK_Ok = 1;
            GSM->Flags.F.PUK_Error = 0;
        }
        if (GSM->Events.F.RespError) {                      /* Response is error */
            GSM->Flags.F.PUK_Ok = 0;
            GSM->Flags.F.PUK_Error = 1;
        }
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PIN_REMOVE) {          /* Remove PIN */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CLCK=\"SC\",0,\""));            /* Send data to device */
        UART_SEND_STR(F(Pointers.CPtr1));                   /* Send actual PIN */
        UART_SEND_STR(F("\""));                             
        UART_SEND_STR(F(GSM_CRLF));
        StartCommand(GSM, CMD_PIN_REMOVE, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PIN_ADD) {             /* Add pin to SIM card without pin */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CLCK=\"SC\",1,\""));            /* Send data to device */
        UART_SEND_STR(F(Pointers.CPtr1));                   /* Send actual PIN */
        UART_SEND_STR(F("\""));                             
        UART_SEND_STR(F(GSM_CRLF));
        StartCommand(GSM, CMD_PIN_ADD, NULL);               /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    }
    PT_END(pt);                                             /* Stop thread */
}

gstatic
PT_THREAD(PT_Thread_CALL(struct pt* pt, gvol GSM_t* GSM)) {
    char str[6];
    
    PT_BEGIN(pt);                                           /* Begin thread */
    GSM_EXECUTE_SIM_READY_CHECK(GSM);                       /* SIM must be ready to operate in this case! */
    
    if (GSM->ActiveCmd == CMD_CALL_VOICE) {                 /* Create voice call */        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("ATD"));                            /* Send command */
        UART_SEND_STR(Pointers.CPtr1);                      /* Send number formatted as string */
        UART_SEND_STR(F(";"));                              /* Semicolon is required for voice call */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_VOICE, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_CALL_VOICE_SIM_POS) {  /* Create voice call from SIM  */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert position to string */
        UART_SEND_STR(F("ATD>\"SM\" "));                    /* Send command */
        UART_SEND_STR(str);                                 /* Send number formatted as string */
        UART_SEND_STR(F(";"));                              /* Process voice call */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_VOICE_SIM_POS, NULL);    /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */        
        GSM_IDLE(GSM);                                      /* Go IDLE mode */ 
    } else if (GSM->ActiveCmd == CMD_CALL_DATA) {           /* Create data call */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("ATD"));                            /* Send command */
        UART_SEND_STR(Pointers.CPtr1);                      /* Send number formatted as string */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_DATA, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */ 
    } else if (GSM->ActiveCmd == CMD_CALL_DATA_SIM_POS) {   /* Create data call from SIM  */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert position to string */
        UART_SEND_STR(F("ATD>\"SM\" "));                    /* Send command */
        UART_SEND_STR(str);                                 /* Send number formatted as string */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_DATA_SIM_POS, NULL);     /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */ 
    } else if (GSM->ActiveCmd == CMD_CALL_ANSWER) {         /* If we want to answer call */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("ATA"));                            /* Send command to answer incoming call */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_ANSWER, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_CALL_HANGUP) {         /* If we want to answer call */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("ATH"));                            /* Send command to hang up */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_HANGUP, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    }
    PT_END(pt);                                             /* End thread */
}

gstatic
PT_THREAD(PT_Thread_SMS(struct pt* pt, gvol GSM_t* GSM)) {
    char terminate = 26;
    char str[6];
    GSM_SMS_Entry_t* ReadSMSPtr = (GSM_SMS_Entry_t *)Pointers.Ptr1;
    
    PT_BEGIN(pt);                                           /* Begin thread */
    GSM_EXECUTE_SIM_READY_CHECK(GSM);                       /* SIM must be ready to operate in this case! */
    
    /**** Enter SMS text mode ****/
    GSM_CMD_SAVE(GSM);                                      /* Save current command */
    GSM_RESET_EVENTS_RESP(GSM);                             /* Reset events */
    UART_SEND_STR(F("AT+CMGF=1"));                          /* Go to SMS text mode */
    UART_SEND_STR(GSM_CRLF);
    StartCommand(GSM, CMD_SMS_CMGF, NULL);                  /* Start command */
    
    PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                        GSM->Events.F.RespError);           /* Wait for response */
    GSM_CMD_RESTORE(GSM);                                   /* Restore command */
    
    if (GSM->Events.F.RespError) {
        GSM->Flags.F.SMS_SendError = 1;                     /* SMS error flag */
        GSM->ActiveResult = gsmENTERTEXTMODEERROR;          /* Can not go to text mode */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
        PT_EXIT(pt);                                        /* Exit thread */
    }
    
    if (GSM->ActiveCmd == CMD_SMS_SEND) {                   /* Process send SMS */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CMGS=\""));                     /* Send number to GSM */
        UART_SEND_STR(GSM->SMS.Number);                     /* Send actual number formatted as string */
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_SMS_SEND, F("+CMGS"));        /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespBracket ||
                            GSM->Events.F.RespError);       /* Wait for > character and timeout */
        
        if (GSM->Events.F.RespBracket) {                    /* We received bracket */
            GSM_RESET_EVENTS_RESP(GSM);                     /* Reset events */
            UART_SEND_STR(GSM->SMS.Data);                   /* Send SMS data */
            UART_SEND_CH(&terminate);                       /* Send terminate SMS character */
            
            PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk ||
                                GSM->Events.F.RespError);   /* Wait for OK or ERROR */
            
            if (GSM->Events.F.RespOk) {                     /* Here, we should have SMS memoy number for sent SMS */
                GSM->Flags.F.SMS_SendOk = 1;                /* SMS sent OK */
            } else if (GSM->Events.F.RespError) {           /* Error sending */
                GSM->Flags.F.SMS_SendError = 1;             /* SMS was not send */
            }                                               /* Go IDLE mode */
        } else if (GSM->Events.F.RespError) {
            GSM->Flags.F.SMS_SendError = 1;                 /* SMS error flag */
            GSM->ActiveResult = gsmERROR;                   /* Process error */
        }
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_SMS_READ) {            /* Process read SMS */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        
        NumberToString(str, ReadSMSPtr->Position);          /* Convert number to string */
        UART_SEND_STR(F("AT+CMGR="));                       /* Send command */
        UART_SEND_STR(str);
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_SMS_READ, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_SMS_DELETE) {          /* Process delete SMS */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        UART_SEND_STR(F("AT+CMGD="));                       /* Send command */
        UART_SEND_STR(str);
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_SMS_CMGD, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_SMS_MASSDELETE) {      /* Process mass delete SMS */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        UART_SEND_STR(F("AT+CMGDA=\"DEL "));                /* Send command */
        UART_SEND_STR((char *)Pointers.CPtr1);
        UART_SEND_STR(F("\""));                             /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_SMS_CMGD, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */  
    } else if (GSM->ActiveCmd == CMD_SMS_LIST) {            /* Process get all phonebook entries */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CMGL=\""));                     /* Send command */
        UART_SEND_STR(F(Pointers.CPtr1));                   /* Send type of SMS messages to list */
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_SMS_LIST, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    }
    PT_END(pt);                                             /* End thread */
}

gstatic
PT_THREAD(PT_Thread_PB(struct pt* pt, gvol GSM_t* GSM)) {
    char str[6];
    
    PT_BEGIN(pt);                                           /* Begin thread */
    GSM_EXECUTE_SIM_READY_CHECK(GSM);                       /* SIM must be ready to operate in this case! */
    
    if (GSM->ActiveCmd == CMD_PB_ADD) {                     /* Process add phonebook entry */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CPBW="));                       /* Send command */
        UART_SEND_STR(F(",\""));
        UART_SEND_STR((char *)Pointers.CPtr1);              /* Send number */
        UART_SEND_STR(F("\",129,\""));                      /* Send national number format */
        UART_SEND_STR((char *)Pointers.CPtr2);              /* Send name */
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_ADD, NULL);                /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PB_EDIT) {             /* Process edit phonebook entry */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        UART_SEND_STR(F("AT+CPBW="));                       /* Send command */
        UART_SEND_STR(str);                                 /* Send index */
        UART_SEND_STR(F(",\""));
        UART_SEND_STR((char *)Pointers.CPtr1);              /* Send number */
        UART_SEND_STR(F("\",129,\""));                      /* Send national number format */
        UART_SEND_STR((char *)Pointers.CPtr2);              /* Send name */
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_EDIT, NULL);               /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PB_GET) {              /* Process read phonebook entry */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        UART_SEND_STR(F("AT+CPBR="));                       /* Send command */
        UART_SEND_STR(str);                                 /* Send index */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_GET, NULL);                /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PB_DELETE) {           /* Process delete phonebook entry */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        UART_SEND_STR(F("AT+CPBW="));                       /* Send command */
        UART_SEND_STR(str);                                 /* Send index */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_DELETE, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PB_LIST) {             /* Process get all phonebook entries */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CPBR="));                       /* Send command */
        NumberToString(str, (Pointers.UI >> 16) & 0xFFFF);  /* Convert number to string for start index */
        UART_SEND_STR(str);                                 /* Send start index */
        UART_SEND_STR(F(","));
        NumberToString(str, (Pointers.UI) & 0xFFFF);        /* Convert number to string for number of elements */
        UART_SEND_STR(str);                                 /* Send start index */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_LIST, NULL);               /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PB_SEARCH) {           /* Process search phonebook entries */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CPBF="));                       /* Send command */
        UART_SEND_STR(F("\""));
        UART_SEND_STR(Pointers.CPtr1);                      /* Send start index */
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_SEARCH, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    }
    
    PT_END(pt);                                             /* End thread */
}

gstatic
PT_THREAD(PT_Thread_DATETIME(struct pt* pt, gvol GSM_t* GSM)) {    
    PT_BEGIN(pt);                                           /* Begin thread */
    
//    GSM_RESET_EVENTS_RESP(GSM);                             /* Reset events */
//    UART_SEND_STR(F("AT+CLTS=1"));                          /* Send command */
//    UART_SEND_STR(GSM_CRLF);
//    StartCommand(GSM, CMD_DATETIME_GET, NULL);              /* Start command */
//    
//    PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
//                        GSM->Events.F.RespError);           /* Wait for response */
    
    if (GSM->ActiveCmd == CMD_DATETIME_GET) {               /* Process add phonebook entry */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CCLK?"));                       /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_DATETIME_GET, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    }
    
    PT_END(pt);                                             /* End thread */
}
gstatic
PT_THREAD(PT_Thread_GPRS(struct pt* pt, gvol GSM_t* GSM)) {
    char str[7], ch;
    char terminate = 26;
    gstatic uint32_t start;
    GSM_CONN_t* conn = (GSM_CONN_t *)Pointers.Ptr1;
    
    PT_BEGIN(pt);                                           /* Begin thread */
    
    if (GSM->ActiveCmd != CMD_GPRS_ATTACH && 
        GSM->ActiveCmd != CMD_GPRS_DETACH) {                /* Check if network attached */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        GSM_EXECUTE_NETWORK_CHECK(GSM);                     /* Check for network state */
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
    }

    if (GSM->ActiveCmd == CMD_GPRS_SETAPN) {                /* Process APN settings */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CSTT=\""));                     /* Send command */
        UART_SEND_STR(F(Pointers.CPtr1));
        UART_SEND_STR(F("\",\""));
        if (Pointers.CPtr2) {
            UART_SEND_STR(F(Pointers.CPtr2));
        }
        UART_SEND_STR(F("\",\""));
        if (Pointers.CPtr3) {
            UART_SEND_STR(F(Pointers.CPtr3));
        }
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_SETAPN, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_ATTACH) {         /* Network attach */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
    
        /**** Detach from network first if not already ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CGACT=0"));                     /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CGACT, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response and don't care what it actually is */
        
        /**** Attach to network ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CGACT=1"));                     /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CGACT, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }
        
        /**** Detach from network ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CGATT=0"));                     /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CGATT, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response and don't care what it actually is */
        
        /**** Attach to network ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CGATT=1"));                     /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CGATT, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }
        
        /**** Shutdown CIP connections ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CIPSHUT"));                     /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPSHUT, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }
        
        /**** Set multiple connections ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CIPMUX=1"));                    /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPMUX, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }
        
        /**** Receive data manually ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CIPRXGET=1"));                  /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPRXGET, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        } 
        
        /**** Set APN data ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CSTT=\""));                     /* Send command */
        UART_SEND_STR(F(Pointers.CPtr1));
        UART_SEND_STR(F("\",\""));
        if (Pointers.CPtr2) {
            UART_SEND_STR(F(Pointers.CPtr2));               /* Send username */
        }
        UART_SEND_STR(F("\",\""));
        if (Pointers.CPtr3) {
            UART_SEND_STR(F(Pointers.CPtr3));               /* Send password */
        }
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CSTT, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }

        /**** Start wireless connection ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CIICR"));                       /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIICR, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }

        /**** Get IP address ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CIFSR"));                       /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIFSR, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }
        
        /**** Terminate HTTP ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+HTTPTERM"));                    /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPTERM, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response, ignore it */
        
        /**** SAPBR start for HTTP ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+SAPBR=1,1"));                   /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_SAPBR, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response, ignore it */
        
cmd_gprs_attach_clean:
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        //GSM_IDLE(GSM);                                      /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_DETACH) {         /* Detach from network */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        
        /**** Stop SAPBR ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+SAPBR=0,1"));                   /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_SAPBR, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response, ignore it */
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CGATT=0"));                     /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CGATT, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPSTART) {       /* Start new connection as client */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CIPSTART=0,\""));               /* Send command */
        UART_SEND_STR(F(Pointers.CPtr2));                   /* TCP/UDP */
        UART_SEND_STR(F("\",\""));
        UART_SEND_STR(F(Pointers.CPtr1));                   /* Domain/IP */
        UART_SEND_STR(F("\","));
        UART_SEND_STR(F(str));                              /* Port number */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPSTART, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        if (GSM->ActiveResult == gsmOK) {
            PT_WAIT_UNTIL(pt, GSM->Events.F.RespConnectOk ||
                                GSM->Events.F.RespConnectFail ||
                                GSM->Events.F.RespConnectAlready); /* Wait for connect response */
            
            if (GSM->Events.F.RespConnectOk) {
                conn->ID = 0;                               /* Set connection ID */
                GSM->Conns[0] = (GSM_CONN_t *)Pointers.Ptr1;    /* Save connection pointer */
            }
        }
        
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPCLOSE) {       /* Close client connection */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CIPCLOSE=0"));                  /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPCLOSE, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespError || 
                            GSM->Events.F.RespCloseOk);     /* Wait for response */
        
        GSM->Conns[0]->Flags.F.Active = 0;                  /* Connection is not active anymore */
        
        GSM->ActiveResult = GSM->Events.F.RespCloseOk ? gsmOK : gsmERROR; /* Set result to return */
        
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPSEND) {
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CIPSEND=0"));                   /* Send number to GSM */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPSEND, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespBracket ||
                            GSM->Events.F.RespError);       /* Wait for > character and timeout */
        
        if (GSM->Events.F.RespBracket) {                    /* We received bracket */
            GSM_RESET_EVENTS_RESP(GSM);                     /* Reset events */
            UART_SEND((uint8_t *)Pointers.CPtr1, Pointers.UI);  /* Send data */
            UART_SEND_CH(&terminate);                       /* Send terminate character */
            
            PT_WAIT_UNTIL(pt, GSM->Events.F.RespSendOk ||
                                GSM->Events.F.RespSendFail);   /* Wait for OK or ERROR */
            
            GSM->ActiveResult = GSM->Events.F.RespSendOk ? gsmOK : gsmSENDFAIL; /* Set result to return */
        } else if (GSM->Events.F.RespError) {
            GSM->Flags.F.SMS_SendError = 1;                 /* Error flag */
            GSM->ActiveResult = gsmERROR;                   /* Process error */
        }
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPRXGET) {       /* Read data from device */
        GSM_CMD_SAVE(GSM);                                  /* Save command */

        start = GSM->Time;                                  /* Start counting */
        PT_WAIT_UNTIL(pt, GSM->Time - start >= conn->ReadTimeout);  /* Wait start timeout */
        
        if (conn->BytesToRead > 1460) {                     /* Check max read size */
            conn->BytesToRead = 1460;
        }
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+CIPRXGET=2,"));                 /* Send command */
        NumberToString(str, conn->ID);                      /* Convert number to string for connection ID */
        UART_SEND_STR(F(str));
        UART_SEND_STR(F(","));
        NumberToString(str, conn->BytesToRead);             /* Convert number to string */
        UART_SEND_STR(F(str));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPRXGET, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmOK) {
            conn->BytesReadTotal += conn->BytesRead;        /* Increase number of total read bytes */
            if (Pointers.Ptr2 != NULL) {
                *(uint16_t *)Pointers.Ptr2 = conn->BytesRead;   /* Save number of read bytes in last request */
            }
        }
        
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPBEGIN) {
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        
        /**** HTTP INIT ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+HTTPINIT"));                    /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPINIT, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_httpbegin_clean;                  
        }

        /**** HTTP PARAMETER CID ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+HTTPPARA=\"CID\",1"));          /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPINIT, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
cmd_gprs_httpbegin_clean:
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPSEND) {       /* Send data to HTTP buffer */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        
        /**** HTTP DATA ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+HTTPDATA="));                   /* Send command */
        NumberToString(str, GSM->HTTP.DataLength);          /* Send data length */
        UART_SEND_STR(F(str));
        UART_SEND_STR(F(",10000"));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPDATA, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespDownload ||
                            GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        if (!GSM->Events.F.RespError && GSM->Events.F.RespDownload) {
            GSM_DEBUG("Sending actual data!\r\n");
            UART_SEND(GSM->HTTP.Data, GSM->HTTP.DataLength);/* Send actual data! */
        } else {
            goto cmd_gprs_httpsend_clean;
        }
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_httpsend_clean;
        }
        
cmd_gprs_httpsend_clean:                                    /* Clean everything */
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE */ 
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPEXECUTE) {    /* Execute command */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        
        /**** HTTP PARA URL ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+HTTPPARA=\"URL\",\""));         /* Send command */
        UART_SEND_STR(F(GSM->HTTP.TMP));
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPPARA, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_httpexecute_clean;                  
        }

        /**** HTTP METHOD ****/
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+HTTPACTION="));                 /* Send command */
        ch = ((uint8_t)GSM->HTTP.Method) + '0';
        UART_SEND_CH(&ch);
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPACTION, NULL);       /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_httpexecute_clean;                  
        }
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespHttpAction);    /* Wait for response */
        
cmd_gprs_httpexecute_clean:                                 /* Clean everything */
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPREAD) {       /* Read data HTTP response */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+HTTPREAD="));                   /* Send command */
        NumberToString(str, GSM->HTTP.BytesReadTotal);      /* Convert number to string for connection ID */
        UART_SEND_STR(F(str));
        UART_SEND_STR(F(","));
        NumberToString(str, GSM->HTTP.BytesToRead);         /* Convert number to string */
        UART_SEND_STR(F(str));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPREAD, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmOK) {
            GSM->HTTP.BytesReadTotal += GSM->HTTP.BytesRead;    /* Increase number of total read bytes */
            if (Pointers.Ptr1 != NULL) {
                *(uint32_t *)Pointers.Ptr1 = GSM->HTTP.BytesRead;   /* Save number of read bytes for user */
            }
        }
        
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPCONTENT) {    /* Read data HTTP response */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+HTTPPARA=\"CONTENT\",\""));     /* Send command */
        UART_SEND_STR(F(GSM->HTTP.TMP));
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPPARA, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPTERM) {       /* Terminate HTTP */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+HTTPTERM"));                    /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPTERM, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_FTPBEGIN) {       /* Start FTP session */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+FTPCID=1"));                    /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPCID, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_FTPCONNECT) {     /* Set user data */
        GSM_CMD_SAVE(GSM);                                  /* Save command */
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+FTPSERV=\""));                  /* Send command */
        UART_SEND_STR(F(Pointers.CPtr1));
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPSERV, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpbegin_clean;
        }
        
        NumberToString(str, Pointers.UI);                   /* Convert port to string */
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+FTPPORT="));                    /* Send command */
        UART_SEND_STR(F(str));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPPORT, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpbegin_clean;
        }
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+FTPUN=\""));                    /* Send command */
        UART_SEND_STR(F(Pointers.CPtr2));
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPUN, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpbegin_clean;
        }
        
        GSM_RESET_EVENTS_RESP(GSM);                         /* Reset events */
        UART_SEND_STR(F("AT+FTPPW=\""));                    /* Send command */
        UART_SEND_STR(F(Pointers.CPtr3));
        UART_SEND_STR(F("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPPW, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */

cmd_gprs_ftpbegin_clean:                                    /* Clean everything */
        GSM_CMD_RESTORE(GSM);                               /* Restore command */
        GSM_IDLE(GSM);                                      /* Go IDLE mode */
    }
    
    PT_END(pt);                                             /* End thread */
}
/******************************************************************************/
/******************************************************************************/
/***                                Public API                               **/
/******************************************************************************/
/******************************************************************************/
GSM_Result_t GSM_Init(gvol GSM_t* G, const char* pin, uint32_t Baudrate) {
    BUFFER_t* Buff = &Buffer;
    GSM = G;                                                /* Save working pointer */
    
    memset((void *)GSM, 0x00, sizeof(GSM_t));               /* Clear structure first */
    
    /* Set start values */
    GSM->CPIN = GSM_CPIN_Unknown;                           /* Force SIM checking */
    
    GSM->Time = 0;                                          /* Reset time start time */
    BUFFER_Init(Buff, GSM_BUFFER_SIZE, Buffer_Data);        /* Init buffer for receive */
    
    /* Low-Level initialization */
    GSM->LL.Baudrate = Baudrate;
    if (GSM_LL_Init((GSM_LL_t *)&GSM->LL)) {                /* Init low-level */
        GSM_RETURN(GSM, gsmLLERROR);                        /* Return error */
    }
    
#if GSM_RTOS
    /* RTOS support */
    if (GSM_SYS_Create((GSM_RTOS_SYNC_t *)&GSM->Sync)) {    /* Init sync object */
        GSM_RETURN(GSM, gsmSYSERROR);
    }
#endif
    
    /* Init all threads */
    PT_INIT(&pt_GEN);
    PT_INIT(&pt_INFO);
    PT_INIT(&pt_PIN);
    PT_INIT(&pt_SMS);
    PT_INIT(&pt_CALL);
    PT_INIT(&pt_DATETIME);
    PT_INIT(&pt_GPRS);
    
    /* Send initialization commands */
    GSM->Flags.F.IsBlocking = 1;                            /* Process blocking calls */
    
    GSM_ACTIVE_CMD(GSM, CMD_GEN_FACTORY_SETTINGS);          /* Restore to factory settings */
    GSM_WaitReady(GSM, 1000);
    GSM_IDLE(GSM);
    GSM_ACTIVE_CMD(GSM, CMD_GEN_ERROR_NUMERIC);             /* Enable numeric error codes */
    GSM_WaitReady(GSM, 1000);
    GSM_IDLE(GSM);
    GSM_ACTIVE_CMD(GSM, CMD_GEN_CALL_CLCC);                 /* Enable auto notification for call, +CLCC statement */
    GSM_WaitReady(GSM, 1000);
    GSM_IDLE(GSM);
    Pointers.CPtr1 = pin;
    GSM_ACTIVE_CMD(GSM, CMD_PIN);                           /* Enable auto notification for call, +CLCC statement */
    GSM_WaitReady(GSM, 1000);
    GSM_IDLE(GSM);
    
    GSM->Flags.F.IsBlocking = 0;                            /* Reset blocking calls */
    
    GSM_RETURN(G, gsmOK);
}

GSM_Result_t GSM_WaitReady(gvol GSM_t* GSM, uint32_t timeout) {
    GSM->ActiveCmdTimeout = timeout;                        /* Set timeout value */
    do {
#if !GSM_RTOS && !GSM_UPDATE_ASYNC
        GSM_Update(GSM);
#endif
    } while (GSM_IS_BUSY(GSM));
    GSM_RETURN(GSM, GSM->ActiveResult);                     /* Return active result from command */
}

GSM_Result_t GSM_Delay(gvol GSM_t* GSM, uint32_t timeout) {
    gvol uint32_t start = GSM->Time;
    do {
#if !GSM_RTOS && !GSM_UPDATE_ASYNC
        GSM_Update(GSM);
#endif
    } while (GSM->Time - start < timeout);
    GSM_RETURN(GSM, gsmOK);
}

GSM_Result_t GSM_Update(gvol GSM_t* GSM) {
    char ch;
    static char prev_ch, prev2_ch;
    BUFFER_t* Buff = &Buffer;
    uint16_t processedCount = 500;
    
    if (GSM->ActiveCmd != CMD_IDLE && GSM->Time - GSM->ActiveCmdStart > GSM->ActiveCmdTimeout) {
        GSM->Events.F.RespError = 1;                        /* Set active error and process */
    }
    
    while (
#if !GSM_RTOS && GSM_UPDATE_ASYNC
        processedCount-- &&
#else
        processedCount &&
#endif
        BUFFER_Read(Buff, (uint8_t *)&ch, 1)                /* Read single character from buffer */
    ) {
        if (GSM->ActiveCmd == CMD_GPRS_HTTPREAD && GSM->Flags.F.HTTP_Read_Data) { /* We are trying to read raw data? */
            GSM->HTTP.Data[GSM->HTTP.BytesRead++] = ch;     /* Save character */
            GSM->HTTP.BytesReadRemaining--;                 /* Decrease number of remaining bytes to read */
            if (!GSM->HTTP.BytesReadRemaining) {            /* We finished? */
                GSM->Flags.F.HTTP_Read_Data = 0;            /* Reset flag, go back to normal parsing */
                processedCount = 0;                         /* Stop further processing */
            }
        } else if (GSM->ActiveCmd == CMD_GPRS_CIPRXGET && GSM->Flags.F.CLIENT_Read_Data) {  /* We are trying to read raw data? */
            GSM_CONN_t* conn = (GSM_CONN_t *)Pointers.Ptr1;
            conn->ReceiveData[conn->BytesRead++] = ch;      /* Save character */
            conn->BytesReadRemaining--;                     /* Decrease number of remaining bytes to read */
            if (!conn->BytesReadRemaining) {                /* We finished? */
                GSM->Flags.F.CLIENT_Read_Data = 0;          /* Reset flag, go back to normal parsing */
                conn->Flags.F.RxGetReceived = 0;            /* Reset RX received flag */
                processedCount = 0;                         /* Stop further processing */
            }
        } else if (GSM->ActiveCmd == CMD_SMS_READ && GSM->Flags.F.SMS_Read_Data) {  /* We are execution SMS read command and we are trying to read actual SMS data */
            GSM_SMS_Entry_t* read = (GSM_SMS_Entry_t *) Pointers.Ptr1;  /* Read pointer and cast to SMS entry */
            if (read->DataLen < (sizeof(read->Data) - 1)) { /* Still memory available? */
                read->Data[read->DataLen++] = ch;           /* Save character */
            }
            if (ch == '\n' && prev_ch == '\r' && read->DataLen >= 2) {  /* We finished? */
                if (read->Data[read->DataLen - 2] == '\r' && read->Data[read->DataLen - 1] == '\n') {
                    read->DataLen -= 2;                     /* Remove CRLF characters */
                }
                read->Data[read->DataLen] = 0;              /* Finish this statement */
                GSM->Flags.F.SMS_Read_Data = 0;             /* Reset flag, stop further processing */
                processedCount = 0;                         /* Stop further processing */
            }
        } else if (GSM->ActiveCmd == CMD_SMS_LIST && GSM->Flags.F.SMS_Read_Data) {   /* We received data for SMS as list command */
            if (*(uint16_t *)Pointers.Ptr2 < Pointers.UI) { /* If there is still memory available */
                GSM_SMS_Entry_t* read = (GSM_SMS_Entry_t *) Pointers.Ptr1;  /* Read pointer and cast to SMS entry */
                read->Data[read->DataLen++] = ch;           /* Save character */
                if (ch == '\n' && prev_ch == '\r' && read->DataLen >= 2) {  /* We finished? */
                    read->DataLen -= 2;                     /* Remove CRLF characters */
                    read->Data[read->DataLen] = 0;          /* Finish this statement */
                    
                    Pointers.Ptr1 = (GSM_SMS_Entry_t *)Pointers.Ptr1 + 1;  /* Increase pointer */ 
                    *(uint16_t *)Pointers.Ptr2 = (*(uint16_t *)Pointers.Ptr2) + 1;  /* Increase number of parsed elements */
                    
                    GSM->Flags.F.SMS_Read_Data = 0;         /* Clear flag, no more reading data */
                    processedCount = 0;                     /* Stop further processing */
                }
            } else if (ch == '\n' && prev_ch == '\r') {     /* Process dummy read, ignore messages if no memory specified buy still finish reading when done */
                GSM->Flags.F.SMS_Read_Data = 0;             /* Clear flag, no more reading data */
                processedCount = 0;                         /* Stop further processing */
            }
        } else {
            switch (ch) {
                case '\n':
                    GSM_RECEIVED_ADD(ch);                   /* Add character */
                    ParseReceived(GSM, &Received);          /* Parse received string */
                    GSM_RECEIVED_RESET();
                    break;
                default: 
                    if (ch == ' ' && prev_ch == '>' && prev2_ch == '\n') { /* Check if bracket received */
                        GSM->Events.F.RespBracket = 1;
                    }
                    GSM_RECEIVED_ADD(ch);                   /* Add character to buffer */
                break;
            }
        }
        prev2_ch = prev_ch;                                 /* Save previous character to prevprev character */
        prev_ch = ch;                                       /* Save current character as previous */      
    }
    return GSM_Process(GSM);                                /* Process stack */
}

GSM_Result_t GSM_Process(gvol GSM_t* GSM) {
    if (CMD_IS_ACTIVE_GENERAL(GSM)) {                       /* General related commands */
        PT_Thread_GEN(&pt_GEN, GSM);                       
    }
    if (CMD_IS_ACTIVE_INFO(GSM)) {                          /* General related commands */
        PT_Thread_INFO(&pt_INFO, GSM);                       
    }
    if (CMD_IS_ACTIVE_PIN(GSM)) {                           /* On active PIN related command */
        PT_Thread_PIN(&pt_PIN, GSM);
    }
    if (CMD_IS_ACTIVE_SMS(GSM)) {                           /* On active SMS related command */
        PT_Thread_SMS(&pt_SMS, GSM);
    }
    if (CMD_IS_ACTIVE_CALL(GSM)) {                          /* On active CALL related command */
        PT_Thread_CALL(&pt_CALL, GSM);
    }
    if (CMD_IS_ACTIVE_PB(GSM)) {                            /* On active PHONEBOOK related command */
        PT_Thread_PB(&pt_PB, GSM);
    }
    if (CMD_IS_ACTIVE_DATETIME(GSM)) {                      /* On active DATETIME related command */
        PT_Thread_DATETIME(&pt_DATETIME, GSM);
    }
    if (CMD_IS_ACTIVE_GPRS(GSM)) {                          /* On active GPRS related command */
        PT_Thread_GPRS(&pt_GPRS, GSM);
    }
#if !GSM_RTOS && !GSM_UPDATE_ASYNC
    GSM_ProcessCallbacks(GSM);                              /* Process callbacks */
#endif
    GSM_RETURN(GSM, gsmOK);
}

GSM_Result_t GSM_UpdateTime(gvol GSM_t* GSM, uint32_t ticksNum) {
    GSM->Time += ticksNum;                                  /* Increase milliseconds for amount of ticks */
    GSM_RETURN(GSM, gsmOK);
}

GSM_Result_t GSM_ProcessCallbacks(gvol GSM_t* GSM) {
    /* Process callbacks */
    if (GSM->ActiveCmd == CMD_IDLE && GSM->Flags.F.Call_Idle) {
        GSM->Flags.F.Call_Idle = 0;
        GSM_Callback_Idle(GSM);
    }
    if (GSM_IS_READY(GSM) && GSM->Flags.F.CALL_CLCC_Received) {
        GSM->Flags.F.CALL_CLCC_Received = 0;
        GSM_Callback_CALL_Info(GSM);
    }
    if (GSM_IS_READY(GSM) && GSM->Flags.F.CALL_RING_Received) {
        GSM->Flags.F.CALL_RING_Received = 0;
        GSM_Callback_CALL_Ring(GSM);
    }
    if (GSM_IS_READY(GSM) && GSM->Flags.F.SMS_CMTI_Received) {
        GSM->Flags.F.SMS_CMTI_Received = 0;
        GSM_Callback_SMS_Info(GSM);
    }
    GSM_RETURN(GSM, gsmOK);
}

GSM_Result_t GSM_GetLastReturnStatus(gvol GSM_t* GSM) {
    GSM_Result_t tmp = GSM->ActiveResult;
    GSM->ActiveResult = gsmOK;
    
    return tmp;
}

uint32_t GSM_DataReceived(uint8_t* ch, uint32_t count) {
    BUFFER_t* Buff = &Buffer;
    return BUFFER_Write(Buff, ch, count);                   /* Write data to internal buffer and return number of written elements */
}

/******************************************************************************/
/***                                INFO API                                 **/
/******************************************************************************/
GSM_Result_t GSM_INFO_GetManufacturer(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking) {
    GSM_CHECK_INPUTS(str != NULL);                          /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_INFO_CGMI);                     /* Set active command */
    
    Pointers.Ptr1 = str;                                    /* Save pointer to PIN */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_INFO_GetModel(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking) {
    GSM_CHECK_INPUTS(str != NULL);                          /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_INFO_CGMM);                     /* Set active command */
    
    Pointers.Ptr1 = str;                                    /* Save pointer to PIN */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_INFO_GetRevision(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking) {
    GSM_CHECK_INPUTS(str != NULL);                          /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_INFO_CGMR);                     /* Set active command */
    
    Pointers.Ptr1 = str;                                    /* Save pointer to PIN */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_INFO_GetSerialNumber(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking) {
    GSM_CHECK_INPUTS(str != NULL);                          /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_INFO_CGSN);                     /* Set active command */
    
    Pointers.Ptr1 = str;                                    /* Save pointer to PIN */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

/******************************************************************************/
/***                               PIN/PUK API                               **/
/******************************************************************************/
GSM_Result_t GSM_PIN_Enter(gvol GSM_t* GSM, const char* pin, uint32_t blocking) {
    GSM_CHECK_INPUTS(pin != NULL);                          /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_PIN);                           /* Set active command */
    
    Pointers.CPtr1 = pin;                                   /* Save pointer to PIN */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_PIN_Remove(gvol GSM_t* GSM, const char* current_pin, uint32_t blocking) {
    GSM_CHECK_INPUTS(current_pin != NULL);                  /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy flag */
    GSM_ACTIVE_CMD(GSM, CMD_PIN_REMOVE);                    /* Set active command */
    
    Pointers.CPtr1 = current_pin;                           /* Save pointer to PIN */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_PIN_Add(gvol GSM_t* GSM, const char* new_pin, uint32_t blocking) {
    GSM_CHECK_INPUTS(new_pin != NULL);                      /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy flag */
    GSM_ACTIVE_CMD(GSM, CMD_PIN_ADD);                       /* Set active command */
    
    Pointers.CPtr1 = new_pin;                               /* Save pointer to PIN */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_PUK_Enter(gvol GSM_t* GSM, const char* puk, const char* new_pin, uint32_t blocking) {
    GSM_CHECK_INPUTS(puk != NULL && new_pin != NULL);       /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_PUK);                           /* Set active command */
    
    Pointers.CPtr1 = puk;                                   /* Save pointer to PUK */
    Pointers.CPtr2 = new_pin;                               /* Save pointer to new PIN */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

/******************************************************************************/
/***                                CALL API                                 **/
/******************************************************************************/
GSM_Result_t GSM_CALL_Voice(gvol GSM_t* GSM, const char* number, uint32_t blocking) {
    GSM_CHECK_INPUTS(number != NULL);                       /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_CALL_VOICE);                    /* Set active command */
    
    Pointers.CPtr1 = number;                                /* Save pointer to PIN */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);
}

GSM_Result_t GSM_CALL_VoiceFromSIMPosition(gvol GSM_t* GSM, uint16_t pos, uint32_t blocking) {
    GSM_CHECK_INPUTS(pos);                                  /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_CALL_VOICE_SIM_POS);            /* Set active command */
    
    Pointers.UI = pos;                                      /* Save position */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);
}

GSM_Result_t GSM_CALL_DataFromSIMPosition(gvol GSM_t* GSM, uint16_t pos, uint32_t blocking) {
    GSM_CHECK_INPUTS(pos);                                  /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_CALL_DATA_SIM_POS);             /* Set active command */
    
    Pointers.UI = pos;                                      /* Save position */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);
}

GSM_Result_t GSM_CALL_Data(gvol GSM_t* GSM, const char* number, uint32_t blocking) {
    GSM_CHECK_INPUTS(number != NULL);                       /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_CALL_DATA);                     /* Set active command */
    
    Pointers.CPtr1 = number;                                /* Save pointer to PIN */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return as blocking if required */
}

GSM_Result_t GSM_CALL_Answer(gvol GSM_t* GSM, uint32_t blocking) {
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_CALL_ANSWER);                   /* Set active command */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_CALL_HangUp(gvol GSM_t* GSM, uint32_t blocking) {
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_CALL_HANGUP);                   /* Set active command */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */   
}

GSM_CallInfo_t* GSM_CALL_GetInfo(gvol GSM_t* GSM, uint32_t blocking) {
    return (GSM_CallInfo_t *)&GSM->CallInfo;
}

GSM_Result_t GSM_CALL_ClearInfo(gvol GSM_t* GSM, GSM_CallInfo_t* info, uint32_t blocking) {
    return gsmOK;
}

/******************************************************************************/
/***                                 SMS API                                 **/
/******************************************************************************/
GSM_Result_t GSM_SMS_Send(gvol GSM_t* GSM, const char* number, const char* data, uint32_t blocking) {
    GSM_CHECK_INPUTS(number != NULL && 
        data != NULL && strlen(data) <= GSM_SMS_MAX_LENGTH);/* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_SMS_SEND);                      /* Set active command */
    
    GSM->SMS.Data = data;
    GSM->SMS.Number = number;
    
    GSM_RETURN_BLOCKING(GSM, blocking, 30000);              /* Return with blocking possibility */
}

GSM_Result_t GSM_SMS_Read(gvol GSM_t* GSM, GSM_SMS_Entry_t* SMS, uint16_t position, uint32_t blocking) {
    GSM_CHECK_INPUTS(SMS != NULL);                          /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_SMS_READ);                      /* Set active command */
    
    SMS->Position = position;                               /* Save position for SMS */
    Pointers.Ptr1 = SMS;                                    /* Save non-const pointer */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 500);                /* Return with blocking possibility */
}

GSM_Result_t GSM_SMS_List(gvol GSM_t* GSM, GSM_SMS_ReadType_t type, GSM_SMS_Entry_t* entries, uint16_t entries_count, uint16_t* entries_read, uint32_t blocking) {
    GSM_CHECK_INPUTS(entries != NULL 
                        && entries_read != NULL);           /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_SMS_LIST);                      /* Set active command */
    
    *entries_read = 0;                                      /* Reset variable */
    
    switch (type) {                                         /* Check for proper type */
        case GSM_SMS_ReadType_ALL: Pointers.CPtr1 = F("ALL"); break;
        case GSM_SMS_ReadType_READ: Pointers.CPtr1 = F("READ"); break;
        case GSM_SMS_ReadType_UNREAD: Pointers.CPtr1 = F("UNREAD"); break;
        default: return gsmPARERR;
    }
    Pointers.Ptr1 = entries;                                /* Save pointer to entries */
    Pointers.Ptr2 = entries_read;                           /* Save pointer to entries we already read */
    Pointers.UI = entries_count;                            /* Save number of entries we can save in entries */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking possibility */
}

GSM_Result_t GSM_SMS_Delete(gvol GSM_t* GSM, uint16_t position, uint32_t blocking) {
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_SMS_DELETE);                    /* Set active command */
    
    Pointers.UI = position;                                 /* Save number */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 50);                 /* Return with blocking possibility */
}

GSM_Result_t GSM_SMS_MassDelete(gvol GSM_t* GSM, GSM_SMS_MassDelete_t Type, uint32_t blocking) {
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_SMS_MASSDELETE);                /* Set active command */
    
    switch (Type) {
        case GSM_SMS_MassDelete_Read:
            Pointers.CPtr1 = F("READ");
            break;
        case GSM_SMS_MassDelete_Unread:
            Pointers.CPtr1 = F("UNREAD");
            break;
        case GSM_SMS_MassDelete_Sent:
            Pointers.CPtr1 = F("SENT");
            break;
        case GSM_SMS_MassDelete_Unsent:
            Pointers.CPtr1 = F("UNSENT");
            break;
        case GSM_SMS_MassDelete_Inbox:
            Pointers.CPtr1 = F("INBOX");
            break;
        case GSM_SMS_MassDelete_All:
            Pointers.CPtr1 = F("ALL");
            break;
        default:
            break;
    }
    GSM_RETURN_BLOCKING(GSM, blocking, 50);                 /* Return with blocking possibility */   
}

GSM_SmsInfo_t* GSM_SMS_GetReceivedInfo(gvol GSM_t* GSM, uint32_t blocking) {
    uint8_t i = 0;
    for (i = 0; i < GSM_MAX_RECEIVED_SMS_INFO; i++) {
        if (GSM->SmsInfos[i].Flags.F.Received) {            /* Check if any new SMS */
            GSM->SmsInfos[i].Flags.F.Received = 0;          /* Reset flag */
            GSM->SmsInfos[i].Flags.F.Used = 1;              /* Set used flag */
            return (GSM_SmsInfo_t *)&GSM->SmsInfos[i];      /* Get pointer value */
        }
    }
    
    return NULL;
}

GSM_Result_t GSM_SMS_ClearReceivedInfo(gvol GSM_t* GSM, GSM_SmsInfo_t* info, uint32_t blocking) {
    info->Flags.Value = 0;                                  /* Reset all flags */
    
    return gsmOK;
}

/******************************************************************************/
/***                              PHONEBOOK API                              **/
/******************************************************************************/
GSM_Result_t GSM_PB_Add(gvol GSM_t* GSM, const char* name, const char* number, uint32_t blocking) {
    GSM_CHECK_INPUTS(name != NULL && number != NULL);       /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_PB_ADD);                        /* Set active command */
    
    Pointers.CPtr1 = number;                                /* Save pointer to number */
    Pointers.CPtr2 = name;                                  /* Save pointer to name */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_PB_Edit(gvol GSM_t* GSM, uint32_t index, const char* name, const char* number, uint32_t blocking) {
    GSM_CHECK_INPUTS(index && name != NULL && number != NULL);  /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_PB_EDIT);                       /* Set active command */
    
    Pointers.CPtr1 = number;                                /* Save pointer to number */
    Pointers.CPtr2 = name;                                  /* Save pointer to name */
    Pointers.UI = index;                                    /* Save index */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_PB_Delete(gvol GSM_t* GSM, uint32_t index, uint32_t blocking) {
    GSM_CHECK_INPUTS(index);                                /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_PB_DELETE);                     /* Set active command */
    
    Pointers.UI = index;                                    /* Save index */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_PB_Get(gvol GSM_t* GSM, GSM_PB_Entry_t* entry, uint32_t index, uint32_t blocking) {
    GSM_CHECK_INPUTS(index && entry != NULL);               /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_PB_GET);                        /* Set active command */
    
    Pointers.Ptr1 = entry;                                  /* Save pointer to entry object */
    Pointers.UI = index;                                    /* Save index */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_PB_List(gvol GSM_t* GSM, GSM_PB_Entry_t* entries, uint16_t start_index, uint16_t entries_count, uint16_t* entries_read, uint32_t blocking) {
    GSM_CHECK_INPUTS(entries != NULL && start_index 
                && entries_count && entries_read != NULL);  /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_PB_LIST);                       /* Set active command */
    
    *entries_read = 0;                                      /* Reset count first */
    
    Pointers.Ptr1 = entries;                                /* Save pointer to entries */
    Pointers.Ptr2 = entries_read;                           /* Save pointer to number of entries read */
    Pointers.UI = start_index << 16 | ((start_index + entries_count - 1) & 0xFFFF); /* Save start index and entries count to read */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_PB_Search(gvol GSM_t* GSM, const char* search, GSM_PB_Entry_t* entries, uint16_t entries_count, uint16_t* entries_read, uint32_t blocking) {
    GSM_CHECK_INPUTS(search != NULL && entries != NULL && entries_count 
                && entries_read != NULL);  /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_PB_SEARCH);                     /* Set active command */
    
    *entries_read = 0;                                      /* Reset count first */
    
    Pointers.CPtr1 = search;                                /* Save pointer to search string */
    Pointers.Ptr1 = entries;                                /* Save pointer to entries */
    Pointers.Ptr2 = entries_read;                           /* Save pointer to number of entries read */
    Pointers.UI = entries_count;                            /* Save number of max elements we can read from search command */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}
/******************************************************************************/
/***                              DATETIME API                               **/
/******************************************************************************/
GSM_Result_t GSM_DATETIME_Get(gvol GSM_t* GSM, GSM_DateTime_t* datetime, uint32_t blocking) {
    GSM_CHECK_INPUTS(datetime != NULL);                     /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_DATETIME_GET);                  /* Set active command */
    
    Pointers.Ptr1 = datetime;                               /* Save pointer to entries */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

/******************************************************************************/
/***                                GPRS API                                 **/
/******************************************************************************/
GSM_Result_t GSM_GPRS_Attach(gvol GSM_t* GSM, const char* apn, const char* user, const char* pwd, uint32_t blocking) {
    GSM_CHECK_INPUTS(apn != NULL);                          /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_ATTACH);                   /* Set active command */
     
    Pointers.CPtr1 = apn;                                   /* Save pointers */
    Pointers.CPtr2 = NULL;
    Pointers.CPtr3 = NULL;
    if (user && strlen(user)) {
        Pointers.CPtr2 = user;                              /* Set pointer to username */
    }
    if (pwd && strlen(pwd)) {
        Pointers.CPtr3 = pwd;                               /* Set pointer to password */
    }   
    GSM_RETURN_BLOCKING(GSM, blocking, 180000);             /* Return with blocking support */
}

GSM_Result_t GSM_GPRS_Detach(gvol GSM_t* GSM, uint32_t blocking) {
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_DETACH);                   /* Set active command */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

/******************************************************************************/
/***                           CLIENT TCP/UDP API                            **/
/******************************************************************************/
GSM_Result_t GSM_CONN_ConnStart(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, 
    GSM_CONN_ConnType_t type, const char* host, uint16_t port, uint32_t blocking) {
        
    GSM_CHECK_INPUTS(conn != NULL && host != NULL);         /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_CIPSTART);                 /* Set active command */
     
    Pointers.Ptr1 = conn;                                   /* Save connection pointer */
    Pointers.CPtr1 = host;                                  /* Save pointers */
    if (type == GSM_CONN_ConnType_TCP) {
        Pointers.CPtr2 = F("TCP");                          /* TCP connection */
    } else {
        Pointers.CPtr2 = F("UDP");                          /* UDP connection */
    }
    Pointers.UI = port;                                     /* Save port */
    
    conn->ID = 0;                                           /* Set connection ID */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_CONN_ConnSend(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, const void* data, uint16_t length, uint32_t blocking) {
    GSM_CHECK_INPUTS(conn != NULL && data != NULL && length);   /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_CIPSEND);                  /* Set active command */
     
    Pointers.Ptr1 = conn;                                   /* Save connection pointer */
    Pointers.CPtr1 = data;                                  /* Save data pointer */
    Pointers.UI = length;                                   /* Save data length */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_CONN_ConnReceive(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, void* data, uint16_t length, uint16_t* read, uint16_t timeBeforeRead, uint32_t blocking) {
    GSM_CHECK_INPUTS(conn != NULL && data != NULL && length);   /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_CIPRXGET);                 /* Set active command */
    
    conn->ReceiveData = data;                               /* Save pointer to data */
    conn->BytesToRead = length;
    conn->BytesRead = 0;
    conn->ReadTimeout = timeBeforeRead;                     /* Set time before reading */
    
    Pointers.Ptr1 = conn;                                   /* Save connection pointer */
    Pointers.Ptr2 = read;                                   /* Save read pointer */
    if (read != NULL) {                                     /* Reset counter */
        *read = 0;
    }
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_CONN_ConnClose(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, uint32_t blocking) {
    GSM_CHECK_INPUTS(conn != NULL);                         /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_CIPCLOSE);                 /* Set active command */
     
    Pointers.Ptr1 = conn;                                   /* Save connection pointer */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

uint32_t GSM_CONN_DataAvailable(gvol GSM_t* GSM, const gvol GSM_CONN_t* conn, uint32_t blocking) {
    if (conn == NULL) {                                     /* Check valid connection */
        return 0;
    }
    return conn->BytesRemaining || conn->Flags.F.RxGetReceived; /* At least one should be more than zero */
}

/******************************************************************************/
/***                                 HTTP API                                **/
/******************************************************************************/
GSM_Result_t GSM_HTTP_Begin(gvol GSM_t* GSM, uint32_t blocking) {
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_HTTPBEGIN);                /* Set active command */
    
    memset((void *)&GSM->HTTP, 0x00, sizeof(GSM->HTTP));    /* Set structure to zero */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_HTTP_End(gvol GSM_t* GSM, uint32_t blocking) {
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_HTTPTERM);                 /* Set active command */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_HTTP_SetData(gvol GSM_t* GSM, const void* data, uint32_t length, uint32_t blocking) {
    GSM_CHECK_INPUTS(data != NULL && length);               /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_HTTPSEND);                 /* Set active command */
    
    GSM->HTTP.Data = (uint8_t *)data;                       /* Save data pointer */
    GSM->HTTP.DataLength = length;                          /* Set data length */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_HTTP_SetContent(gvol GSM_t* GSM, const char* content, uint32_t blocking) {
    GSM_CHECK_INPUTS(content != NULL);                      /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_HTTPCONTENT);              /* Set active command */
    
    GSM->HTTP.TMP = content;                                /* Save content pointer */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_HTTP_Execute(gvol GSM_t* GSM, const char* url, GSM_HTTP_Method_t method, uint32_t blocking) {
    GSM_CHECK_INPUTS(url != NULL);                          /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_HTTPEXECUTE);              /* Set active command */
    
    /* Reset informations */
    GSM->HTTP.BytesRead = 0;
    GSM->HTTP.BytesReadRemaining = 0;
    GSM->HTTP.BytesReadTotal = 0;
    GSM->HTTP.BytesReceived = 0;
    GSM->HTTP.BytesToRead = 0;
    
    
    GSM->HTTP.TMP = url;                                    /* Save URL */
    GSM->HTTP.Method = method;                              /* Set request method */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_HTTP_Read(gvol GSM_t* GSM, void* data, uint32_t length, uint32_t* read, uint32_t blocking) {
    GSM_CHECK_INPUTS(data != NULL && length);               /* Check valid data */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_HTTPREAD);                 /* Set active command */
    
    GSM->HTTP.Data = data;                                  /* Set data pointer */
    GSM->HTTP.DataLength = length;                          /* Set data array length */
    GSM->HTTP.BytesToRead = length;                         /* Set number of bytes to read from response */
    Pointers.Ptr1 = read;
    if (read != NULL) {                                     /* Reset counter */
        *read = 0;
    }
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

uint32_t GSM_HTTP_DataAvailable(gvol GSM_t* GSM, uint32_t blocking) {
    return GSM->HTTP.BytesReceived - GSM->HTTP.BytesReadTotal;
}

/******************************************************************************/
/***                                 FTP API                                 **/
/******************************************************************************/
GSM_Result_t GSM_FTP_Begin(gvol GSM_t* GSM, uint32_t blocking) {
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_FTPBEGIN);                 /* Set active command */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_FTP_End(gvol GSM_t* GSM, uint32_t blocking) {
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_FTPEND);                   /* Set active command */
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_FTP_Connect(gvol GSM_t* GSM, const char* server, uint16_t port, const char* user, const char* pass, uint32_t blocking) {
    GSM_CHECK_INPUTS(server != NULL && port && 
                        user != NULL && pass != NULL);      /* Check input values */
    GSM_CHECK_BUSY(GSM);                                    /* Check busy status */
    GSM_ACTIVE_CMD(GSM, CMD_GPRS_FTPCONNECT);               /* Set active command */
    
    Pointers.CPtr1 = server;                                /* Save pointers */
    Pointers.CPtr2 = user;
    Pointers.CPtr3 = pass;
    Pointers.UI = port;
    
    GSM_RETURN_BLOCKING(GSM, blocking, 1000);               /* Return with blocking support */
}

GSM_Result_t GSM_FTP_UploadFile(gvol GSM_t* GSM, const char* folder, const char* file, const void* data, uint32_t length, uint32_t blocking) {

}

GSM_Result_t GSM_FTP_Disconnect(gvol GSM_t* GSM, uint32_t blocking) {

}

/******************************************************************************/
/***                              CALLBACKS API                              **/
/******************************************************************************/
__weak void GSM_Callback_Idle(gvol GSM_t* GSM) {
    /* NOTE: This function Should not be modified, when the callback is needed,
           the GSM_Callback_Idle should be implemented in the user file
    */
}

__weak void GSM_Callback_CALL_Info(gvol GSM_t* GSM) {
    /* NOTE: This function Should not be modified, when the callback is needed,
           the GSM_Callback_CALL_Info should be implemented in the user file
    */
}

__weak void GSM_Callback_CALL_Ring(gvol GSM_t* GSM) {
    /* NOTE: This function Should not be modified, when the callback is needed,
           the GSM_Callback_CALL_Ring should be implemented in the user file
    */
}

__weak void GSM_Callback_SMS_Info(gvol GSM_t* GSM) {
    /* NOTE: This function Should not be modified, when the callback is needed,
           the GSM_Callback_SMS_Info should be implemented in the user file
    */
}
