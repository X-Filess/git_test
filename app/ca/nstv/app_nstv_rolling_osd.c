/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_rolling_osd.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.04.18		   B.Z.			  Creation
*****************************************************************************/
#include "app_module.h"
#include "app_config.h"
#include "app.h"
#include "app_cascam_pop.h"

#if NSCAS_SUPPORT
#define WND_ROLLING_R2L_TOP                         "ca_rolling_text_r2l_prompt" // in style.xml
#define PROMPT_ROLLING_R2L_TOP_NAME                 "prompt_ca_rolling_r2l"
#define TXT_ROLLING_R2L_TOP                         "txt_ca_rolling_r2l"

#define WND_ROLLING_R2L_BOTTOM                      "ca_rolling_text_r2l_prompt_1" // in style.xml
#define PROMPT_ROLLING_R2L_BOTTOM_NAME              "prompt_ca_rolling_r2l_1"
#define TXT_ROLLING_R2L_BOTTOM                      "txt_ca_rolling_r2l_1"

typedef enum _NSTVNOSDStyleEnum{
    ROLLING_TOP,
    ROLLING_BOTTOM,
    NOSD_FULLSCREEN,
    NOSD_HALFSCREEN,
    NSTV_NOSD_STYLE_TOTAL,
}NSTVNOSDStyleEnum;

typedef struct _NSTVNOSDMenuClass{
    char *prompt_name;
    char *text_name;
    unsigned int repeat_times;
    unsigned int duration;
    NSTVNOSDStyleEnum style;
}NSTVNOSDMenuClass;

static event_list  *thiz_query_repeat_timer[NSTV_NOSD_STYLE_TOTAL] = {NULL, NULL};
static event_list  *thiz_query_duration_timer[NSTV_NOSD_STYLE_TOTAL] = {NULL, NULL}; 
static unsigned int thiz_rolling_times[NSTV_NOSD_STYLE_TOTAL] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
static NSTVNOSDMenuClass thiz_menu_info[NSTV_NOSD_STYLE_TOTAL];

static void _record_rolling_times(unsigned int times, NSTVNOSDStyleEnum style)
{
    thiz_rolling_times[style] = times;
}

#if ROLLING_COMPLETE_EXIT
static unsigned int thiz_rolling_complete_exit[NSTV_NOSD_STYLE_TOTAL] = {0, 0, 0, 0};
static unsigned int thiz_rolling_times_exit[NSTV_NOSD_STYLE_TOTAL] = {0, 0, 0, 0};
static int _exit_rolling_osd(int style)
{
    int ret = 0;
    if((thiz_menu_info[style].prompt_name)
            && GUI_CheckPrompt(thiz_menu_info[style].prompt_name) == GXCORE_SUCCESS)
    {

        GUI_SetProperty(thiz_menu_info[style].text_name, "rolling_stop", NULL);
        GUI_EndPrompt(thiz_menu_info[style].prompt_name);
    }
    // timer
    if(thiz_query_repeat_timer[style])
    {
        remove_timer(thiz_query_repeat_timer[style]);
        thiz_query_repeat_timer[style] = NULL;
    }
    if(thiz_query_duration_timer[style])
    {
        remove_timer(thiz_query_duration_timer[style]);
        thiz_query_duration_timer[style] = NULL;
    }
    _record_rolling_times(0xffffffff, style);
    memset(&thiz_menu_info[style], 0, sizeof(NSTVNOSDMenuClass));
    thiz_rolling_complete_exit[style] = 0;
    thiz_rolling_times_exit[style] = 0;
    return ret;
}

static int _complete_exit_control(int style, unsigned int rolling_times)
{
    int ret = 0;
    if(0 == thiz_rolling_complete_exit[style])
    {
        return 0;
    }
    if(thiz_rolling_times_exit[style] == rolling_times)
    {
        _exit_rolling_osd(style);
    }
    return ret;
}
#endif


static int _query_repeat_timer(void *usrdata)
{
    unsigned int rolling_times = 0;
    NSTVNOSDMenuClass *data = (NSTVNOSDMenuClass*)usrdata;

    if(GUI_CheckPrompt(data->prompt_name) == GXCORE_SUCCESS)
    {
        GUI_GetProperty(data->text_name, "times", &rolling_times);
        _record_rolling_times(rolling_times, data->style);
    }
    if(rolling_times >= data->repeat_times)
    {
        cmm_cascam_exit_dialog3(data->style);
    }
#if ROLLING_COMPLETE_EXIT
    _complete_exit_control(data->style, rolling_times);
#endif
    return 0;
}

