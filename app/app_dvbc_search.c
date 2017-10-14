#include "app.h"
#if (DEMOD_DVB_C > 0)
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "app_frontend.h"
#include "app_pop.h"
#include "app_epg.h"
#include "full_screen.h"

/* Private variable------------------------------------------------------- */
#define MANUSAL_SEARCH_FREQ_DV       (299)
#define MANUSAL_SEARCH_SYMBOL_DV   (6875)
#define MANUSAL_SEARCH_QAM_DV         (2)
#define FRE_BEGIN_LOW (115)
#define FRE_BEGIN_HIGH (858)
#define DVB_TS_SRC								(0)
#define SAT_MAX_NUM									(1)
#define TP_MAX_NUM									(200)
#define SERVICE_MAX_NUM								(1000)
#define PAT_FILTER_TIMEOUT							(3000)
#define SDT_FILTER_TIMEOUT							(5000)
#define NIT_FILTER_TIMEOUT							(10000)
#define PMT_FILTER_TIMEOUT							(8000)
#define CABLE_NETWORK_NAME          ("Cable(DVB-C)")
#define NETWORK_ID			0xffff//(0x01)


typedef struct
{
    uint32_t fre;
    uint32_t symbol_rate;
    uint32_t qam;
    uint8_t  flag;//错误的频点参数标志

}search_dvbc_param;

typedef struct
{
    const char * widget_name_strength_bar;  /*信号强度百分条*/
    const char * widget_name_strength_bar_value;  /*信号强度值*/
    const char* widget_name_strength_bar_value_format; /* 信号强度值格式如db , % */
    char        lock_status; /*1锁定，0失锁*/
    char        lock_status_invaild; /*0 in vaild , 1 vaild*/
    char        unit; /*0 dB , 1 %*/

}strength_xml;

typedef struct
{
    const char * widget_name_signal_bar;  /*信号质量百分条*/
    const char * widget_name_signal_bar_value; /*信号强度百分值显示db*/
    const char * widget_name_error_rate_bar; /*误码率百分条*/
    const char * widget_name_error_rate; /*误码率值显示
                                           格式"%d.%02dE-%02d"*/
    char       lock_status;
    char       lock_status_invaild; /*0  vaild , 1 invaild*/
    char	   unit; /*0 dBuV mode, 1 % mode*/

}signal_xml;

typedef struct
{
    uint8_t app_buf_tv[400];
    uint8_t app_buf_radio[400];
    uint16_t  app_tv_num ;
    uint16_t  app_radio_num ;
    uint16_t tp_num; /*实际搜索TP个数，超过最大TP个数情况下，
                       tp_num小于设置的个数*/
    uint16_t tp_cur; /*当前搜索TP索引*/
    uint32_t app_tpid[TP_MAX_NUM]; /*实际搜索TP对应ID，超过最大TP个数情况下，
                                     个数小于设置的个数*/
    uint32_t app_tv_num_perTP[TP_MAX_NUM]; /*每个TP对应搜索到的电视节目数*/
    uint32_t app_radio_num_perTP[TP_MAX_NUM]; /*每个TP对应搜索到的广播节目数*/

}search_result;

typedef struct
{
    uint16_t app_fre_array[TP_MAX_NUM];
    uint16_t app_qam_array[TP_MAX_NUM];
    uint16_t app_symb_array[TP_MAX_NUM];
    uint16_t app_fre_tsid[TP_MAX_NUM];
    uint16_t num;
    uint8_t nit_flag;
}search_fre_list;

/*enum BOX_ITEM_NAME
  {
  BOX_ITEM_COUNTRY = 0,
  BOX_ITEM_FREQ_NUM,
  BOX_ITEM_FREQ_VALUE,
  BOX_ITEM_BANDWIDTH,
  BOX_ITEM_PROG_MODE,
  BOX_ITEM_SEARCH_MODE,
  };*/
enum BOX_ITEM_NAME
{
    BOX_ITEM_FREQ_VALUE = 0,
    BOX_ITEM_SYMBOL,
    BOX_ITEM_QAM,
    BOX_ITEM_NIT,
    BOX_ITEM_SEARCH_MODE,
};

enum
{
    SEARCH_SEL_AUTO,
    SEARCH_SEL_MANUAL,
};


//#define PROG_MODE_CONTENT	"[FTA,Scrambled,FTA&Scrambled]"
#define QAM_CONTENT	"[16QAM,32QAM,64QAM,128QAM,256QAM]"
#define SEARCH_MODE_CONTENT	"[Auto,Manual]"

static search_dvbc_param previous_param={0};
static int freq_diff_prog = 0;
static GxFrontend sProg_Tp = {0};
static event_list* spmanualsearchtime = NULL;
static GxBusPmDataSat thizNetworkCablePmData ;

handle_t         sApp_frontend_device_handle = -1;
handle_t         sApp_frontend_demux_handle = -1;
search_fre_list searchFreList ;
search_result searchresultpara;
uint8_t* pNitSectionFlag = NULL;
static int32_t s_NitSubtId = -1;
static uint8_t app_nit_start_flag = 0;
static uint32_t s_NitRequestId = 0;

extern void app_table_nit_search_filter_close(void);
extern void app_table_nit_get_search_filter_info(int32_t* pNitSubtId,uint32_t* pNitRequestId);
extern void app_search_scan_cable_start(search_fre_list searchlist);
extern private_parse_status app_table_nit_search_section_parse(uint8_t* p_section_data, size_t Size);
extern status_t app_search_lock_tp(uint32_t fre, uint32_t symb, fe_spectral_inversion_t inversion, fe_modulation_t modulation,uint32_t delayms);
extern void app_search_set_search_type(SearchType type);

