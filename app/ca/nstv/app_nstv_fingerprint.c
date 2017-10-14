/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_fingerprint.c
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
#include <math.h>

#if NSCAS_SUPPORT

static char thiz_fingerprint_content[50] = {0};
/****************************************************************************
stage1: 不规则线条显示。
stage2：显示字符串（卡号），随机，2分钟
stage3: 在屏幕左下方显示实时时间“Time xx:xx”， 10个组合，一个1S
        黑底绿字，黑底白字，黑底淡蓝字，黑底红字，黑底黄字
        蓝底绿字，蓝底白字，蓝底淡蓝字，蓝底红字，蓝底黄字
stage4：在屏幕上显示CA号码，显示要像水印一样，要肉眼不好看出来，显示10分钟，重
        新进入stage1。
****************************************************************************/
#define PROMPT_STAGE1_LINE1_NAME        "prompt_ca_line10"
#define WND_STAGE1_LINE1                "ca_line10_prompt"
#define PROMPT_STAGE1_LINE2_NAME        "prompt_ca_line15"
#define WND_STAGE1_LINE2                "ca_line15_prompt"
#define PROMPT_STAGE1_LINE3_NAME        "prompt_ca_line20"
#define WND_STAGE1_LINE3                "ca_line20_prompt"
#define PROMPT_STAGE1_LINE4_NAME        "prompt_ca_line25"
#define WND_STAGE1_LINE4                "ca_line25_prompt"
#define PROMPT_STAGE1_LINE5_NAME        "prompt_ca_line30"
#define WND_STAGE1_LINE5                "ca_line30_prompt"
#define PROMPT_STAGE1_LINE6_NAME        "prompt_ca_line35"
#define WND_STAGE1_LINE6                "ca_line35_prompt"
#define PROMPT_STAGE1_LINE7_NAME        "prompt_ca_line40"
#define WND_STAGE1_LINE7                "ca_line40_prompt"

#define PROMPT_STAGE2_NAME          "prompt_ca_fingerprint"
#define WND_STAGE2                  "ca_fingerprint_prompt"
#define TXT_STAGE2                  "txt_fingerprint"
#define PROMPT_STAGE3_NAME          "prompt_ca_fingerprint"
#define WND_STAGE3                  "ca_fingerprint_prompt"
#define TXT_STAGE3                  "txt_fingerprint"
//#define PROMPT_STAGE4_NAME          "prompt_ca_fingerprint"
//#define WND_STAGE4                  "ca_fingerprint_prompt"
//#define TXT_STAGE4                  "txt_fingerprint"
#define PROMPT_STAGE4_NAME          "prompt_ca_finger_water_mark"
#define WND_STAGE4                  "ca_finger_water_mark_prompt"
#define TXT_STAGE4                  "txt_finger_water_mark"

static void _generat_random_pos(unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h,
                                  unsigned int *b_x, unsigned int *b_y, unsigned int *b_w, unsigned int *b_h)
{
// 区域控制，排除ROLLING信息的影响
#define SCREEN_WIDTH                860
#define SCREEN_HEIGHT               420
#define SCREEN_HORIZONTAL_OFFSET    40
#define SCREEN_VERTICAL_OFFSET      78

#define CENTRE_RECT_LX 149
#define CENTRE_RECT_RX 701
#define CENTRE_RECT_TY 134
#define CENTRE_RECT_BY 286

    unsigned int fingerprint_width = 0;
    unsigned int fingerprint_height = 0;
    unsigned int tmp_x = 0;
    unsigned int tmp_y = 0;
    GuiWidget *widget = NULL;
    widget = gui_get_window("prompt_ca_fingerprint");
    if(widget == NULL)
    {
        printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }

    fingerprint_width = widget->rect.w;
    fingerprint_height = widget->rect.h;

    tmp_x = (rand()%(SCREEN_WIDTH - fingerprint_width - SCREEN_HORIZONTAL_OFFSET)) + SCREEN_HORIZONTAL_OFFSET;
    tmp_y = (rand()%(SCREEN_HEIGHT - fingerprint_height - SCREEN_VERTICAL_OFFSET)) + SCREEN_VERTICAL_OFFSET;

    if(tmp_y > CENTRE_RECT_TY && tmp_y < CENTRE_RECT_BY && tmp_x > CENTRE_RECT_LX && tmp_y < CENTRE_RECT_RX)
    {
        if(tmp_y > (SCREEN_HEIGHT - fingerprint_height)/2)
        {
            tmp_y = (rand()%(SCREEN_HEIGHT - CENTRE_RECT_BY)) + CENTRE_RECT_BY;
        }
        else
        {
            tmp_y = (rand()%(CENTRE_RECT_TY - SCREEN_HORIZONTAL_OFFSET)) + SCREEN_HORIZONTAL_OFFSET;
        }
    }

    *x = tmp_x;
    *y = tmp_y;
    *w = fingerprint_width;
    *h = fingerprint_height;

    *b_x =  widget->rect.x;
    *b_y =  widget->rect.y;
    *b_w =  widget->rect.w;
    *b_h =  widget->rect.h;
}

