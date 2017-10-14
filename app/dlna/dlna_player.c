#include "app.h"
#include "app_send_msg.h"

#if (DLNA_SUPPORT > 0)
#include "dlna.h"
#include "gxcore.h"
#include <string.h>

#define DLNA_PLAYER_AV   "player_dlna_av"

#define DLNA_PLAYER_PIC  "player_dlna_pic"
#define DLNA_PIC_PATH    "/tmp"
#define DLNA_PIC_NAME    "dlna"

#define DLNA_SUFFIX_PIC		"jpg,JPG,jpeg,JPEG,bmp,BMP,"
#define DLNA_SUFFIX_AUDIO	"mp3,MP3,aac,AAC,m4a,M4A,ac3,AC3"
#define DLNA_SUFFIX_VIDEO	"rm,RM,rmvb,RMVB,mkv,MKV,real,REAL,avi,AVI,ts,TS,mp4,MP4,mpg,MPG,flv,FLV,dat,DAT,vob,VOB,mov,MOV,dvr,MPEG,mpeg,m2ts,M2TS, mpg_ts, mpg_ps"
#define DLNA_TEXT_NAME		"txt_dlna_name"

#define DLNA_SHOW_PP()     do{\
                GUI_SetProperty("wnd_full_screen", "back_ground", "null");\
                GUI_SetInterface("draw_now", NULL);\
                GUI_SetInterface("video_enable", NULL);\
}while(0)

#define DLNA_SHOW_SPP()   do{\
            GUI_SetInterface("flush", NULL);\
            GUI_SetInterface("video_disable", NULL);\
}while(0)

extern  void _show_dlna_title_menu(int status);
extern void _close_dlna_mp3_menu(void);
extern void app_volume_value_set(int volume);
extern uint32_t app_system_vol_get(void);
/********************************************************************
 *  URL PARSE FOR DLNA
********************************************************************/
//clear it later
static void play_prepare(const char *url, char *play_url, char *file_type, int type_len)
{
	static int mp3_draw_tag=0;
	//---------------------------------------------------------------
	// for qq video dlna
	if (strstr(url, "qq.com") != NULL) {
		printf("[DLNA] qq.com\n");
		DLNA_SHOW_PP();
		strcpy(play_url, url);
		strcpy(file_type, "mp4");

		GUI_SetProperty(DLNA_TEXT_NAME, "string", "qq.com");
		return;
	}
	//---------------------------------------------------------------

	char *pname = strrchr(url, '/');
	char *ptype = strrchr(url, '.');

	char *name = GxCore_Strdup((char *)(pname+1));
	char *type = GxCore_Strdup((char *)(ptype+1));

	if (strlen(type) < type_len)
		strncpy(file_type, type, type_len);

	printf("name = %s  type = %s\n", name, type);
	if (strstr(DLNA_SUFFIX_PIC, type) != NULL) {
		printf("[DLNA] picture\n");

#define BUF_MAX_LEN  (1024)
		char *buf = (char *)GxCore_Malloc(BUF_MAX_LEN);
		if (buf == NULL) {
			GxCore_Free(name);
			GxCore_Free(type);
			return;
		}

		system("killall wget");

		// get picture
		memset(buf, 0, BUF_MAX_LEN);
		sprintf(buf, "cd %s; wget -T 3 -O %s.%s %s ; sync ", DLNA_PIC_PATH, DLNA_PIC_NAME, type, url);
		system(buf);

		// set url
		memset(buf, 0, BUF_MAX_LEN);
		sprintf(buf, "%s/%s.%s", DLNA_PIC_PATH, DLNA_PIC_NAME, type);
		strcpy(play_url, buf);

		GxCore_Free(buf);
	} else if (strstr(DLNA_SUFFIX_AUDIO, type) != NULL) {
		printf("[DLNA] music\n");
		DLNA_SHOW_SPP();
		strcpy(play_url, url);
		//spectrum_create("dlna_canvas_spectrum", 0, 50, 700, 300);
		//spectrum_start();
		mp3_draw_tag =0xff;
		printf("__start d,=%d___",__LINE__);
	} else if (strstr(DLNA_SUFFIX_VIDEO, type) != NULL) {
		printf("[DLNA] movie\n");
		if(mp3_draw_tag == 0xff)
		{
			mp3_draw_tag = 0;
			_close_dlna_mp3_menu();
		}
		_show_dlna_title_menu(0);
		DLNA_SHOW_PP();
		strcpy(play_url, url);
	}
	else
		printf("[DLNA] NULL\n");

	GUI_SetProperty(DLNA_TEXT_NAME, "string", name);

	GxCore_Free(name);
	GxCore_Free(type);
}

/********************************************************************
 *  PLAYER FOR DLNA
********************************************************************/

