#include "gxgui_view.h"
#include "gui_core.h"
#include "app.h"
#include "gxmsg.h"
#include "gxoem.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "module/app_nim.h"
#if TKGS_SUPPORT
#include "module/gxtkgs.h"
#endif
#ifdef NETAPPS_SUPPORT
#include <gx_apps.h>
#include "netapps/app_netapps_service.h"
#endif
#include <fcntl.h>
#if CASCAM_SUPPORT
#include "app_cascam_pop.h"
#endif
extern GuiWidgetOps file_image_ops;
extern status_t app_system_gui_init(void);
extern void app_system_init(void);
extern status_t signal_connect_handler(void);
extern void app_system_init(void);
//extern void app_epg_init(void);
extern void GxTtx_Init(void);
extern status_t app_get_panel_keymap(void);
extern uint16_t app_get_panel_keycode(PanelKeymap key_index);
handle_t g_app_msg_self = 0;
static int s_unlisten_flag = 1;
static bool s_msg_register_flag = false;

handle_t bsp_panel_handle;
handle_t bsp_gpio_handle;

static void _serial2_cvbs_mute_config(bool flag) //false: mute off; true: mute on
{
#ifdef LINUX_OS
    static int uart2_fd = -1;

    if((uart2_fd != -1)
            || ((uart2_fd = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY)) > 0))
    {
        if(flag == false)
            ioctl(uart2_fd, TIOCSBRK, NULL);
        else
            ioctl(uart2_fd, TIOCCBRK, NULL);
        ////don't close fd////
    }
#endif
#ifdef ECOS_OS
    #define _CONFIG_SERIAL_HW_BREAK              0x0189
    {
        if(flag)
        {
            // uart2tx low high level
            *(volatile unsigned int*)0xa0401000 |= (1<<3);
            *(volatile unsigned int*)0xa0401000 &= ~(1<<10);
        }
        else
        {
            // output high level
            *(volatile unsigned int*)0xa0401000 |= (1<<3);
            *(volatile unsigned int*)0xa0401000 |= (1<<10);
        }
    }

#endif
// TODO: ECOS
//
}

void app_set_hardware_mute(void)
{
// mute
#if GPIO_CONFIG_SUPPORT
#if 0
	GpioData _gpio;

	_gpio.port = V_GPIO_MUTE;
	ioctl(bsp_gpio_handle, GPIO_OUTPUT, &_gpio);
	ioctl(bsp_gpio_handle, GPIO_LOW, &_gpio);
#endif
// use serial anlog
    _serial2_cvbs_mute_config(true);
#endif
}

void app_set_hardware_unmute(void)
{
// unmute
#if GPIO_CONFIG_SUPPORT
#if 0
	GpioData _gpio;

	_gpio.port = V_GPIO_MUTE;
	ioctl(bsp_gpio_handle, GPIO_OUTPUT, &_gpio);
	ioctl(bsp_gpio_handle, GPIO_HIGH, &_gpio);
#endif
// use serial anlog
    _serial2_cvbs_mute_config(false);
#endif
}

