/*
 * =====================================================================================
 *
 *       Filename:  app_top.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2011年08月29日 15时10分04秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app.h"
#include "full_screen.h"
#include "gxextra.h"
#include "gxmsg.h"
#include "app_module.h"
#include "app_pop.h"
#include "app_bsp.h"
#include "app_book.h"
#include "app_key.h"
#include "app_default_params.h"
#include "app_epg.h"
#ifdef ECOS_OS
#include <cyg/io/gx3110_irr.h>
#endif

#include "app_msg.h"
#include "app_send_msg.h"
#if TKGS_SUPPORT
#include "tkgs/app_tkgs_background_upgrade.h"
#include "app_tkgs_upgrade.h"

#endif
#define SIMPLE_LOWPOWER_SUPPORT 0

typedef enum
{
	USB_PLUG_IN,
	USB_PLUG_OUT,
#if NETWORK_SUPPORT && WIFI_SUPPORT
	USB_WIFI_PLUG_IN,
	USB_WIFI_PLUG_OUT,
#endif
#if NETWORK_SUPPORT && MOD_3G_SUPPORT
    USB_3G_PLUG_IN,
    USB_3G_PLUG_OUT
#endif
}UsbPlugStatus;

#if NETWORK_SUPPORT && WIFI_SUPPORT
static event_list* s_usb_wifi_info_timer = NULL;
#endif
#if NETWORK_SUPPORT && MOD_3G_SUPPORT
static event_list *s_usb_3g_info_timer = NULL;
#endif
static event_list* s_usb_info_timer = NULL;
static bool s_avcodec_running = false;
static bool s_message_usbout_flag = false;
static bool s_simple_lowpower = false;

extern void tms_speed_change(float spd_speed);
extern void tms_state_change(PlayerStatus state);
extern void app_dm_list_hotplug_proc(GxMessage* msg);
extern void app_clean_format_status(GxMessage* msg);
extern status_t app_pvr_media_hotplug(char *hotplug_dev);
extern status_t app_mcentre_hotplug_out(char *hotplug_dev);
extern status_t app_mcentre_hotplug_in(void);
extern bool app_multiscreen_on(void);
extern void app_pvr_patition_full_proc(void);
extern void app_find_old_info_change(void);
void app_set_pvr_usbin_flag(bool bflag);

extern FullArb g_AppFullArb;
extern uint32_t app_get_panel_keycode(PanelKeymap key_index);
extern bool app_check_play_exec_flag(void);
extern status_t app_video_standard_hotkey(void);
extern int app_get_keymap_data(const char *key_name, unsigned int **key_val);
//extern status_t app_epg_record_status_running(void);
extern void zoom_refresh(void);
extern void app_set_hardware_mute(void);

//#ifdef LINUX_OS
#if NETWORK_SUPPORT
extern void app_network_msg_proc(GxMessage *msg);

#if WIFI_SUPPORT
extern void app_wifi_msg_proc(GxMessage *msg);
extern int g_wifi_hotplug_status;
#endif

#if MOD_3G_SUPPORT
extern void app_3g_msg_proc(GxMessage *msg);
#endif

#if BOX_SERVICE
extern status_t app_box_set_diseqc(int prog_id);
#if DLNA_SUPPORT
extern void app_dlna_msg_proc(GxMessage *msg);
#endif
#endif

#if IPTV_SUPPORT
extern int app_iptv_browser_msg_proc(GxMessage *msg);
extern int app_iptv_play_msg_proc(GxMessage *msg);
extern int app_iptv_recovery_status(void);
#endif

#endif // NETWORK_SUPPORT
#ifdef NETAPPS_SUPPORT
extern int app_net_video_avcodec_status(GxMessage* msg);
extern int app_net_video_play_status(GxMessage* msg);
#endif

static event_list* s_panel_led_timer = NULL;
static event_list* s_simple_wakeup_timer = NULL;
static bool s_reboot_flag = false;

static GxTime  time_start;//test 1203
static GxTime  time_end;
static float  switch_min = 50.0;
static float  switch_max = 0.0;
static float  switch_avg = 0.0;
static float  switch_sum = 0.0;
static int    switch_count = 0;
#if TKGS_SUPPORT
void app_tkgs_power_on(void);
void app_tkgs_power_off(time_t sleep_time);
extern APP_TKGS_UPGRADE_MODE app_tkgs_get_upgrade_mode(void);
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
#endif
#ifndef ECOS_OS
typedef struct lowpower_info_s
{
    unsigned int WakeTime;
    unsigned int GpioMask;
    unsigned int GpioData;
    unsigned int	key;
	unsigned char	*cmdline;
}lowpower_info;
#define IRR_LOWPOWER 0x2000
#endif

void app_simple_power_on(void)
{
    GpioData val = {0};
    unsigned int led_time = PANEL_END_TIME;
    uint8_t i = 0;
    unsigned int standby = 0;
    AppFrontend_Monitor monitor = FRONTEND_MONITOR_ON;

    if(s_simple_wakeup_timer != NULL)
	{
		remove_timer(s_simple_wakeup_timer);
		s_simple_wakeup_timer = NULL;
	}

	if(s_panel_led_timer != NULL)
	{
		remove_timer(s_panel_led_timer);
		s_panel_led_timer = NULL;
	}

    ioctl(bsp_panel_handle, PANEL_SHOW_STANDBY, &standby);
	ioctl(bsp_panel_handle, PANEL_SHOW_TIME, &led_time);
    // for unmute
    {
        extern void app_set_hardware_unmute(void);
        app_set_hardware_unmute();
    }

    for (i=0; i<NIM_MODULE_NUM; i++)
    {
        app_ioctl(i, FRONTEND_MONITOR_SET, &monitor);
    }

    val.port = V_GPIO_LNB_A;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_HIGH, &val);

    val.port = V_GPIO_LNB_B;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_HIGH, &val);

    val.port = V_GPIO_USB_A;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_HIGH, &val);

    val.port = V_GPIO_USB_B;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_HIGH, &val);

    GxCore_ThreadDelay(2000); //wait lock tp

    int dev  = GxAvdev_CreateDevice(0);
	handle_t vo   = GxAvdev_OpenModule(dev, GXAV_MOD_VIDEO_OUT, 0);
	handle_t vo1  = GxAvdev_OpenModule(dev, GXAV_MOD_VIDEO_OUT, 1);

    GxAvdev_CloseModule(dev, vo);
    GxAvdev_CloseModule(dev, vo1);
    GxAvdev_DestroyDevice(dev);

    if (g_AppFullArb.state.mute == STATE_OFF)
    {
        GxMsgProperty_PlayerAudioMute player_mute = 0;
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
    }
}

static int _panel_show_time(void *userdata)
{
    unsigned int led_time = 0;

    led_time = g_AppTime.utc_get(&g_AppTime);
    led_time = get_display_time_by_timezone(led_time);
    ioctl(bsp_panel_handle, PANEL_SHOW_TIME, &led_time);

    return 0;
}

static int _simple_power_wakeup(void *userdata)
{
    app_simple_power_on();
    s_simple_lowpower = false;
    return 0;
}

void app_simple_power_off(time_t sleep_time)
{
    GpioData val = {0};
    uint8_t i =0;
    unsigned int lock = 0;
    unsigned int standby = 1;
    AppFrontend_Monitor monitor = FRONTEND_MONITOR_OFF;

    _panel_show_time(NULL);
    if (reset_timer(s_panel_led_timer) != 0)
	{
		s_panel_led_timer = create_timer(_panel_show_time, 1000, NULL, TIMER_REPEAT);
	}

    if (g_AppFullArb.state.mute == STATE_OFF)
    {
        GxMsgProperty_PlayerAudioMute player_mute = 1;
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
    }

    int dev  = GxAvdev_CreateDevice(0);
	handle_t vo   = GxAvdev_OpenModule(dev, GXAV_MOD_VIDEO_OUT, 0);
	handle_t vo1  = GxAvdev_OpenModule(dev, GXAV_MOD_VIDEO_OUT, 1);

    GxAvdev_CloseModule(dev, vo);
    GxAvdev_CloseModule(dev, vo1);
    GxAvdev_DestroyDevice(dev);

    for (i=0; i<NIM_MODULE_NUM; i++)
    {
        app_ioctl(i, FRONTEND_MONITOR_SET, &monitor);
    }

    ioctl(bsp_panel_handle, PANEL_SHOW_LOCK, &lock);
    ioctl(bsp_panel_handle, PANEL_SHOW_STANDBY, &standby);
 // for mute
    app_set_hardware_mute();


    val.port = V_GPIO_LNB_A;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_LOW, &val);

    val.port = V_GPIO_LNB_B;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_LOW, &val);

    val.port = V_GPIO_USB_A;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_LOW, &val);

    val.port = V_GPIO_USB_B;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_LOW, &val);

    if(sleep_time > 0)
    {
        if (reset_timer(s_simple_wakeup_timer) != 0)
        {
            s_simple_wakeup_timer = create_timer(_simple_power_wakeup, sleep_time * 1000, NULL, TIMER_ONCE);
        }
	}
}

//plz free result if != NULL
char *_get_lowpower_cmdline(void)
{
    unsigned int *val = NULL;
    int num = 0;
    int i = 0;
    int cmd_len = 0;
    char *cmd = NULL;
    char temp[20] = {0};

    num = app_get_keymap_data("GUIK_H", &val);
    if(num > 0)
    {
        cmd_len = num * 11 + 6;
        cmd = (char*)GxCore_Calloc(1, cmd_len);
        if(cmd != NULL)
        {
            strcat(cmd, "keys=");
            for(i = 0; i < num; i++)
            {
                //printf("[%s]%d %d/%d: 0x%x\n", __FILE__, __LINE__, i, num, val[i]);
                memset(temp, 0, sizeof(temp));
                sprintf(temp, "0x%x,", val[i]);
                strcat(cmd, temp);
            }
            cmd_len = strlen(cmd);
            cmd[cmd_len - 1] = 0;

            printf("####[%s]%d cmd: %s####\n", __func__, __LINE__, cmd);
        }
        GxCore_Free(val);
        val = NULL;
    }

    return cmd;
}

void app_update_sync_data(void)
{
    s_reboot_flag = false;
	app_send_msg_exec(GXMSG_PM_CLOSE, NULL);//new 20130608
	GxCore_Close(bsp_panel_handle);//close panel
	g_AppTime.stop(&g_AppTime);
	g_AppFullArb.timer_stop();

	return;
}

void app_update_reboot(void)
{
#ifdef LINUX_OS
    system("reboot");
#else
    int fd = 0;
	lowpower_info lowpower;

	app_set_hardware_mute();
	lowpower.WakeTime = 1;
	lowpower.GpioMask = 0xffffffff;
	lowpower.GpioData = 0;
	lowpower.key = 0x00;
	lowpower.cmdline = (unsigned char*)"0xbfaf,0xbbaf";//(unsigned char*)_get_lowpower_cmdline();

	fd = open("/dev/gxirr0", O_RDWR);
	ioctl(fd, IRR_LOWPOWER, &lowpower);
    GxCore_Free(lowpower.cmdline);
#endif
	return;
}

void app_low_power(time_t sleep_time, uint32_t scan_code)
{
#if SIMPLE_LOWPOWER_SUPPORT > 0
    if(s_reboot_flag == false)
    {
        if(s_simple_lowpower == false)
        {
            s_simple_lowpower = true;
            app_simple_power_off(sleep_time);
        }
        else
        {
            s_simple_lowpower = false;
            app_simple_power_on();
        }
        return;
    }
#endif
//#ifdef ECOS_OS
#if TKGS_SUPPORT
if(sleep_time == 0&&scan_code != 0)
{
    if(s_reboot_flag == false)
    {
        if(s_simple_lowpower == false)
        {
            s_simple_lowpower = true;
            app_tkgs_power_off(60);
        }
        else
        {
            s_simple_lowpower = false;
            app_tkgs_power_on();
        }
        return;
    }

}
#endif
	app_send_msg_exec(GXMSG_PM_CLOSE, NULL);//new 20130608
#ifndef MCU_LOWPOWER
// for panel
    unsigned int lock = 0;
    unsigned int standby = 1;
    ioctl(bsp_panel_handle, PANEL_SHOW_LOCK, &lock);
    ioctl(bsp_panel_handle, PANEL_SHOW_STANDBY, &standby);
   
 // for mute
    //GxMsgProperty_PlayerAudioMute player_mute = 0;
    //app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));

    app_set_hardware_mute();


	GxCore_Close(bsp_panel_handle);//close panel

	int fd = 0;
	lowpower_info lowpower;

#ifndef SHOWTIME_LOWPOWER
	lowpower.WakeTime = sleep_time;
	lowpower.GpioMask = 0xffffffff;
	lowpower.GpioData = 0;
	lowpower.key = 0x00;
	lowpower.cmdline = (unsigned char*)_get_lowpower_cmdline();
#else
    struct tm* time;
    time_t seconds;

    seconds = g_AppTime.seconds + g_AppTime.config.zone*3600;
    seconds += g_AppTime.config.summer*3600;

    time = localtime(&seconds);
    lowpower.WakeTime = sleep_time;
    //lowpower.GpioMask = time->tm_hour;
    //lowpower.GpioData = time->tm_min;
#endif

	if(scan_code == 0)
		scan_code = app_get_panel_keycode(PANEL_KEYMAP_PW);

	fd = open("/dev/gxirr0", O_RDWR);

	ioctl(fd, IRR_LOWPOWER, &lowpower);
    GxCore_Free(lowpower.cmdline);
#else
	if(sleep_time != 0)// for update reboot or book reboot
	{
		GxCore_Close(bsp_panel_handle);//close panel

		int fd = 0;
		lowpower_info lowpower;

		lowpower.WakeTime = sleep_time;
		lowpower.GpioMask = 0xffffffff;
		lowpower.GpioData = 0;
		//lowpower.GpioNum = 0xffffffff;
		//lowpower.GpioLevel = 0xffffffff;

		if(scan_code == 0)
			scan_code = app_get_panel_keycode(PANEL_KEYMAP_PW);

		lowpower.key = (unsigned char)(scan_code&0xff);
		fd = open("/dev/gxirr0", O_RDWR);

		ioctl(fd, IRR_LOWPOWER, &lowpower);
	}
	else
		ioctl(bsp_panel_handle, PANEL_MCU_LOWPOWER, "");
#endif
//#else
#ifndef ECOS_OS
    system("halt");//system("lowpower");
#endif
//#endif
}
#if TKGS_SUPPORT
 extern uint8_t s_tkgs_force_stop ;
/*
* 退出到指定界面后，响应对应的遥控器或创建指定的窗体
*/
void app_win_exist_to_win_widget(const char* widget_win_name)
{
	GUI_Event keyEvent = {0};
	int rtn = 1;
//	if (TRUE == app_get_pop_msg_flag_status())
//		return;

	if (NULL != widget_win_name)
		{
			char* focus_Window = (char*)GUI_GetFocusWindow();
			//printf("%s %d app_win_exist_to_win_widget focus_Window=%s\n",__FILE__,__LINE__,focus_Window);
			if (NULL != focus_Window)
				{
					rtn = strcasecmp(widget_win_name, focus_Window);
					while(rtn != 0)
						{
						
							if (NULL != focus_Window)
								{
									//printf("%s %d app_win_exist_to_win_widget focus_Window=%s\n",__FILE__,__LINE__,focus_Window);

									if ((0 == strcasecmp("wnd_full_screen" ,widget_win_name))&&
										((0 == strcasecmp("wnd_main_menu" ,focus_Window))||(0 == strcasecmp("wnd_main_menu_item" ,focus_Window))
										||(0 == strcasecmp("wnd_tkgs_upgrade_process" ,focus_Window))||(0 == strcasecmp("wnd_pop_tip" ,focus_Window))
										||(0 == strcasecmp("wnd_channel_info" ,focus_Window))||(0 == strcasecmp("wnd_audio" ,focus_Window))
										||(0 == strcasecmp("wnd_subtitling" ,focus_Window))||(0 == strcasecmp("wnd_subtitling" ,focus_Window))))
										{
											/*
											* 系统设置、网络多媒体退出时重新播放节目，直接关闭窗体
											* 避免退出到全屏前，播放上一节目
											*/
											GUI_EndDialog(focus_Window);
										}
										else
											if ((0 == strcasecmp("win_pic_view" ,focus_Window))
												||(0 == strcasecmp("win_music_view" ,focus_Window)))
											{
												/*
												* 对于会引起死锁界面，直接GUI_EndDialog。
												*/
												GUI_EndDialog(focus_Window);												
											}
											else
											{
												keyEvent.type = GUI_KEYDOWN;
												keyEvent.key.sym = STBK_EXIT;	
												GUI_SendEvent(focus_Window,&keyEvent);													
											}

								}
//							GxCore_ThreadDelay(50);
							GUI_LoopEvent();
							rtn = 1;
							focus_Window = (char*)GUI_GetFocusWindow();

							if (NULL != focus_Window)
								{
									rtn = strcasecmp(widget_win_name, focus_Window);
									if (0 == rtn)
										{
										//	printf("%s %d app_win_exist_to_win_widget focus_Window=%s\n",__FILE__,__LINE__,focus_Window);
										}
								}
						}
				}
		}

	//printf("%s %d app_win_exist_to_win_widget end\n",__FILE__,__LINE__); 
	
	return ;
}
static int _tkgs_real_lowpower(void *userdata)
{
	int fd = 0;
	lowpower_info lowpower;

	lowpower.WakeTime = 0;
	lowpower.GpioMask = 0xffffffff;
	lowpower.GpioData = 0;
	lowpower.key = 0x00;
	lowpower.cmdline = (unsigned char*)_get_lowpower_cmdline();

	fd = open("/dev/gxirr0", O_RDWR);
	ioctl(fd, IRR_LOWPOWER, &lowpower);
    GxCore_Free(lowpower.cmdline);

	return 0;
}
bool app_get_tkgs_power_status(void)
{
return s_simple_lowpower;
}
void app_tkgs_power_on(void)
{

    if(s_simple_wakeup_timer != NULL)
	{
		remove_timer(s_simple_wakeup_timer);
		s_simple_wakeup_timer = NULL;
	}


    int dev  = GxAvdev_CreateDevice(0);
	handle_t vo   = GxAvdev_OpenModule(dev, GXAV_MOD_VIDEO_OUT, 0);
	handle_t vo1  = GxAvdev_OpenModule(dev, GXAV_MOD_VIDEO_OUT, 1);

	//GxVideoOutProperty_Visible vissible;
    //visible.enable = 1;

    //GxAVSetProperty(dev, vo, GxVideoOutPropertyID_Visible, &visible, sizeof(GxVideoOutProperty_Visible));
    //GxAVSetProperty(dev, vo1, GxVideoOutPropertyID_Visible, &visible, sizeof(GxVideoOutProperty_Visible));

    GxAvdev_CloseModule(dev, vo);
    GxAvdev_CloseModule(dev, vo1);
    GxAvdev_DestroyDevice(dev);

// for panel
    unsigned int standby = 0;
    ioctl(bsp_panel_handle, PANEL_SHOW_STANDBY, &standby);
 // for unmute
    {
        extern void app_set_hardware_unmute(void);
        app_set_hardware_unmute();
    }

#if 1
    GUI_SetInterface("video_enable", NULL);

					if (g_AppPlayOps.normal_play.play_total != 0)
					{
					// deal for recall , current will copy to recall
						g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT,
	                                                                    g_AppPlayOps.normal_play.play_count);

						if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
							g_AppFullArb.tip = FULL_STATE_LOCKED;
						else
							g_AppFullArb.tip = FULL_STATE_DUMMY;
						if(GUI_CheckDialog("wnd_passwd") != GXCORE_SUCCESS)
						{
						//	GUI_CreateDialog("wnd_channel_info");
						}

					}
			#endif

    if (g_AppFullArb.state.mute == STATE_OFF)
    {
        GxMsgProperty_PlayerAudioMute player_mute = 0;
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
    }
}

