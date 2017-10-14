//#include "app_root.h"
#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "full_screen.h"
#include "app_default_params.h"
#include "app_book.h"
#include "app_config.h"
#include "youtube_tools.h"
#if TKGS_SUPPORT
#include "module/gxtkgs.h"
#include "app_tkgs_upgrade.h"
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
extern void app_tkgs_upgrade_menu_exec(void);
#endif

#if CA_SUPPORT
#if EX_SHIFT_SUPPORT
#define	SPECIAL_SETTING_SUPPORT
#endif
#endif

#if MINI_16BIT_WIN8_OSD_SUPPORT
/**  menu sub list  **/
#define WND_MENU_SUB_LIST    "wnd_menu_sub_list"
#define TITLE_MENU_LIST      "text_menu_sub_list_title"
#define LISTVIEW_MENU_LIST   "list_menu_sub_list"

/**  menu sub list  **/
typedef struct
{   
    char *title;
    int item_num;
    char **item_content;
	void (*PressOK_cb)(int sel);
}MenuSubList;
static MenuSubList *s_menulist = NULL;

/** system list xml **/
#define WND_SYS_LIST    	  "wnd_system_list"
#define LISTVIEW_SYS_LIST     "list_system_list"
#define LISTVIEW_SYS_LIST_SUB "list_system_list_sub"

typedef enum
{
	MS_INFO = 50,
	MS_SYSTEM,
	MS_SETTING,
	MS_TOTAL,
}SystemListId;

typedef enum
{
	MM_SYSINFO = 60,
}SysInfoId;

typedef enum
{
	MM_TIME_SETTING = 70,
    MM_UPGRADE,
    MM_RESET,
}SystemId;

typedef enum
{
	MM_TIMER_SETTING = 80,
	MM_LANGUAGE,
	MM_AV_OUTPUT,
	MM_PARENTAL_LOCK,
	MM_OSD_SETTING,
	MM_COLOR_SETTING,
#if NETWORK_SUPPORT
    MM_NETWORK,
#endif
}SettingId;

static char* s_SysList[] = {"Information", "System", "Setting"};
static char* s_Info[] = {STR_ID_SYSTEM_INFO};
static char* s_System[] = {STR_ID_TIME_SET, STR_ID_FIRMWARE_UPGRADE, STR_ID_FACTORY_RESET};
#if NETWORK_SUPPORT
static char* s_Setting[] = { STR_ID_TIMER_SET, STR_ID_LANG, STR_ID_AV_SET, 
				    STR_ID_LOCK_CTRL, STR_ID_OSD_SET, STR_ID_COLOR_SET, STR_ID_NETWORK};
#else
static char* s_Setting[] = {STR_ID_TIMER_SET, STR_ID_LANG, STR_ID_AV_SET, 
					        STR_ID_LOCK_CTRL, STR_ID_OSD_SET, STR_ID_COLOR_SET};
#endif
static SystemListId s_CurList = MS_INFO;

/** main menu **/
#define TITLE_IMG_BG  "img_main_menu_title_bg"
#define TITLE_IMG     "img_main_menu_title"
#define TITLE_BTN     "btn_main_menu_title"

typedef enum
{
	MM_INSTALLATION = 0,
	MM_UTILITY,
	MM_SYSTEM_SETTING,
	MM_CHANNEL_EDIT,
	MM_MEDIA,
	MM_TOTAL,
	MM_EPG,
	MM_TV,
	MM_RADIO,
}MainMenuId;

typedef enum
{
#if GAME_ENABLE
    MM_GAME,
#endif
    MM_PVR_MANAGEMENT,
#if OTA_SUPPORT
    MM_OTA,
#endif
#ifdef SPECIAL_SETTING_SUPPORT
    MM_SPECIAL,
#endif
#if CASCAM_SUPPORT
    MM_CA,
#endif
#ifdef ECOS_OS                                                                
#if IPTV_SUPPORT                                                              
    MM_NET_IPTV,                                                              
#endif                                                                        
#if WEATHER_SUPPORT                                                           
    MM_NET_WEATHER,                                                           
#endif                                                                        
#if MAP_SUPPORT                                                               
    MM_NET_MAP,                                                               
#endif                                                                        
#if YOUTUBE_SUPPORT                                                           
    MM_NET_YOUTUBE,                                                           
#endif                                                                        
#if YOUPORN_SUPPORT                                                           
    MM_NET_YOUPORN,                                                           
#endif                                                                        
#endif                 
    MM_UTI_TOTAL,
}UtilityMenuId;

static char* s_MenuItemName[] = {STR_ID_INSTALL, STR_ID_UTILITY, STR_ID_SYS_SET, STR_ID_CH_EDIT};
#if TKGS_SUPPORT
static char* s_Installation[] = {STR_ID_ATNE_SET, STR_ID_POSI_SET, STR_ID_SAT_LIST, STR_ID_TP_LIST,STR_ID_TKGS_UPGRATE};
#else
static char* s_Installation[] = {STR_ID_ATNE_SET, STR_ID_POSI_SET, STR_ID_SAT_LIST, STR_ID_TP_LIST};
#endif
static char* s_ChannelEdit[] = {STR_ID_TV_CH, STR_ID_RADIO_CH, STR_ID_DEL_ALL};

static char* s_Utility[] = {
#if GAME_ENABLE
    STR_ID_GAME_CENTER, 
#endif
    STR_ID_PVR_MANAGE,
#if OTA_SUPPORT
    STR_ID_OTA_UPGRADE, 
#endif
#ifdef SPECIAL_SETTING_SUPPORT
    STR_ID_SPECIAL,
#endif
#if CASCAM_SUPPORT
    STR_ID_SPECIAL,
#endif
#ifdef ECOS_OS                                                                
#if IPTV_SUPPORT                                                              
    STR_ID_NET_IPTV,                                                          
#endif                                                                        
#if WEATHER_SUPPORT                                                           
    STR_ID_NET_WEATHER,                                                       
#endif                                                                        
#if MAP_SUPPORT                                                               
    STR_ID_NET_MAP,                                                           
#endif                                                                        
#if YOUTUBE_SUPPORT                                                           
    STR_ID_NET_YOUTUBE,                                                       
#endif                                                                        
#if YOUPORN_SUPPORT                                                           
    STR_ID_NET_YOUPORN,                                                       
#endif                         
#endif                         
};

