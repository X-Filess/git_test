#include "app_book.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "full_screen.h"
#include "channel_edit.h"
#define SEC_PER_DAY (60 * 60 * 24)

typedef enum
{
	EXEC_WND_NONE,
	EXEC_WND_FULLSCREEN,
	EXEC_WND_MENU,
}_BookExecWnd;

static bool s_tv_radio_switch = false;
static bool s_book_enable = false;
static event_list* s_book_ignore_timer = NULL;
static event_list* s_book_pop_timer = NULL;
static GxBook s_exec_book;
static const char *s_focus_wnd = NULL;
extern void app_low_power(time_t sleep_time, uint32_t scan_code);
extern bool app_check_av_running(void);
extern bool app_ota_idle(void);
extern void music_view_mode_pause(void);
static int32_t app_get_book(GxBookGet *pbookGet);
#if NETWORK_SUPPORT && WIFI_SUPPORT
extern void app_wifi_tip_flush(void);
extern void app_wifi_tip_lost_focus_hide(void);
#endif

//return  返回是星期几，0~6 指星期天，星期一...星期六
static WeekDay app_tm_get_wday(time_t time)
{
	struct tm *ptm;
	ptm = localtime(&time);
	return ptm->tm_wday;
}

static WeekDay app_book_mode2wday(uint32_t repeat_mode)
{
	WeekDay wday = WEEK_SUN;
	switch(repeat_mode)
	{
		case BOOK_REPEAT_MON:
			wday =  WEEK_MON;
			break;

		case BOOK_REPEAT_TUES:
			wday =  WEEK_TUES;
			break;

		case BOOK_REPEAT_WED:
			wday =  WEEK_WED;
			break;

		case BOOK_REPEAT_THURS:
			wday =  WEEK_THURS;
			break;

		case BOOK_REPEAT_FRI:
			wday =  WEEK_FRI;
			break;

		case BOOK_REPEAT_SAT:
			wday =  WEEK_SAT;
			break;

		case BOOK_REPEAT_SUN:
			wday =  WEEK_SUN;
			break;
			
		default:
			break;
	}

	return wday;
}

static uint32_t app_book_wday2mode(WeekDay wday)
{
	uint32_t mode = BOOK_REPEAT_ONCE;

	switch(wday)
	{
		case WEEK_MON:
			mode = BOOK_REPEAT_MON;
			break;

		case WEEK_TUES:
			mode = BOOK_REPEAT_TUES;
			break;

		case WEEK_WED:
			mode = BOOK_REPEAT_WED;
			break;

		case WEEK_THURS:
			mode = BOOK_REPEAT_THURS;
			break;

		case WEEK_FRI:
			mode = BOOK_REPEAT_FRI;
			break;

		case WEEK_SAT:
			mode = BOOK_REPEAT_SAT;
			break;

		case WEEK_SUN:
			mode = BOOK_REPEAT_SUN;
			break;

		default:
			break;
	}
	return mode;
}
static time_t app_get_sleep_time(time_t cur_time)
{
	GxBookGet bookGet;
	int count = 0;
	int i;
	time_t sleep_time = 0;
	time_t time_interval;
	time_t time_off;
	time_t time_on;
	
	count = app_get_book(&bookGet);
	time_interval = sleep_time;
	for(i = 0; i < count; i++)
	{
		if(bookGet.book[i].book_type == BOOK_POWER_ON)
		{
			if(bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE)
			{
				if(cur_time < bookGet.book[i].trigger_time_start)
				{
					time_interval = bookGet.book[i].trigger_time_start - cur_time;
				}
			}
			else if(bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_EVERY_DAY)
			{

				time_off = cur_time % SEC_PER_DAY;
				time_on = bookGet.book[i].trigger_time_start % SEC_PER_DAY;

				if(time_on < time_off)
					time_on += SEC_PER_DAY;

				time_interval = time_on - time_off;	
			}
			else
			{
				WeekDay wday1;
				WeekDay wday2;
				time_t time_temp1;
				time_t time_temp2;
				uint8_t day_count;
				
				time_temp1 = cur_time % SEC_PER_DAY;
				wday1 = app_tm_get_wday(cur_time);

				time_temp2 = bookGet.book[i].trigger_time_start % SEC_PER_DAY;
				wday2 = app_book_mode2wday(bookGet.book[i].repeat_mode.mode);
	
				if(wday2 > wday1)	
				{
					day_count = wday2 - wday1;
				}
				else if(wday2 < wday1)
				{
					day_count = (wday2 + 7) - wday1;
				}
				else //==
				{
					if(time_temp2 < time_temp1)
						day_count = 7;
					else
						day_count = 0;
				}
				
				time_on = (cur_time - time_temp1) + (day_count * SEC_PER_DAY) + time_temp2;
				time_interval = time_on - cur_time;	
			}

			if((sleep_time == 0) || (time_interval < sleep_time))
			{
				sleep_time = time_interval;
			}
		}
	}

	return sleep_time;
}

