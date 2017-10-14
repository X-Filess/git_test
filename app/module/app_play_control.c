/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009-2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_play_control.c
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.2.27	           shenbin         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "app_module.h"
#include "app_send_msg.h"
#include "app_msg.h"
#include "app_bsp.h"
#include "app_default_params.h"
#include "../full_screen.h" /* need sync lock state, so embeded  */
#include "app_frontend.h"
#include "app_epg.h"
//#include "module/app_ca_manager.h"
#include "app_pop.h"
#include "app_frontend.h"

#if TKGS_SUPPORT
#include "module/gxtkgs.h"
#include "tkgs/app_tkgs_background_upgrade.h"
#endif

//#define ALL_TV_KEY      "pos>tv"
//#define ALL_TV_KEY      "pos>tv"
//#define ALL_TV_POS      0
//#define ALL_RADIO_KEY   "pos->radio"
//#define ALL_RADIO_POS   0

#define ALL_TV_KEY      "prog_id>tv"
#define ALL_TV_ID      0
#define ALL_RADIO_KEY   "prog_id>radio"
#define ALL_RADIO_ID   0
#define PLAY_DELAY_MS (300)

typedef struct
{
    int32_t     play_count;
    uint32_t    key;
} PlayPwdCbData;

static PlayerWindow s_NormalRect = {0, 0, APP_XRES, APP_YRES};

static event_list *s_play_delay_timer = NULL;
static event_list *s_clear_paly_flag_timer = NULL;
static event_list *s_play_rec_timer = NULL;
static bool s_play_exec_flag = false;
static time_t s_rec_sec = 0;
static bool s_rec_flag = false;
static int s_next_prog_num = 0;
#if RECALL_LIST_SUPPORT
static int s_recall_view_num = 1;//20130419
#endif

void app_pause_disable(void);
//void app_audio_lang_free(void);
#if MUTI_TS_SUPPORT
void app_set_muti_ts_para(int32_t tuner,int8_t ts_id,int8_t code);
#endif

extern bool app_full_check_scramble(uint32_t flag);
extern void app_set_avcodec_flag(bool state);
extern bool app_check_av_running(void);
extern void app_channel_info_update(void);
extern status_t app_volume_scope_status(void);
extern int app_get_mute_state(void);
extern status_t app_system_vol_set(GxMsgProperty_PlayerAudioVolume val);
extern void app_epg_info_full_speed(void);

/* Private functions ------------------------------------------------------ */
//#define PLAY_POS_GET()      app_play_pos_get()
//#define PLAY_POS_SET(POS)   app_play_pos_set(POS)
#define PLAY_KEY_GET()      app_play_key_get()
#define PLAY_DISC_SET()

#if TKGS_SUPPORT
#include "gxtkgs.h"
static bool app_channel_blocking_check(uint32_t prog_id)
{
    GxTkgsBlockCheckParm block_check = {prog_id, 0};

    app_send_msg_exec(GXMSG_TKGS_BLOCKING_CHECK, &block_check);
    if (block_check.resault == 1)
    {
        return true;
    }
    else
    {
        return false;
    }

}

#if 0
static bool app_parental_rating_check(uint8_t prog_rating)
{
    int parental_rating = 0;
    bool status = false;

    GxBus_ConfigGetInt(PARENTAL_RATING_KEY, &parental_rating, PARENTAL_RATING);

    if (parental_rating <= 0)
    {
        status = true;
    }
    else if (parental_rating >= 17)
    {
        status = false;
    }
    else
    {
        parental_rating -= 1;
        if (parental_rating < prog_rating)
        {
            status = true;
        }
        else
        {
            status = false;
        }
    }

    return status;
}
#endif
#endif
static int app_channel_info_get_next_prog_num(int key)
{
    int init_value = 0;

    if (key)
    {
        init_value = g_AppPlayOps.normal_play.play_count + 1;
        if (init_value >  g_AppPlayOps.normal_play.play_total - 1)
        {
            init_value = 0;
        }
    }
    else
    {
        init_value = g_AppPlayOps.normal_play.play_count - 1;
        if (init_value <  0)
        {
            init_value = g_AppPlayOps.normal_play.play_total - 1;
        }
    }

    s_next_prog_num = init_value;
    return init_value;
}

int app_channel_info_get_prog_lock_flag(void)
{
    int ret = 0;
    GxMsgProperty_NodeByPosGet node_prog = {0};
    // prog
    node_prog.node_type = NODE_PROG;
    node_prog.pos = s_next_prog_num;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
    ret = node_prog.prog_data.lock_flag;
    return ret;
}

status_t app_get_prog_pos_by_id(GxBusPmViewInfo *pm_view, uint32_t prog_id, int32_t *prog_pos)
{
    GxBusPmViewInfo cur_pm_view = {0};
    GxMsgProperty_NodeNumGet node_num_get = {0};
    GxMsgProperty_NodeByPosGet node = {0};
    uint32_t i = 0;

    if (prog_id == 0)
    {
        return GXCORE_ERROR;
    }

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&cur_pm_view));

    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(pm_view));

    node_num_get.node_type = NODE_PROG;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));

    node.node_type = NODE_PROG;
    for (i = 0; i < node_num_get.node_num; i++)
    {
        node.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

        if (node.prog_data.id == prog_id)
        {
            app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&cur_pm_view));
            *prog_pos  = i;
            return GXCORE_SUCCESS;
        }
    }

    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&cur_pm_view));

    return GXCORE_ERROR;
}

#if 0
uint32_t app_play_pos_get(void)
{
#define ALL_GROUP (0)
#define SAT_GROUP (1)
#define FAV_GROUP (2)
    GxBusPmViewInfo view_info = {0};
    GxMsgProperty_NodeByIdGet node = {0};
    int32_t pos = -1;

    // get system info
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    switch (view_info.group_mode)
    {
        case ALL_GROUP:
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                GxBus_ConfigGetInt(ALL_TV_KEY, &pos, ALL_TV_POS);
            }
            else
            {
                GxBus_ConfigGetInt(ALL_RADIO_KEY, &pos, ALL_RADIO_POS);
            }
            break;
        case SAT_GROUP:
            node.node_type = NODE_SAT;
            node.id = view_info.sat_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                pos = node.sat_data.cur_tv;
            }
            else
            {
                pos = node.sat_data.cur_radio;
            }
            break;
        case FAV_GROUP:
            node.node_type = NODE_FAV_GROUP;
            node.id = view_info.fav_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                pos = node.fav_item.cur_tv;
            }
            else
            {
                pos = node.fav_item.cur_radio;
            }
            break;
    }

    if (view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        view_info.tv_prog_cur = pos;
    }
    else
    {
        view_info.radio_prog_cur = pos;
    }
    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

    return pos;
}

uint32_t app_play_pos_set(int pos)
{
#define ALL_GROUP (0)
#define SAT_GROUP (1)
#define FAV_GROUP (2)
    GxBusPmViewInfo view_info = {0};
    GxMsgProperty_NodeByIdGet node_get = {0};
    GxMsgProperty_NodeModify node_modify = {0};
    uint32_t panel_led;

    if (g_AppPlayOps.normal_play.play_total == 0)
    {
        panel_led = 0;
    }
    else
    {
        panel_led = g_AppPlayOps.normal_play.play_count + 1;

    }
    ioctl(bsp_panel_handle, PANEL_SHOW_NUM, &panel_led);

    // get system info
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    switch (view_info.group_mode)
    {
        case ALL_GROUP:
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                GxBus_ConfigSetInt(ALL_TV_KEY, pos);
            }
            else
            {
                GxBus_ConfigSetInt(ALL_RADIO_KEY, pos);
            }
            break;

        case SAT_GROUP:
            node_get.node_type = NODE_SAT;
            node_get.id = view_info.sat_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_get);
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                node_get.sat_data.cur_tv = pos;
            }
            else
            {
                node_get.sat_data.cur_radio = pos;
            }

            node_modify.node_type = NODE_SAT;
            node_modify.sat_data = node_get.sat_data;
            app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
            break;

        case FAV_GROUP:
            node_get.node_type = NODE_FAV_GROUP;
            node_get.id = view_info.fav_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_get);
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                node_get.fav_item.cur_tv = pos;
            }
            else
            {
                node_get.fav_item.cur_radio = pos;
            }

            node_modify.node_type = NODE_FAV_GROUP;
            node_modify.fav_item = node_get.fav_item;
            app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
            break;
    }

    if (view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        view_info.tv_prog_cur = pos;
    }
    else
    {
        view_info.radio_prog_cur = pos;
    }

    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

    return 0;
}
#endif

