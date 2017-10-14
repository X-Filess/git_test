/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_entitle.c
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

static void _cddate_to_ymd(unsigned short cd_data,
                            unsigned short* year,
                            unsigned char* month,
                            unsigned char* day)
{
    unsigned int mjd = 0;
    unsigned char week = 0;
    if((NULL == year) || (NULL == month) || (NULL == day))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    cascam_ymd_to_mjd(&mjd, (2000-1900), 1, 1);
    mjd += cd_data;
    cascam_mjd_to_ymd(mjd, year, month, day, &week);
}


static int _entitle_infomation_count(unsigned short *pCount)
{
    unsigned int ret = 0;
    unsigned short operator_id = 0;
    SCDCAEntitles  entitle_info = {0};

    if(NULL == pCount)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        *pCount = 0;
        return -1;
    }
    operator_id = *pCount;// means operator id, need input
    GxCas_CdGetEntitle  info = {0};
    info.operator_id = operator_id;
    ret = GxCas_Get(GXCAS_CD_GET_ENTITLE_INFO, (void *)&info);
    memcpy(&entitle_info, &info.entitle_info, sizeof(SCDCAEntitles));
    if (ret != CDCA_RC_OK)
    //ret = GxCas_Get(GXCAS_GET_ENTITLE_COUNT, (void*)&temp, sizeof(unsigned short));
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        *pCount = 0;
        return -1;
    }
    *pCount = entitle_info.m_wProductCount;
    return ret;
}

#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))

static int _entitle_information_data(CASCAMNSTVEntitlementInfoClass *Info)
{
    unsigned int ret = 0;
	unsigned short i = 0;
    CASCAMNSTVEntitlementUnitClass *p = NULL;
    unsigned short temp = 0;
    SCDCAEntitles entitle_info = {0};
    unsigned short year = 0;
    unsigned char month = 0;
    unsigned char day = 0;

    if((NULL == Info) || (0 == Info->info_count) || (NULL == Info->info_unit))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    cascam_memset(&entitle_info, 0, sizeof(SCDCAEntitles));
    p = Info->info_unit;
    //ret = GxCas_Get(GXCAS_GET_ENTITLE_COUNT, (void*)&temp, sizeof(unsigned short));
    GxCas_CdGetEntitle  info = {0};
    info.operator_id = Info->operator_id;
    ret = GxCas_Get(GXCAS_CD_GET_ENTITLE_INFO, (void *)&info);
    memcpy(&entitle_info, &info.entitle_info, sizeof(SCDCAEntitles));
    if (ret != CDCA_RC_OK)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        Info->info_count = 0;
        return -1;
    }
    temp = Info->info_count > entitle_info.m_wProductCount ? entitle_info.m_wProductCount:Info->info_count;
    for(i = 0; i < temp; i++)
    {
        cascam_sprintf(p[i].product_id, "%u", entitle_info.m_Entitles[i].m_dwProductID);
        _cddate_to_ymd(entitle_info.m_Entitles[i].m_tBeginDate, &year, &month, &day);
        cascam_sprintf(p[i].start_time, "%4d-%02d-%02d", year, month, day);
        _cddate_to_ymd(entitle_info.m_Entitles[i].m_tExpireDate, &year, &month, &day);
        cascam_sprintf(p[i].end_time, "%4d-%02d-%02d", year, month, day);
        p[i].is_support_record = entitle_info.m_Entitles[i].m_bCanTape;

    }
    Info->info_count = temp;
    // TODO: 20160409
    //Info->info_count = 1;
    //cascam_memcpy(p->product_id, "00001", 5);
    //cascam_memcpy(p->start_time, "2015-10-16", 10);
    //cascam_memcpy(p->end_time, "2016-10-16", 10);
    //p->is_support_record = 1;
	return ret;

}

int nstv_entitle_information_get(unsigned int sub_id, void *property, unsigned int len)
{
    int ret = -1;
    if((NULL == property) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_OPERATOR_ENTITLEMENT_COUNT:
            if(len < sizeof(short))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
            ret = _entitle_infomation_count((unsigned short*)property);
            break;
        case NSTV_ENTITLEMENT_DATA:
            if(len != sizeof(CASCAMNSTVEntitlementInfoClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
            ret = _entitle_information_data((CASCAMNSTVEntitlementInfoClass*)property);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
            break;
    }

    return ret;
}

