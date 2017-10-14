//#include "app_root.h"
#include "app.h"
#include "app_module.h"
#include "app_utility.h"
#include "app_send_msg.h"
#include "app_book.h"
#include "app_pop.h"
#include "full_screen.h"

#define LIST_VIEW_TM   "list_timer_setting_timer"
#define TXT_TIMER_YMD  "txt_timer_setting_date"
#define TXT_TIMER_DATE "txt_timer_setting_time"

#define EVENT_WAKEUP   "Power On"
#define EVENT_SLEEP    "Power Off"
#define EVENT_PLAY     "Play"
#define EVENT_PVR      "PVR"
#define BLANK_PROG     "----"
#define BLANK_DURA     "--:--"

#define STR_MODE_ONCE  "Once"
#define STR_MODE_DAILY "Daily"
#define STR_MODE_MON   "Mon."
#define STR_MODE_TUES  "Tues."
#define STR_MODE_WED   "Wed."
#define STR_MODE_THURS "Thurs."
#define STR_MODE_FRI   "Fri."
#define STR_MODE_SAT   "Sat."
#define STR_MODE_SUN   "Sun."
//#define MODE_WEEKLY  "Weekly"
//#define MODE_OFF     "Off"
#define LOCAL_DATE_LEN 40

#define SEC_PER_DAY (60*60*24)

static GxBookGet bookGet;
static uint32_t book_id[APP_BOOK_NUM] = {0};
static bool s_timer_on_fullscreen = false;
static bool s_add_timer_flag = false;
static event_list* s_tm_update_timer = NULL;


extern bool app_add_timer_menu_exec(void);
extern bool app_edit_timer_menu_exec(GxBook *book);

void app_add_timer_flag_set(bool bflag)
{
	s_add_timer_flag = bflag;
}

static int app_timer_setting_tm_update_timer(void* usrdata)
{
	char sys_time_str[24] = {0};
	char time_str_temp[24] = {0};

	g_AppTime.time_get(&g_AppTime, sys_time_str, sizeof(sys_time_str));
	// time
	if((strlen(sys_time_str) > 24) || (strlen(sys_time_str) < 5))
	{
		printf("\nlenth error\n");
		return 1;
	}
	memcpy(time_str_temp,&sys_time_str[strlen(sys_time_str)-5],5);// 00:00
	GUI_SetProperty(TXT_TIMER_DATE, "string", time_str_temp);

	// ymd
	memcpy(time_str_temp,sys_time_str,strlen(sys_time_str));// 0000/00/00
	time_str_temp[strlen(sys_time_str)-5] = '\0';
	GUI_SetProperty(TXT_TIMER_YMD, "string", time_str_temp);

	return 0;
}

static void app_timer_setting_timer_edit_hide_time(void)
{
	if(s_tm_update_timer != NULL)
		timer_stop(s_tm_update_timer);
}

static void app_timer_setting_timer_edit_show_time(void)
{
	app_timer_setting_tm_update_timer(NULL);
	if(0 != reset_timer(s_tm_update_timer))
	{
		s_tm_update_timer = create_timer(app_timer_setting_tm_update_timer, 2000, NULL, TIMER_REPEAT);
	}
}

static char* app_time_data_to_string(char *time_str,time_t time_sec)
{
	struct tm tm_start;
	memset(time_str,0,sizeof(char)*LOCAL_DATE_LEN);
	memcpy(&tm_start, localtime(&time_sec), sizeof(struct tm));
	sprintf(time_str,"%d.%02d.%02d %02d:%02d", tm_start.tm_year+1900,tm_start.tm_mon+1,tm_start.tm_mday,tm_start.tm_hour,tm_start.tm_min);
	return time_str;
}

