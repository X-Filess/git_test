#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_utility.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "gxbook.h"
#include "app_pop.h"
#include "app_book.h"

#define SEC_PER_DAY (60*60*24)
#define STR_NONE "None"
#define STR_TV "TV"
#define STR_RADIO "Radio"

enum TIMER_ITEM_NAME 
{
	ITEM_TIMER_MODE = 0,
	ITEM_TIMER_TYPE,
	ITEM_TIMER_DATE,
	ITEM_TIMER_TIME,
	ITEM_TIMER_DURATION,
	ITEM_TIMER_CHANNEL,
	ITEM_TIMER_TOTAL
};

typedef enum
{
	TIMER_MODE_POWER_ON,
	TIMER_MODE_POWER_OFF,
	TIMER_MODE_PLAY,
	TIMER_MODE_PVR
}TimerModeSel;

typedef enum
{
	TIMER_TYPE_ONCE,
	TIMER_TYPE_DAILY,
	TIMER_TYPE_MON,
	TIMER_TYPE_TUES,
	TIMER_TYPE_WED,
	TIMER_TYPE_THURS,
	TIMER_TYPE_FRI,
	TIMER_TYPE_SAT,
	TIMER_TYPE_SUN
}TimerTypeSel;

typedef struct
{
	int prog_id;
	uint8_t prog_name[MAX_PROG_NAME];
}ProgInfo;

typedef struct
{
	int book_id;
	TimerModeSel mode;
	TimerTypeSel type;
	time_t time;
	time_t duration;
	ProgInfo prog_info;
}SysTimerPara;

static enum
{
	TIMER_STATE_ADD = 0,
	TIMER_STATE_MODIFY,
	TIMER_STATE_EPG
}s_set_timer_state = TIMER_STATE_ADD;

static SystemSettingOpt s_timer_edit_opt;
static SystemSettingItem s_timer_edit_item[ITEM_TIMER_TOTAL];
static SysTimerPara s_timer_para;
static ProgInfo s_prog_info_trace = {0};
static char *s_timer_date_str = NULL;
static char *s_timer_time_str = NULL;
static char *s_timer_dura_str = NULL;
static WndStatus s_channel_list_state = WND_EXEC;
static bool s_timer_date_valid = false;
GxBusPmViewInfo s_pmview_cur = {0};
static  PopList pop_list;

static int app_timer_get_channel_dlg(int cur_channel_id, ProgInfo *prog_get);
extern int app_book_get_total_num(void);

static void app_timer_edit_para_new(void)
{
	GxMsgProperty_NodeNumGet nodeNum = {0};
	GxMsgProperty_NodeByPosGet nodeInfo = {0};
	time_t temp_sec;

	s_timer_para.mode = TIMER_MODE_POWER_ON;
	s_timer_para.type = TIMER_TYPE_ONCE;
	s_timer_para.time = g_AppTime.utc_get(&g_AppTime);
	temp_sec = get_display_time_by_timezone(s_timer_para.time);
	if(temp_sec >= 0)
	{
		s_timer_para.time = temp_sec;
	}
	s_timer_para.time = s_timer_para.time - s_timer_para.time % 60;
	s_timer_para.time += 60;
	s_timer_para.duration = 60;

	s_timer_para.prog_info.prog_id= -1;
	memset(s_timer_para.prog_info.prog_name, 0, MAX_PROG_NAME);

	nodeInfo.node_type = NODE_PROG;
	nodeInfo.pos = g_AppPlayOps.normal_play.play_count;
	
#ifndef RADIO_REC_BOOK
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		memcpy(&pmview_temp, &s_pmview_cur, sizeof(GxBusPmViewInfo));
		pmview_temp.group_mode = GROUP_MODE_ALL;
		pmview_temp.taxis_mode = TAXIS_MODE_NON;
		pmview_temp.stream_type = GXBUS_PM_PROG_TV;
		app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_temp));
		nodeInfo.pos = 0;//g_AppPlayOps.normal_play.view_info.tv_prog_cur;
	}
#endif

	nodeNum.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &nodeNum);
	if(nodeNum.node_num > 0)
	{
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&nodeInfo)))
		{
			s_timer_para.prog_info.prog_id = nodeInfo.prog_data.id;
			memcpy(s_timer_para.prog_info.prog_name, nodeInfo.prog_data.prog_name, MAX_PROG_NAME);
		}
		else
		{
			strcpy((char *)s_timer_para.prog_info.prog_name, STR_NONE);
		}
	}
	else
	{
		strcpy((char *)s_timer_para.prog_info.prog_name, STR_NONE);
	}
}

