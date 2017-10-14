/**
 *
 * @file        cascam_wanfa.h
 * @brief
 * @version     1.1.0
 * @date        09/13/2015 09:47:49 AM
 * @author      B.Z., 88156088@nationalchip.com
 *
 */
// BASIC_INFO: sub id
typedef enum  _WFBasicInfoEnum{
    WF_BASIC_ALL,
    WF_BASIC_CARD_NUM,
    WF_BASIC_IS_MOTHER_CARD,
    WF_BASIC_IPPV,
    WF_BASIC_STB_NUM,
    WF_BASIC_CAS_ID,
    WF_BASIC_CAS_VER,
    WF_BASIC_CAS_PROVIDER,
    WF_BASIC_CAS_DES,
    WF_BASIC_CARD_VAILD,
    WF_BASIC_CAS_TIME,
    WF_BASIC_STB_ID,
    WF_BASIC_STB_PAIR,
    WF_BASIC_CHIP_ID,
    WF_BASIC_SCPU_ID,
}WFBasicInfoEnum;

typedef struct _CASCAMWFBasicInfoClass{
    char *card_num;
    unsigned char is_mother_card;
    char *ippv_info;
    char *stb_num;
    char *CAS_ID;
    unsigned char CAS_version;
    char *CAS_provider;
    char *CAS_description;
    char *card_vaild_info;
    char *CAS_current_time;
    char *stb_id;
    char *stb_match_num;
    char *chip_id;
    char *scpu_id;
}CASCAMWFBasicInfoClass;

// advanced security
typedef struct _CASCAMWFAdvancedSecurityInfoClass{
    unsigned char *chip_id;
    unsigned char *scpu_id;
}CASCAMWFAdvancedSecurityInfoClass;

// cas info
typedef struct _CASCAMWFCASInfoClass{
    unsigned short CAS_ID;
    unsigned short CAS_SUB_ID;
    unsigned int   CAS_version;
    unsigned char  *description;
}CASCAMWFCASInfoClass;

// provider info
typedef struct _CASCAMWFProviderInfoClass{
    unsigned char *provider_id;
    unsigned char *provider_name;
    int            balance;
}CASCAMWFProviderInfoClass;

// purse info
typedef struct _CASCAMWFPurseInfoClass{
    unsigned int balance;
    unsigned int used;
}CASCAMWFPurseInfoClass;

// watch time
typedef struct _CASCAMWFWatcTimeClass{
    unsigned char *watch_time;
    unsigned char *current_time;
}CASCAMWFWatchTimeClass;

// ENTITLEMENT_INFO:sub id
typedef enum _WFEntitlementEnum{
    WF_ENTITLEMENT_COUNT,
    WF_ENTITLEMENT_DATA,
}WFEntitlementEnum;

typedef struct _CASCAMWFEntitlementUnitClass{
    char product_id[9];
    char start_time[20];
    char end_time[20];
}CASCAMWFEntitlementUnitClass;

typedef struct _CASCAMWFEntitlementInfoClass{
    CASCAMWFEntitlementUnitClass *info_unit;
    unsigned int info_count; // the max is 350, from wanfa cas
}CASCAMWFEntitlementInfoClass;


// maturity control
typedef enum _WFMaturityEnum{
    WF_MATURITY_SET,
}WFMaturityEnum;

typedef struct _CASCAMWFMaturityInfoClass{
    unsigned char maturity_level;
    unsigned short validity_minites;
}CASCAMWFMaturityInfoClass;

// EMAIL_INFO:sub id
typedef enum _WFEmailEnum{
    // GET
    WF_EMAIL_GET_COUNT,// the max is more than 50, 
    WF_EMAIL_GET_DATA_HEAD,
    WF_EMAIL_GET_DATA_BODY,
    // SET
    WF_EMAIL_DEL_ALL,
    WF_EMAIL_DEL_BY_POS,
    WF_EMAIL_SORT,
    WF_EMAIL_MASK_READ_BY_POS,
    WF_EMAIL_MASK_UNREAD_BY_POS,
}WFEmailEnum;