static char* app_time_mode_to_string(uint32_t repeat_mode)
{
	char *str = NULL;
	switch(repeat_mode)
	{
		case BOOK_REPEAT_ONCE:
			str = STR_ID_ONCE;
			break;

        case BOOK_REPEAT_EVERY_DAY:
            str = STR_ID_DAILY;
            break;

		case BOOK_REPEAT_MON:
			str =  STR_MODE_MON;
			break;

		case BOOK_REPEAT_TUES:
			str =  STR_MODE_TUES;
			break;

		case BOOK_REPEAT_WED:
			str =  STR_MODE_WED;
			break;

		case BOOK_REPEAT_THURS:
			str =  STR_MODE_THURS;
			break;

		case BOOK_REPEAT_FRI:
			str =  STR_MODE_FRI;
			break;

		case BOOK_REPEAT_SAT:
			str =  STR_MODE_SAT;
			break;

		case BOOK_REPEAT_SUN:
			str =  STR_MODE_SUN;
			break;
			
		default:
			break;
	}

	return str;
}

static void _sort_book_id(time_t * a,uint32_t * b,uint32_t num)
{
    uint32_t i,j,tmp = 0;
    for(i=0;i<num;i++)
    {
        for(j=0;j<num-i-1;j++)
        {
            if(a[j]>a[j+1])
            {
                tmp = a[j];
                a[j] = a[j+1];
                a[j+1] = tmp;
                
                tmp = b[j];
                b[j] = b[j+1];
                b[j+1] = tmp;
            }
        }
    }
}


void app_create_timer_setting_fullsreeen(void)
{
	PopDlg pop = {0};

	if(GUI_CheckDialog("wnd_channel_info")== GXCORE_SUCCESS)
	{
		GUI_EndDialog("wnd_channel_info");
		GUI_SetInterface("flush", NULL);
	}
	
	if (g_AppPvrOps.state != PVR_DUMMY)
	{
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
	        g_AppFullArb.state.pause = STATE_OFF;
	        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
	        g_AppPlayOps.program_stop();
	        GUI_CreateDialog("wnd_timer_setting");
	        s_timer_on_fullscreen = true;
	}
}

static void app_timer_setting_quit(void)
{
	GUI_EndDialog("wnd_timer_setting");
	GUI_SetProperty("wnd_timer_setting","draw_now",NULL);

	if(s_timer_on_fullscreen == true)
	{
	    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
	    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
	    if (g_AppPlayOps.normal_play.play_total != 0)
	    {
	        g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
	    }
	}
	s_timer_on_fullscreen = false;
}

int app_book_get_total_num(void)
{
	int count = 0;
	count = g_AppBook.get(&bookGet);
	return count;
}

SIGNAL_HANDLER int app_timer_list_get_total(GuiWidget *widget, void *usrdata)
{
	return app_book_get_total_num();
}