static void app_timer_edit_para_get(GxBook *book)
{
	GxMsgProperty_NodeNumGet nodeNum = {0};
	GxMsgProperty_NodeByPosGet nodeInfo = {0};
	WeekDay local_weekday;
	WeekDay gmt_weekday;

	s_timer_para.book_id = book->id;

	if(book->book_type == BOOK_PROGRAM_PLAY)
	{
		s_timer_para.mode= TIMER_MODE_PLAY;
	}
	else if(book->book_type == BOOK_POWER_ON)
	{
		s_timer_para.mode = TIMER_MODE_POWER_ON;
	}
	else if(book->book_type == BOOK_POWER_OFF)
	{
		s_timer_para.mode = TIMER_MODE_POWER_OFF;
	}
	else if(book->book_type == BOOK_PROGRAM_PVR)
	{
		s_timer_para.mode = TIMER_MODE_PVR;
	}
	
	if(book->trigger_mode == BOOK_TRIG_OFF)
	{
		s_timer_para.type= TIMER_TYPE_ONCE;
	}
	else
	{
		if(book->repeat_mode.mode == BOOK_REPEAT_ONCE)
		{
			s_timer_para.type = TIMER_TYPE_ONCE;
		}
		else if(book->repeat_mode.mode == BOOK_REPEAT_EVERY_DAY)
		{
			s_timer_para.type = TIMER_TYPE_DAILY;
		}
		else
		{
			gmt_weekday = g_AppBook.mode2wday(book->repeat_mode.mode);
			local_weekday = app_weekday_gmt2local(gmt_weekday, book->trigger_time_start % SEC_PER_DAY);

			if(local_weekday == WEEK_MON)
			{
				s_timer_para.type = TIMER_TYPE_MON;
			}
			else if(local_weekday == WEEK_TUES)
			{
				s_timer_para.type = TIMER_TYPE_TUES;
			}
			else if(local_weekday == WEEK_WED)
			{
				s_timer_para.type = TIMER_TYPE_WED;
			}
			else if(local_weekday == WEEK_THURS)
			{
				s_timer_para.type = TIMER_TYPE_THURS;
			}
			else if(local_weekday == WEEK_FRI)
			{
				s_timer_para.type = TIMER_TYPE_FRI;
			}
			else if(local_weekday == WEEK_SAT)
			{
				s_timer_para.type = TIMER_TYPE_SAT;
			}
			else if(local_weekday == WEEK_SUN)
			{
				s_timer_para.type = TIMER_TYPE_SUN;
			}
			else
			{
				s_timer_para.type = TIMER_TYPE_ONCE;
			}
		}
	}

	if(s_timer_para.type == TIMER_TYPE_ONCE)
	{
		s_timer_para.time = get_display_time_by_timezone(book->trigger_time_start);
		//s_timer_para.duration = book->trigger_time_end - book->trigger_time_start;
	}
	else
	{
		time_t time_temp1;
		time_t time_temp2;
		
		time_temp1 = g_AppTime.utc_get(&g_AppTime);
		time_temp1 = get_display_time_by_timezone(time_temp1);
		time_temp1 = time_temp1 - time_temp1 % SEC_PER_DAY;
		time_temp2 = get_display_time_by_timezone(book->trigger_time_start);
		time_temp2 %= SEC_PER_DAY;
		s_timer_para.time = time_temp1 + time_temp2;

		//time_temp1 = book->trigger_time_start %SEC_PER_DAY;
		//time_temp2 = book->trigger_time_end % SEC_PER_DAY;

		//if(time_temp2 < time_temp1)
			//time_temp2 += SEC_PER_DAY;

		//s_timer_para.duration = time_temp2 - time_temp1;
	}
	
	s_timer_para.duration = 60;
	s_timer_para.prog_info.prog_id = -1;
	memset(s_timer_para.prog_info.prog_name, 0, MAX_PROG_NAME);
	
	if((book->book_type == BOOK_PROGRAM_PLAY) || (book->book_type == BOOK_PROGRAM_PVR))
	{
		GxMsgProperty_NodeByIdGet node;
		BookProgStruct book_prog;
		
		//int prog_id = *(int*)book->struct_buf;
		memcpy(&book_prog, (BookProgStruct*)book->struct_buf, sizeof(BookProgStruct));
		s_timer_para.prog_info.prog_id = book_prog.prog_id;
		node.node_type = NODE_PROG;
		node.id = s_timer_para.prog_info.prog_id;
		if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node)) == GXCORE_SUCCESS)
		{
			memcpy(s_timer_para.prog_info.prog_name, node.prog_data.prog_name, MAX_PROG_NAME);
		}
		else
		{
			strcpy((char *)s_timer_para.prog_info.prog_name, STR_NONE);
		}

		if(book->book_type == BOOK_PROGRAM_PVR)
		{
			s_timer_para.duration = book_prog.duration;
		}
	}
	else
	{
		nodeNum.node_type = NODE_PROG;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &nodeNum);
		if(nodeNum.node_num > 0)
		{
			nodeInfo.node_type = NODE_PROG;
			nodeInfo.pos = g_AppPlayOps.normal_play.play_count;
			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&nodeInfo)))
			{
				s_timer_para.prog_info.prog_id= nodeInfo.prog_data.id;
				memcpy(s_timer_para.prog_info.prog_name, nodeInfo.prog_data.prog_name, MAX_PROG_NAME);
			}
			else
			{
				strcpy((char *)s_timer_para.prog_info.prog_name, STR_NONE);
			}
		}
		else
		{
			strcpy((char *)s_timer_para.prog_info.prog_name, STR_NONE);
		}
	}
}

static void app_timer_epg_para_get(GxBusPmDataProg *prog_data, GxEpgInfo *event_info)
{
	s_timer_para.mode = TIMER_MODE_PLAY;
	s_timer_para.type = TIMER_TYPE_ONCE;
	
	s_timer_para.time = get_display_time_by_timezone(event_info->start_time);
	s_timer_para.duration = event_info->duration;

	s_timer_para.prog_info.prog_id = prog_data->id;
	memcpy(s_timer_para.prog_info.prog_name, prog_data->prog_name, MAX_PROG_NAME);
}

static void app_timer_edit_result_para_get(SysTimerPara *ret_para)
{
	time_t date;
	time_t time;

	if(app_edit_str_to_date_sec(s_timer_edit_item[ITEM_TIMER_DATE].itemProperty.itemPropertyEdit.string, &date) == GXCORE_SUCCESS)
	{
		time = app_edit_str_to_hourmin_sec(s_timer_edit_item[ITEM_TIMER_TIME].itemProperty.itemPropertyEdit.string);
		ret_para->time= date + time;

    		ret_para->mode = s_timer_edit_item[ITEM_TIMER_MODE].itemProperty.itemPropertyCmb.sel;
    		if(s_set_timer_state == TIMER_STATE_EPG)
    		{
    			ret_para->mode += TIMER_MODE_PLAY;
    		}
    		ret_para->type= s_timer_edit_item[ITEM_TIMER_TYPE].itemProperty.itemPropertyCmb.sel;
    		ret_para->duration= app_edit_str_to_hourmin_sec(s_timer_edit_item[ITEM_TIMER_DURATION].itemProperty.itemPropertyEdit.string);
    		memcpy(&ret_para ->prog_info, &s_prog_info_trace, sizeof(ProgInfo));

		s_timer_date_valid = true;
		if(ret_para->type == TIMER_TYPE_ONCE)
		{
    			time = get_local_time_by_timezone(ret_para->time);
    			s_timer_date_valid = app_time_range_valid(time);
    		}
    	}
    	else
    	{
    		s_timer_date_valid = false;
    	}

    	if(s_timer_date_valid == false)
    	{
    		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
        	pop.type = POP_TYPE_OK;
       	pop.str = STR_ID_ERR_DATE;
       	pop.mode = POP_MODE_BLOCK;
       	popdlg_create(&pop);
    	}
}

