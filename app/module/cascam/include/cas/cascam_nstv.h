/**
 *
 * @file        cascam_nstv.h
 * @brief
 * @version     1.1.0
 * @date        04/07/2016 09:47:49 AM
 * @author      B.Z.
 *
 */
#include "app_config.h"

#define NSTV_AUTHENTICATION_SUPPORT     0

#if NSTV_AUTHENTICATION_SUPPORT
#define ROLLING_COMPLETE_EXIT 0
#else
#define ROLLING_COMPLETE_EXIT 1
#endif

// BASIC_INFO: sub id
typedef enum  _NSTVBasicInfoEnum{
    NSTV_BASIC_ALL,
    NSTV_BASIC_CARD_SN,
    NSTV_BASIC_STB_ID,
    NSTV_BASIC_STB_PAIR,
    NSTV_BASIC_CAS_VER,
    NSTV_BASIC_WORK_TIME,
    NSTV_BASIC_WATCH_RATING,
    NSTV_BASIC_OPERATOR_ID,
    NSTV_BASIC_MARKET_ID,
    NSTV_BASIC_JTAG_STATUS,
    NSTV_BASIC_SECURITY_START,
}NSTVBasicInfoEnum;

typedef struct _CASCAMNSTVBasicInfoClass{
    char card_sn[17];// ex. 8000302100000333, CDCA_MAXLEN_SN
    char stb_id[13];//ex. 141D567A9127, platform id (4bytes)+ unique id(8bytes),CDCA_MAXLEN_STBSN
    char stb_match_status[256];// "Paired Ok" or "Invalid Smartcard" or "Not paired with any STB" or "Paired to other STB"
    char cas_version[11];// ex. 0x12345678. max len is 10 bytes 
    char work_time[20];// ex. 00:00:00 - 23:59:59, (start[hh:mm:ss] - end[hh:mm:ss] )
    char watch_rating[3];//ex. from 4 to 18
    char operator_id[30];// ex. 1, 2, 3, 4. [maxnum is four]
    char market_id[11];// ex. 0xF0F0F0F0
    char jtag_status[4];// ex. On or Off
    char security_start[4];// ex. On or Off
}CASCAMNSTVBasicInfoClass;

// UPDATE_INFO:
typedef enum _NSTVCardUpdateEnum{
    NSTV_CARD_UPDATE_GET,
}NSTVCardUpdateEnum;

typedef struct _CASCAMNSTVCardUpdateInfoClass{
    unsigned char update_status;// ex. 0: no update information; 1: updating; 2: update success; 3: update failed
    unsigned char update_time[20];// ex. 2016-04-08 09:54
    unsigned int  magic;//fix:0x12345678
}CASCAMNSTVCardUpdateInfoClass;

// card update
typedef struct _CASCAMNSTVCardUpdateClass{
    unsigned char mark;// 1：升级数据接收中；2：卡升级中
    unsigned char process;// 进度 0～100
}CASCAMNSTVCardUpdateClass;

// PIN_CONTROL:
typedef enum _NSTVPINEnum{
    NSTV_PIN_GET,
    NSTV_PIN_SET,
}NSTVPINEnum;

typedef struct _CASCAMNSTVPINControlClass{
    unsigned char old_pin[6];// ex. size is CDCA_MAXLEN_PINCODE, from 0~9
    unsigned char new_pin[6];// ex. size is CDCA_MAXLEN_PINCODE, from 0~9
}CASCAMNSTVPINControlClass;

// PARENTAL_CONTROL:
typedef enum _NSTVWatchLevelEnum{
    NSTV_WATCH_LEVEL_GET,
    NSTV_WATCH_LEVEL_SET,
}NSTVWatchLevelEnum;

typedef struct _CASCAMNSTVWatchLevelClass{
    unsigned char pin[6];// ex. size is CDCA_MAXLEN_PINCODE, from 0~9
    unsigned char watch_level;// from 4 to 18
}CASCAMNSTVWatchLevelClass;

