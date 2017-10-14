/*
 * =====================================================================================
 *
 *       Filename:  app_oem.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年05月11日 10时33分06秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#ifndef __APP_OEM_H__
#define __APP_OEM_H__

#include "gxcore.h"

typedef enum
{
    OEM_LANG_OSD,
    OEM_LANG_SUBT,
    OEM_LANG_TTX,
    OEM_HW_VERSION,
    OEM_SW_VERSION,
    OEM_TRANS,
    OEM_VOLUME,
    OEM_TV_STANDARD,
    OEM_TIME_ZONE,
    OEM_SUMMER_TIME,
    OEM_SEARCH_MODE,
    OEM_SCART_CONF,
    OEM_SPDIF_CONF,
   // OEM_TUNER_TYPE,
    OEM_TUNER_CONFIG,
    //OEM_DEMOD_TYPE,
    //OEM_DEMOD_CHIPADDR,
    //OEM_IQ_SWAP,
    //OEM_HV_SWITCH,
    OEM_TYPE_MAX
}app_oem_type;

extern status_t app_oem_get(app_oem_type type, char * buf, size_t size);
extern status_t app_oem_get_int(app_oem_type type, int32_t *result);
extern status_t app_oem_get_float(app_oem_type type, float *result);
#endif

