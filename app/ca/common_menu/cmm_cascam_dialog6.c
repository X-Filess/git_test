/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cmm_cascam_dialog6.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.04.19		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_config.h"
#include "app_cascam_pop.h"

#if CASCAM_SUPPORT
#define PROMPT_FINGERPRINT_NAME    "prompt_ca_fingerprint"
#define WND_FINGERPRINT            "ca_fingerprint_prompt"
static int _create_dialog_check(void)
{
#if 0
    if((GXCORE_SUCCESS == GUI_CheckDialog("wnd_epg"))
            || (GXCORE_SUCCESS == GUI_CheckDialog("wnd_main_menu"))
            || (GXCORE_SUCCESS == GUI_CheckDialog("wnd_channel_info_signal"))) 
    {
        return 0;
    }
#endif
    return 1;
}

static void _generat_random_pos(unsigned int *x, unsigned int *y)
{
#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   576
    unsigned int fingerprint_width = 0;
    unsigned int fingerprint_height = 0;
    GuiWidget *widget = NULL;
    widget = gui_get_window(PROMPT_FINGERPRINT_NAME);
    if(widget == NULL)
    {
        printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    fingerprint_width = widget->rect.w;
    fingerprint_height = widget->rect.h;
    *x = rand()%(SCREEN_WIDTH - fingerprint_width);
    *y = rand()%(SCREEN_HEIGHT - fingerprint_height);
}
#define MAX_FINGERPRINT_STATE 1
static char *thiz_finger_widget[] = {
    "PROMPT_FINGERPRINT_NAME",
};
static event_list  *thiz_query_move_timer[MAX_FINGERPRINT_STATE] = {NULL}; 
static int _query_move_timer(void *usrdata)
{
    unsigned int x = 0;
    unsigned int y = 0;
    int index = *(int*)usrdata;

    _generat_random_pos(&x, &y);
    GUI_EndPrompt(thiz_finger_widget[index]);
    GUI_CreatePrompt(x, y, thiz_finger_widget[index], WND_FINGERPRINT, NULL, "ontop");
    return 0;
}

static int _create_move_timer(unsigned int duration_sec, int index)
{
    static int timer_index[MAX_FINGERPRINT_STATE] = {0};
    timer_index[index] = index;
    if (reset_timer(thiz_query_move_timer[index]) != 0)
    {
        thiz_query_move_timer[index] = create_timer(_query_move_timer, duration_sec*1000, (void*)&timer_index[index], TIMER_REPEAT);
    }
    return 0;
}

static int _destroy_move_timer(int index)
{
    int ret = 0;
    int i = 0;

    // TODO: 20160419
    index = 0;
    if(index == -1)
    {
        for(i = 0; i < MAX_FINGERPRINT_STATE; i++)
        {
            if(thiz_query_move_timer[i] != NULL)
            {
                remove_timer(thiz_query_move_timer[i]);
                thiz_query_move_timer[i] = NULL;
            }
        }
    }
    else
    {
        if(thiz_query_move_timer[index] != NULL)
        {
            remove_timer(thiz_query_move_timer[index]);
            thiz_query_move_timer[index] = NULL;
        }

    }
    return ret;
}

void cmm_create_dialog6(CmmFingerprintClass *para)
{
#define X1_DEFAUT 900
#define Y1_DEFAULT 100
    unsigned int x_pos = 0;
    unsigned int y_pos = 0;
    char *prompt_name = NULL;
    char *wnd_name = NULL;

    if(NULL == para)
    {
        printf("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }
    if( CMM_STRING == para->type)
    {
        prompt_name = PROMPT_FINGERPRINT_NAME;
        wnd_name = WND_FINGERPRINT;
        if(para->random)
        {
            _generat_random_pos(&x_pos, &y_pos);
        }
        else
        {
            x_pos = X1_DEFAUT;
            y_pos = Y1_DEFAULT;
        }
    }
    else
    {
        // TODO: 20160419
    }

    _destroy_move_timer(para->type);
    if(GUI_CheckPrompt(prompt_name) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(prompt_name);
    }

    if(_create_dialog_check())
    {
        GUI_CreatePrompt(x_pos, y_pos, prompt_name, wnd_name, NULL, "ontop");
    }
    if(para->random)
    {
       _create_move_timer(1,para->type);// 1s
    }
}

void cmm_exit_dialog6(CmmFingerTypeEnum type)
{
    _destroy_move_timer(type);
    if(GUI_CheckPrompt(PROMPT_FINGERPRINT_NAME) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(PROMPT_FINGERPRINT_NAME);
    }
}
#endif