// WORK_TIME_CONTROL:
typedef enum _NSTVWorkTimeEnum{
    NSTV_WORK_TIME_GET,
    NSTV_WORK_TIME_SET,
}NSTVWorkTimeEnum;

typedef struct _CASCAMNSTVWorkTimeClass{
    unsigned char pin[6];// ex. size is CDCA_MAXLEN_PINCODE, from 0~9
    unsigned int  start_time;//BCD CODE, ex. 0x235959 means 23:59:59(hh:mm:ss)
    unsigned int  end_time;//BCD CODE, ex. 0x235959 means 23:59:59(hh:mm:ss)
}CASCAMNSTVWorkTimeClass;

// MAIL_INFO: sub id
typedef enum _NSTVMailEnum{
    // GET
    NSTV_MAIL_GET_COUNT,// the max num is 100, nstv doc
    NSTV_MAIL_GET_DATA_HEAD,
    NSTV_MAIL_GET_DATA_BODY,
    // SET
    NSTV_MAIL_DEL_ALL,
    NSTV_MAIL_DEL_BY_POS,
}NSTVMailEnum;

#define MAIL_BODY_SIZE  165*3 // from nstv doc
#define MAIL_TITLE_SIZE 37*3  // from nstv doc
typedef struct _CASCAMNSTVMailBodyClass{
    char body[MAIL_BODY_SIZE];
    unsigned int pos;
}CASCAMNSTVMailBodyClass;

typedef struct _CASCAMNSTVMailHeadClass{
    unsigned int mail_id;//
    unsigned char read_status;// 0:read; 1:unread
    char          create_time[20];//2015-09-18 13:20:09
    char          importance;// 0: normal; 1:important
    char          title[MAIL_TITLE_SIZE];
}CASCAMNSTVMailHeadClass;

typedef struct _CASCAMNSTVMailInfoClass{
    unsigned short mail_total;// the max num is 100
    unsigned short mail_unread_count;
    CASCAMNSTVMailHeadClass *mail_head;
}CASCAMNSTVMailInfoClass;


// depend on operator
// ENTITLEMENT_INFO:sub id
typedef enum _NSTVEntitlementEnum{
    NSTV_OPERATOR_ENTITLEMENT_COUNT,
    NSTV_ENTITLEMENT_DATA,
}NSTVEntitlementEnum;

typedef struct _CASCAMNSTVEntitlementUnitClass{
    char product_id[11];// ex. 1 ~ 4294967295
    char start_time[11];// ex. 2015-10-16
    char end_time[11];// ex. 2016-10-16
    char is_support_record; // ex. 1: support; 0:not support
}CASCAMNSTVEntitlementUnitClass;

typedef struct _CASCAMNSTVEntitlementInfoClass{
    CASCAMNSTVEntitlementUnitClass *info_unit;
    unsigned short info_count; // the max is 300, from novel super tv doc
    unsigned int operator_id;
}CASCAMNSTVEntitlementInfoClass;

typedef struct _CASCAMNSTVOperatorEntitlementClass{
    CASCAMNSTVEntitlementInfoClass *operator_entitlement;
    unsigned short operator_count;
}CASCAMNSTVOperatorEntitlementClass;

// message
typedef struct _NSTVEntitlementMessageClass{
    unsigned short operator_id;
}NSTVEntitlementMessageClass;

// WALLET_INFO: sub id
typedef enum _NSTVWalletInfoEnum{
    NSTV_OPERATOR_WALLET_COUNT,
    NSTV_WALLET_DATA,
}NSTVWalletEnum;

typedef struct _CASCAMNSTVWalletUnitClass{
    unsigned int wallet_id;
    unsigned int credit_point;//
    unsigned int used_point;//
}CASCAMNSTVWalletUnitClass;

typedef struct _CASCAMNSTVWalletInfoClass{
    CASCAMNSTVWalletUnitClass *info_unit;
    unsigned short info_count; // the max is 20, from novel super tv doc, slot
    unsigned int operator_id;
}CASCAMNSTVWalletInfoClass;

