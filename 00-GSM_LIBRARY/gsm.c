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
#include "math.h"

/******************************************************************************/
/******************************************************************************/
/***                           Private structures                            **/
/******************************************************************************/
/******************************************************************************/
typedef struct {
    uint8_t Length;
    uint8_t Data[128];
} Received_t;
#define RECEIVED_ADD(c)                     do { Received.Data[Received.Length++] = (c); Received.Data[Received.Length] = 0; } while (0)
#define RECEIVED_RESET()                    do { Received.Length = 0; Received.Data[0] = 0; } while (0)
#define RECEIVED_LENGTH()                   Received.Length

typedef struct {
    gvol const void* CPtr1;
    gvol const void* CPtr2;
    gvol const void* CPtr3;
    gvol void* Ptr1;
    gvol void* Ptr2;
    gvol uint32_t UI;
} Pointers_t;

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
#define CHARISNUM(x)                        ((x) >= '0' && (x) <= '9')
#define CHARISHEXNUM(x)                     (((x) >= '0' && (x) <= '9') || ((x) >= 'a' && (x) <= 'f') || ((x) >= 'A' && (x) <= 'F'))
#define CHARTONUM(x)                        ((x) - '0')
#define CHARHEXTONUM(x)                     (((x) >= '0' && (x) <= '9') ? ((x) - '0') : (((x) >= 'a' && (x) <= 'z') ? ((x) - 'a' + 10) : (((x) >= 'A' && (x) <= 'Z') ? ((x) - 'A' + 10) : 0)))
#define FROMMEM(x)                          ((const char *)(x))
    
#define UART_SEND_STR(str)                  GSM_LL_SendData((GSM_LL_t *)&GSM->LL, (const uint8_t *)(str), strlen((const char *)(str)))
#define UART_SEND(str, len)                 GSM_LL_SendData((GSM_LL_t *)&GSM->LL, (const uint8_t *)(str), (len))
#define UART_SEND_CH(ch)                    GSM_LL_SendData((GSM_LL_t *)&GSM->LL, (const uint8_t *)(ch), 1)

#define GSM_OK                              FROMMEM("OK\r\n")
#define GSM_ERROR                           FROMMEM("ERROR\r\n")
#define GSM_BUSY                            FROMMEM("BUSY\r\n")
#define GSM_NO_CARRIER                      FROMMEM("NO CARRIER\r\n")
#define GSM_RING                            FROMMEM("RING\r\n")
#define GSM_CRLF                            FROMMEM("\r\n")

/* List of active commands */
#define CMD_IDLE                            ((uint16_t)0x0000)
#define CMD_GEN_SMSNOTIFY                   ((uint16_t)0x0001)
#define CMD_GEN_ERROR_NUMERIC               ((uint16_t)0x0002)
#define CMD_GEN_CALL_CLCC                   ((uint16_t)0x0003)
#define CMD_GEN_FACTORY_SETTINGS            ((uint16_t)0x0004)
#define CMD_GEN_AT                          ((uint16_t)0x0005)
#define CMD_GEN_ATE0                        ((uint16_t)0x0006)
#define CMD_GEN_ATE1                        ((uint16_t)0x0007)
#define CMD_GEN_CFUN                        ((uint16_t)0x0008)
#define CMD_GEN_CFUN_SET                    ((uint16_t)0x0009)
#define CMD_GEN_CFUN_GET                    ((uint16_t)0x000A)
#define CMD_IS_ACTIVE_GENERAL(p)            ((p)->ActiveCmd >= 0x0001 && (p)->ActiveCmd < 0x0100)

#define CMD_PIN                             ((uint16_t)0x0101)
#define CMD_PUK                             ((uint16_t)0x0102)
#define CMD_PIN_REMOVE                      ((uint16_t)0x0103)
#define CMD_PIN_ADD                         ((uint16_t)0x0104)
#define CMD_IS_ACTIVE_PIN(p)                ((p)->ActiveCmd >= 0x0100 && (p)->ActiveCmd < 0x0200)

#define CMD_SMS                             ((uint16_t)0x0200)
#define CMD_SMS_SEND                        ((uint16_t)0x0201)
#define CMD_SMS_READ                        ((uint16_t)0x0202)
#define CMD_SMS_DELETE                      ((uint16_t)0x0203)
#define CMD_SMS_MASSDELETE                  ((uint16_t)0x0204)
#define CMD_SMS_LIST                        ((uint16_t)0x0205)
#define CMD_SMS_CMGF                        ((uint16_t)0x0210)
#define CMD_SMS_CMGS                        ((uint16_t)0x0211)
#define CMD_SMS_CMGR                        ((uint16_t)0x0212)
#define CMD_SMS_CMGD                        ((uint16_t)0x0213)
#define CMD_IS_ACTIVE_SMS(p)                ((p)->ActiveCmd >= 0x0200 && (p)->ActiveCmd < 0x0300)

#define CMD_CALL                            ((uint16_t)0x0300)
#define CMD_CALL_VOICE                      ((uint16_t)0x0301)
#define CMD_CALL_DATA                       ((uint16_t)0x0302)
#define CMD_CALL_ANSWER                     ((uint16_t)0x0303)
#define CMD_CALL_HANGUP                     ((uint16_t)0x0304)
#define CMD_CALL_VOICE_SIM_POS              ((uint16_t)0x0305)
#define CMD_CALL_DATA_SIM_POS               ((uint16_t)0x0306)
#define CMD_IS_ACTIVE_CALL(p)               ((p)->ActiveCmd >= 0x0300 && (p)->ActiveCmd < 0x0400)

#define CMD_INFO                            ((uint16_t)0x0400)
#define CMD_INFO_CGMM                       ((uint16_t)0x0401)
#define CMD_INFO_CGMI                       ((uint16_t)0x0402)
#define CMD_INFO_CGMR                       ((uint16_t)0x0403)
#define CMD_INFO_CNUM                       ((uint16_t)0x0404)
#define CMD_INFO_CGSN                       ((uint16_t)0x0405)
#define CMD_INFO_GMR                        ((uint16_t)0x0406)
#define CMD_INFO_CBC                        ((uint16_t)0x0407)
#define CMD_IS_ACTIVE_INFO(p)               ((p)->ActiveCmd >= 0x0400 && (p)->ActiveCmd < 0x0500)

#define CMD_PB                              ((uint16_t)0x0500)
#define CMD_PB_ADD                          ((uint16_t)0x0501)
#define CMD_PB_EDIT                         ((uint16_t)0x0502)
#define CMD_PB_DELETE                       ((uint16_t)0x0503)
#define CMD_PB_GET                          ((uint16_t)0x0504)
#define CMD_PB_LIST                         ((uint16_t)0x0505)
#define CMD_PB_SEARCH                       ((uint16_t)0x0506)
#define CMD_IS_ACTIVE_PB(p)                 ((p)->ActiveCmd >= 0x0500 && (p)->ActiveCmd < 0x0600)

#define CMD_DATETIME                        ((uint16_t)0x0600)
#define CMD_DATETIME_GET                    ((uint16_t)0x0601)
#define CMD_IS_ACTIVE_DATETIME(p)           ((p)->ActiveCmd >= 0x0600 && (p)->ActiveCmd < 0x0700)

#define CMD_GPRS                            ((uint16_t)0x0700)
#define CMD_GPRS_SETAPN                     ((uint16_t)0x0701)
#define CMD_GPRS_ATTACH                     ((uint16_t)0x0702)
#define CMD_GPRS_DETACH                     ((uint16_t)0x0703)
#define CMD_GPRS_HTTPBEGIN                  ((uint16_t)0x0704)
#define CMD_GPRS_HTTPEND                    ((uint16_t)0x0705)
#define CMD_GPRS_HTTPEXECUTE                ((uint16_t)0x0706)
#define CMD_GPRS_HTTPSEND                   ((uint16_t)0x0707)
#define CMD_GPRS_HTTPCONTENT                ((uint16_t)0x0708)
#define CMD_GPRS_FTPBEGIN                   ((uint16_t)0x0709)
#define CMD_GPRS_FTPEND                     ((uint16_t)0x070A)
#define CMD_GPRS_FTPAUTH                    ((uint16_t)0x070B)
#define CMD_GPRS_FTPDOWN                    ((uint16_t)0x070C)
#define CMD_GPRS_FTPDOWNBEGIN               ((uint16_t)0x070D)
#define CMD_GPRS_FTPDOWNEND                 ((uint16_t)0x070E)
#define CMD_GPRS_FTPUP                      ((uint16_t)0x070F)
#define CMD_GPRS_FTPUPBEGIN                 ((uint16_t)0x0710)
#define CMD_GPRS_FTPUPEND                   ((uint16_t)0x0711)

#define CMD_GPRS_CIPSHUT                    ((uint16_t)0x0720)
#define CMD_GPRS_CGATT                      ((uint16_t)0x0721)
#define CMD_GPRS_CGACT                      ((uint16_t)0x0722)
#define CMD_GPRS_SAPBR                      ((uint16_t)0x0723)
#define CMD_GPRS_CIICR                      ((uint16_t)0x0724)
#define CMD_GPRS_CIFSR                      ((uint16_t)0x0725)
#define CMD_GPRS_CSTT                       ((uint16_t)0x0726)
#define CMD_GPRS_CIPMUX                     ((uint16_t)0x0727)
#define CMD_GPRS_CIPSTATUS                  ((uint16_t)0x0728)
#define CMD_GPRS_CIPSTART                   ((uint16_t)0x0729)
#define CMD_GPRS_CIPSEND                    ((uint16_t)0x072A)
#define CMD_GPRS_CIPCLOSE                   ((uint16_t)0x072B)
#define CMD_GPRS_CIPRXGET                   ((uint16_t)0x072C)
#define CMD_GPRS_HTTPINIT                   ((uint16_t)0x072D)
#define CMD_GPRS_HTTPPARA                   ((uint16_t)0x072E)
#define CMD_GPRS_HTTPDATA                   ((uint16_t)0x072F)
#define CMD_GPRS_HTTPACTION                 ((uint16_t)0x0730)
#define CMD_GPRS_HTTPREAD                   ((uint16_t)0x0731)
#define CMD_GPRS_HTTPTERM                   ((uint16_t)0x0732)
#define CMD_GPRS_CREG                       ((uint16_t)0x0733)
#define CMD_GPRS_FTPCID                     ((uint16_t)0x0734)
#define CMD_GPRS_FTPSERV                    ((uint16_t)0x0735)
#define CMD_GPRS_FTPPORT                    ((uint16_t)0x0736)
#define CMD_GPRS_FTPUN                      ((uint16_t)0x0737)
#define CMD_GPRS_FTPPW                      ((uint16_t)0x0738)
#define CMD_GPRS_FTPPUTNAME                 ((uint16_t)0x0739)
#define CMD_GPRS_FTPPUTPATH                 ((uint16_t)0x073A)
#define CMD_GPRS_FTPPUT                     ((uint16_t)0x073B)
#define CMD_GPRS_FTPGETPATH                 ((uint16_t)0x073C)
#define CMD_GPRS_FTPGETNAME                 ((uint16_t)0x073D)
#define CMD_GPRS_FTPGET                     ((uint16_t)0x073E)
#define CMD_GPRS_FTPPUTOPT                  ((uint16_t)0x073F)
#define CMD_GPRS_FTPQUIT                    ((uint16_t)0x0740)
#define CMD_GPRS_FTPMODE                    ((uint16_t)0x0741)
#define CMD_GPRS_HTTPSSL                    ((uint16_t)0x0742)
#define CMD_GPRS_FTPSSL                     ((uint16_t)0x0743)
#define CMD_GPRS_CIPSSL                     ((uint16_t)0x0744)
#define CMD_GPRS_CIPGSMLOC                  ((uint16_t)0x0745)
#define CMD_IS_ACTIVE_GPRS(p)               ((p)->ActiveCmd >= 0x0700 && (p)->ActiveCmd < 0x0800)

#define CMD_OP_SCAN                         ((uint16_t)0x0800)
#define CMD_OP_COPS_SCAN                    ((uint16_t)0x0820)
#define CMD_OP_COPS_READ                    ((uint16_t)0x0821)
#define CMD_IS_ACTIVE_OP(p)                 ((p)->ActiveCmd >= 0x0800 && (p)->ActiveCmd < 0x0900)

#define __DEBUG(fmt, ...)                   printf(fmt, ##__VA_ARGS__)

#if GSM_RTOS
#define __IS_BUSY(p)                        ((p)->ActiveCmd != CMD_IDLE || (p)->Flags.F.Call_Idle != 0)
#else
#define __IS_BUSY(p)                        ((p)->ActiveCmd != CMD_IDLE || (p)->Flags.F.Call_Idle != 0)
#endif
#define __IS_READY(p)                       (!__IS_BUSY(p))
#define __CHECK_BUSY(p)                     do { if (__IS_BUSY(p)) { __RETURN(p, gsmBUSY); } } while (0)
#define __CHECK_INPUTS(c)                   do { if (!(c)) { __RETURN(GSM, gsmPARERROR); } } while (0)

#if GSM_RTOS == 1
#define __IDLE(GSM)                         do {    \
    if (GSM_SYS_Release((GSM_RTOS_SYNC_t *)&(GSM)->Sync)) { \
    }                                           \
    (GSM)->ActiveCmd = CMD_IDLE;                \
    __RESET_THREADS(GSM);                       \
    if (!(GSM)->Flags.F.IsBlocking) {           \
        (GSM)->Flags.F.Call_Idle = 1;           \
    }                                           \
    memset((void *)&Pointers, 0x00, sizeof(Pointers));  \
} while (0)
#else
#define __IDLE(GSM)                         do {    \
    (GSM)->ActiveCmd = CMD_IDLE;                \
    __RESET_THREADS(GSM);                       \
    if (!(GSM)->Flags.F.IsBlocking) {           \
        (GSM)->Flags.F.Call_Idle = 1;           \
    }                                           \
    memset((void *)&Pointers, 0x00, sizeof(Pointers));  \
} while (0)
#endif

#if GSM_RTOS
#define __ACTIVE_CMD(GSM, cmd)              do {\
    if (GSM_SYS_Request((GSM_RTOS_SYNC_t *)&(GSM)->Sync)) { \
        return gsmTIMEOUT;                      \
    }                                           \
    if ((GSM)->ActiveCmd == CMD_IDLE) {         \
        (GSM)->ActiveCmdStart = (GSM)->Time;    \
    }                                           \
    (GSM)->ActiveCmd = (cmd);                   \
} while (0)
#else
#define __ACTIVE_CMD(GSM, cmd)        do {      \
    if ((GSM)->ActiveCmd == CMD_IDLE) {         \
        (GSM)->ActiveCmdStart = (GSM)->Time;    \
    }                                           \
    (GSM)->ActiveCmd = (cmd);                   \
} while (0)
#endif

#define __CMD_SAVE(GSM)                       (GSM)->ActiveCmdSaved = (GSM)->ActiveCmd
#define __CMD_RESTORE(GSM)                    (GSM)->ActiveCmd = (GSM)->ActiveCmdSaved

