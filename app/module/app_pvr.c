/*
 * =====================================================================================
 *
 *       Filename:  app_pvr.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2011年08月05日 10时29分42秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "gxcore.h" 
#include "app_module.h"
#include "gxhotplug.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "module/config/gxconfig.h"
#include "app_epg.h"
#include "app_utility.h"
#include "file_edit.h"
#include "module/app_psi.h"
#include "app_audio.h"

#define TMS_FILE_KEY    "pvr>tms_file"
#define CHECK_ALLOC(p, size)   do {\
    if (p == NULL) {\
    p = (char*)GxCore_Malloc(size);\
    if (p == NULL)  return GXCORE_ERROR;\
    };\
    memset(p, 0, size);\
}while(0)

#define FREE_NULL(p) do{if (p!= NULL){\
    GxCore_Free(p);\
    p=NULL;}}while(0)
/*单时移时，切台处理
 * FB到头，自动开始播放？
 * 时移满了会咋样
 * */
/* *****************FUNCTION********************* */
#define PVR_DEV_LEN    (16)
#define PVR_PATH_LEN    (64)
#define PVR_NAME_LEN    (64)
#define PVR_URL_LEN     GX_PM_MAX_PROG_URL_SIZE
#define PVR_DIR         "/SunPvr"
#define PVR_PATITION_SIZE_GATE    95    //percent

#define PVR_PLAYER      PLAYER_FOR_REC

typedef enum
{
	NO_SECTION = 0,
	ON_SECTION = 3
}SectionNums;

static handle_t s_pvr_mon_sem = -1;
static handle_t s_pvr_mon_thread = -1;
static int s_pvr_mon_enable = 0;
static event_list* s_pvr_mon_timer = NULL;
static uint64_t s_pvr_patition_pencent = 0;

static int32_t pvr_percent_set(AppPvrOps *pvr);

#ifdef LINUX_OS
//获取指定目录下是否存在的叫做[GXPVR](忽略大小写)文件夹的名字，并保存。
status_t _get_pvr_dir_name(const char *path)
{
	char *cmdline = NULL;
	char *p = NULL;
	char temp_buf[100] = {0};
	int len = 0;
	FILE *read_fp = NULL;
	status_t ret = GXCORE_ERROR;

	if((path == NULL) || (strlen(path) == 0))
		return GXCORE_ERROR;

	len = strlen(path) + 100;
	if((cmdline = (char *)GxCore_Calloc(1, len)) == NULL)
		return GXCORE_ERROR;

	strcat(cmdline, "find ");
	strncat(cmdline, path, p - path);
	strcat(cmdline, " -maxdepth 1 -type d -name [Gg][Xx][Pp][Vv][Rr]");

	read_fp = popen(cmdline, "r");
    if (read_fp != NULL)
    {
        if(fread(temp_buf, 1, 99, read_fp) > 0)
        {
			p = strrchr(temp_buf, '/');
			if(p == NULL)
				return GXCORE_ERROR;
			GxBus_ConfigSet(PVR_TRUE_DIR_NAME_KEY, p);
			ret = GXCORE_SUCCESS;
		}
		pclose(read_fp);
	}

	GxCore_Free(cmdline);
	cmdline = NULL;

	return ret;

}
#endif

// return: 0-failed 1-path len
static uint32_t pvr_path_get(char *path, uint32_t max_len)
{
    uint32_t path_len;
    int i;
    char partition[PVR_PATH_LEN] = {0};
    HotplugPartitionList *phpl = NULL;

    phpl = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if (phpl->partition_num < 1)
    {
        return 0;
    }

    GxBus_ConfigGet(PVR_PARTITION_KEY, partition, PVR_PATH_LEN, PVR_PARTITION);
    for(i = 0; i < phpl->partition_num; i++)
    {
        if(strcmp(partition, phpl->partition[i].dev_name) == 0)
        {
            break;
        }
    }
    if(i >= phpl->partition_num)
    {
        i = 0;
        GxBus_ConfigSet(PVR_PARTITION_KEY, phpl->partition[i].dev_name);
    }

	char dir_name[10] = {0};
#ifdef LINUX_OS
	//获取真实的GxPvr目录的名字（主要解决存在的目录名字大小写的差异问题）
	if(_get_pvr_dir_name(phpl->partition[i].partition_entry) == GXCORE_ERROR) {
		strcpy(dir_name, PVR_DIR);
	}else{
		GxBus_ConfigGet(PVR_TRUE_DIR_NAME_KEY, dir_name, 6, PVR_TRUE_DIR_NAME);
	}
#else
	strcpy(dir_name, PVR_DIR);
#endif
    // pvr path space
    path_len = ((strlen(phpl->partition[i].partition_entry))+(strlen(dir_name)));

    if (path_len > max_len)
    {
        return 0;
    }

    // set pvr path
    strcpy(path, phpl->partition[i].partition_entry);
    strcat(path, dir_name);
    //strcat(path, PVR_DIR);

    return path_len;
}

