#include "app.h"
#if (DEMOD_DVB_S > 0)

#include "app_wnd_search.h"
#include "app_module.h"
#include "app_utility.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "app_frontend.h"
#include "app_pop.h"
#include "app_epg.h"
#include "app_default_params.h"
#include "full_screen.h"
#include "app_book.h"
#include "app_factory_test_date.h"
#include "app_keyboard_language.h"

#if TKGS_SUPPORT
#include "module/gxtkgs.h"
#endif

extern uint32_t app_tuner_sat_num_get(uint32_t tuner);
extern void app_sat_list_change_sel(uint32_t sat_sel);
extern SatSelStatus satsel;
extern bool scan_mode_disable_flag;

/* Private variable------------------------------------------------------- */

enum SAT_LIST_POPUP_BOX
{
	SAT_LIST_POPUP_BOX_ITEM_NAME = 0,
	SAT_LIST_POPUP_BOX_ITEM_LONGITUDE,
	SAT_LIST_POPUP_BOX_ITEM_DIRECT,
	SAT_LIST_POPUP_BOX_ITEM_INPUT_SELECT,
	SAT_LIST_POPUP_BOX_ITEM_OK
};

static enum
{
	SAT_ADD = 0,
	SAT_EDIT,
	SAT_DELETE,
	SAT_SEARCH
}s_CurMode;

static event_list* sp_SatListTimerSignal = NULL;
static GxMsgProperty_NodeByPosGet s_CurSat = {0};
static GxMsgProperty_NodeByPosGet s_CurTp = {0};
static uint8_t s_Changed = 0;
static int s_CurSatSel = 0;
uint16_t s_satlist_sat_total = 0;
static uint16_t s_satlist_sat_choise = 0;
static AppIdOps* s_SatIdOps = NULL;
static AppIdOps* s_SatLastIdOps = NULL;
static event_list* sp_SatListLockTimer=NULL;

static void _sat_list_set_diseqc(void);
static void _sat_list_set_tp(SetTpMode set_mode);
static int app_satellite_list_info_func(void);


// create dialog
int app_satellite_list_init(int SatSel)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_satellite_list"))
    {
        GUI_EndDialog("wnd_satellite_list");
    }
    GUI_CreateDialog("wnd_satellite_list");
    if(SatSel >= 0)
        GUI_SetProperty("list_sat_list", "select", &SatSel);
    return 0;
}

/**
 * Function:    app_sat_list_del_sel_status
 * Date:        2013-06-19
 * Description: delete sat sel status after factory reset!
 * Callby:      
 * Bug:         mantis bug2078
 */
void app_sat_list_del_sel_status(void)
{
    uint16_t i;
    for(i = 0; i < s_satlist_sat_total; i++)
    {
        if (s_SatLastIdOps->id_check(s_SatLastIdOps, i) != 0)
        {
            s_SatLastIdOps->id_delete(s_SatLastIdOps, i);
            s_satlist_sat_choise--;
        }
    }
}
void app_sat_list_close_lnb(void)
{
	AppFrontend_PolarState Polar =SEC_VOLTAGE_OFF ;
	app_ioctl(s_CurSat.sat_data.tuner, FRONTEND_POLAR_SET, (void*)&Polar);
	return;
}

#if FACTORY_TEST_SUPPORT
/*
**
** propose :just for factory test
** function:add sat to satlist
**data :2012 10 31
*/
//status_t MKTech_Add_Sat(uint32_t tuner , uint32_t * psat_id)
status_t app_factory_test_add_sat(uint32_t tuner ,FRONTPARA *para, uint32_t * psat_id)
{
    GxMsgProperty_NodeAdd NodeAdd;
    memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));
    NodeAdd.node_type = NODE_SAT;
    NodeAdd.sat_data.type = GXBUS_PM_SAT_S;
    NodeAdd.sat_data.tuner = 0;
    NodeAdd.sat_data.sat_s.lnb1 = 5150;
    NodeAdd.sat_data.sat_s.lnb2 = 5750;
    NodeAdd.sat_data.sat_s.lnb_power = para->lnb_power;//GXBUS_PM_SAT_LNB_POWER_ON;
    NodeAdd.sat_data.sat_s.switch_22K = para->switch22k;//GXBUS_PM_SAT_22K_OFF;
    //printf("g debug:fuc:%s:line:%d:22k%d\n",__FUNCTION__,__LINE__,NodeAdd.sat_data.sat_s.switch_22K);
    NodeAdd.sat_data.sat_s.diseqc11 = 0;
    NodeAdd.sat_data.sat_s.diseqc12_pos = DISEQ12_USELESS_ID;
    NodeAdd.sat_data.sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_1_0;
    NodeAdd.sat_data.sat_s.diseqc10 = GXBUS_PM_SAT_DISEQC_OFF;
    NodeAdd.sat_data.sat_s.switch_12V = GXBUS_PM_SAT_12V_OFF;
    NodeAdd.sat_data.sat_s.longitude_direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
    NodeAdd.sat_data.sat_s.reserved = 0;
    NodeAdd.sat_data.sat_s.longitude = 0;//longitude;
    memset((char *)NodeAdd.sat_data.sat_s.sat_name, 0, MAX_SAT_NAME);
    if( 0 == tuner )
        memcpy((char *)NodeAdd.sat_data.sat_s.sat_name, "AsiaSatA 3s"/*name_str*/, MAX_SAT_NAME -1);
    else
        memcpy((char *)NodeAdd.sat_data.sat_s.sat_name, "AsiaSatB 3s"/*name_str*/, MAX_SAT_NAME -1);
    NodeAdd.sat_data.tuner = tuner;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
        *psat_id = NodeAdd.sat_data.id;

    //printf("g debug :line:%d:sat_id:%d:tuner:%d\n",__LINE__,*psat_id,tuner);

    return GXCORE_SUCCESS;
}
#endif


#if TKGS_SUPPORT
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
extern bool app_tkgs_check_sat_is_turish(GxMsgProperty_NodeByPosGet * p_nodesat);
extern int app_tkgs_get_turksat_pInfo(GxMsgProperty_NodeByPosGet ** p_TKGSSat);
extern void app_tkgs_cantwork_popup(void);


static bool app_satlist_focus_is_turk(void)
{
	if(app_tkgs_check_sat_is_turish(&s_CurSat))
	{
		app_tkgs_cantwork_popup();
		return TRUE;
	}
	
	return FALSE;
}


#endif

/* widget macro -------------------------------------------------------- */

#define ANTENNA_TIMER
static int exist_tp(void)
{
    GxMsgProperty_NodeNumGet node_num = {0};
    node_num.node_type = NODE_TP;
    node_num.sat_id = s_CurSat.sat_data.id;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
    if(node_num.node_num ==0)
        return 0;
    return 1;
}


static int app_sat_list_clean_signal(void)
{
    unsigned int value = 0;
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};

    // progbar strength
    siganl_bar.strength_bar = "progbar_sat_list_strength";
    value=0;
    siganl_val.strength_val = value;


    GUI_SetProperty("text_sat_list_strength_value", "string", " ");

    // progbar quality
    siganl_bar.quality_bar= "progbar_sat_list_quality";
    siganl_val.quality_val = value;

    GUI_SetProperty("text_sat_list_quality_value", "string", " ");

    app_signal_progbar_update(s_CurSat.sat_data.tuner, &siganl_bar, &siganl_val);

    return 0;
}


static int timer_sat_list_show_signal(void *userdata)
{
    unsigned int value = 0;
    char Buffer[20] = {0};
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};

    // progbar strength
    app_ioctl(s_CurSat.sat_data.tuner, FRONTEND_STRENGTH_GET, &value);
	if(value > 99)
	{
		value = 100;
	}
    siganl_bar.strength_bar = "progbar_sat_list_strength";
    if(!exist_tp())
        value=0;
    siganl_val.strength_val = value;

    // strength value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_sat_list_strength_value", "string", Buffer);

    // progbar quality
    app_ioctl(s_CurSat.sat_data.tuner, FRONTEND_QUALITY_GET, &value);
	if(value > 99)
	{
		value = 100;
	}
    siganl_bar.quality_bar= "progbar_sat_list_quality";
    if(!exist_tp())
        value=0;
    siganl_val.quality_val = value;

    // quality value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_sat_list_quality_value", "string", Buffer);

    app_signal_progbar_update(s_CurSat.sat_data.tuner, &siganl_bar, &siganl_val);

    ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &value);

    return 0;
}

