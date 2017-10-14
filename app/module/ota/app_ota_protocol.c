#include "app_module.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "app_ota_upgrade.h"
#include "app_ota_protocol.h"
#include "app_pop.h"

#if (OTA_SUPPORT > 0)
#include "app_ota_config.h"
#include "app_str_id.h"
extern GxOTA_ProtoOTAData_t data_for_upgrade;
extern uint8_t g_chFlagEntryFlash;
extern uint8_t g_chBissUpdate;
extern uint8_t g_chDefaultDataUpdate;
extern uint8_t g_chLogoUpdate;
uint8_t *sp_SectionData = NULL;
// add in 20151203
GxHwMallocObj hw_malloc_block;
static unsigned int thiz_new_version = 0;

OtaDsmInfo_t OtaDsmInfo={
							0,
							0,
							0,
							0,
							0x333333,
							0x3333,
							0x3333,
							0x3330,
							0x4000,
							{0},
							{0}};
extern void app_reboot(void);

/* Private functions ------------------------------------------------------ */
#define SUB_FUNC_DSI
static int32_t _ota_dsm_parse_dsi( uint8_t *pBuffIn, OtaDsmInfo_t* pOtaDsmInfo)
{
	 int32_t  ret_value = 0;
        uint16_t wDsiGroupNum = 0;
        uint16_t wDsiCnt = 0;
        uint16_t wTmpCnt = 0;
        uint16_t wDsiComLength = 0;
        uint8_t chDsiDataLength = 0;
        uint8_t chAdaptationLength = 0;

        uint32_t nDsiOui = 0;
        uint16_t wDsiHwModule = 0;
        uint16_t wDsiHwVersion = 0;
        uint16_t wDsiSwModule = 0;
        uint16_t wDsiSwVersion = 0;

        uint16_t g_info_len;

        pBuffIn += 8;//DSM-CC section header
        chAdaptationLength = pBuffIn[9];//Adap_len always is 0
        pBuffIn += 12;//DSM-CC Message header
        pBuffIn += chAdaptationLength;//
        pBuffIn += 20;//20 0xff

        pOtaDsmInfo->m_wDsiSize = (uint16_t)pBuffIn[2]<<8 | pBuffIn[3];
        pOtaDsmInfo->m_wDsiGroupNum = (uint16_t)pBuffIn[4]<<8 | pBuffIn[5];

        pBuffIn += 6;

        for(wDsiGroupNum = 0; wDsiGroupNum < pOtaDsmInfo->m_wDsiGroupNum; wDsiGroupNum++)
        {
                pOtaDsmInfo->m_nDsiGroupId = (uint32_t)pBuffIn[0]<<24 | (uint32_t)pBuffIn[1]<<16 \
                                                                        | (uint32_t)pBuffIn[2]<<8 | (uint32_t)pBuffIn[3];
                pOtaDsmInfo->m_nDsiGroupSize = (uint32_t)pBuffIn[4]<<24 | (uint32_t)pBuffIn[5]<<16 \
                                                                        | (uint32_t)pBuffIn[6]<<8 | (uint32_t)pBuffIn[7];
                GxOtaPrintf("\n--dsi---\nDsiGroupId:0x%x  \nDsiGroupSize:0x%x  ",\
                                                        pOtaDsmInfo->m_nDsiGroupId, pOtaDsmInfo->m_nDsiGroupSize);
                wDsiComLength = (uint16_t)pBuffIn[8]<<8 | pBuffIn[9];
                wDsiCnt = (uint16_t)pBuffIn[10]<<8 | pBuffIn[11];
                pBuffIn += 12;
                for(wTmpCnt = 0; wTmpCnt < wDsiCnt; wTmpCnt++)
                {
                        chDsiDataLength = pBuffIn[1];
                        if(0x01 == pBuffIn[0])//hw
                        {
                                nDsiOui = (((uint32_t)pBuffIn[2]<<24)&(0x00)) | (uint32_t)pBuffIn[3]<<16 \
                                                | (uint32_t)pBuffIn[4]<<8 | (uint32_t)pBuffIn[5];
                                wDsiHwModule = (uint16_t)pBuffIn[6]<<8 | pBuffIn[7];
                                wDsiHwVersion = (uint16_t)pBuffIn[8]<<8 | pBuffIn[9];
                                GxOtaPrintf("\nDsiOui:0x%x  \nDsiHwModule:0x%x  \nDsiHwVersion:0x%x",\
                                                        nDsiOui, wDsiHwModule, wDsiHwVersion);
                        }
                        else if(0x02 == pBuffIn[0])//sw
                        {
                                nDsiOui = (((uint32_t)pBuffIn[2]<<24)&(0x00)) | (uint32_t)pBuffIn[3]<<16 \
                                                | (uint32_t)pBuffIn[4]<<8 | (uint32_t)pBuffIn[5];
                                wDsiSwModule = (uint16_t)pBuffIn[6]<<8 | pBuffIn[7];
                                wDsiSwVersion = (uint16_t)pBuffIn[8]<<8 | pBuffIn[9];
                                GxOtaPrintf("  \nDsiSwModule:0x%x  \nDsiSwVersion:0x%x  ",\
                                                        wDsiSwModule, wDsiSwVersion);
                        }
                        else
                        {
                                ;//reserve
                        }
                        pBuffIn += chDsiDataLength + 2;
                }
                g_info_len = (uint16_t)pBuffIn[0]<<8 | pBuffIn[1];
                pBuffIn += g_info_len+2;

		if(nDsiOui == pOtaDsmInfo->m_nDsiOUI 
			&& wDsiHwModule == pOtaDsmInfo->m_nDsiHwModule
			&& wDsiHwVersion == pOtaDsmInfo->m_nDsiHwVersion
			&& wDsiSwModule == pOtaDsmInfo->m_nDsiSwModule)
		{
			if(wDsiSwVersion > pOtaDsmInfo->m_nDsiSwVersion)
			{
				GxOtaPrintf("--dsi---find the soft version is new\n");
				
				//GxCore_Malloc the Buffer for OTA Upgrade
				if(0 == pOtaDsmInfo->m_nDsiGroupSize)
				{
					GxOtaPrintf("\n m_nDsiGroupSize error!!!!\n");
					return -1;
				}
				sp_SectionData = app_ota_malloc(pOtaDsmInfo->m_nDsiGroupSize, (void*)&hw_malloc_block);//GxCore_Malloc(pOtaDsmInfo->m_nDsiGroupSize);
				if(sp_SectionData==NULL)
				{
					GxOtaPrintf("\nGxCore_Malloc error!!!!\n");
					return -1;
				}
				else
				{
					GxOtaPrintf("\nMalloc Successful!!!!:0x%08x\n",pOtaDsmInfo->m_nDsiGroupSize);
					memset(sp_SectionData,0,pOtaDsmInfo->m_nDsiGroupSize);
                    thiz_new_version = wDsiSwVersion;
				}
				
				return 1;//OTA_DSI_VERSION_IS_NEW
			}
			else
			{
				GxOtaPrintf("--dsi---find the soft version is not new\n");
				ret_value = OTA_DSI_VERSION_IS_OLD;
				break;
			}
		}
		else
		{
			GxOtaPrintf("--dsi---do not maching\n");
			pBuffIn += 2;//Notice there!
		}
	}
	return ret_value;
}

