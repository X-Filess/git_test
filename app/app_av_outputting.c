#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"

/*
TV_STANDARD_KEY  --  VEDIO_OUT_KEY
TV_RATIO_KEY
ASPECT_MODE_KEY
SOUND_MODE_KEY
SPDIF_OUTPUT_KEY
//TV_SCART


PLAYER_CONFIG_VIDEO_RESOLUTION
PLAYER_CONFIG_VIDEO_DISPLAY_SCREEN
PLAYER_CONFIG_VIDEO_ASPECT
PLAYER_CONFIG_AUDIO_TRACK
PLAYER_CONFIG_AUDIO_SPDIF_SOURCE
*/
#define AUTO_CHANGE_SUPPORT
#define SUPPORT_1080P_OUTPUT
//#define SUPPORT_TV_SCART
#define SUPPORT_SPDIF

enum ITEM_AV_OUT
{
	ITEM_TV_STANDARD = 0,
	ITEM_TV_RATIO,
	ITEM_ASPECT_MODE,
	ITEM_SOUND_MODE,
	ITEM_SPDIF,
#ifdef SUPPORT_TV_SCART
	ITEM_TV_SCART,
#endif
#ifdef AUDIO_DESCRIPTOR_SUPPORT
	ITEM_AD_MODE,
#endif
	ITEM_AVOUT_TOTAL
};
 		

typedef enum
{
	AV_STANDARD_AUTO,
    AV_STANDARD_576I,
    AV_STANDARD_576P,
    AV_STANDARD_720P_50HZ,
    AV_STANDARD_720P_60HZ,
    AV_STANDARD_1080I_50HZ,
    AV_STANDARD_1080I_60HZ,
#ifdef SUPPORT_1080P_OUTPUT
    AV_STANDARD_1080P_50HZ,
    AV_STANDARD_1080P_60HZ,
#endif
    AV_STANDARD_TOTAL,
}AvTvStandard;

typedef enum
{
    AV_RATIO_4_3,
    AV_RATIO_16_9,
    AV_RATIO_TOTAL
}AvTvRatio;

typedef enum
{
	AV_ASPECT_AUTO,
	AV_ASPECT_LETTER_BOX,
	AV_ASPECT_PAN_SCAN,
    AV_ASPECT_TOTAL
}AvAspectMode;

typedef enum
{
    AV_VOLUME_GLOBAL,
    AV_VOLUME_CHANNEL,	
   /* AV_SOUND_STEREO,
    AV_SOUND_LEFT,
    AV_SOUND_RIGHT,
    AV_SOUND_MONO,*/
    AV_SOUND_TOTAL
}AvSoundMode;

typedef enum
{
    AV_SPDIF_DECODE,//AC3 decord
    AV_SPDIF_BYPASS,//AV3 BYPASS
    AV_SPDIF_AUTO,
    AV_SPDIF_TOTAL,
}AvSpdif;

static char* spdif[AV_SPDIF_TOTAL] = {
     "AC3 Decode,","AC3 Bypass,","AC3 Auto,"
};

#ifdef SUPPORT_TV_SCART
typedef enum
{
    AV_SCART_CVBS,
    AV_SCART_RGB,
    AV_SCART_TOTAL
}AvScart;

static char* scart[AV_SCART_TOTAL] = {
    "CVBS,","RGB,"
};

#endif

static char* item_avout[ITEM_AVOUT_TOTAL] = 
{
    "TV Standard",
    "TV Ratio",
    "Aspect Mode",
    "Volume Scope",// "Sound Mode",
    "HDMI-Audio",
    #ifdef SUPPORT_TV_SCART
    "TV-Scart",
    #endif
	#ifdef AUDIO_DESCRIPTOR_SUPPORT
	"Audio Descriptor",
	#endif
};

static char* tv_ratio[AV_RATIO_TOTAL] = {
   "4:3,","16:9,"
};
static char* aspect_mode[AV_ASPECT_TOTAL] = {
   "Auto,","Letter Box,","Pan & Scan,"
};
static char* tv_standard[AV_STANDARD_TOTAL] = {
	"Auto,",
    "576i,","576p,","720p@50Hz,","720p@60Hz,","1080i@50Hz,","1080i@60Hz,",
#ifdef SUPPORT_1080P_OUTPUT
    "1080p@50Hz,","1080p@60Hz,",
#endif
};
static char* sound_mode[AV_SOUND_TOTAL] = {
    "Global,","Channel,", //"Stereo,","Left,","Right,","Mono",
};

#ifdef AUDIO_DESCRIPTOR_SUPPORT
typedef enum
{
	AUDIO_DESCRIPTOR_OFF,
	AUDIO_DESCRIPTOR_ON,
	AUDIO_DESCRIPTOR_TOTAL
}AudioDescriptor;

static char* ad_status[AUDIO_DESCRIPTOR_TOTAL] = {
    "Off,","On,"
};

#endif

typedef enum
{
	AV_ADAPT_NOAUTO,
	AV_ADAPT_AUTO
}AvAutoAdapt;

typedef enum
{
	AV_VOLUME_SCOPE_GLOBAL,
	AV_VOLEME_SCOPE_CHANNEL
}AvVolumeScope;

typedef struct
{
	VideoOutputMode         vout_mode;// HDMI 1080  YPBPR 1080...output config. standard
	DisplayScreen           ratio;  // DisplayScreen . aspect  
	VideoAspect             aspect;  // 4:3 letterbox ...
	AvAutoAdapt          display_adapt;
	AvVolumeScope	volume_scope;
	AudioTrack              audio_track;
#ifdef SUPPORT_TV_SCART
    VideoOutputSource		scart_source;// 0:CVBS; 1: RGB
    VideoOutputScreen       scart_screen;// 0:4:3;  1:16:9
#endif
	AvSpdif         spdif;             //SPDIF_OUTPUT_KEY
	VideoOutputInterface    vout_interface;// RCA, YUV, HDMI ...output config.UI needn't input
	#ifdef AUDIO_DESCRIPTOR_SUPPORT
	AudioDescriptor ad_status;
	#endif
}AvPlayerPara;

typedef struct
{
    //AvAutoAdapt     adapt;
    AvTvStandard    tv_standard;       //TV_STANDARD_KEY  --  VEDIO_OUT_KEY
    AvTvRatio       tv_ratio;          //TV_RATIO_KEY
    AvAspectMode    aspect_mode;       //ASPECT_MODE_KEY
    AvSoundMode     volume_mode;        //SOUND_MODE_KEY  sound_mode
    AvSpdif         spdif;             //SPDIF_OUTPUT_KEY
    #ifdef SUPPORT_TV_SCART
    AvScart         tv_scart;
	#endif
	#ifdef AUDIO_DESCRIPTOR_SUPPORT
	AudioDescriptor ad_mode;
	#endif 
}AvAppPara;


