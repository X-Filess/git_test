#include "app.h"
#if (DEMOD_DVB_S > 0)

#include "app_default_params.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "app_frontend.h"
#include "app_pop.h"
#include "app_frontend.h"
#include "app_epg.h"

extern void app_sat_list_change_sel(uint32_t sat_sel);
extern SatSelStatus satsel;

#define IMG_BAR_HALF_FOCUS_L "s_bar_half_focus_l.bmp"
#define IMG_BAR_HALF_FOCUS_R "s_bar_half_focus_r.bmp"
#define IMG_BAR_HALF_UNFOCUS "s_bar_half_unfocus.bmp"

#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
static char *s_item_chioce_ok[] =
{
	"img_positioner_ok1",
	"img_positioner_ok2",
	"img_positioner_ok3",
	"img_positioner_ok4",
	"img_positioner_ok5",
	"img_positioner_ok6",
	"img_positioner_ok7",
	"img_positioner_ok8"
};
#define IMG_BAR_HALF_FOCUS_OK "s_bar_half_focus_l.bmp"
#define IMG_BAR_HALF_FOCUS_AR "s_bar_half_focus_r.bmp"
#define IMG_BAR_HALF_UNFOCUS_AR "s_bar_half_unfocus.bmp"

#else
#define IMG_BAR_HALF_FOCUS_OK "s_bar_half_focus_ok.bmp"
#define IMG_BAR_HALF_FOCUS_AR "s_bar_half_focus_arrow.bmp"
#define IMG_BAR_HALF_UNFOCUS_AR "s_bar_half_unfocus_arrow.bmp"
#endif

#define MAX_POSITIONER_OFF_ITEM 4
#define MAX_POSITIONER_ON_ITEM 8

#define DISEQC12_LIMIT_CONTENT "[E,W]"
#define MAX_DISEQC12_POS 64

static enum
{
	POSITIONER_OFF,
	POSITIONER_DISEQC12,
	POSITIONER_USALS
}s_positioner_mode = POSITIONER_OFF;

static char *s_positioner_item_widget[][MAX_POSITIONER_ON_ITEM] =
{
	{
		"cmb_positioner_sat_opt",
		"cmb_positioner_tp_opt",
		"cmb_positioner_motor_opt",
		"btn_positioner_off_save",
	},

	{
		"cmb_positioner_sat_opt",
		"cmb_positioner_tp_opt",
		"cmb_positioner_motor_opt",
		"cmb_positioner_fine_drive",
		"cmb_positioner_limit",
		"btn_positioner_recalc",
		"cmb_positioner_diseqc12_goto",
		"btn_positioner_save"
	},

	{
		"cmb_positioner_sat_opt",
		"cmb_positioner_tp_opt",
		"cmb_positioner_motor_opt",
		"box_positioner_longitude",
		"box_positioner_latitude",
		"btn_positioner_longitude",
		"btn_positioner_usals_goto",
		"btn_positioner_save"
	}
};

static char *s_positioner_title_content[][MAX_POSITIONER_ON_ITEM] =
{
	{
		STR_ID_SATELLITE,
		STR_ID_TP,
		STR_ID_MOTOR_TYPE,
		STR_ID_SAVE,
	},

	{
		STR_ID_SATELLITE,
		STR_ID_TP,
		STR_ID_MOTOR_TYPE,
		STR_FINE_DRIVE,
		STR_LIMIT,
		STR_RECALC,
		STR_GOTO,
		STR_SAVE_POSITION
	},

	{
		STR_ID_SATELLITE,
		STR_ID_TP,
		STR_ID_MOTOR_TYPE,
		STR_LOCAL_LONGITUDE,
		STR_LOCAL_LATITUDE,
		STR_SAT_LONGITUDE,
		STR_GOTO,
		STR_ID_SAVE
	}
};

static char *s_positioner_item_title[] =
{
	"text_positioner_item_title1",
	"text_positioner_item_title2",
	"text_positioner_item_title3",
	"text_positioner_item_title4",
	"text_positioner_item_title5",
	"text_positioner_item_title6",
	"text_positioner_item_title7",
	"text_positioner_item_title8",
};

static char *s_positioner_choice_back[] =
{
	"img_positioner_item_choice1",
	"img_positioner_item_choice2",
	"img_positioner_item_choice3",
	"img_positioner_item_choice4",
	"img_positioner_item_choice5",
	"img_positioner_item_choice6",
	"img_positioner_item_choice7",
	"img_positioner_item_choice8",
};

static int s_cur_max_item = MAX_POSITIONER_OFF_ITEM;
static int s_positioner_sel = 0;
static GxMsgProperty_NodeByPosGet s_PosiSat = {0};
static GxMsgProperty_NodeByPosGet s_PosiTp = {0};
static uint8_t s_diseqc12_pos_map[MAX_DISEQC12_POS] = {0};
uint16_t s_posi_sat_total = 0;
uint16_t s_posi_tp_total = 0;
static event_list* sp_PosiTimerSignal = NULL;
static event_list* sp_PosiLockTimer = NULL;
static event_list* sp_PosiArrow = NULL;
static LongitudeDirect s_local_longitude_direct;
static LatitudeDirect s_local_latitude_direct;
static char* s_moto_type_content[] =
{
	"Off",
	"DiSEqC1.2",
	"USALS"
};
static enum
{
	DISH_MOVE_NONE,
	DISH_MOVE_EAST,
	DISH_MOVE_WEST
}s_dish_moving_direct = DISH_MOVE_NONE;

static void _positioner_diseqc(void);
static void _positioner_set_tp(SetTpMode set_mode);

#define POSITIONER_TIMER
static int timer_positioner_show_signal(void *userdata)
{
    uint32_t value = 0;
    char Buffer[20] = {0};
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};

    if(s_posi_tp_total > 0)
    {
        // progbar strength
        app_ioctl(s_PosiSat.sat_data.tuner, FRONTEND_STRENGTH_GET, &value);
        siganl_bar.strength_bar = "progbar_positioner_strength";
        siganl_val.strength_val = value;

        // strength value
        memset(Buffer,0,sizeof(Buffer));
        sprintf(Buffer,"%d%%",value);
        GUI_SetProperty("text_positioner_strength_value", "string", Buffer);

        // progbar quality
        app_ioctl(s_PosiSat.sat_data.tuner, FRONTEND_QUALITY_GET, &value);
        siganl_bar.quality_bar= "progbar_positioner_Quality";
        siganl_val.quality_val = value;

        // quality value
        memset(Buffer,0,sizeof(Buffer));
        sprintf(Buffer,"%d%%",value);
        GUI_SetProperty("text_positioner_quality_value", "string", Buffer);

        app_signal_progbar_update(0, &siganl_bar, &siganl_val);
        ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &value);
    }
    else
    {
        siganl_bar.strength_bar = "progbar_positioner_strength";
        siganl_val.strength_val = 0;
        GUI_SetProperty("text_positioner_strength_value", "string", "0%");

        siganl_bar.quality_bar= "progbar_positioner_Quality";
        siganl_val.quality_val = 0;
        GUI_SetProperty("text_positioner_quality_value", "string", "0%");

        app_signal_progbar_update(0, &siganl_bar, &siganl_val);
        ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &siganl_val.quality_val);
    }
    return 0;
}

static void timer_positioner_show_signal_stop(void)
{
	timer_stop(sp_PosiTimerSignal);
}

static void timer_positioner_show_signal_reset(void)
{
	reset_timer(sp_PosiTimerSignal);
}

static int timer_positioner_set_frontend(void *userdata)
{
	GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
	remove_timer(sp_PosiLockTimer);
	sp_PosiLockTimer = NULL;

	_positioner_diseqc();
	_positioner_set_tp(SET_TP_FORCE);

	return 0;
}

