/**
 *
 * @file        cascam_types.h
 * @brief
 * @version     1.1.0
 * @date        08/25/2015 09:47:49 AM
 * @author      B.Z., 88156088@nationalchip.com
 *
 */
#ifndef __CASCAM_TYPES_H__
#define __CASCAM_TYPES_H__
#ifdef __cplusplus
extern "C" {
#endif
// TODO:
#ifdef NULL
#undef NULL
#endif
#define NULL (void*)(0)


typedef enum _CASCAMCASGetDataTypeEnum{
    BASIC_INFO = 0,
    ENTITLEMENT_INFO,
    OPERATOR_INFO,
    EMAIL_INFO,
    UPDATE_INFO,
    WALLET_INFO,
    IPPV_INFO,
    DETITLE_INFO,
    FEATURES_INFO,
    PARENTAL_INFO,
    PIN_INFO,
    WORK_TIME_INFO,
    PARENT_CHILD_INFO,
}CASCAMCASGetDataTypeEnum;

typedef enum _CASCAMCASSetDataTypeEnum{
    EMAIL_CONTROL = 0,
    PARENTAL_CONTROL,
    PIN_CONTROL,
    WORK_TIME_CONTROL,
    DETITLE_CONTROL,
    PARENT_CHILD_CONTROL,
}CASCAMCASSetDataTypeEnum;

#if 0
typedef enum _CASCAMCADataTypeEnum{
    CA_BASE_INFO=0,		        /*基本信息*/
	CA_OPERATOR_INFO,		    /*运营商信息*/
	CA_EMAIL_INFO,			    /*邮件*/
	CA_ENTITLE_INFO ,			/*授权*/
	CA_DETITLE_INFO,            /*反授权*/
	CA_EMERGENCY_INFO,          /*应急广播*/
	CA_ROLLING_INFO,            /*OSD滚动消息*/
	CA_FINGER_INFO,             /*指纹*/
	CA_CARD_UPDATE_INFO,        /*卡升级*/
	CA_FETURE_INFO,             /*特征值*/
	CA_IPPV_POP_INFO,           /*IPPV购买框信息*/
	CA_IPPT_POP_INFO,           /*IPPT购买框信息*/
	CA_IPPV_PROG_INFO,          /*IPPV已购买节目列表信息*/
	CA_IPPV_SLOT_INFO,          /*IPPV钱包列表信息*/
	CA_MOTHER_CARD_INFO,        /*母卡信息*/
	CA_CHANGE_PIN_INFO,         /*修改密码信息*/
	CA_PROGRESS_INFO,           /*智能卡升级进度信息*/
	CA_RATING_INFO,             /*成人级别信息*/
	CA_WORK_TIME_INFO,          /*工作时段信息*/
	CA_CURTAIN_INFO,            /*窗帘信息*/
	CA_PAIRED_INFO,             /*配对信息*/
	CA_CONSUME_HISTORY,			/*历史充值/消费记录*/
	/*扩展，CA其他数据类型*/
	CA_OTA,                     /*ota升级*/
	CA_NIT,                     /*NIT virsion change*/
	CA_CONDITION_SEARCH,        /*德赛CA 条件搜索*/
	CA_FORCE_MSG,               /*数码CA强制消息*/
	CA_WATCH_LIMIT_MSG,         /*永新CA连续观看限制通知*/
	CA_CARD_ACTION_REQUEST,     /*永新CA通知*/
	CA_BMP,                     /*AD CA*/
	CA_GIF,                     /*AD CA*/
	CA_SCROLL_BMP,              /*AD CA*/
	CA_MAX_INFO,
	/*extend */
	CA_FORCE_OSD_INFO

}CASCAMCADataTypeEnum;
#endif

typedef struct _CASCAMProgramParaClass{
   unsigned int	    DemuxID;
   unsigned char	ScrambleFlag;
   unsigned short	VideoPid;
   unsigned short	AudioPid;
   unsigned short	ServiceID;
   unsigned short	PMTPid;
   unsigned short	VideoECMPid;
   unsigned short	AudioECMPid;
   unsigned short   CasPid;
   unsigned short   NetworkID;
   unsigned short   StreamID;
}CASCAMProgramParaClass;

typedef struct _CASCAMDemuxFlagClass{
   unsigned int     Flags;
}CASCAMDemuxFlagClass;

enum{
    DEMUX_VIDEO     = 1<<0,
    DEMUX_AUDIO     = 1<<1,
    DEMUX_PMT	    = 1<<2,
    DEMUX_CAT	    = 1<<3,
    DEMUX_ECM	    = 1<<4,
    DEMUX_EMM	    = 1<<5,
    DEMUX_NIT	    = 1<<6,
    DEMUX_BAT       = 1<<7,
    DEMUX_OTA	    = 1<<8,
    DEMUX_PRIVATE   = 1<<9,
    DEMUX_ALL       = 1<<31,
};

// email, common struct
typedef enum _CASCAMEmailSortTypeEnum{
    EMAIL_TIME_NEW_FIRST,
    EMAIL_TIME_OLD_FIRST,
    EMAIL_UNREAD_FIRST,
    EMAIL_LEVEL,
}CASCAMEmailSortTypeEnum;

typedef struct _CASCAMEmailBoxInfoClass{
    unsigned int total_memory;// bytes
    unsigned int used_memory;// bytes
    unsigned int mail_total_num;
    unsigned int mail_unread_num;
    CASCAMEmailSortTypeEnum sort_type;
}CASCAMEmailBoxInfoClass;

#define	CASCAM_EMAIL_AUTHOR     (32)
#define	CASCAM_EMAIL_TITLE      (64)

typedef struct _CASCAMEmailHeadClass{
    char author[CASCAM_EMAIL_AUTHOR];
    char title[CASCAM_EMAIL_TITLE];
    unsigned int mail_level; // the higher the value, the lower the level. 
    // for get data
    unsigned int create_data_ymd;//0x20150918: bcd code, means 2015/09/18
    unsigned int create_data_hms;//0xxx203025: bcd code, means 20:30:25
    unsigned int mail_state;// 0: read; 1:unread
    unsigned int mail_id;
}CASCAMEmailHeadClass;

#define	CASCAM_EMAIL_CONTEXT     (1024*3)			/*邮件正文长度  */
#define	CASCAM_EMAIL_CC          (128)			/*邮件抄送人  */
#define	CASCAM_EMAIL_ADDRESSEE   (128)			/*邮件收件人列表 */
#define	CASCAM_EMAIL_SING        (128)			/*邮件签名档  */
typedef struct _CASCAMEmailBodyClass{
    char context[CASCAM_EMAIL_CONTEXT];
    char cc[CASCAM_EMAIL_CC];
    char addressee[CASCAM_EMAIL_ADDRESSEE];
    char singnature[CASCAM_EMAIL_SING];
}CASCAMEmailBodyClass;

typedef struct _CASCAMEmailClass{
    CASCAMEmailHeadClass mail_head;
    CASCAMEmailBodyClass mail_body;
}CASCAMEmailClass;


// for osd exec
typedef enum _CASCAMOsdExecTypeEnum{
    CASCAM_OSD_DRAW = 0,
    CASCAM_OSD_TRANSFER_STR,
    CASCAM_OSD_GET_ROLLING_TIMES,
    CASCAM_OSD_EMAIL_FULL,
    CASCAM_OSD_EMAIL_NEW,
}CASCAMOsdExecTypeEnum;

// function type
//
typedef int (*CASCAMInitFunc)(void);

typedef int (*CASCAMCloseFunc)(void);

//
typedef int (*CASCAMUpdateFunc)(void);

// smartcard
typedef int (*CASCAMCardInFunc)(char*, unsigned char);

typedef int (*CASCAMCardOutFunc)(void);

// change program
typedef int (*CASCAMChangeProgramFunc)(CASCAMProgramParaClass* pParams);
// change frequency for cat, nit and ...
typedef int (*CASCAMChangeFreFunc)(void);
// now for gxcas ...
typedef int (*CASCAMSetZoneFunc)(double);

// release demux source
typedef int (*CASCAMReleaseDemuxFunc)(CASCAMDemuxFlagClass *pFlags);
// start
typedef int (*CASCAMStartDemuxFunc)(CASCAMDemuxFlagClass *pFlags, unsigned short);

//
typedef int (*CASCAMIsVaildCardFunc)(char *atr);
//
typedef int (*CASCAMGetCASIDFunc)(void);
//
typedef int (*CASCAMIsVaildCASFunc)(unsigned short CASID);
// data
typedef int (*CASCAMGetPropertyFunc)(CASCAMCASGetDataTypeEnum id, int sub_id, void *Info, unsigned int len);
typedef int (*CASCAMSetPropertyFunc)(CASCAMCASSetDataTypeEnum id, int sub_id, void *Info, unsigned int len);

//
typedef struct _CASCAMDemuxClass{
    int DemuxID;//need to change by solution
	int DoBySelf;//set demux filter by CAS LIB
	CASCAMReleaseDemuxFunc      release_demux;
    CASCAMStartDemuxFunc        start_demux;
}CASCAMDemuxClass;

// CASCAM PLATFORM
typedef struct _CASCAMControlBlockClass{
    char                        *CASName;
    char                        is_advanced_security; // 1: is; 0: not
	CASCAMInitFunc              init;
	CASCAMCloseFunc             close;
    CASCAMChangeProgramFunc     change_program;
	CASCAMChangeFreFunc         change_frequency;
    CASCAMSetZoneFunc           set_zone;
    CASCAMGetCASIDFunc	        get_cas_id;
	CASCAMIsVaildCASFunc	    is_vaild_cas;
    CASCAMGetPropertyFunc	    get_property;
    CASCAMSetPropertyFunc	    set_property;
    CASCAMDemuxClass            demux_class;
}CASCAMControlBlockClass;

typedef struct _CASCAMCASTableClass{
    CASCAMControlBlockClass     CASBlock_class;

	struct _CASCAMCASTableClass *pPreview;
    struct _CASCAMCASTableClass *pNext;
}CASCAMCASTableClass;

typedef struct _CASCAMPlatformClass{
    int CASNumIndex;
    CASCAMControlBlockClass CASCAMBlock_class;
}CASCAMPlatformClass;

#ifdef __cplusplus
}
#endif
#endif