static char *s_Week[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
static event_list* spAppMainMenuTimer = NULL;
static bool s_book_flag = false;

extern status_t menu_sub_list_create(MenuSubList *list);
extern status_t app_create_epg_menu(void);
#endif //MINI_16BIT_WIN8_OSD_SUPPORT

typedef enum
{
	MM_ANTENNA = 10,
	MM_POSITIONER,
	MM_SATELLITE_LIST,
	MM_TP_LIST,
	#if TKGS_SUPPORT
	MM_TKGS_UPGRADE
	#endif
}InstallationMenuId;

typedef enum
{
	MM_TV_EDIT = 20,
	MM_RADIO_EDIT,
	MM_DELETE_ALL
}ChannelMenuId;

typedef enum
{
	MM_DEL_ALL_NO_CH = 0,
	MM_DEL_ALL_HAVE_CH,
	MM_DEL_ALL,
	MM_NO_CHANNEL,
	MM_NO_SAT
}MainMenuPopupType;
typedef struct
{
	uint32_t total_tv;
	uint32_t total_radio;
}TotalChannelNum;

static MainMenuId s_CurMenu = MM_INSTALLATION;
static MainMenuPopupType s_PopupType;

extern void app_reboot(void);
extern void app_time_setting_menu_exec(void);
extern void app_lock_setting_menu_exec(void);
extern void app_av_setting_menu_exec(void);
extern void app_pvr_set_menu_exec(void);
extern void app_lang_setting_menu_exec(void);
extern void app_osd_setting_menu_exec(void);
extern void app_color_setting_menu_exec(void);
extern void app_upgrade_menu_exec(void);
extern void app_game_menu_exec(void);
extern void app_sysinfo_menu_exec(void);
extern void app_system_reset(void);
#if MINI_16BIT_WIN8_OSD_SUPPORT
extern void app_main_menu_display_time_status(int mode);
#else
extern void app_create_pvr_media_menu(void);
#endif
#ifdef SPECIAL_SETTING_SUPPORT
extern void app_special_menu_exec(void);
#endif
#if CASCAM_SUPPORT
extern void app_ca_menu_exec(void);
#endif

#if NETWORK_SUPPORT
#if WEATHER_SUPPORT
extern void app_weather_map_set_wnd(uint8_t wnd);
#endif
#endif

/* widget macro -------------------------------------------------------- */
#define SUB_FUNC
void _delete_all_channels(void)
{
	GxBusPmViewInfo view_info;
	GxBusPmViewInfo view_info_bak;
	GxMsgProperty_NodeNumGet node_num_get = {0};
	GxMsgProperty_NodeByPosGet node_get = {0};
	GxMsgProperty_NodeDelete node_del = {0};
	uint32_t*p_id_array = NULL;
	uint32_t i=0;
	
	//delete all tv
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	view_info_bak = view_info;	//bak
	view_info.stream_type = GXBUS_PM_PROG_TV;
	view_info.group_mode = 0;//all
	view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

	node_num_get.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
	if(node_num_get.node_num == 0)
	{
		printf("____delete_all_channels not TV program\n");
        	//restore viewinfo
        	g_AppPlayOps.play_list_create(&view_info_bak);
		//return; /*if have radio,the radio cann't be deleted*/
	}
	else
	{
		p_id_array = GxCore_Malloc(node_num_get.node_num*sizeof(uint32_t));//need GxCore_Free
		if(p_id_array == NULL)
		{
			//printf("\_delete_all_channels, malloc failed...%s,%d\n",__FILE__,__LINE__);
        		//restore viewinfo
        		g_AppPlayOps.play_list_create(&view_info_bak);
			//return ; /*if have radio,the radio cann't be deleted*/
		}
		else
		{
			for(i=0; i<node_num_get.node_num; i++)
			{
				node_get.node_type = NODE_PROG;
				node_get.pos = i;
				app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);
				p_id_array[i] = node_get.prog_data.id;
			}
			node_del.node_type = NODE_PROG;
			node_del.id_array = p_id_array;
			node_del.num = node_num_get.node_num;
			app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node_del);
			GxCore_Free(p_id_array);
		}
	}

	//delete all radio
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	view_info.stream_type = GXBUS_PM_PROG_RADIO;
	view_info.group_mode = 0;//all
	view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

	node_num_get.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
	if(node_num_get.node_num == 0)
	{
		printf("____delete_all_channels not TV program\n");
        	//restore viewinfo
        	g_AppPlayOps.play_list_create(&view_info_bak);
		return;
	}
	p_id_array = GxCore_Malloc(node_num_get.node_num*sizeof(uint32_t));//need GxCore_Free
	if(p_id_array == NULL)
	{
		//printf("\_delete_all_channels, malloc failed...%s,%d\n",__FILE__,__LINE__);
        //restore viewinfo
        g_AppPlayOps.play_list_create(&view_info_bak);
		return ;
	}
	for(i=0; i<node_num_get.node_num; i++)
	{
		node_get.node_type = NODE_PROG;
		node_get.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);
		p_id_array[i] = node_get.prog_data.id;
	}
	node_del.node_type = NODE_PROG;
	node_del.id_array = p_id_array;
	node_del.num = node_num_get.node_num;
	app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node_del);
	GxCore_Free(p_id_array);

	//restore viewinfo
	g_AppPlayOps.play_list_create(&view_info_bak);

	g_AppBook.invalid_remove();
    
	return;
}

#if MINI_16BIT_WIN8_OSD_SUPPORT
enum{
    ZOOM_MM_X = 0,
    ZOOM_MM_Y = 0,
    ZOOM_MM_W = APP_XRES,
    ZOOM_MM_H = APP_YRES 

};
static PlayerWindow video_wnd;


extern bool app_check_av_running(void);

static void app_main_check_video_state(void)
{
	PlayerStatusInfo  StatusInfo;
	GxPlayer_MediaGetStatus("player0", &StatusInfo);
	if(StatusInfo.vcodec.state == AVCODEC_RUNNING)
	{
		GUI_SetProperty("txt_main_menu_video","state","hide");
	}
	else
	{
		//GUI_SetProperty("txt_main_menu_video","state","show");
	}
}
static void app_main_menu_start_video(int play_mode)
{
#define VIDEO_TEXT "txt_main_menu_for_pp"
#define PROG_STREAM_TYPE(play_ops)   play_ops.normal_play.view_info.stream_type

    int x,y,w,h = 0;
	GxMsgProperty_NodeByPosGet node_prog = {0};
	GxMsgProperty_NodeByIdGet node_tp = {0};
	GxMsgProperty_NodeNumGet node_num_get = {0};
	GxMsgProperty_NodeNumGet node_num_sat = {0};
    
	char Buffer[80];
	char* PolarStr[2] = {"H", "V"};
	
    sscanf((widget_get_property(gui_get_widget(NULL,VIDEO_TEXT),"rect")),"[%d,%d,%d,%d]",&x,&y,&w,&h);

	
    // zoom out
    video_wnd.x = x;
    video_wnd.y = y;
    video_wnd.width = w;
    video_wnd.height = h;

    GUI_SetInterface("flush", NULL);
    g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);

    node_num_sat.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_sat));

	node_prog.node_type = NODE_PROG;
	node_prog.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

	//avoid 1st success search but can not play channel  qingyzh add 20160824
	if(0 == g_AppPlayOps.normal_play.play_total)
	{
		node_num_get.node_type = NODE_PROG;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
		g_AppPlayOps.normal_play.play_total = node_num_get.node_num;
	}
	//add end
		
    if(app_check_av_running() == false)
    {
    	//GUI_SetInterface("video_off", NULL);
    	if(play_mode)
    	{
        	g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    	}
		//GUI_SetProperty("wnd_main_menu", "back_ground", (void*)"null");
		//GUI_SetInterface("flush",NULL);
    }
    else
    {
        //GUI_SetInterface("video_top", NULL);
    }
    //g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
    GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
    
	if((0 == g_AppPlayOps.normal_play.play_count && 0 == g_AppPlayOps.normal_play.play_total)
            || 0 == node_num_sat.node_num)
	{
		GUI_SetProperty("txt_main_menu_title60", "string", " ");
		GUI_SetProperty("txt_main_menu_title70", "string", " ");
	}
	else
	{
		memset(Buffer, 0, 80);
		sprintf((char*)Buffer,"%04d",   g_AppPlayOps.normal_play.play_count+1);
		sprintf(Buffer+strlen(Buffer), " %s", node_prog.prog_data.prog_name);
		GUI_SetProperty("txt_main_menu_title60", "string", " ");
		GUI_SetProperty("txt_main_menu_title60", "string", Buffer);
		//sprintf(Buffer, "PID : %d/%d/%d", node_prog.prog_data.video_pid,
		//node_prog.prog_data.cur_audio_pid,node_prog.prog_data.pcr_pid);
		node_tp.node_type = NODE_TP;
	    node_tp.id = node_prog.prog_data.tp_id;
	    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

		GxMsgProperty_NodeByIdGet pmdata_sat = {0};
	    pmdata_sat.node_type = NODE_SAT;
	    pmdata_sat.id = node_prog.prog_data.sat_id;
	    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &pmdata_sat);
	    sprintf(Buffer, "TP : %d / %s / %d", node_tp.tp_data.frequency, (pmdata_sat.sat_data.type != GXBUS_PM_SAT_S) ? "NULL" :PolarStr[node_tp.tp_data.tp_s.polar], node_tp.tp_data.tp_s.symbol_rate );
		GUI_SetProperty("txt_main_menu_title70", "string", Buffer);
	}

	//GUI_SetProperty("wnd_main_menu", "back_ground", (void*)"bg.jpg");
}

static void app_main_menu_full_video(void)
{
    // zoom out full screen
    PlayerWindow video_wnd = {ZOOM_MM_X, ZOOM_MM_Y, ZOOM_MM_W, ZOOM_MM_H};

    GUI_SetInterface("flush", NULL);
    g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);

    if(app_check_av_running() == false)
    {
        g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    }

	GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
	GUI_SetProperty("txt_main_menu_video", "state", "show");
 
}

static void app_main_menu_get_local_time(struct tm *pLocalTime)
{
	int init_value = TIME_ZONE_VALUE;
	GxTime time;
	
	int32_t second;

	if (NULL == pLocalTime)
		return;

	GxCore_GetLocalTime(&time);
	GxBus_ConfigGetInt(TIME_ZONE, &init_value, TIME_ZONE_VALUE);
	if(init_value < -12)
	    init_value = -12;
	if(init_value > 12)
	    init_value = 12;

	second = time.seconds + 3600 * init_value;
	//GxBus_ConfigGetInt(TIME_SUMMER, &init_value, TIME_SUMMER_VALUE);
	//second += time.seconds+HOUR_TO_SEC*init_value;

	memset(pLocalTime,0,sizeof(struct tm));
	memcpy((void*)pLocalTime, localtime((const time_t*) &second), sizeof(struct tm));
	pLocalTime->tm_year += 1900;
	pLocalTime->tm_mon ++;

	return ;
}

