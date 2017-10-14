/*****************************************************************************
* 			  CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_default_params.c
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.2.27	            shenbin         creation
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __GXAPP_DEFAULT_PARAMS__
#define __GXAPP_DEFAULT_PARAMS__

/* Includes --------------------------------------------------------------- */
#include "app_config.h"

/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif
#include "gui_core.h"
#include "app_key.h"
#include "module/config/gxconfig.h"
/* Exported Constants ----------------------------------------------------- */

//1 KEY
// first power on
#define POWER_ON_KEY		"power>first"

// Color Setting
#define BRIGHTNESS_KEY	"av>video_brightness"
#define CONTRAST_KEY	"av>video_contrast"
#define SATURATION_KEY	"av>video_saturation"

// Av Setting
#define AUDIO_VOLUME_SCOPE	"av>audio_scope"
#define AUDIO_VOLUME_KEY	PLAYER_CONFIG_AUDIO_VOLUME
// audio show different with volume to player, exchange will cause missmatch, so sotre 
#define AUDIO_MAP_VAL_KEY	"av>audio_map"
#define SOUND_MODE_KEY		PLAYER_CONFIG_AUDIO_TRACK
#define VOLUME_MODE_KEY	"av>volume_scope"
#define AUDIO_LEVEL_KEY		"av>audio_level"
#define MUTE_KEY			"av>mute"
/* VideoOutputInterface */
#define	ASPECT_MODE_KEY		 	PLAYER_CONFIG_VIDEO_ASPECT
#define	VIDEO_INTERFACE_KEY	    PLAYER_CONFIG_VIDEO_INTERFACE 
#define	TV_STANDARD_KEY		    "av>video_standard"
//#define	TV_STANDARD1_KEY		    PLAYER_CONFIG_VIDEO_RESOLUTION1
#define NTSC_TYPE_KEY          PLAYER_CONFIG_VIDEO_AUTO_NTSC
#define	SPDIF_OUTPUT_KEY		PLAYER_CONFIG_AUDIO_SPDIF_SOURCE
#define TV_SPDIF_KEY    PLAYER_CONFIG_AUDIO_AC3_MODE
#define SPDIF_CONF_KEY       "av>spdif_config"

// delay parameters
#define PP_OUTPUT_MPEG2_TIME 	PLAYER_CONFIG_VIDEO_MPEG2_COVER_FRAME   
#define PP_OUTPUT_H264_TIME     PLAYER_CONFIG_VIDEO_H264_COVER_FRAME

#define SCART_CONF_KEY       "av>scart_config"
#define TV_SCART_KEY            PLAYER_CONFIG_VIDEO_SCART_SOURCE1//"av>scart"
#define TV_SCART_SCREEN			PLAYER_CONFIG_VIDEO_SCART_SCREEN1

#define AD_CONF_KEY       	"av>ad_config"
#define	RF_MODE_KEY				"av>rf_mode"
#define TV_RATIO_KEY		    PLAYER_CONFIG_VIDEO_DISPLAY_SCREEN
#define TV_ADAPT_KEY	    PLAYER_CONFIG_VIDEO_AUTO_ADAPT 

// OSD Setting	
#define	OSD_LANG_KEY	"osd>osd_lang"
#define	TRANS_LEVEL_KEY	"osd>trans_level"
#define	INFOBAR_TIMEOUT_KEY		"osd>infobar_timeout"
#define	VOLUMEBAR_TIMEOUT_KEY	"osd>volumebar_timeout"

// Language Setting
//#define	MENU_LANG_KEY	"lang>osd_lang" equal OSD_LANG_KEY
#define	AUDIO_1ST_KEY	"lang>audio_1st"
#define	AUDIO_2ND_KEY	"lang>audio_2nd"
#define   SUBTILE_1ST_KEY "lang>sub_1st"
#define   SUBTILE_2ND_KEY "lang>sub_2nd"
#define	EPG_LANG_KEY	"lang>epg_lang"
#define	TTX_LANG_KEY	"lang>ttx_lang"
//priority sound track
#define PRIORITY_LANG_KEY GXBUS_SEARCH_LANG_PRIORITY