static void timer_sat_list_show_signal_stop(void)
{
    timer_stop(sp_SatListTimerSignal);
}

static void timer_sat_list_show_signal_reset(void)
{
    reset_timer(sp_SatListTimerSignal);
}

static int timer_sat_list_set_frontend(void *userdata)
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
	remove_timer(sp_SatListLockTimer);
    sp_SatListLockTimer = NULL;
	
    _sat_list_set_diseqc();
    _sat_list_set_tp(SET_TP_FORCE);

    return 0;
}

#define SUB_FUNC
static void _sat_list_set_diseqc(void)
{
    AppFrontend_DiseqcParameters diseqc10 = {0};
    AppFrontend_DiseqcParameters diseqc11 = {0};
    GxMsgProperty_NodeNumGet tp_num = {0};
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
#if (DEMOD_DVB_COMBO > 0)
	app_set_tune_mode(s_CurSat.sat_data.type);
#endif

    if(s_satlist_sat_total > 0)
    {
        diseqc10.type = DiSEQC10;
        diseqc10.u.params_10.chDiseqc = s_CurSat.sat_data.sat_s.diseqc10;
        diseqc10.u.params_10.bPolar = app_show_to_lnb_polar(&s_CurSat.sat_data);
        
        tp_num.node_type = NODE_TP;
        tp_num.sat_id = s_CurSat.sat_data.id;
        app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &tp_num);
        if(tp_num.node_num== 0)
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
		&&(tp_num.node_num!= 0))
    {
        app_diseqc12_goto_position(s_CurSat.sat_data.tuner,s_CurSat.sat_data.sat_s.diseqc12_pos, false);
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

static void _sat_list_set_tp(SetTpMode set_mode)
{
    AppFrontend_SetTp params = {0};
    GxMsgProperty_NodeNumGet tp_num = {0};

    if(s_satlist_sat_total > 0)
    {
        tp_num.node_type = NODE_TP;
        tp_num.sat_id = s_CurSat.sat_data.id;
        app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &tp_num);
        if(tp_num.node_num > 0)
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
            params.polar = SEC_VOLTAGE_OFF;
        params.sat22k = app_show_to_sat_22k(s_CurSat.sat_data.sat_s.switch_22K, &s_CurTp.tp_data);
    }
    //params.inversion = INVERSION_AUTO;
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
				params.qam = s_CurTp.tp_data.tp_dtmb.symbol_rate; // the bandwidth		
				params.type_1501 = DTMB;
			}
			else
			{
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
	
	app_set_tp(s_CurSat.sat_data.tuner, &params, set_mode);
	extern int ex_sat_search_mode;
	if(/*tp_num.node_num==0&&*/ex_sat_search_mode==1)
	{
		ex_sat_search_mode = 0;
		if(tp_num.node_num==0)
		{
			GxFrontendClearErrFreFlag();
		}
	}
	return;
}

#define S_INFO_ALL 0
#define S_INFO_ADD 1
//#define S_HELP_RENAME 2
static void sat_list_hide_help_info(int s_help_info)
{
	
	GUI_SetProperty("img_sat_list_tip_0", "state", "hide");
	GUI_SetProperty("img_sat_list_tip_red", "state", "hide");
	GUI_SetProperty("img_sat_list_tip_yellow", "state", "hide");
	GUI_SetProperty("img_sat_list_tip_blue", "state", "hide");

	GUI_SetProperty("text_sat_list_tip_0", "state", "hide");
	GUI_SetProperty("text_sat_list_tip_red", "state", "hide");
	if(S_INFO_ALL == s_help_info)
	{
		GUI_SetProperty("img_sat_list_tip_green", "state", "hide");
		GUI_SetProperty("text_sat_list_tip_green", "state", "hide");
	}
	else
	{
	//		GUI_SetProperty("text_sat_list_tip_green", "string", STR_ID_RENAME);
		GUI_SetProperty("text_sat_list_tip_green", "string", STR_ID_ADD);
		GUI_SetProperty("img_sat_list_tip_green", "state", "show");
		GUI_SetProperty("text_sat_list_tip_green", "state", "show");
	}
	GUI_SetProperty("text_sat_list_tip_yellow", "state", "hide");
	GUI_SetProperty("text_sat_list_tip_blue", "state", "hide");

	GUI_SetProperty("img_satellite_tip_info", "state", "hide");
	GUI_SetProperty("text_satellite_tip_info", "state", "hide");
}

static void sat_list_show_help_info(void)
{	
	
	GUI_SetProperty("img_sat_list_tip_0", "state", "show");
	GUI_SetProperty("img_sat_list_tip_red", "state", "show");
	GUI_SetProperty("img_sat_list_tip_green", "state", "show");
	GUI_SetProperty("img_sat_list_tip_yellow", "state", "show");
	GUI_SetProperty("img_sat_list_tip_blue", "state", "show");


	GUI_SetProperty("text_sat_list_tip_0", "state", "show");
	GUI_SetProperty("text_sat_list_tip_red", "state", "show");
	GUI_SetProperty("text_sat_list_tip_green", "state", "show");
	GUI_SetProperty("text_sat_list_tip_green", "string", STR_ID_ADD);
	//	GUI_SetProperty("text_sat_list_tip_green", "string", STR_ID_RENAME);
	GUI_SetProperty("text_sat_list_tip_yellow", "state", "show");
	GUI_SetProperty("text_sat_list_tip_blue", "state", "show");

    GUI_SetProperty("img_satellite_tip_info", "state", "show");
    GUI_SetProperty("text_satellite_tip_info", "state", "show");
}

static int app_satellite_list_info_func(void)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_antenna_setting"))
    {
        // step 1: check the mode of the antenna, tuner1 or tuner2
        if(app_tuner_sat_num_get(app_tuner_cur_tuner_get()) > 0)
        {
            int SatSel = 0;
            GUI_GetProperty("list_sat_list", "select", &SatSel);
            app_sat_list_change_sel(SatSel);
            GUI_EndDialog("wnd_satellite_list");
            app_antenna_setting_init(-1, -1, SatSel);
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
        int SatSel = 0;

        GUI_GetProperty("list_sat_list", "select", &SatSel);
        GxMsgProperty_NodeByPosGet Sat = {0};
        Sat.node_type = NODE_SAT;
        Sat.pos = SatSel;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &Sat);

        app_sat_list_change_sel(SatSel);
        app_tuner_cur_tuner_set(Sat.sat_data.tuner);
        timer_sat_list_show_signal_stop();
        app_antenna_setting_init(-1, -1, SatSel);
    }
    return 0;
}

#define SAT_LIST
SIGNAL_HANDLER int app_sat_list_create(GuiWidget *widget, void *usrdata)
{
    GxMsgProperty_NodeNumGet node_num = {0};

    s_satlist_sat_choise = 0;
    s_SatIdOps = new_id_ops();
    if(s_SatIdOps == NULL)
        return GXCORE_ERROR;
	
	//app_sat_list_clean_signal();
    //get total sat num
    memset(&s_CurSat, 0, sizeof(s_CurSat));
    memset(&s_CurTp, 0, sizeof(s_CurTp));
	s_CurSatSel = -1;
	node_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	s_satlist_sat_total = node_num.node_num;
	if(s_satlist_sat_total > 0)
	{
		if(satsel.sel_in_all >= s_satlist_sat_total)
			s_CurSatSel = 0;
		else
			s_CurSatSel = satsel.sel_in_all;

		s_SatIdOps->id_open(s_SatIdOps, MANAGE_SAME_LIST, s_satlist_sat_total);

		if(s_SatLastIdOps != NULL)
		{
			uint16_t i;
			for(i = 0; i < s_satlist_sat_total; i++)
			{
				if (s_SatLastIdOps->id_check(s_SatLastIdOps, i) != 0)
				{	
					GxMsgProperty_NodeByPosGet s_Sat = {0};
					
					s_Sat.node_type = NODE_SAT;
					s_Sat.pos = i;
					app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_Sat);
					
					s_SatIdOps->id_add(s_SatIdOps, i, s_Sat.sat_data.id);
						s_satlist_sat_choise++;
	
				}
			}
		}

		s_CurSat.node_type = NODE_SAT;
		s_CurSat.pos = s_CurSatSel;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurSat);

		memset(&s_CurTp, 0, sizeof(GxMsgProperty_NodeByPosGet));
		node_num.node_type = NODE_TP;
		node_num.sat_id = s_CurSat.sat_data.id;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
		if(node_num.node_num > 0)
		{
			s_CurTp.node_type = NODE_TP;
			s_CurTp.pos = 0;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);
		}
	}
	else
	{
		sat_list_hide_help_info(S_INFO_ADD);
	}
	
	GUI_SetProperty("list_sat_list", "select", &s_CurSatSel);
	app_lang_set_menu_font_type("text_sat_list_title");
	app_sat_list_clean_signal();
	

	//set tp
	// for clear the frontend mseeage queue
	GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
	
	_sat_list_set_diseqc();
	_sat_list_set_tp(SET_TP_FORCE);
	
	if (reset_timer(sp_SatListTimerSignal) != 0) 
	{
		sp_SatListTimerSignal = create_timer(timer_sat_list_show_signal, 100, NULL, TIMER_REPEAT);
	}