void app_search_si_subtable_msg(GxMessage * msg)
{
    GxMsgProperty_SiSubtableOk *parse_result = NULL;
    int32_t NitSubtId=0;
    uint32_t NitRequestId=0;
    uint32_t symbol_rate = 0;
    uint32_t qam = 0;
    if (NULL == msg)
    {
        return ;
    }
    switch(msg->msg_id)
    {
        case GXMSG_SI_SUBTABLE_OK:
            /*
             * 所有nit表section 过滤成功
             */
            symbol_rate = MANUSAL_SEARCH_SYMBOL_DV;
            qam = MANUSAL_SEARCH_QAM_DV;
            app_table_nit_get_search_filter_info(&NitSubtId,&NitRequestId);
            parse_result = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_SiSubtableOk);
            if((NitSubtId == parse_result->si_subtable_id) && (NitRequestId == parse_result->request_id))
            {
                app_table_nit_search_filter_close();
                //	if(TRUE == app_table_nit_get_networkid_flag())
                {
                    if(parse_result->table_id == 0x40)
                    {
                        searchFreList.nit_flag = GX_SEARCH_NIT_DISABLE;
                        app_search_scan_cable_start(searchFreList);
                    }
                    else
                    {
                        printf("\n####error####service_msg_to_app:%d#######\n",parse_result->table_id );
                    }
                }
                //	else
                //	{
                /*
                 * 必须等待NIT收到或超时退出，否则会出现死机
                 */
                //		app_search_scan_stop();
                //	}
            }
            return ;
        case GXMSG_SI_SUBTABLE_TIME_OUT:
            /*
             * nit表过滤超时
             */
            symbol_rate = MANUSAL_SEARCH_SYMBOL_DV;
            qam = MANUSAL_SEARCH_QAM_DV;
            app_table_nit_get_search_filter_info(&NitSubtId,&NitRequestId);
            parse_result = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_SiSubtableOk);
            if((NitSubtId == parse_result->si_subtable_id) && (NitRequestId == parse_result->request_id))
            {
                printf("filter nit table timeout  fre = %d!!\n",searchFreList.app_fre_array[0]);
                app_table_nit_search_filter_close();
                { // 备用主频点NIT过滤超时
                    searchFreList.nit_flag = GX_SEARCH_NIT_DISABLE;
                    app_search_scan_cable_start(searchFreList);
                }

            }
            return ;
        default:
            break;
    }

    return ;
}

void app_table_nit_search_filter_close(void)
{
    GxMsgProperty_SiRelease params_release;
    if (NULL !=pNitSectionFlag )
    {
        GxCore_Free(pNitSectionFlag);
        pNitSectionFlag = NULL;
    }

    if (-1 != s_NitSubtId)
    {
        params_release = s_NitSubtId;
        //app_send_msg(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
        app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
    }
    s_NitSubtId = -1;
    app_nit_start_flag = 0;/*NIT搜索FILTER标志清零*/

}

void app_table_nit_search_filter_open(void)
{

    static GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiCreate	params_create;
    GxMsgProperty_SiStart params_start;


    if (NULL !=pNitSectionFlag )
    {
        GxCore_Free(pNitSectionFlag);
        pNitSectionFlag = NULL;
    }
    app_nit_start_flag = 1; /*NIT搜索FILTER标志置1*/
    //	nit_monitor_close();
    //	app_table_nit_monitor_filter_close(); /*关闭后台监测NIT表FILTER*/

    if(s_NitSubtId == -1)
    {
        AppFrontend_Config cfg = {0};
        app_ioctl(0, FRONTEND_CONFIG_GET, &cfg);	 
        subt_detail.ts_src = cfg.ts_src; // multi-ts need this TS1-0 TS2-1 TS3-2
        subt_detail.demux_id = 0;
        subt_detail.time_out = 10000;
        subt_detail.si_filter.pid = NIT_PID;
        subt_detail.si_filter.match_depth = 1/*5*/;
        subt_detail.si_filter.eq_or_neq = EQ_MATCH;
        subt_detail.si_filter.match[0] = NIT_ACTUAL_NETWORK_TID;
        subt_detail.si_filter.mask[0] = 0xff;
        params_create = &subt_detail;

        subt_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
        subt_detail.table_parse_cfg.table_parse_fun = app_table_nit_search_section_parse;

        //app_send_msg(GXMSG_SI_SUBTABLE_CREATE,(void*)&params_create);
        app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);

        s_NitSubtId = subt_detail.si_subtable_id;
        s_NitRequestId = subt_detail.request_id;

        // start si
        params_start = s_NitSubtId;
        //app_send_msg(GXMSG_SI_SUBTABLE_START, (void*)&params_start);
        app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);
    }
}

void app_table_nit_get_search_filter_info(int32_t* pNitSubtId,uint32_t* pNitRequestId)
{
    if ((NULL == pNitSubtId)||(NULL == pNitRequestId))
        return;

    *pNitSubtId = s_NitSubtId ;
    *pNitRequestId = s_NitRequestId;
    return;
}

/*
 * NIT搜索，分析NIT表
 */

uint8_t zonecode;

