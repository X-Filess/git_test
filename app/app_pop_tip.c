/*
 * =====================================================================================
 *
 *       Filename:  app_pop_tip.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2011年08月18日 09时47分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app_pop.h"
#include "app_utility.h"
#include "app_module.h"

#define WND_POPDLG          "wnd_pop_tip"
#define TXT_TITLE           "txt_pop_tip_title"
#define IMG_OPT             "img_pop_tip_opt"
#define BMP_OK              "s_bar_choice_blue4.bmp"
#define BMP_OK_R            "s_bar_choice_blue4.bmp"
#define BMP_CANCEL          "s_bar_choice_blue4_l.bmp"
#define BTN_OK              "btn_pop_tip_ok"
#define BTN_CANCEL          "btn_pop_tip_cancel"
#define TXT_TIP             "txt_pop_tip"
#define TXT_TITILE          "txt_pop_tip_title"
#define TXT_TIME            "txt_pop_time"

#define STR_OK              "OK"
#define STR_CANCEL          "Cancel"

#define SEC_PER_DAY (60 * 60 * 24)

static PopDlgRet       s_PopDlgRet        = POP_VAL_CANCEL;
static PopDlgRet       s_RetTemp        = POP_VAL_OK;
static PopDlg s_PopDlgParam = {0};
static const char *s_focus_wnd = NULL;

static enum
{
	POP_DLG_END,
	POP_DLG_START,
}s_PopDlgStatus = POP_DLG_END;

extern uint32_t app_diseqc12_get_tuner_id(void);

static event_list  *s_PopDlgTimer = NULL;
static char *s_time_str = NULL;
static bool s_pop_block_destroy_quick = false;

SIGNAL_HANDLER int app_popdlg_create(GuiWidget *widget, void *usrdata)
{
#if (MINI_16_BITS_OSD_SUPPORT == 1)
#define WND_POPTIP_X       320
#define WND_POPTIP_Y       152
#else
#define WND_POPTIP_X    400
#define WND_POPTIP_Y    190
#endif

    int32_t pos_x = 0;
    int32_t pos_y = 0;

    if(POP_TYPE_NO_BTN == s_PopDlgParam.type || POP_TYPE_WAIT == s_PopDlgParam.type)
    {
        GUI_SetProperty(IMG_OPT,    "state", "hide");
        GUI_SetProperty(BTN_OK,     "state", "hide");
        GUI_SetProperty(BTN_CANCEL, "state", "hide");
    }
    else if(POP_TYPE_OK == s_PopDlgParam.type)
    {
        GUI_SetProperty(BTN_CANCEL, "state", "hide");
        GUI_SetProperty(IMG_OPT, "img", BMP_OK_R);
        GUI_SetProperty(BTN_OK, "string", STR_OK);
        GUI_SetFocusWidget(BTN_OK);
    }
    else
    {
        if (s_RetTemp == POP_VAL_OK)
        {
            GUI_SetProperty(IMG_OPT, "img", BMP_OK);
            GUI_SetProperty(BTN_OK, "string", STR_OK);
            GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
            GUI_SetFocusWidget(BTN_OK);
        }
        else
        {
            GUI_SetProperty(IMG_OPT, "img", BMP_CANCEL);
            GUI_SetProperty(BTN_OK, "string", STR_OK);
            GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
            GUI_SetFocusWidget(BTN_CANCEL);
        }
    }

    if((s_PopDlgParam.pos.x == 0) && (s_PopDlgParam.pos.y == 0))
    {
        pos_x = WND_POPTIP_X;
        pos_y = WND_POPTIP_Y;
    }
    else
    {
        pos_x = s_PopDlgParam.pos.x;
        pos_y = s_PopDlgParam.pos.y;
    }
    GUI_SetProperty(WND_POPDLG, "move_window_x", &pos_x);
    GUI_SetProperty(WND_POPDLG, "move_window_y", &pos_y);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_popdlg_destroy(GuiWidget *widget, void *usrdata)
{
    if(s_PopDlgTimer != NULL)
    {
        remove_timer(s_PopDlgTimer);
        s_PopDlgTimer = NULL;
    }
#if MINI_256_COLORS_OSD_SUPPORT
   s_PopDlgRet = s_RetTemp;
#else
    s_RetTemp = POP_VAL_OK;
#endif

    if(s_time_str != NULL)
    {
        GxCore_Free(s_time_str);
        s_time_str = NULL;
    }

    GUI_SetProperty(TXT_TIME, "string", " ");
    //GUI_SetProperty(TXT_TIME, "draw_now", NULL); 
    if(s_PopDlgParam.mode == POP_MODE_BLOCK)
    {
        app_block_msg_init(g_app_msg_self);
    }
    
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_popdlg_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case STBK_OK:
                if ((s_PopDlgParam.type == POP_TYPE_YES_NO)
                        || (s_PopDlgParam.type == POP_TYPE_OK))
                {
                    timer_stop(s_PopDlgTimer);
                    s_PopDlgRet = s_RetTemp;
                    if(s_PopDlgParam.mode == POP_MODE_BLOCK)
                    {
                        s_PopDlgStatus = POP_DLG_END;
                    }
                    else
                    {
                        GUI_EndDialog(WND_POPDLG);
                        if(NULL != s_PopDlgParam.exit_cb)
                        {
                            GUI_SetInterface("flush",NULL);
                            s_PopDlgParam.exit_cb(s_PopDlgRet);
                        }
                    }
                }

                break;

            case STBK_LEFT:
            case STBK_RIGHT:
                if(s_PopDlgParam.type == POP_TYPE_YES_NO)
                {
                    if (s_RetTemp == POP_VAL_CANCEL)
                    {
                        s_RetTemp = POP_VAL_OK;
                        GUI_SetProperty(IMG_OPT, "img", BMP_OK);
                        GUI_SetProperty(BTN_OK, "string", STR_OK);
                        GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
                        GUI_SetFocusWidget(BTN_OK);
                    }
                    else
                    {
                        s_RetTemp = POP_VAL_CANCEL;
                        GUI_SetProperty(IMG_OPT, "img", BMP_CANCEL);
                        GUI_SetProperty(BTN_OK, "string", STR_OK);
                        GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
                        GUI_SetFocusWidget(BTN_CANCEL);
                    }
                }
                break;

            case STBK_MENU:
            case STBK_EXIT:
                timer_stop(s_PopDlgTimer);
                s_PopDlgRet = POP_VAL_CANCEL;
                if(s_PopDlgParam.mode == POP_MODE_BLOCK)
                {
                    s_PopDlgStatus = POP_DLG_END;
                }
                else
                {
                    GUI_EndDialog(WND_POPDLG);
                    if(NULL != s_PopDlgParam.exit_cb)
                    {
                        GUI_SetInterface("flush",NULL);
                        s_PopDlgParam.exit_cb(s_PopDlgRet);
                    }
                }
                break;

            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

extern event_list* text_auto_roll_timer;
static int pop_timeout_cb(void *usrdata)
{
    s_PopDlgParam.timeout_sec--;

    if(s_PopDlgParam.show_time == true)
    {
        if(s_time_str != NULL)
        {
            GxCore_Free(s_time_str);
            s_time_str = NULL;
        }
        s_time_str = app_time_to_hms_str(s_PopDlgParam.timeout_sec);
        GUI_SetProperty(TXT_TIME, "string", (void*)(s_time_str));
        GUI_SetProperty(TXT_TIME, "draw_now", NULL); 
    }    
    
    if(s_PopDlgParam.timeout_sec == 0)
    {
        timer_stop(s_PopDlgTimer);

        if(s_PopDlgParam.mode == POP_MODE_BLOCK)
        {
            s_PopDlgStatus = POP_DLG_END;
        }
        else
        {
            GUI_EndDialog(WND_POPDLG);
             if(NULL != s_PopDlgParam.exit_cb)
			 {
				 GUI_SetInterface("flush",NULL);
                s_PopDlgParam.exit_cb(s_PopDlgRet);
			 }
        }
    }

#if 0
	if(GUI_CheckDialog("win_text_view") == GXCORE_SUCCESS)
	{
		reset_timer(text_auto_roll_timer);
	}
#endif

    return 0;
}


/*
just for factory test
*/
void FactoryTest_EndDialog(void)
{
	GUI_EndDialog(WND_POPDLG);
}


