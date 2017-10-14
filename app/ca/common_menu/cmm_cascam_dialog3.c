/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cmm_cascam_dialog3.c
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
#if 0
#define  WND_ROLLING_TEXT               "ca_rolling_text"

static CmmOsdRollingClass thiz_rolling_record = {0};
void cmm_create_dialog3(CmmOsdRollingClass *para)
{
#define  X_DEFAULT_POS 320
#define  Y_DEFAULT_POS 152
    if((NULL == para)||(NULL == para->str))
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    if(GUI_CheckDialog(WND_ROLLING_TEXT) == GXCORE_SUCCESS)
    {
        if(0 == memcmp(&thiz_rolling_record, para, sizeof(CmmOsdRollingClass)))
        {// same
            return ;
        }
        memcpy(&thiz_rolling_record, para, sizeof(CmmOsdRollingClass));
        GUI_EndDialog(WND_ROLLING_TEXT);
    }
    app_create_dialog(WND_ROLLING_TEXT);

    if((0 != para->x) || (0 != para->y))
    {
        GUI_SetProperty(WND_ROLLING_TEXT, "move_window_x", &thiz_rolling_record.x);
        GUI_SetProperty(WND_ROLLING_TEXT, "move_window_y", &thiz_rolling_record.y);
    }
    if(para->direction)
    {
        GUI_SetProperty(WND_ROLLING_TEXT, "format", thiz_rolling_record.direction);
    }
}

void cmm_exit_dialog3(void)
{
    if(GUI_CheckDialog(WND_ROLLING_TEXT) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_ROLLING_TEXT);
    }
    memset(&thiz_rolling_record, 0, sizeof(CmmOsdRollingClass));
}
#endif

#define WND_ROLLING_R2L                 "ca_rolling_text_r2l_prompt" // in style.xml
#define PROMPT_ROLLING_R2L_NAME         "prompt_ca_rolling_r2l"
#define WND_ROLLING_L2R                 "ca_rolling_text_l2r_prompt" // in style.xml
#define PROMPT_ROLLING_L2R_NAME         "prompt_ca_rolling_l2r"
#define TXT_ROLLING_R2L                 "txt_ca_rolling_r2l"
#define TXT_ROLLING_L2R                 "txt_ca_rolling_l2r"

static event_list  *thiz_query_repeat_timer = NULL; 
static unsigned int thiz_rolling_times = 0;

static void _record_rolling_times(unsigned int times)
{
    thiz_rolling_times = times;
}

static int _query_repeat_timer(void *usrdata)
{
    unsigned int rolling_times = 0;

    if(GUI_CheckPrompt(PROMPT_ROLLING_R2L_NAME) == GXCORE_SUCCESS)
    {
        GUI_GetProperty(TXT_ROLLING_R2L, "times", &rolling_times);
    }
    if(GUI_CheckPrompt(PROMPT_ROLLING_L2R_NAME) == GXCORE_SUCCESS)
    {
        GUI_GetProperty(TXT_ROLLING_L2R, "times", &rolling_times);
    }
    _record_rolling_times(rolling_times);

    if(rolling_times >= *(unsigned int*)usrdata)
    {
        cmm_cascam_exit_dialog3(0);
    }
    return 0;
}

static unsigned int thiz_repeat_times = 0;
static int _create_repeat_timer(unsigned int repeat_times)
{
    thiz_repeat_times = repeat_times;
    if (reset_timer(thiz_query_repeat_timer) != 0) 
    {
        thiz_query_repeat_timer = create_timer(_query_repeat_timer, 100,  (void*)&thiz_repeat_times, TIMER_REPEAT);
    }
    return 0;
}

static event_list  *thiz_query_duration_timer = NULL; 
static int _query_duration_timer(void *usrdata)
{
    static char flag = 0;
    if(0 == flag)
    {
        GUI_SetProperty(TXT_ROLLING_L2R, "state", "hide");
        GUI_SetProperty(TXT_ROLLING_R2L, "state", "hide");
        flag = 1;
    }
    else
    {
        GUI_SetProperty(TXT_ROLLING_L2R, "state", "show");
        GUI_SetProperty(TXT_ROLLING_R2L, "state", "show");
        flag = 0;
    }
    return 0;
}

