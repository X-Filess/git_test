/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_detitle.c
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

#if NSCAS_SMC_VERSION

static int _detitle_infomation_count(unsigned short *pCount)
{
    unsigned int ret = 0;
	unsigned short i = 0;
    unsigned short operator_id = 0;
    unsigned char  read_flag = 0;
    unsigned long detitle_chknums[CDCA_MAXNUM_DETITLE] = {0};

    if(NULL == pCount)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        *pCount = 0;
        return -1;
    }
    operator_id = *pCount;
    cascam_memset(detitle_chknums, 0, sizeof(detitle_chknums));
    GxCas_CdDetitleChk param = {0};
    param.operator_id = operator_id;
    ret = GxCas_Get(GXCAS_CD_GET_DETITLE, (void *)&param);
    read_flag = param.read_status;
    memcpy(detitle_chknums, param.chknums, sizeof(CDCA_U32)* CDCA_MAXNUM_DETITLE);
    if (ret == CDCA_RC_OK)
    {
        for(i = 0; i < CDCA_MAXNUM_DETITLE; i++ )
        {
            if(0 == detitle_chknums[i])
            {
                break;
            }
        }
    }
    else
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        *pCount = 0;
        return -1;
    }

    *pCount = i;
    return ret;
}

#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))

static int _detitle_information_data(CASCAMNSTVDetitleInfoClass *Info)
{
    int ret = 0;
    int i = 0;
    unsigned char  read_flag = 0;
    unsigned long detitle_chknums[CDCA_MAXNUM_DETITLE] = {0};
    CASCAMNSTVDetitleUnitClass *p = NULL;
    unsigned int count = 0;

    if((NULL == Info) || (0 == Info->info_count) || (NULL == Info->info_unit))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    p = Info->info_unit;
    cascam_memset(detitle_chknums, 0, sizeof(detitle_chknums));

    GxCas_CdDetitleChk param = {0};
    param.operator_id = Info->operator_id;;
    ret = GxCas_Get(GXCAS_CD_GET_DETITLE, (void *)&param);
    read_flag = param.read_status;
    memcpy(detitle_chknums, param.chknums, sizeof(CDCA_U32)* CDCA_MAXNUM_DETITLE);
    if (ret == CDCA_RC_OK)
    {
        count = Info->info_count > CDCA_MAXNUM_DETITLE? CDCA_MAXNUM_DETITLE: Info->info_count;
        for(i = 0; i < count; i++)
        {
            if(0 == detitle_chknums[i])
            {
                break;
            }
            p[i].detitle_code = detitle_chknums[i]; 
            p[i].read_flag = 1;
        }
    }
    else
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        Info->info_count = 0;
        return -1;
    }
    Info->info_count = i;

#if 0
    // TODO: 20160409
    Info->info_count = 1;
    Info->info_unit[0].detitle_code = 1234;
    Info->info_unit[0].read_flag = 0;
#endif
	return 0;

}

static int _detitle_delete(CASCAMNSTVDetitleDeleteClass *Info)
{
    int ret = 0;
    if(NULL == Info)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    //Info->detitle_code: 0: delete all
    GxCas_CdSetDelDetitleNum cd_del_detitle = {0};
    cd_del_detitle.wTvsID = Info->operator_id;
    cd_del_detitle.dwDetitleChkNum = Info->detitle_code;
    ret = GxCas_Set(GXCAS_CD_DELETE_DETITLE, (void *)&cd_del_detitle);
    if (ret == CDCA_FALSE)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    return 0;
}

#endif

int nstv_detitle_information_get(unsigned int sub_id, void *property, unsigned int len)
{
    int ret = -1;

#if NSCAS_SMC_VERSION
    if((NULL == property) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_OPERATOR_DETITLE_COUNT:
            if(len < sizeof(short))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
            ret = _detitle_infomation_count((unsigned short*)property);
            break;
        case NSTV_DETITLE_DATA:
            if(len != sizeof(CASCAMNSTVDetitleInfoClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
            ret = _detitle_information_data((CASCAMNSTVDetitleInfoClass*)property);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
            break;
    }
#endif

    return ret;
}

int nstv_detitle_information_edit(unsigned int sub_id, void *property, unsigned int len)
{
    int ret = -1;

#if NSCAS_SMC_VERSION
    if((NULL == property) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_DETITLE_DEL:
            if(len != sizeof(CASCAMNSTVDetitleDeleteClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
                return -1;
            }
            ret = _detitle_delete((CASCAMNSTVDetitleDeleteClass*)property);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
            break;
    }
#endif

    return ret;
}
