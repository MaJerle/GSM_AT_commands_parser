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
#include "gsm_ll_template.h"

/******************************************************************************/
/******************************************************************************/
/***      Copy this file to project directory and rename it to "gsm_ll.c"    **/
/******************************************************************************/
/******************************************************************************/

uint8_t GSM_LL_Init(GSM_LL_t* LL) {
	/* Init UART */

	/* Init reset pin */

	return 0;
}


uint8_t GSM_LL_SendData(GSM_LL_t* LL, const uint8_t* data, uint16_t count) {
	/* Send data via UART */

	return 0;
}


uint8_t GSM_LL_SetReset(GSM_LL_t* LL, uint8_t state) {
	/* Set reset pin */
	if (state == GSM_RESET_LOW) {
		/* Set pin low */
	} else {
		/* Set pin high */
	}

	return 0;
}

uint8_t GSM_LL_SetRTS(GSM_LL_t* LL, uint8_t state) {
	if (state == GSM_RTS_LOW) {
		/* Set pin low */
	} else {
		/* Set pin high */
	}

	return 0;
}