uint32_t app_play_id_get(void)
{
#define ALL_GROUP (0)
#define SAT_GROUP (1)
#define FAV_GROUP (2)
    GxBusPmViewInfo view_info = {0};
    GxMsgProperty_NodeByIdGet node = {0};
    uint32_t id = 0;

    // get system info
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    switch (view_info.group_mode)
    {
        case ALL_GROUP:
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                GxBus_ConfigGetInt(ALL_TV_KEY, (int32_t *)&id, ALL_TV_ID);
            }
            else
            {
                GxBus_ConfigGetInt(ALL_RADIO_KEY, (int32_t *)&id, ALL_RADIO_ID);
            }
            break;

        case SAT_GROUP:
            node.node_type = NODE_SAT;
            node.id = view_info.sat_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                id = node.sat_data.cur_tv;
            }
            else
            {
                id = node.sat_data.cur_radio;
            }
            break;

        case FAV_GROUP:
            node.node_type = NODE_FAV_GROUP;
            node.id = view_info.fav_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                id = node.fav_item.cur_tv;
            }
            else
            {
                id = node.fav_item.cur_radio;
            }
            break;

        default:
            break;
    }

    if (view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        view_info.tv_prog_cur = id;
    }
    else
    {
        view_info.radio_prog_cur = id;
    }

    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

    return id;
}

uint32_t app_play_id_set(uint32_t id)
{
#define ALL_GROUP (0)
#define SAT_GROUP (1)
#define FAV_GROUP (2)
    GxBusPmViewInfo view_info = {0};
    GxMsgProperty_NodeByIdGet node_get = {0};
    GxMsgProperty_NodeModify node_modify = {0};
    /*uint32_t panel_led;

    if (g_AppPlayOps.normal_play.play_total == 0)
    {
        panel_led = 0;
    }
    else
    {
        panel_led = g_AppPlayOps.normal_play.play_count + 1;

    }
    ioctl(bsp_panel_handle, PANEL_SHOW_NUM, &panel_led);*/

    // get system info
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    switch (view_info.group_mode)
    {
        case ALL_GROUP:
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                GxBus_ConfigSetInt(ALL_TV_KEY, id);
            }
            else
            {
                GxBus_ConfigSetInt(ALL_RADIO_KEY, id);
            }
            break;

        case SAT_GROUP:
            node_get.node_type = NODE_SAT;
            node_get.id = view_info.sat_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_get);
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                node_get.sat_data.cur_tv = id;
            }
            else
            {
                node_get.sat_data.cur_radio = id;
            }

            node_modify.node_type = NODE_SAT;
            node_modify.sat_data = node_get.sat_data;
            app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
            break;

        case FAV_GROUP:
            node_get.node_type = NODE_FAV_GROUP;
            node_get.id = view_info.fav_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_get);
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                node_get.fav_item.cur_tv = id;
            }
            else
            {
                node_get.fav_item.cur_radio = id;
            }

            node_modify.node_type = NODE_FAV_GROUP;
            node_modify.fav_item = node_get.fav_item;
            app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
            break;

        default:
            break;
    }

    if (view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        view_info.tv_prog_cur = id;
    }
    else
    {
        view_info.radio_prog_cur = id;
    }
    //
    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

#if RECALL_LIST_SUPPORT
    memcpy(&(g_AppPlayOps.current_play.view_info), &view_info, sizeof(GxBusPmViewInfo));
#endif
    return 0;
}

int32_t app_play_key_get(void)
{
    int32_t lock_state = 0;

    GxBus_ConfigGetInt(CHANNEL_LOCK_KEY, &lock_state, CHANNEL_LOCK);

    return lock_state;
}

bool app_check_play_exec_flag(void)
{
    return s_play_exec_flag;
}

bool app_check_play_rec_flag(void)
{
    return s_rec_flag;
}

static void viewinfo_init(GxBusPmViewInfo  *view_info)
{
    int status = 0;

    // get system info
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)view_info);

    if (view_info->status != VIEW_INFO_ENABLE)
    {
        view_info->type = VIEW_INFO_ID;
        view_info->tv_prog_cur = 0;
        view_info->radio_prog_cur = 0;
        view_info->group_mode = GROUP_MODE_ALL;
#if 0//TKGS_SUPPORT	//2014.09.25. del TAXIS_MODE_LCN mode
        extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
        if (app_tkgs_get_operatemode() == GX_TKGS_AUTOMATIC)
        {
            view_info->taxis_mode = TAXIS_MODE_LCN;
        }
        else
        {
            view_info->taxis_mode = TAXIS_MODE_NON;
        }
#else
        view_info->taxis_mode = TAXIS_MODE_NON;
#endif
        view_info->stream_type = GXBUS_PM_PROG_TV;
        view_info->fav_id =  0; // GROUP_MODE_ALL needn't care it
        view_info->sat_id = 0; // GROUP_MODE_ALL needn't care it
        view_info->cas_id = 0; //TAXIS_MODE_NON needn't care it
        view_info->tp_id = 0; //TAXIS_MODE_NON needn't care it
        view_info->tuner_num = 0; //TAXIS_MODE_NON needn't care it
        view_info->pip_switch = 0; // full screen program
        view_info->pip_main_tp_id = 0; // cur tp id
        memset(view_info->letter, 0, MAX_CMP_NAME);  // TAXIS_MODE_NON needn't care it
        view_info->reserved1 = 0;
        view_info->status = VIEW_INFO_ENABLE;
        status = 1;
    }

    if (view_info->skip_view_switch == VIEW_INFO_SKIP_CONTAIN)
    {
        view_info->skip_view_switch = VIEW_INFO_SKIP_VANISH;
    }
    // set system info
    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)view_info);
#if TKGS_SUPPORT
    if (status == 1)
    {
        app_tkg_lcn_flush();
        app_tkgs_sort_channels_by_lcn();//2014.09.25 add
        status = 0;
    }
#endif
}

/**
 * @brief when finish the list operation, rebuild the play list
 *         !!! will change g_AppPlayOps.normal_play.view_info !!!
 * @param play mode
 * @Return no program return GXCORE_ERROR
 *          else GXCORE_SUCCESS
 */
static status_t app_play_list_create(GxBusPmViewInfo *p_view_info)
{
    GxMsgProperty_NodeNumGet node_num_get = {0};
    GxBusPmViewInfo view_info_temp = {0};
    uint32_t panel_led;
    uint32_t prog_id;

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info_temp));

    if (p_view_info != &(g_AppPlayOps.normal_play.view_info))
    {
        memcpy(&(g_AppPlayOps.normal_play.view_info), p_view_info, sizeof(GxBusPmViewInfo));
    }

    // new list create
    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *) & (g_AppPlayOps.normal_play.view_info));

    node_num_get.node_type = NODE_PROG;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));

    if ((node_num_get.node_num == 0)
            && (g_AppPlayOps.normal_play.view_info.group_mode != GROUP_MODE_ALL)
            && (g_AppPlayOps.normal_play.view_info.skip_view_switch != view_info_temp.skip_view_switch))
    {
        p_view_info->group_mode = GROUP_MODE_ALL;
        g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
        app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *) & (g_AppPlayOps.normal_play.view_info));
        node_num_get.node_type = NODE_PROG;
        app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
    }

    if (node_num_get.node_num == 0)
    {
        g_AppPlayOps.normal_play.play_total = 0;
        g_AppPlayOps.normal_play.play_count = 0;
        panel_led = 0;
        ioctl(bsp_panel_handle, PANEL_SHOW_NUM, &panel_led);
        return GXCORE_ERROR;
    }

    g_AppPlayOps.normal_play.play_total = node_num_get.node_num;
    prog_id = app_play_id_get();
    if (app_get_prog_pos_by_id(&g_AppPlayOps.normal_play.view_info, prog_id, &g_AppPlayOps.normal_play.play_count) == GXCORE_ERROR)
    {
        g_AppPlayOps.normal_play.play_count = 0;
    }

    // error happend, play No. larger than the total number
    if (g_AppPlayOps.normal_play.play_count >= g_AppPlayOps.normal_play.play_total)
    {
        printf("[play] play count larger than total!!!!!\n");
        g_AppPlayOps.normal_play.play_count = 0;
        panel_led = 0;
    }
    else
    {
        panel_led = g_AppPlayOps.normal_play.play_count + 1;
    }

    ioctl(bsp_panel_handle, PANEL_SHOW_NUM, &panel_led);
    return GXCORE_SUCCESS;
}

