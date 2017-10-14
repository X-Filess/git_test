/*****************************************************************************
 * 			  CONFIDENTIAL								
 *        Hangzhou GuoXin Science and Technology Co., Ltd.             
 *                      (C)2009-2014, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	app_dvbt2_search.c
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

#if (DEMOD_DVB_T > 0)
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
#include "app_dvbt2_search.h"

#define DEBUG_DVBT2_SEARCH
#ifdef DEBUG_DVBT2_SEARCH
#define dvbt2_search_printf		printf
#else
#define dvbt2_search_printf
#endif
//#define DVB_T2_NIT_SUPPORT

#define MAX_NIT_SECTION_NUM						(32)
#define TP_MAX_NUM									(200)
#define PAT_FILTER_TIMEOUT							(3000)
#define SDT_FILTER_TIMEOUT							(5000)
#define NIT_FILTER_TIMEOUT							(10000)
#define PMT_FILTER_TIMEOUT							(8000)

#define MAIN_FREQ_DEFAULT_VALUE		        (850000000)
#define MAIN_BANDWIDTH_DEFAULT_VALUE		(0)

#define BOX_T2_MANUAL_OPTIONS	"box_t2_search_options"
#define CMB_T2_MANUAL_SEARCH_CH	"cmbox_t2_search_channel"
#define EDIT_T2_MANUAL_SEARCH_FRE	"edit_t2_manual_search_fre"
#define CMB_T2_MANUAL_SEARCH_WIDTH	"cmbox_t2_search_bandwidth"
#define CMB_T2_MANUAL_SEARCH_MODE	"cmbox_t2_search_mode"

#define VHF_MINI_BAND	174.0
#define VHF_MAX_BAND	230.0
#define UHF_MINI_BAND	470.0
#define UHF_MAX_BAND	862.0
#define VHF_MINI_BAND_STR	"174.0"
#define VHF_MAX_BAND_STR	"230.0"
#define UHF_MINI_BAND_STR	"470.0"
#define UHF_MAX_BAND_STR	"862.0"


enum BOX_MANUAL_ITEM_NAME
{
    T2_MANUAL_SEARCH_BOX_ITEM_CH = 0,
    T2_MANUAL_SEARCH_BOX_ITEM_FREQ,
    T2_MANUAL_SEARCH_BOX_ITEM_BANDWIDTH,
    T2_MANUAL_SEARCH_BOX_ITEM_MODE,
    T2_MANUAL_SEARCH_BOX_ITEM_OK,
};

enum
{
    ITEM_DVBT2_SEARCH_1,
    ITEM_DVBT2_SEARCH_2,
    ITEM_DVBT2_SEARCH_3,
    ITEM_DVBT2_SEARCH_4,
    ITEM_DVBT2_SEARCH_5,
    ITEM_DVBT2_SEARCH_TOTAL
};

enum DVBT2_ITEM_NAME
{
    ITEM_T2_AUTO_SEARCH = 0,
    ITEM_T2_MANUAL_SEARCH,
    ITEM_T2_FULL_BAND_SEARCH,
    ITEM_DVBT2_TOTAL,
};

enum
{
    ITEM_T2_ALLBAND = 0,
    ITEM_T2_VHF ,
    ITEM_T2_UHF,
};


#ifdef DVB_T2_NIT_SUPPORT //nit search need test
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
}classDvbt2SearchNit;

static classDvbt2SearchNit thizDvbt2NitResult ;
static volatile uint8_t thizNitParseFlag = 0;
#endif

static classDefaultFreqCh *thizFreqChannel = NULL;
static event_list* thizDvbt2TimerSignal = NULL;
static event_list* sp_Dvbt2LockTimer = NULL;
static SetTpMode s_set_tp_mode = SET_TP_AUTO;

static uint32_t thizDvbt2SatId = 0;
static uint32_t thizDvbt2TpidArray[TP_MAX_NUM];
static uint32_t thizDvbt2TsidArray[TP_MAX_NUM] ;

extern void app_search_set_search_type(SearchType type);
extern  int app_t2_search_cmb_channel_change(GuiWidget *widget, void *usrdata);
#ifdef DVB_T2_NIT_SUPPORT
static void dvbt2_nit_scan_tp(GxMsgProperty_ScanDvbt2Start * p_msg_dvbt2_scan);
static uint32_t app_t2_nit_auto_search_get_tp(uint32_t sat_id,uint32_t *tp_array);

#endif
static status_t _dvbt2_modify_prog_callback(GxBusPmDataProg* prog)
{
    //for logic num
    return GXCORE_SUCCESS;
}

static void app_t2_manual_search_set_tp(void)
{
    AppFrontend_SetTp params = {0};
    char *Fre_value = NULL;
    uint8_t Width_value = 0;
    GUI_GetProperty(EDIT_T2_MANUAL_SEARCH_FRE,"string",&Fre_value);
    GUI_GetProperty(CMB_T2_MANUAL_SEARCH_WIDTH,"select",&Width_value);

    params.fre = atof(Fre_value)*1000;
    params.type = FRONTEND_DVB_T2;
    params.bandwidth = (Width_value);
    params.data_plp_id = 0xff;
    params.common_plp_exist = 0xff;
    params.common_plp_id = 0xff;

    params.type_1501 = DVBT_AUTO_MODE;
    //dvbt2_search_printf("----fre=%s,fre=%d, bandwidth=%d----\n", Fre_value, params.fre, params.bandwidth);
    app_ioctl(0, FRONTEND_TP_SET, &params);
}

static uint32_t app_t2_search_get_sat_id(void)
{
    GxBusPmDataSat sat={0};
    uint32_t num=0;
    int i = 0;
    int32_t ret = 0;

    num = GxBus_PmSatNumGet();
    if(num == 0)
    {
        sat.type = GXBUS_PM_SAT_DVBT2;
        sat.tuner = 0;//
        memcpy(sat.sat_s.sat_name, DVBT2_NETWORK_NAME,sizeof(DVBT2_NETWORK_NAME));
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
                if(GXBUS_PM_SAT_DVBT2 == sat.type )
                {
                    break;
                }
            }else
            {
                dvbt2_search_printf("[DVBT2 Search] this dvbt2 is not what you want\n");
            }
        }
        if(i >= num)
        {
            sat.type = GXBUS_PM_SAT_DVBT2;
            sat.tuner = 0;//
            memcpy(sat.sat_s.sat_name, DVBT2_NETWORK_NAME,sizeof(DVBT2_NETWORK_NAME));
            if(GXCORE_SUCCESS != GxBus_PmSatAdd(&sat))
            {
                return -1;
            }
            GxBus_PmSync(GXBUS_PM_SYNC_SAT);
        }
    }

    return sat.id;
}

static uint32_t app_t2_manual_search_get_tp(uint32_t sat_id,uint32_t *tp_array)
{
    int list_num = 0;
    //int i = 0;
    int32_t  ret= 0;
    GxBusPmDataTP tp={0};
    char* freq_value = NULL;
    uint8_t  width_value;
    uint32_t freq_tmp = 0;
    uint32_t tpid_temp = 0;

    list_num = 1;

    GUI_GetProperty(EDIT_T2_MANUAL_SEARCH_FRE,"string",&freq_value);
    GUI_GetProperty(CMB_T2_MANUAL_SEARCH_WIDTH,"select",&width_value);
    freq_tmp = atof(freq_value)*1000;

    //GxBus_PmTpExistChek can't use because of param freq is uint16_t
    ret = GxBus_PmTpExistChek((uint16_t)sat_id,(uint32_t) freq_tmp, (uint32_t)width_value, 0, 0,(uint16_t *)&tpid_temp);
//	 printf("ret tp exist = %d\n",ret);
	
    if(ret == 0)
    {
        tp.sat_id = sat_id;
        tp.frequency = freq_tmp;
        {
            tp.tp_t.bandwidth  = width_value;
        }
        ret = GxBus_PmTpAdd(&tp);
        if (GXCORE_SUCCESS != ret)
        {
            //tp加入不成功
            printf("can't add Tp\n");
            //diag error datebase error
            return -1;
            //not finish
        }
        tp_array[0]= tp.id;
    }
    else if(ret > 0)
    {
        tp_array[0]  = tpid_temp;
    }
    else
    {
        dvbt2_search_printf("[DVBT2 Search]can't add Tp\n");
        return -1;
    }

    GxBus_PmSync(GXBUS_PM_SYNC_TP);
    return list_num;
}

static uint32_t app_t2_default_auto_search_get_tp(uint32_t sat_id,uint32_t *tp_array)
{
    int list_num = 0;
    int i = 0;
    int32_t  ret= 0;
    GxBusPmDataTP tp={0};
    //char* freq_value = NULL;
    //uint8_t  width_value;
    //uint32_t freq_tmp = 0;
    uint32_t tpid_temp;

    list_num = AppDvbt2GetFreqList(&thizFreqChannel);
    if(TP_MAX_NUM < list_num)
    {
        list_num = TP_MAX_NUM;
    }
    for(i=0;i<list_num;i++)
    {
        ret = GxBus_PmTpExistChek((uint16_t)sat_id,(uint32_t)1000* thizFreqChannel[i].freq, (uint32_t)thizFreqChannel[i].band, 0, 0,(uint16_t *)&tpid_temp);
        if(ret == 0)
        {
            tp.sat_id = sat_id;
            tp.frequency = 1000* thizFreqChannel[i].freq;
            {
                tp.tp_t.bandwidth  = thizFreqChannel[i].band;
            }
            ret = GxBus_PmTpAdd(&tp);
            if (GXCORE_SUCCESS != ret)
            {
                //tp加入不成功
                printf("can't add Tp\n");
                //diag error datebase error
                break;
                //not finish
            }
            tp_array[i]= tp.id;
        }else if(ret > 0)
        {
            tp_array[i] = tpid_temp;
        }
        else
        {
            dvbt2_search_printf("[DVBT2 Search]can't add Tp\n");
            return -1;
        }
    }

    GxBus_PmSync(GXBUS_PM_SYNC_TP);
    return list_num;
}

static int app_t2_manual_search_start(void)
{
    GxMsgProperty_ScanDvbt2Start msg_dvbt2_scan={0,};
    uint8_t tp_max_num = 0;
    GxMsgProperty_NodeByIdGet node = {0};
    AppFrontend_Config cfg = {0};
    int i = 0;
    GxBusPmDataSat sat = {0};

    memset(thizDvbt2TpidArray,0,sizeof(uint32_t)*TP_MAX_NUM);
    memset(thizDvbt2TsidArray,0,sizeof(uint32_t)*TP_MAX_NUM);

    if(0 ==  thizDvbt2SatId ||GxBus_PmSatGetById(thizDvbt2SatId, &sat) == GXCORE_ERROR)
    {
        thizDvbt2SatId = app_t2_search_get_sat_id();
        if(thizDvbt2SatId  < 0)
        {
            //msg
        }
    }
    tp_max_num = app_t2_manual_search_get_tp(thizDvbt2SatId,thizDvbt2TpidArray);
    if(tp_max_num <=0)
        return -1;

    node.node_type = NODE_SAT;
    node.id = thizDvbt2SatId;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    app_ioctl(node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
    for(i=0;i<tp_max_num;i++)
    {
        thizDvbt2TsidArray[i] = cfg.ts_src;
    }

    //dvbt2_search_printf("ts_id = %d\n",thizDvbt2TsidArray[0]);

    msg_dvbt2_scan.scan_type = GX_SEARCH_MANUAL;
    msg_dvbt2_scan.tv_radio = GX_SEARCH_TV_RADIO_ALL;
    msg_dvbt2_scan.fta_cas = GX_SEARCH_FTA_CAS_ALL;//mode
    msg_dvbt2_scan.nit_switch = GX_SEARCH_NIT_DISABLE;

    msg_dvbt2_scan.params_t.sat_id =thizDvbt2SatId ;
    msg_dvbt2_scan.params_t.max_num = tp_max_num;
    msg_dvbt2_scan.params_t.array = thizDvbt2TpidArray;
    msg_dvbt2_scan.params_t.ts = thizDvbt2TsidArray;
    // needn't ext
    msg_dvbt2_scan.ext = NULL;
    msg_dvbt2_scan.ext_num = 0;

    msg_dvbt2_scan.time_out.pat = PAT_FILTER_TIMEOUT;
    msg_dvbt2_scan.time_out.sdt = SDT_FILTER_TIMEOUT;
    msg_dvbt2_scan.time_out.nit = NIT_FILTER_TIMEOUT;
    msg_dvbt2_scan.time_out.pmt = PMT_FILTER_TIMEOUT;

    msg_dvbt2_scan.modify_prog = _dvbt2_modify_prog_callback;

    //display search window right now
    app_search_set_search_type(SEARCH_DVBT2);
    GUI_EndDialog("wnd_search_setting_dvbt");
    GUI_CreateDialog("wnd_search");
    GUI_SetInterface("flush", NULL);
    app_send_msg_exec(GXMSG_SEARCH_SCAN_DVBT2_START,(void*)&msg_dvbt2_scan);

    return 0;
}

int AppT2AutoSearchStart(uint8_t mode)
{
    //uint32_t sat_id = 0;
    GxMsgProperty_ScanDvbt2Start msg_dvbt2_scan={0,};
    uint8_t tp_max_num = 0;
    GxMsgProperty_NodeByIdGet node = {0};
    AppFrontend_Config cfg = {0};
    int i = 0;
    GxBusPmDataSat sat = {0};

    memset(thizDvbt2TpidArray,0,sizeof(uint32_t)*TP_MAX_NUM);
    memset(thizDvbt2TsidArray,0,sizeof(uint32_t)*TP_MAX_NUM);

    if(0 ==  thizDvbt2SatId ||GxBus_PmSatGetById(thizDvbt2SatId, &sat) == GXCORE_ERROR)
    {
        thizDvbt2SatId = app_t2_search_get_sat_id();
    }

    app_search_set_search_type(SEARCH_DVBT2);

    GUI_CreateDialog("wnd_search");
    GUI_SetInterface("flush", NULL);

    node.node_type = NODE_SAT;
    node.id = thizDvbt2SatId;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

#ifdef DVB_T2_NIT_SUPPORT //nit search need test
    AppFrontend_SetTp params = {0};
    memset(&thizDvbt2NitResult , 0, sizeof(thizDvbt2NitResult));
    thizNitParseFlag = 0;

    params.fre = MAIN_FREQ_DEFAULT_VALUE;
    params.type = FRONTEND_DVB_T2;
    params.bandwidth =MAIN_BANDWIDTH_DEFAULT_VALUE ;
    params.data_plp_id = 0xff;
    params.common_plp_exist = 0xff;
    params.common_plp_id = 0xff;

    params.type_1501 = DVBT_AUTO_MODE;
    //dvbt2_search_printf("----fre=%s,fre=%d, bandwidth=%d----\n", Fre_value, params.fre, params.bandwidth);
    app_ioctl(node.sat_data.tuner, FRONTEND_TP_SET, &params);

    GxCore_ThreadDelay(100);

    AppFrontend_LockState lock;
    app_ioctl(node.sat_data.tuner, FRONTEND_LOCK_STATE_GET, &lock);

    if (FRONTEND_LOCKED == lock)
    {
        dvbt2_nit_scan_tp(&msg_dvbt2_scan);
    }
    if(thizDvbt2NitResult.num>0)
    {
        tp_max_num = app_t2_nit_auto_search_get_tp(thizDvbt2SatId,thizDvbt2TpidArray);
    }
    else
#endif
    {
        tp_max_num = app_t2_default_auto_search_get_tp(thizDvbt2SatId,thizDvbt2TpidArray);
    }

    if(tp_max_num <=0)
    {
        dvbt2_search_printf("tp_max_num  = %d !!error\n",tp_max_num);
    }

    app_ioctl(node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
    for(i=0;i<tp_max_num;i++)
    {
        thizDvbt2TsidArray[i] = cfg.ts_src;
    }

    msg_dvbt2_scan.scan_type = GX_SEARCH_MANUAL;
    msg_dvbt2_scan.tv_radio = GX_SEARCH_TV_RADIO_ALL;
    msg_dvbt2_scan.fta_cas = mode;//GX_SEARCH_FTA_CAS_ALL;//mode
    msg_dvbt2_scan.nit_switch = GX_SEARCH_NIT_ENABLE;//nit enable for auto scan
    //dvbt2_search_printf("tp id = %d    %d\n",thizDvbt2TpidArray[0],thizDvbt2TpidArray[1]);

    msg_dvbt2_scan.params_t.sat_id = thizDvbt2SatId ;
    msg_dvbt2_scan.params_t.max_num = tp_max_num;
    msg_dvbt2_scan.params_t.array = thizDvbt2TpidArray;
    msg_dvbt2_scan.params_t.ts = thizDvbt2TsidArray;
    // needn't ext
    msg_dvbt2_scan.ext = NULL;
    msg_dvbt2_scan.ext_num = 0;

    msg_dvbt2_scan.time_out.pat = PAT_FILTER_TIMEOUT;
    msg_dvbt2_scan.time_out.sdt = SDT_FILTER_TIMEOUT;
    msg_dvbt2_scan.time_out.nit = NIT_FILTER_TIMEOUT;
    msg_dvbt2_scan.time_out.pmt = PMT_FILTER_TIMEOUT;

    msg_dvbt2_scan.modify_prog = _dvbt2_modify_prog_callback;

    app_send_msg_exec(GXMSG_SEARCH_SCAN_DVBT2_START,(void*)&msg_dvbt2_scan);

    return 0;
}

#ifdef DVB_T2_NIT_SUPPORT //nit search need test
static uint32_t app_t2_nit_auto_search_get_tp(uint32_t sat_id,uint32_t *tp_array)
{
    int list_num = 0;
    int i = 0;
    int32_t  ret= 0;
    GxBusPmDataTP tp={0};
    //char* freq_value = NULL;
    //uint8_t  width_value;
    //uint32_t freq_tmp = 0;
    uint32_t tpid_temp;

    list_num = thizDvbt2NitResult.num;
    if(TP_MAX_NUM < list_num)
    {
        list_num = TP_MAX_NUM;
    }
    for(i=0;i<list_num;i++)
    {
        ret = GxBus_PmTpExistChek((uint16_t)sat_id,(uint32_t)thizDvbt2NitResult.app_fre_array[i], (uint32_t)thizDvbt2NitResult.app_band_array[i], 0, 0,(uint16_t *)&tpid_temp);
        if(ret == 0)
        {
            tp.sat_id = sat_id;
            tp.frequency = thizDvbt2NitResult.app_fre_array[i];
            {
                tp.tp_t.bandwidth  = thizDvbt2NitResult.app_band_array[i];
            }
            ret = GxBus_PmTpAdd(&tp);
            if (GXCORE_SUCCESS != ret)
            {
                //tp加入不成功
                printf("can't add Tp\n");
                //diag error datebase error
                break;
                //not finish
            }
            tp_array[i]= tp.id;
        }
        else if(ret > 0)
        {
            tp_array[i]  = tpid_temp;
        }
        else
        {
            dvbt2_search_printf("[DVBT2 Search]can't add Tp\n");
            return -1;
        }
    }

    GxBus_PmSync(GXBUS_PM_SYNC_TP);

    return list_num;
}

static int dvbt2_search_nit_section_parse(uint8_t* p_section_data, size_t section_size, classDvbt2SearchNit *nit_result)
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
    dvbt2_search_printf("dvbt2_search_nit_section_parse size=%d len=%d\n",section_size, ((section_data[1]&0x0f)<<8) + section_data[2] + 3 );
    dvbt2_search_printf("dvbt2_search_nit_section_parse section_data[7]=%d section_data[6]=%d\n",section_data[7],section_data[6]);

    if(NULL == nit_result->nit_subtable_flag)
    {
        nit_result->nit_subtable_flag = GxCore_Malloc(MAX_NIT_SECTION_NUM);
        if (NULL == nit_result->nit_subtable_flag)
        {
            dvbt2_search_printf("[APP dvbt2 Search] malloc error : %s\n", __func__);
            return -1;
        }

        dvbt2_search_printf("[APP dvbt2 Search] Malloc nit_subtable_flag\n");
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
        dvbt2_search_printf("[APP dvbt2 Search] First empty Nit flag index: %d\n", i);
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
        dvbt2_search_printf("[APP dvbt2 Search] Get New Nit section \n");
        {
            nitVersion = (section_data[5] & 0x3E) >> 1;
            //app_flash_save_config_center_nit_fre_version(nitVersion);
            freq = nit_result->app_fre_array[0];
            dvbt2_search_printf("MAIN_FREQ_NIT=%d\n", freq);
            dvbt2_search_printf("MAIN_FREQ_NITVERSION=%d\n", nitVersion);
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
                            dvbt2_search_printf("dvbt2_search_nit_section_parse dvbt2 freq=%d\n",freq);
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

static private_parse_status dvbt2_search_nit_section_process_callback(uint8_t* p_section_data, size_t section_size)
{
    int32_t  parse_result = -1;
    thizNitParseFlag = 1;
    //copy data
    parse_result = dvbt2_search_nit_section_parse(p_section_data, section_size, &thizDvbt2NitResult);
    dvbt2_search_printf("\n\n[SI Cabllback] Recv Nit counter: %d\n\n\n", thizDvbt2NitResult.nit_recv_num);
    if(parse_result == 0 || thizDvbt2NitResult.nit_recv_num > MAX_NIT_SECTION_NUM)
    {
        return PRIVATE_SUBTABLE_OK;
    }

    return PRIVATE_SECTION_OK;
}

static void dvbt2_nit_scan_tp(GxMsgProperty_ScanDvbt2Start * p_msg_dvbt2_scan)
{
    GxSubTableDetail subtable_detail = {0};
    GxMsgProperty_SiCreate	msg_si_create = {0};
    GxMsgProperty_SiStart msg_si_start = {0};
    GxTime start_time = {0};
    GxTime local_time = {0};

    //set filter config
    subtable_detail.ts_src = (p_msg_dvbt2_scan->params_t.ts[0]);
    subtable_detail.demux_id = 0;
    subtable_detail.time_out = NIT_FILTER_TIMEOUT;
    subtable_detail.si_filter.pid = NIT_PID;
    subtable_detail.si_filter.match_depth = 1/*5*/;
    subtable_detail.si_filter.eq_or_neq = EQ_MATCH;
    subtable_detail.si_filter.match[0] = NIT_ACTUAL_NETWORK_TID;
    subtable_detail.si_filter.mask[0] = 0xff;
    subtable_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
    subtable_detail.table_parse_cfg.table_parse_fun = dvbt2_search_nit_section_process_callback;
    msg_si_create = &subtable_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE,(void*)&msg_si_create);

    msg_si_start = subtable_detail.si_subtable_id;

    app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&msg_si_start);

    GxCore_GetTickTime(&start_time);
    GxCore_GetTickTime(&local_time);

    while(thizNitParseFlag == 0  &&(start_time.seconds + (NIT_FILTER_TIMEOUT/1000) > local_time.seconds) )
    {
        dvbt2_search_printf("nit wait\n");
        GxCore_GetTickTime(&local_time);
        GxCore_ThreadDelay(100);
    }

    app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&msg_si_start);
    if(NULL != thizDvbt2NitResult.nit_subtable_flag)
    {
        GxCore_Free(thizDvbt2NitResult.nit_subtable_flag);
        thizDvbt2NitResult.nit_subtable_flag = NULL;
        thizDvbt2NitResult.nit_recv_num = 0;
    }
}
#endif

