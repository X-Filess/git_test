/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_work_time.c
* Author    :   B.Z.
* Project   :	GoXceed
* Type      :   CA Module
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
VERSION	    Date			  AUTHOR         Description
1.0	      2015.10.01		   B.Z.			  creation
*****************************************************************************/
#include "../../../include/cascam_platform.h"
#include "../../../include/cas/cascam_nstv.h"
#include "../../../include/cascam_os.h"
#include "../include/porting/CDCASS.h"
#include "../include/porting/cd_api.h"

#if NSCAS_SMC_VERSION

static int _get_work_time(CASCAMNSTVWorkTimeClass * param)
{

	GxCas_CdWorkTime work_time;
	int32_t ret = 0;
	memset(&work_time, 0, sizeof(GxCas_CdWorkTime));
	ret = GxCas_Get(GXCAS_CD_GET_WORKTIME, (void *)&work_time);
	if (ret != CDCA_RC_OK) {
		cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	cascam_abc_to_bcd(&param->start_time, (unsigned short)(work_time.start_hour), work_time.start_min, work_time.start_sec);
	cascam_abc_to_bcd(&param->end_time, (unsigned short)(work_time.end_hour), work_time.end_min, work_time.end_sec);

    return 0;
}

static int _set_work_time(CASCAMNSTVWorkTimeClass * param)
{
    int ret = 0;
    unsigned short  start_hour = 0;
    unsigned char  start_min = 0;
    unsigned char  start_sec = 0;
    unsigned short  end_hour = 0;
    unsigned char  end_min = 0;
    unsigned char  end_sec = 0;
    unsigned char pin[6] = {0};
    int i = 0;

    if(NULL == param)
    {
        return -1;
    }
    for(i = 0; i < 6; i++)
    {
        pin[i] = (param->pin[i] - 0x30);
    }

    GxCas_CdWorkTime time;
    memset(&time, 0, sizeof(GxCas_CdWorkTime));
    memcpy(time.pin, pin, 6);
    cascam_bcd_to_abc(param->start_time, &start_hour, &start_min, &start_sec);
    cascam_bcd_to_abc(param->end_time, &end_hour, &end_min, &end_sec);
    time.start_hour = start_hour;
    time.start_min = start_min;
    time.start_sec = start_sec;
    time.end_hour = end_hour;
    time.end_min = end_min;
    time.end_sec = end_sec;
    ret = GxCas_Set(GXCAS_CD_SET_WORKTIME, (void *)&time);
    switch(ret)
    {
        case CDCA_RC_OK:
             //"Working Hours changed successfully"
             break;
        case CDCA_RC_CARD_INVALID:
             //"Invalid Smartcard"
             return -1;
        case CDCA_RC_WORKTIME_INVALID:
             //"Invalid working hours"
             return -1;
        case CDCA_RC_PIN_INVALID:
             //"Invalid PIN"
             return -1;
        default:
             //"Time Setting Fail"
             return -1;
    }
    return ret;
}

#endif

int nstv_work_time_set(int sub_id, void* param, unsigned int len)
{
    int ret = 0;

#if NSCAS_SMC_VERSION

    if((NULL == param) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_WORK_TIME_SET:
            if(len != sizeof(CASCAMNSTVWorkTimeClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _set_work_time((CASCAMNSTVWorkTimeClass*)param);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }

#endif

    return ret;
}

int nstv_work_time_get(int sub_id, void* param, unsigned int len)
{
    int ret = 0;

#if NSCAS_SMC_VERSION

    if((NULL == param) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_WORK_TIME_GET:
            if(len != sizeof(CASCAMNSTVWorkTimeClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _get_work_time((CASCAMNSTVWorkTimeClass*)param);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }

#endif

    return ret;
}