static uint32_t pvr_dev_get(char *dev, uint32_t max_len)
{
    uint32_t dev_len;
    int i;
    char partition[PVR_PATH_LEN] = {0};
    HotplugPartitionList *phpl = NULL;
    char *p = NULL;

    phpl = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if (phpl->partition_num < 1)
    {
        return 0;
    }

    GxBus_ConfigGet(PVR_PARTITION_KEY, partition, PVR_PATH_LEN, PVR_PARTITION);
    for(i = 0; i < phpl->partition_num; i++)
    {
        if(strcmp(partition, phpl->partition[i].dev_name) == 0)
        {
            break;
        }
    }
    if(i >= phpl->partition_num)
    {
        i = 0;
        GxBus_ConfigSet(PVR_PARTITION_KEY, phpl->partition[i].dev_name);
    }

    dev_len = strlen(phpl->partition[i].dev_name);
    if (dev_len > max_len)
    {
        return 0;
    }

    strncpy(dev, phpl->partition[i].dev_name, dev_len);
    p = dev + dev_len - 1;

    if(*p >= '0' && *p <= '9') //remove partition num
    {
        *p = '\0';
        dev_len--;
    }

    return phpl->partition[i].disk_id;
}


static uint32_t pvr_url_get(char *url, uint32_t max_len)
{
    GxMsgProperty_NodeByPosGet node = {0};
    GxMsgProperty_GetUrl url_get;
    char ts_path[30] = {0};
	char dmx_path[16] = {0};

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        return 0;
    }

    url_get.prog_info = node.prog_data;
    url_get.size = PVR_URL_LEN;
    url_get.url = (int8_t*)url;
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_GET_URL, (void *)(&url_get)))
    {
        return 0;
    }
    {
	    uint16_t demxid  = 0;
	    uint16_t ts_src = g_AppPlayOps.normal_play.ts_src;

#if CA_SUPPORT
        int time = app_tsd_time_get();
        if(time > 0)
        {
            memset(ts_path,0,sizeof(ts_path));
		    sprintf(ts_path, "&tsdelay:%d",time);
	        strcat(url, ts_path);
        }
        // 此处，需要保证PVR使用的URL中的DEMUX和TS SOURCE和普通播放保持一致，无
        // 论是TSD模式和是普通模式，如果录制使用的TS SOURCE和普通播放URL使用的一
        // 样，那必须要使用普通播放URL的TS SOURCE。该处理是配合PLAYER中的数据记
        // 录，虽然在TSD模式下，PLAYER会使用两个TS SOOURCE，但是记录的会是实际
        // DEMOD数据输入到DEMUX的TS SOURCE。【换言，目前PLAYER中，通过DEMUX ID和
        // TS SOURCE来判断是否要开启多路录制设备以及是否要单独申请DEMUX过滤资源
        // 。如果只有单独录制功能开启，那此处改与不改是没有影响的，但是当方案上
        // 已经开启时移，并调用了转录制的功能后，那此处的TS DOURCE就很关键，如果
        // 不保持和普通播放一致，那PLAYER就会开启多一个录制设备，多浪费2MB多的内
        // 存空间，并且会增加USB的负担，时移和录制的文件都会写入USB设备，系统负
        // 担会增大，USB的效率会成为问题。。。】
        //app_demux_param_adjust(&ts_src, &demxid, 1);
#endif
        memset(ts_path,0,sizeof(ts_path));
	    sprintf(ts_path, "&tsid:%d", ts_src);// need to keep the same ts source with the normal play
	    sprintf(dmx_path, "&dmxid:%d", demxid);//used demux2
    }
    strcat(url, ts_path);
    strcat(url, dmx_path);
    if (strlen(url) > max_len)
    {
        return 0;
    }

    return strlen(url);
}

static void pvr_name_format(char *format_name, char *orig_name)
{
    uint32_t i = 0;
    uint32_t name_len = strlen(orig_name);

    if (format_name == NULL || orig_name == NULL)
    {
        return;
    }

   if(name_len > MAX_PROG_NAME-1)
   {
	name_len = MAX_PROG_NAME-2;
   }

	for(i=(name_len-1);i>=0;i--)
	{
		if(orig_name[i] != 0x20)
		{
			break;
		}
		orig_name[i] = '\0';
	}
	name_len = strlen(orig_name);
    for (i=0; i<name_len; i++)
    {
#if 0
        if ((*(orig_name+i) > 0x7a) 
			||( (*(orig_name+i) < 0x30)&&((*(orig_name+i) != 0x20))))
        {
            *(format_name+i) = '-';
        }
        else
#endif
        {
            *(format_name+i) = *(orig_name+i);
        }
    }

}

static void pvr_time_format(char * file_name)
{
#define ONE_HOUR   (60*60)
    time_t time;
    struct tm *ptm;
    char buf[16];

    time = g_AppTime.utc_get(&g_AppTime);
    time = get_display_time_by_timezone(time);

    ptm = localtime(&time);

    memset(buf, 0, 16);
    sprintf(buf, "_%d%02d%02d%02d%02d%02d", ptm->tm_year+1900, ptm->tm_mon+1,
            ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    strcat(file_name, buf);

}
 
#define PVR_TYPE_TS     0
#define PVR_TYPE_PS     1
#define PVR_REC_TYPE()  PVR_TYPE_TS
// ret: prog id
static uint32_t pvr_name_get(char *name, uint32_t max_len)
{
    GxMsgProperty_NodeByPosGet node = {0};

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        return 0;
    }

    pvr_name_format(name, (char*)node.prog_data.prog_name);
    pvr_time_format(name);

    if (PVR_REC_TYPE() == PVR_TYPE_TS)
    {
        strcat(name, ".ts.dvr");
    }

    return node.prog_data.id;
}