static bool app_timer_edit_para_change(SysTimerPara *ret_para)
{
	bool ret = FALSE;

	if(s_timer_date_valid == false)
		return ret;
	
	if((s_set_timer_state == TIMER_STATE_ADD)
		|| (s_set_timer_state == TIMER_STATE_EPG))
	{
		ret = TRUE;
	}
	else
	{
		if(s_timer_para.mode != ret_para->mode)
		{
			ret = TRUE;
		}

		if(s_timer_para.type != ret_para->type)
		{
			ret = TRUE;
		}

		if(s_timer_para.type == TIMER_TYPE_ONCE)
		{
			if(s_timer_para.time != ret_para->time)
			{
				ret = TRUE;
			}
		}
		else
		{
			if(s_timer_para.time % SEC_PER_DAY != ret_para->time % SEC_PER_DAY)
			{
				ret = TRUE;
			}
		}

		if(ret_para->mode == TIMER_MODE_PVR)
		{
			if(s_timer_para.duration != ret_para->duration)
				ret = TRUE;
		}

		if((ret_para->mode == TIMER_MODE_PLAY) || (ret_para->mode == TIMER_MODE_PVR))
		{
			if(s_timer_para.prog_info.prog_id != ret_para->prog_info.prog_id)
				ret = TRUE;
		}
	}
	return ret;
}

static status_t app_timer_edit_para_save(SysTimerPara *ret_para)
{
	GxBook book;
	GxBookType bookType;
	WeekDay gmt_weekday;
	WeekDay local_weekday;
	int startT;
	status_t ret = GXCORE_SUCCESS;
/////////////book_type, trigger_mode/////////////////
	if(ret_para->mode == TIMER_MODE_POWER_OFF)
	{
		book.book_type = BOOK_POWER_OFF;
		book.trigger_mode = BOOK_TRIG_BY_SEGMENT;
	}
	else if(ret_para->mode == TIMER_MODE_POWER_ON)
	{
		book.book_type = BOOK_POWER_ON;
		book.trigger_mode = BOOK_TRIG_BY_SEGMENT;
	}
	else if(ret_para->mode == TIMER_MODE_PLAY)
	{
		book.book_type = BOOK_PROGRAM_PLAY;
		book.trigger_mode = BOOK_TRIG_BY_SEGMENT;
	}
	else if(ret_para->mode == TIMER_MODE_PVR)
	{
		book.book_type = BOOK_PROGRAM_PVR;
		book.trigger_mode = BOOK_TRIG_BY_SEGMENT;
	}

/////////////////repeat_mode//////////////////
	if(ret_para->type == TIMER_TYPE_ONCE)
	{
		book.repeat_mode.mode = BOOK_REPEAT_ONCE;
	}
	else if(ret_para->type == TIMER_TYPE_DAILY)
	{
		book.repeat_mode.mode = BOOK_REPEAT_EVERY_DAY;
	}
	else
	{
		if(ret_para->type == TIMER_TYPE_MON)
		{
			local_weekday = WEEK_MON;
		}
		else if(ret_para->type == TIMER_TYPE_TUES)
		{
			local_weekday = WEEK_TUES;
		}
		else if(ret_para->type == TIMER_TYPE_WED)
		{
			local_weekday = WEEK_WED;
		}
		else if(ret_para->type == TIMER_TYPE_THURS)
		{
			local_weekday = WEEK_THURS;
		}
		else if(ret_para->type == TIMER_TYPE_FRI)
		{
			local_weekday = WEEK_FRI;
		}
		else if(ret_para->type == TIMER_TYPE_SAT)
		{
			local_weekday = WEEK_SAT;
		}
		else
		{
			local_weekday = WEEK_SUN;
		}
		
		gmt_weekday = app_weekday_local2gmt(local_weekday, ret_para->time % SEC_PER_DAY);
		book.repeat_mode.mode = g_AppBook.wday2mode(gmt_weekday);
	}
////////////trigger_time_start, trigger_time_end/////////////
	startT = get_local_time_by_timezone(ret_para->time);
	if(startT < 0)
	{
		book.trigger_time_start = startT + SEC_PER_DAY;
	}
	else
	{
		book.trigger_time_start = startT;
	}

/////////////trigger_time_end/////////////
	if((ret_para->mode == TIMER_MODE_PVR)
		&& (ret_para->duration == 0))
	{
		return GXCORE_ERROR;
	}

	book.trigger_time_end = book.trigger_time_start + 2;

////////////struct_size, struct_buf//////////////
	if((ret_para->mode == TIMER_MODE_PLAY)
		|| (ret_para->mode == TIMER_MODE_PVR))
	{
		if(ret_para->prog_info.prog_id == -1)
		{
			return GXCORE_ERROR;
		}
		
		//book.struct_size = sizeof(int);
		//memcpy(book.struct_buf, (uint8_t*)(&(ret_para->prog_info.prog_id)), sizeof(int));
		BookProgStruct prog_data;

		prog_data.prog_id = ret_para->prog_info.prog_id;
		prog_data.duration = ret_para->duration;

		book.struct_size = sizeof(BookProgStruct);
		memcpy(book.struct_buf, (uint8_t*)(&(prog_data)), book.struct_size);
		
	}
	else
	{
		book.struct_size = sizeof(GxBookType);
		bookType = book.book_type;
		memcpy(book.struct_buf,(uint8_t*)(&bookType),sizeof(GxBookType));
	}
	
	if((s_set_timer_state == TIMER_STATE_ADD)
		|| (s_set_timer_state == TIMER_STATE_EPG))
	{
		ret = g_AppBook.creat(&book);
	}
	else if(s_set_timer_state == TIMER_STATE_MODIFY)
	{
		book.id = s_timer_para.book_id;
		ret = g_AppBook.modify(&book);
	}

	return ret;
}

