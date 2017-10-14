#include "app.h"
#include "app_data_type.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_epg.h"
#include "app_module.h"


#define EPG_SERVICE_CNT 100
#define EPG_EVENT_CNT 3000
#if (MINI_256_COLORS_OSD_SUPPORT||MINI_16_BITS_OSD_SUPPORT)
#define EPG_PER_EVENT_SIZE 512 //--- add new 20131228
#else
#define EPG_PER_EVENT_SIZE 1024
#endif
#define EPG_DAY_CNT (MAX_EPG_DAY + 1)
#define SEC_PER_DAY (60*60*24)

struct epg_switch
{
    uint32_t ts_src;
    uint32_t dmx_id;
    uint32_t flag;
#define INVALID_TS  (~(uint32_t)(0))
#define INVALID_DMX  (~(uint32_t)(0))
#define EPG_ON      1
#define EPG_OFF     0
}s_Switch = {INVALID_TS, INVALID_DMX, EPG_OFF};

static char s_event_info[EVENT_GET_NUM*EVENT_GET_MAX_SIZE] = {0};
extern char *app_get_short_lang_str(uint32_t lang);

#ifdef MULTI_TP_EPG_INFO
static status_t app_epg_event_modify(GxBusEpgEventAddInfo* event)
{
	if(event != NULL)
	{
		event->ts_id = app_epg_get_cur_prog_tp_id();
	}
	return 0;
}
#endif

static status_t app_epg_event_cmp(GxBusEpgEventComp* src,GxBusEpgEventComp* cur)
{
#ifdef MULTI_TP_EPG_INFO
	if(src->service_id == cur->service_id
            && src->ts_id == cur->ts_id
    )
    {
        return GX_BUS_EPG_OK;
    }
#else
    if(src->service_id == cur->service_id
			&& src->ts_id == cur->ts_id
            //&& src->original_id == cur->original_id
    )
    {
        return GX_BUS_EPG_OK;
    }
#endif
    return GX_BUS_EPG_ERR;
}

#define EPG_6937_SUPPORT 0

#if EPG_6937_SUPPORT
unsigned char* epg_lang_recode_event_info(unsigned char* event_info,int len)
{
	int relen;
	int i = 1;
	unsigned char* iconv_str = NULL;

	if((event_info!=NULL)&&(event_info[0] > 0x20))
	{
		char* encoding_name = NULL;
		int init_value;
		char *lang = NULL;
		encoding_name = (char*)GxCore_Malloc(12);
		memset(encoding_name,0,12);
		GxBus_ConfigGetInt(EPG_LANG_KEY, &init_value, EPG_LANG);
		lang = app_get_short_lang_str(init_value);
		if(strcmp(lang,"ara") ==0)
		{
			i = 6;
			sprintf(encoding_name,"iso8859-0%d",i); 
		}
		else if(strcmp(lang,"eng") == 0)
		{
			i = 1;
			sprintf(encoding_name,"iso8859-0%d",i);//具体前缀有具体地区决定，1为西欧，6为阿拉伯
		}
		else if(strcmp(lang,"tur") == 0)
		{
			i = 9;
			sprintf(encoding_name,"iso8859-0%d",i);//具体前缀有具体地区决定，1为西欧，6为阿拉伯
		}
		gdi_encoding_convert("iso6937",encoding_name);
		if(gxgdi_iconv(event_info,&iconv_str,len,(unsigned int *)&relen,NULL) == 1)
		{
			iconv_str = NULL;
			return NULL;
		}
		gdi_encoding_convert(encoding_name,"iso6937");
		GxCore_Free(encoding_name);
		return iconv_str;
	}
	else
	{
		if(gxgdi_iconv(event_info,&iconv_str,len,(unsigned int *)&relen,NULL) == 1)
		{
			iconv_str = NULL;
			return NULL;
		}
		return iconv_str;
	}
}
#endif

