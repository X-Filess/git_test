#include "app.h"
#if (BOX_SERVICE > 0)
#include "app_pop.h"
#include "full_screen.h"
#include "dlna/dlna.h"
#include "jansson.h"
#include "gxbox.h"
#include "module/app_play_control.h"
#include "app_frontend.h"

#define SAT_ID_BASE 0
#define FAV_ID_BASE 20000000
#define TP_ID_BASE  40000000

#define SAT_ID(id)  (id|SAT_ID_BASE)
#define FAV_ID(id)  (id|FAV_ID_BASE)
#define TP_ID(id)   (id|TP_ID_BASE)

#if JSON_INTEGER_IS_LONG_LONG
#define COMMAND_OK(ret)     {\
	json_seti(ret, "result", 0ll);\
	json_sets(ret, "error", "");\
}while(0)

#define COMMAND_ERROR(ret, error_msg)     {\
	json_seti(ret, "result", 1ll);\
	json_sets(ret, "error", error_msg);\
}while(0)

#else
#define COMMAND_OK(ret)     {\
	json_seti(ret, "result", 0l);\
	json_sets(ret, "error", "");\
}while(0)

#define COMMAND_ERROR(ret, error_msg)     {\
	json_seti(ret, "result", 1l);\
	json_sets(ret, "error", error_msg);\
}while(0)
#endif
extern json_int_t json_geti(json_t *js, const char *key, json_int_t def);
extern void json_seti(json_t *js, const char *key, json_int_t value);
extern void json_sets(json_t *js, const char *key, const char *value);
extern const char *json_gets(json_t *js, const char *key, const char *def);
#if (DLNA_SUPPORT > 0)
extern void app_create_dlna_menu(void);
#endif
extern void app_play_set_diseqc(uint32_t sat_id, uint16_t tp_id, bool ForceFlag);

void cmd_push_channel(json_t *ret, json_t *js)
{
	int ch_id = json_geti(js, "channel_id", -1);
	if (ret==NULL || js==NULL)
        return;

	if (ch_id == -1)
		return;

    if(strcmp(GUI_GetFocusWindow(), "wnd_full_screen") != 0)
        return;
	if (g_AppPlayOps.normal_play.view_info.group_mode != GROUP_MODE_ALL) {
		g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
		g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));
	}

	int pos = 0;
	int total = GxBus_PmProgNumGet();
	GxBusPmDataProg *progs = (GxBusPmDataProg *)GxCore_Malloc(sizeof(GxBusPmDataProg)*total);
	if (progs == NULL)
		return;

	GxBus_PmProgGetByPos(0, total, progs);

	for (pos=0; pos<total; pos++) {
		if (progs[pos].id == ch_id) {
			printf("push channel: %s\n", progs[pos].prog_name);
			g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, pos);
			GxCore_Free(progs);
			COMMAND_OK(ret);
            return;
		}
	}

	printf("push channel not found\n");

	GxCore_Free(progs);
	COMMAND_ERROR(ret, "push channel not found");
}

/*
 * "group_type"  "group_id"  "channel_pos?"
 */
void cmd_push_group(json_t *ret, json_t *js)
{
	if (ret==NULL || js==NULL) return;

	int group_mode = json_geti(js, "group_type", -1);
	int group_id = json_geti(js, "group_id", -1);
	int channel_pos = json_geti(js, "channel_pos", -1);

	if ((group_mode==-1) || ((group_mode>GROUP_MODE_ALL)&&(group_id==-1)))
		return;

	g_AppPlayOps.normal_play.view_info.group_mode = group_mode;
	if (group_mode == GROUP_MODE_SAT)
		g_AppPlayOps.normal_play.view_info.sat_id = group_id;
	else if (group_mode == GROUP_MODE_FAV)
		g_AppPlayOps.normal_play.view_info.fav_id = (group_id & (~FAV_ID_BASE));
	else
	{}


	g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));

	if (channel_pos == -1)
		g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
	else
		g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, channel_pos);
	COMMAND_OK(ret);
}
/*
 * "sync_db_from_stb"
 */
void cmd_sync_db_from_stb(json_t *ret, json_t *js)
{
#if 1
	if (ret==NULL || js==NULL) return;

	int groupType = g_AppPlayOps.normal_play.view_info.group_mode;
	int groupId = 0;
	if (groupType == GROUP_MODE_SAT)
		groupId = g_AppPlayOps.normal_play.view_info.sat_id | SAT_ID_BASE;
	else if (groupType == GROUP_MODE_FAV)
		groupId = g_AppPlayOps.normal_play.view_info.fav_id | FAV_ID_BASE;
	else
	{}

	int channelPos = g_AppPlayOps.normal_play.play_count;
	GxBusPmDataProg ch;
	GxBus_PmProgGetByPos(channelPos, 1, &ch);

	json_seti(ret, "group_type",groupType);
	json_seti(ret, "group_id",groupId);
	json_seti(ret, "channel_pos",channelPos);
	json_seti(ret, "channel_id",ch.id);

	COMMAND_OK(ret);
#endif
}

/*
 * IPC for DLNA
 */