static int timer_main_menu_msg_tip(void *userdata)
{
	struct tm LocalTime;
	static char pArry1[20] = {0};
	static char pArry2[20] = {0};
	static char pArry3[80] = {0};
	static int count = 0;
	char sys_time_str[24] = {0};
	char time_str_temp[24] = {0};
	char hour[2] = {0};
    char min[2] = {0};
    int i_hour = 0, i_min = 0;

	app_main_menu_get_local_time(&LocalTime);
	g_AppTime.time_get(&g_AppTime, sys_time_str, sizeof(sys_time_str)); 
	if((strlen(sys_time_str) > 24) || (strlen(sys_time_str) < 5))
	{
		printf("\nlenth error\n");
		return 1;
	}
	memcpy(time_str_temp,&sys_time_str[strlen(sys_time_str)-5],5);// 00:00
	memcpy(hour,&time_str_temp,2);
	memcpy(min,&time_str_temp[strlen(time_str_temp)-2],2);
    i_hour = atoi(hour);
    i_min = atoi(min);
	if(count == 0)
	{
		sprintf(pArry2,"%02d %02d", i_hour,i_min);
		count = 1;
	}
	else if(count == 1)
	{
		sprintf(pArry2,"%02d:%02d", i_hour, i_min);
		count = 0;
	}
	sprintf(pArry1,"%02d",LocalTime.tm_sec);
	if(( 0 <= LocalTime.tm_wday ) && (LocalTime.tm_wday < 7) && (LocalTime.tm_mon > 0) && (LocalTime.tm_mon < 13))
	{
	    sprintf(pArry3,", %02d %02d, %04d",LocalTime.tm_mon,LocalTime.tm_mday,LocalTime.tm_year);
	}
	GUI_SetProperty("txt_main_menu_seconds", "string", pArry1);
	GUI_SetProperty("txt_main_menu_time", "string", pArry2);
	GUI_SetProperty("txt_main_menu_date", "string", pArry3);
	GUI_SetProperty("txt_main_menu_week", "string", s_Week[LocalTime.tm_wday]);

	return 0;
}

void app_main_menu_display_time_status(int mode)
{
	if(mode)
	{
		app_main_menu_start_video(1);
		reset_timer(spAppMainMenuTimer);
		g_AppFullArb.timer_start();
//		GUI_SetProperty("wnd_main_menu", "back_ground", (void*)"bg.jpg");
		GUI_SetProperty("txt_main_menu_seconds","state","show");
		GUI_SetProperty("txt_main_menu_time","state","show");
		GUI_SetProperty("txt_main_menu_date","state","show");
        GUI_SetProperty("txt_main_menu_week","state","show");
	    GUI_SetProperty("txt_main_menu_seconds", "string", " ");
        GUI_SetProperty("txt_main_menu_time", "string", " ");
        GUI_SetProperty("txt_main_menu_week", "string", " ");
        GUI_SetProperty("txt_main_menu_date", "string", " ");
		app_main_check_video_state();
		GUI_SetInterface("flush", NULL);
		GUI_SetInterface("video_top",NULL);
	}
	else
	{
		timer_stop(spAppMainMenuTimer);
		g_AppFullArb.timer_stop();
		GUI_SetProperty("txt_main_menu_seconds","state","hide");
		GUI_SetProperty("txt_main_menu_time","state","hide");
		GUI_SetProperty("txt_main_menu_date","state","hide");
        GUI_SetProperty("txt_main_menu_week","state","hide");
	}
	return;
}

int app_main_menu_create_flag(void)
{
	if(s_CurMenu == MM_EPG)
	{
		app_main_menu_display_time_status(1);
		return 1;
	}
	return 0;
}
#endif

TotalChannelNum _check_total_channel_num(void)
{
	GxBusPmViewInfo view_info;
	GxBusPmViewInfo view_info_bak;
	GxMsgProperty_NodeNumGet node_num_get = {0};
	TotalChannelNum total={0};

	//check tv
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	view_info_bak = view_info;	//bak
	view_info.group_mode = GROUP_MODE_ALL;
	view_info.taxis_mode = TAXIS_MODE_NON;
	view_info.stream_type = GXBUS_PM_PROG_TV;
	view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

	node_num_get.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
	total.total_tv = node_num_get.node_num;

	//check radio
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	view_info.stream_type = GXBUS_PM_PROG_RADIO;
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

	node_num_get.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
	total.total_radio = node_num_get.node_num;

	//restore viewinfo
	app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info_bak));

	return total;
}

static int _reset_passwd_poptip_creat_cb(void)
{
    GUI_SetInterface("flush",NULL);
	app_system_reset();
#if (0 == MINI_16BIT_WIN8_OSD_SUPPORT)
	GUI_SetProperty("wnd_main_menu", "update", NULL);
#if MV_WIN_SUPPORT
    GUI_SetProperty("wnd_main_menu_item", "update", NULL);
#endif
#endif
    return 0;
}

int _reset_reboot_cb(PopDlgRet ret)
{
    app_reboot();
    return 0;
}

int _reset_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
	if(ret->result ==  PASSWD_OK)
	{	
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_YES_NO;
        	pop.format = POP_FORMAT_DLG;
       		pop.str = STR_ID_SURE_RESET;
       		pop.title = STR_ID_FACTORY_RESET;
       		pop.exit_cb = NULL;
       		//pop.timeout_sec = 8;
		if(popdlg_create(&pop) == POP_VAL_OK)
		{
			memset(&pop, 0, sizeof(PopDlg));
        		pop.type = POP_TYPE_NO_BTN;
	       		pop.str  = STR_ID_PLZ_WAIT;
			//pop.mode = POP_MODE_UNBLOCK;
	       		pop.creat_cb = _reset_passwd_poptip_creat_cb;
	       		popdlg_create(&pop);

			//GxCore_ThreadDelay(10);

			/*memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.str = STR_ID_FACTORY_RESET_SUC;
                        pop.timeout_sec = 1;
                        pop.exit_cb = NULL;
                        popdlg_create(&pop);*/
                        /*pop.str = STR_ID_FACTORY_RESET_SUC;
			popdlg_update_status(&pop);
			GxCore_ThreadDelay(100);*/

			memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.str = STR_REBOOT_NOW;
                        pop.timeout_sec = 1;
                        pop.exit_cb = _reset_reboot_cb;
                        popdlg_create(&pop);
			/*pop.str = STR_REBOOT_NOW;
			popdlg_update_status(&pop);
                        GxCore_ThreadDelay(100);
    			app_reboot();*/
		}
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
		PasswdDlg passwd_dlg;
                memset(&passwd_dlg, 0, sizeof(PasswdDlg));
                passwd_dlg.usr_data = usr_data;
                passwd_dlg.passwd_exit_cb = _reset_passwd_dlg_cb;
                passwd_dlg_create(&passwd_dlg);
	}
	else
    {}

	return 0;	
}

void create_sub_menu_installation(int select)
{
	GxMsgProperty_NodeNumGet sat_num = {0};
#if MINI_16BIT_WIN8_OSD_SUPPORT
	switch(select+10)
#else
	switch(select)
#endif
	{
		case MM_ANTENNA:
			sat_num.node_type = NODE_SAT;
			app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
			if(sat_num.node_num > 0)
			{
#if DOUBLE_S2_SUPPORT
				extern int app_tuner_select_create_dlg(char *Widget);
				app_tuner_select_create_dlg("wnd_antenna_setting");
#else
				GUI_CreateDialog("wnd_antenna_setting");
#endif
			}
			else
			{
				s_PopupType = MM_NO_SAT;
                GUI_CreateDialog("wnd_mainmenu_tip");
			}
			break;		

		case MM_POSITIONER:
			sat_num.node_type = NODE_SAT;
			app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
			if(sat_num.node_num > 0)
			{
				GUI_CreateDialog("wnd_positioner_setting");
			}
			else
			{
				s_PopupType = MM_NO_SAT;
				GUI_CreateDialog("wnd_mainmenu_tip");
			}
			break;		

		case MM_SATELLITE_LIST:
			GUI_CreateDialog("wnd_satellite_list");
			break;
			
		case MM_TP_LIST:
			sat_num.node_type = NODE_SAT;
			app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
			if(sat_num.node_num > 0)
			{
				GUI_CreateDialog("wnd_tp_list");
			}
			else
			{
				s_PopupType = MM_NO_SAT;
				GUI_CreateDialog("wnd_mainmenu_tip");
			}
			break;
		#if TKGS_SUPPORT
		case MM_TKGS_UPGRADE:
			sat_num.node_type = NODE_SAT;
			app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
			if(sat_num.node_num > 0)
			{
				//GUI_CreateDialog("wnd_tkgs_upgrade");
				app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_MENU);
				app_tkgs_upgrade_menu_exec();
			}
			else
			{
				s_PopupType = MM_NO_SAT;
				GUI_CreateDialog("wnd_mainmenu_tip");
			}
			break;
		#endif
		default:
			break;
	}

	return;
}