static uint32_t s_av_item_map[ITEM_AVOUT_TOTAL];
static SystemSettingOpt s_av_setting_opt;
static SystemSettingItem s_av_item[ITEM_AVOUT_TOTAL];
static AvPlayerPara s_AvPlayerPara;
static AvAppPara s_AvAppPara;
static VideoOutputMode s_vout_mode;
#ifdef SUPPORT_TV_SCART
static int32_t s_scart_enable = 0;
#endif
static int32_t s_spdif_enable = 0;
static bool s_av_menu_falg = false;
static bool s_av_power_flag = false;
static  PopList pop_list;

static void av_player_to_app_para(AvPlayerPara *player, AvAppPara *app)
{
    if (player == NULL || app == NULL)
        return;

    if (player->display_adapt != AV_ADAPT_AUTO)
    {
        // standard
        switch (player->vout_mode)
        {
            case VIDEO_OUTPUT_HDMI_576I:
                app->tv_standard = AV_STANDARD_576I;
                break;
            case VIDEO_OUTPUT_HDMI_576P:
                app->tv_standard = AV_STANDARD_576P;
                break;
            case VIDEO_OUTPUT_HDMI_720P_50HZ:
                app->tv_standard = AV_STANDARD_720P_50HZ;
                break;
			case VIDEO_OUTPUT_HDMI_720P_60HZ:
                app->tv_standard = AV_STANDARD_720P_60HZ;
                break;
            case VIDEO_OUTPUT_HDMI_1080I_50HZ:
                app->tv_standard = AV_STANDARD_1080I_50HZ;
                break;
			case VIDEO_OUTPUT_HDMI_1080I_60HZ:
                app->tv_standard = AV_STANDARD_1080I_60HZ;
                break;
#ifdef SUPPORT_1080P_OUTPUT
            case VIDEO_OUTPUT_HDMI_1080P_50HZ:
                app->tv_standard = AV_STANDARD_1080P_50HZ;
                break;
			case VIDEO_OUTPUT_HDMI_1080P_60HZ:
                app->tv_standard = AV_STANDARD_1080P_60HZ;
                break;
#endif
            default:
                break;
        }
    }
    else
    {
        // standard - adapt
        app->tv_standard = AV_STANDARD_AUTO;
    }

    // ratio
    switch (player->ratio.aspect)
    {
        case DISPLAY_SCREEN_4X3:
            app->tv_ratio = AV_RATIO_4_3;
            break;
        case DISPLAY_SCREEN_16X9:
            app->tv_ratio = AV_RATIO_16_9;
            break;
        default:
            break;
    }

    // aspect
    switch (player->aspect)
    {
        case ASPECT_NORMAL:
            app->aspect_mode = AV_ASPECT_AUTO;
            break;
        case ASPECT_LETTER_BOX:
            app->aspect_mode = AV_ASPECT_LETTER_BOX;
            break;
        case ASPECT_PAN_SCAN:
            app->aspect_mode = AV_ASPECT_PAN_SCAN;
            break;
        default:
            break;
    }

#if 0
    // sound mode
    switch (player->audio_track)
    {
        case AUDIO_TRACK_STEREO:
            app->sound_mode = AV_SOUND_STEREO;
            break;
        case AUDIO_TRACK_LEFT:
            app->sound_mode = AV_SOUND_LEFT;
            break;
        case AUDIO_TRACK_RIGHT:
            app->sound_mode = AV_SOUND_RIGHT;
            break;
        case AUDIO_TRACK_MONO:
            app->sound_mode = AV_SOUND_MONO;
            break;
        default:
            break;
    }
   #else
            app->volume_mode = player->volume_scope;
    #endif

// scart output
#ifdef SUPPORT_TV_SCART
    if(s_scart_enable == 1) 
    {
        switch(player->scart_source)
        {
            case VIDEO_OUTPUT_SOURCE_CVBS:
                app->tv_scart = AV_SCART_CVBS;
                break;

            case VIDEO_OUTPUT_SOURCE_RGB:
                app->tv_scart = AV_SCART_RGB;
                break;

            default:
                break;
        }
    }	
#endif  

#ifdef AUDIO_DESCRIPTOR_SUPPORT
	app->ad_mode = player->ad_status ;	
#endif   

//SPDIF output
    if(s_spdif_enable == 1)
    {
        switch(player->spdif)
        {
            case AV_SPDIF_DECODE:
                app->spdif = AV_SPDIF_DECODE;
                break;
            case AV_SPDIF_BYPASS:
                app->spdif = AV_SPDIF_BYPASS;
                break;
            case AV_SPDIF_AUTO:
                app->spdif = AV_SPDIF_AUTO;
                break;
        
            default:
                break;
        		
        }
    }

}

static void app_av_app_to_player_para(AvAppPara *app, AvPlayerPara *player)
{
    if (player == NULL || app == NULL)  return;
    
    uint32_t standard[AV_STANDARD_TOTAL] = {
            VIDEO_OUTPUT_HDMI_576I,
            VIDEO_OUTPUT_HDMI_576P,
            VIDEO_OUTPUT_HDMI_720P_50HZ,
            VIDEO_OUTPUT_HDMI_720P_60HZ,
            VIDEO_OUTPUT_HDMI_1080I_50HZ,
            VIDEO_OUTPUT_HDMI_1080I_60HZ,
#ifdef SUPPORT_1080P_OUTPUT
            VIDEO_OUTPUT_HDMI_1080P_50HZ,
            VIDEO_OUTPUT_HDMI_1080P_60HZ,
#endif
    };
    uint32_t ratio[AV_RATIO_TOTAL] = {DISPLAY_SCREEN_4X3, DISPLAY_SCREEN_16X9};
    uint32_t aspect[AV_ASPECT_TOTAL] = {ASPECT_NORMAL, ASPECT_LETTER_BOX, ASPECT_PAN_SCAN};
    uint32_t sound[AV_SOUND_TOTAL] =  {AV_VOLUME_SCOPE_GLOBAL,AV_VOLEME_SCOPE_CHANNEL};//{AUDIO_TRACK_STEREO, AUDIO_TRACK_LEFT, AUDIO_TRACK_RIGHT, AUDIO_TRACK_MONO};
    #ifdef SUPPORT_TV_SCART
    uint32_t scart[AV_SCART_TOTAL] = {VIDEO_OUTPUT_SOURCE_CVBS, VIDEO_OUTPUT_SOURCE_RGB};
	#endif
	#ifdef AUDIO_DESCRIPTOR_SUPPORT
	//uint32_t ad_status[AUDIO_DESCRIPTOR_TOTAL] = {AUDIO_DESCRIPTOR_OFF, AUDIO_DESCRIPTOR_ON};
	#endif
    uint32_t spdif[AV_SPDIF_TOTAL] = {AV_SPDIF_DECODE,AV_SPDIF_BYPASS,AV_SPDIF_AUTO};
    // standard
    if (app->tv_standard > AV_STANDARD_AUTO)
    {
        player->vout_mode = standard[app->tv_standard - 1]; 
        player->display_adapt = AV_ADAPT_NOAUTO;
    }
    else
    {
	    player->display_adapt = AV_ADAPT_AUTO;
#if 0
        // TODO: 自动模式如何设置视频输出
        player->vout_mode = standard[AV_STANDARD_1080I_50HZ]; 
#endif
    }

    // ratio
    player->ratio.aspect = ratio[app->tv_ratio];
#ifdef SUPPORT_TV_SCART
    if(s_scart_enable == 1)// 0:4:3; 1: 16:9
    {
        player->scart_screen = ratio[app->tv_ratio];
        player->scart_source = scart[app->tv_scart];
    }
#endif

    if (s_spdif_enable == 1)
        player->spdif = spdif[app->spdif];

    player->ratio.xres = APP_XRES;
    player->ratio.yres = APP_YRES;

    // aspect
    player->aspect = aspect[app->aspect_mode];

    // sound mode
  //  player->audio_track = sound[app->volume_mode];
    player->volume_scope= sound[app->volume_mode];
	
#ifdef AUDIO_DESCRIPTOR_SUPPORT
	player->ad_status = app->ad_mode;
#endif

}

