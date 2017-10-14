#include "app.h"
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_frontend.h"
#include "app_default_params.h"
#include "app_pop.h"
#include <math.h>

#define   MAX_ANGLE_LONGITUDE               36000
#define   MAX_LONGITUDE                     18000
#define   HALF_MAX_LONGITUDE                9000
#define   MAX_LATITUDE                      9000
#define   MIN_LATITUDE                      0
#define   MAX_ANGLE_STAB_USALS              16000
#define   RADIUS_OF_EARTH                   6378
#define   RADIUS_OF_GEO_ORBIT               42164//42164.2
#define   QUADATE_RADIUS_OF_EARTH           40678884
#define   QUADATE_RADIUS_OF_GEO_ORBIT       1.7778028964e9// 3.555639523e9
#define   PI                                3.141592654
#define   COM_SIN			sin    //sin计算
#define   COM_SQRT		sqrt    // sqrt计算
#define   COM_COS			cos   //cos计算
#define   COM_ACOS		acos    //acos计算

#define DISH_MOVE_SEC 70

//static bool thiz_interrupt_flag = false;

static uint32_t diseqcTuner = 0;
void app_diseqc12_set_tuner_id(int id)
{
	diseqcTuner = id;
}

uint32_t app_diseqc12_get_tuner_id(void)
{
	return diseqcTuner;
}


void app_lnb_freq_sel_to_data(uint16_t cur_sel, GxBusPmDataSat *p_sat)
{
#if UNICABLE_SUPPORT
	if(p_sat->sat_s.unicable_para.lnb_fre_index!=0X20)
	{
		switch (cur_sel)
		{
			case ID_UNICANBLE_9750_10600:
				p_sat->sat_s.lnb1 = 9750;
				p_sat->sat_s.lnb2 = 10600;
				break;
			case ID_UNICANBLE_9750_10700:
				p_sat->sat_s.lnb1 = 9750;
				p_sat->sat_s.lnb2 = 10700;
				break;
			case ID_UNICANBLE_9750_10750:
				p_sat->sat_s.lnb1 = 9750;
				p_sat->sat_s.lnb2 = 10750;
				break;
			case ID_UNICANBLE_9750:
				p_sat->sat_s.lnb1 = 9750;
				p_sat->sat_s.lnb2 = 0;
				break;
			case ID_UNICANBLE_10000:
				p_sat->sat_s.lnb1 = 10000;
				p_sat->sat_s.lnb2 = 0;
				break;
			case ID_UNICANBLE_10600:
				p_sat->sat_s.lnb1 = 10600;
				p_sat->sat_s.lnb2 = 0;
				break;
			case ID_UNICANBLE_10700:
				p_sat->sat_s.lnb1 = 10700;
				p_sat->sat_s.lnb2 = 0;
				break;
			case ID_UNICANBLE_10750:
				p_sat->sat_s.lnb1 = 10750;
				p_sat->sat_s.lnb2 = 0;
				break;
			case ID_UNICANBLE_11250:
				p_sat->sat_s.lnb1 = 11250;
				p_sat->sat_s.lnb2 = 0;
				break;
			case ID_UNICANBLE_11300:
				p_sat->sat_s.lnb1 = 11300;
				p_sat->sat_s.lnb2 = 0;
				break;
		}
		printf("unicable lnb1=%d,lnb2=%d\n",p_sat->sat_s.lnb1,p_sat->sat_s.lnb2);
		return;
	}
#endif
	switch (cur_sel)
	{
		case 0:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 10600;
			break;
		case 1:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 10700;
			break;
		case 2:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 10750;
			break;
		/*case 3:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 11300;
			break;*/
		case 3:
			p_sat->sat_s.lnb1 = 5150;
			p_sat->sat_s.lnb2 = 5750;
			break;
		case 4:
			p_sat->sat_s.lnb1 = 5750;
			p_sat->sat_s.lnb2 = 5150;
			break;
		case 5:
			p_sat->sat_s.lnb1 = 5150;
			p_sat->sat_s.lnb2 = 0;
			break;
		case 6:
			p_sat->sat_s.lnb1 = 5750;
			p_sat->sat_s.lnb2 = 0;
			break;
		case 7:
			p_sat->sat_s.lnb1 = 5950;
			p_sat->sat_s.lnb2 = 0;
			break;
		case 8:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 0;
			break;
		case 9:
			p_sat->sat_s.lnb1 = 10000;
			p_sat->sat_s.lnb2 = 0;
			break;
		case 10:
			p_sat->sat_s.lnb1 = 10600;
			p_sat->sat_s.lnb2 = 0;
			break;
		case 11:
			p_sat->sat_s.lnb1 = 10700;
			p_sat->sat_s.lnb2 = 0;
			break;
		case 12:
			p_sat->sat_s.lnb1 = 10750;
			p_sat->sat_s.lnb2 = 0;
			break;
		case 13:
			p_sat->sat_s.lnb1 = 11250;
			p_sat->sat_s.lnb2 = 0;
			break;
		case 14:
			p_sat->sat_s.lnb1 = 11300;
			p_sat->sat_s.lnb2 = 0;
			break;
#if  UNICABLE_SUPPORT
		case ID_UNICABLE:
			p_sat->sat_s.lnb1 = 9750;
			p_sat->sat_s.lnb2 = 10600;
#endif
		default:
			break;
	}
}