void create_sub_menu_channel(int select)
{
    TotalChannelNum total;
#if MINI_16BIT_WIN8_OSD_SUPPORT
	switch(select + 20)

#else
	switch(select + 20)
#endif
    {
        case MM_TV_EDIT:
            total = _check_total_channel_num();
            if(total.total_tv == 0)//no tv
            {
                s_PopupType = MM_NO_CHANNEL;
                GUI_CreateDialog("wnd_mainmenu_tip");
            }
            else
            {
                // get total earlier than create, so move from channel_edit to here
		//g_OldStreamState.old_view_info = g_AppPlayOps.normal_play.view_info;
                g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_TV;
                // set tv mode

                // this will change full screen state
		//g_OldStreamState.old_tv_state = g_AppFullArb.state.tv;
                g_AppFullArb.state.tv = STATE_ON;
                GUI_CreateDialog("wnd_channel_edit");
            }
            break;	
        case MM_RADIO_EDIT:
            total = _check_total_channel_num();
            if(total.total_radio == 0)//no radio 
            {
                s_PopupType = MM_NO_CHANNEL;
                GUI_CreateDialog("wnd_mainmenu_tip");
            }
            else
            {
                // get total earlier than create, so move from channel_edit to here
		//g_OldStreamState.old_view_info = g_AppPlayOps.normal_play.view_info;
                g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_RADIO;
                // set radio mode

                // this will change full screen state
		//g_OldStreamState.old_tv_state = g_AppFullArb.state.tv;
                g_AppFullArb.state.tv = STATE_OFF;
                GUI_CreateDialog("wnd_channel_edit");
            }
            break;
		case MM_DELETE_ALL:
			#if TKGS_SUPPORT

			if(GX_TKGS_AUTOMATIC == app_tkgs_get_operatemode())
			{
				PopDlg  pop;
				memset(&pop, 0, sizeof(PopDlg));
				pop.type = POP_TYPE_OK;
				pop.str = STR_ID_TKGS_CANT_WORK;
				pop.mode = POP_MODE_UNBLOCK;
				pop.format = POP_FORMAT_DLG;
				popdlg_create(&pop);	
				break;
			}
			#endif
			s_PopupType = MM_DEL_ALL;
			GUI_CreateDialog("wnd_mainmenu_tip");
			break;	
			
		default:
			break;
	}

	return;
}

void create_sub_menu_syssetting(int select)
{
	switch(select)
	{
#if (0 == MINI_16BIT_WIN8_OSD_SUPPORT)
		case MM_TIME_SETTING:
			app_time_setting_menu_exec();
			break;	
#endif
		case MM_TIMER_SETTING:
			GUI_CreateDialog("wnd_timer_setting");
			break;	

		case MM_LANGUAGE:
			app_lang_setting_menu_exec();
			break;	

		case MM_AV_OUTPUT:
			app_av_setting_menu_exec();
			break;	

		case MM_PARENTAL_LOCK:
			app_lock_setting_menu_exec();
			break;	
			
		case MM_OSD_SETTING:
			app_osd_setting_menu_exec();
			break;	

		case MM_COLOR_SETTING:
			app_color_setting_menu_exec();
			break;
#if NETWORK_SUPPORT
		case MM_NETWORK:
			GUI_CreateDialog("wnd_network");
			break;
#endif
		default:
			break;
	}

	return;
}

int _sys_setting_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
	int select = *(int*)usr_data;
	if(ret->result ==  PASSWD_OK)
	{	
		create_sub_menu_syssetting(select);
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
		PasswdDlg passwd_dlg;
		memset(&passwd_dlg, 0, sizeof(PasswdDlg));
		passwd_dlg.usr_data = usr_data;
		passwd_dlg.passwd_exit_cb = _sys_setting_passwd_dlg_cb;
		passwd_dlg_create(&passwd_dlg);
	}
	else
    	{}

	return 0;	
}

void create_sub_menu_utility(int select)
{
	GxMsgProperty_NodeNumGet sat_num = {0};
    switch (select)
    {
    
#if (GAME_ENABLE > 0)
        case MM_GAME:
			if ((g_AppPvrOps.state == PVR_RECORD)||(g_AppPvrOps.state == PVR_TMS_AND_REC)
				||(g_AppPvrOps.state == PVR_TIMESHIFT))
					break;
			app_game_menu_exec();
            break;
#endif

		case MM_PVR_MANAGEMENT:
            app_pvr_set_menu_exec();
			break;

#if (OTA_SUPPORT > 0)		
	case MM_OTA:
            sat_num.node_type = NODE_SAT;
            app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
            if(sat_num.node_num > 0)
            {
                g_AppFullArb.timer_stop();
                GUI_CreateDialog("wnd_ota_upgrade");
            }
            else
            {
                s_PopupType = MM_NO_SAT;
                GUI_CreateDialog("wnd_mainmenu_tip");
            }
            break;		
#endif
#ifdef SPECIAL_SETTING_SUPPORT
		case MM_SPECIAL:
			//app_uart_menu_exec();
			app_special_menu_exec();
			break;
#endif

#if CASCAM_SUPPORT
        case MM_CA:
            app_ca_menu_exec();
            break;
#endif

#if IPTV_SUPPORT                                                              
        case MM_NET_IPTV:                                                     
        ┊   app_create_iptv_menu();                                           
        ┊   break;                                                            
#endif                                                                        
#if WEATHER_SUPPORT                                                           
        case MM_NET_WEATHER:                                                  
            GUI_CreateDialog("wnd_weather");                                  
            break;                                                            
#endif                                                                        
#if MAP_SUPPORT                                                               
        case MM_NET_MAP:                                                      
            GUI_CreateDialog("wnd_map");                                      
            break;                                                            
#endif                                                                        
#if YOUTUBE_SUPPORT                                                           
        case MM_NET_YOUTUBE:                                                  
            {
                extern status_t app_create_youtube_menu(void);
                app_create_youtube_menu();                                        
            }
            break;                                                            
#endif                                                                        
#if YOUPORN_SUPPORT                                                           
    ┊   case MM_NET_YOUPORN:                                              
    ┊   ┊   app_create_youporn_menu();                                    
    ┊   ┊   break;                                                        
#endif                                                                        

        default:
            break;
    }
}

void app_main_item_menu_create(int value)
{
	return;
}

void app_main_menu_media_item_create(int value)
{
	#if MV_WIN_SUPPORT
	s_CurMenu = MM_MEDIA;
	if(GUI_CheckDialog("wnd_main_menu") != GXCORE_SUCCESS)
	{
		GUI_CreateDialog("wnd_main_menu");	
	}
	#endif
}

#if MINI_16BIT_WIN8_OSD_SUPPORT
static void app_main_menu_set_item_focus(MainMenuId id)
{
	if(id < 0 || id >= MM_TOTAL)
		return;

	int i = 0;
	char img_bg[30] = {0};
	char btn[30] = {0};

	for(i=0; i<MM_TOTAL; i++)
	{
		memset(img_bg, 0, sizeof(img_bg));
		if(i == id)
		{
			sprintf(img_bg, "%s%d", TITLE_IMG_BG, i);
			GUI_SetProperty(img_bg, "state", (void *)"show");

			memset(btn, 0, sizeof(btn));
			sprintf(btn, "%s%d", TITLE_BTN, i);
			GUI_SetFocusWidget(btn);
		}
		else
		{
			sprintf(img_bg, "%s%d", TITLE_IMG_BG, i);
			GUI_SetProperty(img_bg, "state", (void *)"hide");
		}
	}
	GUI_SetProperty("wnd_main_menu", "update", NULL);
	return;
}
#endif

