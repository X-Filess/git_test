#include "app.h"
#if (DEMOD_DVB_S > 0)
#include "app_module.h"

#define SIGNAL_GRADIENT_COUNT 1

typedef struct
{
	uint8_t signal_value[SIGNAL_GRADIENT_COUNT];
	uint8_t signal_num;
}GradientSignal;

static AppFrontend_Monitor thiz_frontend_monitor[NIM_MODULE_NUM];
static int thiz_frontend_polar_status = 0;
/* Private functions ------------------------------------------------------ */

static void dvbs_diseqc_record(AppNim* nim, AppFrontend_DiseqcParameters* diseqc)
{
    // record DiSEqC param, when unlock can set DiSEqC by the record param
    if (diseqc->type == DiSEQC10)
    {
        // update DiSEqC1.1
        nim->diseqc11.u.params_11.b22k = nim->diseqc10.u.params_10.b22k;
        nim->diseqc11.u.params_11.chCom = nim->diseqc10.u.params_10.chDiseqc;
        nim->diseqc11.u.params_11.bPolar = nim->diseqc10.u.params_10.bPolar;

        memcpy(&nim->diseqc10, diseqc, sizeof(AppFrontend_DiseqcParameters));
    }
    else if (diseqc->type == DiSEQC11)
    {
        memcpy(&nim->diseqc11, diseqc, sizeof(AppFrontend_DiseqcParameters));
    }
    else if (diseqc->type == DiSEQC12) //record  move dish cmd
    {
		uint8_t i = 0;
		for(i = 0; i < diseqc->u.params_12.CmdCount; i++)
		{
	        if((diseqc->u.params_12.DiseqcControl[i] == DISEQC_GOTO_NN)
	            || (diseqc->u.params_12.DiseqcControl[i] == DISEQC_GOTO_XX))
	        {
	            memcpy(&nim->diseqc12, diseqc, sizeof(AppFrontend_DiseqcParameters));
				break;
	        }
		}
    }
}

static uint32_t dvbs_gradient_quality(unsigned int value)
{
	static GradientSignal gradient_quality = {{0},0};
	unsigned int ret = 0;
	int i;

	//printf("[APP Ioctl] get quality before gradient = %d\n", value);
	gradient_quality.signal_value[gradient_quality.signal_num] = value;
	gradient_quality.signal_num++;
	gradient_quality.signal_num %= SIGNAL_GRADIENT_COUNT;

	for(i = 0; i < SIGNAL_GRADIENT_COUNT; i++)
	{
		ret += gradient_quality.signal_value[i];
	}
	ret /= SIGNAL_GRADIENT_COUNT;
    if(ret > 100)
        ret = 100;
	//printf("[APP Ioctl] get quality after gradient = %d\n", ret);
	return ret;
}

static uint32_t dvbs_gradient_strength(unsigned int value)
{
	static GradientSignal gradient_strength = {{0},0};
	unsigned int ret = 0;
	int i;

	//printf("[APP Ioctl] get signal strength = %d\n", value);
	gradient_strength.signal_value[gradient_strength.signal_num] = value;
	gradient_strength.signal_num++;
	gradient_strength.signal_num %= SIGNAL_GRADIENT_COUNT;

	for(i = 0; i < SIGNAL_GRADIENT_COUNT; i++)
	{
		ret += gradient_strength.signal_value[i];
	}
	ret /= SIGNAL_GRADIENT_COUNT;
    if(ret > 100)
        ret = 100;
	//printf("[APP Ioctl] signal strength gradient = %d\n", ret);
	return ret;
}

static void dvbs_tp_set(AppNim *nim, AppFrontend_SetTp *params) 
{
    // the follow struct is the msg for bus(frontend)
    AppFrontend_SetTp frontend;

    // copy fre, symb, polar & 22k
    memcpy(&frontend, params, sizeof(AppFrontend_SetTp));

    frontend.dev = nim->fro_dev_hdl;
    frontend.demux = nim->dmx_mod_hdl;
    frontend.tuner = nim->tuner;

    // set tp
    app_send_msg_exec(GXMSG_FRONTEND_SET_TP, (void*)(&frontend));
}