static void app_av_player_para_get(AvPlayerPara *player)
{
    int init_value = 0;

    //auto adapt
    GxBus_ConfigGetInt(TV_ADAPT_KEY, &init_value, TV_ADAPT_NOAUTO);
    player->display_adapt = init_value;

   // tv standard
    GxBus_ConfigGetInt(TV_STANDARD_KEY, &init_value, TV_STANDARD);
	s_vout_mode = init_value;
    player->vout_mode = init_value;

    // ratio
    GxBus_ConfigGetInt(TV_RATIO_KEY, &init_value, TV_RATIO);
    player->ratio.aspect = init_value;

    // ascept
    GxBus_ConfigGetInt(ASPECT_MODE_KEY, &init_value, ASPECT_MODE);
    player->aspect = init_value;

    // sound mode
    GxBus_ConfigGetInt(SOUND_MODE_KEY, &init_value, SOUND_MODE);
    player->audio_track = init_value;
	
   GxBus_ConfigGetInt(VOLUME_MODE_KEY, &init_value, VOLUME_SCOMPE_);
    player->volume_scope= init_value;

#ifdef SUPPORT_TV_SCART
    if(s_scart_enable == 1)
    {
        /*SCART OUTPUT*/
        GxBus_ConfigGetInt(TV_SCART_KEY, &init_value, TV_SCART);
        player->scart_source = init_value;//  0:CVBS ,1:RGB
        player->scart_screen = player->aspect;// 0: 4:3; 1: 16:9
    }
#endif

    if(s_spdif_enable == 1)
    {
        /*SPDIF OUTPUT*/
        GxBus_ConfigGetInt(TV_SPDIF_KEY, &init_value, AC3_BYPASS);
        if (init_value == DECODE_MODE)
            player->spdif = AV_SPDIF_DECODE;
        else if(init_value == BYPASS_MODE)
            player->spdif = AV_SPDIF_BYPASS;
        else if(init_value == (DECODE_MODE|BYPASS_MODE))
            player->spdif = AV_SPDIF_AUTO;
    }    
	#ifdef AUDIO_DESCRIPTOR_SUPPORT
	GxBus_ConfigGetInt(AD_CONF_KEY, &init_value, AD_CONFIG);
    player->ad_status= init_value;
	#endif

}

static void app_av_app_para_get(AvAppPara *app)
{
    app->tv_standard   = s_av_item[s_av_item_map[ITEM_TV_STANDARD]].itemProperty.itemPropertyCmb.sel;
    app->tv_ratio      = s_av_item[s_av_item_map[ITEM_TV_RATIO]].itemProperty.itemPropertyCmb.sel;
    app->aspect_mode   = s_av_item[s_av_item_map[ITEM_ASPECT_MODE]].itemProperty.itemPropertyCmb.sel;
    app->volume_mode    = s_av_item[s_av_item_map[ITEM_SOUND_MODE]].itemProperty.itemPropertyCmb.sel;
#ifdef SUPPORT_TV_SCART
	if(s_scart_enable == 1)
    {
        app->tv_scart = s_av_item[s_av_item_map[ITEM_TV_SCART]].itemProperty.itemPropertyCmb.sel;
    }
    else
    {
    	  app->tv_scart = s_AvAppPara.tv_scart;
    }
#endif

#ifdef AUDIO_DESCRIPTOR_SUPPORT
	 app->ad_mode= s_av_item[s_av_item_map[ITEM_AD_MODE]].itemProperty.itemPropertyCmb.sel;
#endif

    if(s_spdif_enable == 1)
        app->spdif = s_av_item[s_av_item_map[ITEM_SPDIF]].itemProperty.itemPropertyCmb.sel;
    else
        app->spdif = s_AvAppPara.spdif;
}

static bool app_av_setting_para_change(AvAppPara *app, AvAppPara *bak)
{
	bool ret = FALSE;

    if (0 != memcmp(app, bak, sizeof(AvAppPara)))
    {
        ret = TRUE; 
    }
	if(s_vout_mode != s_AvPlayerPara.vout_mode)
	{
		ret = TRUE;
	}

	return ret;
}

#if 0
static void app_av_set_video_interface(AvPlayerPara *player)
{
    switch (player->vout_mode)
    {
#if 0
            case VIDEO_OUTPUT_HDMI_576I:
            case VIDEO_OUTPUT_HDMI_576P:
            case VIDEO_OUTPUT_HDMI_720P_50HZ:
			case VIDEO_OUTPUT_HDMI_720P_60HZ:
            case VIDEO_OUTPUT_HDMI_1080I_50HZ:
			case VIDEO_OUTPUT_HDMI_1080I_60HZ:
            case VIDEO_OUTPUT_HDMI_1080P_50HZ:
			case VIDEO_OUTPUT_HDMI_1080P_60HZ:
#if (SCART_ENABLE == 1)
				video_interface = VIDEO_OUTPUT_HDMI|VIDEO_OUTPUT_SCART1;
#else
                video_interface = VIDEO_OUTPUT_HDMI|VIDEO_OUTPUT_RCA1;
#endif
                GxBus_ConfigSetInt(VIDEO_INTERFACE_KEY, video_interface);
                break;
#endif
            default:
                break;
    }
}
#endif

