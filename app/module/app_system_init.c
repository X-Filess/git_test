/*****************************************************************************
* 			  CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009-2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_system_init.c
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
#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "sys_setting_manage.h"
#include "app_utility.h"
#include "full_screen.h"
#include "app_config.h"
#include "gxcore.h"
#include "tree.h"
#include "parser.h"
#include "app_book.h"
#if TKGS_SUPPORT
#include "app_tkgs_upgrade.h"
#endif
#ifdef NETAPPS_SUPPORT
#include <gx_apps.h>
#endif

#include <fcntl.h>
#if (DEMOD_DVB_S > 0)
extern void app_sat_sel_clear(void);
extern void app_sat_list_del_sel_status(void);
extern void app_sat_list_close_lnb(void);
#endif
extern void app_volume_value_init(void);
extern void app_init_sleep_set(void);
extern VideoOutputMode app_av_get_cvbs_standard(VideoOutputMode vout_mode);
extern status_t app_system_vol_save(GxMsgProperty_PlayerAudioVolume val);
extern uint32_t app_system_vol_get(void);
extern status_t app_system_vol_set(GxMsgProperty_PlayerAudioVolume val);
extern status_t app_system_vol_set_val(GxMsgProperty_PlayerAudioVolume val);

#if TKGS_SUPPORT
extern status_t app_tkg_lcn_flush(void);
extern void app_tkgs_sort_channels_by_lcn(void);
extern void app_tkgs_init_operatemode(void);
#endif

/* Private functions ------------------------------------------------------ */
// set the value that the same with player service
static void app_system_video_init(void)
{
	#define MAX_COLOR_VALUE 100
	int32_t init_value;
 	int32_t chroma;
	int32_t brightness;
	int32_t contrast;
	GxMsgProperty_PlayerDisplayScreen screen_ratio;
	GxMsgProperty_PlayerVideoAspect aspect; 
	GxMsgProperty_PlayerVideoColor color;
	GxMsgProperty_PlayerPVRConfig  pvrConfig = {0};

	// 4:3  16:9, for screen size
	GxBus_ConfigGetInt(TV_RATIO_KEY, &init_value, TV_RATIO);
	screen_ratio.aspect = init_value;
	screen_ratio.xres = APP_XRES;
	screen_ratio.yres = APP_YRES;
	GxPlayer_SetDisplayScreen(screen_ratio);

	// Letter / Pilar box  pan scan
	GxBus_ConfigGetInt(ASPECT_MODE_KEY, &init_value, ASPECT_MODE);
    	aspect = init_value;
    	GxPlayer_SetVideoAspect(aspect);

	//saturation brightness contrast
	GxBus_ConfigGetInt(SATURATION_KEY, &chroma, SATURATION);
	GxBus_ConfigGetInt(BRIGHTNESS_KEY, &brightness, BRIGHTNESS);
	GxBus_ConfigGetInt(CONTRAST_KEY, &contrast, CONTRAST);
	if(MAX_COLOR_VALUE < chroma)
	    chroma = MAX_COLOR_VALUE;
	if(MAX_COLOR_VALUE < brightness)
	    brightness = MAX_COLOR_VALUE;
	if(MAX_COLOR_VALUE < contrast)
	    contrast = MAX_COLOR_VALUE;

   // color.vout_id = 0;// default for hdmi
    color.interface = VIDEO_OUTPUT_HDMI;
	color.brightness = brightness;
	color.saturation = chroma;
	color.contrast = contrast;
    GxPlayer_SetVideoColors(color);
    color.interface = VIDEO_OUTPUT_RCA;
    GxPlayer_SetVideoColors(color);

	// freezon
	GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
	GxPlayer_SetFreezeFrameSwitch(init_value);
	
	GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &init_value, AUDIO_VOLUME);
    	if(init_value > 100)
    	{
        	init_value = 100;
    	}
       app_system_vol_set_val(init_value);
	GxPlayer_SetAudioVolume(init_value);

	GxBus_ConfigGetInt(PVR_FILE_SIZE_KEY, &init_value, PVR_FILE_SIZE_VALUE);
	GxBus_ConfigSetInt(PVR_FILE_SIZE_KEY, init_value);
	pvrConfig.volume_sizemb = init_value;
	GxPlayer_SetPVRConfig(&pvrConfig);

	return;
}