static usb_check_state pvr_usb_check(AppPvrOps* pvr)
{
	int32_t ret_value = 0;
    char env_path[PVR_PATH_LEN] = {0};
    char partition_path[PVR_PATH_LEN] = {0};
    GxDiskInfo disk_info;
    uint64_t free_space;
    usb_check_state state = USB_OK;

    // pvr file path
    if (0 == pvr_path_get(env_path, PVR_PATH_LEN))
    {
        state = USB_ERROR;
    }
    else
    {
        strncpy(partition_path, env_path,
            (strlen(env_path)-strlen(PVR_DIR)));
        ret_value = GxCore_DiskInfo(partition_path, &disk_info);
		if(ret_value == GXCORE_ERROR)
		{
			 state =  USB_ERROR;
		}
        free_space = disk_info.freed_size/1048576;
        if((disk_info.total_size == 0)||((disk_info.used_size * 100) / disk_info.total_size >= PVR_PATITION_SIZE_GATE))
        {
            state =  USB_NO_SPACE;
        }
        else if (GxCore_FileExists(env_path) == GXCORE_FILE_UNEXIST)
        {
            if(GxCore_Mkdir(env_path) < 0)
            {
                state =  USB_ERROR;
            }
        }
    }

    return state;
}

static status_t pvr_env_sync(AppPvrOps* pvr)
{
    if (pvr == NULL)  return GXCORE_ERROR;

    pvr->env_clear(pvr);

    printf("[sync] 1\n");
    // pvr file path
    CHECK_ALLOC(pvr->env.path, PVR_PATH_LEN);
    if (0 == pvr_path_get(pvr->env.path, PVR_PATH_LEN))
    {
        FREE_NULL(pvr->env.path);
        return GXCORE_ERROR;
    }

    printf("[sync] 2\n");
    if (GxCore_FileExists(pvr->env.path) == GXCORE_FILE_UNEXIST) 
    {
        if(GxCore_Mkdir(pvr->env.path) < 0)
        {
            FREE_NULL(pvr->env.path);
            return GXCORE_ERROR;
        }
    }

    printf("[sync] 3\n");
    // pvr url
    CHECK_ALLOC(pvr->env.url, PVR_URL_LEN);
    if (0 == pvr_url_get(pvr->env.url, PVR_URL_LEN))
    {
        FREE_NULL(pvr->env.path);
        FREE_NULL(pvr->env.url);
        return GXCORE_ERROR;
    }

    printf("[sync] 4\n");
    // pvr rec prog id
    CHECK_ALLOC(pvr->env.name, PVR_NAME_LEN);
    pvr->env.prog_id = pvr_name_get(pvr->env.name, PVR_NAME_LEN);

    CHECK_ALLOC(pvr->env.dev, PVR_DEV_LEN);
    pvr_dev_get(pvr->env.dev, PVR_DEV_LEN);

    printf("[sync] 5\n");
    return GXCORE_SUCCESS;
}

static void pvr_env_clear(AppPvrOps* pvr)
{
    if (pvr == NULL)  return;
    // sync env
    pvr->env.prog_id = 0;
    FREE_NULL(pvr->env.name);
    FREE_NULL(pvr->env.path);
    FREE_NULL(pvr->env.url);
    FREE_NULL(pvr->env.dev);
}

static void _pvr_monitor(void* arg)
{
 
    AppPvrOps *pvr = (AppPvrOps*)arg;
    GxCore_ThreadDetach();
    while(1)
	{
		GxCore_SemWait(s_pvr_mon_sem);
		if(s_pvr_mon_enable == 0)
		{
			break;
		}
		s_pvr_patition_pencent = pvr_percent_set(pvr);
		if(s_pvr_patition_pencent >= PVR_PATITION_SIZE_GATE)
		{
			s_pvr_mon_enable = 0;
			GxCore_SemDelete(s_pvr_mon_sem);
			s_pvr_mon_sem = -1;
			s_pvr_mon_thread = -1;
			break;
		}
	}
}

extern void app_pvr_patition_full_proc(void);
static int pvr_mon_timer_cb(void *userdata)
{
    if(s_pvr_mon_thread > 0)
    {
        GxCore_SemPost(s_pvr_mon_sem);
    }
    else
    {
        remove_timer(s_pvr_mon_timer);
        s_pvr_mon_timer = NULL;
        app_pvr_patition_full_proc();
        s_pvr_patition_pencent = 0;
    }

    return 0;
}

static status_t pvr_monitor_start(AppPvrOps *pvr)
{
    if(s_pvr_mon_thread <= 0)
    {
        s_pvr_patition_pencent = 0;
		s_pvr_mon_enable = 1;
		s_pvr_patition_pencent = pvr_percent_set(pvr);
		GxCore_SemCreate(&s_pvr_mon_sem, 1);
        GxCore_ThreadCreate("app_pvr_mon_proc", &s_pvr_mon_thread, _pvr_monitor, (void*)pvr, 10*1024, GXOS_DEFAULT_PRIORITY);
        //GxCore_SemPost(s_pvr_mon_sem);
        if (s_pvr_mon_timer == NULL)
        {
            s_pvr_mon_timer = create_timer(pvr_mon_timer_cb, 2000, NULL, TIMER_REPEAT);
        }
    }

    return GXCORE_SUCCESS;
}