static void app_program_play_init(void)
{
    GxBusPmViewInfo view_info = {0};
    //add for 1211 /1801	by dugws
    AppFrontend_Config cfg = {0};
    app_ioctl(0, FRONTEND_CONFIG_GET, &cfg);

    g_AppPlayOps.normal_play.ts_src = cfg.ts_src; // multi-ts need this TS1-0 TS2-1 TS3-2
    //add for 1211 /1801	end

    viewinfo_init(&view_info);
    app_play_list_create(&view_info);

    // set recall_play params
#if RECALL_LIST_SUPPORT
    //g_AppPlayOps.add_recall_list(&(g_AppPlayOps.normal_play));
    //g_AppPlayOps.recall_play[s_recall_view_num] = g_AppPlayOps.normal_play;//--20130703--
#else
    g_AppPlayOps.recall_play = g_AppPlayOps.normal_play;
#endif

}

static void app_play_window_set(AppPlayMode play_mode, PlayerWindow *rect)
{
    GxMsgProperty_PlayerVideoWindow play_wnd;

    memcpy(&s_NormalRect, rect, sizeof(PlayerWindow));
    play_wnd.player = PLAYER_FOR_NORMAL;
    memcpy(&(play_wnd.rect), rect, sizeof(PlayerWindow));

    app_send_msg_exec(GXMSG_PLAYER_VIDEO_WINDOW, (void *)(&play_wnd));
}

void app_play_set_diseqc(uint32_t sat_id, uint16_t tp_id, bool ForceFlag)
{
    AppFrontend_DiseqcParameters diseqc10 = {0};
    AppFrontend_DiseqcParameters diseqc11 = {0};
    GxMsgProperty_NodeNumGet tp_num = {0};
    GxMsgProperty_NodeByIdGet node_sat = {0};
    GxMsgProperty_NodeByIdGet node_tp = {0};
    bool diseqc_force = ForceFlag;

    diseqc10.type = DiSEQC10;
    node_sat.node_type = NODE_SAT;
    node_sat.id = sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat);
    //#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
#if (DEMOD_DVB_COMBO > 0)
    app_set_tune_mode(node_sat.sat_data.type);
