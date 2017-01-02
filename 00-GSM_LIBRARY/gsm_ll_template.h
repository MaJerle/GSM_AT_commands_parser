/**
 * \author  Tilen Majerle
 * \email   tilen@majerle.eu
 * \website 
 * \license MIT
 * \brief   GSM Low-Level
 *	
\verbatim
   ----------------------------------------------------------------------
    Copyright (c) 2016 Tilen Majerle

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software, 
    and to permit persons to whom the Software is furnished to do so, 
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
    AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------
\endverbatim
 */
#ifndef GSM_LL_H
#define GSM_LL_H 010

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

/* Include core libraries */
#include "stdint.h"
#include "stdlib.h"

/******************************************************************************/
/******************************************************************************/
/***      Copy this file to project directory and rename it to "gsm_ll.h"    **/
/******************************************************************************/
/******************************************************************************/

/**
 * \defgroup LOWLEVEL
 * \brief    GSM Low-Level implementation
 * \{
 */

/**
 * \brief  Low level structure for driver
 * \note   For now it has basic settings only without hardware flow control.
 */
typedef struct _GSM_LL_t {
    uint32_t Baudrate;          /*!< Baudrate to be used for UART */
} GSM_LL_t;
    
/* Include library */
#include "gsm.h"

#define GSM_RTS_SET         1   /*!< RTS should be set high */
#define GSM_RTS_CLR         0   /*!< RTS should be set low */
#define GSM_RESET_SET       1   /*!< Reset pin should be set */
#define GSM_RESET_CLR       0   /*!< Reset pin should be cleared */
    
/**
 * \brief  Initializes Low-Level driver to communicate with SIM module
 * \param  *LL: Pointer to \ref GSM_LL_t structure with settings
 * \retval Success status:
 *            - 0: Successful
 *            - > 0: Error
 */
uint8_t GSM_LL_Init(GSM_LL_t* LL);

/**
 * \brief  Sends data to SIM module from GSM stack
 * \note   Send can be implemented using DMA or IRQ easily,
 *            without waiting for finish.
 *            However, when multiple calls are executed,
 *            you must take care to send previous data first.
 *            Using DMA or IRQ, you can use cyclic buffers for implementation.
 *
 * \param  *LL: Pointer to \ref GSM_LL_t structure with settings
 * \param  *data: Data to be sent to module
 * \param  count: Number of bytes to be sent to module
 * \retval Success status:
 *            - 0: Successful
 *            - > 0: Error
 */
uint8_t GSM_LL_SendData(GSM_LL_t* LL, const uint8_t* data, uint16_t count);

/**
 * \brief  Set reset pin high or low
 * \param  *LL: Pointer to \ref GSM_LL_t structure with settings
 * \param  state: State for reset pin, it can be high or low. Check \ref GSM_RESET_SET and \ref GSM_RESET_CLR
 * \retval Success status:
 *            - 0: Successful
 *            - > 0: Error
 */
uint8_t GSM_LL_SetReset(GSM_LL_t* LL, uint8_t state);

/**
 * \brief  Initializes Low-Level driver to communicate with SIM module
 * \param  *LL: Pointer to \ref GSM_LL_t structure with settings
 * \param  state: State for reset pin, it can be high or low. Check \ref GSM_RTS_SET and \ref GSM_RTS_CLR
 * \retval Success status:
 *            - 0: Successful
 *            - > 0: Error
 */
uint8_t GSM_LL_SetRTS(GSM_LL_t* LL, uint8_t state);

/**
 * \}
 */

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
