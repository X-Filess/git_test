#include "app.h"
#include "app_frontend.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_utility.h"
#include "full_screen.h"
#include "app_epg.h"
#include "app_pop.h"
#if TKGS_SUPPORT
#include "module/gxtkgs.h"
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
#endif

/* Private variable------------------------------------------------------- */
static event_list* sp_ChanInfoTimer = NULL;
static event_list* sp_TimerSignal = NULL;
static AppEpgOps s_ch_epg_ops;
static GxEpgInfo *s_cur_epg_info = NULL;
static GxEpgInfo *s_next_epg_info = NULL;
static EpgProgInfo s_epg_prog_info;
static int s_timer_cnt = 0;
static GxBusPmDataProgType s_stream_bak = GXBUS_PM_PROG_ALL;
static void _channel_info_show(void);

extern uint32_t GX1131_Get_ErrorRate(uint32_t *E_param);
extern int app_channel_info_get_prog_lock_flag(void);
extern int app_get_ca_system_pid_status(int mode,int *count);
/* widget macro -------------------------------------------------------- */
#define WND_PROGBAR_INFO "wnd_channel_info_signal"
#define TEXT_CHANNEL_INFO "text_signal_parameters"
#define TEXT_SIGNAL_TYPE "text_signal_type"
#define IMG_TV_RADIO "img_progbar_tv_radio"
#define TEXT_EPG_CUR "text_channel_info_current"
#define TEXT_EPG_NEXT "text_channel_info_next"
#define TEXT_EPG_CUR_T "text_channel_info_current_time"
#define TEXT_EPG_NEXT_T "text_channel_info_next_time"
#define TEXT_SIGNA_VIDEO_PID "text_signal_video_pid_value"
#define TEXT_SIGNA_AUDIO_PID "text_signal_audio_pid_value"
#define TEXT_SIGNA_PCR_PID "text_signal_pcr_pid_value"

typedef struct CaSystem_s
{
	uint16_t m_wCasIdBigen;
	uint16_t m_wCasIdEnd;
	uint8_t m_chCasName[MAX_CAS_NAME];
}CaSystem_t;

CaSystem_t default_ca_system[MAX_CA_SYSTEM]= //TODO
{
    {CAS_ID_FREE,	CAS_ID_FREE,		"Free"},
    {0x0500,			0x05ff,			"Viaccess"},
    {0x0100,			0x01ff,			"SECA"},
    {0x0600,			0x06ff,			"Irdeto"},//IRDETO
    {0x1800,			0x18ff,			"Nagravision"},
    {0x001,			    0x001,			"Alphacrypt"},//?
    {0x1700,			0x17ff,			"Betacrypt"},
    {0x0b00,            0x0bff,		    "Conax"},
    {0x0d00,            0x0dff,         "Cryptoworks"},
    {0x0900,            0x09ff,			"NDS"},
    {0x0e00,            0x0eff,         "PowerVU"},
    {0x001,             0x001,          "Skycrypt"},//?
    {0x001,             0x001,          "Firecrpt"},//?
    {0x4a00,            0x4aff,			"Xcrypt"},
    {0x2600,            0x26ff,			"BISS"},
    {CAS_ID_UNKNOW,     CAS_ID_UNKNOW,	"Unknow"}
};

static int app_get_ca_system_by_id(uint16_t cas_id, CaSystem_t** pp_ca_system)//sub
{
	uint16_t index = 0;
	static uint16_t ret_value = 0;
	
	*pp_ca_system = (CaSystem_t*)&default_ca_system[MAX_CA_SYSTEM - 1];

	for( index = 0; index < MAX_CA_SYSTEM; index++)//MAX_CA_SYSTEM - 1 is unknow //zfz 20070828
	{

		 if(cas_id >= default_ca_system[index].m_wCasIdBigen &&
			cas_id <= default_ca_system[index].m_wCasIdEnd)
		 {
		 	*pp_ca_system = (CaSystem_t*)&default_ca_system[index];
			ret_value = index;
			return index;
		 }
	}


	return ret_value;
}

static void _channel_info_show_ca_system_name(uint16_t cas_id)
{
	#define MAX_CAS_TOTAL	16
	#define SPACE           " "
	#define MAX_BUF_LEN		200
	char buffer[MAX_BUF_LEN];
	CaSystem_t *pCaNode;
	int count  = 1;
	int card_num  = 0;
	int flag = 0;
	uint16_t pmt_cas_id = 0;
	uint8_t index  = 0;
	
	memset(buffer, 0, MAX_BUF_LEN);
	GUI_SetProperty("text_channel_info_smartcard", "string", buffer);
	app_get_ca_system_pid_status(0,&count);
	if((count>MAX_CAS_TOTAL)||(count<0))
	{
		count = MAX_CAS_TOTAL-1;
	}

	if(count == 0)
	{
		GUI_SetProperty("text_channel_info_smartcard", "string", NULL);
		return;
	}
	
	for(index = 0; index < count; index++ )
	{
		pmt_cas_id = app_get_ca_system_pid_status(index,&count);
		card_num = app_get_ca_system_by_id(pmt_cas_id, &pCaNode);

		if((flag & (1 << card_num)) != 0)
		{
			continue;
		}
		flag |= (1 << card_num);

		if((strlen(buffer)+strlen((char*)pCaNode->m_chCasName) + 1) > MAX_BUF_LEN -1)
		{
			printf("[%s]---error--len--\n",__FUNCTION__);
			break;
		}
		
       	sprintf(strlen(buffer)+buffer,"%s",pCaNode->m_chCasName);
		strcat(buffer,SPACE);	
		
	}
	
	GUI_SetProperty("text_channel_info_smartcard", "string", buffer);
	return;
}