#endif
    diseqc10.u.params_10.chDiseqc = node_sat.sat_data.sat_s.diseqc10;
    diseqc10.u.params_10.bPolar = app_show_to_lnb_polar(&node_sat.sat_data);

    node_tp.node_type = NODE_TP;
    node_tp.id = tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

    if (diseqc10.u.params_10.bPolar == SEC_VOLTAGE_ALL)
    {
        tp_num.node_type = NODE_TP;
        tp_num.sat_id = sat_id;
        app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &tp_num);
        if (tp_num.node_num == 0)
        {
            diseqc10.u.params_10.bPolar = SEC_VOLTAGE_18;
        }
        else
        {
            diseqc10.u.params_10.bPolar = app_show_to_tp_polar(&node_tp.tp_data);
        }
    }
    diseqc10.u.params_10.b22k = app_show_to_sat_22k(node_sat.sat_data.sat_s.switch_22K, &node_tp.tp_data);

    diseqc11.u.params_11.chUncom = node_sat.sat_data.sat_s.diseqc11;

    diseqc11.type = DiSEQC11;
    diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc ;
    diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
    diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;

    // for polar
    //GxFrontend_SetVoltage(sat_node.sat_data.tuner, polar);
    AppFrontend_PolarState Polar = diseqc10.u.params_10.bPolar ;
    app_ioctl(node_sat.sat_data.tuner, FRONTEND_POLAR_SET, (void *)&Polar);

    // for 22k
    //GxFrontend_Set22K(sat_node.sat_data.tuner, sat22k);
    AppFrontend_22KState _22K = diseqc10.u.params_10.b22k;
    app_ioctl(node_sat.sat_data.tuner, FRONTEND_22K_SET, (void *)&_22K);

    app_set_diseqc_10(node_sat.sat_data.tuner, &diseqc10, diseqc_force);
    app_set_diseqc_11(node_sat.sat_data.tuner, &diseqc11, diseqc_force);

    if ((node_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
    {
        app_diseqc12_goto_position(node_sat.sat_data.tuner, node_sat.sat_data.sat_s.diseqc12_pos, diseqc_force);
    }
    else if ((node_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
    {
        LongitudeInfo local_longitude = {0};
        local_longitude.longitude = node_sat.sat_data.sat_s.longitude;
        local_longitude.longitude_direct = node_sat.sat_data.sat_s.longitude_direct;
        app_usals_goto_position(node_sat.sat_data.tuner, &local_longitude, NULL, diseqc_force);
    }

    // POST SEM
    GxFrontendOccur(node_sat.sat_data.tuner);
}

void app_play_set_tp(GxMsgProperty_NodeByIdGet sat_get, 
							GxMsgProperty_NodeByIdGet tp_get,
							GxMsgProperty_NodeByPosGet prog_get)
{
    AppFrontend_SetTp params = {0};

    switch (sat_get.sat_data.type)
    {
#if (DEMOD_DVB_S > 0)
        case GXBUS_PM_SAT_S:
        {
            params.type = FRONTEND_DVB_S;
            params.fre = app_show_to_tp_freq(&sat_get.sat_data, &tp_get.tp_data);
            params.symb = tp_get.tp_data.tp_s.symbol_rate;

            params.polar = app_show_to_lnb_polar(&sat_get.sat_data);
            if (params.polar == SEC_VOLTAGE_ALL)
            {
                params.polar = app_show_to_tp_polar(&tp_get.tp_data);
            }
            params.sat22k = app_show_to_sat_22k(sat_get.sat_data.sat_s.switch_22K, &tp_get.tp_data);
            params.tuner = sat_get.sat_data.tuner;

            // add in 20121012
            // for polar
            AppFrontend_PolarState Polar = params.polar ;
            app_ioctl(sat_get.sat_data.tuner, FRONTEND_POLAR_SET, (void *)&Polar);

            // for 22k
            AppFrontend_22KState _22K =  params.sat22k;
            app_ioctl(sat_get.sat_data.tuner, FRONTEND_22K_SET, (void *)&_22K);
#if UNICABLE_SUPPORT
            {
                uint32_t centre_frequency = 0 ;
                uint32_t lnb_index = 0 ;
                uint32_t set_tp_req = 0;
                if (sat_get.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
                {
                    lnb_index = app_calculate_set_freq_and_initfreq(&sat_get.sat_data, &tp_get.tp_data, &set_tp_req, &centre_frequency);
                    params.fre  = set_tp_req;
                    params.lnb_index  = lnb_index;
                }
                else
                {
                    params.lnb_index  = 0x20;
                }
                params.centre_fre  = centre_frequency;
                params.if_channel_index  = sat_get.sat_data.sat_s.unicable_para.if_channel;
                params.if_fre  = sat_get.sat_data.sat_s.unicable_para.centre_fre[params.if_channel_index];
            }
#endif
#if MUTI_TS_SUPPORT
				params.pls_chanage = prog_get.prog_data.ts_pls_chanage;
				params.pls_code = prog_get.prog_data.ts_pls_code;
				params.pls_ts_id = prog_get.prog_data.muti_ts_id;
				params.pls_ts_status = prog_get.prog_data.muti_ts_status;
#endif
            break;
        }
#endif
#if (DEMOD_DVB_C > 0)
        case GXBUS_PM_SAT_C:
        {
            params.fre = tp_get.tp_data.frequency;
            params.symb = tp_get.tp_data.tp_c.symbol_rate;
            params.qam = tp_get.tp_data.tp_c.modulation;
            params.type = FRONTEND_DVB_C;
            break;
        }
#endif
#if (DEMOD_DVB_T > 0)
        case GXBUS_PM_SAT_T:
        case GXBUS_PM_SAT_DVBT2:
        {
            params.fre = tp_get.tp_data.frequency;
            params.bandwidth = tp_get.tp_data.tp_dvbt2.bandwidth;
            params.type = FRONTEND_DVB_T2;
            params.type_1501 = DVBT_AUTO_MODE;//PLP Data not in tp note,should be auto
            break;
        }
#endif
#if (DEMOD_DTMB > 0)
        case GXBUS_PM_SAT_DTMB:
            if (GXBUS_PM_SAT_1501_DTMB == sat_get.sat_data.sat_dtmb.work_mode)
            {
                params.fre = tp_get.tp_data.frequency;
                params.qam = tp_get.tp_data.tp_dtmb.symbol_rate;
                params.type = FRONTEND_DTMB;
                params.type_1501 = DTMB;
            }
            else
            {
                // DVBC mode
                params.fre = tp_get.tp_data.frequency;
                params.symb = tp_get.tp_data.tp_dtmb.symbol_rate;
                params.qam = tp_get.tp_data.tp_dtmb.modulation;
                params.type_1501 = DTMB_C;
            }
            break;
#endif

        default:
            break;
    }

    app_set_tp(sat_get.sat_data.tuner, &params, SET_TP_AUTO);
}


static void app_normal_play(GxMsgProperty_NodeByPosGet *prog_node)
{
    GxMsgProperty_GetUrl url_get ;

    GxMsgProperty_PlayerPlay play = {0};
    AppFrontend_Config config;
#ifdef UNICABLE_SUPPORT
    char ts_dmx[100] = {0};
#else
    char ts_dmx[24] = {0};
#endif

    play.player = PLAYER_FOR_NORMAL;
    memset(play.url, 0, PLAYER_URL_LONG);

    /*************************************************************/
// TODO: 20160302 FOR CA
    /*************************************************************/

    // get program url
    url_get.prog_info = prog_node->prog_data;
    url_get.url = (int8_t *)(play.url);
    url_get.size = PLAYER_URL_LONG;
    app_send_msg_exec(GXMSG_PM_NODE_GET_URL, (void *)(&url_get));

    /*************************************************************
     * get the play program's ts_src, set to normal_play here,
     *  multi-tuner ts control entry  */
    app_ioctl(prog_node->prog_data.tuner, FRONTEND_CONFIG_GET, &config);
    g_AppPlayOps.normal_play.tuner = config.tuner;
    g_AppPlayOps.normal_play.ts_src = config.ts_src;
    g_AppPlayOps.normal_play.dmx_id = config.dmx_id;
    /************************************************************/

    // tell player ts_src & dmx_id & prog_pos
    sprintf(ts_dmx, "&tsid:%d&dmxid:%d&progid:%d", config.ts_src, config.dmx_id, prog_node->prog_data.id);
    strcat(play.url, ts_dmx);
#if CA_SUPPORT
    app_descrambler_close_by_control_type(CONTROL_NORMAL);

    memset(ts_dmx, 0, sizeof(ts_dmx));
    {
        int time = app_tsd_time_get();
        if((time > 0)
                && ((((prog_node->prog_data.cas_id) & 0xf00) == 0x900) || (((prog_node->prog_data.cas_id) & 0xf00) == 0xe00)))
        {
            sprintf(ts_dmx, "&tsdelay:%d", time);
            strcat(play.url, ts_dmx);
        }
    }
#endif
#if 0//def AUDIO_DESCRIPTOR_SUPPORT
    {
        int32_t ad_mode = 1;
        int ad_type = 0;
        int ad_pid = 0;

        GxBus_ConfigGetInt(AD_CONF_KEY, &ad_mode, AD_CONFIG);
        if (ad_mode)
        {
            app_get_ad_pid_and_type(&ad_type, &ad_pid);
            memset(ts_dmx, 0, sizeof(ts_dmx));
            sprintf(ts_dmx, "&adpid:%d&adcodec:%d", ad_pid, ad_type);
            //sprintf(ts_dmx, "&adpid:%d",ad_mode);
            strcat(play.url, ts_dmx);
        }
    }
#endif

#if UNICABLE_SUPPORT
    {
        uint32_t centre_frequency = 0 ;
        uint32_t lnb_index = 0x20 ;
        uint32_t set_tp_req = 0;
        GxMsgProperty_NodeByIdGet sat_get = {0};
        GxMsgProperty_NodeByIdGet tp_get = {0};
        sat_get.node_type = NODE_SAT;
        sat_get.id = prog_node->prog_data.sat_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_get);

        tp_get.node_type = NODE_TP;
        tp_get.id = prog_node->prog_data.tp_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_get);
        if (sat_get.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10)
        {

            lnb_index = app_calculate_set_freq_and_initfreq(&sat_get.sat_data, &tp_get.tp_data, &set_tp_req, &centre_frequency);
            sprintf(ts_dmx, "&ifindex:%d&lnbindex:%d&centrefre:%d&iffre:%d", sat_get.sat_data.sat_s.unicable_para.if_channel, \
                    lnb_index, centre_frequency, sat_get.sat_data.sat_s.unicable_para.centre_fre[sat_get.sat_data.sat_s.unicable_para.if_channel]);
            strcat(play.url, ts_dmx);
            //printf("[%s]%s\n",__FUNCTION__,ts_dmx);
        }
    }
#endif


#if MUTI_TS_SUPPORT
	{
		if(prog_node->prog_data.muti_ts_status)
		{
			sprintf(ts_dmx, 
					"&pls_ts_id:%d&pls_code:%d&pls_change:%d&pls_status:%d", 
					prog_node->prog_data.muti_ts_id, 
					prog_node->prog_data.ts_pls_code,
					prog_node->prog_data.ts_pls_chanage,
					prog_node->prog_data.muti_ts_status);
			strcat(play.url, ts_dmx);
			printf("-------[%s]%s\n",__FUNCTION__,ts_dmx);
		}
	}
#endif


    /* url = dvbs://fre:1342&polar:1&symbol:8800&22k:0&vpid:6496&apid:6499&pcrpid:6496
    &vcodec:1&acodec:0&tuner:0&scramble:0&pmt:5001&tsid:0&dmxid:0&progid:40*/

    printf("~~~~~~~~~~~~~~~~~~~~[%s] %s, %d, url = %s~~~~~~~~~~~~~~~~~~~~~\n", __FILE__, __func__, __LINE__, play.url);

    // record player window
    memcpy(&play.window, &s_NormalRect, sizeof(PlayerWindow));
#ifdef MENCENT_FREEE_SPACE
    if ((GXBUS_PM_PROG_TV == prog_node->prog_data.service_type)
            && ((GUI_CheckDialog("wnd_channel_edit") != GXCORE_SUCCESS)
                && (GUI_CheckDialog("wnd_epg") != GXCORE_SUCCESS)
#if MINI_16BIT_WIN8_OSD_SUPPORT
                && (GUI_CheckDialog("wnd_main_menu") != GXCORE_SUCCESS)
#endif
               ))//g_AppPlayOps.normal_play.view_info.stream_type
    {
        GUI_SetInterface("free_space", "fragment|spp");
        // GxCore_HwCleanCache();
    }
    else
    {
        GUI_SetInterface("free_space", "fragment");
    }
    GxCore_HwCleanCache();
#endif
    // send message to player
    //app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)(&play));
    if (GUI_CheckDialog("wnd_timer_setting") != GXCORE_SUCCESS)
    {
        GxPlayer_MediaPlay(play.player, play.url, 0, 0, NULL);//&play.window
		if(g_AppFullArb.state.tv == STATE_ON &&g_AppFullArb.tip == FULL_STATE_DUMMY)//错误 #43832
        {
			if(0 == strcasecmp("wnd_epg", GUI_GetFocusWindow()))
			{
				GUI_SetProperty("txt_epg_signal", "state", "osd_trans_hide");//错误 #44317
			}
		}
        int32_t audio_vol = 0;
        if (app_volume_scope_status())
        {
            if (app_get_mute_state() == 0)
            {
                audio_vol = prog_node->prog_data.audio_volume;
                app_system_vol_set(audio_vol);
            }
        }
        else
        {
            if (app_get_mute_state() == 0)
            {
                GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_vol, AUDIO_VOLUME);
                app_system_vol_set(audio_vol);
            }
        }
    }
}


static void app_prepare_for_play(GxMsgProperty_NodeByPosGet *prog_node) /* TODO: some must do befor play */
{
    static uint16_t prog_tp_id = 0;
    AppFrontend_Config config;
    GxMsgProperty_NodeByIdGet SatNode = {0};
    GxMsgProperty_NodeByIdGet TpNode = {0};
    GxMsgProperty_FrontendSetTp Param = {0};
    fe_sec_voltage_t polar;
    int32_t fre = 0;
    int32_t symb = 0;
    int32_t tempfre = 0;
    //int32_t audio_vol = 0;
    int freeze = 1;

    {                                                                         
        extern void app_set_descrambler_delay_flag(unsigned char flag);       
        extern void app_descrambler_delay_timer_destroy(void);                

        app_descrambler_delay_timer_destroy();                                
        app_set_descrambler_delay_flag(0);                                    
        app_set_avcodec_flag(false);
    }           

    g_AppFullArb.scramble_set(1);

    SatNode.node_type = NODE_SAT;
    SatNode.id = prog_node->prog_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &SatNode);

    TpNode.node_type = NODE_TP;
    TpNode.id = prog_node->prog_data.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &TpNode);


