#include "app.h"
#include "app_book.h"
#include "app_module.h"
#include "app_pop.h"

#define WND_SLEEP "wnd_sleep_set"
#define CMB_SLEEP "combobox_sleep_set"
#define SLEEP_INFO_SEC (30)

enum
{
	SLEEP_OFF = 0,
	SLEEP_15_MIN,
	SLEEP_30_MIN,
	SLEEP_60_MIN,
	SLEEP_120_MIN,
};

static event_list * sp_TimerSleepExec = NULL;
static event_list * sp_TimerSleepEnd = NULL;
static int s_sleep_sel = SLEEP_OFF;

extern void app_low_power(time_t sleep_time, uint32_t scan_code);

static int _sleep_end_timeout(void* data)
{
	timer_stop(sp_TimerSleepEnd);
	GUI_EndDialog(WND_SLEEP);
	return 0;
}

static int _sleep_exec(PopDlgRet ret)
{
	time_t timerDur;
	time_t cur_time;

	if(ret == POP_VAL_OK)
	{
		cur_time = g_AppTime.utc_get(&g_AppTime);
		timerDur =  g_AppBook.get_sleep_time(cur_time);
		GxCore_ThreadDelay(1000);
		app_low_power(timerDur, 0);
	}
	
	return 0;						
}

static int _sleep_exec_timeout(void* data)
{
	remove_timer(sp_TimerSleepExec);
	sp_TimerSleepExec = NULL;
	s_sleep_sel = SLEEP_OFF;

	if((GUI_CheckDialog("wnd_search") == GXCORE_SUCCESS)
		|| (GUI_CheckDialog("wnd_upgrade_process")== GXCORE_SUCCESS)
		|| (GUI_CheckDialog("wnd_ota_upgrade")== GXCORE_SUCCESS)) 	
	{
		return 0;
	}

	{
		PopDlg sleep_pop;

		g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);

		memset(&sleep_pop, 0, sizeof(PopDlg));
		sleep_pop.type = POP_TYPE_YES_NO;
		sleep_pop.mode = POP_MODE_UNBLOCK;
		sleep_pop.str = STR_ID_POWER_OFF;
		sleep_pop.timeout_sec = SLEEP_INFO_SEC;
		sleep_pop.default_ret = POP_VAL_OK;
		sleep_pop.show_time = true;
		sleep_pop.exit_cb = _sleep_exec;
		popdlg_create(&sleep_pop);
	}
	
	return 0;
}

static void app_set_sleep_time(int sel)
{
	uint8_t min;
	time_t sleep_exec_sec;
	
	switch(sel)
	{
		case SLEEP_15_MIN:
			min = 15;
			break;

		case SLEEP_30_MIN:
			min = 30;
			break;

		case SLEEP_60_MIN:
			min = 60;
			break;

		case SLEEP_120_MIN:
			min = 120;
			break;

		default:
			min = 0;
			break;
	}

	if(min == 0)
	{
		remove_timer(sp_TimerSleepExec);
		sp_TimerSleepExec = NULL;
	}
	else
	{
		if (reset_timer(sp_TimerSleepExec) != 0) 
		{
			sleep_exec_sec = min * 60 - SLEEP_INFO_SEC;
			sp_TimerSleepExec = create_timer(_sleep_exec_timeout, sleep_exec_sec * 1000, NULL, TIMER_ONCE);
		}
	}

}

SIGNAL_HANDLER int app_sleep_set_cmb_change(const char* widgetname, void *usrdata)
{
	GUI_GetProperty(CMB_SLEEP, "select", &s_sleep_sel);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sleep_set_cmb_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_LEFT:
			case STBK_RIGHT:
				ret = EVENT_TRANSFER_STOP;
				break;

			default:
				break;
		}
	}
	
	return ret;
}

