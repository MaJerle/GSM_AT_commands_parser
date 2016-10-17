/**
 * Keil project example for GSM SIM800/900 for HTTP and RTOS support
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
 * This examples shows how you can make HTTP request to server and get HTTP response from it
 *
 * When button is pressed, 2 actions will be executed.
 * Actions will toggle between button presses:
 *
 * - GET method to "http://stm32f4-discovery.net/gsm_example.php"
 * - POST method to "http://stm32f4-discovery.net/gsm_example.php" with data sent to server
 *
 * In both cases, responses will be returned and readed with stack.
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

/* SMS read structure */
GSM_SMS_Entry_t SMS_Entry;

/* Pointer to SMS info */
GSM_SmsInfo_t* SMS_Info = NULL;

/* GSM pin code */
#define GSM_PIN         "1234"
/* GSM APN settings */
#define GSM_APN         "internet"
#define GSM_APN_USER    ""
#define GSM_APN_PASS    ""

/* Request URL */
#define GSM_HTTP_URL    "http://stm32f4-discovery.net/gsm_example.php"

/* Array to receive data */
uint8_t receive[1000];
uint32_t br;

/* Array with data to send */
uint8_t send[] = "Hello from GSM module! The same as I sent you I just get back!";

/* Thread prototypes */
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
    TM_USART_Init(DEBUG_USART, DEBUG_USART_PP, 921600);     /* Init USART for debug purpose */

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
    uint8_t action = 0;
    
    /* Init GSM library with PIN code */
    printf("GSM Init status: %d\r\n", GSM_Init(&GSM, GSM_PIN, 115200));
    
    while (1) {
        /* Process callback checks */
        GSM_ProcessCallbacks(&GSM);
        
        /* If button is pressed */
        if (TM_DISCO_ButtonOnPressed()) {
            
            /* Try to connect to network */
            if ((gsmRes = GSM_GPRS_Attach(&GSM, GSM_APN, GSM_APN_USER, GSM_APN_PASS, 1)) == gsmOK) {
                printf("GPRS Attached\r\n");
                
                /* We are connected, now begin HTTP */
                if ((gsmRes = GSM_HTTP_Begin(&GSM, 1)) == gsmOK) {
                    printf("HTTP Begin OK\r\n");
                    
                    /* Select job according to selected action */
                    if (action) {
                        /* GET REQUEST */
                        /* Make actual HTTP request */
                        if ((gsmRes = GSM_HTTP_Execute(&GSM, GSM_HTTP_URL, GSM_HTTP_Method_GET, 1)) == gsmOK) {
                            /* HTTP request executed to server, wait for response data */
                            while (GSM_HTTP_DataAvailable(&GSM, 1)) {
                                /* Read as many bytes as possible */
                                if ((gsmRes = GSM_HTTP_Read(&GSM, receive, sizeof(receive), &br, 1)) == gsmOK) {
                                    printf("Successfully read %d/%d bytes of data\r\n", br, sizeof(receive));
                                } else {
                                    printf("Error trying to read %d bytes of data: %d\r\n", sizeof(receive), gsmRes);
                                }
                            }
                        } else {
                            printf("Could not execute request to server: %d\r\n", gsmRes);
                        }
                    } else {
                        /* POST REQUEST with data */
                        
                        /* Set data to send first */
                        if ((gsmRes = GSM_HTTP_SetData(&GSM, send, sizeof(send), 1)) == gsmOK) {
                            /* Make actual HTTP request */
                            if ((gsmRes = GSM_HTTP_Execute(&GSM, GSM_HTTP_URL, GSM_HTTP_Method_POST, 1)) == gsmOK) {
                                /* HTTP request executed to server, wait for response data */
                                while (GSM_HTTP_DataAvailable(&GSM, 1)) {
                                    /* Read as many bytes as possible */
                                    /* We are expecting only one read here as we are expecting less data than our array can hold! */
                                    if ((gsmRes = GSM_HTTP_Read(&GSM, receive, sizeof(receive), &br, 1)) == gsmOK) {
                                        printf("Successfully read %d/%d bytes of data\r\n", br, sizeof(receive));
                                    
                                        /* Check if data are the same */
                                        if (strncmp((void *)receive, (void *)send, sizeof(send)) == 0) {
                                            printf("Received and sent data are the same!\r\n");
                                        } else {
                                            printf("Received is not the same as sent!\r\n");
                                        }
                                    } else {
                                        printf("Error trying to read %d bytes of data: %d\r\n", sizeof(receive), gsmRes);
                                    }
                                }
                            } else {
                                printf("Could not execute request to server: %d\r\n", gsmRes);
                            }
                        } else {
                            printf("Error trying to set POST data: %d\r\n", gsmRes);
                        }
                    }
                    
                    /* End HTTP */
                    GSM_HTTP_End(&GSM, 1);
                } else {
                    printf("Problems trying to begin HTTP: %d\r\n", gsmRes);
                }
                
                /* Detach from GPRS */
                if ((gsmRes = GSM_GPRS_Detach(&GSM, 1)) == gsmOK) {
                    printf("GPRS Detached\r\n");
                } else {
                    printf("Problems trying to detach GPRS: %d\r\n", gsmRes);
                }
            } else {
                printf("Could not attach to GPRS: %d\r\n", gsmRes);
            }
            
            /* Toggle action */
            action = !action;
        }
    }
}

/***********************************************/
/**               Library callbacks           **/
/***********************************************/

/* printf handler */
int fputc(int ch, FILE* fil) {
    TM_USART_Putc(DEBUG_USART, ch);         /* Send over debug USART */
    return ch;                              /* Return OK */
}
