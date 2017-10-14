/*
 * =====================================================================================
 *
 *       Filename:  app_channel_edit.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2011年07月04日 15时06分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "channel_edit.h"
#include "app_pop.h"
#include "app_default_params.h"
#include "full_screen.h"    /* only for signal state  */
#include "app_book.h"
#include "app_keyboard_language.h"

#if TKGS_SUPPORT
#include "module/gxtkgs.h"
extern void app_tkgs_cantwork_popup(void);
GxTkgsOperatingMode app_tkgs_get_operatemode(void);
static uint32_t selbackuppos=0xffffffff;
#endif

SIGNAL_HANDLER int app_pop_fav_list_get_total(GuiWidget *widget, void *usrdata);
#define WND_CH_ED           "wnd_channel_edit"
#define WND_POP_FAV         "wnd_pop_fav"
#define WND_POP_SORT        "wnd_pop_sort"
#define LISTVIEW_CH_ED      "listview_channel_edit"
#define LISTVIEW_POP_FAV    "listview_pop_fav"
#define GROUP_CH_ED         "cmb_channel_edit_group"
#define TXT_SAT_INFO        "txt_channel_edit_sat_info"
#define TXT_PROG_INFO       "txt_channel_edit_prog_info"
#define TXT_TP_INFO         "txt_channel_edit_tp_info"
#define TXT_PID_INFO        "txt_channel_edit_pid_info"

#define FAV_MASK            (0x80000000)
#define SORT_MODE_TOTAL   (SORT_MAX_NUM)

#define TIME_PRECISION  (30)

#if MINI_256_COLORS_OSD_SUPPORT
#define MINI_CHANNELEDIT_STATUS
#endif

#if(MINI_16_BITS_OSD_SUPPORT == 1)
#define CH_EDIT_POPDLG_POS_X 180
#define CH_EDIT_POPDLG_POS_Y 180
#else
#define CH_EDIT_POPDLG_POS_X 215
#define CH_EDIT_POPDLG_POS_Y 220
#endif

// special: can't use play program, it will pop up passwd. 
// so record postion use the function in app_play_control
//extern uint32_t app_play_pos_set(int32_t pos);
extern bool app_check_av_running(void);
extern void app_set_avcodec_flag(bool state);

static PlayerWindow video_wnd;
static AppIdOps* sp_GpIdOps = NULL;
static uint32_t s_ListSel = 0; // used for fav
static event_list *sp_ChEdTimer = NULL; // refresh time 
static uint32_t s_VideoStart = 0;
static bool s_exit_flag = false;
static bool s_save_flag = false;
static bool s_ch_ed_fullscreen = false;
static bool s_favname_flag = false;
static int32_t got_focus_sel = 0;//20150105

typedef enum
{
    CH_ED_MODE_NONE = 0,
    CH_ED_MODE_DEL,
#if TKGS_SUPPORT
    CH_ED_MODE_SWAP,
#else
    CH_ED_MODE_MOVE,
#endif
    CH_ED_MODE_SKIP,
    CH_ED_MODE_LOCK,
    CH_ED_MODE_FAV
}ch_ed_mode;
#if (MINI_16_BITS_OSD_SUPPORT)
ch_ed_mode s_Old_Mode = CH_ED_MODE_NONE;
#endif
ch_ed_mode s_Mode = CH_ED_MODE_NONE;

enum{
    ZOOM_IN_X = 0,
    ZOOM_IN_Y = 0,
    ZOOM_IN_W = APP_XRES,
    ZOOM_IN_H = APP_YRES 

};

static void channel_edit_cmb_group_build(void);
static void app_channel_edit_group_change(void);

static void channel_edit_change_prog_state(void)
{
	GxBusPmViewInfo view_info;
	GxMsgProperty_NodeNumGet node_num_get = {0};
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	node_num_get.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
	if(node_num_get.node_num == 0)
	{
		if(view_info.stream_type == GXBUS_PM_PROG_RADIO)
		{
			 view_info.stream_type = GXBUS_PM_PROG_TV;
			 g_AppFullArb.state.tv = STATE_ON;
		}
		else
		{
			view_info.stream_type = GXBUS_PM_PROG_RADIO;
			g_AppFullArb.state.tv = STATE_OFF;
		}
		view_info.group_mode = g_AppPlayOps.normal_play.view_info.group_mode;//all
		view_info.skip_view_switch = VIEW_INFO_SKIP_VANISH;
		app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));
		
		g_AppPlayOps.play_list_create(&view_info);
		g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
		return; /*if have radio,the radio cann't be deleted*/
	}
#if 0
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	view_info.stream_type = GXBUS_PM_PROG_RADIO;
	view_info.group_mode = g_AppPlayOps.normal_play.view_info.group_mode;//all
	view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

	node_num_get.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
	if(node_num_get.node_num == 0)
	{
		view_info.stream_type = GXBUS_PM_PROG_TV;
		view_info.group_mode = g_AppPlayOps.normal_play.view_info.group_mode;//all
		view_info.skip_view_switch = VIEW_INFO_SKIP_VANISH;
		app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));
		
        g_AppPlayOps.play_list_create(&view_info);
		return;
	}		
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info_bak));
	//restore viewinfo
	g_AppPlayOps.play_list_create(&view_info_bak);

	return;
#endif
}

static void ch_ed_progname_release_cb(PopKeyboard *data)
{
    uint32_t sel = 0;
    GxMsgProperty_NodeByIdGet node;
    GxMsgProperty_NodeModify node_modify = {0};

    if(data->in_ret == POP_VAL_CANCEL)
    {
	  GUI_SetProperty("wnd_channel_edit","update",NULL);  
          return;
    }

    if (data->out_name == NULL)
        return;

    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
    node.node_type = NODE_PROG;
    node.id = g_AppChOps.get_id(sel);
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    node_modify.node_type = NODE_PROG;
    node_modify.prog_data = node.prog_data;
    memcpy(node_modify.prog_data.prog_name, data->out_name, MAX_PROG_NAME-1);
    app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
    GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
    GUI_SetProperty(TXT_PROG_INFO, "string", node_modify.prog_data.prog_name);
}

static void ch_ed_favname_release_cb(PopKeyboard *data)
{
    uint32_t sel = 0;
    GxMsgProperty_NodeByPosGet node;
    GxMsgProperty_NodeModify node_modify = {0};

    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;

	//invalid characters
	if(strchr(data->out_name,',') != NULL
            || strchr(data->out_name,'[') != NULL
            || strchr(data->out_name,']') != NULL
            || (strcmp((const char*)(data->in_name),"HD") ==0 && strcmp((const char*)(data->out_name),"HD") !=0))
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.format = POP_FORMAT_DLG;
		pop.str = STR_ID_ERR_PVRNAME;
		pop.timeout_sec= 2;
		popdlg_create(&pop);

		return;
	}

	GUI_GetProperty(LISTVIEW_POP_FAV, "select", &sel);
#if 1
	GxMsgProperty_NodeNumGet num_node = {0};
	int i ;
	num_node.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&num_node));
#if TKGS_SUPPORT
	if (num_node.node_num>9)
	{
		num_node.node_num -= 9;
	}
#endif
	for(i = 0; i<num_node.node_num; i++)
	{
		if(i != sel)
		{
			node.pos = i;
			node.node_type = NODE_FAV_GROUP;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
			if(strcmp((const char*)data->out_name, (const char*)node.fav_item.fav_name) == 0)
			{
				PopDlg  pop;
				memset(&pop, 0, sizeof(PopDlg));
				pop.type = POP_TYPE_NO_BTN;
				pop.format = POP_FORMAT_DLG;
				pop.str = "the name exist!";
				pop.timeout_sec= 2;
				popdlg_create(&pop);
				return;
			}
		}
	}
#endif

	node.pos = sel;
    node.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

    node_modify.node_type = NODE_FAV_GROUP;
    node_modify.fav_item = node.fav_item;
    strncpy((char*)node_modify.fav_item.fav_name,
            (const char*)data->out_name, MAX_FAV_GROUP_NAME-1);

    app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
    GUI_SetProperty(LISTVIEW_POP_FAV, "update_row", &sel);

	s_favname_flag = true;
}

static void channel_edit_show_tip(char *red, char *green, char *yellow, char *mode)
{
#define IMG_RED     "img_ch_edit_tip_red"
#define TXT_RED     "text_ch_edit_tip_red"
#define IMG_GREEN   "img_ch_edit_tip_green"
#define TXT_GREEN   "text_ch_edit_tip_green"
#define IMG_YELLOW   "img_ch_edit_tip_yellow"
#define TXT_YELLOW   "text_ch_edit_tip_yellow"
#define IMG_MODE	"img_channel_edit_select"
#define TXT_MODE	"text_channel_edit_select"
    if (red == NULL)
    {
#ifdef MINI_CHANNELEDIT_STATUS
        GUI_SetProperty(IMG_RED, "state", "osd_trans_hide");   
        GUI_SetProperty(TXT_RED, "state", "osd_trans_hide");   
#else
        GUI_SetProperty(IMG_RED, "state", "hide");   
        GUI_SetProperty(TXT_RED, "state", "hide"); 
#endif  
    }
    else
    {
        // first line force update img
        GUI_SetProperty(IMG_RED, "img", "s_ts_red.bmp");   
        GUI_SetProperty(IMG_RED, "state", "show");   
        GUI_SetProperty(TXT_RED, "string", red);
        GUI_SetProperty(TXT_RED, "state", "show");   
    }

    if (green == NULL)
    {
#ifdef MINI_CHANNELEDIT_STATUS
        GUI_SetProperty(IMG_GREEN, "state", "osd_trans_hide");   
        GUI_SetProperty(TXT_GREEN, "state", "osd_trans_hide");   
#else
        GUI_SetProperty(IMG_GREEN, "state", "hide");   
        GUI_SetProperty(TXT_GREEN, "state", "hide");   
#endif  
    }
    else
    {
        GUI_SetProperty(IMG_GREEN, "state", "show");   
        GUI_SetProperty(TXT_GREEN, "string", green);
        GUI_SetProperty(TXT_GREEN, "state", "show");   
    }

    if (yellow == NULL)
    {
#ifdef MINI_CHANNELEDIT_STATUS
        GUI_SetProperty(IMG_YELLOW, "state", "osd_trans_hide");   
        GUI_SetProperty(TXT_YELLOW, "state", "osd_trans_hide");   
#else
        GUI_SetProperty(IMG_YELLOW, "state", "hide");   
        GUI_SetProperty(TXT_YELLOW, "state", "hide");   
#endif  
    }
    else
    {
        GUI_SetProperty(IMG_YELLOW, "state", "show");   
        GUI_SetProperty(TXT_YELLOW, "string", yellow);
        GUI_SetProperty(TXT_YELLOW, "state", "show");   
    }

	if(mode == NULL)
	{
#ifdef MINI_CHANNELEDIT_STATUS
		GUI_SetProperty(IMG_MODE, "state", "osd_trans_hide");   
		GUI_SetProperty(TXT_MODE, "state", "osd_trans_hide");   
#else
		GUI_SetProperty(IMG_MODE, "state", "hide");   
		GUI_SetProperty(TXT_MODE, "state", "hide");  
#endif   
	}
	else
	{
		GUI_SetProperty(IMG_MODE, "state", "show");   
		GUI_SetProperty(TXT_MODE, "string", mode);
		GUI_SetProperty(TXT_MODE, "state", "show");   
	}
}

void macro_ch_ed_start_video(void)
{
#define VIDEO_TEXT "txt_channel_edit_for_pp"
    int x,y,w,h = 0;
    sscanf((widget_get_property(gui_get_widget(NULL,VIDEO_TEXT),"rect")),"[%d,%d,%d,%d]",&x,&y,&w,&h);

    // zoom out
    video_wnd.x = x;
    video_wnd.y = y;
    video_wnd.width = w;
    video_wnd.height =h;

	GUI_SetInterface("flush", NULL);
    g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
	GUI_SetInterface("video_on", NULL);//GUI_SetInterface("video_off", NULL);
    g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    s_VideoStart = 1;
}

void macro_ch_ed_stop_video(void)
{
    // zoom in
    PlayerWindow video_wnd = {ZOOM_IN_X, ZOOM_IN_Y, ZOOM_IN_W, ZOOM_IN_H};

    g_AppPlayOps.program_stop();
    g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);

    s_VideoStart = 0;
	GUI_SetProperty("txt_channel_edit_signal", "state", "show");
	GUI_SetInterface("flush",NULL);
	GUI_SetInterface("video_off", NULL);	
}