status_t app_system_gui_init(void)
{
	status_t ret = GXCORE_SUCCESS;
	GUI_Rect region_size = {0, 0, APP_XRES, APP_YRES};
	GxMsgProperty_PlayerVideoInterface Interface;
	GxMsgProperty_PlayerVideoModeConfig  mode_config;
	GxMsgProperty_PlayerVideoAutoAdapt auto_adapt = {0};
	int32_t init_value;
	//GxTime time;
	//GxTime time_end;
	//GxCore_GetTickTime(&time);
	//printf("app_system_gui_init-[%d]----%x\n",__LINE__,*(int*)0xa4800040);
		/*gui init*/
#ifdef ECOS_OS
	ret = GUI_Init("/dvb/theme/theme.xml");
#else
	ret = GUI_Init(WORK_PATH"theme/theme.xml");
#endif

	//GxCore_GetTickTime(&time_end);

	if(GXCORE_SUCCESS != ret)
	{
		printf("[GUI_Init] ERROR\n");
		return GXCORE_ERROR;
	}
	//GxCore_GetTickTime(&time);
	//this is spp region size
	GUI_SetInterface("image_size", &region_size);
	// this is osd region size
	GUI_SetInterface("interface_size", &region_size);

	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
	GUI_SetInterface("osd_language", g_LangName[init_value]); 

	GxBus_ConfigGetInt(TRANS_LEVEL_KEY, &init_value, TRANS_LEVEL);
	GUI_SetInterface("osd_alpha_global", &init_value);
	
#if THREE_HOURS_AUTO_STANDBY
		extern void app_set_auto_standby_time(int mode);
		app_set_auto_standby_time(1);
#endif

	//app_system_video_init();
	GxBus_ConfigGetInt(TV_ADAPT_KEY, &init_value, TV_ADAPT_NOAUTO);
	auto_adapt.enable = init_value;
	auto_adapt.pal = VIDEO_OUTPUT_PAL;
	auto_adapt.ntsc = VIDEO_OUTPUT_NTSC_M;
	GxPlayer_SetVideoAutoAdapt(&auto_adapt);

	//interface HDMI & RCA
	GxBus_ConfigGetInt(VIDEO_INTERFACE_KEY, &Interface, VIDEO_INTERFACE_MODE);
	GxPlayer_SetVideoOutputSelect(Interface);

	//printf("[%d]-2---0x%x, 0x%x, 0x%x\n",__LINE__,*(volatile unsigned int*)0xa4800040, *(volatile unsigned int*)0xa48000dc, *(volatile unsigned int*)0xa4800000);

    //standard
	GxBus_ConfigGetInt(TV_STANDARD_KEY, &init_value, TV_STANDARD);
    //HDMI
	mode_config.interface = VIDEO_OUTPUT_HDMI;
	mode_config.mode = init_value;
	GxPlayer_SetVideoOutputConfig(mode_config);

	app_system_video_init();
	#if 0
	 //for RCA1
	mode_config.mode = app_av_get_format_for_rca1(init_value);
	GxBus_ConfigGetInt(TV_SCART_SCREEN, &init_value, TV_RATIO);
	mode_config.config.scart.screen = init_value;
	//GxBus_ConfigGetInt(TV_SCART_KEY, &init_value, TV_SCART);
	mode_config.config.scart.source = init_value;
	GxBus_ConfigGetInt(SCART_CONF_KEY, &init_value, SCART_CONF);
	if(init_value == 0)
	{
		mode_config.interface = VIDEO_OUTPUT_RCA1;
	}
	else
	{
		mode_config.interface = VIDEO_OUTPUT_SCART1;
	}
#endif
    //RCA
	mode_config.interface = VIDEO_OUTPUT_RCA;
	mode_config.mode = app_av_get_cvbs_standard(init_value);
	if(VIDEO_OUTPUT_MODE_MAX != mode_config.mode)
	{
		GxPlayer_SetVideoOutputConfig(mode_config);	
	}

	return GXCORE_SUCCESS;
}

void app_system_params_init(void)
{
	int32_t init_value;
	int32_t ret_value = 0;
    float zone = 0;
	
	GxBus_ConfigGetInt(POWER_ON_KEY, &init_value, POWER_ON);
	if (init_value == POWER_ON)  //first boot
	{
		//log
		GxBus_ConfigSetInt(LOG_OUT_PUT_TYPE, 0);//初始为串口输出
		//full screen
		GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_SCREEN_XRES,APP_XRES);
		GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_SCREEN_YRES,APP_YRES);

		//logo
		#if BOOT_LOGO_SUPPORT
		GxBus_ConfigSetInt(PLAYER_CONFIG_AUTOPLAY_RADIO_FLAG, 0);
		GxBus_ConfigSetInt(PLAYER_CONFIG_AUTOPLAY_PLAY_FLAG, 0);
		#else
		GxBus_ConfigSet(PLAYER_CONFIG_AUTOPLAY_VIDEO_NAME, "player4");
#ifdef ECOS_OS
		GxBus_ConfigSet(PLAYER_CONFIG_AUTOPLAY_VIDEO_URL, "/dvb/theme/logo.bin");
