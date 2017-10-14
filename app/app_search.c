#include "app.h"
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "app_frontend.h"
#include "app_epg.h"
#include "app_default_params.h"
#include "full_screen.h"
#if TKGS_SUPPORT
#include "tkgs/app_tkgs_background_upgrade.h"
#endif

//satellite search opt content
#define SAT_SEARCH_MODE_CONTENT	"[Preset,Blind]"
#define SAT_SEARCH_BLIND_CONTENT	"[Blind]"
#define YES_NO_CONTENT	"[No,Yes]"
#define SCAN_CHANNEL_CONTENT	"[All,TV,Radio]"
//tp search opt content
//#define TP_SEARCH_MODE_CONTENT	"[Normal]"//"[Normal, PID]"

// MAX time is 65535
#define TIMEOUT_PAT 5000
#define TIMEOUT_PMT 7000
#define TIMEOUT_SDT 7000
#define TIMEOUT_NIT 10000

typedef enum
{
	SAERCH_ERROR = 0,
	SEARCH_OVERFLOW,
	SEARCH_OK,
}SearchStopType_e;

typedef struct
{
	SearchStopType_e m_StopType;
	uint32_t m_nHaveNewProg;
}SearchStopType_t;

#if (DEMOD_DVB_S > 0)
extern bool scan_mode_disable_flag;
#endif

/* Private variable------------------------------------------------------- */
static event_list* sp_SearchTimerSignal = NULL;
static SearchStopType_t sg_StopType = {0};
static uint32_t sg_TvCount=0;
static uint32_t sg_RadioCount=0;

//static WndStatus sg_search_menu_state = WND_EXEC;
static WndStatus sg_search_opt_state = WND_EXEC;
static SearchType sg_search_type = SEARCH_NONE;
static SearchIdSet *sg_search_id_set = NULL;
static GxMsgProperty_ScanSatStart s_Search;
static GxMsgProperty_BlindScanStart s_Blind;
static uint32_t *sp_scan_id = NULL;
static uint32_t *sp_ts_src = NULL;
static enum
{
	SAT_SEARCH_AUTO,
	SAT_SEARCH_BLINDE
}sg_sat_search_mode = SAT_SEARCH_AUTO;

// add in 20121012
static uint32_t sg_SatId = 0xffff;// for search moto control 

//add in 20130410
static GxBlindSearchStage s_blind_stage =  BLIND_STAGE_NONE;
/* widget macro -------------------------------------------------------- */

extern status_t app_volume_scope_status(void);
extern uint32_t app_tuner_cur_tuner_get(void);
#if TKGS_SUPPORT
extern status_t app_tkg_lcn_flush(void);
extern void app_tkgs_sort_channels_by_lcn(void);
#endif
void app_search_clr_msg_process_si_subtable(void);
#if(DEMOD_DVB_C>0)
extern void app_table_nit_get_search_filter_info(int32_t* pNitSubtId,uint32_t* pNitRequestId);
#endif

#define SEARCH_TIMER
static int timer_search_show_signal(void *userdata)
{
	unsigned int value = 0;
    char Buffer[20] = {0};
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};
    static int IconCount = 0;
#if (DEMOD_DVB_S > 0)
    extern uint32_t app_tuner_cur_tuner_get(void);
    uint32_t tuner = app_tuner_cur_tuner_get();
#else
    uint32_t tuner = 0;
#endif

    app_ioctl(tuner, FRONTEND_STRENGTH_GET, &value);
    siganl_bar.strength_bar = "progbar_search_strength";
    siganl_val.strength_val = value;

    // strength value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_search_strength_value", "string", Buffer);

    // progbar quality
    //printf("[APP Search] get quality\n");
    app_ioctl(tuner, FRONTEND_QUALITY_GET, &value);
    siganl_bar.quality_bar= "progbar_search_quality";
    siganl_val.quality_val = value;

    // quality value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_search_quality_value", "string", Buffer);

    app_signal_progbar_update(tuner, &siganl_bar, &siganl_val);

    ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &value);

    IconCount++;
    if(IconCount == 1)
    {
        GUI_SetProperty("img_search_magnifier","img","s_icon_sat1.bmp");
    }
    else if(IconCount == 2)
    {
        GUI_SetProperty("img_search_magnifier","img","s_icon_sat2.bmp");
    }
    else if(IconCount == 3)
    {
        GUI_SetProperty("img_search_magnifier","img","s_icon_sat3.bmp");
    }
    else if(IconCount == 4)
    {
        GUI_SetProperty("img_search_magnifier","img","s_icon_sat4.bmp");
        IconCount = 0;
    }

    return 0;
}


/*void timer_search_show_signal_stop(void)
{
	timer_stop(sp_SearchTimerSignal);
}

void timer_search_show_signal_reset(void)
{
	reset_timer(sp_SearchTimerSignal);
}*/

#define SUB_FUNC
static void _set_auto_search_display(void)
{
	GUI_SetProperty("text_search_tp_title", "state", "hide");
	GUI_SetProperty("img_search_tp_back", "state", "hide");
	GUI_SetProperty("text_search_tp_name1", "state", "hide");
	GUI_SetProperty("text_search_tp_name2", "state", "hide");
	GUI_SetProperty("text_search_tp_name3", "state", "hide");
	GUI_SetProperty("text_search_tp_name4", "state", "hide");
	GUI_SetProperty("text_search_tp_name5", "state", "hide");
	GUI_SetProperty("text_search_tp_name6", "state", "hide");

	GUI_SetProperty("text_search_tv_title", "state", "show");
	GUI_SetProperty("img_search_tv_back", "state", "show");
	GUI_SetProperty("text_search_tv_name1", "state", "show");
	GUI_SetProperty("text_search_tv_name2", "state", "show");
	GUI_SetProperty("text_search_tv_name3", "state", "show");
	GUI_SetProperty("text_search_tv_name4", "state", "show");
	GUI_SetProperty("text_search_tv_name5", "state", "show");
	GUI_SetProperty("text_search_tv_name6", "state", "show");

	GUI_SetProperty("text_search_radio_title", "state", "show");
	GUI_SetProperty("img_search_radio_back", "state", "show");
	GUI_SetProperty("text_search_radio_name1", "state", "show");
	GUI_SetProperty("text_search_radio_name2", "state", "show");
	GUI_SetProperty("text_search_radio_name3", "state", "show");
	GUI_SetProperty("text_search_radio_name4", "state", "show");
	GUI_SetProperty("text_search_radio_name5", "state", "show");
	GUI_SetProperty("text_search_radio_name6", "state", "show");
}

static void _set_blind_search_display(void)
{
    _set_auto_search_display();
}


static void _search_show_tv_channel(uint32_t channel_num, GxMsgProperty_NewProgGet* pNewProgGet)
{
#define CHANNEL_NUM_ONE_PAGE	6
#define TV_NAME_BUFFER_LENGTH	(MAX_PROG_NAME+10)
    static char s_TvName[6][TV_NAME_BUFFER_LENGTH];
    char* s_TextName[6] = {"text_search_tv_name1", "text_search_tv_name2",
        "text_search_tv_name3", "text_search_tv_name4",
        "text_search_tv_name5", "text_search_tv_name6"};

    char* s_ImgName[6] = {"text_search_scrambler1", "text_search_scrambler2",
        "text_search_scrambler3", "text_search_scrambler4",
        "text_search_scrambler5", "text_search_scrambler6"};

    static char s_ImgState[6][20];
    uint8_t CurLine=0;
    uint8_t i=0;

    CurLine = channel_num%CHANNEL_NUM_ONE_PAGE;

    if(pNewProgGet->scramble_flag)
        sprintf(s_ImgState[CurLine],"%s","$");
    else
        sprintf(s_ImgState[CurLine],"%s"," ");

    char Buffer[TV_NAME_BUFFER_LENGTH] = {0};
    memset(Buffer, 0, TV_NAME_BUFFER_LENGTH);
    sprintf((char*)Buffer,"%04d    %s",channel_num+1,pNewProgGet->name);
    printf("------%04d    %s [%d]------\n",channel_num+1,pNewProgGet->name, pNewProgGet->service_id);
    strcpy(s_TvName[CurLine], Buffer);

    //to roll
    if(channel_num<CHANNEL_NUM_ONE_PAGE)
    {
        GUI_SetProperty(s_TextName[CurLine], "string", s_TvName[CurLine]);
        GUI_SetProperty(s_ImgName[CurLine], "string", s_ImgState[CurLine]);
    }
    else
    {
        for(i=0; i<CHANNEL_NUM_ONE_PAGE; i++)
        {
            if((CurLine+i)<5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_TvName[CurLine+i+1]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i+1]);
            }
            else if((CurLine+i)>=5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_TvName[CurLine+i-5]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i-5]);
            }
        }
    }

    return;
}

