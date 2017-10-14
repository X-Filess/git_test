#include "app_module.h"
#include "gxsi.h"
#include "gxmsg.h"
#include "app_send_msg.h"
#include "gxcore.h"
#include "app_wnd_search.h"
#include "module/app_sdt.h"
#include "app_book.h"
#if 1
typedef struct service_infor_tag
{
    int serviceId;
    int service_name_len;
    char service_name[32];
}service_infor_t;
static int program_count = 0;

service_infor_t SdtServiceInfo[128];
#define NEQ_PARSE_TIME_MS 24*60*60*1000UL
static handle_t s_sdt_mutex = -1;
uint8_t ver_no_match;
extern void app_epg_info_clean(void);
void app_sdt_modify(void);
static int32_t app_sdt_parse_des(service_infor_t *serviceInfor,uint8_t* src,uint32_t len)
{
    int32_t size = len;
    uint8_t* p = src;
    uint32_t descriptor_length = 0;
    uint32_t service_provider_length = 0;
    
    while(size > 0)
    {
        switch(p[0])
        {
            case 0x48:
            {
                descriptor_length = p[1];
                service_provider_length = p[3];

                p += 4;//跳过头部
                size -= 4;
                p += service_provider_length;//跳过第一个描述
                size -= service_provider_length;
                serviceInfor->service_name_len= p[0];
                if (serviceInfor->service_name_len != 0)
                {
                    memset(serviceInfor->service_name,0x0,MAX_PROG_NAME);
                    memcpy( serviceInfor->service_name ,&p[1],serviceInfor->service_name_len);
                    printf("0x%x   ----%s \n",serviceInfor->serviceId,serviceInfor->service_name);
                }
                p += 1;//跳过第二个描述
                p += serviceInfor->service_name_len;
                size -= 1;
                size -= serviceInfor->service_name_len;
            }
            break;
            default:
            {
                descriptor_length = p[1];
                p += 2;//跳过头
                p += descriptor_length;//跳过该tag剩余字节
                size -= 2;
                size -= descriptor_length;
            }
            break;
        }
    }
    return 0;
}
static int32_t app_sdt_parse_loop(uint8_t* src,uint32_t len) 
{
    int32_t size = len;
    uint32_t sdt_info_count = 0;
    uint8_t* p = src;
    int32_t ret = 0;
    int32_t descr_loop_len = 0;
    if (src == NULL)
    {
        return 0;
    }
    //printf("start to app_sdt_parse_loop--------------->>>>len = %d \n",len);
    while(size > 0)
    {
        sdt_info_count++;
        SdtServiceInfo[sdt_info_count-1].serviceId= ((p[0]<<8 )|p[1]);
        descr_loop_len = (((p[3] & 0x0f) << 8 ) |p[4]);
       // printf("service id = %d descr_loop_len =%d \n",SdtServiceInfo[sdt_info_count-1].serviceId,descr_loop_len);
        ret = app_sdt_parse_des(& SdtServiceInfo[sdt_info_count-1],
                &p[5],descr_loop_len);
        if(ret < 0)
        {
            return 0;
        }
        p += 5;
        p += descr_loop_len;
        size -= 5;
        size -= descr_loop_len;
    }
    return sdt_info_count;


}
#if 0
static void ch_ed_progname_release_cb(PopKeyboard *data)
{
    uint32_t sel = 0;
    GxMsgProperty_NodeByIdGet node;
    GxMsgProperty_NodeModify node_modify = {0};

    if(data->in_ret == POP_VAL_CANCEL)
    {
	  GUI_SetProperty("wnd_channel_edit","update",NULL);  
          return;
    }

    if (data->out_name == NULL)
        return;

    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
    node.node_type = NODE_PROG;
    node.id = g_AppChOps.get_id(sel);
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    node_modify.node_type = NODE_PROG;
    node_modify.prog_data = node.prog_data;
    memcpy(node_modify.prog_data.prog_name, data->out_name, MAX_PROG_NAME-1);
    app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
    GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
    GUI_SetProperty(TXT_PROG_INFO, "string", node_modify.prog_data.prog_name);
}
#endif
private_parse_status sdt_data_got(uint8_t* pSectionData, size_t Size)
{
	uint32_t ret = PRIVATE_SECTION_OK;
	GxMsgProperty_NodeByPosGet node_prog = {0};
	uint8_t *date;
	int16_t len1 = 0;
	uint16_t ts_id = 0;
	uint8_t version = 0;
	uint16_t gSdt_tsid = 0;
       int section_len = 0;
	date = pSectionData;
      int len  = Size;
	//printf("===========================[%s]=====================================\n",__FUNCTION__);
	if (NULL == pSectionData)
		return ret;
	
    if(date[0] ==0x42)
    {
        program_count = 0;

        section_len = ((date[1]&0x0f) << 8) | date[2];
        ts_id = (date[3] << 8) | date[4];
        version = ((date[5] >> 1) & 0x1f);

        node_prog.node_type = NODE_PROG;
        node_prog.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

        gSdt_tsid = node_prog.prog_data.ts_id;
        printf("=======program sdt _ver =%d currsdt_ver=%d\n",node_prog.prog_data.sdt_version,version);
        printf("=======program_tsid =%d curr_tsid=%d\n",node_prog.prog_data.ts_id,gSdt_tsid);
	 gSdt_tsid = node_prog.prog_data.ts_id;


        if ((gSdt_tsid != ts_id)||(node_prog.prog_data.sdt_version == version))
        {
            printf("sdt len error = %d \n",len1);
            return ret;
        }		

        len -= 11;//头部解析完成
        len -= 4;//去掉crc

        program_count =app_sdt_parse_loop(&date[11],len);

        g_AppSdt.ver = version;
        ret = PRIVATE_SUBTABLE_OK;
    }

	return ret;
}

