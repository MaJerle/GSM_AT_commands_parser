/**
 * Keil project example for GSM SIM800/900 for CALL and RTOS support
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
 * This examples shows how you can receive CALL and answer it by pressing a button.
 * It works in several ways:
 *
 * - If someone is calling us, button press will answer call
 * - If call is active, button press will hang up call
 * - If no call active, button press will start call to desired number, check example below.
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
#include "cmsis_os.h"

#define DEBUG_USART         USART2
#define DEBUG_USART_PP      TM_USART_PinsPack_1

/* GSM working structure and result enumeration */
gvol GSM_t GSM;
GSM_Result_t gsmRes;

/* Pointer to SMS info */
GSM_CallInfo_t* CallInfo = NULL;

/* GSM pin code */
#define GSM_PIN      "1234"

/* Set number to call us */
#define CALL_NUMBER  "your_number"

/* Thread prototypes */
void GSM_Update_Thread(void const* params);
void GSM_CALL_Thread(void const* params);
void GSM_SMS_Thread(void const* params);

/* Thread definitions */
osThreadDef(GSM_Update, GSM_Update_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
osThreadDef(GSM_CALL, GSM_CALL_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
osThreadDef(GSM_SMS, GSM_SMS_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);

osThreadId GSM_Update_ThreadId, GSM_CALL_ThreadId, GSM_SMS_ThreadId;

/* GSM callback declaration */
int GSM_Callback(GSM_Event_t evt, GSM_EventParams_t* params);

int main(void) {    
    TM_RCC_InitSystem();                                    /* Init system */
    HAL_Init();                                             /* Init HAL layer */
    TM_DISCO_LedInit();                                     /* Init leds */
    TM_DISCO_ButtonInit();                                  /* Init button */
    TM_DELAY_Init();                                        /* Init delay */
    TM_USART_Init(DEBUG_USART, DEBUG_USART_PP, 921600);     /* Init USART for debug purpose */

    /* Print first screen message */
    printf("GSM commands parser; Compiled: %s %s \r\n", __DATE__, __TIME__);

    /* Initialize threads */
    GSM_Update_ThreadId = osThreadCreate(osThread(GSM_Update), NULL);
    GSM_CALL_ThreadId = osThreadCreate(osThread(GSM_CALL), NULL);

    /* Start kernel */
    osKernelStart();
    
	while (1) {

	}
}

/* 1ms handler */
void TM_DELAY_1msHandler() {
    GSM_UpdateTime(&GSM, 1);                /* Update GSM library time for 1 ms */
    osSystickHandler();                     /* Kernel systick handler processing */
}

/***********************************************/
/**            Thread implementations         **/
/***********************************************/

/**
 * \brief  Update GSM received data thread
 */
void GSM_Update_Thread(void const* params) {
    while (1) {
        /* Process GSM update */
        GSM_Update(&GSM);
    }
}

/**
 * \brief  Application thread to work with CALL
 */
void GSM_CALL_Thread(void const* params) {
    /* Init GSM library with PIN code */
    printf("GSM Init status: %d\r\n", GSM_Init(&GSM, GSM_PIN, 115200, GSM_Callback));
    
    /* Create SMS thread */
    GSM_SMS_ThreadId = osThreadCreate(osThread(GSM_SMS), NULL);
    
    while (1) {
        /* Process callback checks */
        GSM_ProcessCallbacks(&GSM);
        
        /* If button is pressed */
        if (TM_DISCO_ButtonOnPressed()) {
            /* Get call info */
            CallInfo = GSM_CALL_GetInfo(&GSM, 1);
            
            /* No call active */
            if (CallInfo->State == GSM_CallState_Disconnect) {
                printf("Call disconnected, trying to call someone!\r\n");
                /* Try to call someone */
                if ((gsmRes = GSM_CALL_Voice(&GSM, CALL_NUMBER, 1)) == gsmOK) {
                    printf("Calling!\r\n");
                } else {
                    printf("Error trying to call: %d\r\n", gsmRes);
                }
            } else if (CallInfo->State == GSM_CallState_Incoming) {
                /* Someone is calling us, answer it */
                if ((gsmRes = GSM_CALL_Answer(&GSM, 1)) == gsmOK) {
                    printf("Answered OK\r\n");
                } else {
                    printf("Error trying to answer: %d\r\n", gsmRes);
                }
            } else if (CallInfo->State == GSM_CallState_Active) {
                /* If call is active, hang up */
                if ((gsmRes = GSM_CALL_HangUp(&GSM, 1)) == gsmOK) {
                    printf("HangedUp OK\r\n");
                } else {
                    printf("Error trying to hang up: %d\r\n", gsmRes);
                }
            }
        }
    }
}

/**
 * \brief  Application thread to work with SMS
 */
void GSM_SMS_Thread(void const* params) {
    GSM_SmsInfo_t* SMS_Info;
    GSM_SMS_Entry_t SMS_Entry;
    
    while (1) {        
        /* We have received SMS messages? */
        while ((SMS_Info = GSM_SMS_GetReceivedInfo(&GSM, 1)) != NULL) {
            /* Read SMS from memory */
            if ((gsmRes = GSM_SMS_Read(&GSM, SMS_Info->Position, &SMS_Entry, 1)) == gsmOK) {
                printf("SMS READ OK!\r\n");
                
                /* Make actions according to received SMS string */
                if (strcmp(SMS_Entry.Data, "LED ON") == 0) {
                    TM_DISCO_LedOn(LED_ALL);
                    gsmRes = GSM_SMS_Send(&GSM, SMS_Entry.Number, "OK", 1);
                } else if (strcmp(SMS_Entry.Data, "LED OFF") == 0) {
                    TM_DISCO_LedOff(LED_ALL);
                    gsmRes = GSM_SMS_Send(&GSM, SMS_Entry.Number, "OK", 1);
                } else if (strcmp(SMS_Entry.Data, "LED TOGGLE") == 0) {
                    TM_DISCO_LedToggle(LED_ALL);
                    gsmRes = GSM_SMS_Send(&GSM, SMS_Entry.Number, "OK", 1);
                } else {
                    gsmRes = GSM_SMS_Send(&GSM, SMS_Entry.Number, "ERROR", 1);
                }
                
                /* Send it back to user */
                if (gsmRes == gsmOK) {
                    printf("SEND BACK OK!\r\n");
                } else {
                    printf("Error trying to send: %d\r\n", gsmRes);
                }
            } else {
                printf("Error trying to read: %d\r\n", gsmRes);
            }
            
            /* Clear information about new SMS */
            GSM_SMS_ClearReceivedInfo(&GSM, SMS_Info, 1);
        }
    }
}

/***********************************************/
/**               Library callback            **/
/***********************************************/
int GSM_Callback(GSM_Event_t evt, GSM_EventParams_t* params) {
    switch (evt) {                              /* Check events */
        case gsmEventIdle:
            printf("Stack is IDLE!\r\n");
            break;
        case gsmEventCallCLCC: {                  /* We received event because of call change status */
            /* When call status is changed, notification is send immediatelly to this function */
            GSM_CallInfo_t* CallInfo = GSM_CALL_GetInfo(&GSM, 1);
        
            /* Call just went active? */
            if (CallInfo->State == GSM_CallState_Active) {
                if (CallInfo->Dir == GSM_CallDir_MO) {
                    printf("Someone has answered our call!\r\n");
                } else {
                    printf("We have answered incoming call!\r\n");
                }
            }
            /* Call just finished? */
            if (CallInfo->State == GSM_CallState_Disconnect) {
                printf("Call finished!\r\n");
            }
            break;
        }
        case gsmEventCallRING:                  
            /* Ring was received here */
            printf("RINGING!\r\n");
            break;
        case gsmEventUVPowerDown:
            printf("Powerdown\r\n");
            break;
        case gsmEventUVWarning:
            printf("Warning\r\n");
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