static void app_ota_dsi_init(void)
{
	int32_t i = 0;
    unsigned int pid = 0;
    if(app_ota_get_data_pid(&pid))
    {
        printf("\nMENU OTA, error, %s, %d\n",__FUNCTION__,__LINE__);
        return ;
    }
    thiz_new_version = 0;

	g_AppOtaDsi.pid = pid;
	g_AppOtaDsi.filter_count = 1;
	g_AppOtaDsi.enable = 1;
	g_AppOtaDii.enable = 0;
	g_AppOtaDdb.enable = 0;
	memset(g_AppOtaDsi.ota_sub,0,sizeof(AppOtaSub_t)*MAX_OTA_MODULE_NUM);
	for (i=0; i<MAX_OTA_MODULE_NUM; i++)
	{
		g_AppOtaDsi.ota_sub[i].subt_id = -1;
	}
}


static uint32_t app_ota_stop(AppOta *ota)//For DSI/DII/DDB
{
	int32_t i = 0;
	for(i = 0; i < ota->filter_count; i++)
	{
		if(ota->ota_sub[i].subt_id != -1)
		{
			GxSubTableDetail subt_detail = {0};
			GxMsgProperty_SiGet si_get;
			GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)ota->ota_sub[i].subt_id;

			subt_detail.si_subtable_id = ota->ota_sub[i].subt_id;
			si_get = &subt_detail;
			app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void*)&si_get);

			app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
			ota->ota_sub[i].subt_id = -1;
		}
	}

	memset(ota->ota_sub,0,sizeof(AppOtaSub_t)*MAX_OTA_MODULE_NUM);
	for (i=0; i<MAX_OTA_MODULE_NUM; i++)
	{
		ota->ota_sub[i].subt_id = -1;
	}

	return 0;
}