#if NDEMOD_WITH_1TUNER
    uint32_t i = 0;
    AppFrontend_Monitor monitor = FRONTEND_MONITOR_ON;

    for (i = 0; i < NIM_MODULE_NUM; i++)
    {
        if (SatNode.sat_data.tuner  == i)
        {
            monitor = FRONTEND_MONITOR_ON;
            printf("[%s]>> Start tuner%d monitor!\n", __func__, i);
        }
        else
        {
            monitor = FRONTEND_MONITOR_OFF;
            printf("[%s]>> Stop tuner%d monitor!\n", __func__, i);
        }

        app_ioctl(i, FRONTEND_MONITOR_SET, &monitor);
    }

    if (SatNode.sat_data.type == GXBUS_PM_SAT_DTMB)
    {
        // lnb polar off
        AppFrontend_PolarState polar = FRONTEND_POLAR_OFF;
        GxFrontend_SetVoltage(0, polar);
    }
#if (DEMOD_DTMB > 0)
    if (SatNode.sat_data.type == GXBUS_PM_SAT_DTMB)
    {
        fre = TpNode.tp_data.frequency;
        symb = 0;
        polar = TpNode.tp_data.tp_dtmb.modulation;

        GxFrontend_GetCurFre(prog_node->prog_data.tuner, &Param);

    }
    else if (SatNode.sat_data.type == GXBUS_PM_SAT_S)
#endif
#endif
    {
        fre = app_show_to_tp_freq(&SatNode.sat_data, &TpNode.tp_data);
        symb = TpNode.tp_data.tp_s.symbol_rate;
        polar = app_show_to_lnb_polar(&SatNode.sat_data);
        if (polar == SEC_VOLTAGE_ALL)
        {
            polar = app_show_to_tp_polar(&TpNode.tp_data);
        }

        GxFrontend_GetCurFre(prog_node->prog_data.tuner, &Param);
    }

    //printf("app---->fre %d=%d  polar  %d=%d symb %d=%d\n",Param.fre,fre,Param.polar,polar,Param.symb,symb);
#if MUTI_TS_SUPPORT
	if(prog_node->prog_data.muti_ts_status)
	{
		app_set_muti_ts_para(SatNode.sat_data.tuner,prog_node->prog_data.muti_ts_id, prog_node->prog_data.ts_pls_code);
	}
#endif

#if (DEMOD_DTMB > 0)
    extern bool app_full_screen_is_menu_back_get(void);
    extern void app_full_screen_is_menu_back_set(bool in);
#endif
#if UNICABLE_SUPPORT
	if(SatNode.sat_data.sat_s.unicable_para.lnb_fre_index < 0x10&& Param.centre_fre>0)		
		tempfre = Param.centre_fre;
	else
#endif 
		tempfre = Param.fre;
    if((prog_node->prog_data.tp_id != prog_tp_id)
        || (/*Param.fre*/tempfre != fre)           
            || (Param.polar != polar)
            || (Param.symb != symb)
#if (DEMOD_DTMB > 0)
            || (app_full_screen_is_menu_back_get())
#endif
       )
    {
#ifndef MULTI_TP_EPG_INFO
        app_epg_info_clean(); //20141028
#endif
        prog_tp_id = prog_node->prog_data.tp_id;
        GxFrontend_ForceExitEx(prog_node->prog_data.tuner);
        GxFrontend_SetUnlock(prog_node->prog_data.tuner);// for TS disable output
        GxFrontend_CleanRecordPara(prog_node->prog_data.tuner);
        //GxFrontend_CleanRecord(prog_node->prog_data.tuner);
#if TKGS_SUPPORT
        g_AppTkgsBackUpgrade.init(prog_node->prog_data.sat_id, prog_tp_id, prog_node->prog_data.tuner);
#endif
#if (DEMOD_DTMB > 0)
        if (app_full_screen_is_menu_back_get())
        {
            app_full_screen_is_menu_back_set(false);
        }
#endif
#if CASCAM_SUPPORT
{
       CASCAM_Change_Frequence(); 
}
#endif

    }
#if CASCAM_SUPPORT
    CASCAM_Osd_Clean();
#endif

#if CA_SUPPORT
    if (prog_node->prog_data.scramble_flag)
    {
        //TODO:
        extern int app_send_prepare(ServiceClass *param);
        ServiceClass param = {0};
        unsigned int casid[1];

        casid[0] = prog_node->prog_data.cas_id;
        param.cas_id = casid[0];
        app_send_prepare(&param);

        if(((prog_node->prog_data.cas_id) & 0xf00) == 0x900)
        {
            app_ca_set_flag(0);
        }
        if((((prog_node->prog_data.cas_id) & 0xf00) == 0xe00))
        {
            app_ca_set_flag(1);
        }
        app_ca_set_flag(2);

    }
    else
    {
        app_ca_clear_flag();
    }
#endif

	app_epg_info_full_speed();
    // add in 20121126 for double tuner monitor control
    app_frontend_demod_control(prog_node->prog_data.tuner, FALSE);

    app_play_set_diseqc(prog_node->prog_data.sat_id, prog_node->prog_data.tp_id, false);

    // for epg control
    app_ioctl(prog_node->prog_data.tuner, FRONTEND_CONFIG_GET, &config);

    //set tp if necessary
    app_play_set_tp(SatNode, TpNode, *prog_node);
    // do not enable epg filter when PVR is open
    if (PVR_DUMMY == g_AppPvrOps.state)
    {
        app_epg_enable(config.ts_src, TS_2_DMX);
    }
    {
        int track_sel = 0;
        if (prog_node->prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_MONO)
        {
            track_sel = 0;
        }
        else if (prog_node->prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_LEFT)
        {
            track_sel = 1;
        }
        else if (prog_node->prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_RIGHT)
        {
            track_sel = 2;
        }
        else
        {
            track_sel = 3;
        }
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_TRACK, &track_sel);
    }

    GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);

}