//add for HD
VideoOutputMode app_av_get_cvbs_standard(VideoOutputMode vout_mode)
{
	VideoOutputMode OutMode = VIDEO_OUTPUT_MODE_MAX;
    switch (vout_mode)
    {
    		case VIDEO_OUTPUT_YCBCR_480I:
			case VIDEO_OUTPUT_YCBCR_1080I_60HZ:
			case VIDEO_OUTPUT_YPBPR_480P:
			case VIDEO_OUTPUT_YPBPR_720P_60HZ:
			case VIDEO_OUTPUT_YPBPR_1080P_60HZ:
			case VIDEO_OUTPUT_HDMI_480I:
			case VIDEO_OUTPUT_HDMI_480P:
			case VIDEO_OUTPUT_HDMI_720P_60HZ:
			case VIDEO_OUTPUT_HDMI_1080I_60HZ:
#ifdef SUPPORT_1080P_OUTPUT
			case VIDEO_OUTPUT_HDMI_1080P_60HZ:
#endif
				OutMode = NTSC_TYPE;//VIDEO_OUTPUT_PAL_M,VIDEO_OUTPUT_NTSC_443
				break;
			case VIDEO_OUTPUT_YCBCR_576I:			
			case VIDEO_OUTPUT_YCBCR_1080I_50HZ:			
			case VIDEO_OUTPUT_YPBPR_576P:
			case VIDEO_OUTPUT_YPBPR_720P_50HZ:			
			case VIDEO_OUTPUT_YPBPR_1080P_50HZ:    		
            case VIDEO_OUTPUT_HDMI_576I:
            case VIDEO_OUTPUT_HDMI_576P:
            case VIDEO_OUTPUT_HDMI_720P_50HZ:			
            case VIDEO_OUTPUT_HDMI_1080I_50HZ:	
#ifdef SUPPORT_1080P_OUTPUT
            case VIDEO_OUTPUT_HDMI_1080P_50HZ:
#endif
				OutMode = VIDEO_OUTPUT_PAL; //VIDEO_OUTPUT_PAL_N, VIDEO_OUTPUT_PAL_NC               
                break;
            default:
				printf("\n---wrong format for RCA1----\n");
                break;
    }
	return OutMode;
}

static int app_av_set_standard(AvPlayerPara *player)
{
    GxMsgProperty_PlayerVideoModeConfig video_mode;
    GxMsgProperty_PlayerVideoAutoAdapt display_adapt;

    // adapt
    display_adapt.enable = (int)player->display_adapt;
    display_adapt.pal = VIDEO_OUTPUT_PAL;
    display_adapt.ntsc = NTSC_TYPE;
    GxBus_ConfigSetInt(TV_ADAPT_KEY, display_adapt.enable);
    app_send_msg_exec(GXMSG_PLAYER_VIDEO_AUTO_ADAPT, &display_adapt);

    //standard
    if(s_av_power_flag == true)
    {
        s_av_power_flag=false;
        GxBus_ConfigSetInt(TV_STANDARD_KEY, player->vout_mode);
    }
    if (player->display_adapt != AV_ADAPT_AUTO)
    {
        //HDMI - standard
        video_mode.interface = VIDEO_OUTPUT_HDMI;
        video_mode.mode = player->vout_mode;
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&video_mode);

#ifdef SUPPORT_TV_SCART
        if (s_scart_enable == 1)
        {
        //SCART - standard
            video_mode.interface = VIDEO_OUTPUT_SCART;// if use this configure, yuv is not support
            video_mode.config.scart.screen = player->scart_screen;
            video_mode.config.scart.source = player->scart_source;
        }
        else
#endif
        {
        //RCA - standard
            video_mode.interface = VIDEO_OUTPUT_RCA;
        }
        video_mode.mode = app_av_get_cvbs_standard(player->vout_mode);
        if(VIDEO_OUTPUT_MODE_MAX != video_mode.mode)
            app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&video_mode);
    }
    else //auto
    {
        //HDMI - standard
        video_mode.interface = VIDEO_OUTPUT_HDMI;
        video_mode.mode = player->vout_mode;
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&video_mode);

#ifdef SUPPORT_TV_SCART
        if (s_scart_enable == 1)
        {
        //SCART - standard
            video_mode.interface = VIDEO_OUTPUT_SCART;
            video_mode.config.scart.screen = player->scart_screen;
            video_mode.config.scart.source = player->scart_source;
        }
        else
#endif
        {
        //RCA - standard
            video_mode.interface = VIDEO_OUTPUT_RCA;
        }
        video_mode.mode = VIDEO_OUTPUT_PAL;// TODO: temp
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&video_mode);
    }

    return 0;
}

static int app_av_set_ratio(AvPlayerPara *player)
{
//    GxMsgProperty_PlayerDisplayScreen screen_ratio;

 //   screen_ratio.aspect = player->ratio.aspect;
//    screen_ratio.xres = player->ratio.xres;
//    screen_ratio.yres = player->ratio.yres;
    
    GxBus_ConfigSetInt(TV_RATIO_KEY,player->ratio.aspect);
//    app_send_msg_exec(GXMSG_PLAYER_DISPLAY_SCREEN, &screen_ratio);

    return 0;
}

static int app_av_set_aspect(AvPlayerPara *player)
{
//    GxMsgProperty_PlayerVideoAspect video_aspect = player->aspect;

    GxBus_ConfigSetInt(ASPECT_MODE_KEY,player->aspect);
//    app_send_msg_exec(GXMSG_PLAYER_VIDEO_ASPECT, &video_aspect);     

    return 0;
}

void app_av_setting_init(void)
{
	int  init_value = 0;
    GxBus_ConfigGetInt(TV_RATIO_KEY, &init_value, TV_RATIO);
	GxMsgProperty_PlayerDisplayScreen screen_ratio;
	screen_ratio.aspect = init_value;
	screen_ratio.xres = APP_XRES;
	screen_ratio.yres = APP_YRES;
	app_send_msg_exec(GXMSG_PLAYER_DISPLAY_SCREEN, &screen_ratio);
    GxBus_ConfigGetInt(ASPECT_MODE_KEY, &init_value, ASPECT_MODE);
	GxMsgProperty_PlayerVideoAspect video_aspect = init_value;
	app_send_msg_exec(GXMSG_PLAYER_VIDEO_ASPECT, &video_aspect);
}