static int _create_repeat_timer(NSTVNOSDMenuClass *menu_info)
{
    if (reset_timer(thiz_query_repeat_timer[menu_info->style]) != 0) 
    {
        thiz_query_repeat_timer[menu_info->style] = create_timer(_query_repeat_timer, 100,  (void*)menu_info, TIMER_REPEAT);
    }
    return 0;
}

static int _query_duration_timer(void *usrdata)
{
    static char flag = 0;
    NSTVNOSDMenuClass *data = (NSTVNOSDMenuClass*)usrdata;
    if(0 == flag)
    {
        GUI_SetProperty(data->text_name, "state", "hide");
        flag = 1;
    }
    else
    {
        GUI_SetProperty(data->text_name, "state", "show");
        flag = 0;
    }
    return 0;
}

static int _create_duration_timer(NSTVNOSDMenuClass *menu_info)
{
    if (reset_timer(thiz_query_duration_timer[menu_info->style]) != 0) 
    {
        thiz_query_duration_timer[menu_info->style] = create_timer(_query_duration_timer, ((menu_info->duration)*1000), (void*)menu_info, TIMER_REPEAT);
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

    return 1;
    if((GXCORE_SUCCESS == GUI_CheckDialog("wnd_epg"))
            || (GXCORE_SUCCESS == GUI_CheckDialog("wnd_main_menu"))
            /*|| (GXCORE_SUCCESS == GUI_CheckDialog("wnd_channel_info"))*/) 
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

void app_nstv_osd_get_rolling_times(unsigned int *times, int style)
{
    if( (NULL == times) || (style > NSTV_NOSD_STYLE_TOTAL))
    {
        printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    *times = thiz_rolling_times[style];
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

void app_nstv_osd_check_rolling_osd(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    unsigned int x_pos = 0, y_pos = 0, width = 0, height = 0;
    if(GUI_CheckPrompt(PROMPT_ROLLING_R2L_TOP_NAME) == GXCORE_SUCCESS)
    {
        if(_get_rolling_osd_rect(&x_pos, &y_pos, &width, &height, PROMPT_ROLLING_R2L_TOP_NAME) == 0)
        {
            if(_check_rect_overlap(x, y, w, h, x_pos, y_pos, width, height))
            {
                //cmm_cascam_exit_dialog3(ROLLING_TOP);
            }
        }
    }

    if(GUI_CheckPrompt(PROMPT_ROLLING_R2L_BOTTOM_NAME) == GXCORE_SUCCESS)
    {
        if(_get_rolling_osd_rect(&x_pos, &y_pos, &width, &height, PROMPT_ROLLING_R2L_BOTTOM_NAME) == 0)
        {
            if(_check_rect_overlap(x, y, w, h, x_pos, y_pos, width, height))
            {
                //cmm_cascam_exit_dialog3(ROLLING_BOTTOM);
            }
        }
    }
}

#if 1
#define MAX_DISPLAY_SIZE (180*3 + 1)
typedef struct _RollingOSDControlClass{
    int flag; // 1: create; 0: destroy
    NSTVNOSDStyleEnum style;
    char string[MAX_DISPLAY_SIZE];
}RollingOSDControlClass;

static event_list  *thiz_create_exit_timer[NSTV_NOSD_STYLE_TOTAL] = {NULL, NULL, NULL, NULL};
static RollingOSDControlClass thiz_rolling_control[NSTV_NOSD_STYLE_TOTAL];

static void _rolling_osd_create(int pos, char* str)
{
#define  X_TOP_POS  40
#define  Y_TOP_POS  16
#define  X_BOTTOM_POS 40
#define  Y_BOTTOM_POS 516
    char *prompt_name = PROMPT_ROLLING_R2L_TOP_NAME;
    char *window_name = WND_ROLLING_R2L_TOP;
    char *text_name = TXT_ROLLING_R2L_TOP;
    unsigned int x_default_pos = X_TOP_POS;
    unsigned int y_default_pos = Y_TOP_POS;

    if(NULL == str)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return ;
    }
    if(0 == pos)// top
    {
        if(GUI_CheckPrompt(PROMPT_ROLLING_R2L_TOP_NAME) == GXCORE_SUCCESS)
        {
            GUI_SetProperty(TXT_ROLLING_R2L_TOP, "rolling_stop", NULL);
            GUI_EndPrompt(PROMPT_ROLLING_R2L_TOP_NAME);
        }
        x_default_pos = X_TOP_POS;
        y_default_pos = Y_TOP_POS;

    }
    else
    {
        if(GUI_CheckPrompt(PROMPT_ROLLING_R2L_BOTTOM_NAME) == GXCORE_SUCCESS)
        {
            GUI_SetProperty(TXT_ROLLING_R2L_BOTTOM, "rolling_stop", NULL);
            GUI_EndPrompt(PROMPT_ROLLING_R2L_BOTTOM_NAME);
        }
        x_default_pos = X_BOTTOM_POS;
        y_default_pos = Y_BOTTOM_POS;
        prompt_name = PROMPT_ROLLING_R2L_BOTTOM_NAME;
        window_name = WND_ROLLING_R2L_BOTTOM;
        text_name = TXT_ROLLING_R2L_BOTTOM;

    }

    if(_create_dialog_check(x_default_pos, y_default_pos, 1024, 30))
    {
        GUI_CreatePrompt(x_default_pos, y_default_pos, prompt_name, window_name, str,"ontop");
    }
    else
    {
        return ;
    }

    _record_rolling_times(0, pos);
#if ROLLING_COMPLETE_EXIT
    thiz_rolling_times_exit[pos] = 0;
    thiz_rolling_complete_exit[pos] = 0;
#endif
    thiz_menu_info[pos].prompt_name = prompt_name;
    thiz_menu_info[pos].text_name = text_name;
    // here, need create the repeat timer, for complete exit or create..., so
    // use the max value, 0xffffffff;
    thiz_menu_info[pos].repeat_times = 0xffffffff;
    thiz_menu_info[pos].duration = 0;
    thiz_menu_info[pos].style = pos;
    if(thiz_menu_info[pos].repeat_times > 0)
    {
        _create_repeat_timer(&thiz_menu_info[pos]);
    }

    if(thiz_menu_info[pos].duration > 0)
    {
        _create_duration_timer(&thiz_menu_info[pos]);
    }
}

static void _rolling_osd_exit(int style)
{
    #if ROLLING_COMPLETE_EXIT
    if(style >= 0)
    {
        thiz_rolling_complete_exit[style] = 1;
        if(thiz_rolling_times[style] != 0xffffffff)
        {
            thiz_rolling_times_exit[style] = thiz_rolling_times[style] + 1;
        }
        else
        {
            thiz_rolling_times_exit[style] = 1;
        }
    }
    else
    {
        int i = 0;
        for(i = 0; i < NSTV_NOSD_STYLE_TOTAL; i++)
        {
            thiz_rolling_complete_exit[i] = 1;
            if(thiz_rolling_times[i] != 0xffffffff)
            {
                thiz_rolling_times_exit[i] = thiz_rolling_times[i] + 1;
            }
            else
            {
                thiz_rolling_times_exit[i] = 1;
            }
        }
    }
#else
    if(style >= 0)
    {
        if((thiz_menu_info[style].prompt_name)
                && GUI_CheckPrompt(thiz_menu_info[style].prompt_name) == GXCORE_SUCCESS)
        {

            GUI_SetProperty(thiz_menu_info[style].text_name, "rolling_stop", NULL);
            GUI_EndPrompt(thiz_menu_info[style].prompt_name);
        }
        // timer
        if(thiz_query_repeat_timer[style])
        {
            remove_timer(thiz_query_repeat_timer[style]);
            thiz_query_repeat_timer[style] = NULL;
        }
        if(thiz_query_duration_timer[style])
        {
            remove_timer(thiz_query_duration_timer[style]);
            thiz_query_duration_timer[style] = NULL;
        }
        _record_rolling_times(0xffffffff, style);
        memset(&thiz_menu_info[style], 0, sizeof(NSTVNOSDMenuClass));
    }
    else
    {
        int i = 0;
        for(i = 0; i < NSTV_NOSD_STYLE_TOTAL; i++)
        {
            if((thiz_menu_info[i].prompt_name)
                    && GUI_CheckPrompt(thiz_menu_info[i].prompt_name) == GXCORE_SUCCESS)
            {
                GUI_SetProperty(thiz_menu_info[i].text_name, "rolling_stop", NULL);
                GUI_EndPrompt(thiz_menu_info[i].prompt_name);
            }
            // timer
            if(thiz_query_repeat_timer[i])
            {
                remove_timer(thiz_query_repeat_timer[i]);
                thiz_query_repeat_timer[i] = NULL;
            }
            if(thiz_query_duration_timer[i])
            {
                remove_timer(thiz_query_duration_timer[i]);
                thiz_query_duration_timer[i] = NULL;
            }
            _record_rolling_times(0xffffffff, i);
            memset(&thiz_menu_info[i], 0, sizeof(NSTVNOSDMenuClass));
        }
    }
#endif
}

static int _create_exit_control(void *usrdata)
{
    if(NULL == usrdata)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    RollingOSDControlClass *data = (RollingOSDControlClass*)usrdata;
    if(thiz_create_exit_timer[data->style])
    {
        remove_timer(thiz_create_exit_timer[data->style]);
        thiz_create_exit_timer[data->style] = NULL;
    }
    if(0 == data->flag)
    {// exit
       _rolling_osd_exit(data->style);
    }
    else
    {// create
        _rolling_osd_create(data->style, data->string);
    }
    return 0;
}

static int _create_exit_timer(RollingOSDControlClass *control_info)
{
    if(thiz_create_exit_timer[control_info->style])
    {
        remove_timer(thiz_create_exit_timer[control_info->style]);
        thiz_create_exit_timer[control_info->style] = NULL;
    }
    thiz_create_exit_timer[control_info->style] = create_timer(_create_exit_control, 1000, (void*)control_info, TIMER_REPEAT_NOW);
    return 0;
}

void app_nstv_rolling_osd_create(CmmOsdRollingClass *param)
{
    NSTVNOSDStyleEnum style = 0;

    if((NULL == param)||(NULL == param->str))
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }

    if(0 == param->pos)// top
    {
        style = ROLLING_TOP;
    }
    else
    {
        style = ROLLING_BOTTOM;
    }
    // check & remove timer
    if(thiz_query_repeat_timer[style])
    {
        remove_timer(thiz_query_repeat_timer[style]);
        thiz_query_repeat_timer[style] = NULL;
    }
    if(thiz_query_duration_timer[style])
    {
        remove_timer(thiz_query_duration_timer[style]);
        thiz_query_duration_timer[style] = NULL;
    }

    thiz_rolling_control[style].flag = 1;
    thiz_rolling_control[style].style = style;
    memset(thiz_rolling_control[style].string, 0, MAX_DISPLAY_SIZE);
    memcpy(thiz_rolling_control[style].string, param->str, strlen(param->str));
    _create_exit_timer(&thiz_rolling_control[style]);
}

void app_nstv_rolling_osd_exit(int style)
{
    if(style >= NSTV_NOSD_STYLE_TOTAL)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return ;
    }

    thiz_rolling_control[style].flag = 0;
    thiz_rolling_control[style].style = style;
    memset(thiz_rolling_control[style].string, 0, MAX_DISPLAY_SIZE);
    _create_exit_timer(&thiz_rolling_control[style]);
}

#else
void app_nstv_rolling_osd_create(CmmOsdRollingClass *param)
{
#define  X_TOP_POS  40
#define  Y_TOP_POS  16
#define  X_BOTTOM_POS 40
#define  Y_BOTTOM_POS 516
    char *prompt_name = PROMPT_ROLLING_R2L_TOP_NAME;
    char *window_name = WND_ROLLING_R2L_TOP;
    char *text_name = TXT_ROLLING_R2L_TOP;
    unsigned int x_default_pos = X_TOP_POS;
    unsigned int y_default_pos = Y_TOP_POS;
    NSTVNOSDStyleEnum style = 0;

    if((NULL == param)||(NULL == param->str))
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }

    if(0 == param->pos)// top
    {
        if(GUI_CheckPrompt(PROMPT_ROLLING_R2L_TOP_NAME) == GXCORE_SUCCESS)
        {
            GUI_SetProperty(TXT_ROLLING_R2L_TOP, "rolling_stop", NULL);
            GUI_EndPrompt(PROMPT_ROLLING_R2L_TOP_NAME);
        }
        x_default_pos = X_TOP_POS;
        y_default_pos = Y_TOP_POS;
        style = ROLLING_TOP;
    }
    else
    {
        if(GUI_CheckPrompt(PROMPT_ROLLING_R2L_BOTTOM_NAME) == GXCORE_SUCCESS)
        {
            GUI_SetProperty(TXT_ROLLING_R2L_BOTTOM, "rolling_stop", NULL);
            GUI_EndPrompt(PROMPT_ROLLING_R2L_BOTTOM_NAME);
        }
        x_default_pos = X_BOTTOM_POS;
        y_default_pos = Y_BOTTOM_POS;
        prompt_name = PROMPT_ROLLING_R2L_BOTTOM_NAME;
        window_name = WND_ROLLING_R2L_BOTTOM;
        text_name = TXT_ROLLING_R2L_BOTTOM;
        style = ROLLING_BOTTOM;
    }
    // check & remove timer
    if(thiz_query_repeat_timer[style])
    {
        remove_timer(thiz_query_repeat_timer[style]);
        thiz_query_repeat_timer[style] = NULL;
    }
    if(thiz_query_duration_timer[style])
    {
        remove_timer(thiz_query_duration_timer[style]);
        thiz_query_duration_timer[style] = NULL;
    }

    if((0 == param->x) && (0 == param->y))
    {
        if(_create_dialog_check(x_default_pos, y_default_pos, 1024, 30))
        {
            GUI_CreatePrompt(x_default_pos, y_default_pos, prompt_name, window_name, param->str, "ontop");
        }
        else
        {
            return ;
        }
    }
    else
    {
        if(_create_dialog_check(param->x, param->y, 1024, 30))
        {
            GUI_CreatePrompt(param->x, param->y, prompt_name, window_name, param->str, "ontop");
        }
        else
        {
            return ;
        }
    }

    _record_rolling_times(0, style);
#if ROLLING_COMPLETE_EXIT
    thiz_rolling_times_exit[style] = 0;
    thiz_rolling_complete_exit[style] = 0;
#endif
    thiz_menu_info[style].prompt_name = prompt_name;
    thiz_menu_info[style].text_name = text_name;
    thiz_menu_info[style].repeat_times = param->repeat_times;
    thiz_menu_info[style].duration = param->duration;
    thiz_menu_info[style].style = style;
    if(param->repeat_times > 0)
    {
        _create_repeat_timer(&thiz_menu_info[style]);
    }

    if(param->duration > 0)
    {
        _create_duration_timer(&thiz_menu_info[style]);
    }
}