static int timer_positioner_stop_fine_drive(void *userdata)
{
    remove_timer(sp_PosiArrow);
    sp_PosiArrow = NULL;
    {
        extern void GxFrontend_ForceExitEx(int32_t tuner);
#if DOUBLE_S2_SUPPORT
        GxFrontend_ForceExitEx(0);
        GxFrontend_ForceExitEx(1);
#else
        GxFrontend_ForceExitEx(s_PosiSat.sat_data.tuner);
#endif
    }

    app_diseqc12_stop_drictory(s_PosiSat.sat_data.tuner,3);	//X2
    {
        extern void GxFrontend_StartMonitor(int32_t tuner);
        GxFrontend_StartMonitor(s_PosiSat.sat_data.tuner);
    }
    timer_positioner_show_signal_reset();
    switch(s_dish_moving_direct)
    {
        case DISH_MOVE_WEST:
            GUI_SetProperty("cmb_positioner_fine_drive", "focus_left_img", "s_arrow_l.bmp");
            break;

        case DISH_MOVE_EAST:
            GUI_SetProperty("cmb_positioner_fine_drive", "focus_right_img", "s_arrow_r.bmp");
            break;

        default:
            break;
    }

    s_dish_moving_direct = DISH_MOVE_NONE;

    return 0;
}

#define SUB_FUNC

static void _positioner_focus_item(int sel)
{
#define MS_FOCUS_FORE_COLOR "[text_focus_color,text_color,text_disable_color]"
    char *img = IMG_BAR_HALF_FOCUS_L;

    if((sel == 0) || (sel == 1) || (sel == 2))
    {
#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
        GUI_SetProperty(s_item_chioce_ok[sel], "state", "show");
#else
        img = IMG_BAR_HALF_FOCUS_OK;
#endif
    }
    if(s_positioner_mode == POSITIONER_DISEQC12)
    {
        if((sel == 4) || (sel == 6))
        {
#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
            GUI_SetProperty(s_item_chioce_ok[sel], "state", "show");
#else
            img = IMG_BAR_HALF_FOCUS_OK;
#endif
        }
    }
    if(s_positioner_mode == POSITIONER_USALS)
    {
        uint32_t t_sel = 0;
        if(sel == 3)
            GUI_SetProperty("edit_positioner_longitude", "select", &t_sel);
        else if(sel == 4)
            GUI_SetProperty("edit_positioner_latitude", "select", &t_sel);
    }
    GUI_SetProperty(s_positioner_choice_back[sel], "img", img);
    GUI_SetProperty(s_positioner_item_title[sel], "string", s_positioner_title_content[s_positioner_mode][sel]);
    GUI_SetProperty(s_positioner_item_title[sel], "forecolor", MS_FOCUS_FORE_COLOR);//the color of text is unfocus color

    GUI_SetProperty(s_positioner_item_widget[s_positioner_mode][sel], "state", "focus");
}

static void _positioner_unfocus_item(int sel)
{
#define MS_UNFOCUS_FORE_COLOR "[text_color,text_color,text_disable_color]"
    GUI_SetProperty(s_positioner_choice_back[sel], "img", IMG_BAR_HALF_UNFOCUS);
    GUI_SetProperty(s_positioner_item_title[sel], "string", s_positioner_title_content[s_positioner_mode][sel]);
    GUI_SetProperty(s_positioner_item_title[sel], "forecolor", MS_UNFOCUS_FORE_COLOR);//the color of text is unfocus color
#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
    GUI_SetProperty(s_item_chioce_ok[sel], "state", "hide");
#endif
}

static void _positioner_off_view(void)
{
	int i;

	GUI_SetProperty(s_positioner_item_widget[POSITIONER_DISEQC12][3], "state", "hide");
	GUI_SetProperty(s_positioner_item_widget[POSITIONER_USALS][3], "state", "hide");

	for(i = 4; i < MAX_POSITIONER_ON_ITEM; i++)
	{
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_DISEQC12][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_USALS][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_title[i], "state", "hide");
		GUI_SetProperty(s_positioner_choice_back[i], "state", "hide");
	}

	GUI_SetProperty(s_positioner_item_title[3], "string", s_positioner_title_content[POSITIONER_OFF][3]);
	GUI_SetProperty(s_positioner_item_widget[POSITIONER_OFF][3], "state", "show");
	GUI_SetProperty(s_positioner_item_title[3], "state", "show");
	GUI_SetProperty(s_positioner_choice_back[3], "state", "show");
}

static void _positioner_diseqc12_view(void)
{
	int i;

	for(i = 3;  i < MAX_POSITIONER_ON_ITEM; i++)
	{
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_OFF][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_USALS][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_title[i], "string", s_positioner_title_content[POSITIONER_DISEQC12][i]);
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_DISEQC12][i], "state", "show");
		GUI_SetProperty(s_positioner_item_title[i], "state", "show");
		GUI_SetProperty(s_positioner_choice_back[i], "state", "show");
	}
}

static void _positioner_usals_view(void)
{
	int init_value = 0;
	char buff[10] = {0};
	int i;

	for(i = 3;  i < MAX_POSITIONER_ON_ITEM; i++)
	{
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_OFF][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_DISEQC12][i], "state", "hide");
		GUI_SetProperty(s_positioner_item_title[i], "string", s_positioner_title_content[POSITIONER_USALS][i]);
	}

	GxBus_ConfigGetInt(LONGITUDE_KEY, &init_value, LONGITUDE);
	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%03d.%d", init_value/10, init_value%10);
	GUI_SetProperty("edit_positioner_longitude", "string", buff);

	GxBus_ConfigGetInt(LONGITUDE_DIRECT_KEY, &init_value, LONGITUDE_DIRECT);
	s_local_longitude_direct = init_value;
	memset(buff, 0, sizeof(buff));
	if(s_local_longitude_direct == LONGITUDE_EAST) //E
		buff[0] = 'E';
	else
		buff[0] = 'W';
	GUI_SetProperty("text_positioner_longitude_direct", "string", buff);

	GxBus_ConfigGetInt(LATITUDE_KEY, &init_value, LATITUDE);
	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%02d.%d", init_value/10, init_value%10);
	GUI_SetProperty("edit_positioner_latitude", "string", buff);

	GxBus_ConfigGetInt(LATITUDE_DIRECT_KEY, &init_value, LATITUDE_DIRECT);
	s_local_latitude_direct= init_value;
	memset(buff, 0, sizeof(buff));
	if(s_local_latitude_direct == LATITUDE_SOUTH)
		buff[0] = 'S';
	else
		buff[0] = 'N';
	GUI_SetProperty("text_positioner_latitude_direct", "string", buff);

	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%03d.%d", s_PosiSat.sat_data.sat_s.longitude/10, s_PosiSat.sat_data.sat_s.longitude%10);
	buff[5]= ' ';
	if(s_PosiSat.sat_data.sat_s.longitude_direct == GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST)
	{
		buff[6] = 'E';
	}
	else if(s_PosiSat.sat_data.sat_s.longitude_direct == GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST)
	{
		buff[6] = 'W';
	}
	else
		;
	GUI_SetProperty("btn_positioner_longitude", "string", buff);

	for(i = 0;  i < MAX_POSITIONER_ON_ITEM; i++)
	{
		GUI_SetProperty(s_positioner_item_widget[POSITIONER_USALS][i], "state", "show");
		GUI_SetProperty(s_positioner_item_title[i], "state", "show");
		GUI_SetProperty(s_positioner_choice_back[i], "state", "show");
	}
}