static int app_av_set_audio_track(AvPlayerPara *player)
{

	#if 0
	GxMsgProperty_PlayerAudioTrack track = player->audio_track;

    	GxBus_ConfigSetInt(SOUND_MODE_KEY,player->audio_track);  
	app_send_msg_exec(GXMSG_PLAYER_AUDIO_TRACK, &track);
	#else
	GxBus_ConfigSetInt(VOLUME_MODE_KEY,player->volume_scope);  
	#endif

	return 0;
}

#ifdef SUPPORT_TV_SCART
static int app_av_set_scart_source(AvPlayerPara *player)
{
	GxMsgProperty_PlayerVideoModeConfig video_mode;

	GxBus_ConfigSetInt(TV_SCART_KEY, player->scart_source);
	video_mode.interface = VIDEO_OUTPUT_SCART;// if use this configure, yuv is not support
	video_mode.mode = app_av_get_cvbs_standard(player->vout_mode);
	video_mode.config.scart.source =  player->scart_source;
	video_mode.config.scart.screen =  player->scart_screen;
    app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&video_mode);
	return 0;

}

static int app_av_set_scart_ratio(AvPlayerPara *player)
{
	GxMsgProperty_PlayerVideoModeConfig video_mode;

	GxBus_ConfigSetInt(TV_SCART_SCREEN,player->scart_screen);
	video_mode.interface = VIDEO_OUTPUT_SCART;// if use this configure, yuv is not support
	video_mode.mode = app_av_get_cvbs_standard(player->vout_mode);
	video_mode.config.scart.source =  player->scart_source;
	video_mode.config.scart.screen =  player->scart_screen;
    app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&video_mode);
	return 0;
}
#endif

#ifdef AUDIO_DESCRIPTOR_SUPPORT
static int app_audio_descriptor_set_mode(AvPlayerPara *player)
{

	GxBus_ConfigSetInt(AD_CONF_KEY, player->ad_status);
	return 0;

}
#endif

static int app_av_set_spdif_output(AvPlayerPara *player)
{
	GxMsgProperty_PlayerAudioAC3Mode  mode = {0};//0: decord; 2.bypas 3.auto
	if(player->spdif ==  AV_SPDIF_DECODE)
		mode.mode = AUDIO_AC3_DECODE_MODE;
	else if(player->spdif == AV_SPDIF_BYPASS)
		mode.mode = AUDIO_AC3_BYPASS_MODE;
	else if(player->spdif == AV_SPDIF_AUTO)
		mode.mode = AUDIO_AC3_BYPASS_MODE|AUDIO_AC3_DECODE_MODE;
	else
		mode.mode = AUDIO_AC3_DECODE_MODE;
	GxBus_ConfigSetInt(TV_SPDIF_KEY, mode.mode);

	app_send_msg_exec(GXMSG_PLAYER_AUDIO_AC3_MODE,(void*)&mode);
	return 0;
}

static int app_av_setting_para_save(AvAppPara *result)
{
    AvAppPara *app = &s_AvAppPara;
    AvPlayerPara *player = &s_AvPlayerPara;

    if (app->tv_standard != result->tv_standard)
    {
		s_av_power_flag = true;
        app_av_set_standard(player);
    }
	else if(s_vout_mode != player->vout_mode)
	{
		app_av_set_standard(player);
	}
    if (app->tv_ratio != result->tv_ratio)
    {
        app_av_set_ratio(player);
#ifdef SUPPORT_TV_SCART
        if(s_scart_enable == 1)
        {
            app_av_set_scart_ratio(player);
        }
#endif
    }
    
    if (app->aspect_mode != result->aspect_mode)
    {
        app_av_set_aspect(player);
    }
    if (app->volume_mode != result->volume_mode)
    {
        app_av_set_audio_track(player);
    }

    if ((s_spdif_enable == 1) && (app->spdif != result->spdif))
    {
		app_av_set_spdif_output(player);
    }
#ifdef SUPPORT_TV_SCART
    if((s_scart_enable == 1) && (app->tv_scart != result->tv_scart))
    {
		app_av_set_scart_source(player);
    }
#endif

#ifdef AUDIO_DESCRIPTOR_SUPPORT
	if (app->ad_mode!= result->ad_mode)
	{
		app_audio_descriptor_set_mode(player);
	}
#endif
    // TODO:
//    init_value = ret_para->display_adapt;
//    GxBus_ConfigSetInt(TV_TYPE_IS_AUTO_KEY,init_value);    

	return 0;
}

static int app_av_setting_exit_callback(ExitType exit_type)
{
	AvAppPara result;
	int ret = 0;

	app_av_app_para_get(&result);
	if(exit_type == EXIT_ABANDON)
	{
		app_av_app_to_player_para(&s_AvAppPara, &s_AvPlayerPara);
		app_av_setting_para_save(&result);
	}
	else
	{
		if(app_av_setting_para_change(&s_AvAppPara, &result) == TRUE)
		{
			PopDlg  pop;
			memset(&pop, 0, sizeof(PopDlg));
			pop.type = POP_TYPE_YES_NO;
			pop.str = STR_ID_SAVE_INFO;
			if(popdlg_create(&pop) == POP_VAL_OK)
			{
				app_av_app_para_get(&result);
				app_av_app_to_player_para(&result, &s_AvPlayerPara);
				if (s_AvAppPara.tv_standard == result.tv_standard)
				{
					s_AvPlayerPara.vout_mode = s_vout_mode;
					GxBus_ConfigSetInt(TV_STANDARD_KEY, s_vout_mode);
				}
				app_av_setting_para_save(&result);
				ret =1;
			}
#ifdef AUTO_CHANGE_SUPPORT
			else
			{
				app_av_app_to_player_para(&s_AvAppPara, &s_AvPlayerPara);
				app_av_setting_para_save(&result);
			}
#endif
		}
	}
    app_av_setting_init();
    s_av_menu_falg = false;
	return ret;
}

static void av_param_init(uint32_t item, uint32_t sel, char* cmb_content, uint32_t content_total,
        char** content, int (*func)(int), int (*press_callback)(int key))
{
    uint32_t i;

	if(s_av_item_map[item] >= s_av_setting_opt.itemNum)
		return;

	s_av_item[s_av_item_map[item]].itemTitle = item_avout[item];
	s_av_item[s_av_item_map[item]].itemType = ITEM_CHOICE;

    // cmb content create
    strcpy(cmb_content, "[");
    for (i=0; i<content_total; i++)
    {
        strcat(cmb_content, content[i]);
    }
    strcat(cmb_content, "]");

	s_av_item[s_av_item_map[item]].itemProperty.itemPropertyCmb.content = cmb_content;
	s_av_item[s_av_item_map[item]].itemProperty.itemPropertyCmb.sel = sel;
	s_av_item[s_av_item_map[item]].itemCallback.cmbCallback.CmbChange= func;
    s_av_item[s_av_item_map[item]].itemCallback.cmbCallback.CmbPress= press_callback;
	s_av_item[s_av_item_map[item]].itemStatus = ITEM_NORMAL;
}