typedef struct _CASCAMNSTVOperatorWalletClass{
    CASCAMNSTVWalletInfoClass *operator_wallet;
    unsigned short operator_count;
}CASCAMNSTVOperatorWalletClass;

// IPPV_INFO: sub id
typedef enum _NSTVIPPVInfoEnum{
    NSTV_OPERATOR_IPPV_COUNT,
    NSTV_IPPV_DATA,
}NSTVIPPVEnum;

typedef struct _CASCAMNSTVIPPVUnitClass{
    char product_id[11];// ex. 1 ~ 4294967295(0xffffffff)
    unsigned char product_status;// 0: unknow; 1: booking; 2:expired; 3: viewed
    unsigned char record_flag;// 1: can record; 0: can not record
    unsigned short product_price;//
    unsigned short slot_id; // 
    char expired_time[11];// ex. 2016-10-16
}CASCAMNSTVIPPVUnitClass;

typedef struct _CASCAMNSTVIPPVInfoClass{
    CASCAMNSTVIPPVUnitClass *info_unit;
    unsigned short info_count; // the max is 300, from novel super tv doc
    unsigned int operator_id;
}CASCAMNSTVIPPVInfoClass;

typedef struct _CASCAMNSTVOperatorIPPVClass{
    CASCAMNSTVIPPVInfoClass *operator_ippv;
    unsigned short operator_count;
}CASCAMNSTVOperatorIPPVClass;

// for buy
typedef enum _NSTVIPPVTypeEnum{
    NSTV_IPPV,
    NSTV_IPPT,
}NSTVIPPVTypeEnum;

typedef enum _NSTVIPPVPriceTypeEnum{
    NSTV_IPPVVIEW = 0,
    NSTV_IPPVVIEWTAPING,
}NSTVIPPVPriceTypeEnum;

typedef struct _NSTVIPPVPriceUnitClass{
   unsigned short price; 
   NSTVIPPVPriceTypeEnum  price_code;//0x0:不回传，不录像类型; 0x1:不回传，可录像类型
}NSTVIPPVPriceUnitClass;

#define NSTV_IPPV_PRICE_MAX      2
typedef struct _NSTVIPPVPriceClass{
    NSTVIPPVPriceUnitClass  unit_info[NSTV_IPPV_PRICE_MAX];//max from nstv doc
    unsigned char unit_count;
}NSTVIPPVPriceClass;

typedef struct _CASCAMNSTVIPPVBuyClass{
    unsigned short price;
    NSTVIPPVPriceTypeEnum  price_code;//0x0: 不回传，不录像类型 ; 0x1:不回传，可录像类型
    unsigned short ecm_pid;// from ippv notify message
    char pin[6];
    NSTVIPPVTypeEnum ippt_flag;// 0:ippv; 1:ippt
}CASCAMNSTVIPPVBuyClass;

typedef struct _CASCAMNSTVIPPVNotifyClass{
    unsigned int product_id;
    unsigned int operator_id;
    unsigned char wallet_id;
    unsigned short ecm_id;
    char *message;
    union {
      unsigned short   expired_date;/*节目过期时间,IPPV用*///, real data  = mjd_to_ymd((ymd_to_mjd(2000/01/01) + expired_date))
      unsigned short   interval_min;/*时间间隔，单位分钟,IPPT用*/
    }time;
    NSTVIPPVPriceClass buy_price;
    NSTVIPPVTypeEnum ippt_flag;// 0:ippv; 1:ippt
    char* (*exit_func)(char buy, CASCAMNSTVIPPVBuyClass* param);
}CASCAMNSTVIPPVNotifyClass;

// ippv return struct
typedef struct _CASCAMNSTVIPPVBuyReturnClass{
    char *notice;
    char status;// 0: ok; 1: not ok
}CASCAMNSTVIPPVBuyReturnClass;

// DETITLE_INFO: sub id
typedef enum _NSTVDetitleInfoEnum{
    NSTV_OPERATOR_DETITLE_COUNT,
    NSTV_DETITLE_DATA,
    // for edit
    NSTV_DETITLE_DEL,
}NSTVDetitleEnum;