static void app_ota_dsi_start(AppOta *ota_dsi)
{
	int32_t i = 0;
	GxSubTableDetail subt_detail = {0};
	GxMsgProperty_SiCreate  params_create;
	GxMsgProperty_SiStart params_start;

	//create Filter for OTA Dsi
	for(i = 0; i < ota_dsi->filter_count; i++)
	{
		subt_detail.ts_src = (uint16_t)ota_dsi->ts_src;
		subt_detail.si_filter.pid = (uint16_t)ota_dsi->pid;
		subt_detail.si_filter.match_depth = 12;
		subt_detail.si_filter.crc = CRC_OFF;
		subt_detail.si_filter.eq_or_neq = EQ_MATCH;
		subt_detail.si_filter.match[0] = APP_OTA_DSI_TABLE_ID;
		subt_detail.si_filter.mask[0] = 0xff;
		subt_detail.si_filter.match[8] = 0x11;
		subt_detail.si_filter.mask[8] = 0xff;
		subt_detail.si_filter.match[9] = 0x03;
		subt_detail.si_filter.mask[9] = 0xff;
		subt_detail.si_filter.match[10] = 0x10;
		subt_detail.si_filter.mask[10] = 0xff;
		subt_detail.si_filter.match[11] = 0x06;
		subt_detail.si_filter.mask[11] = 0xff;
		subt_detail.time_out = 50000000*2;//50S

		subt_detail.table_parse_cfg.mode = PARSE_SECTION_ONLY;
		subt_detail.table_parse_cfg.table_parse_fun = NULL;

		params_create = &subt_detail;
		app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);

		//Get the return "si_subtable_id"
		ota_dsi->ota_sub[i].subt_id = subt_detail.si_subtable_id;
		ota_dsi->ota_sub[i].req_id = subt_detail.request_id;

		params_start = ota_dsi->ota_sub[i].subt_id;
		//for test
		GxOtaPrintf("---[dsi][start]---\n");
		GxOtaPrintf("---[ts_src = %d][pid = 0x%x][subt_id = %d][req_id = %d]---\n",
		subt_detail.ts_src,subt_detail.si_filter.pid,ota_dsi->ota_sub[i].subt_id,ota_dsi->ota_sub[i].req_id);
		GxOtaPrintf("---[dsi][end]---\n");
		app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);

		ota_dsi->ota_sub[i].ver = 0xff;
	}
 }

static status_t app_ota_dsi_analyse(AppOta *ota_dsi, GxMsgProperty_SiSubtableOk *data)
{
	int32_t i = 0;
	int32_t value = 0;

	for(i = 0; i < ota_dsi->filter_count; i++)
	{
		if (data->si_subtable_id == ota_dsi->ota_sub[i].subt_id
			&& data->parsed_data[0] == APP_OTA_DSI_TABLE_ID
			&& data->parsed_data[11] == 0x06
			&& data->request_id == ota_dsi->ota_sub[i].req_id)
		{
			value = _ota_dsm_parse_dsi(data->parsed_data,&OtaDsmInfo);

			GxBus_SiParserBufFree(data->parsed_data);
			ota_dsi->stop(ota_dsi);
			if(value == OTA_DSI_VERSION_IS_OLD)
			{
				app_ota_upgrade_ver_old();
				return value;
			}
			g_AppOtaDii.init();
			g_AppOtaDii.start(&g_AppOtaDii);
		}
	}
	return GXCORE_SUCCESS;
}

#define SUB_FUNC_DII
static void _ota_dsm_parse_dii( uint8_t *pBuffIn, OtaDsmInfo_t* pOtaDsmInfo)
{
        uint8_t chAdaptationLength = 0;
        uint8_t chModuleNum = 0;
        uint8_t chModuleInfoLength = 0;
        uint8_t chModuleInfoDesTag = 0;

        pBuffIn += 8;//DSM-CC section header
        chAdaptationLength = pBuffIn[9];//Adap_len always is 0
        pBuffIn += 12;//DSM-CC Message header
        pBuffIn += chAdaptationLength;//

        pOtaDsmInfo->OtaDiiInfo.m_nDiiDownloadId = (uint32_t)pBuffIn[0]<<24 \
                                                                                        | (uint32_t)pBuffIn[1]<<16 \
                                                                                        | (uint32_t)pBuffIn[2]<<8 \
                                                                                        | (uint32_t)pBuffIn[3]; 
        pOtaDsmInfo->OtaDiiInfo.m_wDiiBlockSize = (uint16_t)pBuffIn[4]<<8 \
                                                                                        | (uint16_t)pBuffIn[5];
        pBuffIn += (12 + 6);
        pOtaDsmInfo->OtaDiiInfo.m_wDiiModuleNum = (uint16_t)pBuffIn[0]<<8 \
                                                                                        | (uint16_t)pBuffIn[1];
        GxOtaPrintf("----dii--DiiDownloadId :0x%x \nDiiBlockSize :0x%x \nDiiModuleNum :0x%x\n",\
                                pOtaDsmInfo->OtaDiiInfo.m_nDiiDownloadId, \
                                pOtaDsmInfo->OtaDiiInfo.m_wDiiBlockSize, \
                                pOtaDsmInfo->OtaDiiInfo.m_wDiiModuleNum);
        pBuffIn += 2;

        for(chModuleNum = 0; chModuleNum < pOtaDsmInfo->OtaDiiInfo.m_wDiiModuleNum; chModuleNum++)
        {
                chModuleInfoLength = pBuffIn[7];
                pOtaDsmInfo->OtaDiiInfo.m_OtaDiiModule[chModuleNum].m_wDiiModuleId = 
                                                                                                (uint16_t)pBuffIn[0]<<8 \
                                                                                                | (uint16_t)pBuffIn[1];
                pOtaDsmInfo->OtaDiiInfo.m_OtaDiiModule[chModuleNum].m_nDiiModuleSize = 
                                                                                                (uint32_t)pBuffIn[2]<<24 \
                                                                                                | (uint32_t)pBuffIn[3]<<16 \
                                                                                                | (uint32_t)pBuffIn[4]<<8 \
                                                                                                | (uint32_t)pBuffIn[5];
                GxOtaPrintf("\r\n----dii--Get Info of ModuleNum:%d \nDiiModuleId :0x%x \nDiiModuleSize :0x%x \r\n",\
                                chModuleNum, \
                                pOtaDsmInfo->OtaDiiInfo.m_OtaDiiModule[chModuleNum].m_wDiiModuleId, \
                                pOtaDsmInfo->OtaDiiInfo.m_OtaDiiModule[chModuleNum].m_nDiiModuleSize);

                chModuleInfoDesTag = pBuffIn[8];
                pBuffIn += (chModuleInfoLength + 8);
        }
}

