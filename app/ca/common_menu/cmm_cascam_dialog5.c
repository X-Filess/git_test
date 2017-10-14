/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cmm_cascam_dialog5.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.10.13		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_config.h"
#include "app_cascam_pop.h"

#if CASCAM_SUPPORT
#define PROMPT_EMAIL_NOTICE_NEW_NAME    "prompt_ca_email_new"
#define WND_EMAIL_NOTICE_NEW            "ca_email_new_prompt"
#define PROMPT_EMAIL_NOTICE_FULL_NAME   "prompt_ca_email_box_full"
#define WND_EMAIL_NOTICE_FULL           "ca_email_box_full_prompt"

#define MAX_MAIL_STATE 2
static char *thiz_mail_widget[] = {
    "PROMPT_EMAIL_NOTICE_NEW_NAME",
    "PROMPT_EMAIL_NOTICE_FULL_NAME",
};


static int _create_dialog_check(int index)
{
#if 0
    if((GXCORE_SUCCESS == GUI_CheckDialog("wnd_epg"))
            || (GXCORE_SUCCESS == GUI_CheckDialog("wnd_main_menu"))) 
    {
        return 0;
    }
#endif
    return 1;
}


static event_list  *thiz_query_flicker_timer[MAX_MAIL_STATE] = {NULL, NULL}; 
static int _query_flicker_timer(void *usrdata)
{
    static char flag = 0;
    int index = *(int*)usrdata;
    if(0 == flag)
    {
        GUI_SetProperty(thiz_mail_widget[index], "state", "hide");
        flag = 1;
    }
    else
    {
        GUI_SetProperty(thiz_mail_widget[index], "state", "show");
        flag = 0;
    }
    return 0;
}

static int _create_flicker_timer(unsigned int duration_sec, int index)
{
    static int timer_index[MAX_MAIL_STATE] = {0, 0};
    timer_index[index] = index;
    if (reset_timer(thiz_query_flicker_timer[index]) != 0)
    {
        thiz_query_flicker_timer[index] = create_timer(_query_flicker_timer, duration_sec*1000, (void*)&timer_index[index], TIMER_REPEAT);
    }
    return 0;
}

static int _destroy_flicker_timer(int index)
{
    int ret = 0;
    int i = 0;
    if(index == -1)
    {
        for(i = 0; i < MAX_MAIL_STATE; i++)
        {
            if(thiz_query_flicker_timer[i] != NULL)
            {
                remove_timer(thiz_query_flicker_timer[i]);
                thiz_query_flicker_timer[i] = NULL;
            }
        }
    }
    else
    {
        if(thiz_query_flicker_timer[index] != NULL)
        {
            remove_timer(thiz_query_flicker_timer[index]);
            thiz_query_flicker_timer[index] = NULL;
        }

    }
    return ret;
}

#define EMAIL_HIDE_SHOW_CONTROL
#ifdef EMAIL_HIDE_SHOW_CONTROL
// for global control
static event_list  *thiz_query_state_timer = NULL;
typedef struct _MailParamClass{
    char index;// new or full
    char flicker;// 0: static; 1:fliker
    char *prompt_name;
    char *wnd_name;
    int x_pos;
    int y_pos;
    char show_state;
}MailParamClass;

static MailParamClass thiz_mail_param[MAX_MAIL_STATE];

static int _query_state_timer(void *usrdata)
{
    if((GXCORE_SUCCESS == GUI_CheckDialog("wnd_epg"))
            || (GXCORE_SUCCESS == GUI_CheckDialog("wnd_main_menu"))
            || (GXCORE_SUCCESS == GUI_CheckDialog("wnd_nstv_mail_detail")))
    {
        if (thiz_mail_param[0].show_state == 1)
        {
            if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME) == GXCORE_SUCCESS)
            {
                GUI_EndPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME);
            }
        }
        if (thiz_mail_param[1].show_state == 1)
        {
            if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME) == GXCORE_SUCCESS)
            {
                GUI_EndPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME);
            }
        }
    }
    else
    {
        if (thiz_mail_param[0].show_state == 1)
        {
            if(thiz_mail_param[0].flicker)
            {
                //_destroy_flicker_timer(0);
                if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME) == GXCORE_SUCCESS)
                {
                    GUI_EndPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME);
                }
                else
                {
                    GUI_CreatePrompt(thiz_mail_param[0].x_pos, thiz_mail_param[0].y_pos, thiz_mail_param[0].prompt_name, thiz_mail_param[0].wnd_name, NULL, "ontop");
                }
            }
            else
            {
                if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME) == GXCORE_SUCCESS)
                {
                    ;
                }
                else
                {
                    GUI_CreatePrompt(thiz_mail_param[0].x_pos, thiz_mail_param[0].y_pos, thiz_mail_param[0].prompt_name, thiz_mail_param[0].wnd_name, NULL, "ontop");
                }
            }
        }
        if (thiz_mail_param[1].show_state == 1)
        {
            if(thiz_mail_param[1].flicker)
            {
                //_destroy_flicker_timer(0);
                if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME) == GXCORE_SUCCESS)
                {
                    GUI_EndPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME);
                }
                else
                {
                    GUI_CreatePrompt(thiz_mail_param[1].x_pos, thiz_mail_param[1].y_pos, thiz_mail_param[1].prompt_name, thiz_mail_param[1].wnd_name, NULL, "ontop");
                }
            }
            else
            {
                if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME) == GXCORE_SUCCESS)
                {
                    ;
                }
                else
                {
                    GUI_CreatePrompt(thiz_mail_param[1].x_pos, thiz_mail_param[1].y_pos, thiz_mail_param[1].prompt_name, thiz_mail_param[1].wnd_name, NULL, "ontop");
                }
            }
        }
    }

    return 0;
}