#if DOUBLE_S2_SUPPORT
		GUI_SetProperty("text_satlist_tuner_select","state","show");
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sat_list_destroy(GuiWidget *widget, void *usrdata)
{
	uint32_t panel_led;
	
	remove_timer(sp_SatListTimerSignal);
	sp_SatListTimerSignal = NULL;

	remove_timer(sp_SatListLockTimer);
	sp_SatListLockTimer = NULL;

	// del_id_ops(s_SatIdOps);
	if(s_SatLastIdOps != NULL)
		del_id_ops(s_SatLastIdOps);
	s_SatLastIdOps = s_SatIdOps;
	 s_SatIdOps = NULL;

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
static void app_satlist_start_dlna_cb(void)
{
    timer_sat_list_show_signal_stop();
}

static void app_satlist_stop_dlna_cb(void)
{
    timer_sat_list_show_signal_reset();
}
#endif

SIGNAL_HANDLER int app_sat_list_keypress(GuiWidget *widget, void *usrdata)
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
                app_dlna_ui_exec(app_satlist_start_dlna_cb, app_satlist_stop_dlna_cb);
                break;
#endif

            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
				
			case STBK_EXIT:
			case STBK_MENU:
				if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_antenna_setting"))
				{
					// step 1: check the mode of the antenna, tuner1 or tuner2
					if(app_tuner_sat_num_get(app_tuner_cur_tuner_get()) == 0)
					{
						GUI_EndDialog("after wnd_main_menu");
        					GUI_SetProperty("wnd_main_menu", "draw_now", NULL); 
#ifndef MINI_16BIT_WIN8_OSD_SUPPORT
						#if MV_WIN_SUPPORT
						extern void app_main_item_menu_create(int value);
						app_main_item_menu_create(0);
						#endif
#endif
						ret = EVENT_TRANSFER_STOP;
						break;
					}
					if(s_satlist_sat_total > 0)
						app_satellite_list_info_func();
				}
                else
                {
                g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
                }
				GUI_EndDialog("wnd_satellite_list");
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_GREEN:	//add
				if(s_satlist_sat_total >= SYS_MAX_SAT)
				{
					PopDlg  pop;
					memset(&pop, 0, sizeof(PopDlg));
        			pop.type = POP_TYPE_OK;
       				pop.str = STR_ID_MAX_SAT;
       				pop.mode = POP_MODE_UNBLOCK;
                    pop.format = POP_FORMAT_DLG;
					popdlg_create(&pop);
				}
				else
				{
					sat_list_hide_help_info(S_INFO_ALL);
					s_CurMode = SAT_ADD;
					GUI_CreateDialog("wnd_sat_list_popup");
				}
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_YELLOW:	//edit 500k 6605
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
				if(GXBUS_PM_SAT_S != s_CurSat.sat_data.type)
					break;
//#endif
				#if TKGS_SUPPORT
				if(FALSE == app_satlist_focus_is_turk())
				{
					s_CurMode = SAT_EDIT;
					if(s_satlist_sat_total > 0)
					{
						sat_list_hide_help_info(S_INFO_ALL);
						GUI_CreateDialog("wnd_sat_list_popup");
					}
				}
				#else
				s_CurMode = SAT_EDIT;
				if(s_satlist_sat_total > 0)
				{
#if 0
					if(s_SatIdOps->id_total_get(s_SatIdOps) >= 0) //no sat selected
					{
						s_SatIdOps->id_add(s_SatIdOps, s_CurSatSel, s_CurSat.sat_data.id);
						GUI_SetProperty("list_sat_list", "update_row", &s_CurSatSel);
					}
#endif
					sat_list_hide_help_info(S_INFO_ALL);
					GUI_CreateDialog("wnd_sat_list_popup");
				}	
				#endif
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_RED:		//del
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
				if(GXBUS_PM_SAT_S != s_CurSat.sat_data.type)
					break;
//#endif
				#if TKGS_SUPPORT
					if(s_satlist_sat_total > 0)
					{
						uint32_t opt_num = 0;
						opt_num = s_SatIdOps->id_total_get(s_SatIdOps);
						
						if(opt_num == 0) //no sat selected
						{
							if(app_satlist_focus_is_turk())
							{
								ret = EVENT_TRANSFER_STOP;
								break;
							}
							s_SatIdOps->id_add(s_SatIdOps, s_CurSatSel, s_CurSat.sat_data.id);
							GUI_SetProperty("list_sat_list", "update_row", &s_CurSatSel);
						}
						else
						{
							GxMsgProperty_NodeByPosGet * p_TKGSSat = NULL;
							int tkgsSatId = app_tkgs_get_turksat_pInfo(&p_TKGSSat);
							if(1== s_SatIdOps->id_check(s_SatIdOps,(uint32_t)tkgsSatId))
							{
								s_SatIdOps->id_delete(s_SatIdOps,tkgsSatId);
								GUI_SetProperty("list_sat_list", "update_row", &s_CurSatSel);
								if(opt_num == 1)
								{
									app_tkgs_cantwork_popup();
									ret = EVENT_TRANSFER_STOP;		
									break;
								}
							}
						}
						
						s_CurMode = SAT_DELETE;
						sat_list_hide_help_info(S_INFO_ALL);
						GUI_CreateDialog("wnd_sat_list_popup");
					}	

				#else		
				s_CurMode = SAT_DELETE;
				
				if(s_satlist_sat_total > 0)
				{
					uint32_t opt_num = 0;
					opt_num = s_SatIdOps->id_total_get(s_SatIdOps);
					if(opt_num == 0) //no sat selected
					{
						s_SatIdOps->id_add(s_SatIdOps, s_CurSatSel, s_CurSat.sat_data.id);
						GUI_SetProperty("list_sat_list", "update_row", &s_CurSatSel);
					}
					
					sat_list_hide_help_info(S_INFO_ALL);
					GUI_CreateDialog("wnd_sat_list_popup");
				}	
				#endif
				ret = EVENT_TRANSFER_STOP;
				break;
	
			case STBK_BLUE:
			{
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
				if(GXBUS_PM_SAT_S != s_CurSat.sat_data.type)
					break;
//#endif
				SearchIdSet search_id;
				SearchType search_type;
				GxMsgProperty_NodeNumGet tp_num = {0};
				uint32_t opt_num = 0;
				#if TKGS_SUPPORT
				if(app_tkgs_get_operatemode() == GX_TKGS_AUTOMATIC)
				{
					app_tkgs_cantwork_popup();
					ret = EVENT_TRANSFER_STOP;
					break;
				}
				#endif
				if(s_satlist_sat_total > 0)
				{
					opt_num = s_SatIdOps->id_total_get(s_SatIdOps);
					if(opt_num == 0) //no sat selected
					{
						s_SatIdOps->id_add(s_SatIdOps, s_CurSatSel, s_CurSat.sat_data.id);
						GUI_SetProperty("list_sat_list", "update_row", &s_CurSatSel);
						search_id.id_num = 1;
					}
					else
					{
						search_id.id_num = opt_num;
					}
					
					search_id.id_array = (uint32_t *)GxCore_Malloc(search_id.id_num * sizeof(uint32_t));
					if(search_id.id_array != NULL)
					{
						s_SatIdOps->id_get(s_SatIdOps, (uint32_t *)search_id.id_array);

			#if 0
						if (s_SatIdOps->id_check(s_SatIdOps, s_CurSatSel) == 0)//ID_UNEXIST
						{
							tp_num.sat_id = search_id.id_array[0];
						}
						else
						{
							tp_num.sat_id = s_CurSat.sat_data.id;
						}
						tp_num.node_type = NODE_TP;
						app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &tp_num);
			#else
						for(opt_num = 0; opt_num < search_id.id_num; opt_num++)
						{
							tp_num.sat_id = search_id.id_array[opt_num];
							tp_num.node_type = NODE_TP;
							app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &tp_num);
							if(tp_num.node_num > 0)
								break;
						}
			#endif
						if(tp_num.node_num > 0)
						{
							search_type = SEARCH_SAT;
                            scan_mode_disable_flag = false;
						}
						else
						{
							search_type = SEARCH_BLIND_ONLY;
                            scan_mode_disable_flag = true;
						}
						sat_list_hide_help_info(S_INFO_ALL);
						if(app_search_opt_dlg(search_type, &search_id) == WND_OK)
						{
							timer_sat_list_show_signal_stop();
							timer_stop(sp_SatListLockTimer);
							app_search_start(0);
						}
						GxCore_Free(search_id.id_array);
						sat_list_show_help_info();
					}
				}
				else
				{
					s_CurMode = SAT_SEARCH;
					//GUI_CreateDialog("wnd_sat_list_popup"); //sat not exsit
				}
				ret = EVENT_TRANSFER_STOP;
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
				if(s_satlist_sat_total > 0)
					app_satellite_list_info_func();
				ret = EVENT_TRANSFER_STOP;
				break;
		
			default:
				break;
		}		
	}

	return ret;
}

