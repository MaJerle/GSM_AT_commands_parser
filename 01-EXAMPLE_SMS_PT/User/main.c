/**
 * Keil project example for GSM SIM800/900 for SMS without RTOS support
 *
 * @note      Check defines.h file for configuration settings!
 * @note      When using Nucleo F411 board, example has set 8MHz external HSE clock!
 *
 * Before you start, select your target, on the right of the "Load" button
 *
 * @author    Tilen Majerle
 * @email     tilen@majerle.eu
 * @website   http://stm32f4-discovery.net
 * @ide       Keil uVision 5
 * @conf      PLL parameters are set in "Options for Target" -> "C/C++" -> "Defines"
 * @packs     STM32F4xx/STM32F7xx Keil packs are requred with HAL driver support
 * @stdperiph STM32F4xx/STM32F7xx HAL drivers required
 *
 * \par Description
 *
 * This examples shows how you can receive SMS and read it.
 * If you send SMS with specific content, it will do actions with LED on board:
 *
 * - LED ON: led will turn on,
 * - LED OFF: Led will turn off,
 * - LED TOGGLE: Led will toggle.
 *
 * After that, SMS with "OK" or "ERROR" should be returned to received number to confirm action
 *
 * Everything is done using protothreads, which are "fake" RTOS system.
 * For more infomations on how protothreads work, you should check Adam Dunkels's website.
 *
 * \par Pinout for example (Nucleo STM32F411)
 *
\verbatim
GSM         STM32F4xx           DESCRIPTION
 
RX          PA9                 TX from STM to RX from GSM
TX          PA10                RX from STM
VCC         3.3V                Use external 3.3V regulator
GND         GND
RST         PA0
CTS         PA3                 RTS from ST to CTS from GSM
            BUTTON(PA0, PC13)   Discovery/Nucleo button, depends on configuration
            
            PA2                 TX for debug purpose (connect to PC) with 921600 bauds
\endverbatim
 */
/* Include core modules */
#include "stm32fxxx_hal.h"
/* Include my libraries here */
#include "defines.h"
#include "tm_stm32_disco.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_usart.h"
#include "gsm.h"

#define DEBUG_USART         USART2
#define DEBUG_USART_PP      TM_USART_PinsPack_1

/* GSM working structure and result enumeration */
gvol GSM_t GSM;
GSM_Result_t gsmRes;

/* SMS read structure */
GSM_SMS_Entry_t SMS_Entry;

/* Pointer to SMS info */
GSM_SmsInfo_t* SMS_Info = NULL;

/* GSM pin code */
#define GSM_PIN      "1234"

/* GSM callback declaration */
int GSM_Callback(GSM_Event_t evt, GSM_EventParams_t* params);

/* Protothread */
struct pt PT;