//#if MV_WIN_SUPPORT
// delete the "," for display
static int _av_standard_cmb_press_callback(int key)
{
    static char* s_StandardData[AV_STANDARD_TOTAL]= {NULL};
	char TempBuf[AV_STANDARD_TOTAL][15];// 15--must long than "1080p@60Hz"
	AvAppPara appara = {0};
	int cmb_sel =-1, i = 0;

	for(i = 0; i < AV_STANDARD_TOTAL; i++)
	{
		memcpy(TempBuf[i],tv_standard[i],strlen(tv_standard[i])>1?strlen(tv_standard[i])-1:0);
		TempBuf[i][strlen(tv_standard[i])>1?strlen(tv_standard[i])-1:0] = '\0';
		s_StandardData[i] = TempBuf[i];
	}
	if(key == STBK_OK)
	{
		app_av_app_para_get(&appara);
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = item_avout[0];
		pop_list.item_num = AV_STANDARD_TOTAL;
		pop_list.item_content = s_StandardData;
		pop_list.sel = appara.tv_standard;//s_AvAppPara.tv_standard;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_5;
        pop_list.pos.x= 630;
        pop_list.pos.y= 136;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt1", "select", &cmb_sel);
	}
	return EVENT_TRANSFER_KEEPON;
}
//#endif
static int _av_standard_change(int sel)
{
#ifdef AUTO_CHANGE_SUPPORT
	AvAppPara AVPara;
	AvPlayerPara player;
    uint32_t standard[AV_STANDARD_TOTAL] = {
            VIDEO_OUTPUT_HDMI_576I,
            VIDEO_OUTPUT_HDMI_576P,
            VIDEO_OUTPUT_HDMI_720P_50HZ,
            VIDEO_OUTPUT_HDMI_720P_60HZ,
            VIDEO_OUTPUT_HDMI_1080I_50HZ,
            VIDEO_OUTPUT_HDMI_1080I_60HZ,
#ifdef SUPPORT_1080P_OUTPUT
            VIDEO_OUTPUT_HDMI_1080P_50HZ,
            VIDEO_OUTPUT_HDMI_1080P_60HZ,
#endif
    };
	app_system_set_item_data_update(s_av_item_map[ITEM_TV_STANDARD]);
	app_av_app_para_get(&AVPara);
	app_av_app_to_player_para(&AVPara, &player);
	if (sel > AV_STANDARD_AUTO)
    {
        player.vout_mode = standard[sel - 1]; 
		s_vout_mode = player.vout_mode;
        player.display_adapt = AV_ADAPT_NOAUTO;
    }
    else
    {
        // modify by zhouzhr 20120317
        player.display_adapt = AV_ADAPT_AUTO;
    }
	app_av_set_standard(&player);
#endif
	return 0;
}
static void app_av_standard_init(void)
{
#define STANDARD_MAX    (100)
    static char cmb_standard[STANDARD_MAX];

    memset(cmb_standard, 0, STANDARD_MAX);

    av_param_init(ITEM_TV_STANDARD, s_AvAppPara.tv_standard, cmb_standard, AV_STANDARD_TOTAL,
                  tv_standard, _av_standard_change, _av_standard_cmb_press_callback);
}

static int _av_ratio_change(int sel)
{
#ifdef AUTO_CHANGE_SUPPORT
#ifdef SUPPORT_TV_SCART
	if (s_scart_enable == 1)
	{
		AvAppPara AVPara;
		AvPlayerPara player;
		uint32_t ratio[AV_RATIO_TOTAL] = {DISPLAY_SCREEN_4X3, DISPLAY_SCREEN_16X9};
		app_system_set_item_data_update(s_av_item_map[ITEM_TV_RATIO]);
		app_av_app_para_get(&AVPara);
		app_av_app_to_player_para(&AVPara, &player);
		player.scart_screen = ratio[sel];
		app_av_set_scart_ratio(&player);
	}
#endif
#endif
	return 0;
}

static int _av_ratio_cmb_press_callback(int key)
{
	static char* s_RatioData[AV_RATIO_TOTAL];
	char RatioBuf[AV_RATIO_TOTAL][12];
    AvAppPara appara = {0};
	int cmb_sel = -1, i = 0;

	for(i = 0; i < AV_RATIO_TOTAL; i++)
	{
		memcpy(RatioBuf[i], tv_ratio[i], strlen(tv_ratio[i])>1?strlen(tv_ratio[i])-1:0);
		RatioBuf[i][strlen(tv_ratio[i])>1?strlen(tv_ratio[i])-1:0] = '\0';
        s_RatioData[i] = RatioBuf[i];
	}
	if(key == STBK_OK)
	{
		app_av_app_para_get(&appara);
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = item_avout[ITEM_TV_RATIO];
		pop_list.item_num = AV_RATIO_TOTAL;
		pop_list.item_content = s_RatioData;
		pop_list.sel = appara.tv_ratio;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_2;
        pop_list.pos.x= 630;
        pop_list.pos.y= 167;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt2", "select", &cmb_sel);
	}

	return EVENT_TRANSFER_KEEPON;
}

static void app_av_ratio_init(void)
{
#define RATIO_MAX    (12)
    static char cmb_ratio[RATIO_MAX];

    memset(cmb_ratio, 0, RATIO_MAX);

    av_param_init(ITEM_TV_RATIO, s_AvAppPara.tv_ratio, cmb_ratio, AV_RATIO_TOTAL,
                  tv_ratio, _av_ratio_change, _av_ratio_cmb_press_callback);
}

static int _av_aspect_cmb_press_cb(int key)
{
	static char* s_AspectData[AV_ASPECT_TOTAL];
	char AspectBuf[AV_ASPECT_TOTAL][15];
    AvAppPara appara = {0};
	int cmb_sel = -1, i = 0;

	for(i = 0; i < AV_ASPECT_TOTAL; i++)
	{
		memcpy(AspectBuf[i], aspect_mode[i], strlen(aspect_mode[i])>1?strlen(aspect_mode[i])-1:0);
		AspectBuf[i][strlen(aspect_mode[i])>1?strlen(aspect_mode[i])-1:0] = '\0';
        s_AspectData[i] = AspectBuf[i];
	}

	if(key == STBK_OK)
	{
		app_av_app_para_get(&appara);
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = item_avout[ITEM_ASPECT_MODE];
		pop_list.item_num = AV_ASPECT_TOTAL;
		pop_list.item_content = s_AspectData;
		pop_list.sel = appara.aspect_mode;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_3;
        pop_list.pos.x= 630;
        pop_list.pos.y= 198;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt3", "select", &cmb_sel);
	}

	return EVENT_TRANSFER_KEEPON;
}