typedef struct _CASCAMNSTVDetitleUnitClass{
    unsigned int detitle_code;// ex.
    int          read_flag;// 1: read; 0: unread
}CASCAMNSTVDetitleUnitClass;

typedef struct _CASCAMNSTVDetitleInfoClass{
    CASCAMNSTVDetitleUnitClass *info_unit;
    unsigned short info_count; // the max is 5, from novel super tv doc
    unsigned int operator_id;
}CASCAMNSTVDetitleInfoClass;

typedef struct _CASCAMNSTVOperatorDetitleClass{
    CASCAMNSTVDetitleInfoClass *operator_detitle;
    unsigned short operator_count;
}CASCAMNSTVOperatorDetitleClass;

typedef struct _CASCAMNSTVDetitleDeleteClass{
    unsigned int detitle_code;// (check code). if the value is 0, means delete all
    int          operator_id;//
}CASCAMNSTVDetitleDeleteClass;

// message notify
typedef enum _CASCAMNSTVDetitleNoticeTypeEnum{
    NSTV_DETITLE_RECEIVE_NEW,
    NSTV_DETITLE_SPACE_FULL,
}CASCAMNSTVDetitleNoticeTypeEnum;


// FEATURES_INFO: sub id
typedef enum _NSTVFeaturesInfoEnum{
    NSTV_OPERATOR_FEATURES_COUNT,
    NSTV_FEATURES_DATA,
}NSTVFeaturesEnum;

typedef struct _CASCAMNSTVFeaturesUnitClass{
    unsigned int area_code;//
    unsigned int bouquet_id;//
    unsigned int features_id;//
    unsigned int features_data;//
}CASCAMNSTVFeaturesUnitClass;

typedef struct _CASCAMNSTVFeaturesInfoClass{
    CASCAMNSTVFeaturesUnitClass *info_unit;
    unsigned short info_count; // the max is 18, from novel super tv doc, aclist
    unsigned int operator_id;
}CASCAMNSTVFeaturesInfoClass;

typedef struct _CASCAMNSTVOperatorFeaturesClass{
    CASCAMNSTVFeaturesInfoClass *operator_features;
    unsigned short operator_count;
}CASCAMNSTVOperatorFeaturesClass;

// OPERATOR_INFO: sub id
typedef enum _NSTVOperatorEnum{
    NSTV_OPERATOR_INFO_COUNT,
    NSTV_OPERATOR_DATA,
}NSTVOperatorEnum;

typedef struct _CASCAMNSTVOperatorUnitClass{
    char operator_des[34];// "CDCA_MAXLEN_TVSPRIINFO + 1 + 1", from novel super tv doc
    char operator_id[6];// ex. 00001, max num is 65536
}CASCAMNSTVOperatorUnitClass;

typedef struct _CASCAMNSTVOperatorInfoClass{
    CASCAMNSTVOperatorUnitClass *operator_info;
    unsigned short operator_count;
}CASCAMNSTVOperatorInfoClass;

// PARENT_CHILD_INFO:
typedef enum _NSTVCardParentChildEnum{
    NSTV_CARD_PARENT_CHILD_INFO_GET,
	NSTV_CARD_PARENT_CHILD_DATA_GET,
	NSTV_CARD_PARENT_CHILD_FEED,
}NSTVCardParentChildEnum;

typedef struct _CASCAMNSTVParentChildInfoClass{
	unsigned short wTvsID;
	unsigned char pbyChild;
	unsigned short pbyDelayTime;
	unsigned long pLastFeedTime;
	char pParentCardSN[16 + 1];
	unsigned char pbIsCanFeed;
}CASCAMNSTVParentChildInfoClass;


// MESSAGE TYPE
typedef enum _CASCAMNSTVMessageMainIdEnum{
    NSTV_MESSAGE_ENTITLE,
    NSTV_MESSAGE_DETITLE,
    NSTV_MESSAGE_CURTAIN,
    NSTV_MESSAGE_ACTION,
    NSTV_MESSAGE_LOCK_SERVICE,
    NSTV_MESSAGE_IPPV,
    NSTV_MESSAGE_CARD_UPDATE,
}CASCAMNSTVMessageMainIdEnum;