status_t app_block_msg_init(handle_t self)
{
    if(s_unlisten_flag == 1)
    {
        g_app_msg_self = self;

        GxBus_MessageEmptyByOps(&g_service_list[SERVICE_GUI_VIEW]);

        GxBus_MessageListen(self, GXMSG_SEARCH_NEW_PROG_GET);
        GxBus_MessageListen(self, GXMSG_SEARCH_SAT_TP_REPLY);
        GxBus_MessageListen(self, GXMSG_SEARCH_STATUS_REPLY);
        GxBus_MessageListen(self, GXMSG_SEARCH_STOP_OK);
        GxBus_MessageListen(self, GXMSG_EXTRA_SYNC_TIME_OK);
        GxBus_MessageListen(self, GXMSG_PLAYER_STATUS_REPORT);
        GxBus_MessageListen(self, GXMSG_PLAYER_AVCODEC_REPORT);
        GxBus_MessageListen(self, GXMSG_PLAYER_SPEED_REPORT);
        GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_FINISH);
        GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_REPLY);
        //GxBus_MessageListen(self, GXMSG_HOTPLUG_IN);
        //GxBus_MessageListen(self, GXMSG_HOTPLUG_OUT);
        GxBus_MessageListen(self, GXMSG_EPG_FULL);
		//GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
		//GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
		#ifdef HDMI_HOTPLUG_SUPPORT
		GxBus_MessageListen(self, GXMSG_HDMI_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_HDMI_HOTPLUG_OUT);
		#endif
        GxBus_MessageListen(self, GXMSG_HOTPLUG_CLEAN);
        GxBus_MessageListen(self, GXMSG_UPDATE_STATUS);
        //GxBus_MessageListen(self, GXMSG_BOOK_TRIGGER);
        GxBus_MessageListen(self, GXMSG_BOOK_FINISH);
        GxBus_MessageListen(self, GXMSG_FRONTEND_LOCKED);
        GxBus_MessageListen(self, GXMSG_FRONTEND_UNLOCKED);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_INIT);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_DESTROY);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_HIDE);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_SHOW);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_DRAW);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_CLEAR);

//#ifdef LINUX_OS
#if NETWORK_SUPPORT
        GxBus_MessageListen(self, GXMSG_NETWORK_STATE_REPORT);
        //GxBus_MessageListen(self, GXMSG_NETWORK_PLUG_IN);
        //GxBus_MessageListen(self, GXMSG_NETWORK_PLUG_OUT);
        GxBus_MessageListen(self, GXMSG_WIFI_SCAN_AP_OVER);

#if BOX_SERVICE
        GxBus_MessageListen(self, GXMSG_BOX_SET_DISEQC);
#if DLNA_SUPPORT
        GxBus_MessageListen(self, GXMSG_DLNA_TRIGGER);
#endif
#endif

#endif

//#endif
#if TKGS_SUPPORT
        GxBus_MessageListen(self, GXMSG_TKGS_UPDATE_STATUS);
#endif

#if CASCAM_SUPPORT
        GxBus_MessageListen(self, GXMSG_CASCAM_EVENT);
#endif
#if OTA_MONITOR_SUPPORT
        GxBus_MessageListen(self, GXMSG_OTA_EVENT);
#endif
#if LNB_SHORTCUT_SUPPORT
        GxBus_MessageListen(self, GXMSG_LNB_SHORTCUT_EVENT);
#endif

    }
    if(s_unlisten_flag > 0)
    {
        s_unlisten_flag--;
    }
    return GXCORE_SUCCESS;
}

status_t app_block_msg_destroy(handle_t self)
{
    if(s_unlisten_flag == 0)
    {
        GxBus_MessageUnListen(self, GXMSG_SEARCH_NEW_PROG_GET);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_SAT_TP_REPLY);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_STATUS_REPLY);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_STOP_OK);
        GxBus_MessageUnListen(self, GXMSG_EXTRA_SYNC_TIME_OK);
        GxBus_MessageUnListen(self, GXMSG_PLAYER_STATUS_REPORT);
        GxBus_MessageUnListen(self, GXMSG_PLAYER_SPEED_REPORT);
        GxBus_MessageUnListen(self, GXMSG_PLAYER_AVCODEC_REPORT);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_BLIND_SCAN_FINISH);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_BLIND_SCAN_REPLY);
        //GxBus_MessageUnListen(self, GXMSG_HOTPLUG_IN);
        //GxBus_MessageUnListen(self, GXMSG_HOTPLUG_OUT);
		GxBus_MessageUnListen(self, GXMSG_EPG_FULL);
		//GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
        //GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
		#ifdef HDMI_HOTPLUG_SUPPORT
		GxBus_MessageUnListen(self, GXMSG_HDMI_HOTPLUG_IN);
		GxBus_MessageUnListen(self, GXMSG_HDMI_HOTPLUG_OUT);
		#endif
        GxBus_MessageUnListen(self, GXMSG_HOTPLUG_CLEAN);
        GxBus_MessageUnListen(self, GXMSG_UPDATE_STATUS);
       // GxBus_MessageUnListen(self, GXMSG_BOOK_TRIGGER);
        GxBus_MessageUnListen(self, GXMSG_BOOK_FINISH);
        GxBus_MessageUnListen(self, GXMSG_FRONTEND_LOCKED);
        GxBus_MessageUnListen(self, GXMSG_FRONTEND_UNLOCKED);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_INIT);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_DESTROY);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_HIDE);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_SHOW);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_DRAW);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_CLEAR);