static void _search_show_tp(uint32_t tp_num, uint32_t fre, uint32_t sym, GxBusPmTpPolar polar)
{
#define TP_NUM_ONE_PAGE	3
	static char s_TpFre[3][10];
	static char s_TpSym[3][10];
	static char s_TpPol[3][10];

	char* s_TextTpNo[3] = {"text_search_tp1", "text_search_tp2", "text_search_tp3"};
	char* s_TextTpFre[3] = {"text_search_tp_fre1", "text_search_tp_fre2", "text_search_tp_fre3"};
	char* s_TextTpSym[3] = {"text_search_tp_sym1", "text_search_tp_sym2", "text_search_tp_sym3"};
	char* s_TextTpPol[3] = {"text_search_tp_pol1", "text_search_tp_pol2", "text_search_tp_pol3"};
	char* s_TextTpOk[3] = {"text_search_tp_ok1", "text_search_tp_ok2", "text_search_tp_ok3"};

    uint8_t CurLine=0;
    uint8_t i=0;
    char* PolarStr[2] = {"H", "V"};
    char Buffer[10] = {0};

    CurLine = tp_num % TP_NUM_ONE_PAGE;

    strcpy(s_TpPol[CurLine], PolarStr[polar]);
    sprintf((char*)s_TpFre[CurLine],"%d", fre);
    sprintf((char*)s_TpSym[CurLine],"%d", sym);

    //to roll
    if(tp_num<TP_NUM_ONE_PAGE)
    {
        sprintf(Buffer, "%04d", (tp_num+1));
        GUI_SetProperty(s_TextTpNo[CurLine], "string", Buffer);
        GUI_SetProperty(s_TextTpFre[CurLine], "string", s_TpFre[CurLine]);
        GUI_SetProperty(s_TextTpSym[CurLine], "string", s_TpSym[CurLine]);
        GUI_SetProperty(s_TextTpPol[CurLine], "string", s_TpPol[CurLine]);
        if(CurLine >= 1)
            GUI_SetProperty(s_TextTpOk[CurLine-1], "state", "show");
    }
    else
    {
        for(i=0; i<TP_NUM_ONE_PAGE; i++)
        {
            sprintf(Buffer, "%04d", (tp_num+1 - CHANNEL_NUM_ONE_PAGE + (i+1)));
            GUI_SetProperty(s_TextTpNo[i], "string", Buffer);
            if((CurLine+i)<2)
            {
                GUI_SetProperty(s_TextTpFre[i], "string", s_TpFre[CurLine+i+1]);
                GUI_SetProperty(s_TextTpSym[i], "string", s_TpSym[CurLine+i+1]);
                GUI_SetProperty(s_TextTpPol[i], "string", s_TpPol[CurLine+i+1]);
            }
            else if((CurLine+i)>=2)
            {
                GUI_SetProperty(s_TextTpFre[i], "string", s_TpFre[CurLine+i-2]);
                GUI_SetProperty(s_TextTpSym[i], "string", s_TpSym[CurLine+i-2]);
                GUI_SetProperty(s_TextTpPol[i], "string", s_TpPol[CurLine+i-2]);
            }
        }
    }

    return;
}

static void _search_show_radio_channel(uint32_t channel_num, GxMsgProperty_NewProgGet* pNewProgGet)
{
#define CHANNEL_NUM_ONE_PAGE	6
#define RADIO_NAME_BUFFER_LENGTH	(MAX_PROG_NAME+10)
	static char s_RadioName[6][RADIO_NAME_BUFFER_LENGTH];
	char* s_TextName[6] = {"text_search_radio_name1", "text_search_radio_name2",
					"text_search_radio_name3", "text_search_radio_name4",
					"text_search_radio_name5", "text_search_radio_name6"};

	char* s_ImgName[6] = {"text_search_scrambler7", "text_search_scrambler8",
					"text_search_scrambler9", "text_search_scrambler10",
					"text_search_scrambler11", "text_search_scrambler12"};
	static char s_ImgState[6][20];

    uint8_t CurLine=0;

	uint8_t i=0;

	CurLine = channel_num%CHANNEL_NUM_ONE_PAGE;


    if(pNewProgGet->scramble_flag)
		sprintf(s_ImgState[CurLine],"%s","$");
	else
        sprintf(s_ImgState[CurLine],"%s"," ");

    char Buffer[RADIO_NAME_BUFFER_LENGTH] = {0};
    memset(Buffer, 0, RADIO_NAME_BUFFER_LENGTH);
    sprintf((char*)Buffer,"%04d    %s",channel_num+1,pNewProgGet->name);
    strcpy(s_RadioName[CurLine], Buffer);

    //to roll
    if(channel_num<CHANNEL_NUM_ONE_PAGE)
    {
        GUI_SetProperty(s_TextName[CurLine], "string", s_RadioName[CurLine]);
        GUI_SetProperty(s_ImgName[CurLine], "string", s_ImgState[CurLine]);
    }
    else
    {
        for(i=0; i<CHANNEL_NUM_ONE_PAGE; i++)
        {
            if((CurLine+i)<5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_RadioName[CurLine+i+1]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i+1]);
            }
            else if((CurLine+i)>=5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_RadioName[CurLine+i-5]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i-5]);
            }
        }
    }

    return;
}

static int _search_service_ext(GxMessage *pMsg)
{
#define BUFFER_LENGTH_	200
	GxMsgProperty_NewProgGet* pNewProgGet = NULL;
	GxMsgProperty_SatTpReply* pSatTpReply = NULL;
	GxMsgProperty_StatusReply* pStatusReply = NULL;
	GxMsgProperty_BlindScanReply *pBlindReply = NULL;
    char Buffer[BUFFER_LENGTH_] = {0};
    char* PolarStr[2] = {"H", "V"};
    float Progress=0, MaxSat=0, CurSat=0, MaxTp=0, CurTp=0;
    uint32_t intProgress=0;
    static uint32_t TPNumber = 0;
    static uint32_t blind_progress = 0;
	static uint32_t blind_progress_bak = 0;

//    printf("[APP Search], service get, id = %d\n", pMsg->msg_id);
	switch(pMsg->msg_id)
	{
		case GXMSG_SEARCH_NEW_PROG_GET:
			{
				pNewProgGet = (GxMsgProperty_NewProgGet*)GxBus_GetMsgPropertyPtr(pMsg,
										GxMsgProperty_NewProgGet);
				//need to save new program
				if(GXBUS_PM_PROG_NOT_EXIST == pNewProgGet->flag)
				{
					sg_StopType.m_nHaveNewProg = TRUE;
				}

				if(GXBUS_PM_PROG_TV == pNewProgGet->type)
				{
					_search_show_tv_channel(sg_TvCount, pNewProgGet);
					sg_TvCount++;
				}
				else if(GXBUS_PM_PROG_RADIO == pNewProgGet->type)
				{
					_search_show_radio_channel(sg_RadioCount, pNewProgGet);
					sg_RadioCount++;
				}
			}
			break;

		case GXMSG_SEARCH_SAT_TP_REPLY:
			{

                GxMsgProperty_NodeByIdGet pmdata_sat = {0}; 
                GxMsgProperty_NodeByIdGet pmdata_tp = {0}; 

                //printf("[APP Search] recieved msg: GXMSG_SEARCH_SAT_TP_REPLY\n");
                pSatTpReply = (GxMsgProperty_SatTpReply*)GxBus_GetMsgPropertyPtr(pMsg,
                        GxMsgProperty_SatTpReply);
                //printf("[APP Search] sat num: %d, name: %s, len:%d\n",pSatTpReply->sat_num,pSatTpReply->sat_name,strlen((const char *)pSatTpReply->sat_name));
                {
                    pmdata_tp.node_type = NODE_TP;
                    pmdata_tp.id = pSatTpReply->tp_id;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &pmdata_tp);

                    pmdata_sat.node_type = NODE_SAT; 
                    pmdata_sat.id = pmdata_tp.tp_data.sat_id;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &pmdata_sat);
                    app_tuner_cur_tuner_set(pmdata_sat.sat_data.tuner);
				}
                //show sat info
                memset(Buffer, 0, BUFFER_LENGTH_);
                sprintf((char*)Buffer,
                        "(%d / %d)  %s",
                        pSatTpReply->sat_num,
                        pSatTpReply->sat_max_count,
                        (char*)pmdata_sat.sat_data.sat_s.sat_name);
                GUI_SetProperty("text_search_sat", "string", Buffer);

                //show tp info
                memset(Buffer, 0, BUFFER_LENGTH_);
                printf("search show tp freq = %d \n",pSatTpReply->frequency);
#if DEMOD_DVB_T
                if(GXBUS_PM_SAT_DVBT2 == pmdata_sat.sat_data.type)
                {
                    if(pSatTpReply->nit_flag == NIT)
                    {
                        sprintf((char*)Buffer,
                                "(%d / %d) [NIT] %d.%d",
                                pSatTpReply->tp_num,
                                pSatTpReply->tp_max_count,
                                pSatTpReply->frequency/1000,
                                (pSatTpReply->frequency/100)%10);
                    }
                    else
                    {
                        sprintf((char*)Buffer,
                                "(%d / %d)  %d.%d",
                                pSatTpReply->tp_num,
                                pSatTpReply->tp_max_count,
                                pSatTpReply->frequency/1000,
                                (pSatTpReply->frequency/100)%10);
                    }
                }
#endif

#if DEMOD_DVB_C
                if(GXBUS_PM_SAT_C == pmdata_sat.sat_data.type)
                {
                    char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};
                    if(pSatTpReply->nit_flag == NIT)
                    {
                        if(pSatTpReply->tp_c.modulation <QAM_AUTO )
                            sprintf((char*)Buffer,
                                    "(%d / %d) [NIT] %d /%s  /%d ",
                                    pSatTpReply->tp_num,
                                    pSatTpReply->tp_max_count,
                                    pSatTpReply->frequency/1000,
                                    modulation_str[pSatTpReply->tp_c.modulation],pSatTpReply->tp_c.symbol_rate/1000);
                    }
                    else
                    {
                        if(pSatTpReply->tp_c.modulation <QAM_AUTO )
                            sprintf((char*)Buffer,
                                    "(%d / %d)  %d / %s  /%d",
                                    pSatTpReply->tp_num,
                                    pSatTpReply->tp_max_count,
                                    pSatTpReply->frequency/1000,
                                    modulation_str[pSatTpReply->tp_c.modulation],pSatTpReply->tp_c.symbol_rate/1000);
                    }
                }