static int app_timer_edit_exit_callback(ExitType exit_type)
{

	SysTimerPara para_ret = {0};
	int ret = 0;
	if(exit_type != EXIT_ABANDON)
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		app_timer_edit_result_para_get(&para_ret);
		if(app_timer_edit_para_change(&para_ret) == TRUE)
		{
			pop.type = POP_TYPE_YES_NO;
			pop.str = STR_ID_SAVE_INFO;
			if(popdlg_create(&pop) == POP_VAL_OK)
			{
				if(app_timer_edit_para_save(&para_ret) == GXCORE_SUCCESS)
				{
					if(s_set_timer_state == TIMER_STATE_EPG)
					{
						extern void app_epg_event_timer_set(bool bflag);
						app_epg_event_timer_set(true);
					}
					if(s_set_timer_state == TIMER_STATE_ADD)
					{
						extern void app_add_timer_flag_set(bool bflag);
						app_add_timer_flag_set(true);
					}
					ret = 1;	
				}
				else
				{
					pop.type = POP_TYPE_OK;
					pop.str = STR_ID_ERR_TIMER;
					popdlg_create(&pop);
				}
			}

		}
	}
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&s_pmview_cur));


	if(s_timer_date_str != NULL)
	{
		GxCore_Free(s_timer_date_str);
		s_timer_date_str = NULL;
	}
	
	if(s_timer_time_str != NULL)
	{
		GxCore_Free(s_timer_time_str);
		s_timer_time_str = NULL;
	}

	return ret;
}

static int app_timer_mode_change_callback(int sel)
{

	if(s_set_timer_state == TIMER_STATE_EPG)
    	{
    		sel += TIMER_MODE_PLAY;
    	}
    	
	if((sel == TIMER_MODE_POWER_ON) || (sel == TIMER_MODE_POWER_OFF))
	{
		app_system_set_item_state_update(ITEM_TIMER_DURATION, ITEM_DISABLE);
		app_system_set_item_state_update(ITEM_TIMER_CHANNEL, ITEM_DISABLE);
	}

	if((sel == TIMER_MODE_PLAY) && (s_set_timer_state != TIMER_STATE_EPG))
	{
		app_system_set_item_state_update(ITEM_TIMER_DURATION, ITEM_DISABLE);
		app_system_set_item_state_update(ITEM_TIMER_CHANNEL, ITEM_NORMAL);
	}

	if((sel == TIMER_MODE_PVR) && (s_set_timer_state != TIMER_STATE_EPG))
	{
		app_system_set_item_state_update(ITEM_TIMER_DURATION, ITEM_NORMAL);
		app_system_set_item_state_update(ITEM_TIMER_CHANNEL, ITEM_NORMAL);
	}

	return EVENT_TRANSFER_KEEPON;
}

static int app_timer_type_change_callback(int sel)
{
	if(sel == TIMER_TYPE_ONCE)
	{
		app_system_set_item_state_update(ITEM_TIMER_DATE, ITEM_NORMAL);
	}
	else 
	{
		app_system_set_item_state_update(ITEM_TIMER_DATE, ITEM_DISABLE);
	}

	return EVENT_TRANSFER_KEEPON;
}

static int app_timer_channel_button_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
		{
			if(GXCORE_SUCCESS == app_timer_get_channel_dlg(s_prog_info_trace.prog_id, &s_prog_info_trace))
			{
				s_timer_edit_item[ITEM_TIMER_CHANNEL].itemProperty.itemPropertyBtn.string = (char *)s_prog_info_trace.prog_name;
				app_system_set_item_property(ITEM_TIMER_CHANNEL, &s_timer_edit_item[ITEM_TIMER_CHANNEL].itemProperty);
			}
			break;
		}

		default:
			break;
	}

	return ret;
}

static int app_timer_mode_press_callback(int key)
{
#if PVR_SUPPORT
    static char* s_EditEPGMode[]= {"Play","PVR"};
    static char* s_EditMode[]= {"Power On","Power Off","Play","PVR"};
#else
    static char* s_EditEPGMode[]= {"Play"};
    static char* s_EditMode[]= {"Power On","Power Off","Play"};
#endif
	int cmb_sel =-1;

	if(key == STBK_OK)
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_MODE;
        if(s_set_timer_state == TIMER_STATE_EPG)
        {
            pop_list.item_num = sizeof(s_EditEPGMode)/sizeof(*s_EditEPGMode);
            pop_list.item_content = s_EditEPGMode;
        }
        else
        {
            pop_list.item_num = sizeof(s_EditMode)/sizeof(*s_EditMode);
            pop_list.item_content = s_EditMode;
        }
		pop_list.sel = s_timer_edit_item[ITEM_TIMER_MODE].itemProperty.itemPropertyCmb.sel;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_2;
        pop_list.pos.x= 630;
        pop_list.pos.y= 136;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt1", "select", &cmb_sel);
	}


	return EVENT_TRANSFER_KEEPON;
}