void app_epg_enable(uint32_t ts_src, uint32_t dmx_id)
{
	GxMsgProperty_EpgCreate epgCreate;
	int init_value;
	char *lang = NULL;

    if(s_Switch.flag == EPG_ON)
        return;
    s_Switch.flag = EPG_ON;

	if(ts_src == s_Switch.ts_src
            && dmx_id == s_Switch.dmx_id)
		return;

	epgCreate.ts_src = ts_src;
	epgCreate.dmx_id = dmx_id;

    GxBus_RegisterCompFunc(app_epg_event_cmp); //epg event parse
    #ifdef MULTI_TP_EPG_INFO
    GxBus_RegisterModifyEventCb(app_epg_event_modify);
	#endif
#if 0
// add in 20121212
#if CA_SUPPORT
	//app_check_param(&epgCreate.ts_src,&epgCreate.dmx_id,0);
	{
		CheckAndModifyPara_t Params = {0};
		Params.pTs = &epgCreate.ts_src;
		Params.pDemux = &epgCreate.dmx_id;
		Params.ExtFlag = 0;
		app_ioctl(0,APP_CA_MODIFY_DEMUX_PARA,(void*)(&Params));
	}
	//epgCreate.dmx_id = dmx_id;
#endif
#endif



	epgCreate.service_count = EPG_SERVICE_CNT;
	epgCreate.event_count = EPG_EVENT_CNT;
	epgCreate.event_size = EPG_PER_EVENT_SIZE;
	epgCreate.epg_day = MAX_EPG_DAY;
	epgCreate.cur_tp_only = 0;
#if EPG_6937_SUPPORT
	epgCreate.txt_format = EPG_TXT_PRIVATE;
	epgCreate.ttx_private = (gx_epg_ttx_private)epg_lang_recode_event_info;
#else
	epgCreate.txt_format = EPG_TXT_UTF8;
#endif
	GxBus_ConfigGetInt(EPG_LANG_KEY, &init_value, EPG_LANG);
	lang = app_get_short_lang_str(init_value);

	memcpy(epgCreate.language, lang, sizeof(epgCreate.language));

	app_send_msg_exec(GXMSG_EPG_CREATE, (void *)(&epgCreate));
	s_Switch.ts_src = ts_src;
	s_Switch.dmx_id = dmx_id;
}

void app_epg_disable(void)
{

    if(s_Switch.flag == EPG_OFF)
        return;
    s_Switch.flag = EPG_OFF;
	
	app_send_msg_exec(GXMSG_EPG_CLEAN,(void *)NULL);
	app_send_msg_exec(GXMSG_EPG_RELEASE,(void *)NULL);
	
	s_Switch.ts_src = INVALID_TS;
	s_Switch.dmx_id = INVALID_DMX;

    GxBus_UnregisterCompFunc();
	#ifdef MULTI_TP_EPG_INFO
	GxBus_UnregisterModifyEventCb();
	#endif
}

void app_epg_info_clean(void)
{
    app_send_msg_exec(GXMSG_EPG_CLEAN,(void *)NULL);
}

void app_epg_info_full_speed(void)
{
    app_send_msg_exec(GXMSG_EPG_FULL_SPEED,(void *)NULL);
}

static int _get_day_event_pos(AppEpgOps *ops, int day, int sel)
{
	int cnt = 0;
	int pos = 0;
	int i;

	if((ops == NULL) || (day >= MAX_EPG_DAY))
		return -1;

	cnt = ops->event_cnt.day_cnt[day];
	if((cnt == 0) || (cnt <= sel))
		return -1;

	//pos += ops->event_cnt.cur_cnt;
	pos = 2;
	for(i = 0; i < day; i++)
	{
		pos += ops->event_cnt.day_cnt[i];
	}
	pos += sel;

	return pos;
}
	
static status_t epg_count_update(AppEpgOps *ops)
{
	GxMsgProperty_EpgNumGet epgNumGet;
	int32_t init_value = TIME_ZONE_VALUE;

	if(ops == NULL)
		return GXCORE_ERROR;

	memset(&epgNumGet, 0, sizeof(GxMsgProperty_EpgNumGet));
	epgNumGet.ts_id = ops->prog_info.ts_id;
	epgNumGet.orig_network_id =ops->prog_info.orig_network_id;
	epgNumGet.service_id = ops->prog_info.service_id;
#ifdef MULTI_TP_EPG_INFO
    epgNumGet.ts_id = app_epg_get_cur_prog_tp_id();//ops->prog_info.tp_id;
#endif
	GxBus_ConfigGetInt(TIME_ZONE, &init_value, TIME_ZONE_VALUE);
	epgNumGet.time_zone = init_value;
	GxBus_ConfigGetInt(TIME_SUMMER, &init_value, TIME_SUMMER_VALUE);
	epgNumGet.time_zone += init_value;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_EPG_NUM_GET, (void*)(&epgNumGet)))
	{
		int i = 0;
		
		memset(&ops->event_cnt, 0, sizeof(EventCnt));
		ops->event_cnt.cur_cnt = epgNumGet.event_per_day[0];
		for(i = 0; i<MAX_EPG_DAY; i++)
		{
			ops->event_cnt.day_cnt[i] = epgNumGet.event_per_day[i+1];	
		}
		return GXCORE_SUCCESS;
	}

	return GXCORE_ERROR;
}

static status_t epg_prog_info_update(AppEpgOps *ops, EpgProgInfo *prog_info)
{
	if((ops == NULL) || (prog_info == NULL))
		return GXCORE_ERROR;

	memcpy(&(ops->prog_info), prog_info, sizeof(EpgProgInfo));
	memset(&ops->event_cnt, 0, sizeof(ops->event_cnt));

	return GXCORE_SUCCESS;
}