void app_nstv_rolling_osd_exit(int style)
{
    if(style >= NSTV_NOSD_STYLE_TOTAL)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return ;
    }

#if ROLLING_COMPLETE_EXIT
    if(style >= 0)
    {
        thiz_rolling_complete_exit[style] = 1;
        if(thiz_rolling_times[style] != 0xffffffff)
        {
            thiz_rolling_times_exit[style] = thiz_rolling_times[style] + 1;
        }
        else
        {
            thiz_rolling_times_exit[style] = 1;
        }
    }
    else
    {
        int i = 0;
        for(i = 0; i < NSTV_NOSD_STYLE_TOTAL; i++)
        {
            thiz_rolling_complete_exit[i] = 1;
            if(thiz_rolling_times[i] != 0xffffffff)
            {
                thiz_rolling_times_exit[i] = thiz_rolling_times[i] + 1;
            }
            else
            {
                thiz_rolling_times_exit[i] = 1;
            }
        }
    }
#else
    if(style >= 0)
    {
        if((thiz_menu_info[style].prompt_name)
                && GUI_CheckPrompt(thiz_menu_info[style].prompt_name) == GXCORE_SUCCESS)
        {

            GUI_SetProperty(thiz_menu_info[style].text_name, "rolling_stop", NULL);
            GUI_EndPrompt(thiz_menu_info[style].prompt_name);
        }
        // timer
        if(thiz_query_repeat_timer[style])
        {
            remove_timer(thiz_query_repeat_timer[style]);
            thiz_query_repeat_timer[style] = NULL;
        }
        if(thiz_query_duration_timer[style])
        {
            remove_timer(thiz_query_duration_timer[style]);
            thiz_query_duration_timer[style] = NULL;
        }
        _record_rolling_times(0xffffffff, style);
        memset(&thiz_menu_info[style], 0, sizeof(NSTVNOSDMenuClass));
    }
    else
    {
        int i = 0;
        for(i = 0; i < NSTV_NOSD_STYLE_TOTAL; i++)
        {
            if((thiz_menu_info[i].prompt_name)
                    && GUI_CheckPrompt(thiz_menu_info[i].prompt_name) == GXCORE_SUCCESS)
            {
                GUI_SetProperty(thiz_menu_info[i].text_name, "rolling_stop", NULL);
                GUI_EndPrompt(thiz_menu_info[i].prompt_name);
            }
            // timer
            if(thiz_query_repeat_timer[i])
            {
                remove_timer(thiz_query_repeat_timer[i]);
                thiz_query_repeat_timer[i] = NULL;
            }
            if(thiz_query_duration_timer[i])
            {
                remove_timer(thiz_query_duration_timer[i]);
                thiz_query_duration_timer[i] = NULL;
            }
            _record_rolling_times(0xffffffff, i);
            memset(&thiz_menu_info[i], 0, sizeof(NSTVNOSDMenuClass));
        }
    }