void app_lnb_freq_data_to_sel(GxBusPmDataSat *p_sat, uint32_t *p_sel)
{
	if (p_sat->sat_s.lnb1 == 0
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
	&&p_sat->type == GXBUS_PM_SAT_S
//#endif
	) 
	{
		printf("[Antenna Setting] sat param err\n");
		APP_ASSERT(0);
		return;
	}
#if UNICABLE_SUPPORT
	if(p_sat->sat_s.unicable_para.lnb_fre_index!=0X20)
	{
		*p_sel = ID_UNICABLE;
		return;
	}
#endif
	if(p_sat->sat_s.lnb2 == 0 ||p_sat->sat_s.lnb2 == p_sat->sat_s.lnb1)
	{
		switch(p_sat->sat_s.lnb1)
		{
			case 5150:	*p_sel=5;		break;
			case 5750:	*p_sel=6;		break;
			case 5950:	*p_sel=7;		break;
			case 9750:	*p_sel=8;		break;
			case 10000:	*p_sel=9;	break;
			case 10600:	*p_sel=10;	break;
			case 10700:	*p_sel=11;	break;
			case 10750:	*p_sel=12;	break;
			case 11250:	*p_sel=13;	break;
			case 11300:	*p_sel=14;	break;
			default:	
				*p_sel = 0;
				printf("[dishset] err1\n");	break;
		}
	}
	else
	{
		switch(p_sat->sat_s.lnb2)
		{
			case 5750:	*p_sel=3;		break;
			case 5150:	*p_sel=4;		break;
			
			case 10600:	*p_sel=0;		break;
			case 10700:	*p_sel=1;		break;
			case 10750:	*p_sel=2;		break;
			//case 11300:	*p_sel=3;		break;
			default:
				*p_sel = 0;
				printf("[dishset] err2\n");	break;
		}
	}
}

//transfer pm to frontend
uint32_t app_show_to_sat_22k(GxBusPmSat22kSwitch switch_22K, GxBusPmDataTP *p_tp)
{
#define DIVERSION_FREQUENCY     (11700)
	static uint32_t tone_22k = 0;

	switch (switch_22K)
	{
		case GXBUS_PM_SAT_22K_OFF:
			tone_22k = SEC_TONE_OFF;
			break;
			
		case GXBUS_PM_SAT_22K_ON:
			tone_22k = SEC_TONE_ON;
			break;

		default:
			if(p_tp->frequency >= DIVERSION_FREQUENCY)
			{
				tone_22k = SEC_TONE_ON;
			}
			else
			{
			tone_22k = SEC_TONE_OFF;
			}
			break;
	}

	return tone_22k;
}

uint32_t app_show_to_tp_freq(GxBusPmDataSat *p_sat, GxBusPmDataTP *p_tp)
{
#define DIVERSION_FREQUENCY     (11700)
	uint16_t lnb_fre = 0;
	uint32_t fre=0;
	GxBusPmSatLnbType type = 0;

	if((p_sat->sat_s.lnb2==0) || (p_sat->sat_s.lnb1 == p_sat->sat_s.lnb2))
	{
		type = GXBUS_PM_SAT_LNB_USER;
	}
	else if(p_sat->sat_s.lnb1<6000)
	{
		type = GXBUS_PM_SAT_LNB_OCS;
	}
	else if(p_sat->sat_s.lnb1>6000)
	{
		type = GXBUS_PM_SAT_LNB_UNIVERSAL;
	}

	switch(type)
	{
		case GXBUS_PM_SAT_LNB_USER:
			lnb_fre = p_sat->sat_s.lnb1;
			break;

		case GXBUS_PM_SAT_LNB_UNIVERSAL:
			if (p_tp->frequency >= DIVERSION_FREQUENCY)
			{
				lnb_fre = p_sat->sat_s.lnb2;
			}
			else
			{
				lnb_fre = p_sat->sat_s.lnb1;
			}
            break;

		case GXBUS_PM_SAT_LNB_OCS:
			switch(p_tp->tp_s.polar)
			{
				case GXBUS_PM_TP_POLAR_H:
					lnb_fre = p_sat->sat_s.lnb1;
					break;

				case GXBUS_PM_TP_POLAR_V:
					lnb_fre = p_sat->sat_s.lnb2;
					break;

				case GXBUS_PM_TP_POLAR_AUTO:
					lnb_fre = p_sat->sat_s.lnb1;
					break;

				default:
					return GXCORE_ERROR;
					break;
			}
			break;

		default:
			#ifdef GX_BUS_SEARCH_DBUG 
			GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---sat lnb type err!!\n");
			#endif
			return GXCORE_ERROR;
			break;
	}
	//if(lnb_fre >= p_tp->frequency)
	if(lnb_fre < 6000)
	{
		fre = lnb_fre - p_tp->frequency;
	}
	else
	{
		fre = p_tp->frequency - lnb_fre;
	}
	return fre;
}

