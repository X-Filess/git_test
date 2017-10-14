/*
 * :i
 * =====================================================================================
 *
 *       Filename:  app_pvr_bar.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2011年09月21日 11时16分35秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app_module.h"
#include "app_send_msg.h"
#include "app.h"
#include "app_pop.h"
#include "full_screen.h"
#include "app_default_params.h"
#include "app_epg.h"

//#include "module/app_ca_manager.h"


typedef enum
{
    PVR_SHOW,
    PVR_HIDE
}pvr_ui;

typedef enum
{
    PVR_PAUSE_ENABLE,
    PVR_PAUSE_DISABLE
}pvr_pause;

typedef enum
{
    PVR_TIMER_TICK,
    PVR_TIMER_SLIDER,
    PVR_TIMER_REC_END,
    PVR_TIMER_TOTAL
}pvr_timer_mode;

typedef struct _pvr_timer pvr_timer;
struct _pvr_timer
{
    event_list      *timer[PVR_TIMER_TOTAL];
    uint32_t        interval[PVR_TIMER_TOTAL];
    TIMER_FLAG      flag[PVR_TIMER_TOTAL];
    int             (*cb[PVR_TIMER_TOTAL])(void*);
    void    (*init)(pvr_timer*);
    void    (*release)(pvr_timer*);
    void    (*start)(pvr_timer*, pvr_timer_mode);
};

typedef struct
{
    pvr_ui          ui;
    uint32_t        slider_refresh; // 0-stop refresh  1-timer refresh
    bool              shift_end;
    uint32_t        tick_refresh_delay;
    void            (*taxis)(uint32_t);
    void            (*state_change)(PlayerStatus);// ff fb pause play dummy
    pvr_timer       timer;
    bool            shift_start;
#define TAXIS_NONE  (0)
#define TAXIS_TP    (1)
}AppPvrUi;

static void _get_pvr_duration_from_event(void);
static int pvr_timer_rec_duration(void *usrdata);
void app_pvr_patition_full_proc(void);

extern void app_find_old_info_change(void);
//extern int app_get_channel_list_mode(void);
extern AppPvrUi g_AppPvrUi;
static time_t pvr_duration = 0;
static event_list* sp_PvrBarTimer = NULL;
static bool s_pvr_usbin_flag = false;

#define WND_PVR_BAR     "wnd_pvr_bar"
#define TXT_NAME        "txt_pvr_prog_name"
#define TXT_PERCENT     "txt_pvr_percent"
#define IMG_PERCENT     "img_percent_"
#define IMG_STATE       "img_pvr_state"
#define TXT_STATE       "txt_pvr_state"
#define TXT_CUR_TICK    "txt_pvr_cur_time"
#define TXT_TOTAL_TICK  "txt_pvr_total_time"
#define TXT_SEEKMIN_TICK  "txt_pvr_seekmin_time"
#define SLIDER_BAR      "slider_pvr_process"
#define SLIDER_BAR_SEEK      "slider_pvr_seek"
#define EDIT_DURATION      "edit_duration_set"

#define TICK_DELAY (1)

#define SPD_NUM (5)
pvr_speed spd_val[SPD_NUM+1] = {PVR_SPD_1,PVR_SPD_2,PVR_SPD_4,PVR_SPD_8,PVR_SPD_16,PVR_SPD_32};


void app_set_pvr_usbin_flag(bool bflag)
{
	s_pvr_usbin_flag = bflag;
}

static bool app_check_pvr_usbin_flag()
{
	return s_pvr_usbin_flag;
}

static int _pvr_bar_timeout(void* data)
{
    if(sp_PvrBarTimer != NULL)
    {
        remove_timer(sp_PvrBarTimer);
        sp_PvrBarTimer = NULL;
    }
	GUI_EndDialog(WND_PVR_BAR);
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		app_create_dialog("wnd_channel_info");
	}
	return 0;
}

static void sectohour(time_t time, struct tm *tm_time)
{

    if(time < 0)
        time = 0;
    tm_time->tm_hour = (uint32_t)(time/3600);
    tm_time->tm_min = (uint32_t)((time - tm_time->tm_hour*3600)/60);
    tm_time->tm_sec = (uint32_t)(time%60);
    return;
}

static void pvr_taxis_mode(uint32_t mode)
{
    GxBusPmViewInfo view_info;
    GxMsgProperty_NodeByPosGet node;
    uint32_t prog_id = 0;

    // prog data get, tp id get
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
    prog_id = node.prog_data.id;

    // view info get
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, &view_info);

    // view info set
#if 0
    if (mode == TAXIS_NONE)
    {
        view_info.tp_id = 0;
        view_info.taxis_mode = TAXIS_MODE_NON;
    }
    else
    {
        view_info.tp_id = node.prog_data.tp_id;
        view_info.taxis_mode = TAXIS_MODE_TP;
    }
#endif
    if (mode == TAXIS_NONE)
    {
        view_info.pip_switch = 0;
        view_info.pip_main_tp_id = 0;
    }
    else
    {
        view_info.pip_switch = 1;
        view_info.pip_main_tp_id = node.prog_data.tp_id;
    }

    g_AppPlayOps.play_list_create(&view_info);
}

static int pvr_cur_total_tick_get(void)
{
    GxMsgProperty_GetPlayTime time = {0};
    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;

    if (pvr->state == PVR_DUMMY)
    {
        return 0;
    }
    else if((pvr->state == PVR_RECORD) || (pvr->state == PVR_TP_RECORD))
    {
        time.player_name = (int8_t*)PLAYER_FOR_REC;
    }
    else
    {
        // pvr || pvr&tms
        time.player_name = (int8_t*)PLAYER_FOR_TIMESHIFT;
    }

    if(GXCORE_SUCCESS
            == app_send_msg_exec(GXMSG_GET_MEDIA_TIME,(void *)&time))
    {
#ifdef LOOP_TMS_SUPPORE
		pvr->time.seekmin_tick = (uint32_t)(time.seekmin_time/SEC_TO_MILLISEC);
		pvr->time.total_tick = (uint32_t)(time.total_time/SEC_TO_MILLISEC)-pvr->time.seekmin_tick;
		if(arb->state.pause == STATE_OFF)
        {
            pvr->time.cur_tick = (uint32_t)(time.cur_time/SEC_TO_MILLISEC)-pvr->time.seekmin_tick;
        }

#else
		pvr->time.total_tick = (uint32_t)(time.total_time/SEC_TO_MILLISEC);
		if(arb->state.pause == STATE_OFF)
        {
            pvr->time.cur_tick = (uint32_t)(time.cur_time/SEC_TO_MILLISEC);
        }
#endif
    }
    else
    {
    	return -1;	
    }

    if((pvr->time.total_tick > 0) && (pvr->time.total_tick > pvr->time.cur_tick))
    {
        pvr->time.remaining_tick = pvr->time.total_tick - pvr->time.cur_tick;
    }
    else
    {
        pvr->time.remaining_tick = 0;
    }
    return 0;
}

static void pvr_time_show(void)
{
    AppPvrOps *pvr = &g_AppPvrOps;
    AppPvrUi *pvr_ui = &g_AppPvrUi;
    char cur_tick[16]= {0};
    char total_tick[16]= {0};
   struct tm tm = {0};

    if(pvr->state == PVR_DUMMY)
    {
        return;
    }

    if((pvr->state == PVR_RECORD) || (pvr->state == PVR_TP_RECORD))
    {
        if (pvr_ui->timer.timer[PVR_TIMER_REC_END] != NULL)
        {
            if(pvr->time.total_tick > pvr->time.rec_duration)
                pvr->time.total_tick = pvr->time.rec_duration;
            GUI_SetProperty(TXT_TOTAL_TICK, "state", "show");
        }
//        else//change from tms_and_rec to rec,the timer PVR_TIMER_REC_END is invalid
//        {
//            GUI_SetProperty(TXT_TOTAL_TICK, "state", "hide");
//        }
    }

    if((pvr->state == PVR_RECORD) || (pvr->state == PVR_TP_RECORD))
    {
        sectohour((time_t)(pvr->time.total_tick), &tm);
    }
    else
    {
        sectohour((time_t)(pvr->time.cur_tick), &tm);
    }
    sprintf(cur_tick, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    GUI_SetProperty(TXT_CUR_TICK, "string", cur_tick);

    if((pvr->state == PVR_RECORD) || (pvr->state == PVR_TP_RECORD))
    {

		if(pvr->time.rec_duration < pvr->time.total_tick)
		{
			 sectohour((time_t)(pvr->time.total_tick), &tm);
		}
		else
		{
			sectohour((time_t)(pvr->time.rec_duration), &tm);
		}
	}
    else
    {
        sectohour((time_t)(pvr->time.total_tick), &tm);
    }
	uint32_t hour;
	hour = 24*tm.tm_mday+tm.tm_hour;
	sprintf(total_tick, "%02d:%02d:%02d", hour, tm.tm_min, tm.tm_sec);
    GUI_SetProperty(TXT_TOTAL_TICK, "string", total_tick);
}

static void pvr_slider_show(char *slider)
{
	uint32_t bar_value = 0;
	AppPvrOps *pvr = &g_AppPvrOps;
    AppPvrUi *pvr_ui = &g_AppPvrUi;

	if ((pvr->time.cur_tick == 0) && (pvr->spd > 0))
		return;

	if (pvr->state == PVR_RECORD)
	{
        if (pvr_ui->timer.timer[PVR_TIMER_REC_END] != NULL)
        {
			if(pvr->time.rec_duration != 0)
				bar_value = (pvr->time.total_tick*100)/(pvr->time.rec_duration);
        }
        else
        {
            bar_value = 100;
        }
    }
	else if (pvr->state == PVR_TIMESHIFT
		|| pvr->state == PVR_TMS_AND_REC)
	{
			if(pvr->time.total_tick != 0)
			{
				bar_value = (pvr->time.cur_tick*100)/(pvr->time.total_tick);
			}
   }
	// bar show here
	GUI_SetProperty(slider, "value", &bar_value);
}

static void pvr_percent_show(void)
{
#define PER_STR_LEN     8
#define PER_IMG_LEN     16
    uint32_t i;
    int32_t percent;
    char per_str[PER_STR_LEN] = {0};
    char per_img[PER_IMG_LEN];

    AppPvrOps *pvr = &g_AppPvrOps;
    percent = pvr->percent(pvr);
    if((percent < 0)||(percent>100))
        return;

	if(percent == 0)
	{
		percent = 1;
	}
    sprintf(per_str, "%d%%", percent);
    GUI_SetProperty(TXT_PERCENT, "string", per_str);

    // 5 dot to show the percent
    for (i=0; i<4; i++)
    {
        memset(per_img, 0, PER_IMG_LEN);
        sprintf(per_img, "%s%d", IMG_PERCENT, i);

        if (i*25 < percent)
        {
            if (i < 3)
            {
                GUI_SetProperty(per_img, "img", "s_pvr_progress_blue.bmp");
            }
            else if (i == 3)
            {
                GUI_SetProperty(per_img, "img", "s_pvr_progress_yellow.bmp");
            }
            else
            {
                GUI_SetProperty(per_img, "img", "s_pvr_progress_red.bmp");
            }
        }
        else
        {
            GUI_SetProperty(per_img, "img", "s_pvr_progress_grey.bmp");
        }
    }
}

static int pvr_timer_tick(void *usrdata)
{
    static int32_t percent_show = 0;
    AppPvrUi *pvr_ui = &g_AppPvrUi;

     // get tick
    if(pvr_cur_total_tick_get()<0)
  	 return 0;

    if (GUI_CheckDialog(WND_PVR_BAR) != GXCORE_SUCCESS)
    {
        return 0;
    }

    if(pvr_ui->tick_refresh_delay == 0)
    {
        pvr_time_show();
    }

    if(pvr_ui->tick_refresh_delay > 0)
    {
        pvr_ui->tick_refresh_delay--;
    }

    if(pvr_ui->slider_refresh <= 1)
    {
        pvr_slider_show(SLIDER_BAR);
        if(pvr_ui->slider_refresh == 1)
        {
            pvr_slider_show(SLIDER_BAR_SEEK);
        }
    }

    if(pvr_ui->slider_refresh>1)
    {
        pvr_ui->slider_refresh--;
    }

    // 10s , refresh usb percent
    if (percent_show++ == 10)
    {
        percent_show = 0;
        pvr_percent_show();
    }

    return 0;
}

static int pvr_timer_slider(void *usrdata)
{
    return 0;
}

static void pvr_timer_init(pvr_timer *timer)
{
    timer->cb[PVR_TIMER_TICK]         = pvr_timer_tick;
    timer->interval[PVR_TIMER_TICK]   = 1*SEC_TO_MILLISEC;
    timer->flag[PVR_TIMER_TICK]       = TIMER_REPEAT;

    timer->cb[PVR_TIMER_SLIDER]       = pvr_timer_slider;
    timer->interval[PVR_TIMER_SLIDER] = 1*SEC_TO_MILLISEC;
    timer->flag[PVR_TIMER_SLIDER]     = TIMER_ONCE;

}

static void pvr_timer_release(pvr_timer *timer)
{
    uint32_t i;

    for (i=0; i<PVR_TIMER_SLIDER; i++)
    {
        if (timer->timer[i] != NULL)
        {
            remove_timer(timer->timer[i]);
            timer->timer[i] = NULL;
        }
    }
}

static void pvr_timer_start(pvr_timer *timer, pvr_timer_mode mode)
{
    if(0 != reset_timer(timer->timer[mode]))
    {
        timer->timer[mode] = create_timer(timer->cb[mode],
        timer->interval[mode], NULL, timer->flag[mode]);
    }
}

static void pvr_name_show(void)
{
	char *prog_name = NULL;
	if(g_AppPvrOps.env.prog_id == 0)
	{
		GxMsgProperty_NodeByPosGet node_prog = {0};
		// prog
		node_prog.node_type = NODE_PROG;
		node_prog.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
		prog_name = (char*)node_prog.prog_data.prog_name;
	}
	else
	{
		GxMsgProperty_NodeByIdGet node_prog = {0};
		// prog
		node_prog.node_type = NODE_PROG;
		node_prog.id= g_AppPvrOps.env.prog_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_prog);
		prog_name = (char*)node_prog.prog_data.prog_name;
	}

	GUI_SetProperty(TXT_NAME, "string", prog_name);
}

// bk-0. use bakc   1. use force
static void pvr_slider_bk(uint32_t bk)
{
    char *bk_img[2][3] = {{"s_pvr_progress_l.bmp", "s_pvr_progress_m.bmp", "s_pvr_progress_r.bmp"},
                    {"s_pvr_progress_l2.bmp", "s_pvr_progress_m2.bmp", "s_pvr_progress_r2.bmp"}};

    GUI_SetProperty(SLIDER_BAR, "back_image_l", bk_img[bk][0]);
    GUI_SetProperty(SLIDER_BAR, "back_image_m", bk_img[bk][1]);
    GUI_SetProperty(SLIDER_BAR, "back_image_r", bk_img[bk][2]);
}

SIGNAL_HANDLER int app_pvr_bar_create(GuiWidget *widget, void *usrdata)
{
    AppPvrUi *pvr_ui = &g_AppPvrUi;
    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;
    char *spd_str[] = {"X1","X2","X4","X8","X16","X32"};
	if(app_check_pvr_usbin_flag() == true)
	{
		app_set_pvr_usbin_flag(false);
#define PER_IMG_LEN     16
		uint32_t i;
		char per_img[PER_IMG_LEN];
		GUI_SetProperty(TXT_PERCENT, "string", "0%");

		for (i=0; i<4; i++)
		{
			memset(per_img, 0, PER_IMG_LEN);
			sprintf(per_img, "%s%d", IMG_PERCENT, i);
			GUI_SetProperty(per_img, "img", "s_pvr_progress_grey.bmp");
		}

	}
	pvr_name_show();
	pvr_percent_show();
	//tv/radio
    if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
    {
        GUI_SetProperty(IMG_STATE, "img", "s_bar_radio.bmp");
    }

    if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        GUI_SetProperty(IMG_STATE, "img", "s_bar_tv.bmp");
    }


    if((pvr->state == PVR_RECORD) || (pvr->state == PVR_DUMMY) || (pvr->state == PVR_TP_RECORD))
    {
        pvr_slider_bk(0);
        GUI_SetProperty(SLIDER_BAR_SEEK, "state", "hide");
        GUI_SetFocusWidget(SLIDER_BAR);
    }
    else
    {
        pvr_slider_bk(1);
        if (pvr->spd != 0)
        {
            GUI_SetFocusWidget(SLIDER_BAR);
        }
        else
        {
            GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
            GUI_SetFocusWidget(SLIDER_BAR_SEEK);
        }
    }

    pvr_cur_total_tick_get();
    pvr_time_show();
    pvr_slider_show(SLIDER_BAR);
    pvr_slider_show(SLIDER_BAR_SEEK);

    pvr_ui->timer.init(&(pvr_ui->timer));
    pvr_ui->timer.start(&(pvr_ui->timer), PVR_TIMER_TICK);
    pvr_ui->ui = PVR_SHOW;

    if(arb->state.pause == STATE_OFF)
    {
        if(pvr->spd > 0)
        {
            GUI_SetProperty(TXT_STATE, "string", spd_str[pvr->spd]);
        }
        else if(pvr->spd < 0)
        {
            GUI_SetProperty(TXT_STATE, "string", spd_str[pvr->spd*(-1)]);
        }
        else
        {
            GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
        }
    }
    else
    {
        GUI_SetProperty(TXT_STATE, "string", STR_ID_PAUSE);
    }
	
#ifdef MENCENT_FREEE_SPACE
		GUI_SetInterface("flush", NULL);// ??????????,????Player???????????
#endif

	if (reset_timer(sp_PvrBarTimer) != 0)
	{
		sp_PvrBarTimer = create_timer(_pvr_bar_timeout, PVRBAR_TIMEOUT*SEC_TO_MILLISEC, NULL, TIMER_REPEAT);
	}

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_bar_destroy(GuiWidget *widget, void *usrdata)
{
    AppPvrUi *pvr_ui = &g_AppPvrUi;

    if(GUI_CheckDialog("wnd_duration_set") == GXCORE_SUCCESS)
        GUI_EndDialog("wnd_duration_set");

    pvr_ui->timer.release(&(pvr_ui->timer));
    pvr_ui->ui = PVR_HIDE;
    pvr_ui->slider_refresh = 1;
    pvr_ui->tick_refresh_delay = 0;

    if(sp_PvrBarTimer != NULL)
    {
        remove_timer(sp_PvrBarTimer);
        sp_PvrBarTimer = NULL;
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_bar_got_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_pvr_bar_lost_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_KEEPON;
}

void app_pvr_rec_stop(void)
{
    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;
    AppPvrUi *pvr_ui = &g_AppPvrUi;

    if((pvr->state != PVR_RECORD)
        && (pvr->state != PVR_TMS_AND_REC)
        && (pvr->state != PVR_TP_RECORD))
        return;
    arb->state.rec = STATE_OFF;
    pvr->rec_stop(pvr);
    pvr_ui->taxis(TAXIS_NONE);
}

void app_pvr_tms_stop(void)
{
    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;
    AppPvrUi *pvr_ui = &g_AppPvrUi;
    pvr_timer_mode mode     = PVR_TIMER_REC_END;

    if((pvr->state != PVR_TIMESHIFT)
        && (pvr->state != PVR_TMS_AND_REC))
        return;

    //Rebuild Timer PVR_TIMER_REC_END
    if((pvr_cur_total_tick_get() > 0) 
	&&(pvr->state == PVR_TMS_AND_REC)
        &&(pvr->time.rec_duration > pvr->time.total_tick))
    {
        pvr_ui->timer.cb[mode]         = pvr_timer_rec_duration;
        pvr_ui->timer.interval[mode]   = (pvr->time.rec_duration - pvr->time.total_tick)*SEC_TO_MILLISEC;
        pvr_ui->timer.flag[mode]       = TIMER_ONCE;

        if(0 != reset_timer(pvr_ui->timer.timer[mode]))
        {
            pvr_ui->timer.timer[mode] = create_timer(pvr_ui->timer.cb[mode], 
                    pvr_ui->timer.interval[mode], NULL, pvr_ui->timer.flag[mode]); 
        }
    }
    //end

    pvr->spd = 0;
    arb->state.pause = STATE_OFF;
	arb->state.tms = STATE_OFF;//-#36605
	arb->draw[EVENT_STATE](arb);//-#36605
    arb->draw[EVENT_PAUSE](arb);
    pvr->tms_stop(pvr);
}

void app_pvr_stop(void)
{
    if (GXCORE_SUCCESS ==  GUI_CheckDialog("wnd_find") && GXCORE_SUCCESS == GUI_CheckDialog("wnd_keyboard_language"))
    {
     GUI_Event event;
     event.type = GUI_KEYDOWN;
     event.key.sym = STBK_F1;
     GUI_SendEvent("wnd_keyboard_language", &event);
     GUI_SetInterface("flush", NULL);
    }

    AppPvrOps *pvr = &g_AppPvrOps;
    AppPvrUi *pvr_ui = &g_AppPvrUi;

     if(pvr->state == PVR_DUMMY)
        return;

    app_pvr_tms_stop();
    app_pvr_rec_stop();

    if (pvr_ui->timer.timer[PVR_TIMER_REC_END] != NULL)
    {
        remove_timer(pvr_ui->timer.timer[PVR_TIMER_REC_END]);
        pvr_ui->timer.timer[PVR_TIMER_REC_END] = NULL;
    }

    g_AppFullArb.exec[EVENT_STATE](&g_AppFullArb);
    g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
    if(GUI_CheckDialog("wnd_find")== GXCORE_SUCCESS)
        app_find_old_info_change();
}

static int pvr_timer_rec_duration(void *usrdata)
{
    //GxMsgProperty_NodeByPosGet node_prog = {0};
    //uint32_t rec_prog_id = g_AppPvrOps.env.prog_id;

	uint32_t sel;
	uint32_t active_sel;	
   // app_pvr_stop();
	{
	AppPvrOps *pvr = &g_AppPvrOps;
	AppPvrUi *pvr_ui = &g_AppPvrUi;

	 if(pvr->state == PVR_DUMMY)
	    return -1;

	//app_pvr_tms_stop();
	app_pvr_rec_stop();

	if (pvr_ui->timer.timer[PVR_TIMER_REC_END] != NULL)
	{
	    remove_timer(pvr_ui->timer.timer[PVR_TIMER_REC_END]);
	    pvr_ui->timer.timer[PVR_TIMER_REC_END] = NULL;
	}

	g_AppFullArb.exec[EVENT_STATE](&g_AppFullArb);
	g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
	}
	GUI_EndDialog(WND_PVR_BAR);
	if (GUI_CheckDialog("wnd_channel_list") == GXCORE_SUCCESS)
	{
		GUI_SetProperty("listview_channel_list", "update_all", NULL);
		sel = g_AppPlayOps.normal_play.play_count;
		GUI_SetProperty("listview_channel_list", "select", &sel);
		active_sel = sel;
		GUI_SetProperty("listview_channel_list", "active", &active_sel);
	}
	if (GUI_CheckDialog("wnd_epg") == GXCORE_SUCCESS)
	{
		GUI_SetProperty("listview_epg_prog", "update_all", NULL);
		sel = g_AppPlayOps.normal_play.play_count;
		GUI_SetProperty("listview_epg_prog", "select", &sel);
		active_sel = sel;
		GUI_SetProperty("listview_epg_prog", "active", &active_sel);	
	}

	if (g_AppFullArb.state.tv == STATE_OFF)
	{
		GUI_CreateDialog("wnd_channel_info");
	}
    //node_prog.node_type = NODE_PROG;
    //node_prog.pos = g_AppPlayOps.normal_play.play_count;
    //app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
    //
    //if(node_prog.prog_data.id == rec_prog_id)
    //{
    //    g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
    //    g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    //    //g_AppPmt.start(&g_AppPmt);
    //}
    return 0;
}

// dura_sec : 0-use system setting time
void app_pvr_rec_start(uint32_t dura_sec)
{
#define HALF_HOUR_SEC       (60*30)
    AppPvrOps   *pvr        = &g_AppPvrOps;
    FullArb     *arb        = &g_AppFullArb;
    AppPvrUi    *pvr_ui     = &g_AppPvrUi;
    pvr_timer_mode mode     = PVR_TIMER_REC_END;

    if (pvr->state == PVR_RECORD
            || pvr->state == PVR_TMS_AND_REC)
    {
        return;
    }

    _get_pvr_duration_from_event();

    if (pvr->state == PVR_DUMMY)
    {
        pvr_slider_bk(0);
    }

    pvr_ui->taxis(TAXIS_TP);

    if (dura_sec == 0)
    {
        pvr->time.rec_duration = pvr_duration;
    }
    else
    {
        pvr->time.rec_duration = dura_sec;
    }
	
    pvr->rec_start(pvr);//Pvr state change
    arb->state.rec = STATE_ON;
    arb->draw[EVENT_STATE](arb);
    pvr_percent_show();

    if(pvr->state == PVR_RECORD) //TMS->TMS_REC Don't need timer_rec_end; Just Record
    {
        // set timer
        pvr_ui->timer.cb[PVR_TIMER_REC_END]         = pvr_timer_rec_duration;
        //+3 sec, timer is not accuate, record more data is better than less, maybe...
        pvr_ui->timer.interval[PVR_TIMER_REC_END]   = pvr->time.rec_duration*SEC_TO_MILLISEC + 3;
        pvr_ui->timer.flag[PVR_TIMER_REC_END]       = TIMER_ONCE;

        if(0 != reset_timer(pvr_ui->timer.timer[mode]))
        {
            pvr_ui->timer.timer[mode] = create_timer(pvr_ui->timer.cb[mode],
                    pvr_ui->timer.interval[mode], NULL, pvr_ui->timer.flag[mode]);
        }
    }
}

void app_tp_rec_start(uint32_t dura_sec)
{
#define HALF_HOUR_SEC       (60*30)
    AppPvrOps   *pvr        = &g_AppPvrOps;
    FullArb     *arb        = &g_AppFullArb;
    AppPvrUi    *pvr_ui     = &g_AppPvrUi;
//    pvr_timer_mode mode     = PVR_TIMER_REC_END;

    if (pvr->state != PVR_DUMMY)
    {
        return;
    }

    //pvr_slider_bk(0);

    pvr_ui->taxis(TAXIS_TP);

#if 0
    if (dura_sec == 0)
    {
        pvr->time.rec_duration = pvr_duration;
    }
    else
    {
        pvr->time.rec_duration = dura_sec;
    }
#endif

    pvr->tp_rec_start(pvr);//Pvr state change
    arb->state.rec = STATE_ON;
    arb->draw[EVENT_STATE](arb);
    pvr_percent_show();

#if 0
    if(pvr->state == PVR_RECORD) //TMS->TMS_REC Don't need timer_rec_end; Just Record
    {
        // set timer
        pvr_ui->timer.cb[PVR_TIMER_REC_END]         = pvr_timer_rec_duration;
        //+3 sec, timer is not accuate, record more data is better than less, maybe...
        pvr_ui->timer.interval[PVR_TIMER_REC_END]   = pvr->time.rec_duration*SEC_TO_MILLISEC + 3;
        pvr_ui->timer.flag[PVR_TIMER_REC_END]       = TIMER_ONCE;

        if(0 != reset_timer(pvr_ui->timer.timer[mode]))
        {
            pvr_ui->timer.timer[mode] = create_timer(pvr_ui->timer.cb[mode],
                    pvr_ui->timer.interval[mode], NULL, pvr_ui->timer.flag[mode]);
        }
    }
#endif
}

void pvr_taxis_mode_clear(void)
{
    g_AppPvrUi.taxis(TAXIS_NONE);
}

void tms_speed_change(float spd_speed)
{

    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;

    if (pvr->state != PVR_TIMESHIFT
        && pvr->state != PVR_TMS_AND_REC)
    {
        return;
    }

    AppPvrUi  *pvr_ui = &g_AppPvrUi;

    if(spd_speed == PVR_SPD_1)
    {
        GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
        GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
        GUI_SetFocusWidget(SLIDER_BAR_SEEK);
        pvr->spd = 0;
        reset_timer(sp_PvrBarTimer);
    }
    else
    {
        timer_stop(sp_PvrBarTimer);
        pvr->spd = 1;

        if(spd_speed < 0)
        {
            spd_speed *= -1;
            pvr->spd *= -1;
        }
		switch((int)spd_speed)//the value of pvr->spd depends on spd_val[]
		{
			case PVR_SPD_2:
				GUI_SetProperty(TXT_STATE, "string", "X2");
				break;

			case PVR_SPD_4:
				GUI_SetProperty(TXT_STATE, "string", "X4");
				pvr->spd *= 2;
				break;

			case PVR_SPD_8:
				GUI_SetProperty(TXT_STATE, "string", "X8");
				pvr->spd *= 3;
				break;

			case PVR_SPD_16:
				GUI_SetProperty(TXT_STATE, "string", "X16");
				pvr->spd *= 4;
				break;

			case PVR_SPD_32:
				GUI_SetProperty(TXT_STATE, "string", "X32");
				pvr->spd *= 5;
				break;

			default:
				GUI_SetProperty(TXT_STATE, "string", " ");
				break;
		}
		pvr_ui->slider_refresh = 1;
		pvr_ui->tick_refresh_delay = 0;
		GUI_SetProperty(SLIDER_BAR_SEEK, "state", "hide");
		GUI_SetInterface("flush",NULL);
		GUI_SetFocusWidget(SLIDER_BAR);
	}

	arb->draw[EVENT_SPEED](arb);

}

void tms_state_change(PlayerStatus state)
{
	AppPvrOps *pvr = &g_AppPvrOps;

    if (pvr->state != PVR_TIMESHIFT
        && pvr->state != PVR_TMS_AND_REC)
    {
        return;
    }

    AppPvrUi  *pvr_ui = &g_AppPvrUi;
	FullArb *arb = &g_AppFullArb;

    switch(state)
    {
        case PLAYER_STATUS_RECORD_FULL:
        {
            app_pvr_patition_full_proc();
#if 0
            PopDlg pop;
            g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_NO_SPACE;
            pop.mode = POP_MODE_UNBLOCK;

            app_pvr_stop();
            GUI_EndDialog("wnd_pvr_bar");
            GUI_EndDialog("wnd_channel_info_signal");
            GUI_EndDialog("wnd_channel_info");
            GUI_EndDialog("wnd_volume_value");

            g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
            g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
            popdlg_create(&pop);
#endif
            break;
        }

        case PLAYER_STATUS_SHIFT_START:
        {//时移开始，解复用相关配置完成
            pvr_ui->shift_start = true;
            pvr->spd = 0;
            GUI_SetProperty(TXT_STATE, "string", STR_ID_PAUSE);
            GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
            GUI_SetFocusWidget(SLIDER_BAR_SEEK);
#if CA_SUPPORT
            // TODO: reset cmb ...
#if ECM_SUPPORT
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
            {
                if(app_tsd_check())
                {
                    return EVENT_TRANSFER_KEEPON;
                }

                if(0 == g_AppEcm2.ecm_count)
                {
                    int demux_id = 0;
                    memcpy(&g_AppEcm2,&g_AppEcm,sizeof(AppEcm));
                    app_getvalue(g_AppPvrOps.env.url,"dmxid",&demux_id);
                    g_AppEcm2.CaManager.ProgId = g_AppPvrOps.env.prog_id;
                    g_AppEcm2.CaManager.Type = CONTROL_PVR;
                    g_AppEcm2.init(&g_AppEcm2);
                    g_AppEcm2.start(&g_AppEcm2);
                }
            }
#endif
#endif
            app_descrambler_close_by_control_type(CONTROL_NORMAL);
#endif
            break;
        }

        case PLAYER_STATUS_SHIFT_RUNNING:
        {
            pvr_ui->shift_start = false;
            pvr_ui->slider_refresh = 1;
            pvr_ui->tick_refresh_delay = 0;

            if(pvr->spd == 0)
			{
				GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
				GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
				GUI_SetFocusWidget(SLIDER_BAR_SEEK);
			}
#if CA_SUPPORT
            app_descrambler_close_by_control_type(CONTROL_NORMAL);
#endif
            break;
        }

        case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
        {
            pvr_ui->shift_start = false;
            pvr_ui->slider_refresh = 1;
            pvr_ui->tick_refresh_delay = 0;
			if(pvr->spd >= 0)
			{
				pvr->spd = 0;
				GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
				GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
				GUI_SetFocusWidget(SLIDER_BAR_SEEK);
				arb->draw[EVENT_SPEED](arb); 

				//	arb->state.pause = STATE_OFF;
				//	arb->draw[EVENT_PAUSE](arb);//错误 #39358
#if CA_SUPPORT

                GxMsgProperty_NodeByPosGet node;
                int demux_id = g_AppPlayOps.normal_play.dmx_id;

				node.node_type = NODE_PROG;
				node.pos = g_AppPlayOps.normal_play.play_count;
				if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&node))
                {
                    app_extend_send_service(pvr->env.prog_id, node.prog_data.sat_id, node.prog_data.tp_id, demux_id, CONTROL_NORMAL);
                }

#endif
			}
            break;
        }

        case PLAYER_STATUS_SHIFT_PAUSE:
        {
            GUI_SetProperty(TXT_STATE, "string", STR_ID_PAUSE);
            break;
        }

        default:
            break;
    }
}

//Duration Set Start!
static void _get_pvr_duration_from_event(void)
{
#define HALF_HOUR_SEC       (60*30)
    time_t system_sec = 0;
    int32_t     duration;
    bool cur_flag = false;
    bool next_flag = false;
    AppEpgOps ch_epg_ops;
    GxEpgInfo *cur_epg_info = NULL;
    GxEpgInfo *next_epg_info = NULL;
    EpgProgInfo epg_prog_info;
    GxMsgProperty_NodeByPosGet node_prog = {0};

    epg_ops_init(&ch_epg_ops);
    GxBus_ConfigGetInt(PVR_DURATION_KEY, &duration, PVR_DURATION_VALUE);
    pvr_duration = (duration+4)*HALF_HOUR_SEC;//2H

    node_prog.node_type = NODE_PROG;
    node_prog.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

	epg_prog_info.ts_id= node_prog.prog_data.ts_id;
	epg_prog_info.orig_network_id= node_prog.prog_data.original_id;
	epg_prog_info.service_id= node_prog.prog_data.service_id;
	epg_prog_info.prog_id= node_prog.prog_data.id;
	epg_prog_info.prog_pos = node_prog.pos;

    if(cur_epg_info != NULL)
    {
        GxCore_Free(cur_epg_info);
        cur_epg_info = NULL;
    }
    cur_epg_info = (GxEpgInfo *)GxCore_Malloc(EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
	if(cur_epg_info == NULL)
	{
		printf("_get_pvr_duration_from_event, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}

    if(next_epg_info != NULL)
    {
        GxCore_Free(next_epg_info);
        next_epg_info = NULL;
    }
    next_epg_info = (GxEpgInfo *)GxCore_Malloc(EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
	if(next_epg_info == NULL)
	{
		printf("_get_pvr_duration_from_event, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}

    ch_epg_ops.prog_info_update(&ch_epg_ops, &epg_prog_info);
    ch_epg_ops.event_cnt_update(&ch_epg_ops);
    if(ch_epg_ops.get_cur_event_cnt(&ch_epg_ops) > 0)
    {
        if(cur_epg_info != NULL)
        {
            if(ch_epg_ops.get_cur_event_info(&ch_epg_ops, cur_epg_info) == GXCORE_SUCCESS)
            {
                if(cur_epg_info->event_name_len > 0)
                {
                    cur_flag = true;
                }
            }
        }

        if(next_epg_info != NULL)
        {
            if(ch_epg_ops.get_next_event_info(&ch_epg_ops, next_epg_info) == GXCORE_SUCCESS)
            {
                if(next_epg_info->event_name_len > 0)
                {
                    next_flag = true;
                }
            }
        }
    }

    if(cur_flag)
    {
        system_sec = g_AppTime.utc_get(&g_AppTime);
        if(system_sec > cur_epg_info->start_time)
        {
            pvr_duration = cur_epg_info->duration - (system_sec - cur_epg_info->start_time);
            if(pvr_duration < 3*60)//3 MIN
            {
                if(next_flag)
                {
                    pvr_duration += next_epg_info->duration;
                    if(pvr_duration < 3*60)
                    {
                        pvr_duration = 3*60;
                    }
                }
                else
                {
                    pvr_duration = (duration+4)*HALF_HOUR_SEC;//2H
                }
            }
        }
    }

    if(cur_epg_info != NULL)
    {
        GxCore_Free(cur_epg_info);
        cur_epg_info = NULL;
    }

    if(next_epg_info != NULL)
    {
        GxCore_Free(next_epg_info);
        next_epg_info = NULL;
    }
}

SIGNAL_HANDLER int app_duration_set_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
	time_t time = 0;
	char *buffer = NULL;
    AppPvrOps   *pvr        = &g_AppPvrOps;
    AppPvrUi    *pvr_ui     = &g_AppPvrUi;
    pvr_timer_mode mode     = PVR_TIMER_REC_END;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog("wnd_duration_set");
                    break;
                case STBK_OK:
                    GUI_GetProperty(EDIT_DURATION, "string", &buffer);
                    time = app_edit_str_to_hourmin_sec(buffer);

                    if(time == 0 || time == pvr->time.rec_duration)
					{
						if(time == 0)
						{
							PopDlg  pop;

							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_OK;
							pop.str = STR_ID_END_TIME_ERR;
							pop.mode = POP_MODE_UNBLOCK;
							popdlg_create(&pop);
						}
						else
						{
							GUI_EndDialog("wnd_duration_set");
						}
                        break;//nothing to do!
                    }
                    else if(time < pvr->time.total_tick)
                    {
						PopDlg  pop;
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_OK;
						pop.str = STR_ID_END_TIME_ERR;
						pop.mode = POP_MODE_UNBLOCK;
						popdlg_create(&pop);
//						GUI_EndDialog("wnd_duration_set");	
                        break;//nothing to do!
                    }
                    else
                    {
                        pvr->time.rec_duration = time;
                    }

                    //remove old timer
                    if (pvr_ui->timer.timer[mode] != NULL)
                    {
                        remove_timer(pvr_ui->timer.timer[mode]);
                        pvr_ui->timer.timer[mode] = NULL;
                    }

                    //create new timer
                    pvr_ui->timer.cb[mode]         = pvr_timer_rec_duration;
                    pvr_ui->timer.interval[mode]   = (pvr->time.rec_duration - pvr->time.total_tick)*SEC_TO_MILLISEC;
                    pvr_ui->timer.flag[mode]       = TIMER_ONCE;

                    if(0 != reset_timer(pvr_ui->timer.timer[mode]))
                    {
                        pvr_ui->timer.timer[mode] = create_timer(pvr_ui->timer.cb[mode],
                                pvr_ui->timer.interval[mode], NULL, pvr_ui->timer.flag[mode]);
                    }

                    GUI_EndDialog("wnd_duration_set");
                    break;
                default:
                    break;
            }
    }

    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_duration_set_create(GuiWidget *widget, void *usrdata)
{
#define HM_STR_LEN 16
	char *string = NULL;
    AppPvrOps   *pvr        = &g_AppPvrOps;
	struct tm temp_tm = {0};

	string = (char *)GxCore_Malloc(HM_STR_LEN);
	if(string != NULL)
	{
		memset(string, 0, HM_STR_LEN);
        sectohour((time_t)(pvr->time.rec_duration), &temp_tm);
		sprintf(string, "%02d:%02d", temp_tm.tm_hour , temp_tm.tm_min);
	}
    //string = app_time_to_hourmin_edit_str(pvr->time.rec_duration);
    if(string != NULL)
    {
        GUI_SetProperty(EDIT_DURATION, "string", string);
        GxCore_Free(string);
        string = NULL;
    }

    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_duration_set_destroy(GuiWidget *widget, void *usrdata)
{
	if (reset_timer(sp_PvrBarTimer) != 0)
	{
		sp_PvrBarTimer = create_timer(_pvr_bar_timeout, PVRBAR_TIMEOUT*SEC_TO_MILLISEC, NULL, TIMER_ONCE);
	}
    return EVENT_TRANSFER_STOP;
}
//Duration Set End!
static pvr_state before_stop_state = PVR_DUMMY;
void app_pvr_set_before_stop_state(pvr_state state)
{
	before_stop_state = state;
}

int app_pvr_stop_cb(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        GxMsgProperty_NodeByPosGet node_prog = {0};
        uint32_t rec_prog_id = g_AppPvrOps.env.prog_id;

        app_pvr_stop();
        if((before_stop_state != PVR_RECORD) && (before_stop_state != PVR_TP_RECORD))
        {
            before_stop_state = PVR_DUMMY;
            node_prog.node_type = NODE_PROG;
            node_prog.pos = g_AppPlayOps.normal_play.play_count;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
            if(node_prog.prog_data.id == rec_prog_id)
            {
                g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
                g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
                //g_AppPmt.start(&g_AppPmt);
            }	
        }		
    }
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		if(GUI_CheckDialog("wnd_channel_info") != GXCORE_SUCCESS)
				app_create_dialog("wnd_channel_info");
	}
    return 0;
}

SIGNAL_HANDLER int app_pvr_bar_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;
    AppPvrUi  *pvr_ui = &g_AppPvrUi;
    int freeze = 0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
#if (DLNA_SUPPORT > 0)
                case VK_DLNA_TRIGGER:
                    GUI_EndDialog(WND_PVR_BAR);
					GUI_SendEvent("wnd_full_screen", event);
                    break;
#endif
                case STBK_EXIT:
                case STBK_MENU:
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_PVR_BAR);
                    break;

                case STBK_PAUSE:
                case STBK_PAUSE_STB:
                case STBK_TMS:
                {
                    reset_timer(sp_PvrBarTimer);
                    GxMsgProperty_NodeByPosGet node_prog = {0};
                    node_prog.node_type = NODE_PROG;
                    node_prog.pos = g_AppPlayOps.normal_play.play_count;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
                    if((pvr->state != PVR_TP_RECORD)
                        && ((pvr->state == PVR_DUMMY)
                            || (node_prog.prog_data.id == g_AppPvrOps.env.prog_id)))
                    {                       
                        if(arb->state.pause == STATE_OFF)
                        {
                            pvr_slider_bk(1);
                            freeze = 1;
                            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);

                            GUI_SetProperty(TXT_STATE, "string", STR_ID_PAUSE);
                            GUI_SetInterface("flush", NULL);
                            if(pvr->state == PVR_RECORD)
                            {
                                pvr->time.cur_tick = pvr->time.total_tick;
                           //     if (pvr_ui->timer.timer[PVR_TIMER_REC_END] != NULL)
                                {                              
	                      //          remove_timer(pvr_ui->timer.timer[PVR_TIMER_REC_END]);
	                      //          pvr_ui->timer.timer[PVR_TIMER_REC_END] = NULL;
                                }
                            }
                            pvr->pause(pvr);
                            arb->state.pause = STATE_ON;
							arb->draw[EVENT_PAUSE](arb);
                            arb->state.tms = STATE_ON;
                            pvr_percent_show();
                        }
                        else
                        {
                            GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
                            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);

                            GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
                            if(pvr_ui->shift_start == true)
                            {
                                pvr_ui->tick_refresh_delay = TICK_DELAY * 2;
                                pvr_ui->slider_refresh = 4;
                            }
                            if (pvr->spd != 0)
                            {
#if 1//(TRICK_PLAY_SUPPORT > 0)
//#9886
								pvr_slider_bk(1);
								float spd = 0;
								// if fb or ff, set normal speed
								if(pvr->spd < 0)
								{
									spd = ((float)spd_val[(pvr->spd*(-1))])*(-1);
								}
								else
								{
									spd = spd_val[pvr->spd];
								}
								//pvr->spd = 0;
//								pvr->speed(pvr,PVR_SPD_1);
								arb->state.pause = STATE_OFF;
								arb->draw[EVENT_PAUSE](arb);
								pvr->resume(pvr);
								tms_speed_change(spd);
#else
								pvr->spd = 0;
								arb->state.pause = STATE_OFF;
								arb->draw[EVENT_PAUSE](arb);
								pvr->speed(pvr,PVR_SPD_1);
#endif
							}
							else
							{
								arb->state.pause = STATE_OFF;
								arb->draw[EVENT_PAUSE](arb);
								pvr->resume(pvr);
							}
						}
					}
					else
                    {
                        GxMsgProperty_PlayerPause player_pause;
                        GxMsgProperty_PlayerResume player_resume;
                        if (arb->state.pause == STATE_ON)
                        {
                            GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
                            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);
                            player_resume.player = PLAYER_FOR_NORMAL;
                            app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
                            arb->state.pause = STATE_OFF;
                        }
                        else
                        {
                            app_subt_pause();
                            freeze = 1;
                            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);
                            player_pause.player = PLAYER_FOR_NORMAL;
                            app_send_msg_exec(GXMSG_PLAYER_PAUSE, (void *)(&player_pause));
                            arb->state.pause = STATE_ON;
                        }
                        GUI_EndDialog(WND_PVR_BAR);
                        GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);
						arb->draw[EVENT_PAUSE](arb);
                    }
                    arb->draw[EVENT_STATE](arb);
                    break;
                }

                case STBK_REC_START:
                    if (pvr->state == PVR_RECORD)
                    {
                        app_create_dialog("wnd_duration_set");
                        timer_stop(sp_PvrBarTimer);
                    }
                    else if(pvr->state != PVR_TP_RECORD)
                    {
                        app_pvr_rec_start(0);
                    }
                    break;

                case STBK_REC_STOP:
                    {
                        PopDlg pop;
                        if(GUI_CheckDialog(WND_PVR_BAR) == GXCORE_SUCCESS)
                        {
                            GUI_EndDialog(WND_PVR_BAR);
                            GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);
                        }
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_YES_NO;
                        if(pvr->state == PVR_TIMESHIFT)
                        {
                            pop.str = STR_ID_STOP_TMS_INFO;
                        }
                        else
                        {
                            pop.str = STR_ID_STOP_REC_INFO;
                        }
                        app_pvr_set_before_stop_state(pvr->state);
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.exit_cb = app_pvr_stop_cb;
                        popdlg_create(&pop);

                    }
                    break;

                case STBK_MUTE:
                    reset_timer(sp_PvrBarTimer);
                    arb->exec[EVENT_MUTE](arb);
                    arb->draw[EVENT_MUTE](arb);
                    break;

			case STBK_CH_UP:
			case STBK_UP:
			case STBK_CH_DOWN:
			case STBK_DOWN:
				if(GUI_CheckDialog(WND_PVR_BAR) == GXCORE_SUCCESS)
				{
					GUI_EndDialog(WND_PVR_BAR);
                GUI_SetInterface("flush", NULL);
				}
				GUI_SendEvent(GUI_GetFocusWindow(), event);
				break;

			case STBK_OK:
            case STBK_EPG:
				if(GUI_CheckDialog(WND_PVR_BAR) == GXCORE_SUCCESS)
				{
					GUI_EndDialog(WND_PVR_BAR);
                GUI_SetInterface("flush", NULL);
				}
				GUI_SendEvent(GUI_GetFocusWindow(), event);
				break;

		    case STBK_VOLDOWN:
		    case STBK_VOLUP:
			    if(GUI_CheckDialog(WND_PVR_BAR) == GXCORE_SUCCESS)
			    {
				    GUI_EndDialog(WND_PVR_BAR);
                 GUI_SetInterface("flush", NULL);
			    }
              app_create_dialog("wnd_volume_value");
              GUI_SendEvent("wnd_volume_value", event);
              break;
			 #ifdef REC_FULL_RECALL_SUPPORT
			 case STBK_RECALL:
			 	{
					if (g_AppPvrOps.state != PVR_DUMMY)
					{
						PopDlg pop;
						pvr_state cur_pvr = PVR_DUMMY;
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_YES_NO;
			
                            if(g_AppPvrOps.state == PVR_TMS_AND_REC)
                            {
#if CA_SUPPORT
                                if(app_extend_control_pvr_change())
                                {
                                    pop.str = STR_ID_STOP_REC_INFO;
                                    cur_pvr = PVR_RECORD;
                                }
                                else
#endif
                                {
                                    pop.str = STR_ID_STOP_TMS_INFO;
                                    cur_pvr = PVR_TIMESHIFT;
                                }
                            }
                            else if(g_AppPvrOps.state == PVR_RECORD)
                            {
#if CA_SUPPORT
                                if(app_extend_control_pvr_change())
                                {
                                    pop.str = STR_ID_STOP_REC_INFO;
                                    cur_pvr = PVR_RECORD;
                                }
#endif
						}
						else
						{
							pop.str = STR_ID_STOP_TMS_INFO;
							cur_pvr = PVR_TIMESHIFT;
						}
			
						if(cur_pvr != PVR_DUMMY)
						{					 
							if (POP_VAL_OK != popdlg_create(&pop))
							{
								break;
							}
			
							if(cur_pvr == PVR_TIMESHIFT)
							{
								app_pvr_tms_stop();
							}
							else
							{
								app_pvr_stop();
							}
						}
					}			
					extern void full_recall_arbitrate(void);
			 	 	full_recall_arbitrate();
			 	}
				break;
			 #endif

           default:
              break;
        }
    }

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_sliderbar_keypress(GuiWidget *widget, void *usrdata)
{
    status_t ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = (GUI_Event *)usrdata;
    AppPvrOps *pvr = &g_AppPvrOps;
    //pvr_speed spd_val[] = {PVR_SPD_1,PVR_SPD_2,PVR_SPD_4,PVR_SPD_8,PVR_SPD_16,PVR_SPD_32};
    FullArb *arb = &g_AppFullArb;


    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
        switch(event->key.sym)
        {
            case STBK_LEFT:
            case STBK_RIGHT:
            break;

            case STBK_FB:
            arb->state.pause = STATE_OFF;
            if (pvr->spd > 0)
                pvr->spd = 0;

            (pvr->spd<=-SPD_NUM)?(pvr->spd=0):(pvr->spd--);
            if(pvr->spd != 0)
            {
                pvr->speed(pvr, ((float)spd_val[(pvr->spd*(-1))])*(-1));
            }
            else
            {
                pvr->speed(pvr, spd_val[pvr->spd]);
            }

            break;

        case STBK_FF:
			arb->state.pause = STATE_OFF;
			if (pvr->spd < 0)
				pvr->spd = 0;

			(pvr->spd>=SPD_NUM)?(pvr->spd=0):(pvr->spd++);
			pvr->speed(pvr, spd_val[pvr->spd]);


            break;

        case STBK_PLAY:
            reset_timer(sp_PvrBarTimer);
            if (pvr->spd != 0)
            {
                pvr->spd = 0;
				pvr->speed(pvr, PVR_SPD_1);
            }
			else
			{
				GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
				if(pvr->enter_shift == 0)
					pvr->resume(pvr);
			}
			arb->state.pause = STATE_OFF;
		//	arb->draw[EVENT_PAUSE](arb);
			arb->draw[EVENT_SPEED](arb);

            break;

        default:
            ret = EVENT_TRANSFER_KEEPON;
            break;
        }
    }
    return ret;
}

SIGNAL_HANDLER int app_pvr_seek_slider_keypress(GuiWidget *widget, void *usrdata)
{
#define TMS_SEEK_DELAY (0) //ms
	status_t ret = EVENT_TRANSFER_STOP;
	uint32_t slider = 0;
	GUI_Event *event = NULL;
    AppPvrOps *pvr = &g_AppPvrOps;
    AppPvrUi  *pvr_ui = &g_AppPvrUi;
    FullArb *arb = &g_AppFullArb;
	int32_t spd_fb = 0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
    switch(event->key.sym)
    {
		case STBK_LEFT:
		case STBK_RIGHT:
            reset_timer(sp_PvrBarTimer);
            if((pvr->time.remaining_tick == 0) && (pvr_ui->slider_refresh != 0) && (event->key.sym == STBK_RIGHT))
                break;

            // fb or ff can't control slider bar
            if ((pvr->spd != 0) && (arb->state.pause == STATE_OFF))
                break;
            // rec can't control slider bar
            if ((pvr->state!=PVR_TIMESHIFT) && (pvr->state!=PVR_TMS_AND_REC))
                break;

            pvr_ui->slider_refresh = 0;
            ret = EVENT_TRANSFER_KEEPON;
            break;

            case STBK_OK:
                if(pvr_ui->slider_refresh == 0)
                {
                    reset_timer(sp_PvrBarTimer);
                    if ((pvr->state == PVR_TIMESHIFT) || (pvr->state == PVR_TMS_AND_REC))
                    {
                        uint32_t seek_time = 0;
                        GUI_GetProperty(SLIDER_BAR_SEEK, "value", &slider);
                        seek_time = pvr->time.total_tick*slider/100;
						char cur_tick[16]= {0};
						struct tm tm = {0};
						sectohour(seek_time, &tm);
						sprintf(cur_tick, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
						GUI_SetProperty(TXT_CUR_TICK, "string", cur_tick);

                        if(pvr->time.total_tick - seek_time < 5)
                        {
                            pvr_ui->slider_refresh = 4;
                            pvr_ui->tick_refresh_delay = TICK_DELAY * 4;
                        }
                        else
                        {
                            pvr_ui->slider_refresh = 1;
                            pvr_ui->tick_refresh_delay = TICK_DELAY;
                        }
                        if(arb->state.pause == STATE_ON)
                        {
                             if (pvr->spd != 0)
                            {
                                // if fb or ff, set normal speed
                                pvr->spd = 0;
                                pvr->speed(pvr,PVR_SPD_1);
                            }
                            else
                            {
                                GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
								if(pvr->enter_shift == 0)
                                pvr->resume(pvr);
                            }
#ifdef LOOP_TMS_SUPPORE
							pvr->seek(pvr, pvr->time.total_tick*slider/100+pvr->time.seekmin_tick);
#else
							pvr->seek(pvr, pvr->time.total_tick*slider/100);
#endif
	
                            arb->state.pause = STATE_OFF;
                            arb->draw[EVENT_PAUSE](arb);
                            arb->draw[EVENT_SPEED](arb);

						}
						else
						{
#ifdef LOOP_TMS_SUPPORE
							pvr->seek(pvr, pvr->time.total_tick*slider/100+pvr->time.seekmin_tick);
#else
							pvr->seek(pvr, pvr->time.total_tick*slider/100);
#endif
						}
						GUI_SetProperty(SLIDER_BAR, "value", &slider);
					}
                    else
                    {}
                }
                else
                {
                    ret = EVENT_TRANSFER_KEEPON;
                }
            break;

            case STBK_FB:
        
#if REDIO_RECODE_SUPPORT
                if(arb->state.tv == STATE_OFF)
                    break;
#endif	
                if ((pvr->state!=PVR_TIMESHIFT)&&(pvr->state!=PVR_TMS_AND_REC))
                    break;

                if(pvr->time.total_tick <= TMS_SEEK_DELAY)
                    break;

                if(pvr->time.cur_tick == 0)
                    break;
 
                if(pvr_ui->shift_start == true) 
                {
                    if(pvr->time.remaining_tick > TMS_SEEK_DELAY)
                    {
                        pvr->resume(pvr); 
                    }
                    else
                    {
                        break;
                    }
                }    

				if(pvr->spd > 0)
					pvr->spd = 0;
				
				spd_fb = pvr->spd;

				(spd_fb<=-SPD_NUM)?(pvr->spd=spd_fb=0):(spd_fb--);
                
                arb->state.pause = STATE_OFF;
				arb->draw[EVENT_PAUSE](arb);
                //pvr->speed(pvr, ((float)spd_val[(pvr->spd*(-1))])*(-1));
				if(spd_fb != 0)
	            {
	                pvr->speed(pvr, ((float)spd_val[(spd_fb*(-1))])*(-1));
	            }
	            else
	            {
	                pvr->speed(pvr, spd_val[spd_fb]);
				}

                break;

            case STBK_FF:
      
#if REDIO_RECODE_SUPPORT
                if(arb->state.tv == STATE_OFF)
                    break;
#endif	
                if ((pvr->state != PVR_TIMESHIFT)&&(pvr->state!= PVR_TMS_AND_REC))
                    break;

                if(pvr->time.total_tick<= TMS_SEEK_DELAY)
                    break;

                if(pvr->time.remaining_tick <= TMS_SEEK_DELAY)
                    break;

                if(pvr_ui->shift_start == true)
                {
                    pvr->resume(pvr); 
                }
                
				if(pvr->spd < 0)
					pvr->spd = 0;

				(pvr->spd>=SPD_NUM)?(pvr->spd=0):(pvr->spd++);

                arb->state.pause = STATE_OFF;
                arb->draw[EVENT_PAUSE](arb);    
                pvr->speed(pvr, spd_val[pvr->spd]);
           
                break;

            case STBK_PLAY://pause resume
                reset_timer(sp_PvrBarTimer);
                if(arb->state.pause == STATE_ON)
                {
                    GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
                    if(pvr_ui->shift_start == true)
                    {
                        pvr_ui->tick_refresh_delay = TICK_DELAY * 2;
                        pvr_ui->slider_refresh = 4; 
                    }
                    if (pvr->spd != 0)
                    {
                        // if fb or ff, set normal speed
                        pvr->spd = 0;
                        pvr->speed(pvr,PVR_SPD_1);
                    }
                    else
                    {
                        pvr->resume(pvr);
                    } 
                    arb->state.pause = STATE_OFF;
                    arb->draw[EVENT_PAUSE](arb);
                    arb->draw[EVENT_SPEED](arb);

                }

            break;

            default:
                ret = EVENT_TRANSFER_KEEPON; 
                break;
        }
    }
    return ret;
}

void app_pvr_patition_full_proc(void)
{
    PopDlg  pop;
    GxMsgProperty_NodeByPosGet node_prog = {0};
    uint32_t rec_prog_id = g_AppPvrOps.env.prog_id;

    g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_OK;
    pop.str = STR_ID_NO_SPACE;
    pop.mode = POP_MODE_UNBLOCK;

    uint32_t sel = 0;

    app_pvr_stop();
    GUI_EndDialog("wnd_pvr_bar");
    if (GUI_CheckDialog("wnd_channel_list") == GXCORE_SUCCESS)
    {
        {
            GUI_SetProperty("listview_channel_list", "update_all", NULL);
            sel = g_AppPlayOps.normal_play.play_count;
            GUI_SetProperty("listview_channel_list", "select", &sel);
            GUI_SetProperty("listview_channel_list", "active", &sel);
        }
    }
    GUI_EndDialog("wnd_channel_info_signal");
    GUI_EndDialog("wnd_channel_info");
    GUI_EndDialog("wnd_volume_value");

    node_prog.node_type = NODE_PROG;
    node_prog.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
    if(node_prog.prog_data.id == rec_prog_id)
    {
        g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
        g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
        g_AppPmt.start(&g_AppPmt);
    }
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		if(GUI_CheckDialog("wnd_channel_info") != GXCORE_SUCCESS)
			app_create_dialog("wnd_channel_info");
	}
	if(GUI_CheckDialog("wnd_pop_book") != GXCORE_SUCCESS)
	{
		popdlg_create(&pop);
	}
}

AppPvrUi g_AppPvrUi =
{
    .ui             = PVR_HIDE,
    .slider_refresh = 1,
    .taxis          = pvr_taxis_mode,
    .timer  = {{NULL,},{0,},{0,},{NULL,},pvr_timer_init,pvr_timer_release,pvr_timer_start},
};

