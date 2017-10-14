#include "app_module.h"
#include "gxsi.h"
#include "gxmsg.h"
#include "app_send_msg.h"
#include "gxcore.h"
#include "app_wnd_search.h"
#include "module/app_pat.h"
#include "app_book.h"

#define NEQ_PARSE_TIME_MS 24*60*60*1000UL
static int pat_stop_flag =0;
static handle_t s_pat_mutex = -1;
uint8_t ver_no_match;
bool BackStageSearch = false;
extern uint32_t app_t2_get_acu_config(void);
extern void app_epg_info_clean(void);
extern uint8_t app_backstage_search(void);

int app_get_backsearch_status(void)
{
    return BackStageSearch;
}

int app_set_backsearch_status(bool flag)
{
    BackStageSearch = flag;
    return BackStageSearch;
}

private_parse_status pat_data_got(uint8_t* pSectionData, size_t Size)
{
	uint32_t ret = PRIVATE_SECTION_OK;
	GxMsgProperty_NodeByPosGet node_prog = {0};
	uint8_t *date;
	int16_t len1 = 0;
	uint16_t ts_id = 0;
	uint8_t version = 0;
	uint16_t gPat_tsid = 0;
	date = pSectionData;
	//printf("===========================[%s]=====================================\n",__FUNCTION__);
	if (NULL == pSectionData)
		return ret;
	
	if(date[0] == PAT_TID)
	{
		len1 = ((date[1]&0x0f) << 8) | date[2];
		ts_id = (date[3] << 8) | date[4];
		version = (date[5]&0x3e)>>1;

		node_prog.node_type = NODE_PROG;
		node_prog.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

		gPat_tsid = node_prog.prog_data.ts_id;
		//printf("=======stream_ver =%d curr_ver=%d  gPat_tsid = %d ts_id \n",node_prog.prog_data.pat_version,version,gPat_tsid,ts_id);
		if ((len1 > 1021)||(gPat_tsid != ts_id)||(node_prog.prog_data.pat_version == version))
		{
			printf("pat len error = %d gPat_tsid = %d ts_id =%d \n",len1,gPat_tsid,ts_id);
			return ret;
		}		
             if(pat_stop_flag == 1)
            {
                printf("pat already stop ver = %d \n",version);
                return ret;
            }
		g_AppPat.ver = version;
		//g_AppPlayOps.program_stop();//stop play
		ret = PRIVATE_SUBTABLE_OK;
		
	}
	return ret;
}

static uint32_t app_pat_stop(AppPat *pat)
{
        pat_stop_flag = 1;
	printf("===========================[%s]=====================================\n",__FUNCTION__);
	if (pat->subt_id != -1)
	{
		GxCore_MutexLock(s_pat_mutex);
		GxSubTableDetail subt_detail = {0};
		GxMsgProperty_SiGet si_get;
		GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)pat->subt_id;

		subt_detail.si_subtable_id = pat->subt_id;
		si_get = &subt_detail;
		app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void*)&si_get);

		app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
		pat->subt_id = -1;
		GxCore_MutexUnlock(s_pat_mutex);
		return subt_detail.si_filter.pid;
	}
	return 0;
}

static void app_pat_start(AppPat *pat)
{
	GxSubTableDetail subt_detail = {0};
	GxMsgProperty_SiCreate  params_create;
	GxMsgProperty_SiStart params_start;
	printf("===========================[%s]=====================================\n",__FUNCTION__);
	if(BackStageSearch)
		return;
    if(s_pat_mutex == -1)
    {
        GxCore_MutexCreate(&s_pat_mutex);
    }
	pat->stop(pat);	
    pat_stop_flag = 0;

	subt_detail.ts_src = (uint16_t)pat->ts_src;;
	subt_detail.demux_id = 0;
	subt_detail.time_out = NEQ_PARSE_TIME_MS;
	subt_detail.si_filter.pid = PAT_PID;
	subt_detail.si_filter.match_depth = 6;
	subt_detail.si_filter.eq_or_neq = NEQ_MATCH;
	subt_detail.si_filter.match[0] = PAT_TID;
	subt_detail.si_filter.mask[0] = 0xff;
	subt_detail.si_filter.match[5] = pat->ver<<1;
	subt_detail.si_filter.mask[5] = 0x3e;
	subt_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
	subt_detail.table_parse_cfg.table_parse_fun = pat_data_got;
	GxCore_MutexLock(s_pat_mutex);
	params_create = &subt_detail;

	app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);

	pat->subt_id = subt_detail.si_subtable_id;
	pat->req_id = subt_detail.request_id;
	params_start = pat->subt_id;
	app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);
	GxCore_MutexUnlock(s_pat_mutex);
}

void app_pat_modify(void)
{
	GxSubTableDetail subt_detail = {0};
	GxMsgProperty_SiGet params_get = NULL;
	GxMsgProperty_SiModify params_modify = NULL;
	GxCore_MutexLock(s_pat_mutex);
	// get original pat info
	subt_detail.si_subtable_id = g_AppPat.subt_id;
	params_get = &subt_detail;

	app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void*)(&params_get));

	// change to unmatch filter
	subt_detail.si_filter.match_depth = 6;
	subt_detail.si_filter.eq_or_neq = NEQ_MATCH;
	subt_detail.si_filter.match[5] = g_AppPat.ver<<1;
	subt_detail.si_filter.mask[5] = 0x3e;
	subt_detail.time_out = NEQ_PARSE_TIME_MS;
	subt_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
	subt_detail.table_parse_cfg.table_parse_fun = pat_data_got;

	params_modify = &subt_detail;
	app_send_msg_exec(GXMSG_SI_SUBTABLE_MODIFY, (void*)&params_modify);
	GxCore_MutexUnlock(s_pat_mutex);
}

AppPat g_AppPat = 
{
	.subt_id    = -1,
	.req_id     = 0,
	.ver        = 0xff,
	.start      = app_pat_start,
	.stop       = app_pat_stop,
	.analyse    = NULL,
	.cb         = NULL,
};