#else
		GxBus_ConfigSet(PLAYER_CONFIG_AUTOPLAY_VIDEO_URL, WORK_PATH"theme/logo.bin");
#endif
		GxBus_ConfigSetInt(PLAYER_CONFIG_AUTOPLAY_RADIO_FLAG, 0);
		GxBus_ConfigSetInt(PLAYER_CONFIG_AUTOPLAY_PLAY_FLAG, 1);
		#endif
		
		//audio
		ret_value = app_oem_get_int(OEM_VOLUME, &init_value);
		init_value *=VOLUME_NUM;
		if(ret_value == GXCORE_ERROR)
		{
			init_value = AUDIO_VOLUME;
		}
        app_system_vol_save(init_value);

		// spdif
		if(app_oem_get_int(OEM_SPDIF_CONF, &init_value) == GXCORE_ERROR)
		{
			init_value = SPDIF_CONF;
			GxBus_ConfigSetInt(SPDIF_CONF_KEY, init_value);
			init_value = BYPASS_MODE;//AC3_BYPASS;
			GxBus_ConfigSetInt(TV_SPDIF_KEY, init_value);
		}
		else
		{
			if(init_value == -1)
			{
				init_value = 0;
				GxBus_ConfigSetInt(SPDIF_CONF_KEY, init_value);
                init_value = BYPASS_MODE;//AC3_BYPASS;
                GxBus_ConfigSetInt(TV_SPDIF_KEY, init_value);
			}
			else
			{
                init_value = DECODE_MODE;
				GxBus_ConfigSetInt(TV_SPDIF_KEY, init_value);
				init_value = 1;
				GxBus_ConfigSetInt(SPDIF_CONF_KEY, init_value);
			}
		} 
		
		GxBus_ConfigSetInt(AUDIO_LEVEL_KEY, AUDIO_LEVEL);
		GxBus_ConfigSetInt(MUTE_KEY, MUTE);
		GxBus_ConfigSetInt(SOUND_MODE_KEY, SOUND_MODE);	
		//GxBus_ConfigSetInt(SPDIF_OUTPUT_KEY, SPDIF_OUTPUT);

		//color
		GxBus_ConfigSetInt(SATURATION_KEY, SATURATION);
		GxBus_ConfigSetInt(BRIGHTNESS_KEY, BRIGHTNESS);
		GxBus_ConfigSetInt(CONTRAST_KEY, CONTRAST);
#if 0
        //scart
        if(app_oem_get_int(OEM_SCART_CONF, &init_value) == GXCORE_ERROR)
        {
            init_value = SCART_CONF;
            GxBus_ConfigSetInt(SCART_CONF_KEY, init_value);
            init_value = TV_SCART;
            GxBus_ConfigSetInt(TV_SCART_KEY, init_value);
        }
        else
        {
            if(init_value == -1)
            {
                init_value = 0;
                GxBus_ConfigSetInt(SCART_CONF_KEY, init_value);
            }
            else
            {
                GxBus_ConfigSetInt(TV_SCART_KEY, init_value);
                init_value = 1;
                GxBus_ConfigSetInt(SCART_CONF_KEY, init_value);
            }
        }

        GxBus_ConfigSetInt(TV_SCART_SCREEN, TV_RATIO);
#endif

#if 1
		GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DEC_MOSAIC_GATE, VIDEO_MOSAIC_GATE);
#endif
		//video output
		GxBus_ConfigSetInt(TV_STANDARD_KEY, TV_STANDARD);
		GxBus_ConfigSetInt(TV_RATIO_KEY, TV_RATIO);
		GxBus_ConfigSetInt(TV_ADAPT_KEY, TV_ADAPT_NOAUTO);
		GxBus_ConfigSetInt(ASPECT_MODE_KEY, ASPECT_MODE);
		GxBus_ConfigSetInt(NTSC_TYPE_KEY, NTSC_TYPE);
#if 0
		GxBus_ConfigGetInt(SCART_CONF_KEY, &init_value, SCART_CONF);
		if(init_value == 0)
		{
			GxBus_ConfigSetInt(VIDEO_INTERFACE_KEY, VIDEO_INTERFACE_MODE); 
		}
		else
		{
			GxBus_ConfigSetInt(VIDEO_INTERFACE_KEY, VIDEO_INTERFACE_SCART); 
		}
