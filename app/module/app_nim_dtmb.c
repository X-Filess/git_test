#include "app.h"
#if (DEMOD_DTMB > 0)
#include "app_module.h"

#define SIGNAL_GRADIENT_COUNT 1

typedef struct
{
	uint8_t signal_value[SIGNAL_GRADIENT_COUNT];
	uint8_t signal_num;
}GradientSignal;

static uint32_t dtmb_gradient_quality(unsigned int value)
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

	//printf("[APP Ioctl] get quality after gradient = %d\n", ret);
	return ret;
}

static uint32_t dtmb_gradient_strength(unsigned int value)
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

	//printf("[APP Ioctl] signal strength gradient = %d\n", ret);
	return ret;
}

static void dtmb_tp_set(AppNim *nim, AppFrontend_SetTp *params) 
{
    // the follow struct is the msg for bus(frontend)
    AppFrontend_SetTp frontend;

    // copy fre, symb, polar & 22k
    memcpy(&frontend, params, sizeof(AppFrontend_SetTp));

    frontend.dev = nim->fro_dev_hdl;
    frontend.demux = nim->dmx_mod_hdl;
    frontend.tuner = nim->tuner;

    // set tp
    printf("\nDEBUG, %s, %d\n",__FUNCTION__,__LINE__);
    app_send_msg_exec(GXMSG_FRONTEND_SET_TP, (void*)(&frontend));
}

static void dtmb_lock_state_set(AppNim* nim, AppFrontend_LockState params)
{
    nim->lock_state = params;
}

static void dtmb_lock_state_get(AppNim* nim, AppFrontend_LockState *params)
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

static void dtmb_monitor_set(AppNim *nim, AppFrontend_Monitor params)
{
    GxMsgProperty_FrontendMonitor tuner = nim->tuner;

   // if (params == FRONTEND_MONITOR_ON)
   // {
   //     app_send_msg_exec(GXMSG_FRONTEND_START_MONITOR, &tuner);
  //  }
  //  else
  //  {
        app_send_msg_exec(GXMSG_FRONTEND_STOP_MONITOR, &tuner);
  //  }
}

static void dtmb_strength_get(AppNim *nim, AppFrontend_Strengh *str)
{
    *str = GxFrontend_GetStrength(nim->tuner);
    if(*str > 0xFF)
    {
        *str = CALC_SIGNAL_STRENGTH(*str);
    }
    *str = dtmb_gradient_strength(*str);
}

static void dtmb_quality_get(AppNim *nim, AppFrontend_Quality *qua)
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

        *qua= dtmb_gradient_quality(*qua);
		
	   //add by leixj for fake value
		*qua=*qua*5/4;
        if (*qua>=100)
			*qua=99;
		//end add
    }
    else
    {
        *qua= 0;
    }
    //printf("[APP Ioctl] get quality = %d\n", *qua );
}

static void dtmb_errorrate_get(AppNim *nim, AppFrontend_ErrorRate *err)
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

static void dtmb_info_get(AppNim *nim, AppFrontend_Info *info)
{
    int32_t Handle = 0;
    if((nim == NULL) || (info == NULL))
        return;
    if(-1 != (Handle = GxFrontend_IdToHandle(nim->tuner)))
        GxFrontend_GetInfo(Handle, info);
}

static void dtmb_config_get(AppNim *nim, AppFrontend_Config *cfg)
{
    cfg->tuner  = nim->tuner;
    cfg->ts_src = nim->ts_src;
    cfg->dmx_id = nim->dmx_id;
}

static status_t dtmb_frontend_open(AppNim* nim, AppFrontend_Open *fopen)
{

    GxDemuxProperty_ConfigDemux demux;

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

    return GXCORE_SUCCESS;
}

static status_t dtmb_demux_config(AppNim *nim)
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

static status_t dtmb_frontend_close(AppNim *nim)
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

AppNim nim_dtmb =
{
    .lock_state     = false,
    .open           = dtmb_frontend_open,
    .close          = dtmb_frontend_close,
    .tp_set         = dtmb_tp_set,
    .monitor_set    = dtmb_monitor_set,
    .lock_state_set = dtmb_lock_state_set,
    .lock_state_get = dtmb_lock_state_get,
    .strength_get   = dtmb_strength_get,
    .quality_get    = dtmb_quality_get,
    .errorrate_get  = dtmb_errorrate_get,
    .info_get       = dtmb_info_get,
    .config_get     = dtmb_config_get,
    .demux_cfg      = dtmb_demux_config,
};

#endif