uint32_t app_show_to_lnb_polar(GxBusPmDataSat *p_sat)
{
// 0-off 1-on 2-13V 3-18V
	uint32_t polar = SEC_VOLTAGE_OFF;
	
	switch (p_sat->sat_s.lnb_power)
	{
		case GXBUS_PM_SAT_LNB_POWER_OFF:
			polar = SEC_VOLTAGE_OFF;
			break;
		case GXBUS_PM_SAT_LNB_POWER_ON:
			polar = SEC_VOLTAGE_ALL;
			break;
		case GXBUS_PM_SAT_LNB_POWER_13V:
			polar = SEC_VOLTAGE_13;
			break;
		case GXBUS_PM_SAT_LNB_POWER_18V:
			polar = SEC_VOLTAGE_18;
			break;
		default:
			polar = SEC_VOLTAGE_OFF;
			break;
	}
	return polar;
}

uint32_t app_show_to_tp_polar(GxBusPmDataTP *p_tp)
{
// 0-18V 1-13V 2-auto
	uint32_t polar = SEC_VOLTAGE_OFF;

	switch (p_tp->tp_s.polar)
	{
		case GXBUS_PM_TP_POLAR_H:
			polar = SEC_VOLTAGE_18;
			break;
		case GXBUS_PM_TP_POLAR_V:
			polar = SEC_VOLTAGE_13;
			break;
		default:
			polar = SEC_VOLTAGE_OFF;
			break;
	}

	return polar;
}

void app_set_diseqc_10(uint32_t tuner, AppFrontend_DiseqcParameters *diseqc10, bool force)
{
    AppFrontend_DiseqcChange change;

    change.diseqc = diseqc10;
    app_ioctl(tuner, FRONTEND_DISEQC_CHANGE, &change);

    if(force == true || change.state == FRONTEND_DISEQC_CHANGED) 
    {
        app_ioctl(tuner, FRONTEND_DISEQC_SET, diseqc10);
        printf("--------set diseqc 1.0, %d--------\n", diseqc10->u.params_10.chDiseqc);  
    }
    
//	if((force == true) 
//		|| (g_AppNim.diseqc_change(&g_AppNim, diseqc10) == true))//(diseqc_port != diseqc10->u.params_10.chDiseqc)
//	{
//		g_AppNim.property_set(&g_AppNim, AppFrontendID_DiseqcCmd, (void*)diseqc10, 
//		        sizeof(AppFrontend_DiseqcParameters));
//		printf("--------set diseqc 1.0, %d--------\n", diseqc10->u.params_10.chDiseqc);  
//	}

}

void app_set_diseqc_11(uint32_t tuner, AppFrontend_DiseqcParameters *diseqc11, bool force)
{ 
    AppFrontend_DiseqcChange change;

    change.diseqc = diseqc11;
    app_ioctl(tuner, FRONTEND_DISEQC_CHANGE, &change);
 
    if(force == true || change.state == FRONTEND_DISEQC_CHANGED) 
    {
        app_ioctl(tuner, FRONTEND_DISEQC_SET, diseqc11);
        printf("--------set diseqc 1.1, %d--------\n", diseqc11->u.params_11.chUncom);  
    }
 
//	if((force == true) 
//		|| (g_AppNim.diseqc_change(&g_AppNim, diseqc11) == true))//(diseqc_port != diseqc11->u.params_11.chUncom)
//	{
//		g_AppNim.property_set(&g_AppNim, AppFrontendID_DiseqcCmd, (void*)diseqc11, 
//		        sizeof(AppFrontend_DiseqcParameters));
//		printf("--------set diseqc 1.1, %d--------\n", diseqc11->u.params_11.chUncom);  
//	}
}


// Flag == FALSE ,only used in play function now, for monitor open
void app_frontend_demod_control(uint32_t Tuner,uint8_t Flag)
{
#if DOUBLE_S2_SUPPORT
		static uint8_t UseFlag = 0;
		static int32_t TunerRecord = -1;
		
		if((TRUE == Flag) || (TRUE == UseFlag) || (TunerRecord != Tuner))
		{
			AppFrontend_Monitor monitor = FRONTEND_MONITOR_ON;
			AppFrontend_LNBState LnbOutput = FRONTEND_LNB_ON;
	// 切换两个没有TP的卫星的时候，会出现打开LNB供电的操作，导致界面出现错误的锁频信息。
			if(TunerRecord != Tuner)
				TunerRecord = Tuner;
			
	        app_ioctl(Tuner, FRONTEND_MONITOR_SET, &monitor);
			//app_ioctl(tuner, FRONTEND_LNB_CFG, &LnbOutput);
			if(g_AppPvrOps.state != PVR_DUMMY)
			{
				GxMsgProperty_NodeByIdGet node;
		    
		    	node.node_type = NODE_PROG;
		    	node.pos = g_AppPvrOps.env.prog_id;
		   	 	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void*)&node);
				if(node.prog_data.tuner == Tuner)
				{
					goto TunerCfg;
				}
			}
			else
			{
	TunerCfg:	if(Tuner == 1)
				{
					monitor = FRONTEND_MONITOR_OFF;
					app_ioctl(0, FRONTEND_MONITOR_SET, &monitor);
					LnbOutput = FRONTEND_LNB_OFF;
					app_ioctl(0, FRONTEND_LNB_CFG, &LnbOutput);
				}
				else
				{
					monitor = FRONTEND_MONITOR_OFF;
					app_ioctl(1, FRONTEND_MONITOR_SET, &monitor);
					LnbOutput = FRONTEND_LNB_OFF;
					app_ioctl(1, FRONTEND_LNB_CFG, &LnbOutput);
				}
			}
		}

		UseFlag = Flag;
