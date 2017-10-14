/**
 *
 * @file        cascam_tnt.h
 * @brief
 * @version     1.1.0
 * @date        10/15/2015 09:47:49 AM
 * @author      B.Z., 88156088@nationalchip.com
 *
 */
// BASIC_INFO: sub id
typedef enum  _TNTBasicInfoEnum{
    TNT_BASIC_ALL,
    TNT_BASIC_CARD_ID,
    TNT_BASIC_CARD_VER,
    TNT_BASIC_STB_ID,
    TNT_BASIC_STB_PAIR,
    TNT_BASIC_CAS_ID,
    TNT_BASIC_CAS_VER,
    TNT_BASIC_WATCH_TIME,
    TNT_BASIC_WATCH_RATING,
    TNT_BASIC_CHIP_ID,
}TNTBasicInfoEnum;

typedef struct _CASCAMTNTBasicInfoClass{
    char card_id[9];// ex. 80000030
    char card_version[30];// ex. DVBCA1.0
    char stb_id[9];//ex. 01020304
    char stb_match_id[50];// max support 4 stb, ex. 01020304, 01020305, 01020306, 01020307
    char cas_id[15];//ex. 0x1773
    char cas_version[30];// ex. ThinewCAS
    char watch_time[20];// ex. 00:00 - 23:59, (start[hh:mm] - end[hh:mm] )
    char watch_rating[30];//ex. Less than 18 years old
    char chip_id[16];// ex. 20151015154355
}CASCAMTNTBasicInfoClass;

// ENTITLEMENT_INFO:sub id
typedef enum _TNTEntitlementEnum{
    TNT_OPERATOR_ENTITLEMENT_COUNT,
    TNT_ENTITLEMENT_DATA,
}TNTEntitlementEnum;

typedef struct _CASCAMTNTEntitlementUnitClass{
    char product_id[6];// ex. 00001 ~ 65536
    char start_time[11];// ex. 2015-10-16
    char end_time[11];// ex. 2016-10-16
}CASCAMTNTEntitlementUnitClass;

typedef struct _CASCAMTNTEntitlementInfoClass{
    CASCAMTNTEntitlementUnitClass info_unit[100];
    unsigned short info_count; // the max is 100, from thinew cas
    unsigned int operator_id;
}CASCAMTNTEntitlementInfoClass;

typedef struct _CASCAMTNTOperatorEntitlementClass{
    CASCAMTNTEntitlementInfoClass *operator_entitlement;
    unsigned short operator_count;
}CASCAMTNTOperatorEntitlementClass;

// OPERATOR_INFO: sub id
typedef enum _TNTOperatorEnum{
    TNT_OPERATOR_INFO_COUNT,
    TNT_OPERATOR_DATA,
}TNTOperatorEnum;

typedef struct _CASCAMTNTOperatorUnitClass{
    unsigned char operator_name[20];// "DVBCA_OPERATOR_NAME_SIZE", from thinew doc
    unsigned char network_id[6];// ex. 00001
    unsigned char operator_id[11];// ex. 0000000001
    unsigned char expired_time[20];//ex. 2015-10-16 11:20:55
}CASCAMTNTOperatorUnitClass;

typedef struct _CASCAMTNTOperatorInfoClass{
    CASCAMTNTOperatorUnitClass *operator_info;
    unsigned short operator_count;
}CASCAMTNTOperatorInfoClass;

// for OSD
#define TNT_OSD_NOT_LIMITED             "Not Limited"/*没有限制*/
#define TNT_OSD_LESS_18                 "Less Than 18 Years Old"/*小于18岁*/
#define TNT_OSD_LESS_16                 "Less Than 16 Years Old"/*小于16岁*/
#define TNT_OSD_LESS_14                 "Less Than 14 Years Old"/*小于14岁*/
#define TNT_OSD_LESS_12                 "Less Than 12 Years Old"/*小于12岁*/
#define TNT_OSD_LESS_8                  "Less Than 8 Years Old"/*小于8岁*/
#define TNT_OSD_LESS_6                  "Less Than 6 Years Old"/*小于6岁*/
#define TNT_OSD_LESS_4                  "Less Than 4 Years Old"/*小于4岁*/
// 
#define TNT_OSD_INSERT_CARD             "Encryption program, please insert a smart card!"/*加扰节目，请插入智能卡！*/
#define TNT_OSD_UNUSED_CARD             "Unable to identify card!"/*无法识别智能卡*/
#define TNT_OSD_NOENTITLE               "Not authored"/* 节目未经授权 */
#define TNT_OSD_NOOPERATORS             "No program operators in card!"/*卡中不存在节目运营商*/
#define TNT_OSD_BLOCK_AREA              "Regional prohibition!"/*区域禁播*/
#define TNT_OSD_EXPIRED                 "Expired"/* 节目不在授权的期间内 */
#define TNT_OSD_PASSWORD_ERROR          "Operator password error!"/*运营商密码错误！*/
#define TNT_OSD_ERRREGION               "Error zone"/* 错误的区域 */
#define TNT_OSD_PAIRING_ERROR           "Card not match"/* 非法的机顶盒,或者机卡不匹配 */
#define TNT_OSD_WATCHLEVEL              "Viewing level limit!"/*节目级别高于设定观看级别!*/
#define TNT_OSD_EXPIRED_CARD            "Expired card!"/*智能卡过期！*/
#define TNT_OSD_DECRYPTFAIL             "Decryption failure!"/*解密失败！*/
#define TNT_OSD_OUT_TIME                "Not in working hours!"/*不再工作时段内！*/
#define TNT_OSD_UPDATE                  "Discover new programs, start searching！"/*发现新节目，开始自动搜索！*/

