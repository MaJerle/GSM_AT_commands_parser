/**
 * Keil project example for GSM SIM800/900 for PHONEBOOK and RTOS support
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
 * This examples shows how you can manipulate with SIM phonebook entries.
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

/* Phonebook entry structure */
GSM_PB_Entry_t Entry;

/* Array of entries */
GSM_PB_Entry_t entries[100];
uint16_t br;

/* GSM pin code */
#define GSM_PIN      "1234"

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
    uint8_t i = 0;
    
    /* Init GSM library with PIN code */
    printf("GSM Init status: %d\r\n", GSM_Init(&GSM, GSM_PIN, 115200, GSM_Callback));
    
    osDelay(5000);
    
    /* Add new entry to phonebook */
    if ((gsmRes = GSM_PB_Add(&GSM, "Name", "0123456789", 1)) == gsmOK) {
        printf("New entry added to phonebook!\r\n");
    }
    
    /* Find first PB entry from 0 to 100 */
    for (i = 0; i < 100; i++) {
        /* Read entry from location number in SIM memory */
        if ((gsmRes = GSM_PB_Get(&GSM, i, &Entry, 1)) == gsmOK) {
            printf("Entry on index %d has: Name = %s, Number: %s\r\n", Entry.Index, Entry.Name, Entry.Number);
            
            /* Edit number on entry, set new name and new number on previously read index */
            /* If there is no phone number on this index, new will be automatically created */
            if ((gsmRes = GSM_PB_Edit(&GSM, Entry.Index, "New name", "123321456", 1)) == gsmOK) {
                printf("Entry has been updated!\r\n");
            } else {
                printf("Error trying to update entry: %d\r\n", gsmRes);
            }
            
            /* Stop execution */
            break;
        } else {
            printf("Error trying to read entry: %d\r\n", gsmRes);
        }
    }
    
    /* List entries from desired start index */
    /* Read entries from start index 0, read as many bytes as possible to store in our array*/
    if ((gsmRes = GSM_PB_List(&GSM, entries, Entry.Index, sizeof(entries) / sizeof(entries[0]), &br, 1)) == gsmOK) {
        printf("Successfully read %d entries from memory\r\n", br);
    } else {
        printf("Error trying to read entries: %d\r\n", gsmRes);
    }
    
    /* Search entries from memory */
    if ((gsmRes = GSM_PB_Search(&GSM, "name", entries, sizeof(entries) / sizeof(entries[0]), &br, 1)) == gsmOK) {
        printf("Search result got %d entries from memory\r\n", br);
    } else {
        printf("Error trying to search entries: %d\r\n", gsmRes);
    }
    
    /* Delete index from first read location */
    if ((gsmRes = GSM_PB_Delete(&GSM, Entry.Index, 1)) == gsmOK) {
        printf("Entry on index %d has been deleted\r\n", Entry.Index);
    } else {
        printf("Error trying to delete entry: %d\r\n", gsmRes);
    }
    
    /* Do nothing */
    while (1) {
        /* Process callback checks */
        GSM_ProcessCallbacks(&GSM);
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