static void _positioner_cmb_sat_rebuild(void)
{
#define BUFFER_LENGTH	(MAX_SAT_NAME + 20)
	static GxMsgProperty_NodeByPosGet node;
	int i = 0;
	char *buffer_all = NULL;
	char buffer[BUFFER_LENGTH] = {0};

	//rebuild sat list in combo box
	if(0 == s_posi_sat_total)
	{
		printf("_positioner_cmb_sat_rebuild, total error..%s,%d\n",__FILE__,__LINE__);
		return;
	}
	buffer_all = GxCore_Malloc(s_posi_sat_total * BUFFER_LENGTH);
	if(buffer_all == NULL)
	{
		printf("_positioner_cmb_sat_rebuild, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}
	memset(buffer_all, 0, s_posi_sat_total * BUFFER_LENGTH);

	//first one
	i = 0;
	s_PosiSat.node_type = NODE_SAT;
	s_PosiSat.pos = i;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_PosiSat);
	memset(buffer, 0, BUFFER_LENGTH);
	sprintf(buffer,"[(%d/%d) %s", i+1, s_posi_sat_total, (char*)s_PosiSat.sat_data.sat_s.sat_name);
	strcat(buffer_all, buffer);

	if(s_posi_sat_total > 1)
	{
		for(i=1; i<s_posi_sat_total; i++)
		{
			node.node_type = NODE_SAT;
			node.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
			memset(buffer, 0, BUFFER_LENGTH);
			sprintf(buffer,",(%d/%d) %s", i+1, s_posi_sat_total, (char*)node.sat_data.sat_s.sat_name);
			strcat(buffer_all, buffer);
		}
	}
	//only 1 sat or finished
	strcat(buffer_all, "]");
	GUI_SetProperty("cmb_positioner_sat_opt","content",buffer_all);

	GxCore_Free(buffer_all);

	return;
}

static void _positioner_cmb_tp_rebuild(void)
{
#define BUFFER_LENGTH_TP	(64)

	static GxMsgProperty_NodeByPosGet node;
	GxMsgProperty_NodeNumGet node_num = {0};
	int i = 0;
	char *buffer_all = NULL;
	char buffer[BUFFER_LENGTH_TP] = {0};

	// get tp number
	node_num.node_type = NODE_TP;
	node_num.sat_id = s_PosiSat.sat_data.id;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	s_posi_tp_total = node_num.node_num;
	if(0 == s_posi_tp_total)
	{
		memset(&s_PosiTp, 0, sizeof(s_PosiTp));
		GUI_SetProperty("cmb_positioner_tp_opt","content","[NONE]");
	}
	else
	{
		buffer_all = GxCore_Malloc(s_posi_tp_total * (BUFFER_LENGTH_TP));
		if(buffer_all == NULL)
		{
			printf("_positioner_cmb_tp_rebuild, malloc failed...%s,%d\n",__FILE__,__LINE__);
			return ;
		}
		memset(buffer_all, 0, s_posi_tp_total * (BUFFER_LENGTH_TP));

		//first one
		i = 0;
		s_PosiTp.node_type = NODE_TP;
		s_PosiTp.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_PosiTp);
		memset(buffer, 0, BUFFER_LENGTH_TP);
#if (DEMOD_DVB_T > 0)	
				if(GXBUS_PM_SAT_DVBT2 == s_PosiSat.sat_data.type)
				{
					char* bandwidth_str[3] = {"8M", "7M","6M"};
					
					if(s_PosiTp.tp_data.tp_dvbt2.bandwidth <BANDWIDTH_AUTO )
					sprintf(buffer,"[(%d/%d) %d.%d/%s", i+1, s_posi_tp_total, s_PosiTp.tp_data.frequency/1000, \
						(s_PosiTp.tp_data.frequency/100)%10,bandwidth_str[s_PosiTp.tp_data.tp_dvbt2.bandwidth]);
					strcat(buffer_all, buffer);
					
					if(s_posi_tp_total > 1)
					{
						GxMsgProperty_NodeByPosGet node_tp = {0};
						for(i = 1; i < s_posi_tp_total; i++)
						{
							node_tp.node_type = NODE_TP;
							node_tp.pos = i;
							app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
							memset(buffer, 0, BUFFER_LENGTH_TP);
							sprintf(buffer,",(%d/%d) %d.%d/%s", i+1, s_posi_tp_total, node_tp.tp_data.frequency/1000, \
								(node_tp.tp_data.frequency/100)%10,bandwidth_str[node_tp.tp_data.tp_dvbt2.bandwidth]);
							strcat(buffer_all, buffer);
						}
					}
		
					
				}else 
#endif
#if (DEMOD_DVB_C > 0)
				if(GXBUS_PM_SAT_C == s_PosiSat.sat_data.type)
				{
					char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};
					
					if(s_PosiTp.tp_data.tp_c.modulation < QAM_AUTO)	
					sprintf(buffer,"[(%d/%d) %d /%s /%d", i+1, s_posi_tp_total, s_PosiTp.tp_data.frequency/1000  \
						, modulation_str[s_PosiTp.tp_data.tp_c.modulation],s_PosiTp.tp_data.tp_c.symbol_rate/1000);
					
					strcat(buffer_all, buffer);
					if(s_posi_tp_total > 1)
					{
						GxMsgProperty_NodeByPosGet node_tp = {0};
						for(i = 1; i < s_posi_tp_total; i++)
						{
							node_tp.node_type = NODE_TP;
							node_tp.pos = i;
							app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
							memset(buffer, 0, BUFFER_LENGTH_TP);
							sprintf(buffer,",(%d/%d) %d /%s /%d", i+1, s_posi_tp_total, node_tp.tp_data.frequency/1000  \
									,modulation_str[node_tp.tp_data.tp_c.modulation],node_tp.tp_data.tp_c.symbol_rate/1000);
							strcat(buffer_all, buffer);
						}
					}			
				}else
#endif
#if (DEMOD_DTMB > 0)
		if(GXBUS_PM_SAT_DTMB == s_PosiSat.sat_data.type)
		{
			if(GXBUS_PM_SAT_1501_DTMB == s_PosiSat.sat_data.sat_dtmb.work_mode)
			{
				char* bandwidth_str[3] = {"8M", "7M","6M"};// have 7M?
						
				if(s_PosiTp.tp_data.tp_dtmb.symbol_rate < BANDWIDTH_AUTO )
				sprintf(buffer,"[(%d/%d) %d.%d/%s", i+1, s_posi_tp_total, s_PosiTp.tp_data.frequency/1000, \
					(s_PosiTp.tp_data.frequency/100)%10,bandwidth_str[s_PosiTp.tp_data.tp_dtmb.symbol_rate]);
				strcat(buffer_all, buffer);
				
				if(s_posi_tp_total > 1)
				{
					GxMsgProperty_NodeByPosGet node_tp = {0};
					for(i = 1; i < s_posi_tp_total; i++)
					{
						node_tp.node_type = NODE_TP;
						node_tp.pos = i;
						app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
						memset(buffer, 0, BUFFER_LENGTH_TP);
						sprintf(buffer,",(%d/%d) %d.%d/%s", i+1, s_posi_tp_total, node_tp.tp_data.frequency/1000, \
							(node_tp.tp_data.frequency/100)%10,bandwidth_str[node_tp.tp_data.tp_dtmb.symbol_rate]);
						strcat(buffer_all, buffer);
					}
				}
			}
			else
			{
					char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};
					
					if(s_PosiTp.tp_data.tp_dtmb.modulation < QAM_AUTO)	
					sprintf(buffer,"[(%d/%d) %d /%s /%d", i+1, s_posi_tp_total, s_PosiTp.tp_data.frequency/1000  \
						, modulation_str[s_PosiTp.tp_data.tp_dtmb.modulation],s_PosiTp.tp_data.tp_dtmb.symbol_rate/1000);
					
					strcat(buffer_all, buffer);
					if(s_posi_tp_total > 1)
					{
						GxMsgProperty_NodeByPosGet node_tp = {0};
						for(i = 1; i < s_posi_tp_total; i++)
						{
							node_tp.node_type = NODE_TP;
							node_tp.pos = i;
							app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
							memset(buffer, 0, BUFFER_LENGTH_TP);
							sprintf(buffer,",(%d/%d) %d /%s /%d", i+1, s_posi_tp_total, node_tp.tp_data.frequency/1000  \
									,modulation_str[node_tp.tp_data.tp_dtmb.modulation],node_tp.tp_data.tp_dtmb.symbol_rate/1000);
							strcat(buffer_all, buffer);
						}
					}			
			}
				
		}else
