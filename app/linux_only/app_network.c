#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_pop.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_default_params.h"

#if NETWORK_SUPPORT
#define IMG_OPT_HALF_FOCUS_L			"s_bar_half_focus_l.bmp"
#define IMG_OPT_HALF_FOCUS_R			"s_bar_half_focus_r.bmp"
#define IMG_OPT_HALF_UNFOCUS		"s_bar_half_unfocus.bmp"
#define IMG_SCAN_GREEN           "img_network_green"
#define TXT_SCAN_GREEN           "text_network_green"

typedef enum
{
    NET_ITEM_DEV,
    NET_ITEM_MODE,
    NET_ITEM_TYPE,
    NET_ITEM_IPADDR,
    NET_ITEM_SUB_MASK,
    NET_ITEM_GATEWAY,
    NET_ITEM_DNS1,
    NET_ITEM_DNS2,
    NET_ITEM_SAVE,
    NET_ITEM_STATE,
    NET_ITEM_TOTAL,
}NetWorkItem;

static uint32_t s_dev = 0;
static uint8_t s_sel = 0;
static uint32_t s_dev_total = 0;
static ubyte64 *s_network_dev = NULL;
static uint32_t s_mode = 0;
static uint32_t s_ip_type = 0;
static IfType s_dev_type = IF_TYPE_UNKOWN;
GxMsgProperty_NetDevIpcfg s_ip_cfg;
static IfState s_state = IF_STATE_INVALID;
static char *s_mode_content = "[Off,On]";
static char *s_iptype_content = "[Static IP,DHCP]";
static bool s_pppoe_flag = false;
static event_list *s_network_pop_timer = NULL;
static bool s_network_pop_flag = false;

#if WIFI_SUPPORT
extern status_t app_create_wifi_menu(ubyte64 *wifi_dev);
#endif

#if MOD_3G_SUPPORT
extern status_t app_create_3g_menu(ubyte64 *dev_3g);
#endif

#if PPPOE_SUPPORT
static char *s_pppoe_content = "[PPPoE]";
extern status_t app_create_pppoe_menu(ubyte64 *pppoe_dev);
#endif

static char *s_network_opt[NET_ITEM_TOTAL] = {
	"cmb_network_opt1",
	"cmb_network_opt2",
    "cmb_network_opt3",
	"edit_network_opt1",
	"edit_network_opt2",
	"edit_network_opt3",
	"edit_network_opt4",
	"edit_network_opt5",
	"btn_network_opt1",
	"text_network_opt1",
};

static char *s_network_text[NET_ITEM_TOTAL] = {
	"text_network_item1",
	"text_network_item2",
	"text_network_item3",
	"text_network_item4",
	"text_network_item5",
	"text_network_item6",
	"text_network_item7",
	"text_network_item8",
	"text_network_item9",
	"text_network_item10",
};

static char *s_network_img[NET_ITEM_TOTAL] =
{
	"img_network_item_choice1",
	"img_network_item_choice2",
	"img_network_item_choice3",
	"img_network_item_choice4",
	"img_network_item_choice5",
	"img_network_item_choice6",
	"img_network_item_choice7",
	"img_network_item_choice8",
	"img_network_item_choice9",
	"img_network_item_choice10",
};

static char *s_network_title_str[NET_ITEM_TOTAL] = {
    STR_ID_DEVICE,
    STR_ID_MODE,
    STR_ID_TYPE,
    STR_ID_IPADDR,
    STR_ID_SUB_MASK,
    STR_ID_GATE_WAY,
    STR_ID_DNS1,
    STR_ID_DNS2,
    STR_ID_SAVE,
    STR_ID_STATE,
};

static char* app_network_get_state_str(IfState state)
{
    static char *str = NULL;

    switch(state)
    {
        case IF_STATE_INVALID:
            str=STR_ID_INVALID;
            break;

        case IF_STATE_IDLE:
            str=STR_ID_NO_CONNECT;
            break;

        case IF_STATE_START:
        case IF_STATE_CONNECTING:
            str=STR_ID_CONNECTTING;
            break;

        case IF_STATE_CONNECTED:
            str=STR_ID_CONNECTED;
            break;

        case IF_STATE_REJ:
#if (WIFI_SUPPORT > 0)
            if(s_dev_type == IF_TYPE_WIFI)
                str = STR_ID_ERR_PASSWD;
            else
#endif
                str=STR_ID_NO_CONNECT;
            break;

        default:
            break;
    }

    return str;
}

void app_network_ip_update(void)
{
    app_network_ip_completion(s_ip_cfg.ip_addr);
    GUI_SetProperty(s_network_opt[NET_ITEM_IPADDR], "string", s_ip_cfg.ip_addr);

    app_network_ip_completion(s_ip_cfg.net_mask);
    GUI_SetProperty(s_network_opt[NET_ITEM_SUB_MASK], "string", s_ip_cfg.net_mask);

    app_network_ip_completion(s_ip_cfg.gate_way);
    GUI_SetProperty(s_network_opt[NET_ITEM_GATEWAY], "string", s_ip_cfg.gate_way);

    app_network_ip_completion(s_ip_cfg.dns1);
    GUI_SetProperty(s_network_opt[NET_ITEM_DNS1], "string", s_ip_cfg.dns1);

    app_network_ip_completion(s_ip_cfg.dns2);
    GUI_SetProperty(s_network_opt[NET_ITEM_DNS2], "string", s_ip_cfg.dns2);
}

void app_network_ip_clear(void)
{
#define INVALIDE_IP "000.000.000.000"
#define PUBLIC_DNS1 "008.008.008.008"
#define PUBLIC_DNS2 "208.067.222.222"
    int i = 0;

    //for(i = NET_ITEM_IPADDR; i <= NET_ITEM_GATEWAY; i++)
    //{
	//	GUI_SetProperty(s_network_opt[i], "string", INVALIDE_IP);
    //}

    //GUI_SetProperty(s_network_opt[NET_ITEM_DNS1], "string", PUBLIC_DNS1);
    //GUI_SetProperty(s_network_opt[NET_ITEM_DNS2], "string", PUBLIC_DNS2);
    
    for(i = NET_ITEM_IPADDR; i <= NET_ITEM_DNS2; i++)
    {
		GUI_SetProperty(s_network_opt[i], "string", INVALIDE_IP);
    }
}