#if 0
static void _channel_info_show_dobly_status(uint32_t ac3_stauts)
{
#define IMG_DD    "img_channel_info_ac3"

	GUI_SetProperty(IMG_DD, "img", "DD.bmp");

	 if( (ac3_stauts== GXBUS_PM_AUDIO_AC3)
  	||(ac3_stauts == GXBUS_PM_AUDIO_AC3_PLUS)
  	||(ac3_stauts == GXBUS_PM_AUDIO_EAC3)
  	||(ac3_stauts== GXBUS_PM_AUDIO_AC3_PLUS_SEC))
	{
	    GUI_SetProperty(IMG_DD, "state", "show");
	}
	else
	{
	    GUI_SetProperty(IMG_DD, "state", "hide");
	}
	GUI_SetProperty(IMG_DD, "draw_now", NULL);
	return;
}
#endif

static int timer_channel_info_timeout(void *userdata)
{
	if((g_AppFullArb.state.tv == STATE_OFF)
        || (GUI_CheckDialog("wnd_passwd") == GXCORE_SUCCESS)
        || (GUI_CheckDialog(WND_PROGBAR_INFO) == GXCORE_SUCCESS))
	{
        if(GUI_CheckDialog("wnd_pvr_bar") != GXCORE_SUCCESS)
        {
		    GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &s_timer_cnt, INFOBAR_TIMEOUT);
            _channel_info_show();
        }
	}
	else
	{
        s_timer_cnt--;
		if(s_timer_cnt == 0)
		{
        	if(NULL != sp_ChanInfoTimer)
            {      
            	remove_timer(sp_ChanInfoTimer);
                sp_ChanInfoTimer = NULL;
            }
			GUI_EndDialog("wnd_channel_info");
		}
		else
		{
            _channel_info_show();
		}
	}

    return 0;
}

#if 0
static void timer_channel_info_timeout_stop(void)
{
	timer_stop(sp_ChanInfoTimer);
}
#endif

static void timer_channel_info_timeout_reset(void)
{
	int init_value = INFOBAR_TIMEOUT;

	GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);
	s_timer_cnt = init_value;
    //remove_timer(sp_ChanInfoTimer);
	if(reset_timer(sp_ChanInfoTimer) != 0)
	{
		sp_ChanInfoTimer = create_timer(timer_channel_info_timeout, 1000, NULL, TIMER_REPEAT);
	}
}

#define SUB_FUNC
static void _channel_info_show_epg(void)
{
#define CH_EPG_TIME_LEN 15
#define MAX_EVENT_LEN   150

    time_t time = 0;
    char *time_str = NULL;
    uint8_t epg_cur_str[CH_EPG_TIME_LEN + MAX_EVENT_LEN + 1] = {0};
    uint8_t epg_next_str[CH_EPG_TIME_LEN + MAX_EVENT_LEN + 1] = {0};
    bool cur_flag = false;
    bool next_flag = false;
    char *tmp_str = NULL;
    uint16_t cp_len = 0;

    s_ch_epg_ops.prog_info_update(&s_ch_epg_ops, &s_epg_prog_info);
    s_ch_epg_ops.event_cnt_update(&s_ch_epg_ops);

    if(s_ch_epg_ops.get_cur_event_cnt(&s_ch_epg_ops) > 0)
    {
        if(s_cur_epg_info != NULL)
        {
            if(s_ch_epg_ops.get_cur_event_info(&s_ch_epg_ops, s_cur_epg_info) == GXCORE_SUCCESS && s_cur_epg_info->start_time != 0 )
            {
                if(s_cur_epg_info->event_name_len > 0)
                {
                    memset(epg_cur_str, 0, CH_EPG_TIME_LEN + MAX_EVENT_LEN + 1);
                    time = get_display_time_by_timezone(s_cur_epg_info->start_time);
                    time_str = app_time_to_hourmin_edit_str(time);
                    if(time_str != NULL)
                    {
                        strcpy((char*)epg_cur_str, time_str);
                        GxCore_Free(time_str);
                        time_str = NULL;
                    }
                    epg_cur_str[5] = ' ';
                    epg_cur_str[6] = '-';
                    epg_cur_str[7] = ' ';
                    time = get_display_time_by_timezone(s_cur_epg_info->start_time + s_cur_epg_info->duration);
                    time_str = app_time_to_hourmin_edit_str(time);
                    if(time_str != NULL)
                    {
                        strcpy((char*)epg_cur_str + 8, time_str);
                        GxCore_Free(time_str);
                        time_str = NULL;
                    }
                    epg_cur_str[13] = ' ';
                    epg_cur_str[14] = ' ';

                    (s_cur_epg_info->event_name_len < MAX_EVENT_LEN) ? (cp_len = s_cur_epg_info->event_name_len) : (cp_len = MAX_EVENT_LEN);
                    strncpy((char*)(epg_cur_str + CH_EPG_TIME_LEN), (char*)s_cur_epg_info->event_name, cp_len);
                    cur_flag = true;
                }
            }
        }

        if(s_next_epg_info != NULL)
        {
            if(s_ch_epg_ops.get_next_event_info(&s_ch_epg_ops, s_next_epg_info) == GXCORE_SUCCESS && s_next_epg_info->start_time != 0)
            {
                if(s_next_epg_info->event_name_len > 0)
                {
                    memset(epg_next_str, 0, CH_EPG_TIME_LEN + MAX_EVENT_LEN + 1);
                    time = get_display_time_by_timezone(s_next_epg_info->start_time);
                    time_str = app_time_to_hourmin_edit_str(time);
                    if(time_str != NULL)
                    {
                        strcpy((char*)epg_next_str, time_str);
                        GxCore_Free(time_str);
                        time_str = NULL;
                    }
                    epg_next_str[5] = ' ';
                    epg_next_str[6] = '-';
                    epg_next_str[7] = ' ';
                    time = get_display_time_by_timezone(s_next_epg_info->start_time + s_next_epg_info->duration);
                    time_str = app_time_to_hourmin_edit_str(time);
                    if(time_str != NULL)
                    {
                        strcpy((char*)epg_next_str + 8, time_str);
                        GxCore_Free(time_str);
                        time_str = NULL;
                    }
                    epg_next_str[13] = ' ';
                    epg_next_str[14] = ' ';

                    (s_next_epg_info->event_name_len < MAX_EVENT_LEN) ? (cp_len = s_next_epg_info->event_name_len) : (cp_len = MAX_EVENT_LEN);
                    strncpy((char*)(epg_next_str + CH_EPG_TIME_LEN), (char*)s_next_epg_info->event_name, cp_len);
                    next_flag = true;
                }
            }
        }
    }

    if(cur_flag == false)
    {
        memset(epg_cur_str, 0, CH_EPG_TIME_LEN + MAX_EVENT_LEN + 1);
        strcpy((char*)epg_cur_str, STR_ID_NO_INFO);
    }
    if(next_flag == false)
    {
        memset(epg_next_str, 0, CH_EPG_TIME_LEN + MAX_EVENT_LEN + 1);
        strcpy((char*)epg_next_str, STR_ID_NO_INFO);
    }

    GUI_GetProperty(TEXT_EPG_CUR, "string", &tmp_str);
    if((tmp_str == NULL)
            || (strncmp(tmp_str, (char*)epg_cur_str, strlen((char*)epg_cur_str)) != 0))
    {
        GUI_SetProperty(TEXT_EPG_CUR, "string", epg_cur_str);
    }
    GUI_GetProperty(TEXT_EPG_NEXT, "string", &tmp_str);
    if((tmp_str == NULL)
            || (strncmp(tmp_str, (char*)epg_next_str, strlen((char*)epg_next_str)) != 0))
    {
        GUI_SetProperty(TEXT_EPG_NEXT, "string", epg_next_str);
    }
}

