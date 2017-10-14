#include "app.h"
#if (DEMOD_DVB_S > 0)

#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "app_frontend.h"
#include "app_pop.h"
#include "app_epg.h"
#include "full_screen.h"
#include "app_book.h"
#if TKGS_SUPPORT
#include "module/gxtkgs.h"

extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
#endif


extern uint32_t app_tuner_sat_num_get(uint32_t tuner);
extern void app_sat_list_change_sel(uint32_t sat_sel);
extern void app_antenna_tp_change_set(uint32_t SatSel);
extern SatSelStatus satsel;
/* Private variable------------------------------------------------------- */

enum TP_LIST_MODE
{
	TP_ADD = 0,
	TP_EDIT,
	TP_DELETE,
	TP_SEARCH,
	TP_CHANGED
};

enum TP_LIST_POPUP_BOX
{
	TP_LIST_POPUP_BOX_ITEM_FREQ = 0,
	TP_LIST_POPUP_BOX_ITEM_POLAR,
	TP_LIST_POPUP_BOX_ITEM_SYM,
	TP_LIST_POPUP_BOX_ITEM_OK
};

enum TP_PARA_VALID_OR_NOT
{
	TP_PARA_VALID=0,
	TP_PARA_INVALID
};

typedef struct 
{
	uint32_t frequency;
	uint32_t symbol_rate;
	GxBusPmTpPolar polar;
}TpListTpPara;
#define MAX_FRE_LEN (5)

static event_list* sp_TpListTimerSignal = NULL;
static GxMsgProperty_NodeByPosGet s_CurSat = {0};
static GxMsgProperty_NodeByPosGet s_CurTp = {0};
static uint8_t s_CurMode=0;
static uint8_t s_Changed=0;
static int s_CurTpSel = 0;	//tp listview sel
static uint16_t s_tplist_tp_choise = 0;
uint16_t s_tplist_tp_total = 0;
uint16_t s_tplist_sat_total = 0;
static event_list* sp_TpListLockTimer=NULL;
// Tl - tp list
static AppIdOps* s_TlIdOps = NULL;

static void _tp_list_set_diseqc(void);
static void _tp_list_set_tp(SetTpMode set_mode);

static char s_tplist_edit_input[6] = {0};
static int match_exist_sel =0;
static bool match_status = 0;
static bool edit_command_mode = 1;
static bool edit_del_to_zero = 0;
static bool up_down_status = 0;
static bool tp_exist_status = 0;
char buffer_fre_bak[MAX_FRE_LEN +1] = {0};
char buffer_sym_bak[MAX_FRE_LEN +1] = {0};

// create dialog
void app_set_info_cursel(uint32_t sel)
{
	s_CurTpSel = sel;
}

int app_tp_list_init(int SatSel, int TpSel)
{
	if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_tp_list"))
	{
		GUI_EndDialog("wnd_tp_list");
	}
	GUI_CreateDialog("wnd_tp_list");

	if(SatSel > 0)
		GUI_SetProperty("cmb_tp_list_sat", "select", &SatSel);
	if(TpSel >= 0)
		GUI_SetProperty("list_tp_list", "select", &TpSel);
	return 0;
}

/*
function:
data :2012 11 1
*/
status_t app_factory_test_add_tp(uint32_t sat_id,uint32_t * ptp_id)
{
    status_t ret = GXCORE_ERROR;
    GxMsgProperty_NodeAdd NodeAdd;
    memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));
    NodeAdd.node_type = NODE_TP;
    NodeAdd.tp_data.sat_id = sat_id ;//s_CurSat.sat_data.id;
    NodeAdd.tp_data.frequency = 4000;// InputTpPara.frequency;
    NodeAdd.tp_data.tp_s.symbol_rate = 26850;//InputTpPara.symbol_rate;
    NodeAdd.tp_data.tp_s.polar = 0;// 0:H  1:V   InputTpPara.polar;

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
    {
        *ptp_id = NodeAdd.tp_data.id;
        ret = GXCORE_SUCCESS;
    }
    return ret;
}

/* widget macro -------------------------------------------------------- */
#define ANTENNA_TIMER
static int timer_tp_list_show_signal(void *userdata)
{
	unsigned int str_value = 0;
	unsigned int qua_value = 0;
	char Buffer[20] = {0};
	SignalBarWidget siganl_bar = {0};
	SignalValue siganl_val = {0};

	if(s_tplist_tp_total > 0)
	{
		app_ioctl(s_CurSat.sat_data.tuner, FRONTEND_STRENGTH_GET, &str_value);
		app_ioctl(s_CurSat.sat_data.tuner, FRONTEND_QUALITY_GET, &qua_value);
	}
	else
	{
		str_value = 0;
		qua_value = 0;
		// clear the montor parameter
        GxFrontend_CleanRecordPara(s_CurSat.sat_data.tuner);
		GxFrontend_SetUnlock(s_CurSat.sat_data.tuner);
	}

	if(str_value > 99)
	{
		str_value = 100;
	}
	// progbar strength
	//g_AppNim.property_get(&g_AppNim, AppFrontendID_Strength,(void*)(&value),4);
	siganl_bar.strength_bar = "progbar_tp_list_strength";
	siganl_val.strength_val = str_value;
	
	// strength value
	memset(Buffer,0,sizeof(Buffer));
	sprintf(Buffer,"%d%%",str_value);
	GUI_SetProperty("text_tp_list_strength_value", "string", Buffer);

	if(qua_value > 99)
	{
		qua_value = 100;
	}
	// progbar quality
	//g_AppNim.property_get(&g_AppNim, AppFrontendID_Quality,(void*)(&value),4);
	siganl_bar.quality_bar= "progbar_tp_list_quality";
	siganl_val.quality_val = qua_value;
	
	// quality value
	memset(Buffer,0,sizeof(Buffer));
	sprintf(Buffer,"%d%%",qua_value);
	GUI_SetProperty("text_tp_list_quality_value", "string", Buffer);

	app_signal_progbar_update(s_CurSat.sat_data.tuner, &siganl_bar, &siganl_val);
	ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &qua_value);

	return 0;
}

static int app_tp_list_clear_signal(void)
{
	unsigned int str_value = 0;
	unsigned int qua_value = 0;
	char Buffer[10] = {0};
	SignalBarWidget siganl_bar = {0};
	SignalValue siganl_val = {0};

	// progbar strength
	siganl_bar.strength_bar = "progbar_tp_list_strength";
	siganl_val.strength_val = str_value;
	
	// strength value
	memset(Buffer,0,sizeof(Buffer));
	sprintf(Buffer,"%d%%",str_value);
	GUI_SetProperty("text_tp_list_strength_value", "string", Buffer);

	// progbar quality
	siganl_bar.quality_bar= "progbar_tp_list_quality";
	siganl_val.quality_val = qua_value;
	
	// quality value
	memset(Buffer,0,sizeof(Buffer));
	sprintf(Buffer,"%d%%",qua_value);
	GUI_SetProperty("text_tp_list_quality_value", "string", Buffer);

	app_signal_progbar_update(s_CurSat.sat_data.tuner, &siganl_bar, &siganl_val);


	return 0;
}

static void timer_tp_list_show_signal_stop(void)
{
	timer_stop(sp_TpListTimerSignal);
}

static void timer_tp_list_show_signal_reset(void)
{
	reset_timer(sp_TpListTimerSignal);
}

static int timer_tp_list_set_frontend(void *userdata)
{

	GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
	{
		extern void GxFrontend_ForceExitEx(int32_t tuner);
#if DOUBLE_S2_SUPPORT
		GxFrontend_ForceExitEx(0);
		GxFrontend_ForceExitEx(1);
#else
		GxFrontend_ForceExitEx(s_CurSat.sat_data.tuner);
#endif
	}
	remove_timer(sp_TpListLockTimer);
	sp_TpListLockTimer = NULL;
	
	_tp_list_set_diseqc();
	_tp_list_set_tp(SET_TP_FORCE);
	
	return 0;
}