static bool app_book_time_valid(GxBook *book) //check new book time
{
    GxBookGet bookGet;
    int count = 0;
    int i;
    bool ret = true;
    time_t time_temp1;
    time_t time_temp2;
    time_t end_time1;
    time_t end_time2;
    BookProgStruct book_data1 = {0};
    BookProgStruct book_data2 = {0};
    if(book == NULL)
        return false;

    count = app_get_book(&bookGet);
    if(count == 0)
        return true;

    for(i = 0; i < count; i++)
    {
        if(bookGet.book[i].id == book->id) //same timer
        {
            continue;
        }

        time_temp1 = bookGet.book[i].trigger_time_start % SEC_PER_DAY;
        time_temp2 = book->trigger_time_start % SEC_PER_DAY;

        if(time_temp1 == time_temp2)
        {
            ///////check start time conflict/////////
            ret = false;

            if((bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE)
                    && (book->repeat_mode.mode == BOOK_REPEAT_ONCE))
            {
                if(bookGet.book[i].trigger_time_start != book->trigger_time_start)
                {
                    ret = true;
                }
            }
            else
            {
                if((bookGet.book[i].repeat_mode.mode != BOOK_REPEAT_ONCE) && (bookGet.book[i].repeat_mode.mode != BOOK_REPEAT_EVERY_DAY)
                        && (book->repeat_mode.mode != BOOK_REPEAT_ONCE) && (book->repeat_mode.mode != BOOK_REPEAT_EVERY_DAY))
                {
                    if(bookGet.book[i].repeat_mode.mode != book->repeat_mode.mode)
                    {
                        ret =true;
                    }
                }
                else if((bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE)
                        &&(book->repeat_mode.mode != BOOK_REPEAT_ONCE) && (book->repeat_mode.mode != BOOK_REPEAT_EVERY_DAY))
                {
                    WeekDay old_book_wday;
                    WeekDay new_book_wday;

                    old_book_wday = app_tm_get_wday(bookGet.book[i].trigger_time_start);
                    new_book_wday = app_book_mode2wday(book->repeat_mode.mode);
                    if(new_book_wday != old_book_wday)
                    {
                        ret = true;
                    }
                }
                else if((book->repeat_mode.mode == BOOK_REPEAT_ONCE)
                        &&(bookGet.book[i].repeat_mode.mode != BOOK_REPEAT_ONCE) && (bookGet.book[i].repeat_mode.mode != BOOK_REPEAT_EVERY_DAY))
                {
                    WeekDay old_book_wday;
                    WeekDay new_book_wday;

                    old_book_wday = app_book_mode2wday(bookGet.book[i].repeat_mode.mode);
                    new_book_wday = app_tm_get_wday(book->trigger_time_start);
                    if(new_book_wday != old_book_wday)
                    {
                        ret = true;
                    }
                }
            }

            if(ret == false) //time conflict
            {
                break;
            }
        }
        else
        {

            memcpy(&book_data1, (BookProgStruct*)book->struct_buf, sizeof(BookProgStruct));
            memcpy(&book_data2, (BookProgStruct*)bookGet.book[i].struct_buf, sizeof(BookProgStruct));

            end_time1= book->trigger_time_start+book_data1.duration;
            end_time2= bookGet.book[i].trigger_time_start+book_data2.duration;

            //printf("%d %d %d\n",bookGet.book[i].trigger_time_start,book_data1.duration,end_time2);
            //printf("%d %d %d\n",book->trigger_time_start,book_data2.duration,end_time1);

            if((book->book_type==BOOK_PROGRAM_PVR)&&(bookGet.book[i].book_type==BOOK_PROGRAM_PVR))
            {
                if((end_time1<=bookGet.book[i].trigger_time_start)|| (book->trigger_time_start>=end_time2))
                {
                    ret = true;
                }
                else
                {
                    ret = false;
                }
            }
        }
    }

    return ret;
}

