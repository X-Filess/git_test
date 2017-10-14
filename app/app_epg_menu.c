#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_epg.h"
#include "app_utility.h"
#include "app_book.h"
#include "app_config.h"

#include "app_pop.h"
#include "full_screen.h"
#if TKGS_SUPPORT
#include "module/gxtkgs.h"
GxTkgsOperatingMode app_tkgs_get_operatemode(void);
#endif


#define WND_EPG                     "wnd_epg"
#define LISTVIEW_EPG_PROG           "listview_epg_prog"
#define TIMELIST_EPG_EVENT          "timelist_epg_event"
#define GROUP_EPG_PROG              "cmb_epg_prog_group"
#define TEXT_EGP_DATE               "text_epg_date"
#define TEXT_EPG_SYS_TIME           "text_epg_sys_time"
#define IMG_EPG_DETAILS_BACK        "img_epg_details_back"
//#define BTN_EPG_DETAILS_BACK      "button_epg_event_details_back"
#define IMG_EPG_EVENT_DETAILS_BACK  "img_epg_event_details_back"

#define NOTEPAD_EPG_EVENT           "notepad_epg_event_details"
#define TEXT_EVENT_TITLE            "text_epg_event_details_title"
#define TEXT_EVENT_START_TIME       "text_epg_event_details_startT"
#define TEXT_EVENT_END_TIME         "text_epg_event_details_endT"
#define TEXT_EVENT_TIME_INTERVAL    "text_epg_event_details_interval"
#define BTN_EVNET_DETAILS_EXIT      "btn_epg_event_details_exit"
#define IMG_EPG_PREV_DAY            "img_epg_green_tip"
#define IMG_EPG_NEXT_DAY            "img_epg_yellow_tip"
#define IMG_EPG_SHOW_DETAIL         "img_epg_show_detail_tip"
#define TEXT_EPG_PREV_DAY           "text_epg_prev_day_tip"
#define TEXT_EPG_NEXT_DAY           "text_epg_next_day_tip"
#define TEXT_EPG_SHOW_DETAIL        "text_epg_show_detail_tip"



#define MAX_EPG_PROG 5
#define TMLIST_ELEM_FIX_LEN 23 //
#define SEC_PER_HOUR (60 * 60)
#define SEC_PER_DAY (60 * 60 * 24)
#define MAX_DISPLAY_EVENT 80

#if(MINI_16_BITS_OSD_SUPPORT == 1)
#define EPG_POPDLG_POS_X 180
#define EPG_POPDLG_POS_Y 180
#else
#define EPG_POPDLG_POS_X 215
#define EPG_POPDLG_POS_Y 220
#endif

enum{
	ZOOM_FULL_X = 0,
	ZOOM_FULL_Y = 0,
	ZOOM_FULL_W = APP_XRES,
	ZOOM_FULL_H = APP_YRES

};
static     PlayerWindow video_wnd ;
//static int thiz_cur_select_status = 0;

extern bool app_check_av_running(void);
#if MINI_16BIT_WIN8_OSD_SUPPORT
extern int app_main_menu_create_flag(void);
#endif

void epg_macro_ch_ed_start_video(void)
{
#define VIDEO_TEXT "txt_epg_for_pp"
    int x,y,w,h = 0;
    sscanf((widget_get_property(gui_get_widget(NULL,VIDEO_TEXT),"rect")),"[%d,%d,%d,%d]",&x,&y,&w,&h);

    // zoom out
    video_wnd.x = x;
    video_wnd.y = y;
    video_wnd.width = w;
    video_wnd.height = h;

    GUI_SetInterface("flush", NULL);
    g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);

    if(g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK)
    {
        g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    }
    else
    {
        GUI_SetInterface("video_top", NULL);
    }
    g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
    GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
}

void epg_macro_ch_ed_start_full_video(void)
{

    // zoom out full screen
    PlayerWindow video_wnd = {ZOOM_FULL_X, ZOOM_FULL_Y, ZOOM_FULL_W, ZOOM_FULL_H};
    GUI_SetInterface("flush", NULL);
    g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);

    if((g_AppPlayOps.normal_play.key == PLAY_KEY_UNLOCK)
            ||(g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK))//(thiz_cur_select_status == g_AppPlayOps.normal_play.play_count)
    {
		g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    }
	GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);

    //GUI_SetInterface("video_off", NULL);
    //g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
}

#define EPG_START_VIDEO()   epg_macro_ch_ed_start_video()
#define EPG_START_FULL_VIDEO()   epg_macro_ch_ed_start_full_video()

typedef struct
{
	char **elem_array;
	EpgIndex *elem_index;
	int elem_cnt;
}TmlistData;

static AppEpgOps sp_EpgOps[MAX_EPG_PROG];
static int s_epg_prog_map[MAX_EPG_PROG];
static int s_epg_prog_map_bak[MAX_EPG_PROG];
static TmlistData s_epg_dispaly_data[MAX_EPG_PROG];
static GxEpgInfo *s_event_info = NULL;
static event_list* s_get_epg_timer = NULL;

static event_list* s_details_show_timer = NULL;

static event_list* s_tm_update_timer = NULL;
static  uint8_t s_epg_day = 0;
static  int s_epg_cnt_bak[MAX_EPG_PROG] = {0};
static time_t s_time_hour = 0;
static time_t s_time_date = 0;
static time_t s_offset_sec = 0;
static time_t s_next_evnt_time = 0;
static GxEpgInfo *s_focus_event = NULL;
static bool s_epg_event_details_show = false;
//static event_list* s_book_info_timer = NULL;
static GxMsgProperty_NodeByPosGet s_cur_prog = {0};

static bool s_epg_event_timer = false;

void app_epg_event_timer_set(bool bflag)
{
	if(s_epg_event_timer == true)
	{
		 GUI_SetProperty("text_epg_book_tip", "string", STR_ID_DELETE);
	}
	s_epg_event_timer = bflag;
}

static char *s_egp_event_row[] =
{
	"timeitem_epg_evnt_list1",
	"timeitem_epg_evnt_list2",
	"timeitem_epg_evnt_list3",
	"timeitem_epg_evnt_list4",
	"timeitem_epg_evnt_list5",
	"timeitem_epg_evnt_list6",
	"timeitem_epg_evnt_list7",
	"timeitem_epg_evnt_list8",
	"timeitem_epg_evnt_list9",
	"timeitem_epg_evnt_list10",
};

extern bool app_epg_timer_menu_exec(GxBusPmDataProg *prog_data, GxEpgInfo *event_info);
int app_epg_prog_list_change(GuiWidget *widget, void *usrdata);
static int app_epg_tm_update_timer(void* usrdata);
static int event_details_show_timer(void* usrdata);
static status_t app_epg_get_event_book(uint16_t prog_id,  GxEpgInfo *event, GxBook *book_ret);
static uint16_t app_epg_dispaly_event_zone(int prog, uint8_t  day,  time_t offset_sec, EpgIndex *index_ret);
static status_t app_epg_tmlist_data_get(int prog, uint8_t  day, time_t offset_sec, TmlistData *data);

static void app_epg_timer_edit_hide_time(void)
{
	if(s_tm_update_timer != NULL)
		timer_stop(s_tm_update_timer);
}

static void app_epg_timer_edit_show_time(void)
{
	app_epg_tm_update_timer(NULL);
	if(0 != reset_timer(s_tm_update_timer))
	{
		s_tm_update_timer = create_timer(app_epg_tm_update_timer, 800, NULL, TIMER_REPEAT);
	}
}

static void app_epg_prog_list_map_update(int prog_sel,int cur_sel)
{
    int first_sel;
    int i;

    first_sel = prog_sel - cur_sel;
    for(i = 0; i < MAX_EPG_PROG; i++)
    {
        if(first_sel + i < g_AppPlayOps.normal_play.play_total)
        {
            s_epg_prog_map[i] = first_sel + i;
        }
        else
        {
            s_epg_prog_map[i] = -1;
        }

    }
}

static void app_epg_prog_info_update(int prog_num)
{
    EpgProgInfo prog_info;
    GxMsgProperty_NodeByPosGet node;

    if(s_epg_prog_map[prog_num] < 0)
        return;

    memset(&node,0,sizeof(GxMsgProperty_NodeByPosGet));
    node.node_type = NODE_PROG;
    node.pos = s_epg_prog_map[prog_num];
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        prog_info.ts_id= node.prog_data.ts_id;
        prog_info.orig_network_id= node.prog_data.original_id;
        prog_info.service_id= node.prog_data.service_id;
        prog_info.prog_id= node.prog_data.id;
        prog_info.prog_pos = node.pos;
        // use tp id to identify the different TS's infomation
        //prog_info.tp_id   = node.prog_data.tp_id;

        sp_EpgOps[prog_num].prog_info_update(&sp_EpgOps[prog_num], &prog_info);
    }
}

void app_epg_tmlist_info_update(int prog_sel,int cur_sel)
{

    int i;
	app_epg_prog_list_map_update(prog_sel,cur_sel);
	for(i = 0; i < MAX_EPG_PROG; i++)
	{
		app_epg_prog_info_update(i);
		sp_EpgOps[i].event_cnt_update(&sp_EpgOps[i]);
		s_epg_cnt_bak[i] = app_epg_dispaly_event_zone(i, s_epg_day, s_offset_sec, NULL);
		app_epg_tmlist_data_get(i, s_epg_day, s_offset_sec, &s_epg_dispaly_data[i]);
		s_epg_prog_map_bak[i] = s_epg_prog_map[i];
	}
}