#endif
	{
		if(s_PosiTp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
		{
			sprintf(buffer,"[(%d/%d) %d / %s / %d", i+1,s_posi_tp_total,
				s_PosiTp.tp_data.frequency, "V" ,s_PosiTp.tp_data.tp_s.symbol_rate);
		}
		else if(s_PosiTp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
		{
			sprintf(buffer,"[(%d/%d) %d / %s / %d", i+1, s_posi_tp_total,
				s_PosiTp.tp_data.frequency, "H" ,s_PosiTp.tp_data.tp_s.symbol_rate);
		}
		strcat(buffer_all, buffer);

		if(s_posi_tp_total > 1)
		{
			for(i=1; i<s_posi_tp_total; i++)
			{
				node.node_type = NODE_TP;
				node.pos = i;
				app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
				memset(buffer, 0, BUFFER_LENGTH_TP);
				if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
				{
					sprintf(buffer,",(%d/%d) %d / %s / %d", i+1, s_posi_tp_total,
						node.tp_data.frequency, "V" ,node.tp_data.tp_s.symbol_rate);
				}
				else if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
				{
					sprintf(buffer,",(%d/%d) %d / %s / %d", i+1, s_posi_tp_total,
						node.tp_data.frequency, "H" ,node.tp_data.tp_s.symbol_rate);
				}
				strcat(buffer_all, buffer);
			}
		}
	}
		//only 1 sat or finished
		strcat(buffer_all, "]");
		GUI_SetProperty("cmb_positioner_tp_opt","content",buffer_all);

		GxCore_Free(buffer_all);
	}
	return;
}

static void _positioner_diseqc(void)
{
	AppFrontend_DiseqcParameters diseqc10 = {0};
	AppFrontend_DiseqcParameters diseqc11 = {0};
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)	
#if (DEMOD_DVB_COMBO > 0)	
	app_set_tune_mode(s_PosiSat.sat_data.type); 	