static status_t pvr_monitor_stop(void)
{
    if(s_pvr_mon_thread > 0)
    {
        remove_timer(s_pvr_mon_timer);
        s_pvr_mon_timer = NULL;

        s_pvr_mon_enable = 0;
        GxCore_SemPost(s_pvr_mon_sem);
        GxCore_ThreadJoin(s_pvr_mon_thread);
        s_pvr_mon_thread = -1;
        GxCore_SemDelete(s_pvr_mon_sem);
        s_pvr_mon_sem = -1;
        s_pvr_patition_pencent = 0;
    }

    return GXCORE_SUCCESS;
}

static status_t _pvr_pat_ts_pack(uint16_t prog_id, AppTsData *ts_data)
{
    char *pat_data = NULL;
    int data_len = 0;
    status_t ret = GXCORE_ERROR;

    if(ts_data == NULL)
        return ret;

    pat_data = app_pat_generate(&prog_id, 1);
    if(pat_data != NULL)
    {
        data_len = ((pat_data[1] & 0x0f) << 8) | pat_data[2];
        data_len += 3;
        ret = app_ts_pack(0, pat_data, data_len, ts_data);
        GxCore_Free(pat_data);
        pat_data = NULL;
    }

    return ret;
}

static status_t _pvr_pmt_ts_pack(uint16_t prog_id, AppTsData *ts_data)
{
    char *pmt_data = NULL;
    int data_len = 0;
    GxMsgProperty_NodeByIdGet node = {0};
    status_t ret = GXCORE_ERROR;

    if(ts_data == NULL)
        return ret;

    if((pmt_data = app_pmt_generate(prog_id)) != NULL)
    {
        if(pmt_data != NULL)
        {
            node.node_type = NODE_PROG;
            node.id = prog_id;
            if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node) == GXCORE_SUCCESS)
            {
                data_len = ((pmt_data[1] & 0x0f) << 8) | pmt_data[2];
                data_len += 3;
                ret = app_ts_pack(node.prog_data.pmt_pid, pmt_data, data_len, ts_data);
            }

            GxCore_Free(pmt_data);
            pmt_data = NULL;
        }
    }
    return ret;
}

static void pvr_rec_config(AppPvrOps *pvr, const char *player)
{
    PlayerRecordConfig *config = NULL;
    GxMsgProperty_NodeByPosGet node = {0};
    int i = 0;
    int cnt = 0;
    AppTsData ts_data = {0};
    AudioLang_t *audio_list = NULL;

    if((pvr == NULL) || (player == NULL))
        return;

    config = (PlayerRecordConfig*)GxCore_Calloc(sizeof(PlayerRecordConfig), 1);
    if(config == NULL)
        return;

    config->ext_info.is_inserttable = 1;

    _pvr_pat_ts_pack(pvr->env.prog_id, &ts_data);
    for(i = 0; i < ts_data.pack_num; i++)
    {
        memmove(config->ext_data.userdata + i * 188, ts_data.pack_data[i], 188);
        config->ext_data.userdata_len += 188;
    }
    app_ts_data_free(&ts_data);

    _pvr_pmt_ts_pack(pvr->env.prog_id, &ts_data);
    for(i = 0; i < ts_data.pack_num; i++)
    {
        memmove(config->ext_data.userdata + config->ext_data.userdata_len + i * 188, ts_data.pack_data[i], 188);
        config->ext_data.userdata_len += 188;
    }
    app_ts_data_free(&ts_data);

#if 0
    printf("config->ext_data.userdata_len = %d\n", config->ext_data.userdata_len);
    for(i = 0; i < config->ext_data.userdata_len; i++)
    {
        printf("%02x ", config->ext_data.userdata[i]);
        if(i % 16 == 15)
            printf("\n");
    }
#endif

    ////other audio pid
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        config->ext_info.ext_num = 0;
        audio_list = app_get_audio_lang();
        if((audio_list != NULL) && (audio_list->audioLangList != NULL) && (audio_list->audioLangCnt != 0))
        {
            for(i = 0; i < audio_list->audioLangCnt; i++)
            {
                if(audio_list->audioLangList[i].id != node.prog_data.cur_audio_pid)
                {
                	
					config->ext_info.ext_types[cnt] = DEMUX_SLOT_PES;
                    config->ext_info.ext_pids[cnt++] = audio_list->audioLangList[i].id;
                    //config->ext_info.ext_types[cnt] = DEMUX_SLOT_AUDIO;
                    config->ext_info.ext_num++;
                    //printf("---[%s] id = %d, num = %d ---\n", __func__, config->ext_info.ext_pids[i], config->ext_info.ext_num);
                }
            }
        }
    }

    // sbut
    //for(i = 0; i < g_AppTtxSubt.ttx_num + g_AppTtxSubt.subt_num; i++)
    for(i = g_AppTtxSubt.ttx_num; i < g_AppTtxSubt.ttx_num + g_AppTtxSubt.subt_num; i++)
    {
		config->ext_info.ext_types[cnt] = DEMUX_SLOT_PES;
        config->ext_info.ext_pids[cnt++] = g_AppTtxSubt.content[i].elem_pid;
        config->ext_info.ext_num++;
    }

	#if 0
	if(is_ad() != -1)
	{
		config->ext_info.ext_types[cnt]= DEMUX_SLOT_AUDIO;
		config->ext_info.ext_pids[cnt++]= Ad_elem_pid;
		config->ext_info.ext_num++;
	}
	#endif

    GxPlayer_MediaRecordConfig(player, config);

    GxCore_Free(config);
    config = NULL;
}

