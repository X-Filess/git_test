/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_curtain.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.04.24		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_config.h"
#include "app_cascam_pop.h"

#if NSCAS_SUPPORT
#define  PROMPT_CURTAIN     "prompt_advanced_preview" 
#define  WND_CURTAIN        "ca_advanced_preview_prompt"

static void _create_curtain_notice(void)
{
    if(GUI_CheckPrompt(PROMPT_CURTAIN) == GXCORE_SUCCESS)
    {
        ;//GUI_EndPrompt(PROMPT_CURTAIN);
    }
    else
    {
        GUI_CreatePrompt(800, 430, PROMPT_CURTAIN, WND_CURTAIN, NULL, "ontop");
    }
}

static void _exit_curtain(void)
{
    if(GUI_CheckPrompt(PROMPT_CURTAIN) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(PROMPT_CURTAIN);
    }
}

static void _curtain_notice(char *message)
{
    CmmNoticeClass notice = {0};
    notice.str = message;
    cmm_cascam_create_dialog2(&notice);
    _create_curtain_notice();
}

static void _curtain_notice_clear(void)
{
    cmm_cascam_exit_dialog2();
}


void app_nstv_curtain_notify(char type, char* notice)
{
    switch(type)
    {
        case NSTV_CURTAIN_CANCEL:/*取消高级预览显示*/
            _curtain_notice_clear();
	        _exit_curtain();
            break;
        case NSTV_CURTAIN_OK:/*高级预览节目正常解密*/
            _curtain_notice_clear();
	        _create_curtain_notice();
            break;
        case NSTV_CURTAIN_TOTTIME_ERROR:/*高级预览节目禁止解密：已经达到总观看时长*/
            _curtain_notice(notice);
            break;
        case NSTV_CURTAIN_WATCHTIME_ERROR:/*高级预览节目禁止解密：已经达到WatchTime限制*/
            _curtain_notice(notice);
            break;
        case NSTV_CURTAIN_TOTCNT_ERROR:/*高级预览节目禁止解密节目禁止解密：已经达到总允许观看次数*/
            _curtain_notice(notice);
            break;
        case NSTV_CURTAIN_ROOM_ERROR:/*高级预览节目禁止解密：高级预览节目记录空间不足*/
            _curtain_notice(notice);
            break;
        case NSTV_CURTAIN_PARAM_ERROR:/*高级预览节目禁止解密：节目参数错误*/
            _curtain_notice(notice);
            break;
        case NSTV_CURTAIN_TIME_ERROR:/*高级预览节目禁止解密节目禁止解密节目禁止解密：数据错误*/
            _curtain_notice(notice);
            break;
        default:
            printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }

}
void app_nstv_curtain_notify_exit(char type, char* notice)
{
    _curtain_notice_clear();
}
#endif