static int _ch_edit_pop_save_cb(PopDlgRet ret)
{
    PlayerStatusInfo player_status;

    memset(&player_status, 0, sizeof(player_status));
    GxPlayer_MediaGetStatus(PLAYER_FOR_NORMAL, &player_status);

    if(player_status.vcodec.state == AVCODEC_RUNNING) //radio
    {
        if(g_AppFullArb.state.tv == STATE_ON)
        {
            app_set_avcodec_flag(true);
        }
    }

    if(player_status.acodec.state == AVCODEC_RUNNING) //radio
    {
        if(g_AppFullArb.state.tv == STATE_OFF)
        {
            app_set_avcodec_flag(true);
        }
    }

    if((player_status.vcodec.state == AVCODEC_STOPPED) //tv
            || (player_status.vcodec.state == AVCODEC_READY))
    {
        if(g_AppFullArb.state.tv == STATE_ON)
        {
            app_set_avcodec_flag(false);
        }
    }

    if((player_status.acodec.state == AVCODEC_STOPPED) //radio
            ||(player_status.acodec.state == AVCODEC_READY))
    {
        if(g_AppFullArb.state.tv == STATE_OFF)
        {
            app_set_avcodec_flag(false);
        }
    }

    return 0;
}

bool macro_ch_ed_save(void)
{
    PopDlg  pop;
    GxBusPmViewInfo  view_info_temp = {0};
    
    if (g_AppChOps.verity() == true)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_SAVE_INFO;
        pop.format = POP_FORMAT_DLG;
        pop.exit_cb = _ch_edit_pop_save_cb;
        channel_edit_show_tip(STR_ID_SORT, STR_ID_RENAME, STR_ID_PID, STR_ID_MODE);  
		s_save_flag = false;
        if (POP_VAL_OK == popdlg_create(&pop))
        {
            // do save
            g_AppChOps.save();
			s_save_flag = true;
            
            app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
            view_info_temp.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;

            if (GXCORE_ERROR == g_AppPlayOps.play_list_create(&view_info_temp))
            {
                    return false;
            }
        }
		else
		{
			uint32_t sel;
			sel =g_AppPlayOps.normal_play.play_count;
			if(GUI_CheckDialog("wnd_pop_book") != GXCORE_SUCCESS)
			{
				GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
				GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
			}
		}
    }

    return true;
}

#define START_VIDEO()   macro_ch_ed_start_video()
#define STOP_VIDEO()    macro_ch_ed_stop_video()
#define POP_SAVE()      macro_ch_ed_save()

// groub build

static void ch_ed_fav_flag_to_id_add(uint32_t flag)//flag != 0
{
	uint32_t i=0;
	GxMsgProperty_NodeNumGet node_num = {0};
	GxMsgProperty_NodeByPosGet node_get = {0};

	if (flag == 0)  
		return;
	node_num.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);

	for(i=0; i<node_num.node_num; i++)
	{
		if(0 != (flag&(1<<i)))
		{
			node_get.node_type = NODE_FAV_GROUP;
			node_get.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);
			sp_GpIdOps->id_add(sp_GpIdOps, 0, node_get.fav_item.id|0x80000000);
		}
	}
}

void app_show_channel_pid(GxBusPmDataProg *prog_data)
{
	char buffer[40] = {0};

	sprintf(buffer, "VPID:%d APID:%d PCRPID:%d", prog_data->video_pid,
	prog_data->cur_audio_pid,prog_data->pcr_pid);
	GUI_SetProperty(TXT_PID_INFO, "string",buffer);
}

#ifdef MINI_CHANNELEDIT_STATUS
void ch_ed_mode_change(ch_ed_mode mode)
{
#define IMG_DEL         "img_ch_ed_del"
#define IMG_MOVE        "img_ch_ed_move"
#define IMG_SKIP        "img_ch_ed_skip"
#define IMG_LOCK        "img_ch_ed_lock"
#define IMG_FAV         "img_ch_ed_fav"

#define IMG_DEL_UP         "img_ch_ed_del_up"
#define IMG_MOVE_UP        "img_ch_ed_move_up"
#define IMG_SKIP_UP        "img_ch_ed_skip_up"
#define IMG_LOCK_UP        "img_ch_ed_lock_up"
#define IMG_FAV_UP         "img_ch_ed_fav_up"

#define IMG_ARR_1       "img_ch_ed_arr_1"
#define IMG_ARR_2       "img_ch_ed_arr_2"
#define IMG_ARR_3       "img_ch_ed_arr_3"
#define IMG_ARR_4       "img_ch_ed_arr_4"
#define IMG_ARR_5       "img_ch_ed_arr_5"

#define TXT_DEL         "txt_ch_ed_del"
#define TXT_MOVE        "txt_ch_ed_move"
#define TXT_SKIP        "txt_ch_ed_skip"
#define TXT_LOCK        "txt_ch_ed_lock"
#define TXT_FAV         "txt_ch_ed_fav"

#define TXT_DEL_UP         "txt_ch_ed_del_up"
#define TXT_MOVE_UP        "txt_ch_ed_move_up"
#define TXT_SKIP_UP        "txt_ch_ed_skip_up"
#define TXT_LOCK_UP        "txt_ch_ed_lock_up"
#define TXT_FAV_UP         "txt_ch_ed_fav_up"

    char *mode_img[5] = {IMG_DEL, IMG_MOVE, IMG_SKIP, IMG_LOCK, IMG_FAV};
    char *mode_img_up[5] = {IMG_DEL_UP, IMG_MOVE_UP, IMG_SKIP_UP, IMG_LOCK_UP, IMG_FAV_UP};
    char *mode_arr[5] = {IMG_ARR_1, IMG_ARR_2, IMG_ARR_3, IMG_ARR_4, IMG_ARR_5};
    char *mode_text[5] = {TXT_DEL, TXT_MOVE, TXT_SKIP, TXT_LOCK, TXT_FAV};
    char *mode_text_up[5] = {TXT_DEL_UP, TXT_MOVE_UP, TXT_SKIP_UP, TXT_LOCK_UP, TXT_FAV_UP};
	uint32_t i=0;

	GUI_SetProperty("img_ch_edit_tip_ok", "state", "hide");
	GUI_SetProperty("text_ch_edit_tip_ok", "state", "hide");
    GUI_SetProperty("img_channel_edit_tip_sat", "state", "show");
    GUI_SetProperty("txt_channel_edit_tip_sat", "state", "show");
    GUI_SetProperty("img_channel_edit_tip_ok", "state", "show");
    GUI_SetProperty("txt_channel_edit_tip_ok", "state", "show");

    if (mode != CH_ED_MODE_NONE)
    {
		for(i=0; i<5; i++)
		{
			if(i != (mode -1))
			{
				GUI_SetProperty(mode_img_up[i], "state", "osd_trans_hide");
				GUI_SetProperty(mode_text_up[i], "state", "osd_trans_hide");
				GUI_SetProperty(mode_img[i], "state", "show");
				GUI_SetProperty(mode_text[i], "state", "show");
			}
			else
			{
				GUI_SetProperty(mode_img[i], "state", "osd_trans_hide");
				GUI_SetProperty(mode_text[i], "state", "osd_trans_hide");
				GUI_SetProperty(mode_img_up[i], "state", "show");
				GUI_SetProperty(mode_text_up[i], "state", "show");
				GUI_SetProperty(mode_arr[i], "state", "show");
			}
		}
        if (mode == CH_ED_MODE_DEL
        ||  mode == CH_ED_MODE_LOCK || mode == CH_ED_MODE_SKIP)
        {
			channel_edit_show_tip(STR_ID_ALL, NULL, NULL, NULL);
			GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
			GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
			GUI_SetProperty("text_ch_edit_tip_ok", "string", "Select");
            GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
        }
        else if(mode == CH_ED_MODE_MOVE)
        {
			//channel_edit_show_tip(NULL, STR_ID_GROUP_MOVE, NULL, NULL);
			channel_edit_show_tip(NULL, STR_ID_SELECT_GROUP_CHANNELS, NULL, NULL);
			GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
			GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
			GUI_SetProperty("text_ch_edit_tip_ok", "string", "Select");
            GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
        }
        else
        {
			channel_edit_show_tip(NULL, NULL, NULL, NULL);
			GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
			GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
			GUI_SetProperty("text_ch_edit_tip_ok", "string", "Select");
            GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
        }
    }
    else
    {
		for(i=0; i<5; i++)
		{
			GUI_SetProperty(mode_arr[i], "state", "osd_trans_hide");
			GUI_SetProperty(mode_img_up[i], "state", "osd_trans_hide");
			GUI_SetProperty(mode_text_up[i], "state", "osd_trans_hide");
			GUI_SetProperty(mode_img[i], "state", "show");
			GUI_SetProperty(mode_text[i], "state", "show");
		}
        channel_edit_show_tip(STR_ID_SORT, STR_ID_RENAME, STR_ID_PID, STR_ID_MODE);
	}
}

#else
void ch_ed_mode_change(ch_ed_mode mode)
{
#define IMG_DEL         "img_ch_ed_del"
#define IMG_MOVE        "img_ch_ed_move"
#define IMG_SKIP        "img_ch_ed_skip"
#define IMG_LOCK        "img_ch_ed_lock"
#define IMG_FAV         "img_ch_ed_fav"
#if (MINI_16_BITS_OSD_SUPPORT)
#define IMG_DEL_N         "img_ch_ed_del1"
#define IMG_MOVE_N        "img_ch_ed_move1"
#define IMG_SKIP_N        "img_ch_ed_skip1"
#define IMG_LOCK_N        "img_ch_ed_lock1"
#define IMG_FAV_N         "img_ch_ed_fav1"
#endif

#define TXT_DEL         "txt_ch_ed_del"
#define TXT_MOVE        "txt_ch_ed_move"
#define TXT_SKIP        "txt_ch_ed_skip"
#define TXT_LOCK        "txt_ch_ed_lock"
#define TXT_FAV         "txt_ch_ed_fav"
//#define STR_DEL         "Delete"
//#define STR_MOVE        "Move"
//#define STR_SKIP        "Skip"
//#define STR_LOCK        "Lock"
//#define STR_FAV         "Favor"
#if (MINI_16_BITS_OSD_SUPPORT)
	char *mode_img[5] = {IMG_DEL, IMG_MOVE, IMG_SKIP, IMG_LOCK, IMG_FAV};
    char *mode_img_n[5] = {IMG_DEL_N, IMG_MOVE_N, IMG_SKIP_N, IMG_LOCK_N, IMG_FAV_N};
#else
    static ch_ed_mode mode_bak = CH_ED_MODE_NONE;
    char *mode_img[5] = {IMG_DEL, IMG_MOVE, IMG_SKIP, IMG_LOCK, IMG_FAV};
    char *mode_text[5] = {TXT_DEL, TXT_MOVE, TXT_SKIP, TXT_LOCK, TXT_FAV};
    char *mode_str[5] = {STR_ID_DELETE, STR_ID_MOVE, STR_ID_SKIP, STR_ID_LOCK, STR_ID_FAV};
    char mode_bmp[24] = {0};
 #endif
	GUI_SetProperty("img_ch_edit_tip_ok", "state", "hide");
	GUI_SetProperty("text_ch_edit_tip_ok", "state", "hide");
	GUI_SetProperty("img_channel_edit_tip_sat", "state", "show");
	GUI_SetProperty("txt_channel_edit_tip_sat", "state", "show");
	GUI_SetProperty("img_channel_edit_tip_ok", "state", "show");
	GUI_SetProperty("txt_channel_edit_tip_ok", "state", "show");

    if (mode != CH_ED_MODE_NONE)
    {
	#if (MINI_16_BITS_OSD_SUPPORT)
		GUI_SetProperty(mode_img[mode-1], "state","hide");
		GUI_SetProperty(mode_img_n[mode-1], "state","show");
	#else
        sprintf(mode_bmp, "s_channellist_%d_c.bmp", mode);
        GUI_SetProperty(mode_img[mode-1], "img", mode_bmp);
        GUI_SetProperty(mode_text[mode-1], "string", mode_str[mode-1]);
#endif

        if (mode == CH_ED_MODE_DEL
                ||  mode == CH_ED_MODE_LOCK || mode == CH_ED_MODE_SKIP)
        {
            channel_edit_show_tip(STR_ID_ALL, NULL, NULL, NULL);
            GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
            GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
            GUI_SetProperty("text_ch_edit_tip_ok", "string", "Select");
            GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
        }
#if TKGS_SUPPORT
        else if(mode == CH_ED_MODE_SWAP)
        {
            channel_edit_show_tip(NULL, NULL, NULL, NULL);
            GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
            GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
            GUI_SetProperty("text_ch_edit_tip_ok", "string", "Select");
            GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
        }
#else
        else if(mode == CH_ED_MODE_MOVE)
        {
            //channel_edit_show_tip(NULL, STR_ID_GROUP_MOVE, NULL, NULL);
            channel_edit_show_tip(NULL, STR_ID_SELECT_GROUP_CHANNELS, NULL, NULL);
            GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
            GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
            GUI_SetProperty("text_ch_edit_tip_ok", "string", "Select");

            GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
        }
#endif
        else
        {
            channel_edit_show_tip(NULL, NULL, NULL, NULL);
            GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
            GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
            GUI_SetProperty("text_ch_edit_tip_ok", "string", "Select");
            GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
            GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
            GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
        }
    }
    else
    {
#if (MINI_16_BITS_OSD_SUPPORT)
        if(s_Old_Mode != CH_ED_MODE_NONE)
        {
            GUI_SetProperty(mode_img_n[s_Old_Mode-1], "state","hide");
            GUI_SetProperty(mode_img[s_Old_Mode-1], "state","show");
            channel_edit_show_tip(STR_ID_SORT, STR_ID_RENAME, STR_ID_PID, STR_ID_MODE);
        }
#else
        if(CH_ED_MODE_NONE != mode_bak)
        {
            sprintf(mode_bmp, "s_channellist_%d.bmp", mode_bak);
            GUI_SetProperty(mode_img[mode_bak-1], "img", mode_bmp);
            GUI_SetProperty(mode_text[mode_bak-1], "string", mode_str[mode_bak-1]);
            channel_edit_show_tip(STR_ID_SORT, STR_ID_RENAME, STR_ID_PID, STR_ID_MODE);
        }
#endif
    }
#if (MINI_16_BITS_OSD_SUPPORT)
    GUI_SetProperty("wnd_channel_edit", "update", NULL);
#else

    mode_bak = mode;
#endif
}
#endif

