/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_platform.c
* Author    :	B.Z.
* Project   :	CA Module
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.04.09		   B.Z.			  Creation
*****************************************************************************/
#include "../../include/cascam_os.h"
#include "../../include/cascam_types.h"
#include "../../include/cas/cascam_nstv.h"
#include "include/nstv_platform.h"
#include "include/porting/CDCASS.h"
#include "include/porting/cd_api.h"
#include "include/porting/gxcas.h"

#define CAS_Dbg cascam_printf

static CASCAMProgramParaClass thiz_program_para;

extern CASCAMControlBlockClass NSTVCASControlBlock;

static int init_flag = 0;
static int _init(void)
{
    if (init_flag) {
        return 0;
    }
    init_flag = 1;
#if 0
    GxCasInitParam init = {0};
	memset(&init, 0, sizeof(init));

    init.dmx_id = 0;
    init.ts_id = 0;

    nstv_osd_mail_notice_init();
    strcpy(init.flash.name, "PRIVATE");
	init.flash.offset = 0;
	init.flash.size = 3 * 64 * 1024;
	strcpy(init.backup.name, "PRIVATE");
	init.backup.offset = 3 * 64 * 1024;
	init.backup.size = 3 * 64 * 1024;

#if NSCAS_SMC_VERSION
    init.sci.sci_switch = 1;
    init.sci.detect_pole = GXCAS_SMC_HIGH_LEVEL;
    init.sci.vcc_pole = GXCAS_SMC_LOW_LEVEL;
#else
    init.sci.sci_switch = 0;
#endif

#if 0 // if want to debug gxcas, please set
	GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CASLIB);
    GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CAS);
    GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_DEMUX);
    GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_DESC);
    GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_SMC);
    GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_FLASH);
    GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_SI);
#endif

    GxCas_CdHsInit(init);

    extern void nstv_event_task();
    nstv_event_task();

    // for time zone
    double zone = (double)cascam_get_time_zone();
    GxCas_Set(GXCAS_CD_SET_TIMEZONE, (void *)&zone);
#endif
    return 0;
}

static int _close(void)
{
    cascam_printf("\n\033[39mCASCAM, exit, %s, %d\033[0m\n", __func__, __LINE__);
    CDCASTB_Close();
    init_flag = 0;
    return 0;
}

static int _change_prog(CASCAMProgramParaClass *pProgParam)
{
    if (NULL == pProgParam) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (pProgParam->ServiceID != 0) {
        GxCasSet_SwitchChannel gxcas_switch = {0};
        gxcas_switch.pmt_pid = pProgParam->PMTPid;
        gxcas_switch.service_id = pProgParam->ServiceID;
        gxcas_switch.video_pid = pProgParam->VideoPid;
        gxcas_switch.audio_pid = pProgParam->AudioPid;
        GxCas_Set(GXCAS_CD_SWITCH_CHANNEL, (void *)&gxcas_switch);
    }
    else {
        GxCas_Service param[8];
        memset(param, 0, sizeof(GxCas_Service) * 8);
        param[0].ecm_pid = pProgParam->AudioECMPid;
        param[0].pid = pProgParam->AudioPid;
        param[1].ecm_pid = pProgParam->VideoECMPid;
        param[1].pid = pProgParam->VideoPid;
		GxCas_Set(GXCAS_CD_SET_LOCK_SERVICE, (void *)param);
    }

    cascam_memcpy(&thiz_program_para, pProgParam, sizeof(CASCAMProgramParaClass));
    return 0;
}

int nstv_get_program_info(CASCAMProgramParaClass *pProgPara)
{
    if (NULL == pProgPara) {
        cascam_printf("\nCASCAM, error, %d, %s\n", __FUNCTION__, __LINE__);
        return -1;
    }

    cascam_memcpy(pProgPara, &thiz_program_para, sizeof(CASCAMProgramParaClass));
    return 0;
}

static int _change_frequency(void)
{
    GxCas_Set(GXCAS_CD_STOP_CA, NULL);
    GxCas_Set(GXCAS_CD_START_CA, NULL);

    return 0;
}

static int _get_cas_id(void)
{
    return 0x4A02;
}

static int _is_vaild_cas(unsigned short CASID)
{
    //TODO: Delete
    int ret = 0;
    ret = CDCASTB_IsCDCa(CASID);
    if (CDCA_TRUE == ret) {
        ret = 1;
    }
    return ret;
}

static int _set_zone(double zone)
{
    double value = zone;
    return GxCas_Set(GXCAS_CD_SET_TIMEZONE, (void *)&value);
}