// LOCK SERVICE/ UNLOCK
typedef enum _CASCAMNSTVLockServiceTypeEnum{
    LOCK_SERVICE_UNFORCE,// can switch channel, and ...
    LOCK_SERVICE_FORCE,// can not support remote and panel.
}CASCAMNSTVLockServiceTypeEnum;

typedef struct _CASCAMNSTVLockServiceClass{
    unsigned int    fre;
    unsigned int    sym;
    unsigned int    polar;
    unsigned short  pcr_pid;
    unsigned short  video_pid;
    unsigned short  audio_pid;
    unsigned int    video_type;
    unsigned int    audio_type;
    unsigned int    scramble_flag;  //1: scrambled; 0:free
    unsigned short  video_ecm_pid;
    unsigned short  audio_ecm_pid;
    CASCAMNSTVLockServiceTypeEnum    force_flag;// 1: force, can not switch channel
}CASCAMNSTVLockServiceClass;

// ACTION REQUEST
typedef enum _CASCAMNSTVActionTypeEnum{
    NSTV_ACTION_REBOOT,/*重启机顶盒*/
    NSTV_ACTION_FREEZE,/*冻结机顶盒*/
    NSTV_ACTION_RESEARCH,/*重新搜索节目*/
    NSTV_ACTION_UPGRADE,/*机顶盒程序升级*/
    NSTV_ACTION_UNFREEZE,/*解除冻结机顶盒*/
    NSTV_ACTION_INIT_STB,/*初始化机顶盒*/
    NSTV_ACTION_DISPLAY_SYSTEM_INFO,/*显示系统信息*/
    NSTV_ACTION_DISABLE_EPG_AD,/*禁用EPG广告信息功能*/
    NSTV_ACTION_ENABLE_EPG_AD,/*恢复EPG广告信息功能*/
    NSTV_ACTION_FINISH_CARD_IN,/*插卡处理完成*/
}CASCAMNSTVActionTypeEnum;

// WATCH LIMIT

// CURTAIN
typedef enum _CASCAMNSTVCurtainTypeEnum{
    NSTV_CURTAIN_CANCEL = 0,/*取消高级预览显示*/
    NSTV_CURTAIN_OK,/*高级预览节目正常解密*/
    NSTV_CURTAIN_TOTTIME_ERROR,/*高级预览节目禁止解密：已经达到总观看时长*/
    NSTV_CURTAIN_WATCHTIME_ERROR,/*高级预览节目禁止解密：已经达到WatchTime限制*/
    NSTV_CURTAIN_TOTCNT_ERROR,/*高级预览节目禁止解密节目禁止解密：已经达到总允许观看次数*/
    NSTV_CURTAIN_ROOM_ERROR,/*高级预览节目禁止解密：高级预览节目记录空间不足*/
    NSTV_CURTAIN_PARAM_ERROR,/*高级预览节目禁止解密：节目参数错误*/
    NSTV_CURTAIN_TIME_ERROR,/*高级预览节目禁止解密节目禁止解密节目禁止解密：数据错误*/
}CASCAMNSTVCurtainTypeEnum;

