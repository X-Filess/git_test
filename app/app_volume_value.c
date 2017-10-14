/*****************************************************************************
* 			  CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009-2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_full_screen.c
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.3.8	           shenbin         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "module/config/gxconfig.h"
#include "full_screen.h"

#define BMP_VOL_MUTE        "s_vol_mute.bmp"
#define BMP_VOL_UNMUTE      "s_vol.bmp"
#define IMG_VOL             "img_volume"
#define PROGBAR_VOL         "progbar_volume"
#define TXT_VOL             "txt_volume"

static int volume_bar_timeout(void* data);
extern status_t app_volume_scope_status(void);
extern int app_audio_prog_track_set_mode(int value,uint8_t mode);
extern int app_audio_volume_prog_get_mode(void);
// from app_mute_pause.c
static event_list* sp_timer = NULL;

/* static variable ------------------------------------------------------ */
static GxMsgProperty_PlayerAudioVolume s_Volume;
static GxMsgProperty_PlayerAudioVolume s_sys_volume;

#define MAX_DISPLAY_VOL (25)

// 0-40 Ó³Éä³É0-100
#define AUDIO_MAP_40(audio)	(((audio)*5)>>1)

#define AUDIO_MAP_25(audio)	((audio) * VOLUME_NUM)

// 0-50 to 0-75
#define AUDIO_MAP_50(audio) ((audio)/2 + (audio)) 

status_t app_system_vol_save(GxMsgProperty_PlayerAudioVolume val)
{
    if(val > MAX_DISPLAY_VOL*VOLUME_NUM)
        val = MAX_DISPLAY_VOL*VOLUME_NUM;
	GxBus_ConfigSetInt(AUDIO_VOLUME_KEY, val);

    return GXCORE_SUCCESS;
}

status_t app_system_vol_set(GxMsgProperty_PlayerAudioVolume val)
{
    if(val > MAX_DISPLAY_VOL*VOLUME_NUM)
        val = MAX_DISPLAY_VOL*VOLUME_NUM;
    s_sys_volume = val;
    app_send_msg_exec(GXMSG_PLAYER_AUDIO_VOLUME, (void*)(&s_sys_volume));

    return GXCORE_SUCCESS;
}

status_t app_system_vol_set_val(GxMsgProperty_PlayerAudioVolume val)
{
    s_sys_volume = val;
    return GXCORE_SUCCESS;
}

uint32_t app_system_vol_get(void)
{
    return s_sys_volume;
}

void app_volume_value_init(void)
{
	int32_t val = 0;
	if( app_volume_scope_status())
	{
		val = app_audio_volume_prog_get_mode();
	}
	else
	{
        val = app_system_vol_get();
	}
	if(val > MAX_DISPLAY_VOL*VOLUME_NUM)
	{
		s_Volume = MAX_DISPLAY_VOL;
	}
	else
	{
		s_Volume = val / VOLUME_NUM;
	}
	#if(MINI_16_BITS_OSD_SUPPORT == 1)
	char volume_buf[4] = {0};
	GUI_SetProperty(PROGBAR_VOL, "value", (void *)(&s_Volume));
	sprintf(volume_buf, "%d", s_Volume);
	GUI_SetProperty(TXT_VOL, "string", volume_buf);
	#endif
}

void app_volume_value_set(int volume)
{
	char volume_buf[4] = {0};

    app_system_vol_set(volume);
    app_system_vol_save(volume);

    if(GUI_CheckDialog("wnd_volume_value") == GXCORE_ERROR)
    {
        app_create_dialog("wnd_volume_value");
    }
    else
    {
        int init_value = INFOBAR_TIMEOUT;
        app_volume_value_init();
        GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);
        if (reset_timer(sp_timer) != 0)
        {
            // reset time failed, do
            sp_timer = create_timer(volume_bar_timeout, init_value*SEC_TO_MILLISEC, NULL, TIMER_ONCE);
        }
    }

	GUI_SetProperty(PROGBAR_VOL, "value", (void *)(&s_Volume));
	sprintf(volume_buf, "%d", s_Volume);
	GUI_SetProperty(TXT_VOL, "string", volume_buf);
}
/* Private functions ------------------------------------------------------ */
static void volume_change(uint16_t key)
{
    FullArb *arb = &g_AppFullArb;
	uint32_t audio_vol = 0;
	GxMsgProperty_PlayerAudioVolume dispValue = 0;
	char volume_buf[4];

	// when change volue, enable mute
	if ((key == STBK_LEFT) ||(key == STBK_VOLDOWN))
	{
		if (s_Volume > 1)
        {
			s_Volume--;
		}
        else
        {
			s_Volume = 0;
		}
	}
	else if ((key == STBK_RIGHT) || (key == STBK_VOLUP))
	{
		if (s_Volume >= MAX_DISPLAY_VOL)
        {
			s_Volume = MAX_DISPLAY_VOL;
		}
        else
        {
			s_Volume++;
		}
	}

    if(s_Volume > 0)
    {
		GUI_SetProperty(IMG_VOL, "img", BMP_VOL_UNMUTE);
        GUI_SetProperty("img_full_mute1", "state", "hide");
        GUI_SetProperty("img_full_mute", "state", "hide");
    }
    else
    {
		GUI_SetProperty(IMG_VOL, "img", BMP_VOL_MUTE);
        GUI_SetProperty("img_full_mute1", "state", "show");
        GUI_SetProperty("img_full_mute", "state", "show");
        GUI_SetProperty("img_full_mute", "img", "s_vol_mute.bmp");
    }

	// player do sth
	audio_vol = AUDIO_MAP_25(s_Volume);
	app_system_vol_set(audio_vol);
	
    if (arb->state.mute == STATE_ON)
    {
        arb->exec[EVENT_MUTE](arb);
        arb->draw[EVENT_MUTE](arb);
    }

    dispValue = s_Volume;
	GUI_SetProperty(PROGBAR_VOL, "value", (void *)(&(dispValue)));

	// set number value
	memset(volume_buf, 0, 4);
	sprintf(volume_buf, "%d", s_Volume);
	GUI_SetProperty(TXT_VOL, "string", volume_buf);
}