static void dvbs_diseqc_set(AppNim *nim, AppFrontend_DiseqcParameters* params)
{
    GxMsgProperty_FrontendSetDiseqc frontend_diseqc;

    dvbs_diseqc_record(nim, (AppFrontend_DiseqcParameters*)params);

    if((params->type == DiSEQC10) && (params->u.params_10.chDiseqc == 0))
        return;

    if(params->type == DiSEQC11)
    {
        if(params->u.params_11.chUncom == 0)
        {
            return;
        }
        else
        {
            if(params->u.params_11.chCom == 0)
                params->u.params_11.chCom = 1;
        }
    }

   // moto if polar off user diseqc12 must be delay
	if((params->type == DiSEQC12)
		&&(thiz_frontend_polar_status == FRONTEND_POLAR_OFF))
	{
		thiz_frontend_polar_status = 0;
		GxCore_ThreadDelay(1200);
	}

    frontend_diseqc.tuner = nim->tuner;
    memcpy((void*)(&frontend_diseqc.diseqc), params, sizeof(AppFrontend_DiseqcParameters));

    app_send_msg_exec(GXMSG_FRONTEND_SET_DISEQC, &frontend_diseqc);
}

static void dvbs_diseqc_set_drictory(AppNim *nim, AppFrontend_DiseqcParameters* params)
{
    GxMsgProperty_FrontendSetDiseqc frontend_diseqc;

    dvbs_diseqc_record(nim, (AppFrontend_DiseqcParameters*)params);
    if((params->type == DiSEQC10) && (params->u.params_10.chDiseqc == 0))
        return;

    if(params->type == DiSEQC11)
    {
        if(params->u.params_11.chUncom == 0)
        {
            return;
        }
        else
        {
            if(params->u.params_11.chCom == 0)
                params->u.params_11.chCom = 1;
        }
    }

    if(thiz_frontend_monitor[nim->tuner] == FRONTEND_MONITOR_ON)
    {
        app_send_msg_exec(GXMSG_FRONTEND_STOP_MONITOR, &(nim->tuner));
    }

    frontend_diseqc.tuner = nim->tuner;
    memcpy((void*)(&frontend_diseqc.diseqc), params, sizeof(AppFrontend_DiseqcParameters));

    GxFrontend_SetDiseqc(&frontend_diseqc);

    if(thiz_frontend_monitor[nim->tuner] == FRONTEND_MONITOR_ON)
    {
        app_send_msg_exec(GXMSG_FRONTEND_START_MONITOR, &(nim->tuner));
    }
}

void _lnb_off(int32_t tuner)
{
    GpioData val = {0};
    uint8_t tuner_gpio[NIM_MODULE_NUM] = {V_GPIO_LNB_A, V_GPIO_LNB_B};

    if(tuner >= NIM_MODULE_NUM)
        return;

    val.port = tuner_gpio[tuner];
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_LOW, &val);
}

void _lnb_on(int32_t tuner)
{
    GpioData val = {0};
    uint8_t tuner_gpio[NIM_MODULE_NUM] = {V_GPIO_LNB_A, V_GPIO_LNB_B};

    if(tuner >= NIM_MODULE_NUM)
        return;

    val.port = tuner_gpio[tuner];
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_HIGH, &val);
}

static void dvbs_polar_set(AppNim *nim, AppFrontend_PolarState* params)
{
    nim->Polar = *params;
    GxFrontend_SetVoltage(nim->tuner,*params);
	if(nim->Polar == FRONTEND_POLAR_OFF)
	{
		thiz_frontend_polar_status = FRONTEND_POLAR_OFF;
	}
}

static void dvbs_22k_set(AppNim *nim, AppFrontend_22KState * params)
{
    // *params == 0: SEC_TONE_ON; *params == 1:SEC_TONE_OFF
    if((*params <= FRONTEND_22K_NULL)&&(*params >= FRONTEND_22K_ON))
    {
        nim->_22k = *params;
        GxFrontend_Set22K(nim->tuner,*params);
    }
}