#endif
}
#endif

void app_nstv_rolling_osd_show(int style)
{
    if(style >= NSTV_NOSD_STYLE_TOTAL)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return ;
    }
    if(style >= 0)
    {
        if((thiz_menu_info[style].prompt_name)
                && GUI_CheckPrompt(thiz_menu_info[style].prompt_name) == GXCORE_SUCCESS)
        {
            GUI_SetProperty(thiz_menu_info[style].text_name, "continue_rolling", NULL);
            GUI_SetProperty(thiz_menu_info[style].prompt_name, "state", "show");
        }
        // timer
        if(thiz_query_repeat_timer[style])
        {
            reset_timer(thiz_query_repeat_timer[style]);
        }
        if(thiz_query_duration_timer[style])
        {
            reset_timer(thiz_query_duration_timer[style]);
        }
    }
    else
    {
        int i = 0;
        for(i = 0; i < NSTV_NOSD_STYLE_TOTAL; i++)
        {
            if((thiz_menu_info[i].prompt_name)
                    && GUI_CheckPrompt(thiz_menu_info[i].prompt_name) == GXCORE_SUCCESS)
            {
                GUI_SetProperty(thiz_menu_info[i].text_name, "continue_rolling", NULL);
                GUI_SetProperty(thiz_menu_info[i].prompt_name, "state", "show");
            }
            // timer
            if(thiz_query_repeat_timer[i])
            {
                reset_timer(thiz_query_repeat_timer[i]);
            }
            if(thiz_query_duration_timer[i])
            {
                reset_timer(thiz_query_duration_timer[i]);
            }
        }
    }
}