static void app_ota_dii_init(void)
{
	int32_t i = 0;
    unsigned int pid = 0;
    if(app_ota_get_data_pid(&pid))
    {
        printf("\nMENU OTA, error, %s, %d\n",__FUNCTION__,__LINE__);
        return ;
    }

	g_AppOtaDii.pid = pid;
	g_AppOtaDii.filter_count = 1;
	g_AppOtaDsi.enable = 0;
	g_AppOtaDii.enable = 1;
	g_AppOtaDdb.enable = 0;

	memset(g_AppOtaDii.ota_sub,0,sizeof(AppOtaSub_t)*MAX_OTA_MODULE_NUM);
	for (i=0; i<MAX_OTA_MODULE_NUM; i++)
	{
		g_AppOtaDii.ota_sub[i].subt_id = -1;
	}
}

static void app_ota_dii_start(AppOta *ota_dii)
{
	int32_t i = 0;
	GxSubTableDetail subt_detail = {0};
	GxMsgProperty_SiCreate  params_create;
	GxMsgProperty_SiStart params_start;

	//create Filter for OTA Dsi
	for(i = 0; i < ota_dii->filter_count; i++)
	{
		subt_detail.ts_src = (uint16_t)ota_dii->ts_src;
		subt_detail.si_filter.pid = (uint16_t)ota_dii->pid;
		subt_detail.si_filter.match_depth = 16;
		subt_detail.si_filter.eq_or_neq = EQ_MATCH;
		subt_detail.si_filter.match[0] = APP_OTA_DII_TABLE_ID;
		subt_detail.si_filter.mask[0] = 0xff;
		subt_detail.si_filter.match[8] = 0x11;
		subt_detail.si_filter.mask[8] = 0xff;
		subt_detail.si_filter.match[9] = 0x03;
		subt_detail.si_filter.mask[9] = 0xff;
		subt_detail.si_filter.match[10] = 0x10;
		subt_detail.si_filter.mask[10] = 0xff;
		subt_detail.si_filter.match[11] = 0x02;
		subt_detail.si_filter.mask[11] = 0xff;

		subt_detail.si_filter.match[12] = (OtaDsmInfo.m_nDsiGroupId>>24)&0xFF;
		subt_detail.si_filter.mask[12] = 0xff;
		subt_detail.si_filter.match[13] = (OtaDsmInfo.m_nDsiGroupId>>16)&0xFF;
		subt_detail.si_filter.mask[13] = 0xff;
		subt_detail.si_filter.match[14] = (OtaDsmInfo.m_nDsiGroupId>>8)&0xFF;
		subt_detail.si_filter.mask[14] = 0xff;
		subt_detail.si_filter.match[15] = (OtaDsmInfo.m_nDsiGroupId)&0xFF;
		subt_detail.si_filter.mask[15] = 0xff;

		subt_detail.time_out = 50000000;//50S

		subt_detail.table_parse_cfg.mode = PARSE_SECTION_ONLY;
		subt_detail.table_parse_cfg.table_parse_fun = NULL;

		params_create = &subt_detail;
		app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);

		//Get the return "si_subtable_id"
		ota_dii->ota_sub[i].subt_id = subt_detail.si_subtable_id;
		ota_dii->ota_sub[i].req_id = subt_detail.request_id;

		params_start = ota_dii->ota_sub[i].subt_id;
		//for test
		GxOtaPrintf("---[dii][start]---\n");
		GxOtaPrintf("---[ts_src = %d][pid = 0x%x][subt_id = %d][req_id = %d]---\n",
		subt_detail.ts_src,subt_detail.si_filter.pid,ota_dii->ota_sub[i].subt_id,ota_dii->ota_sub[i].req_id);
		GxOtaPrintf("---[dii][end]---\n");
		app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);

		ota_dii->ota_sub[i].ver = 0xff;
	}
 }