void channel_edit_get_valid_group_id(void)
{
#define ALL_SAT     0x5a5a5a5a
	GxBusPmViewInfo view_info;
	GxBusPmViewInfo view_info_bak;
	GxMsgProperty_NodeNumGet prog_num = {0};
	GxMsgProperty_NodeNumGet sat_num = {0};
	GxMsgProperty_NodeNumGet fav_num = {0};
	GxMsgProperty_NodeByPosGet node_get = {0};
	uint32_t i=0;
	uint32_t fav_flag=0;

	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	view_info_bak = view_info;	
    // include skip program
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
	view_info.group_mode = 0;	//all
    view_info.stream_type = 
        g_AppPlayOps.normal_play.view_info.stream_type;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

	sat_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&sat_num));
	fav_num.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&fav_num));
    prog_num.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&prog_num));
	
	// id manage
	if(sp_GpIdOps == NULL)
	{
    	return;	
	}
	sp_GpIdOps->id_close(sp_GpIdOps);   // clear first
	
	if (sp_GpIdOps->id_open(sp_GpIdOps, MANAGE_DIFF_LIST, 
                sat_num.node_num+fav_num.node_num+1) != GXCORE_SUCCESS) 
		return;

	//add 0: all satellite
	sp_GpIdOps->id_add(sp_GpIdOps, 0, ALL_SAT);
	
	//get sat id
	for(i=0; i<prog_num.node_num; i++)
	{
		node_get.node_type = NODE_PROG;
		node_get.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

		sp_GpIdOps->id_add(sp_GpIdOps, 0, node_get.prog_data.sat_id);
	}
	//get fav id
	for(i=0; i<prog_num.node_num; i++)
	{
		node_get.node_type = NODE_PROG;
		node_get.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

        fav_flag |= node_get.prog_data.favorite_flag;
	}
    ch_ed_fav_flag_to_id_add(fav_flag);

	//restore
	view_info = view_info_bak;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

}

//根据当前view_info找到现在是第几个组
static uint32_t channel_edit_get_cur_group_num(void)
{
	uint32_t i = 0;
	uint32_t total_id=0;
	uint32_t fav_id=0;
	uint32_t *id_buffer = NULL;

	total_id = sp_GpIdOps->id_total_get(sp_GpIdOps);
	//get all of id
	if(total_id == 0)
	{
		printf("error total id para\n");
		return 0;
	}
	id_buffer = GxCore_Malloc(total_id*sizeof(uint32_t));//need GxCore_Free
	if(id_buffer == NULL)
	{
		printf("\nchannel_edit_get_cur_group, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	memset(id_buffer, 0, total_id*sizeof(uint32_t));
	sp_GpIdOps->id_get(sp_GpIdOps, id_buffer);

	if(0 == g_AppPlayOps.normal_play.view_info.group_mode)//all satellite
	{
		GxCore_Free(id_buffer);
		return 0;
	}
	else if(1 == g_AppPlayOps.normal_play.view_info.group_mode)//sat
	{
		for(i=1; i<total_id; i++)
		{
			if((id_buffer[i] & FAV_MASK) == 0)//sat
			{
				if(g_AppPlayOps.normal_play.view_info.sat_id == id_buffer[i])
				{
					GxCore_Free(id_buffer);
					return i;
				}	
			}
		}
	}
	else if(2 == g_AppPlayOps.normal_play.view_info.group_mode)//fav 
	{
		for(i=1; i<total_id; i++)
		{
			if((id_buffer[i] & FAV_MASK) != 0)//fav
			{
				fav_id = (id_buffer[i] & (~FAV_MASK));
				if(g_AppPlayOps.normal_play.view_info.fav_id == fav_id)
				{
					GxCore_Free(id_buffer);
					return i;
				}	
			}
		}
	}
	GxCore_Free(id_buffer);
	//default all
	return 0;
}

static void channel_edit_cmb_group_build(void)
{
#define BUFFER_LENGTH_GROUP	(24)
	uint32_t i = 0;
	uint32_t total_id=0;
	char *buffer_all = NULL;
	char buffer[BUFFER_LENGTH_GROUP] = {0};
	uint32_t *id_buffer = NULL;
	GxMsgProperty_NodeByIdGet node;
	int sel=0;
    // get valid id
    channel_edit_get_valid_group_id();

	total_id = sp_GpIdOps->id_total_get(sp_GpIdOps);
	if(total_id == 0)
	{
		printf("error total id para\n");
		return ;
	}
	buffer_all = GxCore_Malloc(total_id*(BUFFER_LENGTH_GROUP));//need GxCore_Free
	if(buffer_all == NULL)
	{
		printf("channel_edit_cmb_group_build, malloc failed...\n");
		return ;
	}
	memset(buffer_all, 0, total_id*(BUFFER_LENGTH_GROUP));

	//get all of id
	id_buffer = GxCore_Malloc(total_id*sizeof(uint32_t));//need GxCore_Free
	if(id_buffer == NULL)
	{
		printf("channel_edit_cmb_group_build, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}
	memset(id_buffer, 0, total_id*sizeof(uint32_t));
	sp_GpIdOps->id_get(sp_GpIdOps, id_buffer);

	//first one
	memset(buffer, 0, BUFFER_LENGTH_GROUP);
	sprintf(buffer,"[%s",STR_ID_ALL);
	strcat(buffer_all, buffer);

	for(i=1; i<total_id; i++)
	{
		if((id_buffer[i] & 0x80000000) == 0)//sat
		{
			node.node_type = NODE_SAT;
			node.id = id_buffer[i];
			app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

			memset(buffer, 0, BUFFER_LENGTH_GROUP);
			sprintf(buffer,",%s",node.sat_data.sat_s.sat_name);
		}
		else //fav
		{
			node.node_type = NODE_FAV_GROUP;
			node.id = (id_buffer[i] & 0x7FFFFFFF);
			app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

			memset(buffer, 0, BUFFER_LENGTH_GROUP);
			sprintf(buffer,",%s",node.fav_item.fav_name);
		}
		strcat(buffer_all, buffer);
	}
	strcat(buffer_all, "]");
	GUI_SetProperty(GROUP_CH_ED,"content",buffer_all);
    sel = channel_edit_get_cur_group_num();

	GUI_SetProperty(GROUP_CH_ED, "select", &sel);

	GxCore_Free(buffer_all);
	GxCore_Free(id_buffer);
}

// cmb group
static void app_channel_edit_group_change(void)
{
	uint32_t sel = 0;
	uint32_t total_id=0;
	uint32_t *id_buffer = NULL;
	uint32_t old_id = 0;
	GxMsgProperty_NodeByPosGet node = {0};
	
    GUI_GetProperty(LISTVIEW_CH_ED, "active", &sel);
    old_id = g_AppChOps.get_id(sel);

	total_id = sp_GpIdOps->id_total_get(sp_GpIdOps);
	if(total_id == 0)
	{
		printf("\app_channel_edit_group_change, total id error...%s,%d\n",__FILE__,__LINE__);
		return;
	}
	//get all of id
	id_buffer = GxCore_Malloc(total_id*sizeof(uint32_t));//need GxCore_Free
	if(id_buffer == NULL)
	{
		printf("\app_channel_edit_group_change, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}
	memset(id_buffer, 0, total_id*sizeof(uint32_t));
	sp_GpIdOps->id_get(sp_GpIdOps, id_buffer);

	GUI_GetProperty(GROUP_CH_ED, "select", &sel);
	if(0 == sel)//all satellite
	{
		g_AppPlayOps.normal_play.view_info.group_mode = 0;
	}
	else//sat or fav
	{
		if((id_buffer[sel] & 0x80000000) == 0)//sat
		{
			g_AppPlayOps.normal_play.view_info.group_mode = 1;
			g_AppPlayOps.normal_play.view_info.sat_id = id_buffer[sel];
		}
		else//fav
		{
			g_AppPlayOps.normal_play.view_info.group_mode = 2;
			g_AppPlayOps.normal_play.view_info.fav_id = (id_buffer[sel] & 0x7FFFFFFF);
		}
	}

	g_AppPlayOps.normal_play.view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
	if (GXCORE_ERROR == g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info)))
	{
	    GxCore_Free(id_buffer);
	    return;
	}

	// initial channel_edit list edit
    g_AppChOps.create(g_AppPlayOps.normal_play.play_total);
	
	//update listview show
	GUI_SetProperty(LISTVIEW_CH_ED,"update_all",NULL);

    sel = g_AppPlayOps.normal_play.play_count;
	GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
    GUI_SetProperty(LISTVIEW_CH_ED,"select",&sel);

    GUI_SetInterface("flush", NULL);

    if (s_VideoStart != 0)
    {
        // special deal: when just in, only do ui, play will do in VIDEO_START 
        node.node_type = NODE_PROG;
        node.pos = sel;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

        if(old_id != node.prog_data.id)
        {
			//相同节目不做切换，否则txt_channel_edit_signal会遮住节目
			g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
			//GUI_SetInterface("video_off", NULL); //连续切台有黑影
			g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
		}
        else
        {
            g_AppPlayOps.current_play.play_count = g_AppPlayOps.normal_play.play_count;
        }
    }

    GxCore_Free(id_buffer);
    return;
}


// signal channel eidt listview
SIGNAL_HANDLER int app_channel_edit_list_change(GuiWidget *widget, void *usrdata)
{	
#define INFO_LEN	24
	uint32_t sel=0;
	uint32_t listview_sel = 0;
	static GxMsgProperty_NodeByPosGet ProgNode;
	static GxMsgProperty_NodeByIdGet node;
	GxMsgProperty_NodeByIdGet SatNode = {0};
	char buffer[INFO_LEN];
	
	GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
	got_focus_sel = sel;//20150105

    if (g_AppChOps.check(sel, MODE_FLAG_MOVING))
        return EVENT_TRANSFER_STOP;

    // if move need get real pos to show name

    //get prog node
    GUI_SetProperty(TXT_PROG_INFO, "string", " ");
    ProgNode.node_type = NODE_PROG;
    ProgNode.pos = sel;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);
    GUI_SetProperty(TXT_PROG_INFO, "string", ProgNode.prog_data.prog_name);

    //get sat node
    node.node_type = NODE_SAT;
    node.id = ProgNode.prog_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
    memset(buffer, 0 ,INFO_LEN);
    sel = strlen((const char*)(node.sat_data.sat_s.sat_name));
    if(sel >= MAX_SAT_NAME)
        sel = MAX_SAT_NAME;
    memcpy(buffer,node.sat_data.sat_s.sat_name,sel);
#if DOUBLE_S2_SUPPORT
    if(node.sat_data.tuner)//T2
        strcat(buffer," ,T2");
    else
        strcat(buffer," ,T1");
#endif
    GUI_SetProperty(TXT_SAT_INFO, "string", buffer);

    GxBusPmDataSatType type;
    type = node.sat_data.type;

	memcpy(&SatNode,&node,sizeof(GxMsgProperty_NodeByIdGet));

    node.node_type = NODE_TP;
    node.id = ProgNode.prog_data.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
    memset(buffer, 0 ,INFO_LEN);

#if DEMOD_DVB_T
    if(GXBUS_PM_SAT_DVBT2 == type)
    {
        char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};

        if(node.tp_data.tp_dvbt2.bandwidth <BANDWIDTH_AUTO)
            sprintf(buffer, "%d.%d / %s",
                    node.tp_data.frequency/1000,(node.tp_data.frequency/100)%10,
                    bandwidth_str[node.tp_data.tp_dvbt2.bandwidth]);
    }
#endif
#if DEMOD_DVB_C
    if(GXBUS_PM_SAT_C == type)
    {
        char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};

        if(node.tp_data.tp_c.modulation <QAM_AUTO)
            sprintf(buffer, "%d / %s /%d",
                    node.tp_data.frequency/1000,
                    modulation_str[node.tp_data.tp_c.modulation],
                    node.tp_data.tp_c.symbol_rate/1000);
    }
#endif

#if DEMOD_DTMB
    if(GXBUS_PM_SAT_DTMB == type)
    {
		if(GXBUS_PM_SAT_1501_DTMB == SatNode.sat_data.sat_dtmb.work_mode)
		{
			char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};// HAVE 7M?
			if(node.tp_data.tp_dtmb.symbol_rate <BANDWIDTH_AUTO)
				sprintf(buffer, "%d.%d / %s",
						node.tp_data.frequency/1000,(node.tp_data.frequency/100)%10,
						bandwidth_str[node.tp_data.tp_dtmb.symbol_rate]);
		}
		else
		{// work in dvbc mode
			char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};

	        if(node.tp_data.tp_c.modulation <QAM_AUTO)
	            sprintf(buffer, "%d / %s /%d",
	                    node.tp_data.frequency/1000,
	                    modulation_str[node.tp_data.tp_dtmb.modulation],
	                    node.tp_data.tp_dtmb.symbol_rate/1000);
		}
	   
    }