//free result outside
static char* app_epg_tmlist_elem_format(int sel, uint8_t event_day, GxEpgInfo *event, GxMsgProperty_NodeByIdGet *node)
{
    char *eve_str = NULL;
    char *temp_str = NULL;
	char *temp_str2 = NULL;
	char start[8] = "";
    int name_len = 0;
    int sel_len = 0;
    int total_len = 0;
    time_t time;
    char *ptr = NULL;
    uint8_t interval_day = 0;
    GxBook prog_book;
    time_t cur_time = 0;

    char day_str[2] = {0};
    char *p_color_fore= NULL;
    char *p_color_back= NULL;
    char *p_font= "Arial";

    if((event == NULL) || (node == NULL))
        return NULL;

    name_len= strlen((char*)event->event_name);

    if(sel < 10)
        sel_len = 1;
    else if((sel >= 10) && (sel < 100))
        sel_len = 2;
    else if((sel  >= 100) && sel < 1000)
        sel_len = 3;
    else
        sel_len = 4;

    total_len = TMLIST_ELEM_FIX_LEN + name_len + sel_len;

    cur_time = g_AppTime.utc_get(&g_AppTime);
    if((s_cur_prog.prog_data.id == node->prog_data.id)
        && (cur_time > event->start_time && cur_time < event->start_time + event->duration)) //current event
    {
		s_next_evnt_time = event->start_time + event->duration;
        p_color_fore= "#f4f4f4";
        p_color_back = "#005a99";
    }
    else
    {
        memset(&prog_book, 0, sizeof(GxBook));
        if(app_epg_get_event_book(node->prog_data.id, event, &prog_book) == GXCORE_SUCCESS)
        {
            p_color_fore= "#f4f4f4";
            if (prog_book.book_type == BOOK_PROGRAM_PLAY)
            {
                p_color_back = "#008000";
            }
            else if (prog_book.book_type == BOOK_PROGRAM_PVR)
            {
                p_color_back = "#E6A200";
            }
        }
    }
	if(p_color_back != NULL)
    {
        total_len += strlen(p_color_fore);
        total_len += strlen(p_color_back);
        total_len += strlen(p_font);
        total_len += 6;
    }
    eve_str = (char*)GxCore_Malloc(total_len);
    if(eve_str == NULL)
        return NULL;

    memset(eve_str, 0, total_len);

    ptr = eve_str;
    ////////No.////////
    memset(ptr, 0, total_len);
    sprintf(ptr, "<%d>", sel);
    ptr = ptr + sel_len + 2;

    ///////start time///////
	GUI_GetProperty(TIMELIST_EPG_EVENT, "start",start);
    *ptr++ = '<';
    time = get_display_time_by_timezone(event->start_time);
	temp_str = app_time_to_hourmin_edit_str(time);
	time %= SEC_PER_DAY;
	time += event->duration;
	if(time >= SEC_PER_DAY && strncmp(start,"<00:00>",strlen("<00:00>")) == 0)
	{

		temp_str2 = app_time_to_hourmin_edit_str(time);
		sprintf(eve_str,"<0><%s-1><%s><%s>",temp_str,temp_str2,(char*)event->event_name);
		return eve_str;
	}
    memcpy(ptr, temp_str, 5);
    GxCore_Free(temp_str);
    ptr += 5;

    if(event_day != s_epg_day)
    {
        if(s_epg_day > event_day)
        {
			*ptr++ = '-';
			interval_day = s_epg_day - event_day;
			sprintf(day_str, "%d", interval_day);
			*ptr++ = day_str[0];
		}
        else
        {
			if(time < SEC_PER_DAY && sel != 0)
			{

				*ptr++ = '+';
				interval_day = event_day - s_epg_day;
				sprintf(day_str, "%d", interval_day);
				*ptr++ = day_str[0];
			}

       }
   }
    *ptr++ = '>';

    ///////end time///////
    *ptr++ = '<';
    time = get_display_time_by_timezone(event->start_time + event->duration);
    temp_str = app_time_to_hourmin_edit_str(time);
    memcpy(ptr, temp_str, 5);
    GxCore_Free(temp_str);
    ptr += 5;

    if(event_day < s_epg_day)
    {
        interval_day = s_epg_day - event_day;
        time = get_display_time_by_timezone(event->start_time);
        time %= SEC_PER_DAY;
        time += event->duration;
		if(time >= SEC_PER_DAY && interval_day > 0)
		{
				interval_day--;
		}
       if(interval_day != 0)
        {
            *ptr++ = '-';
            sprintf(day_str, "%d", interval_day);
            *ptr++ = day_str[0];
        }
    }
    else if(event_day >= s_epg_day)
    {
        interval_day = event_day - s_epg_day;
        time = get_display_time_by_timezone(event->start_time);
        time %=SEC_PER_DAY;
        time += event->duration;
		if(time >= SEC_PER_DAY && interval_day < 1)
		{
				interval_day++;
		}
       if(interval_day != 0)
        {
            *ptr++ = '+';
			sprintf(day_str, "%d", interval_day);
            *ptr++ = day_str[0];
        }
    }

    *ptr++ = '>';

    ////////name///////
    *ptr++ = '<';
    memcpy(ptr, (char*)event->event_name, name_len);
    ptr += name_len;
    *ptr++ = '>';

    if(p_color_back != NULL)
    {
        *ptr++ = '<';
        memcpy(ptr, p_color_fore, strlen(p_color_fore));
        ptr += strlen(p_color_fore);
        *ptr++ = '>';
        *ptr++ = '<';
        memcpy(ptr, p_color_back, strlen(p_color_back));
        ptr += strlen(p_color_back);
        *ptr++ = '>';
        //font
        *ptr++ = '<';
        memcpy(ptr, p_font, strlen(p_font));
        ptr += strlen(p_font);
        *ptr++ = '>';
    }
    *ptr='\0';
    //printf("\n-----------------    eve_str:%s   --------\n",eve_str);
    return eve_str;
}

static void  app_epg_tmlist_data_init(TmlistData *data)
{
	if(data != NULL)
	{
		data->elem_array = (char **)GxCore_Malloc(sizeof(char*));
		if(data->elem_array != NULL)
		{
			data->elem_array[0] = "<end>";
		}
		data->elem_index= NULL;
		data->elem_cnt = 0;
	}
}

static void app_epg_tmlist_data_release(TmlistData *data)
{
    uint16_t i;
    if(data != NULL)
    {
        if(data->elem_array != NULL)
        {
            for(i = 0; i < data->elem_cnt; i++)
            {
                if(data->elem_array[i] != NULL)
                {
                    GxCore_Free(data->elem_array[i]);
                    data->elem_array[i] = NULL;
                }
            }
            GxCore_Free(data->elem_array);
            data->elem_array = NULL;
        }

        if(data->elem_index!= NULL)
        {
            GxCore_Free(data->elem_index);
            data->elem_index= NULL;
        }

        data->elem_cnt = 0;
    }
}

static uint16_t app_epg_dispaly_event_zone(int prog, uint8_t  day,  time_t offset_sec, EpgIndex *index_ret)
{
    uint16_t ret = 0;
    uint16_t count = 0;
    int16_t i,j,m1 = 0, m2 = 0;
    time_t cur_time = 0;
    time_t epg_time = 0;
    EpgIndex epg_index = {0};
    EpgIndex last_day_index[20];

    if(s_epg_prog_map[prog] < 0)
        return 0;
#if 0
#ifndef MULTI_TP_EPG_INFO
        GxMsgProperty_NodeByPosGet cur_tp_node = {0};
        GxMsgProperty_NodeByPosGet prog_tp_node = {0};

        cur_tp_node.node_type = NODE_PROG;
        cur_tp_node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&cur_tp_node));

        prog_tp_node.node_type = NODE_PROG;
        prog_tp_node.pos = s_epg_prog_map[prog];
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_tp_node));

        if(cur_tp_node.prog_data.tp_id != prog_tp_node.prog_data.tp_id)
            return 0;
