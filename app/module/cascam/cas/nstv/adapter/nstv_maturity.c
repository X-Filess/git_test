/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_maturity.c
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

static int _get_maturity(CASCAMNSTVWatchLevelClass * param)
{
    int ret = 0;
    unsigned char rating = 0;

    if(NULL == param)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    ret = GxCas_Get(GXCAS_CD_GET_RATING, (void *)&rating);
    if (ret != CDCA_RC_OK)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    param->watch_level = rating;
    return ret;
}

static int _set_maturity(CASCAMNSTVWatchLevelClass * param)
{
    int ret = 0;
    unsigned char pin[6] = {0};
    int i = 0;

    if(NULL == param)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    // change char to dec
    for(i = 0; i < 6; i++)
    {
        pin[i] = param->pin[i] - 0x30;
    }
    GxCas_CdSetRating rating_param;
    memset(&rating_param, 0, sizeof(GxCas_CdSetRating));
    memcpy(rating_param.pin, pin, 6);
    rating_param.rating = param->watch_level;
    ret = GxCas_Set(GXCAS_CD_SET_RATING, (void *)&rating_param);
    if (ret != CDCA_RC_OK)
    {
        switch(ret)
        {
            case CDCA_RC_OK:
                //"Teleview rating changed successfully"
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
                //"Set Maturity Rating Fail"
                break;
        }
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    // TODO: 20160413
    //thiz_maturity_test_rating = param->watch_level;

    return 0;
}

int nstv_maturity_get(int sub_id, void* param, unsigned int len)
{
    int ret = 0;
    if((NULL == param) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_WATCH_LEVEL_GET:
            if(len != sizeof(CASCAMNSTVWatchLevelClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _get_maturity((CASCAMNSTVWatchLevelClass*)param);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }
    return ret;

}

int nstv_maturity_set(int sub_id, void* param, unsigned int len)
{
    int ret = 0;
    if((NULL == param) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_WATCH_LEVEL_SET:
            if(len != sizeof(CASCAMNSTVWatchLevelClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _set_maturity((CASCAMNSTVWatchLevelClass*)param);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }
    return ret;

}