#endif


#if DEMOD_DVB_S
    if(GXBUS_PM_SAT_S == type)
    {
        char* PolarStr[2] = {"H", "V"};
        //get tp node
        node.node_type = NODE_TP;
        node.id = ProgNode.prog_data.tp_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
        memset(buffer, 0 ,INFO_LEN);
        sprintf(buffer, "%d / %s / %d",
                node.tp_data.frequency,
                PolarStr[node.tp_data.tp_s.polar],
                node.tp_data.tp_s.symbol_rate);
    }
#endif

    GUI_SetProperty(TXT_TP_INFO, "string",buffer);

    // PID
    app_show_channel_pid(&ProgNode.prog_data);
// 20141224   
	GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
    	/*if(g_AppPlayOps.normal_play.play_count == sel)//if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
	{
		//listview_sel = -1;
	}
	else
	{
		listview_sel =g_AppPlayOps.normal_play.play_count;
		//sel = g_AppPlayOps.normal_play.play_count;
		g_AppChOps.real_select(&listview_sel);
	}*/
	listview_sel =g_AppPlayOps.normal_play.play_count;
	g_AppChOps.real_select(&listview_sel);/*after move, list in channel edit, is differ with play list*/
	GUI_SetProperty(LISTVIEW_CH_ED, "active", &listview_sel);

    	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_edit_list_create(GuiWidget *widget, void *usrdata)
{
    // initial channel_edit list edit
    g_AppChOps.create(g_AppPlayOps.normal_play.play_total);

    return GXCORE_SUCCESS;
}


SIGNAL_HANDLER int app_channel_edit_list_get_total(GuiWidget *widget, void *usrdata)
{
	return g_AppPlayOps.normal_play.play_total;
}

SIGNAL_HANDLER int app_channel_edit_list_get_data(GuiWidget *widget, void *usrdata)
{
#define PROG_COUNT_LEN  (8)	
#define PROG_NAME_LEN   (24)	
	ListItemPara* item = NULL;
	static GxMsgProperty_NodeByIdGet node;
	static char prog_count[PROG_COUNT_LEN];
    int init_value = 0;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	node.node_type = NODE_PROG;
	node.id= g_AppChOps.get_id(item->sel);
	//get node
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

	//col-0: num
	item->x_offset = 0;
	item->image = NULL;
	memset(prog_count, 0, PROG_COUNT_LEN);
#if TKGS_SUPPORT
	AppProgExtInfo prog_ext_info = {{0}};

    if((app_tkgs_get_operatemode() == GX_TKGS_AUTOMATIC)||
            (app_tkgs_get_operatemode() == GX_TKGS_CUSTOMIZABLE)
      )
          {
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
		{
            sprintf(prog_count, "%04d", prog_ext_info.tkgs.logicnum);
		}
		else
	sprintf(prog_count, "%04d", g_AppChOps.get_lcn(item->sel)/*node.prog_data.logicnum*/);
		}
	else
	sprintf(prog_count, " %04d", (item->sel+1));
#else
	sprintf(prog_count, " %04d", (item->sel+1));
#endif
	item->string = prog_count;
#if 1
	//col-1: scramble flag
	item = item->next;
	if (item == NULL) return GXCORE_ERROR;
	item->x_offset = 0;
	item->image = NULL;

	if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
		item->string = "$";
    }
	else
		item->string = NULL;

	// program name
	item = item->next;
	if (item == NULL) return GXCORE_ERROR;
	item->x_offset = 0;
	item->image = NULL;
	item->string = (char*)node.prog_data.prog_name;
#else
	//col-1: channel name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	node.node_type = NODE_PROG;
	node.id= g_AppChOps.get_id(item->sel);
	//get node
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

	static char prog_name[PROG_NAME_LEN];
	memset(prog_name, 0, PROG_NAME_LEN);
	if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
		strcpy(prog_name, "$ ");
    }
    strcat(prog_name, (char*)node.prog_data.prog_name);
	item->string = prog_name;
#endif
	//col-2: del
	item = item->next;
    if (item == NULL) return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = NULL;
	item->image = NULL;
	if (g_AppChOps.check(item->sel, MODE_FLAG_DEL))
	{
        item->image = "s_icon_del.bmp";
	}

	//col-3: move 
	item = item->next;
    if (item == NULL) return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = NULL;
	item->image = NULL;
	if (g_AppChOps.check(item->sel, MODE_FLAG_MOVING)
	    || g_AppChOps.check(item->sel, MODE_FLAG_MOVE))
	{
        item->image = "s_icon_move.bmp";
	}

	//col-4: sikp
	item = item->next;
    if (item == NULL) return GXCORE_ERROR;

	item->x_offset = 0;
	item->string = NULL;
	item->image = NULL;
	if (g_AppChOps.check(item->sel, MODE_FLAG_SKIP))
	{
        item->image = "s_icon_skip.bmp";
	}

	//col-5: lock
	item = item->next;
    if (item == NULL) return GXCORE_ERROR;

	item->x_offset = 0;
	item->string = NULL;
	item->image = NULL;
    GxBus_ConfigGetInt(CHANNEL_LOCK_KEY, &init_value, CHANNEL_LOCK);

	if ((CHANNEL_LOCK_ENABLE == init_value)&&(g_AppChOps.check(item->sel, MODE_FLAG_LOCK)))
	{
    	item->image = "s_icon_lock.bmp";
	}

	//col-6: fav
	item = item->next;
    if (item == NULL) return GXCORE_ERROR;

	item->x_offset = 0;
	item->string = NULL;
	item->image = NULL;
	if (g_AppChOps.check(item->sel, MODE_FLAG_FAV))
	{
        item->image = "s_icon_fav.bmp";
	}

	return EVENT_TRANSFER_STOP;
}

int ch_ed_lock_passwd_cb(PopPwdRet *ret, void *usr_data)
{
	if(ret->result == PASSWD_OK)
	{
        s_Mode = CH_ED_MODE_LOCK;
        ch_ed_mode_change(s_Mode);	
    }
    else if(ret->result == PASSWD_ERROR)
    {
		PasswdDlg passwd_dlg;
		memset(&passwd_dlg, 0, sizeof(PasswdDlg));
		passwd_dlg.passwd_exit_cb = ch_ed_lock_passwd_cb;
		passwd_dlg.pos.x = CH_EDIT_POPDLG_POS_X;
		passwd_dlg.pos.y = CH_EDIT_POPDLG_POS_Y;
		passwd_dlg_create(&passwd_dlg);
#if 0
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_ERR_PASSWD;
        pop.pos.x = CH_EDIT_POPDLG_POS_X;
        pop.pos.y = CH_EDIT_POPDLG_POS_Y;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        popdlg_create(&pop);
#endif 
    }
    else
    {}

	return 0;	
}

static void _ch_ed_pop_sort_exit(PopDlgRet ret, uint32_t pos)
{
	//uint32_t listview_sel = g_AppPlayOps.normal_play.play_count;
	if(ret == POP_VAL_OK)
	{
		if(g_AppPlayOps.normal_play.rec == PLAY_KEY_UNLOCK)
		{
			g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
		}
		GUI_SetProperty(LISTVIEW_CH_ED, "select", &pos);
		GUI_SetProperty(LISTVIEW_CH_ED, "update_all", NULL);	
		GUI_SetProperty(LISTVIEW_CH_ED, "active", &pos);
		g_AppPlayOps.normal_play.play_count = pos;

		//GUI_SetProperty("txt_channel_edit_signal", "state", "show");for error 47511

		#if RECALL_LIST_SUPPORT
		//extern int g_recall_count;
		g_AppPlayOps.recall_play[0] = g_AppPlayOps.normal_play;//g_recall_count
		#else
		g_AppPlayOps.recall_play = g_AppPlayOps.normal_play; //lost recall after sort 
		#endif
		 
		g_AppPlayOps.current_play= g_AppPlayOps.normal_play;

	}
}

static void _ch_ed_pop_pid_exit(PopDlgRet ret, uint32_t pid_v, uint32_t pid_a, uint32_t pid_pcr)
{
	static int video_status = 0;
	
	if(ret == POP_VAL_OK)
	{
		uint32_t sel = 0;
		GxMsgProperty_NodeByPosGet prog_node = {0}	;

		GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
		g_AppChOps.real_pos(&sel);
		prog_node.node_type = NODE_PROG;
		prog_node.pos = sel;
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node)))
		{
			GxMsgProperty_NodeModify prog_modify = {0};
			prog_modify.node_type= NODE_PROG;
			memcpy(&prog_modify.prog_data, &prog_node.prog_data, sizeof(GxBusPmDataProg));
			if(pid_v < 0x1fff)
			{
				prog_modify.prog_data.video_pid = pid_v;
			}
			if(pid_a < 0x1fff)
			{
				prog_modify.prog_data.cur_audio_pid = pid_a;
			}
			if(pid_pcr < 0x1fff)
			{
				prog_modify.prog_data.pcr_pid = pid_pcr;
			}
			app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &prog_modify);
			if(app_check_av_running() == true)
			{
				 g_AppFullArb.timer_stop();//20121108
				 video_status = 1;
			}
			else
			{
				if(video_status)
				{
					video_status = 0;
					 g_AppFullArb.timer_start();//20121108
				}
			}
			
			//g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
			g_AppPlayOps.program_play(PLAY_MODE_POINT, sel);
			
			//GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
		    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
			GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
			app_show_channel_pid(&prog_modify.prog_data);

		}
	}
}