void app_tkgs_power_off(time_t sleep_time)
{
    GxTkgsLocation s_tkgsLocation;			
    s_tkgsLocation.visible_num = 0;
    s_tkgsLocation.invisible_num = 0;
    app_send_msg_exec(GXMSG_TKGS_GET_LOCATION,&s_tkgsLocation);


    app_win_exist_to_win_widget("wnd_full_screen");
#if 1
    PlayerWindow video_wnd = {0, 0, 1024, 576};
    g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
    GUI_SetInterface("flush",NULL);
    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);//目的处理进节目编辑后退出导致主界面变灰的问题

#endif
    g_AppFullArb.timer_stop();
    g_AppFullArb.state.pause = STATE_OFF;
    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    g_AppPlayOps.program_stop();

    if (g_AppFullArb.state.mute == STATE_OFF)
    {
        GxMsgProperty_PlayerAudioMute player_mute = 1;
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
    }

    int dev  = GxAvdev_CreateDevice(0);
    handle_t vo   = GxAvdev_OpenModule(dev, GXAV_MOD_VIDEO_OUT, 0);
    handle_t vo1  = GxAvdev_OpenModule(dev, GXAV_MOD_VIDEO_OUT, 1);

    //GxVideoOutProperty_Visible visible;
    //visible.enable = 0;

    //GxAVSetProperty(dev, vo, GxVideoOutPropertyID_Visible, &visible, sizeof(GxVideoOutProperty_Visible));
    //GxAVSetProperty(dev, vo1, GxVideoOutPropertyID_Visible, &visible, sizeof(GxVideoOutProperty_Visible));

    GxAvdev_CloseModule(dev, vo);
    GxAvdev_CloseModule(dev, vo1);
    GxAvdev_DestroyDevice(dev);

    // for panel
    unsigned int lock = 0;
    unsigned int standby = 1;
    ioctl(bsp_panel_handle, PANEL_SHOW_LOCK, &lock);
    ioctl(bsp_panel_handle, PANEL_SHOW_STANDBY, &standby);
    // for mute

    app_set_hardware_mute();


    if((s_tkgsLocation.visible_num>0)
            ||(s_tkgsLocation.invisible_num>0))
    {
        if(app_tkgs_get_upgrade_mode() != TKGS_UPGRADE_MODE_STANDBY && s_tkgs_force_stop == 0)
        {
            app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_STANDBY);
            if(app_tkgs_get_operatemode() == GX_TKGS_OFF)
                _tkgsupgradeprocess_upgrade_version_check(0);
            else
                app_tkgs_upgrade_process_menu_exec();
        }
    }
    else
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_TKGS_SET_LOCATION;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        popdlg_create(&pop);
    }

    if(sleep_time > 0)
    {
        if (reset_timer(s_simple_wakeup_timer) != 0)
        {
            s_simple_wakeup_timer = create_timer(_tkgs_real_lowpower, sleep_time * 1000, NULL, TIMER_ONCE);
        }
    }
}
#endif

