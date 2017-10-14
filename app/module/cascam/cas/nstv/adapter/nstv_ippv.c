/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_ippv.c
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

static int _ippv_infomation_count(unsigned short *pCount)
{
    int ret = 0;
    unsigned short operator_id = 0;
    SCDCAIppvInfo   info[CDCA_MAXNUM_IPPVP];
    unsigned short ippv_count = CDCA_MAXNUM_IPPVP;
    SCDCATVSSlotInfo slot_info;
    int i = 0;
    int money = 0;
    unsigned int temp = 0;

    if (NULL == pCount) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        *pCount = 0;
        return -1;
    }

    cascam_memset(info, 0, sizeof(info));
    cascam_memset(&slot_info, 0, sizeof(slot_info));
    operator_id = *pCount;

    GxCas_CdGetIppvInfo ippv_param = {0};
    ippv_param.operator_id = operator_id;
    ippv_param.count = ippv_count;
    ret = GxCas_Get(GXCAS_CD_GET_IPPV_INFO, (void *)&ippv_param);
    if (ret == CDCA_RC_OK) {
        ippv_count = ippv_param.count;
        memcpy(info, ippv_param.info, sizeof(SCDCAIppvInfo) * ippv_param.count);
        for (i = 0; i < ippv_count; i++) {
            GxCas_CdGetSlotInfo slot_param = {0};
            slot_param.operator_id = operator_id;
            slot_param.slot_id = info[i].m_bySlotID;
            ret = GxCas_Get(GXCAS_CD_GET_IPPV_SLOT_INFO, (void *)&slot_param);
            memcpy(&slot_info, &slot_param.info, sizeof(GxCas_CdGetSlotInfo));
            if (ret != CDCA_RC_OK) {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                continue;
            }
            money = slot_info.m_wCreditLimit - slot_info.m_wBalance - info[i].m_wPrice;
            if ((money < 0) || (info[i].m_wPrice == 0)) {
                cascam_printf("\n\033[36mNSTV, ERROR, %s, %d\033[0m\n", __FUNCTION__, __LINE__);
                continue;
            }
            temp++;
        }
        *pCount = temp;
    }
    else {
        *pCount = 0;
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return 0;
}

#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))

static unsigned char  thiz_slot_ids[CDCA_MAXNUM_SLOT] = {0};
static unsigned int   thiz_slot_id_count = 0;
static int _get_slot_ids(unsigned int operator_id)
{
    int ret = 0;
    int i = 0;
    GxCas_CdGetIppvSlotIDS slot_ids;
    memset(&slot_ids, 0 , sizeof(GxCas_CdGetIppvSlotIDS));
    slot_ids.operator_id = operator_id;
    ret = GxCas_Get(GXCAS_CD_GET_IPPV_SLOT_IDS, (void *)&slot_ids);
    memcpy(thiz_slot_ids, slot_ids.slot_id, sizeof(CDCA_U8) * CDCA_MAXNUM_SLOT);
    if (ret == CDCA_RC_OK) {
        for (i = 0; i < CDCA_MAXNUM_SLOT; i++) {
            if (0 == thiz_slot_ids[i]) {
                break;
            }
        }
        thiz_slot_id_count = i;
    }
    else {
        cascam_printf("\nNSTV, error, %s, %d\n", __FUNCTION__, __LINE__);
        thiz_slot_id_count = 0;
    }
    return ret;
}

// 0: invaild, 1:vaild
static int _check_slot_invaild(unsigned int operator_id, unsigned char slot_id)
{
    int i = 0;
    for (i = 0; i < thiz_slot_id_count; i++) {
        if (thiz_slot_ids[i] == slot_id) {
            return 1;
        }
    }
    return 0;
}

static int _ippv_information_data(CASCAMNSTVIPPVInfoClass *Info)
{
    unsigned int ret = 0;
    unsigned short i = 0;
    CASCAMNSTVIPPVUnitClass *p = NULL;
    SCDCAIppvInfo   info[CDCA_MAXNUM_IPPVP];
    unsigned short ippv_count = CDCA_MAXNUM_IPPVP;
    SCDCATVSSlotInfo slot_info = {0};
    int money = 0;
    int j = 0;

    if ((NULL == Info) || (0 == Info->info_count) || (NULL == Info->info_unit)) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    cascam_memset(info, 0, sizeof(info));
    cascam_memset(&slot_info, 0, sizeof(slot_info));

    p = Info->info_unit;
    GxCas_CdGetIppvInfo param = {0};
    param.operator_id = Info->operator_id;
    param.count = ippv_count;
    ret = GxCas_Get(GXCAS_CD_GET_IPPV_INFO, (void *)&param);
    if (ret == CDCA_RC_OK) {
        ippv_count = param.count;
        memcpy(info, param.info, ippv_count * sizeof(SCDCAIppvInfo));
        _get_slot_ids(Info->operator_id);

        for (i = 0; i < ippv_count; i++) {
            if ((money < 0) || (info[i].m_wPrice == 0) || (0 == _check_slot_invaild(Info->operator_id, info[i].m_bySlotID))) {
                cascam_printf("\n\033[36mNSTV, ERROR, %s, %d, money = %d, price = %d, slotid = %d\033[0m\n", __FUNCTION__, __LINE__, money, info[i].m_wPrice, info[i].m_bySlotID);
                continue;
            }
            else {
                unsigned int mjd = 0;
                unsigned short year = 0;
                unsigned char month = 0;
                unsigned char day = 0;
                unsigned char week = 0;
                cascam_sprintf(p[j].product_id, "%u", info[i].m_dwProductID);

                cascam_ymd_to_mjd(&mjd, (2000 - 1900), 1, 1);
                mjd += info[i].m_wExpiredDate;
                cascam_mjd_to_ymd(mjd, &year, &month, &day, &week);
                cascam_sprintf(p[j].expired_time, "%04d-%02d-%02d", year, month, day);

                p[j].product_status = info[i].m_byBookEdFlag;
                p[j].record_flag = info[i].m_bCanTape;
                p[j].product_price = info[i].m_wPrice;
                p[j].slot_id = info[i].m_bySlotID;

                j++;
                if (j > Info->info_count) {
                    break;
                }
            }

        }
    }
    else {
        Info->info_count = 0;
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    Info->info_count = j;
    return ret;
}

#endif

int nstv_ippv_information_get(unsigned int sub_id, void *property, unsigned int len)
{
#if NSCAS_SMC_VERSION
    int ret = -1;
    if ((NULL == property) || (0 == len)) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    switch (sub_id) {
        case NSTV_OPERATOR_IPPV_COUNT:
            if (len < sizeof(short)) {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _ippv_infomation_count((unsigned short *)property);
            break;
        case NSTV_IPPV_DATA:
            if (len != sizeof(CASCAMNSTVIPPVInfoClass)) {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _ippv_information_data((CASCAMNSTVIPPVInfoClass *)property);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }
#endif
    return 0;
}