#define SUB_FUNC
static void _tp_list_cmb_sat_rebuild(uint32_t TotalSat)
{
#define BUFFER_LENGTH	(MAX_SAT_NAME + 20)
	static GxMsgProperty_NodeByPosGet node;
	int i = 0;
	char *buffer_all = NULL;
	char buffer[BUFFER_LENGTH] = {0};

	if(0 == TotalSat)
	{
		printf("\n_tp_list_cmb_sat_rebuild, Total error...%s,%d\n",__FILE__,__LINE__);
		return;
	}
	//rebuild sat list in combo box
	buffer_all = GxCore_Malloc(TotalSat * BUFFER_LENGTH);
	if(buffer_all == NULL)
	{
		printf("\n_tp_list_cmb_sat_rebuild, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}
	memset(buffer_all, 0, TotalSat * BUFFER_LENGTH);
	
	//first one
	i = 0;
	node.node_type = NODE_SAT;
	node.pos = i;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
	memset(buffer, 0, BUFFER_LENGTH);
	sprintf(buffer,"[(%d/%d) %s", i+1, TotalSat, (char*)node.sat_data.sat_s.sat_name);
	strcat(buffer_all, buffer);

	if(TotalSat > 1)
	{
		for(i=1; i<TotalSat; i++)
		{
			node.node_type = NODE_SAT;
			node.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
			memset(buffer, 0, BUFFER_LENGTH);
			sprintf(buffer,",(%d/%d) %s", i+1, TotalSat, (char*)node.sat_data.sat_s.sat_name);
			strcat(buffer_all, buffer);
		}
	}
	//only 1 sat or finished
	strcat(buffer_all, "]");
	GUI_SetProperty("cmb_tp_list_sat","content",buffer_all);

	GxCore_Free(buffer_all);
	
	return;
}

static void _tp_list_set_diseqc(void)
{
	AppFrontend_DiseqcParameters diseqc10 = {0};
	AppFrontend_DiseqcParameters diseqc11 = {0};
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
#if (DEMOD_DVB_COMBO > 0)
		app_set_tune_mode(s_CurSat.sat_data.type);		
#endif

	if(s_tplist_sat_total > 0)
	{
		diseqc10.type = DiSEQC10;
		diseqc10.u.params_10.chDiseqc = s_CurSat.sat_data.sat_s.diseqc10;
		diseqc10.u.params_10.bPolar = app_show_to_lnb_polar(&s_CurSat.sat_data);
		
		if(s_tplist_tp_total == 0)
		{
			diseqc10.u.params_10.bPolar = SEC_VOLTAGE_OFF;
		}
		else
		{
			if(diseqc10.u.params_10.bPolar == SEC_VOLTAGE_ALL)
			{
				diseqc10.u.params_10.bPolar = app_show_to_tp_polar(&s_CurTp.tp_data);
			}
		}
		
		diseqc10.u.params_10.b22k = app_show_to_sat_22k(s_CurSat.sat_data.sat_s.switch_22K, &s_CurTp.tp_data);
		
		diseqc11.u.params_11.chUncom = s_CurSat.sat_data.sat_s.diseqc11;
	}

	diseqc11.type = DiSEQC11;
	diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc ;
	diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
	diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;

	// add in 20121012
	// for polar
	//GxFrontend_SetVoltage(sat_node.sat_data.tuner, polar);
	AppFrontend_PolarState Polar =diseqc10.u.params_10.bPolar ;
	app_ioctl(s_CurSat.sat_data.tuner, FRONTEND_POLAR_SET, (void*)&Polar);

	// for 22k	
	//GxFrontend_Set22K(sat_node.sat_data.tuner, sat22k);
	AppFrontend_22KState _22K = diseqc10.u.params_10.b22k;
	app_ioctl(s_CurSat.sat_data.tuner, FRONTEND_22K_SET, (void*)&_22K);
	
	app_set_diseqc_10(s_CurSat.sat_data.tuner, &diseqc10, true);
	app_set_diseqc_11(s_CurSat.sat_data.tuner, &diseqc11, true);

	if(((s_CurSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
		&&(s_tplist_tp_total != 0))
	{
		app_diseqc12_goto_position(s_CurSat.sat_data.tuner, s_CurSat.sat_data.sat_s.diseqc12_pos, false);
	}
	else if((s_CurSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
	{
		LongitudeInfo local_longitude = {0};
		local_longitude.longitude = s_CurSat.sat_data.sat_s.longitude;
		local_longitude.longitude_direct = s_CurSat.sat_data.sat_s.longitude_direct;
		app_usals_goto_position(s_CurSat.sat_data.tuner,&local_longitude, NULL, false);
	}

	// POST SEM
    GxFrontendOccur(s_CurSat.sat_data.tuner);

}

static void _tp_list_set_tp(SetTpMode set_mode)
{
    AppFrontend_SetTp params = {0};

	if(s_tplist_sat_total > 0)
	{
		params.sat22k = app_show_to_sat_22k(s_CurSat.sat_data.sat_s.switch_22K, &s_CurTp.tp_data);
		if(s_tplist_tp_total > 0)
		{
			params.fre = app_show_to_tp_freq(&s_CurSat.sat_data, &s_CurTp.tp_data);
			params.symb = s_CurTp.tp_data.tp_s.symbol_rate;

			params.polar = app_show_to_lnb_polar(&s_CurSat.sat_data);
			if(params.polar == SEC_VOLTAGE_ALL)
			{
				params.polar = app_show_to_tp_polar(&s_CurTp.tp_data);
			}
		}
		else
		{
			params.polar = SEC_VOLTAGE_OFF;
		}
	}
	params.tuner = s_CurSat.sat_data.tuner;
	#if UNICABLE_SUPPORT
	{
	uint32_t centre_frequency=0 ;
	uint32_t lnb_index=0 ;
		uint32_t set_tp_req=0;
		lnb_index = app_calculate_set_freq_and_initfreq(&s_CurSat.sat_data, &s_CurTp.tp_data,&set_tp_req,&centre_frequency);
		if(s_CurSat.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
		{
			params.fre  = set_tp_req;
			params.lnb_index  = lnb_index;
		}
		else
		{
			params.lnb_index  = 0x20;
		}
		params.centre_fre  = centre_frequency;
		params.if_channel_index  = s_CurSat.sat_data.sat_s.unicable_para.if_channel;
	params.if_fre  = s_CurSat.sat_data.sat_s.unicable_para.centre_fre[params.if_channel_index];
	}
	#endif
#if MUTI_TS_SUPPORT
	{
		GxMsgProperty_NodeByPosGet prog_node = {0};
		char ts_dmx[80] = {0};
		prog_node.node_type = NODE_PROG;
		prog_node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node));
		if(prog_node.prog_data.muti_ts_status)
		{
			params.pls_ts_id = prog_node.prog_data.muti_ts_id;
			params.pls_code = prog_node.prog_data.ts_pls_code;
		}
	}
#endif


	switch(s_CurSat.sat_data.type)
	{
		case GXBUS_PM_SAT_S:
		{
			params.type = FRONTEND_DVB_S;
			break;
		}
		case GXBUS_PM_SAT_C:
		{
#if DEMOD_DVB_C
			params.fre = s_CurTp.tp_data.frequency;
			params.symb = s_CurTp.tp_data.tp_c.symbol_rate;
			params.qam = s_CurTp.tp_data.tp_c.modulation;
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
			params.fre = s_CurTp.tp_data.frequency;
			params.bandwidth = s_CurTp.tp_data.tp_dvbt2.bandwidth;			
			params.type_1501 = DVBT_AUTO_MODE;//PLP Data not in tp note,should be auto
#endif
			params.type = FRONTEND_DVB_T2;
			break;
		}
#if DEMOD_DTMB
		case GXBUS_PM_SAT_DTMB:
			if(GXBUS_PM_SAT_1501_DTMB == s_CurSat.sat_data.sat_dtmb.work_mode)
			{
				params.fre = s_CurTp.tp_data.frequency;
				params.qam = s_CurTp.tp_data.tp_dtmb.symbol_rate;			
				params.type_1501 = DTMB;
			}
			else
			{// DVBC MODE
				params.fre = s_CurTp.tp_data.frequency;
				params.symb = s_CurTp.tp_data.tp_dtmb.symbol_rate;
				params.qam = s_CurTp.tp_data.tp_dtmb.modulation;
				params.type_1501 = DTMB_C;
			}
			params.type = FRONTEND_DTMB;
		break;
#endif
		default:
			params.type = FRONTEND_DVB_S;
	}
	app_set_tp(params.tuner, &params, set_mode);

	return;
}

static int _tp_list_sat_pop(int sel)
{
	PopList pop_list;
	int ret =-1;
	int i;
	char **sat_name = NULL;
	GxMsgProperty_NodeByPosGet node;
	
	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_SATELLITE;
	pop_list.item_num = s_tplist_sat_total;

	if(0 == s_tplist_sat_total)
	{
		printf("\n_tp_list_sat_pop, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	sat_name = (char**)GxCore_Malloc(s_tplist_sat_total * sizeof(char*));
	if(sat_name == NULL)
	{
		printf("\n_tp_list_sat_pop, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	
	if(sat_name != NULL)
	{
		for(i = 0; i < s_tplist_sat_total; i++)
		{
			node.node_type = NODE_SAT;
			node.pos = i; // map pos.
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
    pop_list.line_num= POPLIST_LINE_5;
    pop_list.pos.x= 700;
    pop_list.pos.y= 111;

	ret = poplist_create(&pop_list);

	if(sat_name != NULL)
	{
		for(i = 0; i < s_tplist_sat_total; i++)
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

#define TP_INFO_ALL 0
#define TP_INFO_ADD 1
static void _tp_list_show_tip(void)
{
#define IMG_RED     	"img_tp_list_tip_red"
#define TXT_RED     	"text_tp_list_tip_red"
#define IMG_GREEN   	"img_tp_list_tip_green"
#define TXT_GREEN   	"text_tp_list_tip_green"
#define IMG_YELLOW	"img_tp_list_tip_yellow"
#define TXT_YELLOW	"text_tp_list_tip_yellow"
#define IMG_BLUE	"img_tp_list_tip_blue"
#define TXT_BLUE		"text_tp_list_tip_blue"
#define IMG_0   	"img_tp_list_tip_0"
#define TXT_0   	"text_tp_list_tip_0"
#define IMG_INFO	"img_tp_list_tip_info"
#define TXT_INFO  "text_tp_list_tip_info"

	GUI_SetProperty(IMG_0, "state", "show");
	GUI_SetProperty(TXT_0, "state", "show");

	GUI_SetProperty(IMG_INFO, "state", "show");
	GUI_SetProperty(TXT_INFO, "state", "show");

	GUI_SetProperty(IMG_RED, "state", "show");
	GUI_SetProperty(IMG_GREEN, "state", "show");
	GUI_SetProperty(IMG_YELLOW, "state", "show");
	GUI_SetProperty(IMG_BLUE, "state", "show");

	GUI_SetProperty(TXT_GREEN, "state", "show");
	GUI_SetProperty(TXT_RED, "state", "show");
	GUI_SetProperty(TXT_YELLOW, "state", "show");
	GUI_SetProperty(TXT_BLUE, "state", "show");

}

static void _tp_list_hide_tip(int tp_help_info)
{
#define IMG_RED     	"img_tp_list_tip_red"
#define TXT_RED     	"text_tp_list_tip_red"
#define IMG_GREEN   	"img_tp_list_tip_green"
#define TXT_GREEN   	"text_tp_list_tip_green"
#define IMG_YELLOW	    "img_tp_list_tip_yellow"
#define TXT_YELLOW	    "text_tp_list_tip_yellow"
#define IMG_BLUE	    "img_tp_list_tip_blue"
#define TXT_BLUE		"text_tp_list_tip_blue"
#define IMG_0   	    "img_tp_list_tip_0"
#define TXT_0   	    "text_tp_list_tip_0"
#define IMG_INFO	    "img_tp_list_tip_info"
#define TXT_INFO        "text_tp_list_tip_info"

	GUI_SetProperty(IMG_0, "state", "hide");
	GUI_SetProperty(TXT_0, "state", "hide");

	GUI_SetProperty(IMG_RED, "state", "hide");
	if(tp_help_info == TP_INFO_ALL)
	{
		GUI_SetProperty(IMG_GREEN, "state", "hide");
		GUI_SetProperty(TXT_GREEN, "state", "hide");
		GUI_SetProperty(IMG_INFO, "state", "hide");
		GUI_SetProperty(TXT_INFO, "state", "hide");
	}
	else if (tp_help_info == TP_INFO_ADD)
	{
		GUI_SetProperty(IMG_GREEN, "state", "show");
		GUI_SetProperty(TXT_GREEN, "state", "show");
		GUI_SetProperty(IMG_INFO, "state", "show");
		GUI_SetProperty(TXT_INFO, "state", "show");
	}
	GUI_SetProperty(IMG_YELLOW, "state", "hide");
	GUI_SetProperty(IMG_BLUE, "state", "hide");

	GUI_SetProperty(TXT_RED, "state", "hide");
	GUI_SetProperty(TXT_YELLOW, "state", "hide");
	GUI_SetProperty(TXT_BLUE, "state", "hide");

}


#define TP_LIST
SIGNAL_HANDLER int app_tp_list_create(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_NodeNumGet node_num = {0};
	
	app_lang_set_menu_font_type("text_tp_list_title");
	app_tp_list_clear_signal();
	// add in 20120702 
	g_AppFullArb.timer_stop();

	s_tplist_tp_choise = 0;
	//get total sat num
	memset(&s_CurSat, 0, sizeof(s_CurSat));
	memset(&s_CurTp, 0, sizeof(s_CurTp));
	s_CurTpSel = -1;
	s_tplist_tp_total = 0;
	GUI_SetProperty("cmb_tp_list_sat","content","[NONE]");
	
	node_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	s_tplist_sat_total = node_num.node_num;
	if(s_tplist_sat_total > 0)
	{
		_tp_list_cmb_sat_rebuild(s_tplist_sat_total);

		//get current sat para
		s_CurSat.node_type = NODE_SAT;
		if(satsel.sel_in_all >= s_tplist_sat_total)
		{
			s_CurSat.pos = 0;
		}
		else
		{
			s_CurSat.pos = satsel.sel_in_all;
		}
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurSat);
	
		// get tp number
		node_num.node_type = NODE_TP;
		node_num.sat_id = s_CurSat.sat_data.id;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
		s_tplist_tp_total = node_num.node_num;
		if(s_tplist_tp_total > 0)
		{
			s_CurTpSel = 0;
			s_CurTp.node_type = NODE_TP;
			s_CurTp.pos = s_CurTpSel;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);
		}
		else
		{
			GUI_SetFocusWidget("cmb_tp_list_sat");
			_tp_list_hide_tip(TP_INFO_ADD);
		}

		GUI_SetProperty("cmb_tp_list_sat","select",&s_CurSat.pos);
		GUI_SetProperty("list_tp_list", "update_all", NULL);
		GUI_SetProperty("list_tp_list", "select", &s_CurTpSel);
	}

	

	// id manage
	s_TlIdOps = new_id_ops();
    	if (s_TlIdOps != NULL) 
    	{
		s_TlIdOps->id_open(s_TlIdOps, MANAGE_SAME_LIST, s_tplist_tp_total);	
	}

	// for clear the frontend mseeage queue
	GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
	
	//set tp
	_tp_list_set_diseqc();
	_tp_list_set_tp(SET_TP_FORCE);

	if (reset_timer(sp_TpListTimerSignal) != 0) 
	{
		sp_TpListTimerSignal = create_timer(timer_tp_list_show_signal, 100, NULL, TIMER_REPEAT);
	}
	return EVENT_TRANSFER_STOP;
}

static int app_tp_list_info_func(void)
{
	int SatSel = 0;
	int TpSel = 0;
    if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_antenna_setting"))
    {
        // step 1: check the mode of the antenna, tuner1 or tuner2
        if(app_tuner_sat_num_get(app_tuner_cur_tuner_get()) > 0)
        {
			GUI_GetProperty("cmb_tp_list_sat", "select", &SatSel);
			GUI_GetProperty("list_tp_list", "select", &TpSel);
            GUI_EndDialog("wnd_tp_list");
            app_antenna_setting_init(SatSel, TpSel, 0);
        }
        else
        {// TODO: need to notice , no match date to set
            PopDlg pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_NO_SAT;
            pop.title = NULL;
            pop.mode = POP_MODE_UNBLOCK;
            pop.creat_cb = NULL;
            pop.exit_cb = NULL;
            pop.timeout_sec = 3;
            popdlg_create(&pop);
        }
    }
    else
    {
		GUI_GetProperty("cmb_tp_list_sat", "select", &SatSel);
		GUI_GetProperty("list_tp_list", "select", &TpSel);
        GxMsgProperty_NodeByPosGet Sat = {0};
        Sat.node_type = NODE_SAT;
        Sat.pos = SatSel;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &Sat);

        app_tuner_cur_tuner_set(Sat.sat_data.tuner);
		timer_tp_list_show_signal_stop();
        app_antenna_setting_init(SatSel, TpSel, 0);
	    //GUI_EndDialog("wnd_tp_list");
    }
	return 0;
}

SIGNAL_HANDLER int app_tp_list_destroy(GuiWidget *widget, void *usrdata)
{
	uint32_t panel_led;
	
	timer_tp_list_show_signal_stop();
	
	remove_timer(sp_TpListTimerSignal);
	sp_TpListTimerSignal = NULL;

	remove_timer(sp_TpListLockTimer);
	sp_TpListLockTimer = NULL;

    del_id_ops(s_TlIdOps);
    s_TlIdOps = NULL;

    app_epg_info_clean();

       if (g_AppPlayOps.normal_play.play_total == 0)
     	{
            	panel_led = 0;
     	}
      	else
      	{
      		panel_led = g_AppPlayOps.normal_play.play_count+1;
      	}
    ioctl(bsp_panel_handle, PANEL_SHOW_NUM, &panel_led);

    // for clear the frontend mseeage queue
    GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
    return EVENT_TRANSFER_STOP;
}

#if (DLNA_SUPPORT > 0)
static void app_tplist_start_dlna_cb(void)
{
    timer_tp_list_show_signal_stop();
}

static void app_tplist_stop_dlna_cb(void)
{
    timer_tp_list_show_signal_reset();
}
#endif

SIGNAL_HANDLER int app_tp_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
#if (DLNA_SUPPORT > 0)
            case VK_DLNA_TRIGGER:
                app_dlna_ui_exec(app_tplist_start_dlna_cb, app_tplist_stop_dlna_cb);
                break;
#endif
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				break;

			case STBK_EXIT:
			case STBK_MENU:
				if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_antenna_setting"))
				{
					int num;
					// step 1: check the mode of the antenna, tuner1 or tuner2
					num = app_tuner_sat_num_get(app_tuner_cur_tuner_get());
					if(num == 0)
					{
						GUI_EndDialog("after wnd_main_menu");
						GUI_SetProperty("wnd_main_menu", "draw_now", NULL); 
#if (MINI_16BIT_WIN8_OSD_SUPPORT == 0)
						#if MV_WIN_SUPPORT
						//extern void app_main_item_menu_create(int value);
						//app_main_item_menu_create(0);
						#endif
#endif
						ret = EVENT_TRANSFER_STOP;
						break;
					}
					else if(num > 0)
					{
						if(s_tplist_sat_total > 0)
						{
							int Sel = 0, TpSel = 0;
							GUI_GetProperty("cmb_tp_list_sat", "select", &Sel);
							GUI_GetProperty("list_tp_list", "select", &TpSel);
							GUI_EndDialog("wnd_tp_list");
							app_antenna_setting_init(Sel, TpSel, 0);
							ret = EVENT_TRANSFER_STOP;
							break;
						}
					}
				}
                g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
				GUI_EndDialog("wnd_tp_list");
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_LEFT:
			case STBK_RIGHT:
				GUI_SendEvent("cmb_tp_list_sat", event);
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_GREEN:	//add
                {
				//zl
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
                    if(GXBUS_PM_SAT_S != s_CurSat.sat_data.type)
                        break;
                    //#endif
                    uint32_t tp_num = GxBus_PmTpNumGet();
                    if(tp_num >= SYS_MAX_TP)
                    {
                        PopDlg  pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_OK;
                        pop.str = STR_ID_MAX_TP;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.format = POP_FORMAT_DLG;
                        popdlg_create(&pop);
                    }
                    else
                    {
                        timer_tp_list_show_signal_stop();

                        memset(s_tplist_edit_input,'\0',6);

                        s_CurMode = TP_ADD;
                        _tp_list_hide_tip(TP_INFO_ALL);
                        GUI_CreateDialog("wnd_tp_list_popup");
                    }
                }
                ret = EVENT_TRANSFER_STOP;
                break;

			case STBK_YELLOW:	//edit
				//zl
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
				if(GXBUS_PM_SAT_S != s_CurSat.sat_data.type)
					break;
//#endif

				timer_tp_list_show_signal_stop();
				memset(s_tplist_edit_input,'\0',6);
				if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
				{
					s_CurMode = TP_EDIT;
					//tp_list_hide_help_info();
					_tp_list_hide_tip(TP_INFO_ALL);
					GUI_CreateDialog("wnd_tp_list_popup");
					ret = EVENT_TRANSFER_STOP;
				}
				break;

			case STBK_RED:		//del
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
				if(GXBUS_PM_SAT_S != s_CurSat.sat_data.type)
					break;
//#endif

				//timer_tp_list_show_signal_stop();//slove ##43507
	
				if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
				{
					s_CurMode = TP_DELETE;
					if((s_tplist_sat_total > 0) && (s_tplist_tp_total > 0))
					{
						uint32_t opt_num = 0;
						opt_num = s_TlIdOps->id_total_get(s_TlIdOps);
						if(opt_num == 0)
						{
							s_TlIdOps->id_add(s_TlIdOps, s_CurTpSel, s_CurTp.tp_data.id);
							GUI_SetProperty("list_tp_list", "update_row", &s_CurTpSel);
						}
					}	
					_tp_list_hide_tip(TP_INFO_ALL);
					GUI_CreateDialog("wnd_tp_list_popup");
					ret = EVENT_TRANSFER_STOP;
				}
				break;

			case STBK_BLUE:		//search
			{
				#if TKGS_SUPPORT
				if(app_tkgs_get_operatemode() == GX_TKGS_AUTOMATIC)
				{
					PopDlg  pop;
					memset(&pop, 0, sizeof(PopDlg));
					pop.type = POP_TYPE_OK;
					pop.str = STR_ID_TKGS_CANT_WORK;
					pop.mode = POP_MODE_UNBLOCK;
					pop.format = POP_FORMAT_DLG;
					popdlg_create(&pop);
					ret = EVENT_TRANSFER_STOP;
					break;
					
				}
				#endif
				if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
				{
					SearchIdSet search_id;
					uint32_t opt_num = 0;

					if((s_tplist_sat_total > 0) && (s_tplist_tp_total > 0))
					{
						opt_num = s_TlIdOps->id_total_get(s_TlIdOps);
						if(opt_num == 0)
						{
							s_TlIdOps->id_add(s_TlIdOps, s_CurTpSel, s_CurTp.tp_data.id);
							GUI_SetProperty("list_tp_list", "update_row", &s_CurTpSel);
							search_id.id_num = 1;
						}
						else
						{
							search_id.id_num = opt_num;
						}
					
						search_id.id_array = (uint32_t *)GxCore_Malloc(search_id.id_num * sizeof(uint32_t));
						if(search_id.id_array != NULL)
						{
							s_TlIdOps->id_get(s_TlIdOps, (uint32_t *)search_id.id_array);
						
							_tp_list_hide_tip(TP_INFO_ALL);
							if(app_search_opt_dlg(SEARCH_TP, &search_id) == WND_OK)
							{
								GUI_SetFocusWidget("list_tp_list");
								timer_tp_list_show_signal_stop();
								s_CurMode = TP_SEARCH;
								app_search_start(0);
							}
							
							GxCore_Free(search_id.id_array);
							_tp_list_show_tip();
						}
					}
					else
					{
						s_CurMode = TP_SEARCH;
						GUI_CreateDialog("wnd_tp_list_popup"); //tp not exsit
					}
					ret = EVENT_TRANSFER_STOP;
				}
				break;
			}
	        case STBK_PAGE_UP:
			case STBK_PAGE_DOWN:
			{
				ret = EVENT_TRANSFER_STOP;
			}
			break;
			// add in 20131027
			case STBK_INFO:
				if(s_tplist_sat_total > 0)
					app_tp_list_info_func();
#if 0
				if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_antenna_setting"))
				{
					// need to check
					int SatSel = 0;
					int TpSel = 0;

					GUI_GetProperty("cmb_tp_list_sat", "select", &SatSel);
					GUI_GetProperty("list_tp_list", "select", &TpSel);
					GUI_EndDialog("wnd_tp_list");
					app_antenna_setting_init(SatSel, TpSel,0);
				}
				else
				{
					int SatSel = 0;
					int TpSel = 0;

					GUI_GetProperty("cmb_tp_list_sat", "select", &SatSel);
					GUI_GetProperty("list_tp_list", "select", &TpSel);
					timer_tp_list_show_signal_stop();
					app_antenna_setting_init(SatSel, TpSel,0);
				}
#endif
				break;

			default:
				break;
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_tp_list_lost_focus(GuiWidget *widget, void *usrdata)
{
	if((s_tplist_sat_total > 0) && (s_tplist_tp_total > 0))
	{
		if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
		{
			GUI_SetProperty("list_tp_list", "active", &s_CurTpSel);
			GUI_SetFocusWidget("list_tp_list");
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tp_list_got_focus(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_NodeNumGet node_num = {0};
	int sel =-1;
	int act_sel= -1;

	GUI_SetProperty("list_tp_list", "active", &act_sel);

	sel = s_CurTpSel;
	
	if(TP_EDIT == s_CurMode && TP_CHANGED == s_Changed)//edit tp and changed
	{
		GUI_SetProperty("list_tp_list", "update_row", &sel);
		s_CurTp.node_type = NODE_TP;
		s_CurTp.pos = sel;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);
	}
	else if(TP_ADD == s_CurMode && TP_CHANGED == s_Changed)//add tp
	{
		// get total tp number
		node_num.node_type = NODE_TP;
		node_num.sat_id = s_CurSat.sat_data.id;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
		s_tplist_tp_total = node_num.node_num;
		if(s_tplist_tp_total > 0)
		{
			sel = s_tplist_tp_total - 1;
			s_CurTp.node_type = NODE_TP;
			s_CurTp.pos = sel;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);
			if(s_TlIdOps != NULL)
			{
				s_TlIdOps->id_close(s_TlIdOps);
				s_TlIdOps->id_open(s_TlIdOps, MANAGE_SAME_LIST, s_tplist_tp_total);
			}
		}
		else
		{
			sel =-1;
		}

		GUI_SetProperty("list_tp_list", "update_all", NULL);
		GUI_SetProperty("list_tp_list", "select", &sel);
	}
	else if(TP_DELETE == s_CurMode && TP_CHANGED == s_Changed)	//del tp
	{
		int i;
		int del_count = 0;
		// get total tp number
		node_num.node_type = NODE_TP;
		node_num.sat_id = s_CurSat.sat_data.id;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
		s_tplist_tp_total = node_num.node_num;
		if(s_tplist_tp_total > 0)
		{
			if(s_TlIdOps->id_total_get(s_TlIdOps) > 0)
			{
				del_count = 0;
	            		for(i = 0; i < s_tplist_tp_total + s_TlIdOps->id_total_get(s_TlIdOps); i++)
	            		{
	            			if(true == s_TlIdOps->id_check(s_TlIdOps, i))
	            			{
	            				del_count++;
	            				continue;
	            			}

	            			if(i >= sel)
	            			{
	            				break;
	            			}
	            		}
	            		sel = i - del_count;
	            	}

	            	if(sel >= s_tplist_tp_total)
			{
				sel = s_tplist_tp_total - 1;
			}

	            	s_CurTp.node_type = NODE_TP;
			s_CurTp.pos = sel;
			
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);
		}
		else
		{
			sel = -1;
		}
		if(s_TlIdOps != NULL)
		{
			s_TlIdOps->id_close(s_TlIdOps);
			s_TlIdOps->id_open(s_TlIdOps, MANAGE_SAME_LIST, s_tplist_tp_total);
		}
        GUI_SetProperty("list_tp_list", "update_all", NULL);
		GUI_SetProperty("list_tp_list", "select", &sel);
	}
	else if(TP_SEARCH== s_CurMode)
	{
		int i;
		SearchIdSet search_id = {0};
		GxMsgProperty_NodeByPosGet node = {0};
		node_num.node_type = NODE_TP;
		node_num.sat_id = s_CurSat.sat_data.id;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
		if(s_tplist_tp_total != node_num.node_num)
		{
		s_tplist_tp_total = node_num.node_num;
		if(s_tplist_tp_total > 0)
		{
				//sel = s_tplist_tp_total - 1;
				//s_CurTp.node_type = NODE_TP;
				//s_CurTp.pos = sel;
				//app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);
			search_id.id_num = s_TlIdOps->id_total_get(s_TlIdOps);
			if(search_id.id_num > 0)
			{
				search_id.id_array = (uint32_t *)GxCore_Malloc(search_id.id_num * sizeof(uint32_t));
				if(search_id.id_array != NULL)
				{
				//	s_TlIdOps->id_get(s_TlIdOps, (uint32_t *)search_id.id_array);								
				//	GxCore_Free(search_id.id_array);
					search_id.id_num = 0;
		            		for(i = 0; i < s_tplist_tp_total; i++)
		            		{
		            			if(true == s_TlIdOps->id_check(s_TlIdOps, i))
		            			{
		            				search_id.id_array[search_id.id_num] = i;
							search_id.id_num ++;
		            			}
		            		}				
				}
			}
				
			if(s_TlIdOps != NULL)
			{
				s_TlIdOps->id_close(s_TlIdOps);
				s_TlIdOps->id_open(s_TlIdOps, MANAGE_SAME_LIST, s_tplist_tp_total);
				}

				if(search_id.id_num > 0)
				{
					for(i = 0; i < search_id.id_num; i++)
					{
						node.node_type = NODE_TP;
						node.pos = search_id.id_array[i];
						app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
						s_TlIdOps->id_add(s_TlIdOps, search_id.id_array[i], node.tp_data.id);	
					}
					if(search_id.id_array != NULL)
						GxCore_Free(search_id.id_array);
				}
			}
		}
		
    }
	else
	{
		GUI_SetProperty("list_tp_list", "update_all", NULL);
		GUI_SetProperty("list_tp_list", "select", &sel);
	}
	if(reset_timer(sp_TpListLockTimer) != 0) 
	{
		sp_TpListLockTimer = create_timer(timer_tp_list_set_frontend, FRONTEND_SET_DELAY_MS, NULL, TIMER_ONCE);
	}	
//EXIT:
    s_CurTpSel = sel;
    //clear
    s_CurMode=0;
    s_Changed=0;

    timer_tp_list_show_signal_reset();

    if(1 == match_status)
    {
        int i;
        static GxMsgProperty_NodeByPosGet node_tp;

        for(i=0;i<s_tplist_tp_total;i++)
        {
            node_tp.node_type = NODE_TP;
            node_tp.pos = i;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
            if(node_tp.tp_data.id == match_exist_sel)
            {
                if((0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
					||(0 == strcasecmp("btn_tplist_tip_ok_exist", GUI_GetFocusWidget())))
                {
                	GUI_SetProperty("list_tp_list", "select", &i);
                }
                return EVENT_TRANSFER_STOP;
            }
        }
    }
    return EVENT_TRANSFER_STOP;
}

#define TP_LISTVIEW
SIGNAL_HANDLER int app_tp_list_list_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tp_list_list_change(GuiWidget *widget, void *usrdata)
{
	uint32_t list_sel;

	GUI_GetProperty("list_tp_list", "select", &list_sel);
	s_CurTpSel = list_sel;

	s_CurTp.node_type = NODE_TP;
	s_CurTp.pos = s_CurTpSel;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);

	if(reset_timer(sp_TpListLockTimer) != 0) 
	{
		sp_TpListLockTimer = create_timer(timer_tp_list_set_frontend, FRONTEND_SET_DELAY_MS, NULL, TIMER_ONCE);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tp_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	static GxMsgProperty_NodeByPosGet node;
	GxMsgProperty_NodeNumGet node_num = {0};

	if(s_tplist_sat_total > 0)
	{
		node.node_type = NODE_SAT;
		node.pos = s_CurSat.pos;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
	// get tp number
		node_num.node_type = NODE_TP;
		node_num.sat_id = node.sat_data.id;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	}
	s_tplist_tp_total = node_num.node_num;
	return node_num.node_num;
}

SIGNAL_HANDLER int app_tp_list_list_get_data(GuiWidget *widget, void *usrdata)
{
#define MAX_LEN	30
	ListItemPara* item = NULL;
	static GxMsgProperty_NodeByPosGet node;
	static char buffer1[MAX_LEN];
	static char buffer2[MAX_LEN];
	static char buffer3[MAX_LEN];
	static char buffer4[MAX_LEN];

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	//col-0: choice
	item->x_offset = 0;
	if (s_TlIdOps->id_check(s_TlIdOps, item->sel) == 0)//ID_UNEXIST
	{
		item->image = NULL;
	}
	else 
	{
		item->image = "s_choice.bmp";
	}
	item->string = NULL;

	//col-1: num
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	memset(buffer1, 0, MAX_LEN);
	sprintf(buffer1, " %d", (item->sel+1));
	item->string = buffer1;

	//get tp node
	node.node_type = NODE_TP;
	node.pos = item->sel;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
	
	//col-1: freq	
	GxMsgProperty_NodeByPosGet node_sat;
	node_sat.node_type = NODE_SAT;
	node_sat.pos = s_CurSat.pos;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
#if (DEMOD_DVB_T > 0)
	if(GXBUS_PM_SAT_DVBT2 == node_sat.sat_data.type)
	{
		char* bandwidth_str[3] = {"8M", "7M","6M"};
		item = item->next;//col-1: freq
		item->x_offset = 0;
		item->image = NULL;	
		memset(buffer2, 0, MAX_LEN);
		sprintf(buffer2,"%d.%d",node.tp_data.frequency/1000, (node.tp_data.frequency/100)%10);	
		item->string = buffer2;
		
		item = item->next;//col-1: polar
		item = item->next;//col-1: symb
		item->x_offset = 0;
		item->image = NULL;			
		memset(buffer4, 0, MAX_LEN);
		if(node.tp_data.tp_dvbt2.bandwidth <BANDWIDTH_AUTO )	
		strcpy(buffer4,bandwidth_str[node.tp_data.tp_dvbt2.bandwidth]);
		item->string = buffer4;
	}else 
#endif
#if (DEMOD_DVB_C > 0)
	if(GXBUS_PM_SAT_C == node_sat.sat_data.type)
	{
		char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};
		item = item->next;//col-1: freq
		item->x_offset = 0;
		item->image = NULL;	
		memset(buffer2, 0, MAX_LEN);
		sprintf(buffer2,"%d",node.tp_data.frequency/1000);	
		item->string = buffer2;
		
		item = item->next;//col-1: polar		
		item->x_offset = 0;
		item->image = NULL;			
		memset(buffer3, 0, MAX_LEN);
		if(node.tp_data.tp_c.modulation <QAM_AUTO )	
		strcpy(buffer3,modulation_str[node.tp_data.tp_c.modulation]);
		item->string = buffer3;

		item = item->next;//col-1: symb
		item->x_offset = 0;
		item->image = NULL;	
		memset(buffer4, 0, MAX_LEN);		
		sprintf(buffer4,"%d",node.tp_data.tp_c.symbol_rate/1000);
		item->string = buffer4;
	}else
#endif
#if (DEMOD_DTMB > 0)
	if(GXBUS_PM_SAT_DTMB == node_sat.sat_data.type)
	{
		if(GXBUS_PM_SAT_1501_DTMB == node_sat.sat_data.sat_dtmb.work_mode)
		{
			char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};// have 7M?
			item = item->next;//col-1: freq
			item->x_offset = 0;
			item->image = NULL;	
			memset(buffer2, 0, MAX_LEN);
			sprintf(buffer2,"%d.%d",node.tp_data.frequency/1000, (node.tp_data.frequency/100)%10);	
			item->string = buffer2;
			
			item = item->next;//col-1: polar
			item = item->next;//col-1: symb
			item->x_offset = 0;
			item->image = NULL;			
			memset(buffer4, 0, MAX_LEN);
			if(node.tp_data.tp_dtmb.symbol_rate < BANDWIDTH_AUTO )	
			strcpy(buffer4,bandwidth_str[node.tp_data.tp_dtmb.symbol_rate]);
			item->string = buffer4;
		}
		else
		{
			char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};
			item = item->next;//col-1: freq
			item->x_offset = 0;
			item->image = NULL;	
			memset(buffer2, 0, MAX_LEN);
			sprintf(buffer2,"%d",node.tp_data.frequency/1000);	
			item->string = buffer2;
			
			item = item->next;//col-1: polar		
			item->x_offset = 0;
			item->image = NULL;			
			memset(buffer3, 0, MAX_LEN);
			if(node.tp_data.tp_dtmb.modulation <QAM_AUTO )	
			strcpy(buffer3,modulation_str[node.tp_data.tp_dtmb.modulation]);
			item->string = buffer3;

			item = item->next;//col-1: symb
			item->x_offset = 0;
			item->image = NULL;	
			memset(buffer4, 0, MAX_LEN);		
			sprintf(buffer4,"%d",node.tp_data.tp_dtmb.symbol_rate/1000);
			item->string = buffer4;
		}
	}else
#endif
	{			
		//col-1: freq
		item = item->next;
		item->x_offset = 0;
		item->image = NULL;
		memset(buffer2, 0, MAX_LEN);
		if(node.tp_data.frequency > 99999)
		{
			sprintf(buffer2,"%05d",node.tp_data.frequency);
		}
		else
		{
			sprintf(buffer2,"%d",node.tp_data.frequency);
		}
		item->string = buffer2;
			
		
		//col-1: symb
		item = item->next;
		item->x_offset = 0;
		item->image = NULL;
		memset(buffer4, 0, MAX_LEN);
		if(node.tp_data.tp_s.symbol_rate > 99999)
		{
			sprintf(buffer4,"%05d",node.tp_data.tp_s.symbol_rate);
		}
		else
		{
			sprintf(buffer4,"%d",node.tp_data.tp_s.symbol_rate);
		}
		
		item->string = buffer4;

		//col-1: polar
		item = item->next;
		item->x_offset = 0;
		item->image = NULL;
		memset(buffer3, 0, MAX_LEN);
		if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
		{
			sprintf(buffer3,"%s", "V");
		}
		else if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
		{
			sprintf(buffer3,"%s", "H");
		}
		item->string = buffer3;

	}

	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_tp_list_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	//uint32_t sel=0;
	int cur_sel = 0;
	
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
					//GUI_GetProperty("list_tp_list", "select", &sel);
					if((s_tplist_sat_total > 0) && (s_tplist_tp_total > 0))
					{
						if (s_TlIdOps->id_check(s_TlIdOps, s_CurTpSel) == 0)//ID_UNEXIST
						{
							s_TlIdOps->id_add(s_TlIdOps, s_CurTpSel, s_CurTp.tp_data.id);
							s_tplist_tp_choise++;
						}
						else
						{
							s_TlIdOps->id_delete(s_TlIdOps, s_CurTpSel);
							s_tplist_tp_choise--;
						}
						GUI_SetProperty("list_tp_list", "update_row", &s_CurTpSel);
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				}
				case STBK_0:
				{
					if((s_tplist_sat_total > 0) && (s_tplist_tp_total > 0))
					{
						uint16_t i = 0;
						GxMsgProperty_NodeByPosGet node = {0};

						if(s_tplist_tp_choise < s_tplist_tp_total)
						{
							for(i = 0; i < s_tplist_tp_total; i++)
							{
								node.node_type = NODE_TP;
								node.pos = i;
								app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
								s_TlIdOps->id_add(s_TlIdOps, i, node.tp_data.id);	
							}
							s_tplist_tp_choise = s_tplist_tp_total;
						}
						else
						{
							for(i = 0; i < s_tplist_tp_total; i++)
							{
								s_TlIdOps->id_delete(s_TlIdOps, i);
							}
							s_tplist_tp_choise = 0;
						}
						GUI_SetProperty("list_tp_list", "update", NULL);
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				}
				break;
				case STBK_UP:
                memset(s_tplist_edit_input,'\0',6);
                
					GUI_GetProperty("list_tp_list", "select", &cur_sel);
					if(cur_sel == 0)
					{
						GUI_SetFocusWidget("cmb_tp_list_sat");
						_tp_list_hide_tip(TP_INFO_ADD);
						ret = EVENT_TRANSFER_STOP;
					}
					break;
				case STBK_DOWN:
                memset(s_tplist_edit_input,'\0',6);
                
					GUI_GetProperty("list_tp_list", "select", &cur_sel);
					if(cur_sel == (s_tplist_tp_total - 1))
					{
						GUI_SetFocusWidget("cmb_tp_list_sat");
						_tp_list_hide_tip(TP_INFO_ADD);
						ret = EVENT_TRANSFER_STOP;
					}
                    break;
                case STBK_PAGE_UP:
                    {
                        uint32_t sel;
                        GUI_GetProperty("list_tp_list", "select", &sel);
                        if (sel == 0)
                        {
                            sel = app_tp_list_list_get_total(NULL,NULL)-1;
                            GUI_SetProperty("list_tp_list", "select", &sel);
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
                        GUI_GetProperty("list_tp_list", "select", &sel);
                        if (app_tp_list_list_get_total(NULL,NULL)
                                == sel+1)
                        {
                            sel = 0;
                            GUI_SetProperty("list_tp_list", "select", &sel);
                            ret =  EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            ret =  EVENT_TRANSFER_KEEPON;
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

#define CMB_SAT
SIGNAL_HANDLER int app_tp_list_cmb_sat_change(GuiWidget *widget, void *usrdata)
{
    uint32_t box_sel = 0;
    uint32_t list_sel = -1;
	GxMsgProperty_NodeNumGet node_num = {0};

	if(s_tplist_sat_total == 0)
		return EVENT_TRANSFER_STOP;
		
	GUI_GetProperty("cmb_tp_list_sat", "select", &box_sel);
	s_tplist_tp_choise = 0;
	
	//get current sat para
	s_CurSat.node_type = NODE_SAT;
	s_CurSat.pos = box_sel;
    app_sat_list_change_sel(s_CurSat.pos);
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurSat);

	// get tp number
	list_sel = -1;
	memset(&s_CurTp, 0, sizeof(s_CurTp));
	node_num.node_type = NODE_TP;
	node_num.sat_id = s_CurSat.sat_data.id;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	s_tplist_tp_total = node_num.node_num;
	if(s_tplist_tp_total > 0)
	{
	// get tp para
		list_sel = 0;
		s_CurTp.node_type = NODE_TP;
		s_CurTp.pos = list_sel;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);
		GUI_SetProperty("list_tp_list", "update_all", NULL);
		GUI_SetProperty("list_tp_list", "select", &list_sel);
	}
	else
	{	
		GUI_SetProperty("list_tp_list", "update_all", NULL);
		GUI_SetProperty("list_tp_list", "select", &list_sel);
		GUI_SetFocusWidget("cmb_tp_list_sat");
		_tp_list_hide_tip(TP_INFO_ADD);
	}
	
	s_CurTpSel = list_sel;

	if(s_TlIdOps != NULL)
	{	
		s_TlIdOps->id_close(s_TlIdOps);
		s_TlIdOps->id_open(s_TlIdOps, MANAGE_SAME_LIST, s_tplist_tp_total);
	}
	
	if(reset_timer(sp_TpListLockTimer) != 0) 
	{
		sp_TpListLockTimer = create_timer(timer_tp_list_set_frontend, FRONTEND_SET_DELAY_MS, NULL, TIMER_ONCE);
	}

	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_tp_list_cmb_sat_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	int cmb_sel = 0;
	int cur_sel = 0;
	
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
				case STBK_OK:
					GUI_GetProperty("cmb_tp_list_sat", "select", &cmb_sel);
                    GUI_SetProperty("img_tp_list_tip_green", "state", "hide");
					GUI_SetProperty("text_tp_list_tip_green", "state", "hide");
					GUI_SetProperty("img_tp_list_tip_info", "state", "hide");
					GUI_SetProperty("text_tp_list_tip_info", "state", "hide");
					cmb_sel = _tp_list_sat_pop(cmb_sel);
					GUI_SetProperty("cmb_tp_list_sat", "select", &cmb_sel);
				    break;
				case STBK_UP:
					if(s_tplist_tp_total > 0)
					{
						cur_sel = s_tplist_tp_total - 1;
						GUI_SetFocusWidget("list_tp_list");
						GUI_SetProperty("list_tp_list", "select", &cur_sel);
						_tp_list_show_tip();
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				case STBK_DOWN:
					if(s_tplist_tp_total > 0)
					{
						cur_sel = 0;
						GUI_SetFocusWidget("list_tp_list");
						GUI_SetProperty("list_tp_list", "select", &cur_sel);
						_tp_list_show_tip();
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				
				case STBK_GREEN: //add tp
					break;

				case STBK_RED:
				case STBK_YELLOW:
				case STBK_BLUE:
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


#define TP_LIST_POPUP
static bool _check_tp_para_valid(GxBusPmDataSat *p_sat, TpListTpPara *tp_para)
{
    if((p_sat == NULL) || (tp_para == NULL))
        return false;

    if((tp_para->symbol_rate >= 800) && (tp_para->symbol_rate <= 46000))
    {
        uint32_t tp_frequency = 0;
        GxBusPmDataTP temp_tp = {0};
        temp_tp.frequency= tp_para->frequency;
        temp_tp.tp_s.polar = tp_para->polar;
        tp_frequency = app_show_to_tp_freq(p_sat, &temp_tp);
        if((tp_frequency >= 915) && (tp_frequency <= 2185))
        {
            return true;
        }
    }

    return false;
}
static bool _check_sym_para_valid(void)
{
	uint32_t symbol_rate;
	char *buffer_sym = NULL;
	
	GUI_GetProperty("text_tplist_popup_sym2", "string", &buffer_sym);
	symbol_rate = atoi(buffer_sym);
	printf("symbol_rate = %d \n",symbol_rate);
    if((symbol_rate >= 800) && (symbol_rate <= 46000))
    {
       return true;
    }

    return false;
}

static status_t _tplist_tp_edit(void)
{
#define TP_LEN	20

    GxMsgProperty_NodeModify NodeModify;
    char *atoi_buf = NULL;
    uint32_t sel=0;
    TpListTpPara InputTpPara={0};
    char buffer[TP_LEN] = {0};
    status_t ret = GXCORE_ERROR;

    //get input value
    //freq
    GUI_GetProperty("text_tplist_popup_freq2", "string", &atoi_buf);
    InputTpPara.frequency = atoi(atoi_buf);
    //sym
    GUI_GetProperty("text_tplist_popup_sym2", "string", &atoi_buf);
    InputTpPara.symbol_rate = atoi(atoi_buf);
    GUI_GetProperty("cmb_tplist_popup_polar", "select", &sel);

    if((InputTpPara.symbol_rate == 0) && (InputTpPara.frequency != 0))
        return ret;


    if(0 == sel)
        InputTpPara.polar = GXBUS_PM_TP_POLAR_H;
    else
        InputTpPara.polar = GXBUS_PM_TP_POLAR_V;

    if(false == _check_tp_para_valid(&s_CurSat.sat_data, &InputTpPara))
    {//show: invalid TP value

        GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
        GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
        memset(s_tplist_edit_input,'\0',6);

        //hide the box
        GUI_SetProperty("box_tplist_popup_para", "state", "hide");
        //show info
        GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_INVALID_TP);

        GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);

        GxCore_ThreadDelay(1200);

        memset(buffer, 0 ,TP_LEN);
        sprintf(buffer, "%05d", s_CurTp.tp_data.frequency);
        GUI_SetProperty("edit_tplist_popup_freq", "string", buffer);
        GUI_SetProperty("text_tplist_popup_freq2", "string", buffer);
        //polar
        sel = s_CurTp.tp_data.tp_s.polar;
        GUI_SetProperty("cmb_tplist_popup_polar", "select", &sel);
        //sym
        memset(buffer, 0 ,TP_LEN);
        sprintf(buffer, "%05d", s_CurTp.tp_data.tp_s.symbol_rate);
        GUI_SetProperty("edit_tplist_popup_sym", "string", buffer);
        GUI_SetProperty("text_tplist_popup_sym2", "string", buffer);

        //resume
        GUI_SetProperty("text_tplist_popup_tip1", "string", " ");
        //show the box
        GUI_SetProperty("box_tplist_popup_para", "state", "show");
        GUI_SetFocusWidget("box_tplist_popup_para");

        GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
        GUI_SetProperty("text_tplist_popup_freq2", "state", "show");
        GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
        GUI_SetProperty("text_tplist_popup_sym2", "state", "show");
        GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);
    }
    else//value is ok, check if the tp is exist
    {
        uint16_t tp_id;
        if(0 != GxBus_PmTpExistChek(s_CurTp.tp_data.sat_id, InputTpPara.frequency,
                    InputTpPara.symbol_rate, InputTpPara.polar, QPSK, &tp_id))
        {//tp already exist
            //hide the box
            match_status = 1;
            match_exist_sel = tp_id;

            GUI_SetProperty("box_tplist_popup_para", "state", "hide");
            //show info
            GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_TP_EXIST);

            GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);

            //zl
            GUI_SetProperty("text_tplist_popup_tip1_exist", "state", "show");
            GUI_SetProperty("btn_tplist_tip_cancel_exist", "state", "show");
            GUI_SetProperty("btn_tplist_tip_ok_exist", "state", "show");
            GUI_SetProperty("img_tplist_tip_opt_exist", "state", "show");

            GUI_SetFocusWidget("btn_tplist_tip_ok_exist");
            GUI_SetProperty("img_tplist_tip_opt_exist","img","s_bar_choice_blue4.bmp");
            tp_exist_status = 1;
        }
        else//modify
        {
            match_status = 0;

            NodeModify.node_type = NODE_TP;
            NodeModify.tp_data = s_CurTp.tp_data;
            NodeModify.tp_data.frequency = InputTpPara.frequency;
            NodeModify.tp_data.tp_s.symbol_rate = InputTpPara.symbol_rate;
            NodeModify.tp_data.tp_s.polar = InputTpPara.polar;
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &NodeModify))
            {
                int SatSel = 0;
                GUI_GetProperty("cmb_tp_list_sat", "select", &SatSel);
                app_antenna_tp_change_set(SatSel);
                s_Changed = TP_CHANGED;
                ret = GXCORE_SUCCESS;
            }
        }
    }

    return ret;
}


static status_t _tplist_check_sym_add(void)
{
 
	uint32_t sel=0;
    status_t ret = GXCORE_ERROR;

	if(false == _check_sym_para_valid())
	{//show: invalid TP value
		//hide the box
        
		GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
                GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
                memset(s_tplist_edit_input,'\0',6);
		
		GUI_SetProperty("box_tplist_popup_para", "state", "hide");
		//show info
		GUI_SetProperty("text_tplist_popup_tip1", "string", "Invalid symbol !");
		
		GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);

		GxCore_ThreadDelay(1200);

		GUI_SetProperty("edit_tplist_popup_freq", "string", buffer_fre_bak);
		GUI_SetProperty("text_tplist_popup_freq2", "string", buffer_fre_bak);
		//polar
		sel = 0;
		GUI_SetProperty("cmb_tplist_popup_polar", "select", &sel);
		//sym
		GUI_SetProperty("edit_tplist_popup_sym", "string", buffer_sym_bak);
		GUI_SetProperty("text_tplist_popup_sym2", "string", buffer_sym_bak);

		//resume
		GUI_SetProperty("text_tplist_popup_tip1", "string", " ");
		//show the box
		GUI_SetProperty("box_tplist_popup_para", "state", "show");
		GUI_SetFocusWidget("box_tplist_popup_para");

		GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
		GUI_SetProperty("text_tplist_popup_freq2", "state", "show");
		GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
		GUI_SetProperty("text_tplist_popup_sym2", "state", "show");
		GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);
	}
	
    return ret ;
}


static status_t _tplist_check_tp_add(void)
{
    char *atoi_buf = NULL;
    uint32_t sel=0;
    TpListTpPara InputTpPara={0};
    status_t ret = GXCORE_ERROR;

    //get input value
    //freq
    GUI_GetProperty("text_tplist_popup_freq2", "string", &atoi_buf);
    InputTpPara.frequency = atoi(atoi_buf);
    //sym
	GUI_GetProperty("text_tplist_popup_sym2", "string", &atoi_buf);
	InputTpPara.symbol_rate = atoi(atoi_buf);
	GUI_GetProperty("cmb_tplist_popup_polar", "select", &sel);
	if(0 == sel)
		InputTpPara.polar = GXBUS_PM_TP_POLAR_H;
	else
		InputTpPara.polar = GXBUS_PM_TP_POLAR_V;
		
	if(false == _check_tp_para_valid(&s_CurSat.sat_data, &InputTpPara))
	{//show: invalid TP value
		//hide the box   
		GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
                GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
                memset(s_tplist_edit_input,'\0',6);
		
		GUI_SetProperty("box_tplist_popup_para", "state", "hide");
		//show info
		GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_INVALID_TP);
		
		GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);

		GxCore_ThreadDelay(1200);

		GUI_SetProperty("edit_tplist_popup_freq", "string", buffer_fre_bak);
		GUI_SetProperty("text_tplist_popup_freq2", "string", buffer_fre_bak);
		//polar
		//sel = s_CurTp.tp_data.tp_s.polar;
		//GUI_SetProperty("cmb_tplist_popup_polar", "select", &sel);
		//sym
		GUI_SetProperty("edit_tplist_popup_sym", "string", buffer_sym_bak);
		GUI_SetProperty("text_tplist_popup_sym2", "string", buffer_sym_bak);

		//resume
		GUI_SetProperty("text_tplist_popup_tip1", "string", " ");
		//show the box
		GUI_SetProperty("box_tplist_popup_para", "state", "show");
		GUI_SetFocusWidget("box_tplist_popup_para");

		GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
		GUI_SetProperty("text_tplist_popup_freq2", "state", "show");
		GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
		GUI_SetProperty("text_tplist_popup_sym2", "state", "show");
		GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);      
	}

    return ret ;
}


static status_t _tplist_tp_add(void)
{
    GxMsgProperty_NodeAdd NodeAdd;
    char *atoi_buf = NULL;
    uint16_t tp_id;
    uint32_t sel=0;
    TpListTpPara InputTpPara={0};
    status_t ret = GXCORE_ERROR;

    //get input value
    //freq
    GUI_GetProperty("text_tplist_popup_freq2", "string", &atoi_buf);
    InputTpPara.frequency = atoi(atoi_buf);
    //sym
	GUI_GetProperty("text_tplist_popup_sym2", "string", &atoi_buf);
	InputTpPara.symbol_rate = atoi(atoi_buf);
	GUI_GetProperty("cmb_tplist_popup_polar", "select", &sel);
	if(0 == sel)
		InputTpPara.polar = GXBUS_PM_TP_POLAR_H;
	else
		InputTpPara.polar = GXBUS_PM_TP_POLAR_V;
		
	if(false == _check_tp_para_valid(&s_CurSat.sat_data, &InputTpPara))
	{//show: invalid TP value
		//hide the box
        
		GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
                GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
                memset(s_tplist_edit_input,'\0',6);
		
		GUI_SetProperty("box_tplist_popup_para", "state", "hide");
		//show info
		GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_INVALID_TP);
		
		GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);

		GxCore_ThreadDelay(1200);

		GUI_SetProperty("edit_tplist_popup_freq", "string", buffer_fre_bak);
		GUI_SetProperty("text_tplist_popup_freq2", "string", buffer_fre_bak);
		//polar
		sel = 0;
		GUI_SetProperty("cmb_tplist_popup_polar", "select", &sel);
		//sym
		GUI_SetProperty("edit_tplist_popup_sym", "string", buffer_sym_bak);
		GUI_SetProperty("text_tplist_popup_sym2", "string", buffer_sym_bak);

		//resume
		GUI_SetProperty("text_tplist_popup_tip1", "string", " ");
		//show the box
		GUI_SetProperty("box_tplist_popup_para", "state", "show");
		GUI_SetFocusWidget("box_tplist_popup_para");

		GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
		GUI_SetProperty("text_tplist_popup_freq2", "state", "show");
		GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
		GUI_SetProperty("text_tplist_popup_sym2", "state", "show");
		GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);

        
	}
	else//value is ok, check if the tp is exist
	{
		
		if(0 != GxBus_PmTpExistChek(s_CurSat.sat_data.id, InputTpPara.frequency, 
			InputTpPara.symbol_rate, InputTpPara.polar, QPSK, &tp_id))
		{//tp already exist
			//hide the box
			//zl
			match_status = 1;
			match_exist_sel = tp_id;
			
			
			GUI_SetProperty("box_tplist_popup_para", "state", "hide");
			//show info
			GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_TP_EXIST);
			
			GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);

			//zl
                        GUI_SetProperty("text_tplist_popup_tip1_exist", "state", "show");
                        GUI_SetProperty("btn_tplist_tip_cancel_exist", "state", "show");
                        GUI_SetProperty("btn_tplist_tip_ok_exist", "state", "show");
                        GUI_SetProperty("img_tplist_tip_opt_exist", "state", "show");

                        GUI_SetFocusWidget("btn_tplist_tip_ok_exist");
                        GUI_SetProperty("img_tplist_tip_opt_exist","img","s_bar_choice_blue4.bmp");
                        tp_exist_status = 1;
	
		}
		else//save
		{
			match_status = 0;
		
			memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));
			NodeAdd.node_type = NODE_TP;
			NodeAdd.tp_data.sat_id = s_CurSat.sat_data.id;
			NodeAdd.tp_data.frequency = InputTpPara.frequency;
			NodeAdd.tp_data.tp_s.symbol_rate = InputTpPara.symbol_rate;
			NodeAdd.tp_data.tp_s.polar = InputTpPara.polar;

			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
			{
				int SatSel = 0;
				GUI_GetProperty("cmb_tp_list_sat", "select", &SatSel);
				app_antenna_tp_change_set(SatSel);
				s_Changed = TP_CHANGED;
				_tp_list_show_tip();
				
				ret = GXCORE_SUCCESS;
			}	
		}
	}
	
    return ret ;
}