static void pvr_rec_start(AppPvrOps *pvr)
{
    GxMsgProperty_PlayerRecord rec_start = {0};
    GxTime tick;

    if (pvr == NULL)
        return;

#ifdef LOOP_TMS_SUPPORE
    GxMsgProperty_PlayerPVRConfig  pvrConfig = {0};
    int32_t init_value;
    GxBus_ConfigGetInt(PVR_FILE_SIZE_KEY, &init_value, PVR_FILE_SIZE_VALUE);
    pvrConfig.volume_sizemb = init_value;
    pvrConfig.volume_maxnum = 0;
    GxPlayer_SetPVRConfig(&pvrConfig);
#endif

    // player open
    if (pvr->state == PVR_DUMMY)
    {
        if (GXCORE_SUCCESS != pvr->env_sync(pvr))
        {
            pvr->env_clear(pvr);
            return;
        }

        if (app_player_open(PVR_PLAYER) != PLAYER_OPEN_OK)
        {
            pvr->env_clear(pvr);
            return;
        }

        //app_epg_disable();

        pvr_monitor_start(pvr);
        rec_start.player = PVR_PLAYER;
        strcpy((char*)rec_start.url, pvr->env.url);
        sprintf((char*)rec_start.file, "%s/%s", pvr->env.path, pvr->env.name);
#if UNICABLE_SUPPORT
		{
			uint32_t centre_frequency=0 ;
			uint32_t lnb_index=0x20 ;
			uint32_t set_tp_req=0;
			char ts_dmx[80] = {0};
			GxMsgProperty_NodeByIdGet sat_get = {0};
			GxMsgProperty_NodeByIdGet tp_get = {0};
			GxMsgProperty_NodeByPosGet prog_node = {0};

			prog_node.node_type = NODE_PROG;
		    prog_node.pos = g_AppPlayOps.normal_play.play_count;
		    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node));
			
			sat_get.node_type = NODE_SAT;
			sat_get.id = prog_node.prog_data.sat_id;
			app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_get);
	
			tp_get.node_type = NODE_TP;
			tp_get.id = prog_node.prog_data.tp_id;
			app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_get);		
			if(sat_get.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
			{
	
				lnb_index = app_calculate_set_freq_and_initfreq(&sat_get.sat_data, &tp_get.tp_data,&set_tp_req,&centre_frequency);
				sprintf(ts_dmx, "&ifindex:%d&lnbindex:%d&centrefre:%d&iffre:%d",sat_get.sat_data.sat_s.unicable_para.if_channel,\
					lnb_index, centre_frequency,sat_get.sat_data.sat_s.unicable_para.centre_fre[sat_get.sat_data.sat_s.unicable_para.if_channel]);
				strcat(rec_start.url, ts_dmx);
			}
		}
#endif

#if MUTI_TS_SUPPORT
	{
		GxMsgProperty_NodeByPosGet prog_node = {0};
		char ts_dmx[80] = {0};
		prog_node.node_type = NODE_PROG;
	    prog_node.pos = g_AppPlayOps.normal_play.play_count;
	    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node));
		if(prog_node.prog_data.muti_ts_status)
		{
			sprintf(ts_dmx, 
				    "&pls_ts_id:%d&pls_code:%d&pls_change:%d&pls_status:%d", 
				    prog_node.prog_data.muti_ts_id, 
				    prog_node.prog_data.ts_pls_code,
				    prog_node.prog_data.ts_pls_chanage,
				    prog_node.prog_data.muti_ts_status);
			strcat(rec_start.url, ts_dmx);
		}
	}
#endif


        pvr_rec_config(pvr, PLAYER_FOR_REC);

        if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PLAYER_RECORD, (void *)(&rec_start)))
        {
            app_player_close(PVR_PLAYER);
        }

        GxCore_GetTickTime(&tick);
        pvr->time.cur_tick = 0;
        pvr->time.total_tick = 0;
        pvr->time.remaining_tick = 0;
        pvr->enter_shift   = 0;

        pvr->state = PVR_RECORD;
    }
    else if (pvr->state == PVR_TIMESHIFT)
    {
        if (app_player_open(PVR_PLAYER) == PLAYER_OPEN_OK) //for logic
        {
            rec_start.player = PVR_PLAYER;
            strcpy((char*)rec_start.url, pvr->env.url);
            sprintf((char*)rec_start.file, "%s/%s", pvr->env.path, pvr->env.name);
            if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PLAYER_RECORD, (void *)(&rec_start)))
            {
                app_player_close(PVR_PLAYER);
            }
            else
            {
                pvr->state = PVR_TMS_AND_REC;
                GxBus_ConfigSet(TMS_FILE_KEY, "abc");
            }
        }
    }
    else
    {}

    // play list lock current tp

