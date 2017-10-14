#include "app.h"
#if IPTV_SUPPORT
#include "app_module.h"
#include "app_iptv_play.h"
#include "full_screen.h"


#ifdef ECOS_OS
#define GIF_PATH "/dvb/theme/image/netapps/loading.gif"
#else
#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
#endif

#define WND_IPTV_PLAY    "wnd_iptv_play"
#define WND_IPTV_INFO    "wnd_iptv_info"
#define TEXT_START_TIME    "text_iptv_info_start_time"
#define TEXT_STOP_TIME    "text_iptv_info_stop_time"
#define TEXT_MEDIA_TITLE    "text_iptv_info_title"
#define IMG_MUTE    "img_iptv_play_mute"
#if MINI_16_BITS_OSD_SUPPORT
#define IMG_MUTE_1    "img_iptv_play_mute1"
#endif
#define IMG_GIF    "img_iptv_play_gif"
#define TEXT_PLAY_WAIT "text_iptv_play_wait"
#define TXT_POPUP    "txt_iptv_play_popup"
#define IMG_POPUP    "img_iptv_play_popup"
#define INFO_SHOW_SEC (7)
#define AD_PIC_SHOW_SEC (7)

extern void app_av_setting_init(void);
IPTVPlayOps s_play_ops;
//static event_list *s_iptv_first_in_timer = NULL;
static event_list *s_iptv_play_gif_timer = NULL;
static event_list *s_state_refresh_timer = NULL;
static event_list *s_inforbar_refresh_timer = NULL;
static event_list *thiz_ad_pic_timer = NULL;
static event_list *s_iptv_play_time_timer = NULL;
static int s_infobar_timer_cnt = 0;
static bool s_infobar_show_flag = false;
static int s_total_prog_dur = 0;
static uint64_t s_cur_time_ms_bak = 0;
static int s_cur_sliderbar_bak = 0;
static bool thiz_pre_play = false;

static IPTVPlayState s_play_state = PLAY_STATE_NONE;
static IPTVPlayState s_play_state_bak = PLAY_STATE_NONE;

static int _iptv_play_draw_gif(void *usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
	return 0;
}