#endif
		GxBus_ConfigSetInt(VIDEO_INTERFACE_KEY, VIDEO_INTERFACE_MODE);

		//RF MODE
		GxBus_ConfigSetInt(RF_MODE_KEY, RF_MODE);

		// dts
		GxBus_ConfigSetInt(APP_SEARCH_DTS_SUPPORT, APP_GXBUS_SEARCH_DTS_NO);
		
		// for player
		#if (MINI_256_COLORS_OSD_SUPPORT||MINI_16_BITS_OSD_SUPPORT)
		GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_SUPPORT_PPZOOM, 0);
		GxBus_ConfigSetInt(PLAYER_CONFIG_RECORD_CACHED_MEMSIZE, 1880*1024);
		GxBus_ConfigSetInt(PLAY_RESERVED_MEMSIZE, 0x400000); // 10M(2M->10M)
		#else
		GxBus_ConfigSetInt(PLAY_RESERVED_MEMSIZE, 0xa00000); // 10M(2M->10M)
		#endif
		//GxBus_ConfigSetInt(PP_OUTPUT_MPEG2_TIME, MPEG_AV_DELAY_FRAME);	
		//GxBus_ConfigSetInt(PP_OUTPUT_H264_TIME, H264_AV_DELAY_FRAME);	

		//time
		if(app_oem_get_float(OEM_TIME_ZONE, &zone) == GXCORE_ERROR)
		{
			zone = TIME_ZONE_VALUE;
		}
		GxBus_ConfigSetDouble(TIME_ZONE, zone);
#if CASCAM_SUPPORT
        CASCAM_Set_Zone(zone);
#endif
		if(app_oem_get_int(OEM_SUMMER_TIME, &init_value) == GXCORE_ERROR)
		{
			init_value = TIME_SUMMER_VALUE;
		}
		GxBus_ConfigSetInt(TIME_SUMMER, init_value);
		
		//language
		if(app_oem_get_int(OEM_LANG_OSD, &init_value) == GXCORE_ERROR)
		{
			init_value = OSD_LANG;
		}
		GxBus_ConfigSetInt(OSD_LANG_KEY, init_value);
		
		GxBus_ConfigSet(PRIORITY_LANG_KEY, PRIORITY_LANG_VALUE);
		
		//osd
		ret_value = app_oem_get_int(OEM_TRANS, &init_value);
		if( ret_value== GXCORE_ERROR)
		{
			init_value = TRANS_LEVEL;
		}
		GxBus_ConfigSetInt(TRANS_LEVEL_KEY, init_value);
		
		GxBus_ConfigSetInt(INFOBAR_TIMEOUT_KEY, INFOBAR_TIMEOUT);
		//GxBus_ConfigSetInt(VOLUMEBAR_TIMEOUT_KEY, VOLUMEBAR_TIMEOUT);
		GxBus_ConfigSetInt(FREEZE_SWITCH, FREEZE_SWITCH_VALUE); 

		//lock
		GxBus_ConfigSetInt(MENU_LOCK_KEY, MENU_LOCK);
		GxBus_ConfigSetInt(CHANNEL_LOCK_KEY, CHANNEL_LOCK);
		GxBus_ConfigSetInt(MENU_BOOT_KEY, BOOT_LOCK);
		GxBus_ConfigSetInt(MENU_STAND3HOURS_KEY, STAND3HOUR_LOCK);

		//passwd
		GxBus_ConfigSetInt(PASSWORD_KEY0, PASSWORD_1);
		GxBus_ConfigSetInt(PASSWORD_KEY1, PASSWORD_2);
		GxBus_ConfigSetInt(PASSWORD_KEY2, PASSWORD_3);
		GxBus_ConfigSetInt(PASSWORD_KEY3, PASSWORD_4);

		
		//local position
		GxBus_ConfigSetInt(LONGITUDE_KEY, LONGITUDE);
		GxBus_ConfigSetInt(LONGITUDE_DIRECT_KEY, LONGITUDE_DIRECT);
		GxBus_ConfigSetInt(LATITUDE_KEY, LATITUDE);
		GxBus_ConfigSetInt(LATITUDE_DIRECT_KEY, LATITUDE_DIRECT);

		//PVR
		GxBus_ConfigSetInt(PVR_DURATION_KEY, PVR_DURATION_VALUE);
		GxBus_ConfigSetInt(PVR_TIMESHIFT_KEY, PVR_TIMESHIFT_FLAG);
		GxBus_ConfigSetInt(PVR_FILE_SIZE_KEY, PVR_FILE_SIZE_VALUE);

		//Subtitle
		GxBus_ConfigSetInt(SUBTITLE_MODE_KEY, SUBTITLE_MODE_VALUE);
		GxBus_ConfigSetInt(SUBTITLE_LANG_KEY, SUBTITLE_LANG_VALUE);

		//frontend monitor
		GxBus_ConfigSetInt(FRONTEND_WATCH_TIME_KEY, FRONTEND_WATCH_TIME_VALUE);

		//for search program name code change.[change to utf8]
		GxBus_ConfigSetInt(SEARCH_PROGRAM_CODE,SEARCH_PROGRAM_CODE_UTF8);

