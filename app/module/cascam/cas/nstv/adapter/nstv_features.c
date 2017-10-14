/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_features.c
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
#include "../include/porting/CDCASS.h"
#include "../include/porting/cd_api.h"

static int _features_infomation_count(unsigned short *pCount)
{
    unsigned int ret = 0;
	unsigned short i = 0;
    unsigned short operator_id = 0;
    unsigned long ac_list[CDCA_MAXNUM_ACLIST] = {0};

    if(NULL == pCount)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        *pCount = 0;
        return -1;
    }
    operator_id = *pCount;
    GxCas_CdGetAclist gxcas_ac = {0};
    gxcas_ac.operator_id = operator_id;
    ret = GxCas_Get(GXCAS_CD_GET_FEATURE, (void *)&gxcas_ac);
    memcpy(ac_list, gxcas_ac.ac_list, sizeof(unsigned long) * CDCA_MAXNUM_ACLIST);
    if (ret != CDCA_RC_OK)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        *pCount = 0;
        return -1;
    }
    //i = CDCA_MAXNUM_ACLIST - 3;
    for(i = 3; i < CDCA_MAXNUM_ACLIST; i++)
    {
        if(ac_list[i] == 0)
        {
            break;
        }
    }
    *pCount = i;
    return ret;
}

#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))

static int _features_information_data(CASCAMNSTVFeaturesInfoClass *Info)
{
    unsigned int ret = 0;
	unsigned short i = 0;
	unsigned short j = 0;
    CASCAMNSTVFeaturesUnitClass *p = NULL;
    unsigned long ac_list[CDCA_MAXNUM_ACLIST] = {0};
    unsigned bytes = 0;

    if((NULL == Info) || (0 == Info->info_count) || (NULL == Info->info_unit))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    p = Info->info_unit;

    GxCas_CdGetAclist cd_aclist = {0};;
    cd_aclist.operator_id  = Info->operator_id;
    ret = GxCas_Get(GXCAS_CD_GET_FEATURE, (void *)&cd_aclist);
    memcpy(ac_list, cd_aclist.ac_list, sizeof(CDCA_U32) * CDCA_MAXNUM_ACLIST);
    if (ret != CDCA_RC_OK)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        Info->info_count = 0;
        return -1;
    }

#if NSCAS_SMC_VERSION
    bytes = (Info->info_count + 3);
    bytes = bytes > CDCA_MAXNUM_ACLIST?CDCA_MAXNUM_ACLIST:bytes;
    // counts, features: 0~6
    bytes = bytes > (7 + 3)?10:bytes;
    for(i = 3; i < bytes; i++)
    {
        //if(ac_list[i] == 0)
        //{
        //    continue;
        //}
        p[j].area_code = ac_list[0];
        p[j].bouquet_id = ac_list[1];
        p[j].features_id = ac_list[2];
        p[j].features_data = ac_list[i];
        j++;
    }
    Info->info_count = j;
#else
    Info->info_count = 1;
    p[0].area_code = ac_list[0];
    p[0].bouquet_id = ac_list[1];
#endif

#if 0
    // TODO: 20160409
    Info->info_count = 1;
    p[0].area_code = 1;
    p[0].bouquet_id = 2;
    p[0].features_id = 3;
    p[0].features_data = 0x12345678;
#endif
	return ret;

}

int nstv_features_information_get(unsigned int sub_id, void *property, unsigned int len)
{
    int ret = -1;
    if((NULL == property) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_OPERATOR_FEATURES_COUNT:
            if(len < sizeof(short))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
            ret = _features_infomation_count((unsigned short*)property);
            break;
        case NSTV_FEATURES_DATA:
            if(len != sizeof(CASCAMNSTVFeaturesInfoClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
            ret = _features_information_data((CASCAMNSTVFeaturesInfoClass*)property);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
            break;
    }

    return ret;
}