#define TIMER_FUNC
static int timer_dvbt2_show_signal(void *userdata)
{
    unsigned int value = 0;
    char Buffer[20] = {0};
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};

    // progbar strength
    //g_AppNim.property_get(&g_AppNim, AppFrontendID_Strength,(void*)(&value),4);
    //app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_STRENGTH_GET, &value);
    app_ioctl(0, FRONTEND_STRENGTH_GET, &value);
    siganl_bar.strength_bar = "progbar_t2_search_strength";
    siganl_val.strength_val = value;

    // strength value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_t2_search_strength_value", "string", Buffer);

    // progbar quality
    //g_AppNim.property_get(&g_AppNim, AppFrontendID_Quality,(void*)(&value),4);
    //app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_QUALITY_GET, &value);
    app_ioctl(0, FRONTEND_QUALITY_GET, &value);
    siganl_bar.quality_bar= "progbar_t2_search_quality";
    siganl_val.quality_val = value;
    // quality value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_t2_search_quality_value", "string", Buffer);

    //app_signal_progbar_update(app_tuner_cur_tuner_get(), &siganl_bar, &siganl_val);
    app_signal_progbar_update(0, &siganl_bar, &siganl_val);

    ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &value);

    return 0;
}

static int timer_dvbt2_set_frontend(void *userdata)
{
    SetTpMode force = 0;

    remove_timer(sp_Dvbt2LockTimer);
    sp_Dvbt2LockTimer = NULL;

    if(userdata != NULL)
    {
        force = *(SetTpMode*)userdata;
        if(force == SET_TP_FORCE)
        {
            GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
        }
    }
    {
        extern void GxFrontend_ForceExitEx(int32_t tuner);
#if DOUBLE_S2_SUPPORT
        GxFrontend_ForceExitEx(0);
        GxFrontend_ForceExitEx(1);
#else
        GxMsgProperty_NodeByIdGet node = {0};
        node.node_type = NODE_SAT;
        node.id = thizDvbt2SatId;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
        GxFrontend_ForceExitEx(node.sat_data.tuner);
#endif
    }
    app_t2_manual_search_set_tp();
    //_antenna_set_tp(force);
    return 0;
}