SIGNAL_HANDLER int app_sat_list_lost_focus(GuiWidget *widget, void *usrdata)
{
	if(s_satlist_sat_total > 0)
	{
		GUI_SetProperty("list_sat_list", "active", &s_CurSatSel);
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sat_list_got_focus(GuiWidget *widget, void *usrdata)
{
	
	GxMsgProperty_NodeNumGet node_num = {0};
	int sel = -1;
	int act_sel= -1;

	GUI_SetProperty("list_sat_list", "active", &act_sel);
	sel = s_CurSatSel;

	if((SAT_EDIT == s_CurMode)&&(s_Changed == 1))
	{
		s_CurSat.node_type = NODE_SAT;
		s_CurSat.pos = sel;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurSat);

		GUI_SetProperty("list_sat_list", "update_row", &sel);
	}
	else	if((SAT_ADD == s_CurMode)&&(s_Changed == 1))
	{
		node_num.node_type = NODE_SAT;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
		s_satlist_sat_total = node_num.node_num;
		if(s_satlist_sat_total > 0)
		{
			sel = s_satlist_sat_total - 1;
			s_CurSat.node_type = NODE_SAT;
			s_CurSat.pos = sel;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurSat);

			memset(&s_CurTp, 0, sizeof(GxMsgProperty_NodeByPosGet));
			node_num.node_type = NODE_TP;
			node_num.sat_id = s_CurSat.sat_data.id;
			app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
			if(node_num.node_num > 0)
			{
				s_CurTp.node_type = NODE_TP;
				s_CurTp.pos = 0;
				app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);
			}
			if(s_SatIdOps != NULL)
			{
				uint16_t i;
				
				if(s_SatLastIdOps != NULL)
					del_id_ops(s_SatLastIdOps);
				s_SatLastIdOps = s_SatIdOps;
				s_SatIdOps = new_id_ops();

				if(s_SatIdOps != NULL)
				{
				s_SatIdOps->id_open(s_SatIdOps, MANAGE_SAME_LIST, s_satlist_sat_total);
				
					for(i = 0; i < s_satlist_sat_total-1; i++)
					{
						if (s_SatLastIdOps->id_check(s_SatLastIdOps, i) != 0)
						{	
							GxMsgProperty_NodeByPosGet s_Sat = {0};
							
							s_Sat.node_type = NODE_SAT;
							s_Sat.pos = i;
							app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_Sat);
							
							s_SatIdOps->id_add(s_SatIdOps, i, s_Sat.sat_data.id);
						}
					}
				}
			}
		}
		else
		{
			sel = -1;
		}

		GUI_SetProperty("list_sat_list", "update_all", NULL);
		GUI_SetProperty("list_sat_list", "select", &sel);

		if(reset_timer(sp_SatListLockTimer) != 0) 
		{
			sp_SatListLockTimer = create_timer(timer_sat_list_set_frontend, FRONTEND_SET_DELAY_MS, NULL, TIMER_ONCE);
		}	
	}
	else if((SAT_DELETE == s_CurMode)&&(s_Changed == 1))
	{
		int i;
		int del_count = 0;
		node_num.node_type = NODE_SAT;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
		s_satlist_sat_total = node_num.node_num;
		if(s_satlist_sat_total > 0)
		{
			if(s_SatIdOps->id_total_get(s_SatIdOps) > 0)
			{
				del_count = 0;
	            		for(i = 0; i < s_satlist_sat_total + s_SatIdOps->id_total_get(s_SatIdOps); i++)
	            		{
	            			if(true == s_SatIdOps->id_check(s_SatIdOps, i))
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

			if(sel >= s_satlist_sat_total )
			{
				sel = s_satlist_sat_total - 1;
			}
				
			s_CurSat.node_type = NODE_SAT;
			s_CurSat.pos = sel;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurSat);

			memset(&s_CurTp, 0, sizeof(GxMsgProperty_NodeByPosGet));
			node_num.node_type = NODE_TP;
			node_num.sat_id = s_CurSat.sat_data.id;
			app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
			if(node_num.node_num > 0)
			{
				s_CurTp.node_type = NODE_TP;
				s_CurTp.pos = 0;
				app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);
			}
		}
		else
		{
			sel = -1;
		}
		if(s_SatIdOps != NULL)
		{
			s_SatIdOps->id_close(s_SatIdOps);
			s_SatIdOps->id_open(s_SatIdOps, MANAGE_SAME_LIST, s_satlist_sat_total);
		}
            	GUI_SetProperty("list_sat_list", "update_all", NULL);
		GUI_SetProperty("list_sat_list", "select", &sel);
	}
	else //SEARCH
	{
		memset(&s_CurTp, 0, sizeof(GxMsgProperty_NodeByPosGet));
		node_num.node_type = NODE_TP;
		node_num.sat_id = s_CurSat.sat_data.id;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
		if(node_num.node_num > 0)
		{
			s_CurTp.node_type = NODE_TP;
			s_CurTp.pos = 0;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);
		}
	}
    app_sat_list_change_sel(s_CurSat.pos);
	// for clear the frontend mseeage queue
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
	
	//_sat_list_set_diseqc();
	//_sat_list_set_tp(SET_TP_FORCE);
	if(reset_timer(sp_SatListLockTimer) != 0) 
	{
		sp_SatListLockTimer = create_timer(timer_sat_list_set_frontend, FRONTEND_SET_DELAY_MS, NULL, TIMER_ONCE);
	}	
	
	s_CurSatSel = sel;
	//clear
	s_Changed = 0;
	timer_sat_list_show_signal_reset();

	//if(s_SatIdOps != NULL)
	//{
	//	s_SatIdOps->id_close(s_SatIdOps);
	//	s_SatIdOps->id_open(s_SatIdOps, MANAGE_SAME_LIST, s_satlist_sat_total);
	//}
	
	return EVENT_TRANSFER_STOP;
}

#define TP_LISTVIEW
SIGNAL_HANDLER int app_sat_list_list_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sat_list_list_change(GuiWidget *widget, void *usrdata)
{
	uint32_t list_sel;
	GxMsgProperty_NodeNumGet node_num = {0};
	
	GUI_GetProperty("list_sat_list", "select", &list_sel);
	
	s_CurSatSel = list_sel; 
	s_CurSat.node_type = NODE_SAT;
	s_CurSat.pos = s_CurSatSel;
       app_sat_list_change_sel(s_CurSat.pos);
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurSat);

	memset(&s_CurTp, 0, sizeof(GxMsgProperty_NodeByPosGet));
	node_num.node_type = NODE_TP;
	node_num.sat_id = s_CurSat.sat_data.id;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	if(node_num.node_num > 0)
	{
		s_CurTp.node_type = NODE_TP;
		s_CurTp.pos = 0;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurTp);
	}

	if(reset_timer(sp_SatListLockTimer) != 0) 
	{
		sp_SatListLockTimer = create_timer(timer_sat_list_set_frontend, FRONTEND_SET_DELAY_MS, NULL, TIMER_ONCE);
	}	

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sat_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_NodeNumGet node_num = {0};
	
	node_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	
	return node_num.node_num;
}