static status_t app_ota_dii_analyse(AppOta *ota_dii, GxMsgProperty_SiSubtableOk *data)
{
	int32_t i = 0;

	for(i = 0; i < ota_dii->filter_count; i++)
	{
		if (data->si_subtable_id == ota_dii->ota_sub[i].subt_id
			&& data->parsed_data[0] == APP_OTA_DII_TABLE_ID
			&& data->parsed_data[11] == 0x02
			&& data->request_id == ota_dii->ota_sub[i].req_id)
		{
			_ota_dsm_parse_dii(data->parsed_data,&OtaDsmInfo);
			GxBus_SiParserBufFree(data->parsed_data);
			ota_dii->stop(ota_dii);
			g_AppOtaDdb.init();
			g_AppOtaDdb.start(&g_AppOtaDdb);
		}
	}
	return GXCORE_SUCCESS;
}

#define SUB_FUNC_DDB
static uint32_t _ota_close_ddb_filter(uint16_t wNum)//For DDB
{
	if(g_AppOtaDdb.ota_sub[wNum].subt_id != -1)
	{
		GxSubTableDetail subt_detail = {0};
		GxMsgProperty_SiGet si_get;
		GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)g_AppOtaDdb.ota_sub[wNum].subt_id;

		subt_detail.si_subtable_id = g_AppOtaDdb.ota_sub[wNum].subt_id;
		si_get = &subt_detail;
		app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void*)&si_get);

		app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
		g_AppOtaDdb.ota_sub[wNum].subt_id = -1;
	}
	return 0;
}

static int32_t _ota_get_process(OtaDsmInfo_t* pOtaDsmInfo)
{
	uint16_t wTemp = 0;
	uint16_t wTamp = 0;
	int32_t nMaxBlocNum = 0;
	int32_t nAcceptBlocNum = 0;

	for(wTemp = 0; wTemp < pOtaDsmInfo->OtaDiiInfo.m_wDiiModuleNum; wTemp++)
	{
		nMaxBlocNum += pOtaDsmInfo->m_OtaDdbInfo.m_DdbModule[wTemp].m_wBlockNum;
		for(wTamp = 0; wTamp <= pOtaDsmInfo->m_OtaDdbInfo.m_DdbModule[wTemp].m_wBlockNum; wTamp++)
		{
			if(pOtaDsmInfo->m_OtaDdbInfo.m_DdbModule[wTemp].m_wDdbBlockAcceptFlg[wTamp] == 1)
			nAcceptBlocNum ++;
		}
		if(nMaxBlocNum <= wTemp*255)
			nMaxBlocNum +=255;
	}
	if(nAcceptBlocNum == 0)
	{
		return 0;
	}
	else
	{
		return (nAcceptBlocNum*1000)/nMaxBlocNum/10;
	}
}