#endif

#if DEMOD_DTMB
			  if(GXBUS_PM_SAT_DTMB == pmdata_sat.sat_data.type)
                {
                    if(GXBUS_PM_SAT_1501_DTMB == pmdata_sat.sat_data.sat_dtmb.work_mode)
                    {
						if(pSatTpReply->nit_flag == NIT)
	                    {
	                        sprintf((char*)Buffer,
	                                "(%d / %d) [NIT] %d.%d",
	                                pSatTpReply->tp_num,
	                                pSatTpReply->tp_max_count,
	                                pSatTpReply->frequency/1000,
	                                (pSatTpReply->frequency/100)%10);
	                    }
	                    else
	                    {
	                        sprintf((char*)Buffer,
	                                "(%d / %d)  %d.%d",
	                                pSatTpReply->tp_num,
	                                pSatTpReply->tp_max_count,
	                                pSatTpReply->frequency/1000,
	                                (pSatTpReply->frequency/100)%10);
	                    }
					}
					else
					{
	                    char* modulation_str[QAM_AUTO] = {"QPSK","16QAM", "32QAM","64QAM","128QAM","256QAM"};
	                    if(pSatTpReply->nit_flag == NIT)
	                    {
	                        if(pSatTpReply->tp_dtmb.modulation < QAM_AUTO )
	                            sprintf((char*)Buffer,
	                                    "(%d / %d) [NIT] %d /%s  /%d ",
	                                    pSatTpReply->tp_num,
	                                    pSatTpReply->tp_max_count,
	                                    pSatTpReply->frequency/1000,
	                                    modulation_str[pSatTpReply->tp_dtmb.modulation],pSatTpReply->tp_dtmb.symbol_rate/1000);
	                    }
	                    else
	                    {
	                        if(pSatTpReply->tp_dtmb.modulation <QAM_AUTO )
	                            sprintf((char*)Buffer,
	                                    "(%d / %d)  %d / %s  /%d",
	                                    pSatTpReply->tp_num,
	                                    pSatTpReply->tp_max_count,
	                                    pSatTpReply->frequency/1000,
	                                    modulation_str[pSatTpReply->tp_dtmb.modulation],pSatTpReply->tp_dtmb.symbol_rate/1000);
	                    }
					}
                }
#endif

#if DEMOD_DVB_S
                if(GXBUS_PM_SAT_S == pmdata_sat.sat_data.type)
                {
                    if(pSatTpReply->nit_flag == NIT)
                    {
                        sprintf((char*)Buffer,
                                "(%d / %d) [NIT] %d /%s / %d",
                                pSatTpReply->tp_num,
                                pSatTpReply->tp_max_count,
                                pSatTpReply->frequency,
                                (strlen((const char*)pSatTpReply->sat_name) == 0) ?  "NULL" : (char *)PolarStr[pSatTpReply->tp_s.polar],
                                pSatTpReply->tp_s.symbol_rate);
                    }
                    else
                    {
                        sprintf((char*)Buffer,
                                "(%d / %d)  %d / %s / %d",
                                pSatTpReply->tp_num,
                                pSatTpReply->tp_max_count,
                                pSatTpReply->frequency,
                                (strlen((const char*)pSatTpReply->sat_name) == 0) ? "NULL" : (char *)PolarStr[pSatTpReply->tp_s.polar],
                                pSatTpReply->tp_s.symbol_rate);
                    }
                }
