//#include "app_root.h"
#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "full_screen.h"
#include "app_pop.h"
#include "app_book.h"
#include "app_default_params.h"

#define IMG_STATE       "img_full_state"
#define TITLE_STATE     "txt_full_title"
#define TXT_STATE       "txt_full_state"

#if (DEMOD_DTMB > 0)
bool s_menuBack = false;
#endif
extern status_t app_create_epg_menu(void);
extern void     app_ttx_magz_open(void);
extern bool  app_check_av_running(void);
extern void app_sysinfo_menu_exec(void);
extern void app_create_pvr_media_fullsreeen(void);
extern void app_create_media_centre_fullsreeen(void);
extern void app_create_timer_setting_fullsreeen(void);
extern bool app_check_play_rec_flag(void);

extern void app_pvr_set_before_stop_state(pvr_state state);
extern int app_pvr_stop_cb(PopDlgRet ret);


#if (DEMOD_DTMB > 0)
void app_full_screen_is_menu_back_set(bool in)
{
    s_menuBack = in;
}

bool app_full_screen_is_menu_back_get(void)
{
    return s_menuBack;
}
#endif

static int32_t tms_flag_check(void)
{
    int32_t tms_flag = 0;

    GxBus_ConfigGetInt(PVR_TIMESHIFT_KEY, &tms_flag, PVR_TIMESHIFT_FLAG);
    return tms_flag;
}
#define PROG_STREAM_TYPE(play_ops)   play_ops.normal_play.view_info.stream_type

#ifdef REC_FULL_RECALL_SUPPORT
void full_recall_arbitrate(void)
{
    FullArb *arb = &g_AppFullArb;

    GUI_SetProperty(TXT_STATE, "string", "");
    GUI_SetProperty(IMG_STATE, "state", "osd_trans_hide");
    GUI_SetProperty(TITLE_STATE, "state", "osd_trans_hide");
    GUI_SetProperty(TXT_STATE, "state", "osd_trans_hide");

    arb->exec[EVENT_RECALL](arb);
    arb->draw[EVENT_RECALL](arb);
	GUI_SetProperty("wnd_full_screen", "draw_now", NULL);

	return ;
}
#endif

#if 0
void app_muti_ts_status(void)
{
     uint8_t tsid[256] = {0};
     uint8_t ts_num,i;
     static uint8_t  tsid_sel;
     ts_num = GxFrontend_GetTsid(0,tsid);
     for(i = 0;i<ts_num;i++)
     printf(" tsid [%d] = %d \n",i,tsid[i]);

     printf("\n ");

     tsid_sel ++;
     if(tsid_sel >= ts_num)
     tsid_sel = 0;

     printf("tsid_sel[ %d] = %d\n ",tsid_sel,tsid[tsid_sel]);
     GxFrontend_SetTsid(0,tsid[tsid_sel]);

}

#endif