static _BookExecWnd app_exec_prog_book_wnd(void)
{
	s_focus_wnd = NULL;

	if((GUI_CheckDialog("wnd_search") == GXCORE_SUCCESS)
		|| (GUI_CheckDialog("wnd_upgrade_process") == GXCORE_SUCCESS)
		|| (GUI_CheckDialog("wnd_ota_upgrade") == GXCORE_SUCCESS)) 
		return EXEC_WND_NONE;
	
	if((GUI_CheckDialog("win_media_centre") == GXCORE_SUCCESS)
		|| (GUI_CheckDialog("win_movie_view") == GXCORE_SUCCESS))
	{
		return EXEC_WND_MENU;
	}
	
//	popdlg_destroy();
	if(GUI_CheckDialog("wnd_pop_sort") == GXCORE_SUCCESS)
	{
		GUI_EndDialog("wnd_pop_sort");
		GUI_SetInterface("flush", NULL);
	}
	if(GUI_CheckDialog("wnd_passwd") == GXCORE_SUCCESS)
	{
		GUI_EndDialog("wnd_passwd");
		GUI_SetInterface("flush", NULL);
	}
	if(GUI_CheckDialog("wnd_keyboard_language") == GXCORE_SUCCESS)
	{
		GUI_EndDialog("wnd_keyboard_language");
		GUI_SetInterface("flush", NULL);
	}
	if(GUI_CheckDialog("wnd_find") == GXCORE_SUCCESS)
	{
		GUI_EndDialog("wnd_find");
		GUI_SetInterface("flush", NULL);
	}
#if NETWORK_SUPPORT && WIFI_SUPPORT
	if(GUI_CheckDialog("wnd_wifi") == GXCORE_SUCCESS)
	{
		app_wifi_tip_lost_focus_hide();
	}
#endif

	s_focus_wnd = GUI_GetFocusWindow();
	if(s_focus_wnd == NULL)
		return EXEC_WND_NONE;
		
	if((strcmp(s_focus_wnd, "wnd_full_screen") == 0)
		|| (strcmp(s_focus_wnd, "wnd_channel_list") == 0)
		|| (strcmp(s_focus_wnd, "wnd_channel_info") == 0)
//		|| ((strcmp(s_focus_wnd, "wnd_volume_value") == 0)&&(GUI_CheckDialog("win_main_menu") != GXCORE_SUCCESS))//排除多媒体中音量
		|| (strcmp(s_focus_wnd, "wnd_number") == 0)	
//		|| (strcmp(s_focus_wnd, "wnd_epg") == 0)
		|| (strcmp(s_focus_wnd, "wnd_channel_info_signal") == 0)
		#ifdef MUITI_SCREEN_SUPPORE
		|| (strcmp(s_focus_wnd, "wnd_multiscreen") == 0)
		#endif
		|| (strcmp(s_focus_wnd, "wnd_zoom") == 0)
		|| (strcmp(s_focus_wnd, "wnd_audio") == 0)
		|| (strcmp(s_focus_wnd, "wnd_pvr_bar") == 0)
		|| (strcmp(s_focus_wnd, "wnd_subtitling") == 0))

		//|| (strcmp(s_focus_wnd, "wnd_system_setting") == 0)
//		|| ((strcmp(s_focus_wnd, "wnd_channel_edit") == 0)&&(GUI_CheckDialog("wnd_main_menu") != GXCORE_SUCCESS))
	{
		return  EXEC_WND_FULLSCREEN;
	}
	if(strcmp(s_focus_wnd, "wnd_duration_set") == 0)
	{
		GUI_EndDialog("wnd_duration_set");
		GUI_EndDialog("wnd_pvr_bar");
		return  EXEC_WND_FULLSCREEN;
	}
	else if((GUI_CheckDialog("wnd_main_menu") == GXCORE_SUCCESS)
		|| (strcmp(s_focus_wnd, "wnd_epg") == GXCORE_SUCCESS)
		|| (GUI_CheckDialog("win_media_centre") == GXCORE_SUCCESS)
        || (GUI_CheckDialog("win_movie_view") == GXCORE_SUCCESS)
        || (strcmp(s_focus_wnd, "wnd_system_setting") == 0)
		|| (strcmp(s_focus_wnd, "wnd_volume_value") == 0)
		|| (GUI_CheckDialog("wnd_channel_edit") == GXCORE_SUCCESS))
	{
		return  EXEC_WND_MENU;
	}
	else if(strcmp(s_focus_wnd, "wnd_ota_upgrade") == 0)
	{
		if(app_ota_idle() == true)
		{
			return  EXEC_WND_MENU;
		}
		else
		{
			s_focus_wnd = NULL;
		}
	}
	else
	{
		s_focus_wnd = NULL;
	}

	return EXEC_WND_NONE;
}