int nstv_stage1_func(void);
int nstv_stage1_stop(void);
int nstv_stage2_func(void);
int nstv_stage2_stop(void);
int nstv_stage3_func(void);
int nstv_stage3_stop(void);
int nstv_stage4_func(void);
int nstv_stage4_stop(void);

typedef struct _FingerFuncClass{
    int (*StageFunc) (void);
}FingerFuncClass;

static FingerFuncClass thiz_stage_func[4] ={
    {nstv_stage2_func},
    {nstv_stage3_func},
    {nstv_stage4_func},
    {nstv_stage1_func},
};

static event_list  *thiz_query_move_timer = NULL;

static int _destroy_move_timer(void)
{
    int ret = 0;
    if(thiz_query_move_timer != NULL)
    {
        remove_timer(thiz_query_move_timer);
        thiz_query_move_timer = NULL;
    }
    return ret;
}

static int _query_move_timer(void *usrdata)
{
    int index = *(int*)usrdata;

    _destroy_move_timer();
    if(thiz_stage_func[index].StageFunc)
    {
        thiz_stage_func[index].StageFunc();
    }

    return 0;
}

static int _create_move_timer(unsigned int duration_sec, int index)
{
    static int timer_index = 0;
    timer_index = index;
    if(thiz_query_move_timer)
    {
        remove_timer(thiz_query_move_timer);
        thiz_query_move_timer = NULL;
    }
    //if (reset_timer(thiz_query_move_timer) != 0)
    {
        thiz_query_move_timer = create_timer(_query_move_timer, duration_sec*1000, (void*)&timer_index, TIMER_REPEAT);
    }
    return 0;
}

static event_list  *thiz_stage_timer = NULL;

// stage1

#define MAX_STAGE1_LINE  7
typedef struct _Stage1LineClass{
    unsigned int x;
    unsigned int y;
    char *prompt_name;
    char *widget_name;
}Stage1LineClass;

static Stage1LineClass thiz_stage1_control[MAX_STAGE1_LINE] ={
    {500, 80, PROMPT_STAGE1_LINE1_NAME, WND_STAGE1_LINE1},
    {400, 100, PROMPT_STAGE1_LINE2_NAME, WND_STAGE1_LINE2},
    {450, 90, PROMPT_STAGE1_LINE3_NAME, WND_STAGE1_LINE3},
    {550, 140, PROMPT_STAGE1_LINE4_NAME, WND_STAGE1_LINE4},
    {550, 140, PROMPT_STAGE1_LINE5_NAME, WND_STAGE1_LINE5},
    {400, 100, PROMPT_STAGE1_LINE6_NAME, WND_STAGE1_LINE6},
    {500, 80, PROMPT_STAGE1_LINE7_NAME, WND_STAGE1_LINE7},
};
static unsigned int thiz_stage1_index = 0;

static int _stage1_display(void)
{
    if(GUI_CheckPrompt(thiz_stage1_control[thiz_stage1_index].prompt_name) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(thiz_stage1_control[thiz_stage1_index].prompt_name);
    }
    thiz_stage1_index = rand()%MAX_STAGE1_LINE;
    GUI_SetInterface("flush", NULL);
    GUI_CreatePrompt(thiz_stage1_control[thiz_stage1_index].x,
                        thiz_stage1_control[thiz_stage1_index].y,
                        thiz_stage1_control[thiz_stage1_index].prompt_name,
                        thiz_stage1_control[thiz_stage1_index].widget_name,
                        NULL,
                        "ontop");
    return 0;
}