#ifdef FULL_HD_PP_MODE_ENABLE
        GxBus_ConfigSetInt(PLAY_PP_MODE_MAX_WIDTH, DEFAULT_PP_SUPPORT_WIDTH);
	    GxBus_ConfigSetInt(PLAY_PP_MODE_MAX_HEIGHT, DEFAULT_PP_SUPPORT_HEIGHT);
#endif

#ifdef NETAPPS_SUPPORT
        GxBus_ConfigSet(CLOUD_ID_KEY, CLOUD_DEFAULT_ID);
        GxBus_ConfigSet(CLOUD_PASSWD_KEY, CLOUD_DEFAULT_PASSWD);
#endif

#if UNICABLE_SUPPORT
		GxBus_ConfigSetInt(GXBUS_FRONTEND_UNICABLE, GXBUS_FRONTEND_UNICABLE_YES);
#endif

#if MUTI_TS_SUPPORT
		GxBus_ConfigSetInt(GXBUS_FRONTEND_MUTITS, GXBUS_FRONTEND_MUTITS_YES);
#endif
        GxBus_ConfigSetInt(POWER_ON_KEY, 1);
	}
	GxBus_ConfigSetInt(FRONTEND_CONFIG_WATCH_COUNT,FRONTEND_WATCH_TIME_COUNT);
}

// set the value that the same with player service
void app_system_service_init(void)
{
	int32_t init_value;
 	int32_t chroma;
	int32_t brightness;
	int32_t contrast;

	GxMsgProperty_PlayerVideoAutoAdapt auto_adapt = {0};
	GUI_Rect region_size = {0, 0, APP_XRES, APP_YRES};
	GxMsgProperty_PlayerVideoInterface Interface;
	GxMsgProperty_PlayerVideoModeConfig  mode_config;

	// 4:3  16:9, for screen size
	GxBus_ConfigGetInt(TV_RATIO_KEY, &init_value, TV_RATIO);
	system_setting_set_screen_ratio(init_value);

	GxBus_ConfigGetInt(TV_ADAPT_KEY, &init_value, TV_ADAPT_NOAUTO);
	auto_adapt.enable = init_value;
	auto_adapt.pal = VIDEO_OUTPUT_PAL;
	auto_adapt.ntsc = VIDEO_OUTPUT_NTSC_M;
	app_send_msg_exec(GXMSG_PLAYER_VIDEO_AUTO_ADAPT,&auto_adapt);

	//interface HDMI & RCA
	GxBus_ConfigGetInt(VIDEO_INTERFACE_KEY, &Interface, VIDEO_INTERFACE_MODE);
	app_send_msg_exec(GXMSG_PLAYER_VIDEO_INTERFACE, &Interface);

    //standard
	GxBus_ConfigGetInt(TV_STANDARD_KEY, &init_value, TV_STANDARD);
    //HDMI
	mode_config.interface = VIDEO_OUTPUT_HDMI;
	mode_config.mode = init_value;
	app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&mode_config);

#if 0
	GxBus_ConfigGetInt(SCART_CONF_KEY, &init_value, SCART_CONF);
    if(init_value == 0)
    {
        //RCA
        mode_config.interface = VIDEO_OUTPUT_RCA;
    }
	else
    {
        //SCART
        mode_config.interface = VIDEO_OUTPUT_SCART;
        GxBus_ConfigGetInt(TV_SCART_SCREEN, &init_value, TV_RATIO);
        mode_config.config.scart.screen = init_value;
        GxBus_ConfigGetInt(TV_SCART_KEY, &init_value, TV_SCART);
        mode_config.config.scart.source = init_value;
    }
#endif

    //RCA
    mode_config.interface = VIDEO_OUTPUT_RCA;
	mode_config.mode = app_av_get_cvbs_standard(init_value);
	if(VIDEO_OUTPUT_MODE_MAX != mode_config.mode)
		app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&mode_config);

	// Letter / Pilar box  pan scan
	GxBus_ConfigGetInt(ASPECT_MODE_KEY, &init_value, ASPECT_MODE);
	system_setting_set_screen_type(init_value);

	//saturation brightness contrast
	GxBus_ConfigGetInt(SATURATION_KEY, &chroma, SATURATION);
	GxBus_ConfigGetInt(BRIGHTNESS_KEY, &brightness, BRIGHTNESS);
	GxBus_ConfigGetInt(CONTRAST_KEY, &contrast, CONTRAST);
	system_setting_set_screen_color(chroma,brightness,contrast);

	// freezon
	GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
	app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

	GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &init_value, AUDIO_VOLUME);
	app_system_vol_set(init_value);

	//this is spp region size
	GUI_SetInterface("image_size", &region_size);
	// this is osd region size
	GUI_SetInterface("interface_size", &region_size);

	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
	GUI_SetInterface("osd_language", g_LangName[init_value]);  
	
	GxBus_ConfigGetInt(PVR_FILE_SIZE_KEY, &init_value, PVR_FILE_SIZE_VALUE);
	system_setting_set_full_gate(init_value);

	GxBus_ConfigGetInt(TRANS_LEVEL_KEY, &init_value, TRANS_LEVEL);
	GUI_SetInterface("osd_alpha_global", &init_value);
}

