#include "app.h"

#if (DLNA_SUPPORT > 0)
#include "dlna.h"
#include "gxcore.h"
#include <string.h>

#if 0
static const char* s_DLNAPlayStart = "dlna_play_start";
static const char* s_DLNAPlayStop = "dlna_play_stop";
static const char* s_DLNAPlayPause = "dlna_play_pause";
static const char* s_DLNAPlayResume = "dlna_play_resume";
static const char* s_DLNAGetTransportInfo = "dlna_get_transport_info";
static const char* s_DLNATransportState = "transport_state";
#endif




static status_t play_start(void)
{
	//TODO:send json command
	return GXCORE_SUCCESS;
}

static status_t play_resume(void)
{
	//TODO:send json command
	return GXCORE_SUCCESS;
}

static status_t play_stop(void)
{
	//TODO:send json command
	return GXCORE_SUCCESS;
}

static status_t play_pause(void)
{
	//TODO:send json command
	return GXCORE_SUCCESS;
}

/**
 *设置音量到STB
 */
static status_t set_volume(int volume)
{
	//TODO:send json command
	return GXCORE_SUCCESS;
}

/**
 *获取STB当前的音量
 */
static status_t get_volume(int *volume)
{
	//TODO:send json command
	if (volume != NULL) {
		*volume = 0;

		return GXCORE_SUCCESS;
	}

	return GXCORE_ERROR;
}

/**
 *获取机顶盒当前的播放状态
 */
static status_t get_transport_info(const char *transportState)
{
	//TODO:send json command
	if (transportState != NULL) {
		transportState = "STOPPED";

		return GXCORE_SUCCESS;
	}

	return GXCORE_ERROR;
}

struct dlna_controller dcontroller = {
	.play_start = play_start,
	.play_resume = play_resume,
	.play_start = play_stop,
	.play_pause = play_pause,
	.set_volume = set_volume,
	.get_volume = get_volume,
	.get_transport_info = get_transport_info,
};



#endif