#define __RETURN(GSM, val)                      do { (GSM)->RetVal = (val); return (val); } while (0)
#define __RETURN_BLOCKING(GSM, b, mt) do {      \
    GSM_Result_t res;                           \
    (GSM)->ActiveCmdTimeout = mt;               \
    if (!(b)) {                                 \
        (GSM)->Flags.F.IsBlocking = 0;          \
        __RETURN(GSM, gsmOK);                   \
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

#define __RST_EVENTS_RESP(p)                    do { (p)->Events.Value = 0; } while (0)
#define __CALL_CALLBACK(p, evt)                 (p)->Callback(evt, (GSM_EventParams_t *)&(p)->CallbackParams)
    
#define GSM_EXECUTE_SIM_READY_CHECK(GSM)  \
if ((GSM)->CPIN != GSM_CPIN_Ready) {                        /* SIM must be ready to call */     \
    __RST_EVENTS_RESP(GSM);                                 /* Reset events */                  \
    UART_SEND_STR(FROMMEM("AT+CPIN?"));                     /* Check again to be sure */        \
    UART_SEND_STR(FROMMEM(GSM_CRLF));                                                           \
    PT_WAIT_UNTIL(pt, (GSM)->Events.F.RespOk ||                                                 \
                        (GSM)->Events.F.RespError);         /* Wait for response */             \
    if ((GSM)->CPIN != GSM_CPIN_Ready) {                                                        \
        GSM->ActiveResult = gsmSIMNOTREADYERROR;            /* SIM is not ready to operate */   \
        __IDLE(GSM);                                        /* Go IDLE mode */                  \
        PT_EXIT(pt);                                        /* Stop execution */                \
    }                                                                                           \
}
#define GSM_EXECUTE_NETWORK_CHECK(GSM)  \
if ((GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredHome && \
    (GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredRoaming) {  /* Check if connected to network */ \
    __CMD_SAVE(GSM);                                                                          \
    __RST_EVENTS_RESP(GSM);                                 /* Reset events */                  \
    UART_SEND_STR(FROMMEM("AT+CREG?"));                     /* Check again to be sure */        \
    UART_SEND_STR(FROMMEM(GSM_CRLF));                                                           \
    StartCommand(GSM, CMD_GPRS_CREG, NULL);                                                     \
    PT_WAIT_UNTIL(pt, (GSM)->Events.F.RespOk ||                                                 \
                        (GSM)->Events.F.RespError);         /* Wait for response */             \
    __CMD_RESTORE(GSM);                                                                       \
    if ((GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredHome &&                             \
        (GSM)->NetworkStatus != GSM_NetworkStatus_RegisteredRoaming) {                          \
        GSM->ActiveResult = gsmNETWORKERROR;                /* Network ERROR */                 \
        __IDLE(GSM);                                        /* Go IDLE mode */                  \
        PT_EXIT(pt);                                        /* Stop execution */                \
    }                                                                                           \
}

/******************************************************************************/
/******************************************************************************/
/***                            Private variables                            **/
/******************************************************************************/
/******************************************************************************/
gstatic BUFFER_t Buffer;                                    /* Buffer structure */
uint8_t Buffer_Data[GSM_BUFFER_SIZE + 1];                   /* Buffer data array */
gstatic Received_t Received;                                /* Received data structure */
gstatic Pointers_t Pointers;                                /* Pointers object */
gstatic gvol GSM_t* GSM;                                    /* Working pointer to GSM_t structure */

gstatic
struct pt pt_GEN, pt_INFO, pt_PIN, pt_CALL, pt_SMS, pt_PB, pt_DATETIME, pt_GPRS, pt_OP;

#define __RESET_THREADS(GSM) do {                                           \
PT_INIT(&pt_GEN); PT_INIT(&pt_INFO); PT_INIT(&pt_PIN); PT_INIT(&pt_CALL);   \
PT_INIT(&pt_SMS); PT_INIT(&pt_PB); PT_INIT(&pt_DATETIME); PT_INIT(&pt_GPRS);\
PT_INIT(&pt_OP);                                                            \
} while (0)

/******************************************************************************/
/******************************************************************************/
/***                            Private functions                            **/
/******************************************************************************/
/******************************************************************************/
/* Default callback for events */
gstatic
int GSM_CallbackDefault(GSM_Event_t evt, GSM_EventParams_t* params) {
    return 0;
}

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
int32_t ParseNumber(const char* ptr, uint8_t* cnt) {
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

/* Parse float number */
static
float ParseFloatNumber(const char* ptr, uint8_t* cnt) {
    uint8_t i = 0, j = 0;
    float sum = 0.0f;

    sum = (float)ParseNumber(ptr, &i);                      /* Parse number */
    j += i;
    ptr += i;
    if (*ptr == '.') {                                      /* Check decimals */
        float dec;
        dec = (float)ParseNumber(ptr + 1, &i) / (float)pow(10, i);
        if (sum >= 0) {
            sum += dec;
        } else {
            sum -= dec;
        }
        j += i + 1;
    }

    if (cnt != NULL) {                                      /* Save number of characters used for number*/
        *cnt = j;
    }
    return sum;                                             /* Return number */
}

/* Parses date from string and stores to date structure */
gstatic
void ParseDATE(gvol GSM_t* GSM, GSM_Date_t* DateStr, const char* str) {
    uint8_t len = 0, i;
    
    DateStr->Year = 2000 + ParseNumber(&str[0], &i);
    len += i + 1;
    DateStr->Month = ParseNumber(&str[len], &i);
    len += i + 1;
    DateStr->Day = ParseNumber(&str[len], &i);
}

/* Parses time from string and stores to time structure */
gstatic
void ParseTIME(gvol GSM_t* GSM, GSM_Time_t* TimeStr, const char* str) {
    uint8_t len = 0, i;
    
    TimeStr->Hours = ParseNumber(&str[0], &i);
    len += i + 1;
    TimeStr->Minutes = ParseNumber(&str[len], &i);
    len += i + 1;
    TimeStr->Seconds = ParseNumber(&str[len], &i);
}

/* Parses +CMGR response for reading SMS data */
gstatic
void ParseCMGR(gvol GSM_t* GSM, GSM_SMS_Entry_t* sms, const char* str) {
    char *p = (char *)str, *saveptr, *token;
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
void ParseCMGL(gvol GSM_t* GSM, GSM_SMS_Entry_t* sms, const char* str) {
    uint8_t cnt;
    
    sms->Position = ParseNumber(str, &cnt);                 /* Parse number */
    str += cnt + 1;                                         /* Remove offset + comma */
    ParseCMGR(GSM, sms, str);                               /* From here, +CMGL is the same as +CMGR so use the same function as used in +CMGR response */
}

/* Parses +CLCC statement */
gstatic
void ParseCLCC(gvol GSM_t* GSM, gvol GSM_CallInfo_t* CallInfo, const char* str) {
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
void ParseCMTI(gvol GSM_t* GSM, gvol GSM_SmsInfo_t* SmsInfo, const char* str) {
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
void ParseCPIN(gvol GSM_t* GSM, GSM_CPIN_t* cpin, char* str) {
    if (strcmp(str, FROMMEM("READY\r\n")) == 0) {
        *cpin = GSM_CPIN_Ready;
    } else if (strcmp(str, FROMMEM("SIM PIN\r\n")) == 0) {
        *cpin = GSM_CPIN_SIM_PIN;
    } else if (strcmp(str, FROMMEM("SIM PUK\r\n")) == 0) {
        *cpin = GSM_CPIN_SIM_PUK;
    } else if (strcmp(str, FROMMEM("SIM PIN2\r\n")) == 0) {
        *cpin = GSM_CPIN_SIM_PIN2;
    } else if (strcmp(str, FROMMEM("SIM PUK2\r\n")) == 0) {
        *cpin = GSM_CPIN_SIM_PUK2;
    } else if (strcmp(str, FROMMEM("PH_SIM PIN\r\n")) == 0) {
        *cpin = GSM_CPIN_PH_SIM_PIN;
    } else if (strcmp(str, FROMMEM("PH_SIM PUK\r\n")) == 0) {
        *cpin = GSM_CPIN_PH_SIM_PUK;
    } else {
        *cpin = GSM_CPIN_Unknown;
    }
}

/* Parses +CFUN statement */
gstatic 
void ParseCFUN(gvol GSM_t* GSM, GSM_Func_t* func, char* str) {
    *func = (GSM_Func_t)ParseNumber(str, NULL);
}

/* Parses +CPBR statement */
gstatic
void ParseCPBR(gvol GSM_t* GSM, GSM_PB_Entry_t* entry, const char* str) {
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
void ParseIP(gvol GSM_t* GSM, uint8_t* ip, const char* str) {
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
void ParseCIPRXGET(gvol GSM_t* GSM, GSM_CONN_t* conn, const char* str) {
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
            conn->Flags.F.CallGetReceived = 1;              /* Notify user with callback */
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
void ParseHTTPACTION(gvol GSM_t* GSM, gvol GSM_HTTP_t* http, const char* str) {
    uint8_t cnt;
    
    http->Method = (GSM_HTTP_Method_t)ParseNumber(str, &cnt);   /* Parse number for method */
    str += cnt + 1;
    http->Code = ParseNumber(str, &cnt);                    /* HTTP code response */
    str += cnt + 1;
    http->BytesReceived = ParseNumber(str, NULL);           /* Parse number of bytes to read */
}

/* Parse +FTPPUT statement */
gstatic
void ParseFTPPUT(gvol GSM_t* GSM, gvol GSM_FTP_t* ftp, const char* str) {
    uint8_t cnt;
    
    ftp->Mode = ParseNumber(str, &cnt);                     /* Parse number for method */
    str += cnt + 1;
    if (ftp->Mode == 1) {                                   /* Opening FTP session */
        ftp->ErrorCode = ParseNumber(str, &cnt);            /* Get error code */
        str += cnt + 1;
        
        if (ftp->ErrorCode == 1) {                          /* When second parameter is 1 */
            ftp->MaxBytesToPut = ParseNumber(str, &cnt);    /* Parse number of bytes we can upload at a time */
            str += cnt;
        }
    } else if (ftp->Mode == 2) {                            /* Uploaidng FTP data */
        
    }
}

/* Parse +FTPGET statement */
gstatic
void ParseFTPGET(gvol GSM_t* GSM, gvol GSM_FTP_t* ftp, const char* str) {
    uint8_t cnt;
    
    ftp->Mode = ParseNumber(str, &cnt);                     /* Parse number for method */
    str += cnt + 1;
    if (ftp->Mode == 1) {                                   /* Opening FTP session */
        ftp->ErrorCode = ParseNumber(str, &cnt);            /* Get error code */
        str += cnt + 1;
        
        if (ftp->ErrorCode == 0) {
            ftp->Flags.F.DownloadActive = 0;                /* No more data available */
        } else if (ftp->ErrorCode == 1) {
            ftp->Flags.F.DataAvailable = 1;                 /* Data available to read */
        }
    } else if (ftp->Mode == 2) {                            /* Reading FTP data */
        ftp->BytesReadRemaining = ParseNumber(str, &cnt);   /* Parse number of bytes we can read in this call */
        str += cnt + 1;
        
        if (ftp->BytesReadRemaining == 0) {                 /* Check if anything to read */
            ftp->Flags.F.DataAvailable = 0;
        }
    }
}

/* Parse +CIPGSMLOC statement */
gstatic
void ParseCIPGSMLOC(gvol GSM_t* GSM, GSM_GPS_t* gps, const char* str) {
    uint8_t cnt;
    
    gps->Error = ParseNumber(str, &cnt);                    /* Check for error */
    str += cnt + 1;
    
    if (gps->Error > 0) {                                   /* Ignore others on error */
        return;
    }
    
    /* Parse GPS location */
    gps->Latitude = ParseFloatNumber(str, &cnt);            /*!< Parse latitude */
    str += cnt + 1;
    gps->Longitude = ParseFloatNumber(str, &cnt);           /*!< Parse longitude */
    str += cnt + 1;
    
    /* Parse date, it has different format in compare to others */
    gps->Date.Year = ParseNumber(str, &cnt);                /* It is YYYY format, no need to add 2000 */
    str += cnt + 1;
    gps->Date.Month = ParseNumber(str, &cnt);
    str += cnt + 1;
    gps->Date.Day = ParseNumber(str, &cnt);
    str += cnt + 1;
    
    /* Parse time */
    gps->Time.Hours = ParseNumber(str, &cnt);
    str += cnt + 1;
    gps->Time.Minutes = ParseNumber(str, &cnt);
    str += cnt + 1;
    gps->Time.Seconds = ParseNumber(str, &cnt);
    str += cnt + 1;
}

/* Parse +CBC statement */
gstatic 
void ParseCBC(gvol GSM_t* GSM, GSM_Battery_t* bat, const char* str) {
    uint8_t cnt;
    
    bat->Charging = ParseNumber(str, &cnt);                 /* Check for error */
    str += cnt + 1;
    bat->Percentage = ParseNumber(str, &cnt);               /* Read battery percentage */
    str += cnt + 1;
    bat->Voltage = ParseNumber(str, &cnt);                  /* Read battery voltage */
}

/* Parse char by char into logical structure */
static 
void ParseCOPSSCAN(gvol GSM_t* GSM, char ch, uint8_t first) {
    typedef struct U {
        union {
            struct {
                uint8_t BracketOpen : 1;                    /* Is bracket open? */
                uint8_t CommaCommaDetected : 1;             /* 2 Commas detected */
                uint8_t TermNum : 2;                        /* 2 bits for 4 different terms */
                uint8_t TermPos : 5;                        /* 5 bits for term position on number */
            } F;
            uint16_t Val;
        } Flags;
    } U_t;
    static U_t u;
    static char prev_ch = 0x00;
    GSM_OP_t* op = (GSM_OP_t *)Pointers.Ptr1;

    if (first) {                                            /* Check for first function call */                                 
        memset((void *)&u, 0x00, sizeof(U_t));
        if (Pointers.UI) {                                  /* Any valid structure */
            memset((void *)op, 0x00, sizeof(GSM_OP_t));     /* Reset structure pointer */
        }
    }

    if (u.Flags.F.CommaCommaDetected || 
        Pointers.UI == 0 ||
        *(uint16_t *)Pointers.Ptr2 >= Pointers.UI) {        /* Check valid valls */
        return;
    }

    if (u.Flags.F.BracketOpen) {                            /* When we are inside one operator */
        if (ch == ')') {
            u.Flags.F.BracketOpen = 0;
            *(uint16_t *)Pointers.Ptr2 = (*(uint16_t *)Pointers.Ptr2) + 1;  /* Increase number of read operators so far */

            /* Start new term here! */
            if (*(uint16_t *)Pointers.Ptr2 < Pointers.UI) { /* Check if empty memory available */
                Pointers.Ptr1 = ((GSM_OP_t *)Pointers.Ptr1) + 1;    /* Go to next GSM OP structure */
                memset((void *)Pointers.Ptr1, 0x00, sizeof(GSM_OP_t));  /* Reset structure */

                /* Reset numbers */
                u.Flags.F.TermNum = 0;                      /* Reset term number inside operator */
                u.Flags.F.TermPos = 0;                      /* Reset term position in term number */
            }
        } else if (ch == ',') {
            u.Flags.F.TermNum++;                            /* Go to next term */
            u.Flags.F.TermPos = 0;                          /* Reset term position */
        } else if (*(uint16_t *)Pointers.Ptr2 < Pointers.UI) {
            if (ch != '"') {
                switch (u.Flags.F.TermNum) {
                    case 0:
                        op->Index = 10 * op->Index + (ch - '0');
                        break;
                    case 1:
                        op->LongName[u.Flags.F.TermPos++] = ch;
                        break;
                    case 2:
                        op->ShortName[u.Flags.F.TermPos++] = ch;
                        break;
                    case 3:
                        op->Num[u.Flags.F.TermPos++] = ch;
                        break;
                    default:
                        break;
                }
            }
            
        }
    } else {
        if (ch == '(') {
            u.Flags.F.BracketOpen = 1;
        } else if (ch == ',') {
            if (prev_ch == ',') {
                u.Flags.F.CommaCommaDetected = 1;           /* We have detected 2 commas in series */
            }
        }
    }
    prev_ch = ch;
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
        is_ok = strcmp(str, FROMMEM("SHUT OK\r\n")) == 0;         /* Check if OK received */
    } else {
        is_ok = strcmp(str, GSM_OK) == 0;                   /* Check if OK received */
    }
    
    /* Check for error */
    if (!is_ok) {
        if (*str == '+') {
            is_error = strncmp(str, "+CME ERROR:", 11) == 0;/* Check if error received */
            if (!is_error) {
                is_error = strncmp(str, "+CMS ERROR:", 11) == 0;/* Check if error received */
            }
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
            if (strncmp(str, FROMMEM("AT+CGMI"), 7) != 0) { /* Device manufacturer */
                if (Pointers.Ptr1) {
                    strncpy((char *)Pointers.Ptr1, str, length - 2);/* Copy received string to memory */
                }
            }
        } else if (GSM->ActiveCmd == CMD_INFO_CGMM) {
            if (strncmp(str, FROMMEM("AT+CGMM"), 7) != 0) { /* Device number */
                if (Pointers.Ptr1) {
                    strncpy((char *)Pointers.Ptr1, str, length - 2);/* Copy received string to memory */
                }
            }
        } else if (GSM->ActiveCmd == CMD_INFO_CGMR) {
            if (strncmp(str, FROMMEM("AT+CGMR"), 7) != 0) { /* Device revision */
                if (strncmp(str, "Revision:", 9) == 0) {    /* Remove revision part if exists */
                    str += 9;
                    length -= 9;
                }
                if (Pointers.Ptr1) {
                    strncpy((char *)Pointers.Ptr1, str, length - 2);/* Copy received string to memory */
                }
            }
        } else if (GSM->ActiveCmd == CMD_INFO_CGSN) {
            if (strncmp(str, FROMMEM("AT+CGSN"), 7) != 0) { /* Device serial number */
                if (Pointers.Ptr1) {
                    strncpy((char *)Pointers.Ptr1, str, length - 2);/* Copy received string to memory */
                }
            }
        } else if (GSM->ActiveCmd == CMD_INFO_GMR) {
            if (strncmp(str, FROMMEM("Revision:"), 9) == 0) {
                if (Pointers.Ptr1) {                        /* If valid pointer */
                    int8_t len = length - 9 - 2;
                    strncpy((char *)Pointers.Ptr1, &str[9], len > 0 ? len : 0);
                    ((char *)Pointers.Ptr1)[len] = 0;       /* Add zero to end of string */
                }
            }
        }
    }
    
    /* On startup process */
    if (strcmp(str, FROMMEM("Call Ready\r\n")) == 0) {
        GSM->Events.F.RespCallReady = 1;
    }
    if (strcmp(str, FROMMEM("SMS Ready\r\n")) == 0) {
        GSM->Events.F.RespSMSReady = 1;
    }
    
    if (GSM->ActiveCmd == CMD_GPRS_CIPSTART && CHARISNUM(*str)) {   /* We are trying to connect as client */
        if (strcmp(&str[1], FROMMEM(", CONNECT OK\r\n")) == 0) {    /* n, CONNECT OK received */
            GSM->Events.F.RespConnectOk = 1;
        } else if (strcmp(&str[1], FROMMEM(", CONNECT FAIL\r\n")) == 0) {   /* n, CONNECT FAIL received */
            GSM->Events.F.RespConnectFail = 1;
        } else if (strcmp(&str[1], FROMMEM(", ALREADY CONNECT\r\n")) == 0) {    /* n, ALREADY CONNECT received */
            GSM->Events.F.RespConnectAlready = 1;
        }
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPCLOSE) {
        if (strcmp(&str[1], FROMMEM(", CLOSE OK\r\n")) == 0) {  /* n, CLOSE OK received */
            GSM->Events.F.RespCloseOk = 1;                  /* Closed OK */
            is_ok = 1;                                      /* Response is OK */
        }
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPSEND) {
        if (strcmp(&str[1], FROMMEM(", SEND OK\r\n")) == 0) {/* n, SEND OK received */
            GSM->Events.F.RespSendOk = 1;
            is_ok = 1;
        } else if (strcmp(&str[1], FROMMEM(", SEND FAIL\r\n")) == 0) {  /* n, SEND FAIL received */
            GSM->Events.F.RespSendFail = 1;
            is_error = 1;
        }
    }
    
    /* Connection closed by remote device */
    if (strcmp(&str[1], FROMMEM(", CLOSED\r\n")) == 0) {    /* n, CLOSED received */
        uint8_t num = CHARTONUM(str[0]);                    /* Get connection number */
        if (GSM->Conns[num]) {
            GSM->Conns[num]->Flags.F.Active = 0;            /* Connection is not active anymore */
            GSM->Conns[num]->Flags.F.CallConnClosed = 1;    /* Call connection closed */
        }
    }
    
    /* Some special cases */
    if (strcmp(str, GSM_RING) == 0) {
        GSM->Flags.F.CALL_RING_Received = 1;                /* Set flag for callbacks */
    } else if (strcmp(str, GSM_BUSY) == 0) {
        
    } else if (strcmp(str, GSM_NO_CARRIER) == 0) {
        
    }
    
    if (*str == '+' && !is_error) {                         /* For statements starting with '+' sign */     
        if (GSM->ActiveCmd == CMD_GPRS_CREG && strncmp(str, FROMMEM("+CREG:"), 6) == 0) {
            ParseCREG(GSM, &str[7]);                        /* Parse CREG response */
        } else if (CMD_IS_ACTIVE_GPRS(GSM)) {
        
        } else if (CMD_IS_ACTIVE_SMS(GSM)) {                /* Currently active command is regarding SMS */
            if (GSM->ActiveCmd == CMD_SMS_SEND && strncmp(str, FROMMEM("+CMGS:"), 6) == 0) {  /* We just sent SMS and number in memory is returned */
                GSM->SMS.SentSMSMemNum = ParseNumber(&str[7], NULL);/* Parse number and save it */
            } else if (GSM->ActiveCmd == CMD_SMS_READ && strncmp(str, FROMMEM("+CMGR:"), 6) == 0) { /* When SMS Read instruction is executed */
                ParseCMGR(GSM, (GSM_SMS_Entry_t *)Pointers.Ptr1, str + 7);  /* Parse received command for read SMS */
                GSM->Flags.F.SMS_Read_Data = 1;             /* Next step is to read actual SMS data */
                ((GSM_SMS_Entry_t *)Pointers.Ptr1)->DataLen = 0; /* Reset data length */
            } else if (GSM->ActiveCmd == CMD_SMS_LIST && strncmp(str, FROMMEM("+CMGL:"), 6) == 0) {  /* When list command is executed */
                if (*(uint16_t *)Pointers.Ptr2 < Pointers.UI) { /* Do we still have empty memory to read data? */
                    ParseCMGL(GSM, (GSM_SMS_Entry_t *)Pointers.Ptr1, str + 7);
                    ((GSM_SMS_Entry_t *)Pointers.Ptr1)->DataLen = 0; /* Reset data length */
                }
                GSM->Flags.F.SMS_Read_Data = 1;             /* Next step is to read actual SMS data 
                                                               Activate this command, even if data won't be saved. 
                                                               Data should be flushed from received buffer and ignored! */
            }
        } else if (CMD_IS_ACTIVE_PB(GSM)) {                 /* Currently active command is regarding PHONEBOOK */
            if (strncmp(str, FROMMEM("+CPBR:"), 6) == 0) {
                ParseCPBR(GSM, (GSM_PB_Entry_t *)Pointers.Ptr1, str + 7);   /* Parse +CPBR statement */
                if (GSM->ActiveCmd == CMD_PB_LIST) {        /* Check for GETALL or SEARCH commands */
                    Pointers.Ptr1 = ((GSM_PB_Entry_t *)Pointers.Ptr1) + 1;  /* Set new pointer offset in data array */
                    *(uint16_t *)Pointers.Ptr2 = (*(uint16_t *)Pointers.Ptr2) + 1;  /* Increase number of received entries */
                }
            } else if (GSM->ActiveCmd == CMD_PB_SEARCH && strncmp(str, FROMMEM("+CPBF:"), 6) == 0) {
                if (*(uint16_t *)Pointers.Ptr2 < Pointers.UI) { /* Check for GETALL or SEARCH commands */
                    ParseCPBR(GSM, (GSM_PB_Entry_t *)Pointers.Ptr1, str + 7);   /* Parse +CPBR statement */
                    Pointers.Ptr1 = ((GSM_PB_Entry_t *)Pointers.Ptr1) + 1;  /* Set new pointer offset in data array */
                    *(uint16_t *)Pointers.Ptr2 = (*(uint16_t *)Pointers.Ptr2) + 1;  /* Increase number of received entries */
                }
            }
        }
        
        /* Below statements can be received even if no command active */
        if (strncmp(str, FROMMEM("+CPIN:"), 6) == 0) {      /* For SIM status response */
            ParseCPIN(GSM, (GSM_CPIN_t *)&GSM->CPIN, str + 7);  /* Parse +CPIN statement */
        } else if (strncmp(str, FROMMEM("+CFUN"), 5) == 0) {/* Phone functionality */
            ParseCFUN(GSM, (GSM_Func_t *)&GSM->Func, str + 7);  /* Parse CFUN response */
            if (Pointers.Ptr1) {                            /* When command executed */
                *(GSM_Func_t *)Pointers.Ptr1 = GSM->Func;   /* Copy value */
            }
        } else if (strncmp(str, FROMMEM("+CLCC:"), 6) == 0) {   /* Call informations changes */
            ParseCLCC(GSM, &GSM->CallInfo, str + 7);        /* Parse +CLCC statement */
            GSM->Flags.F.CALL_CLCC_Received = 1;            /* Set flag for callback */
        } else if (strncmp(str, FROMMEM("+CMTI:"), 6) == 0) {
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
        } else if (strncmp(str, FROMMEM("+CIPRXGET:"), 10) == 0) {/* +CIPRXGET statement */
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
        } else if (strncmp(str, FROMMEM("+HTTPACTION:"), 12) == 0) {  /* Check HTTPACTION */
            ParseHTTPACTION(GSM, &GSM->HTTP, &str[13]);     /* Parse +HTTPACTION response */
            GSM->Events.F.RespHttpAction = 1;               /* HTTP ACTION */
        }  else if (strncmp(str, FROMMEM("+HTTPREAD:"), 10) == 0) {   /* Check HTTPACTION */
            GSM->HTTP.BytesRead = 0;                        /* Reset read bytes */
            GSM->HTTP.BytesReadRemaining = ParseNumber(&str[11], NULL); /* Get number of bytes we have to read in this request */
            GSM->Flags.F.HTTP_Read_Data = 1;                /* HTTP read data */
        } else if (strncmp(str, FROMMEM("+FTPGET:"), 8) == 0) { /* Parse FTPGET */
            ParseFTPGET(GSM, (GSM_FTP_t *)&GSM->FTP, &str[9]);  /* Parse FTP GET statement */
            GSM->Events.F.RespFtpGet = 1;                   /* FTP GET was received */
            
            if (GSM->FTP.Mode == 2) {                       /* Read procedure */
                GSM->FTP.BytesRead = 0;                     /* Reset number of read bytes */
                if (GSM->FTP.BytesReadRemaining > 0) {      /* Check if we should read anything */
                    GSM->Flags.F.FTP_Read_Data = 1;         /* Activate flag to read data */
                }
            }
        } else if (strncmp(str, FROMMEM("+FTPPUT:"), 8) == 0) { /* Parse FTPPUT */
            ParseFTPPUT(GSM, (GSM_FTP_t *)&GSM->FTP, &str[9]);  /* Parse FTPPUT statement */
            GSM->Events.F.RespFtpPut = 1;                   /* FTP PUT was received */
            if (GSM->FTP.Mode == 2) {                       /* +FTPPUT:2,.. received */
                GSM->Events.F.RespFtpUploadReady = 1;       /* Upload is ready to proceed */
            }
        } else if (GSM->ActiveCmd == CMD_GPRS_CIPGSMLOC && strncmp(str, FROMMEM("+CIPGSMLOC"), 10) == 0) {
            if (Pointers.Ptr1) {                            /* Check valid pointer */
                ParseCIPGSMLOC(GSM, (GSM_GPS_t *)Pointers.Ptr1, str + 12);  /* Parse GPS location and time */
            }
        } else if (GSM->ActiveCmd == CMD_INFO_CBC && strncmp(str, FROMMEM("+CBC"), 4) == 0) {
            if (Pointers.Ptr1) {                            /* Check valid pointer */
                ParseCBC(GSM, (GSM_Battery_t *)Pointers.Ptr1, str + 6); /* Parse battery status */
            }
        }
    }
    
    /* Check under-voltage warning, don't know why SIMCOM uses 2 "N" characters in "WARNING" */
    if ((length == 24 && strcmp(str, FROMMEM("UNDER-VOLTAGE WARNNING\r\n")) == 0) ||
        (length == 23 && strcmp(str, FROMMEM("UNDER-VOLTAGE WARNING\r\n")) == 0)) { /* Maybe they will fix some day */
        GSM->Flags.F.Call_UV_Warn = 1;                      /* Set flag for callback */
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
    char str[6];
    PT_BEGIN(pt);                                           /* Begin thread */
    
    if (GSM->ActiveCmd == CMD_GEN_AT) {                     /* Check AT */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT"));                       /* Send test */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GEN_FACTORY_SETTINGS, NULL);  /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);
    } else if (GSM->ActiveCmd == CMD_GEN_FACTORY_SETTINGS) {/* Check active command status */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT&F"));                     /* Factory reset */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GEN_FACTORY_SETTINGS, NULL);  /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);
    } else if (GSM->ActiveCmd == CMD_GEN_SMSNOTIFY) {       /* Check active command status */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CNMI=1,2,0,0,0"));        /* Automatically send notification about new received SMS and SMS itself */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GEN_SMSNOTIFY, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);
    } else if (GSM->ActiveCmd == CMD_GEN_ERROR_NUMERIC) {   /* Enable numeric error reporting */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CMEE=1"));                /* Error reporting */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GEN_ERROR_NUMERIC, NULL);     /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);
    } else if (GSM->ActiveCmd == CMD_GEN_CALL_CLCC) {       /* Enable auto notifications for call */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CLCC=1"));                /* Auto notify about new calls */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GEN_CALL_CLCC, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);
    } else if (GSM->ActiveCmd == CMD_GEN_ATE0) {            /* Disable command echo */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("ATE0"));                     /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GEN_ATE0, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);
    } else if (GSM->ActiveCmd == CMD_GEN_ATE1) {            /* Enable command echo */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("ATE1"));                     /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GEN_ATE1, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);
    } else if (GSM->ActiveCmd == CMD_GEN_CFUN_SET) {        /* Set phone functionality */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Go to string */
        UART_SEND_STR(FROMMEM("AT+CFUN="));                 /* Send command */
        UART_SEND_STR(FROMMEM(str));
        UART_SEND_STR(FROMMEM(GSM_CRLF));
        StartCommand(GSM, CMD_GEN_CFUN, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        if (GSM->ActiveResult == gsmOK) {                   /* Check for success */
            GSM->Func = (GSM_Func_t)Pointers.UI;            /* Update new value */
        }

        __IDLE(GSM);                                        /* Go IDLE state */
    } else if (GSM->ActiveCmd == CMD_GEN_CFUN_GET) {        /* Get phone functionality */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Go to string */
        UART_SEND_STR(FROMMEM("AT+CFUN?"));                 /* Send command */
        UART_SEND_STR(FROMMEM(GSM_CRLF));
        StartCommand(GSM, CMD_GEN_CFUN, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */

        __IDLE(GSM);                                        /* Go IDLE state */
    }
    PT_END(pt);                                             /* End thread */
}

gstatic 
PT_THREAD(PT_Thread_INFO(struct pt* pt, gvol GSM_t* GSM)) {
    PT_BEGIN(pt);
    
    if (GSM->ActiveCmd == CMD_INFO_CGMI) {                  /* Check device manufacturer */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CGMI"));                  /* Send data to device */
        UART_SEND_STR(FROMMEM(GSM_CRLF));
        StartCommand(GSM, CMD_INFO_CGMI, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE state */
    } else if (GSM->ActiveCmd == CMD_INFO_CGMM) {           /* Check device model */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CGMM"));                  /* Send data to device */
        UART_SEND_STR(FROMMEM(GSM_CRLF));
        StartCommand(GSM, CMD_INFO_CGMM, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */

        __IDLE(GSM);                                        /* Go IDLE state */
    } else if (GSM->ActiveCmd == CMD_INFO_CGMR) {           /* Check device revision */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CGMR"));                  /* Send data to device */
        UART_SEND_STR(FROMMEM(GSM_CRLF));
        StartCommand(GSM, CMD_INFO_CGMR, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE state */
    } else if (GSM->ActiveCmd == CMD_INFO_CGSN) {           /* Check device serial number */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CGSN"));                  /* Send data to device */
        UART_SEND_STR(FROMMEM(GSM_CRLF));
        StartCommand(GSM, CMD_INFO_CGSN, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE state */
    } else if (GSM->ActiveCmd == CMD_INFO_GMR) {            /* Check software revision number */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+GMR"));                   /* Send data to device */
        UART_SEND_STR(FROMMEM(GSM_CRLF));
        StartCommand(GSM, CMD_INFO_GMR, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE state */
    } else if (GSM->ActiveCmd == CMD_INFO_CBC) {            /* Battery informations */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CBC"));                   /* Send data to device */
        UART_SEND_STR(FROMMEM(GSM_CRLF));
        StartCommand(GSM, CMD_INFO_CBC, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE state */
    }
    PT_END(pt);                                             /* End thread */
}

gstatic
PT_THREAD(PT_Thread_PIN(struct pt* pt, gvol GSM_t* GSM)) {
    static gvol uint32_t start;
    PT_BEGIN(pt);                                           /* Begin thread */

    __RST_EVENTS_RESP(GSM);                                 /* Reset events */
    UART_SEND_STR(FROMMEM("AT+CPIN?"));                     /* Check SIM status first */
    UART_SEND_STR(FROMMEM(GSM_CRLF));
    
    PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                        GSM->Events.F.RespError);           /* Wait for response */
    
    GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR;
    if (GSM->Events.F.RespError) {                          /* Check error */
        __IDLE(GSM);                                        /* Go IDLE state */
        PT_EXIT(pt);                                        /* Exit with thread */
    }
    
    if (GSM->ActiveCmd == CMD_PIN) {
        if (GSM->CPIN != GSM_CPIN_SIM_PIN && GSM->CPIN != GSM_CPIN_SIM_PIN2) {  /* Check if in valid state for PIN insert */
            __IDLE(GSM);                                    /* Process IDLE */
            PT_EXIT(pt);                                    /* Stop thread */
        }
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CPIN="));                 /* Send data to device */
        UART_SEND_STR((char *)Pointers.CPtr1);              /* PIN pointer is in this variable */
        UART_SEND_STR(FROMMEM(GSM_CRLF));
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
        if (GSM->ActiveResult == gsmOK) {
            start = GSM->Time;
            PT_WAIT_UNTIL(pt, (GSM->Events.F.RespCallReady && GSM->Events.F.RespSMSReady) ||
                               GSM->Time - start > 5000);   /* Wait call and SMS ready */
        }
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PUK) {
        if (GSM->CPIN != GSM_CPIN_SIM_PUK && GSM->CPIN != GSM_CPIN_SIM_PUK2) {  /* Check if in valid state for PIN insert */
            __IDLE(GSM);                                    /* Process IDLE */
            PT_EXIT(pt);                                    /* Stop thread */
        }
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CPIN="));                 /* Send data to device */
        UART_SEND_STR((char *)Pointers.CPtr1);              /* PUK pointer is in this variable */
        UART_SEND_STR(FROMMEM(","));
        UART_SEND_STR((char *)Pointers.CPtr2);              /* New pin pointer */
        UART_SEND_STR(FROMMEM(GSM_CRLF));
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
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PIN_REMOVE) {          /* Remove PIN */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CLCK=\"SC\",0,\""));      /* Send data to device */
        UART_SEND_STR(FROMMEM(Pointers.CPtr1));             /* Send actual PIN */
        UART_SEND_STR(FROMMEM("\""));                             
        UART_SEND_STR(FROMMEM(GSM_CRLF));
        StartCommand(GSM, CMD_PIN_REMOVE, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PIN_ADD) {             /* Add pin to SIM card without pin */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CLCK=\"SC\",1,\""));      /* Send data to device */
        UART_SEND_STR(FROMMEM(Pointers.CPtr1));             /* Send actual PIN */
        UART_SEND_STR(FROMMEM("\""));                             
        UART_SEND_STR(FROMMEM(GSM_CRLF));
        StartCommand(GSM, CMD_PIN_ADD, NULL);               /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    }
    PT_END(pt);                                             /* Stop thread */
}

gstatic
PT_THREAD(PT_Thread_CALL(struct pt* pt, gvol GSM_t* GSM)) {
    char str[6];
    
    PT_BEGIN(pt);                                           /* Begin thread */
    GSM_EXECUTE_SIM_READY_CHECK(GSM);                       /* SIM must be ready to operate in this case! */
    
    if (GSM->ActiveCmd == CMD_CALL_VOICE) {                 /* Create voice call */        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("ATD"));                      /* Send command */
        UART_SEND_STR(Pointers.CPtr1);                      /* Send number formatted as string */
        UART_SEND_STR(FROMMEM(";"));                        /* Semicolon is required for voice call */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_VOICE, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_CALL_VOICE_SIM_POS) {  /* Create voice call from SIM  */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert position to string */
        UART_SEND_STR(FROMMEM("ATD>\"SM\" "));              /* Send command */
        UART_SEND_STR(str);                                 /* Send number formatted as string */
        UART_SEND_STR(FROMMEM(";"));                        /* Process voice call */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_VOICE_SIM_POS, NULL);    /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */        
        __IDLE(GSM);                                        /* Go IDLE mode */ 
    } else if (GSM->ActiveCmd == CMD_CALL_DATA) {           /* Create data call */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("ATD"));                      /* Send command */
        UART_SEND_STR(Pointers.CPtr1);                      /* Send number formatted as string */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_DATA, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */ 
    } else if (GSM->ActiveCmd == CMD_CALL_DATA_SIM_POS) {   /* Create data call from SIM  */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert position to string */
        UART_SEND_STR(FROMMEM("ATD>\"SM\" "));              /* Send command */
        UART_SEND_STR(str);                                 /* Send number formatted as string */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_DATA_SIM_POS, NULL);     /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */ 
    } else if (GSM->ActiveCmd == CMD_CALL_ANSWER) {         /* If we want to answer call */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("ATA"));                      /* Send command to answer incoming call */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_ANSWER, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_CALL_HANGUP) {         /* If we want to answer call */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("ATH"));                      /* Send command to hang up */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_CALL_HANGUP, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
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
    __CMD_SAVE(GSM);                                        /* Save current command */
    __RST_EVENTS_RESP(GSM);                                 /* Reset events */
    UART_SEND_STR(FROMMEM("AT+CMGF=1"));                    /* Go to SMS text mode */
    UART_SEND_STR(GSM_CRLF);
    StartCommand(GSM, CMD_SMS_CMGF, NULL);                  /* Start command */
    
    PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                        GSM->Events.F.RespError);           /* Wait for response */
    __CMD_RESTORE(GSM);                                     /* Restore command */
    
    if (GSM->Events.F.RespError) {
        GSM->Flags.F.SMS_SendError = 1;                     /* SMS error flag */
        GSM->ActiveResult = gsmENTERTEXTMODEERROR;          /* Can not go to text mode */
        __IDLE(GSM);                                        /* Go IDLE mode */
        PT_EXIT(pt);                                        /* Exit thread */
    }
    
    if (GSM->ActiveCmd == CMD_SMS_SEND) {                   /* Process send SMS */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CMGS=\""));               /* Send number to GSM */
        UART_SEND_STR(GSM->SMS.Number);                     /* Send actual number formatted as string */
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_SMS_SEND, FROMMEM("+CMGS"));  /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespBracket ||
                            GSM->Events.F.RespError);       /* Wait for > character and timeout */
        
        if (GSM->Events.F.RespBracket) {                    /* We received bracket */
            __RST_EVENTS_RESP(GSM);                         /* Reset events */
            UART_SEND_STR(GSM->SMS.Data);                   /* Send SMS data */
            UART_SEND_CH(&terminate);                       /* Send terminate SMS character */
            
            PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk ||
                                GSM->Events.F.RespError);   /* Wait for OK or ERROR */
            
            if (GSM->Events.F.RespOk) {                     /* Here, we should have SMS memoy number for sent SMS */
                GSM->Flags.F.SMS_SendOk = 1;                /* SMS sent OK */
                GSM->ActiveResult = gsmOK;
            } else if (GSM->Events.F.RespError) {           /* Error sending */
                GSM->Flags.F.SMS_SendError = 1;             /* SMS was not send */
                GSM->ActiveResult = gsmERROR;
            }                                               /* Go IDLE mode */
        } else if (GSM->Events.F.RespError) {
            GSM->Flags.F.SMS_SendError = 1;                 /* SMS error flag */
            GSM->ActiveResult = gsmERROR;                   /* Process error */
        }
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_SMS_READ) {            /* Process read SMS */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        
        NumberToString(str, ReadSMSPtr->Position);          /* Convert number to string */
        UART_SEND_STR(FROMMEM("AT+CMGR="));                 /* Send command */
        UART_SEND_STR(str);
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_SMS_READ, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_SMS_DELETE) {          /* Process delete SMS */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        UART_SEND_STR(FROMMEM("AT+CMGD="));                 /* Send command */
        UART_SEND_STR(str);
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_SMS_CMGD, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_SMS_MASSDELETE) {      /* Process mass delete SMS */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        UART_SEND_STR(FROMMEM("AT+CMGDA=\"DEL "));          /* Send command */
        UART_SEND_STR((char *)Pointers.CPtr1);
        UART_SEND_STR(FROMMEM("\""));                       /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_SMS_CMGD, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */  
    } else if (GSM->ActiveCmd == CMD_SMS_LIST) {            /* Process get all phonebook entries */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CMGL=\""));               /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr1));             /* Send type of SMS messages to list */
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_SMS_LIST, NULL);              /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    }
    PT_END(pt);                                             /* End thread */
}