#endif

}

static void app_frontend_monitor_control(int32_t tuner)
{
#if NDEMOD_WITH_1TUNER
    uint32_t i = 0;
	AppFrontend_Monitor monitor = FRONTEND_MONITOR_ON;

    for(i = 0; i < NIM_MODULE_NUM; i++)
    {
        if(tuner  == i)
        {
            monitor = FRONTEND_MONITOR_ON;
            printf("[%s]>> Start tuner%d monitor!\n", __func__, i);
        }
        else
        {
            monitor = FRONTEND_MONITOR_OFF;
            printf("[%s]>> Stop tuner%d monitor!\n", __func__, i);
        }

		app_ioctl(i, FRONTEND_MONITOR_SET, &monitor);
    }

    return;
#endif
}

void app_set_tp(uint32_t tuner, AppFrontend_SetTp *params , SetTpMode set_mode)
{
    static AppFrontend_SetTp params_bak[3];

	if(tuner > 3)
	{
		printf("\nSET TP, error, %s, %d\n",__FUNCTION__,__LINE__);
		return ;
	}
    if(set_mode == SAVE_TP_PARAM)
    {
        memcpy(&params_bak, params, sizeof(AppFrontend_SetTp));
        return;
    }
    //check if tp params changed
    if(params->sat22k != params_bak[tuner].sat22k
            || params->fre != params_bak[tuner].fre
            || params->symb != params_bak[tuner].symb
            || params->polar != params_bak[tuner].polar
            || params->tuner != params_bak[tuner].tuner
	#if UNICABLE_SUPPORT
            || params->if_channel_index != params_bak[tuner].if_channel_index
            || params->lnb_index != params_bak[tuner].lnb_index
            || params->centre_fre != params_bak[tuner].centre_fre
            || params->if_fre != params_bak[tuner].if_fre

    #endif
            || set_mode == SET_TP_FORCE)
    {
        app_frontend_demod_control(tuner,TRUE);
        app_frontend_monitor_control(tuner);
        //		if(params->tuner != params_bak.tuner)
        app_ioctl(tuner,FRONTEND_DEMUX_CFG,NULL);

        app_ioctl(tuner, FRONTEND_TP_SET, params);

        printf("--------set tp, frequency = %d, symbol_rate = %d, voltage = %d, sat22k = %d--------\n",
                params->fre, params->symb, params->polar, params->sat22k);
        // for monitor
        //save paras...
        memcpy(&params_bak[tuner], params, sizeof(AppFrontend_SetTp));

#if EMM_SUPPORT
        {
            AppFrontend_Config config;
            app_ioctl(tuner, FRONTEND_CONFIG_GET, &config);
            struct app_cat cat = {0};
            cat.ts_src = config.tuner;
            cat.demuxid = config.dmx_id;
            cat.pid = CAT_PID;
            cat.timeout = 5;
            cat.ver = 0xff;
            app_cat_start(&cat);                                                                                                 
        }
#endif

#if OTA_MONITOR_SUPPORT
        {
            AppFrontend_Config config;
            app_ioctl(tuner, FRONTEND_CONFIG_GET, &config);
            struct app_ota_nit nit = {0};
            nit.ts_src = config.tuner;
            nit.demuxid = config.dmx_id;

            app_ota_nit_start(&nit);                                                                                                 
        }
#endif
    }
    return;
}

// MAX count FRONTEND_DISEQC_MAX_CMD
void app_diseqc12_stop(uint32_t tuner, uint8_t Count)
{
	AppFrontend_DiseqcParameters diseqc12 = {0};
	uint8_t i = 0;
	if (Count > FRONTEND_DISEQC_MAX_CMD)
		Count = FRONTEND_DISEQC_MAX_CMD;
	diseqc12.type = DiSEQC12;
	diseqc12.u.params_12.CmdCount = Count;

	for(i = 0; i < Count;i++)
		diseqc12.u.params_12.DiseqcControl[i] = DISEQC_STOP;

    app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc12);
}

void app_diseqc12_stop_drictory(uint32_t tuner, uint8_t Count)
{
	AppFrontend_DiseqcParameters diseqc12 = {0};
	uint8_t i = 0;
	if (Count > FRONTEND_DISEQC_MAX_CMD)
		Count = FRONTEND_DISEQC_MAX_CMD;
	diseqc12.type = DiSEQC12;
	diseqc12.u.params_12.CmdCount = Count;

	for(i = 0; i < Count;i++)
		diseqc12.u.params_12.DiseqcControl[i] = DISEQC_STOP;

    app_ioctl(tuner, FRONTEND_DISEQC_SET_EX, &diseqc12);
}

static uint32_t sg_Tuner = 0xff;
static int _stop_diseqc12_cb(PopDlgRet ret)
{
    // TODO: ERROR
    /*if(ret == POP_VAL_CANCEL)
    {
        thiz_interrupt_flag = true;
    }
    else if(POP_VAL_OK == ret)
    {
        thiz_interrupt_flag = false;
    }*/
    app_diseqc12_stop(sg_Tuner,1);
    sg_Tuner = 0xff;
    return 0;
}