//
// OSD NOTIFY
#define NSTV_OSD_OPERATOR_ERR           "Operator ID error" /*卡中不存在节目运营商*/
#define NSTV_OSD_OUT_TIME               "Out of Working Hours" /*当前时段被设定为不能观看*/
#define NSTV_OSD_OUT_RATING             "Out of Teleview Rating" /*看节目级别高于设定的观看级别*/
#define NSTV_OSD_NOT_PAIRED             "The card is not paired with this STB" /*级别智能卡与本机顶盒不对应*/
#define NSTV_OSD_NO_ENTITLE             "No Entitlement" /*没有授权*/
#define NSTV_OSD_CHANGE_PIN             "PIN has been changed" /*PIN码修改成功*/
#define NSTV_OSD_INVALID_PIN            "Invalid PIN" /*PIN码无效*/
#define NSTV_OSD_CHANGE_RATING          "Teleview rating changed successfully" /*观看级别修改成功*/
#define NSTV_OSD_CHANGE_TIME            "Working Hours changed successfully" /*设置工作时段成功*/
#define NSTV_OSD_INVALID_TIME           "Invalid working hours" /*设定的工作时段无效*/
#define NSTV_OSD_DELETE_MAIL            "Email deleted successfully" /*邮件删除成功*/
#define NSTV_OSD_DELETE_MAIL_ALL        "All of the emails deleted successfully"/*所有邮件删除成功*/
#define NSTV_OSD_PURCHASED              "IPPV product purchased successfully" /*购买成功*/
#define NSTV_OSD_PURCHASED_IPPT         "IPPT product purchased successfully" /*购买成功*/
#define NSTV_OSD_NO_ROOM                "No room for new IPPV" /*卡空间不足*/
#define NSTV_OSD_NO_ROOM_IPPT           "No room for new IPPT" /*卡空间不足*/
#define NSTV_OSD_NO_MONEY               "No enough money" /*卡内金额不足*/
#define NSTV_OSD_REGIONAL_OUT           "Regional Lockout" /*区域不正确*/
#define NSTV_OSD_VERIFICATION_FAILED    "Card verification failed. Please contact the operator" /*智能卡校验失败,请联系运营商*/
#define NSTV_OSD_PATCHING               "Card in patching. Not to remove the card or power off" /*智能卡升级中,请不要拔卡或者关机*/
#define NSTV_OSD_UPGRADE                "Please upgrade the smart card" /*请升级智能卡*/
#define NSTV_OSD_SWITCH_CHANNEL_NOTICE  "Please do not switch channels too frequently" /*请勿频繁切换频道*/
#define NSTV_OSD_ACCOUNT_LOCKED         "Your account is locked, please contact your operator" /*账户被锁定,请联系运营商*/
#define NSTV_OSD_SEND_RECORDS           "Please send the IPPV records back to the operator" /*智能卡已暂停,请回传收视记录给运营商*/
#define NSTV_OSD_RESTART                "Please restart the STB" /*请重启机顶盒*/