/* SMS send protothread system */
static
PT_THREAD(SMS_PTHREAD(struct pt* pt)) {
    PT_BEGIN(pt);                                           /* Begin with protothread */
    
    if (!SMS_Info) {                                        /* Check valid SMS received info structure */
        PT_EXIT(pt);
    }
    
    PT_WAIT_UNTIL(pt, GSM_IsReady(&GSM) == gsmOK);          /* Wait stack to be ready */
    
    /* Try to read, should be non-blocking in non-RTOS environment */
    if ((gsmRes = GSM_SMS_Read(&GSM, SMS_Info->Position, &SMS_Entry, 0)) == gsmOK) {
        printf("SMS reading has begun! Waiting for response!\r\n");
        
        PT_WAIT_UNTIL(pt, GSM_IsReady(&GSM) == gsmOK);      /* Wait stack to be ready */
        gsmRes = GSM_GetLastReturnStatus(&GSM);             /* Get response status from non-blocking call */
        
        if (gsmRes == gsmOK) {                              /* Check if SMS was actually full read */
            printf("SMS read!\r\n");
            
            /* Make actions according to received SMS string */
            if (strcmp(SMS_Entry.Data, "LED ON") == 0) {
                TM_DISCO_LedOn(LED_ALL);
                gsmRes = GSM_SMS_Send(&GSM, SMS_Entry.Number, "OK", 0);
            } else if (strcmp(SMS_Entry.Data, "LED OFF") == 0) {
                TM_DISCO_LedOff(LED_ALL);
                gsmRes = GSM_SMS_Send(&GSM, SMS_Entry.Number, "OK", 0);
            } else if (strcmp(SMS_Entry.Data, "LED TOGGLE") == 0) {
                TM_DISCO_LedToggle(LED_ALL);
                gsmRes = GSM_SMS_Send(&GSM, SMS_Entry.Number, "OK", 0);
            } else {
                gsmRes = GSM_SMS_Send(&GSM, SMS_Entry.Number, "ERROR", 0);
            }
            
            /* Send it back to user */
            if (gsmRes == gsmOK) {
                printf("Send back started ok!\r\n");
                
                /* Wait till SMS full sent! */
                PT_WAIT_UNTIL(pt, GSM_IsReady(&GSM) == gsmOK);  /* Wait stack to be ready */
                gsmRes = GSM_GetLastReturnStatus(&GSM);     /* Check actual send status from non-blocking call */
                
                if (gsmRes == gsmOK) {
                    printf("SMS has been successfully sent!\r\n");
                } else {
                    printf("Error trying to send SMS: %d\r\n", gsmRes);
                }
            } else {
                printf("Send back start error: %d\r\n", gsmRes);
            }
        }
    } else {
        printf("SMS reading process failed: %d\r\n", gsmRes);
    }
    
    GSM_SMS_ClearReceivedInfo(&GSM, SMS_Info, 0);           /* Check SMS information slot */
    PT_WAIT_UNTIL(pt, GSM_IsReady(&GSM) == gsmOK);          /* Wait stack to be ready */
    
    SMS_Info = NULL;                                        /* Reset received SMS structure */
    PT_END(pt);                                             /* End protothread */
}

int main(void) {    
    TM_RCC_InitSystem();                                    /* Init system */
    HAL_Init();                                             /* Init HAL layer */
    TM_DISCO_LedInit();                                     /* Init leds */
    TM_DISCO_ButtonInit();                                  /* Init button */
    TM_DELAY_Init();                                        /* Init delay */
    TM_USART_Init(DEBUG_USART, DEBUG_USART_PP, 921600);     /* Init USART for debug purpose */

    /* Print first screen message */
    printf("GSM commands parser; Compiled: %s %s \r\n", __DATE__, __TIME__);
    
    /* Proto thread initialization */
    PT_INIT(&PT);                                           /* Initialize SMS protothread */
    
    /* Init GSM */
    printf("GSM Init status: %d\r\n", GSM_Init(&GSM, GSM_PIN, 115200, GSM_Callback));
    
    while (1) {
        GSM_Update(&GSM);                                   /* Process GSM update */
        
        if (SMS_Info) {                                     /* Anything works? */
            SMS_PTHREAD(&PT);                               /* Call protothread function */
        } else {
            /* Check if any SMS received */
            if ((SMS_Info = GSM_SMS_GetReceivedInfo(&GSM, 1)) != NULL) {
                printf("SMS Received, start SMS processing\r\n");
            }
        }
        
        /* If button is pressed */
        if (TM_DISCO_ButtonOnPressed()) {
            /* Delete all SMS messages, process with blocking call */
            if ((gsmRes = GSM_SMS_MassDelete(&GSM, GSM_SMS_MassDelete_All, 1)) == gsmOK) {
                printf("SMS MASS DELETE OK\r\n");
            } else {
                printf("Error trying to mass delete: %d\r\n", gsmRes);
            }
        }
    }
}

/* 1ms handler */
void TM_DELAY_1msHandler() {
    GSM_UpdateTime(&GSM, 1);                                /* Update GSM library time for 1 ms */
}

/***********************************************/
/**               Library callback            **/
/***********************************************/
int GSM_Callback(GSM_Event_t evt, GSM_EventParams_t* params) {
    switch (evt) {                              /* Check events */
        case gsmEventIdle:
            break;
        case gsmEventSMSCMTI:                   /* Information about new SMS received */
            break;
        default:
            break;
    }
    
    return 0;
}

/* printf handler */
int fputc(int ch, FILE* fil) {
    TM_USART_Putc(DEBUG_USART, ch);         /* Send over debug USART */
    return ch;                              /* Return OK */
}