//#ifdef LINUX_OS
#if NETWORK_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_NETWORK_STATE_REPORT);
        //GxBus_MessageUnListen(self, GXMSG_NETWORK_PLUG_IN);
        //GxBus_MessageUnListen(self, GXMSG_NETWORK_PLUG_OUT);
        GxBus_MessageUnListen(self, GXMSG_WIFI_SCAN_AP_OVER);

#if BOX_SERVICE
        GxBus_MessageUnListen(self, GXMSG_BOX_SET_DISEQC);
#if DLNA_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_DLNA_TRIGGER);
#endif
#endif

#endif
//#endif
#if TKGS_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_TKGS_UPDATE_STATUS);
#endif

#if CASCAM_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_CASCAM_EVENT);
#endif
#if OTA_MONITOR_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_OTA_EVENT);
#endif
#if LNB_SHORTCUT_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_LNB_SHORTCUT_EVENT);
#endif
        GxBus_MessageEmptyByOps(&g_service_list[SERVICE_GUI_VIEW]);
    }
    s_unlisten_flag++;

    return GXCORE_SUCCESS;
}

status_t app_msg_init(handle_t self)
{
    if(s_msg_register_flag == false)
    {
        GxBus_MessageRegister(GXMSG_SUBTITLE_INIT, sizeof(GxMsgProperty_AppSubtitle));
        GxBus_MessageRegister(GXMSG_SUBTITLE_DESTROY, sizeof(GxMsgProperty_AppSubtitle));
        GxBus_MessageRegister(GXMSG_SUBTITLE_HIDE, sizeof(GxMsgProperty_AppSubtitle));
        GxBus_MessageRegister(GXMSG_SUBTITLE_SHOW, sizeof(GxMsgProperty_AppSubtitle));
        GxBus_MessageRegister(GXMSG_SUBTITLE_DRAW, sizeof(GxMsgProperty_AppSubtitle));
        GxBus_MessageRegister(GXMSG_SUBTITLE_CLEAR, sizeof(GxMsgProperty_AppSubtitle));

#if DLNA_SUPPORT
        GxBus_MessageRegister(GXMSG_DLNA_TRIGGER, sizeof(GxMsgProperty_DlnaMsg));
#endif

#if CASCAM_SUPPORT
        GxBus_MessageRegister(GXMSG_CASCAM_EVENT, sizeof(CASCAMOSDEventClass));
#endif
#if OTA_MONITOR_SUPPORT
        GxBus_MessageRegister(GXMSG_OTA_EVENT, sizeof(OTAUpdateClass));
#endif
#if LNB_SHORTCUT_SUPPORT
        GxBus_MessageRegister(GXMSG_LNB_SHORTCUT_EVENT, sizeof(void));
#endif

        s_msg_register_flag = true;
    }

    if(s_unlisten_flag == 1)
    {
        g_app_msg_self = self;

        GxBus_MessageEmptyByOps(&g_service_list[SERVICE_GUI_VIEW]);

        GxBus_MessageListen(self, GXMSG_SEARCH_NEW_PROG_GET);
        GxBus_MessageListen(self, GXMSG_SEARCH_SAT_TP_REPLY);
        GxBus_MessageListen(self, GXMSG_SEARCH_STATUS_REPLY);
        GxBus_MessageListen(self, GXMSG_SEARCH_STOP_OK);
        GxBus_MessageListen(self, GXMSG_EXTRA_SYNC_TIME_OK);
        GxBus_MessageListen(self, GXMSG_PLAYER_STATUS_REPORT);
        GxBus_MessageListen(self, GXMSG_PLAYER_AVCODEC_REPORT);
        GxBus_MessageListen(self, GXMSG_PLAYER_SPEED_REPORT);
        GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_FINISH);
        GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_REPLY);
        GxBus_MessageListen(self, GXMSG_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_HOTPLUG_OUT);
		GxBus_MessageListen(self, GXMSG_EPG_FULL);
        GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