PopDlgRet popdlg_create(PopDlg* dlg)
{
    time_t cur_time = 0;
    const char *wnd = GUI_GetFocusWindow();
	int min_count=0;
	s_focus_wnd = NULL;
	s_focus_wnd = GUI_GetFocusWindow(); 
	
    if(GUI_CheckDialog(WND_POPDLG) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_POPDLG);
        GUI_SetProperty(WND_POPDLG, "draw_now", NULL);
        if(NULL != s_PopDlgParam.exit_cb)
            s_PopDlgParam.exit_cb(s_PopDlgRet);
    }

    if((wnd != NULL) && (strcmp(wnd, "wnd_full_screen") == 0))
    {
    #define TXT_STATE       "txt_full_state"
    #define IMG_STATE       "img_full_state"
        GUI_SetProperty(TXT_STATE, "string", " ");
        GUI_SetProperty(TXT_STATE, "state", "hide");
        GUI_SetProperty(IMG_STATE, "state", "hide");
        GUI_SetProperty("txt_full_title", "state", "hide");
    }

#if 0
	if(GUI_CheckDialog("win_text_view") == GXCORE_SUCCESS)
	{
		timer_stop(text_auto_roll_timer);
	}
#endif

    memcpy(&s_PopDlgParam, dlg, sizeof(PopDlg));

    s_PopDlgStatus = POP_DLG_START;
    s_PopDlgRet = s_PopDlgParam.default_ret;
    s_RetTemp = s_PopDlgParam.default_ret;

    if (GUI_CheckDialog("wnd_subtitling") == GXCORE_SUCCESS)
        GUI_EndDialog("wnd_subtitling");
    app_create_dialog(WND_POPDLG);

    if(s_PopDlgParam.format == POP_FORMAT_DLG)
    {
        GUI_SetProperty(WND_POPDLG, "format", "dialog");
    }
    else
    {
        GUI_SetProperty(WND_POPDLG, "format", "popup");
    }

    if(s_PopDlgParam.title != NULL)
        GUI_SetProperty(TXT_TITILE, "string", (void*)(s_PopDlgParam.title));
    if(s_PopDlgParam.str != NULL)
        GUI_SetProperty(TXT_TIP, "string", (void*)(s_PopDlgParam.str));

    if(s_time_str != NULL)
    {
        GxCore_Free(s_time_str);
        s_time_str = NULL;
    }
    if(s_PopDlgParam.show_time == true)
    {
        if(s_PopDlgParam.timeout_sec == 0)
        {
            cur_time = g_AppTime.utc_get(&g_AppTime);
            cur_time = get_display_time_by_timezone(cur_time);
            cur_time %= SEC_PER_DAY;
            s_time_str = app_time_to_hourmin_edit_str(cur_time);
        }
        else
        {
            s_time_str = app_time_to_hms_str(s_PopDlgParam.timeout_sec);
        }

        GUI_SetProperty(TXT_TIME, "string", (void*)(s_time_str));
    }

	//GUI_SetInterface("flush",NULL);

    if(NULL != s_PopDlgParam.creat_cb)
        s_PopDlgParam.creat_cb();

    if(s_PopDlgParam.timeout_sec == 0)
    {
        if(POP_TYPE_NO_BTN == s_PopDlgParam.type)
        {
            if(s_PopDlgParam.mode == POP_MODE_BLOCK)
            {
                s_PopDlgStatus = POP_DLG_END;
            }
            else
            {
                GUI_EndDialog(WND_POPDLG);
                if(NULL != s_PopDlgParam.exit_cb)
				{
					GUI_SetInterface("flush",NULL);
                    s_PopDlgParam.exit_cb(s_PopDlgRet);
				}

#if 0
				if(GUI_CheckDialog("win_text_view") == GXCORE_SUCCESS)
				{
					reset_timer(text_auto_roll_timer);
				}
#endif
            }
        }
    }
    else
    {
        if(s_PopDlgTimer != NULL)
        {
            remove_timer(s_PopDlgTimer);
            s_PopDlgTimer = NULL;
        }
        
        if (reset_timer(s_PopDlgTimer) != 0) 
        {
            s_PopDlgTimer = create_timer(pop_timeout_cb, SEC_TO_MILLISEC,  NULL, TIMER_REPEAT);
        }
    }

	if(s_PopDlgParam.mode == POP_MODE_BLOCK)
	{
		app_block_msg_destroy(g_app_msg_self);      
		while(s_PopDlgStatus == POP_DLG_START)
		{
			if (strncmp(s_PopDlgParam.str, STR_ID_DISH_MOVING, sizeof(STR_ID_DISH_MOVING)) == 0)
			{
				if ((min_count++>10) && (GxFrontend_QueryStatus(app_diseqc12_get_tuner_id()) == 1))
				{
					//	printf("========tuner locked, break;\n");
					break;
				}
			}
			GUI_LoopEvent();
			GxCore_ThreadDelay(50);
		}
		
		GUI_StartSchedule();
		if(s_pop_block_destroy_quick == true)
		{
			s_pop_block_destroy_quick = false;
		}
		else
		{
			app_block_msg_init(g_app_msg_self);
			GUI_EndDialog(WND_POPDLG);
			if(NULL != s_PopDlgParam.exit_cb)
			{
				GUI_SetInterface("flush",NULL);
				s_PopDlgParam.exit_cb(s_PopDlgRet);
			}
		}
#if 0
		if(GUI_CheckDialog("win_text_view") == GXCORE_SUCCESS)
		{
			reset_timer(text_auto_roll_timer);
		}
#endif
	}    
	// add ----20130306
	else if(s_PopDlgParam.type == POP_TYPE_WAIT)
	{
		GUI_LoopEvent();
        GxCore_ThreadDelay(50);
	}
   // printf("g debug:line:%d:FUNC:%s",__LINE__,__FUNCTION__);
    return s_PopDlgRet;

}

