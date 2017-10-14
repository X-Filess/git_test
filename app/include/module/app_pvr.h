/*
 * =====================================================================================
 *
 *       Filename:  app_pvr.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2011年09月20日 14时18分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef __APP_PVR_H__
#define __APP_PVR_H__

#include "gxcore.h"

typedef enum
{
    PVR_DUMMY,
    PVR_RECORD,
    PVR_TIMESHIFT,
    PVR_TMS_AND_REC,
    PVR_SPEED,
    PVR_TP_RECORD,
}pvr_state;

typedef struct
{
    uint32_t    cur_tick;
    uint32_t    total_tick;
    uint32_t    seekmin_tick;
    uint32_t    remaining_tick;
    uint32_t    rec_duration;
}pvr_time;

typedef struct
{
    uint32_t    prog_id;
    char        *dev;   //
    char        *path;  // 录制的路径
    char        *name;  // 录制的件名
    char        *url;   // 发给player时移或录制的url
}pvr_env;

typedef enum
{
    PVR_SPD_1    =   (1),
    PVR_SPD_2    =   (2),
    PVR_SPD_4    =   (4),
    PVR_SPD_8    =   (8),
    PVR_SPD_16   =   (16),
    PVR_SPD_24   =   (24),
    PVR_SPD_32   =   (32)
}pvr_speed;

typedef enum
{
    USB_OK = 0,
    USB_NO_SPACE,
    USB_ERROR,
}usb_check_state;

typedef struct _pvr_ops AppPvrOps;
struct _pvr_ops
{
    pvr_state       state;
    pvr_time        time;
    pvr_env         env;
    int32_t         spd;
	int32_t         enter_shift;
    usb_check_state    (*usb_check)(AppPvrOps*);
    status_t        (*env_sync)(AppPvrOps*);
    void            (*env_clear)(AppPvrOps*);
    void            (*rec_start)(AppPvrOps*);
    void            (*tp_rec_start)(AppPvrOps*);
    void            (*rec_stop)(AppPvrOps*);   // RECORD only
    void            (*tms_stop)(AppPvrOps*); // TMS onlt
    void            (*pause)(AppPvrOps*);
    void            (*resume)(AppPvrOps*);
    void            (*seek)(AppPvrOps*, int64_t);
    void            (*speed)(AppPvrOps*, float);
    int32_t        (*percent)(AppPvrOps*);
    void            (*tms_delete)(AppPvrOps*);  // delete tms file
    //status_t        (*free_space)(AppPvrOps*);
};

extern AppPvrOps g_AppPvrOps;
#endif