#if MOD_3G_SUPPORT
        GxBus_MessageListen(self, GXMSG_USB3G_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_USB3G_HOTPLUG_OUT);
#endif
		#ifdef HDMI_HOTPLUG_SUPPORT
		GxBus_MessageListen(self, GXMSG_HDMI_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_HDMI_HOTPLUG_OUT);
		#endif
        GxBus_MessageListen(self, GXMSG_HOTPLUG_CLEAN);
        GxBus_MessageListen(self, GXMSG_UPDATE_STATUS);
        GxBus_MessageListen(self, GXMSG_BOOK_TRIGGER);
        GxBus_MessageListen(self, GXMSG_BOOK_FINISH);
        GxBus_MessageListen(self, GXMSG_FRONTEND_LOCKED);
        GxBus_MessageListen(self, GXMSG_FRONTEND_UNLOCKED);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_INIT);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_DESTROY);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_HIDE);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_SHOW);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_DRAW);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_CLEAR);

//#ifdef LINUX_OS
#if NETWORK_SUPPORT
        GxBus_MessageListen(self, GXMSG_NETWORK_STATE_REPORT);
        GxBus_MessageListen(self, GXMSG_NETWORK_PLUG_IN);
        GxBus_MessageListen(self, GXMSG_NETWORK_PLUG_OUT);
        GxBus_MessageListen(self, GXMSG_WIFI_SCAN_AP_OVER);

#if BOX_SERVICE
        GxBus_MessageListen(self, GXMSG_BOX_SET_DISEQC);
#if DLNA_SUPPORT
        GxBus_MessageListen(self, GXMSG_DLNA_TRIGGER);
#endif
#endif

#endif

//#endif
#if TKGS_SUPPORT
        GxBus_MessageListen(self, GXMSG_TKGS_UPDATE_STATUS);
#endif

#if CASCAM_SUPPORT
        GxBus_MessageListen(self, GXMSG_CASCAM_EVENT);
#endif
#if OTA_MONITOR_SUPPORT
        GxBus_MessageListen(self, GXMSG_OTA_EVENT);
#endif
#if LNB_SHORTCUT_SUPPORT
        GxBus_MessageListen(self, GXMSG_LNB_SHORTCUT_EVENT);
#endif

    }
    if(s_unlisten_flag > 0)
    {
        s_unlisten_flag--;
    }
    return GXCORE_SUCCESS;
}

status_t app_msg_destroy(handle_t self)
{
    if(s_unlisten_flag == 0)
    {
        GxBus_MessageUnListen(self, GXMSG_SEARCH_NEW_PROG_GET);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_SAT_TP_REPLY);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_STATUS_REPLY);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_STOP_OK);
        GxBus_MessageUnListen(self, GXMSG_EXTRA_SYNC_TIME_OK);
        GxBus_MessageUnListen(self, GXMSG_PLAYER_STATUS_REPORT);
        GxBus_MessageUnListen(self, GXMSG_PLAYER_SPEED_REPORT);
        GxBus_MessageUnListen(self, GXMSG_PLAYER_AVCODEC_REPORT);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_BLIND_SCAN_FINISH);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_BLIND_SCAN_REPLY);
        GxBus_MessageUnListen(self, GXMSG_HOTPLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_HOTPLUG_OUT);
		GxBus_MessageUnListen(self, GXMSG_EPG_FULL);
        GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