static void _init_fav_group_name(void)
{
	extern uint8_t fav_name[9][MAX_FAV_GROUP_NAME];
	uint32_t i = 0;
	int32_t init_value = 0;
	GxMsgProperty_NodeByPosGet node;

	GxBus_ConfigGetInt(FAV_INIT_KEY, &init_value, FAV_INIT_VALUE);
	node.pos = 0;
	node.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
	
	if((init_value == FAV_INIT_VALUE)
		||((node.fav_item.fav_name != NULL)&&(strcmp((const char*)(node.fav_item.fav_name),"HD")!=0)))
	{

		GxMsgProperty_NodeModify node_modify = {0};
		node_modify.node_type = NODE_FAV_GROUP;
		memcpy(&node_modify.fav_item, &node.fav_item, sizeof(GxBusPmDataFavItem));
		strncpy((char*)node_modify.fav_item.fav_name, 
			(const char*)"HD", MAX_FAV_GROUP_NAME-1);
		app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);      

		for(i = 0; i < 8; i++)
		{
			node.pos = i + 1;
			node.node_type = NODE_FAV_GROUP;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

			node_modify.node_type = NODE_FAV_GROUP;
			memcpy(&node_modify.fav_item, &node.fav_item, sizeof(GxBusPmDataFavItem));
			strncpy((char*)node_modify.fav_item.fav_name,
	            (const char*)fav_name[i], MAX_FAV_GROUP_NAME-1);

	            app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);   

		}
		init_value = 1;
		GxBus_ConfigSetInt(FAV_INIT_KEY, init_value);
	}
}

static void default_program_load(void)
{
	GxBus_PmDbaseInit(SYS_MAX_SAT, SYS_MAX_TP, SYS_MAX_PROG, (uint8_t*)DEFAULT_DB_PATH);
}

/* Exported functions ----------------------------------------------------- */
void app_system_init(void)
{
    AppFrontend_Open open;
#if TKGS_SUPPORT
    app_tkgs_init_operatemode();
#endif
    default_program_load();

    _init_fav_group_name();

//#if (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
#if (DEMOD_DVB_COMBO > 0)
    open.tuner = 0;
    open.ts_src = DEMUX_TS2;    //For GX1801+R848
    open.dmx_id = 0;
	app_mod_init(APP_FRONTEND_BASE, open.tuner, DEMOD_TYPE_DVBS);
    app_ioctl(open.tuner, FRONTEND_OPEN, &open);
#else
	#if (DEMOD_DVB_S > 0)
	    open.tuner = 0;
	    open.ts_src = DEMUX_TS1; //0
	    open.dmx_id = 0;
		app_mod_init(APP_FRONTEND_BASE, open.tuner, DEMOD_TYPE_DVBS);
	    app_ioctl(open.tuner, FRONTEND_OPEN, &open);
		#if DOUBLE_S2_SUPPORT
		    open.tuner = 1;// for dtmb
		    open.ts_src = DEMUX_TS2;//1
		    open.dmx_id = 0;
			app_mod_init(APP_FRONTEND_BASE, open.tuner, DEMOD_TYPE_DVBS);
		    app_ioctl(open.tuner, FRONTEND_OPEN, &open);
		#endif
	#endif
	#if (DEMOD_DVB_C > 0)
	    open.tuner = 0;
	    open.ts_src = DEMUX_TS1;//0
	    open.dmx_id = 0;
		app_mod_init(APP_FRONTEND_BASE, open.tuner, DEMOD_TYPE_DVBC);
	    app_ioctl(open.tuner, FRONTEND_OPEN, &open);
	#endif
	#if (DEMOD_DVB_T > 0)
	    open.tuner = 0;
	    open.ts_src = DEMUX_TS2;//1
	    open.dmx_id = 0;
		app_mod_init(APP_FRONTEND_BASE, open.tuner, DEMOD_TYPE_DVBT);
	    app_ioctl(open.tuner, FRONTEND_OPEN, &open);
	#endif
	//
	#if (DEMOD_DTMB > 0)
		open.tuner	= 1;	
		open.ts_src = DEMUX_TS2;//1
		open.dmx_id = 0;
		app_mod_init(APP_FRONTEND_BASE, open.tuner, DEMOD_TYPE_DTMB);
		app_ioctl(open.tuner, FRONTEND_OPEN, &open);
	#endif
#endif


#if TKGS_SUPPORT
	extern void  gx_tkgs_backupgrade_first_init();	
	gx_tkgs_backupgrade_first_init();

    extern status_t app_tkgs_set_default_location(void);
    app_tkgs_set_default_location();
#endif

    g_AppPlayOps.program_play_init();
    g_AppPlayOps.current_play.play_count = g_AppPlayOps.normal_play.play_count;
    g_AppBook.disable(50); //ignore book until 1 min
}