int channel_edit_mode_off_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint32_t sel=0;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_EXIT:
				case STBK_MENU:
					{
						ret = EVENT_TRANSFER_KEEPON;
					}
					break;

				case STBK_DOWN:
				case STBK_UP:
				//case STBK_PAGE_DOWN:
				//case STBK_PAGE_UP:
				case STBK_LEFT:
				case STBK_RIGHT:
				    ret = EVENT_TRANSFER_KEEPON;
					break;
				case STBK_PAGE_UP:
				{
					uint32_t sel;
					GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
					if (sel == 0)
					{
						sel = app_channel_edit_list_get_total(NULL,NULL)-1;
						GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
						ret =   EVENT_TRANSFER_STOP; 
					}
					else
					{
						ret =  EVENT_TRANSFER_KEEPON; 
					}
				}
					break;
				case STBK_PAGE_DOWN:
				{
					uint32_t sel;
					GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
					if (app_channel_edit_list_get_total(NULL,NULL)
					== sel+1)
					{
						sel = 0;
						GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
						ret =  EVENT_TRANSFER_STOP; 
					}
					else
					{
						ret =  EVENT_TRANSFER_KEEPON; 
					}
				}
					break;

				case STBK_1:
				    // del
				    #if TKGS_SUPPORT
				    if(GX_TKGS_OFF != app_tkgs_get_operatemode())
				    {
				    	app_tkgs_cantwork_popup();
					break;
				    }
				    #endif
				    s_Mode = CH_ED_MODE_DEL;
				    ch_ed_mode_change(s_Mode);
				    break;
				case STBK_2:
				    // move
				    #if TKGS_SUPPORT
				    if(GX_TKGS_AUTOMATIC == app_tkgs_get_operatemode())
				    {
				    	app_tkgs_cantwork_popup();
					break;
				    }
				    else
					s_Mode = CH_ED_MODE_SWAP;	
                    #else
				    s_Mode = CH_ED_MODE_MOVE;
				    #endif
				   
				    ch_ed_mode_change(s_Mode);
				    break;
				case STBK_3:
				    // skip
				    s_Mode = CH_ED_MODE_SKIP;
				    ch_ed_mode_change(s_Mode);
				    break;
				case STBK_4:
				    // lock
				    {
				        int32_t lock_state = 0;
				        GxBus_ConfigGetInt(CHANNEL_LOCK_KEY, &lock_state, CHANNEL_LOCK);
				        if (CHANNEL_LOCK_ENABLE == lock_state)
				        {
				            PasswdDlg passwd_dlg;
				            memset(&passwd_dlg, 0, sizeof(PasswdDlg));
							passwd_dlg.passwd_exit_cb = ch_ed_lock_passwd_cb;
							passwd_dlg.pos.x = CH_EDIT_POPDLG_POS_X;
							passwd_dlg.pos.y = CH_EDIT_POPDLG_POS_Y;
							passwd_dlg_create(&passwd_dlg);
				        }
				        else
				        {
				            s_Mode = CH_ED_MODE_LOCK;
				            ch_ed_mode_change(s_Mode);	
				        }
				    }
				    break;
				case STBK_5:
				    // fav
				    s_Mode = CH_ED_MODE_FAV;
				    ch_ed_mode_change(s_Mode);
				    break;
				case STBK_OK:
					{
						GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
						g_AppChOps.real_pos(&sel);
                        if((sel != g_AppPlayOps.normal_play.play_count)
                                || (g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK))
                        {
                            // sel to real postion
                            //g_AppFullArb.timer_stop();
							//GUI_SetInterface("video_off", NULL);	//因为节目编辑界面切台有时小画面会闪现黑色线条注释掉此两行错误 #41369
                            //GUI_SetInterface("flush",NULL);
							g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
							g_AppPlayOps.program_play(PLAY_MODE_POINT,sel);
							g_AppChOps.real_select(&sel);	
							GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);//20141224
                        }
                    }
				    break;
				case STBK_RED:
				#if TKGS_SUPPORT
				if(GX_TKGS_AUTOMATIC == app_tkgs_get_operatemode())
				{
				app_tkgs_cantwork_popup();
				break;
				}
				else
				#endif
                {
                    PopDlgPos pop_pos = {0};
                    uint32_t sel;
                    //int32_t old_playcount,old_total,dec_cout=0;
                    char buffer[21],buffer_new[21];
                    GxMsgProperty_NodeByPosGet progNode;
                    //old_playcount = g_AppPlayOps.normal_play.play_count;
                    GxBusPmDataProgSwitch old_prog_lock;

                    GUI_GetProperty(LISTVIEW_CH_ED, "active", &sel);
					g_AppChOps.real_pos(&sel);
                    progNode.node_type = NODE_PROG;
                    progNode.pos = sel;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &progNode);
                    memset(buffer, 0 ,sizeof(buffer));
                    memcpy(buffer, progNode.prog_data.prog_name, (sizeof(buffer)-1));
                    old_prog_lock = progNode.prog_data.lock_flag;
                    if (false == POP_SAVE())
                    {
                        GxBusPmViewInfo  view_info_temp = {0};
                        s_Mode = CH_ED_MODE_NONE;
                        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
                        view_info_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
                        g_AppPlayOps.play_list_create(&view_info_temp);
                        GUI_EndDialog(WND_CH_ED);
                        GUI_SetInterface("flush",NULL);
                        STOP_VIDEO();
                    }
                    else
                    {
					//	app_channel_edit_group_change();

                        if(GUI_CheckDialog("wnd_pop_book") == GXCORE_SUCCESS)
                        {
                            break;//错误43583
                        }
                        progNode.node_type = NODE_PROG;
                        progNode.pos = g_AppPlayOps.normal_play.play_count;
                        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &progNode);
                        memset(buffer_new, 0 ,sizeof(buffer_new));
                        memcpy(buffer_new, progNode.prog_data.prog_name, (sizeof(buffer_new)-1));
                        if((progNode.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE && g_AppFullArb.tip != FULL_STATE_DUMMY)
                              ||(progNode.prog_data.lock_flag != old_prog_lock)
                              ||(strcmp(buffer,buffer_new) != 0))
                        {
                            g_AppPlayOps.normal_play.play_count = sel;//old_playcount ;
                            g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
                            g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT,g_AppPlayOps.normal_play.play_count);
                            app_channel_edit_list_change(widget, usrdata);/*inorder to refresh proginfo*/
                        }
                        channel_edit_cmb_group_build();
                        GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
                        got_focus_sel = sel;//20150105
                        //GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);//20150105
                        channel_edit_show_tip(NULL, NULL, NULL, NULL);

                        app_pop_sort_create(pop_pos, g_AppPlayOps.normal_play.play_count, _ch_ed_pop_sort_exit);
                    }
                    break;
                }

				case STBK_GREEN:
					#if TKGS_SUPPORT
					if(GX_TKGS_AUTOMATIC == app_tkgs_get_operatemode())
					{
						app_tkgs_cantwork_popup();
						break;
					}
					else
					#endif
				    {
				        static PopKeyboard keyboard;
				        static GxMsgProperty_NodeByIdGet node;

						channel_edit_show_tip(NULL, NULL, NULL, NULL);
				        GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
				        node.node_type = NODE_PROG;
				        node.id = g_AppChOps.get_id(sel);
				        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
				        memset(&keyboard, 0, sizeof(PopKeyboard));
				        keyboard.in_name    = (char*)(node.prog_data.prog_name);
				        keyboard.max_num = MAX_PROG_NAME - 1;
				        keyboard.out_name   = NULL;
				        keyboard.change_cb  = NULL;
				        keyboard.release_cb = ch_ed_progname_release_cb;
				        keyboard.usr_data   = NULL;
						keyboard.pos.x = 200;
						keyboard.pos.y = 130;
                        multi_language_keyboard_create(&keyboard);

				        GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
                        got_focus_sel = sel;//20150105
				        //GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);//20150105
				    }
				    break;

				case STBK_YELLOW: //edit pid
				#if TKGS_SUPPORT
				if(GX_TKGS_AUTOMATIC == app_tkgs_get_operatemode())
				{
					app_tkgs_cantwork_popup();
					break;
				}
				else
				#endif
				{
					PopDlgPos pop_pos = {CH_EDIT_POPDLG_POS_X, CH_EDIT_POPDLG_POS_Y};
					uint32_t cur_prog_id = 0;
					GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
                    got_focus_sel = sel;//20150105
					//GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);//20150105
					g_AppChOps.real_pos(&sel);
					channel_edit_show_tip(NULL, NULL, NULL, NULL);

                    GxMsgProperty_NodeByPosGet ProgNode;
                    ProgNode.node_type = NODE_PROG;
                    ProgNode.pos = sel;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);
                    cur_prog_id = ProgNode.prog_data.id;
					app_ch_pid_pop(pop_pos, cur_prog_id, _ch_ed_pop_pid_exit);
					break;
				}
	               	
				default:
					break;
			}
			default:
				break;
	}

	return ret;

}

static void ch_ed_all_select(uint64_t mode)
{
    uint32_t i=0;
    uint32_t sel=0;
    uint32_t flag = 0;

    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);

    for (i=0; i<g_AppPlayOps.normal_play.play_total; i++)
    {
        if (g_AppChOps.check(i, mode) == FALSE)
        {
            flag = 1;
            break;
        }
    }

    if (flag == 1)
    {
        for (i=0; i<g_AppPlayOps.normal_play.play_total; i++)
        {
            if (g_AppChOps.check(i, mode) == FALSE)
                g_AppChOps.set(i, mode);

        }
    }
    else
    {
        for (i=0; i<g_AppPlayOps.normal_play.play_total; i++)
        {
            g_AppChOps.set(i, mode);
        }

    }

    GUI_SetProperty(LISTVIEW_CH_ED, "update_all", NULL);
    GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
	sel =g_AppPlayOps.normal_play.play_count;
	g_AppChOps.real_select(&sel);/*after move, list in channel edit, is differ with play list*/
	GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);

}
int channel_edit_mode_on_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t sel=0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_EXIT:
                case STBK_MENU:
					s_Old_Mode = s_Mode;
                    s_Mode = CH_ED_MODE_NONE;
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_DOWN:
                case STBK_UP:
#if TKGS_SUPPORT
#else
                    // move mode & normal move
                    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
                    if ((s_Mode == CH_ED_MODE_MOVE) && (g_AppChOps.check(sel, MODE_FLAG_MOVING)))
                    {
                        if (event->key.sym == STBK_UP)
                        {
                            if (sel != 0)
                            {
                                g_AppChOps.move(sel,sel-1);
                            }
                            else
                            {
                                g_AppChOps.move(sel,g_AppPlayOps.normal_play.play_total-1);
                            }
                        }
                        else
                        {
                            if (sel != g_AppPlayOps.normal_play.play_total-1)
                            {
                                g_AppChOps.move(sel,sel+1);
                            }
                            else
                            {
                                g_AppChOps.move(sel,0);
                            }
                        }
                        sel =g_AppPlayOps.normal_play.play_count;
                        g_AppChOps.real_select(&sel);/*after move, list in channel edit, is differ with play list*/
                        GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
                    }
                    else
                    {

                    }
