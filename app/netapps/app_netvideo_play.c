#include "app.h"
#if NETAPPS_SUPPORT
#include "app_module.h"
#include "app_netvideo.h"
#include "app_netvideo_play.h"
#include "full_screen.h"

#ifdef ECOS_OS
#define GIF_PATH "/dvb/theme/image/netapps/loading.gif"
#else
#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
#endif

#define WND_NET_VIDEO_PLAY    "wnd_apps_netvideo_play"
#define IMG_INFOBAR_BACK    "img_netvideo_play_barback"
#define IMG_SLIDER_BACK    "img_netvideo_play_sliderback"
#define SLIDER_CURSOR    "sliderbar_cursor_netvideo_play"
#define SLIDER_PROCESS    "sliderbar_process_netvideo_play"
#define TEXT_START_TIME    "text_netvideo_play_start_time"
#define TEXT_STOP_TIME    "text_netvideo_play_stop_time"
#define TEXT_NET_VIDEO_TITLE    "text_netvideo_play_title"
#define IMG_MUTE    "img_netvideo_play_mute"
#define IMG_MUTE1    "img_netvideo_play_mute1"
#define IMG_GIF    "fimg_netvideo_play_gif"
#define TXT_POPUP    "txt_netvideo_play_popup"
#define IMG_POPUP    "img_netvideo_play_popup"
#define TXT_PLAY_WAIT "text_netvideo_play_wait"
#define WND_VOLUME    "wnd_volume_value"
#define INFO_SHOW_SEC (7)

NetVideoPlayOps s_play_ops;
static event_list *s_netvideo_play_gif_timer = NULL;
static event_list *s_state_refresh_timer = NULL;
static event_list *s_inforbar_refresh_timer = NULL;
static event_list *s_sliderbar_seek_timer = NULL;
static event_list* s_netvideo_geturl_timer = NULL;
static int s_infobar_timer_cnt = 0;
static bool s_infobar_show_flag = false;
static int s_total_video_dur = 0;
static uint64_t s_cur_time_ms_bak = 0;
static int s_cur_sliderbar_bak = 0;
static int is_netvideo_play_busy = 0;
static int netvideo_source_index = 0;

static enum
{
	VIDEO_SEEK_NONE,
	VIDEO_SEEKING,
	VIDEO_SEEKED
}s_seek_flag = VIDEO_SEEK_NONE;

static NetPlayState s_play_state = PLAY_STATE_NONE;
static NetPlayState s_play_state_bak = PLAY_STATE_NONE;
static NetPlayErrorType s_play_error = PLAY_PLAYER_ERROR;
static char *play_error_str = NULL;

static void app_net_video_free_error_str(void)
{
	if(play_error_str)
	{
		free(play_error_str);
		play_error_str = NULL;
	}
}

void app_net_video_set_error_str(char *errstr)
{
	if(errstr)
	{
		app_net_video_free_error_str();
		play_error_str = strdup(errstr);
		s_play_error = PLAY_USER_ERROR;
	}
}

void app_net_video_set_play_error(NetPlayErrorType error)
{
	s_play_error = error;
}

static int _net_video_play_draw_gif(void *usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
    return 0;
}