static void _channel_info_show(void)
{
#define  IMG_TTX        "img_progbar_ttx"
#define  IMG_SUBT       "img_progbar_subt"
#define  IMG_LOCK       "img_progbar_lock"
#define  IMG_SCRAMBLE   "img_progbar_scramble"
#define  IMG_TUNER		"img_progbar_tuner"
#define  IMG_HD			"img_progbar_hd"
#define  IMG_TOTAL      (4)

    GxMsgProperty_NodeByPosGet node_prog = {0};
    GxMsgProperty_NodeByIdGet node_tp = {0};
    char Buffer[100];
    char time_str[24] = {0};
    char* flag_img_widget[IMG_TOTAL] = {IMG_TTX, IMG_SUBT, IMG_LOCK, IMG_SCRAMBLE};
    char *image_flag_focus[IMG_TOTAL] = {"s_bar_ttx.bmp", "s_bar_subt.bmp", "s_bar_lock.bmp", "s_bar_money.bmp"};
    char *image_flag_unfocus[IMG_TOTAL] = {"s_bar_ttx_unfocus.bmp", "s_bar_subt_unfocus.bmp", "s_bar_lock_unfocus.bmp", "s_bar_money_unfocus.bmp"};
    GxBusPmDataProgSwitch flag_type[IMG_TOTAL] = {0};
    char *tmp_str = NULL;
	#if TKGS_SUPPORT
	AppProgExtInfo prog_ext_info = {{0}};
	#endif
    uint32_t i = 0;

    //tv/radio
    if((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
            && (s_stream_bak != GXBUS_PM_PROG_RADIO))
    {
        //stream_bak = GXBUS_PM_PROG_RADIO;
        GUI_SetProperty(IMG_TV_RADIO, "img", "s_bar_radio.bmp");
    }

    if((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
            && (s_stream_bak != GXBUS_PM_PROG_TV))
    {
        //stream_bak = GXBUS_PM_PROG_TV;
        GUI_SetProperty(IMG_TV_RADIO, "img", "s_bar_tv.bmp");
    }

    // prog
    node_prog.node_type = NODE_PROG;
    node_prog.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

    flag_type[0] = node_prog.prog_data.ttx_flag;
    flag_type[1] = node_prog.prog_data.subt_flag | node_prog.prog_data.cc_flag;
    flag_type[2] = node_prog.prog_data.lock_flag;
    if(GXBUS_PM_PROG_BOOL_ENABLE != node_prog.prog_data.scramble_flag)
        flag_type[3] = GXBUS_PM_PROG_BOOL_DISABLE;
    else
        flag_type[3] = GXBUS_PM_PROG_BOOL_ENABLE;

    memset(Buffer, 0, 100);

    //count
#if TKGS_SUPPORT
	if(GX_TKGS_OFF != app_tkgs_get_operatemode())
	{
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node_prog.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
		{
			sprintf((char*)Buffer,"%04d",   prog_ext_info.tkgs.logicnum);
		}
		else
		{
			sprintf((char*)Buffer,"%04d",   g_AppPlayOps.normal_play.play_count+1);
		}
	}
	else
	{
		sprintf((char*)Buffer,"%04d",   g_AppPlayOps.normal_play.play_count+1);
	}
#else 
    sprintf((char*)Buffer,"%04d",   g_AppPlayOps.normal_play.play_count+1);
#endif
    GUI_GetProperty("text_channel_info_name_num", "string", &tmp_str);
    if((tmp_str == NULL)
            || (strcmp(tmp_str, Buffer) != 0)
            || (s_stream_bak != g_AppPlayOps.normal_play.view_info.stream_type))
    {
        GUI_SetProperty("text_channel_info_name_num", "string", Buffer);
        s_stream_bak = g_AppPlayOps.normal_play.view_info.stream_type;
    }

    //name
    GUI_GetProperty("text_channel_info_name", "string", &tmp_str);
    if((tmp_str == NULL)
            || (strcmp(tmp_str, (char*)node_prog.prog_data.prog_name) != 0))
    {
        GUI_SetProperty("text_channel_info_name", "string", node_prog.prog_data.prog_name);
    }

    memset(Buffer, 0, 100);

#if DOUBLE_S2_SUPPORT
    // Tuner input show
    if(node_prog.prog_data.tuner == 1)
    {
        strcpy(Buffer, "T2 - ");
    }
    else
    {
        strcpy(Buffer, "T1 - ");
    }
#endif

    //group
    if(GROUP_MODE_ALL == g_AppPlayOps.normal_play.view_info.group_mode)//all satellite
    {
        //sprintf(Buffer+strlen(Buffer), "%s - ", STR_ID_ALL);
        sprintf(Buffer+strlen(Buffer), "%s", STR_ID_ALL);
    }
    else if(GROUP_MODE_SAT == g_AppPlayOps.normal_play.view_info.group_mode)//sat
    {
        GxMsgProperty_NodeByIdGet node_sat = {0};

        node_sat.node_type = NODE_SAT;
        node_sat.id = g_AppPlayOps.normal_play.view_info.sat_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat);

        //sprintf(Buffer+strlen(Buffer), "%s - ", tmp_group);
        sprintf(Buffer+strlen(Buffer), "%s", node_sat.sat_data.sat_s.sat_name);
    }
    else if(GROUP_MODE_FAV == g_AppPlayOps.normal_play.view_info.group_mode)//fav
    {
        GxMsgProperty_NodeByIdGet node_fav = {0};

        node_fav.node_type = NODE_FAV_GROUP;
        node_fav.id = g_AppPlayOps.normal_play.view_info.fav_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_fav);

        //sprintf(Buffer+strlen(Buffer), "%s - ", tmp_group);
        sprintf(Buffer+strlen(Buffer), "%s", node_fav.fav_item.fav_name);
    }

	GUI_SetProperty("text_channel_info_sat", "string", Buffer);
    memset(Buffer, 0, 100);

    //tp
    node_tp.node_type = NODE_TP;
    node_tp.id = node_prog.prog_data.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

    GxMsgProperty_NodeByIdGet pmdata_sat = {0};
    pmdata_sat.node_type = NODE_SAT;
    pmdata_sat.id = node_prog.prog_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &pmdata_sat);