static int thiz_rec_delay_cnt = 0;
static int _play_rec_cb(void *usrdata)
{
#define RECORD_WAIT_COUNT      9 // time = 500ms*(7+1)=4000ms
	usb_check_state usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
    if ((app_check_av_running() == true) && (0xff != g_AppPmt.ver) && (usb_state == USB_OK))
    {
        if (s_play_rec_timer != NULL)
        {
            remove_timer(s_play_rec_timer);
            s_play_rec_timer = NULL;
        }
        thiz_rec_delay_cnt = 0;
        s_rec_flag = false;
        if (s_rec_sec == 0x0f)
        {
            g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        }
        else
        {
            app_pvr_rec_start(s_rec_sec);
        }
        if (GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
        {
            GUI_EndDialog("wnd_channel_info");
        }
		app_create_dialog("wnd_pvr_bar");
        GUI_SetInterface("flush", NULL);
    }
    else
    {
        if (thiz_rec_delay_cnt++ >= RECORD_WAIT_COUNT)
        {
            if (s_play_rec_timer != NULL)
            {
                remove_timer(s_play_rec_timer);
                s_play_rec_timer = NULL;
            }
            thiz_rec_delay_cnt = 0;
            s_rec_flag = false;

            PopDlg pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_CANT_PVR;
            pop.mode = POP_MODE_UNBLOCK;
            pop.creat_cb = NULL;
            pop.exit_cb = NULL;
            pop.timeout_sec = 3;
            popdlg_create(&pop);
        }
    }
    return 0;
}

static int _clear_play_flag_cb(void *usrdata)
{
    if (s_clear_paly_flag_timer != NULL)
    {
        remove_timer(s_clear_paly_flag_timer);
        s_clear_paly_flag_timer = NULL;
    }
    s_play_exec_flag = false;
    g_AppFullArb.timer_start();

    return 0;
}

static int _play_cb(void *usrdata)
{
    GxMsgProperty_NodeByPosGet node = {0};
    AppPlayKey key_status = PLAY_KEY_LOCK;

    g_AppFullArb.timer_stop();
    app_subt_pause();
    g_AppTtxSubt.ttx_num = 0;
    g_AppTtxSubt.subt_num = 0;
    g_AppTtxSubt.sync_flag = 0;
    g_AppPmt.stop(&g_AppPmt);

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

    app_prepare_for_play(&node);//something must to do befor play

    if (s_play_delay_timer != NULL)
    {
        remove_timer(s_play_delay_timer);
        s_play_delay_timer = NULL;
    }
    key_status = g_AppPlayOps.normal_play.rec ;
    g_AppPlayOps.normal_play.key = PLAY_KEY_DUMMY;
    g_AppPlayOps.normal_play.rec = PLAY_KEY_DUMMY;
	
    if ((PLAY_KEY_GET()
            && (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
            && (node.prog_data.id != g_AppPvrOps.env.prog_id)
            && (!(s_rec_flag == true && key_status == PLAY_KEY_UNLOCK)))
       )
    {
        // stop player then pop up passwd
        app_set_avcodec_flag(false);
        app_player_close(PLAYER_FOR_NORMAL);

        g_AppFullArb.tip = FULL_STATE_LOCKED; /* embeded, no way out */
        g_AppPlayOps.normal_play.key = PLAY_KEY_LOCK;
        g_AppPlayOps.normal_play.rec = PLAY_KEY_LOCK;
        g_AppPlayOps.poppwd_create(g_AppPlayOps.normal_play.play_count);
    }
#if TKGS_SUPPORT
    else if ((app_channel_blocking_check(node.prog_data.id))
             && (node.prog_data.id != g_AppPvrOps.env.prog_id))
    {
        app_set_avcodec_flag(false);
        app_player_close(PLAYER_FOR_NORMAL);

        g_AppFullArb.tip = FULL_STATE_SCRAMBLE; /* embeded, no way out */
        g_AppPlayOps.normal_play.key = PLAY_KEY_LOCK;
        g_AppPlayOps.normal_play.rec = PLAY_KEY_LOCK;
    }
#endif
    else if (g_AppFullArb.scramble_check() == FALSE)
    {
        g_AppFullArb.tip = FULL_STATE_DUMMY;
    }
    else
    {
        g_AppFullArb.tip = FULL_STATE_SCRAMBLE;
    }

    //	g_AppFullArb.exec[EVENT_STATE](&g_AppFullArb);
    g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);

    if (g_AppPlayOps.normal_play.key == PLAY_KEY_DUMMY)
    {
        app_normal_play(&node);
        if (s_rec_flag == true)
        {
            if (0 != reset_timer(s_play_rec_timer))
            {
                s_play_rec_timer = create_timer(_play_rec_cb, 300, NULL, TIMER_REPEAT);
            }
        }
    }

	app_play_id_set(node.prog_data.id);
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&g_AppPlayOps.normal_play.view_info));
	if(g_AppPlayOps.normal_play.play_count != g_AppPlayOps.current_play.play_count || 
			g_AppPlayOps.current_play.view_info.stream_type != g_AppPlayOps.normal_play.view_info.stream_type)
	{
#if RECALL_LIST_SUPPORT
    g_AppPlayOps.current_play.play_count = g_AppPlayOps.normal_play.play_count;//---[2013-07-03]-
    g_AppPlayOps.add_recall_list(&(g_AppPlayOps.current_play));
#else
    g_AppPlayOps.recall_play = g_AppPlayOps.current_play;
#endif
	}
    g_AppPlayOps.current_play = g_AppPlayOps.normal_play;
    if (0 != reset_timer(s_clear_paly_flag_timer))
    {
        s_clear_paly_flag_timer = create_timer(_clear_play_flag_cb, 400, NULL, TIMER_REPEAT);
    }
    return 0;
}
static void app_program_play_exec(AppPlayMode play_mode, uint32_t play_count)
{
    uint32_t panel_led = 0;
    if (0 >= g_AppPlayOps.normal_play.play_total)
    {
        ioctl(bsp_panel_handle, PANEL_SHOW_NUM, &panel_led);
        return;
    }

    if ((play_mode & PLAY_TYPE_FORCE) == PLAY_TYPE_FORCE)
    {
        s_play_exec_flag = false;
    }

    if ((play_mode & PLAY_MODE_NEXT) == PLAY_MODE_NEXT)
    {
        play_count = g_AppPlayOps.normal_play.play_count;
        if (play_count >=  g_AppPlayOps.normal_play.play_total - 1)
        {
            play_count = 0;
        }
        else
        {
            play_count++;
        }
        app_channel_info_get_next_prog_num(1);
    }
    else if ((play_mode & PLAY_MODE_PREV) == PLAY_MODE_PREV)
    {
        play_count = g_AppPlayOps.normal_play.play_count;
        if (play_count ==  0)
        {
            play_count = g_AppPlayOps.normal_play.play_total - 1;
        }
        else
        {
            play_count--;
        }
        app_channel_info_get_next_prog_num(0);
    }
    else if ((play_mode & PLAY_MODE_POINT) == PLAY_MODE_POINT)
    {
        if (play_count >= g_AppPlayOps.normal_play.play_total)
        {
            play_count = 0;
        }
    }

    if (((play_mode & PLAY_MODE_WITH_REC) == PLAY_MODE_WITH_REC)
            || ((play_mode & PLAY_MODE_WITH_TMS) == PLAY_MODE_WITH_TMS))
    {
        GxMsgProperty_NodeByPosGet node = {0};

        s_rec_flag = true;

        node.node_type = NODE_PROG;
        node.pos = play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

        if((app_check_av_running() == true)
                && ( g_AppPlayOps.current_play.view_info.tv_prog_cur == node.prog_data.id)
                && (g_AppPlayOps.current_play.view_info.stream_type == g_AppPlayOps.normal_play.view_info.stream_type))
        {
            //same prog and it's running
            thiz_rec_delay_cnt = 0;
            if(0 != reset_timer(s_play_rec_timer))
            {
                s_play_rec_timer = create_timer(_play_rec_cb, 300, NULL, TIMER_REPEAT);
            }
            return;
        }

        s_play_exec_flag = false;
    }
    else
    {
        s_rec_flag = false;
    }

    if (s_clear_paly_flag_timer != NULL)
    {
        remove_timer(s_clear_paly_flag_timer);
        s_clear_paly_flag_timer = NULL;
    }

    g_AppPlayOps.normal_play.play_count = play_count;
    g_AppFullArb.state.rec = STATE_OFF;
    //	g_AppFullArb.exec[EVENT_STATE](&g_AppFullArb);
    //	g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
    const char *wnd = GUI_GetFocusWindow();
    if (strcmp(wnd, "wnd_full_screen") == 0 &&  s_rec_flag != true)
    {
		app_create_dialog("wnd_channel_info");
    }
    else if (strcmp(wnd, "wnd_channel_info") == 0)
    {
        app_channel_info_update();
    }

    panel_led = g_AppPlayOps.normal_play.play_count + 1;
    ioctl(bsp_panel_handle, PANEL_SHOW_NUM, &panel_led);

    if (s_play_exec_flag == false)
    {
        thiz_rec_delay_cnt = 0;
        if (s_play_rec_timer != NULL)
        {
            remove_timer(s_play_rec_timer);
            s_play_rec_timer = NULL;
        }
        s_play_exec_flag = true;
#if 0
        g_AppFullArb.timer_stop();
        app_subt_pause();
        g_AppTtxSubt.ttx_num = 0;
        g_AppTtxSubt.subt_num = 0;
        g_AppTtxSubt.sync_flag = 0;
        g_AppPmt.stop(&g_AppPmt);
#endif
        _play_cb(NULL);
    }
    else
    {

        //        g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
        if (0 != reset_timer(s_play_delay_timer))
        {
            s_play_delay_timer = create_timer(_play_cb, PLAY_DELAY_MS, NULL, TIMER_REPEAT);
        }
    }
}