SIGNAL_HANDLER int app_timer_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara *item = NULL;
	static char tmStart[LOCAL_DATE_LEN] = {0};
	time_t display_time;
	uint32_t mode;
	WeekDay gmt_wday;
	WeekDay local_wday;
	BookProgStruct book_prog = {0};
    uint32_t i = 0;
    time_t time_for_sort[APP_BOOK_NUM] = {0};
    uint32_t book_sel_map = 0;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
	{
		return EVENT_TRANSFER_KEEPON;
	}

    for(i = 0; i < app_book_get_total_num(); i++)
    {
        time_for_sort[i] = get_display_time_by_timezone(bookGet.book[i].trigger_time_start);
        book_id[i] = i;
    }
    _sort_book_id(time_for_sort,book_id,app_book_get_total_num());
    book_sel_map = book_id[item->sel];
	display_time = get_display_time_by_timezone(bookGet.book[book_sel_map].trigger_time_start);

	if(bookGet.book[book_sel_map].repeat_mode.mode == BOOK_REPEAT_ONCE)
	{
		app_time_data_to_string(tmStart, display_time);
	}
	else
	{
		char *str = NULL;
		str = app_time_to_hourmin_edit_str(display_time);
		if(str != NULL)
		{
			strcpy(tmStart, str);
			GxCore_Free(str);
			str = NULL;
		}
	}
	item->string = tmStart;
	item = item->next;
	if(NULL == item)
		return EVENT_TRANSFER_KEEPON;
	char *event_name = NULL;
	if(BOOK_POWER_ON == bookGet.book[book_sel_map].book_type)
	{
		event_name = STR_ID_POWER_ON;
	}
	else if(BOOK_POWER_OFF == bookGet.book[book_sel_map].book_type)
	{
		event_name = STR_ID_POWER_OFF;
	}
	else if(BOOK_PROGRAM_PLAY == bookGet.book[book_sel_map].book_type)
	{
		event_name = EVENT_PLAY;
	}
	else if(BOOK_PROGRAM_PVR== bookGet.book[book_sel_map].book_type)
	{
		event_name = EVENT_PVR;
	}
	else{}
	item->string = event_name;
	item = item->next;
	if(NULL == item)
		return EVENT_TRANSFER_KEEPON;

	char *prog_name = BLANK_PROG;;
	if((BOOK_PROGRAM_PLAY == bookGet.book[book_sel_map].book_type)
		|| (BOOK_PROGRAM_PVR == bookGet.book[book_sel_map].book_type))
	{
		static GxMsgProperty_NodeByIdGet node;
		//int prog_id;
		//memcpy(&prog_id, bookGet.book[book_sel_map].struct_buf, sizeof(int));
		memcpy(&book_prog, (BookProgStruct*)bookGet.book[book_sel_map].struct_buf, sizeof(BookProgStruct));
		node.node_type = NODE_PROG;
		node.id = book_prog.prog_id;
		if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node)) == GXCORE_SUCCESS)
		{
			prog_name =  (char *)node.prog_data.prog_name;
		}
	}

	item->string = prog_name;

	item = item->next;
	if(NULL == item)
		return EVENT_TRANSFER_KEEPON;

	char *dura_str = NULL;
	if(BOOK_PROGRAM_PVR == bookGet.book[book_sel_map].book_type)
	{
		static char time_value[10] = {0};
		//int duration = bookGet.book[book_sel_map].trigger_time_end - bookGet.book[book_sel_map].trigger_time_start;
		int hour = book_prog.duration / 3600;
       	int min = (book_prog.duration % 3600) / 60;
       	memset(time_value, 0, sizeof(time_value));
       	sprintf(time_value, "%02d:%02d", hour, min);
       	dura_str = time_value;
	}
	else
	{
		dura_str = BLANK_DURA;
	}
	item->string = dura_str;

	item = item->next;
	if(NULL == item)
		return EVENT_TRANSFER_KEEPON;
		
	if((bookGet.book[book_sel_map].repeat_mode.mode == BOOK_REPEAT_ONCE)
		|| (bookGet.book[book_sel_map].repeat_mode.mode == BOOK_REPEAT_EVERY_DAY))
	{
		mode = bookGet.book[book_sel_map].repeat_mode.mode;
	}
	else
	{
		gmt_wday = g_AppBook.mode2wday(bookGet.book[book_sel_map].repeat_mode.mode);
		local_wday = app_weekday_gmt2local(gmt_wday, bookGet.book[book_sel_map].trigger_time_start % SEC_PER_DAY);
		mode = g_AppBook.wday2mode(local_wday);
	}
	
	item->string = app_time_mode_to_string(mode);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_timer_setting_create(GuiWidget *widget, void *usrdata)
{
	app_lang_set_menu_font_type("text_timer_setting_title");
	g_AppBook.invalid_remove();
	GUI_SetProperty(LIST_VIEW_TM, "update_all", NULL);

    app_timer_setting_timer_edit_show_time();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_timer_setting_destroy(GuiWidget *widget, void *usrdata)
{
    if(s_tm_update_timer != NULL)
    {
        remove_timer(s_tm_update_timer);
        s_tm_update_timer = NULL;
    }
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_timer_setting_got_focus(GuiWidget *widget, void *usrdata)
{
    app_timer_setting_timer_edit_show_time();

	g_AppBook.invalid_remove();
	if(s_add_timer_flag == true)
	{
		app_add_timer_flag_set(false);
		int count;
		count = app_book_get_total_num()-1;
		GUI_SetProperty(LIST_VIEW_TM, "select",&count);
	}
	GUI_SetProperty(LIST_VIEW_TM, "update_all", NULL);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_timer_setting_lost_focus(GuiWidget *widget, void *usrdata)
{
    app_timer_setting_timer_edit_hide_time();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_timer_setting_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
#if DLNA_SUPPORT
                case VK_DLNA_TRIGGER:
                    app_dlna_ui_exec(NULL, NULL);
                    break;
#endif
				case STBK_YELLOW:
				{
					int sel;
					
					GUI_GetProperty(LIST_VIEW_TM, "select", &sel);
                    			sel = book_id[sel];
					if(sel >=  bookGet.book_number)
					{
						APP_PRINT("the sel item id ERROR when modified the timer sel = %d , book_number =  %d\n",sel,bookGet.book_number);
						return EVENT_TRANSFER_STOP;
					}
					g_AppBook.get(&bookGet);
					if((BOOK_POWER_ON == bookGet.book[sel].book_type)
						||(BOOK_POWER_OFF == bookGet.book[sel].book_type)
						||BOOK_PROGRAM_PLAY == bookGet.book[sel].book_type
						||BOOK_PROGRAM_PVR == bookGet.book[sel].book_type)
					{
						app_edit_timer_menu_exec(&bookGet.book[sel]);
					}
					break;
				}

				case STBK_GREEN:
				{
					if(bookGet.book_number < APP_BOOK_NUM)
					{
						app_add_timer_menu_exec();
					}
					else
					{
						PopDlg  pop;
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_OK;
						pop.str = STR_ID_TIMER_FULL;
						pop.mode = POP_MODE_UNBLOCK;
						pop.format = POP_FORMAT_DLG;
						popdlg_create(&pop) ;
					}
					break;
				}

				case STBK_RED:
				{
					int sel;
					int tmp;
					GUI_GetProperty(LIST_VIEW_TM, "select", &sel);
					tmp = book_id[sel];
					if(tmp < bookGet.book_number)
					{
						PopDlg  pop;
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_YES_NO;
						/*STR_ID_SURE_DELETE = are you sure to delete it?*/
						pop.str = STR_ID_SURE_DELETE;
						pop.format = POP_FORMAT_DLG;

						if( POP_VAL_OK == popdlg_create(&pop) )
						{
							if(g_AppBook.remove(&bookGet.book[tmp]) == GXCORE_SUCCESS)
							{
								GUI_SetProperty(LIST_VIEW_TM, "update_all", NULL);
								if( bookGet.book_number > 0)
								{
									if(sel >= bookGet.book_number )
									{
										sel = bookGet.book_number  - 1;
									}
									GUI_SetProperty(LIST_VIEW_TM, "select", &sel);
								}	
							}
						}

					}
					break;
				}	
				case STBK_BLUE:
				{
					if (bookGet.book_number > 0)
					{
						PopDlg  pop;
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_YES_NO;
						/*STR_ID_SURE_DELETE_ALL = are you sure to delete all?*/
						pop.str = STR_ID_SURE_DELETE_ALL;
						pop.format = POP_FORMAT_DLG;
						if( POP_VAL_OK == popdlg_create(&pop) )
						{
#if 1 //del all timer
							int sel;
							int tmp;
							for(sel = 0; sel < bookGet.book_number; sel++)
							{
								tmp = book_id[sel];
								if(g_AppBook.remove(&bookGet.book[tmp]) == GXCORE_SUCCESS)
								{
									GUI_SetProperty(LIST_VIEW_TM, "select", &sel);  
								}
							}
							GUI_SetProperty(LIST_VIEW_TM, "update_all", NULL);  
#endif

						}
					}
					break;
				}

				case STBK_EXIT:
				case STBK_MENU:
					{
						//GUI_EndDialog("wnd_timer_setting");
						app_timer_setting_quit();
					}
					break;
					
				case VK_BOOK_TRIGGER:
					GUI_EndDialog("after wnd_full_screen");
					break;
					
				default:
					break;
			}
		default:
			break;
	}

	return ret;
}