#endif
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_PAGE_UP:
                    {
                        uint32_t sel;
                        GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
#if TKGS_SUPPORT
                        if((s_Mode == CH_ED_MODE_SWAP) && (g_AppChOps.check(sel, MODE_FLAG_MOVING)))
#else
                        if((s_Mode == CH_ED_MODE_MOVE) && (g_AppChOps.check(sel, MODE_FLAG_MOVING)))
#endif
                        {
                            if(sel == 0)
                            {
                                g_AppChOps.move(sel,g_AppPlayOps.normal_play.play_total-1);
                                sel = g_AppPlayOps.normal_play.play_total-1;
                            }
                            else if(sel < 7)
                            {
                                g_AppChOps.move(sel,0);
                                sel = 0;
                            }
                            else
                            {
                                g_AppChOps.move(sel,sel-7);
                                sel = sel-7;
                            }
                            GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
                            sel =g_AppPlayOps.normal_play.play_count;
                            g_AppChOps.real_select(&sel);/*after move, list in channel edit, is differ with play list*/
                            GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
                            ret =  EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            if (sel == 0)
                            {
                                sel = app_channel_edit_list_get_total(NULL,NULL)-1;
                                GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
                                ret =   EVENT_TRANSFER_STOP; 
                            }
                            else
                            {
                                ret =  EVENT_TRANSFER_KEEPON; 
                            }
                        }
                    }
                    break;
                case STBK_PAGE_DOWN:
                    {
                        uint32_t sel;
                        GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
#if TKGS_SUPPORT
                        if((s_Mode == CH_ED_MODE_SWAP) && (g_AppChOps.check(sel, MODE_FLAG_MOVING)))
#else
                        if((s_Mode == CH_ED_MODE_MOVE) && (g_AppChOps.check(sel, MODE_FLAG_MOVING)))
#endif
                        {
                            if(sel == g_AppPlayOps.normal_play.play_total-1)
                            {
                                g_AppChOps.move(sel,0);
                                sel = 0;
                            }
                            else if((sel+7) > g_AppPlayOps.normal_play.play_total-1)
                            {
                                g_AppChOps.move(sel,g_AppPlayOps.normal_play.play_total-1);
                                sel = g_AppPlayOps.normal_play.play_total-1;
                            }
                            else
                            {
                                g_AppChOps.move(sel,sel+7);
                                sel = sel+7;

                            }
                            GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
                            sel =g_AppPlayOps.normal_play.play_count;
                            g_AppChOps.real_select(&sel);/*after move, list in channel edit, is differ with play list*/
                            GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
                            ret =  EVENT_TRANSFER_STOP;	
                        }
                        else
                        {
                            if (app_channel_edit_list_get_total(NULL,NULL)
                                    == sel+1)
                            {
                                sel = 0;
                                GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
                                ret = EVENT_TRANSFER_STOP;
                            }
                            else
                            {
                                ret =  EVENT_TRANSFER_KEEPON; 
                            }
                        }
                    }
                    break;
                case STBK_LEFT:
                case STBK_RIGHT:
                    {
#if (MINI_16_BITS_OSD_SUPPORT)
                        s_Old_Mode =s_Mode;
#endif
					   //s_Mode = CH_ED_MODE_NONE;
                       //ch_ed_mode_change(s_Mode);
						ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

                case STBK_1:
                case STBK_2:
                case STBK_3:
                case STBK_4:
                case STBK_5:
                    {
                        if(g_AppChOps.group_check(MODE_FLAG_MOVE) > 0)
                        {
							GUI_GetProperty(LISTVIEW_CH_ED, "active", &sel);
                            g_AppChOps.group_clear(MODE_FLAG_MOVE);
                            GUI_SetProperty(LISTVIEW_CH_ED, "update_all", NULL);
							GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
                        }
                        else
                        {
                            GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
                            if(g_AppChOps.check(sel, MODE_FLAG_MOVING) == true)
                            {
                                g_AppChOps.clear(sel, MODE_FLAG_MOVING);
                                GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                            }
                        }
#if (MINI_16_BITS_OSD_SUPPORT)
                        s_Old_Mode =s_Mode;
#endif
                        s_Mode = CH_ED_MODE_NONE;
                        ch_ed_mode_change(s_Mode);

                        ret = EVENT_TRANSFER_STOP;
                        break;
                    }    

                case STBK_OK:

                    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);

                    switch (s_Mode)
                    {
                        case CH_ED_MODE_DEL:
                            g_AppChOps.set(sel, MODE_FLAG_DEL);
                            GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                            break;
                        case CH_ED_MODE_LOCK:
                            g_AppChOps.set(sel, MODE_FLAG_LOCK);
                            GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                            break;
                        case CH_ED_MODE_SKIP:
                            g_AppChOps.set(sel, MODE_FLAG_SKIP);
                            GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                            break;
#if TKGS_SUPPORT
                        case CH_ED_MODE_SWAP:
                            {
                                if(g_AppChOps.group_check(MODE_FLAG_MOVE) > 0&&selbackuppos!=0xffffffff)
                                {
                                    g_AppChOps.move(selbackuppos,sel);
                                    g_AppChOps.move(sel-1,selbackuppos);
                                    g_AppChOps.clear(sel, MODE_FLAG_MOVING);
                                    g_AppChOps.group_clear(MODE_FLAG_MOVE);
                                    if(g_AppChOps.check(sel, MODE_FLAG_MOVING) == false)
                                    {
                                        channel_edit_show_tip(NULL, NULL,NULL, NULL);
                                        GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "string", "Select");
                                        GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
                                    }
                                    else
                                    {
                                        channel_edit_show_tip(NULL, NULL ,NULL, NULL);
                                        GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "string", "Move");
                                        GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
                                    }
                                    GUI_SetProperty(LISTVIEW_CH_ED, "update_all", NULL);
                                    GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
                                    selbackuppos=0xffffffff;
                                }
                                else
                                {
                                    g_AppChOps.set(sel, MODE_FLAG_MOVE);
                                    selbackuppos = sel;
                                    channel_edit_show_tip(NULL,NULL,NULL,NULL);
                                    GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
                                    GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
                                    GUI_SetProperty("text_ch_edit_tip_ok", "string", "Swap");
                                    GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
                                    GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
                                    GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
                                    GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
                                    GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                                }

                                break;
                            }
#else
                        case CH_ED_MODE_MOVE:
                            {
                                if(g_AppChOps.group_check(MODE_FLAG_MOVE) > 0)
                                {
                                    g_AppChOps.group_move(sel);
                                    if(g_AppChOps.check(sel, MODE_FLAG_MOVING) == false)
                                    {
                                        channel_edit_show_tip(NULL, STR_ID_SELECT_GROUP_CHANNELS,NULL, NULL);
                                        GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "string", "Select");
                                        GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
                                    }
                                    else
                                    {
                                        channel_edit_show_tip(NULL, NULL ,NULL, NULL);
                                        GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "string", "Move");
                                        GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
                                    }

                                    GUI_SetProperty(LISTVIEW_CH_ED, "update_all", NULL);
                                    GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
                                    sel =g_AppPlayOps.normal_play.play_count;
                                    g_AppChOps.real_select(&sel);/*after move, list in channel edit, is differ with play list*/
                                    GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
                                }         
                                else
                                {

                                    g_AppChOps.set(sel, MODE_FLAG_MOVING);
                                    if(g_AppChOps.check(sel, MODE_FLAG_MOVING) == false)
                                    {
                                        channel_edit_show_tip(NULL, STR_ID_SELECT_GROUP_CHANNELS,NULL, NULL);
                                        GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "string", "Select");
                                        GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
                                    }
                                    else
                                    {
                                        channel_edit_show_tip(NULL,NULL,NULL,NULL);
                                        GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
                                        GUI_SetProperty("text_ch_edit_tip_ok", "string", "Move");
                                        GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
                                        GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
                                        GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");
                                    }

                                    GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                                }

                                break;
                            }    
#endif
                        case CH_ED_MODE_FAV:
                            s_ListSel = sel;
                            //GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);//20150105
                            got_focus_sel = sel;//20150105
                            GUI_CreateDialog(WND_POP_FAV);
                            break;
                        default:
                            break;
                    }
                    break;

                case STBK_RED:

                    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);

                    switch (s_Mode)
                    {
                        case CH_ED_MODE_DEL:
                            ch_ed_all_select(MODE_FLAG_DEL);
                            break;
                        case CH_ED_MODE_LOCK:
                            ch_ed_all_select(MODE_FLAG_LOCK);
                            break;
                        case CH_ED_MODE_SKIP:
                            ch_ed_all_select(MODE_FLAG_SKIP);
                            break;
                        default:
                            break;
                    }
                    break;
#if TKGS_SUPPORT 
#else
                case STBK_GREEN:
                    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
                    if((s_Mode == CH_ED_MODE_MOVE)
                            && (g_AppChOps.check(sel, MODE_FLAG_MOVING) == false))
                    {
                        g_AppChOps.set(sel, MODE_FLAG_MOVE);
                        channel_edit_show_tip(NULL,STR_ID_SELECT_GROUP_CHANNELS,NULL,NULL);
                        GUI_SetProperty("img_ch_edit_tip_ok", "state", "show");
                        GUI_SetProperty("text_ch_edit_tip_ok", "state", "show");
                        GUI_SetProperty("text_ch_edit_tip_ok", "string", "Move");
                        GUI_SetProperty("img_channel_edit_tip_sat", "state", "hide");
                        GUI_SetProperty("txt_channel_edit_tip_sat", "state", "hide");
                        GUI_SetProperty("img_channel_edit_tip_ok", "state", "hide");
                        GUI_SetProperty("txt_channel_edit_tip_ok", "state", "hide");

                        GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
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

SIGNAL_HANDLER int app_channel_edit_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;

    if (s_Mode == CH_ED_MODE_NONE)
    {
        ret = channel_edit_mode_off_keypress(widget, usrdata);
    }
    else
    {
        ret = channel_edit_mode_on_keypress(widget, usrdata);
    }

    return ret;
}

static int ch_ed_timeout_cb(void* usrdata)
{
    char time_str[24] = {0};
    char time_str_temp[6] = {0};

    g_AppTime.time_get(&g_AppTime, time_str, sizeof(time_str)); 

	// time
	if(strlen(time_str) > 24)
	{
		printf("\nlenth error\n");
		return 1;
	}
	memcpy(time_str_temp,&time_str[strlen(time_str)-5],5);// 00:00
	GUI_SetProperty("text_channel_edit_time", "string", time_str_temp);

	// ymd
	time_str[strlen(time_str)-5] = '\0';
	GUI_SetProperty("text_channel_edit_ymd", "string", time_str);

    return 0;
}

// signal channel edit
SIGNAL_HANDLER int app_channel_edit_create(GuiWidget *widget, void *usrdata)
{
	uint32_t sel = 0;
	int init_value = 0;

	app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

	//  g_AppFullArb.timer_start();
	 app_lang_set_menu_font_type("img_channel_edit_title");
    GUI_SetFocusWidget(LISTVIEW_CH_ED);

    sp_GpIdOps = new_id_ops();

    if (sp_GpIdOps == NULL)
        return GXCORE_ERROR;

    channel_edit_cmb_group_build();

    sel = channel_edit_get_cur_group_num();

	GUI_SetProperty(GROUP_CH_ED, "select", &sel);
	app_channel_edit_group_change();

//modify 20141224
	sel = g_AppPlayOps.normal_play.play_count;
	GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
	GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);

    // inorder show proginfo
    app_channel_edit_list_change(widget, usrdata);

    // CH ED TIMER
    if (reset_timer(sp_ChEdTimer) != 0) 
    {
        sp_ChEdTimer = create_timer(ch_ed_timeout_cb, 
                TIME_PRECISION*SEC_TO_MILLISEC, NULL, TIMER_REPEAT);
    }
    // first show 
    ch_ed_timeout_cb(NULL);
    s_exit_flag = false;
    s_save_flag= false;

    START_VIDEO();
   g_AppFullArb.timer_start();// 20121108 处理由暗变亮过程
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_edit_destroy(GuiWidget *widget, void *usrdata)
{
    int init_value = 0;

    del_id_ops(sp_GpIdOps);
    sp_GpIdOps = NULL;

   // STOP_VIDEO();

    // CH ED TIMER
    remove_timer(sp_ChEdTimer);
    sp_ChEdTimer = NULL;
    GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);
    g_AppFullArb.timer_stop();
    g_AppBook.invalid_remove();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_edit_show(GuiWidget *widget, void *usrdata)
{
    //GUI_SetInterface("video_top", NULL);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_edit_got_focus(GuiWidget *widget, void *usrdata)
{
    //int32_t unactive = -1;
    //GUI_SetProperty(LISTVIEW_CH_ED, "active", &unactive);
    // channel_edit_show can't do draw
    if (s_Mode == CH_ED_MODE_FAV)
    {
        channel_edit_show_tip(NULL, NULL, NULL, NULL);
    }
    else
    {
        channel_edit_show_tip(STR_ID_SORT, STR_ID_RENAME, STR_ID_PID, STR_ID_MODE);
    }
    //GUI_SetProperty(LISTVIEW_CH_ED, "select", &got_focus_sel);//20150105


    return EVENT_TRANSFER_STOP;
}

#if (DLNA_SUPPORT > 0)
static void app_chedit_start_dlna_cb(void)
{
    g_AppFullArb.timer_stop();
    STOP_VIDEO();
}

static void app_chedit_stop_dlna_cb(void)
{
    START_VIDEO();
    g_AppFullArb.timer_start();
}
#endif

SIGNAL_HANDLER int app_channel_edit_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	GxBusPmViewInfo  view_info_temp = {0};

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					s_Mode = CH_ED_MODE_NONE;
					STOP_VIDEO();
					GUI_EndDialog("after wnd_full_screen");
					s_ch_ed_fullscreen = false;
					break;
#if (DLNA_SUPPORT > 0)
                case VK_DLNA_TRIGGER:
                    app_dlna_ui_exec(app_chedit_start_dlna_cb, app_chedit_stop_dlna_cb);
                    break;
#endif
				case STBK_EXIT:
				case STBK_MENU:
				{
					//extern StreamState g_OldStreamState;
					bool verity_flag = false;
				
					if(s_exit_flag == true)
					{
						break;
					}
				
					POP_SAVE();
					
					if(GUI_CheckDialog("wnd_pop_book") == GXCORE_SUCCESS)
					{
						break;//错误43583
					}

					s_exit_flag = true;
					s_VideoStart = 0;

					if(g_AppChOps.verity() == true)
					{
						verity_flag = true;
					}

					if(g_AppPlayOps.normal_play.play_total == 0 && g_AppFullArb.state.mute == STATE_ON)
					{
						g_AppFullArb.state.mute = STATE_OFF;
					}
					// recover view mode & list, without skip prog
					app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
					view_info_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
					g_AppPlayOps.play_list_create(&view_info_temp);

					//g_AppFullArb.state.tv = g_OldStreamState.old_tv_state;
					//g_AppPlayOps.normal_play.view_info = g_OldStreamState.old_view_info;
                    if(verity_flag == true)
                    {
#if RECALL_LIST_SUPPORT
                        g_AppPlayOps.recall_play[0] = g_AppPlayOps.normal_play;//g_recall_count
#else
                        g_AppPlayOps.recall_play = g_AppPlayOps.normal_play; //lost recall
						g_AppPlayOps.current_play.play_count = g_AppPlayOps.normal_play.play_count;
#endif
                    }
#if RECALL_LIST_SUPPORT == 0
                    if(g_AppPlayOps.normal_play.play_count != g_AppPlayOps.current_play.play_count)
                    {
                        g_AppPlayOps.recall_play.play_count = g_AppPlayOps.normal_play.play_count;
                    }
#endif
					if(s_ch_ed_fullscreen == true)
					{
						g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
						g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
					}
					#if (MINI_256_COLORS_OSD_SUPPORT)
					STOP_VIDEO();
					GUI_EndDialog(WND_CH_ED);
				//	GUI_SetInterface("flush",NULL);
					#else
					GUI_EndDialog(WND_CH_ED);
					//GUI_GetProperty(WND_CH_ED, "draw_now", NULL);

				//	GUI_SetInterface("flush",NULL);
					STOP_VIDEO();
					#endif
					if(s_save_flag == TRUE)
					channel_edit_change_prog_state();
					
					if(s_ch_ed_fullscreen == true)
					{
						s_ch_ed_fullscreen = false;

						if(g_AppPlayOps.normal_play.play_total == 0)
						{
							g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
						}
						
						if (g_AppPlayOps.normal_play.play_total != 0)
						{
							
							// deal for recall , current will copy to recall
							#if RECALL_LIST_SUPPORT
							//extern int g_recall_count;
							g_AppPlayOps.current_play = g_AppPlayOps.recall_play[0];//g_recall_count
							#else
							g_AppPlayOps.current_play = g_AppPlayOps.recall_play;
							#endif
							g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT,
														g_AppPlayOps.normal_play.play_count);
							if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
								g_AppFullArb.tip = FULL_STATE_LOCKED;
							else
								g_AppFullArb.tip = FULL_STATE_DUMMY;

							/*if(GUI_CheckDialog("wnd_passwd") != GXCORE_SUCCESS)
							{
								GUI_CreateDialog("wnd_channel_info");
							}*/
						}
					}
				}
				break;

                case STBK_LEFT:
                case STBK_RIGHT:
                {
                    uint32_t cmb_sel = 0;
                    uint32_t cmb_num_bak = 0;
                    uint32_t cmb_num = 0;

                    cmb_num_bak  = sp_GpIdOps->id_total_get(sp_GpIdOps);
                    GUI_GetProperty(GROUP_CH_ED, "select", &cmb_sel);
                    if (false == POP_SAVE())
                    {
                        s_Mode = CH_ED_MODE_NONE;
						//ch_ed_mode_change(s_Mode);
                        // recover view mode & list, without skip prog
                        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
                        view_info_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
                        g_AppPlayOps.play_list_create(&view_info_temp);

                        GUI_EndDialog(WND_CH_ED);	
                        GUI_SetInterface("flush",NULL);
                        STOP_VIDEO();
                    }
                    else
                    {
						if(GUI_CheckDialog("wnd_pop_book") == GXCORE_SUCCESS)
						{
							break;//错误43583
						}
						channel_edit_cmb_group_build();
                        s_Mode = CH_ED_MODE_NONE;
                        ch_ed_mode_change(s_Mode);

                        cmb_num = sp_GpIdOps->id_total_get(sp_GpIdOps);
                        if(event->key.sym == STBK_LEFT)
                        {
                            if(cmb_sel >= cmb_num)
                            {
                                cmb_sel = cmb_num - 1;
                            }
                            else
                            {
                                (cmb_sel == 0) ? (cmb_sel = cmb_num - 1):(cmb_sel--);
                            }
                        }
                        else
                        {
                            if(cmb_num >= cmb_num_bak)
                            {
                                cmb_sel++;
                            }
                            if(cmb_sel >= cmb_num)
                            {
                                cmb_sel = 0;
                            }
                        }

                        GUI_SetProperty(GROUP_CH_ED, "select", &cmb_sel);
                        app_channel_edit_group_change();
                        // inorder show proginfo
                        app_channel_edit_list_change(widget, usrdata);
			
                    }
                    break;
                }    
				default:
					break;
			}
		default:
			break;
	}

    return EVENT_TRANSFER_STOP;
}