void app_reboot(void)
{
    s_reboot_flag = false;
    //GxBus_PmDbaseClose();
#ifndef ECOS_OS
    system("reboot");
#else
    app_low_power(2,0);
#endif
}

void app_set_avcodec_flag(bool state)
{
    s_avcodec_running = state;
}


static int top_avcodec_report(GxMessage* msg)
{
    GxMsgProperty_PlayerAVCodecReport *avcodec =  GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerAVCodecReport);
    PlayerProgInfo  info;
    int32_t ret = 0;
    GxMsgProperty_NodeByPosGet node = {0};
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.current_play.play_count;
    int32_t prog_id;

    app_getvalue(avcodec->url,"progid",&prog_id);//why progid? please see the function app_normal_play()

    if(strcmp(avcodec->player, PLAYER_FOR_NORMAL) == 0)
    {
        if(app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)) != GXCORE_SUCCESS)
        {
            printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
            return -1;
        }

        if(avcodec->vcodec.state == AVCODEC_RUNNING) //tv
        {
            if(g_AppFullArb.state.tv == STATE_ON) 
            {
		    		float  switch_interval = 0.0;
				GxCore_GetTickTime(&time_end);
		    		switch_interval = ((time_end.seconds - time_start.seconds)*1000000.0 - time_start.microsecs+ time_end.microsecs)/1000000.0;
				if(time_start.seconds > 0)
				{
				switch_sum += switch_interval;
				if( switch_interval > switch_max) switch_max = switch_interval;
				if( switch_interval < switch_min) switch_min = switch_interval;
				switch_count++;
				switch_avg = switch_sum / switch_count;
				}
				if(GUI_CheckDialog("wnd_epg") == GXCORE_SUCCESS || GUI_CheckDialog("wnd_channel_edit") == GXCORE_SUCCESS )
				{
					GUI_SetInterface("video_top", NULL);//错误 #44303

				}

				printf("==========>Render Video time: %d s, %d us\n", time_end.seconds, time_end.microsecs);
				printf("==========>Swtich Video interval: %.3f s\n", switch_interval);
				printf("==========>Swtich Video (min, max, avg, count) = (%.3f s, %.3f s, %.3f s, %d)\n", switch_min, switch_max, switch_avg, switch_count);
                //if(strcmp(full_url, avcodec->url) == 0) //current prog
                if(prog_id == node.prog_data.id)
                {
                    g_AppFullArb.scramble_set(2);
                    app_set_avcodec_flag(true);
                    GxTime time_test = {0};
                    GxCore_GetTickTime(&time_test);
                    ret = GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, &info); //solution check
                    if(ret == 0)
                    {
                        GxMsgProperty_NodeByPosGet prog_node = {0};
                        char solution_str[15] = {0};

                        sprintf(solution_str, "%d x %d", info.video.width, info.video.height);
                        GUI_SetProperty("text_channel_info_solution", "string", solution_str);

                        prog_node.node_type = NODE_PROG;
                        prog_node.pos = g_AppPlayOps.current_play.play_count;
                        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&prog_node);
#if 0
                        AppAppProgExtInfo prog_ext;
                        memset(&prog_ext, 0, sizeof(AppAppProgExtInfo));
                        if(GxBus_PmProgExtInfoGet(prog_node.prog_data.id, sizeof(AppAppProgExtInfo), (void*)&prog_ext) == GXCORE_SUCCESS)
                        {
                            if(prog_ext.video_solution.width != info.video.width
                                || prog_ext.video_solution.height != info.video.height)
                            {
                                prog_ext.video_solution.width = info.video.width;
                                prog_ext.video_solution.height = info.video.height;
                                GxBus_PmProgExtInfoAdd(prog_node.prog_data.id, sizeof(AppAppProgExtInfo), (void*)&prog_ext);
                            }
                        }
                        else
                        {
                            prog_ext.video_solution.width = info.video.width;
                            prog_ext.video_solution.height = info.video.height;
                            GxBus_PmProgExtInfoAdd(prog_node.prog_data.id, sizeof(AppAppProgExtInfo), (void*)&prog_ext);
                        }
#endif
						prog_node.node_type = NODE_PROG;
						prog_node.pos = g_AppPlayOps.current_play.play_count;
						app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&prog_node);

                        if((info.video.height > 576) && (info.video.width > 720))
                        {
                            GxMsgProperty_NodeModify node_modify = {0};

                           
                            node_modify.node_type = NODE_PROG;
                            memcpy(&node_modify.prog_data, &prog_node.prog_data, sizeof(GxBusPmDataProg));
                            node_modify.prog_data.definition = GXBUS_PM_PROG_HD;
#if (1 != TKGS_SUPPORT)                        							
                            node_modify.prog_data.favorite_flag |= 0x01;
#endif 
                            if(memcmp(&prog_node.prog_data, &node_modify.prog_data, sizeof(GxBusPmDataProg)) != 0)
                            {
                                if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
                                {
                                    printf("\nmodify program error\n");
                                }
                            }
                            if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                            {
                                #define  IMG_HD	"img_progbar_hd"
                                GUI_SetProperty(IMG_HD, "img", "s_bar_hd.bmp");
                            }
                        }
                        else
                        {
                            if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                            {
                            	if(prog_node.prog_data.definition!=GXBUS_PM_PROG_HD)
                            	{
                                	#define  IMG_HD	"img_progbar_hd"
                                	GUI_SetProperty(IMG_HD, "img", "s_bar_hd_unfocus.bmp");
                            	}
                            }
                        }

                        #if 0
                        if(GUI_CheckDialog("wnd_channel_edit") == GXCORE_SUCCESS)
                        {
                            #define LISTVIEW_CH_ED      "listview_channel_edit"
                            int32_t sel =  g_AppPlayOps.normal_play.play_count;
                            if (g_AppChOps.check(sel, MODE_FLAG_FAV) == false)
                            {
                                g_AppChOps.set(sel, 1);
                                GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                            }
                        }
                        #endif
                    }
                }
            }
        }

        if(avcodec->acodec.state == AVCODEC_RUNNING) //radio
        {
            if(g_AppFullArb.state.tv == STATE_OFF)
            {
                //if(strcmp(full_url, avcodec->url) == 0) //current prog
                if(prog_id == node.prog_data.id)
                {
                    g_AppFullArb.scramble_set(2);
                    app_set_avcodec_flag(true);
                }
            }
        }

        if((avcodec->vcodec.state == AVCODEC_STOPPED) //tv
            || (avcodec->vcodec.state == AVCODEC_READY))
        {
        //add &&  g_AppFullArb.tip != FULL_STATE_DUMMY
            if(g_AppFullArb.state.tv == STATE_ON &&  g_AppFullArb.tip != FULL_STATE_DUMMY)
            {
                //g_AppFullArb.scramble_set(1);
                app_set_avcodec_flag(false);
                if (GXCORE_SUCCESS == GUI_CheckDialog("wnd_channel_edit"))
                {
                    GUI_SetInterface("video_off", NULL);
                }
            }
        }

        if((avcodec->acodec.state == AVCODEC_STOPPED) //radio
            ||(avcodec->acodec.state == AVCODEC_READY))
        {
            if(g_AppFullArb.state.tv == STATE_OFF && g_AppFullArb.tip != FULL_STATE_DUMMY)
            {
                app_set_avcodec_flag(false);
            }
        }
    }
    return EVENT_TRANSFER_KEEPON;
}