static void app_av_aspect_init(void)
{
#define ASPECT_MAX    (36)
    static char cmb_aspect[ASPECT_MAX];
    memset(cmb_aspect, 0, ASPECT_MAX);

    av_param_init(ITEM_ASPECT_MODE, s_AvAppPara.aspect_mode, cmb_aspect, AV_ASPECT_TOTAL,
                  aspect_mode, NULL, _av_aspect_cmb_press_cb);
}

static int _av_sound_cmb_press_cb(int key)
{
	static char* s_SoundData[AV_SOUND_TOTAL] = {NULL};
	char SoundBuf[AV_SOUND_TOTAL][15];
    AvAppPara appara = {0};
	int cmb_sel = -1, i = 0;

	for(i = 0; i < AV_SOUND_TOTAL; i++)
	{
		memcpy(SoundBuf[i], sound_mode[i], strlen(sound_mode[i])>1?strlen(sound_mode[i])-1:0);
		SoundBuf[i][strlen(sound_mode[i])>1?strlen(sound_mode[i])-1:0] = '\0';
        s_SoundData[i] = SoundBuf[i];
	}
	if(key == STBK_OK)
	{
		app_av_app_para_get(&appara);
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = item_avout[ITEM_SOUND_MODE];
		pop_list.item_num = AV_SOUND_TOTAL;
		pop_list.item_content = s_SoundData;
		pop_list.sel = appara.volume_mode;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_2;
        pop_list.pos.x= 630;
        pop_list.pos.y= 229;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt4", "select", &cmb_sel);
	}

	return EVENT_TRANSFER_KEEPON;
}

static void app_av_sound_init(void)
{
#define SOUND_MAX    (36)
    static char cmb_sound[SOUND_MAX];
    memset(cmb_sound, 0, SOUND_MAX);

    av_param_init(ITEM_SOUND_MODE, s_AvAppPara.volume_mode, cmb_sound, AV_SOUND_TOTAL,
                  sound_mode, NULL, _av_sound_cmb_press_cb);
}

#ifdef SUPPORT_TV_SCART
static int _av_scart_change(int sel)
{
#ifdef AUTO_CHANGE_SUPPORT
	AvAppPara AVPara;
	AvPlayerPara player;
	uint32_t scart[AV_SCART_TOTAL] = {VIDEO_OUTPUT_SOURCE_CVBS, VIDEO_OUTPUT_SOURCE_RGB};
	app_system_set_item_data_update(s_av_item_map[ITEM_TV_SCART]);
	app_av_app_para_get(&AVPara);
	app_av_app_to_player_para(&AVPara, &player);
#ifdef SUPPORT_TV_SCART
	player.scart_source = scart[sel];
	app_av_set_scart_source(&player);
#endif
#endif
	return 0;
}

static int _av_scart_cmb_press_cb(int key)
{
	static char* s_ScartData[AV_SCART_TOTAL] = {NULL};
	char ScartBuf[AV_SCART_TOTAL][15];
    AvAppPara appara = {0};
	int cmb_sel = -1, i = 0;

	for(i = 0; i < AV_SCART_TOTAL; i++)
	{
		memcpy(ScartBuf[i], scart[i], strlen(scart[i])>1?strlen(scart[i])-1:0);
		ScartBuf[i][strlen(scart[i])>1?strlen(scart[i])-1:0] = '\0';
        s_ScartData[i] = ScartBuf[i];
	}
	if((key == STBK_OK) || (key == STBK_RIGHT))
	{
		app_av_app_para_get(&appara);
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = item_avout[ITEM_TV_SCART];
		pop_list.item_num = AV_SCART_TOTAL;
		pop_list.item_content = s_ScartData;
		pop_list.sel = appara.tv_scart;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_2;
        pop_list.pos.x= 630;
        pop_list.pos.y= 260;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt5", "select", &cmb_sel);
	}

    if((key == STBK_LEFT) || (key == STBK_RIGHT))
        return EVENT_TRANSFER_STOP;

	return EVENT_TRANSFER_KEEPON;
}

static void app_av_scart_init(void)
{
#define SCART_MAX    (12)
    static char cmb_scart[SCART_MAX];
    memset(cmb_scart, 0, SCART_MAX);

    av_param_init(ITEM_TV_SCART, s_AvAppPara.tv_scart, cmb_scart, AV_SCART_TOTAL,
                  scart, _av_scart_change, _av_scart_cmb_press_cb);
}
#endif

static int _av_spdif_cmb_press_cb(int key)
{
	static char* s_SpdifData[AV_SPDIF_TOTAL] = {NULL};
	char SpdifBuf[AV_SPDIF_TOTAL][15];
    AvAppPara appara = {0};
	int cmb_sel = -1, i = 0;

	for(i = 0; i < AV_SPDIF_TOTAL; i++)
	{
		memcpy(SpdifBuf[i], spdif[i], strlen(spdif[i])>1?strlen(spdif[i])-1:0);
		SpdifBuf[i][strlen(spdif[i])>1?strlen(spdif[i])-1:0] = '\0';
        s_SpdifData[i] = SpdifBuf[i];
	}
	if(key == STBK_OK)
	{
		app_av_app_para_get(&appara);
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = item_avout[ITEM_SPDIF];
		pop_list.item_num = AV_SPDIF_TOTAL;
		pop_list.item_content = s_SpdifData;
		pop_list.sel = appara.spdif;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_3;
        pop_list.pos.x= 630;
        pop_list.pos.y= 260;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt5", "select", &cmb_sel);
	}

	return EVENT_TRANSFER_KEEPON;
}
static void app_av_spdif_init(void)
{
#define SPDIF_MAX    (36)
    static char cmb_spdif[SPDIF_MAX];
    memset(cmb_spdif, 0, SPDIF_MAX);

    av_param_init(ITEM_SPDIF, s_AvAppPara.spdif, cmb_spdif, AV_SPDIF_TOTAL,
                  spdif, NULL, _av_spdif_cmb_press_cb);
}

#ifdef AUDIO_DESCRIPTOR_SUPPORT

static int _ad_mode_change(int sel)
{
	//AvAppPara AVPara;
	AvPlayerPara player;
	//uint32_t ad[AUDIO_DESCRIPTOR_TOTAL] = {VIDEO_OUTPUT_SOURCE_CVBS, VIDEO_OUTPUT_SOURCE_RGB};
	app_system_set_item_data_update(s_av_item_map[ITEM_AD_MODE]);
	//app_av_app_para_get(&AVPara);
	//app_av_app_to_player_para(&AVPara, &player);
	player.ad_status= sel;
	app_audio_descriptor_set_mode(&player);

	return 0;
}