#define NSTV_OSD_CANCEL                 " "  /* 取消当前的显示 */
#define NSTV_OSD_BADCARD                "Unidentified Card"  /* 无法识别卡 */
#define NSTV_OSD_EXPICARD               "Expired Card"  /* 智能卡过期，请更换新卡 */
#define NSTV_OSD_INSERTCARD             "Please insert the smart card"  /* 加扰节目，请插入智能卡 */
#define NSTV_OSD_NOOPER                 NSTV_OSD_OPERATOR_ERR  /* 不支持节目运营商 */
#define NSTV_OSD_BLACKOUT               "Conditional Blackout" /*条件禁播*/
#define NSTV_OSD_OUTWORKTIME            NSTV_OSD_OUT_TIME  /* 当前时段被设定为不能观看 */
#define NSTV_OSD_WATCHLEVEL             NSTV_OSD_OUT_RATING  /* 节目级别高于设定的观看级别 */
#define NSTV_OSD_PAIRING                NSTV_OSD_NOT_PAIRED  /* 智能卡与本机顶盒不对应 */
#define NSTV_OSD_NOENTITLE              NSTV_OSD_NO_ENTITLE  /* 没有授权 */
#define NSTV_OSD_DECRYPTFAIL            "Decryption Failure"  /* 节目解密失败 */
#define NSTV_OSD_NOMONEY                NSTV_OSD_NO_MONEY  /* 卡内金额不足 */
#define NSTV_OSD_ERRREGION              NSTV_OSD_REGIONAL_OUT  /* 区域不正确 */
#define NSTV_OSD_NEEDFEED               "Please insert the parent-card of the current card"
#define NSTV_OSD_ERRCARD                NSTV_OSD_VERIFICATION_FAILED  /* 智能卡校验失败，请联系运营商 */
#define NSTV_OSD_UPDATE                 NSTV_OSD_PATCHING  /* 智能卡升级中，请不要拔卡或者关机 */
#define NSTV_OSD_LOWCARDVER             NSTV_OSD_UPGRADE  /* 请升级智能卡 */
#define NSTV_OSD_VIEWLOCK               NSTV_OSD_SWITCH_CHANNEL_NOTICE  /* 请勿频繁切换频道 */
#define NSTV_OSD_MAXRESTART             "Card is dormancy. Please insert the card after 5 minutes"
#define NSTV_OSD_FREEZE                 "Your account is locked, please contact your operator" /*账户被锁定,请联系运营商*/ //"Card is locked. Please contact the operator"  /* 智能卡已冻结，请联系运营商 */
#define NSTV_OSD_CALLBACK               NSTV_OSD_SEND_RECORDS  /* 智能卡已暂停，请回传收视记录给运营商 */
#define NSTV_OSD_CURTAIN	            "Advanced preview program, can not be free to watch" /*高级预览节目，该阶段不能免费观看*/
#define NSTV_OSD_CARDTESTSTART          "Upgrade test card testing..." /*升级测试卡测试中...*/
#define NSTV_OSD_CARDTESTFAILD          "Upgrade test card test failed, please check the machine card communication module" /*升级测试卡测试失败，请检查机卡通讯模块*/
#define NSTV_OSD_CARDTESTSUCC           "Upgrade test card test success" /*升级测试卡测试成功*/
#define NSTV_OSD_NOCALIBOPER            "There is no custom operator in the card."/*卡中不存在移植库定制运营商*/
#define NSTV_OSD_STBLOCKED              NSTV_OSD_RESTART  /* 请重启机顶盒 */
#define NSTV_OSD_STBFREEZE              "STB is freezed. Please contact the operator"  /* 机顶盒被冻结 */
#define NSTV_OSD_UNSUPPORTDEVICE        "Unsupported terminal type (number: 0xXXXX)" /*不支持的终端类型（编号：0xXXXX）*/

//
#define NSTV_OSD_UPGRADE_STB            "Please upgrade the STB!"
#define NSTV_OSD_IPPV_STAGE1            "IPPV free preview stage, buy it?" /*IPPV免费预览阶段，是否购买*/
#define NSTV_OSD_IPPV_STAGE2            "IPPV charging stage,buy it?"/*IPPV收费阶段，是否购买*/
#define NSTV_OSD_IPPT_NOTIFY            "IPPT charging stage, buy it?"/*IPPT收费段，是否购买*/
#define NSTV_OSD_EXPIRED_TIME           "Expired Time"
#define NSTV_OSD_INTERVAL_TIME          "Interval Time"
#define NSTV_OSD_MINUTE                 "min"
#define NSTV_OSD_WALLET_ID              "Wallet ID"
#define NSTV_OSD_IPPV_PRICE_TYPE1       "The price without PVR"
#define NSTV_OSD_IPPV_PRICE_TYPE2       "The price with PVR"
#define NSTV_OSD_IPPV_BUY_TYPE          "Buy Type:"
#define NSTV_OSD_IPPV_POINT             "points"
// no signal, no entitlement, no paired
#define NSTV_OPERATOR_NOTICE            "Please contact us. Hot Line: 01-9010220, 01-9010221, 01-9010222, 01-9010223, 01-9010224"
#define NSTV_NO_SIGNAL                  "No Signal!\nPlease contact us. Hot Line: 01-9010220, 01-9010221, 01-9010222, 01-9010223, 01-9010224."
#define NSTV_SCRAMBLE                   "Scrambled! Please contact us. Hot Line: 01-9010220, 01-9010221, 01-9010222, 01-9010223, 01-9010224."
// card update
#define NSTV_OSD_RECEIVE_DATA           "Receiving data"
#define NSTV_OSD_UPDATE_SUCCESS         "Card Update Success"
#define NSTV_OSD_UPDATE_FAIL            "Card Update Failed"