static void  dvbs_diseqc_change(AppNim *nim, AppFrontend_DiseqcChange* change)
{
    change->state = FRONTEND_DISEQC_UNCHANGED;

    if (change->diseqc->type == DiSEQC10)
    {
        if (memcmp(&(change->diseqc->u.params_10), &(nim->diseqc10.u.params_10),
                    sizeof(GxFrontendDiseqc10)) != 0)
        {
            change->state = FRONTEND_DISEQC_CHANGED;
        }
    }
    else if (change->diseqc->type == DiSEQC11)
    {
        if (memcmp(&(change->diseqc->u.params_11), &(nim->diseqc11.u.params_11),
                    sizeof(GxFrontendDiseqc11)) != 0)
        {
            change->state = FRONTEND_DISEQC_CHANGED;
        }
    }
    else if (change->diseqc->type == DiSEQC12)
    {
        // add 20121108
        uint8_t CmdCount = nim->diseqc12.u.params_12.CmdCount;
        change->state = FRONTEND_DISEQC_CHANGED;

        if(CmdCount < 1)
        {
            ;
        }
        else if(nim->diseqc12.u.params_12.DiseqcControl[CmdCount - 1] == DISEQC_GOTO_NN)
        {
            uint8_t j = 0;
            for(j = 0; j < change->diseqc->u.params_12.CmdCount;j++)
            {
                if((change->diseqc->u.params_12.DiseqcControl[j] == DISEQC_GOTO_NN)
                        && (change->diseqc->u.params_12.DiSEqC12Params[j].m_chParam0 == nim->diseqc12.u.params_12.DiSEqC12Params[CmdCount - 1].m_chParam0))
                {
                    change->state = FRONTEND_DISEQC_UNCHANGED;
                    break;
                }
            }
        }
        else if(nim->diseqc12.u.params_12.DiseqcControl[CmdCount - 1] == DISEQC_GOTO_XX)
        {
            uint8_t j = 0;
            for(j = 0; j < change->diseqc->u.params_12.CmdCount;j++)
            {
                if((change->diseqc->u.params_12.DiseqcControl[j] == DISEQC_GOTO_XX)
                        &&(change->diseqc->u.params_12.DiSEqC12Params[j].m_chParam0 == nim->diseqc12.u.params_12.DiSEqC12Params[CmdCount - 1].m_chParam0)
                        &&(change->diseqc->u.params_12.DiSEqC12Params[j].m_chParam1 == nim->diseqc12.u.params_12.DiSEqC12Params[CmdCount - 1].m_chParam1))
                {
                    change->state = FRONTEND_DISEQC_UNCHANGED;
                    break;
                }
            }
        }
    }
    else
    {
        change->state = FRONTEND_DISEQC_CHANGED;
    }
}

static void dvbs_lock_state_set(AppNim* nim, AppFrontend_LockState params)
{
    nim->lock_state = params;
}

static void dvbs_lock_state_get(AppNim* nim, AppFrontend_LockState *params)
{// modify in 20120826
    if((GxFrontend_QueryFrontendStatus(nim->tuner)!=1)
            || (GxFrontend_QueryStatus(nim->tuner)!=1))
    {
        *params = FRONTEND_UNLOCK;
        //printf("\n^^^^^^^^^^^^^^^^^^^^[UNLOCK]---\n");
    }
    else
    {
        *params = FRONTEND_LOCKED;
        //printf("\n^^^^^^^^^^^^^^^^^^^^[LOCKED]---\n");
    }
}

static void dvbs_monitor_set(AppNim *nim, AppFrontend_Monitor params)
{
    GxMsgProperty_FrontendMonitor tuner = nim->tuner;

    if (params == FRONTEND_MONITOR_ON)
    {
        app_send_msg_exec(GXMSG_FRONTEND_START_MONITOR, &tuner);
    }
    else
    {
        app_send_msg_exec(GXMSG_FRONTEND_STOP_MONITOR, &tuner);
    }

    if(tuner < NIM_MODULE_NUM)
        thiz_frontend_monitor[tuner] = params;
}