static void _iptv_play_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
	GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _iptv_play_free_gif(void)
{
	remove_timer(s_iptv_play_gif_timer);
	s_iptv_play_gif_timer = NULL;
	GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void _iptv_play_show_gif(void)
{
	GUI_SetProperty(IMG_GIF, "state", "show");
	if(0 != reset_timer(s_iptv_play_gif_timer))
	{
		s_iptv_play_gif_timer = create_timer(_iptv_play_draw_gif, 100, NULL, TIMER_REPEAT);
	}
}

void app_iptv_play_show_gif(void *userdata)
{
	GUI_SetProperty(IMG_GIF, "state", "show");
	_iptv_play_draw_gif(NULL);
}

static void _iptv_play_hide_gif(void)
{
	timer_stop(s_iptv_play_gif_timer);
	GUI_SetProperty(IMG_GIF, "state", "hide");
}

static void _show_popup_msg(char* msg)
{
	GUI_SetProperty(TXT_POPUP, "string", msg);
	GUI_SetProperty(IMG_POPUP, "state", "show");
	GUI_SetProperty(TXT_POPUP, "state", "show");
	GUI_SetProperty(TEXT_PLAY_WAIT, "state", "hide");
}

static void _hide_popup_msg(void)
{
	GUI_SetProperty(IMG_POPUP, "state", "hide");
	GUI_SetProperty(TXT_POPUP, "state", "hide");
	GUI_SetProperty(TEXT_PLAY_WAIT, "state", "hide");
}

static int _iptv_play_state_draw(void *usrdata)
{
	if(s_play_state == s_play_state_bak)
		return 0;

	s_play_state_bak = s_play_state;

	if(s_play_state == PLAY_STATE_LOAD)
	{
		_hide_popup_msg();
		_iptv_play_show_gif();
	}
	else
	{
		_iptv_play_hide_gif();
		if(s_play_state == PLAY_STATE_ERROR)
		{
			_show_popup_msg(STR_ID_SERVER_FAIL);
		}
		else
		{
			_hide_popup_msg();
		}
	}

	return 0;
}


static void _iptv_state_refresh_start(void)
{
	if(0 != reset_timer(s_state_refresh_timer))
	{
		s_state_refresh_timer = create_timer(_iptv_play_state_draw, 300, NULL, TIMER_REPEAT);
	}
}

static void _iptv_state_refresh_stop(void)
{
	timer_stop(s_state_refresh_timer);
	s_play_state_bak = PLAY_STATE_NONE;

	_hide_popup_msg();
	_iptv_play_hide_gif();
}

static int _iptv_play_update_time(void *userdata)
{
	uint64_t cur_time_ms = 0;
	uint64_t total_time_ms = 0;
	int cur_sliderbar = 0;
	PlayTimeInfo  tinfo = {0};

	if(s_play_state == PLAY_STATE_RUNNING)
	{
		if(GxPlayer_MediaGetTime(PLAYER_FOR_IPTV, &tinfo) == GXCORE_SUCCESS)
		{
			cur_time_ms = tinfo.current;
			total_time_ms = tinfo.totle;
			s_total_prog_dur = total_time_ms;
			if(s_total_prog_dur < 0)
			{
				s_total_prog_dur = 0;
				cur_time_ms = 0;
			}
			else if(s_total_prog_dur > 0)
			{
				if(cur_time_ms > s_total_prog_dur)
				{
					cur_time_ms = s_total_prog_dur;
					cur_sliderbar = 100;
				}
				else
				{
					cur_sliderbar = (cur_time_ms * 100)/ s_total_prog_dur ;
				}
			}
		}
		s_cur_time_ms_bak = tinfo.current;
		s_cur_sliderbar_bak = cur_sliderbar;
	}
	return 0;
}

static void _iptv_play_update_infobar(void)
{
//	uint64_t cur_time_ms = 0;
//	uint64_t total_time_ms = 0;
	uint64_t cur_time_s = 0;
//	int cur_sliderbar = 0;
	struct tm *ptm = NULL;
	char buf_tmp[20] = {0};
//	status_t ret = GXCORE_ERROR;
	/*cur time*/
	cur_time_s = s_cur_time_ms_bak / 1000;
	cur_time_s += ((s_cur_time_ms_bak % 1000) == 0 ? 0 : 1);
	ptm = localtime((const time_t*)  &cur_time_s);
	sprintf(buf_tmp, "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);

}

static void _iptv_infobar_timer_stop(void)
{
	remove_timer(s_inforbar_refresh_timer);
	s_inforbar_refresh_timer = NULL;
	s_infobar_timer_cnt = 0;
}


static int _iptv_infobar_timer_cb(void *userdata)
{
	int show_sec = *(int*)userdata;

	s_infobar_timer_cnt++;
	if(s_infobar_timer_cnt == show_sec)
	{
		GUI_EndDialog(WND_IPTV_INFO);
	}
	else
	{
		_iptv_play_update_infobar();
	}

	return 0;
}

static void _iptv_infobar_timer_reset(void)
{
	static int show_sec = INFO_SHOW_SEC;

	s_infobar_timer_cnt = 0;
	if (reset_timer(s_inforbar_refresh_timer) != 0)
	{
		s_inforbar_refresh_timer = create_timer(_iptv_infobar_timer_cb, 1000, &show_sec, TIMER_REPEAT);
	}
}



static void _iptv_mute_exec(void)
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

static void _iptv_mute_draw(void)
{
	if (g_AppFullArb.state.mute == STATE_ON)
	{
#if MINI_16_BITS_OSD_SUPPORT
		GUI_SetProperty(IMG_MUTE_1, "state", "show");
#endif
		GUI_SetProperty(IMG_MUTE, "state", "show");
	}
	else
	{
#if MINI_16_BITS_OSD_SUPPORT
		GUI_SetProperty(IMG_MUTE_1, "state", "osd_trans_hide");
#endif
		GUI_SetProperty(IMG_MUTE, "state", "osd_trans_hide");
	}
	GUI_SetInterface("flush", NULL);
}

#if 0
static void _iptv_first_timer_stop(void)
{
	if(s_iptv_first_in_timer)
	{
		remove_timer(s_iptv_first_in_timer);
		s_iptv_first_in_timer = NULL;
	}
}

static int _iptv_fisrt_in_timeout(void* userdata)
{
	_iptv_first_timer_stop();
	//if(s_play_ops.get_url != NULL)
	{
	//	s_play_ops.get_url();
	}
	return 0;
}

static void _iptv_first_timer_reset(void)
{
	if (reset_timer(s_iptv_first_in_timer) != 0)
	{
		s_iptv_first_in_timer= create_timer(_iptv_fisrt_in_timeout, 100, NULL,TIMER_ONCE);
	}
}
#endif

#if 0
status_t app_iptv_start_play(char *url, IPTVPlayState play_state, int64_t start, bool show_info)
{
	GxMsgProperty_PlayerPlay iptv_play = {0};

	if(play_state == PLAY_STATE_ERROR)
	{
		s_play_state = play_state;
	}
	else
	{
		if(url == NULL)
			return GXCORE_ERROR;
		if(show_info)
			if(strcmp(GUI_GetFocusWindow(), WND_IPTV_PLAY) == 0)
				_iptv_play_show_infobar();

		sprintf(iptv_play.url, "%s", url);
		iptv_play.player = PLAYER_FOR_IPTV;
		iptv_play.start = start;
		app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)(&iptv_play));
	}
	return GXCORE_SUCCESS;
}
#endif