// Lock Setting	
#define MENU_LOCK_KEY	"lock>menu_lock"
#define	CHANNEL_LOCK_KEY	"lock>channel_lock"
#define	PARENTAL_RATING_KEY	"lock>parental_rating"
#define	PASSWORD_KEY0	"lock>password0"
#define	PASSWORD_KEY1	"lock>password1"
#define	PASSWORD_KEY2	"lock>password2"
#define	PASSWORD_KEY3	"lock>password3"
#define   MENU_BOOT_KEY	"lock>boot_lock"
#define   MENU_STAND3HOURS_KEY	"lock>stand3hours_lock"

// Local Coordinate	
#define LONGITUDE_KEY		"coordinate>longitude"
#define	LONGITUDE_DIRECT_KEY	"coordinate>longitude_direct"
#define	LATITUDE_KEY	"coordinate>latitude"
#define	LATITUDE_DIRECT_KEY	"coordinate>latitude_direct"

//time setting
#define TIME_DISP       "time>disp"
#define TIME_MODE	    "time>mode"
#define TIME_ZONE	    "time>zone"
#define TIME_HALF_ZONE	"time>halfzone"
#define TIME_SUMMER     "time>summer"
#if THREE_HOURS_AUTO_STANDBY
#define TIME_AUTO_STANDBY    "time>auto_standby"
#endif

//LOG SET
#define LOG_OUT_PUT_TYPE   "log>out_put_type"
#define LOG_DISK_SELECT   "log>disk_select"
// PVR Control
#define PVR_PLAYER_TMS_KEY  PLAYER_CONFIG_PVR_TIME_SHIFT_FLAG      
#define PVR_TIMESHIFT_KEY   "pvr>tms_flag"
#define PVR_FILE_SIZE_KEY   PLAYER_CONFIG_PVR_RECORD_VOLUME_SIZE
#define PVR_DURATION_KEY    "pvr>duration" 
#define PVR_PARTITION_KEY   "pvr>partition"
#define PVR_TRUE_DIR_NAME_KEY   "pvr>true_dir_name"
#define PVR_SECTIONRECORD_KEY   "pvr>section_record"

#define PLAYER_PVR_RECORD_EXIST_OVERLAY  PLAYER_CONFIG_PVR_RECORD_EXIST_OVERLAY
/*subtitle*/
#define SUBTITLE_MODE_KEY   "subtitle>mode"
#define SUBTITLE_LANG_KEY    "subtitle>lang"

//nit 
#define SEARCH_NIT_LOOP_MODE		GXBUS_SEARCH_NIT_LOOP
#define APP_SEARCH_NIT_LOOP_YES 	GXBUS_SEARCH_NIT_LOOP_YES ///<nit搜索时进行tp的循环搜索
#define APP_SEARCH_NIT_LOOP_NO 	GXBUS_SEARCH_NIT_LOOP_NO  ///<nit搜索时不进行tp的循环搜索

//dts
#define APP_SEARCH_DTS_SUPPORT  GXBUS_SEARCH_DTS_SUPPORT
#define APP_GXBUS_SEARCH_DTS_NO GXBUS_SEARCH_DTS_NO

#if EX_SHIFT_SUPPORT
#define EXSHIFT_KEY		"ca>exshift_enable"
// second
#define EXSHIFT_TIME		"ca>exshift time"
#define EXSHIFT_DEFAULT			0
#define EXSHIFT_TIME_DEFAULT			0
#endif

#define	UART_BAUD_KEY		"uart>baud_rate_limit"
// 1: UPPER LIMIT; 0:LOWER LIMIT
#define	UART_BAUD_RATE_LIMIT	0 

//freeze 
#define FREEZE_SWITCH	PLAYER_CONFIG_VIDEO_FREEZE_SWITCH

//bus lock circle
#define FRONTEND_WATCH_TIME_KEY FRONTEND_CONFIG_WATCH_TIME