static int _demux_release_demux(CASCAMDemuxFlagClass *pFlags)
{
    if (NULL == pFlags) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if ((DEMUX_ECM & pFlags->Flags) || (DEMUX_PMT & pFlags->Flags))
    {
        GxCas_Set(GXCAS_CD_STOP_DESCRAMBLE, NULL);
    }

    if ((DEMUX_EMM & pFlags->Flags) || (DEMUX_CAT & pFlags->Flags))
    {
        GxCas_Set(GXCAS_CD_STOP_CA, NULL);
    }

    if (DEMUX_ALL & pFlags->Flags)
    {
        GxCas_Set(GXCAS_CD_STOP_DESCRAMBLE, NULL);
        GxCas_Set(GXCAS_CD_STOP_CA, NULL);
    }

    return 0;
}

static int _demux_start_demux(CASCAMDemuxFlagClass *pFlags, unsigned short pid)
{
    if ((NULL == pFlags) || (0x1fff == pid)) {
        return -1;
    }
    if (DEMUX_CAT & pFlags->Flags) {
        GxCas_Set(GXCAS_CD_START_CA, NULL);
    }
    if (DEMUX_PMT & pFlags->Flags) {
    }
    if (DEMUX_NIT & pFlags->Flags) {
    }
    return 0;
}

static int _get_property(CASCAMCASGetDataTypeEnum main_id, int sub_id, void *property, unsigned int len)
{
    int ret = 0;
    if (init_flag == 0) {
        return -1;
    }
    switch (main_id) {
        case BASIC_INFO:
            ret = nstv_basic_information_get(sub_id, property, len);
            break;
        case ENTITLEMENT_INFO:
            ret = nstv_entitle_information_get(sub_id, property, len);
            break;
        case EMAIL_INFO:
            ret = nstv_mail_information_get(sub_id, property, len);
            break;
        case PARENTAL_INFO:
            ret = nstv_maturity_get(sub_id, property, len);
            break;
        case PIN_INFO:
            ret = nstv_pin_get(sub_id, property, len);
            break;
        case WORK_TIME_INFO:
            ret = nstv_work_time_get(sub_id, property, len);
            break;
        case WALLET_INFO:
            ret = nstv_wallet_information_get(sub_id, property, len);
            break;
        case IPPV_INFO:
            ret = nstv_ippv_information_get(sub_id, property, len);
            break;
        case DETITLE_INFO:
            ret = nstv_detitle_information_get(sub_id, property, len);
            break;
        case FEATURES_INFO:
            ret = nstv_features_information_get(sub_id, property, len);
            break;
        case OPERATOR_INFO:
            ret = nstv_operator_information_get(sub_id, property, len);
            break;
        case UPDATE_INFO:
            ret = nstv_card_update_get(sub_id, property, len);
            break;
        case PARENT_CHILD_INFO:
            ret = nstv_card_parent_child_information_get(sub_id, property, len);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            ret = -1;
            break;
    }
    return ret;
}

static int _set_property(CASCAMCASSetDataTypeEnum id, int sub_id, void *property, unsigned int len)
{
    int ret = 0;
    if (init_flag == 0) {
        return -1;
    }
    switch (id) {
        case EMAIL_CONTROL:
            ret = nstv_mail_information_edit(sub_id, property, len);
            break;
        case PARENTAL_CONTROL:
            ret = nstv_maturity_set(sub_id, property, len);
            break;
        case PIN_CONTROL:
            ret = nstv_pin_set(sub_id, property, len);
            break;
        case WORK_TIME_CONTROL:
            ret = nstv_work_time_set(sub_id, property, len);
            break;
        case DETITLE_CONTROL:
            ret = nstv_detitle_information_edit(sub_id, property, len);
            break;
        case PARENT_CHILD_CONTROL:
            ret = nstv_parent_child_set(sub_id, property, len);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            ret = -1;
            break;
    }
    return ret;
}

CASCAMControlBlockClass NSTVCASControlBlock = {
    .CASName                = "CDCAS",
    .is_advanced_security   = 1,
    .set_zone               = _set_zone,
    .init                   = _init,
    .close                  = _close,
    .change_program         = _change_prog,
    .change_frequency       = _change_frequency,
    .get_cas_id             = _get_cas_id,
    .is_vaild_cas           = _is_vaild_cas,
    .get_property           = _get_property,
    .set_property           = _set_property,
    .demux_class            = {
        .DemuxID                = 0,
        .DoBySelf               = 1,//set demux filter by CAS LIB
        .release_demux          = _demux_release_demux,
        .start_demux            = _demux_start_demux,
    },
};