static status_t app_iptv_start_play(char *url, int64_t start)
{
    GxMsgProperty_PlayerPlay iptv_play = {0};

    if(url == NULL)
        return GXCORE_ERROR;

    strlcpy(iptv_play.url, url, sizeof(iptv_play.url));
    iptv_play.player = PLAYER_FOR_IPTV;
    iptv_play.start = start;
    app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)(&iptv_play));

    return GXCORE_SUCCESS;
}

static status_t app_iptv_stop_play(void)
{
    status_t ret = GXCORE_ERROR;
    GxMessage *msg;
    msg = GxBus_MessageNew(GXMSG_PLAYER_STOP);
    if (msg != NULL)
    {
        GxMsgProperty_PlayerStop *player_stop;
        player_stop = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerStop);
        if (player_stop != NULL)
        {
            player_stop->player = PLAYER_FOR_IPTV;
            GxBus_MessageSend(msg);
            ret = GXCORE_SUCCESS;
        }
    }

    return ret;
}

status_t app_iptv_play(IPTVPlayOps *play_ops)
{
    status_t ret = GXCORE_ERROR;

    if((play_ops == NULL) || (play_ops->prog_url == NULL))
        return GXCORE_ERROR;

    //GxPlayer_MediaStop(PLAYER_FOR_IPTV);
    _iptv_state_refresh_stop();
    s_play_state = PLAY_STATE_LOAD;
    memset(&s_play_ops, 0, sizeof(IPTVPlayOps));
    memcpy(&s_play_ops, play_ops, sizeof(IPTVPlayOps));
    if(GUI_CheckDialog(WND_IPTV_PLAY) == GXCORE_ERROR)
    {
        GUI_CreateDialog(WND_IPTV_PLAY);
        _iptv_mute_draw();
        //GUI_SetInterface("flush", NULL);
    }

    //_iptv_play_show_infobar();

    s_total_prog_dur = 0;
#if 0
    GUI_SetProperty(TEXT_START_TIME, "string", "00:00:00");
    GUI_SetProperty(TEXT_STOP_TIME, "string", "00:00:00");
#endif
    if((s_play_ops.pre_media.pre_media_type == IPTV_PRE_AV)
            && (s_play_ops.pre_media.pre_media_url != NULL))
    {
        //play AD video
        thiz_pre_play = true;
        ret = app_iptv_start_play(s_play_ops.pre_media.pre_media_url, 0);
    }
    else if((s_play_ops.pre_media.pre_media_type == IPTV_PRE_PIC)
            && (s_play_ops.pre_media.pre_media_name != NULL))
    {
        //play AD picture
        thiz_pre_play = true;
        app_send_msg_exec(APPMSG_IPTV_GET_AD_PIC, &s_play_ops.pre_media.pre_media_name);
    }
    else
    {
        thiz_pre_play = false;
        _iptv_state_refresh_start();
        ret = app_iptv_start_play(s_play_ops.prog_url, 0);
		GUI_CreateDialog(WND_IPTV_INFO);
    }

    return ret;
}