SIGNAL_HANDLER int app_sat_list_list_get_data(GuiWidget *widget, void *usrdata)
{
#define MAX_LEN	30
	ListItemPara* item = NULL;
	static GxMsgProperty_NodeByPosGet node;
	static char buffer1[MAX_LEN];
	static char buffer2[MAX_LEN];
	//static char buffer3[MAX_LEN];
	//static char buffer4[MAX_LEN];

	item = (ListItemPara*)usrdata;
	if((NULL == item) ||(0 > item->sel))
		return GXCORE_ERROR;


	//col-0: choice
	item->x_offset = 0;
	if (s_SatIdOps->id_check(s_SatIdOps, item->sel) == 0)//ID_UNEXIST
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

	//col-2: name
	item = item->next;
	//get tp node
	node.node_type = NODE_SAT;
	node.pos = item->sel;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
	item->string = (char*)node.sat_data.sat_s.sat_name;

	//col-3: longitude
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	memset(buffer2, 0, MAX_LEN);
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
	if(GXBUS_PM_SAT_S != node.sat_data.type)
	{
		sprintf(buffer2, "N/A");
	}else
//#endif

	sprintf(buffer2, "%d.%d", node.sat_data.sat_s.longitude/10, node.sat_data.sat_s.longitude%10);
	item->string = buffer2;

	//col-4: direct
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
	if(GXBUS_PM_SAT_S != node.sat_data.type)
	{
		item->string = buffer2;
	}else
//#endif
	if(node.sat_data.sat_s.longitude_direct == GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST)
		item->string = "E";
	else
		item->string = "W";

#if DOUBLE_S2_SUPPORT
	//col: tuner input
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	if(node.sat_data.tuner)
		item->string = "T2";
	else
		item->string = "T1";
#endif
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_sat_list_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	//uint32_t sel=0;

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
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
				GxMsgProperty_NodeByPosGet s_Sat = {0};
				s_Sat.node_type = NODE_SAT;
				s_Sat.pos = s_CurSatSel;
				app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_Sat);

				if(s_Sat.sat_data.type != GXBUS_PM_SAT_S)
				{
					break;
				}
//#endif	
					//GUI_GetProperty("list_sat_list", "select", &sel);
					if(s_satlist_sat_total > 0)
					{
						if (s_SatIdOps->id_check(s_SatIdOps, s_CurSatSel) == 0)//ID_UNEXIST
						{
							s_SatIdOps->id_add(s_SatIdOps, s_CurSatSel, s_CurSat.sat_data.id);
							s_satlist_sat_choise++;
						}
						else
						{
							s_SatIdOps->id_delete(s_SatIdOps, s_CurSatSel);
							s_satlist_sat_choise--;
						}
						GUI_SetProperty("list_sat_list", "update_row", &s_CurSatSel);
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				}
			case STBK_0:
				{
					if(s_satlist_sat_total > 0)
					{
						uint16_t i = 0;
						GxMsgProperty_NodeByPosGet node = {0};

						if(s_satlist_sat_choise < s_satlist_sat_total)
						{
							for(i = 0; i < s_satlist_sat_total; i++)
							{
								node.node_type = NODE_SAT;
								node.pos = i;
								app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
								if(node.sat_data.type != GXBUS_PM_SAT_S)
								{									
									continue;
								}
//#endif
								s_SatIdOps->id_add(s_SatIdOps, i, node.sat_data.id);	
							}
							s_satlist_sat_choise = s_satlist_sat_total;
						}
						else
						{
							for(i = 0; i < s_satlist_sat_total; i++)
							{
								s_SatIdOps->id_delete(s_SatIdOps, i);
							}
							s_satlist_sat_choise = 0;
						}
						GUI_SetProperty("list_sat_list", "update", NULL);
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				}
				break;

                case STBK_PAGE_UP:
				{
					uint32_t sel;
					GUI_GetProperty("list_sat_list", "select", &sel);
					if (sel == 0)
					{
						sel = app_sat_list_list_get_total(NULL,NULL)-1;
						GUI_SetProperty("list_sat_list", "select", &sel);
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
					GUI_GetProperty("list_sat_list", "select", &sel);
					if (app_sat_list_list_get_total(NULL,NULL)
					== sel+1)
					{
						sel = 0;
						GUI_SetProperty("list_sat_list", "select", &sel);
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


#define SAT_POPUP
static void _sat_rename_release_cb(PopKeyboard *data)
{
	if(data->in_ret == POP_VAL_CANCEL)
		return;
		
    if (data->out_name == NULL)
        return;

    GUI_SetProperty("btn_sat_list_name", "string", data->out_name);
}

static status_t app_satlist_sat_check_name(char *name_str , int type)
{
    uint32_t i = 0;
    GxMsgProperty_NodeNumGet node_num = {0};
    GxMsgProperty_NodeByPosGet node;

    node_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);

    for(i = 0; i < node_num.node_num; i++)
    {
        //type = 1表示edit,不与自己比较
        if(type == 1 && i == s_CurSatSel)
            continue;
        node.node_type = NODE_SAT;
        node.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

        if(strcmp((char*)node.sat_data.sat_s.sat_name, name_str ) == 0)
        {
            return GXCORE_ERROR;
        }
    }
    return GXCORE_SUCCESS;
}

static status_t app_satlist_sat_edit(void)
{
    GxMsgProperty_NodeModify NodeModify;
    char *name_str = NULL;
    char *atoi_str = NULL;
    int longitude = 0;
    GxBusPmDataSatLongitudeDirect direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
    uint32_t sel = 0;

    //name
    GUI_GetProperty("btn_sat_list_name", "string", &name_str);
    if(app_satlist_sat_check_name(name_str, 1) == GXCORE_ERROR)
    {
        GUI_SetProperty("box_satlist_popup_para", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
        GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
        GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_SAT_EXIT);
		GUI_SetInterface("flush",NULL);

		GxCore_ThreadDelay(1000);

		GUI_SetProperty("box_satlist_popup_para", "state", "show");
		GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
		GUI_SetProperty("text_satlist_popup_tip1", "state", "hide");
		GUI_SetFocusWidget("box_satlist_popup_para");
		GUI_SetInterface("flush",NULL);

		return GXCORE_ERROR;
	}

	//longitude
	GUI_GetProperty("edit_satlist_popup_longitude", "string", &atoi_str);
	longitude = 10 * app_float_edit_str_to_value(atoi_str);
	if(longitude > 1800)
	{
		char buffer[20] = {0};
		//hide the box
		GUI_SetProperty("box_satlist_popup_para", "state", "hide");
		GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
		GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
		GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
		GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_ERR_LONGITUDE);
		GUI_SetProperty("wnd_sat_list_popup", "draw_now", NULL);

		GxCore_ThreadDelay(1200);
		
		GUI_SetProperty("edit_satlist_popup_longitude", "clear", NULL);
		sprintf(buffer, "%03d.%d", s_CurSat.sat_data.sat_s.longitude/10, s_CurSat.sat_data.sat_s.longitude%10);
		GUI_SetProperty("edit_satlist_popup_longitude", "string", buffer);
		
		GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
		GUI_SetProperty("btn_satlist_popup_cancel", "state", "show");
		GUI_SetProperty("text_satlist_popup_tip1", "state", "hide");
		GUI_SetProperty("box_satlist_popup_para", "state", "show");
		GUI_SetProperty("wnd_sat_list_popup", "draw_now", NULL);
		GUI_SetFocusWidget("box_satlist_popup_para");

		return GXCORE_ERROR;
	}


	GUI_GetProperty("cmb_satlist_popup_direct", "select", &sel);
	if(0 == sel)
		direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
	else
		direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST;

	NodeModify.node_type = NODE_SAT;
	memcpy(&NodeModify.sat_data, &s_CurSat.sat_data, sizeof(GxBusPmDataSat));
	memset((char *)NodeModify.sat_data.sat_s.sat_name, 0, MAX_SAT_NAME);
	memcpy((char *)NodeModify.sat_data.sat_s.sat_name, name_str, MAX_SAT_NAME - 1);
	NodeModify.sat_data.sat_s.longitude = longitude;
	NodeModify.sat_data.sat_s.longitude_direct = direct;

#if DOUBLE_S2_SUPPORT
	GUI_GetProperty("cmb_satlist_popup_input_select", "select", &sel);
	NodeModify.sat_data.tuner = sel;// 0:tuner; 1:tuner2
	s_CurSat.sat_data.tuner = sel;
#endif

	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &NodeModify))
	{
#if DOUBLE_S2_SUPPORT
              // for view mode backup
              GxBusPmViewInfo view_info_bakup = {0};
              GxBusPmViewInfo view_info_temp = {0};
	       uint32_t ModifyFlag = 0;
		uint32_t FlagForChangeViewMode = 0;
		   
              app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_bakup)); 
	       memcpy(&view_info_temp,&view_info_bakup,sizeof(GxBusPmViewInfo));
		
	       // step one : all tv program
