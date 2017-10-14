/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_card_update.c
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

static int _get_card_update(CASCAMNSTVCardUpdateInfoClass * param)
{
    int ret = 0;
    if(NULL == param)
    {
        return -1;
    }

    // TODO: 20160409
    param->update_status = 0;
    cascam_memcpy(param->update_time, "2016-04-09 09:54", 19);
    return ret;
}

int nstv_card_update_get(int sub_id, void* param, unsigned int len)
{
    int ret = 0;
    if((NULL == param) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_CARD_UPDATE_GET:
            if(len != sizeof(CASCAMNSTVCardUpdateInfoClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _get_card_update((CASCAMNSTVCardUpdateInfoClass*)param);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }
    return ret;
}