#if DEMOD_DVB_T
    if(GXBUS_PM_SAT_DVBT2 == pmdata_sat.sat_data.type)
    {
        char* bandwidth_str[3] = {"8M", "7M","6M"};

        if(node_tp.tp_data.tp_dvbt2.bandwidth <BANDWIDTH_AUTO )
            sprintf(Buffer + strlen(Buffer), "%d.%d/%s",node_tp.tp_data.frequency/1000, \
                    (node_tp.tp_data.frequency/100)%10,bandwidth_str[node_tp.tp_data.tp_dvbt2.bandwidth]);
    }
#endif
#if DEMOD_DVB_C
    if(GXBUS_PM_SAT_C == pmdata_sat.sat_data.type)
    {
        char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};

        if(node_tp.tp_data.tp_c.modulation < QAM_AUTO)
            sprintf(Buffer + strlen(Buffer),"%d/%s /%d", node_tp.tp_data.frequency/1000  \
                    ,modulation_str[node_tp.tp_data.tp_c.modulation],node_tp.tp_data.tp_c.symbol_rate/1000);
    }
#endif

#if DEMOD_DTMB
    if(GXBUS_PM_SAT_DTMB == pmdata_sat.sat_data.type)
    {
    	if(GXBUS_PM_SAT_1501_DTMB == pmdata_sat.sat_data.sat_dtmb.work_mode)
	    {
	        char* bandwidth_str[3] = {"8M", "7M","6M"};// have 7M?

	        if(node_tp.tp_data.tp_dtmb.symbol_rate < BANDWIDTH_AUTO )
	            sprintf(Buffer + strlen(Buffer), "%d.%d/%s",node_tp.tp_data.frequency/1000, \
	                    (node_tp.tp_data.frequency/100)%10,bandwidth_str[node_tp.tp_data.tp_dtmb.symbol_rate]);
	    }
		else
		{// for dvbc mode
			char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};

	        if(node_tp.tp_data.tp_dtmb.modulation < QAM_AUTO)
	            sprintf(Buffer + strlen(Buffer),"%d/%s /%d", node_tp.tp_data.frequency/1000  \
	                    ,modulation_str[node_tp.tp_data.tp_dtmb.modulation],node_tp.tp_data.tp_dtmb.symbol_rate/1000);
		}
    }
#endif


#if DEMOD_DVB_S
    if(GXBUS_PM_SAT_S == pmdata_sat.sat_data.type)
    {
        char* PolarStr[2] = {"H", "V"};
        sprintf(Buffer + strlen(Buffer), "%d / %s / %d", node_tp.tp_data.frequency, (pmdata_sat.sat_data.type != GXBUS_PM_SAT_S) ? "NULL" :PolarStr[node_tp.tp_data.tp_s.polar], node_tp.tp_data.tp_s.symbol_rate );
    }