SIGNAL_HANDLER int app_main_menu_create(GuiWidget *widget, void *usrdata)
{
#if MINI_16BIT_WIN8_OSD_SUPPORT
	app_main_menu_start_video(0);
	app_main_menu_set_item_focus(MM_INSTALLATION);

	if (NULL == spAppMainMenuTimer)
	{
		spAppMainMenuTimer = create_timer(timer_main_menu_msg_tip, 500, NULL,  TIMER_REPEAT);
	}
	else
	{
		reset_timer(spAppMainMenuTimer);
	}

	g_AppFullArb.timer_start();
	//GUI_SetProperty("txt_main_menu_date","backcolor", "[#1D0DE3,#1D0DE3,#1D0DE3]");
#else
	int value = 0;

	//first to installation
	//s_CurMenu = MM_INSTALLATION;

	GUI_SetProperty(s_MenuTitle[s_CurMenu],"state","disable");
	GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Installation[0]);
	app_lang_set_menu_font_type(WIDGET_NAME_TITLE);

    app_main_item_menu_create(0);

    GUI_SetProperty(WIDGET_NAME_OPT,"update_all",NULL);
    GUI_SetProperty(WIDGET_NAME_UP,"state","hide"/*"osd_trans_hide"*/);//
    GUI_SetFocusWidget(WIDGET_NAME_OPT);

	GUI_SetProperty(WIDGET_NAME_OPT,"select",&value);
	//app_main_get_data_font();  //20131031

	//GUI_SetProperty("wnd_main_menu_image_BG","load_img",WORK_PATH"theme/image/bg.jpg");
#endif	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_main_menu_destroy(GuiWidget *widget, void *usrdata)
{
	s_CurMenu = MM_INSTALLATION;
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_main_menu_got_focus_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_SetProperty("txt_main_menu_title60", "string", " ");
	GUI_SetProperty("txt_main_menu_title70", "string", " ");
    app_main_menu_display_time_status(1);
    //reset_timer(spAppMainMenuTimer);
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_main_menu_lost_focus_keypress(GuiWidget *widget, void *usrdata)
{
    //timer_stop(spAppMainMenuTimer);
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_main_menu_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
#if MINI_16BIT_WIN8_OSD_SUPPORT
	static MenuSubList list_menu;
	TotalChannelNum prog_total;
#endif

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
				case VK_BOOK_TRIGGER:
					s_book_flag = true;
					GUI_EndDialog("wnd_main_menu");
#if MINI_16BIT_WIN8_OSD_SUPPORT
                    if(s_book_flag == false)
                    {
                        app_main_menu_full_video();
                    }
                    else
                    {
                        s_book_flag = false;
                    }
                    remove_timer(spAppMainMenuTimer);
                    spAppMainMenuTimer = NULL;
#endif
					GUI_SetProperty("wnd_main_menu","draw_now",NULL);
					break;

				case STBK_EXIT:
				case STBK_MENU:
				{
					if (g_AppPlayOps.normal_play.play_total == 0)
					{
						FullArb *arb = &g_AppFullArb;
						if(arb->state.mute == STATE_ON)
						{
							arb->state.mute = STATE_OFF;
						}
					}
					g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
                    //g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
                    if(s_CurMenu == MM_MEDIA)
                    {
                        pmpset_init();//2014-0427
                    }

                    GUI_EndDialog("wnd_main_menu");
#if MINI_16BIT_WIN8_OSD_SUPPORT
                    if(s_book_flag == false)
                    {
                        app_main_menu_full_video();
                    }
                    else
                    {
                        s_book_flag = false;
                    }
                    remove_timer(spAppMainMenuTimer);
                    spAppMainMenuTimer = NULL;
#endif
                    GUI_SetInterface("flush",NULL);

#if ((0 == MINI_16BIT_WIN8_OSD_SUPPORT) && MV_WIN_SUPPORT)
					GUI_EndDialog(WND_MAIN_MENU_ITEM);
					GUI_SetInterface("flush",NULL);
#endif

					g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);//目的处理进节目编辑后退出导致主界面变灰的问题
					g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);

					if (g_AppPlayOps.normal_play.play_total != 0)
					{
					// deal for recall , current will copy to recall
						#if RECALL_LIST_SUPPORT
						//extern int g_recall_count; 
						g_AppPlayOps.current_play = g_AppPlayOps.recall_play[0];//g_recall_count
//						#else
//						g_AppPlayOps.current_play = g_AppPlayOps.recall_play;
						#endif


						if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
						{
							g_AppFullArb.tip = FULL_STATE_LOCKED;
							g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT,g_AppPlayOps.normal_play.play_count);
						}
						else
						{
#if (0 == MINI_16BIT_WIN8_OSD_SUPPORT)
							g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT,g_AppPlayOps.normal_play.play_count);
#endif
							g_AppFullArb.tip = FULL_STATE_DUMMY;
							if(GUI_CheckDialog("wnd_passwd") != GXCORE_SUCCESS)
							{
								GUI_CreateDialog("wnd_channel_info");
							}
						}
					}
					break;
				}

				case STBK_LEFT:
					{
#if (0 == MINI_16BIT_WIN8_OSD_SUPPORT)
						GUI_SetProperty(s_MenuTitle[s_CurMenu],"state","enable");
						#if MV_WIN_SUPPORT
						if(MM_CHANNEL_EDIT== s_CurMenu)
						#else
						if(MM_INSTALLATION == s_CurMenu)
						#endif
						{
#if NETWORK_SUPPORT
							s_CurMenu = MM_INTERNET;
#else
							s_CurMenu = MM_MEDIA;
#endif

						}
						else
						{
							s_CurMenu--;
						}
						GUI_SetProperty(s_MenuTitle[s_CurMenu],"state","disable");
						app_lang_set_menu_font_type(WIDGET_NAME_TITLE);

						#if(0 == MV_WIN_SUPPORT)
						GUI_SetProperty(WIDGET_NAME_OPT,"update_all",NULL);
						GUI_SetProperty(WIDGET_NAME_UP,"state","hide"/*"osd_trans_hide"*/);
						GUI_SetProperty(WIDGET_NAME_DOWN,"state","show");
						#endif

						switch(s_CurMenu)
						{
							case MM_INSTALLATION:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Installation[0]);
								break;

                            case MM_CHANNEL_EDIT:
                                GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_ChannelEdit[0]);
                                break;

							case MM_SYSTEM_SETTING:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_SystemSetting[0]);
								break;

							case MM_UTILITY:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Utility[0]);
								break;
#if MV_WIN_SUPPORT
#if(0 == NETWORK_SUPPORT)
							case MM_MEDIA:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Media[0]);
								break;

#else
							case MM_INTERNET:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Internet[0]);
								break;
#endif	
#else
							case MM_MEDIA:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Media[0]);
								break;
							#if NETWORK_SUPPORT
							case MM_INTERNET:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Internet[0]);
								break;
							#endif
#endif
							default:
								break;
						}

						app_main_item_menu_move(STBK_LEFT);
#else
						#if 1
						if(s_CurMenu == MM_INSTALLATION)
							s_CurMenu = MM_MEDIA;
						else
							s_CurMenu--;
						#else
						if(s_CurMenu == MM_INSTALLATION)
							s_CurMenu = MM_TV;
						else
							s_CurMenu--;
						#endif
						app_main_menu_set_item_focus(s_CurMenu);
#endif
					}
					break;

				case STBK_RIGHT:
					{
#if (0 == MINI_16BIT_WIN8_OSD_SUPPORT)
						GUI_SetProperty(s_MenuTitle[s_CurMenu],"state","enable");
#if NETWORK_SUPPORT
						if(MM_INTERNET == s_CurMenu)
#else
						if(MM_MEDIA == s_CurMenu)
#endif
						{
						#if  MV_WIN_SUPPORT 
							s_CurMenu = MM_CHANNEL_EDIT;
						#else
							s_CurMenu = MM_INSTALLATION;
						#endif
						}
						else
						{
							s_CurMenu++;
						}


						GUI_SetProperty(s_MenuTitle[s_CurMenu],"state","disable");
						app_lang_set_menu_font_type(WIDGET_NAME_TITLE);

						#if(0 == MV_WIN_SUPPORT)
						GUI_SetProperty(WIDGET_NAME_OPT,"update_all",NULL);
						GUI_SetProperty(WIDGET_NAME_UP,"state","hide"/*"osd_trans_hide"*/);
						GUI_SetProperty(WIDGET_NAME_DOWN,"state","show");
						#endif

                        switch(s_CurMenu)
                        {
                            case MM_INSTALLATION:
                                GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Installation[0]);
                                break;

                            case MM_CHANNEL_EDIT:
                                GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_ChannelEdit[0]);
                                break;

							case MM_SYSTEM_SETTING:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_SystemSetting[0]);
								break;

							case MM_UTILITY:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Utility[0]);
								break;
#if MV_WIN_SUPPORT
#if(0 == NETWORK_SUPPORT)
							case MM_MEDIA:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Media[0]);
								break;
#else
							case MM_INTERNET:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Internet[0]);
								break;
#endif
#else
							case MM_MEDIA:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Media[0]);
								break;
							#if NETWORK_SUPPORT
							case MM_INTERNET:
								GUI_SetProperty(WIDGET_NAME_TITLE,"string",s_Internet[0]);
								break;
							#endif