static void _net_video_play_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
	GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _net_video_play_free_gif(void)
{
    remove_timer(s_netvideo_play_gif_timer);
    s_netvideo_play_gif_timer = NULL;
	GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void _net_video_play_show_gif(void)
{
	GUI_SetProperty(IMG_GIF, "state", "show");
    if(0 != reset_timer(s_netvideo_play_gif_timer))
	{
		s_netvideo_play_gif_timer = create_timer(_net_video_play_draw_gif, 100, NULL, TIMER_REPEAT);
	}
}

static void _net_video_play_hide_gif(void)
{
	timer_stop(s_netvideo_play_gif_timer);
	GUI_SetProperty(IMG_GIF, "state", "hide");
}

static void _show_popup_msg(char* msg)
{
	GUI_SetProperty(TXT_POPUP, "string", msg);
	GUI_SetProperty(IMG_POPUP, "state", "show");
	GUI_SetProperty(TXT_POPUP, "state", "show");
	GUI_SetProperty(TXT_PLAY_WAIT, "state", "hide");
}

static void _hide_popup_msg(void)
{
	GUI_SetProperty(IMG_POPUP, "state", "hide");
	GUI_SetProperty(TXT_POPUP, "state", "hide");
	GUI_SetProperty(TXT_PLAY_WAIT, "state", "hide");
}

static int _net_video_play_state_draw(void *usrdata)
{
	if(s_play_state == s_play_state_bak)
		return 0;

	s_play_state_bak = s_play_state;

	if(s_play_state == PLAY_STATE_LOAD)
	{
		_hide_popup_msg();
		_net_video_play_show_gif();
	}
	else
	{
		_net_video_play_hide_gif();
		if(s_play_state == PLAY_STATE_ERROR)
		{
			if(s_play_error == PLAY_PLAYER_ERROR)
			{
				_show_popup_msg("Play error!");
			}
			else if(s_play_error == PLAY_CONNECT_ERROR)
			{
				_show_popup_msg(STR_ID_SERVER_FAIL);
			}
			else if(s_play_error == PLAY_SYSTEM_BUSY)
			{
				_show_popup_msg("System busy, please try again!");
			}
			else if((s_play_error == PLAY_USER_ERROR)&&play_error_str)
			{
				_show_popup_msg(play_error_str);
			}
			else
			{
				_show_popup_msg("Internal error!");
			}
		}
		else
		{
			_hide_popup_msg();
		}
	}

	return 0;
}

static void _net_video_state_refresh_start(void)
{
    if(0 != reset_timer(s_state_refresh_timer))
	{
		s_state_refresh_timer = create_timer(_net_video_play_state_draw, 300, NULL, TIMER_REPEAT);
	}
}

static void _net_video_state_refresh_stop(void)
{
    timer_stop(s_state_refresh_timer);
    s_play_state_bak = PLAY_STATE_NONE;

    _hide_popup_msg();
    _net_video_play_hide_gif();
}

static void _netvideo_play_update_infobar(void)
{
	uint64_t cur_time_ms = 0;
	uint64_t total_time_ms = 0;
	uint64_t cur_time_s = 0;
	uint64_t total_time_s = 0;
	int cur_sliderbar = 0;
	struct tm *ptm = NULL;
	char buf_tmp[20] = {0};
	status_t ret = GXCORE_ERROR;
	PlayTimeInfo  tinfo = {0};

	if(s_play_state == PLAY_STATE_RUNNING)
	{
		if((ret=GxPlayer_MediaGetTime(PLAYER_FOR_IPTV,  &tinfo)) == GXCORE_SUCCESS)
		{
			cur_time_ms = tinfo.current;
			total_time_ms = tinfo.totle;
			s_total_video_dur = total_time_ms;
			if(s_total_video_dur < 0)
			{
				s_total_video_dur = 0;
				cur_time_ms = 0;
			}
			else if(s_total_video_dur > 0)
			{
				if(cur_time_ms > s_total_video_dur)
				{
					cur_time_ms = s_total_video_dur;
					cur_sliderbar = 100;
				}
				else
				{
					cur_sliderbar = (cur_time_ms * 100)/ s_total_video_dur ;
				}
			}
		}
		else
		{
			return;
		}
		s_cur_time_ms_bak = cur_time_ms;
		s_cur_sliderbar_bak = cur_sliderbar;
		if(s_seek_flag != VIDEO_SEEKING)
		{
			if(s_play_ops.seek_support == true)
			{
				GUI_SetProperty(SLIDER_PROCESS, "value", &cur_sliderbar);
				if(s_seek_flag == VIDEO_SEEK_NONE)
				{
					GUI_SetProperty(SLIDER_CURSOR, "value", &cur_sliderbar);
				}
				else
				{
					int seeked_val = 0;
					GUI_GetProperty(SLIDER_CURSOR, "value", &seeked_val);
					GUI_SetProperty(SLIDER_CURSOR, "value", &seeked_val);
				}
			}
			/*cur time*/
			cur_time_s = cur_time_ms / 1000;
			cur_time_s += ((cur_time_ms % 1000) == 0 ? 0 : 1);
			ptm = localtime((const time_t*)  &cur_time_s);
			sprintf(buf_tmp, "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
			GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
		}

		if(s_play_ops.seek_support == true)
		{
			/*total time*/
			total_time_s = s_total_video_dur / 1000;
			total_time_s += ((s_total_video_dur % 1000) == 0 ? 0 : 1);
			ptm = localtime((const time_t*)  &total_time_s);
			sprintf(buf_tmp, "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
			GUI_SetProperty(TEXT_STOP_TIME, "string", buf_tmp);
		}
	}
}

static void _netvideo_infobar_timer_stop(void)
{
	remove_timer(s_inforbar_refresh_timer);
	s_inforbar_refresh_timer = NULL;
    s_infobar_timer_cnt = 0;
}

static void _netvideo_play_hide_infobar(void)
{
    _netvideo_infobar_timer_stop();

	GUI_SetProperty(IMG_INFOBAR_BACK, "state", "hide");
	GUI_SetProperty(IMG_SLIDER_BACK, "state", "hide");
	GUI_SetProperty(SLIDER_PROCESS, "state", "hide");
	GUI_SetProperty(SLIDER_CURSOR, "state", "hide");
	GUI_SetProperty(TEXT_START_TIME, "state", "hide");
	GUI_SetProperty(TEXT_STOP_TIME, "state", "hide");
	GUI_SetProperty(TEXT_NET_VIDEO_TITLE, "state", "hide");
    GUI_SetProperty(SLIDER_CURSOR, "state", "hide");

	if(s_play_ops.seek_support)
		APP_TIMER_REMOVE(s_sliderbar_seek_timer);

    s_seek_flag = VIDEO_SEEK_NONE;
    s_infobar_show_flag = false;
}

static int _netvideo_infobar_timer_cb(void *userdata)
{
    int show_sec = *(int*)userdata;

    s_infobar_timer_cnt++;
    if(s_infobar_timer_cnt == show_sec)
    {
        _netvideo_play_hide_infobar();
    }
    else
    {
        _netvideo_play_update_infobar();
    }

	return 0;
}

static void _netvideo_infobar_timer_reset(void)
{
    static int show_sec = INFO_SHOW_SEC;

    s_infobar_timer_cnt = 0;
    if (reset_timer(s_inforbar_refresh_timer) != 0)
	{
		s_inforbar_refresh_timer = create_timer(_netvideo_infobar_timer_cb, 1000, &show_sec, TIMER_REPEAT);
	}
}

static void _netvideo_play_show_infobar(void)
{
	GUI_SetProperty(TEXT_NET_VIDEO_TITLE, "string", s_play_ops.netvideo_title);
	GUI_SetProperty(IMG_INFOBAR_BACK, "state", "show");
	GUI_SetProperty(IMG_SLIDER_BACK, "state", "show");
	GUI_SetProperty(SLIDER_PROCESS, "state", "show");
	GUI_SetProperty(TEXT_START_TIME, "state", "show");
	GUI_SetProperty(TEXT_STOP_TIME, "state", "show");
	GUI_SetProperty(TEXT_NET_VIDEO_TITLE, "state", "show");
	GUI_SetProperty(SLIDER_PROCESS, "state", "show");
	GUI_SetProperty(SLIDER_CURSOR, "state", "show");
	GUI_SetFocusWidget(SLIDER_CURSOR);
	_netvideo_play_update_infobar();
	_netvideo_infobar_timer_reset();

	s_infobar_show_flag = true;
}

static void _net_video_mute_exec(void)
{
    GxMsgProperty_PlayerAudioMute player_mute;

    if (g_AppFullArb.state.mute == STATE_ON)
    {
        player_mute = 0;
        g_AppFullArb.state.mute = STATE_OFF;
    }
    else
    {
        player_mute = 1;
        g_AppFullArb.state.mute = STATE_ON;
    }

    app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
}

static void _net_video_mute_draw(void)
{
    if (g_AppFullArb.state.mute == STATE_ON)
    {
        GUI_SetProperty(IMG_MUTE, "state", "show");
        GUI_SetProperty(IMG_MUTE1, "state", "show");
    }
    else
    {
        GUI_SetProperty(IMG_MUTE, "state", "osd_trans_hide");
        GUI_SetProperty(IMG_MUTE1, "state", "osd_trans_hide");
    }
    GUI_SetProperty(IMG_MUTE, "draw_now", NULL);
    GUI_SetProperty(IMG_MUTE1, "draw_now", NULL);
}

static status_t _net_video_sliderbar_seek_timercb(void* usrdata)
{
    APP_TIMER_REMOVE(s_sliderbar_seek_timer);
    s_seek_flag = VIDEO_SEEKED;

	return GXCORE_SUCCESS;
}

static status_t _net_video_sliderbar_okkey(void)
{
	int cur_time_ms = 0;
	int value=0;
	GxMsgProperty_PlayerSeek seek;

	GUI_GetProperty(SLIDER_CURSOR, "value", &value);
	cur_time_ms = s_total_video_dur*value/100;

    if((cur_time_ms > s_total_video_dur)||(0 == s_total_video_dur))
		return GXCORE_ERROR;

    memset(&seek, 0, sizeof(GxMsgProperty_PlayerSeek));
	seek.player = PLAYER_FOR_IPTV;
	seek.time = cur_time_ms;
	seek.flag = SEEK_ORIGIN_SET;

    app_send_msg_exec(GXMSG_PLAYER_SEEK, &seek);

	s_seek_flag = VIDEO_SEEK_NONE;

	return GXCORE_SUCCESS;
}

static void _net_video_get_url_thread(void* arg)
{
	GxCore_ThreadDetach();
	if(is_netvideo_play_busy)
	{
		s_play_error = PLAY_SYSTEM_BUSY;
		app_net_video_start_play(NULL, PLAY_STATE_ERROR, 0, false);
	}
	else if(s_play_ops.get_url == NULL)
	{
		s_play_error = PLAY_URL_NONE;
		app_net_video_start_play(NULL, PLAY_STATE_ERROR, 0, false);
	}
	else
	{
		is_netvideo_play_busy = 1;
		s_play_ops.get_url(netvideo_source_index);
		is_netvideo_play_busy = 0;
	}
}

static void _net_video_get_url(void)
{
	int thread_id = 0;
	GxCore_ThreadCreate("netvideo_geturl", &thread_id, _net_video_get_url_thread,
		NULL, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static void app_netvideo_geturl_timer_stop(void)
{
	if(s_netvideo_geturl_timer)
	{
		remove_timer(s_netvideo_geturl_timer);
		s_netvideo_geturl_timer = NULL;
	}
}

static int netvideo_geturl_timer_timeout(void *usrdata)
{
	_net_video_get_url();
	app_netvideo_geturl_timer_stop();
	return 0;
}

static void app_netvideo_geturl_timer_start(void)
{
	app_netvideo_geturl_timer_stop();
	s_netvideo_geturl_timer = create_timer(netvideo_geturl_timer_timeout, 10, NULL, TIMER_ONCE);
}

void app_net_video_switch_source(int index)
{
	if((index>=0)&&(index<s_play_ops.source_num))
	{
		netvideo_source_index = index;
		app_netvideo_geturl_timer_start();
	}
	else
	{
		_show_popup_msg("Internal error!");
	}
}

status_t app_net_video_start_play(char *url, NetPlayState play_state, int64_t start, bool show_info)
{
	GxMsgProperty_PlayerPlay video_play = {0};

	if(play_state == PLAY_STATE_ERROR)
	{
		s_play_state = play_state;
	}
	else
	{
		if(url == NULL)
			return GXCORE_ERROR;
		if(show_info)
			if(strcmp(GUI_GetFocusWindow(), WND_NET_VIDEO_PLAY) == 0)
				_netvideo_play_show_infobar();

		sprintf(video_play.url, "%s", url);
		video_play.player = PLAYER_FOR_IPTV;
		video_play.start = start;
		app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)(&video_play));
	}
	return GXCORE_SUCCESS;
}

status_t app_net_video_play(NetVideoPlayOps *play_ops)
{
	int progress = 0;
	if(play_ops == NULL)
		return GXCORE_ERROR;

	GxPlayer_MediaStop(PLAYER_FOR_IPTV);
	_net_video_state_refresh_stop();
	s_play_state = PLAY_STATE_LOAD;
	_netvideo_infobar_timer_stop();
	memset(&s_play_ops, 0, sizeof(NetVideoPlayOps));
	memcpy(&s_play_ops, play_ops, sizeof(NetVideoPlayOps));
	if(s_play_ops.source_num<1)
	{
		s_play_ops.source_num = 1;
	}
	netvideo_source_index = 0;
	if(GUI_CheckDialog(WND_NET_VIDEO_PLAY) == GXCORE_ERROR)
	{
		GUI_CreateDialog(WND_NET_VIDEO_PLAY);
		_net_video_mute_draw();
		GUI_SetInterface("flush", NULL);
	}

	s_total_video_dur = 0;
	GUI_SetProperty(TEXT_START_TIME, "string", "00:00:00");
	GUI_SetProperty(TEXT_STOP_TIME, "string", "00:00:00");
	GUI_SetProperty(SLIDER_CURSOR, "value", &progress);
	GUI_SetProperty(SLIDER_PROCESS, "value", &progress);

	if(strcmp(GUI_GetFocusWindow(), WND_NET_VIDEO_PLAY) == 0)
	{
		_net_video_state_refresh_start();
	}

	_net_video_get_url();
    return 0;
}

int app_net_video_play_status(GxMessage* msg)
{
	GxMsgProperty_PlayerStatusReport* player_status = NULL;
	player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);

	if((player_status == NULL) || (strcmp((char*)(player_status->player), PLAYER_FOR_IPTV) != 0))
	{
        return EVENT_TRANSFER_STOP;
	}

	switch(player_status->status)
	{
		case PLAYER_STATUS_ERROR:
			printf("\n[%s]%d PLAYER_STATUS_ERROR, %d\n", __func__, __LINE__, player_status->error);
			s_play_state = PLAY_STATE_ERROR;
			if(player_status->error == PLAYER_ERROR_NO_DATA_SOURCE)
			{
				if((s_play_ops.source_num>1)&&(netvideo_source_index<(s_play_ops.source_num-1)))
				{
					netvideo_source_index++;
    					GxPlayer_MediaExitPlay(PLAYER_FOR_IPTV);
					s_play_state = PLAY_STATE_LOAD;
					_net_video_get_url();
				}
				else
				{
					s_play_error = PLAY_CONNECT_ERROR;
				}
			}
			else if((player_status->error == PLAYER_ERROR_ACCESS_ERROR)
				||(player_status->error == PLAYER_ERROR_SOURCE_RESTART))
			{
				s_play_error = PLAY_CONNECT_ERROR;
			}
			else
			{
				s_play_error = PLAY_PLAYER_ERROR;
			}
			_net_video_play_state_draw(NULL);
			break;

		case PLAYER_STATUS_PLAY_END:
			printf("\n[%s]%d PLAYER_STATUS_PLAY_END\n", __func__, __LINE__);
			GUI_EndDialog(WND_NET_VIDEO_PLAY);
			if(s_play_ops.play_ctrl.play_exit != NULL)
				s_play_ops.play_ctrl.play_exit();
			break;

		case PLAYER_STATUS_PLAY_RUNNING:
			printf("\n[%s]%d PLAYER_STATUS_PLAY_RUNNING\n", __func__, __LINE__);
			break;

		case PLAYER_STATUS_PLAY_START:
			printf("\n[%s]%d PLAYER_STATUS_PLAY_START\n", __func__, __LINE__);
			break;

        case PLAYER_STATUS_STOPPED:
			printf("\n[%s]%d PLAYER_STATUS_STOPPED\n", __func__, __LINE__);
			break;

        case PLAYER_STATUS_START_FILL_BUF:
            s_play_state = PLAY_STATE_LOAD;
			printf("\n[%s]%d PLAYER_STATUS_START_FILL_BUF\n", __func__, __LINE__);
            break;

        case PLAYER_STATUS_END_FILL_BUF:
            s_play_state = PLAY_STATE_RUNNING;
			printf("\n[%s]%d PLAYER_STATUS_END_FILL_BUF\n", __func__, __LINE__);
            break;

		default:
			break;
	}
    return EVENT_TRANSFER_STOP;
}

int app_net_video_avcodec_status(GxMessage* msg)
{
	GxMsgProperty_PlayerAVCodecReport *avcodec_status = NULL;
    avcodec_status = (GxMsgProperty_PlayerAVCodecReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerAVCodecReport);

    if(AVCODEC_ERROR == avcodec_status->acodec.state|| AVCODEC_ERROR == avcodec_status->vcodec.state)
    {

    }
    else if(avcodec_status->vcodec.state == AVCODEC_RUNNING)
    {
        s_play_state = PLAY_STATE_RUNNING;
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_play_create(GuiWidget *widget, void *usrdata)
{
    _net_video_play_load_gif();
	GUI_SetProperty(TXT_PLAY_WAIT, "state", "show");
	s_cur_time_ms_bak = 0;
	s_cur_sliderbar_bak = 0;
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_play_destroy(GuiWidget *widget, void *usrdata)
{
	_netvideo_play_hide_infobar();
	_net_video_state_refresh_stop();
	remove_timer(s_state_refresh_timer);
	s_state_refresh_timer = NULL;
	GxPlayer_MediaStop(PLAYER_FOR_IPTV);
	memset(&s_play_ops, 0, sizeof(NetVideoPlayOps));
	_net_video_play_free_gif();
	app_net_video_free_error_str();
	app_netvideo_geturl_timer_stop();
	app_netvideo_time_timer_reset();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_play_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    void (*exit_cb)(void);

	int ret = EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;

            case STBK_EXIT:
                if((s_infobar_show_flag == false) || (s_play_state != PLAY_STATE_RUNNING))
                {
                    exit_cb = s_play_ops.play_ctrl.play_exit;
                    GUI_EndDialog(WND_NET_VIDEO_PLAY);
                    if(exit_cb != NULL)
                        exit_cb();
                }
                else
                {
                    _netvideo_play_hide_infobar();
                }
                break;

            case STBK_MENU:
                exit_cb = s_play_ops.play_ctrl.play_exit;
                GUI_EndDialog(WND_NET_VIDEO_PLAY);
                if(exit_cb != NULL)
                    exit_cb();
                break;

            case STBK_INFO:
                _netvideo_play_show_infobar();
                break;

            case STBK_MUTE:
                _net_video_mute_exec();
                _net_video_mute_draw();
                break;

            case STBK_LEFT:
            case STBK_VOLDOWN:
                event->key.sym = APPK_VOLDOWN;
                GUI_CreateDialog(WND_VOLUME);
                GUI_SendEvent(WND_VOLUME, event);
                _net_video_mute_draw();
                break;

            case STBK_RIGHT:
            case STBK_VOLUP:
                event->key.sym = APPK_VOLUP;
                GUI_CreateDialog(WND_VOLUME);
                GUI_SendEvent(WND_VOLUME, event);
                _net_video_mute_draw();
                break;

            case STBK_UP:
                if(s_play_ops.play_ctrl.play_next != NULL)
                    s_play_ops.play_ctrl.play_next();
                break;

            case STBK_DOWN:
                if(s_play_ops.play_ctrl.play_prev != NULL)
                    s_play_ops.play_ctrl.play_prev();
                break;

            case STBK_OK:
                if(s_play_ops.seek_support)
                {
                    _netvideo_play_show_infobar();
                }
                else
                {
                    _netvideo_play_hide_infobar();
                    if(s_play_ops.play_ctrl.play_ok != NULL)
                        s_play_ops.play_ctrl.play_ok();
                }
                break;

            default:
                break;
        }
    }
    return ret;
}

SIGNAL_HANDLER int app_netvideo_play_got_focus(GuiWidget *widget, void *usrdata)
{
    _net_video_state_refresh_start();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_play_lost_focus(GuiWidget *widget, void *usrdata)
{
    //_net_video_state_refresh_stop();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_play_cursor_keypress(const char* widgetname, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	if(s_play_ops.seek_support)
	{
		GUI_Event *event = NULL;
		int64_t cur_time_ms = 0;
		int32_t cur_time_s = 0;
		int value=0,step=0;
		struct tm *ptm = NULL;
		char buf_tmp[20] = {0};

		APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

		event = (GUI_Event *)usrdata;
		switch(event->key.sym)
		{
			case STBK_EXIT:
				_netvideo_play_hide_infobar();
				break;

			case STBK_LEFT:

				s_seek_flag = VIDEO_SEEKING;
				_netvideo_play_show_infobar();

				GUI_GetProperty(SLIDER_CURSOR, "value", &value);
				GUI_GetProperty(SLIDER_CURSOR, "step", &step);
				if(value-step<0)
				{
					value=0;
				}
				else
				{
					value-=step;
				}
				cur_time_ms = s_total_video_dur*value/100;
				cur_time_s = cur_time_ms / 1000;
				cur_time_s += ((cur_time_ms % 1000) == 0 ? 0 : 1);
				ptm = localtime((const time_t*)  &cur_time_s);
				sprintf(buf_tmp, "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
				GUI_SetProperty(SLIDER_CURSOR, "value", &value);

				APP_TIMER_ADD(s_sliderbar_seek_timer, _net_video_sliderbar_seek_timercb, 100, TIMER_ONCE);
				break;

			case STBK_RIGHT:

				s_seek_flag = VIDEO_SEEKING;
				_netvideo_play_show_infobar();

				GUI_GetProperty(SLIDER_CURSOR, "value", &value);
				GUI_GetProperty(SLIDER_CURSOR, "step", &step);
				if(value+step>100)
				{
					value=100;
				}
				else
				{
					value+=step;
				}
				cur_time_ms = s_total_video_dur*value/100;
				cur_time_s = cur_time_ms / 1000;
				cur_time_s += ((cur_time_ms % 1000) == 0 ? 0 : 1);
				ptm = localtime((const time_t*)  &cur_time_s);
				sprintf(buf_tmp, "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
				GUI_SetProperty(SLIDER_CURSOR, "value", &value);

				APP_TIMER_ADD(s_sliderbar_seek_timer, _net_video_sliderbar_seek_timercb, 100, TIMER_ONCE);
				break;

			case STBK_OK:
				_netvideo_play_show_infobar();
				if(s_seek_flag == VIDEO_SEEKED)
				{
					s_play_state = PLAY_STATE_LOAD;
					_net_video_sliderbar_okkey();
				}
				break;;

			default:
				_netvideo_play_hide_infobar();
				ret = EVENT_TRANSFER_KEEPON;
				break;
		}
	}
	else
	{
		ret = EVENT_TRANSFER_KEEPON;
	}
	return ret;
}

#else
SIGNAL_HANDLER int app_netvideo_play_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_netvideo_play_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_netvideo_play_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_netvideo_play_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_netvideo_play_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_netvideo_play_cursor_keypress(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
#endif