static unsigned char thiz_scrambler_delay_display = 0;
static event_list* thiz_descrambler_status_delay_timer = NULL;

unsigned char app_get_descrambler_delay_flag(void)
{

    //printf("\nCAS, get delay flag = %d\n", thiz_scrambler_delay_display);
    return thiz_scrambler_delay_display;
}

void app_set_descrambler_delay_flag(unsigned char flag)
{
    thiz_scrambler_delay_display = flag;
    //printf("\nCAS, set delay flag = %d\n", flag);
}

void app_descrambler_delay_timer_destroy(void)
{
    if(thiz_descrambler_status_delay_timer != NULL)
	{
		remove_timer(thiz_descrambler_status_delay_timer);
		thiz_descrambler_status_delay_timer = NULL;
	}
}

static int _scrambler_status_display_control(void* usrdata)
{
    if(thiz_descrambler_status_delay_timer != NULL)
	{
		remove_timer(thiz_descrambler_status_delay_timer);
		thiz_descrambler_status_delay_timer = NULL;
	}

    app_set_descrambler_delay_flag(1);
    return 0;
}

static int top_play_status_report(GxMessage* msg)
{
    GxMsgProperty_PlayerStatusReport *status;
    status = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);

    if(NULL == status)                                                        
    {                                                                         
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);                  
        return -1;                                                            
    }                                                                         

    int32_t prog_id = 0;                                                      
    app_getvalue(status->url,"progid",&prog_id);                              
    if((0 == prog_id) || (prog_id == 0x7fffffff))                             
    {                                                                         
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);                  
        return -1;                                                            
    }                                 

    //receive pvr full
    if (strcmp((char*)(status->player), PLAYER_FOR_REC) == 0)
    {
        if (status->status == PLAYER_STATUS_RECORD_FULL)
        {
            app_pvr_patition_full_proc();
        }
        else if(status->status == PLAYER_STATUS_RECORD_RUNNING)
        {
#if CA_SUPPORT
		// TODO: 20160302
#if ECM_SUPPORT
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
            if(app_tsd_check())
            {
                return EVENT_TRANSFER_KEEPON;
            }
            if(0 == g_AppEcm2.ecm_count)
            {
                memcpy(&g_AppEcm2,&g_AppEcm,sizeof(AppEcm));
                //g_AppEcm2.DemuxId = 1;// TODO: 20120702
                app_getvalue(g_AppPvrOps.env.url,"dmxid",&g_AppEcm2.DemuxId);
                g_AppEcm2.CaManager.ProgId = g_AppPvrOps.env.prog_id;
                g_AppEcm2.CaManager.Type = CA_PLAYER_TYPE_PVR;
                g_AppEcm2.init(&g_AppEcm2);
                g_AppEcm2.start(&g_AppEcm2);
            }
#endif //DUAL_CHANNEL_DESCRAMBLE_SUPPORT
#endif //ECM_SUPPORT
#endif //CA_SUPPORT
        }
    }
    else if (strcmp((char*)(status->player), PLAYER_FOR_NORMAL) == 0)
    {
        // 目前时移和普通播放使用的都是"PLAYER_FOR_NORMAL"，该配?糜捎τ每刂?
        if(status->status == PLAYER_STATUS_PLAY_RUNNING)
        {
#if 1
        #if TKGS_SUPPORT
		if(g_AppTkgsBackUpgrade.search_status & TKGS_BACKUPGRADE_START)
		{
			g_AppTkgsBackUpgrade.ts_src = g_AppPlayOps.normal_play.ts_src;
			g_AppTkgsBackUpgrade.start();
		}
		else if(g_AppTkgsBackUpgrade.search_status & TKGS_BACKUPGRADE_STOP)
		{
			g_AppPmt.ts_src = g_AppPlayOps.normal_play.ts_src;
			g_AppPmt.start(&g_AppPmt);
			printf("startPmt\n");
		}
		#else
		g_AppPmt.ts_src = g_AppPlayOps.normal_play.ts_src;
		g_AppPmt.start(&g_AppPmt);
		#endif
#if 1
        if(0 != reset_timer(thiz_descrambler_status_delay_timer))
        {
            thiz_descrambler_status_delay_timer = 
                    create_timer(_scrambler_status_display_control, 4000, NULL, TIMER_REPEAT);
        }
#endif

#else
            if(0 != reset_timer(s_play_run_exec_timer))
            {
                s_play_run_exec_timer = create_timer(_play_run_exec, 60, NULL, TIMER_REPEAT);
            }
#endif

            /******************************************************************************/
            // TODO: for DEMUX 3 PID config, tdt/tot(0x14),epg(0x12), cat(0x01),pat(0x00),pmt()
#if CA_SUPPORT
            {
                if(app_tsd_check())
                {
                    int pidnum = 5;
                    int PidData[5] = {0x14,0x12,0x00,0x01};
                    GxMsgProperty_NodeByPosGet node_prog = {0};
                    node_prog.node_type = NODE_PROG;
                    node_prog.pos = g_AppPlayOps.normal_play.play_count;
                    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog))
                    {
                        PidData[4] = node_prog.prog_data.pmt_pid;
                        app_exshift_pid_add(pidnum, PidData);
                    }
                    else
                    {
                        printf("\nERROR, %s, %d\n",__FUNCTION__,__LINE__);
                    }
                }
            }
#endif
        }

        tms_state_change(status->status);
    }

    return EVENT_TRANSFER_KEEPON;
}

static int top_play_speed_report(GxMessage* msg)
{
    GxMsgProperty_PlayerSpeedReport *spd;
    spd = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerSpeedReport);
    if(spd != NULL)
    {
        if (strcmp((char*)(spd->player), PLAYER_FOR_NORMAL) == 0)
        {
            tms_speed_change(spd->speed);
        }
    }

    return EVENT_TRANSFER_KEEPON;
}

static int top_play_resolution_report(GxMessage *msg)
{
    GxMsgProperty_PlayerResolutionReport *value;
    value = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerResolutionReport);

    //GxBus_ConfigSetInt(TV_STANDARD_KEY, value->resolution);
    
    return EVENT_TRANSFER_KEEPON;
}