#endif
							default:
								break;
						}	
						app_main_item_menu_move(STBK_RIGHT);


#else
						#if 1
						if(s_CurMenu == MM_MEDIA)
							s_CurMenu = MM_INSTALLATION;
						else
							s_CurMenu++;
						#else
						if(s_CurMenu == MM_MEDIA)
							s_CurMenu = MM_SYSTEM_SETTING;
						else
							s_CurMenu++;
						#endif
						app_main_menu_set_item_focus(s_CurMenu);
#endif
					}
					break;

				case STBK_UP:
#if MINI_16BIT_WIN8_OSD_SUPPORT
					#if 1
					if(s_CurMenu > MM_SYSTEM_SETTING)
						s_CurMenu -= MM_CHANNEL_EDIT;
					else if(s_CurMenu == MM_SYSTEM_SETTING)
						s_CurMenu = MM_SYSTEM_SETTING;
					else 
					    s_CurMenu += MM_CHANNEL_EDIT;
					#else
					if(s_CurMenu == MM_TV)
						s_CurMenu = MM_INSTALLATION;
					else if(s_CurMenu == MM_SYSTEM_SETTING)
						s_CurMenu = MM_MEDIA;
					else
					    s_CurMenu = (s_CurMenu+5)%10;
					#endif
						app_main_menu_set_item_focus(s_CurMenu);
#endif
					break;

				case STBK_DOWN:
#if MINI_16BIT_WIN8_OSD_SUPPORT
					if(s_CurMenu == MM_SYSTEM_SETTING)
						s_CurMenu = MM_SYSTEM_SETTING;
					else if(s_CurMenu < MM_SYSTEM_SETTING)
						s_CurMenu += 3;
					else 
					    s_CurMenu -= 3;
					app_main_menu_set_item_focus(s_CurMenu);
#endif
					break;

				case STBK_OK:
#if MINI_16BIT_WIN8_OSD_SUPPORT
                    GUI_SetProperty("txt_main_menu_video", "state", "hide");
					app_main_menu_display_time_status(0);
					switch(s_CurMenu)
					{
						case MM_INSTALLATION:
							memset(&list_menu, 0, sizeof(MenuSubList));
							list_menu.title = s_MenuItemName[MM_INSTALLATION];
							list_menu.item_num = sizeof(s_Installation) / sizeof(s_Installation[0]);
							list_menu.item_content = s_Installation;
							list_menu.PressOK_cb = create_sub_menu_installation;
							menu_sub_list_create(&list_menu);
							GUI_SetInterface("flush", NULL);
							g_AppPlayOps.program_stop();
							break;

						case MM_CHANNEL_EDIT:
							g_AppPlayOps.program_stop();
							memset(&list_menu, 0, sizeof(MenuSubList));
							list_menu.title = s_MenuItemName[MM_CHANNEL_EDIT];
							list_menu.item_num = sizeof(s_ChannelEdit) / sizeof(s_ChannelEdit[0]);
							list_menu.item_content = s_ChannelEdit;
							list_menu.PressOK_cb = create_sub_menu_channel;
							menu_sub_list_create(&list_menu);
							break;
						case MM_UTILITY:
							g_AppPlayOps.program_stop();
							memset(&list_menu, 0, sizeof(MenuSubList));
							list_menu.title = s_MenuItemName[MM_UTILITY];
							list_menu.item_num = sizeof(s_Utility) / sizeof(s_Utility[0]);
							list_menu.item_content = s_Utility;
							list_menu.PressOK_cb = create_sub_menu_utility;
							menu_sub_list_create(&list_menu);
							break;
						case MM_SYSTEM_SETTING:
							g_AppPlayOps.program_stop();
							GUI_CreateDialog("wnd_system_list");
							break;
						case MM_TV:
							prog_total = _check_total_channel_num();
							if(prog_total.total_tv == 0)
							{
								s_PopupType = MM_NO_CHANNEL;
                				GUI_CreateDialog("wnd_mainmenu_tip");
							}
							else
							{
								GUI_EndDialog("wnd_main_menu");
								GUI_SetProperty("wnd_full_screen", "back_ground", "null");
				                GUI_SetInterface("flush", NULL);
				                GUI_SetInterface("video_enable", NULL);
							    g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_TV;
								g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));
								g_AppFullArb.state.tv = STATE_ON;
							    GUI_CreateDialog("wnd_channel_list");
							}
							break;
						case MM_RADIO:
							prog_total = _check_total_channel_num();
							if(prog_total.total_radio == 0)
							{
								s_PopupType = MM_NO_CHANNEL;
                				GUI_CreateDialog("wnd_mainmenu_tip");
							}
							else
				            {
				            	GUI_EndDialog("wnd_main_menu");
				                g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_RADIO;
								g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));
				                g_AppFullArb.state.tv = STATE_OFF;
								GUI_SetInterface("clear_image", NULL);
					            GUI_SetInterface("flush", NULL);
					            GUI_SetProperty("wnd_full_screen", "back_ground", (void*)("bg.jpg"));
					            GUI_SetInterface("video_disable", NULL);
				                GUI_CreateDialog("wnd_channel_list");
				            }
							break;
						case MM_EPG:
							app_create_epg_menu();
							break;
						case MM_MEDIA:
							g_AppPlayOps.program_stop();
							g_AppPvrOps.tms_delete(&g_AppPvrOps); 
							GUI_CreateDialog("win_media_centre");
							break;
						default:
							break;
					}
#else
				    GUI_SendEvent(WND_MAIN_MENU_ITEM, event);
#endif

					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