#if MOD_3G_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_USB3G_HOTPLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_USB3G_HOTPLUG_OUT);
#endif
		#ifdef HDMI_HOTPLUG_SUPPORT
		GxBus_MessageUnListen(self, GXMSG_HDMI_HOTPLUG_IN);
		GxBus_MessageUnListen(self, GXMSG_HDMI_HOTPLUG_OUT);
		#endif
        GxBus_MessageUnListen(self, GXMSG_HOTPLUG_CLEAN);
        GxBus_MessageUnListen(self, GXMSG_UPDATE_STATUS);
        GxBus_MessageUnListen(self, GXMSG_BOOK_TRIGGER);
        GxBus_MessageUnListen(self, GXMSG_BOOK_FINISH);
        GxBus_MessageUnListen(self, GXMSG_FRONTEND_LOCKED);
        GxBus_MessageUnListen(self, GXMSG_FRONTEND_UNLOCKED);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_INIT);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_DESTROY);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_HIDE);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_SHOW);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_DRAW);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_CLEAR);

//#ifdef LINUX_OS
#if NETWORK_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_NETWORK_STATE_REPORT);
        GxBus_MessageUnListen(self, GXMSG_NETWORK_PLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_NETWORK_PLUG_OUT);
        GxBus_MessageUnListen(self, GXMSG_WIFI_SCAN_AP_OVER);

#if BOX_SERVICE
        GxBus_MessageUnListen(self, GXMSG_BOX_SET_DISEQC);
#if DLNA_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_DLNA_TRIGGER);
#endif
#endif

#endif
//#endif
#if TKGS_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_TKGS_UPDATE_STATUS);
#endif

#if CASCAM_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_CASCAM_EVENT);
#endif
#if OTA_MONITOR_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_OTA_EVENT);
#endif
#if LNB_SHORTCUT_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_LNB_SHORTCUT_EVENT);
#endif

        GxBus_MessageEmptyByOps(&g_service_list[SERVICE_GUI_VIEW]);
    }
    s_unlisten_flag++;

    return GXCORE_SUCCESS;
}

// modify the IRR simple code filter gate
static void config_irr_filter_gate(void)
{
	handle_t IrrFd = 0;
	int irr_simple_ignore_num = 3;

	IrrFd = open("/dev/gxirr0", O_RDWR);
	if(IrrFd)
	{
		if(ioctl(IrrFd, 3/*IRR_SIMPLE_IGNORE_CONFIG*/, &irr_simple_ignore_num)<0)
			printf("\n>>>>>config irr err<<<<<<\n");

		close(IrrFd);
	}
	else
		printf("\n>>>>>open irr err<<<<<<\n");

}