#endif
#endif
    cur_time = g_AppTime.utc_get(&g_AppTime);
    cur_time = get_display_time_by_timezone(cur_time);
    cur_time = cur_time - cur_time % SEC_PER_DAY;
    cur_time += day * SEC_PER_DAY;
    cur_time += offset_sec;

    //Modified for BUG#12753
    //if((day > 0) && (offset_sec < (SEC_PER_HOUR * 3)))
    if(day > 0)
    {
        count = sp_EpgOps[prog].get_day_event_cnt(&sp_EpgOps[prog], day - 1);
        if(count > 0)
        {
            for(i = count - 1; i >= 0; i--)
            {
                epg_index.day = day - 1;
                epg_index.sel = i;
                if(sp_EpgOps[prog].get_day_event_info(&sp_EpgOps[prog], &epg_index, s_event_info) == GXCORE_SUCCESS)
                {
                    epg_time = get_display_time_by_timezone(s_event_info->start_time);
                    if(((epg_time > cur_time - (SEC_PER_HOUR * 1))||((s_event_info->duration > (SEC_PER_HOUR * 1))&&(epg_time <= cur_time - (1 * SEC_PER_HOUR)))) && (ret <= MAX_DISPLAY_EVENT))
                    {
                        memcpy(&last_day_index[ret], &epg_index, sizeof(EpgIndex));
                        ret++;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            if(index_ret != NULL)
            {
                for(i = 0; i < ret; i++)
                {
                    memcpy(&index_ret[i], &last_day_index[ret - i - 1], sizeof(EpgIndex));
                }
            }
        }

    }

    count = sp_EpgOps[prog].get_day_event_cnt(&sp_EpgOps[prog], day);
#if 1	
    if((count > 0) && (ret <= MAX_DISPLAY_EVENT))
    {
        epg_index.day = day;
        i = 0;
        j = count;
        while(i < j)
        {
            m1 = (i + j) / 2;
            epg_index.sel = m1;
            sp_EpgOps[prog].get_day_event_info(&sp_EpgOps[prog], &epg_index, s_event_info);
            epg_time = get_display_time_by_timezone(s_event_info->start_time);
            if((epg_time > cur_time - (SEC_PER_HOUR * 1))||((epg_time <= cur_time - (SEC_PER_HOUR * 1))&&(s_event_info->duration > (1 *SEC_PER_HOUR))))
            {
                j = m1;
            }
            else if(epg_time < cur_time - (SEC_PER_HOUR * 1))
            {
                i = m1 + 1;
            }
            else
            {
                m1++;
                break;
            }
        }

        i = m1;
        j = count;
        while(i < j)
        {
            m2 = (i + j) / 2;
            epg_index.sel = m2;
            sp_EpgOps[prog].get_day_event_info(&sp_EpgOps[prog], &epg_index, s_event_info);
            epg_time = get_display_time_by_timezone(s_event_info->start_time);
            if(epg_time > cur_time + (SEC_PER_HOUR * 4))
            {
                j = m2;
            }
            else if(epg_time < cur_time + (SEC_PER_HOUR * 4))
            {
                i = m2 + 1;
            }
            else
            {
                break;
            }
        }
        if(i >= j)
        {
            m2++;
        }

        for(i = m1; i < m2 && ret <= MAX_DISPLAY_EVENT; i++)
        {
            epg_index.sel = i;
            if(index_ret != NULL)
            {
                memcpy(&index_ret[ret], &epg_index, sizeof(EpgIndex));
            }
            ret++;
        }
    }
#endif

    if((day < MAX_EPG_DAY -1 ) && (offset_sec > (SEC_PER_HOUR * 20)))
    {
        count = sp_EpgOps[prog].get_day_event_cnt(&sp_EpgOps[prog], day + 1);
        if(count > 0)
        {
            for(i = 0; i < count ; i++)
            {	
                epg_index.day = day + 1;
                epg_index.sel = i;
                if(sp_EpgOps[prog].get_day_event_info(&sp_EpgOps[prog], &epg_index, s_event_info) == GXCORE_SUCCESS)
                {
                    epg_time = get_display_time_by_timezone(s_event_info->start_time);
                    if((epg_time < cur_time + (SEC_PER_HOUR * 4)) && (ret <= MAX_DISPLAY_EVENT))
                    {	
                        if(index_ret != NULL)
                        {
                            memcpy(&index_ret[ret], &epg_index, sizeof(EpgIndex));
                        }	
                        ret++;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }

    return ret;
}

static char* _timlist_get_null_item(char* pBefore,char* pNow,int sel)
{// <xx><xx:xx+1><xx:xx+1><name>
	char *eve_str = NULL;
	char *p = NULL;
	int TempBefore = 0;
	int TempNow = 0;
	char a[20] = {0};
	char b[20] = {0};
	char c[20] = {0};
	int len1 = 0;
	int len2 = 0;


	if(0 == sel)// special, insert the black item "<0><00:00><now start time><>"
	{
		//start time for now
		sscanf((pNow+1+2+1),"%[^>]",b);
		len2 = strlen(b);
		if(0 == memcmp("00:00",b,5))
			return NULL;
		eve_str = (char*)GxCore_Malloc(50);
		if(eve_str == NULL)
			return NULL;
		memset(eve_str, 0, 50);
		p = eve_str;
		sprintf(p,"<0><%s><%s><>",b,b);
		p += 4;
		memcpy(p,"00:00",5);
		eve_str[30] = '\0';		
		return eve_str;
	}


	if((NULL == pBefore) || (NULL == pNow))
		return NULL;

	
	if(sel < 10)
	{
		TempNow = 1;
		TempBefore = 1;
	}
	else if(sel == 10)
	{
		TempNow = 2;
		TempBefore = 1;
	}
	else if((sel > 10) && (sel < 100))
	{
		TempNow = 2;
		TempBefore = 2;
	}
	else if(sel == 100)
	{
		TempNow = 3;
		TempBefore = 2;
	}
	else if((sel  > 100) && sel < 1000)
	{
		TempNow = 3;
		TempBefore = 3;
	}
	else if(sel == 1000)
	{
		TempNow = 4;
		TempBefore = 3;
	}
	else// max value support "9999"
	{
		TempNow = 4;
		TempBefore = 4;
	}

	
	// start time for before
	sscanf((pBefore+TempBefore+2+1),"%[^>]",a);
	len1 = strlen(a);
	//end time for before
	sscanf((pBefore+TempBefore+2+len1+3),"%[^>]",c);
	len1 = strlen(c);
	
	//start time for now
	sscanf((pNow+TempNow+2+1),"%[^>]",b);
	len2 = strlen(b);
	
    //if((len1 == len2)&&(0 == memcmp(c,b,len1)))
    if(0 == memcmp(c,b,5)) //len(xx:xx), ignore date info
		return NULL;
	
	eve_str = (char*)GxCore_Malloc(50);
	if(eve_str == NULL)
		return NULL;
	
	memset(eve_str, 0, 50);
	p = eve_str;

	// add the count "<x>"
	sprintf(p,"<%d>",sel);
	p += strlen(eve_str);

	// add the start time "<xx:xx+1>"
	*p++ = '<';
	memcpy(p,c,len1);
	p += len1;
	*p++ = '>';
	// add the end time "<xx:xx+1>"
	*p++ = '<';
	memcpy(p,b,len2);
	p += len2;
	*p++ = '>';
	// null
	*p++ = '<';
	*p++ = '>';
	eve_str[30] = '\0';

	return eve_str;
}

static status_t app_epg_tmlist_data_get(int prog, uint8_t  day, time_t offset_sec, TmlistData *data)
{
    int i = 0;
    int count = 0;
    char *temp_str = NULL;
    char *temp_str_black = NULL;
    EpgIndex *temp_index = NULL;
    time_t last_start_time = 0;
    time_t last_duration_time = 0;

    GxMsgProperty_NodeByIdGet node;

    if(data == NULL)
        return GXCORE_ERROR;

    app_epg_tmlist_data_release(data);
    temp_index = (EpgIndex*)GxCore_Malloc(MAX_DISPLAY_EVENT * sizeof(EpgIndex));
    if(temp_index == NULL)
    {
        app_epg_tmlist_data_init(data);
        return GXCORE_ERROR;
    }

    count = app_epg_dispaly_event_zone(prog, day, offset_sec, temp_index);
    if(count == 0)
    {
        app_epg_tmlist_data_init(data);
        GxCore_Free(temp_index);
        temp_index = NULL;
        return GXCORE_ERROR;
    }
    if(count > MAX_DISPLAY_EVENT)
        data->elem_cnt = MAX_DISPLAY_EVENT*2;// add for insert black item
    else
        data->elem_cnt = count*2;// add for insert black item
    data->elem_index = (EpgIndex*)GxCore_Malloc(data->elem_cnt * sizeof(EpgIndex));
    if(data->elem_index == NULL)
    {
        app_epg_tmlist_data_init(data);
        GxCore_Free(temp_index);
        temp_index = NULL;
        return GXCORE_ERROR;
    }
    memcpy(data->elem_index, temp_index, data->elem_cnt * sizeof(EpgIndex));
    //GxCore_Free(temp_index);
    //temp_index = NULL;

    data->elem_array = (char **)GxCore_Malloc((data->elem_cnt + 1) * sizeof(char*));
    if(data->elem_array == NULL)
    {
        app_epg_tmlist_data_release(data);
        app_epg_tmlist_data_init(data);
        return GXCORE_ERROR;
    }
    memset(data->elem_array, 0, (data->elem_cnt + 1) * sizeof(char*));

    count = 0;

    // get node, to judge whether the event hade booked
    node.node_type = NODE_PROG;
    node.pos = sp_EpgOps[prog].prog_info.prog_pos;
    //node.pos = g_AppPlayOps.normal_play.play_count;
    //printf("\n************* node.pos:%d   ,pos_sel:%d __LINE__:%d\n",node.pos,pos_sel,__LINE__);
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

    for(i = 0; i < data->elem_cnt/2 ; i++)
    {
        if(sp_EpgOps[prog].get_day_event_info(&sp_EpgOps[prog], &temp_index[i], s_event_info) == GXCORE_SUCCESS)
        {
            if(s_event_info->start_time >= last_start_time + last_duration_time)
            {
                last_start_time = s_event_info->start_time;
                last_duration_time = s_event_info->duration;
            }
            else
            {
                continue;
            }
	
            temp_str = app_epg_tmlist_elem_format(count, temp_index[i].day, s_event_info, &node);

            // add for inser black item
#if 1
            if((count > 0) && (NULL != temp_str))
            {
                temp_str_black = _timlist_get_null_item(data->elem_array[count - 1], temp_str, count);
                if(temp_str_black != NULL)
                {
                    data->elem_array[count] = temp_str_black;
                    // clear the data
                    data->elem_index[count].valid = 0;
                    data->elem_index[count].virtualsel = count;
                    data->elem_index[count].sel = data->elem_cnt - 1;

                    count++;
                    GxCore_Free(temp_str);
                    temp_str = app_epg_tmlist_elem_format(count, temp_index[i].day, s_event_info, &node);
                }
            }
            else if(NULL != temp_str)
            {
                temp_str_black = _timlist_get_null_item(NULL,temp_str,count);
                if(temp_str_black != NULL)
                {
                    data->elem_array[count] = temp_str_black;
                    // clear the data
                    data->elem_index[count].valid = 0;
                    data->elem_index[count].virtualsel = count;
                    data->elem_index[count].sel = data->elem_cnt - 1;

                    count++;
                    GxCore_Free(temp_str);
                    temp_str = app_epg_tmlist_elem_format(count, temp_index[i].day, s_event_info, &node);
                }
            }
#endif
            if(temp_str != NULL)
            {
                data->elem_array[count] = temp_str;
                temp_index[i].valid = 1;
                temp_index[i].virtualsel = count;
                memcpy(&data->elem_index[count],&temp_index[i],sizeof(EpgIndex));
                count++;
            }
        }
    }
    data->elem_array[count] = "<end>";
    data->elem_cnt = count;
    GxCore_Free(temp_index);
    temp_index = NULL;
    return GXCORE_SUCCESS;
}
#if 0
static bool app_epg_prog_map_change(void)
{
	bool ret = false;
	int i;

	for(i = 0; i < MAX_EPG_PROG; i++)
	{
		if(s_epg_prog_map_bak[i] != s_epg_prog_map[i])
		{
			ret = true;
			break;
		}
	}
	return ret;
}
#endif

static int app_epg_tm_update_timer(void* usrdata)
{
	time_t time_temp = 0;
	char *tm_str = NULL;
	char tm_set_str[8] = {0};
	char sys_time_str[24] = {0};
	char time_str_temp[24] = {0};
	int i;
	const char *wnd = NULL;
	
	g_AppTime.time_get(&g_AppTime, sys_time_str, sizeof(sys_time_str)); 
	// time
	if((strlen(sys_time_str) > 24) || (strlen(sys_time_str) < 5))
	{
		printf("\nlenth error\n");
		return 1;
	}
	memcpy(time_str_temp,&sys_time_str[strlen(sys_time_str)-5],5);// 00:00
	GUI_SetProperty(TEXT_EPG_SYS_TIME, "string", time_str_temp);

	// ymd
	memcpy(time_str_temp,sys_time_str,strlen(sys_time_str));// 0000/00/00
	time_str_temp[strlen(sys_time_str)-5] = '\0';
	GUI_SetProperty("text_epg_sys_ymd", "string", time_str_temp);

	wnd = GUI_GetFocusWindow();
	if(wnd == NULL
	    ||(0 != strcmp(wnd,"wnd_epg"))
		|| (s_epg_event_details_show == true))
		return 0;
		
	time_temp = g_AppTime.utc_get(&g_AppTime);
	if(time_temp >= s_next_evnt_time)
	{
		for(i = 0; i < MAX_EPG_PROG; i++)
		{
			sp_EpgOps[i].event_cnt_update(&sp_EpgOps[i]);
			s_epg_cnt_bak[i] = app_epg_dispaly_event_zone(i, s_epg_day, s_offset_sec, NULL);
			app_epg_tmlist_data_get(i, s_epg_day, s_offset_sec, &s_epg_dispaly_data[i]);
		}
		GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);
	}

	time_temp = get_display_time_by_timezone(time_temp);
	time_temp -= time_temp % SEC_PER_HOUR;

	
	if(time_temp != s_time_hour)
	{	
		s_time_hour = time_temp;

		time_temp -= time_temp % SEC_PER_DAY;
		if(s_time_date != time_temp)
		{
			s_time_date = time_temp;
			tm_str = app_time_to_date_edit_str(s_time_date);
			if(tm_str != NULL)
			{
				GUI_SetProperty(TEXT_EGP_DATE, "string", tm_str);
				GxCore_Free(tm_str);
				tm_str = NULL;
			}
			if(s_epg_day != 0)
			{
				s_epg_day--;
			}
		}
		
		if(s_epg_day == 0)
		{
			tm_str = app_time_to_hourmin_edit_str(s_time_hour);
			if(tm_str != NULL)
			{
				tm_set_str[0] = '<';
				memcpy(tm_set_str + 1, tm_str, 5);
				tm_set_str[6] = '>';
				tm_set_str[7] = '\0';
				GxCore_Free(tm_str);
				GUI_SetProperty(TIMELIST_EPG_EVENT, "start_time", tm_set_str);

				s_offset_sec = s_time_hour % SEC_PER_DAY;
				for(i = 0; i < MAX_EPG_PROG; i++)
				{
					sp_EpgOps[i].event_cnt_update(&sp_EpgOps[i]);
					s_epg_cnt_bak[i] = app_epg_dispaly_event_zone(i, s_epg_day, s_offset_sec, NULL);
					app_epg_tmlist_data_get(i, s_epg_day, s_offset_sec, &s_epg_dispaly_data[i]);
				}
				GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);
			}
		}	
	}
	
	return 0;
}


static int app_epg_zone_cnt_check_timer(void* usrdata)
{
	int i;
	int temp_cnt = 0; 
	bool change_flag = false;
	const char *wnd = NULL;
	
	wnd = GUI_GetFocusWindow();
	if((wnd == NULL)
		||(0 != strcmp(wnd,"wnd_epg"))
		|| (s_epg_event_details_show == true))
		return 0;
		
	for(i = 0; i < MAX_EPG_PROG; i++)
	{
		sp_EpgOps[i].event_cnt_update(&sp_EpgOps[i]);
		temp_cnt = app_epg_dispaly_event_zone(i, s_epg_day, s_offset_sec, NULL);
		if(s_epg_cnt_bak[i] != temp_cnt)
		{
			app_epg_tmlist_data_get(i, s_epg_day, s_offset_sec, &s_epg_dispaly_data[i]);
			s_epg_cnt_bak[i] = temp_cnt;
			change_flag = true;
		}
	}

	if(change_flag == true)
	{
		GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);
	}

	return 0;
}

static status_t app_epg_get_event_book(uint16_t prog_id, GxEpgInfo *event, GxBook *book_ret)
{
    GxBookGet bookGet;
    BookProgStruct book_prog;
    int count = 0;
    int i;
    status_t ret = GXCORE_ERROR;

    if((prog_id == 0) || (event == NULL) || (book_ret == NULL))
        return GXCORE_ERROR;

    count = g_AppBook.get(&bookGet);
    if(count == 0)
        return GXCORE_ERROR;

    for(i = 0; i < count; i++)
    {
        if((bookGet.book[i].book_type == BOOK_PROGRAM_PLAY)
                || (bookGet.book[i].book_type == BOOK_PROGRAM_PVR))
        {
            memcpy(&book_prog, (BookProgStruct*)bookGet.book[i].struct_buf, sizeof(BookProgStruct));
            if(prog_id == book_prog.prog_id)
            {
                if(bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE)
                {
                    if(bookGet.book[i].trigger_time_start == event->start_time)
                    {
                        memcpy(book_ret, bookGet.book + i, sizeof(GxBook));
                        ret = GXCORE_SUCCESS;
                    }
                }
                else
                {
                    time_t time_temp1;
                    time_t time_temp2;

                    time_temp1 = bookGet.book[i].trigger_time_start % SEC_PER_DAY;
                    time_temp2 = event->start_time % SEC_PER_DAY;

                    if(time_temp1 == time_temp2)
                    {
                        if(bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_EVERY_DAY)
                        {
                            memcpy(book_ret, bookGet.book + i, sizeof(GxBook));
                            ret = GXCORE_SUCCESS;
                        }
                        else
                        {
                            WeekDay book_wday;
                            struct tm *ptm;

                            ptm = localtime(&(event->start_time));
                            book_wday = g_AppBook.mode2wday(bookGet.book[i].repeat_mode.mode);
                            if(ptm->tm_wday == book_wday)
                            {
                                memcpy(book_ret, bookGet.book + i, sizeof(GxBook));
                                ret = GXCORE_SUCCESS;
                                //	printf("----book_type=%d, ret=%d-----\n", bookGet.book[i].book_type, ret);
                            }
                        }
                    }
                }
                if(ret == GXCORE_SUCCESS)
                {
                    break;
                }
            }
        }
    }

    return ret;
}

static void event_details_init(void)
{
    char prog_name[MAX_PROG_NAME + 1] = {0};
    const char *wnd_epg = NULL;
    char *poldProgName = NULL;
    wnd_epg = GUI_GetFocusWindow();
    if(0 == strcmp(wnd_epg,"wnd_epg"))
    {
        memcpy(prog_name, s_cur_prog.prog_data.prog_name, MAX_PROG_NAME);

        GUI_GetProperty("text_epg_event_prog_name", "string", &poldProgName);
        if ((poldProgName == NULL) || (strcmp(poldProgName, prog_name) != 0))
        {
            GUI_SetProperty("text_epg_event_prog_name","string",prog_name);
        }

        GUI_SetProperty("text_epg_event_details_startT1", "string", STR_BLANK);
        GUI_SetProperty("text_epg_event_details_interval1", "string", STR_BLANK);
        GUI_SetProperty("text_epg_event_details_endT1", "string", STR_BLANK);
        GUI_SetProperty("text_epg_event_event_name", "string", STR_BLANK);
        GUI_SetProperty("text_epg_event_parental_info", "string", STR_BLANK);
    }
}

static int event_details_show_timer(void* usrdata)
{
    char prog_name[MAX_PROG_NAME + 1] = {0};
    int val = 0;
    int sel = 0;
    time_t time = 0;
    char *temp_str = NULL;
	char *tm_str = NULL;
    const char *wnd_epg = NULL;
    char *poldProgName = NULL;
    char *poldEventName = NULL;
    GxBook prog_book;

    wnd_epg = GUI_GetFocusWindow();
    if(0 == strcmp(wnd_epg,"wnd_epg"))
    {
        memcpy(prog_name, s_cur_prog.prog_data.prog_name, MAX_PROG_NAME);
        GUI_GetProperty("text_epg_event_prog_name","string",&poldProgName);
        //	printf("==========poldProgName:%s, prog_name:%s\n", poldProgName, prog_name);
        if ((poldProgName==NULL) || (strcmp(poldProgName, prog_name) != 0))
        {
            GUI_SetProperty("text_epg_event_prog_name","string",prog_name);
        }

        // show event-name and time begin-end
        GUI_GetProperty(LISTVIEW_EPG_PROG, "cur_sel", &sel);
        if((s_epg_dispaly_data[sel].elem_index != NULL) && (s_focus_event != NULL))
        {
            GUI_GetProperty(TIMELIST_EPG_EVENT, "select_column", &val);
            memset(s_focus_event, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);

            if((val < s_epg_dispaly_data[sel].elem_cnt)
                    && (s_epg_dispaly_data[sel].elem_index[val].valid))
            {
                sp_EpgOps[sel].event_cnt_update(&sp_EpgOps[sel]);
                sp_EpgOps[sel].get_day_event_info(&sp_EpgOps[sel], &s_epg_dispaly_data[sel].elem_index[val], s_focus_event);
            }

            if(s_focus_event->event_name != NULL)
            {
                GUI_GetProperty("text_epg_event_event_name","string",&poldEventName);
                //	printf("==========poldEventName:%s, event_name:%s\n", poldEventName, s_focus_event->event_name);
                if ((poldEventName==NULL) || (strcmp(poldEventName, (char*)(s_focus_event->event_name)) != 0))
                {
                    //	printf("~~~~~~set new event name\n");
                    GUI_SetProperty("text_epg_event_event_name", "string", s_focus_event->event_name);
                }
                time = get_display_time_by_timezone(s_focus_event->start_time);
				if(time > (s_time_date + (s_epg_day *SEC_PER_DAY)))
				{
					tm_str = app_time_to_date_edit_str(time);
					if(tm_str != NULL)
					{
						GUI_SetProperty(TEXT_EGP_DATE, "string", tm_str);
						GxCore_Free(tm_str);
						tm_str = NULL;
					}
				}
				temp_str = app_time_to_hourmin_edit_str(time);
                if(temp_str != NULL)
                {
                    GUI_SetProperty("text_epg_event_details_startT1", "string", temp_str);
                    GxCore_Free(temp_str);
                    temp_str = NULL;
                }

                GUI_SetProperty("text_epg_event_details_interval1", "string", "--");

                time = get_display_time_by_timezone(s_focus_event->start_time + s_focus_event->duration);
                temp_str = app_time_to_hourmin_edit_str(time);

                if(temp_str != NULL)
                {
                    GUI_SetProperty("text_epg_event_details_endT1", "string", temp_str);
                    GxCore_Free(temp_str);
                    temp_str = NULL;
                }
				
                GUI_SetProperty("text_epg_book_tip", "state", "show");
                GUI_SetProperty("img_epg_red_tip", "state", "show");
                memset(&prog_book, 0, sizeof(GxBook));
                if(app_epg_get_event_book(s_cur_prog.prog_data.id, s_focus_event, &prog_book) == GXCORE_SUCCESS) //booked
                {
                    GUI_SetProperty("text_epg_book_tip", "string", STR_ID_DELETE);
                }
                else
                {
                    GUI_SetProperty("text_epg_book_tip", "string", STR_ID_BOOK);
                }
            }
            else
            {
                GUI_SetProperty("text_epg_event_details_startT1", "string", STR_BLANK);
                GUI_SetProperty("text_epg_event_details_interval1", "string", STR_BLANK);
                GUI_SetProperty("text_epg_event_details_endT1", "string", STR_BLANK);
                GUI_SetProperty("text_epg_event_event_name", "string", STR_ID_NO_INFO);
                GUI_SetProperty("text_epg_book_tip", "state", "hide");
                GUI_SetProperty("img_epg_red_tip", "state", "hide");
            }
        }
        else
        {
            GUI_SetProperty("text_epg_event_details_startT1", "string", STR_BLANK);
            GUI_SetProperty("text_epg_event_details_interval1", "string", STR_BLANK);
            GUI_SetProperty("text_epg_event_details_endT1", "string", STR_BLANK);
            GUI_SetProperty("text_epg_event_event_name", "string", STR_ID_NO_INFO);
            GUI_SetProperty("text_epg_book_tip", "state", "hide");
            GUI_SetProperty("img_epg_red_tip", "state", "hide");
        }
    }
    else
    {

    }
    return 0;
}

static void app_epg_event_details_show(void)
{
    time_t time = 0;
    char *temp_str = 0;

//    GUI_SetProperty(BTN_EPG_DETAILS_BACK, "state", "show");
    GUI_SetProperty(IMG_EPG_EVENT_DETAILS_BACK, "state", "show");
    GUI_SetProperty(NOTEPAD_EPG_EVENT, "state", "show");
    GUI_SetProperty(IMG_EPG_DETAILS_BACK, "state", "show");
    GUI_SetProperty(TEXT_EVENT_TITLE, "state", "show");
    GUI_SetProperty(TEXT_EVENT_TITLE, "reset_rolling", NULL);
    GUI_SetProperty(TEXT_EVENT_START_TIME, "state", "show");
    GUI_SetProperty(TEXT_EVENT_END_TIME, "state", "show");
    GUI_SetProperty(TEXT_EVENT_TIME_INTERVAL, "state", "show");
    GUI_SetProperty(BTN_EVNET_DETAILS_EXIT, "state", "show");

    GUI_SetProperty(IMG_EPG_PREV_DAY, "state", "hide");
    GUI_SetProperty(IMG_EPG_NEXT_DAY, "state", "hide");
    GUI_SetProperty(IMG_EPG_SHOW_DETAIL, "state", "hide");
    GUI_SetProperty(TEXT_EPG_PREV_DAY, "state", "hide");
    GUI_SetProperty(TEXT_EPG_NEXT_DAY, "state", "hide");
    GUI_SetProperty(TEXT_EPG_SHOW_DETAIL, "state", "hide");

    GUI_SetProperty("text_epg_book_tip", "state", "hide");
    GUI_SetProperty("img_epg_red_tip", "state", "hide");

    if(s_focus_event != NULL)
    {
        GUI_SetProperty(NOTEPAD_EPG_EVENT, "string", s_focus_event->event_detail);

        time = get_display_time_by_timezone(s_focus_event->start_time);
        temp_str = app_time_to_hourmin_edit_str(time);
        if(temp_str != NULL)
        {
            GUI_SetProperty(TEXT_EVENT_START_TIME, "string", temp_str);
            GxCore_Free(temp_str);
            temp_str = NULL;
        }

        time = get_display_time_by_timezone(s_focus_event->start_time + s_focus_event->duration);
        temp_str = app_time_to_hourmin_edit_str(time);
        if(temp_str != NULL)
        {
            GUI_SetProperty(TEXT_EVENT_END_TIME, "string", temp_str);
            GxCore_Free(temp_str);
            temp_str = NULL;
        }

        //GUI_SetProperty(TEXT_EVENT_TITLE, "state", "show");//for Bug#12756
        //GUI_SetProperty(TEXT_EVENT_TITLE, "rolling_stop", NULL);
        GUI_SetProperty(TEXT_EVENT_TITLE, "string", s_focus_event->event_name);
    }
	GUI_SetFocusWidget(BTN_EVNET_DETAILS_EXIT);
}

static void app_epg_event_details_hide(void)
{
    GUI_SetProperty(TEXT_EVENT_TITLE, "rolling_stop", NULL);
	GUI_SetProperty(TEXT_EVENT_TITLE, "string", NULL);
	GUI_SetProperty(TEXT_EVENT_TITLE, "state", "hide");
//	GUI_SetProperty(BTN_EPG_DETAILS_BACK, "state", "hide");
	GUI_SetProperty(IMG_EPG_EVENT_DETAILS_BACK, "state", "hide");
	GUI_SetProperty(NOTEPAD_EPG_EVENT, "state", "hide");
	GUI_SetProperty(IMG_EPG_DETAILS_BACK, "state", "hide");
	GUI_SetProperty(TEXT_EVENT_START_TIME, "string", NULL);
	GUI_SetProperty(TEXT_EVENT_START_TIME, "state", "hide");
	GUI_SetProperty(TEXT_EVENT_END_TIME, "string", NULL);
	GUI_SetProperty(TEXT_EVENT_END_TIME, "state", "hide");
	GUI_SetProperty(TEXT_EVENT_TIME_INTERVAL, "state", "hide");
	GUI_SetProperty(BTN_EVNET_DETAILS_EXIT, "state", "hide");

	GUI_SetProperty(IMG_EPG_PREV_DAY, "state", "show");
	GUI_SetProperty(IMG_EPG_NEXT_DAY, "state", "show");
	GUI_SetProperty(IMG_EPG_SHOW_DETAIL, "state", "show");
	GUI_SetProperty(TEXT_EPG_PREV_DAY, "state", "show");
	GUI_SetProperty(TEXT_EPG_NEXT_DAY, "state", "show");
	GUI_SetProperty(TEXT_EPG_SHOW_DETAIL, "state", "show");

    GUI_SetProperty("text_epg_book_tip", "state", "show");
    GUI_SetProperty("img_epg_red_tip", "state", "show");


	GUI_SetFocusWidget(TIMELIST_EPG_EVENT);

	GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);
}

static void app_epg_details_keypress(uint16_t key)
{
	uint32_t value = 1;

	switch(key)
	{
		case STBK_UP:
			GUI_SetProperty(NOTEPAD_EPG_EVENT, "line_up", &value);
			break;

		case STBK_DOWN:
			GUI_SetProperty(NOTEPAD_EPG_EVENT, "line_down", &value);
			break;

		case STBK_PAGE_UP:
			GUI_SetProperty(NOTEPAD_EPG_EVENT, "page_up", &value);
			break;

		case STBK_PAGE_DOWN:
			GUI_SetProperty(NOTEPAD_EPG_EVENT, "page_down", &value);
			break;
		default:
			break;
	}
}

#if (DLNA_SUPPORT > 0)
static void app_epg_start_dlna_cb(void)
{
    g_AppFullArb.timer_stop();

    if(s_tm_update_timer != NULL)
    {
        remove_timer(s_tm_update_timer);
        s_tm_update_timer = NULL;
    }

    if(s_get_epg_timer != NULL)
    {
        remove_timer(s_get_epg_timer);
        s_get_epg_timer = NULL;
    }

    if(s_details_show_timer != NULL)
    {
        remove_timer(s_details_show_timer );
        s_details_show_timer = NULL;
    }

    g_AppPlayOps.program_stop();
    EPG_START_FULL_VIDEO();
}

static void app_epg_stop_dlna_cb(void)
{
    app_epg_tm_update_timer(NULL);
    if(0 != reset_timer(s_tm_update_timer))
    {
        s_tm_update_timer = create_timer(app_epg_tm_update_timer, 800, NULL, TIMER_REPEAT);
    }

    if(0 != reset_timer(s_get_epg_timer))
    {
        s_get_epg_timer = create_timer(app_epg_zone_cnt_check_timer, 1000, NULL, TIMER_REPEAT);
    }

    //fresh event information
    event_details_show_timer(NULL);
    if(0 != reset_timer(s_details_show_timer))
    {
        s_details_show_timer = create_timer(event_details_show_timer, 500, NULL, TIMER_REPEAT);
    }

    EPG_START_VIDEO();
    g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    g_AppFullArb.timer_start();
}
#endif

status_t app_create_epg_menu(void)
{

	GxPlayer_MediaVideoHide(PLAYER_FOR_NORMAL);
	int i;

    if(s_event_info != NULL)
	{
		GxCore_Free(s_event_info);
		s_event_info = NULL;
	}
	s_event_info = (GxEpgInfo *)GxCore_Calloc(1, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
	if(s_event_info == NULL)
	{
		return GXCORE_ERROR;
	}

	for(i = 0; i < MAX_EPG_PROG; i++)
	{
		app_epg_tmlist_data_init(&s_epg_dispaly_data[i]);
	}
    app_subt_pause();
	app_create_dialog("wnd_epg");
	g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);

	return GXCORE_SUCCESS;
}
#if 0
status_t app_epg_record_status_running(void)
{
    int sel = -1;

    sel = g_AppPlayOps.normal_play.play_count;
    memset(&s_cur_prog, 0, sizeof(GxMsgProperty_NodeByPosGet));
    s_cur_prog.node_type = NODE_PROG;
    s_cur_prog.pos = sel;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_cur_prog);

    if(s_epg_event_details_show == false)
    {
        GUI_SetProperty(LISTVIEW_EPG_PROG, "select", &sel);
        GUI_SetProperty(LISTVIEW_EPG_PROG, "update_all", NULL);
        GUI_SetProperty(LISTVIEW_EPG_PROG, "active", &sel);
    }

    return GXCORE_SUCCESS;
}
#endif

SIGNAL_HANDLER int app_epg_create(GuiWidget *widget, void *usrdata)
{
    int i;
    int sel;
	time_t offset_sec = 0;
	char *tm_str = NULL;
	char tm_set_str[8] = {0};


	for(i = 0; i < MAX_EPG_PROG; i++)
	{
		epg_ops_init(&sp_EpgOps[i]);
	}
    s_focus_event = (GxEpgInfo *)GxCore_Calloc(1, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
	memset(s_epg_cnt_bak, 0, sizeof(int) * MAX_EPG_PROG);
	memset(s_epg_prog_map_bak, 0, sizeof(int) * MAX_EPG_PROG);
	s_epg_day = 0;
	s_time_hour = 24;
	s_time_date = (time_t)~0;
	s_offset_sec = 0;

	GUI_SetFocusWidget(TIMELIST_EPG_EVENT);
	app_lang_set_menu_font_type("text_epg_titile");
	sel = g_AppPlayOps.normal_play.play_count;
	if(sel == 0)
	{
		app_epg_prog_list_change(NULL, NULL);
	}
	else
	{
        GUI_SetProperty(LISTVIEW_EPG_PROG, "select", &sel);
    }

    memset(&s_cur_prog, 0, sizeof(GxMsgProperty_NodeByPosGet));
    s_cur_prog.node_type = NODE_PROG;
    s_cur_prog.pos = sel;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_cur_prog);

    tm_str = app_time_to_hourmin_edit_str(offset_sec);
	if(tm_str != NULL)
	{
		tm_set_str[0] = '<';
		memcpy(tm_set_str + 1, tm_str, 5);
		tm_set_str[6] = '>';
		tm_set_str[7] = '\0';
		GxCore_Free(tm_str);
		GUI_SetProperty(TIMELIST_EPG_EVENT, "start_time", tm_set_str);
	}

	app_epg_tm_update_timer(NULL);
	if(0 != reset_timer(s_tm_update_timer))
	{
		s_tm_update_timer = create_timer(app_epg_tm_update_timer, 800, NULL, TIMER_REPEAT);
	}

	if(0 != reset_timer(s_get_epg_timer))
	{
		s_get_epg_timer = create_timer(app_epg_zone_cnt_check_timer, 1000, NULL, TIMER_REPEAT);
	}

	//fresh event information
	event_details_show_timer(NULL);
	if(0 != reset_timer(s_details_show_timer))
	{
		s_details_show_timer = create_timer(event_details_show_timer, 500, NULL, TIMER_REPEAT);
	}

#ifndef RADIO_REC_BOOK
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		GUI_SetProperty("img_epg_red_tip", "state", "hide");
		GUI_SetProperty("text_epg_book_tip", "state", "hide");
	}
#endif
	EPG_START_VIDEO();

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_destroy(GuiWidget *widget, void *usrdata)
{
    int i;

    if(s_tm_update_timer != NULL)
    {
        remove_timer(s_tm_update_timer);
        s_tm_update_timer = NULL;
    }

    if(s_get_epg_timer != NULL)
    {
        remove_timer(s_get_epg_timer);
        s_get_epg_timer = NULL;
    }

    if(s_details_show_timer != NULL)
    {
        remove_timer(s_details_show_timer );
        s_details_show_timer = NULL;
    }


    if(s_event_info != NULL)
    {
        GxCore_Free(s_event_info);
        s_event_info = NULL;
    }

    if(s_focus_event != NULL)
    {
        GxCore_Free(s_focus_event);
        s_focus_event = NULL;
    }

    for(i = 0; i < MAX_EPG_PROG; i++)
    {
        app_epg_tmlist_data_release(&s_epg_dispaly_data[i]);
    }

    s_epg_event_details_show = false;
	GxPlayer_MediaVideoHide(PLAYER_FOR_NORMAL);
	FullArb *arb = &g_AppFullArb;
	arb->state.pause = STATE_OFF;

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_video_show(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_got_focus(GuiWidget *widget, void *usrdata)
{
	app_epg_event_timer_set(false);
    app_epg_timer_edit_show_time();
    if(s_epg_event_details_show == false)
    {
        GUI_SetProperty("text_epg_event_prog_name", "rolling_reset", NULL);
        GUI_SetProperty("text_epg_event_event_name", "rolling_reset", NULL);

        if(0 != reset_timer(s_details_show_timer))
        {
            s_details_show_timer = create_timer(event_details_show_timer, 500, NULL, TIMER_REPEAT);
        }

        GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);
    }
    else
    {
    }
    if(g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_RADIO)
    {
        //GUI_SetInterface("video_top", NULL);
        //GUI_SetInterface("image_disable", NULL);
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_lost_focus(GuiWidget *widget, void *usrdata)
{
    timer_stop(s_details_show_timer);
    GUI_SetProperty("text_epg_event_prog_name", "rolling_stop", NULL);
    GUI_SetProperty("text_epg_event_event_name", "rolling_stop", NULL);

    if(g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_RADIO)
    {
        //GUI_SetInterface("image_top", NULL);
        //GUI_SetInterface("image_enable", NULL);
    }
    return EVENT_TRANSFER_STOP;
}

#if 0
static int _epg_record_stop_popcb(PopDlgRet ret)
{
    int sel = -1;
    if(ret == POP_VAL_OK)
    {
        app_pvr_stop();

        sel = g_AppPlayOps.normal_play.play_count;

        memset(&s_cur_prog, 0, sizeof(GxMsgProperty_NodeByPosGet));
        s_cur_prog.node_type = NODE_PROG;
        s_cur_prog.pos = sel;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_cur_prog);

        if(s_epg_event_details_show == false)
        {
            GUI_SetProperty(LISTVIEW_EPG_PROG, "update_all", NULL);
            GUI_SetProperty(LISTVIEW_EPG_PROG, "select", &sel);
            GUI_SetProperty(LISTVIEW_EPG_PROG, "active", &sel);
        }
    }

    return 0;
}
#endif
SIGNAL_HANDLER int app_epg_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    time_t offset_sec = 0;
    GxTime local_time;
    char *tm_str = NULL;
    char tm_set_str[8] = {0};
    int i;
    int cur_prog_sel = 0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_EXIT:
                    {
#if 0
                        if(GUI_CheckPrompt("prompt_timer_full") == GXCORE_SUCCESS)
                        {
                            GUI_EndPrompt("prompt_timer_full");
                        }
#endif
                        if(s_epg_event_details_show == true)
                        {
                            app_epg_event_details_hide();
                            s_epg_event_details_show = false;

                            GUI_SetProperty(LISTVIEW_EPG_PROG, "select", &g_AppPlayOps.normal_play.play_count);
                            GUI_SetProperty(LISTVIEW_EPG_PROG, "active", &g_AppPlayOps.normal_play.play_count);

                            GUI_SetProperty("text_epg_event_prog_name", "rolling_reset", NULL);
                            GUI_SetProperty("text_epg_event_event_name", "rolling_reset", NULL);

                            if(0 != reset_timer(s_details_show_timer))
                            {
                                s_details_show_timer = create_timer(event_details_show_timer, 500, NULL, TIMER_REPEAT);
                            }
                        }
                        else
                        {
                            GUI_EndDialog(WND_EPG);
                            GUI_SetInterface("flush",NULL);
							#if (MINI_16BIT_WIN8_OSD_SUPPORT)
							if(app_main_menu_create_flag()==0)
							{
								app_create_dialog("wnd_channel_info");
                            	EPG_START_FULL_VIDEO();
							}	
							#else
                            app_create_dialog("wnd_channel_info");
                            EPG_START_FULL_VIDEO();
							#endif
                            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                            app_subt_change();
                        }

                        ret = EVENT_TRANSFER_STOP;
                        break;
                    }

                case STBK_MENU:
                    {
#if 0
                        if(GUI_CheckPrompt("prompt_timer_full") == GXCORE_SUCCESS)
                        {
                            GUI_EndPrompt("prompt_timer_full");
                        }
#endif
						#if MINI_16BIT_WIN8_OSD_SUPPORT
						
						GUI_EndDialog(WND_EPG);
	                    GUI_SetInterface("flush",NULL);
						if(app_main_menu_create_flag()==0)
						{
	                        app_create_dialog("wnd_channel_info");
	                        EPG_START_FULL_VIDEO();
						}
						
						g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
	                    app_subt_change();
	                    ret = EVENT_TRANSFER_STOP;
						#else
                        GUI_EndDialog(WND_EPG);
                        GUI_SetInterface("flush",NULL);
                        app_create_dialog("wnd_channel_info");
						
                        EPG_START_FULL_VIDEO();
                        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                        app_subt_change();
                        ret = EVENT_TRANSFER_STOP;
						#endif	
                        break;
                    }

                case VK_BOOK_TRIGGER:
                    {
#if 0
                        if(GUI_CheckPrompt("prompt_timer_full") == GXCORE_SUCCESS)
                        {
                            GUI_EndPrompt("prompt_timer_full");
                        }
#endif
                        //GUI_EndDialog(WND_EPG);
                        //GUI_SetInterface("flush",NULL);
						g_AppPlayOps.program_stop();
                        GUI_EndDialog("after wnd_full_screen");
						PlayerWindow video_wnd = {ZOOM_FULL_X, ZOOM_FULL_Y, ZOOM_FULL_W, ZOOM_FULL_H};
						GUI_SetInterface("flush", NULL);
						g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
						//EPG_START_FULL_VIDEO();
						app_subt_change();
						GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
                        ret = EVENT_TRANSFER_STOP;
                        break;
                    }

#if (DLNA_SUPPORT > 0)
                case VK_DLNA_TRIGGER:
                    app_dlna_ui_exec(app_epg_start_dlna_cb, app_epg_stop_dlna_cb);
                    break;
#endif
                case STBK_OK:
                    {
                        int val = 0;
                        int sel = 0;

                        if(s_epg_event_details_show == false)
                        {
                            if(s_focus_event->event_name != NULL)
                            {
                                memset(s_focus_event, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
                                GUI_GetProperty(LISTVIEW_EPG_PROG, "cur_sel", &sel);
                                if(s_epg_dispaly_data[sel].elem_index != NULL)
                                {
                                    GUI_GetProperty(TIMELIST_EPG_EVENT, "select_column", &val);
                                    if((val < s_epg_dispaly_data[sel].elem_cnt)
                                            && (s_epg_dispaly_data[sel].elem_index[val].valid))
                                    {
                                        sp_EpgOps[sel].event_cnt_update(&sp_EpgOps[sel]);
                                        s_epg_event_details_show = true;
                                        if(sp_EpgOps[sel].get_day_event_info(&sp_EpgOps[sel], &s_epg_dispaly_data[sel].elem_index[val], s_focus_event) == GXCORE_SUCCESS)
                                        {
                                            timer_stop(s_details_show_timer);
                                            GUI_SetProperty("text_epg_event_prog_name", "rolling_stop", NULL);
                                            GUI_SetProperty("text_epg_event_event_name", "rolling_stop", NULL);

                                            app_epg_event_details_show();
                                        }
                                        else
                                        {
                                            s_epg_event_details_show = false;
                                        }
                                    }
                                }
                            }
                        }
                        ret = EVENT_TRANSFER_STOP;
                        break;
                    }


                case STBK_LEFT:
                case STBK_RIGHT:
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_UP:
                case STBK_DOWN:
                case STBK_PAGE_UP:
                case STBK_PAGE_DOWN:
                    if(s_epg_event_details_show == true)
                    {
                        app_epg_details_keypress(find_virtualkey_ex(event->key.scancode,event->key.sym));
                    }
                    else
                    {
                        //GUI_SendEvent(LISTVIEW_EPG_PROG, event);
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_RED:
                    {
#ifndef RADIO_REC_BOOK
                        if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
                        {
                            break;
                        }
#endif
                        int val = 0;
                        int sel = 0;

                        GxBookGet book_Get = {0};
                        if(s_epg_event_details_show == false)
                        {
                            if((s_focus_event != NULL) && (s_focus_event->event_name != NULL))
                            {
                                memset(s_focus_event, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
                                GUI_GetProperty(LISTVIEW_EPG_PROG, "cur_sel", &sel);
                                if(s_epg_dispaly_data[sel].elem_index != NULL)
                                {
                                    GUI_GetProperty(TIMELIST_EPG_EVENT, "select_column", &val);
                                    if(val < s_epg_dispaly_data[sel].elem_cnt)
                                    {
                                        sp_EpgOps[sel].event_cnt_update(&sp_EpgOps[sel]);
                                        if(sp_EpgOps[sel].get_day_event_info(&sp_EpgOps[sel], &s_epg_dispaly_data[sel].elem_index[val], s_focus_event) == GXCORE_SUCCESS)
                                        {
                                            GxBook book;

                                            g_AppFullArb.timer_stop();
                                            memset(&book, 0, sizeof(GxBook));
                                            if(app_epg_get_event_book(s_cur_prog.prog_data.id, s_focus_event, &book) == GXCORE_ERROR) //
                                            {
												if(g_AppBook.get(&book_Get) >= APP_BOOK_NUM)
												{
													PopDlg  pop;
													memset(&pop, 0, sizeof(PopDlg));
													pop.type = POP_TYPE_NO_BTN;
													pop.str = STR_ID_TIMER_FULL;
													pop.mode = POP_MODE_UNBLOCK;
													pop.pos.x = EPG_POPDLG_POS_X;
													pop.pos.y = EPG_POPDLG_POS_Y;
													pop.timeout_sec = 3;
													popdlg_create(&pop) ;

													ret = EVENT_TRANSFER_STOP;
													break;
												}

                                                GUI_SetProperty("txt_epg_signal", "state", "show");
												GUI_SetProperty("txt_epg_signal","string"," ");
                                                app_epg_timer_edit_hide_time();
                                                app_player_close(PLAYER_FOR_NORMAL);
                                                app_epg_timer_menu_exec(&s_cur_prog.prog_data, s_focus_event);
												if(GUI_CheckDialog("wnd_epg") != GXCORE_SUCCESS)
												{
													break;
												}
												EPG_START_VIDEO();
												g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);

											}
                                            else//booked, remove book
                                            {
                                                PopDlg  pop;
                                                memset(&pop, 0, sizeof(PopDlg));
                                                pop.type = POP_TYPE_YES_NO;
                                                pop.str = STR_ID_SURE_DELETE;
                                                pop.pos.x = EPG_POPDLG_POS_X;
                                                pop.pos.y = EPG_POPDLG_POS_Y;

                                                if(popdlg_create(&pop) == POP_VAL_OK)
                                                {
                                                    g_AppBook.remove(&book);
                                                    GUI_SetProperty("text_epg_book_tip", "string", STR_ID_BOOK);
                                                }
												if(g_AppFullArb.state.tv == STATE_ON && g_AppFullArb.tip == FULL_STATE_DUMMY)
												{
													GUI_SetProperty("txt_epg_signal", "state", "hide");
												}
                                            }
                                            g_AppFullArb.timer_start();
                                        }
                                    }
                                }
                            }
                        }
                        // update the color of the event , after press red
                        GUI_GetProperty(LISTVIEW_EPG_PROG, "cur_sel", &cur_prog_sel);
                        sp_EpgOps[cur_prog_sel].event_cnt_update(&sp_EpgOps[cur_prog_sel]);
                        s_epg_cnt_bak[cur_prog_sel] = app_epg_dispaly_event_zone(cur_prog_sel, s_epg_day, s_offset_sec, NULL);
                        app_epg_tmlist_data_get(cur_prog_sel, s_epg_day, s_offset_sec, &s_epg_dispaly_data[cur_prog_sel]);

                        ret = EVENT_TRANSFER_STOP;
                        break;
                    }

                case STBK_GREEN:
                case STBK_YELLOW:
                    {
                        int sel = 0;
                        uint32_t current_sel;
                        if(s_epg_event_details_show == false)
                        {
                            int key = find_virtualkey_ex(event->key.scancode,event->key.sym);
                            if(key == STBK_YELLOW)
                            {
                                (s_epg_day == MAX_EPG_DAY - 1) ? (s_epg_day = 0) : (s_epg_day++);
                            }
                            else if(key == STBK_GREEN)
                            {
                                (s_epg_day == 0) ? (s_epg_day =  MAX_EPG_DAY - 1) : (s_epg_day--);
                            }

                            tm_str = app_time_to_date_edit_str(s_time_date + (s_epg_day *SEC_PER_DAY));
                            if(tm_str != NULL)
                            {
                                GUI_SetProperty(TEXT_EGP_DATE, "string", tm_str);
                                GxCore_Free(tm_str);
                                tm_str = NULL;
                            }

                            GUI_GetProperty(TIMELIST_EPG_EVENT, "select_row", &sel);
                            if(s_epg_day != 0)
                            {
                                GUI_SetProperty(TIMELIST_EPG_EVENT, "start_time", "<00:00>");
                                offset_sec = 0;
                            }
                            else
                            {
                                GxCore_GetLocalTime(&local_time);
                                offset_sec = get_display_time_by_timezone(local_time.seconds);
                                offset_sec %= SEC_PER_DAY;
                                offset_sec -= offset_sec % SEC_PER_HOUR;

                                tm_str = app_time_to_hourmin_edit_str(offset_sec);
                                if(tm_str != NULL)
                                {
                                    tm_set_str[0] = '<';
                                    memcpy(tm_set_str + 1, tm_str, 5);
                                    tm_set_str[6] = '>';
                                    tm_set_str[7] = '\0';
                                    GxCore_Free(tm_str);
                                    GUI_SetProperty(TIMELIST_EPG_EVENT, "start_time", tm_set_str);
                                }
                            }
                            s_offset_sec = offset_sec;

                            for(i = 0; i < MAX_EPG_PROG; i++)
                            {
                                sp_EpgOps[i].event_cnt_update(&sp_EpgOps[i]);
                                s_epg_cnt_bak[i] = app_epg_dispaly_event_zone(i, s_epg_day, s_offset_sec, NULL);
                                app_epg_tmlist_data_get(i, s_epg_day, s_offset_sec, &s_epg_dispaly_data[i]);
                            }

                            current_sel = 0;//Force to the first item of Tmlist
                            GUI_SetProperty(s_egp_event_row[sel],"current_item",&current_sel);
                            GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);
                        }
                        ret = EVENT_TRANSFER_STOP;
                        break;
                    }
#if 0
                case STBK_REC_START:
                    if(g_AppPvrOps.state == PVR_DUMMY)
                    {
                        extern void app_set_avcodec_flag(bool state);
                        PopDlg pop;
                        usb_check_state usb_state = USB_OK;
                        GxMsgProperty_NodeByPosGet node_prog = {0};
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
                            pop.pos.x = EPG_POPDLG_POS_X + 100;
                            pop.pos.y = EPG_POPDLG_POS_Y;

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
                            pop.pos.x = EPG_POPDLG_POS_X;
                            pop.pos.y = EPG_POPDLG_POS_Y;

                            popdlg_create(&pop);

                            break;
                        }

                        if(app_check_av_running() == true)
                        {
                            app_pvr_rec_start(0);
                            break;
                        }
                        else
                        {
                            // prog
                            node_prog.node_type = NODE_PROG;
                            node_prog.pos = g_AppPlayOps.normal_play.play_count;
                            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
                            if(node_prog.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE)
                            {
                                AppFrontend_LockState lock;
                                app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);
                                g_AppPlayOps.set_rec_time(0);
                                g_AppPlayOps.program_play(PLAY_MODE_WITH_REC, g_AppPlayOps.normal_play.play_count);
                                break;
                            }
                        }

                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.format = POP_FORMAT_DLG;
                        pop.str = STR_ID_CANT_PVR;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.creat_cb = NULL;
                        pop.exit_cb = NULL;
                        pop.timeout_sec= 3;
                        pop.pos.x = EPG_POPDLG_POS_X;
                        pop.pos.y = EPG_POPDLG_POS_Y;

                        popdlg_create(&pop);
                    }

                    break;

                case STBK_REC_STOP:
                    {
                        PopDlg  pop;
                        if(g_AppPvrOps.state != PVR_RECORD)
                            break;

                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_YES_NO;
                        pop.format = POP_FORMAT_DLG;
                        pop.str = STR_ID_STOP_REC_INFO;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.exit_cb  = _epg_record_stop_popcb;
                        pop.pos.x = EPG_POPDLG_POS_X;
                        pop.pos.y = EPG_POPDLG_POS_Y;

                        popdlg_create(&pop);
                        break;
                    }

                    break;
#endif

                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_epg_list_list_keypress(GuiWidget *widget, void *usrdata)
{
        int ret = EVENT_TRANSFER_KEEPON;
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
				case STBK_PAGE_UP:
					{
						uint32_t sel;
						GUI_GetProperty(LISTVIEW_EPG_PROG, "select", &sel);
						if (sel == 0)
						{
							sel = g_AppPlayOps.normal_play.play_total - 1;
							GUI_SetProperty(LISTVIEW_EPG_PROG, "select", &sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							event->key.sym = STBK_PAGE_UP;
							ret = EVENT_TRANSFER_KEEPON;
						}

					}
					break;

				case STBK_PAGE_DOWN:
					{
						uint32_t sel;
						GUI_GetProperty(LISTVIEW_EPG_PROG, "select", &sel);
						if (sel+1 == g_AppPlayOps.normal_play.play_total)
						{
							sel = 0;
							GUI_SetProperty(LISTVIEW_EPG_PROG, "select", &sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							event->key.sym = STBK_PAGE_DOWN;
							ret = EVENT_TRANSFER_KEEPON;
						}

					}
					break;
				case STBK_DOWN:
					ret = EVENT_TRANSFER_KEEPON;
					break;
				case STBK_UP:
					ret = EVENT_TRANSFER_KEEPON;
					break;
				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

uint32_t app_epg_get_cur_prog_tp_id(void)
{
	GxMsgProperty_NodeByPosGet cur_tp_node = {0};
	
	cur_tp_node.node_type = NODE_PROG;
	cur_tp_node.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&cur_tp_node));

	return cur_tp_node.prog_data.tp_id;
}

SIGNAL_HANDLER int app_epg_prog_list_get_total(GuiWidget *widget, void *usrdata)
{
	return g_AppPlayOps.normal_play.play_total;
}

SIGNAL_HANDLER int app_epg_prog_list_get_data(GuiWidget *widget, void *usrdata)
{
#define PROG_COUNT_LEN  (8)
	#if TKGS_SUPPORT
	AppProgExtInfo prog_ext_info = {{0}};
	#endif
    ListItemPara* item = NULL;
    static GxMsgProperty_NodeByIdGet node;
    static char prog_count[PROG_COUNT_LEN];
    static char prog_name[MAX_PROG_NAME + 1];

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

	node.node_type = NODE_PROG;
	node.pos = item->sel;
	//get node
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

    //col-0: num
    item->x_offset = 0;
    item->image = NULL;
    memset(prog_count, 0, PROG_COUNT_LEN);
#if TKGS_SUPPORT
		if(app_tkgs_get_operatemode() != GX_TKGS_OFF)
		{
			if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
			{
				sprintf(prog_count, "%d", prog_ext_info.tkgs.logicnum);
			}
			else
			{
				sprintf(prog_count, " %d", (item->sel+1));
			}
		}
		else
		{
			sprintf(prog_count, " %d", (item->sel+1));
		}
#else
		sprintf(prog_count, " %d", (item->sel+1));
#endif
	item->string = prog_count;

    //col-1: channel name
    item = item->next;
    if (item == NULL)
        return GXCORE_ERROR;
    item->x_offset = 0;
    item->image = NULL;
	
    memset(prog_name, 0, MAX_PROG_NAME + 1);
    memcpy(prog_name, node.prog_data.prog_name, MAX_PROG_NAME);
    item->string = prog_name;

    //col-2: scramble
    item = item->next;
    if (item == NULL)
        return GXCORE_ERROR;
    item->x_offset = 0;
    item->string = NULL;
    if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
        item->image = "s_icon_money.bmp";
    }
    else
    {
        item->image = NULL;
    }

    //col-3: lock
    item = item->next;
    if (item == NULL)
        return GXCORE_ERROR;
    item->x_offset = 0;
    item->string = NULL;
    item->image = NULL;

    if (g_AppPvrOps.state == PVR_RECORD && node.prog_data.id == g_AppPvrOps.env.prog_id)
    {
        //record
        item->image = "s_ts_red.bmp";
    }
    else
    {
        if (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
        {
            //lock
            item->image = "s_icon_lock.bmp";
        }
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_prog_list_change(GuiWidget *widget, void *usrdata)
{
    int sel = 0;
	int cur_sel = 0;
    GUI_GetProperty(LISTVIEW_EPG_PROG, "select", &sel);
	GUI_GetProperty(LISTVIEW_EPG_PROG, "cur_sel", &cur_sel);
    if(sel != g_AppPlayOps.normal_play.play_count)
    {
        memset(&s_cur_prog, 0, sizeof(GxMsgProperty_NodeByPosGet));
        s_cur_prog.node_type = NODE_PROG;
        s_cur_prog.pos = sel;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_cur_prog);

        event_details_init();
        g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
        g_AppPlayOps.program_play(PLAY_TYPE_NORMAL, sel);

    }
	app_epg_tmlist_info_update(sel,cur_sel);
    GUI_SetProperty(LISTVIEW_EPG_PROG, "active", &sel);
//	GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);
	GUI_SetProperty(TIMELIST_EPG_EVENT, "select_row", s_egp_event_row[cur_sel]);
	GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);

	
    return EVENT_TRANSFER_STOP;
}



SIGNAL_HANDLER int app_epg_tmlist_get_data(GuiWidget *widget, void *usrdata)
{
    TimeItemPara* time = NULL;
    static int sel = 0;
    static int index;
    char **pstr = NULL;
    time = (TimeItemPara *)usrdata;

    if(time->from_start)
    {
        sel = time->sel;
        index = 0;
    }
    if((time->sel >= MAX_EPG_PROG)
            || (s_epg_dispaly_data[time->sel].elem_array == NULL))
    {
        return GXCORE_ERROR;
    }
    pstr = s_epg_dispaly_data[time->sel].elem_array;
    time->string = pstr[index];
    index++;

    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int app_epg_tmlist_change(GuiWidget *widget, void *usrdata)
{
	char tm_str[8] = {0};
	time_t offset_sec = 0;
	char *move_direct = NULL;
	char *date_str = NULL;
	int i;
	time_t time = 0;
	GUI_GetProperty(TIMELIST_EPG_EVENT, "start", tm_str);
	tm_str[6] = '\0';
	offset_sec = app_edit_str_to_hourmin_sec(tm_str + 1);
	offset_sec %= SEC_PER_DAY;
	s_offset_sec = offset_sec;

	move_direct = (char*)usrdata;
	if((offset_sec == 0)&&(strcmp(move_direct, "right") == 0))
	{
		(s_epg_day == MAX_EPG_DAY - 1) ? (s_epg_day = 0) : (s_epg_day++);
		GUI_SetProperty(TIMELIST_EPG_EVENT, "start_time", "<00:00>"); //
	}
	else if((offset_sec == 23*SEC_PER_HOUR)&&(strcmp(move_direct, "left") == 0))
	{
		(s_epg_day == 0) ? (s_epg_day =  MAX_EPG_DAY - 1) : (s_epg_day--);
		GUI_SetProperty(TIMELIST_EPG_EVENT, "start_time", "<23:00>");//
	}
	time = get_display_time_by_timezone(s_focus_event->start_time);
	if(time < (s_time_date + (s_epg_day *SEC_PER_DAY)))
	{
		date_str = app_time_to_date_edit_str(s_time_date + (s_epg_day *SEC_PER_DAY));
		if(date_str != NULL)
		{
			//printf("FILE === %s LINE====%d date_str===%s~~~~~~~~~\n",__FILE__,__LINE__,date_str);
			GUI_SetProperty(TEXT_EGP_DATE, "string", date_str);
			GxCore_Free(date_str);
			date_str = NULL;
		}
	}

	for(i = 0; i < MAX_EPG_PROG; i++)
	{
		sp_EpgOps[i].event_cnt_update(&sp_EpgOps[i]);
		s_epg_cnt_bak[i] = app_epg_dispaly_event_zone(i, s_epg_day, offset_sec, NULL);
		app_epg_tmlist_data_get(i, s_epg_day, offset_sec, &s_epg_dispaly_data[i]);
	}
	return 0;
}

SIGNAL_HANDLER int app_epg_tmlist_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_UP:
                case STBK_DOWN:
                case STBK_PAGE_UP:
                case STBK_PAGE_DOWN:
                    if(s_epg_event_details_show == false)
                    {
                        GUI_SendEvent(LISTVIEW_EPG_PROG, event);
                        g_AppFullArb.state.pause = STATE_OFF;
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