static void app_t2_search_init(void)
{
    int list_num = 0;
    int i = 0;
    char *buffer_all = NULL;
    char buffer[MAX_CH_NAME_LEN +1] = {0};
    //int ch_select = 0;

    list_num = AppDvbt2GetFreqList(&thizFreqChannel);

    buffer_all = GxCore_Malloc(list_num * (MAX_CH_NAME_LEN +1)+2);
    memset(buffer_all, 0, (list_num * (MAX_CH_NAME_LEN +1)+2));
    memset(buffer, 0, MAX_CH_NAME_LEN +1);

    //first
    sprintf(buffer_all,"[%s", thizFreqChannel[0].channel);
    for (i = 1;i<list_num;i++)
    {
        sprintf(buffer,",%s", thizFreqChannel[i].channel);
        strcat(buffer_all, buffer);
    }
    strcat(buffer_all, "]");

    GUI_SetProperty(CMB_T2_MANUAL_SEARCH_CH,"content",buffer_all);
    app_t2_search_cmb_channel_change(NULL,NULL);

    GxCore_Free(buffer_all);
}

SIGNAL_HANDLER int app_t2_manual_search_create (GuiWidget *widget, void *usrdata)
{
    app_t2_search_init();

    GUI_SetFocusWidget("box_t2_search_options");
	
    GUI_SetProperty("boxitem_t2_search_channel", "enable", NULL);
    GUI_SetProperty("boxitem_t2_search_freq", "enable", NULL);
    GUI_SetProperty("boxitem_t2_search_bandwidth", "enable", NULL); 
	
    if (reset_timer(thizDvbt2TimerSignal) != 0)
    {
        thizDvbt2TimerSignal = create_timer(timer_dvbt2_show_signal, 100, NULL, TIMER_REPEAT);
    }
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_t2_manual_search_destroy(GuiWidget* widget, void *usrdata)
{
    remove_timer(thizDvbt2TimerSignal);
    thizDvbt2TimerSignal = NULL;
    remove_timer(sp_Dvbt2LockTimer);
    sp_Dvbt2LockTimer = NULL;
    return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_t2_manual_search_keypress(GuiWidget *widget, void *usrdata)
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
                        GUI_EndDialog("wnd_search_setting_dvbt");
                        GUI_SendEvent("wnd_system_setting", event);
                    }
                    break;
                case STBK_OK:
                    {
                        GUI_GetProperty(BOX_T2_MANUAL_OPTIONS, "select", &box_sel);
                        //if(T2_MANUAL_SEARCH_BOX_ITEM_OK == box_sel)
                        {
                            GUI_GetProperty(CMB_T2_MANUAL_SEARCH_MODE, "select", &box_sel);
                            if(1 == box_sel)
                            {
                                AppT2AutoSearchStart(GX_SEARCH_FTA_CAS_ALL);
                                GUI_EndDialog("wnd_search_setting_dvbt");
                            }
                            else
                                app_t2_manual_search_start();
                        }
                    }
                    break;
                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_t2_search_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;

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
			uint32_t box_sel;
			int32_t mode_sel = -1;
    			GUI_GetProperty("cmbox_t2_search_mode", "select", &mode_sel);
                        GUI_GetProperty("box_t2_search_options", "select", &box_sel);
			if(mode_sel == 0) //manual 
			{
				if( box_sel == T2_MANUAL_SEARCH_BOX_ITEM_CH ) { box_sel = T2_MANUAL_SEARCH_BOX_ITEM_OK;
                                	GUI_SetProperty("box_t2_search_options", "select", &box_sel);
                                	ret = EVENT_TRANSFER_STOP;
				}
				else
				{
                                	ret = EVENT_TRANSFER_KEEPON;
				}
			}
			else if(mode_sel == 1)//auto
			{
				if( box_sel == T2_MANUAL_SEARCH_BOX_ITEM_MODE )
                                {
                                        box_sel = T2_MANUAL_SEARCH_BOX_ITEM_OK;
                                        GUI_SetProperty("box_t2_search_options", "select", &box_sel);
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
                        GUI_GetProperty("cmbox_t2_search_mode", "select", &mode_sel);
                        GUI_GetProperty("box_t2_search_options", "select", &box_sel);
			printf("enter %s:%d mode_sel = %d -------------\n",__func__,__LINE__,mode_sel);
			if(mode_sel == 0) //manual 
                        {
				if( box_sel == T2_MANUAL_SEARCH_BOX_ITEM_OK )
                        	{
                                	box_sel = T2_MANUAL_SEARCH_BOX_ITEM_CH;
                                	GUI_SetProperty("box_t2_search_options", "select", &box_sel);
                                	ret = EVENT_TRANSFER_STOP;
                        	}
                        	else
                        	{
                                	ret = EVENT_TRANSFER_KEEPON;
                        	}
			}
			else if(mode_sel == 1)
			{
				if( box_sel == T2_MANUAL_SEARCH_BOX_ITEM_OK )
                                {
                                        box_sel = T2_MANUAL_SEARCH_BOX_ITEM_MODE;
                                        GUI_SetProperty("box_t2_search_options", "select", &box_sel);
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

SIGNAL_HANDLER int app_t2_search_cmb_channel_change(GuiWidget *widget, void *usrdata)
{
    uint32_t sel;
    char freq_str[8];
    GUI_GetProperty(CMB_T2_MANUAL_SEARCH_CH, "select", &sel);

    sprintf(freq_str,"%3.1f",thizFreqChannel[sel].freq);
    GUI_SetProperty(EDIT_T2_MANUAL_SEARCH_FRE,"string",freq_str);
    GUI_SetProperty(CMB_T2_MANUAL_SEARCH_WIDTH,"select",&(thizFreqChannel[sel].band));

    s_set_tp_mode = SET_TP_FORCE;
    if(reset_timer(sp_Dvbt2LockTimer) != 0)
    {
        sp_Dvbt2LockTimer = create_timer(timer_dvbt2_set_frontend, 500, (void *)&s_set_tp_mode, TIMER_ONCE);
    }
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_t2_search_cmb_freq_change(GuiWidget *widget, void *usrdata)
{
    s_set_tp_mode = SET_TP_FORCE;
   
    if(reset_timer(sp_Dvbt2LockTimer) != 0)
    {
        sp_Dvbt2LockTimer = create_timer(timer_dvbt2_set_frontend, 500, (void *)&s_set_tp_mode, TIMER_ONCE);
    }
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_t2_search_cmb_bandwidth_change(GuiWidget *widget, void *usrdata)
{
    s_set_tp_mode = SET_TP_FORCE;
    if(reset_timer(sp_Dvbt2LockTimer) != 0)
    {
        sp_Dvbt2LockTimer = create_timer(timer_dvbt2_set_frontend, 500, (void *)&s_set_tp_mode, TIMER_ONCE);
    }
    return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_t2_search_cmb_mode_change(GuiWidget *widget, void *usrdata)
{
    uint32_t box_sel;

    GUI_GetProperty("cmbox_t2_search_mode", "select", &box_sel);

    if(box_sel == 1)
    {
        GUI_SetProperty("boxitem_t2_search_channel", "disable", NULL);
        GUI_SetProperty("boxitem_t2_search_freq", "disable", NULL);
        GUI_SetProperty("boxitem_t2_search_bandwidth", "disable", NULL);
    }
    else
    {
        GUI_SetProperty("boxitem_t2_search_channel", "enable", NULL);
        GUI_SetProperty("boxitem_t2_search_freq", "enable", NULL);
        GUI_SetProperty("boxitem_t2_search_bandwidth", "enable", NULL);
    }
    return EVENT_TRANSFER_STOP;
}

enum DVBT_SETTING_ITEM_NAME
{
    ITEM_COUNTRY_SELECT,
    ITEM_DVBT_SETTING_TOTAL
};
typedef struct
{
    enum _country_list country_list;
}SysDvbTPara;

static SystemSettingOpt s_dvbt_setting_opt;
static SystemSettingItem s_dvbt_item[ITEM_DVBT_SETTING_TOTAL];
static SysDvbTPara s_dvbt__para;

static void app_country_select_item_init(void)
{
    static char s_country_array[ITEM_COUNTRY_MAX*MAX_COUNTRY_NAME_LEN] = {0};
    int i=0;
    strcpy(s_country_array, "[");
    for (i=0; i<ITEM_COUNTRY_MAX; i++)
    {
        strcat(s_country_array, AppDvbt2GetCountryName(i));
        strcat(s_country_array, ",");
    }
    strcat(s_country_array, "]");
    s_dvbt_item[ITEM_COUNTRY_SELECT].itemTitle = "Country";
    s_dvbt_item[ITEM_COUNTRY_SELECT].itemType = ITEM_CHOICE;
    s_dvbt_item[ITEM_COUNTRY_SELECT].itemProperty.itemPropertyCmb.content= s_country_array;
    s_dvbt_item[ITEM_COUNTRY_SELECT].itemProperty.itemPropertyCmb.sel = s_dvbt__para.country_list;
    s_dvbt_item[ITEM_COUNTRY_SELECT].itemCallback.cmbCallback.CmbChange = NULL;
    s_dvbt_item[ITEM_COUNTRY_SELECT].itemStatus = ITEM_NORMAL;
}

static void app_dvbt_result_para_get(SysDvbTPara *ret_para)
{
    ret_para->country_list= s_dvbt_item[ITEM_COUNTRY_SELECT].itemProperty.itemPropertyCmb.sel;
}

static bool app_dvbt_setting_para_change(SysDvbTPara *ret_para)
{
    bool ret = FALSE;
    if(s_dvbt__para.country_list != ret_para->country_list)
    {
        ret = TRUE;
    }

    return ret;
}

static int app_dvbt_setting_exit_callback(ExitType exit_type)
{
    SysDvbTPara para_ret = {0};
    int ret = 0;

    app_dvbt_result_para_get(&para_ret);
    if(app_dvbt_setting_para_change(&para_ret) == true)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_SAVE_INFO;
        if(popdlg_create(&pop) == POP_VAL_OK)
        {
            AppDvbt2SetCountry(para_ret.country_list);
            ret =1;
        }
    }
    return ret;
}

void app_country_setting_menu_exec(void)
{
    memset(&s_dvbt_setting_opt, 0 ,sizeof(s_dvbt_setting_opt));

    s_dvbt__para.country_list = AppDvbt2GetCountry();

    s_dvbt_setting_opt.menuTitle = STR_ID_COUNTRY_SET;
    s_dvbt_setting_opt.itemNum = ITEM_DVBT_SETTING_TOTAL;
    s_dvbt_setting_opt.timeDisplay = TIP_HIDE;
    s_dvbt_setting_opt.item = s_dvbt_item;
    app_country_select_item_init();

    s_dvbt_setting_opt.exit = app_dvbt_setting_exit_callback;

    app_system_set_create(&s_dvbt_setting_opt);
}
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
#if (DEMOD_DVB_COMBO_T > 0)
enum DVBT2_NAME
{
    ITEM_T2_SEARCH = 0,
    ITEM_T2_FREQ_LIST,
    ITEM_T2_TOTAL,
};

static SystemSettingOpt thizDvbt2SearchOpt = {0};
static SystemSettingItem thizDvbt2SearchItem[ITEM_T2_TOTAL];
static int app_t2_search_button_press_callback(unsigned short key)
{
	if(key == STBK_OK)
	{   
		GUI_CreateDialog("wnd_search_setting_dvbt");
	}
	return EVENT_TRANSFER_KEEPON;
}
static int app_t2_freq_list_button_press_callback(unsigned short key)
{
	if(key == STBK_OK)
	{   
		app_country_setting_menu_exec();
	}
	return EVENT_TRANSFER_KEEPON;
}

static void app_t2_search_item_init(void)
{
	thizDvbt2SearchItem[ITEM_T2_SEARCH].itemTitle = "Search";
	thizDvbt2SearchItem[ITEM_T2_SEARCH].itemType = ITEM_PUSH;
	thizDvbt2SearchItem[ITEM_T2_SEARCH].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	thizDvbt2SearchItem[ITEM_T2_SEARCH].itemCallback.btnCallback.BtnPress= app_t2_search_button_press_callback;
	thizDvbt2SearchItem[ITEM_T2_SEARCH].itemStatus = ITEM_NORMAL;
}
static void app_t2_freqlist_item_init(void)
{
	thizDvbt2SearchItem[ITEM_T2_FREQ_LIST].itemTitle = "Freq list";
	thizDvbt2SearchItem[ITEM_T2_FREQ_LIST].itemType = ITEM_PUSH;
	thizDvbt2SearchItem[ITEM_T2_FREQ_LIST].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	thizDvbt2SearchItem[ITEM_T2_FREQ_LIST].itemCallback.btnCallback.BtnPress= app_t2_freq_list_button_press_callback;
	thizDvbt2SearchItem[ITEM_T2_FREQ_LIST].itemStatus = ITEM_NORMAL;
}

void Dvbt2SearchCreateDialog(void)
{	
	memset(&thizDvbt2SearchOpt, 0 ,sizeof(thizDvbt2SearchOpt));

	thizDvbt2SearchOpt.menuTitle = STR_ID_DVBT2_SEARCH;
	thizDvbt2SearchOpt.titleImage = "s_title_left_utility.bmp";
	thizDvbt2SearchOpt.itemNum = ITEM_T2_TOTAL;
	thizDvbt2SearchOpt.timeDisplay = TIP_HIDE;
	thizDvbt2SearchOpt.item = thizDvbt2SearchItem;
		
	app_t2_search_item_init();
	app_t2_freqlist_item_init();
	
	thizDvbt2SearchOpt.exit = NULL;

	app_system_set_create(&thizDvbt2SearchOpt);
}
#endif
#else //DEMOD_DVB_T
SIGNAL_HANDLER int app_t2_manual_search_create (GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_t2_manual_search_destroy(GuiWidget* widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_t2_manual_search_keypress(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_t2_search_box_keypress(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_t2_search_cmb_channel_change(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_t2_search_cmb_freq_change(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_t2_search_cmb_bandwidth_change(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_t2_search_cmb_mode_change(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
#endif//DEMOD_DVB_T
