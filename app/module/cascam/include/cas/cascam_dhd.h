#ifndef __CASCAM_DHD_H__
#define __CASCAM_DHD_H__

//#define DHD_OSD_INSERT_CARD    "Encryption program, please insert a smart card!"/* 加扰节目，请插入智能卡！*/
#define DHD_OSD_INSERT_CARD    "加扰节目，请插入智能卡！"
#define DHD_OSD_UNUSED_CARD    "Unable to identify card!"/* 无法识别智能卡*/
#define DHD_OSD_NOENTITLE      "Not authored"/*  节目未经授权 */
#define DHD_OSD_NOOPERATORS    "No program operators in card!"/* 卡中不存在节目运营商*/
#define DHD_OSD_BLOCK_AREA     "Regional prohibition!"/* 区域禁播*/
#define DHD_OSD_EXPIRED        "Expired"/*  节目不在授权的期间内 */
#define DHD_OSD_PASSWORD_ERROR "Operator password error!"/* 运营商密码错误！*/
#define DHD_OSD_ERRREGION      "Error zone"/*  错误的区域 */
#define DHD_OSD_PAIRING_ERROR  "Card not match"/*  非法的机顶盒,或者机卡不匹配 */
#define DHD_OSD_WATCHLEVEL     "Viewing level limit!"/* 节目级别高于设定观看级别!*/
#define DHD_OSD_EXPIRED_CARD   "Expired card!"/* 智能卡过期！*/
#define DHD_OSD_DECRYPTFAIL    "Decryption failure!"/* 解密失败！*/
#define DHD_OSD_OUT_TIME       "Not in working hours!"/* 不再工作时段内！*/
#define DHD_OSD_UPDATE         "Discover new programs, start searching！"/* 发现新节目，开始自动搜索！*/

typedef enum  _DHDBasicInfoEnum{
    DHD_BASIC_ALL,
    DHD_BASIC_CARD_ID,
    DHD_BASIC_CARD_VER,
    DHD_BASIC_STB_ID,
    DHD_BASIC_STB_PAIR,
    DHD_BASIC_CAS_ID,
    DHD_BASIC_CAS_VER,
    DHD_BASIC_WATCH_TIME,
    DHD_BASIC_WATCH_RATING,
    DHD_BASIC_CHIP_ID,
    DHD_BASIC_BALANCE,
}DHDBasicInfoEnum;

typedef struct _CASCAMDHDBasicInfoClass{
    unsigned char card_id[9];
    unsigned char card_version[30];
    unsigned char stb_id[9];
    unsigned char stb_match_id[50];
    unsigned char cas_id[15];
    unsigned char cas_version[30];
    unsigned char watch_time[20];
    unsigned char watch_rating[30];
    unsigned char chip_id[16];
}CASCAMDHDBasicInfoClass;

typedef enum _DHDEntitlementEnum{
    DHD_OPERATOR_ENTITLEMENT_COUNT,
    DHD_ENTITLEMENT_DATA,
}DHDEntitlementEnum;

typedef struct _CASCAMDHDEntitlementUnitClass{
    unsigned char id;
    unsigned short start;
    unsigned short end;
}CASCAMDHDEntitlementUnitClass;

typedef struct _CASCAMDHDEntitlementInfoClass{
    CASCAMDHDEntitlementUnitClass unit[50];
    unsigned short count;
}CASCAMDHDEntitlementInfoClass;

typedef enum _DHDOperatorEnum{
    DHD_OPERATOR_INFO_COUNT,
    DHD_OPERATOR_DATA,
}DHDOperatorEnum;

typedef struct _CASCAMDHDOperatorUnitClass{
    unsigned char operator_name[20];
    unsigned char network_id[6];
    unsigned char operator_id[11];
    unsigned char expired_time[20];
}CASCAMDHDOperatorUnitClass;

typedef struct _CASCAMDHDOperatorInfoClass{
    CASCAMDHDOperatorUnitClass *operator_info;
    unsigned short operator_count;
}CASCAMDHDOperatorInfoClass;

#endif