static int _stage2_display(void)
{
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int w = 0;
    unsigned int h = 0;
    unsigned int b_x = 0;
    unsigned int b_y = 0;
    unsigned int b_w = 0;
    unsigned int b_h = 0;

    char buf[50] = {0};

    _generat_random_pos(&x, &y, &w, &h, &b_x, &b_y, &b_w, &b_h);
    sprintf(buf, "[%d,%d,%d,%d]", x, y, w, h);
    //printf("\nSTAGE2, x=%d, y=%d, w=%d, h=%d, bx=%d, by=%d, bw=%d, bh=%d\n", x,y,w,h,b_x, b_y, b_w, b_h);
    //GUI_SetProperty(PROMPT_STAGE2_NAME, "rect", buf);
    if(GUI_CheckPrompt(PROMPT_STAGE2_NAME) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(PROMPT_STAGE2_NAME);
    }
    GUI_CreatePrompt(x, y, PROMPT_STAGE2_NAME, WND_STAGE2, NULL, "ontop");

    return 0;
}

static unsigned char thiz_time_index = 0;// range from 0 to 9
typedef struct _Stage3Class{
    char *back_color;
    char *fore_color;
}Stage3Class;
static Stage3Class thiz_stage3_color[10] ={
    {"[#000001,#000001,#000001]", "[#4df736,#4df736,#4df736]"},
    {"[#000001,#000001,#000001]", "[#f8f8f8,#f8f8f8,#f8f8f8]"},
    {"[#000001,#000001,#000001]", "[#9feef9,#9feef9,#9feef9]"},
    {"[#000001,#000001,#000001]", "[#f50a10,#f50a10,#f50a10]"},
    {"[#000001,#000001,#000001]", "[#f2dd40,#f2dd40,#f2dd40]"},
    {"[#151a7f,#151a7f,#151a7f]", "[#4df736,#4df736,#4df736]"},
    {"[#151a7f,#151a7f,#151a7f]", "[#f8f8f8,#f8f8f8,#f8f8f8]"},
    {"[#151a7f,#151a7f,#151a7f]", "[#9feef9,#9feef9,#9feef9]"},
    {"[#151a7f,#151a7f,#151a7f]", "[#f50a10,#f50a10,#f50a10]"},
    {"[#151a7f,#151a7f,#151a7f]", "[#f2dd40,#f2dd40,#f2dd40]"},
};
#define STAGE_CLEAR_BACK_COLOR    "[#ff00ff,#ff00ff,#4F4F4F]"
#define STAGE_CLEAR_FORE_COLOR    "[#f1f1f1,#f1f1f1,#f1f1f1]"
#define STAGE_WATER_MARK_FORE_COLOR "[#06f4f4f4,#06f4f4f4,#06f1f1f1]"
//#define STAGE_CLEAR_FORE_COLOR    "[#08ff08,#08ff08,#08ff08]"
static int _get_current_time(unsigned short *year,
                                unsigned char *month,
                                unsigned char *day,
                                unsigned char *hour,
                                unsigned char *minute,
                                unsigned char *second)
{
extern AppTime g_AppTime;
    GxTime sys_time;
    time_t seconds;
    struct tm *_tm = NULL;

    GxCore_GetLocalTime(&sys_time);

    if(g_AppTime.config.zone < 0)
    {
        seconds = sys_time.seconds - (unsigned int)(fabs(g_AppTime.config.zone)*3600);
    }
    else
    {
        seconds = sys_time.seconds + (unsigned int)(fabs(g_AppTime.config.zone)*3600);
    }
    seconds += g_AppTime.config.summer*3600;
    _tm = localtime(&(seconds));
    if(NULL == _tm)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    if(NULL != year)
    {
        *year = _tm->tm_year;
    }
    if(NULL != month)
    {
        *month = _tm->tm_mon;
    }
    if(NULL != day)
    {
        *day = _tm->tm_mday;
    }
    if(NULL != hour)
    {
        *hour = _tm->tm_hour;
    }
    if(NULL != minute)
    {
        *minute = _tm->tm_min;
    }
    if(NULL != second)
    {
        *second = _tm->tm_sec;
    }
    return 0;
}

