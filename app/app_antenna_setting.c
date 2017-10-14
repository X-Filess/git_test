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
#if TKGS_SUPPORT
#include "module/gxtkgs.h"
#endif
extern void app_set_info_cursel(uint32_t sel);
uint32_t app_tuner_cur_tuner_get(void);
uint32_t app_tuner_sat_num_get(uint32_t tuner);
/* Private variable------------------------------------------------------- */

enum BOX_ITEM_NAME 
{
	BOX_ITEM_SAT = 0,
	BOX_ITEM_LNB_POWER,
	BOX_ITEM_LNB_FREQ,
	BOX_ITEM_22K,
	BOX_ITEM_DISEQC10,
	BOX_ITEM_DISEQC11,
	BOX_ITEM_TP,
	BOX_ITEM_SEARCH
};

enum
{
	SEARCH_SEL_SAT,
	SEARCH_SEL_BLIND,
	SEARCH_SEL_TP
};

SatSelStatus satsel;
bool scan_mode_disable_flag = false;//True:Disable the first boxitem
static GxMsgProperty_NodeByPosGet s_CurAtnSat = {0};
static uint32_t* sp_SatBuf = NULL;// save sat pos
static uint32_t s_antenna_sat_total = 0;

static GxMsgProperty_NodeByPosGet s_CurAtnTp = {0};
//static uint32_t* sp_TpBuf = NULL;// save tp pos
static uint32_t s_antenna_tp_total = 0;
static uint32_t s_antenna_tp_tp_change = 0;


static event_list* sp_AtnTimerSignal = NULL;
static event_list* sp_AtnLockTimer = NULL;
/* widget macro -------------------------------------------------------- */
#define IMG_BAR_LONG_FOCUS "s_bar_long_focus.bmp"
#define IMG_BAR_LONG_UNFOCUS ""
#define IMG_BAR_OK_FOCUS   "s_button_ok.bmp"

#define UNIVERSAL_NUM (3)

#define LNB_POWER_CONTENT	"[Off,On,13V,18V]"
#if UNICABLE_SUPPORT
#define LNB_FREQ_CONTENT	"[Universal1(9750/10600),Universal2(9750/10700),Universal3(9750/10750),OCS1(5150/5750),OCS2(5750/5150),5150,5750,5950,9750,10000,10600,10700,10750,11250,11300,Unicable]"
#else
#define LNB_FREQ_CONTENT	"[Universal1(9750/10600),Universal2(9750/10700),Universal3(9750/10750),OCS1(5150/5750),OCS2(5750/5150),5150,5750,5950,9750,10000,10600,10700,10750,11250,11300]"
#endif
#define ANTENNA_22K_CONTENT			"[Off,On]"
#define ANTENNA_22K_AUTO_CONTENT	"[Auto]"
#define DISEQC10_CONTENT	"[Off,Port 1,Port 2,Port 3,Port 4]"
#define DISEQC11_CONTENT	"[Off,Port 1,Port 2,Port 3,Port 4,Port 5,Port 6,Port 7,Port 8,Port 9,Port 10,Port 11,Port 12,Port 13,Port 14,Port 15,Port 16]"
#define ANTENNA_SEARCH_CONTENT	"[Satellite,Blind,TP]"
#define ANTENNA_SAT_SEARCH_CONTENT	"[Blind]"
#if UNICABLE_SUPPORT
#define ANTENNA_SAT_SEARCH_CONTENT_UNICABLE	"[Satellite,TP]"
#endif

static char* s_LnbPower[4] = {STR_ID_OFF,STR_ID_ON,"13V","18V"};
static char* s_22k[2] = {STR_ID_OFF,STR_ID_ON};
static char* s_Diseqc10[5] = {STR_ID_OFF, "Port 1", "Port 2", "Port 3", "Port 4"};
#if UNICABLE_SUPPORT
#define UNICABLE_POSITION_NUM		15
static char* s_LnbFreq[] = {"9750/10600","9750/10700",
    "9750/10750","5150/5750","5750/5150",//OCS2
        "5150","5750","5950","9750","10000","10600","10700","10750","11250","11300","Unicable"};
#else
static char* s_LnbFreq[15] = {"9750/10600","9750/10700",
    "9750/10750","5150/5750","5750/5150",//OCS2
        "5150","5750","5950","9750","10000","10600","10700","10750","11250","11300"};//Universal1
#endif

static char* s_Diseqc11[17] = {STR_ID_OFF, "Port 1", "Port 2", "Port 3", "Port 4", "Port 5", "Port 6", "Port 7", "Port 8", "Port 9", "Port 10", "Port 11", "Port 12","Port 13", "Port 14", "Port 15", "Port 16"};

static char * s_antenna_boxitem[] = 
{
	"boxitem_antenna_satellite",
	"boxitem_antenna_lnbpower",
	"boxitem_antenna_lnbfreq",
	"boxitem_antenna_22k",
	"boxitem_antenna_diseqc10",
	"boxitem_antenna_diseqc11",
	"boxitem_antenna_tp",
	"boxitem_antenna_search",
};

static char * s_antenna_ok_combox[] =
{
	"cmb_antenna_satellite_choice",
	"cmb_antenna_lnb_power_choice",
	"cmb_antenna_lnb_freq_choice",
	"cmb_antenna_22k_choice",
	"cmb_antenna_diseqc10_choice",
	"cmb_antenna_diseqc11_choice",
	"cmb_antenna_tp_choice",
	"cmb_antenna_autoscan_choice",
};

#define ANTENNA_SETTING_HELP1_IMG		"img_antenna_setting_tip_red"
#define ANTENNA_SETTING_HELP1_TXT		"text_antenna_setting_tip_red"
#define ANTENNA_SETTING_HELP2_IMG		"img_antenna_setting_tip_green"
#define ANTENNA_SETTING_HELP2_TXT		"text_antenna_setting_tip_green"

static SetTpMode s_set_tp_mode = SET_TP_AUTO;
//static uint32_t s_nFreOldSelect = 0;
static uint32_t s_n22KSelectRecord = 0;

static void _antenna_sat_para_save(void);
static void _antenna_set_diseqc(void);
static void _antenna_set_tp(SetTpMode set_mode);


#if TKGS_SUPPORT
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
extern bool app_tkgs_check_sat_is_turish(GxMsgProperty_NodeByPosGet * p_nodesat);
extern void app_tkgs_cantwork_popup(void);


static int app_anternna_tkgs_key_handler(void)
{
	if(app_tkgs_check_sat_is_turish(&s_CurAtnSat))
	{
		app_tkgs_cantwork_popup();
		return TRUE;
	}
	return FALSE;
}

#endif
// create dialog
int app_antenna_setting_init(int SatId, int TpSel ,int sat_SatSel)
{
    if(GXCORE_SUCCESS != GUI_CheckDialog("wnd_antenna_setting"))
    {
        GUI_CreateDialog("wnd_antenna_setting");
    }

    if((SatId < 0)&&(TpSel < 0)) //sat_list STBK_INFO
    {
        if(sat_SatSel >= 0)
        {
            GUI_SetProperty("cmbox_antenna_sat", "select", &sat_SatSel);
            //GUI_SetProperty("cmbox_antenna_tp", "select", &sat_TpSel);
        }
        return 0;
    }

    if(SatId >= 0)
    {
        GUI_SetProperty("cmbox_antenna_sat", "select", &SatId);
    }
    if(TpSel >= 0)
        GUI_SetProperty("cmbox_antenna_tp", "select", &TpSel);
    return 0;
}


//tp change control
void app_antenna_tp_change_set(uint32_t SatSel)
{
	GxMsgProperty_NodeByPosGet NodeSat = {0};
	
	if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_antenna_setting"))
	{
		NodeSat.node_type = NODE_SAT;
		NodeSat.pos = SatSel;// all sel
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &NodeSat);
	
		if (0 == memcmp(&(NodeSat.sat_data),&(s_CurAtnSat.sat_data),sizeof(GxBusPmDataSat)))
			s_antenna_tp_tp_change = 1;
	}
}

void app_sat_list_change_sel(uint32_t sat_sel)
{
	static GxMsgProperty_NodeByPosGet node;
    uint32_t i = 0;
    uint32_t num_t1 = 0;
    uint32_t num_t2 = 0;

    satsel.sel_in_all = sat_sel;
    for(i = 0; i < sat_sel+1; i++)
    {
        node.node_type = NODE_SAT;
        node.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
        if(node.sat_data.tuner)//T2
            num_t2++;
        else//T1
            num_t1++;
    }

    if(node.sat_data.tuner)//id:sat_sel
    {
        satsel.sel_in_t2 = satsel.sel_in_all - num_t1;
        satsel.sel_in_t1 = 0;
    }
    else
    {
        satsel.sel_in_t1 = satsel.sel_in_all - num_t2;
        satsel.sel_in_t2 = 0;
    }
}

void app_sat_sel_clear(void)
{
    satsel.sel_in_all = 0;
    satsel.sel_in_t1 = 0;
    satsel.sel_in_t2 = 0;
}

static uint32_t app_sat_sel_get(uint32_t tuner)
{
	if(tuner == 1)
		return satsel.sel_in_t2;
	else if(tuner == 0)
		return satsel.sel_in_t1;
	else
		return satsel.sel_in_all;
}

static void app_antenna_tp_change_clear(void)
{
	s_antenna_tp_tp_change = 0;
}

static uint32_t  app_antenna_tp_change_check(void)
{
	return s_antenna_tp_tp_change;
}

#define ANTENNA_TIMER
static int app_antenna_clean_signal(void)
{
    unsigned int value = 0;
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};

    // progbar strength
    siganl_bar.strength_bar = "progbar_antenna_strength";
    siganl_val.strength_val = value;
    GUI_SetProperty("text_antenna_strength_value", "string", " ");

    siganl_bar.quality_bar= "progbar_antenna_quality";
    siganl_val.quality_val = value;
    GUI_SetProperty("text_antenna_quality_value", "string", " ");

    app_signal_progbar_update(app_tuner_cur_tuner_get(), &siganl_bar, &siganl_val);

    return 0;
}

