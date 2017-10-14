/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_operator.c
* Author    :   B.Z.
* Project   :	GoXceed
* Type      :   CA Module
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
VERSION	    Date			  AUTHOR         Description
1.0	      2015.09.01		   B.Z.			  creation
*****************************************************************************/
#include "../../../include/cascam_platform.h"
#include "../../../include/cas/cascam_nstv.h"
#include "../../../include/cascam_os.h"
#include "../include/porting/cd_api.h"
#include "../include/porting/CDCASS.h"

static int _operator_infomation_count(unsigned short *pCount)
{
    unsigned int ret = 0;
	unsigned short i = 0;
    unsigned short operator_id[CDCA_MAXNUM_OPERATOR] = {0};

    if(NULL == pCount)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        *pCount = 0;
        return -1;
    }
    //*pCount = 1;
    ret = GxCas_Get(GXCAS_CD_GET_OPERATOR_ID, (void *)operator_id);
    if (ret != CDCA_RC_OK)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        *pCount = 0;
        return -1;
    }
    for(i = 0; i < CDCA_MAXNUM_OPERATOR; i++)
    {
        if(operator_id[i] == 0)
            break;
    }
    *pCount = i;
    return ret;
}

#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))

static int _operator_information_data(CASCAMNSTVOperatorInfoClass *Info)
{
    unsigned int ret = 0;
	unsigned short i = 0;
    CASCAMNSTVOperatorUnitClass *p = NULL;
    SCDCAOperatorInfo operator_info;
    unsigned short operator_id[CDCA_MAXNUM_OPERATOR] = {0};

    if((NULL == Info) || (0 == Info->operator_count) || (NULL == Info->operator_info))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    p = Info->operator_info;
    // TODO: 20160409
    //ret = GxCas_Get(GXCAS_GET_ENTITLE_OPERATOR_ID, (void*)operator_id, sizeof(unsigned short)*CDCA_MAXNUM_OPERATOR);
    //if(ret)
    ret = GxCas_Get(GXCAS_CD_GET_OPERATOR_ID, (void *)operator_id);
    if (ret != CDCA_RC_OK)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < Info->operator_count; i++)
    {
        if(operator_id[i] == 0)
            break;
        cascam_memset(&operator_info, 0, sizeof(SCDCAOperatorInfo));
	GxCas_CdGetOperatorInfo gxcas_operator_info = {0};
	gxcas_operator_info.operator_id = operator_id[i];
	ret = GxCas_Get(GXCAS_CD_GET_OPERATOR_INFO, (void *)&gxcas_operator_info);
	memcpy(&operator_info, &gxcas_operator_info.operator_info, sizeof(SCDCAOperatorInfo));
	if (ret != CDCA_RC_OK)
        {
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
            return -1;
        }
        if(cascam_strlen(operator_info.m_szTVSPriInfo) > 0)
        {
            cascam_sprintf(p[i].operator_des, "%s", operator_info.m_szTVSPriInfo);
        }
        else
        {
            cascam_memcpy(p[i].operator_des, "None", 4);
        }
        cascam_sprintf(p[i].operator_id, "%05d", operator_id[i]);    
    }

    //Info->operator_count = 1;
    //cascam_memcpy(p->operator_des, "SkyNet", 6);
    //cascam_memcpy(p->operator_id, "00001", 5);
	return ret;
}

int nstv_operator_information_get(unsigned int sub_id, void *property, unsigned int len)
{
    int ret = -1;
    if((NULL == property) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_OPERATOR_INFO_COUNT:
            if(len < sizeof(short))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
            ret = _operator_infomation_count((unsigned short*)property);
            break;
        case NSTV_OPERATOR_DATA:
            if(len != sizeof(CASCAMNSTVOperatorInfoClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
            ret = _operator_information_data((CASCAMNSTVOperatorInfoClass*)property);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
            break;
    }

    return ret;
}