#endif	

	{
		extern void GxFrontend_ForceExitEx(int32_t tuner);
#if DOUBLE_S2_SUPPORT
		GxFrontend_ForceExitEx(0);
		GxFrontend_ForceExitEx(1);
#else
		GxFrontend_ForceExitEx(s_PosiSat.sat_data.tuner);
#endif
	}

	diseqc10.type = DiSEQC10;
	diseqc10.u.params_10.chDiseqc = s_PosiSat.sat_data.sat_s.diseqc10;
	diseqc10.u.params_10.bPolar = app_show_to_lnb_polar(&s_PosiSat.sat_data);
	if(s_posi_tp_total == 0)
	{
		diseqc10.u.params_10.bPolar = SEC_VOLTAGE_OFF;
	}
	else
	{
		if(diseqc10.u.params_10.bPolar == SEC_VOLTAGE_ALL)
		{
			diseqc10.u.params_10.bPolar = app_show_to_tp_polar(&s_PosiTp.tp_data);
		}
	}
	diseqc10.u.params_10.b22k = app_show_to_sat_22k(s_PosiSat.sat_data.sat_s.switch_22K, &s_PosiTp.tp_data);

	AppFrontend_PolarState Polar =diseqc10.u.params_10.bPolar ;
	app_ioctl(s_PosiSat.sat_data.tuner, FRONTEND_POLAR_SET, (void*)&Polar);

	AppFrontend_22KState _22K = diseqc10.u.params_10.b22k;
	app_ioctl(s_PosiSat.sat_data.tuner, FRONTEND_22K_SET, (void*)&_22K);

	// 1.2 or 1.3
	if((s_PosiSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
	{
        app_diseqc12_goto_position(s_PosiSat.sat_data.tuner, s_PosiSat.sat_data.sat_s.diseqc12_pos, false);
	}
	else if((s_PosiSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
	{
		LongitudeInfo sat_longitude = {0};
		PositionVal local_position;
		char *str = NULL;

		local_position.longitude.longitude_direct = s_local_longitude_direct;
		GUI_GetProperty("edit_positioner_longitude", "string", &str);
		local_position.longitude.longitude = 10 * app_float_edit_str_to_value(str);

		local_position.latitude.latitude_direct = s_local_latitude_direct;
		GUI_GetProperty("edit_positioner_latitude", "string", &str);
		local_position.latitude.latitude = 10 * app_float_edit_str_to_value(str);

		sat_longitude.longitude = s_PosiSat.sat_data.sat_s.longitude;
		sat_longitude.longitude_direct = s_PosiSat.sat_data.sat_s.longitude_direct;
		app_usals_goto_position(s_PosiSat.sat_data.tuner,&sat_longitude,&local_position, false);
	}

	app_set_diseqc_10(s_PosiSat.sat_data.tuner, &diseqc10, true);

	diseqc11.type = DiSEQC11;
	diseqc11.u.params_11.chUncom = s_PosiSat.sat_data.sat_s.diseqc11;
	diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc ;
	diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
	diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;
	app_set_diseqc_11(s_PosiSat.sat_data.tuner, &diseqc11, true);
	// POST SEM
    GxFrontendOccur(s_PosiSat.sat_data.tuner);
}

static void _positioner_set_tp(SetTpMode set_mode)
{
    AppFrontend_SetTp params = {0};

	if(s_posi_tp_total == 0)
	{
		params.fre = 0;
		params.symb = 0;
		params.polar = 0;
	}
	else
	{
		params.fre = app_show_to_tp_freq(&s_PosiSat.sat_data, &s_PosiTp.tp_data);
		params.symb = s_PosiTp.tp_data.tp_s.symbol_rate;

		params.polar = app_show_to_lnb_polar(&s_PosiSat.sat_data);
		if(params.polar == SEC_VOLTAGE_ALL)
		{
			params.polar = app_show_to_tp_polar(&s_PosiTp.tp_data);
		}
	}
	params.sat22k = app_show_to_sat_22k(s_PosiSat.sat_data.sat_s.switch_22K, &s_PosiTp.tp_data);
	params.tuner = s_PosiSat.sat_data.tuner;
	#if UNICABLE_SUPPORT
	{
		uint32_t centre_frequency=0 ;
		uint32_t lnb_index=0 ;
		uint32_t set_tp_req=0;
		lnb_index = app_calculate_set_freq_and_initfreq(&s_PosiSat.sat_data, &s_PosiTp.tp_data,&set_tp_req,&centre_frequency);
		if(s_PosiSat.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
		{
			params.fre  = set_tp_req;
			params.lnb_index  = lnb_index;
		}
		else
		{
			params.lnb_index  = 0x20;
		}
		params.centre_fre  = centre_frequency;
		params.if_channel_index  = s_PosiSat.sat_data.sat_s.unicable_para.if_channel;
		params.if_fre  = s_PosiSat.sat_data.sat_s.unicable_para.centre_fre[params.if_channel_index];
	}
	#endif

    switch(s_PosiSat.sat_data.type)
    {
        case GXBUS_PM_SAT_S:
        {
            params.type = FRONTEND_DVB_S;
            break;
        }
        case GXBUS_PM_SAT_C:
        {
#if DEMOD_DVB_C		
			params.fre = s_PosiTp.tp_data.frequency;
			params.symb = s_PosiTp.tp_data.tp_c.symbol_rate;
			params.qam = s_PosiTp.tp_data.tp_c.modulation;
#endif
            params.type = FRONTEND_DVB_C;
            break;
        }
        case GXBUS_PM_SAT_T:
        {
            params.type = FRONTEND_DVB_T;
            break;
        }
        case GXBUS_PM_SAT_DVBT2:
        {
#if DEMOD_DVB_T		
			params.fre = s_PosiTp.tp_data.frequency;
			params.bandwidth = s_PosiTp.tp_data.tp_dvbt2.bandwidth;			
			params.type_1501 = DVBT_AUTO_MODE;//PLP Data not in tp note,should be auto
#endif
            params.type = FRONTEND_DVB_T2;
            break;
        }
#if DEMOD_DTMB
		case GXBUS_PM_SAT_DTMB:
			if(GXBUS_PM_SAT_1501_DTMB == s_PosiSat.sat_data.sat_dtmb.work_mode)
			{
				params.fre = s_PosiTp.tp_data.frequency;
				params.qam = s_PosiTp.tp_data.tp_dtmb.symbol_rate;// bandwidth	
				params.type_1501 = DTMB;
			}
			else
			{
				params.fre = s_PosiTp.tp_data.frequency;
				params.symb = s_PosiTp.tp_data.tp_dtmb.symbol_rate;
				params.qam = s_PosiTp.tp_data.tp_dtmb.modulation;
				params.type_1501 = DTMB_C;
			}
			 params.type = FRONTEND_DTMB;
			break;
#endif
        default:
            params.type = FRONTEND_DVB_S;
    }

	app_set_tp(s_PosiSat.sat_data.tuner, &params, set_mode);

	return;
}

static void _diseqc12_set_pos_record(uint8_t pos)
{
	if((pos > 0) && (pos <= MAX_DISEQC12_POS))
	{
		s_diseqc12_pos_map[pos -1] = 1;
	}
}

static void _diseqc12_clear_pos_record(uint8_t pos)
{
	if((pos > 0) && (pos <= MAX_DISEQC12_POS))
	{
		s_diseqc12_pos_map[pos -1] = 0;
	}
}

static void _diseqc12_init_pos_record(void)
{
	int i;
	GxMsgProperty_NodeByPosGet node;

	memset(s_diseqc12_pos_map, 0, sizeof(s_diseqc12_pos_map));

	for(i = 0; i < s_posi_sat_total; i++)
	{
		node.node_type = NODE_SAT;
		node.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

		if((node.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
		{
			_diseqc12_set_pos_record(node.sat_data.sat_s.diseqc12_pos);
		}
	}
}

static uint8_t _diseqc12_get_valid_pos(void)
{
	int i;

	for(i = 0; i < MAX_DISEQC12_POS; i++)
	{
		if(s_diseqc12_pos_map[i] == 0)
		{
			return i + 1;
		}
	}

	return 0;
}

static bool _diseqc12_check_pos_record(uint8_t pos)
{
	bool ret = false;

	if((pos > 0) && (pos <= MAX_DISEQC12_POS))
	{
		if(s_diseqc12_pos_map[pos -1] == 1)
		{
			ret = true;
		}
	}

	return ret;
}

static status_t _diseqc12_save_position(void)
{
	GxMsgProperty_NodeModify node_modify;
	uint8_t pos =0;

	if(_diseqc12_check_pos_record(s_PosiSat.sat_data.sat_s.diseqc12_pos) == false)
	{
		pos = _diseqc12_get_valid_pos();
		if(pos > 0)
		{
			_diseqc12_set_pos_record(pos);
			s_PosiSat.sat_data.sat_s.diseqc_version |= GXBUS_PM_SAT_DISEQC_1_2;
			s_PosiSat.sat_data.sat_s.diseqc_version &= ~(GXBUS_PM_SAT_DISEQC_USALS);
			s_PosiSat.sat_data.sat_s.diseqc12_pos = pos;

			node_modify.node_type = NODE_SAT;
			memcpy(&node_modify.sat_data, &s_PosiSat.sat_data, sizeof(GxBusPmDataSat));
			app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
		}
		else
		{
			return GXCORE_ERROR;
		}
	}

	app_diseqc12_save_position(s_PosiSat.sat_data.tuner,s_PosiSat.sat_data.sat_s.diseqc12_pos);

	return GXCORE_SUCCESS;
}


static status_t _usals_save_param(void)
{
    int longitude;
    int latitude;
    char *str = NULL;

    GxMsgProperty_NodeModify node_modify;
    status_t ret = GXCORE_SUCCESS;

    s_PosiSat.sat_data.sat_s.diseqc_version &= ~(GXBUS_PM_SAT_DISEQC_1_2);
    s_PosiSat.sat_data.sat_s.diseqc_version |= GXBUS_PM_SAT_DISEQC_USALS;
    node_modify.node_type = NODE_SAT;
    memcpy(&node_modify.sat_data, &s_PosiSat.sat_data, sizeof(GxBusPmDataSat));
    app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);

    GxBus_ConfigSetInt(LONGITUDE_DIRECT_KEY, s_local_longitude_direct);
    GxBus_ConfigSetInt(LATITUDE_DIRECT_KEY, s_local_latitude_direct);

    GUI_GetProperty("edit_positioner_longitude", "string", &str);
    longitude = 10 * app_float_edit_str_to_value(str);

    GUI_GetProperty("edit_positioner_latitude", "string", &str);
    latitude = 10 * app_float_edit_str_to_value(str);

    GxBus_ConfigSetInt(LONGITUDE_KEY, longitude);
    GxBus_ConfigSetInt(LATITUDE_KEY, latitude);

    return ret;
}

static status_t _positioner_off(void)
{
	GxMsgProperty_NodeModify node_modify;
	status_t ret = GXCORE_SUCCESS;

	_diseqc12_clear_pos_record(s_PosiSat.sat_data.sat_s.diseqc12_pos);
	s_PosiSat.sat_data.sat_s.diseqc_version &= ~(GXBUS_PM_SAT_DISEQC_1_2);
	s_PosiSat.sat_data.sat_s.diseqc_version &= ~(GXBUS_PM_SAT_DISEQC_USALS);
	s_PosiSat.sat_data.sat_s.diseqc12_pos = 0;
	node_modify.node_type = NODE_SAT;
	memcpy(&node_modify.sat_data, &s_PosiSat.sat_data, sizeof(GxBusPmDataSat));
	app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);

	return ret;
}

static int _positioner_sat_pop(int sel)
{
	PopList pop_list;
	int ret =-1;
	int i;
	char **sat_name = NULL;
	GxMsgProperty_NodeByPosGet node;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_SATELLITE;
	pop_list.item_num = s_posi_sat_total;

	if(0 == s_posi_sat_total)
	{
		printf("_positioner_sat_pop,error  sat totol =0...%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	sat_name = (char**)GxCore_Malloc(s_posi_sat_total * sizeof(char*));
	if(sat_name == NULL)
	{
		printf("_positioner_sat_pop, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	if(sat_name != NULL)
	{
		for(i = 0; i < s_posi_sat_total; i++)
		{
			node.node_type = NODE_SAT;
			node.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
			sat_name[i] = (char*)GxCore_Malloc(MAX_SAT_NAME);
			if(sat_name[i] != NULL)
			{
				memset(sat_name[i], 0, MAX_SAT_NAME);
				strcpy(sat_name[i] , (char*)node.sat_data.sat_s.sat_name);
			}
		}
	}
	pop_list.item_content = sat_name;

	pop_list.sel = sel;
	pop_list.show_num= true;

	ret = poplist_create(&pop_list);

	if(sat_name != NULL)
	{
		for(i = 0; i < s_posi_sat_total; i++)
		{
			if(sat_name[i] != NULL)
			{
				GxCore_Free(sat_name[i]);
			}
		}
		GxCore_Free(sat_name);
	}

	return ret;
}

static int _positioner_tp_pop(int sel)
{
#define BUFFER_LENGTH_TP	(64)

	PopList pop_list;
	int ret =-1;
	int i;
	char **tp_info = NULL;
	GxMsgProperty_NodeByPosGet node;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_TP;
	pop_list.item_num = s_posi_tp_total;

	if(0 == s_posi_tp_total)
	{
		printf("_positioner_tp_pop, tp total error...%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	tp_info = (char**)GxCore_Malloc(s_posi_tp_total * sizeof(char*));
	if(tp_info == NULL)
	{
		printf("_positioner_tp_pop, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return 0;
	}

	if(tp_info != NULL)
	{
		for(i = 0; i < s_posi_tp_total; i++)
		{
			node.node_type = NODE_TP;
			node.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

			tp_info[i] = (char*)GxCore_Malloc(BUFFER_LENGTH_TP);
			if(tp_info[i] != NULL)
			{
				memset(tp_info[i], 0, BUFFER_LENGTH_TP);
				if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
				{
					sprintf(tp_info[i],"%d / %s / %d", node.tp_data.frequency, "V" ,node.tp_data.tp_s.symbol_rate);
				}
				else if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
				{
					sprintf(tp_info[i],"%d / %s / %d", node.tp_data.frequency, "H" ,node.tp_data.tp_s.symbol_rate);
				}
			}
		}
	}
	pop_list.item_content = tp_info;

	pop_list.sel = sel;
	pop_list.show_num= true;

	ret = poplist_create(&pop_list);

	if(tp_info != NULL)
	{
		for(i = 0; i < s_posi_tp_total; i++)
		{
			if(tp_info[i] != NULL)
			{
				GxCore_Free(tp_info[i]);
			}
		}
		GxCore_Free(tp_info);
	}

	return ret;
}


static int _positioner_motor_pop(int sel)
{
	PopList pop_list;
	int ret =-1;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_MOTOR_TYPE;
	pop_list.item_num = sizeof(s_moto_type_content)/sizeof(*s_moto_type_content);;
	pop_list.item_content = s_moto_type_content;
	pop_list.sel = sel;
	pop_list.show_num= false;

	ret = poplist_create(&pop_list);

	return ret;
}

#define POSITIONER_SETTING
SIGNAL_HANDLER int app_positioner_setting_create(GuiWidget *widget, void *usrdata)
{
    GxMsgProperty_NodeNumGet node_num = {0};
    uint32_t cur_num = 0;
    //int val;

    s_posi_sat_total = 0;
    s_posi_tp_total = 0;
    memset(&s_PosiSat, 0, sizeof(s_PosiSat));
    memset(&s_PosiTp, 0, sizeof(s_PosiTp));

    //get total sat num
    node_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
    s_posi_sat_total = node_num.node_num;
    if(s_posi_sat_total > 0)
    {
        //rebuild sat list and show cur sat name
        _diseqc12_init_pos_record();
        _positioner_cmb_sat_rebuild();
        _positioner_cmb_tp_rebuild();
        _positioner_set_tp(SAVE_TP_PARAM);

        if(satsel.sel_in_all >= s_posi_sat_total)
        {
            cur_num = 0;
        }
        else
        {
            cur_num = satsel.sel_in_all;
        }
        GUI_SetProperty("cmb_positioner_sat_opt","select",&cur_num);

        if((s_PosiSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
        {
            s_positioner_mode = POSITIONER_DISEQC12;
            _positioner_diseqc12_view();
            s_cur_max_item = MAX_POSITIONER_ON_ITEM;
        }
        else if((s_PosiSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
        {
            s_positioner_mode = POSITIONER_USALS;
            _positioner_usals_view();
            s_cur_max_item = MAX_POSITIONER_ON_ITEM;
        }
        else
        {
            s_positioner_mode = POSITIONER_OFF;
            _positioner_off_view();
            s_cur_max_item = MAX_POSITIONER_OFF_ITEM;
        }
        GUI_SetProperty("cmb_positioner_motor_opt", "select", &s_positioner_mode);
        app_lang_set_menu_font_type("text_positioner_title");
        _positioner_unfocus_item(s_positioner_sel);
        s_positioner_sel = 0;
        _positioner_focus_item(s_positioner_sel);
    }

    if (reset_timer(sp_PosiTimerSignal) != 0)
    {
        sp_PosiTimerSignal = create_timer(timer_positioner_show_signal, 100, NULL, TIMER_REPEAT);
    }
#if DOUBLE_S2_SUPPORT
    {
        GUI_SetProperty("text_positioner_tuner_info", "state", "show");
        if(s_PosiSat.sat_data.tuner == 0)
            GUI_SetProperty("text_positioner_tuner_info", "string", "Tuner1");
        else if(s_PosiSat.sat_data.tuner == 1)
            GUI_SetProperty("text_positioner_tuner_info", "string", "Tuner2");
        else
            GUI_SetProperty("text_positioner_tuner_info", "string", "Tuner1 & Tuner2");
    }
#endif
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_setting_destroy(GuiWidget *widget, void *usrdata)
{
    AppFrontend_Monitor monitor = FRONTEND_MONITOR_ON;

    GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
    timer_positioner_show_signal_stop();

    remove_timer(sp_PosiTimerSignal);
    sp_PosiTimerSignal = NULL;

    remove_timer(sp_PosiLockTimer);
    sp_PosiLockTimer = NULL;

    app_ioctl(s_PosiSat.sat_data.tuner, FRONTEND_MONITOR_SET, &monitor);

    app_epg_info_clean();

    return EVENT_TRANSFER_STOP;
}

#if (DLNA_SUPPORT > 0)
static void app_positioner_start_dlna_cb(void)
{
    timer_positioner_show_signal_stop();
}

static void app_positioner_stop_dlna_cb(void)
{
    timer_positioner_show_signal_reset();
}
#endif

SIGNAL_HANDLER int app_positioner_setting_keypress(GuiWidget *widget, void *usrdata)
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
#if (DLNA_SUPPORT > 0)
                case VK_DLNA_TRIGGER:
                    app_dlna_ui_exec(app_positioner_start_dlna_cb, app_positioner_stop_dlna_cb);
                    break;
#endif
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog("wnd_positioner_setting");
                    break;

                case STBK_UP:
                    _positioner_unfocus_item(s_positioner_sel);
                    (s_positioner_sel == 0) ? s_positioner_sel = s_cur_max_item - 1
                        : s_positioner_sel--;
                    _positioner_focus_item(s_positioner_sel);
                    break;

                case STBK_DOWN:
                    _positioner_unfocus_item(s_positioner_sel);
                    (s_positioner_sel == s_cur_max_item - 1) ? s_positioner_sel = 0
                        : s_positioner_sel++;
                    _positioner_focus_item(s_positioner_sel);
                    break;

                case STBK_OK:
                    {
                        int sel;
                        if((s_positioner_sel == 0) && (s_posi_sat_total > 0))//sat list
                        {
                            GUI_GetProperty("cmb_positioner_sat_opt", "select", &sel);
                            sel = _positioner_sat_pop(sel);
                            GUI_SetProperty("cmb_positioner_sat_opt", "select", &sel);
                        }
                        else if((s_positioner_sel == 1) && (s_posi_tp_total > 0))// tp list
                        {
                            GUI_GetProperty("cmb_positioner_tp_opt", "select", &sel);
                            sel = _positioner_tp_pop(sel);
                            GUI_SetProperty("cmb_positioner_tp_opt", "select", &sel);
                        }
                        else if(s_positioner_sel == 2)// motor type
                        {
                            sel = _positioner_motor_pop(s_positioner_mode);
                            GUI_SetProperty("cmb_positioner_motor_opt", "select", &sel);
                        }
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

SIGNAL_HANDLER int app_positioner_setting_got_focus(GuiWidget *widget, void *usrdata)
{
	char *item_widget = NULL;
	char *left_img = IMG_BAR_HALF_FOCUS_L;
	char *right_img = IMG_BAR_HALF_UNFOCUS;
	char *property = "unfocus_img";

	/////////////left///////////////
	if((s_positioner_sel == 0) || (s_positioner_sel == 1) || (s_positioner_sel == 2))
	{
#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
		GUI_SetProperty(s_item_chioce_ok[s_positioner_sel], "state", "show");
#else
		left_img = IMG_BAR_HALF_FOCUS_OK;
#endif
	}
	if(s_positioner_mode == POSITIONER_DISEQC12)
	{
		if((s_positioner_sel == 4) || (s_positioner_sel == 6))
		{
#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
		GUI_SetProperty(s_item_chioce_ok[s_positioner_sel], "state", "show");
#else
			left_img = IMG_BAR_HALF_FOCUS_OK;
#endif
		}
	}
	GUI_SetProperty(s_positioner_choice_back[s_positioner_sel], "img", left_img);
	GUI_SetProperty(s_positioner_item_title[s_positioner_sel], "string", s_positioner_title_content[s_positioner_mode][s_positioner_sel]);

	/////////////right///////////
	if(s_positioner_mode == POSITIONER_OFF)
	{
		item_widget = s_positioner_item_widget[POSITIONER_OFF][s_positioner_sel];
	}
	else if(s_positioner_mode == POSITIONER_DISEQC12)
	{
		item_widget = s_positioner_item_widget[POSITIONER_DISEQC12][s_positioner_sel];
	}
	else if(s_positioner_mode == POSITIONER_USALS)
	{
		if(s_positioner_sel == 3) //longitude
		{
			item_widget = "boxitem_positioner_longitude";
			right_img = IMG_BAR_HALF_UNFOCUS_AR;
			property = "unfocus_image";
		}
		else if(s_positioner_sel == 4)//latitude
		{
			item_widget = "boxitem_positioner_latitude";
			right_img = IMG_BAR_HALF_UNFOCUS_AR;
			property = "unfocus_image";
		}
		else
		{
			item_widget =  s_positioner_item_widget[POSITIONER_USALS][s_positioner_sel];
		}
	}

	if(item_widget != NULL)
	{
		GUI_SetProperty(item_widget, property, right_img);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_setting_lost_focus(GuiWidget *widget, void *usrdata)
{
	char *item_widget = NULL;
	char *img_l = IMG_BAR_HALF_FOCUS_L;
	char *img_r = IMG_BAR_HALF_FOCUS_R;
	char *property = "unfocus_img";

	//////////left img///////////////
	GUI_SetProperty(s_positioner_choice_back[s_positioner_sel], "img", img_l);
	GUI_SetProperty(s_positioner_item_title[s_positioner_sel], "string", s_positioner_title_content[s_positioner_mode][s_positioner_sel]);
	///////////right img//////////////
	if(s_positioner_mode == POSITIONER_OFF)
	{
		item_widget = s_positioner_item_widget[POSITIONER_OFF][s_positioner_sel];
	}
	else if(s_positioner_mode == POSITIONER_DISEQC12)
	{
		item_widget = s_positioner_item_widget[POSITIONER_DISEQC12][s_positioner_sel];
	}
	else if(s_positioner_mode == POSITIONER_USALS)
	{
		if(s_positioner_sel == 3) //longitude
		{
			item_widget = "boxitem_positioner_longitude";
			img_r = IMG_BAR_HALF_FOCUS_AR;
			property = "unfocus_image";
		}
		else if(s_positioner_sel == 4)//latitude
		{
			item_widget = "boxitem_positioner_latitude";
			img_r = IMG_BAR_HALF_FOCUS_AR;
			property = "unfocus_image";
		}
		else//save
		{
			item_widget =  s_positioner_item_widget[POSITIONER_USALS][s_positioner_sel];
		}
	}

	if(item_widget != NULL)
	{
		GUI_SetProperty(item_widget, property, img_r);
	}
	if(s_positioner_sel == 0)
	{
#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
		GUI_SetProperty(s_item_chioce_ok[s_positioner_sel], "img", "s_button_ok.bmp");
#endif
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_sat_change(GuiWidget *widget, void *usrdata)
{
	uint32_t sel;

	GUI_GetProperty("cmb_positioner_sat_opt", "select", &sel);
	s_PosiSat.node_type = NODE_SAT;
	s_PosiSat.pos = sel;
    app_sat_list_change_sel(s_PosiSat.pos);
	if(sel != s_PosiSat.pos)
	{
		s_posi_tp_total = 0;
	}

	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_PosiSat);
	_positioner_cmb_tp_rebuild();

	if((s_PosiSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
	{
		s_positioner_mode = POSITIONER_DISEQC12;
		_positioner_diseqc12_view();
		s_cur_max_item = MAX_POSITIONER_ON_ITEM;
	}
	else if((s_PosiSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
	{
		s_positioner_mode = POSITIONER_USALS;
		_positioner_usals_view();
		s_cur_max_item = MAX_POSITIONER_ON_ITEM;
	}
	else
	{
		s_positioner_mode = POSITIONER_OFF;
		_positioner_off_view();
		s_cur_max_item = MAX_POSITIONER_OFF_ITEM;
	}
	GUI_SetProperty("cmb_positioner_motor_opt", "select", &s_positioner_mode);

	if(reset_timer(sp_PosiLockTimer) != 0)
	{
		sp_PosiLockTimer = create_timer(timer_positioner_set_frontend, FRONTEND_SET_DELAY_MS, NULL, TIMER_ONCE);
	}

#if DOUBLE_S2_SUPPORT
	if(s_PosiSat.sat_data.tuner == 0)
		GUI_SetProperty("text_positioner_tuner_info", "string", "Tuner1");
	else if(s_PosiSat.sat_data.tuner == 1)
		GUI_SetProperty("text_positioner_tuner_info", "string", "Tuner2");
	else
		GUI_SetProperty("text_positioner_tuner_info", "string", "Tuner1 & Tuner2");
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_tp_change(GuiWidget *widget, void *usrdata)
{
	uint32_t sel;

	GUI_GetProperty("cmb_positioner_tp_opt", "select", &sel);

	s_PosiTp.node_type = NODE_TP;
	s_PosiTp.pos = sel;

	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_PosiTp);

	if(reset_timer(sp_PosiLockTimer) != 0)
	{
		sp_PosiLockTimer = create_timer(timer_positioner_set_frontend, FRONTEND_SET_DELAY_MS, NULL, TIMER_ONCE);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_motor_type_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	int cmb_sel;

	GUI_GetProperty("cmb_positioner_motor_opt", "select", &cmb_sel);
	s_positioner_mode = cmb_sel;

	if(s_positioner_mode ==POSITIONER_OFF)
	{
		_positioner_off_view();
		s_cur_max_item = MAX_POSITIONER_OFF_ITEM;
	}
	else if(s_positioner_mode == POSITIONER_DISEQC12)
	{
		_positioner_diseqc12_view();
		s_cur_max_item = MAX_POSITIONER_ON_ITEM;

	}
	else if(s_positioner_mode ==POSITIONER_USALS)
	{
		_positioner_usals_view();
		s_cur_max_item = MAX_POSITIONER_ON_ITEM;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_fine_drive_keypress(GuiWidget *widget, void *usrdata)
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
            switch(event->key.sym)
            {
                case STBK_LEFT:
                    if(s_dish_moving_direct != DISH_MOVE_WEST)
                    {
                        timer_positioner_show_signal_stop();
                        {
                            extern void GxFrontend_ForceExitEx(int32_t tuner);
#if DOUBLE_S2_SUPPORT
                            GxFrontend_ForceExitEx(0);
                            GxFrontend_ForceExitEx(1);
#else
                            GxFrontend_ForceExitEx(s_PosiSat.sat_data.tuner);
#endif
                        }
                        {
                            extern void GxFrontend_StopMonitor(int32_t tuner);
                            GxFrontend_StopMonitor(s_PosiSat.sat_data.tuner);
                        }
                        app_diseqc12_find_drive_drictory(s_PosiSat.sat_data.tuner,LONGITUDE_WEST);//west
                        GUI_SetProperty("cmb_positioner_fine_drive", "focus_right_img", "s_arrow_r.bmp");
                        GUI_SetProperty("cmb_positioner_fine_drive", "focus_left_img", "s_arrow_l_yellow.bmp");
                        s_dish_moving_direct = DISH_MOVE_WEST;
                    }
                    if (reset_timer(sp_PosiArrow) != 0)
                    {
                        sp_PosiArrow = create_timer(timer_positioner_stop_fine_drive, 500, NULL, TIMER_REPEAT);
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_RIGHT:
                    if(s_dish_moving_direct != DISH_MOVE_EAST)
                    {
                        timer_positioner_show_signal_stop();
                        {
                            extern void GxFrontend_ForceExitEx(int32_t tuner);
#if DOUBLE_S2_SUPPORT
                            GxFrontend_ForceExitEx(0);
                            GxFrontend_ForceExitEx(1);
#else
                            GxFrontend_ForceExitEx(s_PosiSat.sat_data.tuner);
#endif
                        }
                        {
                            extern void GxFrontend_StopMonitor(int32_t tuner);
                            GxFrontend_StopMonitor(s_PosiSat.sat_data.tuner);
                        }
                        app_diseqc12_find_drive_drictory(s_PosiSat.sat_data.tuner,LONGITUDE_EAST);//east
                        GUI_SetProperty("cmb_positioner_fine_drive", "focus_left_img", "s_arrow_l.bmp");
                        GUI_SetProperty("cmb_positioner_fine_drive", "focus_right_img", "s_arrow_r_yellow.bmp");
                        s_dish_moving_direct = DISH_MOVE_EAST;
                    }
                    if (reset_timer(sp_PosiArrow) != 0)
                    {
                        sp_PosiArrow = create_timer(timer_positioner_stop_fine_drive, 500, NULL, TIMER_REPEAT);
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_positioner_limit_keypress(GuiWidget *widget, void *usrdata)
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
            switch(event->key.sym)
            {
                case STBK_OK:
                    if(s_positioner_mode == POSITIONER_DISEQC12)
                    {
                        int sel;
                        PopDlg  pop;

                        {
                            extern void GxFrontend_ForceExitEx(int32_t tuner);
                            GxFrontend_ForceExitEx(s_PosiSat.sat_data.tuner);
                        }

                        GUI_GetProperty("cmb_positioner_limit", "select", &sel);
                        app_diseqc12_save_limit(s_PosiSat.sat_data.tuner,sel);

                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.str = STR_ID_SAVING;
                        pop.creat_cb = NULL;
                        pop.exit_cb = NULL;
                        pop.timeout_sec= 2;
                        popdlg_create(&pop);
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

SIGNAL_HANDLER int app_positioner_recalc_keypress(GuiWidget *widget, void *usrdata)
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
            switch(event->key.sym)
            {
                case STBK_OK:
                    if(s_positioner_mode == POSITIONER_DISEQC12)
                    {
                        PopDlg pop;

                        app_diseqc12_recalculate(s_PosiSat.sat_data.tuner,s_PosiSat.sat_data.sat_s.diseqc12_pos);

                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.str = STR_ID_SAVING;
                        pop.creat_cb = NULL;
                        pop.exit_cb = NULL;
                        pop.timeout_sec= 1;
                        popdlg_create(&pop);

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

SIGNAL_HANDLER int app_positioner_diseqc12_goto_keypress(GuiWidget *widget, void *usrdata)
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
			switch(event->key.sym)
			{
				case STBK_OK:
				{
					int sel = 0;
					if(s_positioner_mode == POSITIONER_DISEQC12)
					{
						bool force = true;
						GUI_GetProperty("cmb_positioner_diseqc12_goto", "select", &sel);
						if(sel == 0)
						{
							app_diseqc12_goto_position(s_PosiSat.sat_data.tuner, s_PosiSat.sat_data.sat_s.diseqc12_pos, force);
						}
						else if(sel == 1)
						{
							app_diseqc12_goto_zero(s_PosiSat.sat_data.tuner,force);
						}
						else
							;
						ret = EVENT_TRANSFER_STOP;
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

SIGNAL_HANDLER int app_positioner_usals_goto_keypress(GuiWidget *widget, void *usrdata)
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
			switch(event->key.sym)
			{
				case STBK_OK:
					if(s_positioner_mode == POSITIONER_USALS) //
					{
						bool force = true;

						LongitudeInfo sat_longitude = {0};
						PositionVal local_position;
						char *str = NULL;

						local_position.longitude.longitude_direct = s_local_longitude_direct;
						GUI_GetProperty("edit_positioner_longitude", "string", &str);
						local_position.longitude.longitude = 10 * app_float_edit_str_to_value(str);

						local_position.latitude.latitude_direct = s_local_latitude_direct;
						GUI_GetProperty("edit_positioner_latitude", "string", &str);
						local_position.latitude.latitude = 10 * app_float_edit_str_to_value(str);

						sat_longitude.longitude = s_PosiSat.sat_data.sat_s.longitude;
						sat_longitude.longitude_direct = s_PosiSat.sat_data.sat_s.longitude_direct;
						app_usals_goto_position(s_PosiSat.sat_data.tuner,&sat_longitude,&local_position, force);
						ret = EVENT_TRANSFER_STOP;
					}
					else
						;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_off_save_keypress(GuiWidget *widget, void *usrdata)
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
			switch(event->key.sym)
			{
				case STBK_OK:
					if(s_positioner_mode == POSITIONER_OFF)
					{
						PopDlg  pop;

						_positioner_off();

						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_NO_BTN;
						pop.str = STR_ID_SAVING;
						pop.creat_cb = NULL;
						pop.exit_cb = NULL;
						pop.timeout_sec= 1;
						popdlg_create(&pop);
					}

					ret = EVENT_TRANSFER_STOP;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}


SIGNAL_HANDLER int app_positioner_on_save_keypress(GuiWidget *widget, void *usrdata)
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
			switch(event->key.sym)
			{
				case STBK_OK:
				{
					PopDlg  pop;

					if(s_positioner_mode == POSITIONER_USALS)
					{
						if(_usals_save_param() == GXCORE_SUCCESS)
						{
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_NO_BTN;
							pop.str = STR_ID_SAVING;
							pop.creat_cb = NULL;
							pop.exit_cb = NULL;
							pop.timeout_sec = 1;
							popdlg_create(&pop);
						}
						else
						{
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_OK;
							pop.str = STR_ID_ERR_POSITION;
							pop.mode = POP_MODE_UNBLOCK;
							popdlg_create(&pop);
						}
					}
					else if(s_positioner_mode == POSITIONER_DISEQC12)
					{
						if(_diseqc12_save_position() == GXCORE_SUCCESS)
						{
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_NO_BTN;
							pop.str = STR_ID_SAVING;
							pop.creat_cb = NULL;
							pop.exit_cb = NULL;
							pop.timeout_sec= 1;
							popdlg_create(&pop);
						}
						else
						{
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_OK;
							pop.str = STR_ID_POS_FULL;
							pop.mode = POP_MODE_UNBLOCK;
							popdlg_create(&pop);
						}
					}
					ret = EVENT_TRANSFER_STOP;
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

SIGNAL_HANDLER int app_positioner_local_keypress(GuiWidget *widget, void *usrdata)
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
			switch(event->key.sym)
			{
				case STBK_UP:
				case STBK_DOWN:
					GUI_SendEvent("wnd_positioner_setting", event);
					ret = EVENT_TRANSFER_STOP;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_longitude_edit_keypress(GuiWidget *widget, void *usrdata)
{

	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	char buff[2] = {0};

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
				case STBK_LEFT:
				case STBK_RIGHT:
					if(s_local_longitude_direct == LONGITUDE_EAST)
					{
						buff[0] = 'W';
						s_local_longitude_direct = LONGITUDE_WEST;
					}
					else
					{
						buff[0] = 'E';
						s_local_longitude_direct = LONGITUDE_EAST;
					}
					GUI_SetProperty("text_positioner_longitude_direct", "string", buff);
					ret = EVENT_TRANSFER_STOP;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_longitude_edit_change(GuiWidget *widget, void *usrdata)
{
	int longitude = 0;
	char *atoi_str = NULL;
	char buff[10] = {0};

	GUI_GetProperty("edit_positioner_longitude", "string", &atoi_str);
	longitude = 10 * app_float_edit_str_to_value(atoi_str);
	if(longitude > 1800)
	{
		longitude = 1800;
	}

	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%03d.%d", longitude/10, longitude%10);
	GUI_SetProperty("edit_positioner_longitude", "string", buff);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_longitude_edit_reachend(GuiWidget *widget, void *usrdata)
{
	uint32_t sel = 0;

	GUI_SetProperty("edit_positioner_longitude", "select", &sel);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_latitude_edit_keypress(GuiWidget *widget, void *usrdata)
{

	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	char buff[2] = {0};

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
				case STBK_LEFT:
				case STBK_RIGHT:
					if(s_local_latitude_direct == LATITUDE_SOUTH)
					{
						buff[0] = 'N';
						s_local_latitude_direct = LATITUDE_NORTH;
					}
					else
					{
						buff[0] = 'S';
						s_local_latitude_direct = LATITUDE_SOUTH;
					}
					GUI_SetProperty("text_positioner_latitude_direct", "string", buff);
					ret = EVENT_TRANSFER_STOP;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_positioner_latitude_edit_change(GuiWidget *widget, void *usrdata)
{
	int latitude = 0;
	char *atoi_str = NULL;
	char buff[10] = {0};

	GUI_GetProperty("edit_positioner_latitude", "string", &atoi_str);
	latitude = 10 * app_float_edit_str_to_value(atoi_str);

	if(latitude > 900)
	{
		latitude = 900;
	}

	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%02d.%d", latitude/10, latitude%10);
	GUI_SetProperty("edit_positioner_latitude", "string", buff);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_positioner_latitude_edit_reachend(GuiWidget *widget, void *usrdata)
{
	uint32_t sel = 0;

    GUI_SetProperty("edit_positioner_latitude", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

#else //DEMOD_DVB_S

SIGNAL_HANDLER int app_positioner_setting_create(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_setting_destroy(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_setting_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_setting_got_focus(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_setting_lost_focus(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_sat_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_tp_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_motor_type_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_fine_drive_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_limit_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_recalc_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_diseqc12_goto_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_usals_goto_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_off_save_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_on_save_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_local_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_longitude_edit_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_longitude_edit_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_longitude_edit_reachend(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_latitude_edit_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_latitude_edit_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_positioner_latitude_edit_reachend(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

#endif