static int _create_state_timer(void)
{
    if (reset_timer(thiz_query_state_timer) != 0)
    {
        thiz_query_state_timer = create_timer(_query_state_timer, 1000, NULL, TIMER_REPEAT);
    }
    return 0;
}

static int _destroy_state_timer(void)
{
    int ret = 0;
    //int i = 0;
    if(thiz_query_state_timer != NULL)
    {
        remove_timer(thiz_query_state_timer);
        thiz_query_state_timer = NULL;
    }

    return ret;
}
#endif

void cmm_create_dialog5(CmmEmailNoticeClass *para)
{
#ifdef EMAIL_HIDE_SHOW_CONTROL
#define X1_DEFAUT 900
#define Y1_DEFAULT 150
#define X2_DEFAUT 900
#define Y2_DEFAULT 200
    unsigned int x_pos = 0;
    unsigned int y_pos = 0;
    char *prompt_name = NULL;
    char *wnd_name = NULL;

    if(NULL == para)
    {
        printf("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }
    if(EMAIL_RECEIVE_NEW == para->type)
    {
        prompt_name = PROMPT_EMAIL_NOTICE_NEW_NAME;
        wnd_name = WND_EMAIL_NOTICE_NEW;
        x_pos = X1_DEFAUT;
        y_pos = Y1_DEFAULT;
    }
    else if(EMAIL_BOX_FULL == para->type)
    {
        prompt_name = PROMPT_EMAIL_NOTICE_FULL_NAME;
        wnd_name = WND_EMAIL_NOTICE_FULL;
        x_pos = X2_DEFAUT;
        y_pos = Y2_DEFAULT;
    }

    if((0 != para->x) || (0 != para->y))
    {
        x_pos = para->x;
        y_pos = para->y;
    }
    if(GUI_CheckPrompt(prompt_name) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(prompt_name);
    }
    
    //if(_create_dialog_check(para->type))
    {
        //GUI_CreatePrompt(x_pos, y_pos, prompt_name, wnd_name, NULL, "ontop");
        thiz_mail_param[para->type].index = para->type;
        thiz_mail_param[para->type].show_state = 1;
        thiz_mail_param[para->type].flicker = para->flicker;
        thiz_mail_param[para->type].x_pos = x_pos;
        thiz_mail_param[para->type].y_pos = y_pos;
        thiz_mail_param[para->type].prompt_name = prompt_name;
        thiz_mail_param[para->type].wnd_name = wnd_name;
        _create_state_timer();
    }
    //if(para->flicker)
    //{
    //   _create_flicker_timer(1,para->type);// 1s
    //}

#else
#define X1_DEFAUT 900
#define Y1_DEFAULT 100
#define X2_DEFAUT 900
#define Y2_DEFAULT 150
    unsigned int x_pos = 0;
    unsigned int y_pos = 0;
    char *prompt_name = NULL;
    char *wnd_name = NULL;

    if(NULL == para)
    {
        printf("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }
    if(EMAIL_RECEIVE_NEW == para->type)
    {
        prompt_name = PROMPT_EMAIL_NOTICE_NEW_NAME;
        wnd_name = WND_EMAIL_NOTICE_NEW;
        x_pos = X1_DEFAUT;
        y_pos = Y1_DEFAULT;
    }
    else if(EMAIL_BOX_FULL == para->type)
    {
        prompt_name = PROMPT_EMAIL_NOTICE_FULL_NAME;
        wnd_name = WND_EMAIL_NOTICE_FULL;
        x_pos = X2_DEFAUT;
        y_pos = Y2_DEFAULT;
    }

    if((0 != para->x) || (0 != para->y))
    {
        x_pos = para->x;
        y_pos = para->y;
    }
    if(GUI_CheckPrompt(prompt_name) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(prompt_name);
    }
    
    if(_create_dialog_check(para->type))
    {
        GUI_CreatePrompt(x_pos, y_pos, prompt_name, wnd_name, NULL, "ontop");
    }
    if(para->flicker)
    {
       _create_flicker_timer(1,para->type);// 1s
    }
#endif
}

void cmm_exit_dialog5(CmmEmailNoticeTypeEnum type)
{
#ifdef EMAIL_HIDE_SHOW_CONTROL
    if((type > 1) || (type < 0))
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return;
    }
    memset(&thiz_mail_param[type], 0, sizeof(MailParamClass));
#endif
    if(EMAIL_RECEIVE_NEW == type)
    {
        if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME) == GXCORE_SUCCESS)
        {
            GUI_EndPrompt(PROMPT_EMAIL_NOTICE_NEW_NAME);
        }
    }
    if(EMAIL_BOX_FULL == type)
    {
        if(GUI_CheckPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME) == GXCORE_SUCCESS)
        {
            GUI_EndPrompt(PROMPT_EMAIL_NOTICE_FULL_NAME);
        }
    }
    _destroy_flicker_timer(type);
}

#endif
