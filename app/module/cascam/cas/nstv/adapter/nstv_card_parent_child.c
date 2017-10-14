/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_card_parent_child.c
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
#include "../include/porting/cd_api.h"
#include "../include/porting/CDCASS.h"

int nstv_card_parent_child_information_get(int sub_id, void* param, unsigned int len)
{
#if NSCAS_SMC_VERSION
    int ret = -1;
    if((NULL == param) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_CARD_PARENT_CHILD_INFO_GET:
            if(len != sizeof(CASCAMNSTVParentChildInfoClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }

			GxCas_CdGetChildInfo info;
	        memset(&info, 0, sizeof(GxCas_CdGetChildInfo));
			CASCAMNSTVParentChildInfoClass *temp = (CASCAMNSTVParentChildInfoClass*)param;
	        info.wTvsID = temp->wTvsID;
	        
	        ret = GxCas_Get(GXCAS_CD_GET_CHILDINFO, &info);
			if (CDCA_RC_OK == ret)
			{
			    temp->pbyChild = info.pbyChild;
				temp->pbyDelayTime = info.pbyDelayTime;
				temp->pLastFeedTime = info.pLastFeedTime;
				temp->pbIsCanFeed = info.pbIsCanFeed;
				memcpy(temp->pParentCardSN, info.pParentCardSN, sizeof(temp->pParentCardSN));
                
				return 0;
			}
			else
			{
			    printf("nstv_card_parent_child_information_get fail, ret = 0x%x \n", ret);
			    return -1;
			}
	
            break;
			
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }
	
    return -1;
#else
    return -1;
#endif
}

int nstv_parent_child_set(unsigned int sub_id, void *property, unsigned int len)
{
#if NSCAS_SMC_VERSION

    int ret = -1;
	CASCAMNSTVParentChildInfoClass *temp = NULL;
	
    if((NULL == property) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }

	switch(sub_id)
	{
	    case NSTV_CARD_PARENT_CHILD_DATA_GET:
			if(len != sizeof(CASCAMNSTVParentChildInfoClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }

			temp = (CASCAMNSTVParentChildInfoClass*)property;
			ret = GxCas_Set(GXCAS_CD_READ_FEED_DATA, &(temp->wTvsID));
			if (CDCA_RC_OK == ret)
			{
			    return 0;
			}
			else
			{
			    return -1;
			}
			break;

		case NSTV_CARD_PARENT_CHILD_FEED:
			if(len != sizeof(CASCAMNSTVParentChildInfoClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }

			temp = (CASCAMNSTVParentChildInfoClass*)property;
			ret = GxCas_Set(GXCAS_CD_WRITE_FEED_DATA, &(temp->wTvsID));
			if (CDCA_RC_OK == ret)
			{
			    return 0;
			}
			else if(CDCA_RC_FEEDTIME_NOT_ARRIVE == ret)
			{
			    return -2;
			}
			else
			{
			    return -1;
			}
			break;

		default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
	}
	
    return -1;
#else
    return -1;
#endif
}