static int timer_atn_show_signal(void *userdata)
{
    unsigned int value = 0;
    char Buffer[20] = {0};
    static unsigned char value_flag = 1;
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};

    if(s_antenna_tp_total > 0)
    {
        // progbar strength
        //g_AppNim.property_get(&g_AppNim, AppFrontendID_Strength,(void*)(&value),4);
        app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_STRENGTH_GET, &value);
		if(value > 99)
		{
			value = 100;
		}
        siganl_bar.strength_bar = "progbar_antenna_strength";
        siganl_val.strength_val = value;

        // strength value
        memset(Buffer,0,sizeof(Buffer));
        sprintf(Buffer,"%d%%",value);
        GUI_SetProperty("text_antenna_strength_value", "string", Buffer);

        // progbar quality
        //g_AppNim.property_get(&g_AppNim, AppFrontendID_Quality,(void*)(&value),4);
        app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_QUALITY_GET, &value);
		if(value > 99)
		{
			value = 100;
		}
        siganl_bar.quality_bar= "progbar_antenna_quality";
        siganl_val.quality_val = value;
        // quality value
        memset(Buffer,0,sizeof(Buffer));
        sprintf(Buffer,"%d%%",value);
        GUI_SetProperty("text_antenna_quality_value", "string", Buffer);

        app_signal_progbar_update(app_tuner_cur_tuner_get(), &siganl_bar, &siganl_val);

        ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &value);
	value_flag = 1;
    }
    else
    {
        value = 0;
        memset(Buffer,0,sizeof(Buffer));
        sprintf(Buffer,"%d%%",value);
        GUI_SetProperty("text_antenna_strength_value", "string", Buffer);
        GUI_SetProperty("text_antenna_quality_value", "string", Buffer);
        if(value_flag == 1)
        {
            value_flag = 0;
            GUI_SetProperty("progbar_antenna_strength", "value", &value);
            GUI_SetProperty("progbar_antenna_quality", "value", &value);
	}

        ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &value);
    }
    return 0;
}

static void timer_atn_show_signal_stop(void)
{
	timer_stop(sp_AtnTimerSignal);
}

static void timer_atn_show_signal_reset(void)
{
	reset_timer(sp_AtnTimerSignal);
}

static int timer_antenna_set_frontend(void *userdata)
{
	SetTpMode force = 0;
    
    if(NULL!=sp_AtnLockTimer)
    {
        remove_timer(sp_AtnLockTimer);
        sp_AtnLockTimer = NULL;
    }

	if(userdata != NULL)
	{
		force = *(SetTpMode*)userdata;
		if(force == SET_TP_FORCE)
		{
			GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
		}
	}
	{
		extern void GxFrontend_ForceExitEx(int32_t tuner);
#if DOUBLE_S2_SUPPORT
		GxFrontend_ForceExitEx(0);
		GxFrontend_ForceExitEx(1);
#else
		GxFrontend_ForceExitEx(s_CurAtnSat.sat_data.tuner);
#endif
	}
	_antenna_set_diseqc();
	_antenna_set_tp(force);

	return 0;
}

#define SUB_FUNC
static void _antenna_cmb_sat_rebuild(void)
{
#define BUFFER_LENGTH	(MAX_SAT_NAME+20)
	int i = 0;
	int sat_count = 0;
	char *buffer_all = NULL;
	char buffer[BUFFER_LENGTH] = {0};
	GxMsgProperty_NodeNumGet sat_num = {0};
	GxMsgProperty_NodeByPosGet node_sat = {0};

	//get total sat num in both tuner
	sat_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);

	s_antenna_sat_total = app_tuner_sat_num_get(app_tuner_cur_tuner_get());
	//rebuild sat list in combo box
	if(sp_SatBuf != NULL)
	{
		GxCore_Free(sp_SatBuf);
		sp_SatBuf = NULL;
	}
	//
	sp_SatBuf = GxCore_Malloc(sat_num.node_num * sizeof(uint32_t));
	if(sp_SatBuf == NULL)
	{
		printf("\nANTENNA SETTING, malloc failed...%s,%d\n",__FILE__,__LINE__);
		s_antenna_sat_total = 0;
		return ;
	}
	memset(sp_SatBuf, 0, sat_num.node_num * sizeof(uint32_t));
	buffer_all = GxCore_Malloc(sat_num.node_num * BUFFER_LENGTH);
	if(buffer_all == NULL)
	{
		printf("\nANTENNA SETTING, malloc failed...%s,%d\n",__FILE__,__LINE__);
		s_antenna_sat_total = 0;
		return ;
	}
	memset(buffer_all, 0, sat_num.node_num * BUFFER_LENGTH);

	//start build string
	strcat(buffer_all, "[");

	for(i = 0; i < sat_num.node_num; i++)
	{
		node_sat.node_type = NODE_SAT;
		node_sat.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);

		/*****************************************
		*sat filter by tuner!*/
		if (app_tuner_cur_tuner_get() != node_sat.sat_data.tuner)
		{
			continue;
		}
		/****************************************/
		sp_SatBuf[sat_count++] = i;

		if (sat_count == 1)
		// give the first value to s_CurAtnSat
			memcpy(&s_CurAtnSat, &node_sat, sizeof(GxMsgProperty_NodeByPosGet));

		if (i == satsel.sel_in_all)
		// give the selected sat value to s_CurAtnSat
			memcpy(&s_CurAtnSat, &node_sat, sizeof(GxMsgProperty_NodeByPosGet));

		memset(buffer, 0, BUFFER_LENGTH);
		sprintf(buffer,"(%d/%d) %s,", sat_count, s_antenna_sat_total,
				(char*)node_sat.sat_data.sat_s.sat_name);
		strcat(buffer_all, buffer);
	}

	// instead last "," to "]", and finish the string build
	memcpy(buffer_all+strlen(buffer_all)-1, "]", 1);
	GUI_SetProperty("cmbox_antenna_sat","content",buffer_all);
#if UNICABLE_SUPPORT
	if(s_CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[s_CurAtnSat.sat_data.sat_s.unicable_para.if_channel]==0)
	{
		s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index = 0x20;
	}
#endif
	GxCore_Free(buffer_all);

	return;
}

static void _antenna_cmb_tp_rebuild(void)
{
#define BUFFER_LENGTH_TP	(64)
	GxMsgProperty_NodeNumGet node_num = {0};
	int i = 0;
	char *buffer_all = NULL;
	char buffer[BUFFER_LENGTH_TP] = {0};

	// get tp number
	node_num.node_type = NODE_TP;
	node_num.sat_id = s_CurAtnSat.sat_data.id;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	s_antenna_tp_total = node_num.node_num;

	if(0 == s_antenna_tp_total)
	{
		// todo
		memset(&s_CurAtnTp, 0, sizeof(s_CurAtnTp));
		GUI_SetProperty("cmbox_antenna_tp","content","[NONE]");
	}
	else
	{
		buffer_all = GxCore_Malloc(s_antenna_tp_total * (BUFFER_LENGTH_TP));
		memset(buffer_all, 0, s_antenna_tp_total * (BUFFER_LENGTH_TP));

		//first one
		i = 0;
		s_CurAtnTp.node_type = NODE_TP;
		s_CurAtnTp.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurAtnTp);

		memset(buffer, 0, BUFFER_LENGTH_TP);
#if (DEMOD_DVB_T > 0)	
		if(GXBUS_PM_SAT_DVBT2 == s_CurAtnSat.sat_data.type)
		{
			char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};
			
			if(s_CurAtnTp.tp_data.tp_dvbt2.bandwidth <BANDWIDTH_AUTO )	
			sprintf(buffer,"[(%d/%d) %d.%d /%s", i+1, s_antenna_tp_total, s_CurAtnTp.tp_data.frequency/1000  \
				, (s_CurAtnTp.tp_data.frequency/100)%10,bandwidth_str[s_CurAtnTp.tp_data.tp_dvbt2.bandwidth]);
			
			strcat(buffer_all, buffer);
			if(s_antenna_tp_total > 1)
			{
				GxMsgProperty_NodeByPosGet node_tp = {0};
				for(i = 1; i < s_antenna_tp_total; i++)
				{
					node_tp.node_type = NODE_TP;
					node_tp.pos = i;
					app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
					memset(buffer, 0, BUFFER_LENGTH_TP);
					sprintf(buffer,",(%d/%d) %d.%d /%s", i+1, s_antenna_tp_total, node_tp.tp_data.frequency/1000  \
							, (node_tp.tp_data.frequency/100)%10,bandwidth_str[node_tp.tp_data.tp_dvbt2.bandwidth]);
					strcat(buffer_all, buffer);
				}
			}			
		}else
#endif
#if ( DEMOD_DVB_C > 0)
		if(GXBUS_PM_SAT_C == s_CurAtnSat.sat_data.type)
		{
			char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};
			
			if(s_CurAtnTp.tp_data.tp_c.modulation < QAM_AUTO)	
			sprintf(buffer,"[(%d/%d) %d /%s /%d", i+1, s_antenna_tp_total, s_CurAtnTp.tp_data.frequency/1000  \
				,modulation_str[s_CurAtnTp.tp_data.tp_c.modulation],s_CurAtnTp.tp_data.tp_c.symbol_rate/1000);
			
			strcat(buffer_all, buffer);
			if(s_antenna_tp_total > 1)
			{
				GxMsgProperty_NodeByPosGet node_tp = {0};
				for(i = 1; i < s_antenna_tp_total; i++)
				{
					node_tp.node_type = NODE_TP;
					node_tp.pos = i;
					app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
					memset(buffer, 0, BUFFER_LENGTH_TP);
					sprintf(buffer,",(%d/%d) %d /%s /%d", i+1, s_antenna_tp_total, node_tp.tp_data.frequency/1000  \
							, modulation_str[node_tp.tp_data.tp_c.modulation],node_tp.tp_data.tp_c.symbol_rate/1000);
					strcat(buffer_all, buffer);
				}
			}			
		}else