static int app_timer_type_press_callback(int key)
{
    static char* s_EditEPGType[]= {"Once"};
    static char* s_EditType[]= {"Once","Daily","Mon.","Tues.","Wed.","Thurs.","Fri.","Sat.","Sun."};
	int cmb_sel =-1;

	if((key == STBK_OK) || (key == STBK_RIGHT))
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_MODE;
        if(s_set_timer_state == TIMER_STATE_EPG)
        {
            pop_list.item_num = sizeof(s_EditEPGType)/sizeof(*s_EditEPGType);
            pop_list.item_content = s_EditEPGType;
        }
        else
        {
            pop_list.item_num = sizeof(s_EditType)/sizeof(*s_EditType);
            pop_list.item_content = s_EditType;
        }
		pop_list.sel = s_timer_edit_item[ITEM_TIMER_MODE].itemProperty.itemPropertyCmb.sel;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_2;
        pop_list.pos.x= 630;
        pop_list.pos.y= 167;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt2", "select", &cmb_sel);
	}

    if((key == STBK_LEFT) || (key == STBK_RIGHT))
        return EVENT_TRANSFER_STOP;

	return EVENT_TRANSFER_KEEPON;
}

static void app_timer_edit_mode_item_init(void)
{
	s_timer_edit_item[ITEM_TIMER_MODE].itemTitle = STR_ID_MODE;
	s_timer_edit_item[ITEM_TIMER_MODE].itemType = ITEM_CHOICE;
#if PVR_SUPPORT
	if(s_set_timer_state == TIMER_STATE_EPG)
	{
		s_timer_edit_item[ITEM_TIMER_MODE].itemProperty.itemPropertyCmb.content= "[Play,PVR]";
	}
	else
	{
		s_timer_edit_item[ITEM_TIMER_MODE].itemProperty.itemPropertyCmb.content= "[Power On,Power Off,Play,PVR]";
	}
#else
	if(s_set_timer_state == TIMER_STATE_EPG)
	{
		s_timer_edit_item[ITEM_TIMER_MODE].itemProperty.itemPropertyCmb.content= "[Play]";
	}
	else
	{
		s_timer_edit_item[ITEM_TIMER_MODE].itemProperty.itemPropertyCmb.content= "[Power On,Power Off,Play]";
	}

#endif
	s_timer_edit_item[ITEM_TIMER_MODE].itemProperty.itemPropertyCmb.sel = 0;
	s_timer_edit_item[ITEM_TIMER_MODE].itemCallback.cmbCallback.CmbChange= app_timer_mode_change_callback;
	s_timer_edit_item[ITEM_TIMER_MODE].itemCallback.cmbCallback.CmbPress = app_timer_mode_press_callback;
	s_timer_edit_item[ITEM_TIMER_MODE].itemStatus = ITEM_NORMAL;
}

static void app_timer_edit_type_item_init(void)
{
	s_timer_edit_item[ITEM_TIMER_TYPE].itemTitle = STR_ID_TYPE;
	s_timer_edit_item[ITEM_TIMER_TYPE].itemType = ITEM_CHOICE;
	if(s_set_timer_state == TIMER_STATE_EPG)
	{
		s_timer_edit_item[ITEM_TIMER_TYPE].itemProperty.itemPropertyCmb.content= "[Once]";
		s_timer_edit_item[ITEM_TIMER_TYPE].itemStatus = ITEM_DISABLE;
	}
	else
	{
		s_timer_edit_item[ITEM_TIMER_TYPE].itemProperty.itemPropertyCmb.content= "[Once,Daily,Mon.,Tues.,Wed.,Thurs.,Fri.,Sat.,Sun.]";
		s_timer_edit_item[ITEM_TIMER_TYPE].itemStatus = ITEM_NORMAL;
	}
	s_timer_edit_item[ITEM_TIMER_TYPE].itemProperty.itemPropertyCmb.sel = s_timer_para.type;
	s_timer_edit_item[ITEM_TIMER_TYPE].itemCallback.cmbCallback.CmbChange= app_timer_type_change_callback;
	s_timer_edit_item[ITEM_TIMER_TYPE].itemCallback.cmbCallback.CmbPress = app_timer_type_press_callback;
}

static void app_timer_edit_date_item_init(void)
{
	char *str_temp = NULL;
	int str_len = 0;
	
	s_timer_edit_item[ITEM_TIMER_DATE].itemTitle = STR_ID_START_DATE;
	s_timer_edit_item[ITEM_TIMER_DATE].itemType = ITEM_EDIT;
	s_timer_edit_item[ITEM_TIMER_DATE].itemProperty.itemPropertyEdit.format = "edit_date";
	s_timer_edit_item[ITEM_TIMER_DATE].itemProperty.itemPropertyEdit.maxlen = "10";
	s_timer_edit_item[ITEM_TIMER_DATE].itemProperty.itemPropertyEdit.intaglio = NULL;
	s_timer_edit_item[ITEM_TIMER_DATE].itemProperty.itemPropertyEdit.default_intaglio = NULL;

	if(s_timer_date_str != NULL)
	{
		GxCore_Free(s_timer_date_str);
		s_timer_date_str = NULL;
	}
	str_temp = app_time_to_date_edit_str(s_timer_para.time);
	if(str_temp != NULL)
	{
		str_len = strlen(str_temp);
		s_timer_date_str = (char*)GxCore_Malloc(str_len + 1);
		if(s_timer_date_str != NULL)
		{
			memset(s_timer_date_str, 0 , str_len + 1);
			memcpy(s_timer_date_str, str_temp, str_len);
		}
		GxCore_Free(str_temp);
		str_temp =NULL;
	}
	s_timer_edit_item[ITEM_TIMER_DATE].itemProperty.itemPropertyEdit.string = s_timer_date_str;
	s_timer_edit_item[ITEM_TIMER_DATE].itemCallback.editCallback.EditReachEnd= NULL;
	s_timer_edit_item[ITEM_TIMER_DATE].itemCallback.editCallback.EditPress= NULL;
	if(s_set_timer_state == TIMER_STATE_EPG)
	{
		s_timer_edit_item[ITEM_TIMER_DATE].itemStatus = ITEM_DISABLE;
	}
	else
	{
		s_timer_edit_item[ITEM_TIMER_DATE].itemStatus = ITEM_NORMAL;
	}
}

