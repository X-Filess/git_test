#ifndef CDCAS_CALIB_APIEX_H
#define CDCAS_CALIB_APIEX_H

#ifdef  __cplusplus
extern "C" {
#endif


/*-------------------------------------������������---------------------------------------*/
typedef unsigned  long  CDCA_U32;
typedef unsigned  short CDCA_U16;
typedef unsigned  char  CDCA_U8;

typedef signed char  CDCA_S8;
typedef signed long  CDCA_S32;

typedef CDCA_U8 CDCA_BOOL;

#define CDCA_TRUE    ((CDCA_BOOL)1)
#define CDCA_FALSE   ((CDCA_BOOL)0)

/*----------------------------------------�궨��--------------------------------------*/

/*--------- FLASH���� -------*/
#define CDCA_FLASH_BLOCK_A        1     /* BLOCK A */
#define CDCA_FLASH_BLOCK_B        2     /* BLOCK B */
#define CDCA_FLASH_BLOCK_C        3     /* BLOCK C */

/*--------- ������� --------*/
#define CDCA_MAXLEN_STBSN         12U   /* �����кŵĳ��� */
/*---------- ������������� ----------*/
#define CDCA_MAXNUM_PROGRAMBYCW   4U    /* һ������������Ľ�Ŀ�� */

#define CDCA_MAXLEN_PINCODE       6U    /* PIN��ĳ��� */
#define CDCA_MAXNUM_OPERATOR      4U    /* ������Ӫ�̸��� */
#define CDCA_MAXLEN_TVSPRIINFO    32U   /* ��Ӫ��˽����Ϣ�ĳ��� */
#define CDCA_MAXNUM_ACLIST        18U   /* ÿ����Ӫ�̵��û��������� */


#define CDCA_MAXNUM_ENTITLE       500U  /* ��Ȩ��Ʒ�������� */


/*---------- CAS��ʾ��Ϣ ---------*/
#define CDCA_MESSAGE_CANCEL_TYPE      0x00  /* ȡ����ǰ����ʾ */
#define CDCA_MESSAGE_NOOPER_TYPE      0x04  /* ��֧�ֽ�Ŀ��Ӫ�� */
#define CDCA_MESSAGE_WATCHLEVEL_TYPE  0x07  /* ��Ŀ��������趨�Ĺۿ����� */
#define CDCA_MESSAGE_NOENTITLE_TYPE   0x09  /* û����Ȩ */
#define CDCA_MESSAGE_DECRYPTFAIL_TYPE 0x0A  /* ��Ŀ����ʧ�� */
#define CDCA_MESSAGE_ERRREGION_TYPE   0x0C  /* ������ȷ */
#define CDCA_MESSAGE_STBLOCKED_TYPE   0x20  /* ������������ */
#define CDCA_MESSAGE_STBFREEZE_TYPE   0x21  /* �����б����� */


/*---------- ���ܵ��÷���ֵ���� ----------*/
#define CDCA_RC_OK                    0x00  /* �ɹ� */
#define CDCA_RC_UNKNOWN               0x01  /* δ֪���� */
#define CDCA_RC_POINTER_INVALID       0x02  /* ָ����Ч */
#define CDCA_RC_PIN_INVALID           0x04  /* PIN����Ч */
#define CDCA_RC_DATASPACE_SMALL       0x06  /* �����Ŀռ䲻�� */
#define CDCA_RC_DATA_NOT_FIND         0x08  /* û���ҵ���Ҫ������ */
#define CDCA_RC_WATCHRATING_INVALID   0x0E  /* �趨�Ĺۿ�������Ч */
#define CDCA_RC_UNSUPPORT             0x22  /* ��֧�� */
#define CDCA_RC_LEN_ERROR             0x23  /* ���ȴ��� */
#define CDCA_RC_SCHIP_ERROR           0x24  /* ��ȫоƬ���ô��� */

/*---------- ECM_PID���õĲ������� ---------*/
#define CDCA_LIST_OK          0x00
#define CDCA_LIST_FIRST       0x01
#define CDCA_LIST_ADD         0x02




/*------------ �ʼ���С���������� ------------*/
#define CDCA_MAXNUM_EMAIL         100U  /* �����б��������ʼ����� */
#define CDCA_MAXLEN_EMAIL_TITLE   30U   /* �ʼ�����ĳ��� */
#define CDCA_MAXLEN_EMAIL_CONTENT 160U  /* �ʼ����ݵĳ��� */
/*------------ �ʼ�ͼ����ʾ��ʽ ------------*/
#define CDCA_Email_IconHide       0x00  /* �����ʼ�֪ͨͼ�� */
#define CDCA_Email_New            0x01  /* ���ʼ�֪ͨ����ʾ���ʼ�ͼ�� */
#define CDCA_Email_SpaceExhaust   0x02  /* ���̿ռ�������ͼ����˸�� */

/*------------ OSD�ĳ������� -----------*/
#define CDCA_MAXLEN_OSD           180U  /* OSD���ݵ���󳤶� */
/*------------ OSD��ʾ���� ------------*/
#define CDCA_OSD_TOP              0x01  /* OSD�����ʾ����Ļ�Ϸ� */
#define CDCA_OSD_BOTTOM           0x02  /* OSD�����ʾ����Ļ�·� */
#define CDCA_OSD_FULLSCREEN       0x03  /* OSD���������ʾ */
#define CDCA_OSD_HALFSCREEN       0x04  /* OSD��񣺰�����ʾ */



/*---------- Ƶ������Ӧ����ض��� ---------*/
#define CDCA_MAXNUM_COMPONENT     5U    /* ��Ŀ��������� */
#define CDCA_MAXLEN_LOCKMESS      40U

/*---------- CDSTBCA_ActionRequestExt �ӿڲ��� ------------------------------*/
#define CDCA_ACTIONREQUEST_RESTARTSTB		   0	/* ���������� */
#define CDCA_ACTIONREQUEST_FREEZESTB		   1	/* ��������� */
#define CDCA_ACTIONREQUEST_SEARCHCHANNEL       2    /* ����������Ŀ */
#define CDCA_ACTIONREQUEST_STBUPGRADE          3    /* �����г������� */
#define CDCA_ACTIONREQUEST_UNFREEZESTB         4    /* ������������ */
#define CDCA_ACTIONREQUEST_INITIALIZESTB	   5    /* ��ʼ�������� */
#define CDCA_ACTIONREQUEST_SHOWSYSTEMINFO      6    /* ��ʾϵͳ��Ϣ */

#define CDCA_ACTIONREQUEST_DISABLEEPGADINFO    201    /* ����EPG�����Ϣ���� */
#define CDCA_ACTIONREQUEST_ENABLEEPGADINFO     202    /* �ָ�EPG�����Ϣ���� */


/*------------------------------------------------------------------------*/




/*-------------------------------------end of �궨��--------------------------------------*/



/*----------------------------------------���ݽṹ----------------------------------------*/

/*-- ϵͳʱ�� --*/
typedef CDCA_U32  CDCA_TIME;
typedef CDCA_U16  CDCA_DATE;

/*-- �ź������壨��ͬ�Ĳ���ϵͳ���ܲ�һ����--*/
typedef CDCA_U32  CDCA_Semaphore;

/*-- ��Ŀ��Ϣ --*/
/* Y10_update : ֻ��ҪECMPID��ServiceID���� */
typedef struct {
    CDCA_U16  m_wEcmPid;         /* ��Ŀ��Ӧ������Ϣ��PID */
    CDCA_U8   m_byServiceNum;    /* ��ǰPID�µĽ�Ŀ���� */
    CDCA_U8   m_byReserved;      /* ���� */
    CDCA_U16  m_wServiceID[CDCA_MAXNUM_PROGRAMBYCW]; /* ��ǰPID�µĽ�ĿID�б� */
}SCDCASServiceInfo;


/*-- ��Ӫ����Ϣ --*/
typedef struct {
    char     m_szTVSPriInfo[CDCA_MAXLEN_TVSPRIINFO+1];  /* ��Ӫ��˽����Ϣ */
    CDCA_U8  m_byReserved[3];    /* ���� */
}SCDCAOperatorInfo;


/*-- ��Ȩ��Ϣ --*/
typedef struct {
    CDCA_U32  m_dwProductID;   /* ��ͨ��Ȩ�Ľ�ĿID */    
    CDCA_DATE m_tBeginDate;    /* ��Ȩ����ʼʱ�� */
    CDCA_DATE m_tExpireDate;   /* ��Ȩ�Ĺ���ʱ�� */
    CDCA_U8   m_bCanTape;      /* �û��Ƿ���¼��1������¼��0��������¼�� */
    CDCA_U8   m_byReserved[3]; /* ���� */
}SCDCAEntitle;

/*-- ��Ȩ��Ϣ���� --*/
typedef struct {
    CDCA_U16      m_wProductCount;
    CDCA_U8       m_m_byReserved[2];    /* ���� */
    SCDCAEntitle  m_Entitles[CDCA_MAXNUM_ENTITLE]; /* ��Ȩ�б� */
}SCDCAEntitles;


/*-- �ʼ�ͷ --*/
typedef struct {
    CDCA_U32   m_dwActionID;                 /* Email ID */
    CDCA_U32   m_tCreateTime;                /* EMAIL������ʱ�� */
    CDCA_U16   m_wImportance;                /* ��Ҫ�ԣ� 0����ͨ��1����Ҫ */
    CDCA_U8    m_byReserved[2];              /* ���� */
    char       m_szEmailHead[CDCA_MAXLEN_EMAIL_TITLE+4]; /* �ʼ����� */    
    CDCA_U8    m_bNewEmail;                  /* ���ʼ���ǣ�0���Ѷ��ʼ���1�����ʼ� */
}SCDCAEmailHead;

/*-- �ʼ����� --*/
typedef struct {
    char     m_szEmail[CDCA_MAXLEN_EMAIL_CONTENT+4];      /* Email������ */
    CDCA_U8  m_byReserved[3];              /* ���� */
}SCDCAEmailContent;



/*-- Ƶ��������Ϣ --*/
/*-- ��Ŀ�����Ϣ --*/
typedef struct {    /* �������֪ͨ�����н�Ŀ���ͼ�PID����Ϣ��һ����Ŀ���ܰ��������� */
    CDCA_U16   m_wCompPID;     /* ���PID */
    CDCA_U16   m_wECMPID;      /* �����Ӧ��ECM����PID���������ǲ����ŵģ���Ӧȡ0�� */
    CDCA_U8    m_CompType;     /* ������� */
    CDCA_U8    m_byReserved[3];/* ���� */
}SCDCAComponent;


/*-- Ƶ��������Ϣ --*/
typedef struct {    
    CDCA_U32   m_dwFrequency;              /* Ƶ�ʣ�BCD�� */
    CDCA_U32   m_symbol_rate;              /* �����ʣ�BCD�� */
    CDCA_U16   m_wPcrPid;                  /* PCR PID */
    CDCA_U8    m_Modulation;               /* ���Ʒ�ʽ */
    CDCA_U8    m_ComponentNum;             /* ��Ŀ������� */
    SCDCAComponent m_CompArr[CDCA_MAXNUM_COMPONENT];       /* ��Ŀ����б� */
    CDCA_U8    m_fec_outer;                /* ǰ��������� */
    CDCA_U8    m_fec_inner;                /* ǰ��������� */
    char       m_szBeforeInfo[CDCA_MAXLEN_LOCKMESS+1]; /* ���� */
    char       m_szQuitInfo[CDCA_MAXLEN_LOCKMESS+1];   /* ���� */
    char       m_szEndInfo[CDCA_MAXLEN_LOCKMESS+1];    /* ���� */
}SCDCALockService;




/*-----------------------------------------------------------------------------------
a. ��ϵͳ�У�����m_dwFrequency��m_symbol_rateʹ��BCD�룬����ǰȡMHzΪ��λ��
   ����ʱ��ǰ4��4-bit BCD���ʾС����ǰ��ֵ����4��4-bit BCD���ʾС������ֵ��
   ���磺
        ��Ƶ��Ϊ642000KHz����642.0000MHz�����Ӧ��m_dwFrequency��ֵӦΪ0x06420000��
        ��������Ϊ6875KHz����6.8750MHz�����Ӧ��m_symbol_rate��ֵӦΪ0x00068750��

b. ��ϵͳ�У�m_Modulation��ȡֵ���£�
    0       Reserved
    1       QAM16
    2       QAM32
    3       QAM64
    4       QAM128
    5       QAM256
    6��255  Reserved
------------------------------------------------------------------------------------*/ 

/*------------------------------------end of ���ݽṹ-------------------------------------*/



/*---------------------------���½ӿ���CA_LIB�ṩ��STB------------------------*/

/*------ CA_LIB���ȹ��� ------*/

/* CA_LIB��ʼ�� */
extern CDCA_BOOL CDCASTB_Init( CDCA_U8 byThreadPrior );

/* �ر�CA_LIB���ͷ���Դ */
extern void CDCASTB_Close( void );

/* CDCASͬ���ж� */
extern CDCA_BOOL  CDCASTB_IsCDCa(CDCA_U16 wCaSystemID);

/*------ Flash���� ------ */

/* �洢�ռ�ĸ�ʽ�� */
extern void CDCASTB_FormatBuffer( void );

/* ���ζԴ洢�ռ�Ķ�д���� */
extern void CDCASTB_RequestMaskBuffer(void);

/* �򿪶Դ洢�ռ�Ķ�д���� */
extern void CDCASTB_RequestUpdateBuffer(void);

/*------ TS������ ------*/

/* ����ECM�ͽ�Ŀ��Ϣ */
extern void CDCASTB_SetEcmPid( CDCA_U8 byType,
                               const SCDCASServiceInfo* pServiceInfo );

/* ����EMM��Ϣ */
extern void  CDCASTB_SetEmmPid(CDCA_U16 wEmmPid);

/* ˽�����ݽ��ջص� */
extern void CDCASTB_PrivateDataGot( CDCA_U8        byReqID,
								  	CDCA_BOOL      bTimeout,
									CDCA_U16       wPid,
									const CDCA_U8* pbyReceiveData,
									CDCA_U16       wLen            );


/* PIN����� */
extern CDCA_U16 CDCASTB_ChangePin( const CDCA_U8* pbyOldPin,
                                   const CDCA_U8* pbyNewPin);

/* �����û��ۿ����� */
extern CDCA_U16 CDCASTB_SetRating( const CDCA_U8* pbyPin,
                                   CDCA_U8 byRating );

/* ��ѯ�û��ۿ����� */
extern CDCA_U16 CDCASTB_GetRating( CDCA_U8* pbyRating );


/*------- ������Ϣ��ѯ -------*/

/* ��ѯCA_LIB�汾�� */
extern CDCA_U32 CDCASTB_GetVer( void );

/* ��ѯ��Ӫ��ID�б� */
extern CDCA_U16 CDCASTB_GetOperatorIds( CDCA_U16* pwTVSID );

/* ��ѯ��Ӫ����Ϣ */
extern CDCA_U16 CDCASTB_GetOperatorInfo( CDCA_U16           wTVSID,
                                         SCDCAOperatorInfo* pOperatorInfo );

/* ��ѯ�û����� */
extern CDCA_U16 CDCASTB_GetACList( CDCA_U16 wTVSID, CDCA_U32* pACArray );


/* ��ѯ��ͨ��Ȩ��Ŀ������� */
extern CDCA_U16 CDCASTB_GetServiceEntitles( CDCA_U16       wTVSID,
                                            SCDCAEntitles* pServiceEntitles );



/*CAͨ�ò�ѯ�ӿ�*/
/*
����˵��:
    byType  :��ѯ���͡�             
    pInData :�������ݵ�ַ�����ݿռ��ɵ����߷��䡣
    pOutData:������ݵ�ַ�����ݿռ��ɵ����߷��䡣
����ֵ��CDCA_RC_OK 	����ʾ�ɹ�
	    ����ֵ		����ʾʧ�ܣ�����鿴ͷ�ļ���"���ܵ��÷���ֵ����"
*/
extern CDCA_U16 CDCASTB_QuerySomething(CDCA_U8 byType,void * pInData, void * pOutData);



/*-------- ��Ȩ��Ϣ���� --------*/

/* ��ѯ��ȨID�б� */
extern CDCA_U16 CDCASTB_GetEntitleIDs( CDCA_U16  wTVSID,
                                       CDCA_U32* pdwEntitleIds );


/* ��ѯ������ƽ̨��� */
extern CDCA_U16 CDCASTB_GetPlatformID( void );




/*-------- �ʼ����� --------*/

/* ��ѯ�ʼ�ͷ��Ϣ */
extern CDCA_U16 CDCASTB_GetEmailHeads( SCDCAEmailHead* pEmailHead,
                                       CDCA_U8*        pbyCount,
                                       CDCA_U8*        pbyFromIndex );

/* ��ѯָ���ʼ���ͷ��Ϣ */
extern CDCA_U16 CDCASTB_GetEmailHead( CDCA_U32        dwEmailID,
                                      SCDCAEmailHead* pEmailHead );

/* ��ѯָ���ʼ������� */
extern CDCA_U16 CDCASTB_GetEmailContent( CDCA_U32           dwEmailID,
                                         SCDCAEmailContent* pEmailContent );

/* ɾ���ʼ� */
extern void CDCASTB_DelEmail( CDCA_U32 dwEmailID );

/* ��ѯ����ʹ����� */
extern CDCA_U16 CDCASTB_GetEmailSpaceInfo( CDCA_U8* pbyEmailNum,
                                           CDCA_U8* pbyEmptyNum );




/*-------- ��ʾ������� --------*/

/* ˢ�½��� */
extern void CDCASTB_RefreshInterface( void );



    

/*------------------------���Ͻӿ���CA_LIB�ṩ��STB---------------------------*/

/******************************************************************************/

/*------------------------���½ӿ���STB�ṩ��CA_LIB---------------------------*/

/*-------- �̹߳��� --------*/

/* ע������ */
extern CDCA_BOOL CDSTBCA_RegisterTask( const char* szName,
                                       CDCA_U8     byPriority,
                                       void*       pTaskFun,
                                       void*       pParam,
                                       CDCA_U16    wStackSize  );

/* �̹߳��� */
extern void CDSTBCA_Sleep(CDCA_U32 wMilliSeconds);


/*-------- �ź������� --------*/

/* ��ʼ���ź��� */
extern void CDSTBCA_SemaphoreInit( CDCA_Semaphore* pSemaphore,
                                   CDCA_BOOL       bInitVal );

/* �ź��������ź� */
extern void CDSTBCA_SemaphoreSignal( CDCA_Semaphore* pSemaphore );

/* �ź�����ȡ�ź� */
extern void CDSTBCA_SemaphoreWait( CDCA_Semaphore* pSemaphore );


/*-------- �ڴ���� --------*/

/* �����ڴ� */
extern void* CDSTBCA_Malloc( CDCA_U32 byBufSize );

/* �ͷ��ڴ� */
extern void  CDSTBCA_Free( void* pBuf );

/* �ڴ渳ֵ */
extern void  CDSTBCA_Memset( void*    pDestBuf,
                             CDCA_U8  c,
                             CDCA_U32 wSize );

/* �ڴ渴�� */
extern void  CDSTBCA_Memcpy( void*       pDestBuf,
                             const void* pSrcBuf,
                             CDCA_U32    wSize );


/*--------- �洢�ռ䣨Flash������ ---------*/

/* ��ȡ�洢�ռ� */
extern void CDSTBCA_ReadBuffer( CDCA_U8   byBlockID,
                                CDCA_U8*  pbyData,
                                CDCA_U32* pdwLen );

/* д��洢�ռ� */
extern void CDSTBCA_WriteBuffer( CDCA_U8        byBlockID,
                                 const CDCA_U8* pbyData,
                                 CDCA_U32       dwLen );


/*-------- TS������ --------*/

/* ����˽�����ݹ����� */
extern CDCA_BOOL CDSTBCA_SetPrivateDataFilter( CDCA_U8        byReqID,  
											   const CDCA_U8* pbyFilter,  
											   const CDCA_U8* pbyMask, 
											   CDCA_U8        byLen, 
											   CDCA_U16       wPid, 
											   CDCA_U8        byWaitSeconds );

/* �ͷ�˽�����ݹ����� */
extern void CDSTBCA_ReleasePrivateDataFilter( CDCA_U8  byReqID,
                                              CDCA_U16 wPid );



/*-------- ��ȫ���� --------*/

/* ��ȡ������Ψһ��� */
extern void CDSTBCA_GetSTBID( CDCA_U16* pwPlatformID,
                              CDCA_U32* pdwUniqueID);

/* ��ȫоƬ�ӿ�(���߰�����CW) */
extern CDCA_U16 CDSTBCA_SCFunction( CDCA_U8* pData);


/*------- �ʼ�/OSD��ʾ���� -------*/

/* �ʼ�֪ͨ */
extern void CDSTBCA_EmailNotifyIcon( CDCA_U8 byShow, CDCA_U32 dwEmailID );

/* ��ʾ��ͨOSD��Ϣ */
extern void CDSTBCA_ShowOSDMessage( CDCA_U8     byStyle,
                                    const char* szMessage );

/* ����OSD��Ϣ*/
extern void CDSTBCA_HideOSDMessage( CDCA_U8 byStyle );




/*-------- ǿ���л�Ƶ�� --------*/
/* Ƶ������ */
extern void CDSTBCA_LockService( const SCDCALockService* pLockService );

/* ���Ƶ������ */
extern void CDSTBCA_UNLockService( void );


/*-------- ��ʾ������� --------*/

/* ���������տ���Ŀ����ʾ */
/*wEcmPID==0��ʾ��wEcmPID�޹ص���Ϣ*/
extern void CDSTBCA_ShowBuyMessage( CDCA_U16 wEcmPID,
                                    CDCA_U8  byMessageType );



/* ָ����ʾ��fingerMsg=NULL ��ʾȡ��ָ��*/
extern void CDSTBCA_ShowFingerMessageExt( CDCA_U16 wEcmPID,
                                      char* fingerMsg );

/*--------- ������֪ͨ --------*/
/* ������֪ͨ */
extern void  CDSTBCA_ActionRequestExt( CDCA_U16 wTVSID,
                                      CDCA_U8  byActionType,
                                      CDCA_U8 byLen,
                                      CDCA_U8* pbyData);




/*--------- ��ȡ��ǰʱ�� --------*/
/*��ȡ�ɹ�����CDCA_TRUE,��ȡʧ�ܷ���CDCA_FALSE*/
//The bias is the difference, in minutes, between Coordinated Universal Time (UTC) and local time, UTC = local time + bias
extern CDCA_BOOL  CDSTBCA_GetCurrentTime( CDCA_S32  *pUTCTime, CDCA_S32 *pTimeZoneBias, CDCA_S32 *pDaylightBias,  CDCA_BOOL* pInDaylight);






/*-------- ���� --------*/

/* ��ȡ�ַ������� */
extern CDCA_U16 CDSTBCA_Strlen(const char* pString );

/* ������Ϣ��� */
extern void CDSTBCA_Printf(CDCA_U8 byLevel, const char* szMesssage );

/* �����ݽ���AES CBC���� */
extern CDCA_BOOL CDSTBCA_AESCBC_Encrypt(CDCA_U8 *pPlainText, CDCA_U32 TextLen, CDCA_U8 *pCipherText);

/* �����ݽ���AES CBC���� */
extern CDCA_BOOL CDSTBCA_AESCBC_Decrypt(CDCA_U8 *pCipherText, CDCA_U32 TextLen, CDCA_U8 *pPlainText);

/*---------------------------���Ͻӿ���STB�ṩ��CA_LIB------------------------*/

#ifdef  __cplusplus
}
#endif
#endif
/*EOF*/