void app_network_show_ip(void)
{
    if(s_ip_cfg.ip_type == s_ip_type && s_ip_type == 0)
    {
        app_network_ip_update();
    }
    else if((s_state != IF_STATE_CONNECTED) || (s_ip_cfg.ip_type == 0 && s_ip_type == 1))
    {
        app_network_ip_clear();
    }
    else
    {
        app_network_ip_update();
    }
}

void app_network_iptype_change()
{
	uint8_t i = 0;
    int cmb_sel = s_ip_type;

    if(s_pppoe_flag == true)
        cmb_sel = 0;

    GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "select", &cmb_sel);

    if((s_ip_type == 1) || (s_pppoe_flag == true))
	{
		for(i = NET_ITEM_IPADDR; i < NET_ITEM_SAVE; i++)
		{
			GUI_SetProperty(s_network_opt[i], "state", "disable");
			GUI_SetProperty(s_network_text[i], "state", "disable");
		}
	}
	else
	{
		for(i = NET_ITEM_IPADDR; i < NET_ITEM_SAVE; i++)
		{
			GUI_SetProperty(s_network_opt[i], "state", "enable");
			GUI_SetProperty(s_network_text[i], "state", "enable");
		}
	}
}

void app_network_mode_change()
{
	uint8_t i = 0;

	GUI_SetProperty(s_network_opt[NET_ITEM_MODE], "select", &s_mode);

    if(0 == s_dev_total)
    {
        GUI_SetProperty(s_network_opt[NET_ITEM_MODE], "state", "disable");
        GUI_SetProperty(s_network_text[NET_ITEM_MODE], "state", "disable");
    }
    else if(s_dev_total > 0)
    {
        GUI_SetProperty(s_network_opt[NET_ITEM_MODE], "state", "enable");
        GUI_SetProperty(s_network_text[NET_ITEM_MODE], "state", "enable");

    }

	if(s_mode == 0)
	{
		for(i = NET_ITEM_TYPE; i < NET_ITEM_SAVE; i++)
		{
			GUI_SetProperty(s_network_opt[i], "state", "disable");
			GUI_SetProperty(s_network_text[i], "state", "disable");
		}
	}
	else
	{
        GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "state", "enable");
		GUI_SetProperty(s_network_text[NET_ITEM_TYPE], "state", "enable");

        app_network_iptype_change();
    }
}

static void _network_dev_enable(char *dev)
{
    GxMsgProperty_NetDevMode dev_mode;

    if((dev == NULL) || (strlen(dev) == 0))
        return;

    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
    strcpy(dev_mode.dev_name, dev);
    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
    if(dev_mode.mode == 0)
    {
        dev_mode.mode = 1;
        app_send_msg_exec(GXMSG_NETWORK_SET_MODE, &dev_mode);
    }
}

static void _network_dev_disable(char *dev)
{
    GxMsgProperty_NetDevMode dev_mode;

    if((dev == NULL) || (strlen(dev) == 0))
        return;

    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
    strcpy(dev_mode.dev_name, dev);
    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
    if(dev_mode.mode == 1)
    {
        dev_mode.mode = 0;
        app_send_msg_exec(GXMSG_NETWORK_SET_MODE, &dev_mode);
    }
}

#ifdef ECOS_OS
static int _network_get_current_dev(void)
{
    int dev_index = 0;
    int i = 0;
    GxMsgProperty_NetDevMode dev_mode;

    if(s_network_dev != NULL)
    {
        for(i = 0; i < s_dev_total; i++)
        {
            memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
            strcpy(dev_mode.dev_name, s_network_dev[i]);
            app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
            if(dev_mode.mode == 1)
            {
                dev_index = i;
                break;
            }
        }
    }

    return dev_index;
}
#endif

#ifdef LINUX_OS
static int _network_get_current_dev(void)
{
    int dev_index = 0;
    int i = 0;
    ubyte64 cur_dev = {0};

    if(s_network_dev != NULL)
    {
        app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
        for(i = 0; i < s_dev_total; i++)
        {
            if(strcmp(cur_dev, s_network_dev[i]) == 0)
            {
                dev_index = i;
                break;
            }
        }
    }

    return dev_index;
}
#endif

void app_network_dev_update(void)
{
    GxMsgProperty_NetDevMode dev_mode;
    GxMsgProperty_NetDevState state;
    GxMsgProperty_NetDevType type;
    char *state_str = NULL;

    GUI_SetProperty(s_network_opt[NET_ITEM_DEV], "select", &s_dev);

    memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
    strcpy(type.dev_name, s_network_dev[s_dev]);
    app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
    s_dev_type = type.dev_type;

    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
    strcpy(dev_mode.dev_name, s_network_dev[s_dev]);
    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
    s_mode = dev_mode.mode;

    memset(&s_ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
    strcpy(s_ip_cfg.dev_name, s_network_dev[s_dev]);
    app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &s_ip_cfg);
#if (MOD_3G_SUPPORT > 0)
    if(s_dev_type == IF_TYPE_3G)
        s_ip_type = 1;//dhcp
    else
#endif
        s_ip_type = s_ip_cfg.ip_type;

#if (PPPOE_SUPPORT > 0)
	GxMsgProperty_PppoeCfg pppoe_cpp_network_show_ipfg;
    memset(&pppoe_cfg, 0, sizeof(GxMsgProperty_PppoeCfg));
    strcpy(pppoe_cfg.dev_name, s_network_dev[s_dev]);
    app_send_msg_exec(GXMSG_PPPOE_GET_PARAM, &pppoe_cfg);
    if(pppoe_cfg.mode > 0)
    {
        s_pppoe_flag = true;
        GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "content", s_pppoe_content);
    }
    else
    {
        s_pppoe_flag = false;
        GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "content", s_iptype_content);
    }
