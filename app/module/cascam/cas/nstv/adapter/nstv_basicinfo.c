/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_baseinfo.c
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
#include "../../../include/cas/cascam_nstv.h"
#include "../../../include/cascam_platform.h"
#include "../../../include/cascam_os.h"
#include "../include/porting/cd_api.h"
#include "../include/porting/CDCASS.h"

#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))

static int _get_basic_information(void *pInfo)
{
    int ret = 0;
    CASCAMNSTVBasicInfoClass *pBasicInfo = NULL;
    if (NULL == pInfo) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    pBasicInfo = (CASCAMNSTVBasicInfoClass *)pInfo;

    #if NSCAS_SMC_VERSION
    // card sn
    ret = GxCas_Get(GXCAS_CD_GET_SN, pBasicInfo->card_sn);
    if (ret != CDCA_RC_OK) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        goto err;
    }
    #endif

    // stb id
    uint16_t platform_id = 0;
    GxCas_Get(GXCAS_CD_GET_PLATFORM, (void *)&platform_id);

    unsigned char chip_id[4] = {0};
    GxCas_Get(GXCAS_CD_GET_CHIP_ID, (void *)&chip_id);
    cascam_sprintf(pBasicInfo->stb_id , "%04x%02x%02x%02x%02x", platform_id, chip_id[3], chip_id[2], chip_id[1], chip_id[0]);

    #if NSCAS_SMC_VERSION
    // stb match status
    // 该方案使用onboard 卡，不存在机卡匹配不上的问题，下面就直接写匹配
    // 运营商要求
    unsigned char stb_num = 0;// max is 5
    unsigned char stb_id[30] = {0};// 6bytes*5 = 30bytes;
    GxCas_CdGetPaired paired = {0};
    ret = GxCas_Get(GXCAS_CD_GET_IS_PAIRED, (void *)&paired);
    stb_num = paired.pbyNum;
    cascam_memcpy(stb_id, paired.stb_id, sizeof(stb_id));
    if (CDCA_RC_OK == ret) {
        //cascam_memcpy(pBasicInfo->stb_match_status, "Paired", 6);
        int i = 0;
        char buf[30] = {0};
        cascam_strcat(pBasicInfo->stb_match_status, "Paired\n");
        for (i = 0; i < stb_num; i++) {
            cascam_memset(buf, 0, sizeof(buf));
            cascam_sprintf(buf, "STBID%d:0x%02X%02X%02X%02X%02X%02X\n",
                           i + 1,
                           stb_id[6 * i],
                           stb_id[6 * i + 1],
                           stb_id[6 * i + 2],
                           stb_id[6 * i + 3],
                           stb_id[6 * i + 4],
                           stb_id[6 * i + 5]
                          );
            cascam_strcat(pBasicInfo->stb_match_status, buf);
        }
        cascam_printf("\n\033[36m,BASEINFO, paired str = %s \033[0m\n", pBasicInfo->stb_match_status);
    }
    else if (CDCA_RC_CARD_INVALID == ret) {
        cascam_memcpy(pBasicInfo->stb_match_status, "Invalid Smartcard", 17);
    }
    else if (CDCA_RC_CARD_NOPAIR == ret) {
        cascam_memcpy(pBasicInfo->stb_match_status, "Not paired with any STB", 23);
    }
    else if (CDCA_RC_CARD_PAIROTHER == ret) {
        //cascam_memcpy(pBasicInfo->stb_match_status, "Un-pair, card is matched to another STB", 39);
        int i = 0;
        char buf[30] = {0};
        cascam_strcat(pBasicInfo->stb_match_status, "Un-pair, card is matched to another STB, ");
        for (i = 0; i < stb_num; i++) {
            cascam_memset(buf, 0, sizeof(buf));
            cascam_sprintf(buf, "STBID%d:0x%02X%02X%02X%02X%02X%02X, ",
                           i + 1,
                           stb_id[6 * i],
                           stb_id[6 * i + 1],
                           stb_id[6 * i + 2],
                           stb_id[6 * i + 3],
                           stb_id[6 * i + 4],
                           stb_id[6 * i + 5]
                          );
            cascam_strcat(pBasicInfo->stb_match_status, buf);
        }
        cascam_printf("\n\033[36m,BASEINFO, paired str = %s \033[0m\n", pBasicInfo->stb_match_status);
    }

    GxCas_CdWorkTime work_time;
    memset(&work_time, 0 , sizeof(GxCas_CdWorkTime));
    ret = GxCas_Get(GXCAS_CD_GET_WORKTIME, (void *)&work_time);
    if (ret != CDCA_RC_OK) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
    }
    cascam_sprintf(pBasicInfo->work_time,
                   "%02d:%02d:%02d - %02d:%02d:%02d",
                   work_time.start_hour, work_time.start_min, work_time.start_sec,
                   work_time.end_hour, work_time.end_min, work_time.end_sec);
    #endif

    // cas version
    unsigned int version = 0;

    GxCas_Get(GXCAS_CD_GET_CAS_VER_ID, (void *)&version);
    cascam_sprintf(pBasicInfo->cas_version, "0x%08X", version);

    // watch rating
    unsigned char rating = 0;
    ret = GxCas_Get(GXCAS_CD_GET_RATING, (void *)&rating);
    if (ret != CDCA_RC_OK) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        goto err;
    }
    cascam_sprintf(pBasicInfo->watch_rating, "%d", rating);

    // operator id
    uint16_t operator_id[CDCA_MAXNUM_OPERATOR] = {0};
    ret = GxCas_Get(GXCAS_CD_GET_OPERATOR_ID, (void *)operator_id);
    if (ret != CDCA_RC_OK) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        goto err;
    }

    int i = 0;
    char buf[6] = {0};
    for (i = 0; i < CDCA_MAXNUM_OPERATOR; i++) {
        if (operator_id[i] == 0) {
            break;
        }
        cascam_memset(buf, 0, sizeof(buf));
        cascam_sprintf(buf, "%d, ", operator_id[i]);
        cascam_strcat(pBasicInfo->operator_id, buf);
    }
    if ((i = cascam_strlen(pBasicInfo->operator_id)) > 2) {
        pBasicInfo->operator_id[i - 2] = '\0';
    }
    else {
        cascam_memcpy(pBasicInfo->operator_id, "None", 4);
    }

    // maket id
    {
        unsigned char market_id[5] = {0};
        cascam_get_market_id(market_id, 4);
        cascam_sprintf(pBasicInfo->market_id, "0x%02x%02x%02x%02x", market_id[3], market_id[2], market_id[1], market_id[0]);
        //cascam_memcpy(pBasicInfo->market_id, "0x00000000", 10);
    }

    // jtag status
    {
        int jtag_support = 0;
        cascam_get_jtag_status(&jtag_support);
        if (1 == jtag_support) {
            cascam_memcpy(pBasicInfo->jtag_status, "On", 2);
        }
        else {
            cascam_memcpy(pBasicInfo->jtag_status, "Off", 3);
        }
    }

    // security start status
    {
        int ads_support = 0;
        cascam_get_ads_status(&ads_support);
        if (1 == ads_support) {
            cascam_memcpy(pBasicInfo->security_start, "On", 2);
        }
        else {
            cascam_memcpy(pBasicInfo->security_start, "Off", 3);
        }
    }
    return 0;

err:
    return -1;
}

int nstv_basic_information_get(int sub_id, void *property, unsigned int len)
{
    int ret = -1;
    if (NULL == property) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    switch (sub_id) {
        case NSTV_BASIC_ALL:
            if (len != sizeof(CASCAMNSTVBasicInfoClass)) {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                ret = -1;
                break;
            }
            ret = _get_basic_information(property);
            break;
        case NSTV_BASIC_CARD_SN:
        case NSTV_BASIC_STB_ID:
        case NSTV_BASIC_STB_PAIR:
        case NSTV_BASIC_CAS_VER:
        case NSTV_BASIC_WORK_TIME:
        case NSTV_BASIC_WATCH_RATING:
        case NSTV_BASIC_OPERATOR_ID:
        case NSTV_BASIC_MARKET_ID:
        case NSTV_BASIC_JTAG_STATUS:
        case NSTV_BASIC_SECURITY_START:
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }
    return ret;
}