gstatic
PT_THREAD(PT_Thread_PB(struct pt* pt, gvol GSM_t* GSM)) {
    char str[6];
    
    PT_BEGIN(pt);                                           /* Begin thread */
    GSM_EXECUTE_SIM_READY_CHECK(GSM);                       /* SIM must be ready to operate in this case! */
    
    if (GSM->ActiveCmd == CMD_PB_ADD) {                     /* Process add phonebook entry */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CPBW="));                 /* Send command */
        UART_SEND_STR(FROMMEM(",\""));
        UART_SEND_STR((char *)Pointers.CPtr1);              /* Send number */
        UART_SEND_STR(FROMMEM("\",129,\""));                      /* Send national number format */
        UART_SEND_STR((char *)Pointers.CPtr2);              /* Send name */
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_ADD, NULL);                /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PB_EDIT) {             /* Process edit phonebook entry */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        UART_SEND_STR(FROMMEM("AT+CPBW="));                 /* Send command */
        UART_SEND_STR(str);                                 /* Send index */
        UART_SEND_STR(FROMMEM(",\""));
        UART_SEND_STR((char *)Pointers.CPtr1);              /* Send number */
        UART_SEND_STR(FROMMEM("\",129,\""));                      /* Send national number format */
        UART_SEND_STR((char *)Pointers.CPtr2);              /* Send name */
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_EDIT, NULL);               /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PB_GET) {              /* Process read phonebook entry */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        UART_SEND_STR(FROMMEM("AT+CPBR="));                 /* Send command */
        UART_SEND_STR(str);                                 /* Send index */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_GET, NULL);                /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PB_DELETE) {           /* Process delete phonebook entry */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        UART_SEND_STR(FROMMEM("AT+CPBW="));                 /* Send command */
        UART_SEND_STR(str);                                 /* Send index */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_DELETE, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PB_LIST) {             /* Process get all phonebook entries */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CPBR="));                 /* Send command */
        NumberToString(str, (Pointers.UI >> 16) & 0xFFFF);  /* Convert number to string for start index */
        UART_SEND_STR(str);                                 /* Send start index */
        UART_SEND_STR(FROMMEM(","));
        NumberToString(str, (Pointers.UI) & 0xFFFF);        /* Convert number to string for number of elements */
        UART_SEND_STR(str);                                 /* Send start index */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_LIST, NULL);               /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_PB_SEARCH) {           /* Process search phonebook entries */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CPBF="));                 /* Send command */
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(Pointers.CPtr1);                      /* Send start index */
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_PB_SEARCH, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    }
    
    PT_END(pt);                                             /* End thread */
}

