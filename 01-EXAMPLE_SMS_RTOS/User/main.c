/**
 * Keil project example for GSM SIM800/900 for SMS and RTOS support
 *
 * @note      Check defines.h file for configuration settings!
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

gvol GSM_t GSM;
GSM_Result_t gsmRes;

char SMS_Data[200];
GSM_SMS_Entry_t SMS_Entry;

GSM_SmsInfo_t* SMS_Info = NULL;

//Thread prototypes
void GSM_Update_Thread(void const* params);
void GSM_Main_Thread(void const* params);

/* Thread definitions */
osThreadDef(GSM_Update, GSM_Update_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
osThreadDef(GSM_Main, GSM_Main_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);

osThreadId GSM_Update_ThreadId, GSM_Main_ThreadId;

int main(void) {
	TM_RCC_InitSystem();                                    /* Init system */
	HAL_Init();                                             /* Init HAL layer */
	TM_DISCO_LedInit();                                     /* Init leds */
	TM_DISCO_ButtonInit();                                  /* Init button */
	TM_DELAY_Init();                                        /* Init delay */
    TM_USART_Init(DEBUG_USART, DEBUG_USART_PP, 921600);     /* Init USART */
    
    /* Print first screen message */
    printf("GSM commands parser; Compiled: %s %s \r\n", __DATE__, __TIME__);
    
    /* Initialize threads */
    GSM_Update_ThreadId = osThreadCreate(osThread(GSM_Update), NULL);
    GSM_Main_ThreadId = osThreadCreate(osThread(GSM_Main), NULL);
    
    /* Start kernel */
    osKernelStart();
    
	while (1) {

	}
}

/* 1ms handler */
void TM_DELAY_1msHandler() {
	GSM_UpdateTime(&GSM, 1);                /* Update ESP8266 library time for 1 ms */
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
 * \brief  Application thread to work with GSM module only
 */
void GSM_Main_Thread(void const* params) {
    /* Init GSM library */
    printf("GSM Init status: %d\r\n", GSM_Init(&GSM, "1234", 115200));
    
    while (1) {
        /* Process callback checks */
        GSM_ProcessCallbacks(&GSM);
        
        /* We have received SMS messages? */
        while ((SMS_Info = GSM_SMS_GetReceivedInfo(&GSM, 1)) != NULL) {
            /* Read SMS from memory */
            if ((gsmRes = GSM_SMS_Read(&GSM, &SMS_Entry, SMS_Info->Position, 1)) == gsmOK) {
                printf("SMS READ OK!\r\n");
                
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
        
        /* If button is pressed */
        if (TM_DISCO_ButtonOnPressed()) {
            /* Delete all SMS messages */
            if ((gsmRes = GSM_SMS_MassDelete(&GSM, GSM_SMS_MassDelete_All, 1)) == gsmOK) {
                printf("SMS MASS DELETE OK\r\n");
            } else {
                printf("Error trying to mass delete: %d\r\n", gsmRes);
            }
        }
    }
}

/***********************************************/
/**               Library callbacks           **/
/***********************************************/
/* Called when new SMS info is received */
void GSM_Callback_SMS_Info(gvol GSM_t* GSM) {
    /* When SMS is received, notification is send immediatelly to this function */
}

/* printf handler */
int fputc(int ch, FILE* fil) {
	TM_USART_Putc(DEBUG_USART, ch);         /* Send over debug USART */
	return ch;                              /* Return OK */
}