status_t dlna_player_play(const char* url, const char* uriMetadata, int start_time_ms)
{
#define FILE_TYPE_LEN   (16)
	if(NULL == url) return GXCORE_ERROR;

	char file_type[FILE_TYPE_LEN] = {0};
	GxMessage *msg;
	GxMsgProperty_PlayerPlay *player_play;

	msg = GxBus_MessageNew(GXMSG_PLAYER_PLAY);
	if (msg == NULL)
		return GXCORE_ERROR;

	player_play = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerPlay);
	if (player_play == NULL)
		return GXCORE_ERROR;

    //printf("[%s]%d, %s\n", __FILE__, __LINE__, url);
	play_prepare(url, player_play->url, file_type, FILE_TYPE_LEN);

	player_play->player = DLNA_PLAYER_AV;
	player_play->start = start_time_ms;

	printf("[player] %s:%s:%lld\n", player_play->url, player_play->player, player_play->start);
	GxBus_MessageSend(msg);

	if (strstr(DLNA_SUFFIX_PIC, file_type) != NULL) {
		printf("[app dlna] draw now -----------\n");
                GUI_SetInterface("draw_now", NULL);
		DLNA_SHOW_SPP();
	}

	return GXCORE_SUCCESS;
}

status_t dlna_player_stop(void)
{
	GxMessage *msg;
	msg = GxBus_MessageNew(GXMSG_PLAYER_STOP);
	_show_dlna_title_menu(1);
	_close_dlna_mp3_menu();
	if (msg == NULL)
		return GXCORE_ERROR;

	GxMsgProperty_PlayerStop *player_stop;
	player_stop = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerStop);
	if (player_stop == NULL)
		return GXCORE_ERROR;

	player_stop->player = DLNA_PLAYER_AV;

	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);
	return GXCORE_SUCCESS;
}

status_t dlna_player_pause(void)
{
	GxMessage *msg;
	msg = GxBus_MessageNew(GXMSG_PLAYER_PAUSE);
	APP_CHECK_P(msg, GXCORE_ERROR);

	GxMsgProperty_PlayerPause *pause;
	pause = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerPause);
	APP_CHECK_P(pause, GXCORE_ERROR);
	pause->player = DLNA_PLAYER_AV;

	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);

	return GXCORE_SUCCESS;
}

status_t dlna_player_resume(void)
{
	GxMessage *msg;
	msg = GxBus_MessageNew(GXMSG_PLAYER_RESUME);
	APP_CHECK_P(msg, GXCORE_ERROR);

	GxMsgProperty_PlayerResume *resume;
	resume = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerResume);
	APP_CHECK_P(resume, GXCORE_ERROR);
	resume->player = DLNA_PLAYER_AV;

	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);

	return GXCORE_SUCCESS;
}

status_t dlna_player_volume_plus()
{
	GUI_Event event = {0};

	event.type = GUI_KEYDOWN;
	event.key.sym = STBK_VOLUP;

	GUI_CreateDialog("wnd_volume_value");
	GUI_SendEvent("wnd_volume_value", &event);
	return GXCORE_SUCCESS;
}

status_t dlna_player_volume_minus()
{
	GUI_Event event = {0};

	event.type = GUI_KEYDOWN;
	event.key.sym = STBK_VOLDOWN;

	GUI_CreateDialog("wnd_volume_value");
	GUI_SendEvent("wnd_volume_value", &event);
	return GXCORE_SUCCESS;
}

status_t dlna_player_volume_set(int volume)
{
    app_volume_value_set(volume);
	return GXCORE_SUCCESS;
}

status_t dlna_player_volume_get(int *pVolume)
{
	// TODO:从STB获取当前的音量，赋值给*pVolume
	if (pVolume != NULL)
    {
        *pVolume = app_system_vol_get();
	}
	return GXCORE_SUCCESS;
}

status_t dlna_player_seek(int32_t time_s)
{
	return GxPlayer_MediaSeek(DLNA_PLAYER_AV,1000*time_s,SEEK_ORIGIN_SET);
}

status_t dlna_get_player_status(PlayerStatusInfo* info)
{
	return GxPlayer_MediaGetStatus(DLNA_PLAYER_AV, info);
}

/* *
 *获取时间，单位MS
 * */
status_t dlna_av_get_time(uint64_t* current_ms, uint64_t* total_ms)
{
	if (current_ms != NULL && total_ms != NULL) {
		return GxPlayer_MediaGetTime(DLNA_PLAYER_AV, current_ms, total_ms);
	}
	return GXCORE_ERROR;
}

struct dlna_player dplayer = {
	.play      = dlna_player_play,
	.stop      = dlna_player_stop,
	.pause     = dlna_player_pause,
	.resume    = dlna_player_resume,
	.vol_set   = dlna_player_volume_set,
	.vol_get   = dlna_player_volume_get,
	.seek      = dlna_player_seek,
	.get_player_status = dlna_get_player_status,
	.get_position_info = dlna_av_get_time,
};
#endif
