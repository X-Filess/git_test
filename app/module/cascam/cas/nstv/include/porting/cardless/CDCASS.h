#ifndef CDCAS_CALIB_APIEX_H
#define CDCAS_CALIB_APIEX_H

#ifdef  __cplusplus
extern "C" {
#endif


/*-------------------------------------基本数据类型---------------------------------------*/
typedef unsigned  long  CDCA_U32;
typedef unsigned  short CDCA_U16;
typedef unsigned  char  CDCA_U8;

typedef signed char  CDCA_S8;
typedef signed long  CDCA_S32;

typedef CDCA_U8 CDCA_BOOL;

#define CDCA_TRUE    ((CDCA_BOOL)1)
#define CDCA_FALSE   ((CDCA_BOOL)0)

/*----------------------------------------宏定义--------------------------------------*/

/*--------- FLASH类型 -------*/
#define CDCA_FLASH_BLOCK_A        1     /* BLOCK A */
#define CDCA_FLASH_BLOCK_B        2     /* BLOCK B */
#define CDCA_FLASH_BLOCK_C        3     /* BLOCK C */

/*--------- 相关限制 --------*/
#define CDCA_MAXLEN_STBSN         12U   /* 机顶盒号的长度 */
/*---------- 机顶盒相关限制 ----------*/
#define CDCA_MAXNUM_PROGRAMBYCW   4U    /* 一个控制字最多解的节目数 */

#define CDCA_MAXLEN_PINCODE       6U    /* PIN码的长度 */
#define CDCA_MAXNUM_OPERATOR      4U    /* 最多的运营商个数 */
#define CDCA_MAXLEN_TVSPRIINFO    32U   /* 运营商私有信息的长度 */
#define CDCA_MAXNUM_ACLIST        18U   /* 每个运营商的用户特征个数 */


#define CDCA_MAXNUM_ENTITLE       500U  /* 授权产品的最大个数 */


/*---------- CAS提示信息 ---------*/
#define CDCA_MESSAGE_CANCEL_TYPE      0x00  /* 取消当前的显示 */
#define CDCA_MESSAGE_NOOPER_TYPE      0x04  /* 不支持节目运营商 */
#define CDCA_MESSAGE_WATCHLEVEL_TYPE  0x07  /* 节目级别高于设定的观看级别 */
#define CDCA_MESSAGE_NOENTITLE_TYPE   0x09  /* 没有授权 */
#define CDCA_MESSAGE_DECRYPTFAIL_TYPE 0x0A  /* 节目解密失败 */
#define CDCA_MESSAGE_ERRREGION_TYPE   0x0C  /* 区域不正确 */
#define CDCA_MESSAGE_STBLOCKED_TYPE   0x20  /* 请重启机顶盒 */
#define CDCA_MESSAGE_STBFREEZE_TYPE   0x21  /* 机顶盒被冻结 */


/*---------- 功能调用返回值定义 ----------*/
#define CDCA_RC_OK                    0x00  /* 成功 */
#define CDCA_RC_UNKNOWN               0x01  /* 未知错误 */
#define CDCA_RC_POINTER_INVALID       0x02  /* 指针无效 */
#define CDCA_RC_PIN_INVALID           0x04  /* PIN码无效 */
#define CDCA_RC_DATASPACE_SMALL       0x06  /* 所给的空间不足 */
#define CDCA_RC_DATA_NOT_FIND         0x08  /* 没有找到所要的数据 */
#define CDCA_RC_WATCHRATING_INVALID   0x0E  /* 设定的观看级别无效 */
#define CDCA_RC_UNSUPPORT             0x22  /* 不支持 */
#define CDCA_RC_LEN_ERROR             0x23  /* 长度错误 */
#define CDCA_RC_SCHIP_ERROR           0x24  /* 安全芯片设置错误 */

/*---------- ECM_PID设置的操作类型 ---------*/
#define CDCA_LIST_OK          0x00
#define CDCA_LIST_FIRST       0x01
#define CDCA_LIST_ADD         0x02




/*------------ 邮件大小及数量限制 ------------*/
#define CDCA_MAXNUM_EMAIL         100U  /* 机顶盒保存的最大邮件个数 */
#define CDCA_MAXLEN_EMAIL_TITLE   30U   /* 邮件标题的长度 */
#define CDCA_MAXLEN_EMAIL_CONTENT 160U  /* 邮件内容的长度 */
/*------------ 邮件图标显示方式 ------------*/
#define CDCA_Email_IconHide       0x00  /* 隐藏邮件通知图标 */
#define CDCA_Email_New            0x01  /* 新邮件通知，显示新邮件图标 */
#define CDCA_Email_SpaceExhaust   0x02  /* 磁盘空间以满，图标闪烁。 */

/*------------ OSD的长度限制 -----------*/
#define CDCA_MAXLEN_OSD           180U  /* OSD内容的最大长度 */
/*------------ OSD显示类型 ------------*/
#define CDCA_OSD_TOP              0x01  /* OSD风格：显示在屏幕上方 */
#define CDCA_OSD_BOTTOM           0x02  /* OSD风格：显示在屏幕下方 */
#define CDCA_OSD_FULLSCREEN       0x03  /* OSD风格：整屏显示 */
#define CDCA_OSD_HALFSCREEN       0x04  /* OSD风格：半屏显示 */



/*---------- 频道锁定应用相关定义 ---------*/
#define CDCA_MAXNUM_COMPONENT     5U    /* 节目组件最大个数 */
#define CDCA_MAXLEN_LOCKMESS      40U

/*---------- CDSTBCA_ActionRequestExt 接口参数 ------------------------------*/
#define CDCA_ACTIONREQUEST_RESTARTSTB		   0	/* 重启机顶盒 */
#define CDCA_ACTIONREQUEST_FREEZESTB		   1	/* 冻结机顶盒 */
#define CDCA_ACTIONREQUEST_SEARCHCHANNEL       2    /* 重新搜索节目 */
#define CDCA_ACTIONREQUEST_STBUPGRADE          3    /* 机顶盒程序升级 */
#define CDCA_ACTIONREQUEST_UNFREEZESTB         4    /* 解除冻结机顶盒 */
#define CDCA_ACTIONREQUEST_INITIALIZESTB	   5    /* 初始化机顶盒 */
#define CDCA_ACTIONREQUEST_SHOWSYSTEMINFO      6    /* 显示系统信息 */

#define CDCA_ACTIONREQUEST_DISABLEEPGADINFO    201    /* 禁用EPG广告信息功能 */
#define CDCA_ACTIONREQUEST_ENABLEEPGADINFO     202    /* 恢复EPG广告信息功能 */


/*------------------------------------------------------------------------*/




/*-------------------------------------end of 宏定义--------------------------------------*/



/*----------------------------------------数据结构----------------------------------------*/

/*-- 系统时间 --*/
typedef CDCA_U32  CDCA_TIME;
typedef CDCA_U16  CDCA_DATE;

/*-- 信号量定义（不同的操作系统可能不一样）--*/
typedef CDCA_U32  CDCA_Semaphore;

/*-- 节目信息 --*/
/* Y10_update : 只需要ECMPID和ServiceID即可 */
typedef struct {
    CDCA_U16  m_wEcmPid;         /* 节目相应控制信息的PID */
    CDCA_U8   m_byServiceNum;    /* 当前PID下的节目个数 */
    CDCA_U8   m_byReserved;      /* 保留 */
    CDCA_U16  m_wServiceID[CDCA_MAXNUM_PROGRAMBYCW]; /* 当前PID下的节目ID列表 */
}SCDCASServiceInfo;


/*-- 运营商信息 --*/
typedef struct {
    char     m_szTVSPriInfo[CDCA_MAXLEN_TVSPRIINFO+1];  /* 运营商私有信息 */
    CDCA_U8  m_byReserved[3];    /* 保留 */
}SCDCAOperatorInfo;


/*-- 授权信息 --*/
typedef struct {
    CDCA_U32  m_dwProductID;   /* 普通授权的节目ID */    
    CDCA_DATE m_tBeginDate;    /* 授权的起始时间 */
    CDCA_DATE m_tExpireDate;   /* 授权的过期时间 */
    CDCA_U8   m_bCanTape;      /* 用户是否购买录像：1－可以录像；0－不可以录像 */
    CDCA_U8   m_byReserved[3]; /* 保留 */
}SCDCAEntitle;

/*-- 授权信息集合 --*/
typedef struct {
    CDCA_U16      m_wProductCount;
    CDCA_U8       m_m_byReserved[2];    /* 保留 */
    SCDCAEntitle  m_Entitles[CDCA_MAXNUM_ENTITLE]; /* 授权列表 */
}SCDCAEntitles;


/*-- 邮件头 --*/
typedef struct {
    CDCA_U32   m_dwActionID;                 /* Email ID */
    CDCA_U32   m_tCreateTime;                /* EMAIL创建的时间 */
    CDCA_U16   m_wImportance;                /* 重要性： 0－普通，1－重要 */
    CDCA_U8    m_byReserved[2];              /* 保留 */
    char       m_szEmailHead[CDCA_MAXLEN_EMAIL_TITLE+4]; /* 邮件标题 */    
    CDCA_U8    m_bNewEmail;                  /* 新邮件标记：0－已读邮件；1－新邮件 */
}SCDCAEmailHead;

/*-- 邮件内容 --*/
typedef struct {
    char     m_szEmail[CDCA_MAXLEN_EMAIL_CONTENT+4];      /* Email的正文 */
    CDCA_U8  m_byReserved[3];              /* 保留 */
}SCDCAEmailContent;



/*-- 频道锁定信息 --*/
/*-- 节目组件信息 --*/
typedef struct {    /* 组件用于通知机顶盒节目类型及PID等信息，一个节目可能包含多个组件 */
    CDCA_U16   m_wCompPID;     /* 组件PID */
    CDCA_U16   m_wECMPID;      /* 组件对应的ECM包的PID，如果组件是不加扰的，则应取0。 */
    CDCA_U8    m_CompType;     /* 组件类型 */
    CDCA_U8    m_byReserved[3];/* 保留 */
}SCDCAComponent;


/*-- 频道参数信息 --*/
typedef struct {    
    CDCA_U32   m_dwFrequency;              /* 频率，BCD码 */
    CDCA_U32   m_symbol_rate;              /* 符号率，BCD码 */
    CDCA_U16   m_wPcrPid;                  /* PCR PID */
    CDCA_U8    m_Modulation;               /* 调制方式 */
    CDCA_U8    m_ComponentNum;             /* 节目组件个数 */
    SCDCAComponent m_CompArr[CDCA_MAXNUM_COMPONENT];       /* 节目组件列表 */
    CDCA_U8    m_fec_outer;                /* 前项纠错外码 */
    CDCA_U8    m_fec_inner;                /* 前项纠错内码 */
    char       m_szBeforeInfo[CDCA_MAXLEN_LOCKMESS+1]; /* 保留 */
    char       m_szQuitInfo[CDCA_MAXLEN_LOCKMESS+1];   /* 保留 */
    char       m_szEndInfo[CDCA_MAXLEN_LOCKMESS+1];    /* 保留 */
}SCDCALockService;




/*-----------------------------------------------------------------------------------
a. 本系统中，参数m_dwFrequency和m_symbol_rate使用BCD码，编码前取MHz为单位。
   编码时，前4个4-bit BCD码表示小数点前的值，后4个4-bit BCD码表示小数点后的值。
   例如：
        若频率为642000KHz，即642.0000MHz，则对应的m_dwFrequency的值应为0x06420000；
        若符号率为6875KHz，即6.8750MHz，则对应的m_symbol_rate的值应为0x00068750。

b. 本系统中，m_Modulation的取值如下：
    0       Reserved
    1       QAM16
    2       QAM32
    3       QAM64
    4       QAM128
    5       QAM256
    6～255  Reserved
------------------------------------------------------------------------------------*/ 

/*------------------------------------end of 数据结构-------------------------------------*/



/*---------------------------以下接口是CA_LIB提供给STB------------------------*/

/*------ CA_LIB调度管理 ------*/

/* CA_LIB初始化 */
extern CDCA_BOOL CDCASTB_Init( CDCA_U8 byThreadPrior );

/* 关闭CA_LIB，释放资源 */
extern void CDCASTB_Close( void );

/* CDCAS同密判断 */
extern CDCA_BOOL  CDCASTB_IsCDCa(CDCA_U16 wCaSystemID);

/*------ Flash管理 ------ */

/* 存储空间的格式化 */
extern void CDCASTB_FormatBuffer( void );

/* 屏蔽对存储空间的读写操作 */
extern void CDCASTB_RequestMaskBuffer(void);

/* 打开对存储空间的读写操作 */
extern void CDCASTB_RequestUpdateBuffer(void);

/*------ TS流管理 ------*/

/* 设置ECM和节目信息 */
extern void CDCASTB_SetEcmPid( CDCA_U8 byType,
                               const SCDCASServiceInfo* pServiceInfo );

/* 设置EMM信息 */
extern void  CDCASTB_SetEmmPid(CDCA_U16 wEmmPid);

/* 私有数据接收回调 */
extern void CDCASTB_PrivateDataGot( CDCA_U8        byReqID,
								  	CDCA_BOOL      bTimeout,
									CDCA_U16       wPid,
									const CDCA_U8* pbyReceiveData,
									CDCA_U16       wLen            );


/* PIN码管理 */
extern CDCA_U16 CDCASTB_ChangePin( const CDCA_U8* pbyOldPin,
                                   const CDCA_U8* pbyNewPin);

/* 设置用户观看级别 */
extern CDCA_U16 CDCASTB_SetRating( const CDCA_U8* pbyPin,
                                   CDCA_U8 byRating );

/* 查询用户观看级别 */
extern CDCA_U16 CDCASTB_GetRating( CDCA_U8* pbyRating );


/*------- 基本信息查询 -------*/

/* 查询CA_LIB版本号 */
extern CDCA_U32 CDCASTB_GetVer( void );

/* 查询运营商ID列表 */
extern CDCA_U16 CDCASTB_GetOperatorIds( CDCA_U16* pwTVSID );

/* 查询运营商信息 */
extern CDCA_U16 CDCASTB_GetOperatorInfo( CDCA_U16           wTVSID,
                                         SCDCAOperatorInfo* pOperatorInfo );

/* 查询用户特征 */
extern CDCA_U16 CDCASTB_GetACList( CDCA_U16 wTVSID, CDCA_U32* pACArray );


/* 查询普通授权节目购买情况 */
extern CDCA_U16 CDCASTB_GetServiceEntitles( CDCA_U16       wTVSID,
                                            SCDCAEntitles* pServiceEntitles );



/*CA通用查询接口*/
/*
参数说明:
    byType  :查询类型。             
    pInData :输入数据地址，数据空间由调用者分配。
    pOutData:输出数据地址，数据空间由调用者分配。
返回值：CDCA_RC_OK 	：表示成功
	    其他值		：表示失败，具体查看头文件的"功能调用返回值定义"
*/
extern CDCA_U16 CDCASTB_QuerySomething(CDCA_U8 byType,void * pInData, void * pOutData);



/*-------- 授权信息管理 --------*/

/* 查询授权ID列表 */
extern CDCA_U16 CDCASTB_GetEntitleIDs( CDCA_U16  wTVSID,
                                       CDCA_U32* pdwEntitleIds );


/* 查询机顶盒平台编号 */
extern CDCA_U16 CDCASTB_GetPlatformID( void );




/*-------- 邮件管理 --------*/

/* 查询邮件头信息 */
extern CDCA_U16 CDCASTB_GetEmailHeads( SCDCAEmailHead* pEmailHead,
                                       CDCA_U8*        pbyCount,
                                       CDCA_U8*        pbyFromIndex );

/* 查询指定邮件的头信息 */
extern CDCA_U16 CDCASTB_GetEmailHead( CDCA_U32        dwEmailID,
                                      SCDCAEmailHead* pEmailHead );

/* 查询指定邮件的内容 */
extern CDCA_U16 CDCASTB_GetEmailContent( CDCA_U32           dwEmailID,
                                         SCDCAEmailContent* pEmailContent );

/* 删除邮件 */
extern void CDCASTB_DelEmail( CDCA_U32 dwEmailID );

/* 查询邮箱使用情况 */
extern CDCA_U16 CDCASTB_GetEmailSpaceInfo( CDCA_U8* pbyEmailNum,
                                           CDCA_U8* pbyEmptyNum );




/*-------- 显示界面管理 --------*/

/* 刷新界面 */
extern void CDCASTB_RefreshInterface( void );



    

/*------------------------以上接口是CA_LIB提供给STB---------------------------*/

/******************************************************************************/

/*------------------------以下接口是STB提供给CA_LIB---------------------------*/

/*-------- 线程管理 --------*/

/* 注册任务 */
extern CDCA_BOOL CDSTBCA_RegisterTask( const char* szName,
                                       CDCA_U8     byPriority,
                                       void*       pTaskFun,
                                       void*       pParam,
                                       CDCA_U16    wStackSize  );

/* 线程挂起 */
extern void CDSTBCA_Sleep(CDCA_U32 wMilliSeconds);


/*-------- 信号量管理 --------*/

/* 初始化信号量 */
extern void CDSTBCA_SemaphoreInit( CDCA_Semaphore* pSemaphore,
                                   CDCA_BOOL       bInitVal );

/* 信号量给予信号 */
extern void CDSTBCA_SemaphoreSignal( CDCA_Semaphore* pSemaphore );

/* 信号量获取信号 */
extern void CDSTBCA_SemaphoreWait( CDCA_Semaphore* pSemaphore );


/*-------- 内存管理 --------*/

/* 分配内存 */
extern void* CDSTBCA_Malloc( CDCA_U32 byBufSize );

/* 释放内存 */
extern void  CDSTBCA_Free( void* pBuf );

/* 内存赋值 */
extern void  CDSTBCA_Memset( void*    pDestBuf,
                             CDCA_U8  c,
                             CDCA_U32 wSize );

/* 内存复制 */
extern void  CDSTBCA_Memcpy( void*       pDestBuf,
                             const void* pSrcBuf,
                             CDCA_U32    wSize );


/*--------- 存储空间（Flash）管理 ---------*/

/* 读取存储空间 */
extern void CDSTBCA_ReadBuffer( CDCA_U8   byBlockID,
                                CDCA_U8*  pbyData,
                                CDCA_U32* pdwLen );

/* 写入存储空间 */
extern void CDSTBCA_WriteBuffer( CDCA_U8        byBlockID,
                                 const CDCA_U8* pbyData,
                                 CDCA_U32       dwLen );


/*-------- TS流管理 --------*/

/* 设置私有数据过滤器 */
extern CDCA_BOOL CDSTBCA_SetPrivateDataFilter( CDCA_U8        byReqID,  
											   const CDCA_U8* pbyFilter,  
											   const CDCA_U8* pbyMask, 
											   CDCA_U8        byLen, 
											   CDCA_U16       wPid, 
											   CDCA_U8        byWaitSeconds );

/* 释放私有数据过滤器 */
extern void CDSTBCA_ReleasePrivateDataFilter( CDCA_U8  byReqID,
                                              CDCA_U16 wPid );



/*-------- 安全控制 --------*/

/* 读取机顶盒唯一编号 */
extern void CDSTBCA_GetSTBID( CDCA_U16* pwPlatformID,
                              CDCA_U32* pdwUniqueID);

/* 安全芯片接口(含高安设置CW) */
extern CDCA_U16 CDSTBCA_SCFunction( CDCA_U8* pData);


/*------- 邮件/OSD显示管理 -------*/

/* 邮件通知 */
extern void CDSTBCA_EmailNotifyIcon( CDCA_U8 byShow, CDCA_U32 dwEmailID );

/* 显示普通OSD信息 */
extern void CDSTBCA_ShowOSDMessage( CDCA_U8     byStyle,
                                    const char* szMessage );

/* 隐藏OSD信息*/
extern void CDSTBCA_HideOSDMessage( CDCA_U8 byStyle );




/*-------- 强制切换频道 --------*/
/* 频道锁定 */
extern void CDSTBCA_LockService( const SCDCALockService* pLockService );

/* 解除频道锁定 */
extern void CDSTBCA_UNLockService( void );


/*-------- 显示界面管理 --------*/

/* 不能正常收看节目的提示 */
/*wEcmPID==0表示与wEcmPID无关的消息*/
extern void CDSTBCA_ShowBuyMessage( CDCA_U16 wEcmPID,
                                    CDCA_U8  byMessageType );



/* 指纹显示，fingerMsg=NULL 表示取消指纹*/
extern void CDSTBCA_ShowFingerMessageExt( CDCA_U16 wEcmPID,
                                      char* fingerMsg );

/*--------- 机顶盒通知 --------*/
/* 机顶盒通知 */
extern void  CDSTBCA_ActionRequestExt( CDCA_U16 wTVSID,
                                      CDCA_U8  byActionType,
                                      CDCA_U8 byLen,
                                      CDCA_U8* pbyData);




/*--------- 获取当前时间 --------*/
/*获取成功返回CDCA_TRUE,获取失败返回CDCA_FALSE*/
//The bias is the difference, in minutes, between Coordinated Universal Time (UTC) and local time, UTC = local time + bias
extern CDCA_BOOL  CDSTBCA_GetCurrentTime( CDCA_S32  *pUTCTime, CDCA_S32 *pTimeZoneBias, CDCA_S32 *pDaylightBias,  CDCA_BOOL* pInDaylight);






/*-------- 其它 --------*/

/* 获取字符串长度 */
extern CDCA_U16 CDSTBCA_Strlen(const char* pString );

/* 调试信息输出 */
extern void CDSTBCA_Printf(CDCA_U8 byLevel, const char* szMesssage );

/* 对数据进行AES CBC加密 */
extern CDCA_BOOL CDSTBCA_AESCBC_Encrypt(CDCA_U8 *pPlainText, CDCA_U32 TextLen, CDCA_U8 *pCipherText);

/* 对数据进行AES CBC解密 */
extern CDCA_BOOL CDSTBCA_AESCBC_Decrypt(CDCA_U8 *pCipherText, CDCA_U32 TextLen, CDCA_U8 *pPlainText);

/*---------------------------以上接口是STB提供给CA_LIB------------------------*/

#ifdef  __cplusplus
}
#endif
#endif
/*EOF*/