#if (DLNA_SUPPORT > 0)
void cmd_ipc_from_dlna(json_t *ret, json_t *js)
{
    int act = 0;
    if(js != NULL)
    {
        const char *action = json_gets(js, "action", "");
        const char *url = json_gets(js, "url", "");
        const char *metadata = json_gets(js, "uri_metadata", "");
        int volume = json_geti(js, "volume", AUDIO_VOLUME);
        GxMsgProperty_DlnaMsg dlna_msg;

        memset(&dlna_msg, 0, sizeof(GxMsgProperty_DlnaMsg));
        strncpy(dlna_msg.action, action, 99);
        strncpy(dlna_msg.url, url, MAX_DLNA_URL_LENGTH - 1);
        strncpy(dlna_msg.metadata, metadata, 99);
        dlna_msg.volume = volume;
        //memcpy(&dlna_msg.js, js, sizeof(json_t));

        //printf("@@@@@@@@@@@@@[%s]%d dlna_action: %s @@@@@@@@@@@@@@\n", __FILE__, __LINE__, action);

        act = app_get_dlna_action(action);
        switch(act)
        {
            case DLNA_VOLUME_GET:
                dplayer.vol_get(&volume);
                printf("[app dlna]current volume is %d\n",volume);
                json_seti(ret, "current_volume" ,volume);
                COMMAND_OK(ret);
                break;

            case DLNA_SEEK:
                {
                    int32_t time_s = json_geti(js, "time", 0);
                    printf("[app dlna]~~~~~~the seek time is :%d\n",time_s);
                    dplayer.seek(time_s);
                }
                break;

            case DLNA_GET_POSITION_INFO:
                {
                    uint64_t current_ms = 0, total_ms = 0;
                    if (GXCORE_SUCCESS != dplayer.get_position_info(&current_ms, &total_ms))
                    {
                        COMMAND_ERROR(ret, "[dlna] get player info err!!!");
                        return;
                    }

                    //printf("[app dlna]current_time = %lld, total_time = %lld\n",current_ms ,total_ms);
                    json_seti(ret, "current_time" ,current_ms);
                    COMMAND_OK(ret);

                    json_seti(ret, "total_time" ,total_ms);
                    COMMAND_OK(ret);
                }
                break;

            case DLNA_GET_TRANSPORT_INFO:
                {
                    PlayerStatusInfo info;
                    char status[20] = {0};

                    if (GXCORE_SUCCESS != dplayer.get_player_status(&info))
                    {
                        COMMAND_ERROR(ret, "[dlna] get player status err!!!");
                        return;
                    }

                    if (info.status == PLAYER_STATUS_PLAY_RUNNING)
                        strcpy(status, "PLAYING");
                    else if (info.status == PLAYER_STATUS_PLAY_PAUSE)
                        strcpy(status, "PAUSED_PLAYBACK");
                    else if (info.status == PLAYER_STATUS_STOPPED)
                        strcpy(status, "STOPPED");
                    else
                        strcpy(status, "NO_MEDIA_PRESENT");

                    printf("[app dlna]~~~~~transport_state = %s\n",status);
                    json_sets(ret, "transport_state" ,status);
                    COMMAND_OK(ret);
                }
                break;

            default:
                app_send_msg_exec(GXMSG_DLNA_TRIGGER, (void *)(&dlna_msg));
                break;
        }
    }
}
#endif

status_t app_box_set_diseqc(int prog_id)
{
    static uint16_t prog_tp_id = 0;

    GxMsgProperty_NodeByIdGet cur_prog = {0};
    GxMsgProperty_NodeByIdGet prog_node = {0};
    GxMsgProperty_NodeByIdGet SatNode = {0};
    GxMsgProperty_NodeByIdGet TpNode = {0};
    GxMsgProperty_FrontendSetTp Param = {0};
    fe_sec_voltage_t polar;
    int32_t fre = 0;
    int32_t symb = 0;

    cur_prog.node_type = NODE_PROG;
    cur_prog.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &cur_prog);

    prog_node.node_type = NODE_PROG;
    prog_node.id = prog_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &prog_node);

    SatNode.node_type = NODE_SAT;
    SatNode.id = prog_node.prog_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &SatNode);

    TpNode.node_type = NODE_TP;
    TpNode.id = prog_node.prog_data.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &TpNode);

    fre = app_show_to_tp_freq(&SatNode.sat_data, &TpNode.tp_data);
    symb = TpNode.tp_data.tp_s.symbol_rate;
    polar = app_show_to_lnb_polar(&SatNode.sat_data);
    if(polar == SEC_VOLTAGE_ALL)
    {
        polar = app_show_to_tp_polar(&TpNode.tp_data);
    }

    GxFrontend_GetCurFre(prog_node.prog_data.tuner,&Param);
    if((prog_node.prog_data.tp_id != cur_prog.prog_data.tp_id)
            || (Param.fre != fre)
            || (Param.polar != polar)
            || (Param.symb != symb))
    {
        prog_tp_id = prog_node.prog_data.tp_id;
        GxFrontend_ForceExitEx(prog_node.prog_data.tuner);
        GxFrontend_SetUnlock(prog_node.prog_data.tuner);// for TS disable output
        GxFrontend_CleanRecordPara(prog_node.prog_data.tuner);
        //GxFrontend_CleanRecord(prog_node->prog_data.tuner);
    }
    // add in 20121126 for double tuner monitor control
    app_frontend_demod_control(prog_node.prog_data.tuner,FALSE);

    app_play_set_diseqc(prog_node.prog_data.sat_id, prog_node.prog_data.tp_id, false);

    return GXCORE_SUCCESS;
}

// this must do after "box_service" init
void app_cmd_init()
{
	zbox_add_command("push_channel", cmd_push_channel);
	zbox_add_command("push_group", cmd_push_group);
	zbox_add_command("sync_db_from_stb", cmd_sync_db_from_stb);
#if (DLNA_SUPPORT > 0)
	zbox_add_command("ipc_dlna", cmd_ipc_from_dlna);
#endif
}
#endif