static int epg_get_cur_event_count(AppEpgOps *ops)
{
	if(ops == NULL) 
		return 0;
	return ops->event_cnt.cur_cnt;
}

static status_t epg_get_cur_event_info(AppEpgOps *ops, GxEpgInfo *event_info)
{
	GxMsgProperty_EpgGet event;

	if((NULL == ops) || (NULL == event_info))
	{
		return GXCORE_ERROR;
	}

	memset(&event, 0, sizeof(GxMsgProperty_EpgGet));
	memset(event_info, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);

	event.ts_id = ops->prog_info.ts_id;
	event.orig_network_id = ops->prog_info.orig_network_id;
	event.service_id = ops->prog_info.service_id;
#ifdef MULTI_TP_EPG_INFO
    event.ts_id = app_epg_get_cur_prog_tp_id();//ops->prog_info.tp_id;
#endif
	event.get_event_pos = 0;
	event.want_event_count = EVENT_GET_NUM;
	event.epg_info_size = EVENT_GET_NUM*EVENT_GET_MAX_SIZE;
	event.epg_info = event_info;

	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_EPG_GET, (void *)(&event))
		&& EVENT_GET_NUM == event.get_event_count)
	{
		return GXCORE_SUCCESS;
	}
	
	return GXCORE_ERROR;
}

static status_t epg_get_next_event_info(AppEpgOps *ops, GxEpgInfo *event_info)
{
	GxMsgProperty_EpgGet event;

	if((NULL == ops) || (NULL == event_info))
	{
		return GXCORE_ERROR;
	}

	memset(&event, 0, sizeof(GxMsgProperty_EpgGet));
	memset(event_info, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);

	event.ts_id = ops->prog_info.ts_id;
	event.orig_network_id = ops->prog_info.orig_network_id;
	event.service_id = ops->prog_info.service_id;
#ifdef MULTI_TP_EPG_INFO
    event.ts_id = app_epg_get_cur_prog_tp_id();//ops->prog_info.tp_id;
#endif
	event.get_event_pos = 1;
	event.want_event_count = EVENT_GET_NUM;
	event.epg_info_size = EVENT_GET_NUM*EVENT_GET_MAX_SIZE;
	event.epg_info = event_info;

	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_EPG_GET, (void *)(&event))
		&& EVENT_GET_NUM == event.get_event_count)
	{
		return GXCORE_SUCCESS;
	}
	
	return GXCORE_ERROR;
}