static int32_t _ota_get_soft_data(uint8_t *pBuffIn, 
								OtaDsmInfo_t* pOtaDsmInfo,
								uint8_t chDdbMaxBlockNum, 
								uint16_t wBlockNum, 
								uint16_t wModuleId)
{	
	uint32_t nError = GXOTA_STATUS_BLOCK_NOT_FINISH;
	uint16_t wTemp = 0;
	uint16_t wTamp = 0;
	uint16_t wDdbBlockSize = 0;
	uint32_t nOfferBuffer = 0;

	for(wTemp = 0; wTemp < pOtaDsmInfo->OtaDiiInfo.m_wDiiModuleNum; wTemp++)
	{
		if(wModuleId == pOtaDsmInfo->OtaDiiInfo.m_OtaDiiModule[wTemp].m_wDiiModuleId)
		{	
			if(pOtaDsmInfo->m_OtaDdbInfo.m_DdbModule[wTemp].m_wDdbBlockAcceptFlg[wBlockNum] == 1)
			{
				return GXOTA_STATUS_BLOCK_HAS_BEEN_DEALWITH;
			}
			
			GxOtaPrintf("\n---dsm--parse DdbModuleId: 0x%x  DdbBlockNum: 0x%x DdbMaxBlockNum: 0x%x\n",wModuleId, wBlockNum, chDdbMaxBlockNum);
			pOtaDsmInfo->m_OtaDdbInfo.m_DdbModule[wTemp].m_wDdbBlockAcceptFlg[wBlockNum] = 1;
			pOtaDsmInfo->m_OtaDdbInfo.m_DdbModule[wTemp].m_wBlockNum = chDdbMaxBlockNum;
			for(wTamp = 0; wTamp <= chDdbMaxBlockNum; wTamp++)
			{
				if(pOtaDsmInfo->m_OtaDdbInfo.m_DdbModule[wTemp].m_wDdbBlockAcceptFlg[wTamp] == 1)
				{					
					;//OTA_DSM_DDB_BLOCK_FINISH;
				}
				else
				{
					GxOtaPrintf("---ddb-- block not finish\n");
					nError = GXOTA_STATUS_BLOCK_NOT_FINISH;
					break;
				}
			}
			if(wTamp > chDdbMaxBlockNum)
			{
				printf("---ddb-- block finish\n");
				pOtaDsmInfo->m_OtaDdbInfo.m_wModuleAcceptFlg[wTemp] = 1;
				_ota_close_ddb_filter(wTemp);
				nError = GXOTA_STATUS_BLOCK_FINISH;
			}
			wTemp++;
			break;
		}
		nOfferBuffer += pOtaDsmInfo->OtaDiiInfo.m_OtaDiiModule[wTemp].m_nDiiModuleSize;
	}

	nOfferBuffer += (wBlockNum)*pOtaDsmInfo->OtaDiiInfo.m_wDiiBlockSize;
	GxOtaPrintf("---ddb-- wBlockNum :0x%x\n",wBlockNum);
	if(wBlockNum < chDdbMaxBlockNum)
	{
		wDdbBlockSize = pOtaDsmInfo->OtaDiiInfo.m_wDiiBlockSize;
		GxOtaPrintf("---ddb-- wDdbBlockSize :0x%x\n",wDdbBlockSize);
	}
	else if(wBlockNum == chDdbMaxBlockNum)//modify by Yanwzh 011223
	{
		wDdbBlockSize = pOtaDsmInfo->OtaDiiInfo.m_OtaDiiModule[wTemp - 1].m_nDiiModuleSize \
					- (wBlockNum)*pOtaDsmInfo->OtaDiiInfo.m_wDiiBlockSize;
		GxOtaPrintf("---ddb-- wDdbBlockSize :0x%x\n",wDdbBlockSize);
		GxOtaPrintf("---ddb-- wTemp :0x%x\n",wTemp);
		GxOtaPrintf("---ddb-- pOtaDsmInfo->OtaDiiInfo.m_OtaDiiModule[wTemp-1].m_nDiiModuleSize :0x%x\n",pOtaDsmInfo->OtaDiiInfo.m_OtaDiiModule[wTemp-1].m_nDiiModuleSize);
		GxOtaPrintf("---ddb-- wBlockNum :0x%x\n",wBlockNum);
		GxOtaPrintf("---ddb-- pOtaDsmInfo->OtaDiiInfo.m_wDiiBlockSize :0x%x\n",pOtaDsmInfo->OtaDiiInfo.m_wDiiBlockSize);
	}

	memcpy(sp_SectionData+nOfferBuffer,pBuffIn,wDdbBlockSize);

	if(nError == GXOTA_STATUS_BLOCK_FINISH)
	{
		for(wTemp = 0; wTemp < pOtaDsmInfo->OtaDiiInfo.m_wDiiModuleNum; wTemp++)
		{
			if(pOtaDsmInfo->m_OtaDdbInfo.m_wModuleAcceptFlg[wTemp] == 1)
			{
				;//OTA_DSM_DDB_MODULE_FINISH;
			}
			else
			{
				//GxOtaPrintf("[ota] ---ddb-- module not finish\n");
				nError = GXOTA_STATUS_MODULE_NOT_FINISH;
				break;
			}
		}
		if(nError == GXOTA_STATUS_BLOCK_FINISH)
		{
			printf("---ddb-- module finish\n");
			nError = GXOTA_STATUS_MODULE_FINISH;
		}
	}

	return nError;
		
}