void popdlg_destroy(void)
{
	if(GUI_CheckDialog(WND_POPDLG) == GXCORE_SUCCESS)
	{
		timer_stop(s_PopDlgTimer);
		s_PopDlgRet = POP_VAL_CANCEL;
		s_PopDlgStatus = POP_DLG_END;
		if(s_PopDlgParam.mode == POP_MODE_BLOCK)
		{
			s_pop_block_destroy_quick = true;
			app_block_msg_init(g_app_msg_self);
			GUI_EndDialog(WND_POPDLG);
			if(NULL != s_PopDlgParam.exit_cb)
			{
				GUI_SetInterface("flush",NULL);
			    s_PopDlgParam.exit_cb(s_PopDlgRet);
			}
		}
		else
		{
			GUI_EndDialog(WND_POPDLG);
			if(NULL != s_PopDlgParam.exit_cb)
			{

				GUI_SetInterface("flush",NULL);
			    s_PopDlgParam.exit_cb(s_PopDlgRet);
			}
		}
	}
}

void popdlg_update_status(PopDlg* dlg)
{
	#if 0
	int ret,state = -1;
		
	if( GUI_CheckDialog(WND_POPDLG) != GXCORE_SUCCESS)
	{
		app_create_dialog(WND_POPDLG);
	}
	
	if(dlg->str != NULL)
	{
		ret = GUI_SetProperty(TXT_TIP, "state","enable");
		ret = GUI_SetProperty(TXT_TIP, "state","show");
		ret = GUI_SetProperty(TXT_TIP, "string", (void*)dlg->str );
		ret = GUI_SetProperty(TXT_TIP, "draw_now", NULL);

		//GUI_GetProperty(TXT_TIP,"state",&state);
		//printf("in %s:%d state = %d\n",__func__,__LINE__,state);
	}
	#endif
	//....
}
