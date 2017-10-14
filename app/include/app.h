#ifndef __APP_H__
#define __APP_H__

#include <gxtype.h>
#include "gui_core.h"
#include "gui_timer.h"
#include "app_key.h"
#include "module/player/gxplayer_module.h"
#include "gxservices.h"
#include "gxoem.h"

#include "app_config.h"
#include "app_send_msg.h"

#include "pmp_setting.h"
#include "pmp_tags.h"
#include "pmp_explorer.h"
#include "pmp_spectrum.h"
#include "pmp_subtitle.h"
#include "pmp_lyrics.h"
#include "pmp_id3.h"

#include "file_view.h"
#include "file_edit.h"
#include "play_manage.h"
#include "play_pic.h"
#include "play_movie.h"
#include "play_music.h"
#include "play_text.h"
#include "app_open_file.h"
#include "app_utility.h"
#include "app_bsp.h"
#include "app_str_id.h"
#include "app_pop.h"

#include "comm_log.h"
#include "app_general.h"
#include "app_utility.h"
#include "app_audio.h"

#include "app_data_type.h"

#if (DLNA_SUPPORT > 0)
#include "app_dlna.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PMP_ATTACH_DVB

#ifdef PMP_ATTACH_DVB

#define APP_Printf_Blue(...)	{printf("\033[036m");\
					printf(__VA_ARGS__);\
					printf("\033[0m");}					
#define APP_Printf(...)	printf(__VA_ARGS__)

#define APP_FREE(x)	if(x){GxCore_Free(x);x=NULL;}

#define APP_CHECK_P(p, r) if(NULL == p){\
							printf("\033[033m");\
							printf("\n[%s: %d] %s\n", __FILE__, __LINE__, __FUNCTION__);\
							printf("%s is NULL\n", #p);\
							printf("\033[0m");\
							return r;}

#define APP_TIMER_ADD(timer, fun, timeout, flag) \
							if(timer){\
								reset_timer(timer);\
							 }else{\
								timer = create_timer(fun, timeout, 0, flag);}

#define APP_TIMER_REMOVE(x)	if(x){remove_timer(x);x=NULL;}

#endif
status_t app_block_msg_init(handle_t self);
status_t app_block_msg_destroy(handle_t self);
status_t app_msg_init(handle_t self);
status_t app_msg_destroy(handle_t self);
status_t app_init(void);
// for ca rolling display
status_t app_create_dialog(const char* window_name);
//1 need modify to other place----------------------------------------

status_t app_root(void);

#define EPG_NAME_LEN 20

typedef struct
{
	uint32_t prog_id;
	time_t  event_time;
	uint8_t epgName[EPG_NAME_LEN];
}BookProgEvent;

typedef enum
{
	STATUS_SCRAMBLE,
	STATUS_LOCK,
	STATUS_NO_SIGNAL,
	STATUS_NO_PROGRAM,
	STATUS_NORMAL,
	STATUS_RECORD
}StbStatus;

//1--------------------------------------------------------------


#ifdef __cplusplus
}
#endif

/////////////////////////////////////add here///////////////////////////////////////
#define GET_ARRAY_LEN(array,len) {len = (sizeof(array)/sizeof(char*));}

#define PVR_VALID
//#undef PVR_VALID

#define RF_VALID
//#undef RF_VALID

#define AGE_LIMIT_VALID
//#undef AGE_LIMIT_VALID

#define STANDBY_VALID
//#undef STANDBY_VALID

//Don't support this in Ecos
#define FILE_EDIT_VALID
#define MEDIA_FILE_EDIT_UNVALID
#define DISK_FORMAT_UNVALID

#ifdef ECOS_OS
#define FILE_EDIT_VALID
#undef DISK_FORMAT_UNVALID
#endif

/*typedef enum
{
	wnd_normal = 0,
	wnd_block,
	wnd_ok,
	wnd_cancle
}WndState;*/
typedef enum
{
	WND_EXEC = 0,
	WND_CANCLE,
	WND_OK 
}WndStatus;

/*typedef enum
{
	WND_TYPE_YOUTUBE = 0,
	WND_TYPE_IPTV,
	WND_TYPE_REDTUBE,
	WND_TYPE_YOUPORE
}WND_TYPE;*/

enum
{
	SERVICE_PLAYER = 0,
#if NETWORK_SUPPORT
	SERVICE_NETWORK_DEAMON,
#endif
	SERVICE_EPG,
	SERVICE_SEARCH,
	SERVICE_SI,
	SERVICE_BOOK,
	SERVICE_EXTRA,
	SERVICE_GUI_VIEW,
	SERVICE_FRONTEND,
	SERVICE_HOTPLUG,
#if WIFI_SUPPORT
	SERVICE_USBWIFI_HOTPLUG,
#endif
#if MOD_3G_SUPPORT
	SERVICE_USB3G_HOTPLUG,
#endif
	SERVICE_UPDATE,
	SERVICE_BLIND_SEARCH,
	SERVICE_APP_DEAMON,
#if (BOX_SERVICE > 0)
	SERVICE_BOX,
#endif
#if TKGS_SUPPORT
	SERVICE_TKGS,
#endif
#if AUTO_TEST_ENABLE
	SERVICE_AUTO_TEST,
#endif
#ifdef HDMI_HOTPLUG_SUPPORT
	SERVICE_HDMI_HOTPLUG,
#endif
	SERVICE_TOTAL
};
typedef char ubyte32[32]; 
typedef char ubyte64[64];

extern handle_t bsp_panel_handle;
extern handle_t bsp_gpio_handle;

extern handle_t g_app_msg_self;
extern GxServiceClass g_service_list[SERVICE_TOTAL]; 
extern int app_lang_set_menu_font_type(const char *widget_name);

#endif /* __APP_H__ */