static int _av_ad_cmb_press_cb(int key)
{
	static char* s_ADData[AUDIO_DESCRIPTOR_TOTAL] = {NULL};
	char ADBuf[AUDIO_DESCRIPTOR_TOTAL][15];
    AvAppPara appara = {0};
	int cmb_sel = -1, i = 0;

	for(i = 0; i <AUDIO_DESCRIPTOR_TOTAL; i++)
	{
		memcpy(ADBuf[i], ad_status[i], strlen(ad_status[i])>1?strlen(ad_status[i])-1:0);
		ADBuf[i][strlen(ad_status[i])>1?strlen(ad_status[i])-1:0] = '\0';
        s_ADData[i] = ADBuf[i];
	}
	if((key == STBK_OK) || (key == STBK_RIGHT))
	{
		app_av_app_para_get(&appara);
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = item_avout[ITEM_AD_MODE];
		pop_list.item_num = AUDIO_DESCRIPTOR_TOTAL;
		pop_list.item_content = s_ADData;
		pop_list.sel = appara.ad_mode;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_2;
        pop_list.pos.x= 630;
        pop_list.pos.y= 291;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt6", "select", &cmb_sel);
	}

    if((key == STBK_LEFT) || (key == STBK_RIGHT))
        return EVENT_TRANSFER_STOP;

	return EVENT_TRANSFER_KEEPON;
}

static void app_audio_descriptor_item_init(void)
{
	#define AD_MAX    (10)
    static char cmb_ad[AD_MAX];
    memset(cmb_ad, 0, AD_MAX);

    av_param_init(ITEM_AD_MODE, s_AvAppPara.ad_mode, cmb_ad, AUDIO_DESCRIPTOR_TOTAL,
                  ad_status, _ad_mode_change, _av_ad_cmb_press_cb);
}
#endif

void app_av_setting_menu_exec(void)
{
	uint8_t i; 

	GxBus_ConfigGetInt(SPDIF_CONF_KEY, &s_spdif_enable, SPDIF_CONF);
#ifdef SUPPORT_TV_SCART
	GxBus_ConfigGetInt(SCART_CONF_KEY, &s_scart_enable, SCART_CONF);
#endif

	memset(&s_av_item_map, ITEM_AVOUT_TOTAL ,sizeof(s_av_item_map));
	for(i = ITEM_TV_STANDARD; i <= ITEM_SOUND_MODE; i++)
	{
		s_av_item_map[i] = i;
	}

    memset(&s_av_setting_opt, 0 ,sizeof(s_av_setting_opt));

	app_av_player_para_get(&s_AvPlayerPara);
	av_player_to_app_para(&s_AvPlayerPara, &s_AvAppPara);

	s_av_setting_opt.menuTitle = STR_ID_AV_SET;
	s_av_setting_opt.timeDisplay = TIP_HIDE;
	s_av_setting_opt.itemNum = ITEM_SOUND_MODE + 1;

	if(s_spdif_enable == 1)
	{
		s_av_setting_opt.itemNum++;
		s_av_item_map[ITEM_SPDIF] = i++;
	}
#ifdef SUPPORT_TV_SCART	
	if(s_scart_enable == 1)
	{
		s_av_setting_opt.itemNum++;
		s_av_item_map[ITEM_TV_SCART] = i++;
	}
#endif
#ifdef AUDIO_DESCRIPTOR_SUPPORT

	s_av_setting_opt.itemNum++;
	s_av_item_map[ITEM_AD_MODE] = i++;
	
#endif

	s_av_setting_opt.item = s_av_item;

	app_av_standard_init();
	app_av_ratio_init();
	app_av_aspect_init();
	app_av_sound_init();

	if(s_spdif_enable == 1)
	{
		app_av_spdif_init();
	}
#ifdef SUPPORT_TV_SCART
	if(s_scart_enable == 1)
	{
		app_av_scart_init();
	}
#endif
#ifdef AUDIO_DESCRIPTOR_SUPPORT
	app_audio_descriptor_item_init();
#endif
	s_av_setting_opt.exit = app_av_setting_exit_callback;
	app_system_set_create(&s_av_setting_opt);
    s_av_menu_falg = true;
}

status_t app_video_standard_hotkey(void)
{
#if 0
    if(s_av_menu_falg == true)
    {
        GUI_Event event = {0};

        event.type = GUI_KEYDOWN;
		event.key.sym = STBK_RIGHT;

        app_system_set_item_event(ITEM_TV_STANDARD, &event);
    }
    else
#endif
    {
        GxBus_ConfigGetInt(SPDIF_CONF_KEY, &s_spdif_enable, SPDIF_CONF);
#ifdef SUPPORT_TV_SCART
	    GxBus_ConfigGetInt(SCART_CONF_KEY, &s_scart_enable, SCART_CONF);
#endif
        app_av_player_para_get(&s_AvPlayerPara);
		s_vout_mode = s_AvPlayerPara.vout_mode;
        av_player_to_app_para(&s_AvPlayerPara, &s_AvAppPara);

        s_AvAppPara.tv_standard++;
        if(s_AvAppPara.tv_standard >= AV_STANDARD_TOTAL)
            s_AvAppPara.tv_standard = 1;

        if(s_av_menu_falg == true)
        {
			s_av_power_flag = true;
            s_av_item[s_av_item_map[ITEM_TV_STANDARD]].itemProperty.itemPropertyCmb.sel = s_AvAppPara.tv_standard;
            app_system_set_item_property(ITEM_TV_STANDARD, &s_av_item[s_av_item_map[ITEM_TV_STANDARD]].itemProperty);
			if (GXCORE_SUCCESS == GUI_CheckDialog("wnd_pop_option_list"))
			{
				GUI_SetProperty("listview_pop_option", "select",&(s_AvAppPara.tv_standard));
				pop_list.sel = s_AvAppPara.tv_standard;
				GUI_SetProperty("wnd_pop_option_list", "update_all", NULL);
			}
        }
        else
        {
            app_av_app_to_player_para(&s_AvAppPara,&s_AvPlayerPara);
			s_av_power_flag = true;
		    app_av_set_standard(&s_AvPlayerPara);
        }
    }

    return GXCORE_SUCCESS;
}

status_t app_volume_scope_status(void)
{
	int ret_value = 0;
	GxBus_ConfigGetInt(VOLUME_MODE_KEY, &ret_value, VOLUME_SCOMPE_);
	return ret_value;
}