static void app_timer_edit_time_item_init(void)
{
	char *str_temp = NULL;
	int str_len = 0;
	
	s_timer_edit_item[ITEM_TIMER_TIME].itemTitle = STR_ID_START_TIME;
	s_timer_edit_item[ITEM_TIMER_TIME].itemType = ITEM_EDIT;
	s_timer_edit_item[ITEM_TIMER_TIME].itemProperty.itemPropertyEdit.format = "edit_time";
	s_timer_edit_item[ITEM_TIMER_TIME].itemProperty.itemPropertyEdit.maxlen = "5";
	s_timer_edit_item[ITEM_TIMER_TIME].itemProperty.itemPropertyEdit.intaglio = NULL;
	s_timer_edit_item[ITEM_TIMER_TIME].itemProperty.itemPropertyEdit.default_intaglio = NULL;

	if(s_timer_time_str != NULL)
	{
		GxCore_Free(s_timer_time_str);
		s_timer_time_str = NULL;
	}
	str_temp = app_time_to_hourmin_edit_str(s_timer_para.time);
	if(str_temp != NULL)
	{
		str_len = strlen(str_temp);
		s_timer_time_str = (char*)GxCore_Malloc(str_len + 1);
		if(s_timer_time_str != NULL)
		{
			memset(s_timer_time_str, 0 , str_len + 1);
			memcpy(s_timer_time_str, str_temp, str_len);
		}
		GxCore_Free(str_temp);
		str_temp =NULL;
	}
	s_timer_edit_item[ITEM_TIMER_TIME].itemProperty.itemPropertyEdit.string = s_timer_time_str;
	s_timer_edit_item[ITEM_TIMER_TIME].itemCallback.editCallback.EditReachEnd= NULL;
	s_timer_edit_item[ITEM_TIMER_TIME].itemCallback.editCallback.EditPress= NULL;
	if(s_set_timer_state == TIMER_STATE_EPG)
	{
		s_timer_edit_item[ITEM_TIMER_TIME].itemStatus = ITEM_DISABLE;
	}
	else
	{
		s_timer_edit_item[ITEM_TIMER_TIME].itemStatus = ITEM_NORMAL;
	}
}

static void app_timer_edit_duration_item_init(void)
{
	char *str_temp = NULL;
	int str_len = 0;
	
	s_timer_edit_item[ITEM_TIMER_DURATION].itemTitle = STR_ID_DURATION;
	s_timer_edit_item[ITEM_TIMER_DURATION].itemType = ITEM_EDIT;
	s_timer_edit_item[ITEM_TIMER_DURATION].itemProperty.itemPropertyEdit.format = "edit_time";
	s_timer_edit_item[ITEM_TIMER_DURATION].itemProperty.itemPropertyEdit.maxlen = "5";
	s_timer_edit_item[ITEM_TIMER_DURATION].itemProperty.itemPropertyEdit.intaglio = NULL;
	s_timer_edit_item[ITEM_TIMER_DURATION].itemProperty.itemPropertyEdit.default_intaglio = NULL;

	//if((s_timer_para.mode == TIMER_MODE_PVR)
		//||(s_set_timer_state == TIMER_STATE_EPG))
	{
		if(s_timer_dura_str != NULL)
		{
			GxCore_Free(s_timer_dura_str);
			s_timer_dura_str = NULL;
		}
		str_temp = app_time_to_hourmin_edit_str(s_timer_para.duration);
		if(str_temp != NULL)
		{
			str_len = strlen(str_temp);
			s_timer_dura_str = (char*)GxCore_Malloc(str_len + 1);
			if(s_timer_dura_str != NULL)
			{
				memset(s_timer_dura_str, 0 , str_len + 1);
				memcpy(s_timer_dura_str, str_temp, str_len);
			}
			GxCore_Free(str_temp);
			str_temp =NULL;
		}

		str_temp = s_timer_dura_str;
	}
	//else
	{
		//str_temp = "00:01";
	}
	s_timer_edit_item[ITEM_TIMER_DURATION].itemProperty.itemPropertyEdit.string = str_temp;

	s_timer_edit_item[ITEM_TIMER_DURATION].itemCallback.editCallback.EditReachEnd= NULL;
	s_timer_edit_item[ITEM_TIMER_DURATION].itemCallback.editCallback.EditPress= NULL;

	if(s_timer_para.mode == TIMER_MODE_PVR)
	{
		s_timer_edit_item[ITEM_TIMER_DURATION].itemStatus = ITEM_NORMAL;
	}
	else
	{
		s_timer_edit_item[ITEM_TIMER_DURATION].itemStatus = ITEM_DISABLE;
	}
}

static void app_timer_prog_button_item_init(void)
{
	s_timer_edit_item[ITEM_TIMER_CHANNEL].itemTitle = STR_ID_CHANNEL;
	s_timer_edit_item[ITEM_TIMER_CHANNEL].itemType = ITEM_PUSH;
	s_timer_edit_item[ITEM_TIMER_CHANNEL].itemProperty.itemPropertyBtn.string= (char *)s_timer_para.prog_info.prog_name;
	s_timer_edit_item[ITEM_TIMER_CHANNEL].itemCallback.btnCallback.BtnPress= app_timer_channel_button_press_callback;

	if((s_timer_para.mode == TIMER_MODE_PVR || s_timer_para.mode == TIMER_MODE_PLAY)
		&& (s_set_timer_state != TIMER_STATE_EPG))
	{
		s_timer_edit_item[ITEM_TIMER_CHANNEL].itemStatus = ITEM_NORMAL;
	}
	else
	{
		s_timer_edit_item[ITEM_TIMER_CHANNEL].itemStatus = ITEM_DISABLE;
	}
}