static void _iptv_play_status(GxMessage* msg)
{
    GxMsgProperty_PlayerStatusReport* player_status = NULL;
    player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);

    if((player_status == NULL) || (strcmp((char*)(player_status->player), PLAYER_FOR_IPTV) != 0))
    {
        return;
    }

    switch(player_status->status)
    {
        case PLAYER_STATUS_ERROR:
            printf("\n[%s]%d PLAYER_STATUS_ERROR, %d\n", __func__, __LINE__, player_status->error);

            if(player_status->error == PLAYER_ERROR_NO_DATA_SOURCE)
            {
                printf("\n[%s]%d PLAYER_STATUS_NO_DATA_SOURCE, %d\n", __func__, __LINE__, player_status->error);
                if((thiz_pre_play == true)
                        && (s_play_ops.pre_media.pre_media_url != NULL)
                        && (strcmp(s_play_ops.pre_media.pre_media_url, player_status->url) == 0))
                {
                    _iptv_state_refresh_start();
                    thiz_pre_play = false;
                    s_play_state = PLAY_STATE_LOAD;
                    app_iptv_start_play(s_play_ops.prog_url, 0);
					GUI_CreateDialog(WND_IPTV_INFO);
                }
                else
                {
                    s_play_state = PLAY_STATE_ERROR;
                }
            }
            else
            {
                if(thiz_pre_play == true)
                {
                    _iptv_state_refresh_start();
					GUI_CreateDialog(WND_IPTV_INFO);
                    thiz_pre_play = false;
                    s_play_state = PLAY_STATE_LOAD;
                }
                app_iptv_start_play(s_play_ops.prog_url, s_cur_time_ms_bak);
            }

            break;

        case PLAYER_STATUS_PLAY_END:
            {
                if((thiz_pre_play == true)
                        && (s_play_ops.pre_media.pre_media_url != NULL)
                        && (strcmp(s_play_ops.pre_media.pre_media_url, player_status->url) == 0))
                {
                    _iptv_state_refresh_start();
                    thiz_pre_play = false;
                    s_play_state = PLAY_STATE_LOAD;
                    app_iptv_start_play(s_play_ops.prog_url, 0);
					GUI_CreateDialog(WND_IPTV_INFO);
                }
                else
                {
                    void (*exit_cb)(void) = s_play_ops.play_ctrl.play_exit;

                    printf("\n[%s]%d PLAYER_STATUS_PLAY_END\n", __func__, __LINE__);
                    GUI_EndDialog(WND_IPTV_PLAY);
                    if(exit_cb != NULL)
                        exit_cb();
                }
            }
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
            if(thiz_pre_play == false)
            {
                s_play_state = PLAY_STATE_LOAD;
                printf("\n[%s]%d PLAYER_STATUS_START_FILL_BUF\n", __func__, __LINE__);
            }
            break;

        case PLAYER_STATUS_END_FILL_BUF:
            if(thiz_pre_play == false)
            {
                s_play_state = PLAY_STATE_RUNNING;
                printf("\n[%s]%d PLAYER_STATUS_END_FILL_BUF\n", __func__, __LINE__);
            }
            break;

        default:
            break;
    }
}

