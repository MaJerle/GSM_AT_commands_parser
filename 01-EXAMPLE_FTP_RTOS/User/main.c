/**
 * Keil project example for GSM SIM800/900 for FTP and RTOS support
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
 * When button is pressed, module will try to connect to GPRS, connect to FTP server
 * First, downloading procedure will be done with existing file on server
 * Second, new file will be uploaded to FTP with custom data.
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

/* Array to receive data */
uint8_t ftp_data[] = "Some data to upload\n"
"A lot of data to upload to FTP file\n"
"";
uint32_t br, bw;

/* Thread prototypes */
void GSM_Update_Thread(void const* params);
void GSM_Main_Thread(void const* params);

/* Thread definitions */
osThreadDef(GSM_Update, GSM_Update_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
osThreadDef(GSM_Main, GSM_Main_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);

osThreadId GSM_Update_ThreadId, GSM_Main_ThreadId;

/* Callback definitions */
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
    char name[50];
    
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
                
                /* Begin with FTP */
                if ((gsmRes = GSM_FTP_Begin(&GSM, GSM_FTP_Mode_Passive, GSM_FTP_SSL_Disable, 1)) == gsmOK) {
                    printf("FTP BEGIN OK!\r\n");
                    
                    /* Authenticate to FTP server */
                    /* Public FTP server is used for this example */
                    if ((gsmRes = GSM_FTP_Authenticate(&GSM, "ftp.swfwmd.state.fl.us", 21, "Anonymous", "example@example.com", 1)) == gsmOK) {
                        printf("Authentication OK\r\n");
                        
                        /*********************************************/
                        /**            Process FTP download         **/
                        /*********************************************/
                        /* Begin with downloading process */
                        if ((gsmRes = GSM_FTP_DownloadBegin(&GSM, "/pub/incoming", "/ReclaimedPipeline.dbf", 1)) == gsmOK) {
                            printf("DOWNLOAD BEGIN OK\r\n");
                            
                            /* Do it until downloading session is active */
                            while (GSM_FTP_DownloadActive(&GSM, 1) == gsmOK) {
                                /* If there are any bytes to read right now */
                                while (GSM_FTP_DownloadAvailable(&GSM, 1) == gsmOK) {
                                    /* Read FTP bytes from GSM device */
                                    if ((gsmRes = GSM_FTP_Download(&GSM, ftp_data, sizeof(ftp_data), &br, 1)) == gsmOK) {
                                        printf("Data read. Bytes read: %d\r\n", br);
                                    } else {
                                        printf("Error trying to read data: %d\r\n", gsmRes);
                                    }
                                }
                            }
                            
                            /* Reading has finished */
                            printf("Reading finished!\r\n");
                        } else {
                            printf("Error FTP download begin: %d\r\n", gsmRes);
                        }
                        
                        /*********************************************/
                        /**            Process FTP upload           **/
                        /*********************************************/
                        sprintf(name, "/_SIM_test_%08d.txt", GSM.Time);
                        
                        /* Begin FTP session */
                        if ((gsmRes = GSM_FTP_UploadBegin(&GSM, "/pub/incoming", name, GSM_FTP_UploadMode_StoreUnique, 1)) == gsmOK) {
                            /* Upload data over FTP */
                            if ((gsmRes = GSM_FTP_Upload(&GSM, ftp_data, sizeof(ftp_data), &bw, 1)) == gsmOK) {
                                printf("Data written over FTP. Number of bytes: %d\r\n", bw);
                            } else {
                                printf("ERROR: %d: Bytes written: %d\r\n", gsmRes, bw);
                            }
                            
                            /* Finish uploading session over FTP */
                            if ((gsmRes = GSM_FTP_UploadEnd(&GSM, 1)) == gsmOK) {
                                printf("Upload finished\r\n");
                            } else {
                                printf("Error finishing upload session: %d;\r\n", gsmRes);
                            }
                        } else {
                            printf("Error FTP upload begin: %d\r\n", gsmRes);
                        }
                    } else {
                        printf("Error trying to authenticate to server: %d\r\n", gsmRes);
                    }
                    
                    /* Finish FTP session */
                    if ((gsmRes = GSM_FTP_End(&GSM, 1)) == gsmOK) {
                        printf("FTP session finished\r\n");
                    } else {
                        printf("Error trying to end FTP session: %d\r\n", gsmRes);
                    }
                } else {
                    printf("Error trying to begin with FTP: %d\r\n", gsmRes);
                }
            } else {
                printf("Could not attach to GPRS: %d\r\n", gsmRes);
            }
        }
    }
}

/***********************************************/
/**               Library callbacks           **/
/***********************************************/
int GSM_Callback(GSM_Event_t evt, GSM_EventParams_t* params) {    
    switch (evt) {                              /* Check events */
        case gsmEventIdle:
            break;
        case gsmEventGPRSAttached:
            break;
        case gsmEventGPRSDetached:
            break;
        case gsmEventUVWarning:
            printf("Under voltage warning!\r\n");
            break;
        case gsmEventUVPowerDown:
            printf("Under voltage power down!\r\n");
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