static status_t app_prog_book_wnd_proc(void)
{
	status_t ret = GXCORE_ERROR;
	static GUI_Event event;	
	s_focus_wnd = GUI_GetFocusWindow();
	if(s_focus_wnd != NULL)
	{
		event.type = GUI_KEYDOWN;
		event.key.sym = VK_BOOK_TRIGGER;
		GUI_SendEvent(s_focus_wnd, &event);
		ret = GXCORE_SUCCESS;
	}
	else
	{
		if((GUI_CheckDialog("win_media_centre") == GXCORE_SUCCESS)
			|| (GUI_CheckDialog("win_movie_view") == GXCORE_SUCCESS))
		{
			GxMsgProperty_PlayerStop player_stop;

			//don't stop play , when second play av in media,can keep the end time 
			//just enddialog after wnd_full_screen 
			//player_stop.player = PMP_PLAYER_AV;
			//ret = app_send_msg_exec(GXMSG_PLAYER_STOP, &player_stop);

			player_stop.player = PMP_PLAYER_PIC;
			ret = app_send_msg_exec(GXMSG_PLAYER_STOP, &player_stop);

			GUI_SetInterface("clear_image", NULL);
			GUI_SetInterface("flush", NULL);
			GUI_EndDialog("after wnd_full_screen");
			GUI_SetInterface("video_enable", NULL);
		}
	}

	return ret;
}

static int _no_program_pop_timer(void* userdata)
{
	if(GUI_CheckDialog("wnd_pop_tip") != GXCORE_SUCCESS)
	{
		APP_TIMER_REMOVE(s_book_pop_timer);
		PopDlg pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.format = POP_FORMAT_DLG;
		pop.str = STR_ID_NO_PROGRAM;
		pop.title = STR_ID_BOOK;
		pop.mode = POP_MODE_UNBLOCK;
		pop.creat_cb = NULL;
		pop.exit_cb = NULL;
		pop.timeout_sec = 3;
		popdlg_create(&pop);
	}
	return 0;
}