gstatic
PT_THREAD(PT_Thread_DATETIME(struct pt* pt, gvol GSM_t* GSM)) {    
    PT_BEGIN(pt);                                           /* Begin thread */
    
//    __RST_EVENTS_RESP(GSM);                                 /* Reset events */
//    UART_SEND_STR(FROMMEM("AT+CLTS=1"));                    /* Send command */
//    UART_SEND_STR(GSM_CRLF);
//    StartCommand(GSM, CMD_DATETIME_GET, NULL);              /* Start command */
//    
//    PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
//                        GSM->Events.F.RespError);           /* Wait for response */
    
    if (GSM->ActiveCmd == CMD_DATETIME_GET) {               /* Process add phonebook entry */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CCLK?"));                 /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_DATETIME_GET, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        __IDLE(GSM);                                        /* Go IDLE mode */
    }
    
    PT_END(pt);                                             /* End thread */
}

gstatic
PT_THREAD(PT_Thread_GPRS(struct pt* pt, gvol GSM_t* GSM)) {
    char str[7], ch;
    static uint32_t start, btw;
    static uint8_t tries;
    GSM_CONN_t* conn = (GSM_CONN_t *)Pointers.Ptr1;
    uint8_t terminate = 26;
    
    PT_BEGIN(pt);                                           /* Begin thread */
    
    __CMD_SAVE(GSM);                                        /* Save command */
    GSM_EXECUTE_NETWORK_CHECK(GSM);                         /* Check for network state */
    __CMD_RESTORE(GSM);                                     /* Restore command */

    if (GSM->ActiveCmd == CMD_GPRS_SETAPN) {                /* Process APN settings */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CSTT=\""));               /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr1));
        UART_SEND_STR(FROMMEM("\",\""));
        if (Pointers.CPtr2) {
            UART_SEND_STR(FROMMEM(Pointers.CPtr2));
        }
        UART_SEND_STR(FROMMEM("\",\""));
        if (Pointers.CPtr3) {
            UART_SEND_STR(FROMMEM(Pointers.CPtr3));
        }
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_SETAPN, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_ATTACH) {         /* Network attach */
        __CMD_SAVE(GSM);                                    /* Save command */
    
        /**** Detach from network first if not already ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CGACT=0"));               /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CGACT, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response and don't care what it actually is */
        
        /**** Attach to network ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CGACT=1"));               /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CGACT, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }
        
        /**** Detach from network ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CGATT=0"));               /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CGATT, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response and don't care what it actually is */
        
        /**** Attach to network ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CGATT=1"));               /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CGATT, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }
        
        /**** Shutdown CIP connections ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CIPSHUT"));               /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPSHUT, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }
        
        /**** Set multiple connections ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CIPMUX=1"));              /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPMUX, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }
        
        /**** Receive data manually ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CIPRXGET=1"));            /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPRXGET, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        } 
        
        /**** Set APN data ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CSTT=\""));               /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr1));
        UART_SEND_STR(FROMMEM("\",\""));
        if (Pointers.CPtr2) {
            UART_SEND_STR(FROMMEM(Pointers.CPtr2));         /* Send username */
        }
        UART_SEND_STR(FROMMEM("\",\""));
        if (Pointers.CPtr3) {
            UART_SEND_STR(FROMMEM(Pointers.CPtr3));         /* Send password */
        }
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CSTT, NULL);             /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }

        /**** Start wireless connection ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CIICR"));                 /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIICR, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }

        /**** Get IP address ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CIFSR"));                 /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIFSR, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
   
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {
            goto cmd_gprs_attach_clean;                     /* Clean thread and stop execution */
        }
        
        /**** Terminate HTTP ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+HTTPTERM"));              /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPTERM, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response, ignore it */
        
        /**** SAPBR start for HTTP ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+SAPBR=1,1"));             /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_SAPBR, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response, ignore it */
        