static int top_hotplug_in(GxMessage* msg)
{
	 if(GUI_CheckDialog("win_file_browser") == GXCORE_SUCCESS) //media centre
	 {
	 	app_mcentre_hotplug_in();
	 }
	 else if(GUI_CheckDialog("wnd_setting_dm") == GXCORE_SUCCESS)
	 {
	 	app_dm_list_hotplug_proc(msg);
	 }

	 g_AppPvrOps.tms_delete(&g_AppPvrOps);
	
	 return EVENT_TRANSFER_STOP;
}
static int plug_exit_pop_cb(PopDlgRet ret)
{
	if (g_AppFullArb.state.tv == STATE_OFF)
	{
		GUI_CreateDialog("wnd_channel_info");
	}
	  return 0;
}
static int top_hotplug_out(GxMessage* msg)
{
    GxMsgProperty_HotPlug *hot_plug = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_HotPlug);
    if(GUI_CheckDialog("wnd_pop_tip") == GXCORE_SUCCESS)
	{
		popdlg_destroy();
	}
    if(GUI_CheckDialog("win_file_browser") == GXCORE_SUCCESS) //media centre
    {
        app_mcentre_hotplug_out(hot_plug->dev_name);
    }
    else if(GUI_CheckDialog("wnd_pvr_media_list") == GXCORE_SUCCESS) //pvr list
    {
        app_pvr_media_hotplug(hot_plug->dev_name);
    }
    else if(GUI_CheckDialog("wnd_setting_dm") == GXCORE_SUCCESS)// dm setting
    {
        app_dm_list_hotplug_proc(msg);
    }

    if ((g_AppPvrOps.state != PVR_DUMMY)
            && (hot_plug->dev_name != NULL)
            && (g_AppPvrOps.env.dev != NULL)
            && (strncmp(hot_plug->dev_name, g_AppPvrOps.env.dev,strlen(hot_plug->dev_name)) == 0))
    {
        PopDlg  pop;
        uint32_t rec_prog_id = g_AppPvrOps.env.prog_id;

        s_message_usbout_flag = true;

        //        pop.pos.x = 280;
        //        pop.pos.y = 100;

        app_pvr_stop();
        GUI_EndDialog("wnd_pvr_bar");
        GUI_EndDialog("wnd_channel_info_signal");
        GUI_EndDialog("wnd_volume_value");

        if((g_AppPlayOps.normal_play.view_info.tv_prog_cur == rec_prog_id && 
                    g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)||
                (g_AppPlayOps.normal_play.view_info.radio_prog_cur == rec_prog_id &&
                 g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO))
        {
            if(GUI_CheckDialog("wnd_find") == GXCORE_SUCCESS)
            {
                GxBusPmViewInfo view_info = {0};
                if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info)))
                {
                    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET,  (void *)(&g_AppPlayOps.normal_play.view_info));
                    g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
                    g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
                    g_AppPmt.start(&g_AppPmt);
                    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET,  (void *)(&view_info));
                }
            }
            else
            {
                g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
                g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
                g_AppPmt.start(&g_AppPmt);
            }
        }
        GUI_SetInterface("flush", NULL);
        if(GUI_CheckDialog("wnd_pop_tip") != GXCORE_SUCCESS)
        {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_USB_OUT;
            pop.mode = POP_MODE_UNBLOCK;
            if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
            {
                pop.exit_cb = plug_exit_pop_cb;
            }
            popdlg_create(&pop);
        }
#if 0
        if(GUI_CheckDialog("wnd_epg") == GXCORE_SUCCESS)/*add by hu yaobo,2015-7-27*/
        {
            GUI_SetProperty("listview_epg_prog", "update_all", NULL);
            GUI_SetProperty("listview_epg_prog", "select", &sel);
            GUI_SetProperty("listview_epg_prog", "active", &sel);
            if(GUI_CheckDialog("wnd_pop_tip") == GXCORE_SUCCESS)
            {
                GUI_SetProperty("wnd_pop_tip", "update", NULL);
            }
        }
#endif
    }

    return EVENT_TRANSFER_STOP;
}

static int top_hotplug_clean(GxMessage* msg)
{
    if(GUI_CheckDialog("wnd_setting_dm") == GXCORE_SUCCESS)// dm setting
    {
        app_dm_list_hotplug_proc(msg);
    }

    return EVENT_TRANSFER_STOP;
}

#ifdef HDMI_HOTPLUG_SUPPORT
static int top_hdmiplug_staus(GxMessage* msg)
{
	static int init_value = 1;
	GxMsgProperty_PlayerVideoWindow play_wnd;
	PlayerWindow video_wnd = {0};
	//GxBus_ConfigGetInt(OUTPUT_KEY, &init_value, OUTPUT_DEFAULT);
	
	if((init_value)
		&&(GUI_CheckDialog("wnd_epg") != GXCORE_SUCCESS)
		&&(GUI_CheckDialog("wnd_main_menu") != GXCORE_SUCCESS)
		&&(GUI_CheckDialog("wnd_channel_edit") != GXCORE_SUCCESS))
	{
		if(msg->msg_id == GXMSG_HDMI_HOTPLUG_IN)																								   
		{
			video_wnd.x = 0;
			video_wnd.y = 9;
			video_wnd.width = 1280*APP_XRES/1280;
			video_wnd.height = 702*APP_YRES/720;
		}
		else//CVBS
		{
			video_wnd.x = 0;
			video_wnd.y = 9;
			video_wnd.width = 1248*APP_XRES/1280;
			video_wnd.height = 702*APP_YRES/720;
		}


		play_wnd.player = PLAYER_FOR_IPTV;
		memcpy(&(play_wnd.rect), &video_wnd, sizeof(PlayerWindow));
		app_send_msg_exec(GXMSG_PLAYER_VIDEO_WINDOW, (void *)(&play_wnd));
		//init_value = 0;
	}
	return EVENT_TRANSFER_STOP;
}

#endif
static int top_sync_time(GxMessage* msg)
{
    GxMsgProperty_ExtraTimeOk *timeData = NULL;

    timeData = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_ExtraTimeOk);

    // TODO TOT 
    if(NULL != timeData
            && TDT_TIME == timeData->time_fountain)
    {
        const char *wnd = GUI_GetFocusWindow();
        
        g_AppTime.sync(&g_AppTime, timeData->utc_second);
        g_AppBook.invalid_remove();
        g_AppBook.enable();
        if((wnd != NULL) && (strcmp(wnd, "wnd_timer_setting") == 0))
        {
        	GUI_SetProperty("list_timer_setting_timer", "update_all", NULL);
        }
    }

    return EVENT_TRANSFER_STOP;
}

static int top_frontend_check(GxMessage* msg)
{
    int led_lock_state = 0;
    AppFrontend_LockState lock;

    if(s_simple_lowpower == true)
        return 0;

    if (msg->msg_id == GXMSG_FRONTEND_UNLOCKED)
    {
        GxMsgProperty_FrontendUnlocked *tuner = NULL;
        const char *wnd = GUI_GetFocusWindow();

        tuner = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_FrontendUnlocked);

        //when unlocked, app need set DiSEqC1.0 & 1.1. maybe the state is changed
        if (wnd != NULL)
        {
            if (//(0 == strcmp(wnd,"wnd_full_screen"))
					(GXCORE_SUCCESS == GUI_CheckDialog("wnd_full_screen"))
                    || (0 == strcmp(wnd,"wnd_antenna_setting"))
                    || (0 == strcmp(wnd,"wnd_tp_list"))
                    || (0 == strcmp(wnd,"wnd_satellite_list"))
                    || (0 == strcmp(wnd,"wnd_positioner_setting"))
                    || (0 == strcmp(wnd,"wnd_channel_edit")))
            {
                //g_AppNim.diseqc_set(&g_AppNim);
                AppFrontend_DiseqcParameters diseqc;
                diseqc.type = DiSEQC10;
				// TODO: NEED TO MODIFY
                app_ioctl(*tuner, FRONTEND_DISEQC_SET_DEFAULT, &diseqc);
                diseqc.type = DiSEQC11;
                app_ioctl(*tuner, FRONTEND_DISEQC_SET_DEFAULT, &diseqc);

				//diseqc.type = DiSEQC12;
                //app_ioctl(*tuner, FRONTEND_DISEQC_SET_DEFAULT, &diseqc);
            }
        }
        //g_AppNim.lock_set(&g_AppNim, false);

        lock = FRONTEND_UNLOCK;
        app_ioctl(*tuner, FRONTEND_LOCK_STATE_SET, &lock);

        led_lock_state = 0;
        ioctl(bsp_panel_handle, PANEL_SHOW_LOCK, &led_lock_state);
    }
    else
    {
        //g_AppNim.lock_set(&g_AppNim, true);
        GxMsgProperty_FrontendLocked *tuner = NULL;

        tuner = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_FrontendLocked);
{
		AppFrontend_DiseqcParameters diseqc;
        diseqc.type = DiSEQC10;
		// TODO: NEED TO MODIFY
        app_ioctl(*tuner, FRONTEND_DISEQC_SET_DEFAULT, &diseqc);
        diseqc.type = DiSEQC11;
        app_ioctl(*tuner, FRONTEND_DISEQC_SET_DEFAULT, &diseqc);
		//printf("_[%s]___set diseqc____\n",__FUNCTION__);
}
        lock = FRONTEND_LOCKED;
        app_ioctl(*tuner, FRONTEND_LOCK_STATE_SET, &lock);

        led_lock_state = 1;
        ioctl(bsp_panel_handle, PANEL_SHOW_LOCK, &led_lock_state);
    }

    return EVENT_TRANSFER_STOP;
}

