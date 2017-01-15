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
#include "gsm_sys.h"
#include "cmsis_os.h"

osMutexId id;

uint8_t GSM_SYS_Create(GSM_RTOS_SYNC_t* Sync) {
    /* Create mutex object inside RTOS */
    id = osMutexCreate(Sync);
    
    /* Return status */
    return id == NULL;
}

uint8_t GSM_SYS_Delete(GSM_RTOS_SYNC_t* Sync) {
    /* Delete mutex object from RTOS */
    osMutexDelete(Sync);
    
    /* We are OK */
    return 0;
}

uint8_t GSM_SYS_Request(GSM_RTOS_SYNC_t* Sync) {
    /* Wait for mutex */
    return osMutexWait(id, 1000) == osOK ? 0 : 1;
}

uint8_t GSM_SYS_Release(GSM_RTOS_SYNC_t* Sync) {
    /* Release mutex ans return status */
    return osMutexRelease(id) == osOK ? 0 : 1;
}
