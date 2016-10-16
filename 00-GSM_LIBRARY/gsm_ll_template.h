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
#define GSM_LL_H 100

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

/* Include core libraries */
#include "stdint.h"
#include "stdlib.h"


/**
 * \defgroup GSM_LL
 * \brief    GSM Low-Level implementation
 * \{
 */
    
/**
 * \defgroup GSM_LL_Typedefs
 * \brief    GSM Low-Level
 * \{
 */

/**
 * \brief  Low level structure for driver
 * \note   For now it has basic settings only without hardware flow control.
 */
typedef struct _GSM_LL_t {
    uint32_t Baudrate;			/*!< Baudrate to be used for UART */
} GSM_LL_t;
    
/**
 * \}
 */
    
/* Include library */
#include "gsm.h"
    
/**
 * \defgroup GSM_LL_Macros
 * \brief    GSM Low-Level macros
 * \{
 */

#define GSM_RTS_HIGH        1   /*!< RTS should be set high */
#define GSM_RTS_CLEAR       0   /*!< RTS should be set low */
    
/**
 * \}
 */

/**
 * \defgroup GSM_LL_Functions
 * \brief    GSM Low-Level implementation
 * \{
 */

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
 * \param  *LL: Pointer to \ref GSM_LL_t structure with settings
 * \param  *data: Data to be sent to module
 * \param  count: Number of bytes to be sent to module
 * \retval Success status:
 *            - 0: Successful
 *            - > 0: Error
 */
uint8_t GSM_LL_SendData(GSM_LL_t* LL, const uint8_t* data, uint16_t count);

/**
 * \brief  Initializes Low-Level driver to communicate with SIM module
 * \param  *LL: Pointer to \ref GSM_LL_t structure with settings
 * \retval Success status:
 *            - 0: Successful
 *            - > 0: Error
 */
uint8_t GSM_LL_SetRTS(GSM_LL_t* LL, uint8_t state);
    
/**
 * \}
 */

/**
 * \}
 */

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