void app_diseqc12_save_position(uint32_t tuner,uint32_t pos)
{
	AppFrontend_DiseqcParameters diseqc = {0};
	
	//app_diseqc12_stop(tuner,1);

	diseqc.type = DiSEQC12;
	diseqc.u.params_12.CmdCount = 2;
	// stop first "app_diseqc12_stop(tuner,1);"
	diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
	// store cmd
	diseqc.u.params_12.DiseqcControl[1] = DISEQC_STORE_NN;
	diseqc.u.params_12.DiSEqC12Params[1].m_chParam0 = pos;
	
    app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc);


}
void app_diseqc12_goto_position(uint32_t tuner, uint32_t pos, bool force)
{
	AppFrontend_DiseqcParameters diseqc12 = {0};
	AppFrontend_DiseqcChange change;
	PopDlg pop;
	//AppFrontend_PolarState Polar = SEC_VOLTAGE_18;

	extern int app_get_motor_state_in_fullscreen(void); 


	if(app_get_motor_state_in_fullscreen())
	{
		diseqc12.type = DiSEQC12;
		diseqc12.u.params_12.CmdCount = 2;
		// stop cmd
		diseqc12.u.params_12.DiseqcControl[0] = DISEQC_STOP;
		// goto position
		diseqc12.u.params_12.DiseqcControl[1] = DISEQC_GOTO_NN;
		diseqc12.u.params_12.DiSEqC12Params[1].m_chParam0 = pos;
	}
	else
	{
		diseqc12.type = DiSEQC12;
		diseqc12.u.params_12.CmdCount = 1;
		// goto position
		diseqc12.u.params_12.DiseqcControl[0] = DISEQC_GOTO_NN;
		diseqc12.u.params_12.DiSEqC12Params[0].m_chParam0 = pos;
	}

    change.diseqc = &diseqc12;
    app_ioctl(tuner, FRONTEND_DISEQC_CHANGE, &change);

	if((force == true)
       /* || (thiz_interrupt_flag == true)*/
		|| (change.state == FRONTEND_DISEQC_CHANGED))
	{
        //thiz_interrupt_flag = false;
		app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc12);
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.str = STR_ID_DISH_MOVING;
		pop.creat_cb = NULL;
		sg_Tuner = tuner;

		if(app_get_motor_state_in_fullscreen())
		{
			pop.exit_cb = NULL;
			pop.timeout_sec= 3;
			pop.mode = POP_MODE_BLOCK;
		}
		else
		{
			app_diseqc12_set_tuner_id(tuner);
			pop.exit_cb = _stop_diseqc12_cb;
			pop.mode = POP_MODE_BLOCK;
			pop.timeout_sec= DISH_MOVE_SEC;
			popdlg_create(&pop);
		}
	}
}

void app_diseqc12_goto_zero(uint32_t tuner, bool force)
{
	app_diseqc12_goto_position(tuner, 0,force);
}

void app_diseqc12_find_drive(uint32_t tuner,LongitudeDirect direct)
{
	AppFrontend_DiseqcParameters diseqc = {0};

	//app_diseqc12_stop(tuner,1);
	
	diseqc.type = DiSEQC12;
	diseqc.u.params_12.CmdCount = 2;
	diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
	
	if(direct == LONGITUDE_EAST)
	{
		diseqc.u.params_12.DiseqcControl[1] = DISEQC_DRIVE_EAST;
	}
	else
	{
		diseqc.u.params_12.DiseqcControl[1] = DISEQC_DRIVE_WEST;
	}

    app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc);

}

void app_diseqc12_find_drive_drictory(uint32_t tuner,LongitudeDirect direct)
{
	AppFrontend_DiseqcParameters diseqc = {0};

	//app_diseqc12_stop(tuner,1);
	
	diseqc.type = DiSEQC12;
	diseqc.u.params_12.CmdCount = 2;
	diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
	
	if(direct == LONGITUDE_EAST)
	{
		diseqc.u.params_12.DiseqcControl[1] = DISEQC_DRIVE_EAST;
	}
	else
	{
		diseqc.u.params_12.DiseqcControl[1] = DISEQC_DRIVE_WEST;
	}

    app_ioctl(tuner, FRONTEND_DISEQC_SET_EX, &diseqc);

}

void app_diseqc12_save_limit(uint32_t tuner,Diseqc12Limit limit)
{

	AppFrontend_DiseqcParameters diseqc = {0};      

	//app_diseqc12_stop();
	
	diseqc.type = DiSEQC12;
	diseqc.u.params_12.CmdCount = 2;
	switch(limit)
	{
		case LIMIT_EAST:
			diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
			diseqc.u.params_12.DiseqcControl[1] = DISEQC_LIMIT_EAST;
			break;

		case LIMIT_WEST:
			diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
			diseqc.u.params_12.DiseqcControl[1] = DISEQC_LIMIT_WEST;
			break;

		default:
			diseqc.u.params_12.CmdCount = 1;
			diseqc.u.params_12.DiseqcControl[0] = DISEQC_LIMIT_OFF;
			//diseqc.u.params_12.DiseqcControl[1] = DISEQC_LIMIT_OFF;
			break;		
	}

	app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc);
}