static void _create_ch_edit(void)
{
     g_AppFullArb.timer_stop();
     g_AppFullArb.state.pause = STATE_OFF;
     g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
     g_AppPlayOps.program_stop();
	 //START_VIDEO();//temp  
     GUI_CreateDialog("wnd_channel_edit");

     s_ch_ed_fullscreen = true;
}

int _ch_edit_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
	if(ret->result ==  PASSWD_OK)
	{	
		_create_ch_edit();
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
		PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_ERR_PASSWD;

        if(GUI_CheckDialog("wnd_channel_edit") == GXCORE_SUCCESS)
        {
            pop.pos.x = CH_EDIT_POPDLG_POS_X;
            pop.pos.y = CH_EDIT_POPDLG_POS_Y;
        }
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        popdlg_create(&pop);
    }
	else
    	{}

	return 0;	
}

static int _ch_edit_pvr_pop_cb(PopDlgRet ret)
{
    if(g_AppFullArb.state.tv == STATE_OFF)
    {
        if(0 == strcasecmp("wnd_full_screen", GUI_GetFocusWindow()))
            GUI_CreateDialog("wnd_channel_info");
    }
    return 0;
}

void app_create_ch_edit_fullsreeen(void)
{
    if (g_AppPlayOps.normal_play.play_total != 0)
    {
        if (g_AppPvrOps.state != PVR_DUMMY)
        {
            PopDlg pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_STOP_PVR_FIRST;
            pop.mode = POP_MODE_UNBLOCK;
            pop.timeout_sec = 3;
            pop.exit_cb = _ch_edit_pvr_pop_cb;
            popdlg_create(&pop);    
        }
        else
        {
            int init_value = 0;
            GxBus_ConfigGetInt(MENU_LOCK_KEY, &init_value, MENU_LOCK);
            if(init_value == 1)
            {
                PasswdDlg passwd_dlg;
                memset(&passwd_dlg, 0, sizeof(PasswdDlg));
                passwd_dlg.passwd_exit_cb = _ch_edit_passwd_dlg_cb;
                if(GUI_CheckDialog("wnd_channel_edit") == GXCORE_SUCCESS)
                {
                    passwd_dlg.pos.x = CH_EDIT_POPDLG_POS_X;
                    passwd_dlg.pos.y = CH_EDIT_POPDLG_POS_Y;
                }

                passwd_dlg_create(&passwd_dlg);
            }
            else
            {
                _create_ch_edit();
            }
        }
     }
}

#define POP_FAV______
SIGNAL_HANDLER int app_pop_fav_list_create(GuiWidget *widget, void *usrdata)
{
    channel_edit_show_tip(NULL, STR_ID_RENAME, NULL, NULL);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_fav_list_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_fav_keypress(GuiWidget *widget, void *usrdata)
{

	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					s_Mode = CH_ED_MODE_NONE;
					STOP_VIDEO();
					GUI_EndDialog("after wnd_full_screen");
					s_ch_ed_fullscreen = false;
					break;
				default:
					break;
			}
		default:
			break;
	}
	return ret;
}

SIGNAL_HANDLER int app_pop_fav_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint32_t sel=0;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			//switch(event->key.sym)
			{
				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(WND_POP_FAV);
					if(s_favname_flag == true)
					{
						uint32_t sel = 0;
						
						s_favname_flag = false;
						
						GUI_GetProperty(GROUP_CH_ED,"select",&sel);
						channel_edit_cmb_group_build();
						GUI_SetProperty(GROUP_CH_ED,"select",&sel);
					}
					break;
                case STBK_UP:
                case STBK_DOWN:
					{
						ret = EVENT_TRANSFER_KEEPON;
					}
					break;
		        case STBK_PAGE_UP:
			        {
				        uint32_t sel;
				        GUI_GetProperty(LISTVIEW_POP_FAV, "select", &sel);
				        if (sel == 0)
				        {
					        sel = app_pop_fav_list_get_total(NULL,NULL)-1;
					        GUI_SetProperty(LISTVIEW_POP_FAV, "select", &sel);
					        ret =   EVENT_TRANSFER_STOP; 
				        }
				        else
				        {
				        	ret =  EVENT_TRANSFER_KEEPON; 
				        }
			        }
			        break;
                case STBK_PAGE_DOWN:
			        {
				        uint32_t sel;
				        GUI_GetProperty(LISTVIEW_POP_FAV, "select", &sel);
				        if (app_pop_fav_list_get_total(NULL,NULL)
				        == sel+1)
				        {
					        sel = 0;
					        GUI_SetProperty(LISTVIEW_POP_FAV, "select", &sel);
                            ret = EVENT_TRANSFER_STOP;
				        }
				        else
				        {
				        	ret =  EVENT_TRANSFER_KEEPON; 
				        }
                    }
                        break;
                case STBK_OK:
                    GUI_GetProperty(LISTVIEW_POP_FAV, "select", &sel);
#if TKGS_SUPPORT
					g_AppChOps.set(s_ListSel, ((uint64_t)1<<(sel+9)));
#else
                    g_AppChOps.set(s_ListSel, ((uint64_t)1<<sel));
#endif
                    GUI_SetProperty(LISTVIEW_POP_FAV, "update_row", &sel);
                    break;
                case STBK_GREEN:
                    {
                        static PopKeyboard keyboard;
                        static GxMsgProperty_NodeByPosGet node;

                        GUI_GetProperty(LISTVIEW_POP_FAV, "select", &sel);
#if TKGS_SUPPORT
						sel += 9;
#endif						
                        node.node_type = NODE_FAV_GROUP;
                        node.pos = sel;
                        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

                        memset(&keyboard, 0, sizeof(PopKeyboard));
                        keyboard.in_name    = (char*)(node.fav_item.fav_name);
                        keyboard.max_num = MAX_FAV_GROUP_NAME - 1;
                        keyboard.out_name   = NULL;
                        keyboard.change_cb  = NULL;
                        keyboard.release_cb = ch_ed_favname_release_cb;
                        keyboard.usr_data   = KEY_LAN_INVALID_CHAR_NAME;
						keyboard.pos.x = 200; 
						keyboard.pos.y = 130;
                        multi_language_keyboard_create(&keyboard);
                    }
                    break;
	            }
    }

    return ret;
}


SIGNAL_HANDLER int app_pop_fav_list_get_total(GuiWidget *widget, void *usrdata)
{
    GxMsgProperty_NodeNumGet node = {0};
    
    node.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node));
#if TKGS_SUPPORT
	if (node.node_num>9)
	{
		node.node_num -= 9;
	}
#endif

    return node.node_num;
}


SIGNAL_HANDLER int app_pop_fav_list_get_data(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_NodeByPosGet node = {0};
	ListItemPara* item = NULL;
    static char fav_name[MAX_FAV_GROUP_NAME];

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	return GXCORE_ERROR;
	if(0 > item->sel)   return GXCORE_ERROR;

	//col-0: num
	item->x_offset = 0;
    item->image = NULL;
    // 2nd param: sel the same value with MODE_FLAG_FAV*
    #if TKGS_SUPPORT
	if (g_AppChOps.check(s_ListSel, (uint64_t)1<<(item->sel+9)))
	#else
    if (g_AppChOps.check(s_ListSel, (uint64_t)1<<item->sel))
	#endif	
	    item->image = "s_choice.bmp";
	item->string = NULL;
	
	//col-1: channel name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	node.node_type = NODE_FAV_GROUP;
	node.pos = item->sel;
	#if TKGS_SUPPORT
	node.pos += 9;
	#endif
	//get node
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
    strcpy(fav_name, (char*)node.fav_item.fav_name);
    item->string = fav_name;

    return GXCORE_SUCCESS;
}

#define POP_SORT______

static char *s_SortMode[SORT_MODE_TOTAL] = {
    "Name(A-Z)","Name(Z-A)",STR_ID_TP,STR_ID_FREE_SCRAMBLE,STR_ID_FAV_UNFAV,STR_ID_LOCK_UNLOCK,"HD/SD"};

static void (*s_pop_sort_exit)(PopDlgRet,uint32_t);
static uint32_t s_cur_prog_pos = ~(uint32_t)0;

SIGNAL_HANDLER int app_pop_sort_list_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_sort_list_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

static int s_sort_mode = -1;
static int _sort_wait_pop_create(void)
{
    g_AppChOps.sort((sort_mode)s_sort_mode, &s_cur_prog_pos);
    g_AppChOps.save();
    return 0;
}