#if CA_SUPPORT
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT 
    if(pvr->env.prog_id && (pvr->state == PVR_RECORD))
    {
        int demux_id = 0;
        GxMsgProperty_NodeByIdGet node = {0};

        app_getvalue(pvr->env.url,"dmxid",&demux_id);
        if(app_tsd_check() > 0)
        {
            if(g_AppPlayOps.normal_play.dmx_id == demux_id)// use the same demux with the normal play,can not release the source
                return ;
        }
        node.node_type = NODE_PROG;
        node.id = pvr->env.prog_id;
        if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node) == GXCORE_SUCCESS)
        {
            app_extend_send_service(pvr->env.prog_id, node.prog_data.sat_id, node.prog_data.tp_id, demux_id, CONTROL_PVR);
        }
    }
#endif
#endif
}

static void pvr_tp_rec_start(AppPvrOps *pvr)
{
    GxMsgProperty_PlayerRecord rec_start = {0};
    PlayerRecordConfig *config = NULL;
    GxTime tick;

    if (pvr == NULL)
        return;

    // player open
    if (pvr->state != PVR_DUMMY)
        return;

    config = (PlayerRecordConfig*)GxCore_Calloc(sizeof(PlayerRecordConfig), 1);
    if(config == NULL)
        return;

    if (GXCORE_SUCCESS != pvr->env_sync(pvr))
    {
        pvr->env_clear(pvr);
        return;
    }

    if (app_player_open(PVR_PLAYER) != PLAYER_OPEN_OK)
    {
        pvr->env_clear(pvr);
        return;
    }

    config->ext_info.is_rawts = 1;
    GxPlayer_MediaRecordConfig(PLAYER_FOR_REC, config);
    GxCore_Free(config);
    config = NULL;

    pvr_monitor_start(pvr);
    rec_start.player = PVR_PLAYER;
    strcpy((char*)rec_start.url, pvr->env.url);
    sprintf((char*)rec_start.file, "%s/%s", pvr->env.path, pvr->env.name);

    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PLAYER_RECORD, (void *)(&rec_start)))
    {
        app_player_close(PVR_PLAYER);
    }

    GxCore_GetTickTime(&tick);
    pvr->time.cur_tick = 0;
    pvr->time.total_tick = 0;
    pvr->time.remaining_tick = 0;

    pvr->state = PVR_TP_RECORD;
}

static void pvr_rec_stop(AppPvrOps *pvr)
{
    if((pvr->state == PVR_RECORD)
        || (pvr->state == PVR_TMS_AND_REC)
        || (pvr->state == PVR_TP_RECORD))
    {
        app_player_close(PVR_PLAYER);
        pvr_monitor_stop();
    }
#if CA_SUPPORT
    if(pvr->env.prog_id)
    {
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
        app_descrambler_close_by_control_type(CONTROL_PVR);
#if ECM_SUPPORT
		// TODO:20120702
		g_AppEcm2.stop(&g_AppEcm2);
#endif
#endif
    }
#endif

    if(pvr->state == PVR_RECORD)
    {
        pvr->env_clear(pvr);
        pvr->state = PVR_DUMMY;
        //g_AppPmt.start(&g_AppPmt);
        //app_epg_enable(g_AppPlayOps.normal_play.ts_src,TS_2_DMX);
    }
    else if(pvr->state == PVR_TP_RECORD)
    {
        pvr->env_clear(pvr);
        pvr->state = PVR_DUMMY;
    }
    else if(pvr->state == PVR_TMS_AND_REC)
    {
        pvr->state = PVR_TIMESHIFT;
    }
}


static void pvr_tms_stop(AppPvrOps *pvr)
{
    GxMsgProperty_PlayerTimeShift time_shift;

    if (pvr == NULL)
        return;

    memset(&time_shift, 0, sizeof(GxMsgProperty_PlayerTimeShift));
    pvr->enter_shift   = 0;
    time_shift.enable = 0;
    app_send_msg_exec(GXMSG_PLAYER_TIME_SHIFT, (void*)(&time_shift));

    if((pvr->state == PVR_TIMESHIFT)
            ||(pvr->state == PVR_TMS_AND_REC))
    {
        app_player_close(PLAYER_FOR_TIMESHIFT);
        app_player_open(PLAYER_FOR_NORMAL);
    }

    if(pvr->state == PVR_TIMESHIFT)
    {
    	
#ifdef LOOP_TMS_SUPPORE
        GxMsgProperty_PlayerPVRConfig  pvrConfig = {0};
        int32_t init_value;
        GxBus_ConfigGetInt(PVR_FILE_SIZE_KEY, &init_value, PVR_FILE_SIZE_VALUE);
        pvrConfig.volume_sizemb = init_value;
        pvrConfig.volume_maxnum = 0;
        GxPlayer_SetPVRConfig(&pvrConfig);
#endif
        pvr_monitor_stop();
        pvr->env_clear(pvr);
        pvr->state = PVR_DUMMY;
        //g_AppPmt.start(&g_AppPmt);
        pvr->tms_delete(pvr);
#if CA_SUPPORT
        {
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
            app_descrambler_close_by_control_type(CONTROL_PVR);
#if ECM_SUPPORT
            // TODO:20120702
            g_AppEcm2.stop(&g_AppEcm2);
#endif
#endif
        }
#endif

    }
    else if(pvr->state == PVR_TMS_AND_REC)
    {
#ifdef LOOP_TMS_SUPPORE
        GxMsgProperty_PlayerPVRConfig  pvrConfig = {0};
        int32_t init_value;
        GxBus_ConfigGetInt(PVR_FILE_SIZE_KEY, &init_value, PVR_FILE_SIZE_VALUE);
        pvrConfig.volume_sizemb = init_value;
        pvrConfig.volume_maxnum = 0;
        GxPlayer_SetPVRConfig(&pvrConfig);
#endif

        pvr->state = PVR_RECORD;
    }
    app_epg_enable(g_AppPlayOps.normal_play.ts_src,TS_2_DMX);
}