// search mode
#define SEARCH_MODE_KEY "search>mode"

// player for play file, the MAX memery pole
#define PLAY_RESERVED_MEMSIZE	PLAYER_CONFIG_PLAY_RESERVED_MEMSIZE

// pp mode control, if the video size is larger than the default size, the PP mode is closed.
#define PLAY_PP_MODE_MAX_WIDTH	PLAYER_CONFIG_VIDEO_PP_MAX_WIDTH
#define PLAY_PP_MODE_MAX_HEIGHT	PLAYER_CONFIG_VIDEO_PP_MAX_HEIGHT
#define DEFAULT_PP_SUPPORT_WIDTH    1920
#define DEFAULT_PP_SUPPORT_HEIGHT   1088

#define FAV_INIT_KEY		"fav>rename"

#define T2_COUNTRY  "osd>t2_country"
#define T2_COUNTRY_VALUE        2
#define DVBC_FREQ_KEY  "search_dvbc>main_frequency"
#define DVBC_FREQ_VALUE  299
#define DVBC_SYM_KEY  "search_dvbc>symbol_rate"
#define DVBC_SYM_VALUE  6875
#define DVBC_QAM_KEY  "search_dvbc>qam"
#define DVBC_QAM_VALUE  2
//1 VALUE
// first power on
#define POWER_ON		0    /*0.not power on/1.already power on*/

// Color Setting
#define BRIGHTNESS	50
#define CONTRAST	50
#define SATURATION	50

// Av Setting
#define AUDIO_SCOPE		1	/*0.channel/1.global*/
#define AUDIO_VOLUME	51	/*0~100*/
#define SOUND_MODE      3	/*0.mono/1.L/2.R/3.Stereo*/
#define VOLUME_SCOMPE_      0	/*0.global/1.channelo*/
#define AUDIO_LEVEL		1	/*0.Low/1.Mid/2.High*/
#define MUTE			0	/*0.unMute/1.mute*/
#define	ASPECT_MODE		0 	/*0.atuo/1.Letter Box/2.Pan & scan*/
#define VIDEO_MOSAIC_GATE		100
// VIDEO OUTPUT DELAY PARA
#define MPEG_AV_DELAY_FRAME		6
#define H264_AV_DELAY_FRAME		1


#define	VIDEO_INTERFACE_SCART VIDEO_OUTPUT_HDMI|VIDEO_OUTPUT_SCART1
#define	VIDEO_INTERFACE_MODE VIDEO_OUTPUT_HDMI|VIDEO_OUTPUT_RCA
// not support the "VIDEO_OUTPUT_HDMI_1080P_50HZ" and "VIDEO_OUTPUT_HDMI_1080P_60HZ"
#define	VIDEO_INTERFACE VIDEO_OUTPUT_HDMI | VIDEO_OUTPUT_RCA
#define	TV_STANDARD	    VIDEO_OUTPUT_HDMI_1080I_60HZ

#define NTSC_TYPE      VIDEO_OUTPUT_NTSC_M //VIDEO_OUTPUT_NTSC_M, VIDEO_OUTPUT_NTSC_443

#define	SPDIF_OUTPUT	3	/*0.PCM/1.ac3 / 2.EAC3 /3.Auto*/
#define	AC3_BYPASS		3	/*1.BYPASS; 0.not bypass*/

#define	RF_MODE			0	/*0.PAL BG/1.PAL I/2.PAL DK/3.NTSC M*/
#define TV_RATIO		0   /*0.4:3/1.16:9*/
#define TV_ADAPT_AUTO 	1  /*0 don't auto 1 auto*/
#define TV_ADAPT_NOAUTO 	0  /*0 don't auto*/
#define TV_SCART        1  //  0 CVBS, 1 RGB
#define SCART_CONF      0 //0 off; 1 on
#define SPDIF_CONF      0 //0 off; 1 on

#define	AD_CONFIG		0	/*1.enable; 0.disable*/

#define FAV_INIT_VALUE 0