#endif
#if (DEMOD_DTMB > 0)
		if(GXBUS_PM_SAT_DTMB == s_CurAtnSat.sat_data.type)
		{
			char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};// no 7M?
			if(s_CurAtnSat.sat_data.sat_dtmb.work_mode == GXBUS_PM_SAT_1501_DTMB)
			{
				if(s_CurAtnTp.tp_data.tp_dtmb.symbol_rate < BANDWIDTH_AUTO )	
				sprintf(buffer,"[(%d/%d) %d.%d /%s", i+1, s_antenna_tp_total, s_CurAtnTp.tp_data.frequency/1000  \
					, (s_CurAtnTp.tp_data.frequency/100)%10,bandwidth_str[s_CurAtnTp.tp_data.tp_dtmb.symbol_rate]);
				
				strcat(buffer_all, buffer);
				if(s_antenna_tp_total > 1)
				{
					GxMsgProperty_NodeByPosGet node_tp = {0};
					for(i = 1; i < s_antenna_tp_total; i++)
					{
						node_tp.node_type = NODE_TP;
						node_tp.pos = i;
						app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
						memset(buffer, 0, BUFFER_LENGTH_TP);
						sprintf(buffer,",(%d/%d) %d.%d /%s", i+1, s_antenna_tp_total, node_tp.tp_data.frequency/1000  \
								, (node_tp.tp_data.frequency/100)%10,bandwidth_str[node_tp.tp_data.tp_dtmb.symbol_rate]);
						strcat(buffer_all, buffer);
					}
				}	
			}
		}else