cmd_gprs_attach_clean:
        if (GSM->ActiveResult == gsmOK) {                   /* Set callback flags */
            GSM->Flags.F.Call_GPRS_Attached = 1;
        } else {
            GSM->Flags.F.Call_GPRS_Attach_Error = 1;
        }
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_DETACH) {         /* Detach from network */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        /**** Stop SAPBR ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+SAPBR=0,1"));             /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_SAPBR, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response, ignore it */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CGATT=0"));               /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CGATT, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        GSM->Flags.F.Call_GPRS_Detached = 1;                /* Set detached flag */
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPSTART) {       /* Start new connection as client */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        /**** CONN over SSL ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CIPSSL="));               /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr3));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPSSL, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        /**** CIP start ****/
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CIPSTART=0,\""));         /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr2));             /* TCP/UDP */
        UART_SEND_STR(FROMMEM("\",\""));
        UART_SEND_STR(FROMMEM(Pointers.CPtr1));             /* Domain/IP */
        UART_SEND_STR(FROMMEM("\","));
        UART_SEND_STR(FROMMEM(str));                        /* Port number */
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
            
            if (GSM->Events.F.RespConnectFail) {            /* Check if connection was successful */
                GSM->ActiveResult = gsmERROR;
            }
        }
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPCLOSE) {       /* Close client connection */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        NumberToString(str, Pointers.UI);                   /* Convert number to string */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CIPCLOSE=0"));            /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPCLOSE, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespError || 
                            GSM->Events.F.RespCloseOk);     /* Wait for response */
        
        GSM->Conns[0]->Flags.F.Active = 0;                  /* Connection is not active anymore */
        
        GSM->ActiveResult = GSM->Events.F.RespCloseOk ? gsmOK : gsmERROR; /* Set result to return */
        
        __CMD_RESTORE(GSM);                               /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPSEND) {
        __CMD_SAVE(GSM);                                    /* Save command */
        if (Pointers.Ptr2) {
            *(uint32_t *)Pointers.Ptr2 = 0;                 /* Set sent bytes to zero first */
        }
        
        tries = 3;                                          /* Give 3 tries to send each packet */
        do {            
            btw = Pointers.UI > 1460 ? 1460 : Pointers.UI;  /* Set length to send */
            
            NumberToString(str, btw);                       /* Get string from number */
            __RST_EVENTS_RESP(GSM);                         /* Reset events */
            UART_SEND_STR(FROMMEM("AT+CIPSEND=0,"));        /* Send number to GSM */
            UART_SEND_STR(str);
            UART_SEND_STR(GSM_CRLF);
            StartCommand(GSM, CMD_GPRS_CIPSEND, NULL);      /* Start command */
            
            PT_WAIT_UNTIL(pt, GSM->Events.F.RespBracket ||
                                GSM->Events.F.RespError);   /* Wait for > character and timeout */
            
            if (GSM->Events.F.RespBracket) {                /* We received bracket */
                __RST_EVENTS_RESP(GSM);                     /* Reset events */
                UART_SEND((uint8_t *)Pointers.CPtr1, btw);  /* Send data */
                UART_SEND_CH(&terminate);
                
                PT_WAIT_UNTIL(pt, GSM->Events.F.RespSendOk ||
                                    GSM->Events.F.RespSendFail ||
                                    GSM->Events.F.RespError);   /* Wait for OK or ERROR */
                
                GSM->ActiveResult = GSM->Events.F.RespSendOk ? gsmOK : gsmSENDFAIL; /* Set result to return */
                
                if (GSM->ActiveResult == gsmOK) {
                    if (Pointers.Ptr2 != NULL) {
                        *(uint32_t *)Pointers.Ptr2 = *(uint32_t *)Pointers.Ptr2 + btw;  /* Increase number of sent bytes */
                    }
                }
            } else if (GSM->Events.F.RespError) {
                GSM->ActiveResult = gsmERROR;               /* Process error */
            }
            if (GSM->ActiveResult == gsmOK) {
                tries = 3;                                  /* Reset number of tries */
                
                Pointers.UI -= btw;                         /* Decrease number of sent bytes */
                Pointers.CPtr1 = (uint8_t *)Pointers.CPtr1 + btw;   /* Set new data memory location to send */
            } else {
                tries--;                                    /* We failed, decrease number of tries and start over */
            }
        } while (Pointers.UI && tries);                     /* Until anything to send */
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPRXGET) {       /* Read data from device */
        __CMD_SAVE(GSM);                                    /* Save command */

        start = GSM->Time;                                  /* Start counting */
        PT_WAIT_UNTIL(pt, GSM->Time - start >= conn->ReadTimeout);  /* Wait start timeout */
        
        if (conn->BytesToRead > 1460) {                     /* Check max read size */
            conn->BytesToRead = 1460;
        }
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CIPRXGET=2,"));           /* Send command */
        NumberToString(str, conn->ID);                      /* Convert number to string for connection ID */
        UART_SEND_STR(FROMMEM(str));
        UART_SEND_STR(FROMMEM(","));
        NumberToString(str, conn->BytesToRead);             /* Convert number to string */
        UART_SEND_STR(FROMMEM(str));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPRXGET, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmOK) {
            conn->BytesReadTotal += conn->BytesRead;        /* Increase number of total read bytes */
            if (Pointers.Ptr2 != NULL) {
                *(uint32_t *)Pointers.Ptr2 = conn->BytesRead;   /* Save number of read bytes in last request */
            }
        }        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPBEGIN) {
        __CMD_SAVE(GSM);                                    /* Save command */
        
        /**** HTTP INIT ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+HTTPINIT"));              /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPINIT, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_httpbegin_clean;                  
        }

        /**** HTTP PARAMETER CID ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+HTTPPARA=\"CID\",1"));    /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPINIT, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
cmd_gprs_httpbegin_clean:
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPSEND) {       /* Send data to HTTP buffer */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        /**** HTTP DATA ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+HTTPDATA="));             /* Send command */
        NumberToString(str, GSM->HTTP.DataLength);          /* Send data length */
        UART_SEND_STR(FROMMEM(str));
        UART_SEND_STR(FROMMEM(",10000"));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPDATA, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespDownload ||
                            GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        if (!GSM->Events.F.RespError && GSM->Events.F.RespDownload) {
            UART_SEND(GSM->HTTP.Data, GSM->HTTP.DataLength);/* Send actual data! */
        } else {
            goto cmd_gprs_httpsend_clean;
        }
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_httpsend_clean;
        }
        
cmd_gprs_httpsend_clean:                                    /* Clean everything */
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE */ 
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPEXECUTE) {    /* Execute command */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        /**** HTTP PARA URL ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+HTTPPARA=\"URL\",\""));   /* Send command */
        UART_SEND_STR(FROMMEM(GSM->HTTP.TMP));
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPPARA, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_httpexecute_clean;                  
        }
        
        /**** HTTP SSL ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+HTTPSSL="));              /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr1));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPSSL, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */

        /**** HTTP METHOD ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+HTTPACTION="));           /* Send command */
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
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE */
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPREAD) {       /* Read data HTTP response */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+HTTPREAD="));             /* Send command */
        NumberToString(str, GSM->HTTP.BytesReadTotal);      /* Convert number to string for connection ID */
        UART_SEND_STR(FROMMEM(str));
        UART_SEND_STR(FROMMEM(","));
        NumberToString(str, GSM->HTTP.BytesToRead);         /* Convert number to string */
        UART_SEND_STR(FROMMEM(str));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPREAD, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmOK) {
            GSM->HTTP.BytesReadTotal += GSM->HTTP.BytesRead;/* Increase number of total read bytes */
            if (Pointers.Ptr1 != NULL) {
                *(uint32_t *)Pointers.Ptr1 = GSM->HTTP.BytesRead;   /* Save number of read bytes for user */
            }
        }
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPCONTENT) {    /* Read data HTTP response */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+HTTPPARA=\"CONTENT\",\""));   /* Send command */
        UART_SEND_STR(FROMMEM(GSM->HTTP.TMP));
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPPARA, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_HTTPTERM) {       /* Terminate HTTP */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+HTTPTERM"));              /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_HTTPTERM, NULL);         /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_FTPBEGIN) {       /* Start FTP session */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        /**** Enable FTP profile ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPCID=1"));              /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPCID, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpbegin_clean;
        }
        
        /**** Set FTP mode ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPMODE="));              /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr1));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPMODE, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpbegin_clean;
        }
        
        /**** Set FTP over SSL ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPSSL="));               /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr2));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPSSL, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
            
cmd_gprs_ftpbegin_clean:                                    /* Clean everything */
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_FTPEND) {         /* Quit FTP session */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPQUIT"));               /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPQUIT, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_FTPAUTH) {        /* Set user data */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPSERV=\""));            /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr1));
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPSERV, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpauth_clean;
        }
        
        NumberToString(str, Pointers.UI);                   /* Convert port to string */
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPPORT="));              /* Send command */
        UART_SEND_STR(FROMMEM(str));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPPORT, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpauth_clean;
        }
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPUN=\""));              /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr2));
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPUN, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpauth_clean;
        }
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPPW=\""));              /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr3));
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPPW, NULL);            /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */

cmd_gprs_ftpauth_clean:                                     /* Clean everything */
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_FTPDOWNBEGIN) {   /* Begin with download, set folder and file */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        /**** Set upload path ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPGETPATH=\""));         /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr1));
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPGETPATH, NULL);       /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpdownbegin_clean;
        }
        
        /**** Set download name ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPGETNAME=\""));         /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr2));
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPGETNAME, NULL);       /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpdownbegin_clean;
        }
        
        /**** Start FTP download ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPGET=1"));              /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPGET, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpdownbegin_clean;
        }
        
        /* Wait +FTPGET response */
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespFtpGet || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        /* Check response from FTP */
        GSM->ActiveResult = gsmERROR;
        if (GSM->Events.F.RespFtpGet) {                     /* FTPGET received */
            if (GSM->FTP.Mode == 1) {                       /* Session opened */         
                if (GSM->FTP.ErrorCode == 1) {              /* Data ready */
                    GSM->FTP.Flags.F.DataAvailable = 1;     /* We have available data to read */
                    GSM->FTP.Flags.F.DownloadActive = 1;    /* Download session is active */
                    GSM->ActiveResult = gsmOK;
                }
            }
        }
        
cmd_gprs_ftpdownbegin_clean:                                /* Clean everything */
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go idle */
    } else if (GSM->ActiveCmd == CMD_GPRS_FTPDOWN) {        /* Read FTP data */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        NumberToString(str, GSM->FTP.BytesToProcess);
        UART_SEND_STR(FROMMEM("AT+FTPGET=2,"));             /* Send command */
        UART_SEND_STR(FROMMEM(str));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPGET, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmOK) {
            if (GSM->FTP.BytesRead != GSM->FTP.BytesToProcess || GSM->FTP.BytesRead == 0) {
                GSM->FTP.Flags.F.DataAvailable = 0;         /* No data available anyomre */
            }
            GSM->FTP.BytesProcessedTotal += GSM->FTP.BytesRead; /* Increase number of total read bytes */
            if (Pointers.Ptr1 != NULL) {
                *(uint32_t *)Pointers.Ptr1 = GSM->FTP.BytesRead;/* Save number of read bytes for user */
            }
        }
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_FTPDOWNEND) {     /* Finish with FTP downloading */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPGET=1,0"));            /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPGET, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_FTPUPBEGIN) {     /* Begin with upload, set folder and file */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        /**** Set upload mode ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPPUTOPT=\""));          /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr3));
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPPUTOPT, NULL);        /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpupbegin_clean;
        }
        
        
        /**** Set upload path ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPPUTPATH=\""));         /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr1));
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPPUTPATH, NULL);       /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpupbegin_clean;
        }
        
        /**** Set upload name ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPPUTNAME=\""));         /* Send command */
        UART_SEND_STR(FROMMEM(Pointers.CPtr2));
        UART_SEND_STR(FROMMEM("\""));
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPPUTNAME, NULL);       /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpupbegin_clean;
        }
        
        /**** Start FTP upload ****/
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPPUT=1"));              /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPPUT, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        if (GSM->ActiveResult == gsmERROR) {                /* Check for errors */
            goto cmd_gprs_ftpupbegin_clean;
        }
        
        /* Wait +FTPPUT response */
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespFtpPut || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        /* Check response from FTP */
        GSM->ActiveResult = gsmERROR;
        if (GSM->Events.F.RespFtpPut) {                     /* FTPPUT received */
            if (GSM->FTP.Mode == 1) {                       /* Session opened */         
                if (GSM->FTP.ErrorCode == 1) {              /* Data ready */
                    GSM->ActiveResult = gsmOK;
                }
            }
        }
        