void app_diseqc12_recalculate(uint32_t tuner,uint32_t pos)
{
	int init_value = 0;
	AppFrontend_DiseqcParameters diseqc;

	//app_diseqc12_stop(tuner,1);


	diseqc.type = DiSEQC12;
	diseqc.u.params_12.CmdCount = 2;
	// for stop
	diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
	// for recalculate
	diseqc.u.params_12.DiseqcControl[1] = DISEQC_SET_POSNS;
	diseqc.u.params_12.DiSEqC12Params[1].m_chParam0 = pos;
	diseqc.u.params_12.DiSEqC12Params[1].m_chParam1 = 0;
	GxBus_ConfigGetInt(LONGITUDE_KEY, &init_value, LONGITUDE);
	diseqc.u.params_12.DiSEqC12Params[1].m_chParam2 = init_value/10;//local Longitude
	GxBus_ConfigGetInt(LATITUDE_KEY, &init_value, LATITUDE);
	diseqc.u.params_12.DiSEqC12Params[1].m_chParam3 = init_value/10;

	app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc);

}

static status_t _usals_diseqc12_calculate_azimuth(LongitudeInfo *local_longitude, LatitudeInfo *local_latitude, LongitudeInfo *sat_longitude, int *ret_angle)
{
	int sat_Longitude_val;
	int local_Longitude_val;
	int local_Latitude_val;
	int      iG20;
	float    iG23;
	float    iG24;
	float    iG25;
	float    iG26;
	float    iG29;
	float    iG30;
	float    iG31;
	float    iG32;
	float    iG33;
	int      iG34;
	int      iG35;
	int      iG36;
	int      iG37;

	local_Longitude_val = local_longitude->longitude * 10;
	if(local_longitude->longitude_direct == LONGITUDE_WEST)
	{
		local_Longitude_val = -local_Longitude_val;
		if (( local_Longitude_val < -MAX_LONGITUDE )||( local_Longitude_val > MAX_LONGITUDE ))
		{
			return GXCORE_ERROR;//STUSIF_ERROR_USALS_Longitude_OUT_OF_RANGE;
		}
	}

	local_Latitude_val = local_latitude->latitude * 10;
	if(local_latitude->latitude_direct == LATITUDE_SOUTH)
	{
		local_Latitude_val = -local_Latitude_val;
		if (( local_Latitude_val < -MAX_LATITUDE )||( local_Latitude_val > MAX_LATITUDE ))
		{
			return GXCORE_ERROR ;//STUSIF_ERROR_USALS_Latitude_OUT_OF_RANGE;
		}
	}

	sat_Longitude_val = sat_longitude->longitude *10;
	if(sat_longitude->longitude_direct == LONGITUDE_WEST)
	{
		sat_Longitude_val = -sat_Longitude_val;
		if (( sat_Longitude_val < -MAX_LONGITUDE )||( sat_Longitude_val > MAX_LONGITUDE ))
		{
			return GXCORE_ERROR ;//STUSIF_ERROR_USALS_ANGLE_OUT_OF_RANGE;
		}
	}

	iG20 = sat_Longitude_val - local_Longitude_val;
	if (( iG20 > MAX_LONGITUDE ))
	{
		iG20 = MAX_ANGLE_LONGITUDE - iG20;
	}
	if (( iG20 < -MAX_ANGLE_STAB_USALS )||( iG20 > MAX_ANGLE_STAB_USALS ))
	{
		*ret_angle = iG20;
		return GXCORE_SUCCESS;//STUSIF_ERROR_USALS_ANGLE_STAB_USALS_OUT_OF_RANGE;
	}
	
	iG23 = (float)RADIUS_OF_EARTH*COM_SIN((float)PI * (float)local_Latitude_val / (float)MAX_LONGITUDE);
	iG24 = COM_SQRT(QUADATE_RADIUS_OF_EARTH - iG23 * iG23);
	iG25 = COM_SQRT(QUADATE_RADIUS_OF_GEO_ORBIT+iG24*iG24-2*RADIUS_OF_GEO_ORBIT*iG24*COM_COS((float)PI*(float)iG20/(float)MAX_LONGITUDE));
	iG26 = COM_SQRT(iG23 * iG23 + iG25 * iG25 );

	iG29 = iG26;
	iG30 = COM_SQRT(((float)RADIUS_OF_GEO_ORBIT-iG24)*((float)RADIUS_OF_GEO_ORBIT-iG24)+iG23*iG23);
	iG31 = COM_SQRT(2*((float)QUADATE_RADIUS_OF_GEO_ORBIT-(float)QUADATE_RADIUS_OF_GEO_ORBIT*COM_COS((float)PI*(float)iG20/(float)MAX_LONGITUDE)));
	//iG38 = (double)( iG29 * iG29 + iG30 * iG30 - iG31 * iG31 ) / ( 2 * iG29 * iG30 );
	iG32 = COM_ACOS((iG29*iG29+iG30*iG30-iG31*iG31)/(2*iG29*iG30));
	iG33 = iG32*(float)MAX_LONGITUDE/(float)PI;
	iG34 = (int)(iG33);

	if (( iG34 < -MAX_ANGLE_STAB_USALS )||( iG34 > MAX_ANGLE_STAB_USALS ))
	{	
		*ret_angle = iG34;
		return GXCORE_SUCCESS ;//STUSIF_ERROR_USALS_ANGLE_STAB_USALS_OUT_OF_RANGE2;
	}
	if ((( sat_Longitude_val < local_Longitude_val ) && ( local_Latitude_val > MIN_LATITUDE)) 
		|| (( sat_Longitude_val > local_Longitude_val ) && ( local_Latitude_val < MIN_LATITUDE)))
	{
		iG35 =   iG34;
	}
	else
	{
		iG35 = - iG34;
	}
	if (( sat_Longitude_val <= -HALF_MAX_LONGITUDE ) && ( local_Longitude_val >= HALF_MAX_LONGITUDE)) 
	{
		iG36 = - iG35;
	}
	else
	{
		iG36 =   iG35;
	}
	if (( sat_Longitude_val >= HALF_MAX_LONGITUDE ) && ( local_Longitude_val <= -HALF_MAX_LONGITUDE)) 
	{
		iG37 = - iG35;
	}
	else
	{
		iG37 = iG36;
	}
	if( iG37 >= 0)
	{ 
		if( iG37%10 >= 5)
		{
			iG37 = iG37 + 10 - iG37%10;
		}
		else
		{
			iG37 = iG37 - iG37%10;
		}
	}
	else
	{
		if( iG37%10 <= -5)
		{
			iG37 = iG37 - 10 - iG37%10;
		}
		else
		{
			iG37 = iG37  - iG37%10;
		}
	}
	*ret_angle = iG37;
	return GXCORE_SUCCESS;
}