static void dvbs_strength_get(AppNim *nim, AppFrontend_Strengh *str)
{
    *str = GxFrontend_GetStrength(nim->tuner);
    if(*str > 0xFF)
    {
        *str = CALC_SIGNAL_STRENGTH(*str);
    }
    *str = dvbs_gradient_strength(*str);
    // soft adjust
	// "20~100" map to "50~100"
    if(*str > 20)
    {
        *str = (37500 + (*str) * 625)/1000;
    }

}

static void dvbs_quality_get(AppNim *nim, AppFrontend_Quality *qua)
{
    GxFrontendSignalQuality sq = {0};

    if (0 == GxFrontend_GetQuality(nim->tuner, &sq))
    {
        // the following code so ugly!!!!!!! by Blacker Liu
        // used for DVB-C
        *qua = sq.snr;

        // used for DVB-S/S2
        if(*qua > 0xFF)
        {
            *qua = sq.snr*100/0xFFFF; // since beta4
        }

        *qua= dvbs_gradient_quality(*qua);
        // soft adjust
		// "20 ~100" map to "50~100"
        if(*qua > 20)
        {
           *qua = (37500 + (*qua) * 625)/1000;
        }
    }
    else
    {
        *qua= 0;
    }
}

static void dvbs_errorrate_get(AppNim *nim, AppFrontend_ErrorRate *err)
{
    GxFrontendSignalQuality sq = {0};

    if (0 == GxFrontend_GetQuality(nim->tuner, &sq))
    {
        *err = sq.error_rate;
    }
    else
    {
        *err = 0;
    }
}

static void dvbs_info_get(AppNim *nim, AppFrontend_Info *info)
{
    int32_t Handle = 0;
    if((nim == NULL) || (info == NULL))
        return;
    if(-1 != (Handle = GxFrontend_IdToHandle(nim->tuner)))
        GxFrontend_GetInfo(Handle, info);
}

static void dvbs_config_get(AppNim *nim, AppFrontend_Config *cfg)
{
    cfg->tuner  = nim->tuner;
    cfg->ts_src = nim->ts_src;
    cfg->dmx_id = nim->dmx_id;
}

static status_t dvbs_frontend_open(AppNim* nim, AppFrontend_Open *fopen)
{
    GxDemuxProperty_ConfigDemux demux;
    uint32_t index;

    nim->tuner      = fopen->tuner;
    nim->ts_src     = fopen->ts_src;
    nim->dmx_id     = fopen->dmx_id;

    nim->dmx_mod_hdl =
        GxAvdev_OpenModule(nim->fro_dev_hdl, GXAV_MOD_DEMUX, nim->dmx_id);
    if (nim->dmx_mod_hdl <= 0) {
        return GXCORE_ERROR;
    }

    // demux config
    demux.source = nim->ts_src;
    demux.ts_select = FRONTEND;
    demux.stream_mode = DEMUX_PARALLEL;
    demux.time_gate = 0xf;
    demux.byt_cnt_err_gate = 0x03;
    demux.sync_loss_gate = 0x03;
    demux.sync_lock_gate = 0x03;
    GxAVSetProperty(nim->fro_dev_hdl, nim->dmx_mod_hdl, GxDemuxPropertyID_Config, &demux, \
            sizeof(GxDemuxProperty_ConfigDemux));

    nim->diseqc10.u.params_10.chDiseqc = 0xff;
    nim->diseqc11.u.params_11.chUncom = 0xff;
    for(index = 0; index < FRONTEND_DISEQC_MAX_CMD; index++)
    {
        nim->diseqc12.u.params_12.DiSEqC12Params[index].m_chParam0 = 0xff;
        nim->diseqc12.u.params_12.DiSEqC12Params[index].m_chParam1 = 0xff;
    }
    nim->diseqc12.u.params_12.CmdCount = 0;

    nim->_22k = FRONTEND_22K_NULL;
    nim->Polar = FRONTEND_POLAR_OFF;

    return GXCORE_SUCCESS;
}