static status_t app_exec_prog_book(GxBook *book)
{
	GxMsgProperty_NodeNumGet nodeNum = {0};
	GxMsgProperty_NodeByPosGet node = {0}; 
	GxBusPmViewInfo pmview_cur = {0};
	GxBusPmViewInfo pmview_temp = {0};
	int pos;
	int skipnums;
	bool isFind = false;
	skipnums = 0;
	//int prog_id;
	BookProgStruct book_data = {0};
	_BookExecWnd book_wnd = EXEC_WND_NONE;

	s_tv_radio_switch =false;
	s_focus_wnd = NULL;

	if((book->book_type != BOOK_PROGRAM_PLAY) && (book->book_type != BOOK_PROGRAM_PVR))
	{
		return GXCORE_ERROR;
	}

	book_wnd = app_exec_prog_book_wnd();
	if(book_wnd  == EXEC_WND_NONE)
	{
		return GXCORE_ERROR;
	}

	if(book_wnd == EXEC_WND_FULLSCREEN)
	{
		app_prog_book_wnd_proc();	
	}


	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&pmview_cur));
	memcpy(&pmview_temp, &pmview_cur, sizeof(GxBusPmViewInfo));
	//memcpy(&prog_id, book->struct_buf, sizeof(int));
	pmview_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_temp));
	memcpy(&book_data, (BookProgStruct*)book->struct_buf, sizeof(BookProgStruct));
	nodeNum.node_type = NODE_PROG;
	if((g_AppPvrOps.state == PVR_DUMMY) && GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&nodeNum)))
	{
		for(pos = 0; pos < nodeNum.node_num; pos++)
		{
			node.node_type = NODE_PROG;
			node.pos = pos;
			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
			{
				if(node.prog_data.id == book_data.prog_id)
				{
					isFind = true;
					break;
				}
			}
		}
	}

	if(isFind == false)
	{
		pmview_temp.group_mode = GROUP_MODE_ALL;
		pmview_temp.taxis_mode = TAXIS_MODE_NON;
		pmview_temp.pip_switch = 0;
		pmview_temp.pip_main_tp_id = 0;
//		pmview_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
		app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_temp));

		nodeNum.node_type = NODE_PROG;
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&nodeNum)))
		{
			for(pos = 0; pos < nodeNum.node_num; pos++)
			{
				node.node_type = NODE_PROG;
				node.pos = pos;
				if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
				{
					if(node.prog_data.id == book_data.prog_id)
					{
	//					if(node.prog_data.skip_flag != GXBUS_PM_PROG_BOOL_ENABLE)
	//					{
						isFind = true;
						break;
	//					}
					}
				}
			}
		}
	}

	if(isFind == false)
	{
		if(pmview_temp.stream_type == GXBUS_PM_PROG_TV)
		{
			pmview_temp.stream_type = GXBUS_PM_PROG_RADIO;
		}
		else
		{
			pmview_temp.stream_type = GXBUS_PM_PROG_TV;
		}
		app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_temp));
		nodeNum.node_type = NODE_PROG;
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&nodeNum)))
		{
			for(pos = 0; pos < nodeNum.node_num; pos++)
			{
				node.node_type = NODE_PROG;
				node.pos = pos;
				if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
				{
					if(node.prog_data.id == book_data.prog_id)
					{
//						if(node.prog_data.skip_flag != GXBUS_PM_PROG_BOOL_ENABLE)
//						{
							isFind = true;
							s_tv_radio_switch = true;
							break;
//						}
					}
				}
			}
		}
	}
	g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_cur));
	if(isFind == false)
	{

		APP_TIMER_ADD(s_book_pop_timer, _no_program_pop_timer, 500, TIMER_REPEAT);

		return GXCORE_ERROR;
	}
	else
	{	
		if(GUI_CheckDialog("win_music_view") == GXCORE_SUCCESS)
		{
			music_view_mode_pause();
			GxCore_ThreadDelay(200);
			GUI_SetInterface("flush", NULL);

		}

		if(book_popdlg_create(BOOKTYPE_PLAY) == WND_OK)
		{
			app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_temp));
			g_AppPlayOps.play_list_create(&pmview_temp);
			if(book->book_type == BOOK_PROGRAM_PVR)
			{
				usb_check_state usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
				if(usb_state == USB_ERROR)
				{
					PopDlg pop;

					app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_cur));

					memset(&pop, 0, sizeof(PopDlg));
					pop.type = POP_TYPE_NO_BTN;
					pop.format = POP_FORMAT_DLG;
					pop.str = STR_ID_INSERT_USB;
					pop.title = STR_ID_BOOK;
					pop.mode = POP_MODE_UNBLOCK;
					pop.creat_cb = NULL;
					pop.exit_cb = NULL;
					pop.timeout_sec= 3;
					popdlg_create(&pop);
					app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_cur));
					g_AppPlayOps.play_list_create(&pmview_cur);
					return GXCORE_ERROR;
				}
				if(usb_state == USB_NO_SPACE)
				{
					PopDlg pop;
					memset(&pop, 0, sizeof(PopDlg));
					pop.type = POP_TYPE_NO_BTN;
					pop.format = POP_FORMAT_DLG;
					pop.str = STR_ID_NO_SPACE;
					pop.title = STR_ID_BOOK;
					pop.mode = POP_MODE_UNBLOCK;
					pop.creat_cb = NULL;
					pop.exit_cb = NULL;
					pop.timeout_sec= 3;
					popdlg_create(&pop);
					app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_cur));
					g_AppPlayOps.play_list_create(&pmview_cur);
					return GXCORE_ERROR;
				}
			
			}
			if(book_wnd == EXEC_WND_MENU)
			{
				app_prog_book_wnd_proc();	
			}
			if(g_AppPvrOps.state != PVR_DUMMY)
			{
				app_pvr_stop();
			}
			if(GUI_CheckDialog("wnd_passwd") == GXCORE_SUCCESS)
			{
				GUI_EndDialog("wnd_passwd");
			}
			if(book->book_type == BOOK_PROGRAM_PVR)
			{
				if(node.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE)
				{
					time_t rec_time = book_data.duration;
					GUI_SetInterface("flush", NULL);
					g_AppPlayOps.set_rec_time(rec_time);
					g_AppPlayOps.program_play(PLAY_MODE_WITH_REC, pos);
				}
				else
				{
					PopDlg pop;
					GUI_SetInterface("flush", NULL);
					g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, pos);
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
			}
			else
			{
				GUI_SetInterface("flush", NULL);
				g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, pos);
			}

			if(s_tv_radio_switch == true)
			{
				if(g_AppFullArb.state.tv == STATE_OFF)
				{
					g_AppFullArb.state.tv = STATE_ON;
				}
				else
				{
					g_AppFullArb.state.tv = STATE_OFF;
				}
				g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
			}

			g_AppFullArb.state.pause = STATE_OFF;
			g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
		}
		else
		{
//			app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_cur));
//			g_AppPlayOps.play_list_create(&pmview_cur);
			if(strcmp(s_focus_wnd, "wnd_tetris") == 0)
			{
				 extern int app_tetris_get_focus(void);
				 app_tetris_get_focus();
			}
			else if(strcmp(s_focus_wnd, "wnd_snake") == 0)
			{
				extern int app_snake_get_focus(void);
				app_snake_get_focus();
			}
			else if(strcmp(s_focus_wnd, "wnd_wifi") == 0)
			{
#if NETWORK_SUPPORT && WIFI_SUPPORT
				app_wifi_tip_flush();
#endif
			}
			else if(strcmp(s_focus_wnd, "win_file_browser") == 0)
			{
				usb_check_state usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
				if(usb_state == USB_ERROR)
				{
					extern void file_brower_book_cancel();
					file_brower_book_cancel();
				}
				GUI_SetProperty(s_focus_wnd,"update",NULL);	
			}
			else
			{
				GUI_SetProperty(s_focus_wnd,"update",NULL);
			}
		}
	}

	return GXCORE_SUCCESS;
}