static status_t _get_usals_cmd(LongitudeInfo *local_longitude, LatitudeInfo *local_latituide, LongitudeInfo *sat_longitude, int8_t *cmd1, int8_t *cmd2)
{
	int32_t nTurnAngle;
	status_t ret = GXCORE_ERROR;
	
	if(_usals_diseqc12_calculate_azimuth(local_longitude, local_latituide, sat_longitude, &nTurnAngle) == GXCORE_SUCCESS)
	{
		nTurnAngle /=10;
		if(nTurnAngle != 0 && nTurnAngle < 800 && nTurnAngle > -800)
		{
			*cmd1 = 0;
			*cmd2 = 0;
			
			if(nTurnAngle > 0)
			{
				*cmd1 |= 0xd0;
			}
			else 
			{
				*cmd1 |= 0xe0;
				nTurnAngle = -nTurnAngle;
			}
			*cmd1 |= (int8_t)(nTurnAngle/10/16);
			*cmd2 |= (int8_t)(nTurnAngle/10%16);
			*cmd2 <<= 4;
			switch(nTurnAngle % 10)
			{
				case 0: *cmd2 |= 0x0; break;
				case 1: *cmd2 |= 0x2; break;
				case 2: *cmd2 |= 0x3; break;
				case 3: *cmd2 |= 0x5; break;
				case 4: *cmd2 |= 0x6; break;
				case 5: *cmd2 |= 0x8; break;
				case 6: *cmd2 |= 0xa; break;
				case 7: *cmd2 |= 0xb; break;
				case 8: *cmd2 |= 0xd; break;
				case 9: *cmd2 |= 0xe; break;
				default:
					break;
			}
			ret = GXCORE_SUCCESS;
		}
	}

	return ret;
}