void app_system_reset(void)
{
    uint32_t panel_led = 0;
    uint32_t led_lock_state = 0;

	int fd = 0;
	fd = open("/dev/gxirr0", O_RDWR);
	ioctl(fd, 0x4, NULL);
	
#if NETWORK_SUPPORT
	app_send_msg_exec(GXMSG_NETWORK_STOP, NULL);
#ifdef LINUX_OS
#if (BOX_SERVICE > 0)
    GxMsgProperty_BoxRecordControl box_rec = {0};
    box_rec.option = BOX_RECORD_ALL_STOP;
    app_send_msg_exec(GXMSG_BOX_RECORD_CONTROL, (void *)(&box_rec));
    app_send_msg_exec(GXMSG_BOX_DISABLE, (void *)NULL);
#endif
    //app_send_msg_exec(GXMSG_NETWORK_STOP, NULL);
    system("rm /tmp/ethernet -rf");
    system("rm /home/gx/local/ethernet/* -f");
#else

#if WIFI_SUPPORT
    if(GxCore_FileExists("/home/gx/ap_psk_record") == GXCORE_FILE_EXIST)
        GxCore_FileDelete("/home/gx/ap_psk_record");
#endif
#if MOD_3G_SUPPORT
    if(GxCore_FileExists("/home/gx/3g_connect_record") == GXCORE_FILE_EXIST)
        GxCore_FileDelete("/home/gx/3g_connect_record");
#endif
#endif
#endif

#if (DEMOD_DVB_S > 0)
    app_sat_sel_clear();
    app_sat_list_del_sel_status();
	app_sat_list_close_lnb();
#endif
    app_init_sleep_set();
    // config ini reset

    GxBus_ConfigLoadDefault();
    //app_pvr_rec_stop();

    // program reset
    app_send_msg_exec(GXMSG_PM_LOAD_DEFAULT, NULL);

    // play status init
    g_AppPlayOps.program_play_init();

    // book reset
	app_send_msg_exec(GXMSG_BOOK_RESET, NULL);
	
	app_system_params_init();
	app_system_service_init();

	_init_fav_group_name();//rename FAV name

	if (g_AppFullArb.state.mute == STATE_ON) 
	{
		GxMsgProperty_PlayerAudioMute player_mute = 0;
		app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
	}

	g_AppFullArb.init(&g_AppFullArb);
	#if TKGS_SUPPORT
       GxCore_ThreadDelay(20);
        #endif
	g_AppTime.stop(&g_AppTime);

	ioctl(bsp_panel_handle, PANEL_SHOW_NUM, &panel_led);
	ioctl(bsp_panel_handle, PANEL_SHOW_LOCK, &led_lock_state);
	#if TKGS_SUPPORT
    app_tkgs_init_operatemode();

	//给默认数据库中的节目指定lcn
	//app_tkg_lcn_flush();
	//app_tkgs_sort_channels_by_lcn();
	
	app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_FACTORY);
	app_tkgs_upgrade_process_menu_exec();
	#endif

		g_AppTime.init(&g_AppTime);
	g_AppTime.start(&g_AppTime);
#if WEATHER_SUPPORT
	extern void weather_factory_reset(void);
	weather_factory_reset();
#endif	

}

uint32_t g_panel_keymap[PANEL_KEYMAP_TOTAL] = {0};
uint32_t app_get_panel_keycode(PanelKeymap key_index)
{
	return g_panel_keymap[key_index];
}

