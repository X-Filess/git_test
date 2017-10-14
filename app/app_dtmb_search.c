/*****************************************************************************
 * 			  CONFIDENTIAL								
 *        Hangzhou GuoXin Science and Technology Co., Ltd.             
 *                      (C)2009-2014, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	app_dtmb_search.c
 * Author    : 	baiyingshan
 * Project   :	dvbt2 Foundation
 ******************************************************************************
 * Purpose   :	
 ******************************************************************************
 * Release History:
 VERSION	Date			  AUTHOR         Description
 1.0  	2014.06	          		 Dugan Du    	Create   
 *****************************************************************************/
//public header files
#include "app.h"

#if (DEMOD_DTMB > 0)
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "app_frontend.h"
#include "app_pop.h"
#include "app_epg.h"
#include "full_screen.h"
#include "app_book.h"

#include "app_wnd_system_setting_opt.h"
//#include "app_dtmb_search.h"

#define DEBUG_SEARCH
#ifdef DEBUG_SEARCH
#define _search_printf		printf
#else
#define _search_printf
#endif

#define MAX_NIT_SECTION_NUM						(32)
#define TP_MAX_NUM									(200)
#define PAT_FILTER_TIMEOUT							(3000)
#define SDT_FILTER_TIMEOUT							(5000)
#define NIT_FILTER_TIMEOUT							(10000)
#define PMT_FILTER_TIMEOUT							(8000)

#define MAIN_FREQ_DEFAULT_VALUE		        (850000000)
#define MAIN_BANDWIDTH_DEFAULT_VALUE		(0)

#define BOX_DTMB_OPTIONS	         "box_dtmb_search_options"
#define TXT_DTMB_CHANNEL             "text_dtmb_search_channel"
#define TXT_DTMB_FREQ                "text_dtmb_search_freq"
#define TXT_DTMB_BANDWIDTH           "text_dtmb_search_bandwidth"
#define CMB_DTMB_SEARCH_CH	         "cmbox_dtmb_search_channel"
#define EDIT_DTMB_SEARCH_FRE	     "edit_dtmb_search_fre"
#define CMB_DTMB_SEARCH_BANDWIDTH    "cmbox_dtmb_search_bandwidth"
#define EDIT_DTMB_SEARCH_ENDFRE      "edit_search_setting_dtmb_endfre"
#define CMB_DTMB_SEARCH_MODE         "cmbox_dtmb_search_mode"
#define CMB_DTMB_SEARCH_FTA          "cmbox_dtmb_search_fta"

#define VHF_MINI_BAND	174.0
#define VHF_MAX_BAND	230.0
#define UHF_MINI_BAND	470.0
#define UHF_MAX_BAND	862.0
#define VHF_MINI_BAND_STR	"174.0"
#define VHF_MAX_BAND_STR	"230.0"
#define UHF_MINI_BAND_STR	"470.0"
#define UHF_MAX_BAND_STR	"862.0"

// networ name
#define DTMB_NETWORK_NAME "DTMB"

#define DTMB_MAX_LEN	(5)

#define DTMB_START_FREQ_STR "115.0"
#define DTMB_END_FREQ_STR   "858.0"
#define DTMB_START_FREQ     (115.0)
#define DTMB_END_FREQ       (858.0)

typedef struct
{
    uint8_t  channel[DTMB_MAX_LEN];
    float freq;
    fe_bandwidth_t band;//bandwidth(0:8M, 1:6M)
}DefaultFreqCh;

static DefaultFreqCh default1_freq_channel[]=
{
//	{"1",52.5,0},	//the first one as main freq is set to nit search when auto scan.by dugan
//    {"2",60.5,0},
//    {"3",68.5,0},
//    {"4",80.0,0},
//    {"5",88.0,0},
    {"6",171.0,0},
    {"7",179.0,0},
    {"8",187.0,0},
    {"9",195.0,0},
    {"10",203.0,0},
    {"11",211.0,0},
    {"12",219.0,0},
    {"13",474.0,0},
    {"14",482.0,0},
    {"15",490.0,0},
    {"16",498.0,0},
    {"17",506.0,0},
    {"18",514.0,0},
    {"19",522.0,0},
    {"20",530.0,0},
    {"21",538.0,0},
    {"22",546.0,0},
    {"23",554.0,0},
    {"24",562.0,0},
    {"25",570.0,0},
    {"26",578.0,0},
    {"27",586.0,0},
    {"28",594.0,0},
    {"29",602.0,0},
    {"30",610.0,0},
    {"31",618.0,0},
    {"32",626.0,0},
    {"33",634.0,0},
    {"34",642.0,0},
    {"35",650.0,0},
    {"36",658.0,0},
    {"37",666.0,0},
    {"38",774.0,0},
    {"39",782.0,0},
    {"40",690.0,0},
    {"41",698.0,0},
    {"42",706.0,0},
    {"43",714.0,0},
    {"44",722.0,0},
    {"45",730.0,0},
    {"46",738.0,0},
    {"47",746.0,0},
    {"48",754.0,0},
    {"49",762.0,0},
    {"50",770.0,0},
    {"51",778.0,0},
    {"52",786.0,0},
    {"53",794.0,0},
    {"54",802.0,0},
    {"55",810.0,0},
    {"56",818.0,0},
    {"57",826.0,0},
    {"58",834.0,0},
    {"59",842.0,0},
    {"60",850.0,0},
    {"61",858.0,0},
};

enum BOX_MANUAL_ITEM_NAME
{
    DTMB_SEARCH_BOX_ITEM_CH = 0,
    DTMB_SEARCH_BOX_ITEM_FREQ,
    DTMB_SEARCH_BOX_ITEM_BANDWIDTH,
    DTMB_SEARCH_BOX_ITEM_MODE,
    DTMB_SEARCH_BOX_ITEM_OK,
};

enum
{
    ITEM_DTMB_SEARCH_1,
    ITEM_DTMB_SEARCH_2,
    ITEM_DTMB_SEARCH_3,
    ITEM_DTMB_SEARCH_4,
    ITEM_DTMB_SEARCH_5,
    ITEM_DTMB_SEARCH_TOTAL
};

enum DTMB_ITEM_NAME
{
    ITEM_DTMB_AUTO_SEARCH = 0,
    ITEM_DTMB_SEARCH,
    ITEM_DTMB_FULL_BAND_SEARCH,
    ITEM_DTMB_TOTAL,
};

enum
{
    ITEM_DTMB_ALLBAND = 0,
    ITEM_DTMB_VHF ,
    ITEM_DTMB_UHF,
};

enum DTMB_MODE_NAME
{
	MODE_MANUAL = 0,
	MODE_AUTO,
	MODE_ALLFRE,
};

#ifdef DVB_DTMB_NIT_SUPPORT //nit search need test
typedef struct
{
    uint32_t app_fre_array[TP_MAX_NUM];
    uint32_t app_band_array[TP_MAX_NUM];
    uint32_t app_fre_tsid[TP_MAX_NUM];
    uint16_t num;
    //uint16_t app_annex_type;
    uint8_t nit_recv_num;
    uint8_t	 *nit_subtable_flag;//[MAX_NIT_SECTION_NUM];
    uint8_t nit_flag;
}classDtmbSearchNit;