#define MAINMENU_TIP
SIGNAL_HANDLER int app_mainmenu_tip_create(GuiWidget *widget, void *usrdata)
{
#define BUFFER_LENGTH		36

	TotalChannelNum total={0};
	char buffer[BUFFER_LENGTH] = {0};


	if(MM_DEL_ALL == s_PopupType)//delete all?
	{
		#if TKGS_SUPPORT
		//int
		GxTkgsOperatingMode curMode ;
		extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
		extern void app_tkgs_set_operatemode_var(GxTkgsOperatingMode mode);
		curMode  = app_tkgs_get_operatemode();
		app_tkgs_set_operatemode_var(GX_TKGS_OFF);
		total = _check_total_channel_num();
		app_tkgs_set_operatemode_var(curMode);
		#else
		total = _check_total_channel_num();
		#endif
		if(total.total_tv == 0 && total.total_radio == 0)//no channel
		{
			s_PopupType = MM_DEL_ALL_NO_CH;
			GUI_SetProperty("text_mainmenu_tip_info1", "string", STR_ID_NO_CH);		
			GUI_SetProperty("text_mainmenu_tip_info2", "string", STR_ID_INSTALL_FIRST);
			//only ok
			GUI_SetProperty("btn_mainmenu_tip_cancel", "state", "hide");
		}
		else//have channel
		{
			s_PopupType = MM_DEL_ALL_HAVE_CH;
			GUI_SetProperty("text_mainmenu_tip_info2", "string", STR_ID_DEL_ALL_CH);
			memset(buffer, 0, BUFFER_LENGTH);
			sprintf(buffer,"[ TV : %d ,  Radio : %d ]", total.total_tv, total.total_radio);
			GUI_SetProperty("text_mainmenu_tip_info1", "string", buffer);
			GUI_SetFocusWidget("btn_mainmenu_tip_ok");
		}
	}
	else if(MM_NO_CHANNEL == s_PopupType)//no channel
	{
		GUI_SetProperty("text_mainmenu_tip_info1", "string", STR_ID_NO_CH);		
		GUI_SetProperty("text_mainmenu_tip_info2", "string", STR_ID_INSTALL_FIRST);
		//only ok
		GUI_SetProperty("btn_mainmenu_tip_cancel", "state", "hide");
	}

	else if(MM_NO_SAT == s_PopupType)
	{
		GUI_SetProperty("text_mainmenu_tip_info1", "string", STR_ID_NO_SAT);		
		GUI_SetProperty("text_mainmenu_tip_info2", "string", STR_ID_INSTALL_FIRST);
		GUI_SetProperty("btn_mainmenu_tip_cancel", "state", "hide");
	}

	GUI_SetProperty("img_mainmenu_tip_opt","img","s_bar_choice_blue4.bmp");
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_mainmenu_tip_destroy(GuiWidget *widget, void *usrdata)
{
	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_mainmenu_tip_keypress(GuiWidget *widget, void *usrdata)
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
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					GUI_EndDialog("wnd_mainmenu_tip");
#if (MINI_16BIT_WIN8_OSD_SUPPORT == 0)
					GUI_SendEvent("wnd_main_menu_item", event);
#else
					GUI_SendEvent("wnd_menu_sub_list", event);
#endif
					break;
				case STBK_MENU:
				case STBK_EXIT:	
					{
						GUI_EndDialog("wnd_mainmenu_tip");
					}
					break;
				case STBK_LEFT:
				case STBK_RIGHT:
					{
						if(MM_DEL_ALL_HAVE_CH == s_PopupType)
						{
							if(0 == strcasecmp("btn_mainmenu_tip_ok", GUI_GetFocusWidget()))
							{
								GUI_SetProperty("img_mainmenu_tip_opt","img","s_bar_choice_blue4_l.bmp");
								GUI_SetFocusWidget("btn_mainmenu_tip_cancel");
							}
							else
							{
								GUI_SetProperty("img_mainmenu_tip_opt","img","s_bar_choice_blue4.bmp");
								GUI_SetFocusWidget("btn_mainmenu_tip_ok");
							}
						}
					}
					break;
				case STBK_OK:
					{
						if(MM_DEL_ALL_HAVE_CH == s_PopupType)
						{
							if(0 == strcasecmp("btn_mainmenu_tip_ok", GUI_GetFocusWidget()))
							{
								//show waiting...
								GUI_SetProperty("text_mainmenu_tip_info1", "string", STR_ID_DELETING);
								GUI_SetProperty("text_mainmenu_tip_info1", "draw_now", NULL);

								GUI_SetProperty("text_mainmenu_tip_info2", "string", " ");
								GUI_SetProperty("text_mainmenu_tip_info2", "draw_now", NULL);

								GUI_SetProperty("btn_mainmenu_tip_cancel", "state", "hide");
								GUI_SetProperty("btn_mainmenu_tip_cancel", "draw_now", NULL);
								
								GUI_SetProperty("btn_mainmenu_tip_ok", "state", "hide");
								GUI_SetProperty("btn_mainmenu_tip_ok", "draw_now", NULL);
								
								GUI_SetProperty("img_mainmenu_tip_opt", "state", "hide");									
								GUI_SetProperty("img_mainmenu_tip_opt", "draw_now", NULL);

								//del all
								#if TKGS_SUPPORT
								{
									GxTkgsOperatingMode curMode ;
									extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
									extern void app_tkgs_set_operatemode_var(GxTkgsOperatingMode mode);
									curMode  = app_tkgs_get_operatemode();
									app_tkgs_set_operatemode_var(GX_TKGS_OFF);
									_delete_all_channels();
									app_tkgs_set_operatemode_var(curMode);
								}
								#else
								_delete_all_channels();
								#endif
								GxCore_ThreadDelay(600);
							}
							if(g_AppPlayOps.normal_play.play_total == 0)
							{
								GUI_SetProperty("txt_main_menu_video", "string", " ");
							}
							GUI_EndDialog("wnd_mainmenu_tip");
						}
						else if(MM_DEL_ALL_NO_CH == s_PopupType
							|| MM_NO_CHANNEL == s_PopupType
							|| MM_NO_SAT== s_PopupType)
						{
							GUI_EndDialog("wnd_mainmenu_tip");
						}
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

#if MINI_16BIT_WIN8_OSD_SUPPORT

/************************* menu sub list xml *****************************/
SIGNAL_HANDLER int app_menu_sub_list_create(GuiWidget *widget, void *usrdata)
{
	int sel = 0;
	GUI_SetProperty(TITLE_MENU_LIST, "string", s_menulist->title);
	GUI_SetProperty(LISTVIEW_MENU_LIST, "select", &sel);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_menu_sub_list_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

#define CHECK_PVR()    do{if ((g_AppPvrOps.state == PVR_RECORD)\
                                || (g_AppPvrOps.state == PVR_TMS_AND_REC)){\
    return EVENT_TRANSFER_KEEPON;}\
}while(0)

static int _menu_sub_list_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
	int select = *(int*)usr_data;
	if(ret->result ==  PASSWD_OK)
	{
		s_menulist->PressOK_cb(select);
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
		PasswdDlg passwd_dlg;
		memset(&passwd_dlg, 0, sizeof(PasswdDlg));
		passwd_dlg.usr_data = usr_data;
		passwd_dlg.passwd_exit_cb = _menu_sub_list_passwd_dlg_cb;
		passwd_dlg_create(&passwd_dlg);
	}
	else
	{}

	return 0;	
}

SIGNAL_HANDLER int app_menu_sub_list_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;

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
				case VK_BOOK_TRIGGER:
					GUI_EndDialog(WND_MENU_SUB_LIST);
					GUI_SendEvent("wnd_main_menu", event);
					break;

				case STBK_EXIT:
				case STBK_MENU:
					
					GUI_EndDialog(WND_MENU_SUB_LIST);
					break;

				default:
					break;
			}
		default:
			break;
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_menu_sub_list_list_keypress(GuiWidget *widget, void *usrdata)
{
	static int sel = 0;
	GUI_Event *event = NULL;
	int init_value = 0;
	PasswdDlg passwd_dlg;

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
				case VK_BOOK_TRIGGER:
					GUI_EndDialog(WND_MENU_SUB_LIST);
					GUI_SendEvent("wnd_main_menu", event);
					break;

				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(WND_MENU_SUB_LIST);
					GUI_SetInterface("flush", NULL);
					//app_main_menu_display_time_status(1);
					break;

				case STBK_UP:
					GUI_GetProperty(LISTVIEW_MENU_LIST, "select", &sel);
					if(sel == 0)
					{
						sel = s_menulist->item_num - 1;
						GUI_SetProperty(LISTVIEW_MENU_LIST, "select", &sel);
						break;
					}
					return EVENT_TRANSFER_KEEPON;

				case STBK_DOWN:
					GUI_GetProperty(LISTVIEW_MENU_LIST, "select", &sel);
					if(sel == (s_menulist->item_num-1))
					{
						sel = 0;
						GUI_SetProperty(LISTVIEW_MENU_LIST, "select", &sel);
						break;
					}
					return EVENT_TRANSFER_KEEPON;

				case STBK_OK:
					if(s_menulist->PressOK_cb != NULL)
					{
                        CHECK_PVR();
						GUI_GetProperty(LISTVIEW_MENU_LIST, "select", &sel);

						GxBus_ConfigGetInt(MENU_LOCK_KEY, &init_value, MENU_LOCK);
						if(init_value == 1)
						{
							memset(&passwd_dlg, 0, sizeof(PasswdDlg));
							passwd_dlg.passwd_exit_cb = _menu_sub_list_passwd_dlg_cb;
							passwd_dlg.usr_data = (void*)&sel;
							passwd_dlg_create(&passwd_dlg);
						}
						else
						{
							s_menulist->PressOK_cb(sel);
						}
					}
					break;

				default:
					break;
			}
		default:
			break;
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_menu_sub_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	return s_menulist->item_num;
}

SIGNAL_HANDLER int app_menu_sub_list_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	item->x_offset = 0;
	item->image = NULL;
	item->string = s_menulist->item_content[item->sel];
	
	return EVENT_TRANSFER_STOP;
}

status_t menu_sub_list_create(MenuSubList *list)
{
	if(list == NULL) return GXCORE_ERROR;
	s_menulist = list;

	GUI_CreateDialog(WND_MENU_SUB_LIST);
	return GXCORE_SUCCESS;
}
/************************* menu sub list xml *****************************/

/************************** system list xml ******************************/
SIGNAL_HANDLER int app_system_list_create(GuiWidget *widget, void *usrdata)
{
	int sel = 0;
	GUI_SetProperty(LISTVIEW_SYS_LIST, "select", &sel);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_list_destroy(GuiWidget *widget, void *usrdata)
{
	s_CurList = MS_INFO;
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_list_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;

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
				case VK_BOOK_TRIGGER:
					GUI_EndDialog(WND_SYS_LIST);
					GUI_SendEvent("wnd_main_menu", event);
					break;
				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(WND_SYS_LIST);
                    GUI_SetInterface("flush", NULL);
					break;

				default:
					break;
			}
		default:
			break;
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_list_list_keypress(GuiWidget *widget, void *usrdata)
{
	int sel = 0;
	GUI_Event *event = NULL;

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
				case VK_BOOK_TRIGGER:
					GUI_EndDialog(WND_SYS_LIST);
					GUI_SendEvent("wnd_main_menu", event);
					break;
				case STBK_EXIT:
				case STBK_MENU:
					//app_main_menu_display_time_status(1);
					GUI_EndDialog(WND_SYS_LIST);
					break;

				case STBK_UP:
					GUI_GetProperty(LISTVIEW_SYS_LIST, "select", &sel);
					if(sel == 0)
					{
						sel = MS_TOTAL-MS_INFO - 1;
						GUI_SetProperty(LISTVIEW_SYS_LIST, "select", &sel);
						break;
					}
					return EVENT_TRANSFER_KEEPON;

				case STBK_DOWN:
					GUI_GetProperty(LISTVIEW_SYS_LIST, "select", &sel);
					if(sel == (MS_TOTAL-MS_INFO-1))
					{
						sel = 0;
						GUI_SetProperty(LISTVIEW_SYS_LIST, "select", &sel);
						break;
					}
					return EVENT_TRANSFER_KEEPON;

				case STBK_OK:
				case STBK_RIGHT:
					GUI_SetFocusWidget(LISTVIEW_SYS_LIST_SUB);
					break;

				default:
					break;
			}
		default:
			break;
	}

	return EVENT_TRANSFER_STOP;
}