#define NSTV_OSD_INSERT_CHILD           "please insert the child-card"
#define NSTV_OSD_INSERT_PARENT          "please insert the parent-card"
#define NSTV_OSD_FEED_SUCCESS           "Feeding successfully"
#define NSTV_OSD_BAD_PARENT             "Invalid parent-card"
#define NSTV_OSD_FEED_FAIL              "Feeding failed"
#define NSTV_OSD_FEED_NO_TIME           "It is no time to feeding"
#define NSTV_OSD_FEED_NO_CARD           "no card find, please insert the card"
#define NSTV_OSD_PARENT                 "parent card"
#define NSTV_OSD_CHILD                  "child card"
#define NSTV_OSD_F1_FEED                "Press F1 will feed card"
#define NSTV_OSD_FEED_NO                "Can not feed"
#define NSTV_OSD_FEED_YES               "Can feed"
#define NSTV_OSD_PARENT_SN              "parent card sn"
#define NSTV_OSD_FEED_PEAR              "feed pear"
#define NSTV_OSD_LAST_FEED_TIME         "last feed time"
#define NSTV_OSD_FEED_UNKNOWN_CARD      "unknown card"
#define NSTV_OSD_FEED_HOURS             "hours"

// OSD MENU
#define NSTV_BASIC_INFO                 "Basic Information"/* CA基本信息*/
#define NSTV_CARD_SN                    "Card SN:"
#define NSTV_STB_ID                     "STB ID:"
#define NSTV_STB_MATCH_STATUS           "STB Match Status:"
#define NSTV_CAS_VERSION                "CAS Version:"
#define NSTV_WORK_TIME                  "Work Time:"
#define NSTV_WATCH_RATING               "Watch Level:"
#define NSTV_OPERATOR_ID                "Operator ID:"
#define NSTV_MARKET_ID                  "Market ID:"
#define NSTV_JTAG_STATUS                "JTAG Debug Status:"
#define NSTV_SECURITY_START_STATUS      "Security Start Status:"
#define NSTV_DETITLE                    "Detitle Information"
#define NSTV_READ                       "Read"
#define NSTV_UNREAD                     "Unread"
#define NSTV_ENTITLEMENT_INFO           "Entitlement Information"
#define NSTV_ALLOW                      "Allow"
#define NSTV_FORBIDDEN                  "Forbidden"
#define NSTV_FEATURES_INFO              "Features Information"
#define NSTV_IPPV_INFO                  "IPPV Information"
#define NSTV_UNKNOW                     "Unknow"
#define NSTV_BOOKING                    "Booking"
#define NSTV_VIEWED                     "Viewed"
#define NSTV_EXPIRED                    "Expired"
#define NSTV_EMAIL_INFO                 "Email List"/*邮件列表*/
#define NSTV_NUMBER			            "No."/*序号*/
#define NSTV_SEND_TIME                  "Send Time"/*发送时间*/
#define NSTV_SUBJECT			        "Subject"/*主题*/
#define NSTV_DELETE_ALL_MAILS		    "Delete All Mails"/*删除所有*/
#define NSTV_PRODUCT_ID                 "Product ID"/*产品ID*/
#define NSTV_END_TIME                   "End Time"/*过期时间*/
#define NSTV_TOTAL                      "Total"
#define NSTV_IMPORTANT                  "Important"
#define NSTV_NORMAL                     "Normal"
#define NSTV_IMPORTANCE                 "Importance"
#define NSTV_OPERATOR_TITLE             "Operator Information"
#define NSTV_ENTITLEMENT                "Entitlement Information"
#define NSTV_WALLET                     "Wallet Information"
#define NSTV_IPPV                       "IPPV Information"
#define NSTV_FEATURES                   "Features Information"
#define NSTV_CARD_UPDATE                "Card Update Information"
#define NSTV_CARD_PARENT_CHILD          "Parent Child Card"
#define NSTV_CARD_PAIR_INFO             "Card Pair Information"

