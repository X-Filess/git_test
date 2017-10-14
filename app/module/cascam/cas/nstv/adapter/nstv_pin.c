/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_pin.c
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

static int _set_pin(CASCAMNSTVPINControlClass * param)
{
    int ret = 0;
    int i = 0;
    unsigned char old_pin[6] = {0};
    unsigned char new_pin[6] = {0};
    if(NULL == param)
    {
        return -1;
    }
    for(i = 0; i < 6; i++)
    {
        old_pin[i] = (param->old_pin[i] - 0x30);
        new_pin[i] = (param->new_pin[i] - 0x30);
    }
    GxCas_CdChangePin pin_param;
    memset(&pin_param, 0, sizeof(GxCas_CdChangePin));
    memcpy(pin_param.old_pin, old_pin, 6);
    memcpy(pin_param.new_pin, new_pin, 6);
    ret = GxCas_Set(GXCAS_CD_SET_PIN, (void *)&pin_param);
    switch(ret)
    {
        case CDCA_RC_OK:
            //"PIN has been changed successfully"
            break;
#if NSCAS_SMC_VERSION
        case CDCA_RC_CARD_INVALID:
            //"Invalid Smartcard"
            break;
#endif
        case CDCA_RC_PIN_INVALID:
            //"Invalid PIN"
            break;
        default:
            break;
    }
    if(ret)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    
#if 0
    // TODO: 20160409
    if(cascam_strncmp(thiz_test_pin, (const char*)param->old_pin, 6) == 0)
    {
        cascam_memcpy(thiz_test_pin, param->new_pin, 6);
    }
    else
    {
        cascam_printf("\nerror, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
#endif
    return 0;
}

static int _get_pin(CASCAMNSTVPINControlClass * param)
{
    int ret = -1;
    if(NULL == param)
    {
        return -1;
    }

    // TODO: 20160409
    //cascam_memcpy(param->new_pin, thiz_test_pin, 6);
    //cascam_memcpy(param->old_pin, thiz_test_pin, 6);
    return ret;
}

int nstv_pin_set(int sub_id, void* param, unsigned int len)
{
    int ret = 0;
    if((NULL == param) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_PIN_SET:
            if(len != sizeof(CASCAMNSTVPINControlClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _set_pin((CASCAMNSTVPINControlClass*)param);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }
    return ret;
}

int nstv_pin_get(int sub_id, void* param, unsigned int len)
{
    int ret = 0;
    if((NULL == param) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_PIN_GET:
            if(len != sizeof(CASCAMNSTVPINControlClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _get_pin((CASCAMNSTVPINControlClass*)param);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }
    return ret;
}