#endif

    memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
    strcpy(state.dev_name, s_network_dev[s_dev]);
    app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
    s_state = state.dev_state;

    app_network_iptype_change();
    app_network_mode_change();

    state_str = app_network_get_state_str(state.dev_state);
    GUI_SetProperty(s_network_opt[NET_ITEM_STATE], "string", state_str);

#if (WIFI_SUPPORT > 0)
    if(s_dev_type == IF_TYPE_WIFI)
    {
        GUI_SetProperty(IMG_SCAN_GREEN, "state", "show");
        GUI_SetProperty(TXT_SCAN_GREEN, "string", STR_ID_SCAN);
        GUI_SetProperty(TXT_SCAN_GREEN, "state", "show");
    }
    else
#endif
#if (MOD_3G_SUPPORT > 0)
    if(s_dev_type == IF_TYPE_3G) //3g
    {
        GUI_SetProperty(IMG_SCAN_GREEN, "state", "show");
        GUI_SetProperty(TXT_SCAN_GREEN, "string", STR_ID_SETUP);
        GUI_SetProperty(TXT_SCAN_GREEN, "state", "show");
    }
    else
#endif
#if (PPPOE_SUPPORT > 0)
    if(s_dev_type == IF_TYPE_WIRED) //pppoe
    {
        GUI_SetProperty(IMG_SCAN_GREEN, "state", "show");
        GUI_SetProperty(TXT_SCAN_GREEN, "string", STR_ID_PPPOE);
        GUI_SetProperty(TXT_SCAN_GREEN, "state", "show");
    }
    else
#endif
    {
        GUI_SetProperty(IMG_SCAN_GREEN, "state", "hide");
        GUI_SetProperty(TXT_SCAN_GREEN, "state", "hide");
    }
}

void app_network_update(void)
{
	uint8_t i =0;
    char *dev_content = NULL;
    ubyte64 display_name = {0};
    GxMsgProperty_NetDevList dev_list;

    memset(&dev_list, 0, sizeof(GxMsgProperty_NetDevList));
    app_send_msg_exec(GXMSG_NETWORK_DEV_LIST, &dev_list);

    if(s_network_dev != NULL)
	{
		GxCore_Free(s_network_dev);
		s_network_dev = NULL;
	}

    s_dev_total = dev_list.dev_num;
    if(s_dev_total == 0)
    {
        char *no_dev_content = "[None]";
        char *state_str = NULL;
        s_dev = 0;
        s_mode = 0;
        s_ip_type = 0;
        s_pppoe_flag = false;
        s_dev_type = IF_TYPE_UNKOWN;
        s_state = IF_STATE_INVALID;
        GUI_SetProperty(s_network_opt[NET_ITEM_DEV], "content", no_dev_content);
        app_network_ip_clear();
        app_network_iptype_change();
        app_network_mode_change();
        state_str = app_network_get_state_str(s_state);
        GUI_SetProperty(s_network_opt[NET_ITEM_STATE], "string", state_str);
    }
    else
    {
        if((s_network_dev = (ubyte64*)GxCore_Malloc(s_dev_total * sizeof(ubyte64))) != NULL)
        {
            memset(s_network_dev, 0, s_dev_total * sizeof(ubyte64));
            if((dev_content = (char*)GxCore_Malloc(s_dev_total * sizeof(ubyte64) + 5)) != NULL)
            {
                memset(dev_content, 0, s_dev_total * sizeof(ubyte64) + 5);
                for(i = 0; i < s_dev_total; i++)
                {
                    strcpy(s_network_dev[i], dev_list.dev_name[i]);
                    memset(display_name, 0, sizeof(ubyte64));
                    if(i ==0)
                    {
                        strcat(dev_content, "[");
                        app_if_name_convert(s_network_dev[i], display_name);
                        strcat(dev_content, display_name);
                    }
                    else
                    {
                        strcat(dev_content, ",");
                        app_if_name_convert(s_network_dev[i], display_name);
                        strcat(dev_content, display_name);
                    }
                }
                strcat(dev_content, "]");
                GUI_SetProperty(s_network_opt[NET_ITEM_DEV], "content", dev_content);
                GxCore_Free(dev_content);
                dev_content = NULL;
            }
        }
    }
}

void app_network_set_mac(char *dev, char *mac)
{
    GxMsgProperty_NetDevMac dev_mac;

    if((dev == NULL) || (strlen(dev) == 0) || (mac == NULL) || (strlen(mac) == 0))
        return;
    memset(&dev_mac, 0, sizeof(GxMsgProperty_NetDevMac));
    strcpy(dev_mac.dev_name, dev);
    strcpy(dev_mac.dev_mac, mac);
    app_send_msg_exec(GXMSG_NETWORK_SET_MAC, &dev_mac);
}

void app_network_get_mac(char *dev, char *mac)
{
    GxMsgProperty_NetDevMac dev_mac;

    if((dev == NULL) || (strlen(dev) == 0) || (mac == NULL))
        return;
    memset(&dev_mac, 0, sizeof(GxMsgProperty_NetDevMac));
    strcpy(dev_mac.dev_name, dev);
    app_send_msg_exec(GXMSG_NETWORK_GET_MAC, &dev_mac);
    strcpy(mac, dev_mac.dev_mac);
}

static void app_network_pop_info_show(const char *info_str)
{
    GUI_SetProperty("img_network_pop_bg", "state", "show");
    GUI_SetProperty("txt_network_pop_tip", "state", "show");
    GUI_SetProperty("txt_network_pop_info", "state", "show");
    GUI_SetProperty("txt_network_pop_info", "string", (void*)info_str);
    s_network_pop_flag = true;
}

static void app_network_pop_info_hide(void)
{
    GUI_SetProperty("img_network_pop_bg", "state", "hide");
    GUI_SetProperty("txt_network_pop_tip", "state", "hide");
    GUI_SetProperty("txt_network_pop_info", "state", "hide");
    s_network_pop_flag = false;
}