status_t app_create_dialog(const char* window_name)
{
    int ret = 0;
#if CASCAM_SUPPORT
    if(NULL == window_name)
    {
        printf("\nError， %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    GuiWidget *widget = NULL;

    widget = gui_get_window(window_name);
    if(NULL != widget)
    {
        if((strcmp("wnd_epg", window_name) == 0)
                || (strcmp("wnd_main_menu", window_name) == 0))
        {
            if(widget->rect.w == 0)
            {
                widget->rect.w = APP_XRES;
            }
            if(widget->rect.h == 0)
            {
                widget->rect.h = APP_YRES;
            }
        }
        cmm_cascam_check_rolling_osd(widget->rect.x, widget->rect.y, widget->rect.w, widget->rect.h);
        cmm_cascam_check_finger(widget->rect.x, widget->rect.y, widget->rect.w, widget->rect.h);
    }
#if 0

   if(cmm_cascam_exist_rolling_osd())
   {
        GuiWidget *widget = NULL;

        widget = gui_get_window(window_name);
        if(NULL != widget)
        {
            cmm_cascam_get_rolling_osd_rect(&x, &y, &w, &h);
            if(((x + w) < widget->rect.x)
                || ((y + h) < widget->rect.y)
                || (y > (widget->rect.y + widget->rect.h))
                || (x > (widget->rect.x + widget->rect.w)))
            {
                ;
            }
            else
            {
                cmm_cascam_exit_dialog3();
            }
        }

   }
#endif
    ret = GUI_CreateDialog(window_name);
    cmm_cascam_resume_finger();
#else
    ret = GUI_CreateDialog(window_name);
#endif
    return ret;
}

#if 0
// BIG5 MAP GB
#define BIG5_TO_GB_SOURCE  "/dvb/theme/table_BIG2GB.txt"
#ifdef ECOS_OS
#define BIG5_TO_GB_DEST    "/mnt/table_BIG2GB.txt"
#else
#define BIG5_TO_GB_DEST    "/tmp/table_BIG2GB.txt"
#endif

static status_t _copy_file2ramfs(char *source_path, char *dest_path)
{
    handle_t src = 0;
    handle_t dest = 0;
    GxFileInfo file_info;
    status_t ret = GXCORE_ERROR;
    char *buf = NULL;
    int i = 0;

    if((NULL == source_path) || (NULL == dest_path))
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return GXCORE_ERROR;
    }

    if(GxCore_FileExists((const char*)source_path) == GXCORE_FILE_UNEXIST)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return GXCORE_ERROR;
    }
    if((buf = (char*)GxCore_Calloc(SIZE_UNIT_K, 1)) == NULL)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return GXCORE_ERROR;
    }
    if((src = GxCore_Open((const char*)source_path, "r")) < 0)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        GxCore_Free(buf);
        return GXCORE_ERROR;
    }

    if((dest = GxCore_Open((const char*)dest_path, "w+")) < 0)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        GxCore_Free(buf);
        GxCore_Close(src);
        return GXCORE_ERROR;
    }

    memset(&file_info, 0, sizeof(GxFileInfo));
    GxCore_GetFileInfo((const char*)source_path, &file_info);

    for(i = 0; i < file_info.size_by_bytes; i += SIZE_UNIT_K)
    {
        memset(buf, 0, SIZE_UNIT_K);
        ret = GxCore_Read(src, (void*)buf, 1, SIZE_UNIT_K);
        if(ret <= 0)
            break;
        ret = GxCore_Write(dest, (void*)buf, 1, ret);
        if(ret <= 0)
            break;
    }

    GxCore_Free(buf);
    GxCore_Close(src);
    GxCore_Close(dest);

    if(ret > 0)
        return GXCORE_SUCCESS;
    else
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return GXCORE_ERROR;
    }
}

static int _copy_file_to_ram(void)
{
    printf("\nTEST, %s, %d\n", __FUNCTION__,__LINE__);
    return _copy_file2ramfs(BIG5_TO_GB_SOURCE,  BIG5_TO_GB_DEST);
    printf("\nTEST, %s, %d\n", __FUNCTION__,__LINE__);
}
#endif