#endif
        {
            if(s_CurAtnTp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
            {
                sprintf(buffer,"[(%d/%d) %d / %s / %d", i+1, s_antenna_tp_total, 
                        s_CurAtnTp.tp_data.frequency, "V" ,s_CurAtnTp.tp_data.tp_s.symbol_rate);
            }
            else if(s_CurAtnTp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
            {
                sprintf(buffer,"[(%d/%d) %d / %s / %d", i+1, s_antenna_tp_total, 
                        s_CurAtnTp.tp_data.frequency, "H" ,s_CurAtnTp.tp_data.tp_s.symbol_rate);
            }
            strcat(buffer_all, buffer);

            if(s_antenna_tp_total > 1)
            {
                GxMsgProperty_NodeByPosGet node_tp = {0};
                for(i = 1; i < s_antenna_tp_total; i++)
                {
                    node_tp.node_type = NODE_TP;
                    node_tp.pos = i;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
                    memset(buffer, 0, BUFFER_LENGTH_TP);
                    if(node_tp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
                    {
                        sprintf(buffer,",(%d/%d) %d / %s / %d", i+1, s_antenna_tp_total, 
                                node_tp.tp_data.frequency, "V" ,node_tp.tp_data.tp_s.symbol_rate);
                    }
                    else if(node_tp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
                    {
                        sprintf(buffer,",(%d/%d) %d / %s / %d", i+1, s_antenna_tp_total, 
                                node_tp.tp_data.frequency, "H" ,node_tp.tp_data.tp_s.symbol_rate);
                    }
                    strcat(buffer_all, buffer);
                }
            }
        }

		//only 1 sat or finished
		strcat(buffer_all, "]");
		GUI_SetProperty("cmbox_antenna_tp","content",buffer_all);
			
		GxCore_Free(buffer_all);
	}
	return;
}

static int _antenna_sat_pop(int sel)
{
	PopList pop_list;
	int ret =-1;
	int i;
	char **sat_name = NULL;
	GxMsgProperty_NodeByPosGet node;
	
	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_SATELLITE;
	pop_list.item_num = s_antenna_sat_total;

	sat_name = (char**)GxCore_Malloc(s_antenna_sat_total * sizeof(char*));
	if(sat_name != NULL)
	{
		for(i = 0; i < s_antenna_sat_total; i++)
		{
			node.node_type = NODE_SAT;
			node.pos = sp_SatBuf[i]; // map pos.
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
    pop_list.line_num = POPLIST_LINE_5;
    pop_list.pos.x= 680;
    pop_list.pos.y= 116;

	ret = poplist_create(&pop_list);

	if(sat_name != NULL)
	{
		for(i = 0; i < s_antenna_sat_total; i++)
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

static int _antenna_tp_pop(int sel)
{
#define BUFFER_LENGTH_TP	(64)

	PopList pop_list;
	int ret =-1;
	int i;
	char **tp_info = NULL;
	GxMsgProperty_NodeByPosGet node;
	
	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_TP;
	pop_list.item_num = s_antenna_tp_total;

	tp_info = (char**)GxCore_Malloc(s_antenna_tp_total * sizeof(char*));
	if(tp_info != NULL)
	{
		for(i = 0; i < s_antenna_tp_total; i++)
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
    pop_list.line_num= POPLIST_LINE_5;
    pop_list.pos.x= 680;
    pop_list.pos.y= 116;

	ret = poplist_create(&pop_list);

	if(tp_info != NULL)
	{
		for(i = 0; i < s_antenna_tp_total; i++)
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

static int _antenna_lnbpower_pop(int sel)
{
	PopList pop_list;
	int ret =-1;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_LNB_POWER;
	pop_list.item_num = sizeof(s_LnbPower)/sizeof(*s_LnbPower);
	pop_list.item_content = s_LnbPower;
	pop_list.sel = sel;
	pop_list.show_num= false;
    pop_list.line_num= POPLIST_LINE_3;
    pop_list.pos.x= 680;
    pop_list.pos.y= 116;

	ret = poplist_create(&pop_list);

	return ret;
}

static int _antenna_lnbfreq_pop(int sel)
{
	PopList pop_list;
	int ret =-1;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_LNB_FREQ;
	pop_list.item_num = sizeof(s_LnbFreq)/sizeof(*s_LnbFreq);
	pop_list.item_content = s_LnbFreq;
	pop_list.sel = sel;
	pop_list.show_num= false;
    pop_list.line_num= POPLIST_LINE_3;
    pop_list.pos.x= 680;
    pop_list.pos.y= 116;

	ret = poplist_create(&pop_list);

	return ret;
}

static int _antenna_22k_pop(int sel)
{
	PopList pop_list;
	int ret =-1;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = "22K";
	pop_list.item_num = sizeof(s_22k)/sizeof(*s_22k);
	pop_list.item_content = s_22k;
	pop_list.sel = sel;
	pop_list.show_num= false;
    pop_list.line_num= POPLIST_LINE_2;
    pop_list.pos.x= 680;
    pop_list.pos.y= 116;

	ret = poplist_create(&pop_list);

	return ret;
}

static int _antenna_diseqc10_pop(int sel)
{
	PopList pop_list;
	int ret =-1;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = "DiSEqC 1.0";
	pop_list.item_num = sizeof(s_Diseqc10)/sizeof(*s_Diseqc10);
	pop_list.item_content = s_Diseqc10;
	pop_list.sel = sel;
	pop_list.show_num= false;
    pop_list.line_num= POPLIST_LINE_3;
    pop_list.pos.x= 680;
    pop_list.pos.y= 116;

	ret = poplist_create(&pop_list);

	return ret;
}

static int _antenna_diseqc11_pop(int sel)
{
	PopList pop_list;
	int ret =-1;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = "DiSEqC 1.1";
	pop_list.item_num = sizeof(s_Diseqc11)/sizeof(*s_Diseqc11);
	pop_list.item_content = s_Diseqc11;
	pop_list.sel = sel;
	pop_list.show_num= false;
    pop_list.line_num= POPLIST_LINE_3;
    pop_list.pos.x= 680;
    pop_list.pos.y= 116;

	ret = poplist_create(&pop_list);

	return ret;
}

static void _antenna_ok_keypress_search(void)
{
	int search_mode;
	SearchIdSet search_id;

#if TKGS_SUPPORT
	if(GX_TKGS_AUTOMATIC == app_tkgs_get_operatemode())
	{
		app_tkgs_cantwork_popup();
		return;
	}
#endif
	GUI_GetProperty("cmbox_antenna_search", "select", &search_mode);

	if(s_antenna_tp_total == 0)
	{
		search_mode = SEARCH_SEL_BLIND;
	}
#if UNICABLE_SUPPORT
	if(s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
	{
		if(s_antenna_tp_total == 0)
		{
			search_mode = SEARCH_SEL_SAT;
		}
		if(SEARCH_SEL_BLIND== search_mode)
			search_mode = SEARCH_SEL_TP;
	}
#endif
	if(SEARCH_SEL_SAT == search_mode)
	{
		search_id.id_num = 1;
		search_id.id_array = (uint32_t *)GxCore_Malloc(search_id.id_num * sizeof(uint32_t));
		if(search_id.id_array != NULL)
		{
			SearchType search_type;

			search_id.id_array[0] = s_CurAtnSat.sat_data.id;
			search_type = SEARCH_SAT;
			scan_mode_disable_flag = true;
			if(app_search_opt_dlg(search_type, &search_id) == WND_OK)
			{
				_antenna_sat_para_save();
				timer_stop(sp_AtnLockTimer);
				timer_atn_show_signal_stop();
				app_search_start(app_tuner_cur_tuner_get());
			}
			GxCore_Free(search_id.id_array);
		}
	}
	else if(SEARCH_SEL_TP == search_mode)
	{
		search_id.id_num = 1;
		search_id.id_array = (uint32_t *)GxCore_Malloc(search_id.id_num * sizeof(uint32_t));
		if(search_id.id_array != NULL)
		{
			GxMsgProperty_NodeByPosGet node;

			node.node_type = NODE_TP;
			node.pos = s_CurAtnTp.pos;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

			search_id.id_array[0] = node.tp_data.id;
			if(app_search_opt_dlg(SEARCH_TP, &search_id) == WND_OK)
			{
				_antenna_sat_para_save();
				timer_stop(sp_AtnLockTimer);
				timer_atn_show_signal_stop();
				app_search_start(app_tuner_cur_tuner_get());
			}
			GxCore_Free(search_id.id_array);
		}
	}
	else if(SEARCH_SEL_BLIND == search_mode)
	{
		search_id.id_num = 1;
		search_id.id_array = (uint32_t *)GxCore_Malloc(search_id.id_num * sizeof(uint32_t));
		if(search_id.id_array != NULL)
		{
			SearchType search_type;

			search_id.id_array[0] = s_CurAtnSat.sat_data.id;
			search_type = SEARCH_BLIND_ONLY;
			scan_mode_disable_flag = true;
			if(app_search_opt_dlg(search_type, &search_id) == WND_OK)
			{
				timer_stop(sp_AtnLockTimer);
				_antenna_sat_para_save();
				timer_atn_show_signal_stop();
				app_search_start(app_tuner_cur_tuner_get());
			}
			GxCore_Free(search_id.id_array);
		}
	}
}

static void _antenna_ok_keypress(void)
{	
	uint32_t box_sel;
	int cmb_sel;
	
	GUI_GetProperty("box_antenna_options", "select", &box_sel);

	if((BOX_ITEM_SAT== box_sel) && (s_antenna_sat_total > 0))
	{
		GUI_GetProperty("cmbox_antenna_sat", "select", &cmb_sel);
		cmb_sel = _antenna_sat_pop(cmb_sel);
		GUI_SetProperty("cmbox_antenna_sat", "select", &cmb_sel);
	}

	if(BOX_ITEM_LNB_POWER== box_sel)
	{
		GUI_GetProperty("cmbox_antenna_lnb_power", "select", &cmb_sel);
		cmb_sel = _antenna_lnbpower_pop(cmb_sel);
		GUI_SetProperty("cmbox_antenna_lnb_power", "select", &cmb_sel);
	}

	if(BOX_ITEM_LNB_FREQ== box_sel)
	{
	#if TKGS_SUPPORT
		if(app_anternna_tkgs_key_handler())
		{
			return;
		}
		#endif
		GUI_GetProperty("cmbox_antenna_lnb_freq", "select", &cmb_sel);
		
		#if UNICABLE_SUPPORT
		
		if(ID_UNICABLE == cmb_sel)
		{
			if(s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index >= ID_UNICANBLE_TATAL_LNB)
			{
				s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index = 0;
				app_lnb_freq_sel_to_data(s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index, &s_CurAtnSat.sat_data);
				extern void app_unicable_init_if_channel_num(GxBusPmDataSat *p_sat);
				app_unicable_init_if_channel_num(&s_CurAtnSat.sat_data);
			 	printf("!!!!!!!!!!lnb_freq_change!!!!!!!\n");

			}
			GUI_CreateDialog("wnd_unicable_set_popup");
		}
		else
		{
			s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index = 0x20;
		}
		if(s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
		{
			 GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SAT_SEARCH_CONTENT_UNICABLE);

		}
		else
		{
			if(s_antenna_tp_total == 0)
			{
				GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SAT_SEARCH_CONTENT);
			}
			else
			{
				GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SEARCH_CONTENT);
			}
		}
	 	printf("%s,lnb_fre_index=%d,%d==%d\n",__FUNCTION__,s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index,ID_UNICABLE,cmb_sel);
		if(ID_UNICABLE == cmb_sel)
			return;
		#endif
		cmb_sel = _antenna_lnbfreq_pop(cmb_sel);
		GUI_SetProperty("cmbox_antenna_lnb_freq", "select", &cmb_sel);
	}

	if(BOX_ITEM_22K == box_sel)
	{
		GUI_GetProperty("cmbox_antenna_22k", "select", &cmb_sel);
		cmb_sel = _antenna_22k_pop(cmb_sel);
		GUI_SetProperty("cmbox_antenna_22k", "select", &cmb_sel);
	}

	if(BOX_ITEM_DISEQC10== box_sel)
	{
		GUI_GetProperty("cmbox_antenna_diseqc10", "select", &cmb_sel);
		cmb_sel = _antenna_diseqc10_pop(cmb_sel);
		GUI_SetProperty("cmbox_antenna_diseqc10", "select", &cmb_sel);
	}

	if(BOX_ITEM_DISEQC11== box_sel)
	{
		GUI_GetProperty("cmbox_antenna_diseqc11", "select", &cmb_sel);
		cmb_sel = _antenna_diseqc11_pop(cmb_sel);
		GUI_SetProperty("cmbox_antenna_diseqc11", "select", &cmb_sel);
	}

	if((BOX_ITEM_TP== box_sel) && (s_antenna_tp_total > 0))
	{
		GUI_GetProperty("cmbox_antenna_tp", "select", &cmb_sel);
		cmb_sel = _antenna_tp_pop(cmb_sel);
		GUI_SetProperty("cmbox_antenna_tp", "select", &cmb_sel);
	}
	
	if(BOX_ITEM_SEARCH == box_sel)
		_antenna_ok_keypress_search();
	
	if(GUI_CheckDialog("wnd_pop_book") != GXCORE_SUCCESS)
	{
		GUI_SetProperty("box_antenna_options", "update", NULL);
	}
}

static void _antenna_set_diseqc(void)
{
#define DISEQC_SET_DELAY_MS 100
	AppFrontend_DiseqcParameters diseqc10 = {0};
	AppFrontend_DiseqcParameters diseqc11 = {0};
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
#if (DEMOD_DVB_COMBO > 0)
	app_set_tune_mode(s_CurAtnSat.sat_data.type);	
#endif

	diseqc10.type = DiSEQC10;
	diseqc10.u.params_10.chDiseqc = s_CurAtnSat.sat_data.sat_s.diseqc10;
	diseqc10.u.params_10.bPolar = app_show_to_lnb_polar(&s_CurAtnSat.sat_data);

	if(s_antenna_tp_total == 0)
	{
		diseqc10.u.params_10.bPolar = SEC_VOLTAGE_OFF;
	}
	else
	{
		if(diseqc10.u.params_10.bPolar == SEC_VOLTAGE_ALL)
		{
			diseqc10.u.params_10.bPolar = app_show_to_tp_polar(&s_CurAtnTp.tp_data);
		}
	}
	
	diseqc10.u.params_10.b22k = app_show_to_sat_22k(s_CurAtnSat.sat_data.sat_s.switch_22K, &s_CurAtnTp.tp_data);

// add in 20121012
	// for polar
	//GxFrontend_SetVoltage(sat_node.sat_data.tuner, polar);
	AppFrontend_PolarState Polar =diseqc10.u.params_10.bPolar ;
	app_ioctl(s_CurAtnSat.sat_data.tuner, FRONTEND_POLAR_SET, &Polar);
	GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);

	// for 22k
	//GxFrontend_Set22K(sat_node.sat_data.tuner, sat22k);
	AppFrontend_22KState _22K = diseqc10.u.params_10.b22k;
	app_ioctl(s_CurAtnSat.sat_data.tuner, FRONTEND_22K_SET, &_22K);
	GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);

	app_set_diseqc_10(s_CurAtnSat.sat_data.tuner,&diseqc10, true);
	GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);

	diseqc11.type = DiSEQC11;
	diseqc11.u.params_11.chUncom = s_CurAtnSat.sat_data.sat_s.diseqc11;
	diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc ;
	diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
	diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;
	app_set_diseqc_11(s_CurAtnSat.sat_data.tuner, &diseqc11, true);
	GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);

	if((s_CurAtnSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
	{
		app_diseqc12_goto_position(s_CurAtnSat.sat_data.tuner,s_CurAtnSat.sat_data.sat_s.diseqc12_pos, false);
		GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);
	}
	else if((s_CurAtnSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
	{
		LongitudeInfo local_longitude = {0};
		local_longitude.longitude = s_CurAtnSat.sat_data.sat_s.longitude;
		local_longitude.longitude_direct = s_CurAtnSat.sat_data.sat_s.longitude_direct;
		app_usals_goto_position(s_CurAtnSat.sat_data.tuner,&local_longitude, NULL, false);
		GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);
	}
	// POST SEM
    GxFrontendOccur(s_CurAtnSat.sat_data.tuner);
}

static void _antenna_set_tp(SetTpMode set_mode)
{
    AppFrontend_SetTp params = {0};
	GxMsgProperty_NodeByPosGet node_get;

	if(s_antenna_tp_total == 0)
	{
		params.fre = 0;
		params.symb = 0;
		params.polar = SEC_VOLTAGE_OFF;
	}
	else
	{
		params.fre = app_show_to_tp_freq(&s_CurAtnSat.sat_data, &s_CurAtnTp.tp_data);
		params.symb = s_CurAtnTp.tp_data.tp_s.symbol_rate;

		params.polar = app_show_to_lnb_polar(&s_CurAtnSat.sat_data);
		if(params.polar == SEC_VOLTAGE_ALL)
		{
			params.polar = app_show_to_tp_polar(&s_CurAtnTp.tp_data);
		}
	}
	params.sat22k = app_show_to_sat_22k(s_CurAtnSat.sat_data.sat_s.switch_22K, &s_CurAtnTp.tp_data);
	//params.inversion = INVERSION_AUTO;
    // get the current tuner
	node_get.node_type = NODE_SAT;
	//node_get.pos = sp_SatBuf[s_CurAtnSat.pos];
	node_get.pos = s_CurAtnSat.pos;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);
	params.tuner = node_get.sat_data.tuner;
	#if UNICABLE_SUPPORT
    {
        uint32_t centre_frequency=0 ;//实际中频
        uint32_t lnb_index=0 ;
        uint32_t set_tp_req=0; //用户下行频率
        if(s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
        {
            lnb_index = app_calculate_set_freq_and_initfreq(&s_CurAtnSat.sat_data, &s_CurAtnTp.tp_data,&set_tp_req,&centre_frequency);
            params.fre  = set_tp_req;
            params.lnb_index  = lnb_index;
        }
        else
        {
            params.lnb_index  = 0x20;
        }
        params.centre_fre  = centre_frequency;
        params.if_channel_index  = s_CurAtnSat.sat_data.sat_s.unicable_para.if_channel;
        params.if_fre  = s_CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[params.if_channel_index];
    }
    printf("[antenna Set tp]fre=%d,lnb_index=%d,centre_fre=%d,if_fre=%d,if_channel_index=%d\\n",params.fre ,params.lnb_index,params.centre_fre ,params.if_fre,params.if_channel_index);
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
			params.pls_ts_status = prog_node.prog_data.muti_ts_status;
			params.pls_chanage = prog_node.prog_data.ts_pls_chanage;
		}
	}
#endif


    switch(node_get.sat_data.type)
    {
        case GXBUS_PM_SAT_S:
        {
            params.type = FRONTEND_DVB_S;
            break;
        }
        case GXBUS_PM_SAT_C:
        {
#if DEMOD_DVB_C	
	params.fre = s_CurAtnTp.tp_data.frequency;
	params.symb = s_CurAtnTp.tp_data.tp_c.symbol_rate;
	params.qam = s_CurAtnTp.tp_data.tp_c.modulation;
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
	params.fre = s_CurAtnTp.tp_data.frequency;
	params.bandwidth = s_CurAtnTp.tp_data.tp_dvbt2.bandwidth;			
	params.type_1501 = DVBT_AUTO_MODE;//PLP Data not in tp note,should be auto
#endif	
            params.type = FRONTEND_DVB_T2;
            break;
        }
#if DEMOD_DTMB
		case GXBUS_PM_SAT_DTMB:
			if(node_get.sat_data.sat_dtmb.work_mode == GXBUS_PM_SAT_1501_DTMB)
			{
				params.fre = s_CurAtnTp.tp_data.frequency;
				params.symb = 0;// NOT USED IN GXBUS
				params.qam = s_CurAtnTp.tp_data.tp_dtmb.symbol_rate;// bandwidth: 0-8M, 2-6M
				params.type_1501 = DTMB;
			}
			else
			{// for dvbc mode
				params.fre = s_CurAtnTp.tp_data.frequency;
				params.symb = s_CurAtnTp.tp_data.tp_dtmb.symbol_rate;
				params.qam = s_CurAtnTp.tp_data.tp_dtmb.modulation;
				params.type_1501 = DTMB_C;
			}
			params.type = FRONTEND_DTMB;
			break;
#endif
        default:
            params.type = FRONTEND_DVB_S;
    }

	app_set_tp(node_get.sat_data.tuner, &params, set_mode);
	
	return;
}

static void _antenna_sat_para_save(void)
{
	GxMsgProperty_NodeByPosGet node_get;
	GxMsgProperty_NodeModify node_modify;

	node_get.node_type = NODE_SAT;
	//node_get.pos = sp_SatBuf[s_CurAtnSat.pos];
	node_get.pos = s_CurAtnSat.pos;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

	if((node_get.sat_data.id != s_CurAtnSat.sat_data.id)
		|| ((node_get.sat_data.sat_s.lnb_power == s_CurAtnSat.sat_data.sat_s.lnb_power)
			&& (node_get.sat_data.sat_s.lnb1 == s_CurAtnSat.sat_data.sat_s.lnb1)
			&& (node_get.sat_data.sat_s.lnb2 == s_CurAtnSat.sat_data.sat_s.lnb2)
			&& (node_get.sat_data.sat_s.switch_22K == s_CurAtnSat.sat_data.sat_s.switch_22K)
			&& (node_get.sat_data.sat_s.diseqc10 == s_CurAtnSat.sat_data.sat_s.diseqc10)
			#if UNICABLE_SUPPORT
			&& (node_get.sat_data.sat_s.unicable_para.if_channel== s_CurAtnSat.sat_data.sat_s.unicable_para.if_channel)
			&& (node_get.sat_data.sat_s.unicable_para.centre_fre[node_get.sat_data.sat_s.unicable_para.if_channel]== s_CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[s_CurAtnSat.sat_data.sat_s.unicable_para.if_channel])
			&& (node_get.sat_data.sat_s.unicable_para.lnb_fre_index== s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index)
			&& (node_get.sat_data.sat_s.unicable_para.sat_pos== s_CurAtnSat.sat_data.sat_s.unicable_para.sat_pos)
			#endif
			&& (node_get.sat_data.sat_s.diseqc11 == s_CurAtnSat.sat_data.sat_s.diseqc11)))
		return;

	//lnb power
	node_get.sat_data.sat_s.lnb_power = s_CurAtnSat.sat_data.sat_s.lnb_power;
	//lnb freq
	node_get.sat_data.sat_s.lnb1 = s_CurAtnSat.sat_data.sat_s.lnb1;
	node_get.sat_data.sat_s.lnb2 = s_CurAtnSat.sat_data.sat_s.lnb2;
	//22k
	node_get.sat_data.sat_s.switch_22K = s_CurAtnSat.sat_data.sat_s.switch_22K;
	//diseqc 1.0
	node_get.sat_data.sat_s.diseqc10 = s_CurAtnSat.sat_data.sat_s.diseqc10;
	//diseqc 1.1
	node_get.sat_data.sat_s.diseqc11 = s_CurAtnSat.sat_data.sat_s.diseqc11;
	#if UNICABLE_SUPPORT
	{
	int index = 0;
	//centre fre
	index = s_CurAtnSat.sat_data.sat_s.unicable_para.if_channel;
	node_get.sat_data.sat_s.unicable_para.centre_fre[index] = s_CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[index];
	node_get.sat_data.sat_s.unicable_para.lnb_fre_index = s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index;
	node_get.sat_data.sat_s.unicable_para.if_channel = s_CurAtnSat.sat_data.sat_s.unicable_para.if_channel;
	node_get.sat_data.sat_s.unicable_para.sat_pos = s_CurAtnSat.sat_data.sat_s.unicable_para.sat_pos;
	}
	#endif
	
	//save
	node_modify.node_type = NODE_SAT;
	memcpy(&node_modify.sat_data, &node_get.sat_data, sizeof(GxBusPmDataSat));
	app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);

	return;
}
static void app_antenna_skip_tplist_green_func(void)
{
	int SatSel = 0;
	int TpSel = 0;
	GUI_GetProperty("cmbox_antenna_sat", "select", &SatSel);
	SatSel = (sp_SatBuf != NULL)?sp_SatBuf[SatSel]:0;//app_sat_sel_get(2);// all satellite mode
	GUI_GetProperty("cmbox_antenna_tp", "select", &TpSel);
	timer_atn_show_signal_stop();
	app_tp_list_init(SatSel,TpSel);
	return;
}

static void app_antenna_skip_satlist_red_func(void)
{
	int SatSel = 0;
	GUI_GetProperty("cmbox_antenna_sat", "select", &SatSel);
	SatSel = (sp_SatBuf != NULL)?sp_SatBuf[SatSel]:0;
	timer_atn_show_signal_stop();
	app_satellite_list_init(SatSel);
	return;
}

#define ANTENNA_SETTING
SIGNAL_HANDLER int app_antenna_setting_create(GuiWidget *widget, void *usrdata)
{
	//static GxMsgProperty_NodeByPosGet node;
	uint32_t cur_num = 0;
	int value = 0;


	GUI_SetProperty("cmbox_antenna_lnb_power", "content", LNB_POWER_CONTENT);
	GUI_SetProperty("cmbox_antenna_lnb_freq", "content", LNB_FREQ_CONTENT);
	GUI_SetProperty("cmbox_antenna_22k", "content", ANTENNA_22K_CONTENT);
	GUI_SetProperty("cmbox_antenna_diseqc10", "content", DISEQC10_CONTENT);
	GUI_SetProperty("cmbox_antenna_diseqc11", "content", DISEQC11_CONTENT);
	app_antenna_clean_signal();


	// clear the boxitem's unfocus image
	/*
	   int sel = 0;
	   for(sel=0; sel<BOX_ITEM_SEARCH+1; sel++)
	   {
	   GUI_SetProperty(s_antenna_boxitem[sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
	   GUI_SetProperty(s_antenna_ok_combox[sel], "unfocus_img", IMG_BAR_LONG_UNFOCUS);
	   }
	   */
	s_antenna_sat_total = 0;
	s_antenna_tp_total = 0;
	app_antenna_tp_change_clear();
	memset(&s_CurAtnSat, 0, sizeof(s_CurAtnSat));
	memset(&s_CurAtnTp, 0, sizeof(s_CurAtnTp));


	s_antenna_sat_total = app_tuner_sat_num_get(app_tuner_cur_tuner_get());
	if(s_antenna_sat_total > 0)
	{
		//rebuild sat list and show cur sat name
		_antenna_cmb_sat_rebuild();
		_antenna_cmb_tp_rebuild();
		_antenna_set_tp(SAVE_TP_PARAM);
		//show cur sat name
		// TODO: s_AtnSet.sat.count 需来自当前系统记录的卫星
#if DOUBLE_S2_SUPPORT
		if(!app_tuner_cur_tuner_get())//T1
		{
			if(satsel.sel_in_t1 >= s_antenna_sat_total)
				cur_num = 0;
			else
				cur_num = satsel.sel_in_t1;

		}
		else
		{
			if(satsel.sel_in_t2 >= s_antenna_sat_total)
				cur_num = 0;
			else
				cur_num = satsel.sel_in_t2;

		}
#else


		if(satsel.sel_in_all >= s_antenna_sat_total)
			cur_num = 0;
		else
			cur_num = satsel.sel_in_all;
#endif
		GUI_SetProperty("cmbox_antenna_sat", "select", &cur_num);

		//refresh sat para
		cur_num = s_CurAtnSat.sat_data.sat_s.lnb_power;
		GUI_SetProperty("cmbox_antenna_lnb_power", "select", &cur_num);

		app_lnb_freq_data_to_sel(&(s_CurAtnSat.sat_data), &cur_num);
		GUI_SetProperty("cmbox_antenna_lnb_freq", "select", &cur_num);

		//disable 22K or not
#if UNICABLE_SUPPORT
		if(cur_num == ID_9750_10600||cur_num == ID_9750_10700
				||(cur_num == ID_UNICABLE &&s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index==ID_UNICANBLE_9750_10600)
				||(cur_num == ID_UNICABLE &&s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index==ID_UNICANBLE_9750_10700)
				||(cur_num == ID_UNICABLE &&s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index==ID_UNICANBLE_9750_10750)
		  )
#else
			if(cur_num < UNIVERSAL_NUM && cur_num >= 0)
#endif
			{
				GUI_SetProperty("cmbox_antenna_22k", "content", ANTENNA_22K_AUTO_CONTENT);
				GUI_SetProperty("boxitem_antenna_22k", "state", "disable");
			}
			else
			{
				//GUI_SetProperty("boxitem_antenna_22k", "state", "enable");
				GUI_SetProperty("boxitem_antenna_22k", "enable", NULL);
				GUI_SetProperty("cmbox_antenna_22k", "content", ANTENNA_22K_CONTENT);
				cur_num = s_CurAtnSat.sat_data.sat_s.switch_22K;
				GUI_SetProperty("cmbox_antenna_22k", "select", &cur_num);
			}

		cur_num = s_CurAtnSat.sat_data.sat_s.diseqc10;
		GUI_SetProperty("cmbox_antenna_diseqc10", "select", &cur_num);

		cur_num = s_CurAtnSat.sat_data.sat_s.diseqc11;
		GUI_SetProperty("cmbox_antenna_diseqc11", "select", &cur_num);

		cur_num = s_antenna_tp_total;
		GUI_SetProperty("cmbox_antenna_tp","select", &cur_num);
		//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
		if((GXBUS_PM_SAT_DVBT2 == s_CurAtnSat.sat_data.type) 
				|| (GXBUS_PM_SAT_C == s_CurAtnSat.sat_data.type)
				|| (GXBUS_PM_SAT_DTMB == s_CurAtnSat.sat_data.type))
		{
			GUI_SetProperty("boxitem_antenna_22k", "state", "disable");
		}
		//#endif

#if UNICABLE_SUPPORT
		GUI_GetProperty("cmbox_antenna_lnb_freq", "select", &cur_num);
		if(cur_num == ID_UNICABLE)
		{
			GUI_SetProperty("boxitem_antenna_diseqc10", "state", "disable");
			GUI_SetProperty("boxitem_antenna_diseqc11", "state", "disable");
		}
		else
		{
			GUI_SetProperty("boxitem_antenna_diseqc10", "enable", NULL);
			GUI_SetProperty("boxitem_antenna_diseqc10", "state", "enable");
			GUI_SetProperty("boxitem_antenna_diseqc11", "enable", NULL);
			GUI_SetProperty("boxitem_antenna_diseqc11", "state", "enable");
		}
#endif
	}
#if UNICABLE_SUPPORT
	if(s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
	{
		GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SAT_SEARCH_CONTENT_UNICABLE);

	}
	else

#endif
	{

		if(s_antenna_tp_total == 0)
		{
			GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SAT_SEARCH_CONTENT);
			GUI_SetProperty("progbar_antenna_strength", "value", &value);
			GUI_SetProperty("progbar_antenna_quality", "value", &value);
		}
		else
		{
			GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SEARCH_CONTENT);
		}
	}


	_antenna_set_diseqc();
	_antenna_set_tp(SET_TP_FORCE);


	if (reset_timer(sp_AtnTimerSignal) != 0) 
	{
		sp_AtnTimerSignal = create_timer(timer_atn_show_signal, 100, NULL, TIMER_REPEAT);
	}
#if DOUBLE_S2_SUPPORT
	GUI_SetProperty("text_antenna_tuner_info", "state", "show");
	if(s_CurAtnSat.sat_data.tuner == 0)
		GUI_SetProperty("text_antenna_tuner_info", "string", "Tuner1");
	else if(s_CurAtnSat.sat_data.tuner == 1)
		GUI_SetProperty("text_antenna_tuner_info", "string", "Tuner2");
	else
		GUI_SetProperty("text_antenna_tuner_info", "string", "Tuner1 & Tuner2");
#endif

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_antenna_setting_destroy(GuiWidget *widget, void *usrdata)
{
	uint32_t panel_led;

	timer_atn_show_signal_stop();

	//save current sat para
	_antenna_sat_para_save();

    if (NULL != sp_AtnTimerSignal)
    {
        remove_timer(sp_AtnTimerSignal);
        sp_AtnTimerSignal = NULL;
    }
    
    if (NULL != sp_AtnLockTimer)
    {
        remove_timer(sp_AtnLockTimer);
        sp_AtnLockTimer = NULL;
    }
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

	GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
	if(sp_SatBuf != NULL)
	{
		GxCore_Free(sp_SatBuf);
		sp_SatBuf = NULL;

	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_antenna_setting_got_focus(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
	GxMsgProperty_NodeNumGet node_num = {0};
	GxMsgProperty_NodeByPosGet NodeSat = {0};
	int SatNumInAnte = 0;
	GUI_GetProperty("box_antenna_options", "select", &box_sel);
	GUI_SetProperty(s_antenna_ok_combox[box_sel], "unfocus_img", IMG_BAR_LONG_UNFOCUS);
	GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);


	// show the help info
	GUI_SetProperty(ANTENNA_SETTING_HELP1_IMG, "state", "show");
	GUI_SetProperty(ANTENNA_SETTING_HELP1_TXT, "state", "show");
	GUI_SetProperty(ANTENNA_SETTING_HELP2_IMG, "state", "show");
	GUI_SetProperty(ANTENNA_SETTING_HELP2_TXT, "state", "show");

	// sat date recover, 20131028
	NodeSat.node_type = NODE_SAT;
	NodeSat.pos = app_sat_sel_get(2);// all sel
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &NodeSat);


	if(((SatNumInAnte = app_tuner_sat_num_get(app_tuner_cur_tuner_get())) != s_antenna_sat_total)
		|| (memcmp(&(NodeSat.sat_data),&(s_CurAtnSat.sat_data),sizeof(GxBusPmDataSat))))
	{
		if(SatNumInAnte == 0)
		{
			GUI_SetProperty("cmbox_antenna_sat","content","NONE");
			s_antenna_sat_total = 0;
		}
		else
		{
            int lnb_power = 0;
			_antenna_cmb_sat_rebuild();
            lnb_power = s_CurAtnSat.sat_data.sat_s.lnb_power;
		    GUI_SetProperty("cmbox_antenna_lnb_power", "select", &lnb_power);
			s_antenna_tp_total = 0xffff;// wrong tp num, for tp rebuild control
		}
	}


	if(s_antenna_sat_total > 0)
	{
		node_num.node_type = NODE_TP;
		node_num.sat_id = s_CurAtnSat.sat_data.id;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);

		if((node_num.node_num != s_antenna_tp_total)
			|| (app_antenna_tp_change_check())) //after blind scan
		{
		#if UNICABLE_SUPPORT
			if(s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
			{
				 GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SAT_SEARCH_CONTENT_UNICABLE);

			}
			else
			{
#endif

			if(node_num.node_num == 0)
			{
				GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SAT_SEARCH_CONTENT);
			}
			else if(s_antenna_tp_total == 0)
			{
				GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SEARCH_CONTENT);
				box_sel = 1; 	/*add by huyb 2015-6-24,it is blind before scan pro*/
				GUI_SetProperty("cmbox_antenna_search", "select", &box_sel); 
			}
#if UNICABLE_SUPPORT
			}
#endif
			_antenna_cmb_tp_rebuild();
		}

		// for clear the frontend mseeage queue
		GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
		{
			extern void GxFrontend_ForceExitEx(int32_t tuner);
#if DOUBLE_S2_SUPPORT
			GxFrontend_ForceExitEx(0);
			GxFrontend_ForceExitEx(1);
#else
			GxFrontend_ForceExitEx(s_CurAtnSat.sat_data.tuner);
#endif
		}
		
		//_antenna_set_diseqc();
		//_antenna_set_tp(SET_TP_FORCE);
		//printf("=======@@==%s, %d\n", __FUNCTION__, __LINE__);
		if(reset_timer(sp_AtnLockTimer) != 0) 
		{
			s_set_tp_mode = SET_TP_FORCE;
			sp_AtnLockTimer = create_timer(timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
		}	
			
		timer_atn_show_signal_reset();
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_antenna_setting_lost_focus(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
	
	GUI_GetProperty("box_antenna_options", "select", &box_sel);
	GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_FOCUS);
	GUI_SetProperty(s_antenna_ok_combox[box_sel], "unfocus_img", IMG_BAR_OK_FOCUS);

	// hide the help info
	GUI_SetProperty(ANTENNA_SETTING_HELP1_IMG, "state", "hide");
	GUI_SetProperty(ANTENNA_SETTING_HELP1_TXT, "state", "hide");
	GUI_SetProperty(ANTENNA_SETTING_HELP2_IMG, "state", "hide");
	GUI_SetProperty(ANTENNA_SETTING_HELP2_TXT, "state", "hide");
	
	app_antenna_tp_change_clear();
	_antenna_sat_para_save();
	return EVENT_TRANSFER_STOP;
}

#if (DLNA_SUPPORT > 0)
static void app_antenna_start_dlna_cb(void)
{
    timer_atn_show_signal_stop();
}

static void app_antenna_stop_dlna_cb(void)
{
    timer_atn_show_signal_reset();
}
#endif

SIGNAL_HANDLER int app_antenna_setting_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
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
				case VK_BOOK_TRIGGER:
					GUI_GetProperty("box_antenna_options", "select", &box_sel);
					GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
					GUI_SetProperty(s_antenna_ok_combox[box_sel], "unfocus_img", IMG_BAR_LONG_UNFOCUS);
					GUI_EndDialog("after wnd_full_screen");
					break;
#if (DLNA_SUPPORT > 0)
                case VK_DLNA_TRIGGER:
                    app_dlna_ui_exec(app_antenna_start_dlna_cb, app_antenna_stop_dlna_cb);
                    break;
#endif
				case STBK_EXIT:
				case STBK_MENU:
					{
						if((GXCORE_SUCCESS == GUI_CheckDialog("wnd_satellite_list"))
							|| (GXCORE_SUCCESS == GUI_CheckDialog("wnd_tp_list")))
						{
							if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_tp_list"))
							{
								app_antenna_skip_tplist_green_func();
							}
							if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_satellite_list"))
							{
								app_antenna_skip_satlist_red_func();
							}
							GUI_GetProperty("box_antenna_options", "select", &box_sel);
							GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
							GUI_SetProperty(s_antenna_ok_combox[box_sel], "unfocus_img", IMG_BAR_LONG_UNFOCUS);
							GUI_EndDialog("wnd_antenna_setting");
						}
						else
						{
							#if MINI_16BIT_WIN8_OSD_SUPPORT
							GUI_SetProperty("txt_main_menu_video", "string", " ");
							g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
							GUI_GetProperty("box_antenna_options", "select", &box_sel);
							GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
							GUI_SetProperty(s_antenna_ok_combox[box_sel], "unfocus_img", IMG_BAR_LONG_UNFOCUS);
							GUI_EndDialog("wnd_antenna_setting");
							#else
#if (DEMOD_DTMB > 0)
							GUI_EndDialog("wnd_antenna_setting");
#else
							GUI_EndDialog("after wnd_main_menu");
                            GUI_SetProperty("wnd_main_menu", "draw_now", NULL);

							#if MV_WIN_SUPPORT
							//extern void app_main_item_menu_create(int value);
							//app_main_item_menu_create(0);
							#endif
#endif
							
							#endif
						}
					}
					break;
			
				case STBK_OK:
					_antenna_ok_keypress();
					break;

				case STBK_RED:

					GUI_GetProperty("box_antenna_options", "select", &box_sel);
					app_antenna_skip_satlist_red_func();
					if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_satellite_list"))
					{
						GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
						GUI_SetProperty(s_antenna_ok_combox[box_sel], "unfocus_img", IMG_BAR_LONG_UNFOCUS);
						//GUI_EndDialog("wnd_antenna_setting");
					}
					break;

				case STBK_GREEN:
					GUI_GetProperty("box_antenna_options", "select", &box_sel);
					app_antenna_skip_tplist_green_func();
					if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_tp_list"))
					{
						GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
						GUI_SetProperty(s_antenna_ok_combox[box_sel], "unfocus_img", IMG_BAR_LONG_UNFOCUS);
						//GUI_EndDialog("wnd_antenna_setting");
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

SIGNAL_HANDLER int app_atn_cmb_sat_change(GuiWidget *widget, void *usrdata)
{
	uint32_t sel;
	
	//save current sat para
	_antenna_sat_para_save();

	//show new sat
	GUI_GetProperty("cmbox_antenna_sat", "select", &sel);
	s_CurAtnSat.node_type = NODE_SAT;
	s_CurAtnSat.pos = sp_SatBuf[sel];
	s_antenna_tp_total = 0;
    app_sat_list_change_sel(sp_SatBuf[sel]);
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurAtnSat);
#if UNICABLE_SUPPORT
	if(s_CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[s_CurAtnSat.sat_data.sat_s.unicable_para.if_channel]==0)
	{
		s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index = 0x20;
	}
#endif
	_antenna_cmb_tp_rebuild();

	_antenna_set_tp(SAVE_TP_PARAM);
	//refresh sat para
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
	if((GXBUS_PM_SAT_DVBT2 == s_CurAtnSat.sat_data.type) 
		|| (GXBUS_PM_SAT_C == s_CurAtnSat.sat_data.type)
		|| (GXBUS_PM_SAT_DTMB == s_CurAtnSat.sat_data.type))
	{	
		GUI_SetProperty("boxitem_antenna_lnbpower","state","disable");
		GUI_SetProperty("boxitem_antenna_lnbfreq","state","disable");
		GUI_SetProperty("boxitem_antenna_22k","state","disable");
		GUI_SetProperty("boxitem_antenna_diseqc10","state","disable");
		GUI_SetProperty("boxitem_antenna_diseqc11","state","disable");
		//GUI_SetProperty("cmbox_antenna_search", "content", "[Satellite,TP]");
		GUI_SetProperty("boxitem_antenna_search","state","disable");
		if(s_antenna_tp_total == 0)
		{
			GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SAT_SEARCH_CONTENT);
		}
		else
		{
			GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SEARCH_CONTENT);
		}
	}else
	{
		GUI_SetProperty("boxitem_antenna_lnbpower","state","enable");
		GUI_SetProperty("boxitem_antenna_lnbfreq","state","enable");
		GUI_SetProperty("boxitem_antenna_22k", "enable", NULL);
		GUI_SetProperty("boxitem_antenna_diseqc10","state","enable");
		GUI_SetProperty("boxitem_antenna_diseqc11","state","enable");
		GUI_SetProperty("boxitem_antenna_search","state","enable");
//#endif
		sel = s_CurAtnSat.sat_data.sat_s.lnb_power;
		GUI_SetProperty("cmbox_antenna_lnb_power","select",&sel);

		app_lnb_freq_data_to_sel(&(s_CurAtnSat.sat_data), &sel);
		GUI_SetProperty("cmbox_antenna_lnb_freq","select",&sel);

		if(sel < UNIVERSAL_NUM && sel >= 0)
		{
			GUI_SetProperty("cmbox_antenna_22k", "content", ANTENNA_22K_AUTO_CONTENT);
	                GUI_SetProperty("boxitem_antenna_22k", "state", "disable");
       		        s_CurAtnSat.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_AUTO;
		}
		else
		{
			GUI_SetProperty("boxitem_antenna_22k", "enable", NULL);
       		        GUI_SetProperty("cmbox_antenna_22k", "content", ANTENNA_22K_CONTENT);

			sel = s_CurAtnSat.sat_data.sat_s.switch_22K;
			GUI_SetProperty("cmbox_antenna_22k","select",&sel);
		}

		sel = s_CurAtnSat.sat_data.sat_s.diseqc10;
		GUI_SetProperty("cmbox_antenna_diseqc10","select",&sel);

		sel = s_CurAtnSat.sat_data.sat_s.diseqc11;
		GUI_SetProperty("cmbox_antenna_diseqc11","select",&sel);

#if UNICABLE_SUPPORT
		if(s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
		{
		 	GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SAT_SEARCH_CONTENT_UNICABLE);

		}
		else
		{
#endif
	
			if(s_antenna_tp_total == 0)
			{
				GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SAT_SEARCH_CONTENT);
			}
			else
			{
				GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SEARCH_CONTENT);
			}
