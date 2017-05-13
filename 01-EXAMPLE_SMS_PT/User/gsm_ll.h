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
#include "stdint.h"
#include "stdio.h"

/* Get config */
#include "gsm_config.h"

#if GSM_RTOS
#include "cmsis_os.h"
#endif /* GSM_RTOS */
    
/**
 * \defgroup LOWLEVEL_Typedefs
 * \brief    GSM Low-Level
 * \{
 */

/**
 * \brief   Loe-level control enumeration with supported callbacks
 * 
 * This enumeration is used in \ref GSM_LL_Callback function with param and result parameters to function
 */
typedef enum _GSM_LL_Control_t {
    /**
     * \brief       Called to initialize low-level part of device, such as UART and GPIO configuration
     *
     * \param[in]   *param: Pointer to \ref GSM_LL_t structure with baudrate setup
     * \param[out]  *result: Pointer to \ref uint8_t variable with result. Set to 0 when OK, or non-zero on ERROR.
     */
    GSM_LL_Control_Init = 0x00,     /*!< Initialization control */
    
    /**
     * \brief       Called to send data to GSM device
     *
     * \param[in]   *param: Pointer to \ref GSM_LL_Send_t structure with data to send
     * \param[out]  *result: Pointer to \ref uint8_t variable with result. Set to 0 when OK, or non-zero on ERROR.
     */
    GSM_LL_Control_Send,            /*!< Send data control */
    
    /**
     * \brief       Called to set software RTS pin when necessary
     *
     * \param[in]   *param: Pointer to \ref uint8_t variable with RTS info. This parameter can be a value of \ref GSM_RTS_SET or \ref GSM_RTS_CLR macros
     * \param[out]  *result: Pointer to \ref uint8_t variable with result. Set to 0 when OK, or non-zero on ERROR.
     */
    GSM_LL_Control_SetRTS,          /*!< Set software RTS control */
    
    /**
     * \brief       Called to set reset pin when necessary
     *
     * \param[in]   *param: Pointer to \ref uint8_t variable with RESET info. This parameter can be a value of \ref GSM_RESET_SET or \ref GSM_RESET_CLR macros
     * \param[out]  *result: Pointer to \ref uint8_t variable with result. Set to 0 when OK, or non-zero on ERROR.
     */
    GSM_LL_Control_SetReset,        /*!< Set reset control */
    
    /**
     * \brief       Called to create system synchronization object on RTOS support
     *
     * \param[in]   *param: Pointer to \ref GSM_RTOS_SYNC_t variable with sync object
     * \param[out]  *result: Pointer to \ref uint8_t variable with result. Set to 0 when OK, or non-zero on ERROR.
     */
    GSM_LL_Control_SYS_Create,      /*!< Creates a synchronization object */
    
    /**
     * \brief       Called to delete system synchronization object on RTOS support
     *
     * \param[in]   *param: Pointer to \ref GSM_RTOS_SYNC_t variable with sync object
     * \param[out]  *result: Pointer to \ref uint8_t variable with result. Set to 0 when OK, or non-zero on ERROR.
     */
    GSM_LL_Control_SYS_Delete,      /*!< Deletes a synchronization object */
    
    /**
     * \brief       Called to grant access to sync object
     *
     * \param[in]   *param: Pointer to \ref GSM_RTOS_SYNC_t variable with sync object
     * \param[out]  *result: Pointer to \ref uint8_t variable with result. Set to 0 when OK, or non-zero on ERROR.
     */
    GSM_LL_Control_SYS_Request,     /*!< Requests grant for specific sync object */
    
    /**
     * \brief       Called to release access from sync object
     *
     * \param[in]   *param: Pointer to \ref GSM_RTOS_SYNC_t variable with sync object
     * \param[out]  *result: Pointer to \ref uint8_t variable with result. Set to 0 when OK, or non-zero on ERROR.
     */
    GSM_LL_Control_SYS_Release,     /*!< Releases grant for specific sync object */
} GSM_LL_Control_t;

/**
 * \brief   Structure for sending data to low-level part
 */
typedef struct _GSM_LL_Send_t {
    const uint8_t* Data;            /*!< Pointer to data to send */
    uint16_t Count;                 /*!< Number of bytes to send */
    uint8_t Result;                 /*!< Result of last send */
} GSM_LL_Send_t;

/**
 * \brief   Low level structure for driver
 * \note    For now it has basic settings only without hardware flow control.
 */
typedef struct _GSM_LL_t {
    uint32_t Baudrate;              /*!< Baudrate to be used for UART */
} GSM_LL_t;
    
/**
 * \}
 */
    
/* Include library */
#include "gsm.h"
    
/**
 * \defgroup LOWLEVEL_Macros
 * \brief    GSM Low-Level macros
 * \{
 */

#define GSM_RTS_SET         1       /*!< RTS should be set high */
#define GSM_RTS_CLR         0       /*!< RTS should be set low */
#define GSM_RESET_SET       1       /*!< Reset pin should be set */
#define GSM_RESET_CLR       0       /*!< Reset pin should be cleared */
    
/**
 * \}
 */

/**
 * \defgroup    LOWLEVEL_Functions
 * \brief       GSM Low-Level implementation
 * \{
 */

/**
 * \brief       Low-level callback for interaction with device specific section
 * \param[in]   ctrl: Control to be done
 * \param[in]   *param: Pointer to parameter setup, depends on control type
 * \param[out]  *result: Optional result parameter in case of commands
 * \retval      1: Control command has been processed
 * \retval      0: Control command has not been processed
 */
uint8_t GSM_LL_Callback(GSM_LL_Control_t ctrl, void* param, void* result);
    
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