static status_t _tplist_tp_del(void)
{
    GxMsgProperty_NodeDelete NodeDel;
    status_t ret = GXCORE_ERROR;

	NodeDel.node_type = NODE_TP;
	NodeDel.num = s_TlIdOps->id_total_get(s_TlIdOps);
	if(NodeDel.num > 0)
	{
		NodeDel.id_array = GxCore_Malloc(NodeDel.num*sizeof(uint32_t));
		if(NodeDel.id_array != NULL)
		{
			s_TlIdOps->id_get(s_TlIdOps, (uint32_t*)(NodeDel.id_array));
			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_DELETE, &NodeDel))
			{
				int SatSel = 0;
				GUI_GetProperty("cmb_tp_list_sat", "select", &SatSel);
				app_antenna_tp_change_set(SatSel);
				
				s_Changed = TP_CHANGED;
				g_AppBook.invalid_remove();
				ret = GXCORE_SUCCESS;
			}
			GxCore_Free(NodeDel.id_array);
		}
    }

    return ret;
}

static void _tplist_popup_ok_keypress(void)
{
    status_t ret = GXCORE_ERROR;

    switch(s_CurMode)
    {
        case TP_ADD:
            if(s_tplist_sat_total > 0)
            {
				ret = _tplist_tp_add();
			}
			break;
		
		case TP_EDIT:
			if((s_tplist_sat_total > 0) && (s_tplist_tp_total > 0))
			{
				ret= _tplist_tp_edit();
			}
			break;

		case TP_DELETE:
			if((s_tplist_sat_total > 0) && (s_tplist_tp_total > 0) 
				&& (0 == strcasecmp("btn_tplist_popup_ok", GUI_GetFocusWidget()))
			)
			{
				GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_DELETING);
				GUI_SetProperty("text_tplist_popup_tip2", "string", NULL);
				GUI_SetProperty("img_tplist_popup_ok_back", "state", "hide");
				GUI_SetProperty("btn_tplist_popup_ok", "state", "hide");
				GUI_SetProperty("btn_tplist_popup_cancel", "state", "hide");
				GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);
				ret = _tplist_tp_del();
			}
			if(0 == strcasecmp("btn_tplist_popup_cancel", GUI_GetFocusWidget()))
			{
				GUI_EndDialog("wnd_tp_list_popup");
			}
			break;
		
		case TP_SEARCH:
			break;
	}

	if(ret == GXCORE_SUCCESS)
	{
		GUI_EndDialog("wnd_tp_list_popup");
		GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);
		//if(s_tplist_tp_total == 0)
		//{
		//	_tp_list_hide_tip();
	//		GUI_SetFocusWidget("cmb_tp_list_sat");
	//	}
		//else
		//{
		//	GUI_SetFocusWidget("list_tp_list");
		//}
	}
	else
	{
		//GUI_EndDialog("wnd_tp_list_popup"); For Mantis Bug:1733 2013-02-27
	}
	
	return;
}