status_t app_init(void)
{
	status_t ret = GXCORE_SUCCESS;
    uint32_t i = 0;
    uint32_t panel_keymap[PANEL_KEYMAP_TOTAL] = {0};
    uint16_t keycode = 0;

#if (BOOT_LOGO_SUPPORT == 0)
    GxMsgProperty_PlayerStop player_stop = {0};
#endif

    extern int print_enable;
    print_enable = 1;

    bsp_gpio_handle = GxCore_Open(GPIO_NAME, "r");
	if (bsp_gpio_handle <= 0)
	{
	    printf("[gpio] open failed!\n");
	}

	app_get_panel_keymap();
	bsp_panel_handle = GxCore_Open(PANEL_NAME, "r");
	if (bsp_panel_handle <= 0)
	{
	    printf("[panel] open failed!\n");
	}

    for(i = 0; i < PANEL_KEYMAP_TOTAL; i++)
    {
        keycode = app_get_panel_keycode(i);
        panel_keymap[i] = app_key_full_code(keycode);
    }
    ioctl(bsp_panel_handle, PANEL_SET_KEYMAP, panel_keymap);
	/*signal connect*/
	ret = signal_connect_handler();
	if(GXCORE_SUCCESS != ret)
	{
		printf("[signal_connect_handler] ERROR\n");
	}

    //gdi_register_gb2312();
    //gdi_encoding_convert("iso6937", "gb2312");
    GxBus_ConfigSetInt(GXBUS_SEARCH_UTF8, GXBUS_SEARCH_UTF8_YES);
	widget_register("file_image", &file_image_ops);

	// clear the logo
	/*usr init*/
	app_system_init();
	GxOem_Init();
	//open player for play
	app_player_open(PLAYER_FOR_NORMAL);

#ifdef NETAPPS_SUPPORT
	app_player_open(PLAYER_FOR_IPTV);
#endif

	// start frontend service
    AppFrontend_Monitor monitor = FRONTEND_MONITOR_ON;

    for (i=0; i<NIM_MODULE_NUM; i++)
    {
        app_ioctl(i, FRONTEND_MONITOR_SET, &monitor);
    }


	config_irr_filter_gate();

#if LNB_SHORTCUT_SUPPORT
// for short cut 
    {
        extern void lnb_shortcut_control(void);
        lnb_shortcut_control();
    }
#endif
    ret = app_system_gui_init();
	if(GXCORE_SUCCESS != ret)
	{
		printf("[-app_system_gui_init-] ERROR\n");
		return GXCORE_ERROR;
	}

#if CASCAM_SUPPORT
{
    unsigned int flag = 0;

#if NSCAS_SUPPORT
    extern CASCAMControlBlockClass NSTVCASControlBlock;
    extern  void app_nstv_init_dialog(void);
    app_nstv_init_dialog();
    CASCAM_Register_CAS(&NSTVCASControlBlock);
    flag |= (1 << CASCAM_NSTV);
#endif

    CASCAMInitParaClass para;
    para.CASFlag = flag;
    CASCAM_Init(para);
}
#endif

#if (BOOT_LOGO_SUPPORT == 0)
    i = 40000000*3;
    while(i--);
    player_stop.player = "player4";
    app_send_msg_exec(GXMSG_PLAYER_STOP, &player_stop);
#endif


#if FACTORY_SERIALIZATION_SUPPORT
{
    extern int app_factory_serialization(int force_flag);
#if WFCAS_SUPPORT
    extern int factory_serialization_enter(int force_flag);
    factory_serialization_enter(1);
    app_factory_serialization_initialize_console();
    ret = 1;
#else
    ret = app_factory_serialization(0);
#endif
    if(ret == 1)
    {
#if (UPDATE_SUPPORT_FLAG)
        {
            ret = app_ota_loader_pop_exec();
#if (OTA_MONITOR_SUPPORT)
            app_ota_sync_init();
#endif
        }
#endif
        if(ret != 0)
        {
            app_create_dialog("wnd_full_screen");
        }

    }
}
#else
#if (UPDATE_SUPPORT_FLAG)
{
    extern int app_ota_loader_pop_exec(void);
    ret = app_ota_loader_pop_exec();
#if (OTA_MONITOR_SUPPORT)
    extern int app_ota_sync_init(void);
    app_ota_sync_init();
#endif
}
#endif
    if(ret != 0)
    {
	    app_create_dialog("wnd_full_screen");
    }
#endif
	printf("[GUI_Init] Success\n");

#if NETWORK_SUPPORT
    app_send_msg_exec(GXMSG_NETWORK_START, NULL);
#if BOX_SERVICE
    // here code for phone client
    extern void app_cmd_init();
    app_cmd_init();
    printf("[DLNA] Box command init\n");
#endif
#endif

#if NETAPPS_SUPPORT
	gxapps_init();
    gx_netapps_service_init();
#endif

#if EMM_SUPPORT
    app_emm_init();                                                                                                               
#endif
// unmute
    app_set_hardware_unmute();

{
    extern int app_rf_modulator_working(void);
    app_rf_modulator_working();
}

	return GXCORE_SUCCESS;
}

