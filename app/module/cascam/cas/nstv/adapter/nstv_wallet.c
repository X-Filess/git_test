/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_wallet.c
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

static int _wallet_infomation_count(unsigned short *pCount)
{
    unsigned int ret = 0;
    unsigned short i = 0;
    unsigned short operator_id = 0;
    unsigned char  slot_arry[CDCA_MAXNUM_SLOT] = {0};

    if (NULL == pCount) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        *pCount = 0;
        return -1;
    }
    operator_id = *pCount;
    GxCas_CdGetIppvSlotIDS slot_ids;
    memset(&slot_ids, 0 , sizeof(GxCas_CdGetIppvSlotIDS));
    slot_ids.operator_id = operator_id;
    ret = GxCas_Get(GXCAS_CD_GET_IPPV_SLOT_IDS, (void *)&slot_ids);
    memcpy(slot_arry, slot_ids.slot_id, sizeof(CDCA_U8) * CDCA_MAXNUM_SLOT);
    if (ret == CDCA_RC_OK) {
        for (i = 0; i < CDCA_MAXNUM_SLOT; i++) {
            if (0 == slot_arry[i]) {
                break;
            }
        }
    }
    else {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    *pCount = i;
    return 0;
}

#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))

static int _wallet_information_data(CASCAMNSTVWalletInfoClass *Info)
{
    unsigned int ret = 0;
    unsigned short i = 0;
    CASCAMNSTVWalletUnitClass *p = NULL;
    unsigned char  slot_arry[CDCA_MAXNUM_SLOT] = {0};
    SCDCATVSSlotInfo slot_info = {0};

    if ((NULL == Info) || (0 == Info->info_count) || (NULL == Info->info_unit)) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    p = Info->info_unit;

    GxCas_CdGetIppvSlotIDS slot_ids;
    memset(&slot_ids, 0 , sizeof(GxCas_CdGetIppvSlotIDS));
    slot_ids.operator_id = Info->operator_id;
    ret = GxCas_Get(GXCAS_CD_GET_IPPV_SLOT_IDS, (void *)&slot_ids);
    memcpy(slot_arry, slot_ids.slot_id, sizeof(CDCA_U8) * CDCA_MAXNUM_SLOT);
    if (ret == CDCA_RC_OK) {
        for (i = 0; i < CDCA_MAXNUM_SLOT; i++) {
            if (0 == slot_arry[i]) {
                break;
            }

            cascam_memset(&slot_info, 0, sizeof(SCDCATVSSlotInfo));
            GxCas_CdGetSlotInfo gxcas_slot_info = {0};
            gxcas_slot_info.operator_id = Info->operator_id;
            gxcas_slot_info.slot_id = slot_arry[i];
            ret = GxCas_Get(GXCAS_CD_GET_IPPV_SLOT_INFO, (void *)&gxcas_slot_info);
            memcpy(&slot_info, &gxcas_slot_info.info, sizeof(SCDCATVSSlotInfo));
            if (ret == CDCA_RC_OK) {
                p[i].wallet_id = slot_arry[i];
                p[i].credit_point = slot_info.m_wCreditLimit;
                p[i].used_point = slot_info.m_wBalance;
            }
            else {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                Info->info_count = 0;
                return -1;
            }
        }
    }
    else {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        Info->info_count = 0;
        return -1;
    }
    Info->info_count = i;

    return ret;

}

#endif

int nstv_wallet_information_get(unsigned int sub_id, void *property, unsigned int len)
{
    int ret = -1;

    #if NSCAS_SMC_VERSION
    if ((NULL == property) || (0 == len)) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    switch (sub_id) {
        case NSTV_OPERATOR_ENTITLEMENT_COUNT:
            if (len < sizeof(short)) {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _wallet_infomation_count((unsigned short *)property);
            break;
        case NSTV_ENTITLEMENT_DATA:
            if (len != sizeof(CASCAMNSTVWalletInfoClass)) {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _wallet_information_data((CASCAMNSTVWalletInfoClass *)property);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }

    #endif
    return ret;
}