private_parse_status app_table_nit_search_section_parse(uint8_t* p_section_data, size_t Size)
{
    uint8_t *date, *date1;
    int16_t len1 = 0;
    int16_t len2 = 0;
    int16_t len3 = 0;
    int16_t len4 = 0;
    int16_t i = 0;
    int16_t j = 0;
    uint32_t		freq = 0;
    uint32_t		symbol = 0;
    uint32_t		modulation = 0;
    int32_t nitVersion = 0;
    int32_t nitNetworkid = 0;
    uint16_t temp = 0;
    uint32_t ret = PRIVATE_SECTION_OK;
    uint16_t ts_id;
    date = p_section_data;


    if (NULL == p_section_data)
        return ret;
    zonecode = date[4];
    if(NULL == pNitSectionFlag)
    {
        pNitSectionFlag = GxCore_Malloc(date[7]+1);
        if (NULL == pNitSectionFlag)
            return ret;
        memset(pNitSectionFlag,0,date[7]+1);
    }
    printf("app_table_nit_search_section_parse Size=%d len=%d\n",Size, ((date[1]&0x0f)<<8) + date[2] + 3 );
    printf("app_table_nit_search_section_parse date[7]=%d date[6]=%d\n",date[7],date[6]);
    if(date[0] == NIT_ACTUAL_NETWORK_TID)
    {
        pNitSectionFlag[date[6]] = 1;
        for(i = 0;i<=date[7];i++)
        {
            if(pNitSectionFlag[i]!=1)
            {
                break;
            }
        }
        if(i == (date[7]+1))
        {
            GxCore_Free(pNitSectionFlag);
            pNitSectionFlag = NULL;
            ret = PRIVATE_SUBTABLE_OK;
        }
        nitNetworkid = (date[3]<<8)|(date[4]);
        if((nitNetworkid != NETWORK_ID) && (NETWORK_ID != 0xffff))
        {
            GxCore_Free(pNitSectionFlag);
            pNitSectionFlag = NULL;
            //osd_msg_enable(OSD_MSG_NETWORK_ERR);
            return PRIVATE_SUBTABLE_OK;
        }
        else
        {
            //osd_msg_disable(OSD_MSG_NETWORK_ERR);
        }
        {
            nitVersion = (date[5] & 0x3E) >> 1;
            //app_flash_save_config_center_nit_fre_version(nitVersion);
            //nit_monitor_version = nitVersion;
            freq = searchFreList.app_fre_array[0];
            printf("MAIN_FREQ_NIT=%d\n", freq);
            printf("MAIN_FREQ_NITVERSION=%d\n", nitVersion);
            freq = 0;
        }
        len1 = ((date[8]&0xf)<<8)+date[9];
        len2 = ((date[9+len1+1]&0xf)<<8)+date[9+len1+2];
        date +=(9+1+len1+2);
        while(len2>0)
        {
            ts_id = ((date[0]<<8)&0xff00)+date[1];
            len4 = len3 = ((date[4]&0xf)<<8)+date[5];
            date1 = &date[6];

            while(len3>0)
            {
                switch(date1[0])
                {
                    case 0x83:
                        break;
                    case 0x44://CABLE_DELIVERY_SYSTEM_DESCRIPTOR:
                        { // -c
                            freq = ((date1[2] & 0xf0)>>4) * 10000000 + (date1[2] & 0xf) * 1000000
                                + ((date1[3] & 0xf0)>>4) * 100000 + (date1[3] & 0xf) * 10000
                                + ((date1[4] & 0xf0)>>4) * 1000 + (date1[4] & 0xf) * 100
                                + ((date1[5] & 0xf0) >> 4) * 10 + (date1[5] & 0xf);
                            symbol = ((date1[9] & 0xf0)>>4) * 1000000 + (date1[9] & 0xf) * 100000
                                + ((date1[10] & 0xf0)>>4) * 10000 + (date1[10] & 0xf) * 1000
                                + ((date1[11] & 0xf0)>>4) * 100 + (date1[11] & 0xf) * 10
                                + ((date1[12] & 0xf0) >> 4) ;
                            symbol = symbol /10;  // k
                            freq= freq/10; // khz
                            freq = freq/1000;
                            modulation = date1[8];
                            printf("app_table_nit_search_section_parse cable freq=%d\n",freq);
                            printf("app_table_nit_search_section_parse cable modulation=%d\n",modulation);
                            for (i=0; i< searchFreList.num;i++)
                            {
                                if (freq == searchFreList.app_fre_array[i])
                                {
                                    searchFreList.app_fre_tsid[i]=ts_id;
                                    break;
                                }
                            }
                            if (i == searchFreList.num)
                            {
                                if (freq > 0)
                                {
                                    searchFreList.app_fre_array[searchFreList.num]=freq;
                                    searchFreList.app_symb_array[searchFreList.num]=symbol;
                                    searchFreList.app_qam_array[searchFreList.num]=modulation-1;
                                    searchFreList.app_fre_tsid[searchFreList.num]=ts_id;
                                    searchFreList.num++;
                                }
                            }
                        }
                        break;
                        //(DVB_DEMOD_DVBT2== DVB_DEMOD_MODE)
                    case 0x5A://TERRSTRIAL_DELIVERY_SYSTEM_DESCRIPTOR:
                        {
                            freq = (uint32_t)((date1[2]<<24 | date1[3]<<16 | date1[4]<<8 | date1[5])*10/1000); // khz
                            modulation=date1[6]>>5;
                            freq = freq/1000;
                            printf("nit_descriptor_parse ter freq=%d\n",freq);

                            for (i=0; i< searchFreList.num;i++)
                            {
                                if (freq == searchFreList.app_fre_array[i])
                                {
                                    searchFreList.app_fre_tsid[i]=ts_id;
                                    break;
                                }
                            }

                            if (i == searchFreList.num)
                            {
                                if (freq > 0)
                                {
                                    searchFreList.app_fre_array[searchFreList.num]=freq;
                                    searchFreList.app_symb_array[searchFreList.num]=MANUSAL_SEARCH_SYMBOL_DV;
                                    searchFreList.app_qam_array[searchFreList.num]=modulation;//app_flash_get_config_center_freq_qam();
                                    searchFreList.app_fre_tsid[searchFreList.num]=ts_id;
                                    searchFreList.num++;
                                }
                            }
                        }
                        break;

                    default:
                        break;
                }
                len3 -= (date1[1]+2);
                date1 += (date1[1]+2);
            }
            date +=(6+len4);
            len2 -=(6+len4);
        }
    }

    /*
     * 频点排序，按频点大小顺序搜索
     */
    if (PRIVATE_SUBTABLE_OK == ret)
    {
        for (i = 0; i< searchFreList.num-1;i++)
            for (j=i+1;j<searchFreList.num;j++)
            {
                if (searchFreList.app_fre_array[j]<searchFreList.app_fre_array[i])
                {
                    temp = searchFreList.app_fre_array[j];
                    searchFreList.app_fre_array[j] = searchFreList.app_fre_array[i];
                    searchFreList.app_fre_array[i] = temp;

                    temp = searchFreList.app_symb_array[j];
                    searchFreList.app_symb_array[j] = searchFreList.app_symb_array[i];
                    searchFreList.app_symb_array[i] = temp;

                    temp = searchFreList.app_qam_array[j];
                    searchFreList.app_qam_array[j] = searchFreList.app_qam_array[i];
                    searchFreList.app_qam_array[i] = temp;

                    temp = searchFreList.app_fre_tsid[j];
                    searchFreList.app_fre_tsid[j] = searchFreList.app_fre_tsid[i];
                    searchFreList.app_fre_tsid[i] = temp;
                }
            }
        GxCore_ThreadDelay(50);
    }
    return ret;
}

int app_win_check_fre_vaild(uint32_t fre)
{
    char buf[256];
    PopDlg pop;

    if (fre < FRE_BEGIN_LOW)
    {
        {
            sprintf((void*)buf,"Frequency error , should not less than %03d",
                    FRE_BEGIN_LOW);
        }
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = buf;
        pop.creat_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }

    if (fre > FRE_BEGIN_HIGH)
    {
        {
            sprintf((void*)buf,"Frequency error , should not larger than %03d",
                    FRE_BEGIN_HIGH);
        }
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = buf;
        pop.creat_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }

    return TRUE;


}

int app_win_check_sym_vaild(uint32_t sym)
{
    char buf[256];
    PopDlg pop;

    if (sym < 2000)
    {
        {
            sprintf((void*)buf,"Symbol rate error , sym should not less than %03d",
                    2000);
        }
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = buf;
        pop.creat_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }

    if (sym > 7000)
    {
        {
            sprintf((void*)buf,"Symbol rate error  , sym should not larger than %03d",
                    7000);
        }
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = buf;
        pop.creat_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }
    return TRUE;
}