//3: 0-25 Ó³Éä³É0-75 4: 0-25 Ó³Éä³É0-100 
#define VOLUME_NUM (4)

// OSD Setting	
#define OSD_TRANS_100PERCENT 255
#define OSD_TRANS_30PERCENT 78*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_40PERCENT 102*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_50PERCENT 128*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_60PERCENT 156*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_70PERCENT 178*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_80PERCENT 204*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_90PERCENT 230*OSD_TRANS_100PERCENT/255

#define	OSD_LANG	0	/*0.English 1.Chinese*/
#define   EPG_LANG   LANG_MAX
#define   TTX_LANG   LANG_MAX
#define	TRANS_LEVEL	OSD_TRANS_90PERCENT /*0~255*/
#define	INFOBAR_TIMEOUT	3	/*1~8*/
#define	VOLUMEBAR_TIMEOUT	3	/*1~8*/
#define	PVRBAR_TIMEOUT	8	

//priority sound track
#define PRIORITY_LANG_VALUE "eng_chs" //AUDIO_1ST & AUDIO_2ND

// Lock Setting	
#define MENU_LOCK	    0	/*0.unlock/1.lock*/
#define	CHANNEL_LOCK	1	/*0.unlock/1.lock*/
#define PARENTAL_RATING	17	/*0~17, 0:lock all, 17:view all, 1~16: 3years~18years*/
#define BOOT_LOCK	    0	/*0.unlock/1.lock*/
#define	STAND3HOUR_LOCK	0	/*0.unlock/1.lock*/
#define	PASSWORD_1	('0')	/*KEY_0*/
#define	PASSWORD_2	('0')	/*KEY_0*/
#define	PASSWORD_3	('0')	/*KEY_0*/
#define	PASSWORD_4	('0')	/*KEY_0*/
#define  GRNERAL_PASSWD  "8765"
  
// Local Coordinate	
#define LONGITUDE	1202	/*-180~180,the real value is LONGITUDE/10.0*/
#define	LONGITUDE_DIRECT	0	/*0.east/1.west*/
#define	LATITUDE	303	/*-90~90,the real value is LATITUDE/10.0*/
#define	LATITUDE_DIRECT	1	/*0.south/1.north*/

//time setting
#define TIME_DISP_VALUE 0   /* 0.ymd/1.dmy */
#define	TIME_MODE_VALUE	0	/*0.GTM/1.LOCAL*/
#define TIME_ZONE_VALUE	0	/*-12 ~ 12*/
#define TIME_HALF_ZONE_VALUE	0
#define TIME_SUMMER_VALUE	0	/*0.off/1.on*/
#if THREE_HOURS_AUTO_STANDBY
#define TIME_AUTO_STANDBY_VALUE	0	/*0.off/1.on*/
#endif

// pvr control
#define PVR_TIMESHIFT_FLAG	    0 /*0.time shift off / 1.time shift on*/
#define PVR_SECTIONRECORD_FLAG 0 /*0.section record off / 1.section record on*/
#define PVR_FILE_SIZE_VALUE     2048/*1024 == 1G ,2048 == 2G, 4096 == 4G*/
#define PVR_DURATION_VALUE      0   /* 0-2Hour ...*/
#ifdef ECOS_OS
#define PVR_PARTITION          "/dev/usb01"
#endif
#ifdef LINUX_OS
#define PVR_PARTITION          "/dev/sda1"
#define PVR_TRUE_DIR_NAME          "/SunPvr"
#endif
/*subtitle*/
#define SUBTITLE_MODE_VALUE	0	/*0.subtitle mode off / 1.dvb subtitle 2.ttx subtitle*/
#define SUBTITLE_LANG_VALUE	0	/*0-off 1-N: lang sel*/

/*freeze*/
#if MINI_256_COLORS_OSD_SUPPORT
#define FREEZE_SWITCH_VALUE   0    /*0. unfreeze / 1. freeze*/
#else
#define FREEZE_SWITCH_VALUE   1    /*0. unfreeze / 1. freeze*/
#endif