SIGNAL_HANDLER int app_sleep_set_create(const char* widgetname, void *usrdata)
{
	GUI_SetProperty(CMB_SLEEP, "select", &s_sleep_sel);
	app_set_sleep_time(s_sleep_sel);

	if (reset_timer(sp_TimerSleepEnd) != 0) 
	{
		sp_TimerSleepEnd = create_timer(_sleep_end_timeout, 5000, NULL, TIMER_ONCE);
	}
	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sleep_set_destroy(const char* widgetname, void *usrdata)
{
	remove_timer(sp_TimerSleepEnd);
	sp_TimerSleepEnd = NULL;
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sleep_set_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
#if (DLNA_SUPPORT > 0)
            case VK_DLNA_TRIGGER:
                GUI_EndDialog("wnd_sleep_set");
                GUI_SetInterface("flush", NULL);
                GUI_SendEvent(GUI_GetFocusWindow(), event);
                ret = EVENT_TRANSFER_STOP;
                break;
#endif

			case STBK_EXIT:
				GUI_EndDialog(WND_SLEEP);
				break;

			case STBK_SLEEP:
				reset_timer(sp_TimerSleepEnd);
				(s_sleep_sel == SLEEP_120_MIN) ? (s_sleep_sel = SLEEP_OFF) : (s_sleep_sel++);
				GUI_SetProperty(CMB_SLEEP, "select", &s_sleep_sel);
				app_set_sleep_time(s_sleep_sel);
				break;

			default:
				GUI_EndDialog(WND_SLEEP);
				GUI_SetProperty(WND_SLEEP, "draw_now", NULL);
				GUI_SendEvent("wnd_full_screen", event);
				break;
		}		
	}
	return ret;
}

void app_init_sleep_set(void)
{
	remove_timer(sp_TimerSleepExec);
	sp_TimerSleepExec = NULL;
	s_sleep_sel = SLEEP_OFF;
}


#if THREE_HOURS_AUTO_STANDBY

#define AUTO_STANDBY_INFO_SEC (120)
#define THREE_HOURS_POWER  (3*60*60*1)
static event_list * sp_AutoStandbyExec = NULL;
extern int app_auto_standby_status(void);
static int _auto_standy_exec_timeout(void* data)
{
	PopDlg sleep_pop;
	
	remove_timer(sp_AutoStandbyExec);
	sp_AutoStandbyExec = NULL;

	if((GUI_CheckDialog("wnd_search") == GXCORE_SUCCESS)
		|| (GUI_CheckDialog("wnd_upgrade_process")== GXCORE_SUCCESS)
		|| (GUI_CheckDialog("wnd_ota_upgrade")== GXCORE_SUCCESS)) 	
	{
		return 0;
	}


	{
		g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);

		memset(&sleep_pop, 0, sizeof(PopDlg));
		sleep_pop.type = POP_TYPE_YES_NO;
		sleep_pop.mode = POP_MODE_UNBLOCK;
		sleep_pop.str = STR_ID_POWER_OFF;
		sleep_pop.timeout_sec = AUTO_STANDBY_INFO_SEC;
		sleep_pop.default_ret = POP_VAL_OK;
		sleep_pop.show_time = true;
		sleep_pop.exit_cb = _sleep_exec;
		popdlg_create(&sleep_pop);
	}
	
	
	return 0;
}

void timer_auto_standby_reset(void)
{
	reset_timer(sp_AutoStandbyExec);
	return;
}

void app_set_auto_standby_time(int mode)
{
	time_t sleep_exec_sec;
	int auto_standby_flag = 0;
	if(mode == 0)
	{
		remove_timer(sp_AutoStandbyExec);
		sp_AutoStandbyExec = NULL;
	}
	else
	{
		auto_standby_flag = app_auto_standby_status();
		if((reset_timer(sp_AutoStandbyExec) != 0) &&(1 == auto_standby_flag))
		{
			sleep_exec_sec = THREE_HOURS_POWER - AUTO_STANDBY_INFO_SEC;
			sp_AutoStandbyExec = create_timer(_auto_standy_exec_timeout, sleep_exec_sec * 1000, NULL, TIMER_ONCE);
		}
	}
	
	return;
}

#endif