GHANGE_MODE:
		if(FlagForChangeViewMode == 0)
		{
               	view_info_temp.stream_type = GXBUS_PM_PROG_TV;
			view_info_temp.group_mode = 0;//all
			view_info_temp.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
			app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info_temp));
		}
		else
		{
			view_info_temp.stream_type = GXBUS_PM_PROG_RADIO;
			view_info_temp.group_mode = 0;//all
			view_info_temp.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
			app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info_temp));

		}
		FlagForChangeViewMode++;
		GxMsgProperty_NodeNumGet NumbGet = {0};
		NumbGet.node_type = NODE_PROG;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &NumbGet);
		if(NumbGet.node_num)
		{
			uint32_t i = 0;
			GxMsgProperty_NodeByPosGet ProgramNode = {0};
			for(i = 0; i < NumbGet.node_num; i++)
			{
				ProgramNode.node_type = NODE_PROG;
				ProgramNode.pos = i;
				app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgramNode);
				if(NodeModify.sat_data.id == ProgramNode.prog_data.sat_id)
				{
					if(ProgramNode.prog_data.tuner == sel)
						break;
					else
					{
						GxMsgProperty_NodeModify node_modify = {0};
						node_modify.node_type = NODE_PROG;
						node_modify.prog_data = ProgramNode.prog_data;
						node_modify.prog_data.tuner = sel;
						if(GXCORE_SUCCESS != GxBus_PmProgInfoModify(&(node_modify.prog_data)))
						{
						       printf("\nmodify TUNER error\n");
							break;
						}
						ModifyFlag = 1;
						#if 0
						if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
						{
							printf("\nmodify TUNER error\n");
							break;
						}
						#endif
					}
				}
			}
		}

		if(FlagForChangeViewMode < 2)
			goto GHANGE_MODE;
		
		if(ModifyFlag)
				GxBus_PmSync(GXBUS_PM_SYNC_PROG);

		// recover the view mode
		app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info_bakup));
		app_ioctl(s_CurSat.sat_data.tuner, FRONTEND_DEMUX_CFG, NULL);
#endif		
		s_Changed = 1;
	}

	return GXCORE_SUCCESS;
}

static status_t app_satlist_sat_add(void)
{
    GxMsgProperty_NodeAdd NodeAdd;
    char *name_str = NULL;
    char *atoi_str = NULL;
	int longitude = 0;
    GxBusPmDataSatLongitudeDirect direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
    uint32_t sel = 0;

    //name
    GUI_GetProperty("btn_sat_list_name", "string", &name_str);
    if(app_satlist_sat_check_name(name_str, 0) == GXCORE_ERROR)
    {
		GUI_SetProperty("box_satlist_popup_para", "state", "hide");
		GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
		GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
		GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
		GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_SAT_EXIT);
		GUI_SetInterface("flush",NULL);
		
		GxCore_ThreadDelay(1000);
		
		GUI_SetProperty("box_satlist_popup_para", "state", "show");
		GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
		GUI_SetProperty("btn_satlist_popup_cancel", "state", "show");
		GUI_SetProperty("text_satlist_popup_tip1", "state", "hide");
		GUI_SetFocusWidget("box_satlist_popup_para");
		GUI_SetInterface("flush",NULL);
		
		return GXCORE_ERROR;
	}
	
	//longitude
	GUI_GetProperty("edit_satlist_popup_longitude", "string", &atoi_str);
	longitude = 10 * app_float_edit_str_to_value(atoi_str);
	if(longitude > 1800)
	{
		//hide the box
		GUI_SetProperty("box_satlist_popup_para", "state", "hide");
		GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
		GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
		GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
		GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_ERR_LONGITUDE);
		GUI_SetProperty("wnd_sat_list_popup", "draw_now", NULL);

		GxCore_ThreadDelay(1200);
		
		GUI_SetProperty("edit_satlist_popup_longitude", "clear", NULL);
		GUI_SetProperty("edit_satlist_popup_longitude", "string", "000.0");

		GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
		GUI_SetProperty("btn_satlist_popup_cancel", "state", "show");
		GUI_SetProperty("text_satlist_popup_tip1", "state", "hide");
		GUI_SetProperty("box_satlist_popup_para", "state", "show");
		GUI_SetProperty("wnd_sat_list_popup", "draw_now", NULL);
		GUI_SetFocusWidget("box_satlist_popup_para");

		return GXCORE_ERROR;
	}

	GUI_GetProperty("cmb_satlist_popup_direct", "select", &sel);
	if(0 == sel)
		direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
	else
		direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST;

	memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));
	NodeAdd.node_type = NODE_SAT;
    	NodeAdd.sat_data.type = GXBUS_PM_SAT_S;
    	NodeAdd.sat_data.tuner = 0;
    	NodeAdd.sat_data.sat_s.lnb1 = 5150;
    	NodeAdd.sat_data.sat_s.lnb2 = 5750;
    	NodeAdd.sat_data.sat_s.lnb_power = GXBUS_PM_SAT_LNB_POWER_ON;
    	NodeAdd.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_OFF;
    	NodeAdd.sat_data.sat_s.diseqc11 = 0;
   	NodeAdd.sat_data.sat_s.diseqc12_pos = DISEQ12_USELESS_ID;
    	NodeAdd.sat_data.sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_1_0;
    	NodeAdd.sat_data.sat_s.diseqc10 = 0;
    	NodeAdd.sat_data.sat_s.switch_12V = GXBUS_PM_SAT_12V_OFF;
    	NodeAdd.sat_data.sat_s.longitude_direct = direct;
    	NodeAdd.sat_data.sat_s.reserved = 0;
    	NodeAdd.sat_data.sat_s.longitude = longitude;
    	memset((char *)NodeAdd.sat_data.sat_s.sat_name, 0, MAX_SAT_NAME);
	memcpy((char *)NodeAdd.sat_data.sat_s.sat_name, name_str, MAX_SAT_NAME -1);
	#if UNICABLE_SUPPORT
	{
	int index = 0;
	//centre fre
	for(index=0;index<8;index++)
	{
		NodeAdd.sat_data.sat_s.unicable_para.centre_fre[index] = 0;
	}
	NodeAdd.sat_data.sat_s.unicable_para.lnb_fre_index = 32;//0;
	NodeAdd.sat_data.sat_s.unicable_para.if_channel = 0;
	NodeAdd.sat_data.sat_s.unicable_para.sat_pos = 0;
	}
	#endif


#if DOUBLE_S2_SUPPORT
	GUI_GetProperty("cmb_satlist_popup_input_select", "select", &sel);
	NodeAdd.sat_data.tuner = sel;
#endif
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
	    	{
	    		s_Changed = 1;
	    	}
    	
	return GXCORE_SUCCESS;
}

static status_t app_satlist_sat_del(void)
{
    GxMsgProperty_NodeDelete node= {0};
    status_t ret = GXCORE_ERROR;
	
	node.node_type = NODE_SAT;
	node.num = s_SatIdOps->id_total_get(s_SatIdOps);
	if(node.num > 0)
	{
		node.id_array = GxCore_Malloc(node.num*sizeof(uint32_t));
		if(node.id_array != NULL)
		{
			s_SatIdOps->id_get(s_SatIdOps, (uint32_t*)(node.id_array));
			
			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node))
			{
				s_Changed = 1;
				g_AppBook.invalid_remove();
				ret = GXCORE_SUCCESS;
            }

            GxCore_Free(node.id_array);
        }
    }
    return ret;
}