#endif
                GUI_SetProperty("text_search_tp", "string", Buffer);

                //show progress
                MaxSat = pSatTpReply->sat_max_count;
                CurSat = pSatTpReply->sat_num;
                MaxTp = pSatTpReply->tp_max_count;
                CurTp = pSatTpReply->tp_num;
                Progress = 100*((CurSat-1)/MaxSat)+100*((1/MaxSat)*(CurTp/MaxTp)) -1;
				if(sg_search_type == SEARCH_TP)
				{
					Progress = 100*((1/MaxSat)*(CurTp/MaxTp));
				}
                intProgress = Progress;
                GUI_SetProperty("progbar_search_progress", "value", &intProgress);
                GUI_SetInterface("flush",NULL);

                memset(Buffer,0,sizeof(Buffer));
                sprintf((char*)Buffer, "%d%%", intProgress);
                GUI_SetProperty("text_search_progress_percent","string",Buffer);
            }
            break;

        case GXMSG_SEARCH_STATUS_REPLY:
            {
                pStatusReply = (GxMsgProperty_StatusReply*)GxBus_GetMsgPropertyPtr(pMsg,
                        GxMsgProperty_StatusReply);
                if(SEARCH_DBASE_OVERFLOW == pStatusReply->type)
                {
                    sg_StopType.m_StopType = SEARCH_OVERFLOW;
                }
                else if(SEARCH_ERROR == pStatusReply->type)
                {
                    sg_StopType.m_StopType = SAERCH_ERROR;
                }
            }
            break;

        case GXMSG_SEARCH_STOP_OK:
            {
                s_blind_stage = BLIND_STAGE_NONE;
                intProgress = 100;
                GUI_SetProperty("progbar_search_progress", "value", &intProgress);
                GUI_SetProperty("text_search_progress_percent","string","100%");
                GUI_CreateDialog("wnd_search_tip");
            }
            break;

        case GXMSG_SEARCH_BLIND_SCAN_REPLY:
            blind_progress_bak = blind_progress;
            pBlindReply = (GxMsgProperty_BlindScanReply*)GxBus_GetMsgPropertyPtr(pMsg,
                    GxMsgProperty_BlindScanReply);
            blind_progress = pBlindReply->progress;


            if(pBlindReply->type == BLIND_TP)
            {
                //show sat info
                memset(Buffer, 0, BUFFER_LENGTH_);
                sprintf((char*)Buffer,
                        "(%d / %d)  %s",
                        pBlindReply->sat_num,
                        pBlindReply->sat_max_count,
                        pBlindReply->sat_name);
                GUI_SetProperty("text_search_sat", "string", Buffer);

                if((pBlindReply->frequency != 0) && (pBlindReply->tp_s.symbol_rate != 0))
                {
                    //show tp info
                    memset(Buffer, 0, BUFFER_LENGTH_);
                    sprintf((char*)Buffer,
                            "(%d / %d)  %d / %s / %d",
                            TPNumber + 1, TPNumber + 1,
                            pBlindReply->frequency,
                            PolarStr[pBlindReply->tp_s.polar],
                            pBlindReply->tp_s.symbol_rate);

                    GUI_SetProperty("text_search_tp", "string", Buffer);

                    _search_show_tp(TPNumber, pBlindReply->frequency, pBlindReply->tp_s.symbol_rate, pBlindReply->tp_s.polar);

                    TPNumber++;
                }
            }
            else if((pBlindReply->type == BLIND_PROGRESS)&&(blind_progress >= blind_progress_bak))
            {
                GUI_SetProperty("progbar_search_progress", "value", &pBlindReply->progress);

                memset(Buffer,0,sizeof(Buffer));
                sprintf((char*)Buffer, "%d%%", pBlindReply->progress);
                GUI_SetProperty("text_search_progress_percent","string",Buffer);
                if(pBlindReply->stage == BLIND_STAGE_TP)
                {
                    static int start = 0;
                    if(s_blind_stage == BLIND_STAGE_NONE)
                    {
                        start = 0;
                    }
                    else if(s_blind_stage == BLIND_STAGE_PROG)
                    {
                        start = pBlindReply->progress;
                        GUI_SetProperty("text_search_tp", "string", NULL);
                    }
                    GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
                    memset(Buffer,0,sizeof(Buffer));
                    sprintf(Buffer, "%d%%", (pBlindReply->progress - start) * 10);
                    GUI_SetProperty("text_scan_tp_value", "string", Buffer);
                }
                s_blind_stage = pBlindReply->stage;
            }
            break;

        case GXMSG_SEARCH_BLIND_SCAN_FINISH:
            {
                s_blind_stage = BLIND_STAGE_NONE;
                intProgress = 100;
                GUI_SetProperty("progbar_search_progress", "value", &intProgress);
                GUI_SetProperty("text_search_progress_percent","string","100%");
                GUI_CreateDialog("wnd_search_tip");
                TPNumber = 0;
                break;
            }
        default:
            break;
    }

    return GXMSG_OK;
}

static uint32_t  _check_ca_system(uint16_t ca_system_id,uint16_t ele_pid,uint16_t ecm_pid)
{
    //printf("\n------[CAS filter control, ca_system_id = 0x%x]------\n",ca_system_id);
    if(((ca_system_id >= 0x900) && (ca_system_id <= 0x9ff))
        || ((ca_system_id >= 0xe00) && (ca_system_id <= 0xeff)))
        return 1;
    else
        return 0;
}