int app_win_check_fre_range_vaild(uint32_t lowfre,uint32_t highfre)
{
    char buf[256];
    PopDlg pop;

    if (FALSE == app_win_check_fre_vaild(lowfre))
    {
        return FALSE;
    }

    if (FALSE == app_win_check_fre_vaild(highfre))
    {
        return FALSE;
    }

    if (lowfre + 8 > highfre)
    {
        {
            sprintf((void*)buf,"Frequency error , the end fre should larger than begin fre 8M or more");
        }
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = buf;
        pop.creat_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }
    return TRUE;
}

static void _cable_search_init_config(void)
{
    GxBusPmDataSat cable_network;
    int32_t network_num = 0;
    int32_t network_id = 0;
    int32_t index = 0;
    int32_t ret = 0;

    network_num = GxBus_PmSatNumGet();
    if( network_num <= 0)
    {
        printf("[APP Cable Search] cannot find sat when init\n");
        return;
    }
    else
    {
        printf("[APP Cable Search] get sat num from DB: %d\n", network_num);
    }

    // Look up Cable network , then return
    for(index = 0; index < network_num; index++)
    {
        memset(&cable_network, 0, sizeof(GxBusPmDataSat));
        // read one sat from database once
        ret = GxBus_PmSatsGetByPos(index, 1, &cable_network);
        if( 1 == ret )
        {
            network_id = cable_network.id;
            if(GXBUS_PM_SAT_C == cable_network.type )
            {
                //if(CABLE_NETWORK_NAME == cable_network.sat_c.reserved)
                if(strcmp((char*)cable_network.sat_s.sat_name, CABLE_NETWORK_NAME) == 0)
                {
                    memcpy(&thizNetworkCablePmData, &cable_network, sizeof(GxBusPmDataSat));
                    printf("[APP Cable Search] in database find Cable: id = %d\n", network_id);

                    return;
                }
                else
                {
                    printf("[APP Cable Search] this cable is not what you want\n");
                }
            }
        }

    }

    // Add a new cable sat into Pmdata if not find
    thizNetworkCablePmData.type = GXBUS_PM_SAT_C;
    thizNetworkCablePmData.tuner = 0;
    memcpy(thizNetworkCablePmData.sat_s.sat_name, CABLE_NETWORK_NAME,sizeof(CABLE_NETWORK_NAME));
    GxBus_PmSatAdd(&thizNetworkCablePmData);
    GxBus_PmSync(GXBUS_PM_SYNC_SAT);
    // reread the PmData
    ret = GxBus_PmSatsGetByPos(network_num, 1, &thizNetworkCablePmData);
    if(1 == ret)
    {
        printf("[APP Cable Search] add a new cable network: id = %d, name = %d\n",thizNetworkCablePmData.id, thizNetworkCablePmData.sat_c.reserved);
    }
    else
    {
        printf("[APP Cable Search] get cable network error after adding\n");
    }

    return;
}

void app_search_scan_nit_mode(void)
{
    uint32_t symbol_rate = 0;
    uint32_t qam = 0;
    int32_t fre = 0;

    symbol_rate = MANUSAL_SEARCH_SYMBOL_DV;
    qam = MANUSAL_SEARCH_QAM_DV;
    GxBus_ConfigGetInt(DVBC_FREQ_KEY, &fre, DVBC_FREQ_VALUE);

    app_table_nit_search_filter_close();

    if(GxBus_PmSatGetById(thizNetworkCablePmData.id, &thizNetworkCablePmData) == GXCORE_ERROR)
    {
        _cable_search_init_config();
    }
    else
    {
        GxBus_PmSatDelete(&(thizNetworkCablePmData.id),1);
    }

    memset(&searchFreList,0,sizeof(search_fre_list));

    searchFreList.num = 1;
    searchFreList.app_fre_array[0] = fre;
    searchFreList.app_qam_array[0]=qam;
    searchFreList.app_symb_array[0]=symbol_rate;
    app_table_nit_search_filter_open();

    return ;
}

int app_win_auto_search(void)
{
    uint32_t symbol_rate = 0;
    uint32_t qam = 0;
    int32_t fre = 0;
    GxFrontend sProg_Tp = {0};
    PopDlg pop;

    GxFrontend_GetCurFre(0,&sProg_Tp);

    symbol_rate = MANUSAL_SEARCH_SYMBOL_DV;
    qam = MANUSAL_SEARCH_QAM_DV;
    GxBus_ConfigGetInt(DVBC_FREQ_KEY, &fre, DVBC_FREQ_VALUE);

    GxFrontend_StopMonitor(0);

    if (0 == app_search_lock_tp(fre, symbol_rate,
                INVERSION_OFF, qam,1000))
    {
        app_search_set_search_type(SEARCH_CABLE);

        GUI_CreateDialog("wnd_search");
        GUI_SetInterface("flush", NULL);
        app_search_scan_nit_mode();
    }
    else
    {
        GxFrontend_StartMonitor(0);
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = "Lock Failed";
        pop.creat_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        app_search_lock_tp(sProg_Tp.fre/1000, sProg_Tp.symb /1000,
                INVERSION_OFF, sProg_Tp.qam -1,0);
        return EVENT_TRANSFER_STOP;
    }

    return EVENT_TRANSFER_KEEPON;
}


void app_search_scan_manual_mode(uint32_t fre,uint32_t symbol_rate,uint32_t qam,uint32_t nit_flag)
{
    if(nit_flag == 0)
    {
        memset(&searchFreList,0,sizeof(search_fre_list));
        searchFreList.app_fre_array[0] = fre;
        searchFreList.app_qam_array[0] = qam;
        searchFreList.app_symb_array[0] = symbol_rate;
        searchFreList.num = 1;
        searchFreList.nit_flag = GX_SEARCH_NIT_DISABLE;
        app_search_scan_cable_start(searchFreList);
    }
    else
    {
        GxFrontend_StopMonitor(0);

        if(GxBus_PmSatGetById(thizNetworkCablePmData.id, &thizNetworkCablePmData) == GXCORE_ERROR)
        {
            _cable_search_init_config();
        }
        else
        {
            GxBus_PmSatDelete(&(thizNetworkCablePmData.id),1);
        }
        memset(&searchFreList,0,sizeof(search_fre_list));

        searchFreList.num = 1;
        searchFreList.app_fre_array[0] = fre;
        searchFreList.app_qam_array[0]=qam;
        searchFreList.app_symb_array[0]=symbol_rate;
        app_table_nit_search_filter_open();
    }
    return ;
}