static status_t app_satlist_popup_ok_keypress(void)
{
    status_t ret = GXCORE_SUCCESS;
    switch(s_CurMode)
    {
        case SAT_ADD:
            ret = app_satlist_sat_add();
            break;

        case SAT_EDIT:
            if(s_satlist_sat_total > 0)
			{
				ret = app_satlist_sat_edit();
			}
			break;

		case SAT_DELETE:
		{
			if(s_satlist_sat_total > 0 
			&& (0 == strcasecmp("btn_satlist_popup_ok", GUI_GetFocusWidget()))
)
			{
				GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_DELETING);
				GUI_SetProperty("text_satlist_popup_tip2", "string", NULL);
				GUI_SetProperty("img_satlist_popup_ok_back", "state", "hide");
				GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
				GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
				GUI_SetProperty("wnd_sat_list_popup", "draw_now", NULL);
				ret = app_satlist_sat_del();
			}
			break;
		}	

		default:
			break;
	}
	return ret;
}

SIGNAL_HANDLER int app_satlist_popup_longitude_edit_change(GuiWidget *widget, void *usrdata)
{
	int longitude = 0;
	char *atoi_str = NULL;
	char buffer[10] = {0};
	
	

	//longitude
	GUI_GetProperty("edit_satlist_popup_longitude", "string", &atoi_str);
	longitude = 10 * app_float_edit_str_to_value(atoi_str);
	if(longitude > 1800)
	{
		//hide the box
		longitude = 1800;
		GUI_SetProperty("box_satlist_popup_para", "state", "hide");
		GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
		GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
		GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
		GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_ERR_LONGITUDE);
		GUI_SetProperty("wnd_sat_list_popup", "draw_now", NULL);

		GxCore_ThreadDelay(1200);
		
		GUI_SetProperty("edit_satlist_popup_longitude", "clear", NULL);
		sprintf(buffer, "%03d.%d", s_CurSat.sat_data.sat_s.longitude/10, s_CurSat.sat_data.sat_s.longitude%10);
		GUI_SetProperty("edit_satlist_popup_longitude", "string", buffer);
		 
	// GUI_SetProperty("edit_satlist_popup_longitude", "string", "000.0");

		GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
		GUI_SetProperty("btn_satlist_popup_cancel", "state", "show");
		GUI_SetProperty("text_satlist_popup_tip1", "state", "hide");
		GUI_SetProperty("box_satlist_popup_para", "state", "show");
		GUI_SetProperty("wnd_sat_list_popup", "draw_now", NULL);
		GUI_SetFocusWidget("box_satlist_popup_para");
	}
	
	//memset(buff, 0, sizeof(buff));
	//sprintf(buff, "%03d.%d", longitude/10, longitude%10);
	//GUI_SetProperty("edit_positioner_longitude", "string", buff);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satlist_popup_longitude_edit_reachend(GuiWidget *widget, void *usrdata)
{
	uint32_t sel = 0;
	
	GUI_SetProperty("edit_satlist_popup_longitude", "select", &sel);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satlist_popup_create(GuiWidget *widget, void *usrdata)
{
#define DIRECT_CONTENT	"[E,W]"
#define STR_LEN	20

	char buffer[STR_LEN];
	uint32_t sel=0;
		
	switch(s_CurMode)
	{
		case SAT_ADD:
		{
			//title
			GUI_SetProperty("text_satlist_popup_title", "string", STR_ID_ADD);
			//name
			GUI_SetProperty("btn_sat_list_name", "string", "New Sat");
			//longitude
			GUI_SetProperty("edit_satlist_popup_longitude","string","000.0");
			//direction
			GUI_SetProperty("cmb_satlist_popup_direct", "content", DIRECT_CONTENT);
			//hide
			GUI_SetProperty("img_satlist_popup_ok_back", "state", "hide");
			GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
			GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");

			GUI_SetProperty("box_satlist_popup_para", "state", "show");
			GUI_SetFocusWidget("box_satlist_popup_para");

		#if DOUBLE_S2_SUPPORT
            GUI_SetProperty("text_satlist_popup_input_chennel","string", "Tuner");
            GUI_SetProperty("cmb_satlist_popup_input_select","content", "[T1,T2]");
        #else
            // for tuner select show control, if not support the double tuner
			GUI_SetProperty("text_satlist_popup_input_chennel","string", "");
			GUI_SetProperty("cmb_satlist_popup_input_select","content", "[]");
			GUI_SetProperty("boxitem_satplist_popup_input_sel","state", "disable");
		#endif
			
		}
		break;

		case SAT_EDIT:
		{
			//title
			GUI_SetProperty("text_satlist_popup_title", "string", STR_ID_EDIT);

			if(s_satlist_sat_total > 0)
			{
				//name
				GUI_SetProperty("btn_sat_list_name", "string", s_CurSat.sat_data.sat_s.sat_name);
				//longitude
				memset(buffer, 0, sizeof(buffer));
				sprintf(buffer, "%03d.%d", s_CurSat.sat_data.sat_s.longitude/10, s_CurSat.sat_data.sat_s.longitude%10);
				GUI_SetProperty("edit_satlist_popup_longitude", "string", buffer);
				//direction
				GUI_SetProperty("cmb_satlist_popup_direct", "content", DIRECT_CONTENT);
				sel = s_CurSat.sat_data.sat_s.longitude_direct;
				GUI_SetProperty("cmb_satlist_popup_direct", "select", &sel);
				// tuner
			#if DOUBLE_S2_SUPPORT
				sel = s_CurSat.sat_data.tuner;
                GUI_SetProperty("cmb_satlist_popup_input_select","content", "[T1,T2]");
				GUI_SetProperty("cmb_satlist_popup_input_select", "select", &sel);
                GUI_SetProperty("text_satlist_popup_input_chennel","string", "Tuner");
			#else
				// for tuner select show control, if not support the double tuner
                GUI_SetProperty("text_satlist_popup_input_chennel","string", "");
                GUI_SetProperty("cmb_satlist_popup_input_select","content", "[]");
                GUI_SetProperty("boxitem_satplist_popup_input_sel","state", "disable");
			#endif
				//hide
				GUI_SetProperty("img_satlist_popup_ok_back", "state", "hide");
				GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
				GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");

				GUI_SetProperty("box_satlist_popup_para", "state", "show");
				GUI_SetFocusWidget("box_satlist_popup_para");
			}
			else
			{
				GUI_SetProperty("box_satlist_popup_para", "state", "hide");
				GUI_SetProperty("text_satlist_popup_tip1", "string", STR_SAT_NOT_EXIST);
			}
		}
		break;

		case SAT_DELETE:
		{
			uint32_t opt_num = 0;
			//title
			GUI_SetProperty("text_satlist_popup_title", "string", "Delete");
			//hide the box
			GUI_SetProperty("box_satlist_popup_para", "state", "hide");
			GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
			GUI_SetProperty("btn_satlist_popup_cancel", "state", "show");
			GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
			GUI_SetProperty("text_satlist_popup_tip2", "state", "show");
			
			opt_num = s_SatIdOps->id_total_get(s_SatIdOps);
			if((s_satlist_sat_total > 0) && (opt_num > 0))
			{
				if(1 == opt_num)//only one
				{
					uint32_t id[1] = {0};
					GxMsgProperty_NodeByIdGet sat_node = {0};
					
					s_SatIdOps->id_get(s_SatIdOps, id);
					sat_node.node_type = NODE_SAT;
					sat_node.id = id[0];
					app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node);

					GUI_SetProperty("text_satlist_popup_tip1", "string", STR_DLE_ONE_SAT);
					//show sat name
					GUI_SetProperty("text_satlist_popup_tip2", "string", sat_node.sat_data.sat_s.sat_name);
					
					GUI_SetFocusWidget("btn_satlist_popup_ok");
					GUI_SetProperty("img_satlist_popup_ok_back","img","s_bar_choice_blue4.bmp");
				}
				else//del selected sat
				{
					GUI_SetProperty("text_satlist_popup_tip1", "string", STR_DLE_MULTI_SAT);
					GUI_SetFocusWidget("btn_satlist_popup_ok");
					GUI_SetProperty("img_satlist_popup_ok_back","img","s_bar_choice_blue4.bmp");
				}
			}
			else
			{
				GUI_SetProperty("text_satlist_popup_tip1", "string", STR_SAT_NOT_EXIST);
			}
		}
		break;

		case SAT_SEARCH:
		{
			//title
			GUI_SetProperty("text_satlist_popup_title", "string", "SAT Search");
			//hide the box
			GUI_SetProperty("box_satlist_popup_para", "state", "hide");

			GUI_SetProperty("text_satlist_popup_tip1", "string", STR_SAT_NOT_EXIST);
		}
		
		default:
			break;

	}
	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satlist_popup_destroy(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_NodeNumGet node_num = {0};
	node_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	s_satlist_sat_total = node_num.node_num;
	if(s_satlist_sat_total > 0)
	{
		sat_list_show_help_info();
	}
	else
	{
		sat_list_hide_help_info(S_INFO_ADD);
	}
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_satlist_popup_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
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
					GUI_EndDialog("after wnd_full_screen");
					ret = EVENT_TRANSFER_STOP;
				break;

				case STBK_EXIT:
				case STBK_MENU:
					{
						GUI_EndDialog("wnd_sat_list_popup");
					}
					break;

				case STBK_OK:
					if((app_satlist_popup_ok_keypress() == GXCORE_SUCCESS)
						|| (s_satlist_sat_total == 0))
					{
						GUI_EndDialog("wnd_sat_list_popup");
						//GUI_SetProperty("wnd_sat_list_popup", "draw_now", NULL);
					}
					break;
				case STBK_RIGHT:
				case STBK_LEFT:
					{
						if(s_CurMode == SAT_DELETE)
						{
							if(0 == strcasecmp("btn_satlist_popup_ok", GUI_GetFocusWidget()))
							{
								FocusdOnOk = 1;
							}
							else if(0 == strcasecmp("btn_satlist_popup_cancel", GUI_GetFocusWidget()))
							{
								FocusdOnOk = 0;
							}

							if(1 == FocusdOnOk)
							{
								GUI_SetProperty("img_satlist_popup_ok_back","img","s_bar_choice_blue4_l.bmp");
								GUI_SetFocusWidget("btn_satlist_popup_cancel");
							}
							else
							{
								GUI_SetProperty("img_satlist_popup_ok_back","img","s_bar_choice_blue4.bmp");
								GUI_SetFocusWidget("btn_satlist_popup_ok");							
							}
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


SIGNAL_HANDLER int app_satlist_popup_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	uint32_t box_sel;

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

				case STBK_UP:
					{
						GUI_GetProperty("box_satlist_popup_para", "select", &box_sel);
						if(SAT_LIST_POPUP_BOX_ITEM_NAME == box_sel)
						{
							box_sel = SAT_LIST_POPUP_BOX_ITEM_OK;
							GUI_SetProperty("box_satlist_popup_para", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
					#if (DOUBLE_S2_SUPPORT == 0)// not support double tuner input
						else if(SAT_LIST_POPUP_BOX_ITEM_OK == box_sel)
						{
							box_sel = SAT_LIST_POPUP_BOX_ITEM_DIRECT;
							GUI_SetProperty("box_satlist_popup_para", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
					#endif
						
					}
					break;

				case STBK_DOWN:					
					{
						GUI_GetProperty("box_satlist_popup_para", "select", &box_sel);
						if(SAT_LIST_POPUP_BOX_ITEM_OK == box_sel)
						{
							box_sel = SAT_LIST_POPUP_BOX_ITEM_NAME;
							GUI_SetProperty("box_satlist_popup_para", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
					#if (DOUBLE_S2_SUPPORT == 0)// not support double tuner input
						else if(SAT_LIST_POPUP_BOX_ITEM_DIRECT == box_sel)
						{
							box_sel = SAT_LIST_POPUP_BOX_ITEM_OK;
							GUI_SetProperty("box_satlist_popup_para", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
					#endif
					}
					break;

				case STBK_LEFT:
				case STBK_RIGHT:
					
					break;

				case STBK_OK:
					{
						GUI_GetProperty("box_satlist_popup_para", "select", &box_sel);
						if(SAT_LIST_POPUP_BOX_ITEM_NAME == box_sel)
						{
#define WND_KEYBOARD_X 360 
#define WND_KEYBOARD_Y 130
							static PopKeyboard keyboard;
							memset(&keyboard, 0, sizeof(PopKeyboard));
							keyboard.pos.x = WND_KEYBOARD_X;
							keyboard.pos.y = WND_KEYBOARD_Y;
							GUI_GetProperty("btn_sat_list_name", "string", &keyboard.in_name);
							keyboard.max_num = MAX_SAT_NAME - 1;
							keyboard.out_name   = NULL;
							keyboard.change_cb  = NULL;
                            keyboard.release_cb = _sat_rename_release_cb;
                            keyboard.usr_data   = KEY_LAN_INVALID_CHAR_NAME;//sat name is not ','
                            multi_language_keyboard_create(&keyboard);
                            ret = EVENT_TRANSFER_STOP;
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

SIGNAL_HANDLER int app_satlist_popup_edit_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
	uint32_t reach_sel = 0;
	int longitude = 0;
	GUI_GetProperty("edit_satlist_popup_longitude", "select", &reach_sel);
	char* atoi_str = NULL;
	GUI_GetProperty("edit_satlist_popup_longitude", "string", &atoi_str);
	longitude = 10 * app_float_edit_str_to_value(atoi_str);
//	printf("file:%s-------line:%d--------reach_sel=%d longitude = %d \n",__FILE__,__LINE__,reach_sel,longitude);
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
				case STBK_1:
					if(reach_sel >1 && longitude >= 1800)
					{
						ret = EVENT_TRANSFER_STOP;
					}
					if(reach_sel == 0 && (longitude > 800 && longitude < 1000))
					{
						 ret = EVENT_TRANSFER_STOP;
					}
					break;
				case STBK_2:
					{
						if(reach_sel == 0)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						if(reach_sel >1 && longitude >= 1800)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						break;
					}
				case STBK_3:
					{
						if(reach_sel == 0)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						if(reach_sel >1 && longitude >= 1800)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						break;
					}
				case STBK_4:
					{
						if(reach_sel == 0)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						if(reach_sel >1 && longitude >= 1800)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						break;
					}

				case STBK_5:
					{
						if(reach_sel == 0)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						if(reach_sel >1 && longitude >= 1800)
						{
							ret = EVENT_TRANSFER_STOP;
						}
					
						break;
					}

				case STBK_6:
					{
						if(reach_sel == 0)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						if(reach_sel >1 && longitude >= 1800)
						{
							ret = EVENT_TRANSFER_STOP;
						}
					
						break;
					}

				case STBK_7:
					{
						if(reach_sel == 0)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						if(reach_sel >1 && longitude >= 1800)
						{
							ret = EVENT_TRANSFER_STOP;
						}
					
						break;
					}

				case STBK_8:
					{
						if(reach_sel == 0)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						if(reach_sel == 1 && longitude > 1000 && longitude%100 != 0)
						{
							 ret = EVENT_TRANSFER_STOP;
						}
						if(reach_sel >1 && longitude >= 1800)
						{
							ret = EVENT_TRANSFER_STOP;
						}
					
						break;
					}

				case STBK_9:
					{
						if(reach_sel == 0)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						if(reach_sel == 1 && longitude >=1000)
						{
							ret = EVENT_TRANSFER_STOP;
						}
						if(reach_sel >1 && longitude >= 1800)
						{
							ret = EVENT_TRANSFER_STOP;
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

#else //DEMOD_DVB_S

SIGNAL_HANDLER int app_sat_list_create(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_sat_list_destroy(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_sat_list_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_sat_list_lost_focus(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_sat_list_got_focus(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_sat_list_list_create(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_sat_list_list_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_sat_list_list_get_total(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_sat_list_list_get_data(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_sat_list_list_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_satlist_popup_create(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_satlist_popup_destroy(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_satlist_popup_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_satlist_popup_box_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_satlist_popup_edit_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_satlist_popup_longitude_edit_change(GuiWidget *widget, void *usrdata)
{	return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_satlist_popup_longitude_edit_reachend(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

#endif