static void _iptv_avcodec_status(GxMessage* msg)
{
	GxMsgProperty_PlayerAVCodecReport *avcodec_status = NULL;
	avcodec_status = (GxMsgProperty_PlayerAVCodecReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerAVCodecReport);

	if(AVCODEC_ERROR == avcodec_status->acodec.state || AVCODEC_ERROR == avcodec_status->vcodec.state )
	{

	}
	else if(avcodec_status->vcodec.state  == AVCODEC_RUNNING)
    {
        if(thiz_pre_play == false)
            s_play_state = PLAY_STATE_RUNNING;
        else
            _hide_popup_msg();
    }
}

static void _iptv_ad_timer_stop(void)
{
    remove_timer(thiz_ad_pic_timer);
    thiz_ad_pic_timer = NULL;
}

static int _iptv_ad_timeout(void *userdata)
{
    _iptv_ad_timer_stop();

    GUI_SetInterface("clear_image", NULL);
    GUI_SetInterface("image_disable", NULL);

    _iptv_state_refresh_start();
	GUI_CreateDialog(WND_IPTV_INFO);
    thiz_pre_play = false;
    s_play_state = PLAY_STATE_LOAD;
    app_iptv_start_play(s_play_ops.prog_url, 0);

    return 0;
}

static void _iptv_ad_timer_start(void)
{
    if (reset_timer(thiz_ad_pic_timer) != 0)
	{
		thiz_ad_pic_timer = create_timer(_iptv_ad_timeout, AD_PIC_SHOW_SEC * 1000, NULL, TIMER_ONCE);
	}
}

void _iptv_play_ad_pic_done(GxMessage* msg)
{
    char **pic_path = GxBus_GetMsgPropertyPtr(msg, char*);

    if(*pic_path != NULL)
    {
        _hide_popup_msg();

        GDI_PlayImage(*pic_path,-1,-1);
		GUI_SetInterface("flush", NULL);
        GxCore_FileDelete(*pic_path);
        _iptv_ad_timer_start();
    }
    else
    {
        _iptv_state_refresh_start();
        thiz_pre_play = false;
        s_play_state = PLAY_STATE_LOAD;
        app_iptv_start_play(s_play_ops.prog_url, 0);
		GUI_CreateDialog(WND_IPTV_INFO);
    }
}

