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
#include "gsm_ll.h"
#include "tm_stm32_usart.h"

uint8_t GSM_LL_Init(GSM_LL_t* LL) {
    /* Init USART */
    TM_USART_Init(USART1, TM_USART_PinsPack_1, LL->Baudrate);
        
    /* We were successful */
    return 0;
}

uint8_t GSM_LL_SendData(GSM_LL_t* LL, const uint8_t* data, uint16_t count) {
    /* Send data */
    TM_USART_Send(USART1, (uint8_t *)data, count);
    
    /* We were successful */
    return 0;
}

/**
 * \brief  USART1 receive handler method
 * \param  Received character on UART
 */
void TM_USART1_ReceiveHandler(uint8_t ch) {
    GSM_DataReceived(&ch, 1);               /* Call data received */
}