static int volume_bar_timeout(void* data)
{
	GUI_EndDialog("wnd_volume_value");
	GUI_SetProperty("wnd_volume_value", "draw_now", NULL);
	GUI_SetProperty("img_full_mute1", "state", "hide");
	GUI_SetProperty("img_full_mute", "state", "hide");
	return GXCORE_SUCCESS;
}

static void _left_right_key_press(uint16_t key)
{
	int init_value = INFOBAR_TIMEOUT;

	//GxBus_ConfigGetInt(VOLUMEBAR_TIMEOUT_KEY, &init_value, VOLUMEBAR_TIMEOUT);
	GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);

	if (reset_timer(sp_timer) != 0) {
		// reset time failed, do
		sp_timer = create_timer(volume_bar_timeout, init_value*SEC_TO_MILLISEC, NULL, TIMER_ONCE);
	}

	volume_change(key);
}

/* Exported functions ----------------------------------------------------- */
SIGNAL_HANDLER int app_volume_value_create(const char* widgetname, void *usrdata)
{
    int pos_x;
    int pos_y;
	int init_value = INFOBAR_TIMEOUT;

    if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
    {
	#if(MINI_16_BITS_OSD_SUPPORT == 1)
		pos_x = 224;
        pos_y = 290;
	#else
        pos_x = 290;
        pos_y = 350;
	#endif
        GUI_SetProperty("wnd_volume_value", "move_window_x", &pos_x);
        GUI_SetProperty("wnd_volume_value", "move_window_y", &pos_y);
    }
    else
    {
	#if(MINI_16_BITS_OSD_SUPPORT == 1)
		pos_x = 224;
        pos_y = 420;
	#else
        pos_x = 290;
        pos_y = 530;
	#endif
        GUI_SetProperty("wnd_volume_value", "move_window_x", &pos_x);
        GUI_SetProperty("wnd_volume_value", "move_window_y", &pos_y);
    }

	s_Volume = 0;
	app_volume_value_init();

	GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);
    if (reset_timer(sp_timer) != 0) {
		// reset time failed, do
		sp_timer = create_timer(volume_bar_timeout, init_value*SEC_TO_MILLISEC, NULL, TIMER_ONCE);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_volume_value_destroy(const char* widgetname, void *usrdata)
{
	if(sp_timer != NULL)
	{
		remove_timer(sp_timer);
		sp_timer = NULL;
	}

//	s_Volume = AUDIO_MAP(s_Volume);
	if(app_volume_scope_status())
	{
		 app_audio_prog_track_set_mode(AUDIO_MAP_25(s_Volume),0xfe);
	}
	else
	{
        app_system_vol_save(AUDIO_MAP_25(s_Volume));
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_volume_value_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	GxMessage * msg;
	
	event = (GUI_Event *)usrdata;	
	msg = (GxMessage*)event->msg.service_msg;
	
	if(GXMSG_PLAYER_SPEED_REPORT == msg->msg_id || GXMSG_PLAYER_STATUS_REPORT == msg->msg_id)
	{
		if(GXCORE_SUCCESS == GUI_CheckDialog("win_movie_view"))
			GUI_SendEvent("win_movie_view", event);	
		if(GXCORE_SUCCESS == GUI_CheckDialog("win_music_view"))
			GUI_SendEvent("win_music_view", event);	
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_volume_value_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
            case VK_BOOK_TRIGGER:
			    GUI_EndDialog("after wnd_full_screen");
			break;

	#if MINI_16_BITS_OSD_SUPPORT
		    case APPK_BACK:
		    case APPK_MENU:
				volume_bar_timeout(NULL);
				const char *wnd = GUI_GetFocusWindow();
				if(0 == strcasecmp("wnd_full_screen", wnd))
				{
					GUI_SendEvent(wnd, event);
				}
				break;
    #endif
			case STBK_VOLDOWN:
			case STBK_VOLUP:
			case STBK_LEFT:
			case STBK_RIGHT:
				_left_right_key_press(event->key.sym);
				break;
			default:
				volume_bar_timeout(NULL);
				GUI_SendEvent(GUI_GetFocusWindow(), event);
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

/* End of file -------------------------------------------------------------*/