void create_system_list_sub_info(int select)
{
	switch(select)
	{
		case MM_SYSINFO:
			app_sysinfo_menu_exec();
			break;

		default:
			break;
	}
	return;
}

void create_system_list_sub_system(int select)
{
	switch(select)
	{
		case MM_TIME_SETTING:
			app_time_setting_menu_exec();
			break;

		case MM_UPGRADE:
			app_upgrade_menu_exec();
			break;

		case MM_RESET:
			if ((g_AppPvrOps.state != PVR_RECORD)
					&& (g_AppPvrOps.state != PVR_TMS_AND_REC))
			{             
				PasswdDlg passwd_dlg;
				memset(&passwd_dlg, 0, sizeof(PasswdDlg));
				passwd_dlg.passwd_exit_cb = _reset_passwd_dlg_cb;
				passwd_dlg_create(&passwd_dlg);
			}
			break;

		default:
			break;
	}

	return;
}

int _info_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
	int select = *(int*)usr_data;
	if(ret->result ==  PASSWD_OK)
	{	
		create_system_list_sub_info(select);
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
		PasswdDlg passwd_dlg;
		memset(&passwd_dlg, 0, sizeof(PasswdDlg));
		passwd_dlg.usr_data = usr_data;
		passwd_dlg.passwd_exit_cb = _info_passwd_dlg_cb;
		passwd_dlg_create(&passwd_dlg);
	}
	else
	{}

	return 0;	
}

int _sys_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
	int select = *(int*)usr_data;
	if(ret->result ==  PASSWD_OK)
	{	
		create_system_list_sub_system(select);
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
		PasswdDlg passwd_dlg;
		memset(&passwd_dlg, 0, sizeof(PasswdDlg));
		passwd_dlg.usr_data = usr_data;
		passwd_dlg.passwd_exit_cb = _sys_passwd_dlg_cb;
		passwd_dlg_create(&passwd_dlg);
	}
	else
	{}

	return 0;	
}

SIGNAL_HANDLER int app_system_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	return (MS_TOTAL-MS_INFO);
}

SIGNAL_HANDLER int app_system_list_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	//col-0: img
	item->x_offset = 0;
	item->image = NULL;
	item->string = NULL;

	//col-1: name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	item->string = s_SysList[item->sel];
	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_list_list_change(GuiWidget *widget, void *usrdata)
{
	int sel = 0;
	GUI_GetProperty(LISTVIEW_SYS_LIST, "select", (void *)(&sel));
	s_CurList = sel + MS_INFO;
	GUI_SetProperty(LISTVIEW_SYS_LIST_SUB, "update_all", (void *)(NULL));
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_list_list_sub_keypress(GuiWidget *widget, void *usrdata)
{
	static int sel = 0;
	int num = 0;
	GUI_Event *event = NULL;
	int init_value = 0;
	PasswdDlg passwd_dlg;

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
				case VK_BOOK_TRIGGER:
					GUI_EndDialog(WND_SYS_LIST);
					GUI_SendEvent("wnd_main_menu", event);
					break;
				case STBK_MENU:
					//app_main_menu_display_time_status(1);
					GUI_EndDialog(WND_SYS_LIST);
					break;

				case STBK_EXIT:
					GUI_SetFocusWidget(LISTVIEW_SYS_LIST);
					break;

				case STBK_UP:
					GUI_GetProperty(LISTVIEW_SYS_LIST_SUB, "select", &sel);
					if(sel == 0)
					{
						switch(s_CurList)
						{
							case MS_INFO:
								num = sizeof(s_Info)/sizeof(*s_Info);
								break;
							case MS_SYSTEM:
								num = sizeof(s_System)/sizeof(*s_System);
								break;
							case MS_SETTING:
								num = sizeof(s_Setting)/sizeof(*s_Setting);
								break;
							default:
								break;
						}
						sel = num - 1;
						GUI_SetProperty(LISTVIEW_SYS_LIST_SUB, "select", &sel);
						break;
					}
					return EVENT_TRANSFER_KEEPON;

				case STBK_DOWN:
					GUI_GetProperty(LISTVIEW_SYS_LIST_SUB, "select", &sel);
					switch(s_CurList)
					{
						case MS_INFO:
							num = sizeof(s_Info)/sizeof(*s_Info);
							break;
						case MS_SYSTEM:
							num = sizeof(s_System)/sizeof(*s_System);
							break;
						case MS_SETTING:
							num = sizeof(s_Setting)/sizeof(*s_Setting);
							break;
						default:
							break;
					}
					if(sel == (num-1))
					{
						sel = 0;
						GUI_SetProperty(LISTVIEW_SYS_LIST_SUB, "select", &sel);
						break;
					}
					return EVENT_TRANSFER_KEEPON;

				case STBK_OK:
					GUI_GetProperty(LISTVIEW_SYS_LIST_SUB, "select", &sel);
					switch(s_CurList)
					{
						case MS_INFO:
							sel = sel + MM_SYSINFO;
							GxBus_ConfigGetInt(MENU_LOCK_KEY, &init_value, MENU_LOCK);
							if((init_value == 1) && (sel != MM_SYSINFO))
							{
								memset(&passwd_dlg, 0, sizeof(PasswdDlg));
								passwd_dlg.passwd_exit_cb = _info_passwd_dlg_cb;
								passwd_dlg.usr_data = (void*)&sel;
								passwd_dlg_create(&passwd_dlg);
							}
							else
							{
								create_system_list_sub_info(sel);
							}
							break;

						case MS_SYSTEM:
							sel = sel + MM_TIME_SETTING;
							GxBus_ConfigGetInt(MENU_LOCK_KEY, &init_value, MENU_LOCK);
							if((init_value == 1) && (sel != MM_RESET))
							{
								memset(&passwd_dlg, 0, sizeof(PasswdDlg));
								passwd_dlg.passwd_exit_cb = _sys_passwd_dlg_cb;
								passwd_dlg.usr_data = (void*)&sel;
								passwd_dlg_create(&passwd_dlg);
							}
							else
							{
								create_system_list_sub_system(sel);
							}
							break;

						case MS_SETTING:
							sel = sel + MM_TIMER_SETTING;
							GxBus_ConfigGetInt(MENU_LOCK_KEY, &init_value, MENU_LOCK);
							if(init_value == 1)
							{
								memset(&passwd_dlg, 0, sizeof(PasswdDlg));
								passwd_dlg.passwd_exit_cb = _sys_setting_passwd_dlg_cb;
								passwd_dlg.usr_data = (void*)&sel;
								passwd_dlg_create(&passwd_dlg);
							}
							else
							{
								create_sub_menu_syssetting(sel);
							}	
							break;

						default:
							break;
					}
					break;

				case STBK_LEFT:
					GUI_SetFocusWidget(LISTVIEW_SYS_LIST);
					break;

				default:
					break;
			}
		default:
			break;
	}

	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_system_list_list_sub_get_total(GuiWidget *widget, void *usrdata)
{
	int num = 0;
	switch(s_CurList)
	{
		case MS_INFO:
			num = sizeof(s_Info)/sizeof(*s_Info);
			break;
		case MS_SYSTEM:
			num = sizeof(s_System)/sizeof(*s_System);
			break;
		case MS_SETTING:
			num = sizeof(s_Setting)/sizeof(*s_Setting);
			break;
		default:
			break;
	}
	return num;
}

SIGNAL_HANDLER int app_system_list_list_sub_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	item->x_offset = 0;
	item->image = NULL;
	switch(s_CurList)
	{
		case MS_INFO:
			item->string = s_Info[item->sel];
			break;
		case MS_SYSTEM:
			item->string = s_System[item->sel];
			break;
		case MS_SETTING:
			item->string = s_Setting[item->sel];
			break;
		default:
			break;
	}
	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_list_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_system_list_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
#endif