static int app_pop_info_timer_cb(void *use)
{
    uint32_t i = 0;
    ubyte64 cur_dev = {0};

    if(s_network_pop_timer != NULL)
    {
        remove_timer(s_network_pop_timer);
        s_network_pop_timer = NULL;
    }
    app_network_pop_info_hide();

    strcpy(cur_dev, s_network_dev[s_dev]);
    app_network_update();
    if(s_dev_total > 0)
    {
        s_dev = 0;
        for(i = 0; i < s_dev_total; i++)
        {
            if(strcmp(cur_dev, s_network_dev[i]) == 0)
            {
                s_dev = i;
                break;
            }
        }
		for(i =  NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_opt[i], "state", "enable");
			GUI_SetProperty(s_network_text[i], "state", "enable");
		}
	
        app_network_dev_update();
    }
	else
	{
		for(i = NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_opt[i], "state", "disable");
			GUI_SetProperty(s_network_text[i], "state", "disable");
		}
	
	}

    return 0;
}

static bool app_network_check_status(void)
{
    GxMsgProperty_NetDevState state;

    memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
    strcpy(state.dev_name, s_network_dev[s_dev]);
    app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
    s_state = state.dev_state;

    return ((state.dev_state == IF_STATE_CONNECTED) ? true : false);
}

static bool app_network_check_config(void)
{
    uint8_t i = 0;
    char *p[5] = {NULL};
    char buf[5][16];
    GxMsgProperty_NetDevMode dev_mode;

    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));

    for(i = 0; i < 5; i++)
    {
        GUI_GetProperty(s_network_opt[i+NET_ITEM_IPADDR], "string", &p[i]);
        app_network_ip_simplify(buf[i], p[i]);
    }

    strcpy(dev_mode.dev_name, s_ip_cfg.dev_name);
    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);

    if(strcmp(s_ip_cfg.dev_name, s_network_dev[s_dev]) != 0
            ||(dev_mode.mode != s_mode)
            ||(s_ip_cfg.ip_type != s_ip_type)
            ||strncmp(s_ip_cfg.ip_addr, buf[0], strlen(s_ip_cfg.ip_addr))
            ||strncmp(s_ip_cfg.net_mask, buf[1], strlen(s_ip_cfg.net_mask))
            ||strncmp(s_ip_cfg.gate_way, buf[2], strlen(s_ip_cfg.gate_way))
            ||strncmp(s_ip_cfg.dns1, buf[3], strlen(s_ip_cfg.dns1))
            ||strncmp(s_ip_cfg.dns2, buf[4], strlen(s_ip_cfg.dns2)))
    {
        return true;
    }

    return false;
}

void app_network_set_config(void)
{
    uint8_t i = 0;
	char *p[5] = {NULL};
	char buf[5][16];
    GxMsgProperty_NetDevMode dev_mode;
    GxMsgProperty_NetDevIpcfg dev_ipcfg;

    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
    memset(&dev_ipcfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));

    for(i = 0; i < 5; i++)
    {
        GUI_GetProperty(s_network_opt[i+NET_ITEM_IPADDR], "string", &p[i]);
        app_network_ip_simplify(buf[i], p[i]);
    }

    strcpy(dev_ipcfg.dev_name, s_network_dev[s_dev]);

    dev_ipcfg.ip_type = s_ip_type;
    strcpy(dev_ipcfg.ip_addr, buf[0]);
    strcpy(dev_ipcfg.net_mask, buf[1]);
    strcpy(dev_ipcfg.gate_way, buf[2]);
    strcpy(dev_ipcfg.dns1, buf[3]);
    strcpy(dev_ipcfg.dns2, buf[4]);

	strcpy(dev_mode.dev_name, s_network_dev[s_dev]);
    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
    if(dev_mode.mode != s_mode)
    {
        if(s_mode == 0)
        {
            dev_mode.mode = 0;
            app_send_msg_exec(GXMSG_NETWORK_SET_MODE, &dev_mode);
        }
        else
        {
            app_send_msg_exec(GXMSG_NETWORK_SET_IPCFG, &dev_ipcfg);
            dev_mode.mode = 1;
            app_send_msg_exec(GXMSG_NETWORK_SET_MODE, &dev_mode);
        }
    }
    else
    {
        if(s_mode == 1)
        {
            app_send_msg_exec(GXMSG_NETWORK_SET_IPCFG, &dev_ipcfg);
        }
    }
}

SIGNAL_HANDLER int app_network_cmb1_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_RIGHT:
				if((s_network_pop_flag == true) || (s_dev_total == 1) || (s_dev_total == 0))
					return EVENT_TRANSFER_STOP;

#ifdef ECOS_OS
                _network_dev_disable(s_network_dev[s_dev]);
#endif
				(s_dev < s_dev_total-1) ? (s_dev++) : (s_dev = 0);
#ifdef ECOS_OS
                _network_dev_enable(s_network_dev[s_dev]);
#endif
				app_network_dev_update();
                app_network_show_ip();
				break;
			case STBK_LEFT:
				if((s_network_pop_flag == true)|| (s_dev_total == 1) || (s_dev_total == 0))
					return EVENT_TRANSFER_STOP;

#ifdef ECOS_OS
                _network_dev_disable(s_network_dev[s_dev]);
#endif
				(s_dev > 0) ? (s_dev--) : (s_dev = s_dev_total - 1);
#ifdef ECOS_OS
                _network_dev_enable(s_network_dev[s_dev]);
