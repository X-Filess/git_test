#include "app.h"
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "app_frontend.h"
#include "app_pop.h"
#include "gxoem.h"
#include "./module/ota/app_ota_upgrade.h"
#include "./module/ota/app_ota_config.h"

#ifdef LINUX_OS
#include "youtube_tools.h"
#endif

extern uint8_t g_chFlagEntryFlash;
extern uint32_t g_nFlagOta;

#if (OTA_SUPPORT > 0)
#define OTA_SEGMENT_CONTENT	"[All,Default Program]"
#define OTA_BOOT_CHOICE_CONTENT "[On,Off]"
//Private 
static uint16_t s_wSatTotal = 0;
static uint16_t s_wTpTotal = 0;

GxMsgProperty_NodeByPosGet s_CurOtaSat = {0};
static GxMsgProperty_NodeByPosGet s_CurOtaTp = {0};
static event_list* sp_OtaTimerSignal = NULL;
static event_list* sp_OtaLockTimer = NULL;
static SetTpMode s_set_tp_mode = SET_TP_AUTO;

enum BOX_ITEM_NAME 
{
	OTA_BOX_ITEM_SAT = 0,
	OTA_BOX_ITEM_TP,	
	//OTA_BOX_ITEM_SEGMENT,
    OTA_BOX_ITEM_PID,
	OTA_BOX_ITEM_OK,
	//OTA_BOX_ITEM_BOOT,
    OTA_BOX_ITEM_MAX,
};

#define OTA_SEGMENT_NUM 2

extern uint32_t app_tuner_cur_tuner_get(void);


static void _ota_set_diseqc(void);
static void _ota_set_tp(SetTpMode set_mode);
//
//
//
#define OTA_UPDATE_TIMEOUT     120  //SECONDS
static unsigned int thiz_start_time = 0;
static unsigned char thiz_timeout_flag = 0;
static void _init_timeout_control(void)
{
extern int app_get_current_time(int *utc_time, int *timezone_min, int *daylight_min, unsigned int *daylight_flag);
    int utc_time = 0;
    int timezone_min = 0;
    int daylight_min = 0;
    unsigned int daylight_flag = 0;
    app_get_current_time(&utc_time, &timezone_min, &daylight_min, &daylight_flag);
    thiz_start_time = utc_time;
    GUI_SetProperty("text_ota_notice_info2", "status", "show");
    thiz_timeout_flag = 1;
}

// from 0~120
static int _get_timeout_time(void)
{
extern int app_get_current_time(int *utc_time, int *timezone_min, int *daylight_min, unsigned int *daylight_flag);
    int utc_time = 0;
    int timezone_min = 0;
    int daylight_min = 0;
    unsigned int daylight_flag = 0;
    app_get_current_time(&utc_time, &timezone_min, &daylight_min, &daylight_flag);
    return (utc_time - thiz_start_time);
}

#define FUNC
void app_ota_process_update(int percent)
{
    char Buffer[20] = {0};

    GUI_SetProperty("progbar_ota_upgrade", "value", &percent);

    memset(Buffer, 0, sizeof(Buffer));
    sprintf(Buffer, "%d%%", percent);
    GUI_SetProperty("text_ota_upgrade_value", "string", Buffer);
    if(percent > 0)
    {
        _init_timeout_control();
    }
    if(100 == percent)
    {
        thiz_timeout_flag = 0;
    }
}

void app_ota_message_update(char *message1, char *message2)
{
    GUI_SetProperty("text_ota_notice_info1", "string", message1);
    GUI_SetProperty("text_ota_notice_info2", "string", message2);
}

static int _ota_timeout_exit(PopDlgRet ret)
{
    // exit menu
    app_ota_upgrade_exit();
   return 0;
}

static int _ota_timeout_pop(void)
{
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.mode = POP_MODE_UNBLOCK;
    pop.type = POP_TYPE_NO_BTN;
    pop.str = STR_ID_UPDATE_FAILED;
    pop.exit_cb = _ota_timeout_exit;
    pop.pos.x = 300;
    pop.pos.y = 150;
    pop.timeout_sec = 3;
    popdlg_create(&pop);
    return 0;
}


/***************************************************************************
  Timer Function
 ***************************************************************************/
