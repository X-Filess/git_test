/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cmm_cascam_dialog2.c
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

static CmmNoticeClass thiz_notice_record = {0};
#if 1
// 
void cmm_create_dialog2(CmmNoticeClass *para)
{
    if((NULL == para)||(NULL == para->str))
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    if(0 == memcmp(&thiz_notice_record, para, sizeof(CmmNoticeClass)))
    {// same
        return ;
    }
    memcpy(&thiz_notice_record, para, sizeof(CmmNoticeClass));
}

void cmm_exit_dialog2(void)
{
    memset(&thiz_notice_record, 0, sizeof(CmmNoticeClass));
}

char cmm_ca_is_notice(void)
{
    if(thiz_notice_record.str)
        return 1;
    else
        return 0;
}

unsigned char *cmm_ca_get_notice_str(void)
{
    return (unsigned char *)thiz_notice_record.str;
}

#else
// use separate dialog
#define WND_NOTICE                  "ca_notice_prompt" // in style.xml
#define PROMPT_NOTICE_NAME          "prompt_ca_notice"

void cmm_cascam_create_dialog2(CmmNoticeClass *para)
{
#define  X_DEFAULT_POS 320
#define  Y_DEFAULT_POS 152
    if((NULL == para)||(NULL == para->str))
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    if(GUI_CheckPrompt(PROMPT_NOTICE_NAME) == GXCORE_SUCCESS)
    {
        if(0 == memcmp(&thiz_notice_record, para, sizeof(CmmNoticeClass)))
        {// same
            return ;
        }
        memcpy(&thiz_notice_record, para, sizeof(CmmNoticeClass));
        GUI_EndPrompt(PROMPT_NOTICE_NAME);
    }
    if((0 == para->x) && (0 == para->y))
    {
        GUI_CreatePrompt(X_DEFAULT_POS, Y_DEFAULT_POS, PROMPT_NOTICE_NAME, WND_NOTICE, para->str, "ontop");
    }
    else
    {
        GUI_CreatePrompt(para->x, para->y, PROMPT_NOTICE_NAME, WND_NOTICE, para->str, "ontop");
    }
}

void cmm_cascam_exit_dialog2(void)
{
    if(GUI_CheckPrompt(PROMPT_NOTICE_NAME) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(PROMPT_NOTICE_NAME);
    }
    memset(&thiz_notice_record, 0, sizeof(CmmNoticeClass));
}
#endif
#endif