static void _dev_prompt_tip_check(void)
{
    if(GUI_CheckPrompt("prompt_usb_in_pop") == GXCORE_SUCCESS)
    {
		GUI_EndPrompt("prompt_usb_in_pop");
    }

    if(GUI_CheckPrompt("prompt_usb_out_pop") == GXCORE_SUCCESS)
    {
		GUI_EndPrompt("prompt_usb_out_pop");
    }

    if(GUI_CheckPrompt("prompt_usb_3g_in_pop") == GXCORE_SUCCESS)
    {
		GUI_EndPrompt("prompt_usb_3g_in_pop");
    }

    if(GUI_CheckPrompt("prompt_usb_3g_out_pop") == GXCORE_SUCCESS)
    {
		GUI_EndPrompt("prompt_usb_3g_out_pop");
    }

    if(GUI_CheckPrompt("prompt_usb_wifi_in_pop") == GXCORE_SUCCESS)
    {
		GUI_EndPrompt("prompt_usb_wifi_in_pop");
    }
    
    if(GUI_CheckPrompt("prompt_usb_wifi_out_pop") == GXCORE_SUCCESS)
    {
		GUI_EndPrompt("prompt_usb_wifi_out_pop");
    }
    return;
}

static int _usb_info_timer_cb(void *usrdata)
{
	UsbPlugStatus state = *(UsbPlugStatus*)usrdata;
	remove_timer(s_usb_info_timer);
	s_usb_info_timer = NULL;

	if((state == USB_PLUG_IN)
		&& (GUI_CheckPrompt("prompt_usb_in_pop") == GXCORE_SUCCESS))
	{
		GUI_EndPrompt("prompt_usb_in_pop");
	}
	else if((state == USB_PLUG_OUT)
			&& (GUI_CheckPrompt("prompt_usb_out_pop") == GXCORE_SUCCESS))
	{
		GUI_EndPrompt("prompt_usb_out_pop");
	}
	else
		;
	return 0;
}

void top_usb_plug_info(UsbPlugStatus state)
{
	static UsbPlugStatus cur_state;
	cur_state = state;

	remove_timer(s_usb_info_timer);

    _dev_prompt_tip_check();
	GUI_SetInterface("flush",NULL);

	if(cur_state == USB_PLUG_IN)
	{
		if(GUI_CheckPrompt("prompt_usb_out_pop") == GXCORE_SUCCESS)
		{
			GUI_EndPrompt("prompt_usb_out_pop");
		}

		if(GUI_CheckPrompt("prompt_usb_in_pop") != GXCORE_SUCCESS)
		{
            if(g_AppFullArb.state.ttx == STATE_OFF)
            {
#if MINI_16_BITS_OSD_SUPPORT
                GUI_CreatePrompt(960, 450, "prompt_usb_in_pop", "usb_in_prompt", "", "ontop");
#else
                GUI_CreatePrompt(1185, 600, "prompt_usb_in_pop", "usb_in_prompt", "", "ontop");
#endif
            }
 #if UPGRADE_AUTO_SELECT_SUPPORT
                  if(GUI_CheckDialog("wnd_system_setting") == GXCORE_SUCCESS)
                {
                    extern char * app_system_get_title();
                    extern void app_upgrade_clear_path();
                    if(app_system_get_title() != NULL && 0 == strcmp(app_system_get_title(), STR_ID_FIRMWARE_UPGRADE))
                    {
                        extern  void app_upgrade_file_auto_select(void);

                        app_upgrade_file_auto_select();
                    }
		}
#endif
		}
        
    }
	else if(cur_state == USB_PLUG_OUT)
	{
		if(GUI_CheckPrompt("prompt_usb_in_pop") == GXCORE_SUCCESS)
		{
			GUI_EndPrompt("prompt_usb_in_pop");
		}

		if(GUI_CheckPrompt("prompt_usb_out_pop") != GXCORE_SUCCESS)
		{
			if(s_message_usbout_flag == true)
			{
				s_message_usbout_flag = false;
                if(g_AppFullArb.state.ttx == STATE_OFF)
                {
#if MINI_16_BITS_OSD_SUPPORT
                    GUI_CreatePrompt(960, 450, "prompt_usb_out_pop", "", "", "ontop");
#else
                    GUI_CreatePrompt(1185, 600, "prompt_usb_out_pop", "", "", "ontop");
#endif
                }
			}
			else
			{
                if(g_AppFullArb.state.ttx == STATE_OFF)
                {
#if MINI_16_BITS_OSD_SUPPORT
                    GUI_CreatePrompt(960, 450, "prompt_usb_out_pop", "usb_out_prompt", "", "ontop");
#else
                    GUI_CreatePrompt(1185, 600, "prompt_usb_out_pop", "usb_out_prompt", "", "ontop");
#endif
                }
			}
            if(GUI_CheckDialog("wnd_system_setting") == GXCORE_SUCCESS)
            {
                extern char * app_system_get_title();
                extern void app_upgrade_clear_path();
                if(app_system_get_title() != NULL && 0 == strcmp(app_system_get_title(), STR_ID_FIRMWARE_UPGRADE))
                {
                    if(0 == strcasecmp("wnd_upgrade_process", GUI_GetFocusWindow()) )
                    {
                        GUI_EndDialog("wnd_upgrade_process");
                    }

                    app_upgrade_clear_path();	
                }
            }
		}
	}
	else
		;
	if(0 != reset_timer(s_usb_info_timer))
	{
		s_usb_info_timer = create_timer(_usb_info_timer_cb, 2000, (void*)&cur_state, TIMER_ONCE);
    }

	if(GUI_CheckDialog("wnd_zoom") == GXCORE_SUCCESS)
	{
		zoom_refresh();
	}
}

#if NETWORK_SUPPORT && MOD_3G_SUPPORT
static int _usb_3g_info_timer_cb(void *usrdata)
{
	UsbPlugStatus state = *(UsbPlugStatus*)usrdata;
	remove_timer(s_usb_3g_info_timer);
	s_usb_3g_info_timer = NULL;

	if((state == USB_3G_PLUG_IN)
		&& (GUI_CheckPrompt("prompt_usb_3g_in_pop") == GXCORE_SUCCESS))
	{
		GUI_EndPrompt("prompt_usb_3g_in_pop");
	}
	else if((state == USB_3G_PLUG_OUT)
			&& (GUI_CheckPrompt("prompt_usb_3g_out_pop") == GXCORE_SUCCESS))
	{
		GUI_EndPrompt("prompt_usb_3g_out_pop");
	}
	else
		;
	if(GUI_CheckDialog("wnd_zoom") == GXCORE_SUCCESS)
	{
		zoom_refresh();
	}

	return 0;
}

static void top_usb_3g_plug_info(UsbPlugStatus state)
{
	static UsbPlugStatus state_3g;
	state_3g = state;

	remove_timer(s_usb_3g_info_timer);
    _dev_prompt_tip_check();

	if(state_3g == USB_3G_PLUG_IN)
	{
		if(GUI_CheckPrompt("prompt_usb_3g_out_pop") == GXCORE_SUCCESS)
		{
			GUI_EndPrompt("prompt_usb_3g_out_pop");
		}

		if(GUI_CheckPrompt("prompt_usb_3g_in_pop") != GXCORE_SUCCESS)
		{
            if(g_AppFullArb.state.ttx == STATE_OFF)
            {
#if MINI_16_BITS_OSD_SUPPORT
                GUI_CreatePrompt(960, 450, "prompt_usb_3g_in_pop", "usb_3g_in_prompt", "", "ontop");
#else
                GUI_CreatePrompt(1210, 600, "prompt_usb_3g_in_pop", "usb_3g_in_prompt", "", "ontop");
#endif
            }
		}
	}
	else if(state_3g == USB_3G_PLUG_OUT)
	{
		if(GUI_CheckPrompt("prompt_usb_3g_in_pop") == GXCORE_SUCCESS)
		{
			GUI_EndPrompt("prompt_usb_3g_in_pop");
		}

		if(GUI_CheckPrompt("prompt_usb_3g_out_pop") != GXCORE_SUCCESS)
		{
            if(g_AppFullArb.state.ttx == STATE_OFF)
            {
#if MINI_16_BITS_OSD_SUPPORT
                GUI_CreatePrompt(960, 450, "prompt_usb_3g_out_pop", "usb_3g_out_prompt", "", "ontop");
#else
                GUI_CreatePrompt(1210, 600, "prompt_usb_3g_out_pop", "usb_3g_out_prompt", "", "ontop");
#endif
            }
		}
	}
	else
		;

	if(0 != reset_timer(s_usb_3g_info_timer)) 
	{
		s_usb_3g_info_timer = create_timer(_usb_3g_info_timer_cb, 2000, (void*)&state_3g, TIMER_ONCE);
	}	
	
	if(GUI_CheckDialog("wnd_zoom") == GXCORE_SUCCESS)
	{
		zoom_refresh();
	}

    return;
}
#endif