static classDtmbSearchNit thizDtmbNitResult ;
static volatile uint8_t thizNitParseFlag = 0;
#endif

static DefaultFreqCh *thizFreqChannel = NULL;
static event_list* thizDtmbTimerSignal = NULL;
static event_list* sp_DtmbLockTimer = NULL;
static SetTpMode s_set_tp_mode = SET_TP_AUTO;

static int thizDtmbSatId = 0;
static uint32_t thizDtmbTpidArray[TP_MAX_NUM];
static uint32_t thizDtmbTsidArray[TP_MAX_NUM] ;
static char* manualModeTxt[] = {"Frequency(MHZ)", "Bandwidth(M)"};
static char* allfreModeTxt[] = {"Start frequency(MHZ)", "End frequency(MHZ)"};

extern void app_search_set_search_type(SearchType type);
extern  int app_dtmb_search_cmb_channel_change(GuiWidget *widget, void *usrdata);
#ifdef DVB_DTMB_NIT_SUPPORT
static void dtmb_nit_scan_tp(GxMsgProperty_ScanDtmbStart * p_msg_dtmb_scan);
static uint32_t app_dtmb_nit_auto_search_get_tp(uint32_t sat_id,uint32_t *tp_array);

#endif

static void app_dtmb_search_mode_show(int mode)
{
	if(mode == MODE_MANUAL)
	{
		GUI_SetProperty(TXT_DTMB_FREQ, "string", manualModeTxt[0]);
		GUI_SetProperty(TXT_DTMB_BANDWIDTH, "string", manualModeTxt[1]);

        GUI_SetProperty("boxitem_dtmb_search_channel", "enable", NULL);
		GUI_SetProperty(TXT_DTMB_CHANNEL, "state", "show");
		GUI_SetProperty(CMB_DTMB_SEARCH_CH, "state", "show");
        GUI_SetProperty("boxitem_dtmb_search_freq", "enable", NULL);
        GUI_SetProperty("boxitem_dtmb_search_bandwidth", "enable", NULL);

		GUI_SetProperty(EDIT_DTMB_SEARCH_ENDFRE, "state", "hide");
		GUI_SetProperty(CMB_DTMB_SEARCH_BANDWIDTH, "state", "show");
	}
	else if(mode == MODE_AUTO)
	{
		GUI_SetProperty(TXT_DTMB_FREQ, "string", manualModeTxt[0]);
		GUI_SetProperty(TXT_DTMB_BANDWIDTH, "string", manualModeTxt[1]);

        GUI_SetProperty("boxitem_dtmb_search_channel", "disable", NULL);
		GUI_SetProperty(TXT_DTMB_CHANNEL, "state", "show");
		GUI_SetProperty(CMB_DTMB_SEARCH_CH, "state", "show");
        GUI_SetProperty("boxitem_dtmb_search_freq", "disable", NULL);
        GUI_SetProperty("boxitem_dtmb_search_bandwidth", "disable", NULL);

		GUI_SetProperty(EDIT_DTMB_SEARCH_ENDFRE, "state", "hide");
		GUI_SetProperty(CMB_DTMB_SEARCH_BANDWIDTH, "state", "show");
	}
	else
	{
		GUI_SetProperty(TXT_DTMB_FREQ, "string", allfreModeTxt[0]);
		GUI_SetProperty(TXT_DTMB_BANDWIDTH, "string", allfreModeTxt[1]);

        GUI_SetProperty("boxitem_dtmb_search_channel", "disable", NULL);
        GUI_SetProperty("boxitem_dtmb_search_freq", "enable", NULL);
        GUI_SetProperty("boxitem_dtmb_search_bandwidth", "enable", NULL);
		GUI_SetProperty(TXT_DTMB_CHANNEL, "state", "hide");
		GUI_SetProperty(CMB_DTMB_SEARCH_CH, "state", "hide");
		GUI_SetProperty(EDIT_DTMB_SEARCH_ENDFRE, "state", "show");

		GUI_SetProperty(CMB_DTMB_SEARCH_BANDWIDTH, "state", "hide");
		GUI_SetProperty(EDIT_DTMB_SEARCH_FRE,"string",DTMB_START_FREQ_STR);
		GUI_SetProperty(EDIT_DTMB_SEARCH_ENDFRE, "string", DTMB_END_FREQ_STR);
	}
	return;
}