void app_usals_goto_position(uint32_t tuner, LongitudeInfo *sat_longitude, PositionVal *local_position, bool force)
{
    int8_t cmd1 = 0;
    int8_t cmd2 = 0;
    LongitudeInfo local_longitude;
    LatitudeInfo local_latitude;
    PopDlg pop;
    extern int app_get_motor_state_in_fullscreen(void); 

    static LongitudeInfo local_longitude_err = {-1, LONGITUDE_ERR};
    static LatitudeInfo local_latitude_err = {-1, LATITUDE_ERR};
    static LongitudeInfo sat_longitude_err = {-1, LONGITUDE_ERR};

    if(local_position == NULL)
    {
        GxBus_ConfigGetInt(LONGITUDE_KEY, &local_longitude.longitude, LONGITUDE);
        GxBus_ConfigGetInt(LONGITUDE_DIRECT_KEY, (int*)&local_longitude.longitude_direct, LONGITUDE_DIRECT);
        GxBus_ConfigGetInt(LATITUDE_KEY, &local_latitude.latitude, LATITUDE);
        GxBus_ConfigGetInt(LATITUDE_DIRECT_KEY, (int*)&local_latitude.latitude_direct, LATITUDE_DIRECT);
    }
    else
    {
        local_longitude.longitude = local_position->longitude.longitude;
        local_longitude.longitude_direct = local_position->longitude.longitude_direct;
        local_latitude.latitude = local_position->latitude.latitude;
        local_latitude.latitude_direct = local_position->latitude.latitude_direct;
    }

    if(_get_usals_cmd(&local_longitude, &local_latitude, sat_longitude, &cmd1, &cmd2) == GXCORE_SUCCESS)
    {
        AppFrontend_DiseqcChange change;
        AppFrontend_DiseqcParameters diseqc = {0};

        diseqc.type = DiSEQC12;
        if(app_get_motor_state_in_fullscreen())
        {
            diseqc.u.params_12.CmdCount = 2;
            // for stop
            diseqc.u.params_12.DiseqcControl[0] = DISEQC_STOP;
            // for goto position
            diseqc.u.params_12.DiseqcControl[1] = DISEQC_GOTO_XX;
            diseqc.u.params_12.DiSEqC12Params[1].m_chParam0 = cmd1;
            diseqc.u.params_12.DiSEqC12Params[1].m_chParam1 = cmd2;
        }
        else
        {
            diseqc.u.params_12.CmdCount = 1;
            // for goto position
            diseqc.u.params_12.DiseqcControl[0] = DISEQC_GOTO_XX;
            diseqc.u.params_12.DiSEqC12Params[0].m_chParam0 = cmd1;
            diseqc.u.params_12.DiSEqC12Params[0].m_chParam1 = cmd2;
        }

        change.diseqc = &diseqc;
        app_ioctl(tuner, FRONTEND_DISEQC_CHANGE, &change);

        if((force == true)
                /*|| (thiz_interrupt_flag == true)*/
                || (change.state == FRONTEND_DISEQC_CHANGED))
        {
            //thiz_interrupt_flag = false;
            local_longitude_err.longitude = -1;
            local_longitude_err.longitude_direct = LONGITUDE_ERR;
            local_latitude_err.latitude = -1;
            local_latitude_err.latitude_direct = LATITUDE_ERR;
            sat_longitude_err.longitude = -1;
            sat_longitude_err.longitude_direct = LONGITUDE_ERR;

            //app_diseqc12_stop(tuner,1);

            app_ioctl(tuner, FRONTEND_DISEQC_SET, &diseqc);

            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.str = STR_ID_DISH_MOVING;
            pop.creat_cb = NULL;
            sg_Tuner = tuner;

            if(app_get_motor_state_in_fullscreen())
            {
                pop.exit_cb = NULL;
                pop.timeout_sec= 3;
            }
            else
            {
                pop.exit_cb = _stop_diseqc12_cb;
                app_diseqc12_set_tuner_id(tuner);

                pop.timeout_sec= DISH_MOVE_SEC*2;//20141030
                popdlg_create(&pop);
            }
        }
    }
    else
    {
        if((force == true)
                || (local_longitude_err.longitude != local_longitude.longitude)
                || (local_longitude_err.longitude_direct != local_longitude.longitude_direct)
                || (local_latitude_err.latitude != local_latitude.latitude)
                || (local_latitude_err.latitude_direct != local_latitude.latitude_direct)
                || (sat_longitude_err.longitude != sat_longitude->longitude)
                || (sat_longitude_err.longitude_direct != sat_longitude->longitude_direct))
        {
            local_longitude_err.longitude = local_longitude.longitude;
            local_longitude_err.longitude_direct = local_longitude.longitude_direct;
            local_latitude_err.latitude = local_latitude.latitude;
            local_latitude_err.latitude_direct = local_latitude.latitude_direct;
            sat_longitude_err.longitude = sat_longitude->longitude;
            sat_longitude_err.longitude_direct = sat_longitude->longitude_direct;

            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.str = STR_ID_OUT_RANGE;
            pop.timeout_sec= 2;
            popdlg_create(&pop);
        }
    }
}

//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
#if (DEMOD_DVB_COMBO > 0)
void app_set_tune_mode(GxBusPmDataSatType type)
{
  //  uint8_t f[32] = {0};
 //   handle_t    frontend;
    int search_mod_flag = 0;
    //AppFrontend_SetTp params = {0};
    int32_t tuner = 0;
  //  sprintf((char*)f,"/dev/dvb/adapter0/frontend%d",tuner);
  //  frontend = open((char*)f,O_RDWR);

    switch(type)
    {
        case GXBUS_PM_SAT_S:
            {
                //   params.type = FRONTEND_DVB_S;
                search_mod_flag = DVBS2_NORMAL;
                break;
            }
        case GXBUS_PM_SAT_C:
            {
                //   params.type = FRONTEND_DVB_C;
                search_mod_flag = DVBC_J83A;
                break;
            }
        case GXBUS_PM_SAT_T:
        case GXBUS_PM_SAT_DVBT2:
            {
                search_mod_flag = DVBT_AUTO_MODE;//GX1801_DVB_T2;
                break;
            }
        default:
            {
                //params.type = FRONTEND_DVB_S;
                // search_mod_flag = GX1801_DVB_S2;
                break;
            }
    }
    printf("app set mode = %d \n", search_mod_flag);
	GxFrontend_SetMode(tuner, search_mod_flag);
//    ioctl(frontend, FE_SET_FRONTEND_TUNE_MODE, search_mod_flag);
//    close(frontend);

    //	 app_ioctl(sat.sat_data.tuner, FRONTEND_TP_SET, &params);
}
#endif

#if MUTI_TS_SUPPORT
extern void GxFrontend_SetTsid(uint32_t tuner, uint8_t ts_id,uint8_t code);
void app_set_muti_ts_para(uint32_t tuner,uint8_t ts_id,uint8_t code)
{
	static uint8_t thiz_ts_id = 0;

	if(thiz_ts_id != ts_id)
	{
		GxFrontend_SetTsid(tuner,ts_id,code);
		thiz_ts_id = ts_id;
	}
	
	return;
}

#endif