cmd_gprs_ftpupbegin_clean:                                  /* Clean everything */
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go idle mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_FTPUP) {          /* Send FTP data */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        do {            
            btw = GSM->FTP.BytesToProcess > GSM->FTP.MaxBytesToPut ? GSM->FTP.MaxBytesToPut : GSM->FTP.BytesToProcess;  /* Set length to send */
            
            NumberToString(str, btw);                       /* Get string from number */
            __RST_EVENTS_RESP(GSM);                         /* Reset events */
            UART_SEND_STR(FROMMEM("AT+FTPPUT=2,"));         /* Send number to GSM */
            UART_SEND_STR(str);
            UART_SEND_STR(GSM_CRLF);
            StartCommand(GSM, CMD_GPRS_FTPPUT, NULL);       /* Start command */
            
            PT_WAIT_UNTIL(pt, GSM->Events.F.RespFtpUploadReady ||
                                GSM->Events.F.RespError);   /* Wait for FTP upload ready or error */
            
            if (GSM->Events.F.RespFtpUploadReady) {         /* We received bracket */
                __RST_EVENTS_RESP(GSM);                     /* Reset events */
                UART_SEND((uint8_t *)GSM->FTP.Data, btw);   /* Send data */
                
        
                /* Wait +FTPPUT response */
                PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                                    GSM->Events.F.RespError);   /* Wait for response */
                
                
                if (GSM->Events.F.RespOk) {                 /* If data were successfully sent */
                    if (Pointers.Ptr1 != NULL) {
                        *(uint32_t *)Pointers.Ptr1 = *(uint32_t *)Pointers.Ptr1 + btw;  /* Increase number of sent bytes */
                    }
                }
                
                PT_WAIT_UNTIL(pt, GSM->Events.F.RespFtpPut || 
                                    GSM->Events.F.RespError);   /* Wait for another FTPPUT */
                
                /* Check response from FTP */
                GSM->ActiveResult = gsmERROR;
                if (GSM->Events.F.RespFtpPut) {             /* FTPPUT received */
                    if (GSM->FTP.Mode == 1) {               /* Positive feedback */         
                        if (GSM->FTP.ErrorCode == 1) {      /* Data ready */
                            GSM->ActiveResult = gsmOK;
                        }
                    }
                }
                
            } else if (GSM->Events.F.RespError) {
                GSM->ActiveResult = gsmERROR;               /* Process error */
            }
            if (GSM->ActiveResult == gsmOK) {
                GSM->FTP.BytesToProcess -= btw;             /* Decrease number of sent bytes */
                Pointers.CPtr1 = (uint8_t *)Pointers.CPtr1 + btw;   /* Set new data memory location to send */
            }
        } while (GSM->FTP.BytesToProcess && GSM->ActiveResult == gsmOK);    /* Until anything to send */
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_FTPUPEND) {       /* Finish with FTP uploading */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+FTPPUT=2,0"));            /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_FTPPUT, NULL);           /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    } else if (GSM->ActiveCmd == CMD_GPRS_CIPGSMLOC) {      /* Get location */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+CIPGSMLOC=1,1"));         /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_GPRS_CIPGSMLOC, NULL);        /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        /* Check if response error number was 0 = OK */
        if (Pointers.Ptr1) {
            if (((GSM_GPS_t *)Pointers.Ptr1)->Error > 0) {  /* Check for response error code */
                GSM->ActiveResult = gsmERROR;               /* Error */
            }
        }
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    }
    
    PT_END(pt);                                             /* End thread */
}

gstatic
PT_THREAD(PT_Thread_OP(struct pt* pt, gvol GSM_t* GSM)) {
    PT_BEGIN(pt);
    
    if (GSM->ActiveCmd == CMD_OP_SCAN) {                    /* Scan for networks */
        __CMD_SAVE(GSM);                                    /* Save command */
        
        __RST_EVENTS_RESP(GSM);                             /* Reset events */
        UART_SEND_STR(FROMMEM("AT+COPS=?"));                /* Send command */
        UART_SEND_STR(GSM_CRLF);
        StartCommand(GSM, CMD_OP_COPS_SCAN, NULL);          /* Start command */
        
        PT_WAIT_UNTIL(pt, GSM->Events.F.RespOk || 
                            GSM->Events.F.RespError);       /* Wait for response */
        
        GSM->ActiveResult = GSM->Events.F.RespOk ? gsmOK : gsmERROR; /* Set result to return */
        
        __CMD_RESTORE(GSM);                                 /* Restore command */
        __IDLE(GSM);                                        /* Go IDLE mode */
    }
    
    PT_END(pt);
}

/* Process all thread calls */
GSM_Result_t ProcessThreads(gvol GSM_t* GSM) {
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
    if (CMD_IS_ACTIVE_OP(GSM)) {                            /* On active network operator related commands */
        PT_Thread_OP(&pt_OP, GSM);                       
    }
#if !GSM_RTOS && !GSM_ASYNC
    GSM_ProcessCallbacks(GSM);                              /* Process callbacks when not in RTOS or ASYNC mode */
#endif
    __RETURN(GSM, gsmOK);
}

/******************************************************************************/
/******************************************************************************/
/***                                Public API                               **/
/******************************************************************************/
/******************************************************************************/
GSM_Result_t GSM_Init(gvol GSM_t* G, const char* pin, uint32_t Baudrate, GSM_EventCallback_t callback) {
    uint32_t i = 50;
    BUFFER_t* Buff = &Buffer;
    
    GSM = G;                                                /* Save working pointer */
    
    memset((void *)GSM, 0x00, sizeof(GSM_t));               /* Clear structure first */
    
    /* Set callback */
    GSM->Callback = callback;                               /* Set event callback */
    if (!GSM->Callback) {
        GSM->Callback = GSM_CallbackDefault;                /* Set default callback function */
    }
    
    /* Set start values */
    GSM->CPIN = GSM_CPIN_Unknown;                           /* Force SIM checking */
    GSM->CallInfo.State = GSM_CallState_Disconnect;         /* Set default call state */
    
    GSM->Time = 0;                                          /* Reset time start time */
    BUFFER_Init(Buff, GSM_BUFFER_SIZE, Buffer_Data);        /* Init buffer for receive */
    
    /* Low-Level initialization */
    GSM->LL.Baudrate = Baudrate;
    if (GSM_LL_Init((GSM_LL_t *)&GSM->LL)) {                /* Init low-level */
        __RETURN(GSM, gsmLLERROR);                          /* Return error */
    }
    
    /* Set reset low */
    GSM_LL_SetReset((GSM_LL_t *)&GSM->LL, GSM_RESET_SET);   /* Set reset */
    GSM_Delay(GSM, 10);
    GSM_LL_SetReset((GSM_LL_t *)&GSM->LL, GSM_RESET_CLR);   /* Clear reset */
    
#if GSM_RTOS
    /* RTOS support */
    if (GSM_SYS_Create((GSM_RTOS_SYNC_t *)&GSM->Sync)) {    /* Init sync object */
        __RETURN(GSM, gsmSYSERROR);
    }
#endif
    
    /* Init all threads */
    __RESET_THREADS(GSM);
    
    /* Send initialization commands */
    GSM->Flags.F.IsBlocking = 1;                            /* Process blocking calls */
    GSM->ActiveCmdTimeout = 1000;                           /* Give 1 second timeout */
    memset((void *)&Pointers, 0x00, sizeof(Pointers));      /* Reset structure */
    while (i) {
        __ACTIVE_CMD(GSM, CMD_GEN_AT);                      /* Restore to factory settings */
        GSM_WaitReady(GSM, 1000);
        if (GSM->ActiveResult == gsmOK) {
            break;
        }
        i--;
    }
    while (i) {
        __ACTIVE_CMD(GSM, CMD_GEN_FACTORY_SETTINGS);        /* Restore to factory settings */
        GSM_WaitReady(GSM, 1000);
        if (GSM->ActiveResult == gsmOK) {
            break;
        }
        i--;
    }
    while (i) {
        __ACTIVE_CMD(GSM, CMD_GEN_ERROR_NUMERIC);           /* Enable numeric error codes */
        GSM_WaitReady(GSM, 1000);
        if (GSM->ActiveResult == gsmOK) {
            break;
        }
        i--;
    }
    while (i) {  
        __ACTIVE_CMD(GSM, CMD_GEN_CALL_CLCC);               /* Enable auto notification for call, +CLCC statement */
        GSM_WaitReady(GSM, 1000);
        if (GSM->ActiveResult == gsmOK) {
            break;
        }
        i--;
    }    
    while (i) {
        Pointers.CPtr1 = pin;
        __ACTIVE_CMD(GSM, CMD_PIN);                         /* Enable auto notification for call, +CLCC statement */
        GSM_WaitReady(GSM, 1000);
        if (GSM->ActiveResult == gsmOK) {
            break;
        }
        GSM_Delay(GSM, 100);
        i--;
    }
    memset((void *)&Pointers, 0x00, sizeof(Pointers));      /* Reset structure */
    while (i) {
        __ACTIVE_CMD(GSM, CMD_INFO_GMR);                    /* Enable auto notification for call, +CLCC statement */
        GSM_WaitReady(GSM, 1000);
        if (GSM->ActiveResult == gsmOK) {
            break;
        }
        i--;
    }
    while (i) {
        __ACTIVE_CMD(GSM, CMD_GEN_ATE1);                    /* Disable ECHO */
        GSM_WaitReady(GSM, 1000);
        if (GSM->ActiveResult == gsmOK) {
            break;
        }
        i--;
    }  
    while (i) {
        __ACTIVE_CMD(GSM, CMD_GEN_CFUN_GET);                /* Get phone functionality */
        GSM_WaitReady(GSM, 1000);
        if (GSM->ActiveResult == gsmOK) {
            break;
        }
        i--;
    }    
    GSM->Flags.F.IsBlocking = 0;                            /* Reset blocking calls */
    __IDLE(GSM);                                            /* Process IDLE */
    GSM->Flags.F.Call_Idle = 0;
    
    __RETURN(G, GSM->ActiveResult);                         /* Return active status */
}

GSM_Result_t GSM_WaitReady(gvol GSM_t* GSM, uint32_t timeout) {
    GSM->ActiveCmdTimeout = timeout;                        /* Set timeout value */
    do {
#if !GSM_RTOS && !GSM_ASYNC
        GSM_Update(GSM);
#endif
    } while (__IS_BUSY(GSM));
    __RETURN(GSM, GSM->ActiveResult);                       /* Return active result from command */
}

GSM_Result_t GSM_Delay(gvol GSM_t* GSM, uint32_t timeout) {
    volatile uint32_t start = GSM->Time;
    do {
#if !GSM_RTOS && !GSM_ASYNC
        GSM_Update(GSM);
#endif
    } while (GSM->Time - start < timeout);
    __RETURN(GSM, gsmOK);
}

GSM_Result_t GSM_IsReady(gvol GSM_t* GSM) {
    return GSM->ActiveCmd != CMD_IDLE ? gsmERROR : gsmOK;
}