#define EMAIL_BODY_SIZE 1024*3 
#define EMAIL_TITLE_SIZE 64
typedef struct _CASCAMWFEmailBodyClass{
    char body[EMAIL_BODY_SIZE];
    unsigned short pos;
}CASCAMWFEmailBodyClass;

typedef struct _CASCAMWFEmailHeadClass{
    unsigned char read_status;// 0:read; 1:unread
    char          create_time[20];//2015-09-18 13:20:09
    char          title[EMAIL_TITLE_SIZE];
}CASCAMWFEmailHeadClass;

typedef struct _CASCAMWFEmailInfoClass{
    unsigned short mail_total;
    unsigned short mail_unread_count;
    CASCAMWFEmailHeadClass *email_head;
}CASCAMWFEmailInfoClass;

// OSD control
// define
/* Constant for Error code */
#define	OSD_WFERR_PARAMETER	                "Parameter error"/* 函数的参数非法 */
#define	OSD_WFERR_MEMALLOC	                "MemAlloc failure"/* 分配存储器失败 */
#define	OSD_WFERR_MEMFREE	                "MemFree failure"/* 释放存储器失败 */
#define	OSD_WFERR_ILLEAGAL	                "Illeagal operating"/* 非法的操作 */
#define	OSD_WFERR_VERIFY		            "Check error"/* 校验失败 */
#define	OSD_WFERR_MEMEXEC	                "Memory error"/* 内存异常 */

#define	OSD_WFERR_SW1SW2		            "SMC SW1SW2 error"/* 执行命令时状态字不等于0x9000,低16位为状态字SW1SW2 */
#define	OSD_WFERR_RWSIZE		            "Too little response bytes"/* 实际响应字节数比要求的少 */
#define	OSD_WFERR_SC_ABSENT		            "No smartcard"/* 没有插入智能卡 */
#define	OSD_WFERR_INVALID		            "Invalid smartcard"/* 非法的智能卡 */
#define	OSD_WFERR_DRIVER		            "Card driver failure"/* 智能卡驱动程序错误从这里开始编号，且小于WFERR_FILE_FORMAT */

#define	OSD_WFERR_FORMAT		            "File format error"/* 文件格式错误 */
#define	OSD_WFERR_CONFLICT		            "Insert failure"/* 插入冲突 */
#define	OSD_WFERR_FULL			            "File full"/* 文件已满 */
#define	OSD_WFERR_EMPTY		                "File empty"/* 文件已空 */
#define	OSD_WFERR_QUERY		                "No records"/* 未能查询到记录 */
#define	OSD_WFERR_ABNORMAL		            "File content exception"/* 文件内容异常 */