static uint32_t app_sdt_stop(AppSdt *sdt)
{
	printf("===========================[%s]=====================================\n",__FUNCTION__);
	if (sdt->subt_id != -1)
	{
		GxCore_MutexLock(s_sdt_mutex);
		GxSubTableDetail subt_detail = {0};
		GxMsgProperty_SiGet si_get;
		GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)sdt->subt_id;

		subt_detail.si_subtable_id = sdt->subt_id;
		si_get = &subt_detail;
		app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void*)&si_get);

		app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
		sdt->subt_id = -1;
		GxCore_MutexUnlock(s_sdt_mutex);
		return subt_detail.si_filter.pid;
	}
	return 0;
}

static void app_sdt_start(AppSdt *sdt)
{
	GxSubTableDetail subt_detail = {0};
	GxMsgProperty_SiCreate  params_create;
	GxMsgProperty_SiStart params_start;
	printf("===========================[%s]=====================================\n",__FUNCTION__);
    if(s_sdt_mutex == -1)
    {
        GxCore_MutexCreate(&s_sdt_mutex);
    }
      memset(&SdtServiceInfo,0x0,sizeof(service_infor_t)*100);
        sdt->stop(sdt);	

	subt_detail.ts_src = (uint16_t)sdt->ts_src;;
	subt_detail.demux_id = 0;
	subt_detail.time_out = NEQ_PARSE_TIME_MS;
	subt_detail.si_filter.pid = SDT_PID;
	subt_detail.si_filter.match_depth = 6;
	subt_detail.si_filter.eq_or_neq = NEQ_MATCH;
	subt_detail.si_filter.match[0] = 0x42;
	subt_detail.si_filter.mask[0] = 0xff;
	subt_detail.si_filter.match[5] = sdt->ver<<1;
	subt_detail.si_filter.mask[5] = 0x3e;
	subt_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
	subt_detail.table_parse_cfg.table_parse_fun = sdt_data_got;
	GxCore_MutexLock(s_sdt_mutex);
	params_create = &subt_detail;

	app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);

	sdt->subt_id = subt_detail.si_subtable_id;
	sdt->req_id = subt_detail.request_id;
	params_start = sdt->subt_id;
	app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);
	GxCore_MutexUnlock(s_sdt_mutex);
}

void app_sdt_modify(void)
{
	GxSubTableDetail subt_detail = {0};
	GxMsgProperty_SiGet params_get = NULL;
	GxMsgProperty_SiModify params_modify = NULL;
        //g_AppSdt.stop(&g_AppSdt);	
	printf("===========================[%s]=[%d]====================================\n",__FUNCTION__,__LINE__);

	GxCore_MutexLock(s_sdt_mutex);
	// get original pat info
	subt_detail.si_subtable_id = g_AppSdt.subt_id;
	params_get = &subt_detail;

	app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void*)(&params_get));

	// change to unmatch filter
	subt_detail.si_filter.match_depth = 6;
	subt_detail.si_filter.eq_or_neq = NEQ_MATCH;
	subt_detail.si_filter.match[5] = g_AppSdt.ver<<1;
	subt_detail.si_filter.mask[5] = 0x3e;
	subt_detail.time_out = NEQ_PARSE_TIME_MS;
	subt_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
	subt_detail.table_parse_cfg.table_parse_fun = sdt_data_got;

	params_modify = &subt_detail;
	app_send_msg_exec(GXMSG_SI_SUBTABLE_MODIFY, (void*)&params_modify);
	GxCore_MutexUnlock(s_sdt_mutex);
}

AppSdt g_AppSdt = 
{
	.subt_id    = -1,
	.req_id     = 0,
	.ver        = 0xff,
	.start      = app_sdt_start,
	.stop       = app_sdt_stop,
	.analyse    = NULL,
	.cb         = NULL,
};
#endif