#define TIMER_FUNC
static int timer_ota_show_signal(void *userdata)
{
	unsigned int value = 0;
	char Buffer[20] = {0};
	SignalBarWidget siganl_bar = {0};
	SignalValue siganl_val = {0};

	// progbar strength
	//g_AppNim.property_get(&g_AppNim, AppFrontendID_Strength,(void*)(&value),4);
    app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_STRENGTH_GET, &value);
	siganl_bar.strength_bar = "progbar_ota_strength";
	siganl_val.strength_val = value;
	
	// strength value
	memset(Buffer,0,sizeof(Buffer));
	sprintf(Buffer,"%d%%",value);
	GUI_SetProperty("text_ota_strength_value", "string", Buffer);

	// progbar quality
	//g_AppNim.property_get(&g_AppNim, AppFrontendID_Quality,(void*)(&value),4);
    app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_QUALITY_GET, &value);
	siganl_bar.quality_bar= "progbar_ota_quality";
	siganl_val.quality_val = value;
	// quality value
	memset(Buffer,0,sizeof(Buffer));
	sprintf(Buffer,"%d%%",value);
	GUI_SetProperty("text_ota_quality_value", "string", Buffer);

	app_signal_progbar_update(app_tuner_cur_tuner_get(), &siganl_bar, &siganl_val);

	ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &value);

// for time display
    if(thiz_timeout_flag == 1)
    {
        char buf[10] = {0};
        int time = _get_timeout_time();

        if(time >= OTA_UPDATE_TIMEOUT)
        {// pop dialog
            GUI_SetProperty("text_ota_notice_info2", "status", "hide");
            //
            thiz_timeout_flag = 0;
            _ota_timeout_pop();
        }
        else
        {
            time = (OTA_UPDATE_TIMEOUT - time);
            sprintf(buf, "%d s", time);
            // 
            GUI_SetProperty("text_ota_notice_info2", "string", buf);
        }
    }
	return 0;
}


void timer_ota_show_signal_stop(void)
{
	timer_stop(sp_OtaTimerSignal);
}

void timer_ota_show_signal_reset(void)
{
	reset_timer(sp_OtaTimerSignal);
}

static int timer_ota_set_frontend(void *userdata)
{
	SetTpMode force = 0;

	remove_timer(sp_OtaLockTimer);
	sp_OtaLockTimer = NULL;

	if(userdata != NULL)
	{
		force = *(SetTpMode*)userdata;
		if(force == SET_TP_FORCE)
		{
			GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
		}
	}
	_ota_set_diseqc();
	_ota_set_tp(force);
	return 0;
}



