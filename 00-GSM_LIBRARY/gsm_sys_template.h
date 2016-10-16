/**
 * \author  Tilen Majerle
 * \email   tilen@majerle.eu
 * \website 
 * \license MIT
 * \brief   GSM System calls
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
#ifndef GSM_SYS_H
#define GSM_SYS_H 100

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

/* Include core libraries */
#include "stdint.h"
#include "stdlib.h"
    
/* Platform dependant */
//#include "cmsis_os.h"
    
/* Include library */
#include "gsm.h"
#include "gsm_config.h"

/**
 * \defgroup GSM_SYS
 * \brief    GSM Syscall implementation
 * \{
 *
 * System calls implmementation is required when working with RTOS mode.
 * 
 * In this case, mutex implementation is required to allow multiple threads communicate with GSM library.
 */
/**
 * \defgroup GSM_SYS_Functions
 * \brief    GSM Syscalls functions
 * \{
 */

/**
 * \brief  Creates a synchronization object
 * \param  *Sync: Pointer to sync object to create in system
 * \retval Successfull status:
 *            - 0: Successful
 *            - > 0: Error
 */
uint8_t GSM_SYS_Create(GSM_RTOS_SYNC_t* Sync);

/**
 * \brief  Deletes a synchronization object
 * \param  *Sync: Pointer to sync object to delete from system
 * \retval Successfull status:
 *            - 0: Successful
 *            - > 0: Error
 */
uint8_t GSM_SYS_Delete(GSM_RTOS_SYNC_t* Sync);

/**
 * \brief  Requests grant for specific sync object
 * \param  *Sync: Pointer to sync object to request grant
 * \retval Successfull status:
 *            - 0: Successful
 *            - > 0: Error
 */
uint8_t GSM_SYS_Request(GSM_RTOS_SYNC_t* Sync);

/**
 * \brief  Releases grant for specific sync object
 * \param  *Sync: Pointer to sync object to release grant
 * \retval Successfull status:
 *            - 0: Successful
 *            - > 0: Error
 */
uint8_t GSM_SYS_Release(GSM_RTOS_SYNC_t* Sync);
    
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