#if NETWORK_SUPPORT && WIFI_SUPPORT
static int _usb_wifi_info_timer_cb(void *usrdata)
{
	UsbPlugStatus state = *(UsbPlugStatus*)usrdata;
	remove_timer(s_usb_wifi_info_timer);
	s_usb_wifi_info_timer = NULL;

	if((state == USB_WIFI_PLUG_IN)
		&& (GUI_CheckPrompt("prompt_usb_wifi_in_pop") == GXCORE_SUCCESS))
	{
		GUI_EndPrompt("prompt_usb_wifi_in_pop");
	}
	else if((state == USB_WIFI_PLUG_OUT)
			&& (GUI_CheckPrompt("prompt_usb_wifi_out_pop") == GXCORE_SUCCESS))
	{
		GUI_EndPrompt("prompt_usb_wifi_out_pop");
	}
	else
		;
	if(GUI_CheckDialog("wnd_zoom") == GXCORE_SUCCESS)
	{
		zoom_refresh();
	}

	return 0;
}

void top_usb_wifi_plug_info(UsbPlugStatus state)
{
	static UsbPlugStatus wifi_state;
	wifi_state = state;

	remove_timer(s_usb_wifi_info_timer);
    _dev_prompt_tip_check();

	if(wifi_state == USB_WIFI_PLUG_IN)
	{
		if(GUI_CheckPrompt("prompt_usb_wifi_out_pop") == GXCORE_SUCCESS)
		{
			GUI_EndPrompt("prompt_usb_wifi_out_pop");
		}

		if(GUI_CheckPrompt("prompt_usb_wifi_in_pop") != GXCORE_SUCCESS)
		{
            if(g_AppFullArb.state.ttx == STATE_OFF)
            {
#if MINI_16_BITS_OSD_SUPPORT
                GUI_CreatePrompt(960, 450, "prompt_usb_wifi_in_pop", "usb_wifi_in_prompt", "", "ontop");
#else
                GUI_CreatePrompt(1210, 600, "prompt_usb_wifi_in_pop", "usb_wifi_in_prompt", "", "ontop");
#endif
            }
        }
    }
	else if(wifi_state == USB_WIFI_PLUG_OUT)
	{
		if(GUI_CheckPrompt("prompt_usb_wifi_in_pop") == GXCORE_SUCCESS)
		{
			GUI_EndPrompt("prompt_usb_wifi_in_pop");
		}

		if(GUI_CheckPrompt("prompt_usb_wifi_out_pop") != GXCORE_SUCCESS)
        {
            if(g_AppFullArb.state.ttx == STATE_OFF)
            {
#if MINI_16_BITS_OSD_SUPPORT
                GUI_CreatePrompt(960, 450, "prompt_usb_wifi_out_pop", "usb_wifi_out_prompt", "", "ontop");
#else
                GUI_CreatePrompt(1210, 600, "prompt_usb_wifi_out_pop", "usb_wifi_out_prompt", "", "ontop");
#endif
            }
        }
		
#if IPTV_SUPPORT
		app_iptv_recovery_status();
#endif
	}
	else
		;

	if(0 != reset_timer(s_usb_wifi_info_timer)) 
	{
		s_usb_wifi_info_timer = create_timer(_usb_wifi_info_timer_cb, 2000, (void*)&wifi_state, TIMER_ONCE);
	}	
	
	if(GUI_CheckDialog("wnd_zoom") == GXCORE_SUCCESS)
	{
		zoom_refresh();
	}
}
#endif

extern  void app_pvr_stop(void);
static int app_top_service(GUI_Event* event)
{
    int ret = EVENT_TRANSFER_KEEPON;
    APP_CHECK_P(event, EVENT_TRANSFER_STOP);
    GxMessage* msg;

    msg = (GxMessage*)event->msg.service_msg;
    if (msg == NULL)    return ret;
	
    switch(msg->msg_id)
    {
		case GXMSG_EPG_FULL:
			app_epg_info_clean();
			break;
        #if TKGS_SUPPORT
        case GXMSG_TKGS_UPDATE_STATUS:
            {
                extern int app_tkgs_upgrade_process_service(GxMessage* pMsg);
                app_tkgs_upgrade_process_service(msg);
            }
            break;
        #endif
        case GXMSG_HOTPLUG_IN:
            top_usb_plug_info(USB_PLUG_IN);
            ret = top_hotplug_in(msg);
			app_set_pvr_usbin_flag(true);
            break;
        case GXMSG_HOTPLUG_OUT:
            ret = top_hotplug_out(msg);
            top_usb_plug_info(USB_PLUG_OUT);
			break;

#if NETWORK_SUPPORT && WIFI_SUPPORT
        case GXMSG_USBWIFI_HOTPLUG_IN:
            top_usb_wifi_plug_info(USB_WIFI_PLUG_IN);
            printf("\n\033[35mUSB WIFI Hotplug in!!\n\033[0m");
            break;

        case GXMSG_USBWIFI_HOTPLUG_OUT:
            top_usb_wifi_plug_info(USB_WIFI_PLUG_OUT);
            printf("\n\033[35mUSB WIFI Hotplug out!!\n\033[0m");
            break;
#endif

#if MOD_3G_SUPPORT
        case GXMSG_USB3G_HOTPLUG_IN:
            top_usb_3g_plug_info(USB_3G_PLUG_IN);
            break;

        case GXMSG_USB3G_HOTPLUG_OUT:
            top_usb_3g_plug_info(USB_3G_PLUG_OUT);
            break;
#endif

		#ifdef HDMI_HOTPLUG_SUPPORT
		case GXMSG_HDMI_HOTPLUG_IN:
		case GXMSG_HDMI_HOTPLUG_OUT:
			top_hdmiplug_staus(msg);
			break;
		#endif
        case GXMSG_HOTPLUG_CLEAN:
            ret = top_hotplug_clean(msg);
            break;
        case GXMSG_PLAYER_AVCODEC_REPORT:
#ifdef NETAPPS_SUPPORT
#if IPTV_SUPPORT
            if(GUI_CheckDialog("wnd_iptv_play") == GXCORE_SUCCESS)
                ret = app_iptv_play_msg_proc(msg);
            else
#endif
#if YOUTUBE_SUPPORT
            if(GUI_CheckDialog("wnd_apps_netvideo_play") == GXCORE_SUCCESS)
                ret = app_net_video_avcodec_status(msg);
            else
#endif
#endif
            ret = top_avcodec_report(msg);
            break;
        case GXMSG_PLAYER_STATUS_REPORT:
#if NETAPPS_SUPPORT
#if IPTV_SUPPORT
            if(GUI_CheckDialog("wnd_iptv_play") == GXCORE_SUCCESS)
                ret = app_iptv_play_msg_proc(msg);
            else
#endif
#if YOUTUBE_SUPPORT
            if(GUI_CheckDialog("wnd_apps_netvideo_play") == GXCORE_SUCCESS)
                ret = app_net_video_play_status(msg);
            else
#endif
#endif
            ret = top_play_status_report(msg);
            break;
        case GXMSG_PLAYER_SPEED_REPORT:
            ret = top_play_speed_report(msg);
            break;
        case GXMSG_PLAYER_RESOLUTION_REPORT:
            ret =  top_play_resolution_report(msg);
            break;
        case GXMSG_EXTRA_SYNC_TIME_OK:
            ret = top_sync_time(msg);
            break;
        case GXMSG_FRONTEND_UNLOCKED:
        case GXMSG_FRONTEND_LOCKED:
            ret = top_frontend_check(msg);
            break;

        case GXMSG_BOOK_TRIGGER:
            {
				if(0 == strcasecmp("wnd_file_list", GUI_GetFocusWindow()) )
				{
					static GUI_Event event;
					event.type = GUI_KEYDOWN;
					event.key.sym = STBK_EXIT;
					GUI_SendEvent("listview_file_list", &event);
				}
				if(0 == strcasecmp("wnd_pop_option_list", GUI_GetFocusWindow()) )
				{
					static GUI_Event event;
					event.type = GUI_KEYDOWN;
					event.key.sym = STBK_EXIT;
					GUI_SendEvent("wnd_pop_option_list", &event);
				}
				if(0 == strcasecmp("wnd_pop_tip", GUI_GetFocusWindow()) )
				{
					popdlg_destroy();
				}
				if(0 == strcasecmp("wnd_sat_search_opt", GUI_GetFocusWindow()))
				{
					static GUI_Event event;
					event.type = GUI_KEYDOWN;
					event.key.sym = STBK_EXIT;
					GUI_SendEvent("box_sat_search_opt", &event);
				}
				if(0 == strcasecmp("wnd_tp_search_opt", GUI_GetFocusWindow()))
				{
					static GUI_Event event;
					event.type = GUI_KEYDOWN;
					event.key.sym = STBK_EXIT;
					GUI_SendEvent("box_tp_search_opt", &event);
				}
				if(0 == strcasecmp("wnd_timer_channel_list", GUI_GetFocusWindow()))
				{
					static GUI_Event event;
					event.type = GUI_KEYDOWN;
					event.key.sym = STBK_EXIT;
					GUI_SendEvent("listview_timer_channel_list", &event);
				}
                GxMsgProperty_BookTrigger *book = NULL;
                book = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_BookTrigger);
                if((NULL != book) && (s_simple_lowpower == false))
                {
					g_AppBook.exec(book);
				}
                ret = EVENT_TRANSFER_STOP;
                break;
            }

//#ifdef LINUX_OS
#if NETWORK_SUPPORT
        case GXMSG_NETWORK_STATE_REPORT:
            {
                const char *wnd = GUI_GetFocusWindow();
                if((wnd != NULL) && (strcmp(wnd, "wnd_network") == 0))
                {
                    app_network_msg_proc(msg);
                }

#if WIFI_SUPPORT
                if(GUI_CheckDialog("wnd_wifi") == GXCORE_SUCCESS)
                {
                    app_wifi_msg_proc(msg);
                }
#endif
#if MOD_3G_SUPPORT
                if(GUI_CheckDialog("wnd_3g") == GXCORE_SUCCESS)
                {
                    app_3g_msg_proc(msg);
                }
#endif
#if IPTV_SUPPORT
                if(GUI_CheckDialog("wnd_iptv") == GXCORE_SUCCESS)
                {
                    app_iptv_browser_msg_proc(msg);
                }
#endif
#if DLNA_SUPPORT
                app_dlna_msg_proc(msg);
#endif
                break;
            }

        case GXMSG_WIFI_SCAN_AP_OVER:
            {
#if WIFI_SUPPORT
                if(GUI_CheckDialog("wnd_wifi") == GXCORE_SUCCESS)
                {
                    app_wifi_msg_proc(msg);
                }
#endif
                break;
            }

        case GXMSG_NETWORK_PLUG_IN:
            {
                const char *wnd = GUI_GetFocusWindow();
                if((wnd != NULL) && (strcmp(wnd, "wnd_network") == 0))
                {
                    app_network_msg_proc(msg);
                }
#if WIFI_SUPPORT
                if(GUI_CheckDialog("wnd_wifi") == GXCORE_SUCCESS)
                {
                    app_wifi_msg_proc(msg);
                }
#endif
#if MOD_3G_SUPPORT
                if((wnd != NULL) && (strcmp(wnd, "wnd_3g") == 0))
                {
                    app_3g_msg_proc(msg);
                }
#endif
                break;
            }

        case GXMSG_NETWORK_PLUG_OUT:
            {
                const char *wnd = GUI_GetFocusWindow();
                if((wnd != NULL) && (strcmp(wnd, "wnd_network") == 0))
                {
                    app_network_msg_proc(msg);
                }

#if WIFI_SUPPORT
                if(GUI_CheckDialog("wnd_wifi") == GXCORE_SUCCESS)
                {
                    app_wifi_msg_proc(msg);
                }
#endif
#if MOD_3G_SUPPORT
                if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_3g"))
                {
                    app_3g_msg_proc(msg);
                }
#endif
                break;
            }

#if BOX_SERVICE
        case GXMSG_BOX_SET_DISEQC:
            {
                GxMsgProperty_BoxSetDiseqc *id = NULL;
                id = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_BoxSetDiseqc);
                app_box_set_diseqc(id->channel_id);

                break;
            }

