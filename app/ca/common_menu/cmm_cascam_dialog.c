/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cmm_cascam_dialog.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.10.05		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_config.h"
#include "app_cascam_pop.h"
#if CASCAM_SUPPORT
extern void cmm_create_dialog1(CmmDialogClass *para);
extern void cmm_exit_dialog1(void);
extern void cmm_create_dialog2(CmmNoticeClass *para);
extern void cmm_exit_dialog2(void);
extern void cmm_create_dialog3(CmmOsdRollingClass *para);
extern void cmm_exit_dialog3(int style);
extern void cmm_create_dialog4(CmmPasswordDlgClass *para);
extern void cmm_exit_dialog4(void);
extern void cmm_create_dialog5(CmmEmailNoticeClass *para);
extern void cmm_exit_dialog5(CmmEmailNoticeTypeEnum type);
extern void cmm_create_dialog6(CmmFingerprintClass *para);
extern void cmm_exit_dialog6(CmmFingerTypeEnum type);
extern void cmm_create_dialogx(CmmSuperMessageClass *param);
extern void cmm_exit_dialogx(CmmSuperMessageClass *param);

extern void cmm_check_rolling_osd(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
extern void cmm_get_rolling_times(unsigned int *times, int style);

static CMMOSDClass thiz_cmm_dialog_func = {
    cmm_create_dialog1,
    cmm_exit_dialog1,
    cmm_create_dialog2,
    cmm_exit_dialog2,
    cmm_create_dialog3,
    cmm_exit_dialog3,
    cmm_create_dialog4,
    cmm_exit_dialog4,
    cmm_create_dialog5,
    cmm_exit_dialog5,
    cmm_create_dialog6,
    cmm_exit_dialog6,
    NULL,//cmm_create_dialogx,
    NULL,//cmm_exit_dialogx,
    // osd rolling rect overlap check
    cmm_check_rolling_osd,
    cmm_get_rolling_times,
    NULL,
    NULL,//cmm_check_finger,
    NULL,//cmm_check_finger_resume,
};

void cmm_cascam_create_dialog1(CmmDialogClass *param)
{
    if(thiz_cmm_dialog_func.CMMCreateDialog1)
    {
        thiz_cmm_dialog_func.CMMCreateDialog1(param);
    }
}

void cmm_cascam_exit_dialog1(void)
{
    if(thiz_cmm_dialog_func.CMMExitDialog1)
    {
        thiz_cmm_dialog_func.CMMExitDialog1();
    }
}

void cmm_cascam_create_dialog2(CmmNoticeClass *param)
{
    if(thiz_cmm_dialog_func.CMMCreateDialog2)
    {
        thiz_cmm_dialog_func.CMMCreateDialog2(param);
    }

}

void cmm_cascam_exit_dialog2(void)
{
    if(thiz_cmm_dialog_func.CMMExitDialog2)
    {
        thiz_cmm_dialog_func.CMMExitDialog2();
    }
}

void cmm_cascam_create_dialog3(CmmOsdRollingClass *param)
{
    if(thiz_cmm_dialog_func.CMMCreateDialog3)
    {
        thiz_cmm_dialog_func.CMMCreateDialog3(param);
    }
}

void cmm_cascam_exit_dialog3(int style)
{
    if(thiz_cmm_dialog_func.CMMExitDialog3)
    {
        thiz_cmm_dialog_func.CMMExitDialog3(style);
    }
}

void cmm_cascam_create_dialog4(CmmPasswordDlgClass *param)
{
    if(thiz_cmm_dialog_func.CMMCreateDialog4)
    {
        thiz_cmm_dialog_func.CMMCreateDialog4(param);
    }
}

void cmm_cascam_exit_dialog4(void)
{
    if(thiz_cmm_dialog_func.CMMExitDialog4)
    {
        thiz_cmm_dialog_func.CMMExitDialog4();
    }
}

void cmm_cascam_create_dialog5(CmmEmailNoticeClass *param)
{
    if(thiz_cmm_dialog_func.CMMCreateDialog5)
    {
        thiz_cmm_dialog_func.CMMCreateDialog5(param);
    }
}

void cmm_cascam_exit_dialog5(CmmEmailNoticeTypeEnum type)
{
    if(thiz_cmm_dialog_func.CMMExitDialog5)
    {
        thiz_cmm_dialog_func.CMMExitDialog5(type);
    }
}

void cmm_cascam_create_dialog6(CmmFingerprintClass *param)
{
    if(thiz_cmm_dialog_func.CMMCreateDialog6)
    {
        thiz_cmm_dialog_func.CMMCreateDialog6(param);
    }
}

void cmm_cascam_exit_dialog6(CmmFingerTypeEnum type)
{
    if(thiz_cmm_dialog_func.CMMExitDialog6)
    {
        thiz_cmm_dialog_func.CMMExitDialog6(type);
    }
}

void cmm_cascam_create_dialogx(CmmSuperMessageClass *param)
{
    if(thiz_cmm_dialog_func.CMMCreateDialogx)
    {
        thiz_cmm_dialog_func.CMMCreateDialogx(param);
    }
}

void cmm_cascam_exit_dialogx(CmmSuperMessageClass *param)
{
    if(thiz_cmm_dialog_func.CMMExitDialogx)
    {
        thiz_cmm_dialog_func.CMMExitDialogx(param);
    }
}

// finger
void cmm_cascam_check_finger(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    if(thiz_cmm_dialog_func.CMMCheckFingerRectOverlap)
    {
        thiz_cmm_dialog_func.CMMCheckFingerRectOverlap(x, y, w, h);
    }
}

void cmm_cascam_resume_finger(void)
{
    if(thiz_cmm_dialog_func.CMMCheckFingerResume)
    {
        thiz_cmm_dialog_func.CMMCheckFingerResume();
    }
}


// osd rolling
void cmm_cascam_check_rolling_osd(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    if(thiz_cmm_dialog_func.CMMCheckRollingRectOverlap)
    {
        thiz_cmm_dialog_func.CMMCheckRollingRectOverlap(x, y, w, h);
    }
}

void cmm_cascam_get_rolling_times(unsigned int *times, int style)
{
    if(thiz_cmm_dialog_func.CMMGetRollingTimes)
    {
        thiz_cmm_dialog_func.CMMGetRollingTimes(times, style);
    }
}

int cmm_cascam_check_lock_service_status(void)
{
    if(thiz_cmm_dialog_func.CMMCheckServiceLockStatus)
    {
        return thiz_cmm_dialog_func.CMMCheckServiceLockStatus(); 
    }
    return 0;
}

void cmm_ca_dialog_func_init(CMMOSDClass func)
{
    // 1
    if(func.CMMCreateDialog1)
    {
        thiz_cmm_dialog_func.CMMCreateDialog1 = func.CMMCreateDialog1;
    }
    if(func.CMMExitDialog1)
    {
        thiz_cmm_dialog_func.CMMExitDialog1 = func.CMMExitDialog1;
    }
    // 2
    if(func.CMMCreateDialog2)
    {
        thiz_cmm_dialog_func.CMMCreateDialog2 = func.CMMCreateDialog2;
    }
    if(func.CMMExitDialog2)
    {
        thiz_cmm_dialog_func.CMMExitDialog2 = func.CMMExitDialog2;
    }
    // 3
    if(func.CMMCreateDialog3)
    {
        thiz_cmm_dialog_func.CMMCreateDialog3 = func.CMMCreateDialog3;
    }
    if(func.CMMExitDialog3)
    {
        thiz_cmm_dialog_func.CMMExitDialog3 = func.CMMExitDialog3;
    }
    // 4
    if(func.CMMCreateDialog4)
    {
        thiz_cmm_dialog_func.CMMCreateDialog4 = func.CMMCreateDialog4;
    }
    if(func.CMMExitDialog4)
    {
        thiz_cmm_dialog_func.CMMExitDialog4 = func.CMMExitDialog4;
    }
    // 5
    if(func.CMMCreateDialog5)
    {
        thiz_cmm_dialog_func.CMMCreateDialog5 = func.CMMCreateDialog5;
    }
    if(func.CMMExitDialog5)
    {
        thiz_cmm_dialog_func.CMMExitDialog5 = func.CMMExitDialog5;
    }
    // 6
    if(func.CMMCreateDialog6)
    {
        thiz_cmm_dialog_func.CMMCreateDialog6 = func.CMMCreateDialog6;
    }
    if(func.CMMExitDialog6)
    {
        thiz_cmm_dialog_func.CMMExitDialog6 = func.CMMExitDialog6;
    }
    // 7
    if(func.CMMCreateDialogx)
    {
        thiz_cmm_dialog_func.CMMCreateDialogx = func.CMMCreateDialogx;
    }
    if(func.CMMExitDialogx)
    {
        thiz_cmm_dialog_func.CMMExitDialogx = func.CMMExitDialogx;
    }

    // rolling osd
    if(func.CMMCheckRollingRectOverlap)
    {
        thiz_cmm_dialog_func.CMMCheckRollingRectOverlap = func.CMMCheckRollingRectOverlap;
    }
    if(func.CMMGetRollingTimes)
    {
        thiz_cmm_dialog_func.CMMGetRollingTimes = func.CMMGetRollingTimes;
    }
    if(func.CMMCheckServiceLockStatus)
    {
        thiz_cmm_dialog_func.CMMCheckServiceLockStatus = func.CMMCheckServiceLockStatus;
    }

    // finger
    if(func.CMMCheckFingerRectOverlap)
    {
        thiz_cmm_dialog_func.CMMCheckFingerRectOverlap = func.CMMCheckFingerRectOverlap;
    }
    if(func.CMMCheckFingerResume)
    {
        thiz_cmm_dialog_func.CMMCheckFingerResume = func.CMMCheckFingerResume;
    }

}
#endif