static void _power_off(void)
{
	time_t timerDur;
	time_t cur_time;
	
	if(s_exec_book.repeat_mode.mode == BOOK_REPEAT_ONCE)
	{
		cur_time = s_exec_book.trigger_time_start;
	}
	else
	{
		time_t time_temp1;
		time_t time_temp2;

		time_temp1 = g_AppTime.utc_get(&g_AppTime);
		time_temp1 = time_temp1 - time_temp1 % SEC_PER_DAY;
		time_temp2 = s_exec_book.trigger_time_start % SEC_PER_DAY;
		cur_time = time_temp1 + time_temp2;
	}
			
	cur_time = g_AppTime.utc_get(&g_AppTime);
	timerDur =  g_AppBook.get_sleep_time(cur_time);
	GxCore_ThreadDelay(10);
	app_low_power(timerDur, 0);
}
#if 0
static event_list* sp_PowerOffCountTimer = NULL;
static unsigned char count = 29;

static int _power_off_exec(PopDlgRet ret)
{
	if(ret == POP_VAL_OK)
	{
		_power_off();
	}
	
	remove_timer(sp_PowerOffCountTimer);
	sp_PowerOffCountTimer = NULL;
	count = 29;
#if NETWORK_SUPPORT && WIFI_SUPPORT
	if(GUI_CheckDialog("wnd_wifi") == GXCORE_SUCCESS)
	{
		app_wifi_tip_flush();
	}
#endif
	if(sp_PowerOffChTimer != NULL)
	{
		remove_timer(sp_PowerOffChTimer);
		sp_PowerOffChTimer = NULL;
	}
	return 0;						
}

static int timer_power_off_refresh_tip(void *userdata)
{
	char buffer[45] = {0};
	sprintf(buffer,"will shutdown after %d seconds .....", count--);
	GUI_SetProperty(TXT_BOOK_TIP, "string", buffer);
	return 0;
}

#endif 
extern char* tip_str;
static bool app_ched_fullstate_change(FullArb *arb)
{
	bool ret = false;
	static int channeledit_fullstate_bak = -1;

	if(channeledit_fullstate_bak != (int)arb->tip)
	{
		ret = true;
		channeledit_fullstate_bak = (int)arb->tip;
	}

	return ret; 
}

static int  timer_power_off_refresh_ch_ed(void *userdata)
{
#define CH_ED_SIGNAL        "txt_channel_edit_signal"
	
		static uint8_t play_delay = 0;
			
		if (g_AppFullArb.tip == FULL_STATE_DUMMY)
		{
			app_ched_fullstate_change(&g_AppFullArb);//update epg_fullstate_bak
			if((g_AppFullArb.state.tv == STATE_ON) && (app_check_av_running() == true))//
			{
				if(play_delay >= 0)
				{
					//GUI_SetProperty(CH_ED_SIGNAL, "state", "osd_trans_hide");
					GUI_SetProperty(CH_ED_SIGNAL, "state", "hide");					
					GUI_SetInterface("video_on", NULL);
					play_delay = 0;
				}
				else
				{
					play_delay++;
				}
			}			
		}
		else
		{
			if(app_ched_fullstate_change(&g_AppFullArb) == true)
			{
				//printf("\n################ line:%d	 [ channeledit_fullstate have changed ]\n",__LINE__);
				if(g_AppFullArb.state.tv == STATE_ON)
				{
				//printf("---[%s]%d video_off---\n", __func__, __LINE__);
					GUI_SetInterface("video_off", NULL);
				}
				//GUI_SetProperty(CH_ED_SIGNAL, "state", "show");
			}
		}
		return 0;
}