#endif
				app_network_dev_update();
                app_network_show_ip();
				break;
            case STBK_UP:
				if(s_dev_total > 0)
				{
					if(s_network_pop_flag == true)
						return EVENT_TRANSFER_STOP;
					GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_UNFOCUS);
					GUI_SetProperty(s_network_text[NET_ITEM_DEV], "string", s_network_title_str[NET_ITEM_DEV]);
					GUI_SetProperty(s_network_img[NET_ITEM_SAVE], "img", IMG_OPT_HALF_FOCUS_L);
					GUI_SetProperty(s_network_text[NET_ITEM_SAVE], "string", s_network_title_str[NET_ITEM_SAVE]);
					s_sel = NET_ITEM_SAVE;
					GUI_SetFocusWidget(s_network_opt[NET_ITEM_SAVE]);
				}
				break;
			case STBK_DOWN:
				if(s_network_pop_flag == true)
					return EVENT_TRANSFER_STOP;
                if(s_dev_total > 0)
                {
				    GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_UNFOCUS);
				    GUI_SetProperty(s_network_text[NET_ITEM_DEV], "string", s_network_title_str[NET_ITEM_DEV]);
				    GUI_SetProperty(s_network_img[NET_ITEM_MODE], "img", IMG_OPT_HALF_FOCUS_L);
				    GUI_SetProperty(s_network_text[NET_ITEM_MODE], "string", s_network_title_str[NET_ITEM_MODE]);
				    s_sel = NET_ITEM_MODE;
				    GUI_SetFocusWidget(s_network_opt[NET_ITEM_MODE]);
                }
				break;

			case STBK_GREEN:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;
                else
                    return EVENT_TRANSFER_KEEPON;

			case STBK_EXIT:
			case STBK_MENU:
				return EVENT_TRANSFER_KEEPON;

			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_cmb2_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_RIGHT:
			case STBK_LEFT:
                if(s_network_pop_flag == true || s_dev_total == 0)
                    return EVENT_TRANSFER_STOP;

				s_mode = (s_mode+1)%2;
				app_network_mode_change();
                app_network_show_ip();
				break;
			case STBK_UP:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_MODE], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_MODE], "string", s_network_title_str[NET_ITEM_MODE]);
                GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_DEV], "string", s_network_title_str[NET_ITEM_DEV]);
				s_sel = NET_ITEM_DEV;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_DEV]);
				break;
			case STBK_DOWN:
				if(s_dev_total > 0)
				{
					if(s_network_pop_flag == true)
						return EVENT_TRANSFER_STOP;

					GUI_SetProperty(s_network_img[NET_ITEM_MODE], "img", IMG_OPT_HALF_UNFOCUS);
					GUI_SetProperty(s_network_text[NET_ITEM_MODE], "string", s_network_title_str[NET_ITEM_MODE]);
					if(s_mode == 1)
					{
						GUI_SetProperty(s_network_img[NET_ITEM_TYPE], "img", IMG_OPT_HALF_FOCUS_L);
						GUI_SetProperty(s_network_text[NET_ITEM_TYPE], "string", s_network_title_str[NET_ITEM_TYPE]);
						s_sel = NET_ITEM_TYPE;
						GUI_SetFocusWidget(s_network_opt[NET_ITEM_TYPE]);
					}
					else
					{
						GUI_SetProperty(s_network_img[NET_ITEM_SAVE], "img", IMG_OPT_HALF_FOCUS_L);
						GUI_SetProperty(s_network_text[NET_ITEM_SAVE], "string", s_network_title_str[NET_ITEM_SAVE]);
						s_sel = NET_ITEM_SAVE;
						GUI_SetFocusWidget(s_network_opt[NET_ITEM_SAVE]);
					}
				}
				else
				{
					if(s_network_pop_flag == true)
						return EVENT_TRANSFER_STOP;
					GUI_SetProperty(s_network_img[NET_ITEM_MODE], "img", IMG_OPT_HALF_UNFOCUS);
					GUI_SetProperty(s_network_text[NET_ITEM_MODE], "string", s_network_title_str[NET_ITEM_MODE]);
					GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_FOCUS_L);
					GUI_SetProperty(s_network_text[NET_ITEM_DEV], "string", s_network_title_str[NET_ITEM_DEV]);
					s_sel = NET_ITEM_DEV;
					GUI_SetFocusWidget(s_network_opt[NET_ITEM_DEV]);
				}
				break;

            case STBK_GREEN:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;
                else
                    return EVENT_TRANSFER_KEEPON;

			case STBK_EXIT:
			case STBK_MENU:
				return EVENT_TRANSFER_KEEPON;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_cmb3_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case STBK_RIGHT:
            case STBK_LEFT:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

#if (MOD_3G_SUPPORT > 0)
                if(s_dev_type == IF_TYPE_3G)
                {
                    s_ip_type = 1;
                }
                else