#ifdef ECOS_OS
#define MOD_TUNER    "|4:2:0x80:41:0:0xC6:&0:1:7:0:0"

#endif

#define USER_BUS_PMT_INFO
//#define MULTI_TP_EPG_INFO
#define MENCENT_FREEE_SPACE
#define LOOP_TMS_SUPPORE
#define HDMI_HOTPLUG_SUPPORT
//#define REC_FULL_RECALL_SUPPORT
//#define AUTO_CHANGE_FIRST_AUDIO_SUPPORT
//#define AUDIO_DESCRIPTOR_SUPPORT


#define FRONTEND_WATCH_TIME_VALUE 100//1500
#define FRONTEND_WATCH_TIME_COUNT 15
#define SEARCH_MODE_VALUE   0

#define SYS_MAX_SAT   (100)
#define SYS_MAX_TP    (3000)
#define SYS_MAX_PROG    (5000)
#ifdef ECOS_OS
#define DEFAULT_DB_PATH	NULL
#else
#define DEFAULT_DB_PATH	"/home/root/default_data.db"
#endif

//for PROGRAM CODE CHARGE
#define SEARCH_PROGRAM_CODE GXBUS_SEARCH_UTF8
#define	SEARCH_PROGRAM_CODE_UTF8 GXBUS_SEARCH_UTF8_YES

//1 CONTENT
#define BRIGHTNESS_0	0
#define BRIGHTNESS_1	10
#define BRIGHTNESS_2	20
#define BRIGHTNESS_3	30
#define BRIGHTNESS_4	40
#define BRIGHTNESS_5	50
#define BRIGHTNESS_6	60
#define BRIGHTNESS_7	70
#define BRIGHTNESS_8	80
#define BRIGHTNESS_9	90
#define BRIGHTNESS_10	100


#define CONTRAST_0	0
#define CONTRAST_1	10
#define CONTRAST_2	20
#define CONTRAST_3	30
#define CONTRAST_4	40
#define CONTRAST_5	50
#define CONTRAST_6	60
#define CONTRAST_7	70
#define CONTRAST_8	80
#define CONTRAST_9	90
#define CONTRAST_10	100


#define SATURATION_0	0
#define SATURATION_1	10
#define SATURATION_2	20
#define SATURATION_3	30
#define SATURATION_4	40
#define SATURATION_5	50
#define SATURATION_6	60
#define SATURATION_7	70
#define SATURATION_8	80
#define SATURATION_9	90
#define SATURATION_10	100



#define AUDIO_TRACK_L	0
#define AUDIO_TRACK_R	1
#define AUDIO_TRACK_STERO	2

#define AUDIO_LEVEL_LOW	0
#define AUDIO_LEVEL_MID	1
#define AUDIO_LEVEL_HIGH	2

#define MUTE_DISABLE  0
#define MUTE_ENABLE  1


#define SCREEN_RATIO_4X3	0
#define SCREEN_RATIO_16X9	1

#define SCREEN_TYPE_AUTO 0
#define SCREEN_TYPE_SCAN  1
#define SCREEN_TYPE_LETTER_BOX  2

#define AV_OUTPUT_SCART_CVBS	0
#define AV_OUTPUT_SCART_RGB	1
#define AV_OUTPUT_S_VIDEO		2

#define SPDIF_OUTPUT_OFF	0
#define SPDIF_OUTPUT_PCM	1
#define SPDIF_OUTPUT_DOLBY_DIGITAL 2


#define	RF_MODE_PAL_BG	0
#define	RF_MODE_PAL_I	1
#define	RF_MODE_PAL_DK	2
#define	RF_MODE_NTSC_M	3

#define MENU_LOCK_DISABLE	0
#define MENU_LOCK_ENABLE	1

#define CHANNEL_LOCK_DISABLE	0
#define CHANNEL_LOCK_ENABLE	1

#define REDIO_RECODE_SUPPORT	1

//NetWork Setting