#if UNICABLE_SUPPORT
		}
#endif
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)	
	}
//#endif
	s_set_tp_mode = SET_TP_FORCE;
	if(reset_timer(sp_AtnLockTimer) != 0) 
	{
		sp_AtnLockTimer = create_timer(timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}
	else
	{
		_antenna_set_diseqc();
		_antenna_set_tp(SET_TP_FORCE);
	}
		
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_lnb_power_change(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
	
	GUI_GetProperty("cmbox_antenna_lnb_power", "select", &box_sel);
	s_CurAtnSat.sat_data.sat_s.lnb_power = box_sel;

	s_set_tp_mode = SET_TP_FORCE;
	if(reset_timer(sp_AtnLockTimer) != 0) 
	{
		sp_AtnLockTimer = create_timer(timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}	
		
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_lnb_freq_change(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel = 0;
	uint32_t s22k_sel = 0;
	
	GUI_GetProperty("cmbox_antenna_lnb_freq", "select", &box_sel);
	app_lnb_freq_sel_to_data(box_sel, &s_CurAtnSat.sat_data);
	//disable 22K
	#if UNICABLE_SUPPORT

	if(ID_UNICABLE == box_sel)
	{
		if(s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index >= ID_UNICANBLE_TATAL_LNB)
		{
			s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index = 0;
			app_lnb_freq_sel_to_data(s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index, &s_CurAtnSat.sat_data);
			extern void app_unicable_init_if_channel_num(GxBusPmDataSat *p_sat);
			app_unicable_init_if_channel_num(&s_CurAtnSat.sat_data);
		 	printf("!!!!!!!!!!app_atn_cmb_lnb_freq_change!!!!!!!\n");

		}
		//GUI_CreateDialog("wnd_unicable_set_popup");
	}
	else
	{
		s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index = 0x20;

	}
	if(s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
	{
		 GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SAT_SEARCH_CONTENT_UNICABLE);

	}
	else
	{
		if(s_antenna_tp_total == 0)
		{
			GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SAT_SEARCH_CONTENT);
		}
		else
		{
			GUI_SetProperty("cmbox_antenna_search", "content", ANTENNA_SEARCH_CONTENT);
		}
	}
 	printf("%s,lnb_fre_index=%d\n",__FUNCTION__,s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index);

	//disable 22K
	if(box_sel == ID_9750_10600||box_sel == ID_9750_10700
		||(box_sel == ID_UNICABLE &&s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index==ID_UNICANBLE_9750_10600)
		||(box_sel == ID_UNICABLE &&s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index==ID_UNICANBLE_9750_10700)
		||(box_sel == ID_UNICABLE &&s_CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index==ID_UNICANBLE_9750_10750)
	 )	
	{
		GUI_SetProperty("cmbox_antenna_22k", "content", ANTENNA_22K_AUTO_CONTENT);
		GUI_SetProperty("boxitem_antenna_22k", "state", "disable");
		s_CurAtnSat.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_AUTO;
		if(box_sel == ID_UNICABLE)
		{
			GUI_SetProperty("boxitem_antenna_diseqc10", "state", "disable");
			GUI_SetProperty("boxitem_antenna_diseqc11", "state", "disable");
		}
		else
		{
			GUI_SetProperty("boxitem_antenna_diseqc10", "enable", NULL);
			GUI_SetProperty("boxitem_antenna_diseqc10", "state", "enable");
			GUI_SetProperty("boxitem_antenna_diseqc11", "enable", NULL);
			GUI_SetProperty("boxitem_antenna_diseqc11", "state", "enable");
		}
	}
	// error
	else 
	{
		//GUI_SetProperty("boxitem_antenna_22k", "state", "enable");
		GUI_SetProperty("boxitem_antenna_22k", "enable", NULL);
		GUI_SetProperty("cmbox_antenna_22k", "content", ANTENNA_22K_CONTENT);

		// for 22K record
		if(s_CurAtnSat.sat_data.sat_s.switch_22K == GXBUS_PM_SAT_22K_AUTO)
			s_CurAtnSat.sat_data.sat_s.switch_22K = s_n22KSelectRecord;
		s22k_sel = s_CurAtnSat.sat_data.sat_s.switch_22K;
		
		GUI_SetProperty("cmbox_antenna_22k", "select", &s22k_sel);
	}

	#else
	if(box_sel < UNIVERSAL_NUM && box_sel >= 0)
	{
		GUI_SetProperty("cmbox_antenna_22k", "content", ANTENNA_22K_AUTO_CONTENT);
		GUI_SetProperty("boxitem_antenna_22k", "state", "disable");
		s_CurAtnSat.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_AUTO;
	}
	// error
	else 
	{
		//GUI_SetProperty("boxitem_antenna_22k", "state", "enable");
		GUI_SetProperty("boxitem_antenna_22k", "enable", NULL);
		GUI_SetProperty("cmbox_antenna_22k", "content", ANTENNA_22K_CONTENT);

		// for 22K record
		if(s_CurAtnSat.sat_data.sat_s.switch_22K == GXBUS_PM_SAT_22K_AUTO)
			s_CurAtnSat.sat_data.sat_s.switch_22K = s_n22KSelectRecord;
		s22k_sel = s_CurAtnSat.sat_data.sat_s.switch_22K;
		
		GUI_SetProperty("cmbox_antenna_22k", "select", &s22k_sel);
	}
	#endif

	s_set_tp_mode = SET_TP_FORCE;
	if(reset_timer(sp_AtnLockTimer) != 0) 
	{
		sp_AtnLockTimer = create_timer(timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}	

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_22k_change(GuiWidget *widget, void *usrdata)
{
	uint32_t sel_lnb;
	uint32_t sel_22k;
	
	GUI_GetProperty("cmbox_antenna_lnb_freq", "select", &sel_lnb);
	if(sel_lnb >= UNIVERSAL_NUM)
	{
		GUI_GetProperty("cmbox_antenna_22k", "select", &sel_22k);
		s_CurAtnSat.sat_data.sat_s.switch_22K = sel_22k;
		// for 22K record
		s_n22KSelectRecord = sel_22k;
	}

	s_set_tp_mode = SET_TP_FORCE;
	if(reset_timer(sp_AtnLockTimer) != 0) 
	{
		sp_AtnLockTimer = create_timer(timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}	
		
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_diseqc10_change(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
	
	GUI_GetProperty("cmbox_antenna_diseqc10", "select", &box_sel);
	s_CurAtnSat.sat_data.sat_s.diseqc10 = box_sel;

	s_set_tp_mode = SET_TP_FORCE;
	if(reset_timer(sp_AtnLockTimer) != 0) 
	{
		sp_AtnLockTimer = create_timer(timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}	
	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_diseqc11_change(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
	
	GUI_GetProperty("cmbox_antenna_diseqc11", "select", &box_sel);
	s_CurAtnSat.sat_data.sat_s.diseqc11 = box_sel;

	s_set_tp_mode = SET_TP_FORCE;
	if(reset_timer(sp_AtnLockTimer) != 0) 
	{
		sp_AtnLockTimer = create_timer(timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_tp_change(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
	
	GUI_GetProperty("cmbox_antenna_tp", "select", &box_sel);

	s_CurAtnTp.node_type = NODE_TP;
	s_CurAtnTp.pos = box_sel;
	app_set_info_cursel(box_sel);
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurAtnTp);

	s_set_tp_mode = SET_TP_FORCE;
	if(reset_timer(sp_AtnLockTimer) != 0) 
	{
		sp_AtnLockTimer = create_timer(timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}	
		
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_atn_box_keypress(GuiWidget *widget, void *usrdata)
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
				case STBK_UP:
					{
						uint32_t box_sel;
	
						GUI_GetProperty("box_antenna_options", "select", &box_sel);
						if(BOX_ITEM_SAT == box_sel)
						{
							//GUI_SetFocusWidget("boxitem_antenna_autoscan");
							box_sel = BOX_ITEM_SEARCH;
							GUI_SetProperty("box_antenna_options", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;
					
				case STBK_DOWN:
					{
						uint32_t box_sel; 
						GUI_GetProperty("box_antenna_options", "select", &box_sel);
						if(BOX_ITEM_SEARCH == box_sel)
						{
							//GUI_SetFocusWidget("boxitem_antenna_satellite");
							box_sel = BOX_ITEM_SAT;
							GUI_SetProperty("box_antenna_options", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;
				#if TKGS_SUPPORT
				case STBK_LEFT:
				case STBK_RIGHT:
				{
					int cmb_sel;
					GUI_GetProperty("box_antenna_options", "select", &cmb_sel);
					if(cmb_sel == BOX_ITEM_LNB_FREQ)
					{
						printf("function = %s,line%d\n",__FUNCTION__,__LINE__);
						if(app_anternna_tkgs_key_handler())
						{
							ret = EVENT_TRANSFER_STOP;
							break;
						}
					}

					ret = EVENT_TRANSFER_KEEPON;

				}
				break;
				#endif
				case STBK_OK:
				case STBK_MENU:
				case STBK_EXIT:
				#if TKGS_SUPPORT == 0
				case STBK_LEFT:
				case STBK_RIGHT:
				#endif
				case STBK_RED:
				case STBK_BLUE:
				case STBK_GREEN:
#if (DLNA_SUPPORT > 0)
                case VK_DLNA_TRIGGER:
#endif
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


#if UNICABLE_SUPPORT
int app_antenna_get_cur_sat_info(GxMsgProperty_NodeByPosGet* p_node_sat )
{
	memcpy(p_node_sat,&s_CurAtnSat, sizeof(GxMsgProperty_NodeByPosGet));
	return 0;
}

int app_antenna_timer_unicable_set_frontend(void )
{
	s_set_tp_mode = SET_TP_FORCE;
	if(reset_timer(sp_AtnLockTimer) != 0) 
	{
		sp_AtnLockTimer = create_timer(timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}
	return 0;
}

 int app_antenna_unicable_sat_para_save(GxMsgProperty_NodeByPosGet* p_node_sat )
 {
 	GxMsgProperty_NodeByPosGet node_get;
	GxMsgProperty_NodeModify node_modify;
	int index = 0;
	
 	if(p_node_sat!=NULL)
 	{
	 	memcpy(&s_CurAtnSat,p_node_sat, sizeof(GxMsgProperty_NodeByPosGet));
 	}

	node_get.node_type = NODE_SAT;
	node_get.pos = s_CurAtnSat.pos;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

	if((node_get.sat_data.id != p_node_sat->sat_data.id)
		|| ((node_get.sat_data.sat_s.unicable_para.if_channel== p_node_sat->sat_data.sat_s.unicable_para.if_channel)
			&& (node_get.sat_data.sat_s.unicable_para.lnb_fre_index== p_node_sat->sat_data.sat_s.unicable_para.lnb_fre_index)
			&& (node_get.sat_data.sat_s.unicable_para.sat_pos== p_node_sat->sat_data.sat_s.unicable_para.sat_pos)))
	{
		return 1;
	}

	//centre fre
	index = s_CurAtnSat.sat_data.sat_s.unicable_para.if_channel;
	node_get.sat_data.sat_s.unicable_para.centre_fre[index] = p_node_sat->sat_data.sat_s.unicable_para.centre_fre[index];
	node_get.sat_data.sat_s.unicable_para.lnb_fre_index = p_node_sat->sat_data.sat_s.unicable_para.lnb_fre_index;
	node_get.sat_data.sat_s.unicable_para.if_channel = p_node_sat->sat_data.sat_s.unicable_para.if_channel;
	node_get.sat_data.sat_s.unicable_para.sat_pos = p_node_sat->sat_data.sat_s.unicable_para.sat_pos;

	//save
	node_modify.node_type = NODE_SAT;
	memcpy(&node_modify.sat_data, &node_get.sat_data, sizeof(GxBusPmDataSat));
	app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);

 	return 0;
 }
#endif

#else //DEMOD_DVB_S

SIGNAL_HANDLER int app_antenna_setting_create(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_antenna_setting_destroy(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_antenna_setting_got_focus(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_antenna_setting_lost_focus(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_antenna_setting_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_atn_cmb_sat_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_atn_cmb_lnb_power_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_atn_cmb_lnb_freq_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_atn_cmb_22k_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_atn_cmb_diseqc10_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_atn_cmb_diseqc11_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_atn_cmb_tp_change(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_atn_box_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

#endif

