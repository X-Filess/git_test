
#ifndef __APP_OTA_PROTOCOL_H__
#define __APP_OTA_PROTOCOL_H__

#include "gxcore.h"
#include "gxsi.h"
#include "app_config.h"

#if (OTA_SUPPORT > 0)


/***************************************************************
		Configure for OTA
***************************************************************/
//#define APP_OTA_PID 			(0x0712)
#define APP_OTA_DSI_TABLE_ID	(0x3B)
#define APP_OTA_DII_TABLE_ID	(0x3B)
#define APP_OTA_DDB_TABLE_ID	(0x3C)


#define MAX_OTA_MODULE_NUM			(0x10)
#define MAX_OTA_BLOCK_NUM			(0xff)


#define OTA_DSI_VERSION_IS_OLD		(0xfff1)

/*The OTA message list*/
#define GXOTA_STATUS_INIT                       (0)
#define GXOTA_STATUS_FRONTEND_ERROR             (1)
#define GXOTA_STATUS_FRONTEND_SETUP             (2)
#define GXOTA_STATUS_FRONTEND_UNLOCK            (3)
#define GXOTA_STATUS_FRONTEND_LOCK              (4)
#define GXOTA_STATUS_DMX_ERROR                  (5)
#define GXOTA_STATUS_DMX_START_FILTER           (6)
#define GXOTA_STATUS_DMX_DATA_FINISH            (7)
#define GXOTA_STATUS_WRITE_FLASH                (8)
#define GXOTA_STATUS_WRITE_FLASH_OK             (9)
#define GXOTA_STATUS_WRITE_FLASH_FAILED         (10)
#define GXOTA_STATUS_UPDATE_FAILED              (11)

#define GXOTA_STATUS_BLOCK_FINISH               (12)
#define GXOTA_STATUS_MODULE_FINISH              (13)
#define GXOTA_STATUS_BLOCK_NOT_FINISH           (14)
#define GXOTA_STATUS_MODULE_NOT_FINISH          (15)
#define GXOTA_STATUS__VERSION_IS_NEW            (16)
#define GXOTA_STATUS_VERSION_IS_OLD             (17)
#define GXOTA_STATUS_NOT_MACHING                (18)
#define GXOTA_STATUS_BLOCK_HAS_BEEN_DEALWITH    (19)

typedef struct AppOtaSub
{
	int32_t     subt_id;
	uint32_t    req_id;
	uint32_t    ver;
}AppOtaSub_t;

typedef struct app_ota AppOta;
struct app_ota
{
	//external, only start need
	uint32_t    	ts_src;
	uint32_t    	pid;
	//internal
	uint32_t 		enable;
	uint32_t		filter_count;
	AppOtaSub_t 	ota_sub[MAX_OTA_MODULE_NUM];
	void        	(*start)(AppOta*);
	uint32_t    	(*stop)(AppOta*);
	status_t    	(*analyse)(AppOta*, GxMsgProperty_SiSubtableOk*); 
	void        	(*init)(void);
};

extern AppOta   g_AppOtaDsi;
extern AppOta   g_AppOtaDii;
extern AppOta   g_AppOtaDdb;



/***************************************************************
		Struct for OTA Protocol DSI/DII/DDB
***************************************************************/
typedef struct OtaDiiModule_s
{
    uint32_t m_nDiiModuleSize;
    uint16_t m_wDiiModuleId;
}OtaDiiModule_t;

typedef struct OtaDiiInfo_s
{
    uint32_t m_nDiiDownloadId;
    uint16_t m_wDiiBlockSize;
    uint16_t m_wDiiModuleNum;       
    OtaDiiModule_t m_OtaDiiModule[MAX_OTA_MODULE_NUM];
}OtaDiiInfo_t;

typedef struct OtaDdbModuleInfo_s
{
    uint16_t m_wBlockNum;//is block num
    uint16_t m_wDdbBlockAcceptFlg[MAX_OTA_BLOCK_NUM+1];
}OtaDdbModuleInfo_t;

typedef struct OtaDdbInfo_s
{
    uint8_t chBankSecondEn;
    uint8_t *pBufferBankOne;
    uint8_t *pBufferBankSecond;
    uint16_t m_wModuleNum;
    uint16_t m_wModuleAcceptFlg[MAX_OTA_MODULE_NUM];
    OtaDdbModuleInfo_t m_DdbModule[MAX_OTA_MODULE_NUM];
}OtaDdbInfo_t;

typedef struct OtaDsmInfo_s//All information of OTA
{
    uint16_t m_wDsiGroupNum;//
    uint16_t m_wDsiSize;//
    uint32_t m_nDsiGroupId;//
    uint32_t m_nDsiGroupSize;
    uint32_t m_nDsiOUI;
    uint16_t m_nDsiHwModule;
    uint16_t m_nDsiHwVersion;
    uint16_t m_nDsiSwModule;
    uint16_t m_nDsiSwVersion;
    OtaDiiInfo_t OtaDiiInfo;
    OtaDdbInfo_t m_OtaDdbInfo;
}OtaDsmInfo_t;


typedef struct GxOTA_ProtoOTAData_s
{
    int                                 segment;
    unsigned char*                      src;
    unsigned int                        size;
    unsigned int                        dst;
} GxOTA_ProtoOTAData_t;


extern void app_ota_upgrade_ver_old(void);
#endif
#endif