#endif
                {
                    s_ip_type = (s_ip_type+1)%2;
                }

                app_network_iptype_change();
                app_network_show_ip();

                break;
            case STBK_UP:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_TYPE], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_TYPE], "string", s_network_title_str[NET_ITEM_TYPE]);
                GUI_SetProperty(s_network_img[NET_ITEM_MODE], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_MODE], "string", s_network_title_str[NET_ITEM_MODE]);
                s_sel = NET_ITEM_MODE;
                GUI_SetFocusWidget(s_network_opt[NET_ITEM_MODE]);
                break;
            case STBK_DOWN:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_TYPE], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_TYPE], "string", s_network_title_str[NET_ITEM_TYPE]);
                if((s_ip_type == 1) || (s_pppoe_flag == true))
                {
                    GUI_SetProperty(s_network_img[NET_ITEM_SAVE], "img", IMG_OPT_HALF_FOCUS_L);
                    GUI_SetProperty(s_network_text[NET_ITEM_SAVE], "string", s_network_title_str[NET_ITEM_SAVE]);
                    s_sel = NET_ITEM_SAVE;
                    GUI_SetFocusWidget(s_network_opt[NET_ITEM_SAVE]);
                }
                else
                {
                    GUI_SetProperty(s_network_img[NET_ITEM_IPADDR], "img", IMG_OPT_HALF_FOCUS_L);
                    GUI_SetProperty(s_network_text[NET_ITEM_IPADDR], "string", s_network_title_str[NET_ITEM_IPADDR]);
                    s_sel = NET_ITEM_IPADDR;
                    GUI_SetFocusWidget(s_network_opt[NET_ITEM_IPADDR]);
                }

                break;

            case STBK_GREEN:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;
                else
                    return EVENT_TRANSFER_KEEPON;

            case STBK_EXIT:
            case STBK_MENU:
                return EVENT_TRANSFER_KEEPON;
            default:
                break;
        }
	}
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_network_edit1_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_UP:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_IPADDR], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_IPADDR], "string", s_network_title_str[NET_ITEM_IPADDR]);
                GUI_SetProperty(s_network_img[NET_ITEM_TYPE], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_TYPE], "string", s_network_title_str[NET_ITEM_TYPE]);
				s_sel = NET_ITEM_TYPE;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_TYPE]);
				return EVENT_TRANSFER_STOP;
			case STBK_DOWN:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_IPADDR], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_IPADDR], "string", s_network_title_str[NET_ITEM_IPADDR]);
                GUI_SetProperty(s_network_img[NET_ITEM_SUB_MASK], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_SUB_MASK], "string", s_network_title_str[NET_ITEM_SUB_MASK]);
                s_sel = NET_ITEM_SUB_MASK;
                GUI_SetFocusWidget(s_network_opt[NET_ITEM_SUB_MASK]);
				return EVENT_TRANSFER_STOP;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_network_edit2_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_UP:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_SUB_MASK], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_SUB_MASK], "string", s_network_title_str[NET_ITEM_SUB_MASK]);
                GUI_SetProperty(s_network_img[NET_ITEM_IPADDR], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_IPADDR], "string", s_network_title_str[NET_ITEM_IPADDR]);
				s_sel = NET_ITEM_IPADDR;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_IPADDR]);
				return EVENT_TRANSFER_STOP;
			case STBK_DOWN:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_SUB_MASK], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_SUB_MASK], "string", s_network_title_str[NET_ITEM_SUB_MASK]);
                GUI_SetProperty(s_network_img[NET_ITEM_GATEWAY], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_GATEWAY], "string", s_network_title_str[NET_ITEM_GATEWAY]);
				s_sel = NET_ITEM_GATEWAY;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_GATEWAY]);
				return EVENT_TRANSFER_STOP;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_network_edit3_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_UP:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_GATEWAY], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_GATEWAY], "string", s_network_title_str[NET_ITEM_GATEWAY]);
                GUI_SetProperty(s_network_img[NET_ITEM_SUB_MASK], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_SUB_MASK], "string", s_network_title_str[NET_ITEM_SUB_MASK]);
				s_sel = NET_ITEM_SUB_MASK;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_SUB_MASK]);
				return EVENT_TRANSFER_STOP;
			case STBK_DOWN:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_GATEWAY], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_GATEWAY], "string", s_network_title_str[NET_ITEM_GATEWAY]);
                GUI_SetProperty(s_network_img[NET_ITEM_DNS1], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_DNS1], "string", s_network_title_str[NET_ITEM_DNS1]);
				s_sel = NET_ITEM_DNS1;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_DNS1]);
				return EVENT_TRANSFER_STOP;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_network_edit4_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_UP:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_DNS1], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_DNS1], "string", s_network_title_str[NET_ITEM_DNS1]);
                GUI_SetProperty(s_network_img[NET_ITEM_GATEWAY], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_GATEWAY], "string", s_network_title_str[NET_ITEM_GATEWAY]);
				s_sel = NET_ITEM_GATEWAY;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_GATEWAY]);
				return EVENT_TRANSFER_STOP;
			case STBK_DOWN:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_DNS1], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_DNS1], "string", s_network_title_str[NET_ITEM_DNS1]);
                GUI_SetProperty(s_network_img[NET_ITEM_DNS2], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_DNS2], "string", s_network_title_str[NET_ITEM_DNS2]);
				s_sel = NET_ITEM_DNS2;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_DNS2]);
				return EVENT_TRANSFER_STOP;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_network_edit5_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_UP:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_DNS2], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_DNS2], "string", s_network_title_str[NET_ITEM_DNS2]);
                GUI_SetProperty(s_network_img[NET_ITEM_DNS1], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_DNS1], "string", s_network_title_str[NET_ITEM_DNS1]);
				s_sel = NET_ITEM_DNS1;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_DNS1]);
				return EVENT_TRANSFER_STOP;
			case STBK_DOWN:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_DNS2], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_DNS2], "string", s_network_title_str[NET_ITEM_DNS2]);
                GUI_SetProperty(s_network_img[NET_ITEM_SAVE], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_SAVE], "string", s_network_title_str[NET_ITEM_SAVE]);
				s_sel = NET_ITEM_SAVE;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_SAVE]);
				return EVENT_TRANSFER_STOP;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_network_btn1_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_UP:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_SAVE], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_SAVE], "string", s_network_title_str[NET_ITEM_SAVE]);

                if(s_mode == 1)
                {
                    if((s_ip_type == 1) || (s_pppoe_flag == true))
                    {
                        GUI_SetProperty(s_network_img[NET_ITEM_TYPE], "img", IMG_OPT_HALF_FOCUS_L);
                        GUI_SetProperty(s_network_text[NET_ITEM_TYPE], "string", s_network_title_str[NET_ITEM_TYPE]);
                        s_sel = NET_ITEM_TYPE;
                        GUI_SetFocusWidget(s_network_opt[NET_ITEM_TYPE]);
                    }
                    else
                    {
                        GUI_SetProperty(s_network_img[NET_ITEM_DNS2], "img", IMG_OPT_HALF_FOCUS_L);
                        GUI_SetProperty(s_network_text[NET_ITEM_DNS2], "string", s_network_title_str[NET_ITEM_DNS2]);
                        s_sel = NET_ITEM_DNS2;
                        GUI_SetFocusWidget(s_network_opt[NET_ITEM_DNS2]);
                    }

                }
				else
				{
					GUI_SetProperty(s_network_img[NET_ITEM_MODE], "img", IMG_OPT_HALF_FOCUS_L);
                    GUI_SetProperty(s_network_text[NET_ITEM_MODE], "string", s_network_title_str[NET_ITEM_MODE]);
                    s_sel = NET_ITEM_MODE;
					GUI_SetFocusWidget(s_network_opt[NET_ITEM_MODE]);
				}
                break;

            case STBK_DOWN:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                GUI_SetProperty(s_network_img[NET_ITEM_SAVE], "img", IMG_OPT_HALF_UNFOCUS);
                GUI_SetProperty(s_network_text[NET_ITEM_SAVE], "string", s_network_title_str[NET_ITEM_SAVE]);
                GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_FOCUS_L);
                GUI_SetProperty(s_network_text[NET_ITEM_DEV], "string", s_network_title_str[NET_ITEM_DEV]);
				s_sel = NET_ITEM_DEV;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_DEV]);
                break;

            case STBK_OK:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                if((!app_network_check_config()) && (app_network_check_status()))
                    return EVENT_TRANSFER_STOP;

				if(s_dev_total > 0)
                {
                    app_network_set_config();
                    app_network_pop_info_show(STR_ID_WAITING);
                    if((s_mode == 0) && ((s_ip_type == 1)||(s_pppoe_flag == true)))
                        app_network_ip_clear();
                    if(0 != reset_timer(s_network_pop_timer))
                    {
                        s_network_pop_timer = create_timer(app_pop_info_timer_cb, 1000, NULL, TIMER_ONCE);
                    }
                }
                break;

            case STBK_GREEN:
                if(s_network_pop_flag == true)
                    return EVENT_TRANSFER_STOP;
                else
                    return EVENT_TRANSFER_KEEPON;

			case STBK_EXIT:
			case STBK_MENU:
				return EVENT_TRANSFER_KEEPON;

            default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_create(const char* widgetname, void *usrdata)
{
	int i = 0;
	for(i = NET_ITEM_MODE; i < NET_ITEM_TOTAL; i++)
	{
		GUI_SetProperty(s_network_img[i], "img", IMG_OPT_HALF_UNFOCUS);
	}

	s_sel = NET_ITEM_DEV;

	GUI_SetProperty(s_network_opt[NET_ITEM_MODE], "content", s_mode_content);
	GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "content", s_iptype_content);

    GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_FOCUS_L);
	app_network_update();
    s_dev = _network_get_current_dev();
    if(s_dev_total > 0)
	{
		for(i = NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_opt[i], "state", "enable");
			GUI_SetProperty(s_network_text[i], "state", "enable");
		}
		app_network_dev_update();
        app_network_show_ip();
	}
	else
	{
		for(i = NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_opt[i], "state", "disable");
			GUI_SetProperty(s_network_text[i], "state", "disable");
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_destroy(const char* widgetname, void *usrdata)
{
    s_network_pop_flag = false;
    if(s_network_pop_timer != NULL)
    {
        remove_timer(s_network_pop_timer);
        s_network_pop_timer = NULL;
    }

	if(s_network_dev != NULL)
	{
		GxCore_Free(s_network_dev);
		s_network_dev = NULL;
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_lost_focus(GuiWidget *widget, void *usrdata)
{
	 GUI_SetProperty(s_network_opt[s_sel], "unfocus_img", IMG_OPT_HALF_FOCUS_R);
	 return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_got_focus(GuiWidget *widget, void *usrdata)
{
	GUI_SetProperty(s_network_opt[s_sel], "unfocus_img", IMG_OPT_HALF_UNFOCUS);

    ubyte64 cur_dev = {0};
    uint32_t i = 0;

    if(s_network_dev != NULL)
    {
        strcpy(cur_dev, s_network_dev[s_dev]);
    }

    app_network_update();
    if(s_dev_total > 0)
    {
        s_dev = 0;
        for(i = 0; i < s_dev_total; i++)
        {
            if(strcmp(cur_dev, s_network_dev[i]) == 0)
            {
                s_dev = i;
                break;
            }
        }
		for(i =  NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_opt[i], "state", "enable");
			GUI_SetProperty(s_network_text[i], "state", "enable");
		}

        app_network_dev_update();
        app_network_show_ip();
		if(s_mode == 0)
		{
			if(s_sel == NET_ITEM_TYPE)
			{
				GUI_SetProperty(s_network_img[s_sel], "img", IMG_OPT_HALF_UNFOCUS);
				GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_FOCUS_L);
				s_sel = NET_ITEM_DEV;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_DEV]);
			}
		}

		if((s_ip_type == 1) || (s_pppoe_flag == true))
		{
			if((s_sel > NET_ITEM_TYPE) && (s_sel < NET_ITEM_SAVE))
			{
				GUI_SetProperty(s_network_img[s_sel], "img", IMG_OPT_HALF_UNFOCUS);
				GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_FOCUS_L);
				s_sel = NET_ITEM_DEV;
				GUI_SetFocusWidget(s_network_opt[NET_ITEM_DEV]);
			}
		}
    }
	else
	{
        GUI_SetProperty(s_network_img[s_sel], "img", IMG_OPT_HALF_UNFOCUS);
        GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_FOCUS_L);
		for(i =  NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_opt[i], "state", "disable");
			GUI_SetProperty(s_network_text[i], "state", "disable");
		}

		 GUI_SetProperty(IMG_SCAN_GREEN, "state", "hide");
		 GUI_SetProperty(TXT_SCAN_GREEN, "state", "hide");
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
#if 0
            case STBK_1:
            {
                ubyte32 mac = {0};
				if(s_dev_total > 0)
                    app_network_get_mac(s_network_dev[s_dev], mac);
                printf("~~~~~~[%s]%d, %s mac: %s!!!!!\n", __func__, __LINE__, s_network_dev[s_dev], mac);
            }
            break;

            case STBK_2:
            {
                char *mac = "00:55:7B:B5:7D:F9";
				if(s_dev_total > 0)
                    app_network_set_mac(s_network_dev[s_dev], mac);
            }
            break;