static status_t dvbs_demux_config(AppNim *nim)
{
    GxDemuxProperty_ConfigDemux demux;

    // demux config
    demux.source = nim->ts_src;
    demux.ts_select = FRONTEND;
    demux.stream_mode = DEMUX_PARALLEL;
    demux.time_gate = 0xf;
    demux.byt_cnt_err_gate = 0x03;
    demux.sync_loss_gate = 0x03;
    demux.sync_lock_gate = 0x03;
    GxAVSetProperty(nim->fro_dev_hdl, nim->dmx_mod_hdl, GxDemuxPropertyID_Config, &demux, \
            sizeof(GxDemuxProperty_ConfigDemux));

    return GXCORE_SUCCESS;
}

static status_t dvbs_frontend_close(AppNim *nim)
{
    status_t  ret = GXCORE_SUCCESS;

    //frontend
    if(nim->fro_dev_hdl <= 0
            || nim->dmx_mod_hdl <= 0)
    {
        return GXCORE_ERROR;
    }

    ret = GxAVCloseModule(nim->fro_dev_hdl, nim->dmx_mod_hdl);
    nim->dmx_mod_hdl = -1;

    return ret;
}

static int32_t dvbs_lnb_config(AppNim *nim, AppFrontend_LNBState *params)
{
    int32_t ret = 0;
    int32_t Frontend = GxFrontend_IdToHandle(nim->tuner);
    GxMsgProperty_FrontendSetTp Param = {0};
    GxFrontend_GetCurFre(nim->tuner,&Param);

    if((FRONTEND_LNB_OFF == *params)
            && (Param.polar != SEC_VOLTAGE_OFF)
            && (Frontend > 0))
    {// close the LNB OUTPUT
        ret = ioctl (Frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_OFF);
    }
    else if((FRONTEND_LNB_OFF != *params)
            && (Param.polar != SEC_VOLTAGE_OFF)
            && (Frontend > 0))
    {// ENABLE lnb output
        ret = ioctl (Frontend, FE_SET_VOLTAGE, Param.polar);
    }

    return ret;
}

static int32_t dvbs_param_get(AppNim *nim, AppFrontend_Param *param)
{
    if((NULL == nim) || (NULL == param))
    {
        printf("\nNIM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }

    GxMsgProperty_FrontendSetTp info = {0};
    GxFrontend_GetCurFre(nim->tuner,&info);

    param->fre = info.fre;
    param->type = info.type;
    param->sat22k = info.sat22k;
    param->symb = info.symb;
    param->polar = info.polar;
    param->diseqc10 = nim->diseqc10.u.params_10.chDiseqc;
    return 0;
}

AppNim nim_dvbs =
{
    .lock_state     = false,
    .open           = dvbs_frontend_open,
    .close          = dvbs_frontend_close,
    .tp_set         = dvbs_tp_set,
    .diseqc_set     = dvbs_diseqc_set,
    .diseqc_set_ex  = dvbs_diseqc_set_drictory,
    .diseqc_change  = dvbs_diseqc_change,
    .monitor_set    = dvbs_monitor_set,
    .lock_state_set = dvbs_lock_state_set,
    .lock_state_get = dvbs_lock_state_get,
    .strength_get   = dvbs_strength_get,
    .quality_get    = dvbs_quality_get,
    .errorrate_get  = dvbs_errorrate_get,
    .info_get       = dvbs_info_get,
    .config_get     = dvbs_config_get,
    .param_get      = dvbs_param_get,
    .demux_cfg      = dvbs_demux_config,
    .lnb_cfg		= dvbs_lnb_config,
    .switch_22k_set	= dvbs_22k_set,
    .polar_set		= dvbs_polar_set,
};

#endif