GSM_Result_t GSM_Update(gvol GSM_t* GSM) {
    char ch;
    static char prev1_ch = 0x00, prev2_ch = 0x00;
    BUFFER_t* Buff = &Buffer;
    uint16_t processedCount = 500;
    
    /* Check for timeout */
    if (GSM->ActiveCmd != CMD_IDLE && (GSM->Time - GSM->ActiveCmdStart) > GSM->ActiveCmdTimeout) {
        GSM->Events.F.RespTimeout = 1;                      /* Timeout received */
        GSM->Events.F.RespError = 1;                        /* Set active error and process */
    }
    
    while (
#if !GSM_RTOS && GSM_ASYNC
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
        } else if (GSM->ActiveCmd == CMD_GPRS_FTPGET && GSM->Flags.F.FTP_Read_Data) {
            GSM->FTP.Data[GSM->FTP.BytesRead++] = ch;       /* Save character */
            GSM->FTP.BytesReadRemaining--;                  /* Decrease number of remaining bytes to read */
            if (!GSM->FTP.BytesReadRemaining) {             /* We finished? */
                GSM->Flags.F.FTP_Read_Data = 0;             /* Reset flag, go back to normal parsing */
                processedCount = 0;                         /* Stop further processing */
            }
        } else if (GSM->ActiveCmd == CMD_GPRS_CIPRXGET && GSM->Flags.F.CLIENT_Read_Data) {  /* We are trying to read raw data? */
            GSM_CONN_t* conn = (GSM_CONN_t *)Pointers.Ptr1;
            conn->ReceiveData[conn->BytesRead++] = ch;      /* Save character */
            conn->BytesReadRemaining--;                     /* Decrease number of remaining bytes to read */
            if (!conn->BytesReadRemaining) {                /* We finished? */
                GSM->Flags.F.CLIENT_Read_Data = 0;          /* Reset flag, go back to normal parsing */
                conn->Flags.F.RxGetReceived = 0;            /* Reset RX received flag */
                conn->Flags.F.CallGetReceived = 0;          /* Reset flag for notification if not already */
                processedCount = 0;                         /* Stop further processing */
            }
        } else if (GSM->ActiveCmd == CMD_SMS_READ && GSM->Flags.F.SMS_Read_Data) {  /* We are execution SMS read command and we are trying to read actual SMS data */
            GSM_SMS_Entry_t* read = (GSM_SMS_Entry_t *) Pointers.Ptr1;  /* Read pointer and cast to SMS entry */
            if (read->DataLen < (sizeof(read->Data) - 1)) { /* Still memory available? */
                read->Data[read->DataLen++] = ch;           /* Save character */
            }
            if (ch == '\n' && prev1_ch == '\r' && read->DataLen >= 2) { /* We finished? */
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
                if (ch == '\n' && prev1_ch == '\r' && read->DataLen >= 2) { /* We finished? */
                    read->DataLen -= 2;                     /* Remove CRLF characters */
                    read->Data[read->DataLen] = 0;          /* Finish this statement */
                    
                    Pointers.Ptr1 = (GSM_SMS_Entry_t *)Pointers.Ptr1 + 1;  /* Increase pointer */ 
                    *(uint16_t *)Pointers.Ptr2 = (*(uint16_t *)Pointers.Ptr2) + 1;  /* Increase number of parsed elements */
                    
                    GSM->Flags.F.SMS_Read_Data = 0;         /* Clear flag, no more reading data */
                    processedCount = 0;                     /* Stop further processing */
                }
            } else if (ch == '\n' && prev1_ch == '\r') {    /* Process dummy read, ignore messages if no memory specified buy still finish reading when done */
                GSM->Flags.F.SMS_Read_Data = 0;             /* Clear flag, no more reading data */
                processedCount = 0;                         /* Stop further processing */
            }
        } else if (GSM->Flags.F.COPS_Read_Operators) {
            if (ch == '\n') {
                GSM->Flags.F.COPS_Read_Operators = 0;       /* Reset reading structure */ 
            } else {
                ParseCOPSSCAN(GSM, ch, ch == ':');          /* Parse COPS scan characters */
            }
        } else {
            switch (ch) {
                case '\n':
                    RECEIVED_ADD(ch);                       /* Add character */
                    ParseReceived(GSM, &Received);          /* Parse received string */
                    RECEIVED_RESET();                       /* Reset received array */
                    break;
                default: 
                    if (ch == ' ' && prev1_ch == '>' && prev2_ch == '\n') { /* Check if bracket received */
                        GSM->Events.F.RespBracket = 1;      /* We receive bracket on command */
                    }
                    RECEIVED_ADD(ch);                       /* Add character to buffer */
                    if (GSM->ActiveCmd == CMD_OP_COPS_SCAN) {   /* In case of scan network operators */
                        if (RECEIVED_LENGTH() == 5 && Received.Data[0] == '+' && strncmp((char *)Received.Data, "+COPS", 5) == 0) {
                            GSM->Flags.F.COPS_Read_Operators = 1;   /* Set COPS scan reading flag */
                            RECEIVED_RESET();               /* Reset received string */
                        }
                    }
                break;
            }
        }
        prev2_ch = prev1_ch;                                /* Save previous character to prevprev character */
        prev1_ch = ch;                                      /* Save current character as previous */      
    }
    return ProcessThreads(GSM);                             /* Process stack */
}

GSM_Result_t GSM_UpdateTime(gvol GSM_t* GSM, uint32_t millis) {
    GSM->Time += millis;                                    /* Increase milliseconds for amount of ticks */
    __RETURN(GSM, gsmOK);
}

GSM_Result_t GSM_ProcessCallbacks(gvol GSM_t* GSM) {
    uint8_t i;
    /* Process callbacks */
    if (GSM->ActiveCmd == CMD_IDLE && GSM->Flags.F.Call_Idle) {
        GSM->Flags.F.Call_Idle = 0;
        __CALL_CALLBACK(GSM, gsmEventIdle);
    }
    if (__IS_READY(GSM) && GSM->Flags.F.CALL_CLCC_Received) {
        GSM->Flags.F.CALL_CLCC_Received = 0;
        GSM->CallbackParams.CP1 = (GSM_CallInfo_t *)&GSM->CallInfo;
        __CALL_CALLBACK(GSM, gsmEventCallCLCC);
    }
    if (__IS_READY(GSM) && GSM->Flags.F.CALL_RING_Received) {
        GSM->Flags.F.CALL_RING_Received = 0;
        GSM->CallbackParams.CP1 = (GSM_CallInfo_t *)&GSM->CallInfo;
        __CALL_CALLBACK(GSM, gsmEventCallRING);
    }
    if (__IS_READY(GSM) && GSM->Flags.F.SMS_CMTI_Received) {
        GSM->Flags.F.SMS_CMTI_Received = 0;
        __CALL_CALLBACK(GSM, gsmEventSMSCMTI);
    }
    if (__IS_READY(GSM) && GSM->Flags.F.Call_GPRS_Attached) {
        GSM->Flags.F.Call_GPRS_Attached = 0;
        __CALL_CALLBACK(GSM, gsmEventGPRSAttached);
    }
    if (__IS_READY(GSM) && GSM->Flags.F.Call_GPRS_Attach_Error) {
        GSM->Flags.F.Call_GPRS_Attach_Error = 0;
        __CALL_CALLBACK(GSM, gsmEventGPRSAttachError);
    }
    if (__IS_READY(GSM) && GSM->Flags.F.Call_GPRS_Detached) {
        GSM->Flags.F.Call_GPRS_Detached = 0;
        __CALL_CALLBACK(GSM, gsmEventGPRSDetached);
    }
    if (__IS_READY(GSM) && GSM->Flags.F.Call_UV_Warn) {
        GSM->Flags.F.Call_UV_Warn = 0;
        __CALL_CALLBACK(GSM, gsmEventUVWarning);
    }
    if (__IS_READY(GSM) && GSM->Flags.F.Call_UV_PD) {
        GSM->Flags.F.Call_UV_PD = 0;
        __CALL_CALLBACK(GSM, gsmEventUVPowerDown);
    }
    /* Check connection specific callbacks */
    for (i = 0; i < 6; i++) {
        if (!GSM->Conns[i]) {                               /* Check if connection is valid */
            continue;
        }
        if (__IS_READY(GSM) && GSM->Conns[i]->Flags.F.CallGetReceived) {  /* Notify user about new data */
            GSM->Conns[i]->Flags.F.CallGetReceived = 0;     /* Reset flag status */
            
            GSM->CallbackParams.CP1 = GSM->Conns[i];        /* Set connection parameters */
            __CALL_CALLBACK(GSM, gsmEventDataReceived);     /* Call user function */
        }
    }
    __RETURN(GSM, gsmOK);
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
/***                         PHONE FUNCTIONALITY API                         **/
/******************************************************************************/
GSM_Result_t GSM_FUNC_Set(gvol GSM_t* GSM, GSM_Func_t func, uint32_t blocking) {
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GEN_CFUN_SET);                    /* Set active command */
    
    Pointers.UI = func;
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_FUNC_Get(gvol GSM_t* GSM, GSM_Func_t* func, uint32_t blocking) {
    __CHECK_INPUTS(func);                                   /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GEN_CFUN_GET);                    /* Set active command */
    
    Pointers.Ptr1 = func;
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

/******************************************************************************/
/***                                INFO API                                 **/
/******************************************************************************/
GSM_Result_t GSM_INFO_GetManufacturer(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking) {
    __CHECK_INPUTS(str);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_INFO_CGMI);                       /* Set active command */
    
    Pointers.Ptr1 = str;
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_INFO_GetModel(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking) {
    __CHECK_INPUTS(str);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_INFO_CGMM);                       /* Set active command */
    
    Pointers.Ptr1 = str;
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_INFO_GetRevision(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking) {
    __CHECK_INPUTS(str);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_INFO_CGMR);                       /* Set active command */
    
    Pointers.Ptr1 = str;
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_INFO_GetSerialNumber(gvol GSM_t* GSM, char* str, uint32_t length, uint32_t blocking) {
    __CHECK_INPUTS(str);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_INFO_CGSN);                       /* Set active command */
    
    Pointers.Ptr1 = str;
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_INFO_GetSoftwareInfo(gvol GSM_t* GSM, char* rev, uint32_t blocking) {
    __CHECK_INPUTS(rev);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_INFO_GMR);                        /* Set active command */
    
    Pointers.Ptr1 = rev;
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_INFO_GetBatteryInfo(gvol GSM_t* GSM, GSM_Battery_t* bat, uint32_t blocking) {
    __CHECK_INPUTS(bat);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_INFO_CBC);                        /* Set active command */
    
    Pointers.Ptr1 = bat;
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

/******************************************************************************/
/***                           NETWORK OPERATOR API                          **/
/******************************************************************************/
GSM_Result_t GSM_OP_Scan(gvol GSM_t* GSM, GSM_OP_t* ops, uint16_t optr, uint16_t* opr, uint32_t blocking) {
    __CHECK_INPUTS(ops && optr && opr);                     /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_OP_SCAN);                         /* Set active command */
    
    Pointers.Ptr1 = ops;
    Pointers.Ptr2 = opr;
    Pointers.UI = optr;
    *opr = 0;
    
    __RETURN_BLOCKING(GSM, blocking, 60000);                 /* Return with blocking support */
}

/******************************************************************************/
/***                               PIN/PUK API                               **/
/******************************************************************************/
GSM_Result_t GSM_PIN_Enter(gvol GSM_t* GSM, const char* pin, uint32_t blocking) {
    __CHECK_INPUTS(pin);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_PIN);                             /* Set active command */
    
    Pointers.CPtr1 = pin;                                   /* Save pointer to PIN */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_PIN_Remove(gvol GSM_t* GSM, const char* current_pin, uint32_t blocking) {
    __CHECK_INPUTS(current_pin);                            /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy flag */
    __ACTIVE_CMD(GSM, CMD_PIN_REMOVE);                      /* Set active command */
    
    Pointers.CPtr1 = current_pin;                           /* Save pointer to PIN */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_PIN_Add(gvol GSM_t* GSM, const char* new_pin, uint32_t blocking) {
    __CHECK_INPUTS(new_pin);                                /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy flag */
    __ACTIVE_CMD(GSM, CMD_PIN_ADD);                         /* Set active command */
    
    Pointers.CPtr1 = new_pin;                               /* Save pointer to PIN */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_PUK_Enter(gvol GSM_t* GSM, const char* puk, const char* new_pin, uint32_t blocking) {
    __CHECK_INPUTS(puk && new_pin);                         /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_PUK);                             /* Set active command */
    
    Pointers.CPtr1 = puk;                                   /* Save pointer to PUK */
    Pointers.CPtr2 = new_pin;                               /* Save pointer to new PIN */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

/******************************************************************************/
/***                                CALL API                                 **/
/******************************************************************************/
GSM_Result_t GSM_CALL_Voice(gvol GSM_t* GSM, const char* number, uint32_t blocking) {
    __CHECK_INPUTS(number);                                 /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_CALL_VOICE);                      /* Set active command */
    
    Pointers.CPtr1 = number;                                /* Save pointer to PIN */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);
}

GSM_Result_t GSM_CALL_VoiceFromSIMPosition(gvol GSM_t* GSM, uint16_t pos, uint32_t blocking) {
    __CHECK_INPUTS(pos);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_CALL_VOICE_SIM_POS);              /* Set active command */
    
    Pointers.UI = pos;                                      /* Save position */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);
}

GSM_Result_t GSM_CALL_DataFromSIMPosition(gvol GSM_t* GSM, uint16_t pos, uint32_t blocking) {
    __CHECK_INPUTS(pos);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_CALL_DATA_SIM_POS);               /* Set active command */
    
    Pointers.UI = pos;                                      /* Save position */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);
}

GSM_Result_t GSM_CALL_Data(gvol GSM_t* GSM, const char* number, uint32_t blocking) {
    __CHECK_INPUTS(number);                                 /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_CALL_DATA);                       /* Set active command */
    
    Pointers.CPtr1 = number;                                /* Save pointer to PIN */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return as blocking if required */
}

GSM_Result_t GSM_CALL_Answer(gvol GSM_t* GSM, uint32_t blocking) {
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_CALL_ANSWER);                     /* Set active command */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_CALL_HangUp(gvol GSM_t* GSM, uint32_t blocking) {
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_CALL_HANGUP);                     /* Set active command */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */   
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
    __CHECK_INPUTS(number && data && 
                    strlen(data) <= GSM_SMS_MAX_LENGTH);    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_SMS_SEND);                        /* Set active command */
    
    GSM->SMS.Data = data;
    GSM->SMS.Number = number;
    
    __RETURN_BLOCKING(GSM, blocking, 30000);                /* Return with blocking possibility */
}

GSM_Result_t GSM_SMS_Read(gvol GSM_t* GSM, uint16_t position, GSM_SMS_Entry_t* SMS, uint32_t blocking) {
    __CHECK_INPUTS(SMS);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_SMS_READ);                        /* Set active command */
    
    SMS->Position = position;                               /* Save position for SMS */
    Pointers.Ptr1 = SMS;                                    /* Save non-const pointer */
    
    __RETURN_BLOCKING(GSM, blocking, 500);                  /* Return with blocking possibility */
}

GSM_Result_t GSM_SMS_List(gvol GSM_t* GSM, GSM_SMS_ReadType_t type, GSM_SMS_Entry_t* entries, uint16_t entries_count, uint16_t* entries_read, uint32_t blocking) {
    __CHECK_INPUTS(entries && entries_read);                /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_SMS_LIST);                        /* Set active command */
    
    *entries_read = 0;                                      /* Reset variable */
    
    switch (type) {                                         /* Check for proper type */
        case GSM_SMS_ReadType_ALL: Pointers.CPtr1 = FROMMEM("ALL"); break;
        case GSM_SMS_ReadType_READ: Pointers.CPtr1 = FROMMEM("READ"); break;
        case GSM_SMS_ReadType_UNREAD: Pointers.CPtr1 = FROMMEM("UNREAD"); break;
        default: return gsmPARERROR;
    }
    Pointers.Ptr1 = entries;                                /* Save pointer to entries */
    Pointers.Ptr2 = entries_read;                           /* Save pointer to entries we already read */
    Pointers.UI = entries_count;                            /* Save number of entries we can save in entries */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking possibility */
}

GSM_Result_t GSM_SMS_Delete(gvol GSM_t* GSM, uint16_t position, uint32_t blocking) {
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_SMS_DELETE);                      /* Set active command */
    
    Pointers.UI = position;                                 /* Save number */
    
    __RETURN_BLOCKING(GSM, blocking, 50);                   /* Return with blocking possibility */
}