#endif

    GUI_GetProperty("text_channel_info_group", "string", &tmp_str);
    if((tmp_str == NULL)
            || (strcmp(tmp_str, Buffer) != 0))
    {
        GUI_SetProperty("text_channel_info_group", "string", Buffer);
    }

    _channel_info_show_ca_system_name(node_prog.prog_data.cas_id);
    if(g_AppFullArb.state.tv == STATE_ON)
    {
        char *video_type[] = {"MPEG2", "AVS", "MPEG4","HEVC"};
        //AppAppProgExtInfo prog_ext;
        //format
        if(node_prog.prog_data.video_type >= sizeof(video_type)/sizeof(video_type[0]))
        {
            GUI_SetProperty("text_channel_info_avformat", "string", NULL);
        }
        else
        {
            GUI_GetProperty("text_channel_info_avformat", "string", &tmp_str);
            if((tmp_str == NULL)
                    || (strcmp(tmp_str, video_type[node_prog.prog_data.video_type]) != 0))
            {
                GUI_SetProperty("text_channel_info_avformat", "string", video_type[node_prog.prog_data.video_type]);
            }
        }

#if 0
        memset(&prog_ext, 0, sizeof(AppAppProgExtInfo));
        if(GxBus_PmProgExtInfoGet(node_prog.prog_data.id, sizeof(AppAppProgExtInfo), &prog_ext) == GXCORE_SUCCESS)
        {
            char solution_str[15] = {0};
            sprintf(solution_str, "%d x %d", prog_ext.video_solution.width, prog_ext.video_solution.height);
            GUI_GetProperty("text_channel_info_solution", "string", &tmp_str);
            if((tmp_str == NULL)
                    || (strcmp(tmp_str, solution_str) != 0))
            {
                GUI_SetProperty("text_channel_info_solution", "string", solution_str);
            }
        }
        else
        {
            GUI_SetProperty("text_channel_info_solution", "string", NULL);
        }
#else
        {
			if(g_AppFullArb.tip == FULL_STATE_DUMMY)
			{
				PlayerProgInfo  info;
				int32_t ret = 0;
				ret = GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, &info); //solution check
				if(ret == 0 && (info.video.width >= 0 && info.video.width <= 1920)&&( info.video.height != 0&&info.video.height<=1088))
				{
					char solution_str[15] = {0};

					sprintf(solution_str, "%d x %d", info.video.width, info.video.height);
					//                GUI_SetProperty("text_channel_info_solution", "string", solution_str);
					GUI_GetProperty("text_channel_info_solution", "string", &tmp_str);
					if((tmp_str == NULL)
							|| (strcmp(tmp_str, solution_str) != 0))
					{
						GUI_SetProperty("text_channel_info_solution", "string", solution_str);
					}
				}
				else
				{
					GUI_SetProperty("text_channel_info_solution", "string", NULL);
				}    
			}
			else
			{
				 GUI_SetProperty("text_channel_info_solution", "string", NULL);
			}
		}
#endif
    }
    else
    {
        char *audio_type[] = {"MPEG-1", "MPEG-2", "AAC", "AC3", "AAC","EAC3","LPCM","DTS","DOLBY","AC3","DTS","DTS","AC3","DTS"};
        if(node_prog.prog_data.cur_audio_type >= sizeof(audio_type)/sizeof(audio_type[0]))
        {
            GUI_SetProperty("text_channel_info_avformat", "string", NULL);
        }
        else
        {
            GUI_GetProperty("text_channel_info_avformat", "string", &tmp_str);
            if((tmp_str == NULL)
                    || (strcmp(tmp_str, audio_type[node_prog.prog_data.cur_audio_type]) != 0))
            {
                GUI_SetProperty("text_channel_info_avformat", "string", audio_type[node_prog.prog_data.cur_audio_type]);
            }
        }

        GUI_SetProperty("text_channel_info_solution", "string", NULL);
    }

    //epg
    s_epg_prog_info.ts_id= node_prog.prog_data.ts_id;
    s_epg_prog_info.orig_network_id= node_prog.prog_data.original_id;
    s_epg_prog_info.service_id= node_prog.prog_data.service_id;
    s_epg_prog_info.prog_id= node_prog.prog_data.id;
    s_epg_prog_info.prog_pos = node_prog.pos;
// add the tp id ,use to identify the different TS's EPG
	// s_epg_prog_info.tp_id    = node_prog.prog_data.tp_id;
    _channel_info_show_epg();

    // lock scramble subt & ttx
    for (i=0; i<IMG_TOTAL; i++)
    {
#if 0
        if (flag_type[i] == GXBUS_PM_PROG_BOOL_ENABLE)
            GUI_SetProperty(flag_img_widget[i], "state", "show");
        else
            GUI_SetProperty(flag_img_widget[i], "state", "hide");
#endif
        if (flag_type[i] == GXBUS_PM_PROG_BOOL_ENABLE)
            GUI_SetProperty(flag_img_widget[i], "img", image_flag_focus[i]);
        else
            GUI_SetProperty(flag_img_widget[i], "img", image_flag_unfocus[i]);
    }
	if( flag_type[3] != GXBUS_PM_PROG_BOOL_ENABLE)
		GUI_SetProperty("text_channel_info_smartcard", "string", NULL);

    /*
#if DOUBLE_S2_SUPPORT
    // Tuner input show
    if(node_prog.prog_data.tuner == 1)
    GUI_SetProperty(IMG_TUNER, "img", "s_ts_t2.bmp");
    else
    GUI_SetProperty(IMG_TUNER, "img", "s_ts_t1.bmp");
#else
GUI_SetProperty(IMG_TUNER, "state", "hide");
#endif
*/
    if(GXBUS_PM_PROG_HD == node_prog.prog_data.definition)
    {
        GUI_SetProperty(IMG_HD, "img", "s_bar_hd.bmp");
    }
    else
    {
        GUI_SetProperty(IMG_HD, "img", "s_bar_hd_unfocus.bmp");
    }
    // time
    g_AppTime.time_get(&g_AppTime, time_str, sizeof(time_str));

    GUI_GetProperty("text_channel_info_time", "string", &tmp_str);
    if((tmp_str == NULL)
            || (strcmp(tmp_str, time_str+MIN_SEC_OFFSET) != 0))
    {
        GUI_SetProperty("text_channel_info_time", "string", time_str+MIN_SEC_OFFSET);
    }

    time_str[MIN_SEC_OFFSET] = '\0';
    GUI_GetProperty("text_channel_info_date", "string", &tmp_str);
    if((tmp_str == NULL)
            || (strcmp(tmp_str, time_str) != 0))
    {
        GUI_SetProperty("text_channel_info_date", "string", time_str);
    }
}

void app_channel_info_update(void)
{
    _channel_info_show();
    timer_channel_info_timeout_reset();
}