/***************************************************************************
						SUB Function
***************************************************************************/
#define SUB_FUNC
static void _ota_cmb_sat_rebuild(void)
{
#define BUFFER_LENGTH	(MAX_SAT_NAME+20)
	int i = 0;
	char *buffer_all = NULL;
	char buffer[BUFFER_LENGTH] = {0};
			
	//rebuild sat list in combo box
	if(s_wSatTotal == 0)
	{
		printf("_ota_cmb_sat_rebuild total error =%s ,%d\n",__FILE__,__LINE__);
	}
	buffer_all = GxCore_Malloc(s_wSatTotal * BUFFER_LENGTH);
	if(buffer_all == NULL)
	{
		printf("_ota_cmb_sat_rebuild, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}
	memset(buffer_all, 0, s_wSatTotal * BUFFER_LENGTH);
	
	//first one
	i = 0;
	s_CurOtaSat.node_type = NODE_SAT;
	s_CurOtaSat.pos = i;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurOtaSat);
	memset(buffer, 0, BUFFER_LENGTH);
	sprintf(buffer,"[(%d/%d) %s", i+1, s_wSatTotal, (char*)s_CurOtaSat.sat_data.sat_s.sat_name);
	strcat(buffer_all, buffer);

	if(s_wSatTotal > 1)
	{
		GxMsgProperty_NodeByPosGet node_sat = {0};
		for(i = 1; i < s_wSatTotal; i++)
		{
			node_sat.node_type = NODE_SAT;
			node_sat.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
			memset(buffer, 0, BUFFER_LENGTH);
			sprintf(buffer,",(%d/%d) %s", i + 1, s_wSatTotal, (char*)node_sat.sat_data.sat_s.sat_name);
			strcat(buffer_all, buffer);
		}
	}
	//only 1 sat or finished
	strcat(buffer_all, "]");
	GUI_SetProperty("cmbox_ota_sat","content",buffer_all);

	GxCore_Free(buffer_all);
	
	return;
}


void _ota_cmb_tp_rebuild(void)
{
#define BUFFER_LENGTH_TP	(64)
	GxMsgProperty_NodeNumGet node_num = {0};
	int i = 0;
	char *buffer_all = NULL;
	char buffer[BUFFER_LENGTH_TP] = {0};
	int cmb_sel;
	
	// get tp number
	node_num.node_type = NODE_TP;
	node_num.sat_id = s_CurOtaSat.sat_data.id;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	s_wTpTotal = node_num.node_num;

	if(0 == s_wTpTotal)
	{
		// todo
		memset(&s_CurOtaTp, 0, sizeof(s_CurOtaTp));
		GUI_SetProperty("cmbox_ota_tp","content","[NONE]");
	}
	else
	{
		buffer_all = GxCore_Malloc(s_wTpTotal * (BUFFER_LENGTH_TP));
		if(buffer_all == NULL)
		{
			printf("_ota_cmb_tp_rebuild, malloc failed...%s,%d\n",__FILE__,__LINE__);
			return ;
		}
		memset(buffer_all, 0, s_wTpTotal * (BUFFER_LENGTH_TP));

		//first one
		i = 0;
		s_CurOtaTp.node_type = NODE_TP;
		s_CurOtaTp.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurOtaTp);

		memset(buffer, 0, BUFFER_LENGTH_TP);
		if(s_CurOtaTp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
		{
			sprintf(buffer,"[(%d/%d) %d / %s / %d", i+1, s_wTpTotal, 
				s_CurOtaTp.tp_data.frequency, "V" ,s_CurOtaTp.tp_data.tp_s.symbol_rate);
		}
		else if(s_CurOtaTp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
		{
			sprintf(buffer,"[(%d/%d) %d / %s / %d", i+1, s_wTpTotal, 
				s_CurOtaTp.tp_data.frequency, "H" ,s_CurOtaTp.tp_data.tp_s.symbol_rate);
		}
		strcat(buffer_all, buffer);

		if(s_wTpTotal > 1)
		{
			GxMsgProperty_NodeByPosGet node_tp = {0};
			for(i = 1; i < s_wTpTotal; i++)
			{
				node_tp.node_type = NODE_TP;
				node_tp.pos = i;
				app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
				memset(buffer, 0, BUFFER_LENGTH_TP);
				if(node_tp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
				{
					sprintf(buffer,",(%d/%d) %d / %s / %d", i+1, s_wTpTotal, 
						node_tp.tp_data.frequency, "V" ,node_tp.tp_data.tp_s.symbol_rate);
				}
				else if(node_tp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
				{
					sprintf(buffer,",(%d/%d) %d / %s / %d", i+1, s_wTpTotal, 
						node_tp.tp_data.frequency, "H" ,node_tp.tp_data.tp_s.symbol_rate);
				}
				strcat(buffer_all, buffer);
			}
		}
		//only 1 sat or finished
		strcat(buffer_all, "]");
		GUI_SetProperty("cmbox_ota_tp","content",buffer_all);
		cmb_sel = 0;
		GUI_SetProperty("cmbox_ota_tp","select",&cmb_sel);
			
		GxCore_Free(buffer_all);
	}
	return;
}

static int _ota_sat_pop(int sel)
{
	PopList pop_list;
	int ret =-1;
	int i;
	char **sat_name = NULL;
	GxMsgProperty_NodeByPosGet node;
	
	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_SATELLITE;
	pop_list.item_num = s_wSatTotal;

	if(0 == s_wSatTotal)
	{
		printf("_ota_sat_pop, no sat..%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	sat_name = (char**)GxCore_Malloc(s_wSatTotal * sizeof(char*));
	if(sat_name == NULL)
	{
		printf("_ota_sat_pop, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	if(sat_name != NULL)
	{
		for(i = 0; i < s_wSatTotal; i++)
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
    pop_list.line_num= POPLIST_LINE_5;
    pop_list.pos.x= 640;
    pop_list.pos.y= 95;

	ret = poplist_create(&pop_list);

	if(sat_name != NULL)
	{
		for(i = 0; i < s_wSatTotal; i++)
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

static int _ota_tp_pop(int sel)
{
#define BUFFER_LENGTH_TP	(64)

	PopList pop_list;
	int ret =-1;
	int i;
	char **tp_info = NULL;
	GxMsgProperty_NodeByPosGet node;
	
	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_TP;
	pop_list.item_num = s_wTpTotal;

	if(0 == s_wTpTotal)
	{
		printf("_ota_sat_pop, no tp..%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	
	tp_info = (char**)GxCore_Malloc(s_wTpTotal * sizeof(char*));
	if(tp_info != NULL)
	{
		for(i = 0; i < s_wTpTotal; i++)
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
    pop_list.pos.x= 640;
    pop_list.pos.y= 95;

	ret = poplist_create(&pop_list);

	if(tp_info != NULL)
	{
		for(i = 0; i < s_wTpTotal; i++)
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

#if 0
static int _ota_segment_pop(int sel)
{
	PopList pop_list;
	int ret =-1;
	char **segment_name = NULL;

	segment_name = (char**)GxCore_Malloc(OTA_SEGMENT_NUM * sizeof(char*));
	if(segment_name == NULL)
	{
		printf("_ota_segment_pop, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_SECTION;//OTA_SEGMENT_CONTENT
	pop_list.item_num = OTA_SEGMENT_NUM;

	segment_name[0] = "ALL";
	segment_name[1] = "Default Program";

	pop_list.item_content = segment_name;

	pop_list.sel = sel;
	pop_list.show_num= true;

	ret = poplist_create(&pop_list);
	GxCore_Free(segment_name);
	return ret;
}
#endif
static int _ota_boot_pop(int sel)
{
	PopList pop_list;
	int ret =-1;
	char **segment_name = NULL;

	segment_name = (char**)GxCore_Malloc(2 * sizeof(char*));
	if(segment_name == NULL)
	{
		printf("_ota_boot_pop, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_OTA_BOOT;
	pop_list.item_num = 2;

	segment_name[0] = "on";
	segment_name[1] = "off";

	pop_list.item_content = segment_name;

	pop_list.sel = sel;
	pop_list.show_num= true;
    pop_list.line_num= POPLIST_LINE_2;
    pop_list.pos.x= 640;
    pop_list.pos.y= 261;

	ret = poplist_create(&pop_list);
	GxCore_Free(segment_name);
	return ret;
}

static void _check_modify_pid(void)
{
    char *pid = NULL;
    unsigned short pid_data = 0;
    GUI_GetProperty("edit_ota_pid", "string", &pid);
    if(NULL == pid)
    {
        printf("\nWRONG, %s, %d\n", __FUNCTION__, __LINE__);
        return ;
    }
    pid_data = atoi(pid);
    app_ota_set_data_pid(pid_data);
}

static void _ota_ok_keypress(void)
{
    uint32_t box_sel;
    int cmb_sel;

    GUI_GetProperty("box_ota_upgrade_options", "select", &box_sel);

    if((OTA_BOX_ITEM_SAT== box_sel) && (s_wSatTotal > 0))
    {
        GUI_GetProperty("cmbox_ota_sat", "select", &cmb_sel);
        cmb_sel = _ota_sat_pop(cmb_sel);
        GUI_SetProperty("cmbox_ota_sat", "select", &cmb_sel);
    }

    if((OTA_BOX_ITEM_TP== box_sel) && (s_wTpTotal > 0))
    {
        GUI_GetProperty("cmbox_ota_tp", "select", &cmb_sel);
        cmb_sel = _ota_tp_pop(cmb_sel);
        GUI_SetProperty("cmbox_ota_tp", "select", &cmb_sel);
    }
#if 0	
    if(OTA_BOX_ITEM_SEGMENT== box_sel)
    {
        GUI_GetProperty("cmbox_ota_segment", "select", &cmb_sel);
        cmb_sel = _ota_segment_pop(cmb_sel);
        GUI_SetProperty("cmbox_ota_segment", "select", &cmb_sel);
    }
#endif
    if((OTA_BOX_ITEM_OK == box_sel) && (0 == g_nFlagOta))
    {
        _check_modify_pid();
        app_ota_upgrade_entry();
// add in 20160518
        _init_timeout_control();
    }

#if 0
    if(OTA_BOX_ITEM_BOOT== box_sel)
    {
        GUI_GetProperty("cmbox_ota_boot", "select", &cmb_sel);
        cmb_sel = _ota_boot_pop(cmb_sel);
        GUI_SetProperty("cmbox_ota_boot", "select", &cmb_sel);
        //GUI_SetProperty("boxitem_ota_boot", "update", NULL);
    }
#endif
    GUI_SetProperty("box_ota_upgrade_options", "update", NULL);
}

static void _ota_set_diseqc(void)
{
    AppFrontend_DiseqcParameters diseqc10 = {0};
    AppFrontend_DiseqcParameters diseqc11 = {0};

    //#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
#if (DEMOD_DVB_COMBO > 0)
    app_set_tune_mode(s_CurOtaSat.sat_data.type);
#endif
    {
        extern void GxFrontend_ForceExitEx(int32_t tuner);
#if DOUBLE_S2_SUPPORT
        GxFrontend_ForceExitEx(0);
        GxFrontend_ForceExitEx(1);
#else
        GxFrontend_ForceExitEx(s_CurOtaSat.sat_data.tuner);
#endif
    }

    diseqc10.type = DiSEQC10;
    diseqc10.u.params_10.chDiseqc = s_CurOtaSat.sat_data.sat_s.diseqc10;
    diseqc10.u.params_10.bPolar = app_show_to_lnb_polar(&s_CurOtaSat.sat_data);
    if(diseqc10.u.params_10.bPolar == SEC_VOLTAGE_ALL)
    {
        if(s_wTpTotal == 0)
        {
            diseqc10.u.params_10.bPolar = SEC_VOLTAGE_OFF;
        }
        else
        {
            diseqc10.u.params_10.bPolar = app_show_to_tp_polar(&s_CurOtaTp.tp_data);
        }
    }
    diseqc10.u.params_10.b22k = app_show_to_sat_22k(s_CurOtaSat.sat_data.sat_s.switch_22K, &s_CurOtaTp.tp_data);

    // add in 20121012
    // for polar
    //GxFrontend_SetVoltage(sat_node.sat_data.tuner, polar);
    AppFrontend_PolarState Polar =diseqc10.u.params_10.bPolar ;
    app_ioctl(s_CurOtaSat.sat_data.tuner, FRONTEND_POLAR_SET, (void*)&Polar);

    // for 22k	
    //GxFrontend_Set22K(sat_node.sat_data.tuner, sat22k);
    AppFrontend_22KState _22K = diseqc10.u.params_10.b22k;
    app_ioctl(s_CurOtaSat.sat_data.tuner, FRONTEND_22K_SET, (void*)&_22K);

    app_set_diseqc_10(s_CurOtaSat.sat_data.tuner, &diseqc10, true);

    diseqc11.type = DiSEQC11;
    diseqc11.u.params_11.chUncom = s_CurOtaSat.sat_data.sat_s.diseqc11;
    diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc ;
    diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
    diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;
    app_set_diseqc_11(s_CurOtaSat.sat_data.tuner,&diseqc11, true);  

    if((s_CurOtaSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
    {
        app_diseqc12_goto_position(s_CurOtaSat.sat_data.tuner,s_CurOtaSat.sat_data.sat_s.diseqc12_pos, false);
    }
    else if((s_CurOtaSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
    {
        LongitudeInfo local_longitude = {0};
        local_longitude.longitude = s_CurOtaSat.sat_data.sat_s.longitude;
        local_longitude.longitude_direct = s_CurOtaSat.sat_data.sat_s.longitude_direct;
        app_usals_goto_position(s_CurOtaSat.sat_data.tuner,&local_longitude, NULL,false);
    }

    // post sem
    GxFrontendOccur(s_CurOtaSat.sat_data.tuner);
}


static void _ota_set_tp(SetTpMode set_mode)
{
    AppFrontend_SetTp params = {0};
    GxMsgProperty_NodeByPosGet node_get;


    if(s_wTpTotal == 0)
    {
        params.fre = 0;
        params.symb = 0;
        params.polar = 0;
    }
    else
    {
        params.fre = app_show_to_tp_freq(&s_CurOtaSat.sat_data, &s_CurOtaTp.tp_data);
        params.symb = s_CurOtaTp.tp_data.tp_s.symbol_rate;

        params.polar = app_show_to_lnb_polar(&s_CurOtaSat.sat_data);
        if(params.polar == SEC_VOLTAGE_ALL)
        {
            params.polar = app_show_to_tp_polar(&s_CurOtaTp.tp_data);
        }
    }
    params.sat22k = app_show_to_sat_22k(s_CurOtaSat.sat_data.sat_s.switch_22K, &s_CurOtaTp.tp_data);
    //get the current tuner
    node_get.node_type = NODE_SAT;
    node_get.pos = s_CurOtaSat.pos;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);
    params.tuner = node_get.sat_data.tuner;
#if UNICABLE_SUPPORT
    {
        uint32_t centre_frequency=0 ;
        uint32_t lnb_index=0 ;
        uint32_t set_tp_req=0;
        if(s_CurOtaSat.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
        {
            lnb_index = app_calculate_set_freq_and_initfreq(&s_CurOtaSat.sat_data, &s_CurOtaTp.tp_data,&set_tp_req,&centre_frequency);
            params.fre  = set_tp_req;
            params.lnb_index  = lnb_index;
        }
        else
        {
            params.lnb_index  = 0x20;
        }
        params.centre_fre  = centre_frequency;
        params.if_channel_index  = s_CurOtaSat.sat_data.sat_s.unicable_para.if_channel;
        params.if_fre  = s_CurOtaSat.sat_data.sat_s.unicable_para.centre_fre[params.if_channel_index];
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
                params.fre = s_CurOtaTp.tp_data.frequency;
                params.symb = s_CurOtaTp.tp_data.tp_c.symbol_rate;
                params.qam = s_CurOtaTp.tp_data.tp_c.modulation;
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
                params.fre = s_CurOtaTp.tp_data.frequency;
                params.bandwidth = s_CurOtaTp.tp_data.tp_dvbt2.bandwidth;			
                params.type_1501 = DVBT_AUTO_MODE;//PLP Data not in tp note,should be auto
#endif
                params.type = FRONTEND_DVB_T2;
                break;
            }
#if DEMOD_DTMB
        case GXBUS_PM_SAT_DTMB:
            {
                if(GXBUS_PM_SAT_1501_DTMB == node_get.sat_data.sat_dtmb.work_mode)
                {
                    params.fre = s_CurOtaTp.tp_data.frequency;
                    params.qam = s_CurOtaTp.tp_data.tp_dtmb.symbol_rate;			
                    params.type_1501 = DTMB;//PLP Data not in tp note,should be auto

                }
                else
                {
                    params.fre = s_CurOtaTp.tp_data.frequency;
                    params.symb = s_CurOtaTp.tp_data.tp_dtmb.symbol_rate;
                    params.qam = s_CurOtaTp.tp_data.tp_dtmb.modulation;
                    params.type_1501 = DTMB_C;
                }
                params.type = FRONTEND_DTMB;
                break;
            }
#endif
        default:
            params.type = FRONTEND_DVB_S;
    }

    app_set_tp(node_get.sat_data.tuner, &params, set_mode);

    return;
}

static int app_ota_read_update_stauts(void)
{
#ifdef LINUX_OS
	#define OTA_TAG_BUF_SIZE	493
	#define OTA_FLAG_REDAD_FILE  "/tmp/mtd_img_ota_para"
	#define OEM_UPDATE_VARIABLE             ("flag=yes")
	FILE *file;
	int ret = 0;
	char argv[64];
	char buf[OTA_TAG_BUF_SIZE];
	char *p_ota_flag = NULL;

	sprintf(argv, "ota_update_read V_OEM \"%s\"", "read");
	system_shell(argv, 0, NULL, NULL, NULL);
	
	sprintf(argv, "%s", OTA_FLAG_REDAD_FILE);
	file = fopen(OTA_FLAG_REDAD_FILE, "r");
	if (file == 0)
	{
		printf("----------file open err.\n");
		return 1;
	}
	
	memset(buf, 0, OTA_TAG_BUF_SIZE);
	ret = fread(buf, 1, OTA_TAG_BUF_SIZE, file);
	if (ret != OTA_TAG_BUF_SIZE) 
	{
		printf("------read failed %d.\n", ret);
	}
	p_ota_flag = buf;
	p_ota_flag = strstr(buf, "flag");
	printf("__11111______%s\n",p_ota_flag);
	if(p_ota_flag != NULL)
	{
		if (strncmp(p_ota_flag, OEM_UPDATE_VARIABLE,
						strlen(OEM_UPDATE_VARIABLE)) == 0)
		{
			fclose(file);
			printf("_[p_ota_flag]__________%s\n",p_ota_flag);
			return 0;
		}
	}
	fclose(file);
#endif
	return 1;
}

#endif


#define OTA_SETTING

#define OTA_MENU_NEW_VERSION_DETECT         "text_ota_update_notice"

SIGNAL_HANDLER int app_ota_upgrade_create(GuiWidget *widget, void *usrdata)
{
#if (OTA_SUPPORT > 0)
	GxMsgProperty_NodeNumGet node_num = {0};
	uint32_t cur_num = 0;
	uint32_t box_sel = 0;
    char buf[10] = {0};
    unsigned int pid = 0;

	app_lang_set_menu_font_type("text_ota_title");
	//GUI_SetProperty("cmbox_ota_segment", "content", OTA_SEGMENT_CONTENT);
	//GUI_SetProperty("cmbox_ota_boot", "content", OTA_BOOT_CHOICE_CONTENT);
    // pid
    app_ota_get_data_pid(&pid);
    sprintf(buf, "%d", pid);
    GUI_SetProperty("edit_ota_pid", "string", buf);

    app_loader_ota_get_update_flag((unsigned char*)&box_sel);
    if(box_sel == 0)
    {
        box_sel = 1;// on
	    GUI_SetProperty("cmbox_ota_boot", "select", &box_sel);
    }
    else
    {
        box_sel = 0;// off
	    GUI_SetProperty("cmbox_ota_boot", "select", &box_sel);
    }
	//get total sat num
	node_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	s_wSatTotal = node_num.node_num;

	_ota_cmb_sat_rebuild();
	_ota_cmb_tp_rebuild();

	cur_num = s_CurOtaSat.pos;
	GUI_SetProperty("cmbox_ota_sat", "select", &cur_num);

    if(s_wTpTotal > 0)
    {
	    cur_num = s_wTpTotal - 1;
    }
    else
    {
	    cur_num = s_wTpTotal;
    }
	GUI_SetProperty("cmbox_ota_tp","select", &cur_num);

	_ota_set_diseqc();
	_ota_set_tp(SET_TP_FORCE);

	if (reset_timer(sp_OtaTimerSignal) != 0)
	{
		sp_OtaTimerSignal = create_timer(timer_ota_show_signal, 100, NULL, TIMER_REPEAT);
	}
    // for new version notice display
    {
        int value = 0;
        //GxBus_ConfigGetInt("ota>flag", &value, 0);
        //app_menu_ota_set_update_flag((unsigned char*)&value);
        app_ota_get_notice_flag((unsigned char*)&value, -1);
        if(1 == value)
        {
	        GUI_SetProperty(OTA_MENU_NEW_VERSION_DETECT, "string", STR_ID_OTA_MONITOR_NOTICE2);
            GUI_SetProperty(OTA_MENU_NEW_VERSION_DETECT,"state", "show");
        }
    }
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ota_upgrade_destroy(GuiWidget *widget, void *usrdata)
{
#if (OTA_SUPPORT > 0)
	remove_timer(sp_OtaTimerSignal);
	sp_OtaTimerSignal = NULL;

	remove_timer(sp_OtaLockTimer);
	sp_OtaLockTimer = NULL;
#endif
	GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ota_upgrade_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
#if (OTA_SUPPORT > 0)
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
					if((0 == g_chFlagEntryFlash) && (0 == g_nFlagOta))
					{
						GUI_EndDialog("after wnd_full_screen");
					}
					break;
				case STBK_EXIT:
				case STBK_MENU:
                    {
                        if(0 == g_chFlagEntryFlash)
                        {
                            if(1 == g_nFlagOta)
                            {
                                PopDlg  pop;
                                memset(&pop, 0, sizeof(PopDlg));
                                pop.type = POP_TYPE_YES_NO;
                                /*are you sure stop OTA upgrade*/
                                pop.str = STR_ID_OTA_UPGRADE_INTERRUNPT;
                                pop.pos.x = 300;
                                pop.pos.y = 150;
                                pop.format = POP_FORMAT_DLG;

                                thiz_timeout_flag = 0;
                                //pop.exit_cb = _interrupt_ota_upgrade_cb;
                                if( POP_VAL_OK == popdlg_create(&pop) )
                                {
                                    thiz_timeout_flag = 0;
                                    _check_modify_pid();
                                    app_ota_upgrade_exit();
                                }
                                else
                                {
                                    thiz_timeout_flag = 1;
                                }
                            }
                            else
                            {
                                _check_modify_pid();
                                GUI_EndDialog("wnd_ota_upgrade");
                            }
                        }
                    }
					break;
				case STBK_OK:
					_ota_ok_keypress();
					break;
				default:
					break;
			}
		default:
			break;
	}
#endif
	return ret;
}


SIGNAL_HANDLER int app_ota_cmb_sat_change(GuiWidget *widget, void *usrdata)
{
#if (OTA_SUPPORT > 0)
	uint32_t sel;
	
	//show new sat
	GUI_GetProperty("cmbox_ota_sat", "select", &sel);
	s_CurOtaSat.node_type = NODE_SAT;
	if(sel != s_CurOtaSat.pos)
	{
		s_CurOtaSat.pos = sel;
		s_wTpTotal = 0;
	}
	
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurOtaSat);
	_ota_cmb_tp_rebuild();
	_ota_set_tp(SAVE_TP_PARAM);
	
	//refresh sat para
	sel = 0;
	GUI_SetProperty("cmbox_ota_tp","select",&sel);

	s_set_tp_mode = SET_TP_FORCE;
	if(reset_timer(sp_OtaLockTimer) != 0) 
	{
		sp_OtaLockTimer = create_timer(timer_ota_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}
#endif	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ota_cmb_tp_change(GuiWidget *widget, void *usrdata)
{
#if (OTA_SUPPORT > 0)
	uint32_t box_sel;
	
	GUI_GetProperty("cmbox_ota_tp", "select", &box_sel);

	s_CurOtaTp.node_type = NODE_TP;
	s_CurOtaTp.pos = box_sel;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurOtaTp);

	s_set_tp_mode = SET_TP_AUTO;
	if(reset_timer(sp_OtaLockTimer) != 0) 
	{
		sp_OtaLockTimer = create_timer(timer_ota_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}	
#endif	
	return EVENT_TRANSFER_STOP;
}

#if 0
int app_ota_cmb_segment_change(GuiWidget *widget, void *usrdata)
{
#if (OTA_SUPPORT > 0)
	uint32_t box_sel;
	char section_buf[10];
	GUI_GetProperty("cmbox_ota_segment", "select", &box_sel);
	switch(box_sel)
	{
		case 0:
			sprintf(section_buf, "%s", "all");
			break;
		case 1:
			sprintf(section_buf, "%s", "kernel");
			break;
		case 2:
			sprintf(section_buf, "%s", "user");
			break;
		case 3:
			sprintf(section_buf, "%s", "data");
			break;
		default:
			break;
	}
#ifdef LINUX_OS
	char ota_cmd_buf[64];

	sprintf(ota_cmd_buf, "ota_write_section V_OEM \"%s\"", section_buf);
	system_shell(ota_cmd_buf, 0, NULL, NULL, NULL);
#else
	GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_FLAG_SECTION, section_buf);
	GxOem_Save();
#endif
	
#endif
	return EVENT_TRANSFER_STOP;
}
#endif

SIGNAL_HANDLER int app_ota_cmb_boot_change(GuiWidget *widget, void *usrdata)
{
#if (OTA_SUPPORT > 0)
	uint32_t box_sel;

#ifdef LINUX_OS
	char ota_cmd_buf[64];
	GUI_GetProperty("cmbox_ota_boot", "select", &box_sel);
	if(1 == box_sel)
	{
		sprintf(ota_cmd_buf, "ota_flag_update V_OEM \"%s\"", "yes");
	}
	else
	{
		sprintf(ota_cmd_buf, "ota_flag_update V_OEM \"%s\"", "no");
	}
	system_shell(ota_cmd_buf, 600000, NULL, NULL, NULL);
#else
	GUI_GetProperty("cmbox_ota_boot", "select", &box_sel);
	if(0 == box_sel)//On
	{
        app_loader_ota_get_update_flag((unsigned char*)&box_sel);
        if(0 == box_sel)
        {
            app_loader_ota_set_update_flag(1);
        }
	}
	else
	{
        app_loader_ota_get_update_flag((unsigned char*)&box_sel);
        if(1 == box_sel)
        {
            app_loader_ota_set_update_flag(0);
        }
	}
#endif
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ota_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
#if (OTA_SUPPORT > 0)
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
                    {
                        if(0 == g_nFlagOta)
                        {
                            uint32_t box_sel;
                            GUI_GetProperty("box_ota_upgrade_options", "select", &box_sel);
                            if(OTA_BOX_ITEM_SAT == box_sel)
                            {
                                box_sel = (OTA_BOX_ITEM_MAX - 1);
                                GUI_SetProperty("box_ota_upgrade_options", "select", &box_sel);
                                ret = EVENT_TRANSFER_STOP;
                            }
                            else
                            {
                                ret = EVENT_TRANSFER_KEEPON;
                            }
                        }
                    }
					break;
				case STBK_DOWN:
                    {
                        if(0 == g_nFlagOta)
                        {
                            uint32_t box_sel;
                            GUI_GetProperty("box_ota_upgrade_options", "select", &box_sel);
                            if((OTA_BOX_ITEM_MAX - 1) == box_sel)
                            {
                                box_sel = OTA_BOX_ITEM_SAT;
                                GUI_SetProperty("box_ota_upgrade_options", "select", &box_sel);
                                ret = EVENT_TRANSFER_STOP;
                            }
                            else
                            {
                                ret = EVENT_TRANSFER_KEEPON;
                            }
                        }
                    }
					break;
				case STBK_OK:
				case STBK_MENU:
				case STBK_EXIT:
				case STBK_LEFT:
				case STBK_RIGHT:
					ret = EVENT_TRANSFER_KEEPON;
					break;
				default:
					break;
			}
		default:
			break;
	}
#endif
	return ret;
}

bool app_ota_idle(void)
{
	return (g_chFlagEntryFlash == 0 && g_nFlagOta == 0);
}