void _search_ok_keypress(void)
{
    uint32_t symbol_rate = 0;
    uint32_t qam = 0;
    uint32_t fre = 0;
    char *value;
    uint32_t value2;
    uint32_t nit_flag;
    PopDlg pop;

    GUI_GetProperty("edit_search_freq", "string", &value);
    fre = atoi(value);
	if (FALSE ==  app_win_check_fre_vaild(fre))
	{
		return;						
	}

    GUI_GetProperty("edit_search_sym", "string", &value);
    symbol_rate = atoi(value);
	if (FALSE ==  app_win_check_sym_vaild(symbol_rate))
	{
		return;						
	}

    GxCore_ThreadDelay(100);

    if(1 != GxFrontend_QueryStatus(0))
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = "Lock Failed";
        pop.creat_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return;
    }
    freq_diff_prog = 0;
    GUI_GetProperty("cmbox_search_qam", "select", &value2);
    qam = value2;

    GUI_GetProperty("cmbox_search_nit", "select", &nit_flag);
    app_search_set_search_type(SEARCH_CABLE);

    GUI_CreateDialog("wnd_search");
    GUI_SetInterface("flush", NULL);
    app_search_scan_manual_mode(fre,symbol_rate,qam,nit_flag);
}

void app_search_scan_cable_start(search_fre_list searchlist)
{
    GxBusPmDataSat sat_arry={0};
    GxBusPmDataTP tp={0};
    uint16_t i=0;
    int32_t Config;

    uint16_t tpid_temp;
    uint32_t tpid_temp1;
    int32_t ret;
    static uint32_t ts_id = DVB_TS_SRC;
    AppFrontend_Config cfg = {0};
    app_ioctl(0, FRONTEND_CONFIG_GET, &cfg);	 
    ts_id = cfg.ts_src; 
	
    GxMsgProperty_ScanCableStart dvbc_scan={0,};

    GxFrontend_StopMonitor(0);
    Config = 0;

    memset(&searchresultpara,0,sizeof(search_result));
    /*
     * 搜索节目前，释放监测表FILTER
     */

    memset(&dvbc_scan,0,sizeof(GxMsgProperty_ScanCableStart));

    if(GxBus_PmSatGetById(thizNetworkCablePmData.id, &thizNetworkCablePmData) == GXCORE_ERROR)
    {
        _cable_search_init_config();
    }
    memcpy(&sat_arry, &thizNetworkCablePmData, sizeof(GxBusPmDataSat));
    for(i = 0;i<searchlist.num;i++)
    {

        ret = GxBus_PmTpExistChek(sat_arry.id, searchlist.app_fre_array[i]*1000, searchlist.app_symb_array[i]*1000, 0, searchlist.app_qam_array[i]+1,&tpid_temp);

        if(ret == 0)
        {
            tp.sat_id = sat_arry.id;
            tp.frequency = searchlist.app_fre_array[i]*1000;
            {
                tp.tp_c.modulation  = searchlist.app_qam_array[i]+1;
                tp.tp_c.symbol_rate = searchlist.app_symb_array[i]*1000;
            }
            ret = GxBus_PmTpAdd(&tp);
            if (GXCORE_SUCCESS != ret)
            {
                //tp加入不成功
                printf("can't add Tp\n");
                break;
            }

            searchresultpara.app_tpid[i] = tp.id;
        }
        else if(ret > 0)
        {
            tp.sat_id = sat_arry.id;
            tp.frequency = searchlist.app_fre_array[i]*1000;
            {
                tp.tp_c.modulation	= searchlist.app_qam_array[i]+1;
                tp.tp_c.symbol_rate = searchlist.app_symb_array[i]*1000;
            }
            tpid_temp1 = tpid_temp;
            ret = GxBus_PmTpDelete(&tpid_temp1, 1);
            ret = GxBus_PmTpAdd(&tp);

            if (GXCORE_SUCCESS != ret)
            {
                //tp加入不成功
                printf("can't add Tp\n");
                break;
            }

            searchresultpara.app_tpid[i] = tp.id;
        }
        else
        {
            //tp加入不成功
            printf("can't add Tp\n");
            return;
        }
    }
    GxBus_PmSync(GXBUS_PM_SYNC_TP);

    if (0 == i)
        return;
    searchresultpara.tp_num = i;
    dvbc_scan.scan_type = GX_SEARCH_MANUAL;
    dvbc_scan.tv_radio = GX_SEARCH_TV_RADIO_ALL;
    dvbc_scan.fta_cas = GX_SEARCH_FTA_CAS_ALL;
    dvbc_scan.nit_switch = GX_SEARCH_NIT_DISABLE;
    /*
     * 添加扩展表过滤条件,需改成回调函数
     */


    /*
     * 搜索节目前，初始化逻辑频道号列表
     */
    dvbc_scan.params_c.sat_id = sat_arry.id;
    dvbc_scan.params_c.max_num = i;//searchlist.num;
    dvbc_scan.params_c.array = searchresultpara.app_tpid;
    dvbc_scan.params_c.ts = &ts_id;
    dvbc_scan.time_out.pat = PAT_FILTER_TIMEOUT; 
    dvbc_scan.time_out.sdt = SDT_FILTER_TIMEOUT; 
    dvbc_scan.time_out.nit = NIT_FILTER_TIMEOUT; 
    dvbc_scan.time_out.pmt = PMT_FILTER_TIMEOUT; 

    //app_send_msg(GXMSG_SEARCH_SCAN_CABLE_START,(void*)&dvbc_scan);
    {
        GxMsgProperty_ScanCableStart *c_scan;
        GxMessage* new_msg_dvbc_search = NULL;
        new_msg_dvbc_search = GxBus_MessageNew(GXMSG_SEARCH_SCAN_CABLE_START);

        c_scan = GxBus_GetMsgPropertyPtr(new_msg_dvbc_search,GxMsgProperty_ScanCableStart);

        memcpy(c_scan,&dvbc_scan,sizeof(GxMsgProperty_ScanCableStart));

        GxBus_MessageSend(new_msg_dvbc_search);
    }
}