void full_arbitrate(uint32_t key)
{
    FullArb *arb = &g_AppFullArb;
	usb_check_state usb_state = USB_OK;
	PopDlg pop;

    switch (key)
    {
        case STBK_TV:
		case STBK_RADIO:
			if(((PROG_STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_TV) 
					&& (STBK_TV == key))
				|| ((PROG_STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_RADIO) 
					&& (STBK_RADIO == key)))
			{
					break;
			}
        case STBK_TV_RADIO:
            GUI_SetProperty(TXT_STATE, "string", "");
            GUI_SetProperty(IMG_STATE, "state", "osd_trans_hide");
            GUI_SetProperty(TITLE_STATE, "state", "osd_trans_hide");
            GUI_SetProperty(TXT_STATE, "state", "osd_trans_hide");

            arb->exec[EVENT_TV_RADIO](arb);
            arb->draw[EVENT_TV_RADIO](arb);
            GUI_SetProperty("wnd_full_screen", "draw_now", NULL);
            if (g_AppPlayOps.normal_play.play_total != 0
			&&(GUI_CheckDialog("wnd_passwd") != GXCORE_SUCCESS))
            {
                //app_create_dialog("wnd_channel_info");
            }
            break;
        case STBK_MUTE:
            arb->exec[EVENT_MUTE](arb);
            arb->draw[EVENT_MUTE](arb);
            break;
        case STBK_PAUSE:
        case STBK_PAUSE_STB:
        case STBK_TMS:
				
            if((g_AppPvrOps.state != PVR_DUMMY) || (app_check_av_running() == true) || (arb->state.pause == STATE_ON))
            {
                arb->exec[EVENT_PAUSE](arb);
                arb->draw[EVENT_PAUSE](arb);
            }
			if(tms_flag_check() ==STATE_ON && arb->state.pause == STATE_ON)
			{
				memset(&pop, 0, sizeof(PopDlg));
				usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
				if(usb_state == USB_NO_SPACE)
				{
					pop.type = POP_TYPE_NO_BTN;
					pop.format = POP_FORMAT_DLG;
					pop.mode = POP_MODE_UNBLOCK;
					pop.str = STR_ID_NO_SPACE;
					pop.creat_cb = NULL;
					pop.exit_cb = NULL;
					pop.timeout_sec= 3;
					popdlg_create(&pop);
					break;
				}
			}
			if(g_AppPvrOps.state == PVR_DUMMY)
			{
				GxMsgProperty_NodeByPosGet node_prog = {0};

				node_prog.node_type = NODE_PROG;
				node_prog.pos = g_AppPlayOps.normal_play.play_count;
				app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
				if((tms_flag_check() ==STATE_ON)&& (g_AppPvrOps.usb_check(&g_AppPvrOps) == USB_OK))
				{
					if(node_prog.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE || node_prog.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
					{
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_NO_BTN;
						pop.format = POP_FORMAT_DLG;
						pop.str = STR_ID_CANT_PVR;
						pop.mode = POP_MODE_UNBLOCK;
						pop.creat_cb = NULL;
						pop.exit_cb = NULL;
						pop.timeout_sec= 3;
						popdlg_create(&pop);
					}
					else 
					{
						if(node_prog.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE)
						{
							AppFrontend_LockState lock;
							app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);
							g_AppPlayOps.set_rec_time(0x0f);
							g_AppPlayOps.program_play(PLAY_MODE_WITH_TMS, g_AppPlayOps.normal_play.play_count);
							g_AppFullArb.state.pause = STATE_OFF;
							//g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
							//g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);

							break;
						}
					}
				}
			}
			//           }
            break;

        case STBK_RECALL:
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
			if(g_AppFullArb.tip == FULL_STATE_DUMMY)
			{
				GUI_SetProperty(TXT_STATE, "string", "");
				GUI_SetProperty(IMG_STATE, "state", "osd_trans_hide");
				GUI_SetProperty(TITLE_STATE, "state", "osd_trans_hide");
				GUI_SetProperty(TXT_STATE, "state", "osd_trans_hide");
			}
			if(arb->state.pause == STATE_ON)
			{
				arb->state.pause = STATE_OFF;
				arb->draw[EVENT_PAUSE](arb);
			}
            arb->exec[EVENT_RECALL](arb);
            arb->draw[EVENT_RECALL](arb);
			GUI_SetProperty("wnd_full_screen", "draw_now", NULL);
            if (g_AppPlayOps.normal_play.play_total != 0
			&&(GUI_CheckDialog("wnd_passwd") != GXCORE_SUCCESS))
            {
                //app_create_dialog("wnd_channel_info");
            }
            break;

        default:
            break;
    }
}

#if (DLNA_SUPPORT > 0)
static void app_fullscreen_start_dlna_cb(void)
{
    g_AppFullArb.timer_stop();
	if((g_AppPvrOps.state == PVR_TMS_AND_REC) || (g_AppPvrOps.state == PVR_TIMESHIFT))
        app_pvr_tms_stop();
    g_AppFullArb.state.pause = STATE_OFF;
    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    g_AppPlayOps.program_stop();
}

static void app_fullscreen_stop_dlna_cb(void)
{
    GUI_SetProperty("wnd_full_screen", "back_ground", "null");
    GUI_SetInterface("flush", NULL);
    GUI_SetInterface("video_enable", NULL);

    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
    if (g_AppPlayOps.normal_play.play_total != 0)
    {
        g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    }
}
#endif