static event_list* sp_PowerOffChTimer = NULL;
static status_t app_exec_off_book(GxBook *book)
{
	status_t ret = GXCORE_ERROR;

	if((GUI_CheckDialog("wnd_search") != GXCORE_SUCCESS)
		&& (GUI_CheckDialog("wnd_upgrade_process") != GXCORE_SUCCESS)
		&& (GUI_CheckDialog("wnd_ota_upgrade") != GXCORE_SUCCESS)) 	
	{
//		popdlg_destroy();
		if(GUI_CheckDialog("win_music_view") == GXCORE_SUCCESS)
		{
			music_view_mode_pause();
			GxCore_ThreadDelay(200);
			GUI_SetInterface("flush", NULL);
		}
#if NETWORK_SUPPORT && WIFI_SUPPORT
		if(GUI_CheckDialog("wnd_wifi") == GXCORE_SUCCESS)
		{
			app_wifi_tip_lost_focus_hide();
		}
#endif
		if(GUI_CheckDialog("wnd_channel_info_signal") == GXCORE_SUCCESS)
		{
			GUI_EndDialog("wnd_channel_info_signal");
			GUI_SetInterface("flush", NULL);
		}
//		PopDlg poweroff_pop;
			
		g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);
#if 0
		if( reset_timer(sp_PowerOffCountTimer) != 0 )
		{
			sp_PowerOffCountTimer = create_timer(timer_power_off_refresh_tip, 1000, NULL, TIMER_REPEAT);
		}
#endif
		if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_channel_edit"))
		{	
			if( reset_timer(sp_PowerOffChTimer) != 0 )
			{
			sp_PowerOffChTimer = create_timer(timer_power_off_refresh_ch_ed, 1000, NULL, TIMER_REPEAT);
			}			
		}
#if 0
		memset(&poweroff_pop, 0, sizeof(PopDlg));
		poweroff_pop.type = POP_TYPE_YES_NO;
        //poweroff_pop.format = POP_FORMAT_DLG;
		poweroff_pop.mode = POP_MODE_UNBLOCK;
		poweroff_pop.str = STR_ID_POWER_OFF_TIP;
		poweroff_pop.title = STR_ID_POWER_OFF;
		poweroff_pop.timeout_sec = 30;
		poweroff_pop.default_ret = POP_VAL_OK;
		poweroff_pop.show_time = true;
		poweroff_pop.exit_cb = _power_off_exec;
		popdlg_create(&poweroff_pop);
#endif
		if(book_popdlg_create(BOOKTYPE_POWOFF) == WND_OK)
		{
			 _power_off();
		}
#if NETWORK_SUPPORT && WIFI_SUPPORT
		if(GUI_CheckDialog("wnd_wifi") == GXCORE_SUCCESS)
		{
			app_wifi_tip_flush();
		}
#endif
		if(sp_PowerOffChTimer != NULL)
		{
			remove_timer(sp_PowerOffChTimer);
			sp_PowerOffChTimer = NULL;
		}

		ret = GXCORE_SUCCESS;
	}

	return ret;
}

static int32_t app_get_book(GxBookGet *pbookGet)
{
	int32_t count = 0;

	if(pbookGet == NULL)
		return 0;

	memset(pbookGet,0,sizeof(GxBookGet));
	pbookGet->book_type = BOOK_POWER_ON|BOOK_POWER_OFF|BOOK_PROGRAM_PLAY|BOOK_PROGRAM_PVR;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_BOOK_GET, (void *)(&pbookGet)))
	{
		count = pbookGet->book_number;
	}
	else
	{
		APP_PRINT("get book ERROR\n");
	}
	return count;
}

static status_t app_creat_book(GxBook *book)
{
	status_t ret = GXCORE_ERROR;
	
	if(book == NULL)
	{
		return GXCORE_ERROR;
	}
	
	if(book->repeat_mode.mode == BOOK_REPEAT_ONCE)
	{
		time_t sec;
		sec = g_AppTime.utc_get(&g_AppTime);
		if(book->trigger_time_start < sec)
		{
			return GXCORE_ERROR;
		}
	}

	book->id = -1;
	if(app_book_time_valid(book) == true)
	{
		ret = app_send_msg_exec(GXMSG_BOOK_CREATE, (void *)(&book));
	}
	
	return ret;
}