SIGNAL_HANDLER int app_tplist_popup_create(GuiWidget *widget, void *usrdata)
{
#define POLAR_CONTENT	"[H,V]"
#define TP_LEN	20

	char buffer[TP_LEN];
	char* PolarStr[2] = {"H", "V"};
	uint32_t sel=0;
	//GxMsgProperty_NodeNumGet tp_num = {0};
 match_exist_sel =0;
 match_status = 0;
char add_fre_buffer[MAX_FRE_LEN +1]="03000";
char add_sym_buffer[MAX_FRE_LEN +1]="01000";
int i;
GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
	switch(s_CurMode)
	{
		case TP_ADD:
		{
			//title
			GUI_SetProperty("text_tplist_popup_title", "string", STR_ID_ADD);
			if(s_tplist_sat_total == 0)
			{
				GUI_SetProperty("box_tplist_popup_para", "state", "hide");
				GUI_SetProperty("text_tplist_popup_tip1", "string", STR_SAT_NOT_EXIST);
			}
			{
				for(i=0;i<MAX_FRE_LEN;i++)
				{
					buffer_fre_bak[i] =add_fre_buffer[i];
					buffer_sym_bak[i] = add_sym_buffer[i];
				}
				//default value:3000/H/1000
				//freq
				GUI_SetProperty("text_tplist_popup_freq2", "string", "03000");
			 	GUI_SetProperty("edit_tplist_popup_freq", "string", "03000");
				//polar
				GUI_SetProperty("cmb_tplist_popup_polar","content",POLAR_CONTENT);
				//sym
				GUI_SetProperty("text_tplist_popup_sym2", "string", "01000");
                                GUI_SetProperty("edit_tplist_popup_sym", "string", "01000");
	
				//hide
				GUI_SetProperty("img_tplist_popup_ok_back", "state", "hide");
				GUI_SetProperty("btn_tplist_popup_ok", "state", "hide");
				GUI_SetProperty("btn_tplist_popup_cancel", "state", "hide");

				GUI_SetFocusWidget("box_tplist_popup_para");
			}
		}
		break;

		case TP_EDIT:
		{
			//title
			GUI_SetProperty("text_tplist_popup_title", "string", STR_ID_EDIT);

			if(s_tplist_sat_total == 0)
			{
				GUI_SetProperty("box_tplist_popup_para", "state", "hide");
				GUI_SetProperty("text_tplist_popup_tip1", "string", STR_SAT_NOT_EXIST);
			}
			else if(s_tplist_tp_total == 0)
			{
				GUI_SetProperty("box_tplist_popup_para", "state", "hide");
				GUI_SetProperty("text_tplist_popup_tip1", "string", STR_TP_NOT_EXIST);
			}
			else
			{
			//freq
				memset(buffer, 0 ,TP_LEN);
				sprintf(buffer, "%05d", s_CurTp.tp_data.frequency);
				GUI_SetProperty("text_tplist_popup_freq2", "string", buffer);
				GUI_SetProperty("edit_tplist_popup_freq", "string", buffer);

			memset(buffer_fre_bak,0,MAX_FRE_LEN +1);
			sprintf(buffer_fre_bak, "%05d", s_CurTp.tp_data.frequency);

			//polar
				GUI_SetProperty("cmb_tplist_popup_polar","content",POLAR_CONTENT);
				sel = s_CurTp.tp_data.tp_s.polar;
				GUI_SetProperty("cmb_tplist_popup_polar", "select", &sel);
			
			//sym
				memset(buffer, 0 ,TP_LEN);
				sprintf(buffer, "%05d", s_CurTp.tp_data.tp_s.symbol_rate);
				GUI_SetProperty("text_tplist_popup_sym2", "string", buffer);
				GUI_SetProperty("edit_tplist_popup_sym", "string", buffer);

			memset(buffer_sym_bak,0,5);
			sprintf(buffer_sym_bak, "%05d", s_CurTp.tp_data.tp_s.symbol_rate);
			//hide
				GUI_SetProperty("img_tplist_popup_ok_back", "state", "hide");
				GUI_SetProperty("btn_tplist_popup_ok", "state", "hide");
				GUI_SetProperty("btn_tplist_popup_cancel", "state", "hide");

				GUI_SetFocusWidget("box_tplist_popup_para");
			}
	
		}
		break;

		case TP_DELETE:
		{
			uint32_t opt_num = 0;
			//title
			GUI_SetProperty("text_tplist_popup_title", "string", STR_ID_DELETE);
			//hide the box
			GUI_SetProperty("box_tplist_popup_para", "state", "hide");
			
			if(s_tplist_sat_total == 0)
			{
				GUI_SetProperty("text_tplist_popup_tip1", "string", STR_SAT_NOT_EXIST);
			}
			else if(s_tplist_tp_total == 0)
			{
				GUI_SetProperty("text_tplist_popup_tip1", "string", STR_TP_NOT_EXIST);
			}
			else
			{
				opt_num = s_TlIdOps->id_total_get(s_TlIdOps);
				if(1 == opt_num)//only one
				{
					uint32_t id[1] = {0};
					GxMsgProperty_NodeByIdGet tp_node = {0};
					
					s_TlIdOps->id_get(s_TlIdOps, id);
					tp_node.node_type = NODE_TP;
					tp_node.id = id[0];
					app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_node);

					GUI_SetProperty("text_tplist_popup_tip1", "string", STR_DLE_ONE_TP);
				//show tp para
					memset(buffer, 0 ,TP_LEN);
					sprintf(buffer, "%d / %s / %d", 
					tp_node.tp_data.frequency,
					PolarStr[tp_node.tp_data.tp_s.polar],
					tp_node.tp_data.tp_s.symbol_rate);
					GUI_SetProperty("text_tplist_popup_tip2", "string", buffer);
				
					GUI_SetFocusWidget("btn_tplist_popup_ok");
					 GUI_SetProperty("img_tplist_popup_ok_back", "img", "s_bar_choice_blue4.bmp");
				}
				else if(1 < opt_num)//del selected tp / tps
				{
					GUI_SetProperty("text_tplist_popup_tip1", "string", STR_DLE_MULTI_TP);
					GUI_SetFocusWidget("btn_tplist_popup_ok");
					 GUI_SetProperty("img_tplist_popup_ok_back", "img", "s_bar_choice_blue4.bmp");
				}
				else
				{
					GUI_SetProperty("text_tplist_popup_tip1", "string", STR_TP_NOT_EXIST);
				}
			}
		}
		break;

		case TP_SEARCH:
		{
			//title
			GUI_SetProperty("text_tplist_popup_title", "string", STR_ID_TP_SEARCH);
			//hide the box
			GUI_SetProperty("box_tplist_popup_para", "state", "hide");

			if(s_tplist_sat_total == 0)
			{
				GUI_SetProperty("text_tplist_popup_tip1", "string", STR_SAT_NOT_EXIST);
			}
			else if(s_tplist_tp_total == 0)
			{
				GUI_SetProperty("text_tplist_popup_tip1", "string", STR_TP_NOT_EXIST);
			}
			else
				;
		}
		break;
	}

	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tplist_popup_destroy(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_NodeNumGet node_num = {0};
	node_num.node_type = NODE_TP;
	node_num.sat_id = s_CurSat.sat_data.id;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	s_tplist_tp_total = node_num.node_num;
	if(s_tplist_tp_total > 0)
	{
		_tp_list_show_tip();
		GUI_SetFocusWidget("list_tp_list");
	}
	else
	{
		GUI_SetFocusWidget("cmb_tp_list_sat");
		_tp_list_hide_tip(TP_INFO_ADD);
	}
	//tp_list_show_help_info();
	return EVENT_TRANSFER_STOP;
}
static void check_input_valid(void)
{
	int sel =0;
	char *buffer_fre = NULL;
        char *buffer_sym = NULL;
        TpListTpPara InputTpPara={0};

	GUI_GetProperty("text_tplist_popup_freq2", "string", &buffer_fre);
	GUI_GetProperty("text_tplist_popup_sym2", "string", &buffer_sym);
	//check input valid?
	InputTpPara.frequency = atoi(buffer_fre);
	//sym
	InputTpPara.symbol_rate = atoi(buffer_sym);
	//polar
	GUI_GetProperty("cmb_tplist_popup_polar", "select", &sel);
	if(0 == sel)
		InputTpPara.polar = GXBUS_PM_TP_POLAR_H;
	else
		InputTpPara.polar = GXBUS_PM_TP_POLAR_V;

	if(false == _check_tp_para_valid(&s_CurSat.sat_data, &InputTpPara))
	{//show: invalid TP value
		GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
                GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
                memset(s_tplist_edit_input,'\0',6);
		buffer_fre = buffer_fre_bak;
		buffer_sym = buffer_sym_bak;   
	}
	else
	{
		int i;
		memset(buffer_fre_bak,'0',MAX_FRE_LEN);
		memset(buffer_sym_bak,'0',MAX_FRE_LEN);
		for(i=MAX_FRE_LEN-strlen(buffer_fre);i<MAX_FRE_LEN;i++)
		{
			buffer_fre_bak[i]=*buffer_fre;
			buffer_fre++;
		}
		for(i=MAX_FRE_LEN-strlen(buffer_sym);i<MAX_FRE_LEN;i++)
		{
			buffer_sym_bak[i]=*buffer_sym;
			buffer_sym++;
		}

		GUI_SetProperty("edit_tplist_popup_freq", "string",buffer_fre_bak);
        	GUI_SetProperty("edit_tplist_popup_sym", "string",buffer_sym_bak);
	
	}

  GUI_SetProperty("text_tplist_popup_freq2", "string",buffer_fre_bak);
  GUI_SetProperty("text_tplist_popup_sym2", "string",buffer_sym_bak);

}
static void edit_change_show()
{
	int box_sel = 0;
	GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
	if((TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)&&(edit_command_mode == 0))
	{
		GUI_SetProperty("edit_tplist_popup_freq", "state", "show");
		GUI_SetProperty("text_tplist_popup_freq2", "state", "hide");

	}
	else if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
	{
		GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
		GUI_SetProperty("text_tplist_popup_freq2", "state", "show");

	}
	else if((TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)&&(edit_command_mode == 0))
	{
		GUI_SetProperty("edit_tplist_popup_sym", "state", "show");
		GUI_SetProperty("text_tplist_popup_sym2", "state", "hide");

	}
	else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
	{
		GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
		GUI_SetProperty("text_tplist_popup_sym2", "state", "show");

	}
	else 
	{
		GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
                GUI_SetProperty("text_tplist_popup_freq2", "state", "show");
		GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
                GUI_SetProperty("text_tplist_popup_sym2", "state", "show");
	}
}