#define	OSD_WFERR_ABSENT		            "CAS not start"/* CAS尚未初始化 */
#define	OSD_WFERR_PRESENT		            "CAS has start"/* CAS已经初始化 */
#define	OSD_WFERR_INVALIDSTB	            "Card not match"/* 非法的机顶盒,或者机卡不匹配 */
#define	OSD_WFERR_UNLICENSED	            "Not authored"/* 节目未经授权 */
#define	OSD_WFERR_OUTOFPERIOD	            "Expired"/* 节目不在授权的期间内 */
#define	OSD_WFERR_FORBIDDEN		            "DSR disabled"/* 禁止解扰 */
#define	OSD_WFERR_COMMAND		            "Invalid command"/* 非法的前端命令 */
#define	OSD_WFERR_LENGTH		            "Parameter length error"/* 命令的参数长度不正确 */
#define	OSD_WFERR_ADDRESS		            "ProviderID error"/* 前端寻址没有寻到本机 */
#define	OSD_WFERR_BALANCE		            "Remain not enough"/* 余额不足 */
//#define	OSD_WFERR_UNMATCHED		            "Parent card"/* 子机与母机不匹配 */
//#define	OSD_WFERR_EXPIRATION	            "Child ard expired"/* 过期的子机授权 */
#define	OSD_WFERR_ZONE			            "Error zone"/* 错误的区域 */
//
//normal strings
#define OSD_WARNING                         "Warning!"/* 警告！*/
#define OSD_NOTICE    			    "Notice"/*提示*/
#define	OSD_REMAIN_DAY			            "Remaining authorized days:"/* 节目剩余授权天数： */
#define	OSD_IPPT_TITLE			            "This is IPPT program:"/* 本节目为IPPT节目： */
#define	OSD_IPPV_TITLE			            "This is IPPV program:"/* 本节目为IPPV节目： */
#define OSD_VALUTION                        "Valuation unit:"/* 计价单位：*/
#define OSD_SEC_TIME                        "seconds/times"/* 秒/次*/
#define OSD_HOUR_TIME                       "hour/times"/* 小时/次*/
#define OSD_PRICE                           "Price:"/*价格：*/
#define OSD_DO_YOU_WANT_TO_WATCH            "Do you want to watch it?"/*确认收看吗？*/
#define OSD_YES                             "Yes"/* 是*/
#define OSD_NO                              "No"/* 否*/
#define OSD_OK                              "OK"/* 确认*/
#define OSD_DAY                             "Day"/* 天*/
#define OSD_DAYS                            "Days"/* 天*/
#define OSD_MATURITY                        "This is maturity level program:"/* 本节目受父母节目控制：*/
#define OSD_ENTER_PASSWORD                  "Please enter the password:"/* 请输入密码：*/
#define OSD_CARD_NUM                        "Card Number:"/* 卡号：*/
#define OSD_TOKENS                          "IPPV Consumption/Total:"/* IPPV 已消费/总额：*/
#define OSD_STB_SN                          "STB SN:"/*机顶盒序列号：*/
#define OSD_CAS_ID                          "CAS ID:"/*CA系统的标识：*/
#define OSD_CAS_PROVIDER                    "CAS Provider:"/*CA系统提供商：*/
#define OSD_CAS_DESCRIPTION                 "CAS Description:"/*CA系统描述：*/
#define OSD_VAILD_TIME                      "Card Available Time:"/*智能卡有效期限：*/
#define OSD_CURRENT_TIME                    "CAS Current Time:"/*CAS当前时间：*/
#define OSD_STB_NUMBER                      "STB Number:"/*机顶盒号：*/
#define OSD_STB_MATCH_NUMBER                "Matched STB Number:"/*匹配机顶盒号：*/
#define OSD_CHIP_ID                         "Chip ID:"/*主CPU标识：*/
#define OSD_SCPU_ID                         "SCPU ID:"/*高安标识：*/
#define OSD_MOTHER_CARD                     "Mother Card"/* 母卡*/
#define OSD_CHILD_CARD                      "Child Card"/* 子卡*/
#define OSD_CAS_VERSION                     "CAS Version:"/* CA系统版本：*/
#define OSD_BASIC_INFO                      "Basic Information"/* CA基本信息*/
#define OSD_ENTITLEMENT_HISTORY             "Entitlement History"/*CA授权记录*/
#define OSD_EMAIL_INFO                      "Email List"/*邮件列表*/
#define OSD_NUMBER 			    "No."/*序号*/
#define OSD_SEND_TIME                       "Send Time"/*发送时间*/
#define OSD_SUBJECT 			    "Subject"/*主题*/
#define OSD_DELETE_ALL_MAILS 		    "Delete All Mails"/*删除所有*/
#define OSD_MATURITY_CONTROL                "Maturity Setting"/*父母锁控制*/
#define OSD_PRODUCT_ID                      "Product ID"/*产品ID*/
#define OSD_END_TIME                        "End Time"/*过期时间*/
#define OSD_READ                            "Read"/*阅读*/
#define OSD_UNREAD                          "Unread"/*未读*/
#define OSD_TOTAL                           "Total"/*总数*/
#define OSD_PASSWORD_VALIDITY               "Password Validity (minite)"/*密码有效期（分钟）*/
#define OSD_MATURITY_LEVEL                  "Maturity Level"/*观看级别*/
//

// pin control
typedef struct _CASCAMWFCASPINInfoClass{
    unsigned char card_password[8];
    unsigned char password_len;
}CASCAMWFCASPINInfoClass;