status_t app_get_panel_keymap(void)
{
#define KEYMAP_FILE "/dvb/theme/keymap.xml"
#define KEYMAP_STR  (uint8_t*)"keymap"
#define KEYMAP_GROUP  (uint8_t*)"group"
#define KEYMAP_NAME  (uint8_t*)"name"

	GXxmlDocPtr doc;
	uint32_t key_flag = 0;
	uint32_t flag_temp = 0;
	uint8_t i = 0;
	char *panel_key_name[PANEL_KEYMAP_TOTAL] =
	{
		"GUIK_M",
		"GUIK_ESCAPE",
		"GUIK_RETURN",
		"GUIK_UP",
		"GUIK_DOWN",
		"GUIK_LEFT",
		"GUIK_RIGHT",
		"GUIK_H",
	};

	doc = GXxmlParseFile(KEYMAP_FILE);
	if (NULL == doc) 
	{
		printf( "[panel keymap]keyGXxml parse error.\n");
		return GXCORE_ERROR;
	}

	// root
	GXxmlNodePtr root = doc->root;
	if (root == NULL) 
	{
		printf( "[panel keymap]keyGXxml empty\n");
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}

	if (0 !=  GXxmlStrcmp(root->name, KEYMAP_STR))
	{
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}

	for(i = 0; i < PANEL_KEYMAP_TOTAL; i++)
	{
		flag_temp |= (1 << i);
	}

	// group
	GXxmlNodePtr group = root->childs;
	while (NULL != group) 
	{
		GXxmlNodePtr item = NULL;
		uint8_t *value = NULL;
		uint8_t *iname = NULL;

		if (GXxmlStrcmp(group->name, KEYMAP_GROUP) == 0)
		{
			item =  group->childs;
			while (item != NULL) 
			{
				iname = GXxmlGetProp(item, KEYMAP_NAME);
				if(iname == NULL)
				{
					item = item->next;
					continue;
				}
				
				if (GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_MENU]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						g_panel_keymap[PANEL_KEYMAP_MENU] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_MENU);
						GxCore_Free(value);
					}
				}
				else if (GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_EXIT]) == 0) 
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						g_panel_keymap[PANEL_KEYMAP_EXIT] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_EXIT);
						GxCore_Free(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_OK]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						g_panel_keymap[PANEL_KEYMAP_OK] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_OK);
						GxCore_Free(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_UP]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						g_panel_keymap[PANEL_KEYMAP_UP]= app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_UP);
						GxCore_Free(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_DOWN]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						g_panel_keymap[PANEL_KEYMAP_DOWN] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_DOWN);
						GxCore_Free(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_LEFT]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						g_panel_keymap[PANEL_KEYMAP_LEFT] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_LEFT);
						GxCore_Free(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_RIGHT]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						g_panel_keymap[PANEL_KEYMAP_RIGHT] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_RIGHT);
						GxCore_Free(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_PW]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						g_panel_keymap[PANEL_KEYMAP_PW] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_PW);
						GxCore_Free(value);
					}
				}
				else
					;

				GxCore_Free(iname);
				iname = NULL;

				if((key_flag & flag_temp) == flag_temp)
				{
					GXxmlFreeDoc(doc);
					return GXCORE_SUCCESS;
				}

				item = item->next;
			}
		}
		group = group->next;
	}

	GXxmlFreeDoc(doc);
	return GXCORE_ERROR;
}

//plz free key_val if result > 0
int app_get_keymap_data(const char *key_name, unsigned int **key_val)
{
#define KEYMAP_FILE "/dvb/theme/keymap.xml"
#define KEYMAP_STR  (uint8_t*)"keymap"
#define KEYMAP_GROUP  (uint8_t*)"group"
#define KEYMAP_NAME  (uint8_t*)"name"

	GXxmlDocPtr doc;
    int ret = 0;

    if((key_name == NULL) || (key_val == NULL))
    {
        return 0;
    }

	doc = GXxmlParseFile(KEYMAP_FILE);
	if (NULL == doc)
	{
		return 0;
	}

	// root
	GXxmlNodePtr root = doc->root;
	if (root == NULL)
	{
		GXxmlFreeDoc(doc);
		return 0;
	}

	if (0 !=  GXxmlStrcmp(root->name, KEYMAP_STR))
	{
		GXxmlFreeDoc(doc);
		return 0;
	}

	// group
	GXxmlNodePtr group = root->childs;
	while (NULL != group)
	{
		GXxmlNodePtr item = NULL;
		uint8_t *value = NULL;
		uint8_t *iname = NULL;

		if (GXxmlStrcmp(group->name, KEYMAP_GROUP) == 0)
		{
			item =  group->childs;
			while (item != NULL)
			{
				iname = GXxmlGetProp(item, KEYMAP_NAME);
				if(iname == NULL)
				{
					item = item->next;
					continue;
				}

				if(GXxmlStrcmp(iname, (uint8_t*)key_name) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
                    {
                        unsigned int *temp = NULL;
                        temp = (unsigned int*)GxCore_Realloc(*key_val, (ret + 1) * sizeof(unsigned int));
                        if(temp != NULL)
                        {
                            *key_val = temp;
                            (*key_val)[ret] = app_hexstr2value(value);
                            //printf("##### value: 0x%x ######\n", ((*key_val)[ret]));
                            ret++;
                        }
                        GxCore_Free(value);
                        value = NULL;
                        GxCore_Free(iname);
                        iname = NULL;
                        break; //group = group->next;
                    }
				}
				else
					;

				GxCore_Free(iname);
				iname = NULL;

				item = item->next;
			}
		}
		group = group->next;
	}

	GXxmlFreeDoc(doc);
	return ret;
}

/* End of file -------------------------------------------------------------*/