GSM_Result_t GSM_SMS_MassDelete(gvol GSM_t* GSM, GSM_SMS_MassDelete_t type, uint32_t blocking) {
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_SMS_MASSDELETE);                  /* Set active command */
    
    switch (type) {                                         /* Select MASS delete type */
        case GSM_SMS_MassDelete_Read:
            Pointers.CPtr1 = FROMMEM("READ");
            break;
        case GSM_SMS_MassDelete_Unread:
            Pointers.CPtr1 = FROMMEM("UNREAD");
            break;
        case GSM_SMS_MassDelete_Sent:
            Pointers.CPtr1 = FROMMEM("SENT");
            break;
        case GSM_SMS_MassDelete_Unsent:
            Pointers.CPtr1 = FROMMEM("UNSENT");
            break;
        case GSM_SMS_MassDelete_Inbox:
            Pointers.CPtr1 = FROMMEM("INBOX");
            break;
        case GSM_SMS_MassDelete_All:
            Pointers.CPtr1 = FROMMEM("ALL");
            break;
        default:
            break;
    }
    __RETURN_BLOCKING(GSM, blocking, 50);                   /* Return with blocking possibility */   
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
    __CHECK_INPUTS(name && number);                         /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_PB_ADD);                          /* Set active command */
    
    Pointers.CPtr1 = number;                                /* Save pointer to number */
    Pointers.CPtr2 = name;                                  /* Save pointer to name */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_PB_Edit(gvol GSM_t* GSM, uint32_t index, const char* name, const char* number, uint32_t blocking) {
    __CHECK_INPUTS(index && name && number);                /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_PB_EDIT);                         /* Set active command */
    
    Pointers.CPtr1 = number;                                /* Save pointer to number */
    Pointers.CPtr2 = name;                                  /* Save pointer to name */
    Pointers.UI = index;                                    /* Save index */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_PB_Delete(gvol GSM_t* GSM, uint32_t index, uint32_t blocking) {
    __CHECK_INPUTS(index);                                  /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_PB_DELETE);                       /* Set active command */
    
    Pointers.UI = index;                                    /* Save index */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_PB_Get(gvol GSM_t* GSM, uint32_t index, GSM_PB_Entry_t* entry, uint32_t blocking) {
    __CHECK_INPUTS(index && entry);                         /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_PB_GET);                          /* Set active command */
    
    Pointers.Ptr1 = entry;                                  /* Save pointer to entry object */
    Pointers.UI = index;                                    /* Save index */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_PB_List(gvol GSM_t* GSM, GSM_PB_Entry_t* entries, uint16_t start_index, uint16_t btr, uint16_t* br, uint32_t blocking) {
    __CHECK_INPUTS(entries && start_index && btr && br);    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_PB_LIST);                         /* Set active command */
    
    *br = 0;
    Pointers.Ptr1 = entries;                                /* Save pointer to entries */
    Pointers.Ptr2 = br;                                     /* Save pointer to number of entries read */
    Pointers.UI = start_index << 16 | ((start_index + btr - 1) & 0xFFFF); /* Save start index and entries count to read */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_PB_Search(gvol GSM_t* GSM, const char* search, GSM_PB_Entry_t* entries, uint16_t btr, uint16_t* br, uint32_t blocking) {
    __CHECK_INPUTS(search && entries && btr && br);         /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_PB_SEARCH);                       /* Set active command */
    
    *br = 0;                                                /* Reset count first */
    Pointers.CPtr1 = search;                                /* Save pointer to search string */
    Pointers.Ptr1 = entries;                                /* Save pointer to entries */
    Pointers.Ptr2 = br;                                     /* Save pointer to number of entries read */
    Pointers.UI = btr;                                      /* Save number of max elements we can read from search command */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}
/******************************************************************************/
/***                              DATETIME API                               **/
/******************************************************************************/
GSM_Result_t GSM_DATETIME_Get(gvol GSM_t* GSM, GSM_DateTime_t* datetime, uint32_t blocking) {
    __CHECK_INPUTS(datetime);                               /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_DATETIME_GET);                    /* Set active command */
    
    Pointers.Ptr1 = datetime;                               /* Save pointer to entries */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

/******************************************************************************/
/***                                GPRS API                                 **/
/******************************************************************************/
GSM_Result_t GSM_GPRS_Attach(gvol GSM_t* GSM, const char* apn, const char* user, const char* pwd, uint32_t blocking) {
    __CHECK_INPUTS(apn);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_ATTACH);                     /* Set active command */
     
    Pointers.CPtr1 = apn;                                   /* Save pointers */
    Pointers.CPtr2 = NULL;
    Pointers.CPtr3 = NULL;
    if (user && strlen(user)) {
        Pointers.CPtr2 = user;                              /* Set pointer to username */
    }
    if (pwd && strlen(pwd)) {
        Pointers.CPtr3 = pwd;                               /* Set pointer to password */
    }   
    __RETURN_BLOCKING(GSM, blocking, 180000);               /* Return with blocking support */
}

GSM_Result_t GSM_GPRS_Detach(gvol GSM_t* GSM, uint32_t blocking) {
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_DETACH);                     /* Set active command */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_GPRS_GetLocationAndTime(gvol GSM_t* GSM, GSM_GPS_t* GPS, uint32_t blocking) {
    __CHECK_INPUTS(GPS);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_CIPGSMLOC);                  /* Set active command */
     
    Pointers.Ptr1 = GPS;
    
    __RETURN_BLOCKING(GSM, blocking, 30000);                /* Return with blocking support */
}

/******************************************************************************/
/***                           CLIENT TCP/UDP API                            **/
/******************************************************************************/
GSM_Result_t GSM_CONN_Start(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, GSM_CONN_Type_t type, GSM_CONN_SSL_t ssl, const char* host, uint16_t port, uint32_t blocking) {
    __CHECK_INPUTS(conn && host);                           /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_CIPSTART);                   /* Set active command */
     
    Pointers.Ptr1 = conn;                                   /* Save connection pointer */
    Pointers.CPtr1 = host;                                  /* Save pointers */
    Pointers.CPtr2 = FROMMEM(type == GSM_CONN_Type_TCP ? "TCP" : "UDP");
    Pointers.CPtr3 = FROMMEM(type == GSM_CONN_Type_TCP && ssl ? "1" : "0");
    Pointers.UI = port;                                     /* Save port */
    
    conn->ID = 0;                                           /* Set connection ID */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_CONN_Send(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, const void* data, uint16_t btw, uint32_t* bw, uint32_t blocking) {
    __CHECK_INPUTS(conn && data && btw);                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_CIPSEND);                    /* Set active command */
     
    Pointers.Ptr1 = conn;                                   /* Save connection pointer */
    Pointers.Ptr2 = bw;                                     /* Pointer to actual data sent */
    Pointers.CPtr1 = data;                                  /* Save data pointer */
    Pointers.UI = btw;                                      /* Save data length */
    if (bw) {
        *bw = 0;
    }
    
    __RETURN_BLOCKING(GSM, blocking, 10000);              /* Return with blocking support */
}

GSM_Result_t GSM_CONN_Receive(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, void* data, uint16_t btr, uint32_t* br, uint16_t timeBeforeRead, uint32_t blocking) {
    __CHECK_INPUTS(conn && data && btr);                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_CIPRXGET);                   /* Set active command */
    
    conn->ReceiveData = data;                               /* Save pointer to data */
    conn->BytesToRead = btr;
    conn->BytesRead = 0;
    conn->ReadTimeout = timeBeforeRead;                     /* Set time before reading */
    
    Pointers.Ptr1 = conn;                                   /* Save connection pointer */
    Pointers.Ptr2 = br;                                     /* Save read pointer */
    if (br != NULL) {                                       /* Reset counter */
        *br = 0;
    }
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_CONN_Close(gvol GSM_t* GSM, gvol GSM_CONN_t* conn, uint32_t blocking) {
    __CHECK_INPUTS(conn);                                   /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_CIPCLOSE);                   /* Set active command */
     
    Pointers.Ptr1 = conn;                                   /* Save connection pointer */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
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
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_HTTPBEGIN);                  /* Set active command */
    
    memset((void *)&GSM->HTTP, 0x00, sizeof(GSM->HTTP));    /* Set structure to zero */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_HTTP_End(gvol GSM_t* GSM, uint32_t blocking) {
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_HTTPTERM);                   /* Set active command */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_HTTP_SetData(gvol GSM_t* GSM, const void* data, uint32_t btw, uint32_t blocking) {
    __CHECK_INPUTS(data && btw);                            /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_HTTPSEND);                   /* Set active command */
    
    GSM->HTTP.Data = (uint8_t *)data;                       /* Save data pointer */
    GSM->HTTP.DataLength = btw;                             /* Set data length */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_HTTP_SetContent(gvol GSM_t* GSM, const char* content, uint32_t blocking) {
    __CHECK_INPUTS(content);                                /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_HTTPCONTENT);                /* Set active command */
    
    GSM->HTTP.TMP = content;                                /* Save content pointer */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_HTTP_Execute(gvol GSM_t* GSM, const char* url, GSM_HTTP_Method_t method, GSM_HTTP_SSL_t ssl, uint32_t blocking) {
    __CHECK_INPUTS(url);                                    /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_HTTPEXECUTE);                /* Set active command */
    
    /* Reset informations */
    GSM->HTTP.BytesRead = 0;
    GSM->HTTP.BytesReadRemaining = 0;
    GSM->HTTP.BytesReadTotal = 0;
    GSM->HTTP.BytesReceived = 0;
    GSM->HTTP.BytesToRead = 0;
    
    GSM->HTTP.TMP = url;                                    /* Save URL */
    GSM->HTTP.Method = method;                              /* Set request method */
    Pointers.CPtr1 = FROMMEM(ssl ? "1" : "0");
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_HTTP_Read(gvol GSM_t* GSM, void* data, uint32_t btr, uint32_t* br, uint32_t blocking) {
    __CHECK_INPUTS(data && btr);                            /* Check valid data */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_HTTPREAD);                   /* Set active command */
    
    GSM->HTTP.Data = data;                                  /* Set data pointer */
    GSM->HTTP.DataLength = btr;                             /* Set data array length */
    GSM->HTTP.BytesToRead = btr;                            /* Set number of bytes to read from response */
    Pointers.Ptr1 = br;
    if (br != NULL) {                                       /* Reset counter */
        *br = 0;
    }
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

uint32_t GSM_HTTP_DataAvailable(gvol GSM_t* GSM, uint32_t blocking) {
    return GSM->HTTP.BytesReceived - GSM->HTTP.BytesReadTotal;
}

/******************************************************************************/
/***                                 FTP API                                 **/
/******************************************************************************/
GSM_Result_t GSM_FTP_Begin(gvol GSM_t* GSM, GSM_FTP_Mode_t mode, GSM_FTP_SSL_t ssl, uint32_t blocking) {
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_FTPBEGIN);                   /* Set active command */
    
    Pointers.CPtr1 = FROMMEM(mode ? "1" : "0");
    switch (ssl) {
        case GSM_FTP_SSL_Disable:
            Pointers.CPtr2 = FROMMEM("0");
            break;
        case GSM_FTP_SSL_Implicit:
            Pointers.CPtr2 = FROMMEM("1");
            break;
        case GSM_FTP_SSL_Explicit:
            Pointers.CPtr2 = FROMMEM("2");
            break;
    }
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_FTP_End(gvol GSM_t* GSM, uint32_t blocking) {
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_FTPEND);                     /* Set active command */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_FTP_Authenticate(gvol GSM_t* GSM, const char* server, uint16_t port, const char* user, const char* pass, uint32_t blocking) {
    __CHECK_INPUTS(server && port && user && pass);         /* Check input values */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_FTPAUTH);                    /* Set active command */
    
    memset((void *)&GSM->FTP, 0x00, sizeof(GSM_FTP_t));     /* Reset structure */
    
    Pointers.CPtr1 = server;                                /* Save pointers */
    Pointers.CPtr2 = user;
    Pointers.CPtr3 = pass;
    Pointers.UI = port;
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}

GSM_Result_t GSM_FTP_DownloadBegin(gvol GSM_t* GSM, const char* folder, const char* file, uint32_t blocking) {
    __CHECK_INPUTS(folder && file);                         /* Check input values */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_FTPDOWNBEGIN);               /* Set active command */
    
    memset((void *)&GSM->FTP, 0x00, sizeof(GSM_FTP_t));     /* Reset structure */
    
    Pointers.CPtr1 = folder;                                /* Save pointers */
    Pointers.CPtr2 = file;
    
    __RETURN_BLOCKING(GSM, blocking, 75000);                /* Return with blocking support */
}

GSM_Result_t GSM_FTP_Download(gvol GSM_t* GSM, void* data, uint32_t btr, uint32_t* br, uint32_t blocking) {
    __CHECK_INPUTS(data && btr && br);                      /* Check input values */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_FTPDOWN);                    /* Set active command */
    
    GSM->FTP.BytesToProcess = btr;
    GSM->FTP.Data = data;
    Pointers.Ptr1 = br;
    if (br) {
        *br = 0;
    }
    
    __RETURN_BLOCKING(GSM, blocking, 2000);                 /* Return with blocking support */
}

GSM_Result_t GSM_FTP_DownloadActive(gvol GSM_t* GSM, uint32_t blocking) {
    return GSM->FTP.Flags.F.DownloadActive ? gsmOK : gsmERROR;
}

GSM_Result_t GSM_FTP_DownloadAvailable(gvol GSM_t* GSM, uint32_t blocking) {
    return GSM->FTP.Flags.F.DataAvailable ? gsmOK : gsmERROR;
}

GSM_Result_t GSM_FTP_DownloadEnd(gvol GSM_t* GSM, uint32_t blocking) {
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_FTPDOWNEND);                 /* Set active command */
    
    __RETURN_BLOCKING(GSM, blocking, 60000);                /* Return with blocking support */
}

GSM_Result_t GSM_FTP_UploadBegin(gvol GSM_t* GSM, const char* folder, const char* file, GSM_FTP_UploadMode_t mode, uint32_t blocking) {
    __CHECK_INPUTS(folder && file);                         /* Check input values */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_FTPUPBEGIN);                 /* Set active command */
    
    memset((void *)&GSM->FTP, 0x00, sizeof(GSM_FTP_t));     /* Reset structure */
    
    Pointers.CPtr1 = folder;                                /* Save pointers */
    Pointers.CPtr2 = file;
    switch (mode) {                                         /* Set FTP upload mode */
        case GSM_FTP_UploadMode_Append:
            Pointers.CPtr3 = FROMMEM("APPE");
            break;
        case GSM_FTP_UploadMode_StoreUnique:
            Pointers.CPtr3 = FROMMEM("STOU");
            break;
        case GSM_FTP_UploadMode_Store:
            Pointers.CPtr3 = FROMMEM("STOR");
            break;
    }
    
    __RETURN_BLOCKING(GSM, blocking, 75000);                /* Return with blocking support */
}

GSM_Result_t GSM_FTP_Upload(gvol GSM_t* GSM, const void* data, uint32_t btw, uint32_t* bw, uint32_t blocking) {
    __CHECK_INPUTS(data && btw && bw);                      /* Check input values */
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_FTPUP);                      /* Set active command */
    
    GSM->FTP.BytesToProcess = btw;
    GSM->FTP.Data = (void *)data;
    Pointers.Ptr1 = bw;
    if (bw) {
        *bw = 0;
    }
    
    __RETURN_BLOCKING(GSM, blocking, 10000);                /* Return with blocking support */
}

GSM_Result_t GSM_FTP_UploadEnd(gvol GSM_t* GSM, uint32_t blocking) {
    __CHECK_BUSY(GSM);                                      /* Check busy status */
    __ACTIVE_CMD(GSM, CMD_GPRS_FTPUPEND);                   /* Set active command */
    
    __RETURN_BLOCKING(GSM, blocking, 1000);                 /* Return with blocking support */
}