static int _dtmb_get_channel_list(DefaultFreqCh **ppList)
{
	int Count = 0;
	if(NULL == ppList)
	{
		printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	*ppList = default1_freq_channel;
	Count = (sizeof(default1_freq_channel)/sizeof(DefaultFreqCh));
	return Count;
}

static status_t _dtmb_modify_prog_callback(GxBusPmDataProg* prog)
{
    //for logic num
    return GXCORE_SUCCESS;
}

static void app_dtmb_search_set_tp(int tuner)
{

    AppFrontend_SetTp params = {0};
	int bandwidth[BANDWIDTH_AUTO] = {0,2,2};
    char *Fre_value = NULL;
    uint8_t Width_value = 0;
    GUI_GetProperty(EDIT_DTMB_SEARCH_FRE,"string",&Fre_value);
    GUI_GetProperty(CMB_DTMB_SEARCH_BANDWIDTH,"select",&Width_value);

	if(Width_value >= BANDWIDTH_AUTO)
	{
		Width_value = 0;
		printf("\nDTMB, error, %s, %d\n", __FUNCTION__,__LINE__);
	}
	params.tuner = tuner;
    params.fre = atof(Fre_value)*1000;
    params.type = FRONTEND_DTMB;
    params.qam = bandwidth[Width_value];
    params.type_1501 = DTMB;
	printf("\nDTMB, %s, fre = %d, bandwidth = %d\n",__FUNCTION__,params.fre,params.qam);
   // app_ioctl(tuner, FRONTEND_TP_SET, &params);
   app_set_tp(tuner,&params,SET_TP_FORCE);
}

static int app_dtmb_search_get_sat_id(void)
{
    GxBusPmDataSat sat={0};
    uint32_t num=0;
    int i = 0;
    int32_t ret = 0;

    num = GxBus_PmSatNumGet();
    if(num == 0)
    {
        sat.type = GXBUS_PM_SAT_DTMB;
        sat.tuner = 1;// TODO: need modify
        sat.sat_dtmb.work_mode = GXBUS_PM_SAT_1501_DTMB;
        memcpy(sat.sat_s.sat_name, DTMB_NETWORK_NAME,sizeof(DTMB_NETWORK_NAME));// TODO: use dvbs struct...
        if(GXCORE_SUCCESS != GxBus_PmSatAdd(&sat))
        {
            return -1;
        }
        GxBus_PmSync(GXBUS_PM_SYNC_SAT);
    }
    else
    {
        for(i = 0; i < num; i++)
        {
            memset(&sat, 0, sizeof(GxBusPmDataSat));
            // read one sat from database once
            ret = GxBus_PmSatsGetByPos(i, 1, &sat);
            if( 1 == ret )
            {
                if(GXBUS_PM_SAT_DTMB == sat.type )
                {
                    break;
                }
            }
			else
            {
                _search_printf("[DTMB Search], error, %s, %d\n",__FUNCTION__,__LINE__);
            }
        }
        if(i >= num)
        {
            sat.type = GXBUS_PM_SAT_DTMB;
            sat.tuner = 1;//
            sat.sat_dtmb.work_mode = GXBUS_PM_SAT_1501_DTMB;// for DTMB work mode
            memcpy(sat.sat_s.sat_name, DTMB_NETWORK_NAME,sizeof(DTMB_NETWORK_NAME));
            if(GXCORE_SUCCESS != GxBus_PmSatAdd(&sat))
            {
                return -1;
            }
            GxBus_PmSync(GXBUS_PM_SYNC_SAT);
        }
    }

    return sat.id;
}

static int app_dtmb_search_get_tp(uint32_t sat_id,uint32_t *tp_array)
{
    int list_num = 0;
    //int i = 0;
    int32_t  ret= 0;
    GxBusPmDataTP tp={0};
    char* freq_value = NULL;
    uint8_t  width_value;
    uint32_t freq_tmp = 0;
    uint16_t tpid_temp = 0;
	char AddFlag = 0;
	uint32_t bandwidth[BANDWIDTH_AUTO] = {BANDWIDTH_8_MHZ,BANDWIDTH_6_MHZ,BANDWIDTH_6_MHZ};

    list_num = 1;

    GUI_GetProperty(EDIT_DTMB_SEARCH_FRE,"string",&freq_value);
    GUI_GetProperty(CMB_DTMB_SEARCH_BANDWIDTH,"select",&width_value);

	if(width_value >= BANDWIDTH_AUTO)
	{
		printf("\nDTMB, error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1; 
	}
    freq_tmp = atof(freq_value)*1000;
    ret = GxBus_PmTpExistChek((uint16_t)sat_id,(uint32_t) freq_tmp, (uint32_t)width_value, 0, 0, &tpid_temp);
	
    if(ret == 0)
    {
        tp.sat_id = sat_id;
        tp.frequency = freq_tmp;
        tp.tp_dtmb.symbol_rate = bandwidth[width_value];//0: 8M, 1:6M
        ret = GxBus_PmTpAdd(&tp);
        if (GXCORE_SUCCESS != ret)
        {
            printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
            return -1;
        }
        tp_array[0]= tp.id;
		AddFlag = 1;
    }
    else if(ret > 0)
    {
        tp_array[0]  = (uint32_t)tpid_temp;
    }
    else
    {
        printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
	if(AddFlag)
    	GxBus_PmSync(GXBUS_PM_SYNC_TP);
    return list_num;
}

static int app_dtmb_default_auto_search_get_tp(uint32_t sat_id,uint32_t *tp_array)
{
    int list_num = 0;
    int i = 0;
    int32_t  ret= 0;
    GxBusPmDataTP tp={0};
    uint16_t tpid_temp = 0;
	char AddFlag = 0;

    list_num = _dtmb_get_channel_list(&thizFreqChannel);
    if(TP_MAX_NUM < list_num)
    {
        list_num = TP_MAX_NUM;
    }
    for(i=0;i<list_num;i++)
    {
        ret = GxBus_PmTpExistChek((uint16_t)sat_id,(uint32_t)1000* thizFreqChannel[i].freq, (uint32_t)thizFreqChannel[i].band, 0, 0, &tpid_temp);
        if(ret == 0)
        {
            tp.sat_id = sat_id;
            tp.frequency = 1000* thizFreqChannel[i].freq;
            tp.tp_dtmb.symbol_rate = thizFreqChannel[i].band;
            ret = GxBus_PmTpAdd(&tp);
            if (GXCORE_SUCCESS != ret)
            {
                printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
                break;
            }
            tp_array[i]= tp.id;
			AddFlag = 1;
        }
		else if(ret > 0)
        {
            tp_array[i] = (uint32_t)tpid_temp;
        }
        else
        {
            printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
            return -1;
        }
    }
	if(AddFlag)
    	GxBus_PmSync(GXBUS_PM_SYNC_TP);
    return list_num;
}

static int app_dtmb_search_scan_all_get_fre(uint32_t *fre_array)
{
    uint16_t i;
    uint16_t fre;
    uint16_t freArrayCount =0;
	uint16_t begin_fre = 0;
	uint16_t end_fre = 0;
    uint16_t end_fre_tmp = 0;
    char *BeginFre = NULL;
	char *EndFre = NULL;

    GUI_GetProperty(EDIT_DTMB_SEARCH_FRE,"string",&BeginFre);
    begin_fre = (uint16_t)atof(BeginFre);

	GUI_GetProperty(EDIT_DTMB_SEARCH_ENDFRE, "string", &EndFre);
    end_fre = (uint16_t)atof(EndFre);
	end_fre_tmp = end_fre;

	if(begin_fre<=52)
	{
		begin_fre = 52;
	}
	else if(begin_fre<=115)
	{
		begin_fre = 115;
	}
	else if(begin_fre<=467)
	{
		begin_fre = (467 - (abs(begin_fre - 467)/8)*8);
	}
	else if(begin_fre <474)
	{
		begin_fre = 474;
	}
	else
	{
		begin_fre = (858 - (abs(begin_fre - 858)/8)*8);
	}


	if(end_fre<=467)
	{
		end_fre = (467 - (abs(end_fre - 467)/8)*8);
        
        if(end_fre>end_fre_tmp)
        {
            end_fre -=8;
        }
        
	}
	else if(end_fre <474)
	{
		end_fre = 474;
	}
	else if(end_fre <=866 )
	{
		end_fre = (866 - (abs(end_fre - 866)/8)*8);
        if(end_fre>end_fre_tmp)
        {
            end_fre -=8;
        }
	}
	else
	{
		end_fre = 866;
	}

	freArrayCount = 0;
	fre = begin_fre;
    
	for(i=0;i<TP_MAX_NUM;i++)
	{
		if(fre==76)
		{
			fre = 80;
		}
		else if (fre==96)
		{
			fre = 115;
		}
		else if (fre==475)
		{
			fre = 474;
		}

		if (fre > end_fre)
			{
				break;
			}
        
		fre_array[i] = fre;
		freArrayCount++;

		{
			fre = fre +8;
		}
	}

    printf("====All Search Freq Info,total:%d=====\n",freArrayCount);
#if 0
    for(i=0;i<freArrayCount;i++)
    {
        printf("%d-----fre:%d\n",i+1,fre_array[i]);
    }
#endif
    
	//app_lcn_list_clear();
	return freArrayCount;
}

static int app_dtmb_search_scan_all_get_tp(uint32_t sat_id, uint32_t *tp_array)
{
	int i = 0;
	int fre_num = 0;
	int32_t ret;
	GxBusPmDataTP tp={0};
	uint16_t tpid_temp = 0;
	char AddFlag = 0;
	uint32_t fre_array[TP_MAX_NUM] = {0};
	uint32_t bandwidth[BANDWIDTH_AUTO] = {BANDWIDTH_8_MHZ,BANDWIDTH_6_MHZ,BANDWIDTH_6_MHZ};

	fre_num = app_dtmb_search_scan_all_get_fre(fre_array);

	for(i=0; i<fre_num; i++)
	{
		ret = GxBus_PmTpExistChek((uint16_t)sat_id, (uint32_t)fre_array[i]*1000, (uint32_t)bandwidth[0], 0, 0,&tpid_temp);
		if(ret == 0)
		{
			tp.sat_id = sat_id;
			tp.frequency = fre_array[i]*1000;
			tp.tp_dtmb.symbol_rate = bandwidth[0];//0: 8M, 1:6M
			ret = GxBus_PmTpAdd(&tp);
			if (GXCORE_SUCCESS != ret)
			{
				//tp加入不成功
				printf("can't add Tp\n");
				break;
			}

			tp_array[i] = tp.id;
			AddFlag = 1;
		}
		else if(ret > 0)
		{
			tp_array[i] = (uint32_t)tpid_temp;
		}
		else
		{
			//tp加入不成功
			printf("can't add Tp\n");
			return -1;
		}
	}
	if(AddFlag)
		GxBus_PmSync(GXBUS_PM_SYNC_TP);

	return i;
}

/*
* 搜索
* searchlist - 搜索参数，频率、符号率、调制方式列表，频点个数等
*/
static int app_dtmb_search_scan_all_start(void)
{
    GxMsgProperty_NodeByIdGet node = {0};
    GxMsgProperty_ScanDtmbStart msg_dtmb_scan={0,};
    AppFrontend_Config cfg = {0};
    GxBusPmDataSat sat = {0};
	int tp_max_num = 0;
	int i = 0;

	//static GxSearchExtend BatParse[SEARCH_EXTEND_MAX];
	//memset(&searchresultpara,0,sizeof(search_result));
	//app_stop_all_monitor_filter();

//	app_lcn_list_clear();

	app_send_msg_exec(GXMSG_SEARCH_BACKUP, NULL);
	memset(&msg_dtmb_scan, 0, sizeof(GxMsgProperty_ScanDtmbStart));


    memset(thizDtmbTpidArray,0,sizeof(uint32_t)*TP_MAX_NUM);
    memset(thizDtmbTsidArray,0,sizeof(uint32_t)*TP_MAX_NUM);

    if((0 >=  thizDtmbSatId) 
		|| (GxBus_PmSatGetById(thizDtmbSatId, &sat)) == GXCORE_ERROR)
    {
        thizDtmbSatId = app_dtmb_search_get_sat_id();
        if(thizDtmbSatId  <= 0)
        {
            printf("\nDTMB, error, get sat err %s, %d\n",__FUNCTION__,__LINE__);
			return -1;
        }
    }

    tp_max_num = app_dtmb_search_scan_all_get_tp(thizDtmbSatId,thizDtmbTpidArray);
    if(tp_max_num <=0)
        return -1;


	/*
	* 添加扩展表过滤条件,需改成回调函数
	if (NULL !=  app_search_add_extend_callback)
		app_search_add_extend_callback(&searchExtendList);

	if (searchExtendList.extendnum > 0)
	{
		memset(BatParse,0,SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
		dtmb_scan.ext_num = searchExtendList.extendnum;
		memcpy(&BatParse[0],&searchExtendList.searchExtend[0],SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
		dtmb_scan.ext = &BatParse[0];
	}
	*/

	/*
	 * 搜索节目前，初始化逻辑频道号列表
	 */
	//app_lcn_list_init();

    node.node_type = NODE_SAT;
    node.id = thizDtmbSatId;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    app_ioctl(node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
    for(i=0;i<tp_max_num;i++)
    {
        thizDtmbTsidArray[i] = cfg.ts_src;
    }

    msg_dtmb_scan.scan_type = GX_SEARCH_MANUAL;
    msg_dtmb_scan.tv_radio = GX_SEARCH_TV_RADIO_ALL;
    msg_dtmb_scan.fta_cas = GX_SEARCH_FTA_CAS_ALL;//mode
    msg_dtmb_scan.nit_switch = GX_SEARCH_NIT_ENABLE;//nit enable for auto scan

    msg_dtmb_scan.params_dtmb.sat_id = thizDtmbSatId ;
    msg_dtmb_scan.params_dtmb.max_num = tp_max_num;
    msg_dtmb_scan.params_dtmb.array = thizDtmbTpidArray;
    msg_dtmb_scan.params_dtmb.ts = thizDtmbTsidArray;
    // needn't ext
    msg_dtmb_scan.ext = NULL;
    msg_dtmb_scan.ext_num = 0;

    msg_dtmb_scan.time_out.pat = PAT_FILTER_TIMEOUT;
    msg_dtmb_scan.time_out.sdt = SDT_FILTER_TIMEOUT;
    msg_dtmb_scan.time_out.nit = NIT_FILTER_TIMEOUT;
    msg_dtmb_scan.time_out.pmt = PMT_FILTER_TIMEOUT;

    msg_dtmb_scan.modify_prog = _dtmb_modify_prog_callback;
    app_search_set_search_type(SEARCH_DTMB);
    GUI_EndDialog("wnd_search_setting_dtmb");
    GUI_CreateDialog("wnd_search");
    GUI_SetInterface("flush", NULL);
    printf("\nDTMB, %s, sat id = %d,ts source = %d, tp num = %d, tp id = %d\n",__FUNCTION__,thizDtmbSatId,thizDtmbTsidArray[0],tp_max_num,thizDtmbTsidArray[0]);

    app_send_msg_exec(GXMSG_SEARCH_SCAN_DTMB_START,(void*)&msg_dtmb_scan);

	return 0;
}

static int app_dtmb_search_start(void)
{
    GxMsgProperty_ScanDtmbStart msg_dtmb_scan={0,};
    uint8_t tp_max_num = 0;
    GxMsgProperty_NodeByIdGet node = {0};
    AppFrontend_Config cfg = {0};
    int i = 0;
    GxBusPmDataSat sat = {0};

    memset(thizDtmbTpidArray,0,sizeof(uint32_t)*TP_MAX_NUM);
    memset(thizDtmbTsidArray,0,sizeof(uint32_t)*TP_MAX_NUM);

    if((0 >=  thizDtmbSatId) 
		|| (GxBus_PmSatGetById(thizDtmbSatId, &sat)) == GXCORE_ERROR)
    {
        thizDtmbSatId = app_dtmb_search_get_sat_id();
        if(thizDtmbSatId  <= 0)
        {
            printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
			return -1;
        }
    }
    tp_max_num = app_dtmb_search_get_tp(thizDtmbSatId,thizDtmbTpidArray);
    if(tp_max_num <=0)
        return -1;
	
    node.node_type = NODE_SAT;
    node.id = thizDtmbSatId;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    app_ioctl(node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
    for(i=0;i<tp_max_num;i++)
    {
        thizDtmbTsidArray[i] = cfg.ts_src;
    }

    //_search_printf("ts_id = %d\n",thizDtmbTsidArray[0]);

    msg_dtmb_scan.scan_type = GX_SEARCH_MANUAL;
    msg_dtmb_scan.tv_radio = GX_SEARCH_TV_RADIO_ALL;
    msg_dtmb_scan.fta_cas = GX_SEARCH_FTA_CAS_ALL;//mode
    msg_dtmb_scan.nit_switch = GX_SEARCH_NIT_DISABLE;

    msg_dtmb_scan.params_dtmb.sat_id =thizDtmbSatId ;
    msg_dtmb_scan.params_dtmb.max_num = tp_max_num;
    msg_dtmb_scan.params_dtmb.array = thizDtmbTpidArray;
    msg_dtmb_scan.params_dtmb.ts = thizDtmbTsidArray;
    // needn't ext
    msg_dtmb_scan.ext = NULL;
    msg_dtmb_scan.ext_num = 0;

    msg_dtmb_scan.time_out.pat = PAT_FILTER_TIMEOUT;
    msg_dtmb_scan.time_out.sdt = SDT_FILTER_TIMEOUT;
    msg_dtmb_scan.time_out.nit = NIT_FILTER_TIMEOUT;
    msg_dtmb_scan.time_out.pmt = PMT_FILTER_TIMEOUT;

    msg_dtmb_scan.modify_prog = _dtmb_modify_prog_callback;

    //display search window right now
    app_search_set_search_type(SEARCH_DTMB);
    GUI_EndDialog("wnd_search_setting_dtmb");
    GUI_CreateDialog("wnd_search");
    GUI_SetInterface("flush", NULL);
    printf("\nDTMB, %s, sat id = %d,ts source = %d, tp num = %d, tp id = %d\n",__FUNCTION__,thizDtmbSatId,thizDtmbTsidArray[0],tp_max_num,thizDtmbTsidArray[0]);
    app_send_msg_exec(GXMSG_SEARCH_SCAN_DTMB_START,(void*)&msg_dtmb_scan);

    return 0;
}

int app_dtmb_auto_search(uint8_t mode)
{
    //uint32_t sat_id = 0;
    GxMsgProperty_ScanDtmbStart msg_dtmb_scan={0,};
    uint8_t tp_max_num = 0;
    GxMsgProperty_NodeByIdGet node = {0};
    AppFrontend_Config cfg = {0};
    int i = 0;
    GxBusPmDataSat sat = {0};

    memset(thizDtmbTpidArray,0,sizeof(uint32_t)*TP_MAX_NUM);
    memset(thizDtmbTsidArray,0,sizeof(uint32_t)*TP_MAX_NUM);

    if((0 >=  thizDtmbSatId) 
		|| (GxBus_PmSatGetById(thizDtmbSatId, &sat) == GXCORE_ERROR))
    {
        thizDtmbSatId = app_dtmb_search_get_sat_id();
		if(thizDtmbSatId <= 0)
		{
			printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
			return -1;
		}
    }

    app_search_set_search_type(SEARCH_DTMB);

    GUI_CreateDialog("wnd_search");
    GUI_SetInterface("flush", NULL);
	
    node.node_type = NODE_SAT;
    node.id = thizDtmbSatId;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

#ifdef DVB_DTMB_NIT_SUPPORT //nit search need test
    AppFrontend_SetTp params = {0};
    memset(&thizDtmbNitResult , 0, sizeof(thizDtmbNitResult));
    thizNitParseFlag = 0;

    params.fre = MAIN_FREQ_DEFAULT_VALUE;
    params.type = FRONTEND_DTMB;
    params.qam = MAIN_BANDWIDTH_DEFAULT_VALUE ;
    params.type_1501 = DTMB;

	app_ioctl(node.sat_data.tuner, FRONTEND_TP_SET, &params);

    GxCore_ThreadDelay(100);// ?

    AppFrontend_LockState lock;
    app_ioctl(node.sat_data.tuner, FRONTEND_LOCK_STATE_GET, &lock);

    if (FRONTEND_LOCKED == lock)
    {
        dtmb_nit_scan_tp(&msg_dtmb_scan);
    }
    if(thizDtmbNitResult.num>0)
    {
        tp_max_num = app_dtmb_nit_auto_search_get_tp(thizDtmbSatId,thizDtmbTpidArray);
    }
    else
#endif
    {
        tp_max_num = app_dtmb_default_auto_search_get_tp(thizDtmbSatId,thizDtmbTpidArray);
    }

    if(tp_max_num <=0)
    {
        printf("tp_max_num  = %d !!error\n",tp_max_num);
    }

    app_ioctl(node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
    for(i=0;i<tp_max_num;i++)
    {
        thizDtmbTsidArray[i] = cfg.ts_src;
    }

    msg_dtmb_scan.scan_type = GX_SEARCH_MANUAL;
    msg_dtmb_scan.tv_radio = GX_SEARCH_TV_RADIO_ALL;
    msg_dtmb_scan.fta_cas = mode;//GX_SEARCH_FTA_CAS_ALL;//mode
    msg_dtmb_scan.nit_switch = GX_SEARCH_NIT_ENABLE;//nit enable for auto scan

    msg_dtmb_scan.params_dtmb.sat_id = thizDtmbSatId ;
    msg_dtmb_scan.params_dtmb.max_num = tp_max_num;
    msg_dtmb_scan.params_dtmb.array = thizDtmbTpidArray;
    msg_dtmb_scan.params_dtmb.ts = thizDtmbTsidArray;
    // needn't ext
    msg_dtmb_scan.ext = NULL;
    msg_dtmb_scan.ext_num = 0;

    msg_dtmb_scan.time_out.pat = PAT_FILTER_TIMEOUT;
    msg_dtmb_scan.time_out.sdt = SDT_FILTER_TIMEOUT;
    msg_dtmb_scan.time_out.nit = NIT_FILTER_TIMEOUT;
    msg_dtmb_scan.time_out.pmt = PMT_FILTER_TIMEOUT;

    msg_dtmb_scan.modify_prog = _dtmb_modify_prog_callback;
#if 0
	for(i=0;i<tp_max_num;i++)
		printf("[Search]: tpid[%d] = %d\n", i, thizDtmbTpidArray[i]);
#endif
    app_send_msg_exec(GXMSG_SEARCH_SCAN_DTMB_START,(void*)&msg_dtmb_scan);

    return 0;
}

#ifdef DVB_DTMB_NIT_SUPPORT //nit search need test
static int app_dtmb_nit_auto_search_get_tp(uint32_t sat_id,uint32_t *tp_array)
{
    int list_num = 0;
    int i = 0;
    int32_t  ret= 0;
    GxBusPmDataTP tp={0};
	char AddFlag = 0;
    uint32_t tpid_temp;

    list_num = thizDtmbNitResult.num;
    if(TP_MAX_NUM < list_num)
    {
        list_num = TP_MAX_NUM;
    }
    for(i=0;i<list_num;i++)
    {
        ret = GxBus_PmTpExistChek((uint16_t)sat_id,(uint32_t)thizDtmbNitResult.app_fre_array[i], (uint32_t)thizDtmbNitResult.app_band_array[i], 0, 0,(uint16_t *)&tpid_temp);
        if(ret == 0)
        {
            tp.sat_id = sat_id;
            tp.frequency = thizDtmbNitResult.app_fre_array[i];
            tp.tp_dtmb.symbol_rate = thizDtmbNitResult.app_band_array[i];
            ret = GxBus_PmTpAdd(&tp);
            if (GXCORE_SUCCESS != ret)
            {
                printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
                break;
            }
            tp_array[i]= tp.id;
			AddFlag = 1;
        }
        else if(ret > 0)
        {
            tp_array[i]  = tpid_temp;
        }
        else
        {
            printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
            return -1;
        }
    }

	if(AddFlag)
    	GxBus_PmSync(GXBUS_PM_SYNC_TP);

    return list_num;
}

static int dtmb_search_nit_section_parse(uint8_t* p_section_data, size_t section_size, classDtmbSearchNit *nit_result)
{
    uint8_t *section_data = NULL, *section_data1 = NULL;
    int16_t len1 = 0;
    int16_t len2 = 0;
    int16_t len3 = 0;
    int16_t len4 = 0;
    int16_t i = 0;
    uint32_t freq = 0;
    uint32_t bandwidth = 0;
    int32_t  nitVersion = 0;
    uint16_t ts_id = 0;
    uint8_t  current_section_num = 0;
    uint8_t  last_section_num = 0;

    if (NULL == p_section_data || NULL == nit_result)
    {
        return -1;
    }

    section_data = p_section_data;
    current_section_num = section_data[6];
    last_section_num = section_data[7];
    _search_printf("dtmb_search_nit_section_parse size=%d len=%d\n",section_size, ((section_data[1]&0x0f)<<8) + section_data[2] + 3 );
    _search_printf("dtmb_search_nit_section_parse section_data[7]=%d section_data[6]=%d\n",section_data[7],section_data[6]);

    if(NULL == nit_result->nit_subtable_flag)
    {
        nit_result->nit_subtable_flag = GxCore_Malloc(MAX_NIT_SECTION_NUM);
        if (NULL == nit_result->nit_subtable_flag)
        {
            _search_printf("[APP dvbt2 Search] malloc error : %s\n", __func__);
            return -1;
        }

        _search_printf("[APP dvbt2 Search] Malloc nit_subtable_flag\n");
        memset(nit_result->nit_subtable_flag,0,(MAX_NIT_SECTION_NUM));
    }
    // NIT table counter
    nit_result->nit_recv_num++;

    if(section_data[0] == NIT_ACTUAL_NETWORK_TID)
    {
        for(i = 0; i <= last_section_num; i++)
        {
            if(nit_result->nit_subtable_flag[i]!=1)
            {
                break;
            }
        }
        _search_printf("[APP dvbt2 Search] First empty Nit flag index: %d\n", i);
        // check all sections received
        if(i == (last_section_num + 1))
        {
            // parsing all nit sections complete
            return 0;
        }

        // check whether this section is duplicated
        if(nit_result->nit_subtable_flag[current_section_num] == 1)
        {
            // this nit section is the same before
            return -2;
        }

        // then parse this nit section
        nit_result->nit_subtable_flag[current_section_num] = 1;
        _search_printf("[APP dvbt2 Search] Get New Nit section \n");
        {
            nitVersion = (section_data[5] & 0x3E) >> 1;
            //app_flash_save_config_center_nit_fre_version(nitVersion);
            freq = nit_result->app_fre_array[0];
            _search_printf("MAIN_FREQ_NIT=%d\n", freq);
            _search_printf("MAIN_FREQ_NITVERSION=%d\n", nitVersion);
            freq = 0;
        }
        len1 = ((section_data[8]&0xf)<<8)+section_data[9];
        len2 = ((section_data[9+len1+1]&0xf)<<8)+section_data[9+len1+2];
        section_data +=(9+1+len1+2);

        while(len2>0)
        {
            ts_id = ((section_data[0]<<8)&0xff00)+section_data[1];
            len4 = len3 = ((section_data[4]&0xf)<<8)+section_data[5];
            section_data1 = &section_data[6];
            while(len3>0)
            {
                switch(section_data1[0])
                {
                    case 0x5a://SI_PARSE_LIB_TER_DELIVERY_SYSTEM_DES://terrestrial_delivery_system_descriptor:
                        { // -t
                            freq = ((section_data1[2] & 0xf0)>>4) * 10000000 + (section_data1[2] & 0xf) * 1000000
                                + ((section_data1[3] & 0xf0)>>4) * 100000 + (section_data1[3] & 0xf) * 10000
                                + ((section_data1[4] & 0xf0)>>4) * 1000 + (section_data1[4] & 0xf) * 100
                                + ((section_data1[5] & 0xf0) >> 4) * 10 + (section_data1[5] & 0xf);

                            freq= freq*10; // hz
                            bandwidth =((section_data1[6] >> 5) & 0x07);
                            _search_printf("dtmb_search_nit_section_parse dvbt2 freq=%d\n",freq);
                            for (i=0; i< nit_result->num;i++)
                            {
                                if (freq == nit_result->app_fre_array[i])
                                {
                                    nit_result->app_fre_tsid[i]=ts_id;
                                    break;
                                }
                            }
                            if (i == nit_result->num)
                            {
                                if (freq > 0)
                                {
                                    nit_result->app_fre_array[nit_result->num]=freq;
                                    nit_result->app_band_array[nit_result->num]=bandwidth;
                                    nit_result->app_fre_tsid[nit_result->num]=ts_id;
                                    nit_result->num++;
                                }
                            }
                        }
                        break;
                    default:
                        break;
                }
                len3 -= (section_data1[1]+2);
                section_data1 += (section_data1[1]+2);
            }
            section_data +=(6+len4);
            len2 -=(6+len4);
        }
    }
    // parsing current nit section complete
    return 1;
}

static private_parse_status dtmb_search_nit_section_process_callback(uint8_t* p_section_data, size_t section_size)
{
    int32_t  parse_result = -1;
    thizNitParseFlag = 1;
    //copy data
    parse_result = dtmb_search_nit_section_parse(p_section_data, section_size, &thizDtmbNitResult);
    _search_printf("\n\n[SI Cabllback] Recv Nit counter: %d\n\n\n", thizDtmbNitResult.nit_recv_num);
    if(parse_result == 0 || thizDtmbNitResult.nit_recv_num > MAX_NIT_SECTION_NUM)
    {
        return PRIVATE_SUBTABLE_OK;
    }

    return PRIVATE_SECTION_OK;
}

static void dtmb_nit_scan_tp(GxMsgProperty_ScanDtmbStart * p_msg_dtmb_scan)
{
    GxSubTableDetail subtable_detail = {0};
    GxMsgProperty_SiCreate	msg_si_create = {0};
    GxMsgProperty_SiStart msg_si_start = {0};
    GxTime start_time = {0};
    GxTime local_time = {0};

    //set filter config
    subtable_detail.ts_src = (p_msg_dtmb_scan->params_t.ts[0]);
    subtable_detail.demux_id = 0;
    subtable_detail.time_out = NIT_FILTER_TIMEOUT;
    subtable_detail.si_filter.pid = NIT_PID;
    subtable_detail.si_filter.match_depth = 1/*5*/;
    subtable_detail.si_filter.eq_or_neq = EQ_MATCH;
    subtable_detail.si_filter.match[0] = NIT_ACTUAL_NETWORK_TID;
    subtable_detail.si_filter.mask[0] = 0xff;
    subtable_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
    subtable_detail.table_parse_cfg.table_parse_fun = dtmb_search_nit_section_process_callback;
    msg_si_create = &subtable_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE,(void*)&msg_si_create);

    msg_si_start = subtable_detail.si_subtable_id;

    app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&msg_si_start);

    GxCore_GetTickTime(&start_time);
    GxCore_GetTickTime(&local_time);

    while(thizNitParseFlag == 0  &&(start_time.seconds + (NIT_FILTER_TIMEOUT/1000) > local_time.seconds) )
    {
        _search_printf("nit wait\n");
        GxCore_GetTickTime(&local_time);
        GxCore_ThreadDelay(100);
    }

    app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&msg_si_start);
    if(NULL != thizDtmbNitResult.nit_subtable_flag)
    {
        GxCore_Free(thizDtmbNitResult.nit_subtable_flag);
        thizDtmbNitResult.nit_subtable_flag = NULL;
        thizDtmbNitResult.nit_recv_num = 0;
    }
}
#endif

#define TIMER_FUNC
static int timer_dtmb_show_signal(void *userdata)
{
    unsigned int value = 0;
    char Buffer[20] = {0};
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};

	GxBusPmDataSat sat = {0};
    if(GXCORE_ERROR == GxBus_PmSatGetById(thizDtmbSatId, &sat))
    {
    	printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
    // progbar strength
    //g_AppNim.property_get(&g_AppNim, AppFrontendID_Strength,(void*)(&value),4);
    //app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_STRENGTH_GET, &value);
    app_ioctl(sat.tuner, FRONTEND_STRENGTH_GET, &value);
    siganl_bar.strength_bar = "progbar_dtmb_search_strength";
    siganl_val.strength_val = value;

    // strength value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_dtmb_search_strength_value", "string", Buffer);

    // progbar quality
    //g_AppNim.property_get(&g_AppNim, AppFrontendID_Quality,(void*)(&value),4);
    //app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_QUALITY_GET, &value);
    app_ioctl(sat.tuner, FRONTEND_QUALITY_GET, &value);
    siganl_bar.quality_bar= "progbar_dtmb_search_quality";
    siganl_val.quality_val = value;
    // quality value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_dtmb_search_quality_value", "string", Buffer);

    //app_signal_progbar_update(app_tuner_cur_tuner_get(), &siganl_bar, &siganl_val);
    app_signal_progbar_update(sat.tuner, &siganl_bar, &siganl_val);

    ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &value);

    return 0;
}

static int timer_dtmb_set_frontend(void *userdata)
{
extern void GxFrontend_ForceExitEx(int32_t tuner);

    SetTpMode force = 0;
	GxMsgProperty_NodeByIdGet node = {0};

    remove_timer(sp_DtmbLockTimer);
    sp_DtmbLockTimer = NULL;

    if(userdata != NULL)
    {
        force = *(SetTpMode*)userdata;
        if(force == SET_TP_FORCE)
        {
            GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
        }
    }
	node.node_type = NODE_SAT;
    node.id = thizDtmbSatId;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
#if DOUBLE_S2_SUPPORT
    GxFrontend_ForceExitEx(0);
    GxFrontend_ForceExitEx(1);
#else
    GxFrontend_ForceExitEx(node.sat_data.tuner);
#endif
    app_dtmb_search_set_tp(node.sat_data.tuner);
    return 0;
}

static void app_dtmb_search_init(void)
{
    int list_num = 0;
    int i = 0;
    char *buffer_all = NULL;
    char buffer[DTMB_MAX_LEN +1] = {0};
	GxBusPmDataSat sat = {0};
    //int ch_select = 0;

	if((0 >=  thizDtmbSatId) 
		|| (GxBus_PmSatGetById(thizDtmbSatId, &sat) == GXCORE_ERROR))
    {
        thizDtmbSatId = app_dtmb_search_get_sat_id();
		if(thizDtmbSatId <= 0)
		{
			printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
			return ;
		}
    }
	
    list_num = _dtmb_get_channel_list(&thizFreqChannel);

    buffer_all = GxCore_Malloc(list_num * (DTMB_MAX_LEN +1)+2);
	if(NULL == buffer_all)
	{
		printf("\nDTMB, error, %s, %d\n",__FUNCTION__,__LINE__);
		return ;
	}
    memset(buffer_all, 0, (list_num * (DTMB_MAX_LEN +1)+2));
    memset(buffer, 0, DTMB_MAX_LEN +1);

    //first
    sprintf(buffer_all,"[%s", thizFreqChannel[0].channel);
    for (i = 1;i<list_num;i++)
    {
        sprintf(buffer,",%s", thizFreqChannel[i].channel);
        strcat(buffer_all, buffer);
    }
    strcat(buffer_all, "]");

    GUI_SetProperty(CMB_DTMB_SEARCH_CH,"content",buffer_all);
    app_dtmb_search_cmb_channel_change(NULL,NULL);

    GxCore_Free(buffer_all);
}

SIGNAL_HANDLER int app_dtmb_search_create (GuiWidget *widget, void *usrdata)
{
    app_dtmb_search_init();

    GUI_SetFocusWidget("box_dtmb_search_options");
	
	app_dtmb_search_mode_show(0);

	// lnb polar off
	AppFrontend_PolarState polar = FRONTEND_POLAR_OFF;
	GxFrontend_SetVoltage(0, polar);
	
    if (reset_timer(thizDtmbTimerSignal) != 0)
    {
        thizDtmbTimerSignal = create_timer(timer_dtmb_show_signal, 100, NULL, TIMER_REPEAT);
    }
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_dtmb_search_destroy(GuiWidget* widget, void *usrdata)
{
    remove_timer(thizDtmbTimerSignal);
    thizDtmbTimerSignal = NULL;
    remove_timer(sp_DtmbLockTimer);
    sp_DtmbLockTimer = NULL;
    return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_dtmb_search_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    uint32_t box_sel;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case STBK_EXIT:
                case STBK_MENU:
                    {
                        GUI_EndDialog("wnd_search_setting_dtmb");
                        GUI_SendEvent("wnd_system_setting", event);
                    }
                    break;
                case STBK_OK:
#if 0
                    {
                        GUI_GetProperty(CMB_DTMB_SEARCH_MODE, "select", &box_sel);
                        if(1 == box_sel)
                        {
                            app_dtmb_auto_search(GX_SEARCH_FTA_CAS_ALL);
                            GUI_EndDialog("wnd_search_setting_dtmb");
                        }
                        else if(box_sel == 0)
						{
                            app_dtmb_search_start();
						}
						else
						{
							app_dtmb_search_scan_all_start();
						}
                    }
#endif
                    break;
                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_dtmb_search_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    uint32_t box_sel = 0;
    int32_t mode_sel = -1;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case STBK_UP:
				{

					GUI_GetProperty("cmbox_dtmb_search_mode", "select", &mode_sel);
					GUI_GetProperty("box_dtmb_search_options", "select", &box_sel);
					if(mode_sel == 0) //manual
					{
						if( box_sel == DTMB_SEARCH_BOX_ITEM_CH )
					    {
						    box_sel = DTMB_SEARCH_BOX_ITEM_OK;
							GUI_SetProperty("box_dtmb_search_options", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					else if(mode_sel == 1)//auto
					{
						if( box_sel == DTMB_SEARCH_BOX_ITEM_MODE )
						{
							box_sel = DTMB_SEARCH_BOX_ITEM_OK;
							GUI_SetProperty("box_dtmb_search_options", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					else
					{
						if( box_sel == DTMB_SEARCH_BOX_ITEM_FREQ)
						{
							box_sel = DTMB_SEARCH_BOX_ITEM_OK;
							GUI_SetProperty("box_dtmb_search_options", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
				}
					break;
	            case STBK_DOWN:
				{
					uint32_t box_sel;
					int32_t mode_sel = -1;

					GUI_GetProperty("cmbox_dtmb_search_mode", "select", &mode_sel);
					GUI_GetProperty("box_dtmb_search_options", "select", &box_sel);
					//printf("enter %s:%d mode_sel = %d -------------\n",__func__,__LINE__,mode_sel);
					if(mode_sel == 0) //manual
					{
						if( box_sel == DTMB_SEARCH_BOX_ITEM_OK )
						{
							box_sel = DTMB_SEARCH_BOX_ITEM_CH;
							GUI_SetProperty("box_dtmb_search_options", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					else if(mode_sel == 1)
					{
						if( box_sel == DTMB_SEARCH_BOX_ITEM_OK )
						{
							box_sel = DTMB_SEARCH_BOX_ITEM_MODE;
							GUI_SetProperty("box_dtmb_search_options", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}

					}
					else
					{
						if( box_sel == DTMB_SEARCH_BOX_ITEM_OK )
						{
							box_sel = DTMB_SEARCH_BOX_ITEM_FREQ;
							GUI_SetProperty("box_dtmb_search_options", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
				}
                break;
	            case STBK_OK:
                    GUI_GetProperty("box_dtmb_search_options", "select", &box_sel);
                    if(box_sel == DTMB_SEARCH_BOX_ITEM_OK)
                    {
                        GUI_GetProperty(CMB_DTMB_SEARCH_MODE, "select", &mode_sel);
                        if(mode_sel == ITEM_DTMB_AUTO_SEARCH)
						{
                            app_dtmb_search_start();
						}
                        else if(mode_sel == ITEM_DTMB_SEARCH)
                        {
                            app_dtmb_auto_search(GX_SEARCH_FTA_CAS_ALL);
                            GUI_EndDialog("wnd_search_setting_dtmb");
                        }
						else
						{
							app_dtmb_search_scan_all_start();
						}
                    }
                    break;

	            case STBK_MENU:
	            case STBK_EXIT:
	            case STBK_LEFT:
	            case STBK_RIGHT:
	                ret = EVENT_TRANSFER_KEEPON;
	                break;

	            default:
	                break;
	        }
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_dtmb_search_cmb_channel_change(GuiWidget *widget, void *usrdata)
{
    uint32_t sel;
    char freq_str[8];
    GUI_GetProperty(CMB_DTMB_SEARCH_CH, "select", &sel);

    sprintf(freq_str,"%3.1f",thizFreqChannel[sel].freq);
	//printf("\nDTMB, %s, str = %s, value = %f\n", __FUNCTION__,freq_str,thizFreqChannel[sel].freq);
    GUI_SetProperty(EDIT_DTMB_SEARCH_FRE,"string",freq_str);
    GUI_SetProperty(CMB_DTMB_SEARCH_BANDWIDTH,"select",&(thizFreqChannel[sel].band));

    s_set_tp_mode = SET_TP_FORCE;
    if(reset_timer(sp_DtmbLockTimer) != 0)
    {
        sp_DtmbLockTimer = create_timer(timer_dtmb_set_frontend, 500, (void *)&s_set_tp_mode, TIMER_REPEAT);
    }
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_dtmb_search_cmb_freq_change(GuiWidget *widget, void *usrdata)
{
    char* fre = NULL;
	float freq_tmp;
	GUI_GetProperty(EDIT_DTMB_SEARCH_FRE, "string", &fre);
    freq_tmp = atof(fre);
	if((freq_tmp < (float)DTMB_START_FREQ) || (freq_tmp > (float)DTMB_END_FREQ))
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		pop.format = POP_FORMAT_DLG;
		pop.str = "Out of range";
		pop.timeout_sec= 2;
		popdlg_create(&pop);

		if(freq_tmp < (float)DTMB_START_FREQ) 
			GUI_SetProperty(EDIT_DTMB_SEARCH_FRE, "string", DTMB_START_FREQ_STR);
		if(freq_tmp > (float)DTMB_END_FREQ)
			GUI_SetProperty(EDIT_DTMB_SEARCH_FRE, "string", DTMB_END_FREQ_STR);
	}

    s_set_tp_mode = SET_TP_FORCE;
   
    if(reset_timer(sp_DtmbLockTimer) != 0)
    {
        sp_DtmbLockTimer = create_timer(timer_dtmb_set_frontend, 500, (void *)&s_set_tp_mode, TIMER_REPEAT);
    }
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_dtmb_search_cmb_bandwidth_change(GuiWidget *widget, void *usrdata)
{
    s_set_tp_mode = SET_TP_FORCE;
    if(reset_timer(sp_DtmbLockTimer) != 0)
    {
        sp_DtmbLockTimer = create_timer(timer_dtmb_set_frontend, 500, (void *)&s_set_tp_mode, TIMER_REPEAT);
    }
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_dtmb_search_endfre_change(GuiWidget *widget, void *usrdata)
{
    char* fre = NULL;
	float freq_tmp;
	GUI_GetProperty(EDIT_DTMB_SEARCH_ENDFRE, "string", &fre);
    freq_tmp = atof(fre);
	if((freq_tmp < (float)DTMB_START_FREQ) || (freq_tmp > (float)DTMB_END_FREQ))
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		pop.format = POP_FORMAT_DLG;
		pop.str = "Out of range";
		pop.timeout_sec= 2;
		popdlg_create(&pop);

		if(freq_tmp < (float)DTMB_START_FREQ) 
			GUI_SetProperty(EDIT_DTMB_SEARCH_ENDFRE, "string", DTMB_START_FREQ_STR);
		if(freq_tmp > (float)DTMB_END_FREQ)
			GUI_SetProperty(EDIT_DTMB_SEARCH_ENDFRE, "string", DTMB_END_FREQ_STR);
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_dtmb_search_cmb_mode_change(GuiWidget *widget, void *usrdata)
{
    uint32_t box_sel;

    GUI_GetProperty("cmbox_dtmb_search_mode", "select", &box_sel);

	app_dtmb_search_mode_show(box_sel);
    return EVENT_TRANSFER_STOP;
}

int app_dtmb_search_setting(void)
{
	GUI_CreateDialog("wnd_search_setting_dtmb");
	return 0;
}
#else //DEMOD_DVB_T
SIGNAL_HANDLER int app_dtmb_search_create (GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_dtmb_search_destroy(GuiWidget* widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_dtmb_search_keypress(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_dtmb_search_box_keypress(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_dtmb_search_cmb_channel_change(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_dtmb_search_cmb_freq_change(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_dtmb_search_cmb_bandwidth_change(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_dtmb_search_endfre_change(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_dtmb_search_cmb_mode_change(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
#endif//DEMOD_DVB_T