void app_nstv_rolling_osd_hide(int style)
{
    if(style >= NSTV_NOSD_STYLE_TOTAL)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return ;
    }
    if(style >= 0)
    {
        if((thiz_menu_info[style].prompt_name)
                && GUI_CheckPrompt(thiz_menu_info[style].prompt_name) == GXCORE_SUCCESS)
        {
            GUI_SetProperty(thiz_menu_info[style].text_name, "rolling_stop", NULL);
            GUI_SetProperty(thiz_menu_info[style].prompt_name, "state", "hide");
        }
        // timer
        if(thiz_query_repeat_timer[style])
        {
            timer_stop(thiz_query_repeat_timer[style]);
        }
        if(thiz_query_duration_timer[style])
        {
            timer_stop(thiz_query_duration_timer[style]);
        }
    }
    else
    {
        int i = 0;
        for(i = 0; i < NSTV_NOSD_STYLE_TOTAL; i++)
        {
            if((thiz_menu_info[i].prompt_name)
                    && GUI_CheckPrompt(thiz_menu_info[i].prompt_name) == GXCORE_SUCCESS)
            {
                GUI_SetProperty(thiz_menu_info[i].text_name, "rolling_stop", NULL);
                GUI_SetProperty(thiz_menu_info[i].prompt_name, "state", "hide");
            }
            // timer
            if(thiz_query_repeat_timer[i])
            {
                timer_stop(thiz_query_repeat_timer[i]);
            }
            if(thiz_query_duration_timer[i])
            {
                timer_stop(thiz_query_duration_timer[i]);
            }
        }
    }
}

#endif