#define CHANNEL_INFO
SIGNAL_HANDLER int app_channel_info_create(GuiWidget *widget, void *usrdata)
{
    s_stream_bak = GXBUS_PM_PROG_ALL;
    epg_ops_init(&s_ch_epg_ops);

    if(s_cur_epg_info != NULL)
    {
        GxCore_Free(s_cur_epg_info);
        s_cur_epg_info = NULL;
    }
    s_cur_epg_info = (GxEpgInfo *)GxCore_Malloc(EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
    if(s_cur_epg_info != NULL)
        memset(s_cur_epg_info, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);

    if(s_next_epg_info != NULL)
    {
        GxCore_Free(s_next_epg_info);
        s_next_epg_info = NULL;
    }
    s_next_epg_info = (GxEpgInfo *)GxCore_Malloc(EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
    if(s_next_epg_info != NULL)
        memset(s_next_epg_info, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);

    _channel_info_show();
    //create timeout timer
    timer_channel_info_timeout_reset();

    if (GUI_CheckDialog("wnd_passwd") == GXCORE_SUCCESS)
    {
        GUI_SetFocusWidget("wnd_passwd");
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_info_destroy(GuiWidget *widget, void *usrdata)
{
    if(sp_ChanInfoTimer != NULL)
    {
        remove_timer(sp_ChanInfoTimer);
        sp_ChanInfoTimer = NULL;
    }

    if(s_cur_epg_info != NULL)
    {
        GxCore_Free(s_cur_epg_info);
        s_cur_epg_info = NULL;
    }

    if(s_next_epg_info != NULL)
    {
        GxCore_Free(s_next_epg_info);
        s_next_epg_info = NULL;
    }

    s_stream_bak = GXBUS_PM_PROG_ALL;
   // GUI_SetProperty(TEXT_EPG_CUR, "string", " ");
   // GUI_SetProperty(TEXT_EPG_NEXT, "string", " ");

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_info_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    pvr_state cur_pvr = PVR_DUMMY;

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

                        g_AppFullArb.state.pause = STATE_OFF;
                        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                        g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_NEXT, 0);

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

                        g_AppFullArb.state.pause = STATE_OFF;
                        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                        g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_PREV, 0);
                        break;
                    }

                case STBK_EXIT:
                case VK_BOOK_TRIGGER:
                    {
                        if(g_AppFullArb.state.tv == STATE_ON)
                        {
                            GUI_EndDialog("wnd_channel_info");
                        }
                    }
                    break;

                case STBK_INFO:
                    {
#define IMG_STATE       "img_full_state"
#define TXT_STATE       "txt_full_state"

                        g_AppFullArb.timer_stop();
                        GUI_SetProperty(TXT_STATE, "string", " ");
                        GUI_SetProperty(IMG_STATE, "state", "osd_trans_hide");
                        GUI_SetProperty(TXT_STATE, "state", "osd_trans_hide");
                        app_create_dialog(WND_PROGBAR_INFO);
                        GUI_SetProperty(WND_PROGBAR_INFO, "draw_now", NULL);
                    }
                    break;
#if RECALL_LIST_SUPPORT
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

#define IMG_STATE       "img_full_state"
#define TXT_STATE       "txt_full_state"

                        g_AppFullArb.timer_stop();
                        GUI_SetProperty(TXT_STATE, "string", " ");
                        GUI_SetProperty(IMG_STATE, "state", "osd_trans_hide");
                        GUI_SetProperty(TXT_STATE, "state", "osd_trans_hide");
                        app_create_dialog("wnd_recall_list");
                        GUI_SetProperty("wnd_recall_list", "draw_now", NULL);
                    }
                    break;
#endif

                default:
                    {
                        if((g_AppFullArb.state.tv == STATE_ON)
                                || ((event->key.sym == STBK_OK)
                                    //|| (event->key.sym == STBK_MENU)
                                    //|| (event->key.sym == STBK_TV_RADIO)
                                    //|| (event->key.sym == STBK_RECALL)
                                    || (event->key.sym == STBK_SAT)
                                    || (event->key.sym == STBK_FAV)
                                    //|| (event->key.sym == STBK_F1)
                                    // || (event->key.sym == STBK_AUDIO)
                                    || (event->key.sym == STBK_EPG)
                                    || (event->key.sym == STBK_TTX)
                                    || (event->key.sym == STBK_SUBT)
#if DLNA_SUPPORT
                                    || (event->key.sym == VK_DLNA_TRIGGER)
#endif
                                   ))
                        {
                            GUI_EndDialog("wnd_channel_info");
                        }
                        GUI_SendEvent("wnd_full_screen", event);
                    }
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_channel_info_got_focus(GuiWidget *widget, void *usrdata)
{
    timer_channel_info_timeout_reset();
    g_AppFullArb.timer_start();

    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_channel_info_lost_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_KEEPON;
}

#define CHANNEL_INFO_SIGNAL
static int timer_show_signal(void *userdata)
{
	unsigned int value = 0;
	unsigned int ber_value;
	char Buffer[20] = {0};
	SignalBarWidget siganl_bar = {0};
	SignalValue siganl_val = {0};
	static uint32_t E_param = 0;

	// progbar strength
	app_ioctl( g_AppPlayOps.normal_play.tuner, FRONTEND_STRENGTH_GET, &value);
	//printf("[APP Signal Info] get strength: %d\n", value);
	siganl_bar.strength_bar = "progbar_signal_strength";
	siganl_val.strength_val = value;

	// strength value
	memset(Buffer,0,sizeof(Buffer));
	sprintf(Buffer,"%d%%",value);
	GUI_SetProperty("text_signal_strength_value", "string", Buffer);

	// progbar quality
	app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_QUALITY_GET, &value);
	//printf("[APP Signal Info] get quality: %d\n", value);
	siganl_bar.quality_bar= "progbar_signal_quality";
	siganl_val.quality_val = value;

    //printf("########[%s]%d strength_val = %d, quality_val = %d ########\n", __func__, __LINE__, siganl_val.strength_val, siganl_val.quality_val);
	ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &value);

	// quality value
	memset(Buffer,0,sizeof(Buffer));
	sprintf(Buffer,"%d%%",value);
	GUI_SetProperty("text_signal_quality_value", "string", Buffer);

    app_signal_progbar_update(g_AppPlayOps.normal_play.tuner, &siganl_bar, &siganl_val);
    // error rate
    app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_ERRORRATE_GET, &E_param);
    memset(Buffer,0,sizeof(Buffer));

    if(1 == GxFrontend_QueryStatus(g_AppPlayOps.normal_play.tuner))
	{
		value = (E_param>>8)&(0xfff);
		E_param = E_param&0xf;
		sprintf((char*)Buffer, "%d.%02dE-%02d", value/100, value%100, E_param);
		GUI_SetProperty("progbar_signal_snr", "fore_image_l", "s_progress3_l.bmp");
		GUI_SetProperty("progbar_signal_snr", "fore_image_m", "s_progress3_m.bmp");
		GUI_SetProperty("progbar_signal_snr", "fore_image_r", "s_progress3_r.bmp");

		ber_value = 12*E_param;
		if((ber_value > 100) || ((E_param == 0) && (value == 0)))
			ber_value = 100;
		else if(ber_value < 48)
		{
			GUI_SetProperty("progbar_signal_snr", "fore_image_l", "s_progress3_l_red.bmp");
			GUI_SetProperty("progbar_signal_snr", "fore_image_m", "s_progress3_m_red.bmp");
			GUI_SetProperty("progbar_signal_snr", "fore_image_r", "s_progress3_r_red.bmp");
		}

		GUI_SetProperty("progbar_signal_snr", "value", &ber_value);
	}
	else
	{
		value = 100;
		E_param = 0;
		sprintf((char*)Buffer, "%d.%02dE-%02d", value/100, value%100, E_param);
		GUI_SetProperty("progbar_signal_snr", "fore_image_l", "s_progress3_l_red.bmp");
		GUI_SetProperty("progbar_signal_snr", "fore_image_m", "s_progress3_m_red.bmp");
		GUI_SetProperty("progbar_signal_snr", "fore_image_r", "s_progress3_r_red.bmp");
		ber_value = 10;
		GUI_SetProperty("progbar_signal_snr", "value", &ber_value);
	}
	GUI_SetProperty("text_signal_snr_value", "string", Buffer);

	return 0;
}