static int _stage3_display(void)
{
    char buf[20] = {0};
    unsigned char minute = 0;
    unsigned char second = 0;
    int ret = 0;

    ret = _get_current_time(NULL, NULL, NULL, NULL, &minute, &second);
    if((ret == 0) && (thiz_time_index < 10))
    {
        sprintf(buf, "Time %02d:%02d", minute, second);
        GUI_SetProperty(TXT_STAGE3, "backcolor", thiz_stage3_color[thiz_time_index].back_color);
        GUI_SetProperty(TXT_STAGE3, "forecolor", thiz_stage3_color[thiz_time_index].fore_color);
        GUI_SetProperty(TXT_STAGE3, "string", buf);
        thiz_time_index++;
    }
    return 0;
}

static char thiz_stage4_count = 0;
static int _stage4_display(void)
{
    static char flag = 0;
    if(flag == 0)
    {
        if(thiz_stage4_count++ > 2)
        {
            GUI_SetProperty(PROMPT_STAGE4_NAME, "state", "hide");
            flag = 1;
            thiz_stage4_count = 0;
        }
    }
    else
    {
        if(thiz_stage4_count++ > 30)
        {
            GUI_SetProperty(PROMPT_STAGE4_NAME, "state", "show");
            flag = 0;
            thiz_stage4_count = 0;
        }
    }
    return 0;
}

static int _stage_display_timer(void *usrdata)
{
    char index = *(char*)usrdata;
    if(index == 0)// stage 1
    {
        _stage1_display();
    }
    else if(index == 1)// stage 2
    {
        _stage2_display();
    }
    else if(index == 2)// stage 3
    {
        _stage3_display(); 
    }
    else // stage 4
    {
        _stage4_display();
    }
    return 0;
}

static int _destroy_stage_timer(void)
{
    int ret = 0;
    if(thiz_stage_timer != NULL)
    {
        remove_timer(thiz_stage_timer);
        thiz_stage_timer = NULL;
    }
    return ret;
}

static int _create_stage_timer(unsigned int duration_ms, char index)
{
    static char timer_index = 0;
    timer_index = index;
    if(thiz_stage_timer)
    {
        remove_timer(thiz_stage_timer);
        thiz_stage_timer = NULL;
    }
    //if (reset_timer(thiz_stage_timer) != 0)
    {
        thiz_stage_timer = create_timer(_stage_display_timer, duration_ms, (void*)&timer_index, TIMER_REPEAT);
    }
    return 0;
}