SIGNAL_HANDLER int app_tplist_popup_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint32_t box_sel;
	uint8_t FocusdOnOk=0;

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
					tp_exist_status = 0;
					GUI_EndDialog("after wnd_full_screen");
                    //GUI_EndDialog("wnd_tp_list_popup");
					//GUI_SendEvent("wnd_tp_list", event);
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    {
						if(tp_exist_status == 1)
						{
							GUI_SetProperty("text_tplist_popup_tip1_exist", "state", "hide");
							GUI_SetProperty("btn_tplist_tip_cancel_exist", "state", "hide");
							GUI_SetProperty("btn_tplist_tip_ok_exist", "state", "hide");
							GUI_SetProperty("img_tplist_tip_opt_exist", "state", "hide");

							//resume
							GUI_SetProperty("text_tplist_popup_tip1", "string", " ");
							//show the box
							GUI_SetProperty("box_tplist_popup_para", "state", "show");
							GUI_SetFocusWidget("box_tplist_popup_para");
							GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);

							tp_exist_status = 0;
						}
						else
						{
							tp_exist_status = 0;
							GUI_EndDialog("wnd_tp_list_popup");
						}
					}
					break;

				case STBK_OK:
					// when tp exist,choose ok or cancel
					if(tp_exist_status == 1)
					{
						if(0 == strcasecmp("btn_tplist_tip_ok_exist", GUI_GetFocusWidget()))
						{
							tp_exist_status = 0;
							GUI_EndDialog("wnd_tp_list_popup");
						}
						else
						{
							GUI_SetProperty("text_tplist_popup_tip1_exist", "state", "hide");
							GUI_SetProperty("btn_tplist_tip_cancel_exist", "state", "hide");
							GUI_SetProperty("btn_tplist_tip_ok_exist", "state", "hide");
							GUI_SetProperty("img_tplist_tip_opt_exist", "state", "hide");

							//resume
							GUI_SetProperty("text_tplist_popup_tip1", "string", " ");
							//show the box
							GUI_SetProperty("box_tplist_popup_para", "state", "show");
							GUI_SetFocusWidget("box_tplist_popup_para");
							GUI_SetProperty("wnd_tp_list_popup", "draw_now", NULL);

						}
						tp_exist_status = 0;
					}
					else 
					{
						if((s_CurMode==TP_ADD) && (s_CurMode==TP_EDIT))
						{
							edit_change_show();
							check_input_valid();
						}
						
						_tplist_popup_ok_keypress();
					}
					break;

				case STBK_UP:
					{
						GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
						if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
						{
							box_sel = TP_LIST_POPUP_BOX_ITEM_OK;
							GUI_SetProperty("box_tplist_popup_para", "select", &box_sel);
						}
					}
					break;

				case STBK_DOWN:
					{
						GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
						if(TP_LIST_POPUP_BOX_ITEM_OK == box_sel)
						{
							box_sel = TP_LIST_POPUP_BOX_ITEM_FREQ;
							GUI_SetProperty("box_tplist_popup_para", "select", &box_sel);
						}
					}
					break;
				case STBK_LEFT:
				case STBK_RIGHT:
					// when tp exist,choose ok or cancel
					if(tp_exist_status == 1)
					{
						if(0 == strcasecmp("btn_tplist_tip_ok_exist", GUI_GetFocusWidget()))
						{
							GUI_SetProperty("img_tplist_tip_opt_exist","img","s_bar_choice_blue4_l.bmp");
							GUI_SetFocusWidget("btn_tplist_tip_cancel_exist");
						}
						else
						{
							GUI_SetProperty("img_tplist_tip_opt_exist","img","s_bar_choice_blue4.bmp");
							GUI_SetFocusWidget("btn_tplist_tip_ok_exist");
						}
					}
					if(s_CurMode == TP_DELETE)
					{
						if(0 == strcasecmp("btn_tplist_popup_ok", GUI_GetFocusWidget()))
						{
							FocusdOnOk = 1;
						}
						else if(0 == strcasecmp("btn_tplist_popup_cancel", GUI_GetFocusWidget()))
						{
							FocusdOnOk = 0;
						}

						if(1 == FocusdOnOk)
						{
							GUI_SetProperty("img_tplist_popup_ok_back","img","s_bar_choice_blue4_l.bmp");
							GUI_SetFocusWidget("btn_tplist_popup_cancel");
						}
						else
						{
							GUI_SetProperty("img_tplist_popup_ok_back","img","s_bar_choice_blue4.bmp");
							GUI_SetFocusWidget("btn_tplist_popup_ok");							
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


SIGNAL_HANDLER int app_tplist_popup_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint32_t box_sel;
	#define MAX_LEN_ 20
	char num[2] = "";
	char *edit_string = NULL;
	uint32_t  edit_num =0;
	int edit_sel = 0;
	char edit_buffer[MAX_LEN_]={0};
	bool keypress_status = 0;	
	
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
					{
						memset(s_tplist_edit_input,'\0',6);

						//Modify by yanwzh 2012-07-19 15:36
						GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
						if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
						{  	
							//box_pos=1;
							_tplist_check_tp_add();
							box_sel = TP_LIST_POPUP_BOX_ITEM_OK;
							GUI_SetProperty("box_tplist_popup_para", "select", &box_sel);
						}
						else
						{
							if(TP_LIST_POPUP_BOX_ITEM_SYM==box_sel)
							{
								_tplist_check_sym_add();
							}
							box_sel = box_sel-1;
							ret = EVENT_TRANSFER_KEEPON;
						}
					up_down_status = 1;
					edit_command_mode = 1;

					break;
					}
				case STBK_DOWN:					
					{
						memset(s_tplist_edit_input,'\0',6);

						//Modify by yanwzh 2012-07-19 15:36
						GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
						if(TP_LIST_POPUP_BOX_ITEM_OK == box_sel)
						{
							box_sel = TP_LIST_POPUP_BOX_ITEM_FREQ;
							GUI_SetProperty("box_tplist_popup_para", "select", &box_sel);
						}
						else
						{

							if(TP_LIST_POPUP_BOX_ITEM_SYM==box_sel)
							{
								_tplist_check_sym_add();
							}

						    if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
                            {
                                // box_pos=1;
                                _tplist_check_tp_add();
                            }                  
							box_sel = box_sel+1;
							ret = EVENT_TRANSFER_KEEPON;
						}
					up_down_status = 1;
					edit_command_mode = 1;

					}
					break;

				case STBK_OK:
					edit_command_mode = 1;
					ret = EVENT_TRANSFER_KEEPON;
                   			 break;
				case STBK_MENU:
				case STBK_EXIT:
					ret = EVENT_TRANSFER_KEEPON;
                   			 break;

				case STBK_LEFT:
				// left = delete
				if(edit_command_mode == 0)
				{	
					GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);

					if(strlen(s_tplist_edit_input) == 1)
					{
						edit_sel = 0;
						s_tplist_edit_input[0] = '0';
						s_tplist_edit_input[1] = '\0';
						edit_del_to_zero = 1;
						if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
						{
							GUI_SetProperty("edit_tplist_popup_freq", "string", s_tplist_edit_input);
							GUI_SetProperty("edit_tplist_popup_freq", "select", &edit_sel);

							GUI_SetProperty("text_tplist_popup_freq2", "string", s_tplist_edit_input);
						}
						else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
						{
							GUI_SetProperty("edit_tplist_popup_sym", "string", s_tplist_edit_input);
							GUI_SetProperty("edit_tplist_popup_sym", "select", &edit_sel);

							GUI_SetProperty("text_tplist_popup_sym2", "string", s_tplist_edit_input);
						}
					}						
					else
					{
						s_tplist_edit_input[strlen(s_tplist_edit_input)-1] = '\0';
						
						#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
						edit_sel = strlen(s_tplist_edit_input)-1;
						#else
						edit_sel = strlen(s_tplist_edit_input);
						#endif
						if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
						{
							GUI_SetProperty("edit_tplist_popup_freq", "string", s_tplist_edit_input);
							GUI_SetProperty("edit_tplist_popup_freq", "select", &edit_sel);
							
							GUI_SetProperty("text_tplist_popup_freq2", "string", s_tplist_edit_input);
						}
						else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
						{
							GUI_SetProperty("edit_tplist_popup_sym", "string", s_tplist_edit_input);
							GUI_SetProperty("edit_tplist_popup_sym", "select", &edit_sel);
				
							GUI_SetProperty("text_tplist_popup_sym2", "string", s_tplist_edit_input);
						}
					}

				}
				// left = -
				if(edit_command_mode == 1)
				{
					memset(edit_buffer,0,MAX_LEN_);
					GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
					if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
					{
							GUI_GetProperty("text_tplist_popup_freq2", "string", &edit_string);
							edit_num = atoi(edit_string);
							if(edit_num > 0)
								edit_num = edit_num-1;
							sprintf(edit_buffer,"%05d",edit_num);
							GUI_SetProperty("text_tplist_popup_freq2", "string", edit_buffer);
							GUI_SetProperty("edit_tplist_popup_freq", "string", edit_buffer);							
					}
					else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
					{
							GUI_GetProperty("text_tplist_popup_sym2", "string", &edit_string);
							edit_num = atoi(edit_string);
							if(edit_num > 0)
								edit_num = edit_num-1;
							sprintf(edit_buffer,"%05d",edit_num);
							GUI_SetProperty("text_tplist_popup_sym2", "string", edit_buffer);	
							GUI_SetProperty("edit_tplist_popup_sym", "string", edit_buffer);
					}
				}	
			
					
				ret = EVENT_TRANSFER_KEEPON;
              			      break;
					
				case STBK_RIGHT:
					//right = +
				      if(edit_command_mode == 1)
				      {
					      memset(edit_buffer,0,MAX_LEN_);
					      GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
					      if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
					      {
						      GUI_GetProperty("text_tplist_popup_freq2", "string", &edit_string);
						      edit_num = atoi(edit_string);
							  if(edit_num<99999)
							  {
						      	edit_num = edit_num+1;
							  }
						      sprintf(edit_buffer,"%05d",edit_num);
						      GUI_SetProperty("text_tplist_popup_freq2", "string", edit_buffer);
						      GUI_SetProperty("edit_tplist_popup_freq", "string", edit_buffer);
					      }
					      else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
					      {
						      GUI_GetProperty("text_tplist_popup_sym2", "string", &edit_string);
						      edit_num = atoi(edit_string);
							  if(edit_num<99999)
							  {
						      	edit_num = edit_num+1;
							  }
						      sprintf(edit_buffer,"%05d",edit_num);
						      GUI_SetProperty("text_tplist_popup_sym2", "string", edit_buffer);		
						      GUI_SetProperty("edit_tplist_popup_sym", "string", edit_buffer);
					      }
				      }	
				      else
				      {
					      GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
					      if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
					      {
							#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
							edit_sel = strlen(s_tplist_edit_input)-1;
							#else
							edit_sel = strlen(s_tplist_edit_input);
							#endif
							GUI_SetProperty("edit_tplist_popup_freq", "select", &edit_sel);
					      }
					      else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
					      {
							#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
							edit_sel = strlen(s_tplist_edit_input)-1;
							#else
							edit_sel = strlen(s_tplist_edit_input);
							#endif
							GUI_SetProperty("edit_tplist_popup_sym", "select", &edit_sel);
					      }
				      }

				      ret = EVENT_TRANSFER_KEEPON;
				      break;
				
				case STBK_0:
				keypress_status = 1;
					memset(num,'0',1);
					break;
				case STBK_1:
				keypress_status = 1;
					memset(num,'1',1);
					break;
				case STBK_2:
				keypress_status = 1;
					memset(num,'2',1);
					break;
				case STBK_3:
				keypress_status = 1;
					memset(num,'3',1);
					break;
				case STBK_4:
				keypress_status = 1;
					memset(num,'4',1);
					break;
				case STBK_5:
				keypress_status = 1;
					memset(num,'5',1);
					break;
				case STBK_6:
				keypress_status = 1;
					memset(num,'6',1);
					break;
				case STBK_7:
				keypress_status = 1;
					memset(num,'7',1);
					break;
				case STBK_8:
				keypress_status = 1;
					memset(num,'8',1);
					break;
				case STBK_9:
				keypress_status = 1;
					memset(num,'9',1);
					break;
					
				default:
					break;
			}
		default:
			break;
	}
	//zl
	if(keypress_status == 1)
	{	
		edit_command_mode = 0;
		keypress_status = 0;

		GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);


		if(strlen(s_tplist_edit_input) < 5)
		{	
			if(edit_del_to_zero == 1)
			{
				edit_del_to_zero = 0;
				memset(s_tplist_edit_input,'\0',6);
			}

			if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
			{
				GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
				strcat(s_tplist_edit_input,num);
				GUI_SetProperty("edit_tplist_popup_freq", "string", s_tplist_edit_input);
				GUI_SetProperty("text_tplist_popup_freq2", "string", s_tplist_edit_input);
			
				#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
				edit_sel = strlen(s_tplist_edit_input)-1;
				#else
				edit_sel = strlen(s_tplist_edit_input);
				#endif
				GUI_SetProperty("edit_tplist_popup_freq", "select", &edit_sel);
			}
			else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
			{
				GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
				strcat(s_tplist_edit_input,num);
				GUI_SetProperty("edit_tplist_popup_sym", "string", s_tplist_edit_input);
				GUI_SetProperty("text_tplist_popup_sym2", "string", s_tplist_edit_input);
			
				#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
				edit_sel = strlen(s_tplist_edit_input)-1;
				#else
				edit_sel = strlen(s_tplist_edit_input);
				#endif
				GUI_SetProperty("edit_tplist_popup_sym", "select", &edit_sel);

			}
		}
		else if(strlen(s_tplist_edit_input) == 5)
        {
            edit_sel = 1;
            memset(s_tplist_edit_input,'\0',6);
            if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
            {
                GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
                strcat(s_tplist_edit_input,num);
                GUI_SetProperty("edit_tplist_popup_freq", "string", s_tplist_edit_input);
                GUI_SetProperty("text_tplist_popup_freq2", "string", s_tplist_edit_input);
				GUI_SetProperty("edit_tplist_popup_freq", "select", &edit_sel);
            }
            else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
            {
                GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
                strcat(s_tplist_edit_input,num);
                GUI_SetProperty("edit_tplist_popup_sym", "string", s_tplist_edit_input);
                GUI_SetProperty("text_tplist_popup_sym2", "string", s_tplist_edit_input);
				GUI_SetProperty("edit_tplist_popup_sym", "select", &edit_sel);
            }

        }

		if(strlen(s_tplist_edit_input) == 5)
			edit_command_mode = 1;
		edit_change_show();

	}
	else if(up_down_status == 1)
	{
		if(strlen(s_tplist_edit_input) == 5)
			edit_command_mode = 1;

		edit_change_show();
		check_input_valid();
		up_down_status = 0;
	}
	return ret;
	
}

#else //DEMOD_DVB_S

SIGNAL_HANDLER int app_tp_list_create(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tp_list_destroy(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tp_list_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tp_list_lost_focus(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tp_list_got_focus(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tp_list_list_create(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tp_list_list_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tp_list_list_get_total(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tp_list_list_get_data(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tp_list_list_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tp_list_cmb_sat_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tp_list_cmb_sat_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tplist_popup_create(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tplist_popup_destroy(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tplist_popup_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tplist_popup_box_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

#endif