status_t app_search_lock_tp(uint32_t fre, uint32_t symb, fe_spectral_inversion_t inversion, fe_modulation_t modulation,uint32_t delayms)
{
    GxFrontend frontend={0};
    int ret = 0;
    int i =0;

    if(sApp_frontend_device_handle<0)
    {
        sApp_frontend_device_handle = GxAvdev_CreateDevice(0);
        if (sApp_frontend_device_handle < 0) {
            printf("[SEARCH]---create device err!!\n");
        }

        sApp_frontend_demux_handle =  GxAvdev_OpenModule(sApp_frontend_device_handle, GXAV_MOD_DEMUX, 0);
        if (sApp_frontend_demux_handle < 0) {
            printf("[SEARCH]---open demux  err!!\n");
        }
    }
    frontend.dev = sApp_frontend_device_handle;
    frontend.demux = sApp_frontend_demux_handle;
    frontend.tuner = 0;
    frontend.fre = fre*1000;
    frontend.symb = symb*1000;
    frontend.qam = modulation+1;
    frontend.type = FRONTEND_DVB_C;
    printf("\n\n\n\n app_search_lock_tp begin \n\n\n");

    GxFrontend_SetTp(&frontend);

    if (0 == delayms)
    {
        /*
         * 不需要等待锁频结果返回，如输入频点、加锁节目锁频等
         */

        return 0;
    }


#if 1
    while(1)
    {
        ret = GxFrontend_QueryFrontendStatus(0);
        if(ret <=0)
        {
#if defined(ECOS_OS)
            GxCore_ThreadDelay(10);
#elif  defined(LINUX_OS)
            GxCore_ThreadDelay(10);
#endif
            i++;
        }
        else if(ret == 1)
        {
            printf("\n [app_common]app_search_lock_tp lock  \n\n");
            break;
        }

#if defined(ECOS_OS)
        if(i >= delayms/10)
#elif defined(LINUX_OS)
            if(i >= delayms/10)
#endif
            {
                break;
            }
    }
#endif
#if defined(ECOS_OS)
    if(i >= delayms/10)
#elif defined(LINUX_OS)
        if(i >= delayms/10)
#endif

        {

            printf("\n [app_common]app_search_lock_tp unlock	\n\n");
            return 1;
        }
    return 0;
}

void app_search_set_strength_progbar(strength_xml strengthxml)
{
    uint32_t value = 0;
    int8_t   chSignalStrBuf[10]= {0};

    if (0 == strengthxml.lock_status_invaild)
    {
        if(strengthxml.lock_status == 1)
            value = GxFrontend_GetStrength(0);
        else
            value = 0;
    }
    else
    {
        value = GxFrontend_GetStrength(0);
    }

    if (NULL != strengthxml.widget_name_strength_bar)
        GUI_SetProperty(strengthxml.widget_name_strength_bar, "value", (void*)&value);

    if (0 == strengthxml.unit)
    {
        sprintf((char*)chSignalStrBuf, "%ddBuV", value);
    }
    else
    {
        sprintf((char*)chSignalStrBuf, "%d%%", value);
    }

    if (NULL != strengthxml.widget_name_strength_bar_value)
        GUI_SetProperty(strengthxml.widget_name_strength_bar_value, "string", (void*)chSignalStrBuf);

    return;
}

/*
 * 显示信号质量百分比/数值
 */
void app_search_set_signal_progbar(signal_xml singalxml)
{
    uint32_t value = 0;
    uint16_t E_param = 0;
    int16_t  nErrotRate = 0;
    int8_t   chErrorRateBuf[20]= {0};
    int8_t   chSignalQualityBuf[10]= {0};
    GxFrontendSignalQuality Singal;
    uint32_t signalvalue = 0;

    if (0 == singalxml.lock_status_invaild)
    {
        if(singalxml.lock_status ==1)
        {
            value = GxFrontend_GetQuality(0, &Singal);
            E_param = (Singal.error_rate&0xffff);
            nErrotRate = (Singal.error_rate>>16)*100;
        }
        else if(singalxml.lock_status ==0)
        {
            value=0;
            E_param=0;
            nErrotRate=0;
            Singal.snr =0;
        }
    }
    else
    {
        value = GxFrontend_GetQuality(0, &Singal);
        E_param = (Singal.error_rate&0xffff);
        nErrotRate = (Singal.error_rate>>16)*100;
    }

    sprintf((char*)chErrorRateBuf, "%d.%02dE-%02d", nErrotRate/100, nErrotRate%100, E_param);
    if (NULL != singalxml.widget_name_error_rate)
        GUI_SetProperty(singalxml.widget_name_error_rate, "string", (void*)chErrorRateBuf);

    if (NULL != singalxml.widget_name_error_rate_bar)
        GUI_SetProperty(singalxml.widget_name_error_rate_bar, "value", (void*)&Singal.error_rate);
    /* zhouhm : 信号质量转化为db显示*/

    signalvalue = Singal.snr *100/35;

    if (signalvalue >=100)
        signalvalue = 100;
    //	signalvalue = Singal.snr;

    if (NULL != singalxml.widget_name_signal_bar)
        GUI_SetProperty(singalxml.widget_name_signal_bar, "value", (void*)&signalvalue);

    if (0 == singalxml.unit)
    {
        sprintf((char*)chSignalQualityBuf, "%ddB", signalvalue);
    }
    else
    {
        sprintf((char*)chSignalQualityBuf, "%d%%", signalvalue);
    }

    if (NULL != singalxml.widget_name_signal_bar_value)
        GUI_SetProperty(singalxml.widget_name_signal_bar_value, "string", (void*)chSignalQualityBuf);

    return;
}

static  int dvbc_search_setting_timer(void *userdata)
{
    strength_xml strengthxml = {0};
    signal_xml singalxml = {0};
    char* focus_Window = (char*)GUI_GetFocusWindow();

    if((NULL !=focus_Window )&&( strcasecmp("wnd_search_setting_dvbc", focus_Window) != 0))
    {
        return 0;
    }

    if(previous_param.flag==0)
    {
        return 0;
    }

    if (previous_param.fre < FRE_BEGIN_LOW || previous_param.fre > FRE_BEGIN_HIGH)
    {
        return 0;
    }

    if (previous_param.symbol_rate < 2000 || previous_param.symbol_rate > 7000)
    {
        return 0;
    }

    if(1 == GxFrontend_QueryStatus(0))
    {
        strengthxml.lock_status = 1;
        singalxml.lock_status =1;
    }
    else
    {
        strengthxml.lock_status = 0;
        singalxml.lock_status =0;
    }
    strengthxml.widget_name_strength_bar = "progbar_dvbc_search_strength";
    strengthxml.widget_name_strength_bar_value = "text_dvbc_search_strength_value";
	strengthxml.unit = 1;
    app_search_set_strength_progbar(strengthxml);

    singalxml.widget_name_signal_bar = "progbar_dvbc_search_quality";
    singalxml.widget_name_signal_bar_value = "text_dvbc_search_quality_value";
    singalxml.widget_name_error_rate_bar = NULL;
    singalxml.widget_name_error_rate = NULL;
	singalxml.unit = 1;
    app_search_set_signal_progbar(singalxml);

    return 0;
}

