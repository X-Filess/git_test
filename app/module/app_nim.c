#include "app.h"
#include "module/app_ioctl.h"

#if (DEMOD_DVB_S > 0)
extern AppNim nim_dvbs;
#endif

#if (DEMOD_DVB_T > 0)
extern AppNim nim_dvbt;
#endif

#if (DEMOD_DVB_C > 0)
extern AppNim nim_dvbc;
#endif

#if (DEMOD_DTMB > 0)
extern AppNim nim_dtmb;
#endif


static AppNim *sp_Nim[NIM_MODULE_NUM]={NULL,NULL};

status_t nim_init(uint32_t Index, int demod_type)
{
   // uint32_t i;
    AppNim *nim_new = NULL;
    AppNim *nim_src[DEMOD_TYPE_TOTAL] = {
#if (DEMOD_DVB_S > 0)
	&nim_dvbs,
#endif
#if (DEMOD_DVB_T > 0)
    &nim_dvbt,
#endif
#if (DEMOD_DVB_C > 0)
    &nim_dvbc,
#endif
#if (DEMOD_DTMB > 0)
	&nim_dtmb,
#endif
    };

	if((Index >= NIM_MODULE_NUM)
		|| (demod_type >= DEMOD_TYPE_TOTAL))
	{
		printf("\nNIM, error, %s, %d\n",__FUNCTION__,__LINE__);
		return GXCORE_ERROR;
	}
  
    
    //memset(sp_Nim, 0, sizeof(AppNim*) * NIM_MODULE_NUM);

    //for (i = 0; i < num; i++)
    {
        nim_new = NULL;
        if ((nim_new = (AppNim*)GxCore_Calloc(1, sizeof(AppNim))) == NULL)
        {
            return GXCORE_ERROR;
        }

        memcpy(nim_new, nim_src[demod_type], sizeof(AppNim));
		//printf("\nNIM, set tp function = 0x%x \n", &(nim_src[demod_type]->tp_set));

        // open dev
        if ((nim_new->fro_dev_hdl = GxAvdev_CreateDevice(0)) <= 0)
        {
            return GXCORE_ERROR;
        }

        sp_Nim[Index] = nim_new;
		//printf("\nNIM, dest = 0x%x , dvbs = 0x%x, dtmb = 0x%x\n", &(sp_Nim[Index]->tp_set),&(nim_dvbs.tp_set),&(nim_dtmb.tp_set));
    }

    return GXCORE_SUCCESS;
}

void nim_release(void)
{
    uint32_t i;
    AppNim* nim = NULL;

    for (i = 0; i < NIM_MODULE_NUM; i++)
    {
        nim = sp_Nim[i];
        if (nim != NULL && nim->fro_dev_hdl != 0)
        {
            GxAvdev_DestroyDevice(nim->fro_dev_hdl);
            GxCore_Free(nim);
            nim = NULL;
        }
    }
}