SIGNAL_HANDLER int app_pop_sort_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	//uint32_t mode =0;
	//PopDlg  pop;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(WND_POP_SORT);
					if(s_pop_sort_exit != NULL)
					{
						(*s_pop_sort_exit)(POP_VAL_CANCEL, s_cur_prog_pos);
					}
					break;

				case STBK_OK:
					GUI_GetProperty("listview_pop_sort", "select", &s_sort_mode);
					//memset(&pop, 0, sizeof(PopDlg));
					//pop.type = POP_TYPE_NO_BTN;
					//pop.str = STR_ID_WAITING;
					//pop.creat_cb = _sort_wait_pop_create;
					//popdlg_create(&pop);
					_sort_wait_pop_create();
					GUI_EndDialog(WND_POP_SORT);
					//GUI_SetInterface("flush",NULL);
					if(s_pop_sort_exit != NULL)
                    {
                        (*s_pop_sort_exit)(POP_VAL_OK, s_cur_prog_pos);
                    }

                    break;

				case STBK_UP:
				case STBK_DOWN:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				default:
					break;
			}
	}

	return ret;
}

SIGNAL_HANDLER int app_pop_sort_list_get_total(GuiWidget *widget, void *usrdata)
{
    return SORT_MODE_TOTAL;
}

SIGNAL_HANDLER int app_pop_sort_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	return GXCORE_ERROR;
	if(0 > item->sel)   return GXCORE_ERROR;

	//col-0: num
	item->x_offset = 0;
    item->image = NULL;
	item->string = NULL;
	
	//col-1:sort mode 
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
    item->string = s_SortMode[item->sel];

    return GXCORE_SUCCESS;
}

void app_pop_sort_create(PopDlgPos pop_pos, uint32_t prog_pos, void (*exit_cb)(PopDlgRet,uint32_t))
{
	uint32_t listview_sel = ~(uint32_t)0;

	g_AppChOps.real_select(&listview_sel);
	s_cur_prog_pos = prog_pos;
	s_pop_sort_exit = exit_cb;
	g_AppChOps.create(g_AppPlayOps.normal_play.play_total);
	GUI_CreateDialog(WND_POP_SORT);

	if(pop_pos.x == 0 && pop_pos.y == 0)
	{
		#if(MINI_16_BITS_OSD_SUPPORT == 0)
		pop_pos.x = 390;
		pop_pos.y = 120;
		#else
		pop_pos.x = 280;
		pop_pos.y = 120;
		#endif
	}

	GUI_SetProperty(WND_POP_SORT, "move_window_x", &(pop_pos.x));
	GUI_SetProperty(WND_POP_SORT, "move_window_y", &(pop_pos.y));
}


#define POP_CH_PID______
#define WND_POP_CH_PID "wnd_pop_ch_pid"
#define EDIT_V "edit_ch_pid_v"
#define EDIT_A "edit_ch_pid_a"
#define EDIT_PCR "edit_ch_pid_pcr"

#define MAX_PID_LEN (4)

static void (*s_pop_ch_pid_exit)(PopDlgRet,uint32_t,uint32_t,uint32_t);
static uint32_t s_ch_pid_v = ~(uint32_t)0;
static uint32_t s_ch_pid_a = ~(uint32_t)0;
static uint32_t s_ch_pid_pcr = ~(uint32_t)0;

SIGNAL_HANDLER int app_ch_pid_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case STBK_UP:
					{
						uint32_t sel;
						GUI_GetProperty("box_pop_ch_pid_opt", "select", &sel);
						if(sel > 1)
							sel--;
						else if(sel ==0)
							sel = 3;
						else
						{
							if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
								sel = 3;
							else
								sel = 0;
						}
						GUI_SetProperty("box_pop_ch_pid_opt", "select", &sel);
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				case STBK_DOWN:
					{
						uint32_t sel;
						GUI_GetProperty("box_pop_ch_pid_opt", "select", &sel);
						if(sel < 3)
							sel++;
						else
						{
							if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
								sel = 1;
							else
								sel = 0;
						}
						GUI_SetProperty("box_pop_ch_pid_opt", "select", &sel);
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				default:
					break;
			}
	}

	return ret;
}

SIGNAL_HANDLER int app_pop_ch_pid_create(GuiWidget *widget, void *usrdata)
{
	char pid_buff[MAX_PID_LEN + 1] = {0};
	
	sprintf(pid_buff, "%04d", s_ch_pid_v);
	GUI_SetProperty(EDIT_V, "string", pid_buff);

	memset(pid_buff, 0, MAX_PID_LEN);
	sprintf(pid_buff, "%04d", s_ch_pid_a);
	GUI_SetProperty(EDIT_A, "string", pid_buff);

	memset(pid_buff, 0, MAX_PID_LEN);
	sprintf(pid_buff, "%04d", s_ch_pid_pcr);
	GUI_SetProperty(EDIT_PCR, "string", pid_buff);

	//box部件存在问题，第一个boxitem不能disable，采用此补丁方式
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		uint32_t sel = 1;

		GUI_SetProperty(EDIT_V, "forecolor", "[#424247,#424247,#424247]");
		GUI_SetProperty("text_ch_pid_v", "forecolor", "[#424247,#424247,#424247]");
		GUI_SetProperty("box_pop_ch_pid_opt", "select", &sel);
	}
	else
	{		
		GUI_SetProperty(EDIT_V, "forecolor", "[pop_text_color,pop_text_focus_color,pop_text_color]");
		GUI_SetProperty("text_ch_pid_v", "forecolor", "[pop_tag_color,pop_tag_focus_color,pop_tag_color]");
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_ch_pid_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_ch_pid_keypress(GuiWidget *widget, void *usrdata)
{
	char *pid_str = NULL;
	uint32_t box_sel;
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					s_Mode = CH_ED_MODE_NONE;
					STOP_VIDEO();
					GUI_EndDialog("after wnd_full_screen");
					s_ch_ed_fullscreen = false;

					break;
				case STBK_EXIT:
				case STBK_MENU:
					GUI_GetProperty(EDIT_V, "string", &pid_str);
					if(pid_str != NULL)
					{
						s_ch_pid_v = atoi(pid_str);
						pid_str = NULL;
					}

					GUI_GetProperty(EDIT_A, "string", &pid_str);
					if(pid_str != NULL)
					{
						s_ch_pid_a = atoi(pid_str);
						pid_str = NULL;
					}

					GUI_GetProperty(EDIT_PCR, "string", &pid_str);
					if(pid_str != NULL)
					{
						s_ch_pid_pcr = atoi(pid_str);
						pid_str = NULL;
					}
					GUI_EndDialog(WND_POP_CH_PID);
					if(s_pop_ch_pid_exit != NULL)
					{
						(*s_pop_ch_pid_exit)(POP_VAL_CANCEL, s_ch_pid_v, s_ch_pid_a, s_ch_pid_pcr);
					}
					break;

				case STBK_OK:
					GUI_GetProperty("box_pop_ch_pid_opt", "select", &box_sel);
					if(box_sel == 3) //save item
					{
						uint32_t ch_pid_v = 0;
                                                uint32_t ch_pid_a = 0;
                                                uint32_t ch_pid_pcr = 0;
                                                bool invalid_flag = false;
						GUI_GetProperty(EDIT_V, "string", &pid_str);
						if(pid_str != NULL)
						{
							ch_pid_v = atoi(pid_str);
							pid_str = NULL;
						}

						GUI_GetProperty(EDIT_A, "string", &pid_str);
						if(pid_str != NULL)
						{
							ch_pid_a = atoi(pid_str);
							pid_str = NULL;
						}

						GUI_GetProperty(EDIT_PCR, "string", &pid_str);
						if(pid_str != NULL)
						{
							ch_pid_pcr = atoi(pid_str);
							pid_str = NULL;
						}
						//GUI_EndDialog(WND_POP_CH_PID);
					
						if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
						{
							if( (ch_pid_v > 0 && ch_pid_v < 0x1fff)\
									&&(ch_pid_a > 0 && ch_pid_a < 0x1fff )\
									&&(ch_pid_pcr > 0 && ch_pid_pcr < 0x1fff) )
							{
								if(s_ch_pid_v != ch_pid_v || s_ch_pid_a != ch_pid_a || s_ch_pid_pcr != ch_pid_pcr)
								{
									invalid_flag = true;
								}
								else
								{
									GUI_EndDialog(WND_POP_CH_PID);
									if(s_pop_ch_pid_exit != NULL)
									{
										(*s_pop_ch_pid_exit)(POP_VAL_CANCEL, s_ch_pid_v, s_ch_pid_a, s_ch_pid_pcr);
									}
									break;
								}
							}
						}
						else //radio
						{
							if( (ch_pid_a > 0 && ch_pid_a < 0x1fff )\
									&&(ch_pid_pcr > 0 && ch_pid_pcr < 0x1fff) )
							{
								if(s_ch_pid_a != ch_pid_a || s_ch_pid_pcr != ch_pid_pcr)
								{
									invalid_flag = true;
								}
								else
								{
									GUI_EndDialog(WND_POP_CH_PID);
									if(s_pop_ch_pid_exit != NULL)
									{
										(*s_pop_ch_pid_exit)(POP_VAL_CANCEL, s_ch_pid_v, s_ch_pid_a, s_ch_pid_pcr);
									}
									break;
								}
							}
						}

						if(invalid_flag == true && ch_pid_v != ch_pid_a)
						{
							s_ch_pid_v = ch_pid_v;
							s_ch_pid_a = ch_pid_a;
							s_ch_pid_pcr = ch_pid_pcr;
							GUI_EndDialog(WND_POP_CH_PID);
							if(s_pop_ch_pid_exit != NULL)
							{
								(*s_pop_ch_pid_exit)(POP_VAL_OK, s_ch_pid_v, s_ch_pid_a, s_ch_pid_pcr);
							}
						}
						else /*pid invalid,pop a dailog*/
						{
							PopDlg  pop;
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_OK;
							pop.mode = POP_MODE_UNBLOCK;
							pop.str = STR_ID_INVALID_PID;
							pop.pos.x = CH_EDIT_POPDLG_POS_X;
							pop.pos.y = CH_EDIT_POPDLG_POS_Y;
							pop.exit_cb = NULL;
							char pid_buff[MAX_PID_LEN + 1] = {0};

							if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
							{
								sprintf(pid_buff, "%04d", s_ch_pid_v);
								GUI_SetProperty(EDIT_V, "string", pid_buff);
							}

							memset(pid_buff, 0, MAX_PID_LEN);
							sprintf(pid_buff, "%04d", s_ch_pid_a);
							GUI_SetProperty(EDIT_A, "string", pid_buff);

							memset(pid_buff, 0, MAX_PID_LEN);
							sprintf(pid_buff, "%04d", s_ch_pid_pcr);
							GUI_SetProperty(EDIT_PCR, "string", pid_buff);

							popdlg_create(&pop);
							int init_value = 0;
							GUI_SetProperty(EDIT_A, "select", &init_value);
							GUI_SetProperty(EDIT_V, "select", &init_value);
							GUI_SetProperty(EDIT_PCR, "select",&init_value);
						}

					}

					break;	

				case STBK_UP:
				case STBK_DOWN:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				default:
					break;
			}
	}

	return ret;
}

void app_ch_pid_pop(PopDlgPos pop_pos, uint32_t prog_id, void (*exit_cb)(PopDlgRet,uint32_t,uint32_t,uint32_t))
{
#if (MINI_16_BITS_OSD_SUPPORT == 1)
#define POP_X    320
#define POP_Y    152
#else
#define POP_X    400
#define POP_Y    190
#endif

	if(prog_id == 0)
		return;

	GxMsgProperty_NodeByIdGet prog_node = {0};
	prog_node.node_type = NODE_PROG;
	prog_node.id = prog_id;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&prog_node)))
    {
        s_ch_pid_v = prog_node.prog_data.video_pid;
        s_ch_pid_a = prog_node.prog_data.cur_audio_pid;
        s_ch_pid_pcr = prog_node.prog_data.pcr_pid;
        s_pop_ch_pid_exit = exit_cb;

        GUI_CreateDialog(WND_POP_CH_PID);

        if(pop_pos.x == 0 && pop_pos.y == 0)
        {
            pop_pos.x = POP_X;
            pop_pos.y = POP_Y;
        }

        GUI_SetProperty(WND_POP_CH_PID, "move_window_x", &(pop_pos.x));
		GUI_SetProperty(WND_POP_CH_PID, "move_window_y", &(pop_pos.y));
	}
}