SIGNAL_HANDLER int app_search_edit_freq_keypress(GuiWidget *widget, void *usrdata)
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
                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_search_edit_sym_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    //char* value;

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
                case STBK_OK:
                    //GUI_GetProperty("edit_search_sym", "string", &value);
                    //GUI_SendEvent("wnd_search_setting_dvbc", event);
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_search_cmb_search_mode_change(GuiWidget *widget, void *usrdata)
{
    uint32_t box_sel;
	int32_t value = 0;
	search_dvbc_param tmp_param;

    GUI_GetProperty("cmbox_search_mode", "select", &box_sel);

    if(box_sel == 0)
    {
        GUI_SetProperty("boxitem_search_freq", "disable", NULL);
        GUI_SetProperty("boxitem_search_sym", "disable", NULL);
        GUI_SetProperty("boxitem_search_qam", "disable", NULL);
        GUI_SetProperty("boxitem_search_nit", "disable", NULL);
		GxBus_ConfigGetInt(DVBC_FREQ_KEY, &value, DVBC_FREQ_VALUE);
		tmp_param.fre = value;
	    tmp_param.symbol_rate= MANUSAL_SEARCH_SYMBOL_DV;//app_flash_get_config_manual_search_symbol_rate();
	    tmp_param.qam= MANUSAL_SEARCH_QAM_DV;//app_flash_get_config_manual_search_qam();
    }
    else
    {
        GUI_SetProperty("boxitem_search_freq", "enable", NULL);
        GUI_SetProperty("boxitem_search_sym", "enable", NULL);
        GUI_SetProperty("boxitem_search_qam", "enable", NULL);
        GUI_SetProperty("boxitem_search_nit", "enable", NULL);
		tmp_param.fre = previous_param.fre;//
	    tmp_param.symbol_rate= previous_param.symbol_rate;
	    tmp_param.qam= previous_param.qam;
    }
	app_search_lock_tp(tmp_param.fre, tmp_param.symbol_rate,
            INVERSION_OFF, tmp_param.qam,200);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_setting_create(GuiWidget *widget, void *usrdata)
{
    //static GxMsgProperty_NodeByPosGet node;
    search_dvbc_param tmp_param;
    char App_Fre[7];//频点
    char sApp_Sym[5];//符号率
    uint32_t sApp_Mod;//调制方式
    uint32_t value = 0;
	int32_t value1 = 0;
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
#if (DEMOD_DVB_COMBO > 0)
	app_set_tune_mode(GXBUS_PM_SAT_C);
	GUI_SetProperty("text_search_setting_tip_red", "state", "hide");
	GUI_SetProperty("img_search_setting_tip_red", "state", "hide");
#endif
    value = 4;
    GUI_SetProperty("box_search_options", "select", &value);
    app_lang_set_menu_font_type("text_search_title");
    GUI_SetProperty("boxitem_search_freq", "disable", NULL);
    GUI_SetProperty("boxitem_search_sym", "disable", NULL);
    GUI_SetProperty("boxitem_search_qam", "disable", NULL);
    GUI_SetProperty("boxitem_search_nit", "disable", NULL);
    GUI_SetProperty("cmbox_search_qam", "content", QAM_CONTENT);
    GUI_SetProperty("cmbox_search_mode", "content", SEARCH_MODE_CONTENT);

    memset(&previous_param,0,sizeof(search_dvbc_param));
    memset(&tmp_param,0,sizeof(search_dvbc_param));
	
    tmp_param.fre= MANUSAL_SEARCH_FREQ_DV;//app_flash_get_config_center_freq();
    memset(App_Fre,0,7);
    sprintf(App_Fre,"%03d", tmp_param.fre);
    GUI_SetProperty("edit_search_freq", "string", App_Fre);

	GxBus_ConfigGetInt(DVBC_FREQ_KEY, &value1, DVBC_FREQ_VALUE);
	tmp_param.fre = value1;
    tmp_param.symbol_rate= MANUSAL_SEARCH_SYMBOL_DV;//app_flash_get_config_manual_search_symbol_rate();
    tmp_param.qam= MANUSAL_SEARCH_QAM_DV;//app_flash_get_config_manual_search_qam();

    memset(sApp_Sym,0,5);//符号率

    sprintf(sApp_Sym, "%04d", tmp_param.symbol_rate);
    sApp_Mod = tmp_param.qam;


    GUI_SetProperty("edit_search_sym", "string", sApp_Sym);
    GUI_SetProperty("cmbox_search_qam", "select", &sApp_Mod);

    GxFrontend_GetCurFre(0,&sProg_Tp);
    /*
     * reset progress zero
     */
    value = 0;
    GUI_SetProperty("progbar_dvbc_search_strength", "value", (void*)&value);
    GUI_SetProperty("progbar_dvbc_search_quality", "value", (void*)&value);

    app_search_lock_tp(tmp_param.fre, tmp_param.symbol_rate,
            INVERSION_OFF, tmp_param.qam,200);
    spmanualsearchtime = create_timer(dvbc_search_setting_timer, 500, NULL,  TIMER_REPEAT);

    previous_param.fre= MANUSAL_SEARCH_FREQ_DV;//tmp_param.fre;
    previous_param.symbol_rate= tmp_param.symbol_rate;
    previous_param.qam= tmp_param.qam;
    previous_param.flag = 1;

    if(((previous_param.fre*1000 ) != sProg_Tp.fre)
            || ((previous_param.symbol_rate*1000 ) != sProg_Tp.symb)
            ||((previous_param.qam  + 1 ) != sProg_Tp.qam))
    {
        freq_diff_prog = 1;
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_setting_destroy(GuiWidget *widget, void *usrdata)
{
    uint32_t panel_led;

    remove_timer(spmanualsearchtime);
    spmanualsearchtime = NULL;

    if(freq_diff_prog == 1)
    {
        app_search_lock_tp(sProg_Tp.fre/1000, sProg_Tp.symb /1000,
                INVERSION_OFF, sProg_Tp.qam -1,0);
    }

    app_epg_info_clean();

    if (g_AppPlayOps.normal_play.play_total == 0)
    {
        panel_led = 0;
    }
    else
    {
        panel_led = g_AppPlayOps.normal_play.play_count+1;
    }
    ioctl(bsp_panel_handle, PANEL_SHOW_NUM, &panel_led);

    GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_setting_got_focus(GuiWidget *widget, void *usrdata)
{

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_setting_lost_focus(GuiWidget *widget, void *usrdata)
{

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_dvb_c_search_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    int32_t sel = -1;
    GUI_GetProperty("cmbox_search_mode", "select", &sel);

    if(sel == 0) /*in auto mode,don't need config,cann't move by UP and DOWN key*/
    {
	return ret;
    }

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
			GUI_GetProperty("box_search_options", "select", &box_sel);
                        if( box_sel == BOX_ITEM_FREQ_VALUE )
                        {
                        	box_sel = BOX_ITEM_SEARCH_MODE;
                                GUI_SetProperty("box_search_options", "select", &box_sel);
                                ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                                ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;
                case STBK_DOWN:
		    {
                        uint32_t box_sel;
                        GUI_GetProperty("box_search_options", "select", &box_sel);
                        if( box_sel == BOX_ITEM_SEARCH_MODE )
                        {
                                box_sel = BOX_ITEM_FREQ_VALUE;
                                GUI_SetProperty("box_search_options", "select", &box_sel);
                                ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                                ret = EVENT_TRANSFER_KEEPON;
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

SIGNAL_HANDLER int app_search_setting_keypress(GuiWidget *widget, void *usrdata)
{

    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t symbol_rate = 0;
    uint32_t qam = 0;
    uint32_t fre = 0;
    char *value;
    uint32_t value2;
    //uint32_t nit_flag;
    strength_xml strengthxml = {0};
    signal_xml singalxml = {0};

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
#if (DLNA_SUPPORT > 0)
                case VK_DLNA_TRIGGER:
                    //app_dlna_ui_exec(app_search_start_dlna_cb, app_search_stop_dlna_cb);
                    break;
#endif
		case STBK_LEFT:
		case STBK_RIGHT:
			GUI_GetProperty("cmbox_search_qam", "select", &value2);
			qam = value2;
			previous_param.flag=1;
			if (qam != previous_param.qam)
			{
					previous_param.qam = qam;
					app_search_lock_tp(previous_param.fre, previous_param.symbol_rate,
								INVERSION_OFF, previous_param.qam,0);
					
			}
			
			break;	
                case STBK_EXIT:
                case STBK_MENU:
                    {
                        if((GXCORE_SUCCESS == GUI_CheckDialog("wnd_satellite_list"))
                                || (GXCORE_SUCCESS == GUI_CheckDialog("wnd_tp_list")))
                        {
                            //GUI_EndDialog("wnd_search_setting");
                            GUI_CreateDialog("wnd_search_setting_dvbc");
                        }
                        else
                        {
//#if !(DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)                        
#if (DEMOD_DVB_COMBO == 0)                        
                            GUI_EndDialog("after wnd_main_menu");
                            GUI_SetProperty("wnd_main_menu", "draw_now", NULL);
#if MV_WIN_SUPPORT
                            extern void app_main_item_menu_create(int value);
                            app_main_item_menu_create(0);
#endif
#else
			   GUI_EndDialog("wnd_search_setting_dvbc");	
			   GUI_SendEvent("wnd_system_setting", event);
#endif
                        }
                    }
                    break;

                case STBK_OK:
                    GUI_GetProperty("cmbox_search_mode", "select", &value2);
                    if(0 == value2)
                        app_win_auto_search();
                    else
                        _search_ok_keypress();
                    break;
                case STBK_RED:
                    app_win_auto_search();
                    break;

                case STBK_GREEN:

                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}
//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
#if (DEMOD_DVB_COMBO_C > 0)
#include "app_wnd_system_setting_opt.h"

enum DVBC_NAME
{
    ITEM_C_SEARCH = 0,
    ITEM_C_FREQ_SETTING,
    ITEM_C_TOTAL,
};

static SystemSettingOpt thizDvbcSearchOpt = {0};
static SystemSettingItem thizDvbcSearchItem[ITEM_C_TOTAL];
static int app_c_search_button_press_callback(unsigned short key)
{
	if(key == STBK_OK)
	{   
		GUI_CreateDialog("wnd_search_setting_dvbc");
	}
	return EVENT_TRANSFER_KEEPON;
}
static int app_c_freq_setting_button_press_callback(unsigned short key)
{
	if(key == STBK_OK)
	{   
		GUI_CreateDialog("wnd_search_setting_frequency");
	}
	return EVENT_TRANSFER_KEEPON;
}
static void app_c_search_item_init(void)
{
	thizDvbcSearchItem[ITEM_C_SEARCH].itemTitle = STR_ID_SEARCH;
	thizDvbcSearchItem[ITEM_C_SEARCH].itemType = ITEM_PUSH;
	thizDvbcSearchItem[ITEM_C_SEARCH].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	thizDvbcSearchItem[ITEM_C_SEARCH].itemCallback.btnCallback.BtnPress= app_c_search_button_press_callback;
	thizDvbcSearchItem[ITEM_C_SEARCH].itemStatus = ITEM_NORMAL;
}
static void app_c_freqsetting_item_init(void)
{
	thizDvbcSearchItem[ITEM_C_FREQ_SETTING].itemTitle = STR_ID_SEARCH_SETTING;
	thizDvbcSearchItem[ITEM_C_FREQ_SETTING].itemType = ITEM_PUSH;
	thizDvbcSearchItem[ITEM_C_FREQ_SETTING].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	thizDvbcSearchItem[ITEM_C_FREQ_SETTING].itemCallback.btnCallback.BtnPress= app_c_freq_setting_button_press_callback;
	thizDvbcSearchItem[ITEM_C_FREQ_SETTING].itemStatus = ITEM_NORMAL;
}
void DvbcSearchCreateDialog(void)
{	
	memset(&thizDvbcSearchOpt, 0 ,sizeof(thizDvbcSearchOpt));

	thizDvbcSearchOpt.menuTitle = STR_ID_SEARCH;
	thizDvbcSearchOpt.titleImage = "s_title_left_utility.bmp";
	thizDvbcSearchOpt.itemNum = ITEM_C_TOTAL;
	thizDvbcSearchOpt.timeDisplay = TIP_HIDE;
	thizDvbcSearchOpt.item = thizDvbcSearchItem;
		
	app_c_search_item_init();
	app_c_freqsetting_item_init();
	
	thizDvbcSearchOpt.exit = NULL;

	app_system_set_create(&thizDvbcSearchOpt);
}
#endif
#else //DEMOD_DVB_C
SIGNAL_HANDLER int app_search_edit_freq_keypress(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_search_edit_sym_keypress(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_search_cmb_search_mode_change(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_search_setting_create(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_search_setting_destroy(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_search_setting_got_focus(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_search_setting_lost_focus(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_search_setting_keypress(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_dvb_c_search_box_keypress(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
#endif