#endif
            case STBK_GREEN:
            {
#if (WIFI_SUPPORT > 0)
                if(s_dev_type == IF_TYPE_WIFI)
                {
                    ubyte64 *wifi_dev = &s_network_dev[s_dev];
                    app_create_wifi_menu(wifi_dev);
                }
#endif

#if (MOD_3G_SUPPORT > 0)
                if(s_dev_type == IF_TYPE_3G)
                {
                    ubyte64 *_3g_dev = &s_network_dev[s_dev];
                    app_create_3g_menu(_3g_dev);
                }
#endif

#if (PPPOE_SUPPORT > 0)
                if(s_dev_type == IF_TYPE_WIRED) //pppoe
                {
                    ubyte64 *pppoe_dev = &s_network_dev[s_dev];
                    app_create_pppoe_menu(pppoe_dev);
                }
#endif
            }
            break;

			case STBK_MENU:
			case STBK_EXIT:
				GUI_EndDialog("wnd_network");
				break;
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

void _network_state_msg_proc(GxMsgProperty_NetDevState *state)
{
    char *state_str = NULL;

    if(state == NULL)
        return;

    if(strcmp(state->dev_name, s_network_dev[s_dev]) != 0)
        return;

    //printf("~~~~~~[%s]%d, %s state change %d!!!!!\n", __func__, __LINE__, state->dev_name,  state->dev_state);

    if(state->dev_state == IF_STATE_INVALID)
    {
        //rebuild
    }
    else
    {
        s_state = state->dev_state;
        if(s_network_pop_flag == false)
        {
            if(state->dev_state == IF_STATE_CONNECTED)
            {
                memset(&s_ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
                strcpy(s_ip_cfg.dev_name, s_network_dev[s_dev]);
                app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &s_ip_cfg);

                if(s_ip_type != s_ip_cfg.ip_type)
                {
                    GUI_SetProperty(s_network_img[s_sel], "img", IMG_OPT_HALF_UNFOCUS);
                    GUI_SetProperty(s_network_text[s_sel], "string", s_network_title_str[s_sel]);
                    GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_FOCUS_L);
                    GUI_SetProperty(s_network_text[NET_ITEM_DEV], "string", s_network_title_str[NET_ITEM_DEV]);
                    s_sel = NET_ITEM_DEV;
                    GUI_SetFocusWidget(s_network_opt[NET_ITEM_DEV]);
                }

#if (MOD_3G_SUPPORT > 0)
                if(s_dev_type == IF_TYPE_3G)
                    s_ip_type = 1; //dhcp
                else
#endif
                    s_ip_type = s_ip_cfg.ip_type;
				app_network_iptype_change();
                app_network_mode_change();
                app_network_show_ip();
            }
            else
            {
                if((s_ip_type == 1) || (s_pppoe_flag == true))
                    app_network_ip_clear();
            }
            state_str = app_network_get_state_str(state->dev_state);
            GUI_SetProperty(s_network_opt[NET_ITEM_STATE], "string", state_str);
        }
    }
}