static int _create_duration_timer(unsigned int duration_sec)
{
    if (reset_timer(thiz_query_duration_timer) != 0) 
    {
        thiz_query_duration_timer = create_timer(_query_duration_timer, duration_sec*1000, NULL, TIMER_REPEAT);
    }
    return 0;
}

// 1: need create; 0: need not create
static int _create_dialog_check(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    extern GuiWidget *gui_get_window(const char *name);
    char *window_name = NULL;
    GUI_Rect rect = {0};
    GuiWidget *widget = NULL;

    if((GXCORE_SUCCESS == GUI_CheckDialog("wnd_epg"))
            || (GXCORE_SUCCESS == GUI_CheckDialog("wnd_main_menu"))
            || (GXCORE_SUCCESS == GUI_CheckDialog("wnd_channel_info"))) 
    {
        return 0;
    }
    window_name = (char *)GUI_GetFocusWindow();
    if(NULL == window_name)
    {
        printf("\nError, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if(0 == strncmp("wnd_full_screen", window_name, strlen(window_name)))
    {
        return 1;
    }
    widget = gui_get_window(window_name);
    if(NULL == widget)
    {
        return -1;
    }
    rect.x = widget->rect.x;
    rect.y = widget->rect.y;
    rect.w = widget->rect.w;
    rect.h = widget->rect.h;
    if(((x + w) < rect.x)
            || ((y + h) < rect.y)
            || (y > (rect.y + rect.h))
            || (x > (rect.x + rect.w)))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void cmm_create_dialog3(CmmOsdRollingClass *para)
{
#define  X_TOP_POS  40
#define  Y_TOP_POS  36
#define  X_BOTTOM_POS 40
#define  Y_BOTTOM_POS 500
    char *prompt_name = PROMPT_ROLLING_R2L_NAME;
    char *window_name = WND_ROLLING_R2L;
    unsigned int x_default_pos = X_TOP_POS;
    unsigned int y_default_pos = Y_TOP_POS;

    if((NULL == para)||(NULL == para->str))
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }

    if(GUI_CheckPrompt(PROMPT_ROLLING_R2L_NAME) == GXCORE_SUCCESS)
    {
        GUI_SetProperty(TXT_ROLLING_R2L, "rolling_stop", NULL);
        GUI_EndPrompt(PROMPT_ROLLING_R2L_NAME);
    }
    if(GUI_CheckPrompt(PROMPT_ROLLING_L2R_NAME) == GXCORE_SUCCESS)
    {
        GUI_SetProperty(TXT_ROLLING_L2R, "rolling_stop", NULL);
        GUI_EndPrompt(PROMPT_ROLLING_L2R_NAME);
    }

    if(para->direction)
    {
        if(0 == memcmp(para->direction, "roll_r2l", 8))
        {
            prompt_name = PROMPT_ROLLING_R2L_NAME;
            window_name = WND_ROLLING_R2L;
        }
        else
        {
            prompt_name = PROMPT_ROLLING_L2R_NAME;
            window_name = WND_ROLLING_L2R;
        }
    }

    if(0 == para->pos)
    {
        x_default_pos = X_TOP_POS;
        y_default_pos = Y_TOP_POS;
    }
    else
    {
        x_default_pos = X_BOTTOM_POS;
        y_default_pos = Y_BOTTOM_POS;

    }

    if((0 == para->x) && (0 == para->y))
    {
        if(_create_dialog_check(x_default_pos, y_default_pos, 1024, 576))
        {
            GUI_CreatePrompt(x_default_pos, y_default_pos, prompt_name, window_name, para->str, "ontop");
        }
        else
        {
            return ;
        }
    }
    else
    {
        if(_create_dialog_check(para->x, para->y, 1024, 576))
        {
            GUI_CreatePrompt(para->x, para->y, prompt_name, window_name, para->str, "ontop");
        }
        else
        {
            return ;
        }
    }

    _record_rolling_times(0);

    if(para->repeat_times > 0)
    {
        _create_repeat_timer(para->repeat_times);
    }

    if(para->duration > 0)
    {
        _create_duration_timer(para->duration);
    }

}

void cmm_exit_dialog3(int style)
{
    if(GUI_CheckPrompt(PROMPT_ROLLING_R2L_NAME) == GXCORE_SUCCESS)
    {
        GUI_SetProperty(TXT_ROLLING_R2L, "rolling_stop", NULL);
        GUI_EndPrompt(PROMPT_ROLLING_R2L_NAME);
    }
    if(GUI_CheckPrompt(PROMPT_ROLLING_L2R_NAME) == GXCORE_SUCCESS)
    {
        GUI_SetProperty(TXT_ROLLING_L2R, "rolling_stop", NULL);
        GUI_EndPrompt(PROMPT_ROLLING_L2R_NAME);
    }
    // timer
    if(thiz_query_repeat_timer)
    {
       remove_timer(thiz_query_repeat_timer);
       thiz_query_repeat_timer = NULL;
    }
    if(thiz_query_duration_timer)
    {
       remove_timer(thiz_query_duration_timer);
       thiz_query_duration_timer = NULL;
    }
    _record_rolling_times(0xffffffff);
    //memset(&thiz_notice_record, 0, sizeof(CmmNoticeClass));
}

void cmm_get_rolling_times(unsigned int *times, int style)
{
    if( NULL == times)
    {
        return ;
    }
#if 0
    if(GUI_CheckPrompt(PROMPT_ROLLING_R2L_NAME) == GXCORE_SUCCESS)
    {
        GUI_GetProperty(TXT_ROLLING_R2L, "times", times);
    }
    else if(GUI_CheckPrompt(PROMPT_ROLLING_L2R_NAME) == GXCORE_SUCCESS)
    {
        GUI_GetProperty(TXT_ROLLING_L2R, "times", times);
    }
    else
    {
        *times = 0xffffffff;
    }
#else
    *times = thiz_rolling_times;
#endif
}

static int _get_rolling_osd_rect(unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h, char *name)
{
    GuiWidget *widget = NULL;

    if((NULL == x) || (NULL == y) || (NULL == w) || (NULL == h) || (NULL == name))
    {
        return -1;
    }
    widget = gui_get_window(name);
    if(NULL == widget)
    {
        return -1;
    }
    *x = widget->rect.x;
    *y = widget->rect.y;
    *w = widget->rect.w;
    *h = widget->rect.h;
    return 0; 
}

static int _check_rect_overlap(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
                                unsigned int x1, unsigned int y1, unsigned int w1, unsigned int h1)
{
    if(((x + w) < x1)
            || ((y + h) < y1)
            || (y > (y1 + h1))
            || (x > (x1 + w1)))
    {
        return 0;// no overlap
    }
    else
    {
        return 1;// operlap
    }
}

void cmm_check_rolling_osd(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    unsigned int x_pos = 0, y_pos = 0, width = 0, height = 0;

    if(GUI_CheckPrompt(PROMPT_ROLLING_R2L_NAME) == GXCORE_SUCCESS)
    {
        if(_get_rolling_osd_rect(&x_pos, &y_pos, &width, &height, PROMPT_ROLLING_R2L_NAME) == 0)
        {
            if(_check_rect_overlap(x, y, w, h, x_pos, y_pos, width, height))
            {
                cmm_cascam_exit_dialog3(0);
            }
        }
    }

    if(GUI_CheckPrompt(PROMPT_ROLLING_L2R_NAME) == GXCORE_SUCCESS)
    {
        if(_get_rolling_osd_rect(&x_pos, &y_pos, &width, &height, PROMPT_ROLLING_L2R_NAME) == 0)
        {
            if(_check_rect_overlap(x, y, w, h, x_pos, y_pos, width, height))
            {
                cmm_cascam_exit_dialog3(0);
            }
        }
    }
}

#endif