void _set_disqec_callback(uint32_t sat_id,fe_sec_voltage_t polar,int32_t sat22k)
{
#define DISEQC_SET_DELAY_MS 100
    GxMsgProperty_NodeByIdGet sat_node = {0};
    AppFrontend_DiseqcParameters diseqc10;
    AppFrontend_DiseqcParameters diseqc11;
    AppFrontend_DiseqcChange change;

    sat_node.node_type = NODE_SAT;
    sat_node.id = sat_id;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node))
    {
        // add in 20121012
        // for polar
        if(sat_node.sat_data.sat_s.lnb_power == GXBUS_PM_SAT_LNB_POWER_OFF)
            return ;
        AppFrontend_PolarState Polar =polar ;
        app_ioctl(sat_node.sat_data.tuner, FRONTEND_POLAR_SET, (void*)&Polar);

        GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);
        // for 22k
        AppFrontend_22KState _22K = FRONTEND_22K_NULL;
        if(sat22k == SEC_TONE_OFF)
            _22K = FRONTEND_22K_OFF;
        else if(sat22k == SEC_TONE_ON)
            _22K = FRONTEND_22K_ON;
        else
            _22K = FRONTEND_22K_NULL;

        app_ioctl(sat_node.sat_data.tuner, FRONTEND_22K_SET, (void*)&_22K);

        GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);
        // for diseqc
        diseqc10.type = DiSEQC10;
        diseqc10.u.params_10.bPolar = polar;
        diseqc10.u.params_10.b22k = sat22k;
        diseqc10.u.params_10.chDiseqc = sat_node.sat_data.sat_s.diseqc10;

        change.diseqc = &diseqc10;
        app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_CHANGE, &change);

        app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_SET_EX, &diseqc10);

        GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);

        diseqc11.type = DiSEQC11;
        diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;
        diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
        diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc;
        diseqc11.u.params_11.chUncom = sat_node.sat_data.sat_s.diseqc11;

        change.diseqc = &diseqc11;
        app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_CHANGE, &change);

        app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_SET_EX, &diseqc11);

        // TODO: change with SAT change
        if(sat_id != sg_SatId)
        {
            sg_SatId = sat_id;
            if((sat_node.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
            {
                app_diseqc12_goto_position(sat_node.sat_data.tuner,sat_node.sat_data.sat_s.diseqc12_pos, false);
            }
            else if((sat_node.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
            {
                LongitudeInfo local_longitude;
                local_longitude.longitude = sat_node.sat_data.sat_s.longitude;
                local_longitude.longitude_direct = sat_node.sat_data.sat_s.longitude_direct;
                app_usals_goto_position(sat_node.sat_data.tuner,&local_longitude, NULL,false);
            }
        }
        GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);
    }
}

static status_t _modify_prog_callback(GxBusPmDataProg* prog)
{
    if(GXBUS_PM_PROG_H264 == prog->video_type)
    {
       prog->definition = GXBUS_PM_PROG_HD;
#if (1 != TKGS_SUPPORT)
       prog->favorite_flag |= 0x01;
#endif
    }

#if 0//TKGS_SUPPORT	//liqg m, 2014.9.21
{
	AppProgExtInfo prog_ext_info = {0};

	prog_ext_info.locator_id = TKGS_INVALID_LOCATOR;//0x8000;//
	prog_ext_info.logicnum = 0;
	if (GXCORE_SUCCESS != GxBus_PmProgExtInfoAdd(prog->id,sizeof(AppProgExtInfo),&prog_ext_info))
	{
		printf("===%s, %d, err, %d, %s\n", __FUNCTION__, __LINE__, prog->id, prog->prog_name);
	}
	else
	{
		printf("===%s, %d, modify prog ent info ok\n", __FUNCTION__, __LINE__);
	}
		
	//prog->logicnum = 0;
	//prog->locator_id = 0x8000;//TKGS_INVALID_LOCATOR
}
#endif
#if 1   ///save default vale 20130803
    prog->audio_mode = GXBUS_PM_PROG_AUDIO_MODE_STEREO;
#endif

    if(app_volume_scope_status())
    {
        prog->audio_volume = AUDIO_VOLUME;
    }

    return GXCORE_SUCCESS;
}

void app_search_set_search_type(SearchType type)
{
    sg_search_type = type;
    return;
}

typedef struct _app_search_si_subtable__ok_for_nit
{
    uint32_t    msg_si_subtable_ok_for_nit;
    void        (*app_si_subtable_stopfilter)(void);
    int        (*app_si_subtable_afterfilter_start)(void);
}app_search_si_subtable__ok_for_nit;
static app_search_si_subtable__ok_for_nit thizFlagProcessMsgSiSubtableOkForNitSection;

uint32_t app_search_get_msg_process_si_subtable(void)
{
    return thizFlagProcessMsgSiSubtableOkForNitSection.msg_si_subtable_ok_for_nit;
}

void app_search_set_msg_process_si_subtable(void (*p1)(void),int (*p2)(void))
{
    thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_stopfilter =p1;
    thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_afterfilter_start = p2;
    thizFlagProcessMsgSiSubtableOkForNitSection.msg_si_subtable_ok_for_nit = 1;
}

void app_search_clr_msg_process_si_subtable(void)
{
    thizFlagProcessMsgSiSubtableOkForNitSection.msg_si_subtable_ok_for_nit = 0;
    thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_stopfilter = NULL;
    thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_afterfilter_start = NULL;
}

void app_search_stopfilter_si_subtable(void)
{
    if(NULL!=thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_stopfilter)
    {
        thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_stopfilter();
    }
}

void app_search_afterfilterok_si_subtable(void)
{
    if(NULL!=thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_afterfilter_start)
    {
        thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_afterfilter_start();
    }
}

#define TIME_SETTING
SIGNAL_HANDLER int app_search_create(GuiWidget *widget, void *usrdata)
{
    char Buffer[50] = {0};
    uint32_t intProgress=0;


    g_AppFullArb.timer_stop();
    //printf("\n[APP Search], create search dialog\n");

    app_epg_disable();
    g_AppPmt.stop(&g_AppPmt);

#if EMM_SUPPORT
    app_emm_release_all();
    app_cat_release();
#endif

#if OTA_MONITOR_SUPPORT
    app_ota_nit_release();
#endif

#ifdef LINUX_OS
#if (BOX_SERVICE > 0)
    GxMsgProperty_BoxRecordControl box_rec = {0};
    box_rec.option = BOX_RECORD_ALL_STOP;
    app_send_msg_exec(GXMSG_BOX_RECORD_CONTROL, (void *)(&box_rec));
    app_send_msg_exec(GXMSG_BOX_DISABLE, (void *)NULL);
#endif
#endif
#if TKGS_SUPPORT
    g_AppTkgsBackUpgrade.stop();
#endif

    sg_TvCount=0;
    sg_RadioCount=0;

    sg_StopType.m_StopType = SEARCH_OK;
    sg_StopType.m_nHaveNewProg = FALSE;

    if(reset_timer(sp_SearchTimerSignal) != 0)
    {
        sp_SearchTimerSignal = create_timer(timer_search_show_signal, 300, NULL, TIMER_REPEAT);
    }

    if(sg_search_type == SEARCH_SAT)
    {
        if(sg_sat_search_mode == SAT_SEARCH_AUTO)
        {
            _set_auto_search_display();
            GUI_SetProperty("text_search_title", "string", STR_PRESENT_SEARCH);
        }
        else
        {
            s_blind_stage = BLIND_STAGE_NONE;
            _set_blind_search_display();
            GUI_SetProperty("text_search_title", "string", STR_ID_BLIND_SEARCH);
            GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
            GUI_SetProperty("text_scan_tp_value", "string", "0%");
        }
    }
    else if(sg_search_type == SEARCH_BLIND_ONLY)
    {
        s_blind_stage = BLIND_STAGE_NONE;
        _set_blind_search_display();
        GUI_SetProperty("text_search_title", "string", STR_ID_BLIND_SEARCH);
        GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
        GUI_SetProperty("text_scan_tp_value", "string", "0%");
    }
    else if(sg_search_type == SEARCH_TP)
    {
        _set_auto_search_display();
        GUI_SetProperty("text_search_title", "string", STR_ID_TP_SEARCH);
    }
    else if(sg_search_type == SEARCH_CABLE)
    {
        _set_auto_search_display();
        GUI_SetProperty("text_search_title", "string", "Cable Search");
        GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
        GUI_SetProperty("text_scan_tp_value", "string", "0%");
    }
    else if(sg_search_type == SEARCH_DVBT2)
    {
        _set_auto_search_display();
        GUI_SetProperty("text_search_title", "string", "DVB T2 Search");
        GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
        GUI_SetProperty("text_scan_tp_value", "string", "0%");
    }
	else if(sg_search_type == SEARCH_DTMB)
    {
        _set_auto_search_display();
        GUI_SetProperty("text_search_title", "string", "DTMB Search");
        GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
        GUI_SetProperty("text_scan_tp_value", "string", "0%");
    }

    memset(Buffer,0,sizeof(Buffer));
    sprintf((char*)Buffer, "%d%%", intProgress);
    GUI_SetProperty("text_search_progress_percent","string",Buffer);
    GUI_SetProperty("progbar_search_progress", "value", &intProgress);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_destroy(GuiWidget *widget, void *usrdata)
{
    uint32_t i;
    int value = 0;
    remove_timer(sp_SearchTimerSignal);
    sp_SearchTimerSignal = NULL;
    AppFrontend_Monitor monitor = FRONTEND_MONITOR_ON;

    s_blind_stage = BLIND_STAGE_NONE;

#ifdef LINUX_OS
#if (BOX_SERVICE > 0)
    app_send_msg_exec(GXMSG_BOX_ENABLE, (void *)NULL);
#endif
#endif

    for (i=0; i<NIM_MODULE_NUM; i++)
    {
        app_ioctl(i, FRONTEND_MONITOR_SET, &monitor);
    }
    ioctl(bsp_panel_handle, PANEL_SHOW_QUALITY, &value);

#if CASCAM_SUPPORT
{
    CASCAMDemuxFlagClass flag;
    flag.Flags = DEMUX_CAT|DEMUX_NIT;
    CASCAM_Start_Demux(&flag, 0);
}
#endif

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
#if (DEMOD_DVB_C>0)
	int32_t pNitSubtId=0;
	uint32_t pNitRequestId=0;
#endif

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
					#if (DEMOD_DVB_C > 0)
						app_table_nit_get_search_filter_info(&pNitSubtId,&pNitRequestId);
						if (-1== pNitSubtId)
						{
							/*
							* ±ØÐëµÈ´ýNITÊÕµ½»ò³¬Ê±ÍË³ö£¬·ñÔò»á³öÏÖËÀ»ú
							*/
							 app_send_msg_exec(GXMSG_SEARCH_SCAN_STOP, (void *)NULL);
						}
					#else
						if(sg_sat_search_mode == SAT_SEARCH_BLINDE)
                        {
                            app_send_msg_exec(GXMSG_SEARCH_BLIND_SCAN_STOP, (void *)NULL);
                        }
                        else
                        {
                            app_send_msg_exec(GXMSG_SEARCH_SCAN_STOP, (void *)NULL);
                        }
					#endif
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

SIGNAL_HANDLER int app_search_service(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            _search_service_ext((GxMessage*)(event->msg.service_msg));
            break;
        default:
            break;
    }
    return ret;
}

#define SEARCH_TIP
SIGNAL_HANDLER int app_search_tip_create(GuiWidget *widget, void *usrdata)
{
    if(SEARCH_OK == sg_StopType.m_StopType)
    {
        if(sg_StopType.m_nHaveNewProg)//need to save
        {
            GUI_SetProperty("text_search_tip", "string", STR_ID_SAVE_INFO);
            GUI_SetProperty("img_search_tip_opt","img","s_bar_choice_blue4.bmp");
            //cancel and ok
        }
        else//no new channel,need not to save
		{
			GUI_SetProperty("text_search_tip", "string", STR_ID_NO_NEW_CH);
			//only ok
            GUI_SetProperty("img_search_tip_opt","img","s_bar_choice_blue4.bmp");
			GUI_SetProperty("btn_search_tip_cancel", "state", "hide");
		}
	}
	else if(SEARCH_OVERFLOW== sg_StopType.m_StopType)//overflow
	{
        uint32_t tp_num = GxBus_PmTpNumGet();
        if(tp_num >= SYS_MAX_TP)
        {
		    GUI_SetProperty("text_search_tip", "string", STR_ID_MAX_TP);
        }
        else
        {
		    GUI_SetProperty("text_search_tip", "string", STR_ID_DB_OVERFLOW);
        }
		//only ok
		GUI_SetProperty("btn_search_tip_cancel", "state", "hide");
        GUI_SetProperty("img_search_tip_opt","img","s_bar_choice_blue4.bmp");
	}
	else //error
	{
	    GUI_SetProperty("text_search_tip", "string", STR_ID_ERR_OCCUR);
		//only ok
	    GUI_SetProperty("btn_search_tip_cancel", "state", "hide");
        GUI_SetProperty("img_search_tip_opt","img","s_bar_choice_blue4.bmp");
	}

	GUI_SetFocusWidget("btn_search_tip_ok");

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_tip_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_tip_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint8_t FocusdOnOk=0;

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
				case STBK_MENU:
                case STBK_EXIT:
                    {
                        if(SEARCH_OK == sg_StopType.m_StopType)
                        {
                            app_send_msg_exec(GXMSG_SEARCH_NOT_SAVE, (void *)NULL);
                            GUI_EndDialog("wnd_search_tip");
                            GUI_EndDialog("wnd_search");
                        }
                        else//error and overflow
                        {
                            if(sg_StopType.m_nHaveNewProg)
                            {
                                GUI_SetProperty("text_search_tip", "string", STR_ID_SAVE_INFO);
                                GUI_SetProperty("btn_search_tip_cancel", "state", "show");
                                sg_StopType.m_StopType = SEARCH_OK;
                            }
                            else
                            {
                                GUI_EndDialog("wnd_search_tip");
                                GUI_EndDialog("wnd_search");
                            }
                        }
                    }
                    break;

                case STBK_LEFT:
                case STBK_RIGHT:
                    if((SEARCH_OK == sg_StopType.m_StopType)
						&& (sg_StopType.m_nHaveNewProg) )//choose save or not?
					{
						if(0 == strcasecmp("btn_search_tip_ok", GUI_GetFocusWidget()))
						{
							FocusdOnOk = 1;
						}
						else if(0 == strcasecmp("btn_search_tip_cancel", GUI_GetFocusWidget()))
						{
							FocusdOnOk = 0;
						}

						if(1 == FocusdOnOk)
						{
							GUI_SetProperty("img_search_tip_opt","img","s_bar_choice_blue4_l.bmp");
							GUI_SetFocusWidget("btn_search_tip_cancel");
						}
						else
						{
							GUI_SetProperty("img_search_tip_opt","img","s_bar_choice_blue4.bmp");
							GUI_SetFocusWidget("btn_search_tip_ok");
						}

					}
					break;

				case STBK_OK:
                    {
                        if(SEARCH_OK == sg_StopType.m_StopType)
                        {
                            if(sg_StopType.m_nHaveNewProg)//need to save
                            {
                                if(0 == strcasecmp("btn_search_tip_ok", GUI_GetFocusWidget()))
                                {
                                    //save
                                    GUI_SetProperty("text_search_tip", "string", STR_ID_SAVING_WAIT);
                                    GUI_SetProperty("text_search_tip", "draw_now", NULL);

                                    GUI_SetProperty("btn_search_tip_cancel", "state", "hide");
                                    GUI_SetProperty("btn_search_tip_cancel", "draw_now", NULL);

                                    GUI_SetProperty("btn_search_tip_ok", "state", "hide");
                                    GUI_SetProperty("btn_search_tip_ok", "draw_now", NULL);

                                    GUI_SetProperty("img_search_tip_opt", "state", "hide");
                                    GUI_SetProperty("img_search_tip_opt", "draw_now", NULL);

                                    GxBus_PmLock();
                                    app_send_msg_exec(GXMSG_SEARCH_SAVE, (void *)NULL);
                                    GxBus_PmUnlock();
									#if 0//def AUDIO_DESCRIPTOR_SUPPORT
									GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);
									#endif
#if TKGS_SUPPORT	//liqg m, 2014.9.21 
									app_tkg_lcn_flush();
									app_tkgs_sort_channels_by_lcn();//2014.09.25 add
#endif
                                    GxCore_ThreadDelay(800);
                                }
                                else//cancel, no need to save
                                {
                                    app_send_msg_exec(GXMSG_SEARCH_NOT_SAVE, (void *)NULL);
                                }
                            }
                            else
                            {
                                app_send_msg_exec(GXMSG_SEARCH_NOT_SAVE, (void *)NULL);
                            }
                            GUI_EndDialog("wnd_search_tip");
                            GUI_EndDialog("wnd_search");

#if FACTORY_TEST_SUPPORT

                            if(GUI_CheckDialog("wnd_factory_test") == GXCORE_SUCCESS)
                            {
                                GxMsgProperty_NodeNumGet prog_num = {0};
                                prog_num.node_type = NODE_PROG;
                                app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &prog_num);
                                if(prog_num.node_num)
                                {
                                    event->key.scancode = GUIK_ESCAPE;//STBK_EXIT
                                    GUI_SendEvent("wnd_factory_test", event);
                                }
                            }

#endif

                        }
                        else//error and overflow
                        {
                            if(sg_StopType.m_nHaveNewProg)
                            {
                                GUI_SetProperty("text_search_tip", "string", STR_ID_SAVE_INFO);
                                GUI_SetProperty("btn_search_tip_cancel", "state", "show");
                                sg_StopType.m_StopType = SEARCH_OK;
                            }
                            else
                            {
                                GUI_EndDialog("wnd_search_tip");
                                GUI_EndDialog("wnd_search");
                            }

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


#define  SAT_SEARCH_OPT
int ex_sat_search_mode = 0;
static status_t app_satellite_search_para_get(void)
{	
	uint32_t box_sel;
    GxMsgProperty_NodeByIdGet sat_node = {0};
    AppFrontend_Config cfg = {0};
    uint32_t i = 0;

	s_Search.scan_type = GX_SEARCH_AUTO;

	GUI_GetProperty("cmb_sat_scan_channel", "select", &box_sel);
	if(0 == box_sel)//all
		s_Search.tv_radio = GX_SEARCH_TV_RADIO_ALL;
	else if(1 == box_sel)//tv
		s_Search.tv_radio = GX_SEARCH_TV;
	else if(2 == box_sel)//radio
		s_Search.tv_radio = GX_SEARCH_RADIO;

	GUI_GetProperty("cmb_sat_fta_only", "select", &box_sel);
	if(0 == box_sel)//no: all
		s_Search.fta_cas = GX_SEARCH_FTA_CAS_ALL;
	else if(1 == box_sel)//yes: fta only
		s_Search.fta_cas = GX_SEARCH_FTA;

	GUI_GetProperty("cmb_sat_network", "select", &box_sel);
	if(0 == box_sel)//no
		s_Search.nit_switch = GX_SEARCH_NIT_DISABLE;
	else if(1 == box_sel)//yes
		s_Search.nit_switch = GX_SEARCH_NIT_ENABLE;
		
	if (sp_scan_id != NULL)
	{
		GxCore_Free(sp_scan_id);
		sp_scan_id = NULL;
	}
	if (sp_ts_src != NULL)
	{
		GxCore_Free(sp_ts_src);
		sp_ts_src = NULL;
	}
	sp_scan_id = (uint32_t*)GxCore_Calloc(1, sg_search_id_set->id_num * sizeof(uint32_t));
    sp_ts_src = (uint32_t*)GxCore_Calloc(1, sg_search_id_set->id_num * sizeof(uint32_t));
	if ((sp_scan_id == NULL) || (sp_ts_src == NULL))
			return -1;

	memcpy(sp_scan_id, sg_search_id_set->id_array,  sg_search_id_set->id_num * sizeof(uint32_t));
    for (i=0; i<sg_search_id_set->id_num; i++)
    {
        sat_node.node_type = NODE_SAT;
        sat_node.id = sp_scan_id[i];
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node);

        app_ioctl(sat_node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
        sp_ts_src[i] = cfg.ts_src;
    }
	s_Search.params_s.array = sp_scan_id;
    s_Search.params_s.ts = sp_ts_src;
    s_Search.demux_id = 0;
    s_Search.params_s.max_num = sg_search_id_set->id_num;

	s_Search.ext = NULL;
	s_Search.ext_num = 0;
	s_Search.time_out.pat = TIMEOUT_PAT;
	s_Search.time_out.pmt = TIMEOUT_PMT;
	s_Search.time_out.sdt = TIMEOUT_SDT;
	s_Search.time_out.nit = TIMEOUT_NIT;
	s_Search.search_diseqc = _set_disqec_callback;
    s_Search.modify_prog = _modify_prog_callback;

    s_Search.check_ca_fun = _check_ca_system; 


	if(sg_search_type == SEARCH_BLIND_ONLY)
	{
		sg_sat_search_mode = SAT_SEARCH_BLINDE;

		s_Blind.max_num = sg_search_id_set->id_num;
		s_Blind.array = sp_scan_id;
		s_Blind.polar_type = BLIND_POLAR_ALL;
		s_Blind.search_type = BLIND_FAST;
		// add 20120618
		s_Blind.fta_cas		= s_Search.fta_cas;
		s_Blind.tv_radio	= s_Search.tv_radio;
		s_Blind.nit_switch  = s_Search.nit_switch;
		
        s_Blind.search_diseqc = _set_disqec_callback;
        s_Blind.modify_prog =  _modify_prog_callback;

        s_Blind.check_ca_fun = _check_ca_system;

        s_Blind.ts = sp_ts_src;
        memset(&(s_Blind.params_pid), 0 , sizeof(GxPidSearch));
        s_Blind.ext = NULL;
        s_Blind.ext_num = 0;
        s_Blind.time_out.pat = TIMEOUT_PAT;
        s_Blind.time_out.pmt = TIMEOUT_PMT;
        s_Blind.time_out.sdt = TIMEOUT_SDT;
        s_Blind.time_out.nit = TIMEOUT_NIT;
    }
    else if(sg_search_type == SEARCH_SAT)
    {
#if (DEMOD_DVB_S > 0)
        if(scan_mode_disable_flag == true)
        {
            GUI_SetProperty("boxitem_sat_search_mode", "state", "disable");
            sg_sat_search_mode = SAT_SEARCH_AUTO;
        }
        else
#endif
        {
            GUI_SetProperty("boxitem_sat_search_mode", "enable", NULL);
            GUI_SetProperty("boxitem_sat_search_mode", "state", "enable");
			GUI_GetProperty("cmb_sat_scan_mode", "select", &box_sel);
			if(0 == box_sel)
			{
				sg_sat_search_mode = SAT_SEARCH_AUTO;
				ex_sat_search_mode = SAT_SEARCH_AUTO;
			}
			else if(1 == box_sel)//satellite blind search
			{
				sg_sat_search_mode = SAT_SEARCH_BLINDE;
				ex_sat_search_mode = SAT_SEARCH_BLINDE;

				s_Blind.max_num = sg_search_id_set->id_num;
				s_Blind.array = sp_scan_id;
				s_Blind.polar_type = BLIND_POLAR_ALL;
				s_Blind.search_type = BLIND_FAST;
				// add 20120618
				s_Blind.fta_cas		= s_Search.fta_cas;
				s_Blind.tv_radio	= s_Search.tv_radio;
				s_Blind.nit_switch  = s_Search.nit_switch;
				
	            s_Blind.search_diseqc = _set_disqec_callback;
                s_Blind.modify_prog = _modify_prog_callback;

				s_Blind.check_ca_fun = _check_ca_system;

                s_Blind.ts = sp_ts_src;
                //s_Blind.nit_switch = GX_SEARCH_NIT_DISABLE;
                memset(&(s_Blind.params_pid), 0 , sizeof(GxPidSearch));
                s_Blind.ext = NULL;
                s_Blind.ext_num = 0;
                s_Blind.time_out.pat = TIMEOUT_PAT;
                s_Blind.time_out.pmt = TIMEOUT_PMT;
                s_Blind.time_out.sdt = TIMEOUT_SDT;
                s_Blind.time_out.nit = TIMEOUT_NIT;
            }
        }
	}

    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int app_search_cmbox_change(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
    GUI_GetProperty("cmb_sat_scan_mode", "select", &box_sel);    
    if(0 == box_sel) //Auto search
    {
		GUI_SetProperty("boxitem_sat_search_network", "enable", NULL);
        GUI_SetProperty("boxitem_sat_search_network", "state", "enable");
        GUI_SetProperty("cmb_sat_network","content",YES_NO_CONTENT);
    }
    else if(1 == box_sel)//Blind search
    {
	uint32_t netsel = 0;
	GUI_SetProperty("cmb_sat_network","select",&netsel);
        GUI_SetProperty("boxitem_sat_search_network", "state", "disable");
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satellite_search_create(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
	box_sel = 4;
	if(sg_search_type == SEARCH_BLIND_ONLY)
    {
        GUI_SetProperty("cmb_sat_scan_mode","content",SAT_SEARCH_BLIND_CONTENT);
        GUI_SetProperty("boxitem_sat_search_network", "state", "disable");
#if (DEMOD_DVB_S > 0)
        if(scan_mode_disable_flag == true)
        {
            GUI_SetProperty("boxitem_sat_search_mode","state","disable");
        }
        else
#endif
        {
            GUI_SetProperty("boxitem_sat_search_mode", "enable", NULL);
            GUI_SetProperty("boxitem_sat_search_mode", "state", "enable");
        }
		GUI_SetProperty("cmb_sat_scan_mode","content",SAT_SEARCH_BLIND_CONTENT);
	}
	else if(sg_search_type == SEARCH_SAT)
	{
		int32_t search_mode;
		GxBus_ConfigGetInt(SEARCH_MODE_KEY, &search_mode, SEARCH_MODE_VALUE);
		GUI_SetProperty("cmb_sat_scan_mode","content",SAT_SEARCH_MODE_CONTENT);
		GUI_SetProperty("cmb_sat_scan_mode", "select", &search_mode);

		GUI_SetProperty("boxitem_sat_search_network", "enable", NULL);
        GUI_SetProperty("boxitem_sat_search_network", "state", "enable");
#if (DEMOD_DVB_S > 0)
        if(scan_mode_disable_flag == true)
        {
            GUI_SetProperty("boxitem_sat_search_mode","state","disable");
            sg_sat_search_mode = SAT_SEARCH_AUTO;
        }
        else
#endif
        {
            GUI_SetProperty("boxitem_sat_search_mode", "enable", NULL);
            GUI_SetProperty("boxitem_sat_search_mode", "state", "enable");
            sg_sat_search_mode = ((search_mode==0)?SAT_SEARCH_AUTO:SAT_SEARCH_BLINDE);
		}
        GUI_SetProperty("cmb_sat_scan_mode","content",SAT_SEARCH_MODE_CONTENT);
		GUI_SetProperty("cmb_sat_scan_mode", "select", &search_mode);


	}
	GUI_SetProperty("cmb_sat_fta_only","content",YES_NO_CONTENT);
	GUI_SetProperty("cmb_sat_scan_channel","content",SCAN_CHANNEL_CONTENT);
	GUI_SetProperty("cmb_sat_network","content",YES_NO_CONTENT);
	GUI_SetProperty("box_sat_search_opt","select",&box_sel);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satellite_search_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel;
    uint32_t box_first_num = 0;

#if (DEMOD_DVB_S > 0)
    box_first_num = (scan_mode_disable_flag == true)?1:0;
#endif
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
						sg_search_opt_state = WND_CANCLE;
					}
					break;
					
				case STBK_DOWN:
					{
						GUI_GetProperty("box_sat_search_opt", "select", &box_sel);
						if(4 == box_sel)
						{
							box_sel = box_first_num;
							GUI_SetProperty("box_sat_search_opt", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;
					
				case STBK_UP:
					{
						GUI_GetProperty("box_sat_search_opt", "select", &box_sel);
						
						if(box_first_num == box_sel)
						{
							box_sel = 4;
							GUI_SetProperty("box_sat_search_opt", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;
					
				case STBK_OK:
					{
						if(app_satellite_search_para_get() == GXCORE_SUCCESS)
						{
							sg_search_opt_state = WND_OK;
						}
						else
						{
							sg_search_opt_state = WND_CANCLE;
						}
						ret = EVENT_TRANSFER_STOP;
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

static WndStatus app_sat_search_opt_dlg(void)
{
	if((sg_search_id_set == NULL) || (sg_search_id_set->id_array == NULL))
	{
		sg_search_opt_state = WND_CANCLE;
	}
	else
	{
		GUI_CreateDialog("wnd_sat_search_opt");
		sg_search_opt_state = WND_EXEC;
	}

	app_block_msg_destroy(g_app_msg_self);
	while(sg_search_opt_state == WND_EXEC)
	{
	   //GUI_Loop();
	   GUI_LoopEvent();
	   GxCore_ThreadDelay(50);
	}
	GUI_StartSchedule();

	GUI_EndDialog("wnd_sat_search_opt");
	GUI_SetInterface("flush",NULL);
	app_block_msg_init(g_app_msg_self);

	return sg_search_opt_state;
}

#define  TP_SEARCH_OPT
static status_t app_tp_search_para_get(void)
{
	status_t ret = GXCORE_SUCCESS;
	uint32_t box_sel;

	s_Search.scan_type = GX_SEARCH_MANUAL;

	GUI_GetProperty("cmb_tp_scan_channel", "select", &box_sel);
	if(0 == box_sel)//all
		s_Search.tv_radio = GX_SEARCH_TV_RADIO_ALL;
	else if(1 == box_sel)//tv
		s_Search.tv_radio = GX_SEARCH_TV;
	else if(2 == box_sel)//radio
		s_Search.tv_radio = GX_SEARCH_RADIO;

	GUI_GetProperty("cmb_tp_fta_only", "select", &box_sel);
	if(0 == box_sel)//no: all
		s_Search.fta_cas = GX_SEARCH_FTA_CAS_ALL;
	else if(1 == box_sel)//yes: fta only
		s_Search.fta_cas = GX_SEARCH_FTA;

	GUI_GetProperty("cmb_tp_network", "select", &box_sel);
	if(0 == box_sel)//no
		s_Search.nit_switch = GX_SEARCH_NIT_DISABLE;
	else if(1 == box_sel)//yes
		s_Search.nit_switch = GX_SEARCH_NIT_ENABLE;

	if (sp_scan_id != NULL)
	{
		GxCore_Free(sp_scan_id);
		sp_scan_id = NULL;
	}
	if (sp_ts_src != NULL)
	{
		GxCore_Free(sp_ts_src);
		sp_ts_src = NULL;
	}
		sp_scan_id = (uint32_t*)GxCore_Calloc(1, sg_search_id_set->id_num * sizeof(uint32_t));
		sp_ts_src = (uint32_t*)GxCore_Calloc(1, sg_search_id_set->id_num * sizeof(uint32_t));
	if ((sp_scan_id == NULL) || (sp_ts_src == NULL))
		return -1;

	memcpy(sp_scan_id, sg_search_id_set->id_array,  sg_search_id_set->id_num * sizeof(uint32_t));
	if(sg_search_id_set->id_num)
	{
		GxMsgProperty_NodeByIdGet node = {0};
		AppFrontend_Config cfg = {0};
		uint32_t i = 0;

		node.node_type = NODE_TP;
		node.id = sp_scan_id[0];
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

		node.node_type = NODE_SAT;
		node.id = node.tp_data.sat_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

		app_ioctl(node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
		for(i = 0; i < sg_search_id_set->id_num; i++)
	    		sp_ts_src[i] = cfg.ts_src;
	}

	s_Search.params_s.array = sp_scan_id;
	s_Search.params_s.ts = sp_ts_src;
	s_Search.params_s.max_num = sg_search_id_set->id_num;

	// needn't ext
	s_Search.ext = NULL;
	s_Search.ext_num = 0;
	s_Search.time_out.pat = TIMEOUT_PAT;
	s_Search.time_out.pmt = TIMEOUT_PMT;
	s_Search.time_out.sdt = TIMEOUT_SDT;
	s_Search.time_out.nit = TIMEOUT_NIT;
	s_Search.search_diseqc = _set_disqec_callback;
    s_Search.modify_prog = _modify_prog_callback;

	s_Search.check_ca_fun = _check_ca_system;


	return ret;
}

SIGNAL_HANDLER int app_tp_search_create(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
	box_sel = 3;

	GUI_SetProperty("cmb_tp_fta_only","content",YES_NO_CONTENT);
	GUI_SetProperty("cmb_tp_scan_channel","content",SCAN_CHANNEL_CONTENT);
	GUI_SetProperty("cmb_tp_network","content",YES_NO_CONTENT);
	GUI_SetProperty("box_tp_search_opt","select",&box_sel);
	sg_sat_search_mode = SAT_SEARCH_AUTO;
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tp_search_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
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
				case STBK_EXIT:
				case STBK_MENU:
					{
						sg_search_opt_state = WND_CANCLE;
					}
					break;
					
				case STBK_DOWN:
					{
						uint32_t box_sel;
	
						GUI_GetProperty("box_tp_search_opt", "select", &box_sel);
						if(3 == box_sel)
						{
							box_sel = 0;
							GUI_SetProperty("box_tp_search_opt", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;
					
				case STBK_UP:
					{
						uint32_t box_sel;
	
						GUI_GetProperty("box_tp_search_opt", "select", &box_sel);
						if(0 == box_sel)
						{
							box_sel = 3;
							GUI_SetProperty("box_tp_search_opt", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;
					
				case STBK_OK:
					{
						if(app_tp_search_para_get() ==  GXCORE_SUCCESS)
						{
							sg_search_opt_state = WND_OK;
						}
						else
						{
							sg_search_opt_state = WND_CANCLE;
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

static WndStatus app_tp_search_opt_dlg(void)
{
	if((sg_search_id_set == NULL) || (sg_search_id_set->id_array == NULL))
	{
		sg_search_opt_state = WND_CANCLE;
	}
	else
	{
		GUI_CreateDialog("wnd_tp_search_opt");
		sg_search_opt_state = WND_EXEC;
	}
	app_block_msg_destroy(g_app_msg_self);

	while(sg_search_opt_state == WND_EXEC)
	{
	  GUI_LoopEvent();
	  GxCore_ThreadDelay(50);
	}
	GUI_StartSchedule();

	GUI_EndDialog("wnd_tp_search_opt");
	GUI_SetInterface("flush",NULL);
	app_block_msg_init(g_app_msg_self);

	return sg_search_opt_state;
}

#define EXTERN
WndStatus app_search_opt_dlg(SearchType type,  SearchIdSet *id)
{
	WndStatus ret = WND_CANCLE;

	sg_search_id_set = id;
	switch(type)
	{
		case SEARCH_SAT:
			sg_search_type = SEARCH_SAT;
			ret = app_sat_search_opt_dlg();
			break;

		case SEARCH_BLIND_ONLY:
			sg_search_type = SEARCH_BLIND_ONLY;
			ret = app_sat_search_opt_dlg();
			break;	

		case SEARCH_TP:
			sg_search_type = SEARCH_TP;
			ret = app_tp_search_opt_dlg();
			break;

		default:
			sg_search_type = SEARCH_NONE;
			break;
	}

	return ret;
}

#if FACTORY_TEST_SUPPORT
/*
function:
data :2012 11 1
*/
void app_factory_test_search_start(GxMsgProperty_ScanSatStart * PSearch_para)
{
    sg_search_type = SEARCH_TP;

    AppFrontend_Monitor monitor = FRONTEND_MONITOR_OFF;
    uint32_t i = 0;

    for (i=0; i<NIM_MODULE_NUM; i++)
    {
        app_ioctl(i, FRONTEND_MONITOR_SET, &monitor);
    }

    // add in 20121012
    sg_SatId = 0xffff;// for search moto control

    GUI_CreateDialog("wnd_search");
    app_send_msg_exec(GXMSG_SEARCH_SCAN_SAT_START, PSearch_para);
}
#endif

void app_search_start(uint32_t tuner)
{
    AppFrontend_Monitor monitor = FRONTEND_MONITOR_OFF;
    uint32_t i = 0;

    for (i=0; i<NIM_MODULE_NUM; i++)
    {
        app_ioctl(i, FRONTEND_MONITOR_SET, &monitor);
        GxFrontend_CleanRecordPara(i);// clear the mornitor parameter
    }

#if CASCAM_SUPPORT
{
    CASCAMDemuxFlagClass flag;
    flag.Flags = DEMUX_ALL;
    CASCAM_Release_Demux(&flag);
}
#endif

    // add in 20121012
    sg_SatId = 0xffff;// for search moto control

    switch(sg_search_type)
    {
        case SEARCH_SAT:
            if(sg_sat_search_mode == SAT_SEARCH_AUTO)
            {
                if(s_Search.nit_switch == GX_SEARCH_NIT_ENABLE)
                {
                    GxBus_ConfigSetInt(SEARCH_NIT_LOOP_MODE, APP_SEARCH_NIT_LOOP_NO);//20140322
                }
                GUI_CreateDialog("wnd_search");
                app_send_msg_exec(GXMSG_SEARCH_SCAN_SAT_START, &s_Search);
            }
            else
            {
                GUI_CreateDialog("wnd_search");
                app_send_msg_exec(GXMSG_SEARCH_BLIND_SCAN_START, &s_Blind);
            }
            break;

        case SEARCH_BLIND_ONLY:
            {
                GUI_CreateDialog("wnd_search");
                app_send_msg_exec(GXMSG_SEARCH_BLIND_SCAN_START, &s_Blind);
                break;
            }

        case SEARCH_TP:
            if(s_Search.nit_switch == GX_SEARCH_NIT_ENABLE)
            {
                GxBus_ConfigSetInt(SEARCH_NIT_LOOP_MODE, APP_SEARCH_NIT_LOOP_YES);//20140322
            }
            GUI_CreateDialog("wnd_search");
            app_send_msg_exec(GXMSG_SEARCH_SCAN_SAT_START, &s_Search);
            break;

		default:
			break;
	}
}