int app_iptv_play_msg_proc(GxMessage *msg)
{
    if(msg == NULL)
        return EVENT_TRANSFER_STOP;

    switch(msg->msg_id)
    {
        case GXMSG_PLAYER_STATUS_REPORT:
            _iptv_play_status(msg);
            break;

        case GXMSG_PLAYER_AVCODEC_REPORT:
            _iptv_avcodec_status(msg);
            break;

        case APPMSG_IPTV_AD_PIC_DONE:
            _iptv_play_ad_pic_done(msg);
            break;
		case GXMSG_NETWORK_PLUG_IN:
			_iptv_play_show_gif();
			 _hide_popup_msg();
			break;
		case GXMSG_NETWORK_PLUG_OUT:
			 _iptv_play_hide_gif();
			 _show_popup_msg(STR_ID_SERVER_FAIL);
			break;
	

        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}


int app_iptv_recovery_status(void)
{
	void (*exit_cb)(void);
	if(GUI_CheckDialog(WND_IPTV_PLAY) == GXCORE_SUCCESS)
	{
		exit_cb = s_play_ops.play_ctrl.play_exit;
        GUI_EndDialog(WND_IPTV_PLAY);
        if(exit_cb != NULL)
        {
            exit_cb();
        }
	}
	return 0;
}


SIGNAL_HANDLER int app_iptv_play_create(GuiWidget *widget, void *usrdata)
{
	app_av_setting_init();
	_iptv_play_load_gif();
	GUI_SetProperty(TEXT_PLAY_WAIT, "state", "show");
	s_cur_time_ms_bak = 0;
	s_cur_sliderbar_bak = 0;
	s_iptv_play_time_timer = create_timer(_iptv_play_update_time, 1000, NULL, TIMER_REPEAT);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_play_destroy(GuiWidget *widget, void *usrdata)
{
    app_iptv_stop_play();
    _iptv_ad_timer_stop();
    _iptv_state_refresh_stop();
    remove_timer(s_state_refresh_timer);
    s_state_refresh_timer = NULL;
	remove_timer(s_iptv_play_time_timer);
	s_iptv_play_time_timer = NULL;
    memset(&s_play_ops, 0, sizeof(IPTVPlayOps));
    _iptv_play_free_gif();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_play_keypress(GuiWidget *widget, void *usrdata)
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
                exit_cb = s_play_ops.play_ctrl.play_exit;
                GUI_EndDialog(WND_IPTV_PLAY);
                if(exit_cb != NULL)
                    exit_cb();
                break;

            case STBK_MENU:
                exit_cb = s_play_ops.play_ctrl.play_exit;
                GUI_EndDialog(WND_IPTV_PLAY);
                if(exit_cb != NULL)
                    exit_cb();
                break;

            case STBK_INFO:
				GUI_CreateDialog(WND_IPTV_INFO);
                break;

            case STBK_MUTE:
                _iptv_mute_exec();
                _iptv_mute_draw();
                break;

            case STBK_LEFT:
            case STBK_VOLDOWN:
                {
                    event->key.sym = APPK_VOLDOWN;

                    GUI_CreateDialog("wnd_volume_value");
                    GUI_SendEvent("wnd_volume_value", event);

                    _iptv_mute_draw();
                }
                break;

            case STBK_RIGHT:
            case STBK_VOLUP:
                {
                    event->key.sym = APPK_VOLUP;

                    GUI_CreateDialog("wnd_volume_value");
                    GUI_SendEvent("wnd_volume_value", event);

                    _iptv_mute_draw();
                }
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
                {
					GUI_EndDialog(WND_IPTV_INFO);
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

SIGNAL_HANDLER int app_iptv_play_got_focus(GuiWidget *widget, void *usrdata)
{
//	_iptv_state_refresh_start();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_play_lost_focus(GuiWidget *widget, void *usrdata)
{
//	_iptv_state_refresh_stop();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_info_create(GuiWidget *widget, void *usrdata)
{
	GUI_SetProperty(TEXT_MEDIA_TITLE, "string", s_play_ops.prog_name);
	_iptv_play_update_infobar();
	_iptv_infobar_timer_reset();
	s_infobar_show_flag = true;
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_info_destroy(GuiWidget *widget, void *usrdata)
{
	_iptv_infobar_timer_stop();
	s_infobar_show_flag = false;
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_info_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = (GUI_Event *)usrdata;

    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case STBK_EXIT:
            case STBK_MENU:
                if(s_play_state == PLAY_STATE_ERROR)
                {
                    void (*exit_cb)(void);
                    exit_cb = s_play_ops.play_ctrl.play_exit;
                    GUI_EndDialog(WND_IPTV_PLAY);
                    if(exit_cb != NULL)
                        exit_cb();
                }
                else
                {
					 GUI_EndDialog(WND_IPTV_INFO);
                }
                break;

            default:
				GUI_EndDialog(WND_IPTV_INFO);
                GUI_SendEvent(WND_IPTV_PLAY, event);
                break;
        }
    }

    return EVENT_TRANSFER_STOP;
}

#else
SIGNAL_HANDLER int app_iptv_play_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_iptv_play_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_iptv_play_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_iptv_play_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_iptv_play_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_iptv_info_keypress(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_iptv_info_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int app_iptv_info_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
#endif