static bool app_timer_edit_menu_create(void)
{
	bool ret = FALSE;

	memset(&s_timer_edit_opt, 0 ,sizeof(s_timer_edit_opt));

    s_timer_edit_opt.menuTitle = STR_ID_TIMER_EDIT;
    s_timer_edit_opt.itemNum = ITEM_TIMER_TOTAL;
    s_timer_edit_opt.item = s_timer_edit_item;
    if(s_set_timer_state == TIMER_STATE_EPG)
    {
        s_timer_edit_opt.backGround = BG_PLAY_RPOG;
    }
    else
    {
        s_timer_edit_opt.backGround = BG_MENU;
    }
    s_timer_edit_opt.timeDisplay = TIP_SHOW;

	app_timer_edit_mode_item_init();
	app_timer_edit_type_item_init();
	app_timer_edit_date_item_init();
	app_timer_edit_time_item_init();
	app_timer_edit_duration_item_init();
	app_timer_prog_button_item_init();

	s_timer_edit_opt.exit = app_timer_edit_exit_callback;

	if(app_system_set_block_create(&s_timer_edit_opt) == WND_OK)
	      ret = TRUE;
	return ret;	
}

bool app_add_timer_menu_exec(void)
{
	s_set_timer_state = TIMER_STATE_ADD;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&s_pmview_cur));
	app_timer_edit_para_new();
	memcpy(&s_prog_info_trace,&s_timer_para.prog_info, sizeof(ProgInfo));
	return  app_timer_edit_menu_create();
}

bool app_edit_timer_menu_exec(GxBook *book)
{
	s_set_timer_state = TIMER_STATE_MODIFY;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&s_pmview_cur));
	app_timer_edit_para_get(book);
	memcpy(&s_prog_info_trace, &s_timer_para.prog_info, sizeof(ProgInfo));
	return app_timer_edit_menu_create();
}

bool app_epg_timer_menu_exec(GxBusPmDataProg *prog_data, GxEpgInfo *event_info)
{
	int count;

	count = app_book_get_total_num();
	if(count < APP_BOOK_NUM)
	{
		s_set_timer_state = TIMER_STATE_EPG;
		app_timer_epg_para_get(prog_data, event_info);
		memcpy(&s_prog_info_trace, &s_timer_para.prog_info, sizeof(ProgInfo));
		app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&s_pmview_cur));
		return app_timer_edit_menu_create();
	}
	else
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.str = STR_ID_TIMER_FULL;
		pop.mode = POP_MODE_UNBLOCK;
		pop.timeout_sec = 3;
		popdlg_create(&pop) ;
		return FALSE;
	}
}

static GxBusPmViewInfo pmview_temp = {0};
static int channel_sel = -1;
static status_t app_timer_get_channel_dlg(int cur_channel_id, ProgInfo *prog_get)
{
	status_t ret = GXCORE_ERROR;
	GxBusPmViewInfo pmview_cur = {0};
	//GxBusPmViewInfo pmview_temp = {0};
	GxMsgProperty_NodeNumGet node_num_get;
	GxMsgProperty_NodeByPosGet node_by_pos = {0};
	//int channel_sel = -1;
	char *stream_str = NULL;

	s_channel_list_state = WND_CANCLE;

	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&pmview_cur));
	memcpy(&pmview_temp, &pmview_cur, sizeof(GxBusPmViewInfo));
	pmview_temp.group_mode = GROUP_MODE_ALL;
	pmview_temp.taxis_mode = TAXIS_MODE_NON;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_temp));
	node_num_get.node_type = NODE_PROG;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get))) //cur group
	{
		int i = 0;
		for(i = 0; i<node_num_get.node_num;i++)
		{
			node_by_pos.node_type = NODE_PROG;
			node_by_pos.pos = i;
			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node_by_pos)))
			{
				if(node_by_pos.prog_data.id == cur_channel_id)
				{	
					channel_sel = i;
					s_channel_list_state = WND_EXEC;

					if(pmview_temp.stream_type == GXBUS_PM_PROG_TV)
					{
						stream_str = STR_TV;
					}
					else
					{
						stream_str = STR_RADIO;
					}
					app_create_dialog("wnd_timer_channel_list");
					GUI_SetProperty("text_timer_channel_list_tv_radio", "string", stream_str);
					GUI_SetProperty("listview_timer_channel_list", "select", &channel_sel);
				}
			}
		}
	}

	if(s_channel_list_state == WND_CANCLE) //another stream
	{
		GxBusPmDataProgType stream_type_bak;

		stream_type_bak = pmview_temp.stream_type;
		switch(pmview_temp.stream_type)
		{
			case GXBUS_PM_PROG_TV:
#ifdef RADIO_REC_BOOK		
				pmview_temp.stream_type = GXBUS_PM_PROG_RADIO;
#endif				
				break;

			default:
				pmview_temp.stream_type = GXBUS_PM_PROG_TV;
				break;
		}

		if(stream_type_bak != pmview_temp.stream_type)
		{
			app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_temp));
			node_num_get.node_type = NODE_PROG;
			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get)))
			{
				int i = 0;
				for(i = 0; i<node_num_get.node_num;i++)
				{
					node_by_pos.node_type = NODE_PROG;
					node_by_pos.pos = i;
					if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node_by_pos)))
					{
						if(node_by_pos.prog_data.id == cur_channel_id)
						{	
							channel_sel = i;
							s_channel_list_state = WND_EXEC;
						}
					}
				}
			}
		}

		if(s_channel_list_state == WND_EXEC)
		{
			if(pmview_temp.stream_type == GXBUS_PM_PROG_TV)
			{
				stream_str = STR_TV;
			}
			else
			{
				stream_str = STR_RADIO;
			}
			app_create_dialog("wnd_timer_channel_list");
			GUI_SetProperty("text_timer_channel_list_tv_radio", "string", stream_str);
			GUI_SetProperty("listview_timer_channel_list", "select", &channel_sel);
		}
	}

	
	app_block_msg_destroy(g_app_msg_self);
#if 0	
	while(s_channel_list_state == WND_EXEC)
	{
		GUI_Exec();
		GxCore_ThreadDelay(50);
	}