static void app_program_play(AppPlayMode play_mode, int32_t play_count)
{
#if TKGS_SUPPORT
    extern bool app_get_tkgs_power_status(void);
    if (app_get_tkgs_power_status() == true)
    {
        return;
    }
#endif
    if (g_AppPlayOps.normal_play.key == PLAY_KEY_UNLOCK)
    {
        GxMsgProperty_NodeByPosGet node = {0};

        // for forbiden rec lock program
        g_AppPlayOps.normal_play.play_count = play_count;
        g_AppPlayOps.normal_play.rec = PLAY_KEY_UNLOCK;
        g_AppFullArb.tip = FULL_STATE_DUMMY; /* embeded, no way out */

        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
        app_normal_play(&node);
        if (s_rec_flag == true)
        {
            thiz_rec_delay_cnt = 0;
            if (0 != reset_timer(s_play_rec_timer))
            {
                s_play_rec_timer = create_timer(_play_rec_cb, 300, NULL, TIMER_REPEAT);
            }
        }

        g_AppPlayOps.normal_play.key = PLAY_KEY_LOCK;
    }
    else
    {
        app_program_play_exec(play_mode, play_count);
    }
}

static void app_program_stop(void)
{
    if (s_play_delay_timer != NULL)
    {
        remove_timer(s_play_delay_timer);
        s_play_delay_timer = NULL;
    }
    if (s_play_rec_timer != NULL)
    {
        remove_timer(s_play_rec_timer);
        s_play_rec_timer = NULL;
    }
    s_rec_flag = false;
    app_set_avcodec_flag(false);
    g_AppPmt.stop(&g_AppPmt);
    app_subt_pause();
    app_epg_disable();
    app_player_close(PLAYER_FOR_NORMAL);
#if CASCAM_SUPPORT
    CASCAM_Osd_Clean();
#endif
}

static int app_play_pwd_cb(PopPwdRet *ret, void *usr_data)
{
	if(ret->result == PASSWD_OK)
	{
		GxMsgProperty_PlayerVideoWindow play_wnd;
		play_wnd.player = PLAYER_FOR_NORMAL;
		memcpy(&(play_wnd.rect), &s_NormalRect, sizeof(PlayerWindow));
		app_send_msg_exec(GXMSG_PLAYER_VIDEO_WINDOW, (void *)(&play_wnd));
	
		g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
		g_AppPlayOps.program_play(PLAY_MODE_POINT,g_AppPlayOps.normal_play.play_count);
	}
    else if (ret->result == PASSWD_ERROR)
    {
        PasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.usr_data = usr_data;
        passwd_dlg.passwd_exit_cb = app_play_pwd_cb;

        if (GUI_CheckDialog("wnd_epg") == GXCORE_SUCCESS)
        {
#if(MINI_16_BITS_OSD_SUPPORT== 1)
            passwd_dlg.pos.x = 180;
            passwd_dlg.pos.y = 180;
#else
            passwd_dlg.pos.x = 215;
            passwd_dlg.pos.y = 220;
#endif
        }
        if (GUI_CheckDialog("wnd_channel_edit") == GXCORE_SUCCESS)
        {
#if(MINI_16_BITS_OSD_SUPPORT== 1)
            passwd_dlg.pos.x = 180;
            passwd_dlg.pos.y = 180;
#else
            passwd_dlg.pos.x = 215;
            passwd_dlg.pos.y = 220;
#endif
        }
        passwd_dlg_create(&passwd_dlg);
    }
    else
    {
        s_rec_flag = false;
    }

    return 0;
}

static void  app_play_poppwd_create(int32_t pos)
{
    PasswdDlg passwd_dlg;
    static PlayPwdCbData cb_data = {0};

    if ((GXCORE_SUCCESS != GUI_CheckDialog("wnd_channel_edit"))
            && (GXCORE_SUCCESS != GUI_CheckDialog("wnd_channel_list"))
            && (GXCORE_SUCCESS != GUI_CheckDialog("wnd_epg")))
    {
#define TXT_STATE       "txt_full_state"
#define IMG_STATE       "img_full_state"
        GUI_SetProperty(TXT_STATE, "string", " ");
        GUI_SetProperty(IMG_STATE, "state", "osd_trans_hide");
        GUI_SetProperty(TXT_STATE, "state", "osd_trans_hide");
        GUI_SetProperty("txt_full_title", "state", "osd_trans_hide");
    }

    cb_data.play_count = pos;
    memset(&passwd_dlg, 0, sizeof(PasswdDlg));
    passwd_dlg.passwd_exit_cb = app_play_pwd_cb;
    passwd_dlg.usr_data = &cb_data;
    if (GUI_CheckDialog("wnd_epg") == GXCORE_SUCCESS)
    {
#if(MINI_16_BITS_OSD_SUPPORT== 1)
        passwd_dlg.pos.x = 180;
        passwd_dlg.pos.y = 180;
#else
        passwd_dlg.pos.x = 215;
        passwd_dlg.pos.y = 220;
#endif
    }
    if (GUI_CheckDialog("wnd_channel_edit") == GXCORE_SUCCESS)
    {
#if(MINI_16_BITS_OSD_SUPPORT== 1)
        passwd_dlg.pos.x = 180;
        passwd_dlg.pos.y = 180;
#else
        passwd_dlg.pos.x = 215;
        passwd_dlg.pos.y = 220;
#endif
    }
#if MINI_16BIT_WIN8_OSD_SUPPORT
    //when on wnd_main_menu, don't popup the password wnd.
    if ((GXCORE_SUCCESS == GUI_CheckDialog("wnd_main_menu"))
            && (GXCORE_SUCCESS != GUI_CheckDialog("wnd_channel_edit"))
            && (GXCORE_SUCCESS != GUI_CheckDialog("wnd_epg")))
    {
        ;
    }
    else
#endif
    {
        if (GXCORE_SUCCESS == GUI_CheckDialog("wnd_epg"))
        {
            GUI_SetProperty("img_epg_red_tip", "state", "hide");
            GUI_SetProperty("text_epg_book_tip", "state", "hide");
        }
        passwd_dlg_create(&passwd_dlg);
    }
}