#define WIFI_STATUS_KEY "network>status"
#define WIFI_STATUS_DEFAULT "none"

#define DEV_WIFI_MODE_KEY "network>wifimode"
#define DEV_3G_MODE_KEY "network>3gmode"
#define IP_TYPE_KEY  "network>iptype"
#define IP_ADDR_KEY  "network>ipaddr"
#define NET_MASK_KEY "network>netmask"
#define GATE_WAY_KEY "network>gateway"
#define DNS1_KEY     "network>dns1"
#define DNS2_KEY     "network>dns2"

#define DEV_MODE_DEFALT 1   //1:open 0:close
#define IP_TYPE_DEFALT  1   //1:dhcp 0:static
#define IP_ADDR_DEFALT  "0.0.0.0"
#define NET_MASK_DEFALT "0.0.0.0"
#define GATE_WAY_DEFALT "0.0.0.0"
#define DNS1_DEFALT     "8.8.8.8"
#define DNS2_DEFALT     "208.67.222.222"

#define AP0_DEV_KEY "network>ap0dev"
#define AP0_SSID_KEY "network>ap0ssid"
#define AP0_MAC_KEY "network>ap0mac"
#define AP0_PSK_KEY "network>ap0psk"

#define AP_DEV_DEFAULT "ra0"
#define AP_SSID_DEFAULT "NO NAME-"
#define AP_MAC_DEFAULT "00:00:00:00:00:00"
#define AP_PSK_DEFAULT ""

#ifdef NETAPPS_SUPPORT
#ifdef YOUTUBE_SUPPORT
#define YOUTUBE_REGION_KEY	"netapps>youtube_region"
#define YOUTUBE_RESOLUTION_KEY	"netapps>youtube_resolution"
#define YOUTUBE_DEFAULT_REGION 0
#define YOUTUBE_DEFAULT_RESOLUTION 0
#endif
#ifdef WEATHER_SUPPORT
#define WEATHER_LANGUAGE_KEY	"netapps>weather_lang"
#define WEATHER_METRIC_KEY	"netapps>weather_metric"
#define WEATHER_CURRENTCITY_KEY "netapps>weather_curcity"
#define WEATHER_DEFAULT_LANGUAGE 0
#define WEATHER_DEFAULT_METRIC 1
#define WEATHER_DEFAULT_CURCITY 0
#endif
#if MAP_SUPPORT
#define MAP_LANGUAGE_KEY	"netapps>map_lang"
#define MAP_MAPTYPE_KEY	"netapps>map_maptype"
#define MAP_LATITUDE_KEY   "netapps>map_latitude"
#define MAP_LONGITUDE_KEY   "netapps>map_longitude"
#define MAP_DEFAULT_LANGUAGE 0
#define MAP_DEFAULT_MAPTYPE 0
#define MAP_DEFAULT_LATITUDE 22.5430960
#define MAP_DEFAULT_LONGITUDE 114.0578650
#endif

#define CLOUD_ID_KEY    "cloud>cloud_id"
#define CLOUD_PASSWD_KEY    "cloud>cloud_passwd"
#define CLOUD_MAX_ID_LEN    (32)
#define CLOUD_MAX_PASSWD_LEN    (32)
#define CLOUD_DEFAULT_ID    "iptvtest"
#define CLOUD_DEFAULT_PASSWD   "88156088"
#endif


//The SIZE unit
#define SIZE_UNIT_K    (uint64_t)(1024) //powers of 1024
#define SIZE_UNIT_M    (uint64_t)(SIZE_UNIT_K * SIZE_UNIT_K)
#define SIZE_UNIT_G    (uint64_t)(SIZE_UNIT_M * SIZE_UNIT_K)
#define SIZE_UNIT_T    (uint64_t)(SIZE_UNIT_G * SIZE_UNIT_K)

#define UPGRADE_AUTO_SELECT_SUPPORT 1
#ifdef __cplusplus
}
#endif

#endif /* __GXAPP_DEFAULT_PARAMS__ */
/* End of file -------------------------------------------------------------*/