#if DLNA_SUPPORT
        case GXMSG_DLNA_TRIGGER:
            app_dlna_msg_proc(msg);
            break;
#endif
#endif
#if IPTV_SUPPORT
        case APPMSG_IPTV_LIST_DONE:
            app_iptv_browser_msg_proc(msg);
            break;

        case APPMSG_IPTV_AD_PIC_DONE:
            if(GUI_CheckDialog("wnd_iptv_play") == GXCORE_SUCCESS)
                ret = app_iptv_play_msg_proc(msg);
            else
                ret = app_iptv_browser_msg_proc(msg);
            break;
#endif
#endif
#if CASCAM_SUPPORT
        case GXMSG_CASCAM_EVENT:
            printf("\nCAS, %s, %d\n", __FUNCTION__,__LINE__);
            CASCAMOSDEventClass *para = NULL;
            para = GxBus_GetMsgPropertyPtr(msg, CASCAMOSDEventClass);
            CASCAM_Do_Event(para);
            break;
#endif
#if OTA_MONITOR_SUPPORT
        case GXMSG_OTA_EVENT:
{
            OTAUpdateClass *param = NULL;
            param = GxBus_GetMsgPropertyPtr(msg, OTAUpdateClass);
            extern int app_ota_ui_msg_recv(void *param);
            app_ota_ui_msg_recv((void*)param);
}
            break;
#endif
#if LNB_SHORTCUT_SUPPORT
        case GXMSG_LNB_SHORTCUT_EVENT:
            {
                extern void app_lnb_shortcut_pop_exec(void);
                app_lnb_shortcut_pop_exec();
            }
            break;
#endif
        default:
            break;
    }
    return ret;
}

static int top_stbk_halt(uint32_t scan_code)
{
	time_t timerDur;
	time_t time_now;

	time_now = g_AppTime.utc_get(&g_AppTime);
	timerDur = g_AppBook.get_sleep_time(time_now);
	app_low_power(timerDur, scan_code);
	return EVENT_TRANSFER_STOP;
}

static int app_top_keypress(GUI_Event* event)
{
    int ret = EVENT_TRANSFER_STOP;

    APP_CHECK_P(event, EVENT_TRANSFER_STOP);

    if((s_simple_lowpower == true) && (event->key.sym != STBK_HALT))
    {
        event->key.sym = SDLK_LAST;
    }

#if CASCAM_SUPPORT
    extern int cmm_cascam_check_lock_service_status(void);
    if(cmm_cascam_check_lock_service_status())
    {// lock the service
        event->key.sym = SDLK_LAST;
    }
#endif

    switch(event->key.sym)
    {
        case STBK_HALT:
       {
	   	#if TKGS_SUPPORT
		 if (g_AppPvrOps.state != PVR_DUMMY)
                        {
                            PopDlg pop;
                            pvr_state cur_pvr = PVR_DUMMY;

                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_YES_NO;

                            if(g_AppPvrOps.state == PVR_TMS_AND_REC)
                            {
                               #if CA_SUPPORT
                                if(app_extend_control_pvr_change())
                                {
                                    pop.str = STR_ID_STOP_REC_INFO;
                                    cur_pvr = PVR_RECORD;
                                }
                                else
                                 #endif					 
                                {
                                    pop.str = STR_ID_STOP_TMS_INFO;
                                    cur_pvr = PVR_TIMESHIFT;
                                }
                            }
                            else if(g_AppPvrOps.state == PVR_RECORD)
                            {
                              #if CA_SUPPORT
                                if(app_extend_control_pvr_change())
                                {
                                    pop.str = STR_ID_STOP_REC_INFO;
                                    cur_pvr = PVR_RECORD;
                                }
                                 else
                                 #endif
                                {
                                    pop.str = STR_ID_STOP_REC_INFO;
                                    cur_pvr = PVR_RECORD;
                                }
                            }
                            else
                            {
                                pop.str = STR_ID_STOP_TMS_INFO;
                                cur_pvr = PVR_TIMESHIFT;
                            }

                            if(cur_pvr != PVR_DUMMY)
                            {                    
                                if (POP_VAL_OK != popdlg_create(&pop))
                                {
                                    break;
                                }

                                if(cur_pvr == PVR_TIMESHIFT)
                                {
                                    app_pvr_tms_stop();
                                }
                                else
                                {
                                    app_pvr_stop();
                                }
                            }
                        }
		 #endif
            ret = top_stbk_halt(event->key.scancode);
        	}
			break;
#if 0
        case STBK_PN:
        {
            app_video_standard_hotkey();
            break;
        }
#endif

        default:
            return EVENT_TRANSFER_KEEPON;
    }

#if THREE_HOURS_AUTO_STANDBY
	void timer_auto_standby_reset(void);
	timer_auto_standby_reset();
#endif
    return ret;
}

bool app_check_av_running(void)
{
    bool ret = false;
    AppFrontend_LockState lock;

    app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);
    if((lock == FRONTEND_LOCKED)
        && (s_avcodec_running == true))
    {
        ret = true;
    }
    else
    {
        if((s_avcodec_running == false) 
            &&((g_AppPvrOps.state == PVR_TIMESHIFT)
                    || (g_AppPvrOps.state == PVR_TMS_AND_REC)))
        {
            ret = true;
        }
        else
        {
            ret = false;
        }
    }
    return ret;
}

SIGNAL_HANDLER int top(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;
    int ret = EVENT_TRANSFER_KEEPON;

    if(NULL == usrdata) return EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_SERVICE_MSG:
            ret = app_top_service(event);
            break;

        case GUI_KEYDOWN:
			GxCore_GetTickTime(&time_start);
			//printf("==========>Keypress time: %d s, %d us\n", time_start.seconds, time_start.microsecs);
            ret = app_top_keypress(event);

        default:
            return EVENT_TRANSFER_KEEPON;
    }

    return ret;
}