SIGNAL_HANDLER int app_channel_info_signal_create(const char* widgetname, void *usrdata)
{
    AppFrontend_Info info;
    GxMsgProperty_NodeByPosGet node = {0};
    /*MPEG1, MPEG2, AAC-LATM, AC3, AAC-ADTS, EAC3, LPCM, DTS, DOLBY-TRUEHD, AC3-PLUS, DTS-HD, DTS_MA, AC3-PLUS-SEC, DTS-HD-SEC*/
    char *audio_type[] = {"MPEG-1", "MPEG-2", "AAC", "AC3", "AAC","EAC3","LPCM","DTS","DOLBY","AC3","DTS","DTS","AC3","DTS"};
    char *video_type[] = {"MPEG2", "AVS", "MPEG4","H.265"};
    char BufferPara[50] = {0};

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        if(node.prog_data.cur_audio_type >= sizeof(audio_type)/sizeof(audio_type[0]))
        {
            GUI_SetProperty("text_signal_audio_type", "string", STR_ID_UNKNOW);
        }
        else
        {
            GUI_SetProperty("text_signal_audio_type", "string", audio_type[node.prog_data.cur_audio_type]);
        }

        if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
        {
            if(node.prog_data.video_type >= sizeof(video_type)/sizeof(video_type[0]))
            {
                GUI_SetProperty("text_signal_video_type", "string", STR_ID_UNKNOW);
            }
            else
            {
                GUI_SetProperty("text_signal_video_type", "string", video_type[node.prog_data.video_type]);
            }
        }
        else
        {
            GUI_SetProperty("text_signal_video", "state", "hide");
            GUI_SetProperty("text_signal_video_type", "state", "hide");
            GUI_SetProperty("text_signal_video_pid_str", "state", "hide");
            GUI_SetProperty("text_signal_video_pid_value", "state", "hide");
        }
    }


	timer_show_signal(NULL);
    if (sp_TimerSignal == NULL)
    {
        sp_TimerSignal = create_timer(timer_show_signal, 100, NULL, TIMER_REPEAT);
    }

    app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_INFO_GET, &info);

    GxMsgProperty_NodeByIdGet node_tp = {0};
    node_tp.node_type = NODE_TP;
    node_tp.id = node.prog_data.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

    GxMsgProperty_NodeByIdGet pmdata_sat = {0};
    pmdata_sat.node_type = NODE_SAT;
    pmdata_sat.id = node_tp.tp_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &pmdata_sat);
#if DEMOD_DVB_C
    if(GXBUS_PM_SAT_C == pmdata_sat.sat_data.type)
    {
        char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};

        if(node_tp.tp_data.tp_c.modulation < QAM_AUTO)
            snprintf(BufferPara,50,"%s%s%s",modulation_str[node_tp.tp_data.tp_c.modulation],", ","DVB -C");

        GUI_SetProperty(TEXT_CHANNEL_INFO, "string", BufferPara);
        sprintf(BufferPara,"%d",node.prog_data.video_pid);
        GUI_SetProperty(TEXT_SIGNA_VIDEO_PID, "string", BufferPara);
        sprintf(BufferPara,"%d",node.prog_data.cur_audio_pid);
        GUI_SetProperty(TEXT_SIGNA_AUDIO_PID, "string", BufferPara);
        sprintf(BufferPara, "%d",node.prog_data.pcr_pid);
        GUI_SetProperty(TEXT_SIGNA_PCR_PID, "string", BufferPara);
    }