static void pvr_pause(AppPvrOps *pvr)
{
    GxMsgProperty_PlayerPause player_pause;
    GxMsgProperty_PlayerTimeShift time_shift;
    GxTime tick;

    if (pvr == NULL)   return;

    //pvr->spd = PVR_SPD_1;
    app_subt_pause();

    if (pvr->state == PVR_DUMMY)
    {
        // first time init pvr env
        if (GXCORE_SUCCESS != pvr->env_sync(pvr))
        {
            pvr->env_clear(pvr);
            return;
        }

#ifdef LOOP_TMS_SUPPORE
        GxMsgProperty_PlayerPVRConfig  pvrConfig = {0};

        char partition_path[PVR_PATH_LEN] = {0};
        GxDiskInfo disk_info = {0};
        uint64_t free_space;
        int32_t init_value;

        strncpy(partition_path, pvr->env.path,
                (strlen(pvr->env.path)-strlen(PVR_DIR)));

        GxCore_DiskInfo(partition_path, &disk_info);
        free_space = disk_info.total_size * PVR_PATITION_SIZE_GATE / 100;
        free_space -= disk_info.used_size;
        free_space /= SIZE_UNIT_M;

        GxBus_ConfigGetInt(PVR_FILE_SIZE_KEY, &init_value, PVR_FILE_SIZE_VALUE);
        pvrConfig.volume_sizemb = init_value;//PVR_FILE_SIZE_VALUE;
        pvrConfig.volume_maxnum = 0;
        if((init_value > 0) && (free_space > init_value))
            pvrConfig.volume_maxnum = free_space / init_value;

        GxPlayer_SetPVRConfig(&pvrConfig);
#endif
        pvr_monitor_start(pvr);
        pvr_rec_config(pvr, PLAYER_FOR_NORMAL);
        //g_AppPmt.stop(&g_AppPmt);
        app_epg_disable();
        memset(&time_shift, 0, sizeof(GxMsgProperty_PlayerTimeShift));
        time_shift.enable = 1;
        // TODO: for new driver for DEMUX control, DEMX2(id = 1) for PVR timeshift
        time_shift.shift_dmxid = 0;
#if CA_SUPPORT
{
        uint16_t ts_src = 0;
        uint16_t DemuxId = time_shift.shift_dmxid;
        app_demux_param_adjust(&ts_src, &DemuxId, 1);
        time_shift.shift_dmxid = DemuxId;
}
#endif
        sprintf((char*)time_shift.url, "%s/%s", pvr->env.path, pvr->env.name);

        app_send_msg_exec(GXMSG_PLAYER_TIME_SHIFT, (void*)(&time_shift));

        GxBus_ConfigSet(TMS_FILE_KEY, pvr->env.name);

        GxCore_GetTickTime(&tick);
        pvr->time.cur_tick = 0;
        pvr->time.total_tick = 0;
        pvr->time.remaining_tick = 0;

        pvr->state = PVR_TIMESHIFT;
    }
    else if (pvr->state == PVR_RECORD)
    {
        pvr->state = PVR_TMS_AND_REC; 
        time_shift.enable = 1;
        // TODO: for new driver for DEMUX control, DEMX2(id = 1) for PVR timeshift
        time_shift.shift_dmxid = 0;
        app_epg_disable();
#if  CA_SUPPORT
{
        uint16_t ts_src = 0;
        uint16_t DemuxId = time_shift.shift_dmxid;

        app_demux_param_adjust(&ts_src, &DemuxId, 1);
        time_shift.shift_dmxid = DemuxId;
}
#endif
        sprintf((char*)time_shift.url, "%s/%s", pvr->env.path, pvr->env.name);

        app_send_msg_exec(GXMSG_PLAYER_TIME_SHIFT, (void*)(&time_shift));
    }
    else
    {}

    player_pause.player = PLAYER_FOR_NORMAL;
    app_send_msg_exec(GXMSG_PLAYER_PAUSE, (void *)(&player_pause));

#if CA_SUPPORT
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT 
    if(pvr->state == PVR_TIMESHIFT)
    {
        int demux_id = 0;
        GxMsgProperty_NodeByIdGet node = {0};

        app_getvalue(pvr->env.url,"dmxid",&demux_id);
        if(app_tsd_check() > 0)
        {
            if(g_AppPlayOps.normal_play.dmx_id == demux_id)// use the same demux with the normal play,can not release the source
                return ;
        }
        node.node_type = NODE_PROG;
        node.id = pvr->env.prog_id;
        if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node) == GXCORE_SUCCESS)
        {
            app_extend_send_service(pvr->env.prog_id, node.prog_data.sat_id, node.prog_data.tp_id, demux_id, CONTROL_PVR);
        }

    }
#endif
#endif
}