static status_t app_modify_book(GxBook *book)
{
	status_t ret = GXCORE_ERROR;
	
	if(book == NULL)
	{
		return GXCORE_ERROR;
	}

	if(book->repeat_mode.mode == BOOK_REPEAT_ONCE)
	{
		time_t sec;
		sec = g_AppTime.utc_get(&g_AppTime);
		if(book->trigger_time_start < sec)
		{
			return GXCORE_ERROR;
		}
	}

	if(app_book_time_valid(book) == true)
	{
		ret = app_send_msg_exec(GXMSG_BOOK_MODIFY, (void *)(&book));
	}
	
	return ret;
}

static status_t app_remove_book(GxBook *book)
{
	status_t ret = GXCORE_ERROR;

	if(book == NULL)
	{
		return GXCORE_ERROR;
	}

	ret = app_send_msg_exec(GXMSG_BOOK_DESTROY, (void *)(&book));

	return ret;
	
}

static status_t app_remove_invalid_book(void)
{
	GxBookGet bookGet;
	int count = 0;
	int i;
	time_t sec;

	sec = g_AppTime.utc_get(&g_AppTime);
	
	count = app_get_book(&bookGet);

	for(i = 0; i < count; i++)
	{
		if(((bookGet.book[i].book_type&BOOK_PROGRAM_PLAY) == BOOK_PROGRAM_PLAY)
			|| ((bookGet.book[i].book_type&BOOK_PROGRAM_PVR) == BOOK_PROGRAM_PVR))
		{
			BookProgStruct book_data = {0};
			GxMsgProperty_NodeByIdGet node = {0};

			memcpy(&book_data, (BookProgStruct*)(bookGet.book[i].struct_buf), sizeof(BookProgStruct));

			node.node_type = NODE_PROG;
			node.id = book_data.prog_id;
			if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node)) == GXCORE_ERROR)
			{
				app_remove_book(&(bookGet.book[i]));
				continue;
			}
		}
		
		if(bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE)
		{
			if((sec > bookGet.book[i].trigger_time_start)
				|| (bookGet.book[i].trigger_mode == BOOK_TRIG_OFF))
			{
				app_remove_book(&(bookGet.book[i]));
			}
		}
	}

	return GXCORE_SUCCESS;
} 

static status_t app_exec_book(GxBook *book)
{
	status_t ret = GXCORE_ERROR; 
	
	if((book == NULL)
		|| (s_book_enable == false))
	{
		return GXCORE_ERROR;
	}
	
	memcpy(&s_exec_book, book, sizeof(GxBook));
	
	printf("[app book]GXBOOK_TRIGGER id = %d\n",book->id);

	if(book->repeat_mode.mode ==  BOOK_REPEAT_ONCE)
	{
		const char *wnd = NULL;

		wnd = GUI_GetFocusWindow();
//		if(strcmp(wnd, "wnd_system_setting") != 0)
//		{
			printf(" [%s] [%d] \n",__FUNCTION__,__LINE__);
			app_remove_book(book);
//		}
		if(NULL != wnd)
		{
			if(strcmp(wnd, "wnd_timer_setting") == 0)
			{
				GUI_SetProperty("list_timer_setting_timer", "update_all", NULL);
			}
		}
	}

	switch(s_exec_book.book_type)
	{
		case BOOK_PROGRAM_PLAY:
		case BOOK_PROGRAM_PVR:
			ret = app_exec_prog_book(&s_exec_book);
			break;

		case BOOK_POWER_OFF:
			ret = app_exec_off_book(&s_exec_book);
			break;

		default:
			break;
	}
	
	return ret;
}

static void app_enable_book(void)
{
    s_book_enable = true;
    
    if(NULL != s_book_ignore_timer)
    {
        remove_timer(s_book_ignore_timer);
        s_book_ignore_timer = NULL;
    }
}

static int _timer_enable_book(void* usrdata)
{
	app_enable_book();
	return 0;
}

static void app_disable_book(time_t sec) //0, forever; 1,ignor sec
{
	time_t ignore_ms = sec * 1000;
	
	s_book_enable = false;
	if((sec != 0) && (0 != reset_timer(s_book_ignore_timer)) )
	{
		s_book_ignore_timer = create_timer(_timer_enable_book, ignore_ms, NULL, TIMER_ONCE);
	}

}

AppBook g_AppBook = 
{
    .get          = app_get_book,
    .creat           = app_creat_book,
    .modify           = app_modify_book,
    .remove       = app_remove_book,
    .invalid_remove       = app_remove_invalid_book,
    .exec        = app_exec_book,
    .get_sleep_time = app_get_sleep_time,
    .mode2wday = app_book_mode2wday,
    .wday2mode = app_book_wday2mode,
    .enable = app_enable_book,
    .disable = app_disable_book,
};