status_t nim_ioctl(uint32_t id, uint32_t cmd, void * params)
{
    AppNim *nim = NULL;

    if (id >= NIM_MODULE_NUM)
    {
        printf("[NIM] id error!\n");
        return GXCORE_ERROR;
    }

    nim = sp_Nim[id];
    if(nim == NULL)
    {
        printf("\n[NIM] ERROR!!!  NIM is NULL,please init NIM!\n");
        return GXCORE_ERROR;
    }

    if(nim->fro_dev_hdl <= 0)
    {
        return GXCORE_ERROR;
    }
    //printf("\nNIM, %s, tuner = %d, cmd = %d, nim = 0x%x\n", __FUNCTION__,id, cmd,nim);
    switch (cmd)
    {
        case FRONTEND_OPEN:
            if(nim->open != NULL)
                nim->open(nim, (AppFrontend_Open*)params);
            break;
        case FRONTEND_TP_SET:
            if(nim->tp_set != NULL)
                nim->tp_set(nim, (AppFrontend_SetTp*)params);
            break;
        case FRONTEND_DISEQC_CHANGE:
            if(nim->diseqc_change != NULL)
                nim->diseqc_change(nim, (AppFrontend_DiseqcChange*)params);
            break;
        case FRONTEND_DISEQC_SET:
            if(nim->diseqc_set != NULL)
                nim->diseqc_set(nim, (AppFrontend_DiseqcParameters*)params);
            break;
        case FRONTEND_DISEQC_SET_EX:
            if(nim->diseqc_set_ex != NULL)
                nim->diseqc_set_ex(nim, (AppFrontend_DiseqcParameters*)params);
            break;
        case FRONTEND_22K_SET:
            if(nim->switch_22k_set != NULL)
                nim->switch_22k_set(nim,(AppFrontend_22KState*)params);
            break;
        case FRONTEND_POLAR_SET:
            if(nim->polar_set != NULL)
                nim->polar_set(nim,(AppFrontend_PolarState*)params);
            break;
        case FRONTEND_DISEQC_SET_DEFAULT:
            if(nim->diseqc_set != NULL)
            {
                AppFrontend_DiseqcParameters *diseqc = (AppFrontend_DiseqcParameters *)params;
                if (diseqc->type == DiSEQC10)
                {
                    memcpy(diseqc, &nim->diseqc10, sizeof(AppFrontend_DiseqcParameters));
                }
                else if (diseqc->type == DiSEQC11)
                {
                    memcpy(diseqc, &nim->diseqc11, sizeof(AppFrontend_DiseqcParameters));
                }
                else if (diseqc->type == DiSEQC12)
                {
                    /*
                       memcpy(diseqc, &nim->diseqc12, sizeof(AppFrontend_DiseqcParameters));
                       if((diseqc->u.params_12.DiseqcControl != DISEQC_GOTO_NN)
                       && (diseqc->u.params_12.DiseqcControl != DISEQC_GOTO_XX))
                       return GXCORE_ERROR;

                       AppFrontend_DiseqcParameters diseqc12 = {0};
                       diseqc12.type = DiSEQC12;
                       diseqc12.u.params_12.CmdCount = 1;
                       diseqc12.u.params_12.DiseqcControl[0] = DISEQC_STOP;
                       nim->diseqc_set(nim, &diseqc12);
                       */
                }
                else
                {
                    return GXCORE_ERROR;
                }
                nim->diseqc_set(nim, diseqc);
            }
            break;
        case FRONTEND_MONITOR_SET:
            if(nim->monitor_set != NULL)
                nim->monitor_set(nim, *(AppFrontend_Monitor*)params);
            break;
        case FRONTEND_LOCK_STATE_SET:
            if(nim->lock_state_set != NULL)
                nim->lock_state_set(nim, *(AppFrontend_LockState*)params);
            break;
        case FRONTEND_LOCK_STATE_GET:
            if(nim->lock_state_get != NULL)
                nim->lock_state_get(nim, (AppFrontend_LockState*)params);
            break;
        case FRONTEND_STRENGTH_GET:
            if(nim->strength_get != NULL)
                nim->strength_get(nim, (AppFrontend_Strengh*)params);
            break;
        case FRONTEND_QUALITY_GET:
            if(nim->quality_get != NULL)
                nim->quality_get(nim, (AppFrontend_Quality*)params);
            break;
        case FRONTEND_ERRORRATE_GET:
            if(nim->errorrate_get != NULL)
                nim->errorrate_get(nim, (AppFrontend_ErrorRate*)params);
            break;
        case FRONTEND_CLOSE:
            if(nim->close != NULL)
                nim->close(nim);
            break;
        case FRONTEND_INFO_GET:
            if(nim->info_get != NULL)
                nim->info_get(nim, (AppFrontend_Info*)params);
            break;
        case FRONTEND_CONFIG_GET:
            if(nim->config_get != NULL)
                nim->config_get(nim, (AppFrontend_Config*)params);
            break;
        case FRONTEND_PARAM_GET:
            if(nim->param_get != NULL)
                return nim->param_get(nim, (AppFrontend_Param*)params);
            break;
        case FRONTEND_DEMUX_CFG:
            if(nim->demux_cfg != NULL)
                nim->demux_cfg(nim);
            break;
        case FRONTEND_LNB_CFG:
            if(nim->lnb_cfg != NULL)
                nim->lnb_cfg(nim,(AppFrontend_LNBState*)params);
            break;
        default:
            return GXCORE_ERROR;
    }

    return GXCORE_SUCCESS;
}