static int epg_get_day_event_count(AppEpgOps *ops, int day)
{
    int cnt = 0;
    int i,event_total_num = 0;
    time_t time_tmp = 0;
    GxEpgInfo *p_info = (GxEpgInfo *)s_event_info;

    if((ops == NULL) || (day >= MAX_EPG_DAY))
        return 0;

    for(i = 0; i < MAX_EPG_DAY; i++)
    {
        event_total_num += ops->event_cnt.day_cnt[i];
    }

    if((day == 0) && (ops->event_cnt.day_cnt[0] == 0))
    {
        if(ops->event_cnt.cur_cnt != 0)
        {
            memset(p_info, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
            if(epg_get_cur_event_info(ops, p_info) == GXCORE_SUCCESS)
            {
                // printf("__[%s] __[%s] __[%d]\n",__FILE__,__FUNCTION__,__LINE__);
                time_tmp = g_AppTime.utc_get(&g_AppTime);
                time_tmp = get_display_time_by_timezone(time_tmp);
                time_tmp = time_tmp - time_tmp % SEC_PER_DAY;
                time_tmp += SEC_PER_DAY;
                if(get_display_time_by_timezone(p_info->start_time) < time_tmp)
                {
                    //  printf("__[%s] __[%s] __[%d]\n",__FILE__,__FUNCTION__,__LINE__);
                    cnt++;
                }
            }

            memset(p_info, 0, EVENT_GET_NUM * EVENT_GET_MAX_SIZE);
            if(epg_get_next_event_info(ops, p_info) == GXCORE_SUCCESS)
            {
                time_tmp = g_AppTime.utc_get(&g_AppTime);
                time_tmp = get_display_time_by_timezone(time_tmp);
                time_tmp = time_tmp - time_tmp % SEC_PER_DAY;
                time_tmp += SEC_PER_DAY;

                if(get_display_time_by_timezone(p_info->start_time) < time_tmp && (p_info->duration != 0))
                {
                    cnt++;
                }
            }
        }
    }
    else if((day == 1) && (event_total_num == 0))
    {
        if(ops->event_cnt.cur_cnt != 0)
        {
            memset(p_info, 0, EVENT_GET_NUM * EVENT_GET_MAX_SIZE);
            if(epg_get_next_event_info(ops, p_info) == GXCORE_SUCCESS)
            {
                time_tmp = g_AppTime.utc_get(&g_AppTime);
                time_tmp = get_display_time_by_timezone(time_tmp);
                time_tmp = time_tmp - time_tmp % SEC_PER_DAY;
                time_tmp += SEC_PER_DAY;

                if(get_display_time_by_timezone(p_info->start_time) >= time_tmp)
                {
                    cnt++;
                }
            }
        }
    }
    else
    {
        cnt = ops->event_cnt.day_cnt[day];
    }
    return cnt;
}

static status_t epg_get_day_event_info(AppEpgOps *ops, EpgIndex *index, GxEpgInfo *event_info)
{
    int pos = -1;
    int i,event_total_num = 0;
    GxMsgProperty_EpgGet event;

    if((NULL == ops) || (NULL == index) || (NULL == event_info))
    {
        return GXCORE_ERROR;
    }

    for(i = 0; i < MAX_EPG_DAY; i++)
    {
        event_total_num += ops->event_cnt.day_cnt[i];
    }

    if((pos = _get_day_event_pos(ops, index->day, index->sel)) < 0)
    {
        int cnt = 0;
        time_t time_tmp = 0;
        GxEpgInfo *p_info = (GxEpgInfo*)s_event_info;

        if((index->day == 0) && (ops->event_cnt.day_cnt[0] == 0))
        {
            if(ops->event_cnt.cur_cnt != 0)
            {
                memset(p_info, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
                if(epg_get_cur_event_info(ops, p_info) == GXCORE_SUCCESS)
                {
                    cnt++;
                }

                memset(p_info, 0, EVENT_GET_NUM * EVENT_GET_MAX_SIZE);
                if(epg_get_next_event_info(ops, p_info) == GXCORE_SUCCESS)
                {
                    time_tmp = g_AppTime.utc_get(&g_AppTime);
                    time_tmp = get_display_time_by_timezone(time_tmp);
                    time_tmp = time_tmp - time_tmp % SEC_PER_DAY;
                    time_tmp += SEC_PER_DAY;

                    if(get_display_time_by_timezone(p_info->start_time) < time_tmp && (p_info->duration != 0))
                    {
                        cnt++;
                    }
                }

                if(index->sel < cnt)
                {
                    pos = index->sel;
                }
                else
                {
                    return GXCORE_ERROR;
                }
            }
            else
            {
                return GXCORE_ERROR;
            }
        }
        else if((index->day== 1) && (event_total_num == 0))
        {
            if(ops->event_cnt.cur_cnt != 0)
            {
                memset(p_info, 0, EVENT_GET_NUM * EVENT_GET_MAX_SIZE);
                if(epg_get_next_event_info(ops, p_info) == GXCORE_SUCCESS)
                {
                    time_tmp = g_AppTime.utc_get(&g_AppTime);
                    time_tmp = get_display_time_by_timezone(time_tmp);
                    time_tmp = time_tmp - time_tmp % SEC_PER_DAY;
                    time_tmp += SEC_PER_DAY;

                    if(get_display_time_by_timezone(p_info->start_time) >= time_tmp)
                    {
                        pos = 1;
                    }
                    else
                    {
                        return GXCORE_ERROR;
                    }      
                }
                else
                {
                    return GXCORE_ERROR;
                }
            }
            else
            {
                return GXCORE_ERROR;
            }
        }
        else
        {
            return GXCORE_ERROR;
        }	
    }

    memset(&event, 0, sizeof(GxMsgProperty_EpgGet));
    memset(event_info, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);

    event.ts_id = ops->prog_info.ts_id;
    event.orig_network_id = ops->prog_info.orig_network_id;
    event.service_id = ops->prog_info.service_id;
#ifdef MULTI_TP_EPG_INFO
    event.ts_id = app_epg_get_cur_prog_tp_id();//ops->prog_info.tp_id;
#endif
    event.get_event_pos = pos;
    event.want_event_count = EVENT_GET_NUM;
    event.epg_info_size = EVENT_GET_NUM*EVENT_GET_MAX_SIZE;
    event.epg_info = event_info;

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_EPG_GET, (void *)(&event))
            && EVENT_GET_NUM == event.get_event_count)
    {
        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

void epg_ops_init(AppEpgOps * ops)
{
	ops->event_cnt_update = epg_count_update;
	ops->prog_info_update = epg_prog_info_update;
	ops->get_day_event_cnt = epg_get_day_event_count;
	ops->get_day_event_info = epg_get_day_event_info;
	ops->get_cur_event_cnt = epg_get_cur_event_count;
	ops->get_cur_event_info = epg_get_cur_event_info;
	ops->get_next_event_info = epg_get_next_event_info;

	memset(&ops->event_cnt, 0, sizeof(ops->event_cnt));
	memset(&ops->prog_info, 0, sizeof(ops->prog_info));
}