static void _network_plug_proc(ubyte64 *net_dev)
{
    ubyte64 cur_dev = {0};
    uint32_t i = 0;

    if(s_network_pop_flag == true)
        return;

    if(s_network_dev != NULL)
    {
        strcpy(cur_dev, s_network_dev[s_dev]);
    }

    app_network_update();
    if(s_dev_total > 0)
    {
#ifdef LINUX_OS
        s_dev = 0;
        for(i = 0; i < s_dev_total; i++)
        {
            if(strcmp(cur_dev, s_network_dev[i]) == 0)
            {
                s_dev = i;
                break;
            }
        }
#endif

#ifdef ECOS_OS
        s_dev = _network_get_current_dev();
#endif

        GUI_SetProperty(s_network_opt[NET_ITEM_SAVE], "state", "enable");
        GUI_SetProperty(s_network_text[NET_ITEM_SAVE], "state", "enable");

        app_network_dev_update();
        app_network_show_ip();

#ifdef LINUX_OS
        if(i >= s_dev_total)
#endif
#ifdef ECOS_OS
        if(strcmp(cur_dev, s_network_dev[s_dev]) != 0)
#endif
        {
            GUI_SetProperty(s_network_img[s_sel], "img", IMG_OPT_HALF_UNFOCUS);
            GUI_SetProperty(s_network_text[s_sel], "string", s_network_title_str[s_sel]);
            GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_FOCUS_L);
            GUI_SetProperty(s_network_text[NET_ITEM_DEV], "string", s_network_title_str[NET_ITEM_DEV]);
            s_sel = NET_ITEM_DEV;
            GUI_SetFocusWidget(s_network_opt[NET_ITEM_DEV]);
        }
    }
    else
    {
        for(i = NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
        {
            GUI_SetProperty(s_network_opt[i], "state", "disable");
            GUI_SetProperty(s_network_text[i], "state", "disable");
        }
        GUI_SetProperty(s_network_img[s_sel], "img", IMG_OPT_HALF_UNFOCUS);
        GUI_SetProperty(s_network_text[s_sel], "string", s_network_title_str[s_sel]);
        GUI_SetProperty(s_network_img[NET_ITEM_DEV], "img", IMG_OPT_HALF_FOCUS_L);
        GUI_SetProperty(s_network_text[NET_ITEM_DEV], "string", s_network_title_str[NET_ITEM_DEV]);
        s_sel = NET_ITEM_DEV;
        GUI_SetFocusWidget(s_network_opt[NET_ITEM_DEV]);

        GUI_SetProperty(IMG_SCAN_GREEN, "state", "hide");
        GUI_SetProperty(TXT_SCAN_GREEN, "state", "hide");
    }
}

void app_network_msg_proc(GxMessage *msg)
{
    if(msg == NULL)
        return;

    switch(msg->msg_id)
    {
        case GXMSG_NETWORK_STATE_REPORT:
        {
            GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
            _network_state_msg_proc(dev_state);
            break;
        }

        case GXMSG_NETWORK_PLUG_IN:
        {
            ubyte64 *net_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            //printf("~~~~~~[%s]%d, %s plug in!!!!!\n", __func__, __LINE__, *net_dev);
            _network_plug_proc(net_dev);
            break;
        }

        case GXMSG_NETWORK_PLUG_OUT:
        {
            ubyte64 *net_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            //printf("~~~~~~[%s]%d, %s plug out!!!!!\n", __func__, __LINE__, *net_dev);
            _network_plug_proc(net_dev);
            break;
        }

        default:
            break;
    }
}

#else //NETWORK_SUPPORT
SIGNAL_HANDLER int app_network_create(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_destroy(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_keypress(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_lost_focus(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_got_focus(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_cmb1_keypress(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_cmb2_keypress(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_cmb3_keypress(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_edit1_keypress(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_edit2_keypress(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_edit3_keypress(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_edit4_keypress(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_edit5_keypress(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

SIGNAL_HANDLER int app_network_btn1_keypress(const char* widgetname, void *usrdata)
{ return EVENT_TRANSFER_STOP;}

#endif //NETWORK_SUPPORT