static void app_play_set_rec_time(uint32_t sec)
{
    s_rec_sec = sec;
}

#if RECALL_LIST_SUPPORT
static status_t app_play_set_recall_number(PlayInfo *p_play_info)
{
    static uint8_t recall_satus = 0;//标记是否是开机第一个节???	//uint16_t pre_sate_id = 0;//上一个卫星ID,用于diseqc
    int16_t j, index_callback = MAX_RECALL_NUM;
    //	printf("[recall]app_play_set_recall_number%d!!cur =%d\n",p_play_info->view_info.stream_type,
    //		p_play_info->view_info.stream_type?p_play_info->view_info.radio_prog_cur:p_play_info->view_info.tv_prog_cur);
    //	printf("[recall]play_count =%d\n",p_play_info->play_count);

    if (p_play_info == NULL)
    {
        return GXCORE_ERROR;
    }
    if (recall_satus == 1)
    {
        //pre_sate_id = g_AppPlayOps.recall_play[1].view_info.sat_id;
        for (j = MAX_RECALL_NUM - 1; j >= 0; j--)
        {
            if ((g_AppPlayOps.recall_play[j].view_info.tv_prog_cur == p_play_info->view_info.tv_prog_cur
                    && g_AppPlayOps.recall_play[j].view_info.stream_type == p_play_info->view_info.stream_type
                    && g_AppPlayOps.recall_play[j].view_info.stream_type == GXBUS_PM_PROG_TV)
                    || (g_AppPlayOps.recall_play[j].view_info.radio_prog_cur == p_play_info->view_info.radio_prog_cur
                        && g_AppPlayOps.recall_play[j].view_info.stream_type == p_play_info->view_info.stream_type
                        && g_AppPlayOps.recall_play[j].view_info.stream_type != GXBUS_PM_PROG_TV)
               )
            {
                //printf("[recall] same j=%d\n",j);
                index_callback = j;//遍历recall取得重复节目
                break;
            }
        }
        if (index_callback > MAX_RECALL_NUM - 1)
        {
            s_recall_view_num ++;
            index_callback = MAX_RECALL_NUM - 1;
        }
        if (s_recall_view_num > MAX_RECALL_NUM)
        {
            s_recall_view_num = MAX_RECALL_NUM;
        }

        for (j = index_callback; j > 0; j--)
        {
            //依次往后push,如果没有重复则丢弃最后一个历史节???
            memcpy(&(g_AppPlayOps.recall_play[j]), &(g_AppPlayOps.recall_play[j - 1]), sizeof(PlayInfo));
        }
    }
    else
    {
        // pre_sate_id = 0XFFFF;
    }

    //当前节目保存???位置

    memcpy(&(g_AppPlayOps.recall_play[!recall_satus]), p_play_info, sizeof(PlayInfo));
    //开机第一个节目历史为自己
    if (recall_satus == 0)
    {
        memcpy(&(g_AppPlayOps.recall_play[0]), &(g_AppPlayOps.recall_play[1]), sizeof(PlayInfo));
    }

    recall_satus = 1;

#if 0
    static PlayInfo t_recall_play[MAX_RECALL_NUM + 1];
    static PlayInfo bal_recall_play[MAX_RECALL_NUM + 1];
    uint16_t recall_num = 0;
    uint8_t index = 0;
    uint8_t find_same_prog = 0;

    if (p_play_info != NULL)
    {
        p_play_info->play_count = g_AppPlayOps.normal_play.play_count;
        memset(&t_recall_play[0], 0, sizeof(PlayInfo)*MAX_RECALL_NUM);
        memcpy(&bal_recall_play[s_recall_view_num], p_play_info, sizeof(PlayInfo));
        s_recall_view_num++;

        for (index = 0; index < s_recall_view_num; index++)
        {
            if ((bal_recall_play[index].view_info.tv_prog_cur != bal_recall_play[s_recall_view_num - 1].view_info.tv_prog_cur) //p_play_info->view_info.tv_prog_cur
                    || (bal_recall_play[index].view_info.group_mode != bal_recall_play[s_recall_view_num - 1].view_info.group_mode))

            {
                memcpy(&t_recall_play[recall_num], &bal_recall_play[index], sizeof(PlayInfo));
                recall_num++;
            }
            else
            {
                find_same_prog++;
            }
        }

        if (recall_num > 0)
        {
            memcpy(&t_recall_play[recall_num], &bal_recall_play[s_recall_view_num - 1], sizeof(PlayInfo));
            recall_num++;
            s_recall_view_num = recall_num;
            for (index = 0; index < s_recall_view_num; index++)
            {
                memcpy(&g_AppPlayOps.recall_play[index], &t_recall_play[s_recall_view_num - index - 1], sizeof(PlayInfo));
            }
        }
        else
        {
            if (find_same_prog > 1)
            {
                s_recall_view_num = 1;
            }
            memcpy(&g_AppPlayOps.recall_play[0], p_play_info, sizeof(PlayInfo));
        }

    }

    if (s_recall_view_num >= MAX_RECALL_NUM)
    {

        s_recall_view_num =  MAX_RECALL_NUM - 1;
    }
#endif
    return GXCORE_SUCCESS;
}

int app_play_get_recall_total_count(void)
{
    int16_t i = MAX_RECALL_NUM - 1, j;
    GxMsgProperty_NodeByPosGet node;
    status_t ret;
    node.node_type = NODE_PROG;
    uint32_t id;
    int recall_num = s_recall_view_num;
    for (i = recall_num - 1; i > 0; i--)
    {
        if (g_AppPlayOps.recall_play[i].view_info.stream_type == GXBUS_PM_PROG_RADIO)
        {
            id = g_AppPlayOps.recall_play[i].view_info.radio_prog_cur;
        }
        else
        {
            id = g_AppPlayOps.recall_play[i].view_info.tv_prog_cur;
        }
        node.id = id;
        ret = app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
        //throw skip prog
        if (ret != GXCORE_SUCCESS || node.id != id || node.prog_data.skip_flag == GXBUS_PM_PROG_BOOL_ENABLE)
        {
            //printf("i=%d s_recall_view_num = %d\n",i,s_recall_view_num);
            for (j = i; j < s_recall_view_num - 1; j++)
            {
                //丢弃无法获得的节???可能已经被删???
                memcpy(&(g_AppPlayOps.recall_play[j]), &(g_AppPlayOps.recall_play[j + 1]), sizeof(PlayInfo));
            }
            s_recall_view_num --;
        }
    }
    if (s_recall_view_num <= 1)
    {
        g_AppPlayOps.recall_play[0] = g_AppPlayOps.normal_play;
        memcpy(&(g_AppPlayOps.recall_play[1]), &(g_AppPlayOps.recall_play[0]), sizeof(PlayInfo));
        s_recall_view_num = 1;
    }

    //printf("s_recall_view_num = %d\n",s_recall_view_num);

    return s_recall_view_num;
}

#endif

#if 1
int app_program_play_set_afd_info(void)
{
    PlayerProgInfo  info;
    PlayerWindow video_wnd = {0, 0, APP_XRES, APP_YRES};
    int32_t ret = 0;
    ret = GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, &info); //solution check
    if ((ret == 0) && (info.video_stat.AFD.enable == 1))
    {
        g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL, &video_wnd);
    }
    return 0;
}

#endif

int app_play_program_user_info(GxMsgProperty_NodeModify node_modify)
{
    GxMsgProperty_NodeByPosGet node = {0};

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
    app_normal_play(&node);
    return 0;
}

AppPlayOps g_AppPlayOps =
{
    .program_play_init = app_program_play_init,
    .play_list_create = app_play_list_create,
    .program_play = app_program_play,
    .program_stop = app_program_stop,
    .play_window_set = app_play_window_set,
    .poppwd_create = app_play_poppwd_create,
    .set_rec_time = app_play_set_rec_time,
#if RECALL_LIST_SUPPORT
    .add_recall_list = app_play_set_recall_number,
#endif
};

/* End of file -------------------------------------------------------------*/