int nstv_stage1_func(void)
{
    int ret = 0;
    _destroy_move_timer();
    _destroy_stage_timer();
    nstv_stage4_stop();
    //GUI_CreatePrompt(X_POS, Y_POS, PROMPT_STAGE1_NAME, WND_STAGE1, NULL, "ontop");
    thiz_stage1_index = rand()%MAX_STAGE1_LINE;
    GUI_SetInterface("flush", NULL);
    GUI_CreatePrompt(thiz_stage1_control[thiz_stage1_index].x,
                        thiz_stage1_control[thiz_stage1_index].y,
                        thiz_stage1_control[thiz_stage1_index].prompt_name,
                        thiz_stage1_control[thiz_stage1_index].widget_name,
                        NULL,
                        "ontop");
    _create_move_timer(4, 0);
    _create_stage_timer(100,0);
    return ret;
}
int nstv_stage1_stop(void)
{
    if(GUI_CheckPrompt(thiz_stage1_control[thiz_stage1_index].prompt_name) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(thiz_stage1_control[thiz_stage1_index].prompt_name);
        //GUI_SetInterface("flush", NULL);
    }
    return 0;
}
// stage2 function
int nstv_stage2_func(void)
{
    unsigned int x = 100;
    unsigned int y = 100;

    //_generat_random_pos(&x, &y);

    _destroy_move_timer();
    _destroy_stage_timer();
    nstv_stage1_stop();
    GUI_SetInterface("flush", NULL);
    GUI_CreatePrompt(x, y, PROMPT_STAGE2_NAME, WND_STAGE2, thiz_fingerprint_content, "ontop");
    GUI_SetProperty(TXT_STAGE2, "backcolor", STAGE_CLEAR_BACK_COLOR);
    GUI_SetProperty(TXT_STAGE2, "forecolor", STAGE_CLEAR_FORE_COLOR);

#if (NSTV_AUTHENTICATION_SUPPORT == 0)
    _create_move_timer(120, 1);// 2 minite
#endif
    _create_stage_timer(1000,1);
    return 0;
}
int nstv_stage2_stop(void)
{
    if(GUI_CheckPrompt(PROMPT_STAGE2_NAME) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(PROMPT_STAGE2_NAME);
       // GUI_SetInterface("flush", NULL);
    }
    return 0;
}
// stage3 function
int nstv_stage3_func(void)
{
    unsigned int x = 100;
    unsigned int y = 450;

    _destroy_move_timer();
    _destroy_stage_timer();
    nstv_stage2_stop();
    thiz_time_index = 0;
    GUI_SetInterface("flush", NULL);
    GUI_CreatePrompt(x, y, PROMPT_STAGE3_NAME, WND_STAGE3, NULL, "ontop");
    _stage3_display();
    _create_move_timer(10, 2);// 10s
    _create_stage_timer(1000,2);
    return 0;
}
int nstv_stage3_stop(void)
{
    if(GUI_CheckPrompt(PROMPT_STAGE3_NAME) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(PROMPT_STAGE3_NAME);
        //GUI_SetInterface("flush", NULL);
    }
    thiz_time_index = 0;

    return 0;
}
// stage4 function
int nstv_stage4_func(void)
{
    unsigned int x = 60;
    unsigned int y = 200;
    int i = 0;
    int j = 0;
    unsigned int len = strlen(thiz_fingerprint_content);
    char * buff = GxCore_Mallocz(len*4 + 1);
    if(NULL == buff)
    {
        printf("\nNSTV, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < len; i++)
    {
       buff[j++] = thiz_fingerprint_content[i];
       buff[j++] = ' ';
       buff[j++] = ' ';
       buff[j++] = ' ';
    }

    _destroy_move_timer();
    _destroy_stage_timer();
    nstv_stage3_stop();
    GUI_SetInterface("flush", NULL);
    GUI_CreatePrompt(x, y, PROMPT_STAGE4_NAME, WND_STAGE4, buff/*thiz_fingerprint_content*/, "ontop");
    GxCore_Free(buff);
    //GUI_SetProperty(TXT_STAGE4, "backcolor", STAGE_CLEAR_BACK_COLOR);
    GUI_SetProperty(TXT_STAGE4, "forecolor", STAGE_WATER_MARK_FORE_COLOR);

    _create_move_timer(600, 3);// 600s
    //_create_stage_timer(500,3);
    return 0;
}
int nstv_stage4_stop(void)
{
    if(GUI_CheckPrompt(PROMPT_STAGE4_NAME) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(PROMPT_STAGE4_NAME);
        //GUI_SetInterface("flush", NULL);
    }
    thiz_stage4_count = 0;
    return 0;
}

static int _create_dialog_check(void)
{
    if((GXCORE_SUCCESS == GUI_CheckDialog("wnd_epg"))
            || (GXCORE_SUCCESS == GUI_CheckDialog("wnd_main_menu"))
            /*|| (GXCORE_SUCCESS == GUI_CheckDialog("wnd_channel_info_signal"))*/) 
    {
        return 0;
    }
    return 1;
}


static event_list  *thiz_finger_timer = NULL;
static char thiz_finger_create_status = 0;// 1: create; 0:destroy
static int _finger_timer(void *usrdata)
{
    remove_timer(thiz_finger_timer);
    thiz_finger_timer = NULL;
    if(thiz_finger_create_status)
    {// create
        _destroy_move_timer();
        _destroy_stage_timer();
#if NSTV_AUTHENTICATION_SUPPORT
        nstv_stage2_func();
#else
        nstv_stage1_stop();
        nstv_stage2_stop();
        nstv_stage3_stop();
        nstv_stage4_stop();
        // start
        nstv_stage1_func();
#endif
    }
    else
    {
        _destroy_move_timer();
        _destroy_stage_timer();
        nstv_stage1_stop();
        nstv_stage2_stop();
        nstv_stage3_stop();
        nstv_stage4_stop();
    }
    return 0;
}

static int _create_finger_timer(char flag)
{
    thiz_finger_create_status = flag;
    if(thiz_finger_timer)
    {
        remove_timer(thiz_finger_timer);
        thiz_finger_timer = NULL;
    }
    //if (reset_timer(thiz_finger_timer) != 0)
    {
        thiz_finger_timer = create_timer(_finger_timer, 1000, NULL, TIMER_REPEAT_NOW);
    }
    return 0;
}

void app_nstv_fingerprint_create(CmmFingerprintClass *para)
{
    unsigned int len = 0;
    if(NULL == para)
    {
        printf("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }

    if(0 == _create_dialog_check())
    {// not create
        return ;
    }
    if((NULL == para->fingerContent.string.content)
            || (0 == strlen(para->fingerContent.string.content)))
    {
        printf("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }
    len = strlen(para->fingerContent.string.content);
    if(len > sizeof(thiz_fingerprint_content))
    {
        printf("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }

    if(strncmp(thiz_fingerprint_content, para->fingerContent.string.content, len) == 0)
    {
        // same,
        return ;
    }
    //_destroy_move_timer();
    //_destroy_stage_timer();
    //nstv_stage1_stop();
    //nstv_stage2_stop();
    //nstv_stage3_stop();
    //nstv_stage4_stop();

    memset(thiz_fingerprint_content, 0, sizeof(thiz_fingerprint_content));
    memcpy(thiz_fingerprint_content, para->fingerContent.string.content, len);
    // start
    //nstv_stage1_func();
    _create_finger_timer(1);
}

void app_nstv_fingerprint_exit(CmmFingerTypeEnum type)
{
    //_destroy_move_timer();
    //_destroy_stage_timer();
    //nstv_stage1_stop();
    //nstv_stage2_stop();
    //nstv_stage3_stop();
    //nstv_stage4_stop();
    memset(thiz_fingerprint_content, 0, sizeof(thiz_fingerprint_content));
    _create_finger_timer(0);
}

static int _get_water_mark_rect(unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h, char *name)
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
    if((((x + w) < x1)
            || ((y + h) < y1)
            || (y > (y1 + h1))
            || (x > (x1 + w1))))
    {
        return 0;// no overlap
    }
    else
    {
        if((w < 1024) && (h < 576))
        {
            return 1;// operlap
        }
        else
        {
            return -1;
        }
    }
}

void app_nstv_osd_check_for_hide(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    unsigned int x_pos = 0, y_pos = 0, width = 0, height = 0;
    int ret = 0;
    if((GUI_CheckPrompt(PROMPT_STAGE1_LINE1_NAME) == GXCORE_SUCCESS)
            || (GUI_CheckPrompt(PROMPT_STAGE1_LINE2_NAME) == GXCORE_SUCCESS)
            || (GUI_CheckPrompt(PROMPT_STAGE1_LINE3_NAME) == GXCORE_SUCCESS)
            || (GUI_CheckPrompt(PROMPT_STAGE1_LINE4_NAME) == GXCORE_SUCCESS)
            || (GUI_CheckPrompt(PROMPT_STAGE1_LINE5_NAME) == GXCORE_SUCCESS)
            || (GUI_CheckPrompt(PROMPT_STAGE1_LINE6_NAME) == GXCORE_SUCCESS)
            || (GUI_CheckPrompt(PROMPT_STAGE1_LINE7_NAME) == GXCORE_SUCCESS)
            || (GUI_CheckPrompt(PROMPT_STAGE2_NAME) == GXCORE_SUCCESS)
            || (GUI_CheckPrompt(PROMPT_STAGE3_NAME) == GXCORE_SUCCESS)
            || (GUI_CheckPrompt(PROMPT_STAGE4_NAME) == GXCORE_SUCCESS))
    {
        if((ret = _check_rect_overlap(x, y, w, h, x_pos, y_pos, width, height)) == 0)
        {
            if(_get_water_mark_rect(&x_pos, &y_pos, &width, &height, PROMPT_STAGE4_NAME) == 0)
            {
                //_destroy_move_timer();
                //nstv_stage4_stop();
            }
        }
        else if(-1 == ret)
        {
            _destroy_move_timer();
            _destroy_stage_timer();
            nstv_stage1_stop();
            nstv_stage2_stop();
            nstv_stage3_stop();
            nstv_stage4_stop();
            memset(thiz_fingerprint_content, 0, sizeof(thiz_fingerprint_content));
        }
    }
}

void app_nstv_fingerprint_water_mark_reshow(void)
{
    if(GUI_CheckPrompt(PROMPT_STAGE4_NAME) == GXCORE_SUCCESS)
    {
        //nstv_stage4_func();
    }
}
#endif