#endif
	while(s_channel_list_state == WND_EXEC)
	{
	    GUI_LoopEvent();
	   GxCore_ThreadDelay(50);
	}
	GUI_StartSchedule();
	
	if(s_channel_list_state == WND_OK)
	{
		node_num_get.node_type = NODE_PROG;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num_get);
		if(node_num_get.node_num > 0)
		{
			GUI_GetProperty("listview_timer_channel_list", "select", &channel_sel);

			node_by_pos.node_type = NODE_PROG;
			node_by_pos.pos = channel_sel;
			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)&node_by_pos))
			{
				prog_get->prog_id = node_by_pos.prog_data.id;
				memcpy(prog_get->prog_name, node_by_pos.prog_data.prog_name, sizeof(prog_get->prog_name));
				ret = GXCORE_SUCCESS;
			}
		}
	}
	GUI_EndDialog("wnd_timer_channel_list");
	//GUI_Exec();
	GUI_SetInterface("flush",NULL);
	app_block_msg_init(g_app_msg_self);

	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pmview_cur));
	return ret;

}

SIGNAL_HANDLER int app_timer_channel_list_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_timer_channel_list_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_timer_channel_list_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;
	uint32_t sel = 0;
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_MENU:
			case STBK_EXIT:
				s_channel_list_state = WND_CANCLE;	
				break;
#if 0
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				if(s_set_timer_state == TIMER_STATE_EPG)
				{
					PlayerWindow video_wnd = {0, 0, 1024, 576};
					GUI_SetInterface("flush", NULL);
					g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
				}
				break;	
#endif 
			case STBK_OK:
				s_channel_list_state = WND_OK;
				break;

			case STBK_LEFT:
			case STBK_RIGHT:
				ret = EVENT_TRANSFER_STOP;
				break;
			case STBK_PAGE_UP:
				{
					GxMsgProperty_NodeNumGet node;
					node.node_type = NODE_PROG;
					app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node);
					GUI_GetProperty("listview_timer_channel_list", "select", &sel);
					if(sel == 0)
					{
						sel = node.node_num-1;
						GUI_SetProperty("listview_timer_channel_list", "select", &sel);
					}
					else
					{
						ret = EVENT_TRANSFER_KEEPON;
					}

				}
				break;
			case STBK_PAGE_DOWN:
				{
					GxMsgProperty_NodeNumGet node;
					node.node_type = NODE_PROG;
					app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node);
					GUI_GetProperty("listview_timer_channel_list", "select", &sel);
					if(sel == node.node_num-1)
					{
						sel=0;
						GUI_SetProperty("listview_timer_channel_list", "select", &sel);
					}
					else
					{
						ret = EVENT_TRANSFER_KEEPON;
					}

				}

				break;

#ifdef RADIO_REC_BOOK
			case STBK_TV_RADIO:
			{
				GxBusPmViewInfo cur_pmview = {0};
				app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&cur_pmview));
				if(cur_pmview.stream_type == GXBUS_PM_PROG_TV)
				{
					cur_pmview.stream_type = GXBUS_PM_PROG_RADIO;
					//sel = cur_pmview.radio_prog_cur;
					GUI_SetProperty("text_timer_channel_list_tv_radio", "string", STR_RADIO);
				}
				else
				{
					cur_pmview.stream_type = GXBUS_PM_PROG_TV;
					//sel = cur_pmview.tv_prog_cur;
					GUI_SetProperty("text_timer_channel_list_tv_radio", "string", STR_TV);
				}

				app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&cur_pmview));

				GUI_SetProperty("listview_timer_channel_list", "update_all", NULL);

				if(cur_pmview.stream_type == pmview_temp.stream_type)
				{
					GUI_SetProperty("listview_timer_channel_list", "select", &channel_sel);
				}
				else
				{
					GUI_SetProperty("listview_timer_channel_list", "select", &sel);
				}

                GUI_SetInterface("flush", 0);
                break;
            }
#endif
            default:
            ret = EVENT_TRANSFER_KEEPON;
            break;
        }
	}
	return ret;
}

SIGNAL_HANDLER int app_timer_channel_list_get_total(GuiWidget *widget, void *usrdata)
{
//#define MAX_PROG_LIST_ITEM 7
	GxMsgProperty_NodeNumGet node;

	node.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node);
#if 0
	if(node.node_num > MAX_PROG_LIST_ITEM)
	{
		GUI_SetProperty("scroll_timer_channel_list", "format", "scroll_show");
	}
	else
	{
		GUI_SetProperty("scroll_timer_channel_list", "format", "scroll_hide");
	}
#endif //get_total 里不能设置GUI属性
	return node.node_num;
}

SIGNAL_HANDLER int app_timer_channel_list_get_data(GuiWidget *widget, void *usrdata)
{
#define CHANNEL_NAME_LEN	10
	ListItemPara* item = NULL;
	static GxMsgProperty_NodeByPosGet node;
	static char buffer[CHANNEL_NAME_LEN];
	static char buffer2[2];

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	//col-0: num
	item->x_offset = 0;
	item->image = NULL;
	memset(buffer, 0, CHANNEL_NAME_LEN);
	sprintf(buffer, " %d", (item->sel+1));
	item->string = buffer;
	
	//col-1: channel name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	node.node_type = NODE_PROG;
	node.pos = item->sel;
	//get node
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
	item->string = (char*)node.prog_data.prog_name;

	//col-2: $
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	memset(buffer2, 0, 2);
	if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
		sprintf(buffer2, "%s", "$");
		item->string = buffer2;
	}

	//col-3: lock
    item = item->next;
    if (item == NULL) 
        return GXCORE_ERROR;
    item->x_offset = 0;
	item->string = NULL;
	if (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
		 item->image = "s_icon_lock.bmp";
	}
	else
	{
		item->image = NULL;
	}

	return EVENT_TRANSFER_STOP;
}