static int32_t _ota_dsm_parse_ddb( uint8_t *pBuffIn, OtaDsmInfo_t* pOtaDsmInfo)
{   
	uint8_t chAdaptationLength = 0;
	uint8_t chDdbMaxBlockNum = 0;
	uint16_t wDdbModuleId = 0;
	uint16_t wDdbBlockNum = 0;
	uint16_t wSectionLength = 0;
	uint32_t nReturn = 0;

	wSectionLength = (((uint16_t)pBuffIn[1] << 8 ) | pBuffIn[2] ) & 0x0FFF;
	chDdbMaxBlockNum = pBuffIn[7];
	pBuffIn += 8;//DSM-CC section header
	chAdaptationLength = pBuffIn[9];//Adap_len always is 0
	pBuffIn += 12;//DSM-CC Message header
	pBuffIn += chAdaptationLength;//

	wDdbModuleId = (uint16_t)pBuffIn[0]<<8 | (uint16_t)pBuffIn[1];
	wDdbBlockNum = (uint16_t)pBuffIn[4]<<8 | (uint16_t)pBuffIn[5];

	pBuffIn += 6;

	nReturn = _ota_get_soft_data(pBuffIn, pOtaDsmInfo, chDdbMaxBlockNum, wDdbBlockNum, wDdbModuleId);
	if(GXOTA_STATUS_MODULE_FINISH == nReturn)
	{
		GxOtaPrintf("---Module-- Module finish\n");
		app_ota_process_update(100);
		GxOtaPrintf("\r\n finish!!!!!!!!!!!!!!!!!!!!!");
		return 1;
	}
	else 
	{			
		app_ota_process_update(_ota_get_process(pOtaDsmInfo));
		GxOtaPrintf("\n");
		return 0;
	}
}

static void app_ota_ddb_init(void)
{
	int32_t i = 0; 
    unsigned int pid = 0;
    if(app_ota_get_data_pid(&pid))
    {
        printf("\nMENU OTA, error, %s, %d\n",__FUNCTION__,__LINE__);
        return ;
    }
	g_AppOtaDdb.pid = pid;
	g_AppOtaDdb.filter_count = OtaDsmInfo.OtaDiiInfo.m_wDiiModuleNum;
	g_AppOtaDsi.enable = 0;
	g_AppOtaDii.enable = 0;
	g_AppOtaDdb.enable = 1;

	memset(g_AppOtaDdb.ota_sub,0,sizeof(AppOtaSub_t)*MAX_OTA_MODULE_NUM);
	for (i=0; i<MAX_OTA_MODULE_NUM; i++)
	{
		g_AppOtaDdb.ota_sub[i].subt_id = -1;
	}
}

static void app_ota_ddb_start(AppOta *ota_ddb)
{
	int32_t i = 0; 
	GxSubTableDetail subt_detail = {0};
	GxMsgProperty_SiCreate  params_create;
	GxMsgProperty_SiStart params_start;


	for (i = 0; i < ota_ddb->filter_count; i++)
	{
		//create Filter for OTA Dsi
		subt_detail.ts_src = (uint16_t)ota_ddb->ts_src;
		subt_detail.si_filter.pid = (uint16_t)ota_ddb->pid;
		subt_detail.si_filter.match_depth = 16;
		subt_detail.si_filter.eq_or_neq = EQ_MATCH;
		subt_detail.si_filter.match[0] = APP_OTA_DDB_TABLE_ID;
		subt_detail.si_filter.mask[0] = 0xff;

		subt_detail.si_filter.match[3] = (uint8_t)((OtaDsmInfo.OtaDiiInfo.m_OtaDiiModule[i].m_wDiiModuleId)>>8);
		subt_detail.si_filter.mask[3] = 0xff;
		subt_detail.si_filter.match[4] = (uint8_t)((OtaDsmInfo.OtaDiiInfo.m_OtaDiiModule[i].m_wDiiModuleId)&0xff);
		subt_detail.si_filter.mask[4] = 0xff;
		
		subt_detail.si_filter.match[8] = 0x11;
		subt_detail.si_filter.mask[8] = 0xff;
		subt_detail.si_filter.match[9] = 0x03;
		subt_detail.si_filter.mask[9] = 0xff;
		subt_detail.si_filter.match[10] = 0x10;
		subt_detail.si_filter.mask[10] = 0xff;
		subt_detail.si_filter.match[11] = 0x03;
		subt_detail.si_filter.mask[11] = 0xff;

		subt_detail.si_filter.match[12] = (OtaDsmInfo.OtaDiiInfo.m_nDiiDownloadId>>24)&0xFF;
		subt_detail.si_filter.mask[12] = 0xff;
		subt_detail.si_filter.match[13] = (OtaDsmInfo.OtaDiiInfo.m_nDiiDownloadId>>16)&0xFF;
		subt_detail.si_filter.mask[13] = 0xff;
		subt_detail.si_filter.match[14] = (OtaDsmInfo.OtaDiiInfo.m_nDiiDownloadId>>8)&0xFF;
		subt_detail.si_filter.mask[14] = 0xff;
		subt_detail.si_filter.match[15] = (OtaDsmInfo.OtaDiiInfo.m_nDiiDownloadId)&0xFF;
		subt_detail.si_filter.mask[15] = 0xff;

		subt_detail.time_out = 50000000;//50S

		subt_detail.table_parse_cfg.mode = PARSE_SECTION_ONLY;
		subt_detail.table_parse_cfg.table_parse_fun = NULL;

		params_create = &subt_detail;
		app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);

		//Get the return "si_subtable_id"
		ota_ddb->ota_sub[i].subt_id = subt_detail.si_subtable_id;
		ota_ddb->ota_sub[i].req_id = subt_detail.request_id;

		params_start = ota_ddb->ota_sub[i].subt_id;
		//for test
		GxOtaPrintf("---[ddb][start]---\n");
		GxOtaPrintf("---[ts_src = %d][pid = 0x%x][subt_id = %d][req_id = %d]---\n",
		subt_detail.ts_src,subt_detail.si_filter.pid,ota_ddb->ota_sub[i].subt_id,ota_ddb->ota_sub[i].req_id);
		GxOtaPrintf("---[ddb][end]---\n");
		app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);

		ota_ddb->ota_sub[i].ver = 0xff;
	}
}