static void pvr_resume(AppPvrOps *pvr)
{
    if (pvr == NULL)
        return;

    pvr->enter_shift   = 1;
    GxMsgProperty_PlayerResume player_resume;
    player_resume.player = PLAYER_FOR_NORMAL;
    app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
}

static void pvr_seek(AppPvrOps *pvr, int64_t offset)
{
#define SEC_TO_MSEL     (1000)
    GxMsgProperty_PlayerSeek seek;

    seek.player = PLAYER_FOR_NORMAL;
    seek.time = offset*SEC_TO_MSEL;
    seek.flag = SEEK_ORIGIN_SET; 

    app_send_msg_exec(GXMSG_PLAYER_SEEK, (void *)(&seek));
}

static void pvr_play_speed(AppPvrOps *pvr, float speed)
{
    GxMsgProperty_PlayerSpeed player_speed;

    if (pvr == NULL)
        return;

    player_speed.player = PLAYER_FOR_NORMAL;
    player_speed.speed = (float)speed;

	//speed 
	switch ((int32_t)speed)
	{
		case PVR_SPD_1:
			  pvr->spd = 0;
			break;
		case PVR_SPD_2:
			pvr->spd = 1;
			break;
		case PVR_SPD_4:
			pvr->spd = 2;
			break;
		case PVR_SPD_8:
			pvr->spd = 3;
			break;
		case PVR_SPD_16:
			pvr->spd = 4;
			break;
		case PVR_SPD_32:
			pvr->spd = 5;
			break;
		default:
			break;
	}  

    printf("start speed %f", speed);
    app_send_msg_exec(GXMSG_PLAYER_SPEED, (void *)(&player_speed));
   
}

static void pvr_tms_delete(AppPvrOps* pvr)
{
    char file_path[PVR_PATH_LEN] = {0};
    char file_name[PVR_NAME_LEN] = {0};
    char tms_file[PVR_PATH_LEN+PVR_NAME_LEN] = {0};
	char tms_folder[PVR_PATH_LEN+PVR_NAME_LEN] = {0};
	char ret,length;

    if (pvr->state!=PVR_DUMMY)   return;
	
	GxBus_ConfigGet(TMS_FILE_KEY, file_name, PVR_NAME_LEN, "abc");
	if(0 == memcmp("abc",file_name,3))  return;

    if (0 == pvr_path_get(file_path, PVR_PATH_LEN))
    {
        return;
    }

    if (GxCore_FileExists(file_path) == GXCORE_FILE_UNEXIST) 
    {
        // no dir , needn't delete
        return;
    }

    sprintf(tms_file, "%s/%s", file_path, file_name);

    //if (GxCore_FileExists(pvr->env.path) == GXCORE_FILE_UNEXIST) 
    //{
    //    return;
    //}
    if (GxCore_FileExists(tms_file) == GXCORE_FILE_UNEXIST) 
    {
        // no dir , needn't delete
        return;
    }

	ret = GxCore_FileDelete(tms_file);

	length = strlen(tms_file);
	strncpy(tms_folder,tms_file,(length-7));
    if (GxCore_FileExists(tms_folder) == GXCORE_FILE_UNEXIST) 
    {
        // no dir , needn't delete
        return;
    }
    ret = file_edit_delete(tms_folder);
	GxBus_ConfigSet(TMS_FILE_KEY, "abc");
}

static int32_t pvr_percent_set(AppPvrOps *pvr)
{
    char partition_path[PVR_PATH_LEN] = {0};
    GxDiskInfo disk_info;

    if(pvr->env.path != NULL)
    {
        strncpy(partition_path, pvr->env.path,
                (strlen(pvr->env.path)-strlen(PVR_DIR)));

        GxCore_DiskInfo(partition_path, &disk_info);
        if(disk_info.total_size > 0)
        {
            return (disk_info.used_size*100)/disk_info.total_size;
        }
    }
	return -1;
}

static int32_t pvr_percent_get(AppPvrOps *pvr)
{
	if(s_pvr_mon_thread > 0)
		return s_pvr_patition_pencent;
    return -1;
}

/*static status_t pvr_check_free_space(AppPvrOps *pvr)
{
	char partition_path[PVR_PATH_LEN] = {0};
    GxDiskInfo disk_info;
	uint32_t free_space;

    strncpy(partition_path, pvr->env.path,
            (strlen(pvr->env.path)-strlen(PVR_DIR)));

    GxCore_DiskInfo(partition_path, &disk_info);
	free_space = disk_info.freed_size/1048576;
	if(free_space<100)
	{
		return GXCORE_ERROR;
	}
	return GXCORE_SUCCESS;
}*/

AppPvrOps g_AppPvrOps =
{
    .usb_check      =   pvr_usb_check,
    .env_sync       =   pvr_env_sync,
    .env_clear      =   pvr_env_clear,
    .rec_start      =   pvr_rec_start,
    .tp_rec_start   =   pvr_tp_rec_start,
    .rec_stop       =   pvr_rec_stop,
    .tms_stop       =   pvr_tms_stop,
    .pause          =   pvr_pause,
    .resume         =   pvr_resume,
    .seek           =   pvr_seek,
    .speed          =   pvr_play_speed,
    .percent        =   pvr_percent_get,
    .tms_delete     =   pvr_tms_delete,
    //.free_space     =   pvr_check_free_space
};