SIGNAL_HANDLER int app_full_screen_create(GuiWidget *widget, void *usrdata)
{
	printf("enter %s:%d\n",__func__,__LINE__);
    GxBusPmViewInfo view_info = {0};

    // get system info
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    if (view_info.pip_main_tp_id != 0)
    {
        pvr_taxis_mode_clear();
    }
    else
    {
#if (0 == TKGS_SUPPORT)	//2014.09.25, d
        if(view_info.taxis_mode != TAXIS_MODE_NON)
        {
            view_info.taxis_mode = TAXIS_MODE_NON;
        }
#else
        view_info.taxis_mode = TAXIS_MODE_NON;
#endif
        g_AppPlayOps.play_list_create(&view_info);
    }

    // time start
    g_AppTime.init(&g_AppTime);
    g_AppTime.start(&g_AppTime);
    g_AppFullArb.init(&g_AppFullArb);
	if(PROG_STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_RADIO)
	{
		g_AppFullArb.state.tv = STATE_OFF;
	}
    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
    g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT,g_AppPlayOps.normal_play.play_count);
    // full arbitrate
    g_AppFullArb.timer_start();

    if (g_AppPlayOps.normal_play.play_total != 0)
    {
        //app_create_dialog("wnd_channel_info");
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_full_screen_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_full_screen_keypress(GuiWidget *widget, void *usrdata)
{
    static int8_t factory_test_num = 0;
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    AppPvrOps *pvr = &g_AppPvrOps;
    pvr_state cur_pvr = PVR_DUMMY;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            {
                if(app_check_play_rec_flag() == true)
                    return ret;
                if (g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt,
                            find_virtualkey_ex(event->key.scancode,event->key.sym)) == GXCORE_SUCCESS)
                {
                    #define IMG_STATE    "img_full_state"
                    #define IMG_REC      "img_full_rec"
                    #if (MINI_16_BITS_OSD_SUPPORT == 1)
                    #define IMG_MUTE_1   "img_full_mute1"
                    #endif
                    #define IMG_MUTE     "img_full_mute"
                    #define IMG_PAUSE    "img_full_pause"

                    if(event->key.sym == STBK_EXIT)
                    {
                        GUI_SetProperty(IMG_STATE, "img", "s_icon_ts.bmp");
                        GUI_SetProperty(IMG_REC, "img", "s_pvr_rec.bmp");

#if (MINI_16_BITS_OSD_SUPPORT == 1)
                        //Refresh mute1
                        if(g_AppFullArb.state.mute == STATE_ON)
                        {
                            GUI_SetProperty(IMG_MUTE_1, "state", "hide");
                            GUI_SetProperty(IMG_MUTE_1, "state", "show");
                        }
#endif
                        GUI_SetProperty(IMG_MUTE, "img", "s_vol_mute.bmp");
                        GUI_SetProperty(IMG_PAUSE, "img", "s_pause.bmp");
                    }

                    return ret;
                }

                switch(event->key.sym)
                {
#if (DLNA_SUPPORT > 0)
                    case VK_DLNA_TRIGGER:
                        app_dlna_ui_exec(app_fullscreen_start_dlna_cb, app_fullscreen_stop_dlna_cb);
                        break;
#endif
                    case STBK_SYS_INFO:
                    case STBK_OK:
                        if (g_AppPlayOps.normal_play.play_total != 0)
                        {
                            GUI_SetProperty("txt_full_state", "state", "hide");
                            GUI_SetProperty("img_full_state", "state", "hide");
                            GUI_SetProperty("txt_full_title", "state", "hide");
                            if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                                GUI_EndDialog("wnd_channel_info");
                            app_create_dialog("wnd_channel_list");
                        }
                        break;

                    case STBK_MENU:
                        {
                            if (pvr->state != PVR_DUMMY)
                            {
                                PopDlg pop;
                                memset(&pop, 0, sizeof(PopDlg));
                                pop.type = POP_TYPE_NO_BTN;
                                pop.format = POP_FORMAT_DLG;
                                pop.str = STR_ID_STOP_PVR_FIRST;
                                pop.mode = POP_MODE_UNBLOCK;
                                pop.timeout_sec = 3;
                                popdlg_create(&pop);
                            }
                            else
                            {
                                GUI_SetProperty("txt_full_state", "state", "hide");
                                GUI_SetProperty("img_full_state", "state", "hide");
                                GUI_SetProperty("txt_full_title", "state", "hide");
                                if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                                    GUI_EndDialog("wnd_channel_info");

                                g_AppFullArb.timer_stop();
                                if(g_AppFullArb.state.pause == STATE_ON)
                                {
                                    g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
                                    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                                }
#if (MINI_16BIT_WIN8_OSD_SUPPORT == 0)
                                g_AppPlayOps.program_stop();
#endif
                                //the following 4 line codes for #47966 
                                uint32_t state_mute_bak = g_AppFullArb.state.mute;
                                g_AppFullArb.state.mute = STATE_OFF;
                                g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
                                g_AppFullArb.state.mute = state_mute_bak;

                                app_subt_pause();
                                app_create_dialog("wnd_main_menu");
#if (DEMOD_DTMB > 0)
                                s_menuBack = true;
#endif
                            }
                            break;
                        }

#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
                        //quick create main_menu_item
                    case STBK_MEDIA:
                        {
                            if (pvr->state != PVR_DUMMY)
                            {
                                PopDlg pop;
                                memset(&pop, 0, sizeof(PopDlg));
                                pop.type = POP_TYPE_NO_BTN;
                                pop.format = POP_FORMAT_DLG;
                                pop.str = STR_ID_STOP_PVR_FIRST;
                                pop.mode = POP_MODE_UNBLOCK;
                                pop.timeout_sec = 3;
                                popdlg_create(&pop);
                            }
                            else
                            {
                                GUI_SetProperty("txt_full_state", "state", "hide");
                                GUI_SetProperty("img_full_state", "state", "hide");
                                GUI_SetProperty("txt_full_title", "state", "hide");

                                if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                                    GUI_EndDialog("wnd_channel_info");

                                g_AppFullArb.timer_stop();
                                g_AppFullArb.state.pause = STATE_OFF;
                                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                                g_AppPlayOps.program_stop();
                                //extern void app_main_menu_media_item_create(int value);
                                //int sel = 4;//MM_MEDIA
                                //app_main_menu_media_item_create(sel);

                            }
                            break;
                        }
#endif

                    case STBK_CH_UP:
                    case STBK_UP:
                        {
                            if (g_AppPvrOps.state != PVR_DUMMY)
                            {
                                PopDlg pop;

                                memset(&pop, 0, sizeof(PopDlg));
                                pop.type = POP_TYPE_YES_NO;

                                if(g_AppPvrOps.state == PVR_TP_RECORD)
                                {
                                    pop.str = STR_ID_STOP_REC_INFO;
                                    cur_pvr = PVR_TP_RECORD;
                                }
                                else if(g_AppPvrOps.state == PVR_TMS_AND_REC)
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

                            if (g_AppPlayOps.normal_play.play_total != 0)
                            {
                                g_AppFullArb.state.pause = STATE_OFF;
                                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                                g_AppFullArb.tip = FULL_STATE_DUMMY;
                                g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);

                                g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_NEXT, 0);
                                //app_create_dialog("wnd_channel_info");
                            }
                            break;
                        }

                    case STBK_CH_DOWN:
                    case STBK_DOWN:
                        {
                            if (g_AppPvrOps.state != PVR_DUMMY)
                            {
                                PopDlg pop;

                                memset(&pop, 0, sizeof(PopDlg));
                                pop.type = POP_TYPE_YES_NO;

                                if(g_AppPvrOps.state == PVR_TP_RECORD)
                                {
                                    pop.str = STR_ID_STOP_REC_INFO;
                                    cur_pvr = PVR_TP_RECORD;
                                }
                                else if(g_AppPvrOps.state == PVR_TMS_AND_REC)
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

                            if (g_AppPlayOps.normal_play.play_total != 0)
                            {
                                g_AppFullArb.state.pause = STATE_OFF;
                                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                                g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_PREV, 0);
                                //app_create_dialog("wnd_channel_info");
                            }
                            break;
                        }

                    case STBK_EPG:
                        if (g_AppPlayOps.normal_play.play_total != 0)
                        {
                            if (pvr->state != PVR_DUMMY)
                            {
                                PopDlg pop;
                                memset(&pop, 0, sizeof(PopDlg));
                                pop.type = POP_TYPE_NO_BTN;
                                pop.format = POP_FORMAT_DLG;
                                pop.str = STR_ID_STOP_PVR_FIRST;
                                pop.mode = POP_MODE_UNBLOCK;
                                pop.timeout_sec = 3;
                                popdlg_create(&pop);
                            }
                            else
                            {
#if 0//(MINI_256_COLORS_OSD_SUPPORT == 1)
                                g_AppFullArb.timer_stop();
#endif
                                GUI_SetProperty(TXT_STATE, "string", " ");
                                GUI_SetProperty(IMG_STATE, "state", "osd_trans_hide");
                                GUI_SetProperty(TITLE_STATE, "state", "osd_trans_hide");
                                GUI_SetProperty(TXT_STATE, "state", "osd_trans_hide");
#define IMG_PAUSE    "img_full_pause"
                                GUI_SetProperty(IMG_PAUSE, "state", "osd_trans_hide");
                                //GUI_SetInterface("flush",NULL);

                                if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                                    GUI_EndDialog("wnd_channel_info");

                                if(g_AppFullArb.state.pause == STATE_ON)
                                {
                                    g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
                                    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                                }
                                app_create_epg_menu();
                            }
                        }
                        break;

                    case STBK_SAT:
                    case STBK_FAV:
                        {
                            if (g_AppPlayOps.normal_play.play_total != 0)
                            {
                                GUI_SetProperty("txt_full_state", "state", "hide");
                                GUI_SetProperty("img_full_state", "state", "hide");
                                GUI_SetProperty("txt_full_title", "state", "hide");

                                if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                                    GUI_EndDialog("wnd_channel_info");

                                app_create_dialog("wnd_channel_list");
                                GUI_SendEvent("wnd_channel_list", event);
                            }
                            break;
                        }
                    case STBK_TV:
                    case STBK_RADIO:
                        if(((PROG_STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_TV)
                                    && (STBK_TV == event->key.sym))
                                || ((PROG_STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_RADIO)
                                    && (STBK_RADIO == event->key.sym)))
                        {
                            break;
                        }
                    case STBK_TV_RADIO:
                        {
                            if(pvr->state != PVR_DUMMY)
                            {
                                PopDlg pop;
                                memset(&pop, 0, sizeof(PopDlg));
                                pop.type = POP_TYPE_NO_BTN;
                                pop.format = POP_FORMAT_DLG;
                                pop.str = STR_ID_STOP_PVR_FIRST;
                                pop.mode = POP_MODE_UNBLOCK;
                                pop.timeout_sec = 3;
                                popdlg_create(&pop);
                            }
                            else
                            {
                                if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                                    GUI_EndDialog("wnd_channel_info");
                                full_arbitrate(event->key.sym);
                            }
                            break;
                        }

                    case STBK_PAUSE:
                    case STBK_PAUSE_STB:
                    case STBK_TMS:
                    case STBK_MUTE:
                        if (g_AppPlayOps.normal_play.play_total != 0)
                        {
                            full_arbitrate(event->key.sym);
                        }
                        break;

                    case STBK_RECALL:
                        if ((g_AppPlayOps.normal_play.play_total != 0)
#ifndef REC_FULL_RECALL_SUPPORT
                                &&(pvr->state == PVR_DUMMY)
#endif
                           )
                        {
                            full_arbitrate(event->key.sym);
                        }
                        break;
#if PVR_SUPPORT
                    case STBK_REC_START:
                        {
                            usb_check_state usb_state = USB_OK;
                            GxMsgProperty_NodeByPosGet node_prog = {0};

#if (0 == REDIO_RECODE_SUPPORT)
                            if(g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_TV)
                                break;
#endif
                            if (g_AppPlayOps.normal_play.play_total == 0)
                                break;

                            if(g_AppPvrOps.state == PVR_DUMMY)
                            {
                                PopDlg pop;
                                // check usb state
                                usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
                                if(usb_state == USB_ERROR)
                                {
                                    memset(&pop, 0, sizeof(PopDlg));
                                    pop.type = POP_TYPE_NO_BTN;
                                    pop.format = POP_FORMAT_DLG;
                                    pop.str = STR_ID_INSERT_USB;
                                    pop.mode = POP_MODE_UNBLOCK;
                                    pop.creat_cb = NULL;
                                    pop.exit_cb = NULL;
                                    pop.timeout_sec= 3;
                                    popdlg_create(&pop);

                                    break;
                                }

                                if(usb_state == USB_NO_SPACE)
                                {
                                    memset(&pop, 0, sizeof(PopDlg));
                                    pop.type = POP_TYPE_NO_BTN;
                                    pop.format = POP_FORMAT_DLG;
                                    pop.str = STR_ID_NO_SPACE;
                                    pop.mode = POP_MODE_UNBLOCK;
                                    pop.creat_cb = NULL;
                                    pop.exit_cb = NULL;
                                    pop.timeout_sec= 3;
                                    popdlg_create(&pop);

                                    break;
                                }

                                if((app_check_av_running() == true) && (0xff != g_AppPmt.ver))
                                {
                                    if(g_AppFullArb.state.pause == STATE_ON)
                                    {
                                        g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
                                        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                                    }
                                    app_pvr_rec_start(0);
                                    app_create_dialog("wnd_pvr_bar");
                                    GUI_SetProperty("wnd_pvr_bar", "draw_now", NULL);
                                    break;
                                }

                                // prog
                                node_prog.node_type = NODE_PROG;
                                node_prog.pos = g_AppPlayOps.normal_play.play_count;
                                app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
                                if((node_prog.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
                                        ||(node_prog.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE))
                                {
                                    memset(&pop, 0, sizeof(PopDlg));
                                    pop.type = POP_TYPE_NO_BTN;
                                    pop.format = POP_FORMAT_DLG;
                                    pop.str = STR_ID_CANT_PVR;
                                    pop.mode = POP_MODE_UNBLOCK;
                                    pop.creat_cb = NULL;
                                    pop.exit_cb = NULL;
                                    pop.timeout_sec= 3;
                                    popdlg_create(&pop);
                                }
                                else if(node_prog.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE)
                                {
                                    AppFrontend_LockState lock;
                                    app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);
                                    g_AppPlayOps.set_rec_time(0);
                                    g_AppPlayOps.program_play(PLAY_MODE_WITH_REC, g_AppPlayOps.normal_play.play_count);
                                    g_AppFullArb.state.pause = STATE_OFF;
                                    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                                    break;
                                }


                            }
                            else
                            {
                                if(g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_RADIO) 
                                {
                                    if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                                        GUI_EndDialog("wnd_channel_info");
                                }
                                app_create_dialog("wnd_pvr_bar");
                                GUI_SetProperty("wnd_pvr_bar", "draw_now", NULL);

                                //#4329
                                if(pvr->state != PVR_RECORD)
                                    GUI_SendEvent("wnd_pvr_bar", event);
                            }
                            break;
                        }

                    case STBK_REC_STOP:
                        {
                            PopDlg  pop;

                            if ((g_AppPvrOps.state != PVR_RECORD)
                                    && (g_AppPvrOps.state != PVR_TIMESHIFT)
                                    && (g_AppPvrOps.state != PVR_TMS_AND_REC)
                                    && (g_AppPvrOps.state != PVR_TP_RECORD))
                                break;

                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_YES_NO;
                            pop.format = POP_FORMAT_DLG;
                            if(g_AppPvrOps.state == PVR_TIMESHIFT)
                            {
                                pop.str = STR_ID_STOP_TMS_INFO;
                            }
                            else
                            {
                                pop.str = STR_ID_STOP_REC_INFO;
                            }


                            app_pvr_set_before_stop_state(g_AppPvrOps.state);
                            pop.mode = POP_MODE_UNBLOCK;
                            pop.exit_cb  =   app_pvr_stop_cb;
                            popdlg_create(&pop);
                            break;
                        }

                    case STBK_PLAY:

                        if((g_AppPvrOps.state == PVR_DUMMY) || (g_AppPvrOps.state == PVR_TP_RECORD))
                        {
                            if(g_AppFullArb.state.pause == STATE_ON)
                            {
                                g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
                                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                            }
                        }
                        else if((g_AppPvrOps.state == PVR_TIMESHIFT)
                                ||(g_AppPvrOps.state == PVR_TMS_AND_REC))
                        {
                            app_create_dialog("wnd_pvr_bar");
                            GUI_SetProperty("wnd_pvr_bar", "draw_now", NULL);
                            if(g_AppFullArb.state.pause == STATE_ON)
                            {
                                GUI_SendEvent("slider_pvr_seek", event);
                            }
                            else
                            {
                                GUI_SendEvent("slider_pvr_process", event);
                            }
                        }

                        break;

                    case STBK_FF:
                    case STBK_FB:
                        if((g_AppPvrOps.state == PVR_TIMESHIFT)
                                ||(g_AppPvrOps.state == PVR_TMS_AND_REC))
                        {
                            app_create_dialog("wnd_pvr_bar");
                            GUI_SetProperty("wnd_pvr_bar", "draw_now", NULL);
                            GUI_SendEvent("slider_pvr_seek", event);
                        }

                        break;
#endif
                    case STBK_VOLDOWN:
                    case STBK_VOLUP:
                    case STBK_LEFT:
                    case STBK_RIGHT:
                        if (g_AppPlayOps.normal_play.play_total != 0)
                        {
                            GUI_EndDialog("wnd_number");
                            app_create_dialog("wnd_volume_value");
                            GUI_SendEvent("wnd_volume_value", event);
                        }
                        break;

                    case STBK_1:
                    case STBK_2:
                    case STBK_3:
                    case STBK_4:
                    case STBK_5:
                    case STBK_6:
                    case STBK_7:
                    case STBK_8:
                    case STBK_9:
                    case STBK_0:
                        if(g_AppPlayOps.normal_play.play_total == 0)
                        {
                            break;
                        }

                        {
                            if (g_AppPvrOps.state != PVR_DUMMY)
                            {
                                PopDlg pop;

                                memset(&pop, 0, sizeof(PopDlg));
                                pop.type = POP_TYPE_YES_NO;

                                if(g_AppPvrOps.state == PVR_TP_RECORD)
                                {
                                    pop.str = STR_ID_STOP_REC_INFO;
                                    cur_pvr = PVR_TP_RECORD;
                                }
                                else if(g_AppPvrOps.state == PVR_TMS_AND_REC)
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
                                    app_pvr_set_before_stop_state(cur_pvr);
                                    pop.exit_cb  =   app_pvr_stop_cb;
                                    if (POP_VAL_OK != popdlg_create(&pop))
                                    {
                                        break;
                                    }
                                }
                            }

                            GUI_EndDialog("wnd_volume_value");
                            app_create_dialog("wnd_number");
                            GUI_SendEvent("wnd_number", event);
                            break;
                        }

                    case STBK_TTX:
                        app_ttx_magz_open();
                        break;

                    case STBK_SUBT:
                        if(g_AppPlayOps.normal_play.play_total == 0)
                        {
                            break;
                        }

                        if((g_AppPvrOps.state == PVR_TIMESHIFT)
                                || (g_AppPvrOps.state == PVR_TMS_AND_REC))
                        {
                            PopDlg pop;
                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_NO_BTN;
                            pop.format = POP_FORMAT_DLG;
                            pop.str = STR_ID_STOP_PVR_FIRST;
                            pop.mode = POP_MODE_UNBLOCK;
                            pop.timeout_sec = 3;
                            popdlg_create(&pop);
                            break;
                        }
                        GUI_SetProperty("txt_full_state", "state", "hide");
                        GUI_SetProperty("img_full_state", "state", "hide");
                        GUI_SetProperty("txt_full_title", "state", "hide");
                        if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                            GUI_EndDialog("wnd_channel_info");

                        app_create_dialog("wnd_subtitling");
                        break;

                    case STBK_INFO:
                        if (g_AppPlayOps.normal_play.play_total != 0)
                        {
                            if (pvr->state != PVR_DUMMY)
                                app_create_dialog("wnd_pvr_bar");
                            else
                                app_create_dialog("wnd_channel_info");
                        }
                        break;

                    case STBK_F1:
                        {
                            factory_test_num++;
                            if (g_AppPlayOps.normal_play.play_total != 0)
                            {
                                GUI_SetProperty("txt_full_state", "state", "hide");
                                GUI_SetProperty("img_full_state", "state", "hide");
                                GUI_SetProperty("txt_full_title", "state", "hide");
                                if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                                    GUI_EndDialog("wnd_channel_info");

                                app_create_dialog("wnd_find");
                            }
                            break;
                        }

                    case STBK_MUL_PIC:
#if 0
                        {
#if FACTORY_TEST_SUPPORT
                            if((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
                                    &&(g_AppPlayOps.normal_play.play_total != 0))
                            {
                                if (pvr->state != PVR_DUMMY)
                                {
                                    PopDlg pop;
                                    memset(&pop, 0, sizeof(PopDlg));
                                    pop.type = POP_TYPE_NO_BTN;
                                    pop.format = POP_FORMAT_DLG;
                                    pop.str = STR_ID_STOP_PVR_FIRST;
                                    pop.mode = POP_MODE_UNBLOCK;
                                    pop.timeout_sec = 3;
                                    popdlg_create(&pop);
                                    break;
                                }
                            }

                            extern void app_factory_test_menu_exec(void);
                            app_factory_test_menu_exec();
#else
                            extern int app_video_multifeed_fullscreen(void);
                            app_video_multifeed_fullscreen();
#endif
                            break;
                        }
#endif
#if 0
                    case STBK_ZOOM:
                        if((g_AppPlayOps.normal_play.play_total == 0)
                                || (g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_TV)
                                || (g_AppFullArb.state.pause == STATE_ON)
                                || (g_AppFullArb.scramble_check() == TRUE)
                                || (g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK))
                        {
                            break;
                        }

                        app_create_dialog("wnd_zoom");
                        break;
#endif

                    case STBK_AUDIO:
                        if (g_AppPlayOps.normal_play.play_total != 0)
                        {
                            GxMsgProperty_NodeByPosGet node = {0};
                            node.node_type = NODE_PROG;
                            node.pos = g_AppPlayOps.normal_play.play_count;
                            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
                            {
                                if ((g_AppPvrOps.state != PVR_DUMMY)
                                        &&(node.prog_data.id == g_AppPvrOps.env.prog_id))
                                {
                                    PopDlg pop;
                                    memset(&pop, 0, sizeof(PopDlg));
                                    pop.type = POP_TYPE_NO_BTN;
                                    pop.format = POP_FORMAT_DLG;
                                    pop.str = STR_ID_STOP_PVR_FIRST;
                                    pop.mode = POP_MODE_UNBLOCK;
                                    pop.timeout_sec = 3;
                                    popdlg_create(&pop);
                                    break;
                                }
                                GUI_SetProperty("txt_full_state", "state", "hide");
                                GUI_SetProperty("img_full_state", "state", "hide");
                                GUI_SetProperty("txt_full_title", "state", "hide");

                                if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                                    GUI_EndDialog("wnd_channel_info");

                                app_create_dialog("wnd_audio");
                            }
                        }
                        break;

                        //case STBK_SYS_INFO:
                        //app_sysinfo_menu_exec();
                        //break;
                    case STBK_TIMER:
                        app_create_timer_setting_fullsreeen();
                        break;
                    case STBK_DISK_INFO:
                        app_create_pvr_media_fullsreeen();
                        break;
                    default:
                        break;
                }

                default:
                break;
            }
    }
    return ret;
}

SIGNAL_HANDLER int app_full_screen_got_focus(GuiWidget *widget, void *usrdata)
{
    extern void app_end_find_extern(void);
    app_end_find_extern();
    g_AppFullArb.timer_start();

    return EVENT_TRANSFER_STOP;
}



