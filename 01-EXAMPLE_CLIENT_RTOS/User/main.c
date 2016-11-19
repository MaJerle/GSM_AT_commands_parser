/**
 * Keil project example for GSM SIM800/900 for CLIENT and RTOS support
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
 * This examples shows how you can make client TCP/UDP request to server and communicate with it
 *
 * When button is pressed, module will try to connect to GPRS, connect to server,
 * send some request (in our case GET request over TCP) and wait for response within 5 seconds timeout.
 *
 * \par Pinout for example (Nucleo STM32F411)
 *
\verbatim
GSM         STM32F4xx           DESCRIPTION
 
RX          PA9                 TX from STM to RX from GSM
TX          PA10                RX from STM to TX from GSM
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

/* GSM pin code */
#define GSM_PIN         "1234"
/* GSM APN settings */
#define GSM_APN         "internet"
#define GSM_APN_USER    ""
#define GSM_APN_PASS    ""

/* Domain or IP for opening connection */
#define GSM_DOMAIN      "stm32f4-discovery.net"
#define GSM_PORT        80

/* Connection structure */
GSM_CONN_t Conn;

/* Array to receive data */
uint8_t receive[1000];
uint32_t br;

/* Data to send once we are connected */
uint8_t send[] = ""
"GET /gsm_example.php HTTP/1.1\r\n"
"Host: stm32f4-discovery.net\r\n"
"Connection: close\r\n"
"\r\n";
uint32_t bw;

/* Thread prototypes */
void GSM_Update_Thread(void const* params);
void GSM_Main_Thread(void const* params);

/* Thread definitions */
osThreadDef(GSM_Update, GSM_Update_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
osThreadDef(GSM_Main, GSM_Main_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);

osThreadId GSM_Update_ThreadId, GSM_Main_ThreadId;

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
    GSM_Main_ThreadId = osThreadCreate(osThread(GSM_Main), NULL);

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
 * \brief  Application thread to work with GSM module only
 */
void GSM_Main_Thread(void const* params) {
    uint8_t action = 0;
    
    /* Init GSM library with PIN code */
    printf("GSM Init status: %d\r\n", GSM_Init(&GSM, GSM_PIN, 115200, GSM_Callback));
    
    while (1) {
        /* Process callback checks */
        GSM_ProcessCallbacks(&GSM);
        
        /* If button is pressed */
        if (TM_DISCO_ButtonOnPressed()) {
            
            /* Try to connect to network */
            if ((gsmRes = GSM_GPRS_Attach(&GSM, GSM_APN, GSM_APN_USER, GSM_APN_PASS, 1)) == gsmOK) {
                printf("GPRS Attached\r\n");
                
                /* Try to connect to server */
                if ((gsmRes = GSM_CONN_Start(&GSM, &Conn, GSM_CONN_Type_TCP, GSM_DOMAIN, GSM_PORT, 1)) == gsmOK) {
                    printf("Connected to server!\r\n");
                    
                    /* We are connected, send data */
                    if ((gsmRes = GSM_CONN_Send(&GSM, &Conn, send, sizeof(send), &bw, 1)) == gsmOK) {
                        volatile uint32_t timeout = GSM.Time;
                        printf("Data sent to server! Number of bytes sent: %d\r\n", bw);
                        
                        do {
                            /* If we have data available to read */
                            if (GSM_CONN_DataAvailable(&GSM, &Conn, 1)) {
                                if ((GSM_CONN_Receive(&GSM, &Conn, receive, sizeof(receive), &br, 10, 1)) == gsmOK) {
                                    printf("Received %d bytes of data\r\n", br);
                                    
                                    /* Reset timeout */
                                    timeout = GSM.Time;
                                } else {
                                    printf("Error trying to receive data: %d\r\n", gsmRes);
                                }
                            }
                        } while (GSM.Time - timeout < 2000);
                        
                        /* Close connection */
                        if ((gsmRes = GSM_CONN_Close(&GSM, &Conn, 1)) == gsmOK) {
                            printf("Connection closed!\r\n");
                        } else {
                            printf("Error trying to close connection: %d\r\n", gsmRes);
                        }
                    } else {
                        printf("Error trying to connect to server: %d\r\n", gsmRes);
                    }
                } else {
                    printf("Could not execute request to server: %d\r\n", gsmRes);
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
/**               Library callback            **/
/***********************************************/
int GSM_Callback(GSM_Event_t evt, GSM_EventParams_t* params) {
    switch (evt) {                              /* Check events */
        case gsmEventIdle:
            printf("Stack is IDLE!\r\n");
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