static status_t app_ota_ddb_analyse(AppOta *ota_ddb, GxMsgProperty_SiSubtableOk *data)
{
	int32_t i = 0;
	int32_t nReturn = 0;

	for(i = 0; i < ota_ddb->filter_count; i++)
	{
		if (data->si_subtable_id == ota_ddb->ota_sub[i].subt_id
			&& data->parsed_data[0] == APP_OTA_DDB_TABLE_ID
			&& data->parsed_data[11] == 0x03
			&& data->request_id == ota_ddb->ota_sub[i].req_id)
		{
			nReturn = _ota_dsm_parse_ddb(data->parsed_data,&OtaDsmInfo);
			GxBus_SiParserBufFree(data->parsed_data);
			if(1 == nReturn)// Data Finish!
			{
				app_ota_message_update(STR_ID_DATA_COMPLETED, STR_BLANK);
				ota_ddb->stop(ota_ddb);
				g_chFlagEntryFlash = 1;//½øÈëFlash²Ù×÷!
				app_ota_message_update(STR_UPGRADE_SUCCESS, STR_REBOOT_NOW);
				app_ota_message_update(STR_UPGRADE_SUCCESS, STR_UPGRADE_SUCCESS);
				app_ota_message_update(STR_UPGRADE, STR_DONT_CUT_PWER);
                GUI_SetInterface("flush", NULL);
// recover the config
                //GxBus_ConfigSetInt("ota>flag", 1);
#if 0
				if(1 == g_chDefaultDataUpdate)//Program channel
				{
					app_ota_defaultdata_update();
				}
				else if(1 == g_chLogoUpdate)//Logo
				{
					app_ota_logo_update();
				}
				else//ALL or other segment
				{
					app_ota_get_segment();
				}
#else
                nReturn = app_ota_check_and_flush(thiz_new_version);
#endif
                if(-1 != nReturn)
                {
                    g_chBissUpdate = 0;
                    g_chDefaultDataUpdate = 0;
                    g_chLogoUpdate = 0;
                    app_ota_message_update(STR_UPGRADE_SUCCESS, STR_REBOOT_NOW);
                    GUI_SetInterface("flush", NULL);
                    //reboot
                    GxCore_ThreadDelay(500);
                    {
                        PopDlg pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.mode = POP_MODE_BLOCK;
                        pop.str = STR_UPGRADE_SUCCESS;
                        pop.timeout_sec = 3;
                        popdlg_create(&pop);
                    }
                }
                else
                {
                    app_ota_message_update(STR_ID_UPDATE_FAILED, STR_REBOOT_NOW);
                    GUI_SetInterface("flush", NULL);
                    //reboot
                    GxCore_ThreadDelay(500);
                    {
                        PopDlg pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.mode = POP_MODE_BLOCK;
                        pop.str = STR_ID_UPDATE_FAILED;
                        pop.timeout_sec = 3;
                        popdlg_create(&pop);
                    }

                }
                app_reboot();
			}
		}
	}
	return GXCORE_SUCCESS;
}


AppOta g_AppOtaDsi =
{
	.filter_count	= 1,
	.start	        = app_ota_dsi_start,
	.stop	        = app_ota_stop,
	.analyse	    = app_ota_dsi_analyse,
	.init			= app_ota_dsi_init,
};

AppOta g_AppOtaDii =
{
	.filter_count	= 1,
	.start	        = app_ota_dii_start,
	.stop	        = app_ota_stop,
	.analyse	    = app_ota_dii_analyse,
	.init			= app_ota_dii_init,
};

AppOta g_AppOtaDdb =
{
	.filter_count	= 1,
	.start	        = app_ota_ddb_start,
	.stop	        = app_ota_stop,
	.analyse	    = app_ota_ddb_analyse,
	.init			= app_ota_ddb_init,
};
#endif