#endif
#if DEMOD_DVB_T
    if(GXBUS_PM_SAT_T == pmdata_sat.sat_data.type)
    {
        if(node.prog_data.fe_mode == DVBT2_BASE ||node.prog_data.fe_mode == DVBT2_LITE)
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "DVB -T2");
        else
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "DVB -T");

        sprintf(BufferPara,"%d",node.prog_data.video_pid);
        GUI_SetProperty(TEXT_SIGNA_VIDEO_PID, "string", BufferPara);
        sprintf(BufferPara,"%d",node.prog_data.cur_audio_pid);
        GUI_SetProperty(TEXT_SIGNA_AUDIO_PID, "string", BufferPara);
        sprintf(BufferPara, "%d",node.prog_data.pcr_pid);
        GUI_SetProperty(TEXT_SIGNA_PCR_PID, "string", BufferPara);
    }
#endif

#if DEMOD_DTMB
    if(GXBUS_PM_SAT_DTMB == pmdata_sat.sat_data.type)
    {
        if(GXBUS_PM_SAT_1501_DTMB == pmdata_sat.sat_data.sat_dtmb.work_mode)
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "DTMB");
        else
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "DVB -C");

        sprintf(BufferPara,"%d",node.prog_data.video_pid);
        GUI_SetProperty(TEXT_SIGNA_VIDEO_PID, "string", BufferPara);
        sprintf(BufferPara,"%d",node.prog_data.cur_audio_pid);
        GUI_SetProperty(TEXT_SIGNA_AUDIO_PID, "string", BufferPara);
        sprintf(BufferPara, "%d",node.prog_data.pcr_pid);
        GUI_SetProperty(TEXT_SIGNA_PCR_PID, "string", BufferPara);
    }
#endif


#if DEMOD_DVB_S
    if(GXBUS_PM_SAT_S == pmdata_sat.sat_data.type)
    {
        switch(info.modulation)
        {
            case FRONTEND_CR12:
            case FRONTEND_QPSK12:
                GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 1/2");
                break;
            case FRONTEND_CR23:
            case FRONTEND_QPSK23:
                GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 2/3");
                break;
            case FRONTEND_CR34:
            case FRONTEND_QPSK34:
                GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 3/4");
                break;
            case FRONTEND_CR45:
            case FRONTEND_QPSK45:
                GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 4/5");
                break;
            case FRONTEND_CR56:
            case FRONTEND_QPSK56:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 5/6");
            break;
        case FRONTEND_CR67:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 6/7");
            break;
        case FRONTEND_CR78:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 7/8");
            break;
        case FRONTEND_QPSK14:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 1/4");
            break;
        case FRONTEND_QPSK13:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 1/3");
            break;
        case FRONTEND_QPSK25:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 2/5");
            break;
        case FRONTEND_QPSK35:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 3/5");
            break;
        case FRONTEND_QPSK89:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 8/9");
            break;
        case FRONTEND_QPSK910:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "QPSK 9/10");
            break;
        case FRONTEND_8PSK35:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "8PSK 3/5");
            break;
        case FRONTEND_8PSK23:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "8PSK 2/3");
            break;
        case FRONTEND_8PSK34:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "8PSK 3/4");
            break;
        case FRONTEND_8PSK56:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "8PSK 5/6");
            break;
        case FRONTEND_8PSK89:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "8PSK 8/9");
            break;
        case FRONTEND_8PSK910:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", "8PSK 9/10");
            break;
        default:
            GUI_SetProperty(TEXT_CHANNEL_INFO, "string", STR_ID_UNKNOW);
            break;
    }

    char *p = NULL;
    GUI_GetProperty(TEXT_CHANNEL_INFO, "string", &p);
    if(NULL == p)
        return EVENT_TRANSFER_STOP;


    switch(info.type)
    {
        case FRONTEND_DVB_S:
            snprintf(BufferPara,50,"%s%s%s",p,", ",STR_ID_DVBS);
            break;
        case FRONTEND_DVB_S2:
            snprintf(BufferPara,50,"%s%s%s",p,", ",STR_ID_DVBS2);
            break;
        default:
            snprintf(BufferPara,50,"%s%s%s",p,", ",STR_ID_UNKNOW);
            break;
    }

    GUI_SetProperty(TEXT_CHANNEL_INFO, "string", BufferPara);
    sprintf(BufferPara,"%d",node.prog_data.video_pid);
    GUI_SetProperty(TEXT_SIGNA_VIDEO_PID, "string", BufferPara);
    sprintf(BufferPara,"%d",node.prog_data.cur_audio_pid);
    GUI_SetProperty(TEXT_SIGNA_AUDIO_PID, "string", BufferPara);
    sprintf(BufferPara, "%d",node.prog_data.pcr_pid);
    GUI_SetProperty(TEXT_SIGNA_PCR_PID, "string", BufferPara);
    }
#endif

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_info_signal_destroy(const char* widgetname, void *usrdata)
{
    uint32_t panel_led = 0;

    remove_timer(sp_TimerSignal);
    sp_TimerSignal = NULL;

    if (g_AppPlayOps.normal_play.play_total == 0)
    {
        panel_led = 0;
    }
    else
    {
        panel_led = g_AppPlayOps.normal_play.play_count+1;
    }
    ioctl(bsp_panel_handle, PANEL_SHOW_NUM, &panel_led);

    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_channel_info_signal_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case STBK_INFO:
            case STBK_EXIT:
            case STBK_MENU:
            case VK_BOOK_TRIGGER:
                {
                    GUI_EndDialog(WND_PROGBAR_INFO);
                    if(g_AppFullArb.state.tv == STATE_ON
                            || (event->key.sym == VK_BOOK_TRIGGER))
                        GUI_EndDialog("wnd_channel_info");
                    break;
                }
#if (DLNA_SUPPORT > 0)
            case VK_DLNA_TRIGGER:
                GUI_EndDialog(WND_PROGBAR_INFO);
                GUI_EndDialog("wnd_channel_info");
                GUI_SendEvent("wnd_full_screen", event);
                break;
#endif

            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_info_signal_got_focus(const char* widgetname, void *usrdata)
{
    reset_timer(sp_TimerSignal);
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_channel_info_signal_lost_focus(const char* widgetname, void *usrdata)
{
    timer_stop(sp_TimerSignal);
    return EVENT_TRANSFER_KEEPON;
}

