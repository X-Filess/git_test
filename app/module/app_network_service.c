#include "app.h"
#include "app_msg.h"
#include "app_module.h"

#ifdef ECOS_OS
#include <sys/socket.h>
#include <net/if.h>
#include <network.h>
#include <dhcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "gxusb3gplug.h"
#if MOD_3G_SUPPORT
#include "gxusb3g.h"
#endif
bool thiz_usb3g_hotplug_status = false;
extern int app_check_wifi_status(void);
//extern int app_get_wifi_mac(char * mac);
//extern bool app_check_dhcp_status(void);
//extern char * app_get_dhcp_ip(void);
//extern char * app_get_dhcp_msg(unsigned char nTag);
//extern int net_ifconfig(char * dev_name, char * cmd);
extern int net_ifconfig(char *ifrname,char *cmd,char *argument);
#endif

//#ifdef LINUX_OS
#if NETWORK_SUPPORT

#define PATH_LEN  (100)
#define RESULT_LEN  (100)
#define CMD_LEN  (100)
#define CONSOLE_DELAY_MS   (800)

#define NET_IF_DEV_LOOP    "lo"
#define NET_IF_DEV_ETH0    "eth0"
#define PROC_NET_DEV       "/proc/net/dev"
#define NETWORK_MONITOR_DIR "/tmp/ethernet/monitor/"
#define NETWORK_MANUAL_DIR "/tmp/ethernet/manual/"
#define PPPOE_FILE "/usr/share/app/ethernet/pppoe_cfg"

//CMD
#define WIRED_START_CMD "wired_start"
#define WIRED_STOP_CMD "wired_stop"
#define WIFI_START_CMD "wifi_start"
#define WIFI_STOP_CMD "wifi_stop"
#define WIFI_SCAN_CMD "wifi_scan"
#define WIFI_SAVE_AP_CMD "wifi_save_ap"
#define WIFI_DEL_AP_CMD "wifi_del_ap"
#define IF_SAVE_CONFIG_CMD "if_save_cfg"
#define IF_GET_CONFIG_CMD "if_get_cfg"
#define RM_FILE_CMD "rm -f"

//3G
#define _3G_OPERATOR_KEY    "3g>operator_id"
#define _3G_OPERATOR_DEFAULT    0

#define USB3G_LOOKUP_CMD "3g_lookup"
#define USB3G_START_CMD "3g_start"
#define USB3G_STOP_CMD "3g_stop"
#define USB3G_SET_PARAM_CMD "tg_setParam"
#define USB3G_GET_PARAM_CMD "tg_getParam"

#define USB3G_DEV_HEAD	"USB3G"

#define SAVE_PPPOE_CMD "pppoe_save_cfg"

enum
{
    MIN_PRIORITY = 0,
    WIRED_PRIORITY = 10,
    WIFI_PRIORITY = 20,
    TG_PRIORITY = 30,
    MAX_PRIORITY = 40,
};

typedef enum
{
    IF_FILE_NONE = 0,
    IF_FILE_START,
    IF_FILE_WAIT,
    IF_FILE_OK,
    IF_FILE_FAIL,
    IF_FILE_REJ
}IfFileRet;

typedef struct
{
    char dev_mode;
    char ip_type;
    ubyte32 ip_addr;
    ubyte32 net_mask;
    ubyte32 gate_way;
    ubyte32 dns1;
    ubyte32 dns2;
    ubyte32 mac;
}IpCfgFileData;

typedef struct
{
    ubyte64 essid;
    ubyte32 mac;
    ubyte32 psk;
}ApFileData;

typedef struct
{
    char mode;
    ubyte32 username;
    ubyte32 passwd;
    ubyte32 dns1;
    ubyte32 dns2;
}PppoeFileData;

typedef struct if_device IfDevice;
typedef struct
{
    status_t (*if_start)(IfDevice*);
    status_t (*if_stop)(IfDevice*);
    IfFileRet (*if_monitor_connect)(IfDevice*); //ret: 1,ok; -1,failed; 0,connecting
}IfOps;

struct if_device
{
    ubyte64 dev_name;
    IfState dev_state;
    IfType dev_type;
    int dev_priority;
    bool user_flag;
    bool main_flag;
    bool cur_flag;
    bool scanning_flag;
    IfOps *dev_ops;
    handle_t start_thread_handle;
    handle_t scan_thread_handle;
    handle_t dev_mutex;
};

typedef struct if_device_list IfDevList;
struct if_device_list
{
    IfDevice net_dev;
    IfDevList *next;
};

static status_t _wired_dev_start(IfDevice *wired_dev);
static status_t _wired_dev_stop(IfDevice *wired_dev);
static IfFileRet _wired_dev_connect_monitor(IfDevice *wired_dev);

#if (WIFI_SUPPORT > 0)
static status_t _wifi_dev_start(IfDevice *wifi_dev);
static status_t _wifi_dev_stop(IfDevice *wifi_dev);
static IfFileRet _wifi_dev_connect_monitor(IfDevice *wifi_dev);
#endif

#if (MOD_3G_SUPPORT > 0)
#ifdef LINUX_OS
#define _3G_CONNNECT_RECORD "/usr/share/app/ethernet/3g_connect_record"
#else
#define _3G_CONNNECT_RECORD "/home/gx/3g_connect_record"
#endif
static status_t _3g_dev_start(IfDevice *_3g_dev);
static status_t _3g_dev_stop(IfDevice *_3g_dev);
static IfFileRet _3g_dev_connect_monitor(IfDevice *_3g_dev);
#endif

static IfOps s_net_ops_set[IF_TYPE_UNKOWN] =
{
    {_wired_dev_start, _wired_dev_stop, _wired_dev_connect_monitor},
#if (WIFI_SUPPORT > 0)
    {_wifi_dev_start,  _wifi_dev_stop,  _wifi_dev_connect_monitor},
#endif
#if (MOD_3G_SUPPORT > 0)
    {_3g_dev_start,    _3g_dev_stop,    _3g_dev_connect_monitor},
#endif
};

static IfState s_global_net_state = IF_STATE_IDLE;
static IfDevList *if_dev_list = NULL;
static handle_t s_dev_list_mutex = -1;
static int s_dev_type_num[IF_TYPE_UNKOWN+1] = {0};
static struct
{
    int dev_num;
    ubyte64 net_dev[MAX_NET_DEV_NUM];
}s_prev_init_dev;

static int s_network_dev_priority = MAX_PRIORITY;
static bool s_network_service_enable = false;
static handle_t s_network_service_mutex = -1;
static int s_wifi_init_status = -1;
int g_wifi_hotplug_status = -1;
static IfDevice* _if_dev_list_get(const char *dev_name);

static status_t _set_ip_cfg_file_data(const char *dev_name, IpCfgFileData *cfg_data)
{
#ifdef LINUX_OS
    char cmd_buf[256] = {0};

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (cfg_data == NULL))
        return GXCORE_ERROR;

    sprintf(cmd_buf, "%s %s %d %d %s %s %s %s %s", IF_SAVE_CONFIG_CMD, dev_name, cfg_data->dev_mode,
            cfg_data->ip_type, cfg_data->ip_addr, cfg_data->net_mask, cfg_data->gate_way, cfg_data->dns1, cfg_data->dns2);
    //printf("-----[%s]%d %s-----\n",  __func__, __LINE__, cmd_buf);
    system(cmd_buf);

    return GXCORE_SUCCESS;
#else
    if((dev_name == NULL) || (strlen(dev_name) == 0) || (cfg_data == NULL))
        return GXCORE_ERROR;

    //dev mode
#if (WIFI_SUPPORT > 0)
    if(strcmp("ra0", dev_name) == 0)
    {
        GxBus_ConfigSetInt(DEV_WIFI_MODE_KEY, cfg_data->dev_mode);
    }
#endif

#if (MOD_3G_SUPPORT > 0)
    if(strcmp("USB3G", dev_name) == 0)
    {
        GxBus_ConfigSetInt(DEV_3G_MODE_KEY, cfg_data->dev_mode);
    }
#endif
//    printf("~~~[%s]%d dev_mode: %d~~~\n", __func__, __LINE__, cfg_data->dev_mode);

    GxBus_ConfigSetInt(IP_TYPE_KEY, cfg_data->ip_type);
//    printf("~~~[%s]%d ip_type: %d~~~\n", __func__, __LINE__,  cfg_data->ip_type);

    //ip addr
    GxBus_ConfigSet(IP_ADDR_KEY, cfg_data->ip_addr);
//    printf("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

    //net mask
    GxBus_ConfigSet(NET_MASK_KEY, cfg_data->net_mask);
//    printf("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

    //gate way
    GxBus_ConfigSet(GATE_WAY_KEY, cfg_data->gate_way);
//    printf("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

    //dns1
    GxBus_ConfigSet(DNS1_KEY, cfg_data->dns1);
//    printf("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

    //dns2
    GxBus_ConfigSet(DNS2_KEY, cfg_data->dns2);
//    printf("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);
    return GXCORE_SUCCESS;
#endif
}

static status_t _get_ip_cfg_file_data(const char *dev_name, IpCfgFileData *cfg_data)
{
#ifdef LINUX_OS
    FILE *ret_fp = NULL;
    char ret_buf[256] = {0};
    char result_path[PATH_LEN] = {0};
    char cmd_buf[CMD_LEN] = {0};

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (cfg_data == NULL))
        return GXCORE_ERROR;

    sprintf(result_path, "%s%s%s", NETWORK_MANUAL_DIR, dev_name, "_config");
    sprintf(cmd_buf, "%s %s %s", IF_GET_CONFIG_CMD, result_path, dev_name);
    //printf("!!!!!!!!!!----- [%s]%d %s ----- !!!!!!!!!!\n",  __func__, __LINE__, cmd_buf);
    system(cmd_buf);

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(result_path))
    {
        return GXCORE_ERROR;
    }

    if((ret_fp = fopen(result_path, "r")) == NULL)
    {
        return GXCORE_ERROR;
    }

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->mac, ret_buf);
    //printf("~~~[%s]%d mac: %s~~~\n", __func__, __LINE__, cfg_data->mac);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    cfg_data->dev_mode = atoi(ret_buf);
    //printf("~~~[%s]%d mac: %s~~~\n", __func__, __LINE__, cfg_data->mac);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    cfg_data->ip_type = atoi(ret_buf);
    //printf("~~~[%s]%d ip_type: %d~~~\n", __func__, __LINE__,  cfg_data->ip_type);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->ip_addr, ret_buf);
    //printf("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->net_mask, ret_buf);
    //printf("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->gate_way, ret_buf);
    //printf("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->dns1, ret_buf);
    //printf("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->dns2, ret_buf);
    //printf("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);

    fclose(ret_fp);

	memset(cmd_buf, 0, CMD_LEN);
    sprintf(cmd_buf, "%s %s", RM_FILE_CMD, result_path);
    system(cmd_buf);

#else
	int32_t init_value;

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (cfg_data == NULL))
        return GXCORE_ERROR;

    //mac
   // app_get_wifi_mac(cfg_data->mac);
//    printf("~~~[%s]%d mac: %s~~~\n", __func__, __LINE__, cfg_data->mac);

    //dev mode
    if(strcmp("ra0", dev_name) == 0)
        GxBus_ConfigGetInt(DEV_WIFI_MODE_KEY, &init_value, DEV_MODE_DEFALT);
    if(strcmp("USB3G", dev_name) == 0)
        GxBus_ConfigGetInt(DEV_3G_MODE_KEY, &init_value, DEV_MODE_DEFALT);
//	GxBus_ConfigGetInt(DEV_MODE_KEY, &init_value, DEV_MODE_DEFALT);
    cfg_data->dev_mode = init_value;
//    printf("~~~[%s]%d dev_mode: %d~~~\n", __func__, __LINE__, cfg_data->dev_mode);

    //ip type
	GxBus_ConfigGetInt(IP_TYPE_KEY, &init_value, IP_TYPE_DEFALT);
    cfg_data->ip_type = init_value;
//    printf("~~~[%s]%d ip_type: %d~~~\n", __func__, __LINE__,  cfg_data->ip_type);

    //ip addr
    GxBus_ConfigGet(IP_ADDR_KEY, cfg_data->ip_addr, sizeof(cfg_data->ip_addr), IP_ADDR_DEFALT);
//    printf("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

    //net mask
    GxBus_ConfigGet(NET_MASK_KEY, cfg_data->net_mask, sizeof(cfg_data->net_mask), NET_MASK_DEFALT);
//    printf("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

    //gate way
    GxBus_ConfigGet(GATE_WAY_KEY, cfg_data->gate_way, sizeof(cfg_data->gate_way), GATE_WAY_DEFALT);
//    printf("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

    //dns1
    GxBus_ConfigGet(DNS1_KEY, cfg_data->dns1, sizeof(cfg_data->dns1), DNS1_DEFALT);
//    printf("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

    //dns2
    GxBus_ConfigGet(DNS2_KEY, cfg_data->dns2, sizeof(cfg_data->dns2), DNS2_DEFALT);
//    printf("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);
#endif

    return GXCORE_SUCCESS;
}

static status_t _network_get_mac_cfg(const char *dev, char* mac)
{
#ifdef LINUX_OS
#define NET_DEV_MAC_DIR "/usr/share/app/ethernet/"
	//int len = 0;
	char buf[128];
    char result_path[PATH_LEN] = {0};
	FILE * fp = NULL;

	if((dev == NULL ) || (mac == NULL))
	{
		printf("\nNET, wrong parameter\n");
		return GXCORE_ERROR;
	}

    sprintf(result_path, "%s%s%s", NET_DEV_MAC_DIR, dev, "_mac");
    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(result_path))
    {
        return GXCORE_ERROR;
    }

	fp = fopen(result_path, "r");
	if(fp ==NULL)
	{
		printf("\nNET, open file failed\n");
		return GXCORE_ERROR;
	}

	memset(buf, 0, 128);
	if(fgets(buf, 32, fp) == NULL)
    {
        fclose(fp);
        return GXCORE_ERROR;
    }

    if(buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = 0;

	fclose(fp);

	strcpy(mac, buf);
	printf("\nNET, GET MAC = %s\n", mac);

    return GXCORE_SUCCESS;
#else

	if((dev == NULL ) || (mac == NULL))
	{
		printf("\nNET, wrong parameter\n");
		return GXCORE_ERROR;
	}

   // app_get_wifi_mac(mac);
	printf("\nNET, GET MAC = %s\n", mac);

    return GXCORE_SUCCESS;
#endif
}

static status_t _network_set_mac(const char *dev, const char *mac)
{// pMac = 00:AA:BB:CC:DD:44
#ifdef LINUX_OS
#define NET_DEV_MAC_DIR "/usr/share/app/ethernet/"
	FILE * fp = NULL;
    char cmd_buf[CMD_LEN] = {0};
    char result_path[PATH_LEN] = {0};

	if((dev == NULL) || (mac == NULL))
	{
		printf("\nNET, wrong MAC\n");
		return GXCORE_ERROR;
	}

    sprintf(cmd_buf, "ifconfig %s hw ether %s", dev, mac);
    //printf("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
    system(cmd_buf);

    sprintf(result_path, "%s%s%s", NET_DEV_MAC_DIR, dev, "_mac");
	// flush the record file
	fp = fopen(result_path, "wb");
	if(fp ==NULL)
	{
		printf("\nNET, open file failed\n");
		return GXCORE_ERROR;
	}
	memset(cmd_buf, 0, CMD_LEN);
	sprintf(cmd_buf, "%s", mac);
	if(fwrite(cmd_buf, strlen(cmd_buf), 1, fp) != 1)
	{
		printf("\nNET, write failed!\n");
		fclose(fp);
		return GXCORE_ERROR;
	}
	fclose(fp);

    return GXCORE_SUCCESS;
#else
//TODO:config_macaddr

    return GXCORE_SUCCESS;
#endif
}

static IfFileRet _check_dev_result_file(IfState if_state, char *dev_name)
{
    IfFileRet ret = -1;
    char ret_buf[RESULT_LEN] = {0};
    FILE *ret_fp = NULL;
    char result_path[PATH_LEN] = {0};

    if(dev_name == NULL)
        return -1;

    sprintf(result_path, "%s%s%c%s", NETWORK_MONITOR_DIR, dev_name, '/', "state");
    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(result_path))
    {
        return IF_FILE_NONE;
    }
    if((ret_fp = fopen(result_path, "r")) == NULL)
    {
        return IF_FILE_NONE;
    }

    fgets(ret_buf, RESULT_LEN - 1, ret_fp);

    if(strncmp(ret_buf, "ok", 2) == 0)
        ret = IF_FILE_OK;
    else if(strncmp(ret_buf, "start", 5) == 0)
        ret = IF_FILE_START;
    else if(strncmp(ret_buf, "wait", 4) == 0)
        ret = IF_FILE_WAIT;
    else if(strncmp(ret_buf, "fail", 4) == 0)
        ret = IF_FILE_FAIL;
    else if(strncmp(ret_buf, "reject", 6) == 0)
        ret = IF_FILE_REJ;
    else
    {
        printf("ret_bufret_bufret_buf %s\n", ret_buf);
        ret = IF_FILE_NONE;
    }

    fclose(ret_fp);

    return ret;
}

#if (WIFI_SUPPORT > 0)
static ubyte32 s_static_ip_addr = {0};
static ApInfo s_ap_list[MAX_AP_NUM];
#ifdef ECOS_OS

static bool _check_is_dhcp(void)
{
	int32_t init_value;
	GxBus_ConfigGetInt(IP_TYPE_KEY, &init_value, IP_TYPE_DEFALT);
    return (init_value == 1) ? true : false;
}

static IfFileRet _check_wifi_result_file(IfState if_state, char *dev_name)
{
    IfFileRet ret = -1;
    char ret_buf[RESULT_LEN] = {0};

    if(dev_name == NULL)
        return -1;

    int nStatus = app_check_wifi_status();
    GxBus_ConfigGet(WIFI_STATUS_KEY, ret_buf, sizeof(ret_buf), WIFI_STATUS_DEFAULT);
    //printf("Wifi Status : %d %s\n", nStatus, ret_buf);

    if(strncmp(ret_buf, "ok", 2) == 0)
        ret = IF_FILE_OK;
    else if(strncmp(ret_buf, "start", 5) == 0)
        ret = IF_FILE_START;
    else if(strncmp(ret_buf, "wait", 4) == 0)
        ret = IF_FILE_WAIT;
    else if(strncmp(ret_buf, "fail", 4) == 0)
        ret = IF_FILE_FAIL;
    else if(strncmp(ret_buf, "reject", 6) == 0)
        ret = IF_FILE_REJ;
    else
    {
        printf("ret_bufret_bufret_buf %s\n", ret_buf);
        ret = IF_FILE_NONE;
    }

    //4:connected
    if(nStatus == 4 && ret == IF_FILE_WAIT)
    {
        extern int net_deladdr(const char *if_name, char *ip);
        extern int network_down(char *if_name);

        //clear last net config
        net_deladdr(dev_name, (char *)s_static_ip_addr);
        network_down(dev_name);

        if(_check_is_dhcp())
        {
            extern void network_init(char *if_name);
            extern int network_dhcp(char *if_name);

            int dhcp_status;

            printf("\033[33mDo DHCP ... !!!\n\033[0m");
            network_init(dev_name);
            dhcp_status = network_dhcp(dev_name);
            if(dhcp_status == 0)
            {
                extern int getip(char *ifrname,unsigned int *ip, unsigned int *mask, unsigned char *mac);
                extern int getdns(char *ifrname,struct in_addr *gateway,struct in_addr *dnsA,struct in_addr *dnsB);
                IpCfgFileData cfg_data;
                unsigned int ip, mask;
                unsigned char mac[15];
                struct in_addr gateway,dnsA,dnsB;
                memset(&cfg_data, 0, sizeof(cfg_data));

                cfg_data.dev_mode = 1;
                cfg_data.ip_type = 1;
                getip(dev_name, &ip, &mask, mac);
                getdns(dev_name, &gateway, &dnsA, &dnsB);
                struct in_addr sin;
                sin.s_addr=ip;
                strcpy(cfg_data.ip_addr, inet_ntoa(sin));
                sin.s_addr=mask;
                strcpy(cfg_data.net_mask, inet_ntoa(sin));
                sin=gateway;
                strcpy(cfg_data.gate_way, inet_ntoa(sin));
                sin=dnsA;
                strcpy(cfg_data.dns1, inet_ntoa(sin));
                sin=dnsB;
                strcpy(cfg_data.dns2, inet_ntoa(sin));

                _set_ip_cfg_file_data(dev_name, &cfg_data);
                memset(s_static_ip_addr, 0, sizeof(cfg_data.ip_addr));
                GxBus_ConfigSet(WIFI_STATUS_KEY, "ok");
            }
            else
            {
                GxBus_ConfigSet(WIFI_STATUS_KEY, "fail");
            }
        }
        else
        {
            IpCfgFileData cfg_data;
            struct bootp bootp_data;
            struct in_addr addr;

            printf("\033[33mDo STATIC ... !!!\n\033[0m");

            memset(&cfg_data, 0, sizeof(IpCfgFileData));
            if(_get_ip_cfg_file_data(dev_name, &cfg_data) == GXCORE_SUCCESS)
            {
                uint32_t ip[4] = {0};
                uint32_t mask[4] = {0};
                unsigned char broadcast[4] = {0};
                char tmp[20] = {0};

                sscanf(cfg_data.ip_addr, "%d.%d.%d.%d", &(ip[0]), &(ip[1]), &(ip[2]), &(ip[3]));
                sscanf(cfg_data.net_mask, "%d.%d.%d.%d", &(mask[0]), &(mask[1]), &(mask[2]), &(mask[3]));

                broadcast[0] = ((unsigned char)ip[0] & (unsigned char)mask[0]) | (unsigned char)(~(mask[0]));
                broadcast[1] = ((unsigned char)ip[1] & (unsigned char)mask[1]) | (unsigned char)(~(mask[1]));
                broadcast[2] = ((unsigned char)ip[2] & (unsigned char)mask[2]) | (unsigned char)(~(mask[2]));
                broadcast[3] = ((unsigned char)ip[3] & (unsigned char)mask[3]) | (unsigned char)(~(mask[3]));

                sprintf(tmp, "%d.%d.%d.%d", broadcast[0], broadcast[1], broadcast[2], broadcast[3]);
                printf("BroadCast Addr : %s\n", tmp);

                build_bootp_record(&bootp_data, 
                        dev_name,
                        cfg_data.ip_addr,
                        cfg_data.net_mask,
                        tmp,
                        cfg_data.gate_way,
                        cfg_data.gate_way);

                if(!init_net("ra0", &bootp_data))
                {
                	net_deladdr("ra0", cfg_data.ip_addr);
                    GxBus_ConfigSet(WIFI_STATUS_KEY, "fail");
                    printf("init_net failed !\n");
                }
                else
                {
                    addr.s_addr = inet_addr(cfg_data.dns1);
                    cyg_dns_res_init(&addr);
                    memcpy(s_static_ip_addr, cfg_data.ip_addr, sizeof(cfg_data.ip_addr));
                    GxBus_ConfigSet(WIFI_STATUS_KEY, "ok");
                    printf("init_net succed.\n");
                }
            }
            else
            {
                GxBus_ConfigSet(WIFI_STATUS_KEY, "fail");
                printf("init_net failed !\n");
            }
        }
    }
    else
    {
        if(nStatus == 4 && ret != IF_FILE_OK)
            printf("\033[32m[ERR:%s] WIFI_S: %d RET: %d\n\033[0m", __func__, nStatus, ret);
    }

    //12:Authancation fail 32:4-way handle shark fail,key fail 
    if((nStatus == 12 || nStatus == 32) && ret == IF_FILE_WAIT)
    {
        //TODO: CLEAR AP RECORD
        GxBus_ConfigSet(WIFI_STATUS_KEY, "reject");
    }
    return ret;
}
#endif

static status_t _get_ap_psk(const char *ap_essid, const char *ap_mac, char *ap_psk)
{
#ifdef LINUX_OS
#define WIFI_AP_REC_FILE "/usr/share/app/ethernet/wifi_ap_record"
    FILE *ret_fp = NULL;
    char ret_buf[RESULT_LEN] = {0};
    char seq_char = ',';
    char *p = NULL;
    char *q = NULL;

    if((ap_essid == NULL) || (ap_mac == 0) || (ap_psk == NULL))
        return GXCORE_ERROR;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(WIFI_AP_REC_FILE))
        return GXCORE_ERROR;

    if((ret_fp = fopen(WIFI_AP_REC_FILE, "r")) == NULL)
        return GXCORE_ERROR;

    while(fgets(ret_buf, RESULT_LEN - 1, ret_fp) != NULL)
    {
        if(ret_buf[strlen(ret_buf) -1] == '\n')
            ret_buf[strlen(ret_buf) -1] = 0;

        p = ret_buf;
        q = strchr(p, seq_char);
        //printf("~~~~~~[%s]%d, ap file essid: %s!!!!!\n", __func__, __LINE__, p);
        if((q != NULL) && (strncmp(ap_essid, p, q-p) == 0)) //ap essid
        {
            if((p = q + 1) != '\0')
            {
                q = strchr(p, seq_char);
                //printf("~~~~~~[%s]%d, ap file mac: %s!!!!!\n", __func__, __LINE__, p);
                if((q != NULL) && (strncmp(ap_mac, p, q-p) == 0)) //mac
                {
                    if((p = q + 1) != '\0')//psk
                    {
                        strcpy(ap_psk, p);
                        //printf("~~~~~~[%s]%d, ap file psk: %s!!!!!\n", __func__, __LINE__, ap_psk);
                    }
                    fclose(ret_fp);
                    return GXCORE_SUCCESS;
                }
            }
        }
        memset(ret_buf, 0, RESULT_LEN);
    }

    fclose(ret_fp);
    return GXCORE_ERROR;

#else
    extern status_t app_wifi_ap_load(char * mac, char * ssid, char * psk);
    ApFileData ap_data;
    memset(&ap_data, 0, sizeof(ApFileData));

    if((ap_essid == NULL) || (ap_mac == 0) || (ap_psk == NULL))
        return GXCORE_ERROR;

    if(GXCORE_SUCCESS == app_wifi_ap_load((char *)ap_mac, ap_data.essid, ap_data.psk))
    {
        //if((ap_data.mac != NULL) && (strncmp(ap_mac, ap_data.mac, sizeof(ap_data.mac)) == 0)) //mac
        {
            //if((ap_data.essid != NULL) && (strncmp(ap_essid, ap_data.essid, sizeof(ap_data.essid)) == 0)) //ap essid
            //{
            //  strcpy((char *)ap_essid, ap_data.essid);
            //}
            strcpy((char *)ap_psk, ap_data.psk);
            printf("\033[31mGet ap data from flash : Name: %s MAC: %s PSK: %s\n\033[0m", ap_essid, ap_mac, ap_data.psk);
            return GXCORE_SUCCESS;
        }
    }

    return GXCORE_ERROR;
#endif
}

static int _ap_quality_cmp(const void *a, const void *b)
{
    ApInfo *ap_1 = (ApInfo*)a;
    ApInfo *ap_2 = (ApInfo*)b;
    int ret = 0;

    if((ap_1 == NULL) || (ap_2 == NULL))
        return 0;

    if(ap_1->ap_quality > ap_2->ap_quality)
        ret = -1;
    else if(ap_1->ap_quality < ap_2->ap_quality)
        ret = 1;

    return ret;
}


#ifdef LINUX_OS
static unsigned int _get_ap_list_file_data(const char *aplist_file, ApInfo *ap_data, unsigned int max_ap_num)
{
#define ADD_HEAD "Address"
#define SSID_HEAD "ESSID"
#define Q_HEAD "Quality"
#define KEY_HEAD "key"

    FILE *ret_fp = NULL;
    char ret_buf[RESULT_LEN] = {0};
    unsigned int i = 0;
    unsigned int res_num = 0;

    if((aplist_file == NULL) || (ap_data == NULL))
        return 0;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(aplist_file))
        return 0;

    if((ret_fp = fopen(aplist_file, "r")) == NULL)
    {
        fclose(ret_fp);
        return 0;
    }

    if(fgets(ret_buf, RESULT_LEN - 1, ret_fp)  == NULL)//mac of wifi dev
    {
        fclose(ret_fp);
        return 0;
    }

    memset(ret_buf, 0, RESULT_LEN);
    if(fgets(ret_buf, RESULT_LEN - 1, ret_fp)  == NULL) //ap num
    {
        fclose(ret_fp);
        return 0;
    }

    memset(ret_buf, 0, RESULT_LEN);
    while(fgets(ret_buf, RESULT_LEN - 1, ret_fp) != NULL)
    {
        if(ret_buf[strlen(ret_buf) -1] == '\n')
            ret_buf[strlen(ret_buf) -1] = 0;

        if(strncmp(ret_buf, ADD_HEAD, strlen(ADD_HEAD)) == 0)
        {
            res_num++;
            i = res_num - 1;
            strcpy(ap_data[i].ap_mac, ret_buf + strlen(ADD_HEAD));
        }
        else if(strncmp(ret_buf, SSID_HEAD, strlen(SSID_HEAD)) == 0)
        {
            strcpy(ap_data[i].ap_essid, ret_buf + strlen(SSID_HEAD));
        }
        else if(strncmp(ret_buf, Q_HEAD, strlen(Q_HEAD)) == 0)
        {
            ap_data[i].ap_quality = atoi(ret_buf + strlen(Q_HEAD));
        }
        else if(strncmp(ret_buf, KEY_HEAD, strlen(KEY_HEAD)) == 0)
        {
            if(strncmp(ret_buf + strlen(KEY_HEAD), "on", 2) == 0)
                ap_data[i].encryption = 1;
            else
                ap_data[i].encryption = 0;
        }
    }

    //sort by quality
    qsort(ap_data, res_num, sizeof(ApInfo), _ap_quality_cmp);

    fclose(ret_fp);
    return res_num;
}
#endif

static status_t _set_wifi_connect_file_data(const char *wifi_dev, ApFileData *ap_data)
{
#ifdef LINUX_OS
    char cmd_buf[256] = {0};

    if((wifi_dev == NULL) || (strlen(wifi_dev) == 0) || (ap_data == NULL))
        return GXCORE_ERROR;

    sprintf(cmd_buf, "%s %s \"%s\" %s \"%s\"", WIFI_SAVE_AP_CMD, wifi_dev, ap_data->essid,
            ap_data->mac, ap_data->psk);
    //printf("-----[%s]%d %s-----\n",  __func__, __LINE__, cmd_buf);
    system(cmd_buf);
#else
    extern status_t app_wifi_ap_save(char * mac, char * ssid, char * psk);

    if(ap_data->psk && 0 != strlen(ap_data->psk))
        app_wifi_ap_save(ap_data->mac, ap_data->essid, ap_data->psk);
    GxBus_ConfigSet(AP0_DEV_KEY, wifi_dev);
    GxBus_ConfigSet(AP0_SSID_KEY, ap_data->essid);
    GxBus_ConfigSet(AP0_MAC_KEY, ap_data->mac);
    GxBus_ConfigSet(AP0_PSK_KEY, ap_data->psk);
#endif

    return GXCORE_SUCCESS;
}

static status_t _get_wifi_connect_file_data(const char *wifi_dev, ApFileData *ap_data)
{
#ifdef LINUX_OS
#define WIFI_CONNECT_FILE "/usr/share/app/ethernet/wifi_connect_record"
    FILE *ret_fp = NULL;
    char ret_buf[256] = {0};
    char seq_char = ',';
    char *p = NULL;
    char *q = NULL;

    if((wifi_dev == NULL) || (strlen(wifi_dev) == 0) || (ap_data == NULL))
        return GXCORE_ERROR;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(WIFI_CONNECT_FILE))
        return GXCORE_ERROR;

    if((ret_fp = fopen(WIFI_CONNECT_FILE, "r")) == NULL)
        return GXCORE_ERROR;

    while(fgets(ret_buf, sizeof(ret_buf), ret_fp) != NULL)
    {
        if(ret_buf[strlen(ret_buf) -1] == '\n')
            ret_buf[strlen(ret_buf) -1] = 0;

        p = ret_buf;
        q = strchr(p, seq_char);
        if((q != NULL) && (strncmp(wifi_dev, p, q-p) == 0)) //wifi name
        {
            if((p = q + 1) != '\0')
            {
                q = strchr(p, seq_char);
                if(q != NULL) //essid
                {
                    strncpy(ap_data->essid, p, q-p);
                    //printf("~~~~~~[%s]%d, wifi file essid: %s!!!!!\n", __func__, __LINE__, ap_data->essid);

                    if((p = q + 1) != '\0')
                    {
                        q = strchr(p, seq_char);
                        if(q != NULL) //mac
                        {
                            strncpy(ap_data->mac, p, q-p);
                            //printf("~~~~~~[%s]%d, wifi file mac: %s!!!!!\n", __func__, __LINE__, ap_data->mac);

                            if((p = q + 1) != '\0')
                            {
                                strcpy(ap_data->psk, p);
                                //printf("~~~~~~[%s]%d, wifi file psk: %s!!!!!\n", __func__, __LINE__, ap_data->psk);
                            }
                        }
                    }
                }
            }
            fclose(ret_fp);
            return GXCORE_SUCCESS;
        }
        memset(ret_buf, 0, sizeof(ret_buf));
    }

    fclose(ret_fp);
    return GXCORE_ERROR;
#else
    char ret_buf[RESULT_LEN] = {0};

    GxBus_ConfigGet(AP0_DEV_KEY, ret_buf, sizeof(ret_buf), AP_DEV_DEFAULT);
    if(strcmp(wifi_dev, ret_buf) == 0) //wifi name
    {
        GxBus_ConfigGet(AP0_MAC_KEY, ap_data->mac, sizeof(ap_data->mac), AP_MAC_DEFAULT);
        if((ap_data->mac != NULL) && (strncmp(ap_data->mac, AP_MAC_DEFAULT, sizeof(AP_MAC_DEFAULT)) == 0))
            return GXCORE_ERROR;

        GxBus_ConfigGet(AP0_SSID_KEY, ap_data->essid, sizeof(ap_data->essid), AP_SSID_DEFAULT);
        GxBus_ConfigGet(AP0_PSK_KEY, ap_data->psk, sizeof(ap_data->psk), AP_PSK_DEFAULT);
        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
#endif
}
#endif

#if (PPPOE_SUPPORT > 0)
static status_t _set_pppoe_file_data(const char *dev_name, PppoeFileData *pppoe_data)
{
    char cmd_buf[256] = {0};

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (pppoe_data == NULL))
        return GXCORE_ERROR;

    sprintf(cmd_buf, "%s %s %d \"%s\" \"%s\" %s %s", SAVE_PPPOE_CMD, dev_name,
            pppoe_data->mode, pppoe_data->username, pppoe_data->passwd, pppoe_data->dns1, pppoe_data->dns2);
    printf("-----[%s]%d %s-----\n",  __func__, __LINE__, cmd_buf);
    system(cmd_buf);

    return GXCORE_SUCCESS;
}

static status_t _get_pppoe_file_data(const char *dev_name, PppoeFileData *pppoe_data)
{
    FILE *ret_fp = NULL;
    char ret_buf[256] = {0};
    char seq_char = ',';
    char *p = NULL;
    char *q = NULL;
    char temp[10] = {0};

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (pppoe_data == NULL))
        return GXCORE_ERROR;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(PPPOE_FILE))
        return GXCORE_ERROR;

    if((ret_fp = fopen(PPPOE_FILE, "r")) == NULL)
        return GXCORE_ERROR;

    while(fgets(ret_buf, sizeof(ret_buf), ret_fp) != NULL)
    {
        if(ret_buf[strlen(ret_buf) -1] == '\n')
            ret_buf[strlen(ret_buf) -1] = 0;

        p = ret_buf;
        q = strchr(p, seq_char);
        if((q != NULL) && (strncmp(dev_name, p, q-p) == 0)) //dev name
        {
            if((p = q + 1) != '\0')
            {
                q = strchr(p, seq_char);
                if(q != NULL) //mode
                {
                    strncpy(temp, p, q-p);
                    pppoe_data->mode = atoi(temp);
                    if((p = q + 1) != '\0')
                    {
                        q = strchr(p, seq_char);
                        if(q != NULL)//username
                        {
                            strncpy(pppoe_data->username, p, q-p);
                            //printf("[%s]%d, %s\n",__func__, __LINE__, p);
                            if((p = q + 1) != '\0')
                            {
                                q = strchr(p, seq_char);
                                if(q != NULL) //passwd
                                {
                                    strncpy(pppoe_data->passwd, p, q-p);
                                    //printf("[%s]%d, %s\n", __func__, __LINE__, p);
                                    if((p = q + 1) != '\0')
                                    {
                                        q = strchr(p, seq_char);
                                        {
                                            if(q != NULL) //dns1
                                            {
                                                //printf("[%s]%d, %s\n", __func__, __LINE__, p);
                                                strncpy(pppoe_data->dns1, p, q-p);
                                                if((p = q + 1) != '\0')
                                                {
                                                //printf("[%s]%d, %s\n", __func__, __LINE__, p);
                                                    strcpy(pppoe_data->dns2, p);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            fclose(ret_fp);
            //printf("~~~~~~[%s]%d, pppoe file passwd: %s!!!!!\n", __func__, __LINE__, pppoe_data->mac);
            return GXCORE_SUCCESS;
        }
        memset(ret_buf, 0, sizeof(ret_buf));
    }

    fclose(ret_fp);
    return GXCORE_ERROR;
}
#endif

#if MOD_3G_SUPPORT
static status_t _get_3g_cfg_file_data(GxMsgProperty_3G *tgParam)
{
    FILE *ret_fp = NULL;
    char ret_buf[256] = {0};
    char seq_char = ',';
    char *p = NULL;
    char *q = NULL;

    if(tgParam == NULL)
        return GXCORE_ERROR;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(_3G_CONNNECT_RECORD))
        return GXCORE_ERROR;

    if((ret_fp = fopen(_3G_CONNNECT_RECORD, "r")) == NULL)
        return GXCORE_ERROR;

    while(fgets(ret_buf, sizeof(ret_buf), ret_fp) != NULL)
    {
        if(ret_buf[strlen(ret_buf) -1] == '\n')
            ret_buf[strlen(ret_buf) -1] = 0;

        p = ret_buf;
        q = strchr(p, seq_char);

        if((q == NULL) || (strncmp(tgParam->dev_name, p, q-p) != 0)) //3g name
        {
            memset(ret_buf, 0, sizeof(ret_buf));
            continue;
        }

        if((p = q + 1) == '\0')
        {
            memset(ret_buf, 0, sizeof(ret_buf));
            continue;
        }

        if((q = strchr(p, seq_char)) == NULL) //operator id
        {
            memset(ret_buf, 0, sizeof(ret_buf));
            continue;
        }

        if(tgParam->operator_id != atoi(p))
        {
            memset(ret_buf, 0, sizeof(ret_buf));
            continue;
        }

        if((p = q + 1) == '\0')
        {
            break;
        }

        q = strchr(p, seq_char);
        if(q != NULL) //apn
        {
            strncpy(tgParam->apn, p, q-p);
            if((p = q + 1) != '\0')
            {
                q = strchr(p, seq_char);
                if(q != NULL) //phone
                {
                    strncpy(tgParam->phone, p, q-p);
                    if((p = q + 1) != '\0')
                    {
                        q = strchr(p, seq_char);
                        if(q != NULL) //username
                        {
                            strncpy(tgParam->username, p, q-p);
                            if((p = q + 1) != '\0')
                            {
                                //password
                                strcpy(tgParam->password, p);
                            }
                        }
                    }
                }
            }
        }

        fclose(ret_fp);
        return GXCORE_SUCCESS;
    }

    fclose(ret_fp);
    return GXCORE_ERROR;
}

#ifdef LINUX_OS
static status_t _set_3g_cfg_file_data(GxMsgProperty_3G *_3g_param)
{
    char cmd_buf[256] = {0};

	if((_3g_param == NULL) || (strlen(_3g_param->dev_name) == 0))
        return GXCORE_ERROR;

	printf("dev_name:%s, operator_id:%d, apn:%s, phone:%s, username:%s, passwd:%s\n", _3g_param->dev_name, _3g_param->operator_id,
            _3g_param->apn, _3g_param->phone, _3g_param->username, _3g_param->password);

	sprintf(cmd_buf, "%s \"%s\" \"%d\" \"%s\" \"%s\" \"%s\" \"%s\"", USB3G_SET_PARAM_CMD, _3g_param->dev_name, _3g_param->operator_id,
            _3g_param->apn, _3g_param->phone, _3g_param->username, _3g_param->password);
	printf("-----[%s]%d %s-----\n",  __func__, __LINE__, cmd_buf);
	system(cmd_buf);

    return GXCORE_SUCCESS;
}
#endif

#ifdef ECOS_OS
static status_t _mv_3g_cfg(const char *src, const char *dest)
{
    FILE *src_fp = NULL;
    FILE *dest_fp = NULL;
    char ret_buf[RESULT_LEN] = {0};

    if((src == NULL) || (dest == NULL))
        return GXCORE_ERROR;

    if(strcmp(src, dest) == 0)
        return GXCORE_ERROR;

    if((src_fp = fopen(src, "r")) == NULL)
        return GXCORE_ERROR;

    if((dest_fp = fopen(dest, "w+")) == NULL)
    {
        fclose(src_fp);
        return GXCORE_ERROR;
    }

    while(fgets(ret_buf, RESULT_LEN - 1, src_fp) != NULL)
    {
        fwrite(ret_buf, 1, strlen(ret_buf), dest_fp);
        memset(ret_buf, 0, sizeof(ret_buf));
    }

    fclose(src_fp);
    fclose(dest_fp);

    GxCore_FileDelete(src);

    return GXCORE_SUCCESS;
}

static status_t _set_3g_cfg_file_data(GxMsgProperty_3G *_3g_param)
{
#define _3G_RECORD_TMP "/mnt/3g_connect_temp"
    FILE *ret_fp = NULL;
    FILE *temp_fp = NULL;
    char ret_buf[256] = {0};
    char seq_char = ',';
    char *p = NULL;
    char *q = NULL;
    bool replace = false;

    if(_3g_param == NULL)
        return GXCORE_ERROR;

    if((temp_fp = fopen(_3G_RECORD_TMP, "w+")) == NULL)
        return GXCORE_ERROR;

    if((ret_fp = fopen(_3G_CONNNECT_RECORD, "r")) != NULL)
    {
        while((ret_fp != NULL) && (fgets(ret_buf, sizeof(ret_buf), ret_fp) != NULL))
        {
            p = ret_buf;
            q = strchr(p, seq_char);

            if((q == NULL) || (strncmp(_3g_param->dev_name, p, q-p) == 0)) //3g name
            {
                p = q + 1;
                if((*p != '\0') && (*p != '\n'))
                {
                    if((q = strchr(p, seq_char)) != NULL) //operator id
                    {
                        if(_3g_param->operator_id == atoi(p))
                        {
                            //same & replace
                            memset(ret_buf, 0, sizeof(ret_buf));
                            sprintf(ret_buf, "%s,%d,%s,%s,%s,%s\n", _3g_param->dev_name, _3g_param->operator_id,
                                    _3g_param->apn, _3g_param->phone, _3g_param->username, _3g_param->password);
                            replace = true;
                        }
                    }
                }
            }

            fwrite(ret_buf, 1, strlen(ret_buf), temp_fp);
            memset(ret_buf, 0, sizeof(ret_buf));
        }
        fclose(ret_fp);
    }

    if(replace == false)
    {
        memset(ret_buf, 0, sizeof(ret_buf));
        sprintf(ret_buf, "%s,%d,%s,%s,%s,%s\n", _3g_param->dev_name, _3g_param->operator_id,
                _3g_param->apn, _3g_param->phone, _3g_param->username, _3g_param->password);
        fwrite(ret_buf, 1, strlen(ret_buf), temp_fp);
    }
    fclose(temp_fp);

    if(GxCore_FileExists(_3G_RECORD_TMP) == GXCORE_FILE_EXIST)
    {
        return _mv_3g_cfg(_3G_RECORD_TMP, _3G_CONNNECT_RECORD);
    }

#if 0
    if(GxCore_FileExists(_3G_RECORD_TMP) == GXCORE_FILE_EXIST)
    {
        printf("[%s]%d, _3G_RECORD_TMP EXIST!!!\n", __func__, __LINE__);
    }
    else
    {
        printf("[%s]%d, _3G_RECORD_TMP UNEXIST!!!\n", __func__, __LINE__);
    }

    if(GxCore_FileExists(_3G_CONNNECT_RECORD) == GXCORE_FILE_EXIST)
    {
        printf("[%s]%d, _3G_CONNNECT_RECORD EXIST!!!\n", __func__, __LINE__);
    }
    else
    {
        printf("[%s]%d, _3G_CONNNECT_RECORD UNEXIST!!!\n", __func__, __LINE__);
    }
#endif
    return GXCORE_ERROR;
}

static void gxusb3g_ip(char *ip_addr)
{
	struct ifreq ifr;
	struct	in_addr sin_addr;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	strcpy(ip_addr, "0.0.0.0");
	if( sock != -1 )
	{
		memset(&sin_addr,0,sizeof(sin_addr));
		strncpy( ifr.ifr_name, "ppp0", sizeof(ifr.ifr_name));
		if( ioctl( sock, SIOCGIFADDR, &ifr ) == 0 )
			sin_addr = ((struct sockaddr_in *)&ifr.ifr_dstaddr)->sin_addr;
		strcpy(ip_addr, inet_ntoa(sin_addr));
		close(sock);
	}
}

static void gxusb3g_netmask(char *net_mask)
{
	struct ifreq ifr;
	struct	in_addr sin_addr;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	strcpy(net_mask, "0.0.0.0");
	if( sock != -1 )
	{
		memset(&sin_addr,0,sizeof(sin_addr));
		strncpy( ifr.ifr_name, "ppp0", sizeof(ifr.ifr_name));
		if( ioctl( sock, SIOCGIFNETMASK, &ifr ) == 0 )
			sin_addr = ((struct sockaddr_in *)&ifr.ifr_dstaddr)->sin_addr;
		strcpy(net_mask, inet_ntoa(sin_addr));
		close(sock);
	}
}

static void _usb3g_set_ip_cfg_file_data(char *dev_name)
{
	IpCfgFileData cfg_data;
	uint32_t  ip = 0;
	uint32_t  gateway = 0;
	uint32_t  dnsA = 0;
	uint32_t  dnsB = 0;
	struct in_addr sin;

	memset(&cfg_data, 0, sizeof(cfg_data));
	cfg_data.dev_mode = 1;
	cfg_data.ip_type = 1;
	gxusb3g_ip(cfg_data.ip_addr);
	gxusb3g_netmask(cfg_data.net_mask);

    usb3g_ifconfig(&ip, &gateway, &dnsA, &dnsB);
    sin.s_addr = gateway;
	strlcpy(cfg_data.gate_way, inet_ntoa(sin), sizeof(cfg_data.gate_way));
    sin.s_addr = dnsA;
	strlcpy(cfg_data.dns1, inet_ntoa(sin), sizeof(cfg_data.dns1));
    sin.s_addr = dnsB;
	strlcpy(cfg_data.dns2, inet_ntoa(sin), sizeof(cfg_data.dns2));

	_set_ip_cfg_file_data(dev_name, &cfg_data);
}

static IfFileRet _check_3g_result_file(IfState if_state, char *dev_name)
{
    IfFileRet file_ret = IF_FILE_NONE;
    IfState state_3g = IF_STATE_INVALID;
    static IfState old_state_3g = IF_STATE_INVALID;

    state_3g = usb3g_getstate();
    switch(state_3g)
    {
        case IF_STATE_START:
            file_ret = IF_FILE_START;
            break;

        case IF_STATE_CONNECTING:
            file_ret = IF_FILE_WAIT;
            break;

        case IF_STATE_CONNECTED:
            file_ret = IF_FILE_OK;
            if(old_state_3g!=state_3g)
            {
                _usb3g_set_ip_cfg_file_data(dev_name);
            }
            break;

        case IF_STATE_REJ:
            file_ret = IF_FILE_REJ;
            break;

        default:
            file_ret = IF_FILE_NONE;
            break;
    }

    old_state_3g = state_3g;
    return file_ret;
}
#endif
#endif

static void _net_thread_wired_start(void* arg)
{
    ubyte32 dev_name = {0};

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

    strcpy(dev_name, (char*)arg);
    if(strlen(dev_name) != 0)
    {
        char cmd_buf[CMD_LEN] = {0};

        sprintf(cmd_buf, "%s %s", WIRED_START_CMD, dev_name);
        //printf("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
        system(cmd_buf);
    }
}

static status_t _wired_dev_start(IfDevice *wired_dev)
{
    if((wired_dev == NULL) || (strlen(wired_dev->dev_name) == 0))
        return GXCORE_ERROR;

    return GxCore_ThreadCreate("wired_start", &(wired_dev->start_thread_handle), _net_thread_wired_start,
            (void*)(wired_dev->dev_name), 10 * 1024, GXOS_DEFAULT_PRIORITY);
}

static status_t _wired_dev_stop(IfDevice *wired_dev)
{
    char cmd_buf[CMD_LEN] = {0};

    if((wired_dev == NULL) || (strlen(wired_dev->dev_name) == 0))
        return GXCORE_ERROR;

    sprintf(cmd_buf, "%s %s", WIRED_STOP_CMD, wired_dev->dev_name);
    system(cmd_buf);
    //printf("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
    return GXCORE_SUCCESS;
}

static IfFileRet _wired_dev_connect_monitor(IfDevice *wired_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;

    if((wired_dev == NULL) || (strlen(wired_dev->dev_name) == 0))
        return IF_FILE_NONE;

    file_ret = _check_dev_result_file(wired_dev->dev_state, wired_dev->dev_name);
    return file_ret;
}

#if (WIFI_SUPPORT > 0)
static void _net_thread_wifi_start(void* arg)
{
    ubyte32 dev_name = {0};

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

    strcpy(dev_name, (char*)arg);
#ifdef LINUX_OS
    if(strlen(dev_name) != 0)
    {
        char cmd_buf[CMD_LEN] = {0};

        sprintf(cmd_buf, "%s %s", WIFI_START_CMD, dev_name);
        //printf("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
        system(cmd_buf);
    }
#else
    extern status_t _wifi_start(const char *wifi_dev, const char *essid, const char *mac, const char *psk);
    ApFileData ap_file_data;
    memset(&ap_file_data, 0, sizeof(ApFileData));
    if(_get_wifi_connect_file_data(dev_name, &ap_file_data) == GXCORE_ERROR) //no ap cfg
    {
        printf("[%s] NO AP CONFIG FILE!\n", __func__);
        return;
    }

    GxBus_ConfigSet(WIFI_STATUS_KEY, "start");
    _wifi_start(dev_name, ap_file_data.essid, ap_file_data.mac, ap_file_data.psk);
#endif
}

static status_t _wifi_dev_start(IfDevice *wifi_dev)
{
    ApFileData ap_file_data;
    if((wifi_dev == NULL) || (strlen(wifi_dev->dev_name) == 0))
        return GXCORE_ERROR;

    memset(&ap_file_data, 0, sizeof(ApFileData));
    if(_get_wifi_connect_file_data(wifi_dev->dev_name, &ap_file_data) == GXCORE_ERROR) //no ap cfg
        return GXCORE_ERROR;

    net_ifconfig(wifi_dev->dev_name, "up", NULL);

    return GxCore_ThreadCreate("wifi_start", &(wifi_dev->start_thread_handle), _net_thread_wifi_start,
                            (void*)(wifi_dev->dev_name), 30 * 1024, GXOS_DEFAULT_PRIORITY);
}

static status_t _wifi_dev_stop(IfDevice *wifi_dev)
{
#ifdef LINUX_OS
    char cmd_buf[CMD_LEN] = {0};

    if((wifi_dev == NULL) || (strlen(wifi_dev->dev_name) == 0))
        return GXCORE_ERROR;

    sprintf(cmd_buf, "%s %s", WIFI_STOP_CMD, wifi_dev->dev_name);
    system(cmd_buf);
    //printf("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
#else
    //TODO: STOP WIFI
    
    if((wifi_dev == NULL) || (strlen(wifi_dev->dev_name) == 0))
        return GXCORE_ERROR;

    net_ifconfig(wifi_dev->dev_name, "down", NULL);

#endif
    return GXCORE_SUCCESS;
}

static IfFileRet _wifi_dev_connect_monitor(IfDevice *wired_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;

    if((wired_dev == NULL) || (strlen(wired_dev->dev_name) == 0))
        return IF_FILE_NONE;

#ifdef LINUX_OS
    file_ret = _check_dev_result_file(wired_dev->dev_state, wired_dev->dev_name);
#endif

#ifdef ECOS_OS
    file_ret = _check_wifi_result_file(wired_dev->dev_state, wired_dev->dev_name);
#endif
    return file_ret;
}
#endif

#if  MOD_3G_SUPPORT
#ifdef LINUX_OS
static void _net_thread_3g_start(void* arg)
{
    ubyte32 dev_name = {0};

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

    strcpy(dev_name, (char*)arg);

    if(strlen(dev_name) != 0)
    {
        char cmd_buf[CMD_LEN] = {0};

        sprintf(cmd_buf, "%s %s", USB3G_START_CMD, dev_name);
        system(cmd_buf);
        //printf("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
    }
}

static status_t _3g_dev_start(IfDevice *_3g_dev)
{
    if((_3g_dev == NULL) ||(strlen(_3g_dev->dev_name) == 0))
        return GXCORE_ERROR;

    return GxCore_ThreadCreate("tg_start", &(_3g_dev->start_thread_handle), _net_thread_3g_start,
                            (void*)(_3g_dev->dev_name), 10 * 1024, GXOS_DEFAULT_PRIORITY);
}

static status_t _3g_dev_stop(IfDevice *_3g_dev)
{
	char cmd_buf[CMD_LEN] = {0};

	if((_3g_dev == NULL) || (strlen(_3g_dev->dev_name) == 0))
		return GXCORE_ERROR;

	sprintf(cmd_buf, "%s %s", USB3G_STOP_CMD, _3g_dev->dev_name);
	//printf("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
	system(cmd_buf);

	return GXCORE_SUCCESS;
}

static IfFileRet _3g_dev_connect_monitor(IfDevice *_3g_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;

    if((_3g_dev == NULL) || (strlen(_3g_dev->dev_name) == 0))
        return IF_FILE_NONE;

    file_ret = _check_dev_result_file(_3g_dev->dev_state, _3g_dev->dev_name);

    return file_ret;
}
#endif

#ifdef ECOS_OS
static void _net_thread_3g_start(void* arg)
{
    usb3g_usrparm param;
    GxMsgProperty_3G tg_param;
    int id;
    status_t status = GXCORE_ERROR;

    GxCore_ThreadDetach();
    if(arg == NULL)
        return;

    memset(&tg_param, 0, sizeof(GxMsgProperty_3G));
    strlcpy(tg_param.dev_name, (char*)arg, sizeof(tg_param.dev_name));
    GxBus_ConfigGetInt(_3G_OPERATOR_KEY, &id, _3G_OPERATOR_DEFAULT);
    tg_param.operator_id = id;
    status = _get_3g_cfg_file_data(&tg_param);
    if(status == GXCORE_SUCCESS)
    {
    printf("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
        memset(&param, 0, sizeof(usb3g_usrparm));
        strlcpy(param.APN, tg_param.apn, sizeof(param.APN));
        strlcpy(param.phone, tg_param.phone, sizeof(param.phone));
        strlcpy(param.username, tg_param.username, sizeof(param.username));
        strlcpy(param.password, tg_param.password, sizeof(param.password));
        usb3g_connect(&param);
    printf("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
    }
}

static status_t _3g_dev_start(IfDevice *_3g_dev)
{
    if((_3g_dev == NULL) ||(strlen(_3g_dev->dev_name) == 0))
        return GXCORE_ERROR;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(_3G_CONNNECT_RECORD))
        return GXCORE_ERROR;

    if((_3g_dev->dev_state == IF_STATE_IDLE) || (_3g_dev->dev_state == IF_STATE_REJ))
        GxCore_ThreadCreate("tg_start", &(_3g_dev->start_thread_handle), _net_thread_3g_start,
                (void*)(_3g_dev->dev_name), 10 * 1024, GXOS_DEFAULT_PRIORITY);
    //GxCore_ThreadJoin(_3g_dev->start_thread_handle);
    return GXCORE_SUCCESS;
}

static status_t _3g_dev_stop(IfDevice *_3g_dev)
{
    if((_3g_dev == NULL) || (strlen(_3g_dev->dev_name) == 0))
	  return GXCORE_ERROR;
    if(thiz_usb3g_hotplug_status == true)
    {
        printf("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
        usb3g_disconnect();
        printf("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
    }
    return GXCORE_SUCCESS;
}

static IfFileRet _3g_dev_connect_monitor(IfDevice *_3g_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;

    if((_3g_dev == NULL) || (strlen(_3g_dev->dev_name) == 0))
        return IF_FILE_NONE;

    file_ret = _check_3g_result_file(_3g_dev->dev_state, _3g_dev->dev_name);

    return file_ret;
}

int app_network_3g_stop(void)
{
    usb3g_disconnect();
    return GXCORE_SUCCESS;
}

#endif
#endif

static bool _if_dev_list_check(const char *dev_name)
{
    IfDevList *p = if_dev_list;
    if((dev_name == NULL) || (p == NULL))
        return false;

    GxCore_MutexLock(s_dev_list_mutex);
    while(p != NULL)
    {
        if(strcmp(p->net_dev.dev_name, dev_name) == 0)
        {
            GxCore_MutexUnlock(s_dev_list_mutex);
            return true;
        }
        p = p->next;
    }
    GxCore_MutexUnlock(s_dev_list_mutex);

    return false;
}

static status_t _if_dev_list_add(IfDevice *new_dev)
{
    IfDevList *p = if_dev_list;
    IfDevList *p_dev_node = NULL;

    if(new_dev == NULL)
    {
        return GXCORE_ERROR;
    }

    GxCore_MutexLock(s_dev_list_mutex);

    if(if_dev_list == NULL)
    {
        if_dev_list = (IfDevList*)GxCore_Malloc(sizeof(IfDevList));
        if(if_dev_list == NULL)
        {
            GxCore_MutexUnlock(s_dev_list_mutex);
            return GXCORE_ERROR;
        }
        memcpy(&(if_dev_list->net_dev), new_dev, sizeof(IfDevice));
        if_dev_list->next = NULL;
    }
    else
    {
        while(p->next != NULL)
        {
            p = p->next;
        }

        p_dev_node = (IfDevList*)GxCore_Malloc(sizeof(IfDevList));
        if(p_dev_node == NULL)
        {
            GxCore_MutexUnlock(s_dev_list_mutex);
            return GXCORE_ERROR;
        }
        memcpy(&(p_dev_node->net_dev), new_dev, sizeof(IfDevice));
        p_dev_node->next = NULL;
        p->next = p_dev_node;
    }

    GxCore_MutexUnlock(s_dev_list_mutex);

    return GXCORE_SUCCESS;
}

#if 0
static status_t _if_dev_list_del(const char *dev_name)
{
    IfDevList *p = if_dev_list;
    IfDevList *p_dev = NULL;

    if((dev_name == NULL) || (p == NULL))
    {
        return GXCORE_ERROR;
    }

    GxCore_MutexLock(s_dev_list_mutex);
    if(strcmp(if_dev_list->net_dev.dev_name, dev_name) == 0)
    {
        p_dev = if_dev_list;
        if_dev_list = if_dev_list->next;
        GxCore_Free(p_dev);
    }
    else
    {
        while(p->next != NULL)
        {
            if(strcmp(p->next->net_dev.dev_name, dev_name) == 0)
            {
                p_dev = p->next;
                p->next = p->next->next;
                GxCore_Free(p_dev);
                break;
            }
            p = p->next;
        }
    }

    GxCore_MutexUnlock(s_dev_list_mutex);
    return GXCORE_SUCCESS;
}
#endif

//plz GxCore_Free result if != NULL
static IfDevice* _if_dev_list_get(const char *dev_name)
{
    IfDevList *p = if_dev_list;
    IfDevice *p_ret = NULL;

    if((dev_name == NULL) || (p == NULL))
    {
        return NULL;
    }

    GxCore_MutexLock(s_dev_list_mutex);

    while(p != NULL)
    {
        if(strcmp(p->net_dev.dev_name, dev_name) == 0)
        {
            p_ret = (IfDevice*)GxCore_Malloc(sizeof(IfDevice));
            if(p_ret != NULL)
            {
                memcpy(p_ret, &(p->net_dev), sizeof(IfDevice));
            }
            break;
        }
        p = p->next;
    }

    GxCore_MutexUnlock(s_dev_list_mutex);

    return p_ret;
}

static status_t _if_dev_list_modify(IfDevice *new_dev)
{
    IfDevList *p = if_dev_list;
    status_t ret = GXCORE_ERROR;

    if((new_dev == NULL) || (p == NULL))
    {
        return GXCORE_ERROR;
    }

    GxCore_MutexLock(s_dev_list_mutex);
    while(p != NULL)
    {
        if(strcmp(p->net_dev.dev_name, new_dev->dev_name) == 0)
        {
            memcpy(&(p->net_dev), new_dev, sizeof(IfDevice));
            ret = GXCORE_SUCCESS;
            break;
        }
        p = p->next;
    }
    GxCore_MutexUnlock(s_dev_list_mutex);

    return ret;
}

static status_t _lock_if_dev(const char *dev_name)
{
    IfDevice *p_dev = NULL;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if((p_dev = _if_dev_list_get(dev_name)) == NULL)
        return GXCORE_ERROR;

    GxCore_MutexLock(p_dev->dev_mutex);
    GxCore_Free(p_dev);
    p_dev = NULL;
    return GXCORE_SUCCESS;
}

static status_t _unlock_if_dev(const char *dev_name)
{
    IfDevice *p_dev = NULL;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if((p_dev = _if_dev_list_get(dev_name)) == NULL)
        return GXCORE_ERROR;

    GxCore_MutexUnlock(p_dev->dev_mutex);
    GxCore_Free(p_dev);
    p_dev = NULL;
    return GXCORE_SUCCESS;
}

#ifdef LINUX_OS
static char *_get_ifname(char *    name,   /* Where to store the name */
          int   nsize,  /* Size of name buffer */
          char *    buf)    /* Current position in buffer */
{
  char *    end;

  /* Skip leading spaces */
  while(isspace(*buf))
    buf++;

//#ifndef IW_RESTRIC_ENUM
  /* Get name up to the last ':'. Aliases may contain ':' in them,
   * but the last one should be the separator */
  end = strrchr(buf, ':');
//#else
  /* Get name up to ": "
   * Note : we compare to ": " to make sure to process aliased interfaces
   * properly. Doesn't work on /proc/net/dev, because it doesn't guarantee
   * a ' ' after the ':'*/
  //end = strstr(buf, ": ");
//#endif

  /* Not found ??? To big ??? */
  if((end == NULL) || (((end - buf) + 1) > nsize))
    return(NULL);

  /* Copy */
  memcpy(name, buf, (end - buf));
  name[end - buf] = '\0';

  /* Return value currently unused, just make sure it's non-NULL */
  return(end);
}
#endif

static int _get_prev_init_dev(ubyte64 *net_dev)
{
#if 0
    FILE *read_fp;
    char cmdline[60] = {0};
    ubyte64 dev_buf = {0};
    int nfs_dev_num = 0;
    char *p = NULL;

    if(net_dev == NULL)
    {
        return 0;
    }

    strcpy(cmdline, "route -n | grep -w U | awk '{print $8}'");
    read_fp = popen(cmdline, "r");
    if (read_fp != NULL)
    {
        while(fgets(dev_buf, sizeof(dev_buf), read_fp))
        {
            p = strchr(dev_buf, '\n');
            if(p != NULL)
                *p = '\0';
            strcpy(net_dev[nfs_dev_num], dev_buf);
            //printf("~~~~~~~~~[%s]%d nfs_dev = %s~~~~~~~~~\n", __func__, __LINE__, net_dev[nfs_dev_num]);
            nfs_dev_num++;
        }
    }

    pclose(read_fp);

    //printf("~~~~~~~~~[%s]%d nfs_dev_num = %d ~~~~~~~~~\n", __func__, __LINE__, nfs_dev_num);
    return nfs_dev_num;
#else
    return 0;
#endif
}

#if 0
static void _delete_prev_init_dev(const char *dev_name)
{
    ubyte64 *temp_dev = NULL;
    int i = 0;

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (s_prev_init_dev.dev_num == 0))
        return;

    temp_dev = GxCore_Calloc(s_prev_init_dev.dev_num, sizeof(ubyte64));
    if(temp_dev != NULL)
    {
        memcpy(temp_dev, s_prev_init_dev.net_dev, s_prev_init_dev.dev_num * sizeof(ubyte64));
        memset(s_prev_init_dev.net_dev, 0, s_prev_init_dev.dev_num * sizeof(ubyte64));
        s_prev_init_dev.dev_num = 0;

        for(i = 0; i < s_prev_init_dev.dev_num; i++)
        {
            if(strcmp(temp_dev[i], dev_name) != 0)
            {
                strcpy(s_prev_init_dev.net_dev[s_prev_init_dev.dev_num++], temp_dev[i]);
            }
        }
    }

    return;
}
#endif

static bool _check_prev_init_dev(const char *dev_name)
{
    int i = 0;
    bool ret = false;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
        return false;

    for(i = 0; i < s_prev_init_dev.dev_num; i++)
    {
        if(strcmp(dev_name, s_prev_init_dev.net_dev[i]) == 0)
        {
            ret = true;
            break;
        }
    }

    return ret;
}

static IfType _if_dev_check_type(const char *dev_name)
{
    IfType ret = IF_TYPE_UNKOWN;
    if(dev_name == NULL)
        return ret;

    if(strncmp(dev_name, "eth", 3) == 0)
    {
        ret = IF_TYPE_WIRED;
    }
#if (WIFI_SUPPORT > 0)
    else if(strncmp(dev_name, "ra", 2) == 0)
    {
        ret = IF_TYPE_WIFI;
    }
    else if(strncmp(dev_name, "wlan", 4) == 0)
    {
        ret = IF_TYPE_WIFI;
    }
#endif
#if (MOD_3G_SUPPORT > 0)
	else if(strncmp(dev_name, USB3G_DEV_HEAD, strlen(USB3G_DEV_HEAD)) == 0)
    {
        ret = IF_TYPE_3G;
    }
#endif

    return ret;
}

static bool _check_dev_user_flag(const char *dev_name)
{
    IpCfgFileData cfg_data;
    bool ret = false;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
    {
        return false;
    }

    memset(&cfg_data, 0, sizeof(IpCfgFileData));
    if(_get_ip_cfg_file_data(dev_name, &cfg_data) == GXCORE_SUCCESS)
    {
        (cfg_data.dev_mode == 0) ? (ret = false) : (ret = true);
    }

    return ret;
}

static bool _check_dev_main_flag(const char *dev_name)
{
    if(dev_name == NULL)
    {
        return false;
    }

    if(strcmp(dev_name, NET_IF_DEV_ETH0) == 0)
        return true;

    return false;
}

#ifdef ECOS_OS
#if (WIFI_SUPPORT > 0)
static void _network_check_wifi_dev(void)
{
    ubyte64 name = {0};
    if(s_wifi_init_status == -1)
        return;
    //TODO:Get Dev List From ECOS
    snprintf(name, sizeof(ubyte64), "%s", "ra0");
    if(_if_dev_list_check(name) == false)
    {
        IfType dev_type = _if_dev_check_type(name);
        if(dev_type != IF_TYPE_UNKOWN)
        {
            IfDevice dev;
            ubyte32 mac = {0};
            char gateway[20] = {0};

            memset(&dev, 0, sizeof(IfDevice));
            strncpy(dev.dev_name, name, IFNAMSIZ);
            dev.dev_type = dev_type;

            if(dev_type == IF_TYPE_WIRED)
                dev.dev_priority = WIRED_PRIORITY;
#if (WIFI_SUPPORT > 0)
            if(dev_type == IF_TYPE_WIFI)
                dev.dev_priority = WIFI_PRIORITY;
#endif
            //if(dev_type == IF_TYPE_3G)
            //    dev.dev_priority = TG_PRIORITY;

            dev.user_flag = _check_dev_user_flag(name);
            dev.main_flag = _check_dev_main_flag(name);
            dev.dev_ops = &s_net_ops_set[dev_type];
            dev.start_thread_handle = -1;
            dev.scan_thread_handle = -1;
            dev.cur_flag = false;
            dev.dev_state = IF_STATE_INVALID;

            if(app_get_gateway(name, gateway) == GXCORE_SUCCESS)
            {
                if(s_global_net_state != IF_STATE_CONNECTED)
                {
                    dev.dev_state = IF_STATE_CONNECTED;
                    s_global_net_state = IF_STATE_CONNECTED;
                    dev.dev_priority = MIN_PRIORITY;
                    s_network_dev_priority = MIN_PRIORITY;
                    dev.cur_flag = true;
                }
            }
            else
            {
                if(_check_prev_init_dev(dev.dev_name) == true)
                    dev.dev_state = IF_STATE_CONNECTING;
            }

            GxCore_MutexCreate(&dev.dev_mutex);

            if(_network_get_mac_cfg(name, mac) == GXCORE_SUCCESS)
            {
                //printf("~~~~~~[%s]%d, %s mac: %s !!!!!\n", __func__, __LINE__, name, mac);
                _network_set_mac(name, mac);
            }

            _if_dev_list_add(&dev);
        }
    }
    else
    {
        bool main_flag = _check_dev_main_flag(name);
        IfDevice *temp_dev = _if_dev_list_get(name);

        if(temp_dev->main_flag != main_flag)
        {
            temp_dev->main_flag = main_flag;
            _if_dev_list_modify(temp_dev);
        }
        GxCore_Free(temp_dev);
        temp_dev = NULL;
    }
}
#endif
#endif

#ifdef LINUX_OS
static bool _network_usb3g_lookup(char* dev_name, int  nsize)
{
#define PROC_3G_DEV	  "/tmp/ethernet/monitor/usb3gDev"

	char	  buff[1024] = {0};
	FILE *	  fh = NULL;
	char cmd_buf[CMD_LEN] = {0};

	sprintf(cmd_buf, "%s %s", USB3G_LOOKUP_CMD, PROC_3G_DEV);
	system(cmd_buf);
	fh = fopen(PROC_3G_DEV, "r");
	if(fh != NULL)
	{
		/* Read each device line */
		while(fgets(buff, sizeof(buff), fh))
		{
			char *s = NULL;

			/* Skip empty or almost empty lines. It seems that in some
			 * * cases fgets return a line with only a newline. */
			if((buff[0] == '\0') || (buff[1] == '\0'))
				continue;

			//s = _get_ifname(name, sizeof(name), buff);
			s = strstr(buff,USB3G_DEV_HEAD);
			if(!s)
			{
				fclose(fh);
				return false;
			}
			else
			{
				if (dev_name != NULL)
				{
					char *	  end=NULL;
					end = strrchr(buff, '#');
					int len=0;

					if (end == NULL)
					{
						len = strlen(USB3G_DEV_HEAD);
					}
					else
					{
						len = ((end - s) + 1) >= nsize ? nsize-1:(end - s);
					}

					memcpy(dev_name, s, len);
					dev_name[len] = '\0';
				}
				fclose(fh);
				return true;
			}
		}
		fclose(fh);
	}
	return false;
}
#else //ecos os
#if (MOD_3G_SUPPORT > 0)
static bool _network_usb3g_lookup(char *dev_name, int nsize)
{
    bool ret = false;
    if(thiz_usb3g_hotplug_status == true)
    {
        if(dev_name != NULL)
        {
            strlcpy(dev_name, USB3G_DEV_HEAD, nsize);
        }
        ret = true;
    }
    return ret;
}
#endif
#endif

#if (MOD_3G_SUPPORT > 0)
static void _network_check_3g_dev(void)
{
	IfDevice dev;

	memset(&dev, 0, sizeof(IfDevice));
	if (_network_usb3g_lookup(dev.dev_name, sizeof(dev.dev_name)))
	{
		if(_if_dev_list_check(dev.dev_name) == false)
		{
			dev.dev_state = IF_STATE_INVALID;
			dev.dev_type = IF_TYPE_3G;
            dev.dev_priority = TG_PRIORITY;
			dev.user_flag = _check_dev_user_flag(dev.dev_name);
			dev.main_flag = false;//_check_dev_main_flag(name);
			dev.cur_flag = false;
			dev.dev_ops = &s_net_ops_set[IF_TYPE_3G];
			dev.start_thread_handle = -1;
			dev.scan_thread_handle = -1;
			GxCore_MutexCreate(&dev.dev_mutex);

			_if_dev_list_add(&dev);
		}
		else
		{
			//do nothing
		}
	}
}
#endif

void _network_check_all_dev(void)
{
#ifdef LINUS_OS
    char      buff[128] = {0};
    FILE *    fh = NULL;

    fh = fopen(PROC_NET_DEV, "r");

    if(fh != NULL)
    {
        fgets(buff, sizeof(buff), fh);
        fgets(buff, sizeof(buff), fh);

        /* Read each device line */
        while(fgets(buff, sizeof(buff), fh))
        {
            ubyte64  name = {0};
            char *s = NULL;

            /* Skip empty or almost empty lines. It seems that in some
             * * cases fgets return a line with only a newline. */

            if((buff[0] == '\0') || (buff[1] == '\0'))
                continue;

            s = _get_ifname(name, sizeof(name), buff);
            if(!s)
            {
                //printf("Cannot parse PROC_NET_DEV \n");
            }
            else
            {
                if(strcmp(name, NET_IF_DEV_LOOP) == 0)
                {
                    continue;
                }

                if(_if_dev_list_check(name) == false)
                {
                    IfType dev_type = _if_dev_check_type(name);
                    if(dev_type != IF_TYPE_UNKOWN)
                    {
                        IfDevice dev;
                        ubyte32 mac = {0};
                        char gateway[20] = {0};

                        memset(&dev, 0, sizeof(IfDevice));
                        //strncpy(dev.dev_name, name, IFNAMSIZ);
                        strncpy(dev.dev_name, name, 64);
                        dev.dev_type = dev_type;

                        if(dev_type == IF_TYPE_WIRED)
                            dev.dev_priority = WIRED_PRIORITY;
                        if(dev_type == IF_TYPE_WIFI)
                            dev.dev_priority = WIFI_PRIORITY;
                        //if(dev_type == IF_TYPE_3G)
                        //    dev.dev_priority = TG_PRIORITY;

                        dev.user_flag = _check_dev_user_flag(name);
                        dev.main_flag = _check_dev_main_flag(name);
                        dev.dev_ops = &s_net_ops_set[dev_type];
                        dev.start_thread_handle = -1;
                        dev.scan_thread_handle = -1;
                        dev.cur_flag = false;
                        dev.dev_state = IF_STATE_INVALID;

                        if(app_get_gateway(name, gateway) == GXCORE_SUCCESS)
                        {
                            if(s_global_net_state != IF_STATE_CONNECTED)
                            {
                                dev.dev_state = IF_STATE_CONNECTED;
                                s_global_net_state = IF_STATE_CONNECTED;
                                dev.dev_priority = MIN_PRIORITY;
                                s_network_dev_priority = MIN_PRIORITY;
                                dev.cur_flag = true;
                            }
                        }
                        else
                        {
                            if(_check_prev_init_dev(dev.dev_name) == true)
                                dev.dev_state = IF_STATE_CONNECTING;
                        }

                        GxCore_MutexCreate(&dev.dev_mutex);

                        if(_network_get_mac_cfg(name, mac) == GXCORE_SUCCESS)
                        {
                            //printf("~~~~~~[%s]%d, %s mac: %s !!!!!\n", __func__, __LINE__, name, mac);
                            _network_set_mac(name, mac);
                        }

                        _if_dev_list_add(&dev);
                    }
                }
                else
                {
                    bool main_flag = _check_dev_main_flag(name);
                    IfDevice *temp_dev = _if_dev_list_get(name);

                    if(temp_dev->main_flag != main_flag)
                    {
                        temp_dev->main_flag = main_flag;
                        _if_dev_list_modify(temp_dev);
                    }
                    GxCore_Free(temp_dev);
                    temp_dev = NULL;
                }
            }
        }
        fclose(fh);
    }

#else //ECOS_OS
#if (WIFI_SUPPORT > 0)
    _network_check_wifi_dev();
#endif
#endif

#if (MOD_3G_SUPPORT > 0)
    //check 3g dev
	_network_check_3g_dev();
#endif
}

#if 0
static status_t _network_set_dev_state(const char *dev_name, IfState state)
{
    IfDevice *temp_dev = NULL;
    status_t ret = GXCORE_ERROR;

    if((dev_name == NULL))
    {
        return GXCORE_ERROR;
    }

    if((temp_dev = _if_dev_list_get(dev_name)) != NULL)
    {
        temp_dev->dev_state = state;
        ret = _if_dev_list_modify(temp_dev);
        GxCore_Free(temp_dev);
        temp_dev = NULL;
    }
    return ret;
}
#endif

#if 0
static status_t _network_get_main_dev(IfDevice *ret)
{
    IfDevice *temp = _if_dev_list_get(NET_IF_DEV_ETH0);

    if(temp != NULL)
    {
        memcpy(ret, temp, sizeof(IfDevice));
        GxCore_Free(temp);
        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}
#endif

static status_t app_network_dev_list_get(GxMsgProperty_NetDevList *dev_list, unsigned char state_flag)
{
    unsigned int num = 0;

    IfDevList *p_dev_list = NULL;
    IfDevice *p_dev = NULL;

    GxCore_MutexLock(s_dev_list_mutex);

    p_dev_list = if_dev_list;
    while(p_dev_list != NULL)
    {
        p_dev = &(p_dev_list->net_dev);
        if((p_dev->dev_state & state_flag) != 0)
        {
            strcpy(dev_list->dev_name[num++], p_dev->dev_name);
        }
        p_dev_list = p_dev_list->next;
    }

    GxCore_MutexUnlock(s_dev_list_mutex);

    dev_list->dev_num = num;

    return GXCORE_SUCCESS;
}

static status_t app_network_dev_get_state(GxMsgProperty_NetDevState *dev_state)
{
    IfDevice *p_dev = NULL;

    if((dev_state == NULL) || (strlen(dev_state->dev_name) == 0))
    {
        dev_state->dev_state = IF_STATE_INVALID;
        return GXCORE_ERROR;
    }

    if((p_dev = _if_dev_list_get(dev_state->dev_name)) == NULL)
        return GXCORE_ERROR;

    dev_state->dev_state = p_dev->dev_state;

    GxCore_Free(p_dev);
    p_dev = NULL;

    return GXCORE_SUCCESS;
}

static status_t app_network_state_report(const char *dev_name, IfState dev_state)
{
    GxMessage *new_msg = NULL;
    GxMsgProperty_NetDevState *state = NULL;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
        return GXCORE_ERROR;

    new_msg = GxBus_MessageNew(GXMSG_NETWORK_STATE_REPORT);
    state= (GxMsgProperty_NetDevState*)GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_NetDevState);
    if(state != NULL)
    {
        strcpy(state->dev_name, dev_name);
        state->dev_state = dev_state;
        GxBus_MessageSend(new_msg);

        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

static status_t app_network_plug_in_report(const char *dev_name)
{
    GxMessage *new_msg = NULL;
    ubyte64 *dev = NULL;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
        return GXCORE_ERROR;

    new_msg = GxBus_MessageNew(GXMSG_NETWORK_PLUG_IN);
    dev = (ubyte64*)GxBus_GetMsgPropertyPtr(new_msg, ubyte64);
    if(dev != NULL)
    {
        strcpy(*dev, dev_name);
        GxBus_MessageSend(new_msg);

        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

static status_t app_network_plug_out_report(const char *dev_name)
{
    GxMessage *new_msg = NULL;
    ubyte64 *dev = NULL;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
        return GXCORE_ERROR;

    new_msg = GxBus_MessageNew(GXMSG_NETWORK_PLUG_OUT);
    dev = (ubyte64*)GxBus_GetMsgPropertyPtr(new_msg, ubyte64);
    if(dev != NULL)
    {
        strcpy(*dev, dev_name);
        GxBus_MessageSend(new_msg);

        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}


static status_t app_network_dev_get_type(GxMsgProperty_NetDevType *dev_type)
{
    IfDevice *p_dev = NULL;

    if((dev_type == NULL) || (strlen(dev_type->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if((p_dev = _if_dev_list_get(dev_type->dev_name)) == NULL)
        return GXCORE_ERROR;

    dev_type->dev_type = p_dev->dev_type;

    GxCore_Free(p_dev);
    p_dev = NULL;

    return GXCORE_SUCCESS;
}

static status_t app_network_get_cur_dev(ubyte64 *dev_name)
{
    IfDevList *p = if_dev_list;
    if((dev_name == NULL) || (p == NULL))
        return GXCORE_ERROR;

    GxCore_MutexLock(s_dev_list_mutex);
    while(p != NULL)
    {
        if(p->net_dev.cur_flag == true)
        {
            strcpy(*dev_name, p->net_dev.dev_name);
            GxCore_MutexUnlock(s_dev_list_mutex);
            return GXCORE_SUCCESS;
        }
        p = p->next;
    }
    GxCore_MutexUnlock(s_dev_list_mutex);

    return GXCORE_ERROR;
}

static status_t app_network_dev_set_mac(GxMsgProperty_NetDevMac *dev_mac)
{
    IfDevice *p_dev = NULL;

    if((dev_mac == NULL) || (strlen(dev_mac->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(dev_mac->dev_name) == true)
        return GXCORE_ERROR;

    _lock_if_dev(dev_mac->dev_name);

    if((p_dev = _if_dev_list_get(dev_mac->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_mac->dev_name);
        return GXCORE_ERROR;
    }

    if((p_dev->dev_state == IF_STATE_START)
        || (p_dev->dev_state == IF_STATE_CONNECTING)
        || (p_dev->dev_state == IF_STATE_CONNECTED)
        || (p_dev->dev_state == IF_STATE_REJ))
    {
        p_dev->dev_state = IF_STATE_IDLE;
        printf("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);

        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                s_network_dev_priority = MAX_PRIORITY;
                printf("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }

        _if_dev_list_modify(p_dev);
    }

    _network_set_mac(dev_mac->dev_name, dev_mac->dev_mac);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(dev_mac->dev_name);
    return GXCORE_SUCCESS;
}

static status_t app_network_dev_get_mac(GxMsgProperty_NetDevMac *dev_mac)
{
    IfDevice *p_dev = NULL;
    IpCfgFileData cfg_data;

    if((dev_mac == NULL) || (strlen(dev_mac->dev_name) == 0))
        return GXCORE_ERROR;

    _lock_if_dev(dev_mac->dev_name);
    if((p_dev = _if_dev_list_get(dev_mac->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_mac->dev_name);
        return GXCORE_ERROR;
    }

    memset(&cfg_data, 0, sizeof(IpCfgFileData));
    if(_get_ip_cfg_file_data(dev_mac->dev_name, &cfg_data) == GXCORE_ERROR)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_mac->dev_name);
        return GXCORE_ERROR;
    }

    strcpy(dev_mac->dev_mac, cfg_data.mac);

    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(dev_mac->dev_name);
    return GXCORE_SUCCESS;
}

static status_t app_network_dev_set_ipcfg(GxMsgProperty_NetDevIpcfg *dev_config)
{
    IfDevice *p_dev = NULL;
    unsigned char mode = 0;
    IpCfgFileData cfg_data;

    if((dev_config == NULL) || (strlen(dev_config->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(dev_config->dev_name) == true)
        return GXCORE_ERROR;

    _lock_if_dev(dev_config->dev_name);

    if((p_dev = _if_dev_list_get(dev_config->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_config->dev_name);
        return GXCORE_ERROR;
    }

    if((p_dev->dev_state == IF_STATE_START)
        || (p_dev->dev_state == IF_STATE_CONNECTING)
        || (p_dev->dev_state == IF_STATE_CONNECTED)
        || (p_dev->dev_state == IF_STATE_REJ))
    {
        p_dev->dev_state = IF_STATE_IDLE;
        printf("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);

        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                s_network_dev_priority = MAX_PRIORITY;
                printf("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }

        _if_dev_list_modify(p_dev);
    }

    (p_dev->user_flag == false) ? (mode = 0) : (mode = 1);

    cfg_data.dev_mode = mode;
    cfg_data.ip_type = dev_config->ip_type;

    strcpy(cfg_data.ip_addr, dev_config->ip_addr);
    strcpy(cfg_data.net_mask, dev_config->net_mask);
    strcpy(cfg_data.gate_way, dev_config->gate_way);
    strcpy(cfg_data.dns1, dev_config->dns1);
    strcpy(cfg_data.dns2, dev_config->dns2);

    _set_ip_cfg_file_data(dev_config->dev_name, &cfg_data);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(dev_config->dev_name);

    return GXCORE_SUCCESS;
}

static status_t app_network_dev_get_ipcfg(GxMsgProperty_NetDevIpcfg *dev_config)
{
    IfDevice *p_dev = NULL;
    IpCfgFileData cfg_data;

    if((dev_config == NULL) || (strlen(dev_config->dev_name) == 0))
        return GXCORE_ERROR;

    _lock_if_dev(dev_config->dev_name);

    if((p_dev = _if_dev_list_get(dev_config->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_config->dev_name);
        return GXCORE_ERROR;
    }

    memset(&cfg_data, 0, sizeof(IpCfgFileData));
    if(_get_ip_cfg_file_data(dev_config->dev_name, &cfg_data) == GXCORE_ERROR)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_config->dev_name);
        return GXCORE_ERROR;
    }

    dev_config->ip_type = cfg_data.ip_type;
    strcpy(dev_config->ip_addr, cfg_data.ip_addr);
    strcpy(dev_config->net_mask, cfg_data.net_mask);
    strcpy(dev_config->gate_way, cfg_data.gate_way);
    strcpy(dev_config->dns1, cfg_data.dns1);
    strcpy(dev_config->dns2, cfg_data.dns2);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(dev_config->dev_name);

    return GXCORE_SUCCESS;
}

static status_t app_network_dev_set_mode(GxMsgProperty_NetDevMode *dev_mode)
{
    IfDevice *p_dev = NULL;
    IpCfgFileData cfg_data;

    if((dev_mode == NULL) || (strlen(dev_mode->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(dev_mode->dev_name) == true)
        return GXCORE_ERROR;

    _lock_if_dev(dev_mode->dev_name);

    if((p_dev = _if_dev_list_get(dev_mode->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_mode->dev_name);
        return GXCORE_ERROR;
    }

    memset(&cfg_data, 0, sizeof(IpCfgFileData));
    if(_get_ip_cfg_file_data(dev_mode->dev_name, &cfg_data) == GXCORE_ERROR)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_mode->dev_name);
        return GXCORE_ERROR;
    }

    (dev_mode->mode == 0) ? (p_dev->user_flag = false) : (p_dev->user_flag = true);
    _if_dev_list_modify(p_dev);

    cfg_data.dev_mode = dev_mode->mode;
    _set_ip_cfg_file_data(dev_mode->dev_name, &cfg_data);

    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(dev_mode->dev_name);

    return GXCORE_SUCCESS;
}

static status_t app_network_dev_get_mode(GxMsgProperty_NetDevMode *dev_mode)
{
    IfDevice *p_dev = NULL;

    if((dev_mode == NULL) || (strlen(dev_mode->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    _lock_if_dev(dev_mode->dev_name);
    if((p_dev = _if_dev_list_get(dev_mode->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_mode->dev_name);
        return GXCORE_ERROR;
    }

    (p_dev->user_flag == false) ? (dev_mode->mode = 0) : (dev_mode->mode = 1);
    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(dev_mode->dev_name);

    return GXCORE_SUCCESS;
}

#if (WIFI_SUPPORT > 0)

static status_t app_wifi_aplist_report(const char *wifi_dev, ApInfo *ap_info, unsigned int ap_num)
{
    GxMessage *new_msg = NULL;
    GxMsgProperty_ApList *ap_list = NULL;

    if((wifi_dev == NULL) || (strlen(wifi_dev) == 0))
        return GXCORE_ERROR;

    if((ap_num > 0) && (ap_info == NULL))
        return GXCORE_ERROR;

    if((new_msg = GxBus_MessageNew(GXMSG_WIFI_SCAN_AP_OVER)) != NULL)
    {
        ap_list= (GxMsgProperty_ApList*)GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_ApList);
        if(ap_list != NULL)
        {
            strcpy(ap_list->wifi_dev, wifi_dev);

            (ap_num <= MAX_AP_NUM) ? (ap_list->ap_num = ap_num) : (ap_list->ap_num = MAX_AP_NUM);
            ap_list->ap_info = ap_info;
            GxBus_MessageSend(new_msg);

            return GXCORE_SUCCESS;
        }
    }
    return GXCORE_ERROR;
}

static void _wifi_thread_scan_ap(void* arg)
{
    ubyte64 wifi_dev = {0};
    IfDevice *p_dev = NULL;
    unsigned int ap_num = 0;
    unsigned int i = 0;
    static unsigned int nTryCount;
    nTryCount = 5;

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

    strlcpy(wifi_dev, *(ubyte64*)arg, sizeof(ubyte64));
    memset(s_ap_list, 0, sizeof(ApInfo) * MAX_AP_NUM);
    if(strlen(wifi_dev) != 0)
    {
#ifdef LINUX_OS
        char cmd_buf[CMD_LEN] = {0};
        char result_path[PATH_LEN] = {0};
#endif

        if((p_dev = _if_dev_list_get(wifi_dev)) == NULL)
        {
            app_wifi_aplist_report(wifi_dev, s_ap_list, ap_num);
            return;
        }

        while(p_dev->dev_state == IF_STATE_START)
        {
            GxCore_ThreadDelay(100);

            GxCore_Free(p_dev);
            p_dev = NULL;
            if((p_dev = _if_dev_list_get(wifi_dev)) == NULL)
            {
                app_wifi_aplist_report(wifi_dev, s_ap_list, ap_num);
                return;
            }
        }

        _lock_if_dev(wifi_dev);
        GxCore_Free(p_dev);
        p_dev = NULL;
        if((p_dev = _if_dev_list_get(wifi_dev)) == NULL)
        {
            _unlock_if_dev(wifi_dev);
            app_wifi_aplist_report(wifi_dev, s_ap_list, ap_num);
            return;
        }

        if(p_dev->dev_state == IF_STATE_INVALID) //plug out
        {
            GxCore_Free(p_dev);
            p_dev = NULL;
            _unlock_if_dev(wifi_dev);
            return;
        }

        p_dev->scanning_flag = true;
        _if_dev_list_modify(p_dev);
        _unlock_if_dev(wifi_dev);

#ifdef LINUX_OS
        sprintf(result_path, "%s%s%s", NETWORK_MANUAL_DIR, wifi_dev, "_aplist");
        sprintf(cmd_buf, "%s %s %s", WIFI_SCAN_CMD, result_path, wifi_dev);
        system(cmd_buf);

        ap_num = _get_ap_list_file_data(result_path, s_ap_list, MAX_AP_NUM);

        memset(cmd_buf, 0, CMD_LEN);
        sprintf(cmd_buf, "%s %s", RM_FILE_CMD, result_path);
        system(cmd_buf);

#else
        {
            extern unsigned int app_get_aplist(ApInfo *ap_data, unsigned int max_ap_num);
            while(nTryCount > 0 && ap_num == 0)
            {
                ap_num = app_get_aplist(s_ap_list, MAX_AP_NUM);
                if(ap_num > 0)
                    break;
                GxCore_ThreadDelay(300);
                nTryCount--;
                printf("\033[31mTry to get ap list again!!\n\033[0m");
            }
            qsort(s_ap_list, ap_num, sizeof(ApInfo), _ap_quality_cmp);
        }
#endif

        for(i = 0; i < ap_num; i++)
        {
            if(s_ap_list[i].encryption == 0) //open ap
                continue;
            _get_ap_psk(s_ap_list[i].ap_essid, s_ap_list[i].ap_mac, s_ap_list[i].ap_psk);
        }

        _lock_if_dev(wifi_dev);
        GxCore_Free(p_dev);
        p_dev = NULL;
        if((p_dev = _if_dev_list_get(wifi_dev)) == NULL)
        {
            _unlock_if_dev(wifi_dev);
            app_wifi_aplist_report(wifi_dev, s_ap_list, ap_num);
            return;
        }
        if(p_dev->dev_state == IF_STATE_INVALID) //plug out
        {
            GxCore_Free(p_dev);
            p_dev = NULL;
            _unlock_if_dev(wifi_dev);
            return;
        }

        p_dev->scanning_flag = false;
        _if_dev_list_modify(p_dev);
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(wifi_dev);
    }

    app_wifi_aplist_report(wifi_dev, s_ap_list, ap_num);
}

static status_t app_wifi_scan_ap(const char *dev_name)
{
    IfDevice *p_dev = NULL;
    status_t ret = GXCORE_ERROR;
    static ubyte64 _dev_name = {0};

    if((dev_name == NULL) || (strlen(dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    _lock_if_dev(dev_name);
    if((p_dev = _if_dev_list_get(dev_name)) == NULL)
    {
        _unlock_if_dev(dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_state == IF_STATE_INVALID)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->scanning_flag == true)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_name);
        return GXCORE_ERROR;
    }

    strlcpy(_dev_name, p_dev->dev_name, sizeof(ubyte64));
    //printf("~~~~~~[%s]%d!!!!!\n", __func__, __LINE__);
    ret = GxCore_ThreadCreate("wifi_scan", &(p_dev->scan_thread_handle), _wifi_thread_scan_ap,
            (void*)(&_dev_name), 30 * 1024, GXOS_DEFAULT_PRIORITY);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(dev_name);
    return ret;
}

static status_t app_wifi_get_cur_ap(GxMsgProperty_CurAp *cur_ap)
{
    //printf("-----[%s]%d-----\n", __func__, __LINE__);
    IfDevice *p_dev = NULL;
    ApFileData ap_file_data;

    if((cur_ap == NULL) || (strlen(cur_ap->wifi_dev) == 0))
        return GXCORE_ERROR;

    _lock_if_dev(cur_ap->wifi_dev);
    if((p_dev = _if_dev_list_get(cur_ap->wifi_dev)) == NULL)
    {
        _unlock_if_dev(cur_ap->wifi_dev);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_type != IF_TYPE_WIFI)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(cur_ap->wifi_dev);
        return GXCORE_ERROR;
    }

    memset(&ap_file_data, 0, sizeof(ApFileData));
    if(_get_wifi_connect_file_data(cur_ap->wifi_dev, &ap_file_data) == GXCORE_SUCCESS)
    {
        strcpy(cur_ap->ap_mac, ap_file_data.mac);
        strcpy(cur_ap->ap_essid, ap_file_data.essid);
        strcpy(cur_ap->ap_psk, ap_file_data.psk);
        //printf("~~~~~~[%s]%d, file: %s, %s, cur_ap: %s, %s!!!!!\n", __func__, __LINE__,
        //ap_file_data.essid, ap_file_data.mac, cur_ap->ap_essid, cur_ap->ap_mac);
    }

    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(cur_ap->wifi_dev);

    return GXCORE_SUCCESS;
}

static status_t app_wifi_set_cur_ap(GxMsgProperty_CurAp *cur_ap)
{
    IfDevice *p_dev = NULL;
    ApFileData ap_data;

    if((cur_ap == NULL) || (strlen(cur_ap->wifi_dev) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(cur_ap->wifi_dev) == true)
        return GXCORE_ERROR;

    _lock_if_dev(cur_ap->wifi_dev);
    if((p_dev = _if_dev_list_get(cur_ap->wifi_dev)) == NULL)
    {
        _unlock_if_dev(cur_ap->wifi_dev);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_type != IF_TYPE_WIFI)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(cur_ap->wifi_dev);
        return GXCORE_ERROR;
    }

    if((p_dev->dev_state == IF_STATE_START)
        || (p_dev->dev_state == IF_STATE_CONNECTING)
        || (p_dev->dev_state == IF_STATE_CONNECTED)
        || (p_dev->dev_state == IF_STATE_REJ))
    {
        p_dev->dev_state = IF_STATE_IDLE;
        printf("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);

        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                s_network_dev_priority = MAX_PRIORITY;
                printf("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }
        _if_dev_list_modify(p_dev);
    }

    memset(&ap_data, 0, sizeof(ApFileData));
    strcpy(ap_data.essid, cur_ap->ap_essid);
    strcpy(ap_data.mac, cur_ap->ap_mac);
    strcpy(ap_data.psk, cur_ap->ap_psk);
    _set_wifi_connect_file_data(p_dev->dev_name, &ap_data);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(cur_ap->wifi_dev);

    return GXCORE_SUCCESS;
}

static status_t app_wifi_del_cur_ap(const char *wifi_dev)
{
#ifdef LINUX_OS
    char cmd_buf[CMD_LEN] = {0};
#endif
    IfDevice *p_dev = NULL;

    if((wifi_dev == NULL) || (strlen(wifi_dev) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(wifi_dev) == true)
        return GXCORE_ERROR;

    _lock_if_dev(wifi_dev);
    if((p_dev = _if_dev_list_get(wifi_dev)) == NULL)
    {
        _unlock_if_dev(wifi_dev);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_type != IF_TYPE_WIFI)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(wifi_dev);
        return GXCORE_ERROR;
    }

    if((p_dev->dev_state == IF_STATE_START)
        || (p_dev->dev_state == IF_STATE_CONNECTING)
        || (p_dev->dev_state == IF_STATE_CONNECTED)
        || (p_dev->dev_state == IF_STATE_REJ))
    {
        p_dev->dev_state = IF_STATE_IDLE;
        printf("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);

        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                s_network_dev_priority = MAX_PRIORITY;
                printf("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }
        _if_dev_list_modify(p_dev);
    }

#ifdef LINUX_OS
    sprintf(cmd_buf, "%s %s", WIFI_DEL_AP_CMD, wifi_dev);
    //printf("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
    system(cmd_buf);
#else
    //clear ap connect record
    GxBus_ConfigSet(AP0_DEV_KEY, AP_DEV_DEFAULT);
    GxBus_ConfigSet(AP0_SSID_KEY, AP_SSID_DEFAULT);
    GxBus_ConfigSet(AP0_MAC_KEY, AP_MAC_DEFAULT);
    GxBus_ConfigSet(AP0_PSK_KEY, AP_PSK_DEFAULT);
#endif

    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(wifi_dev);

    return GXCORE_SUCCESS;
}
#endif

#if (MOD_3G_SUPPORT > 0)
static status_t app_network_3g_set_param(GxMsgProperty_3G *_3g_param)
{
    IfDevice *p_dev = NULL;

    if((_3g_param == NULL) || (strlen(_3g_param->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(_3g_param->dev_name) == true)
        return GXCORE_ERROR;

    _lock_if_dev(_3g_param->dev_name);
    if((p_dev = _if_dev_list_get(_3g_param->dev_name)) == NULL)
    {
        _unlock_if_dev(_3g_param->dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_type != IF_TYPE_3G)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(_3g_param->dev_name);
        return GXCORE_ERROR;
    }

    if((p_dev->dev_state == IF_STATE_START)
            || (p_dev->dev_state == IF_STATE_CONNECTING)
            || (p_dev->dev_state == IF_STATE_CONNECTED)
            || (p_dev->dev_state == IF_STATE_REJ))
    {
        if(p_dev->dev_ops->if_stop != NULL)
            p_dev->dev_ops->if_stop(p_dev);

        p_dev->dev_state = IF_STATE_IDLE;
        printf("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);

        if(p_dev->cur_flag == true)
        {
            p_dev->cur_flag = false;
            s_global_net_state = IF_STATE_IDLE;
            s_network_dev_priority = MAX_PRIORITY;
            printf("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
        }
        app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        _if_dev_list_modify(p_dev);
    }

    if(_set_3g_cfg_file_data(_3g_param) == GXCORE_SUCCESS)
        GxBus_ConfigSetInt(_3G_OPERATOR_KEY, _3g_param->operator_id);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(_3g_param->dev_name);

    return GXCORE_SUCCESS;
}

static status_t app_network_3g_get_param(GxMsgProperty_3G *tgParam)
{
    return _get_3g_cfg_file_data(tgParam);
}
#endif

#if (PPPOE_SUPPORT > 0)
static status_t app_pppoe_set_param(GxMsgProperty_PppoeCfg *pppoe_cfg)
{
    IfDevice *p_dev = NULL;
    PppoeFileData pppoe_data;

    if((pppoe_cfg == NULL) || (strlen(pppoe_cfg->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(pppoe_cfg->dev_name) == true)
        return GXCORE_ERROR;

    _lock_if_dev(pppoe_cfg->dev_name);
    if((p_dev = _if_dev_list_get(pppoe_cfg->dev_name)) == NULL)
    {
        _unlock_if_dev(pppoe_cfg->dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_type != IF_TYPE_WIRED)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(pppoe_cfg->dev_name);
        return GXCORE_ERROR;
    }

    if((p_dev->dev_state == IF_STATE_START)
            || (p_dev->dev_state == IF_STATE_CONNECTING)
            || (p_dev->dev_state == IF_STATE_CONNECTED)
            || (p_dev->dev_state == IF_STATE_REJ))
    {
        p_dev->dev_state = IF_STATE_IDLE;
        printf("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);

        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                s_network_dev_priority = MAX_PRIORITY;
                printf("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }
        _if_dev_list_modify(p_dev);
    }

    memset(&pppoe_data, 0, sizeof(PppoeFileData));
    pppoe_data.mode = pppoe_cfg->mode;
    strcpy(pppoe_data.username, pppoe_cfg->username);
    strcpy(pppoe_data.passwd, pppoe_cfg->passwd);
    strcpy(pppoe_data.dns1, pppoe_cfg->dns1);
    strcpy(pppoe_data.dns2, pppoe_cfg->dns2);
    _set_pppoe_file_data(p_dev->dev_name, &pppoe_data);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(pppoe_cfg->dev_name);

    return GXCORE_SUCCESS;
}

static status_t app_pppoe_get_param(GxMsgProperty_PppoeCfg *pppoe_cfg)
{
    IfDevice *p_dev = NULL;
    PppoeFileData pppoe_file_data;

    if((pppoe_cfg == NULL) || (strlen(pppoe_cfg->dev_name) == 0))
        return GXCORE_ERROR;

    _lock_if_dev(pppoe_cfg->dev_name);
    if((p_dev = _if_dev_list_get(pppoe_cfg->dev_name)) == NULL)
    {
        _unlock_if_dev(pppoe_cfg->dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_type != IF_TYPE_WIRED)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(pppoe_cfg->dev_name);
        return GXCORE_ERROR;
    }

    memset(&pppoe_file_data, 0, sizeof(PppoeFileData));
    if(_get_pppoe_file_data(pppoe_cfg->dev_name, &pppoe_file_data) == GXCORE_SUCCESS)
    {
        pppoe_cfg->mode = pppoe_file_data.mode;
        strcpy(pppoe_cfg->username, pppoe_file_data.username);
        strcpy(pppoe_cfg->passwd, pppoe_file_data.passwd);
        strcpy(pppoe_cfg->dns1, pppoe_file_data.dns1);
        strcpy(pppoe_cfg->dns2, pppoe_file_data.dns2);
        //printf("~~~~~~[%s]%d, mode: %d, user: %s, pwd: %s, dns1: %s, dns2: %s!!!!!\n", __func__, __LINE__,
        //        pppoe_cfg->mode, pppoe_cfg->username, pppoe_cfg->passwd, pppoe_cfg->dns1, pppoe_cfg->dns2);
    }

    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(pppoe_cfg->dev_name);

    return GXCORE_SUCCESS;
}
#endif

static bool app_network_dev_compete(IfDevice *p_dev)
{
    bool ret = true;

    if(p_dev->main_flag == true)
        return true;

    if(p_dev->dev_priority > s_network_dev_priority)
        ret = false;
    else if(p_dev->dev_priority < s_network_dev_priority)
        ret = true;
    else // ==
    {
        if(p_dev->cur_flag == false)
            ret = false;
        else
            ret = true;
    }

    return ret;
}

static status_t _network_stop_service(void)
{
    IfDevList *p_dev_list = NULL;
    IfDevice *p_dev = NULL;

    GxCore_MutexLock(s_network_service_mutex);

    if(s_network_service_enable == true)
    {
        p_dev_list = if_dev_list;
        while(p_dev_list != NULL)
        {
            p_dev = &(p_dev_list->net_dev);

            GxCore_MutexLock(p_dev->dev_mutex);

            if(_check_prev_init_dev(p_dev->dev_name) == false)
            {
                if(p_dev->dev_ops->if_stop != NULL)
                    p_dev->dev_ops->if_stop(p_dev);
                p_dev->dev_state = IF_STATE_IDLE;
                p_dev->scanning_flag = false;
                p_dev->cur_flag = false;

                app_network_plug_out_report(p_dev->dev_name);
            }
            GxCore_MutexUnlock(p_dev->dev_mutex);
            p_dev_list = p_dev_list->next;
        }

        s_global_net_state = IF_STATE_IDLE;
        s_network_dev_priority = MAX_PRIORITY;
        printf("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);

        s_network_service_enable = false;
    }

    GxCore_MutexUnlock(s_network_service_mutex);

    return GXCORE_SUCCESS;
}

static status_t _network_start_service(void)
{
    GxCore_MutexLock(s_network_service_mutex);

    s_network_service_enable = true;

    GxCore_MutexUnlock(s_network_service_mutex);

    return GXCORE_SUCCESS;
}

status_t AppNetworkServiceInit(handle_t self, int priority_offset)
{
    handle_t sch;


#ifdef LINUX_OS
#if (PPPOE_SUPPORT == 0)
    char cmd_buf[CMD_LEN] = {0};
    if(GXCORE_FILE_EXIST == GxCore_FileExists(PPPOE_FILE))
    {
        memset(cmd_buf, 0, CMD_LEN);
        sprintf(cmd_buf, "%s %s", RM_FILE_CMD, PPPOE_FILE);
        system(cmd_buf);
    }
#endif
#endif

    GxBus_MessageRegister(GXMSG_NETWORK_START, 0);
    GxBus_MessageRegister(GXMSG_NETWORK_STOP, 0);

    GxBus_MessageRegister(GXMSG_NETWORK_DEV_LIST, sizeof(GxMsgProperty_NetDevList));

    GxBus_MessageRegister(GXMSG_NETWORK_GET_STATE, sizeof(GxMsgProperty_NetDevState));
    GxBus_MessageRegister(GXMSG_NETWORK_STATE_REPORT, sizeof(GxMsgProperty_NetDevState));

    GxBus_MessageRegister(GXMSG_NETWORK_GET_TYPE, sizeof(GxMsgProperty_NetDevType));

    GxBus_MessageRegister(GXMSG_NETWORK_GET_CUR_DEV, sizeof(ubyte64));

    GxBus_MessageRegister(GXMSG_NETWORK_PLUG_IN, sizeof(ubyte64));
    GxBus_MessageRegister(GXMSG_NETWORK_PLUG_OUT, sizeof(ubyte64));

    GxBus_MessageRegister(GXMSG_NETWORK_SET_MAC, sizeof(GxMsgProperty_NetDevMac));
    GxBus_MessageRegister(GXMSG_NETWORK_GET_MAC, sizeof(GxMsgProperty_NetDevMac));

    GxBus_MessageRegister(GXMSG_NETWORK_SET_IPCFG, sizeof(GxMsgProperty_NetDevIpcfg));
    GxBus_MessageRegister(GXMSG_NETWORK_GET_IPCFG, sizeof(GxMsgProperty_NetDevIpcfg));

    GxBus_MessageRegister(GXMSG_NETWORK_SET_MODE, sizeof(GxMsgProperty_NetDevMode));
    GxBus_MessageRegister(GXMSG_NETWORK_GET_MODE, sizeof(GxMsgProperty_NetDevMode));

    GxBus_MessageRegister(GXMSG_WIFI_SCAN_AP, sizeof(ubyte64));
    GxBus_MessageRegister(GXMSG_WIFI_SCAN_AP_OVER, sizeof(GxMsgProperty_ApList));

    GxBus_MessageRegister(GXMSG_WIFI_GET_CUR_AP, sizeof(GxMsgProperty_CurAp));
    GxBus_MessageRegister(GXMSG_WIFI_SET_CUR_AP, sizeof(GxMsgProperty_CurAp));
    GxBus_MessageRegister(GXMSG_WIFI_DEL_CUR_AP, sizeof(ubyte64));

	//GxBus_MessageRegister(GXMSG_3G_CONNECT, sizeof(GxMsgProperty_3G));
	//GxBus_MessageRegister(GXMSG_3G_DISCONNECT, sizeof(ubyte64));
	GxBus_MessageRegister(GXMSG_3G_SET_PARAM, sizeof(GxMsgProperty_3G));
	GxBus_MessageRegister(GXMSG_3G_GET_PARAM, sizeof(GxMsgProperty_3G));
    GxBus_MessageRegister(GXMSG_3G_GET_CUR_OPERATOR, sizeof(GxMsgProperty_3G_Operator));

    GxBus_MessageRegister(GXMSG_PPPOE_SET_PARAM, sizeof(GxMsgProperty_PppoeCfg));
	GxBus_MessageRegister(GXMSG_PPPOE_GET_PARAM, sizeof(GxMsgProperty_PppoeCfg));

    GxBus_MessageListen(self,GXMSG_NETWORK_START);
    GxBus_MessageListen(self,GXMSG_NETWORK_STOP);
    GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
    GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
    
    GxBus_MessageListen(self,GXMSG_NETWORK_DEV_LIST);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_STATE);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_TYPE);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_CUR_DEV);
    GxBus_MessageListen(self,GXMSG_NETWORK_SET_MAC);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_MAC);
    GxBus_MessageListen(self,GXMSG_NETWORK_SET_IPCFG);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_IPCFG);
    GxBus_MessageListen(self,GXMSG_NETWORK_SET_MODE);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_MODE);
    GxBus_MessageListen(self,GXMSG_WIFI_SCAN_AP);
    GxBus_MessageListen(self,GXMSG_WIFI_GET_CUR_AP);
    GxBus_MessageListen(self,GXMSG_WIFI_SET_CUR_AP);
    GxBus_MessageListen(self,GXMSG_WIFI_DEL_CUR_AP);

	//usrGxBus_MessageListen(self,GXMSG_3G_CONNECT);
	//GxBus_MessageListen(self,GXMSG_3G_DISCONNECT);
	GxBus_MessageListen(self,GXMSG_3G_SET_PARAM);
	GxBus_MessageListen(self,GXMSG_3G_GET_PARAM);
	GxBus_MessageListen(self,GXMSG_3G_GET_CUR_OPERATOR);
	GxBus_MessageListen(self,GXMSG_USB3G_HOTPLUG_IN);
	GxBus_MessageListen(self,GXMSG_USB3G_HOTPLUG_OUT);

    GxBus_MessageListen(self,GXMSG_PPPOE_SET_PARAM);
	GxBus_MessageListen(self,GXMSG_PPPOE_GET_PARAM);

    GxCore_MutexCreate(&s_dev_list_mutex);

    s_network_dev_priority = MAX_PRIORITY;

    GxCore_MutexCreate(&s_network_service_mutex);

    s_prev_init_dev.dev_num = _get_prev_init_dev(s_prev_init_dev.net_dev);

    sch = GxBus_SchedulerCreate("NetworkMsgScheduler", GXBUS_SCHED_MSG,
                                    1024 * 32, GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

    sch = GxBus_SchedulerCreate("NetworkConsoleScheduler", GXBUS_SCHED_CONSOLE,
                                    1024 * 10, GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

    //_network_start_service();

    return GXCORE_SUCCESS;
}

void AppNetworkServiceDestroy(handle_t self)
{
    g_wifi_hotplug_status = -1;
    s_wifi_init_status = -1;

    GxBus_MessageUnListen(self, GXMSG_NETWORK_START);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_STOP);
    GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
    GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
    GxBus_MessageUnListen(self, GXMSG_USB3G_HOTPLUG_IN);
    GxBus_MessageUnListen(self, GXMSG_USB3G_HOTPLUG_OUT);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_DEV_LIST);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_STATE);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_TYPE);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_CUR_DEV);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_SET_MAC);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_MAC);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_SET_IPCFG);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_IPCFG);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_SET_MODE);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_MODE);
    GxBus_MessageUnListen(self, GXMSG_WIFI_SCAN_AP);
    GxBus_MessageUnListen(self, GXMSG_WIFI_GET_CUR_AP);
    GxBus_MessageUnListen(self, GXMSG_WIFI_SET_CUR_AP);
    GxBus_MessageUnListen(self, GXMSG_WIFI_DEL_CUR_AP);
	//GxBus_MessageUnListen(self, GXMSG_3G_CONNECT);
	//GxBus_MessageUnListen(self, GXMSG_3G_DISCONNECT);
	GxBus_MessageUnListen(self, GXMSG_3G_SET_PARAM);
	GxBus_MessageUnListen(self, GXMSG_3G_GET_PARAM);
	GxBus_MessageUnListen(self,GXMSG_3G_GET_CUR_OPERATOR);

    GxBus_MessageUnListen(self, GXMSG_PPPOE_SET_PARAM);
	GxBus_MessageUnListen(self, GXMSG_PPPOE_GET_PARAM);

    GxBus_MessageUnregister(GXMSG_NETWORK_START);
    GxBus_MessageUnregister(GXMSG_NETWORK_STOP);
    GxBus_MessageUnregister(GXMSG_USBWIFI_HOTPLUG_IN);
    GxBus_MessageUnregister(GXMSG_USBWIFI_HOTPLUG_OUT);
    GxBus_MessageUnregister(GXMSG_USB3G_HOTPLUG_IN);
    GxBus_MessageUnregister(GXMSG_USB3G_HOTPLUG_OUT);
    GxBus_MessageUnregister(GXMSG_NETWORK_DEV_LIST);

    GxBus_MessageUnregister(GXMSG_NETWORK_GET_STATE);
    GxBus_MessageUnregister(GXMSG_NETWORK_STATE_REPORT);

    GxBus_MessageUnregister(GXMSG_NETWORK_GET_TYPE);

    GxBus_MessageUnregister(GXMSG_NETWORK_GET_CUR_DEV);

    GxBus_MessageUnregister(GXMSG_NETWORK_PLUG_IN);
    GxBus_MessageUnregister(GXMSG_NETWORK_PLUG_OUT);

    GxBus_MessageUnregister(GXMSG_NETWORK_SET_MAC);
    GxBus_MessageUnregister(GXMSG_NETWORK_GET_MAC);

    GxBus_MessageUnregister(GXMSG_NETWORK_SET_IPCFG);
    GxBus_MessageUnregister(GXMSG_NETWORK_GET_IPCFG);

    GxBus_MessageUnregister(GXMSG_NETWORK_SET_MODE);
    GxBus_MessageUnregister(GXMSG_NETWORK_GET_MODE);

    GxBus_MessageUnregister(GXMSG_WIFI_SCAN_AP);
    GxBus_MessageUnregister(GXMSG_WIFI_SCAN_AP_OVER);
    GxBus_MessageUnregister(GXMSG_WIFI_GET_CUR_AP);
    GxBus_MessageUnregister(GXMSG_WIFI_SET_CUR_AP);
    GxBus_MessageUnregister(GXMSG_WIFI_DEL_CUR_AP);
	//GxBus_MessageUnregister(GXMSG_3G_CONNECT);
	//GxBus_MessageUnregister(GXMSG_3G_DISCONNECT);
	GxBus_MessageUnregister(GXMSG_3G_SET_PARAM);
	GxBus_MessageUnregister(GXMSG_3G_GET_PARAM);
	GxBus_MessageUnregister(GXMSG_3G_GET_CUR_OPERATOR);

    GxBus_MessageUnregister(GXMSG_PPPOE_SET_PARAM);
	GxBus_MessageUnregister(GXMSG_PPPOE_GET_PARAM);

    GxBus_ServiceUnlink(self);

    return;
}

GxMsgStatus AppNetworkServiceRecvMsg(handle_t self, GxMessage *msg)
{
    GxMsgStatus ret = GXCORE_SUCCESS;

    if(msg == NULL)
        return GXCORE_ERROR;

    switch(msg->msg_id)
    {
        case GXMSG_NETWORK_START:
            _network_start_service();
            break;

        case GXMSG_NETWORK_STOP:
            _network_stop_service();
            break;

        case GXMSG_USBWIFI_HOTPLUG_OUT:
            g_wifi_hotplug_status = 0;
            break;

        case GXMSG_USBWIFI_HOTPLUG_IN:
            g_wifi_hotplug_status = 1;
            break;

        case GXMSG_NETWORK_DEV_LIST:
        {
            GxMsgProperty_NetDevList *dev_list = NULL;
            unsigned char state_flag = (IF_STATE_IDLE | IF_STATE_START | IF_STATE_CONNECTING | IF_STATE_CONNECTED | IF_STATE_REJ);

            dev_list = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevList);
            app_network_dev_list_get(dev_list, state_flag);
            break;
        }

        case GXMSG_NETWORK_GET_STATE:
        {
            GxMsgProperty_NetDevState *dev_state = NULL;
            dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
            app_network_dev_get_state(dev_state);
            break;
        }

        case GXMSG_NETWORK_GET_TYPE:
        {
            GxMsgProperty_NetDevType *dev_type = NULL;
            dev_type = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevType);
            app_network_dev_get_type(dev_type);
            break;
        }

        case GXMSG_NETWORK_GET_CUR_DEV:
        {
            ubyte64 *cur_dev = NULL;
            cur_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            app_network_get_cur_dev(cur_dev);
            break;
        }

        case GXMSG_NETWORK_SET_MAC:
        {
            GxMsgProperty_NetDevMac *dev_mac = NULL;
            dev_mac = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevMac);
            app_network_dev_set_mac(dev_mac);
            break;
        }

        case GXMSG_NETWORK_GET_MAC:
        {
            GxMsgProperty_NetDevMac *dev_mac = NULL;
            dev_mac = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevMac);
            app_network_dev_get_mac(dev_mac);
            break;
        }

        case GXMSG_NETWORK_SET_IPCFG:
        {
            GxMsgProperty_NetDevIpcfg *dev_config = NULL;
            dev_config = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevIpcfg);
            app_network_dev_set_ipcfg(dev_config);
            break;
        }

        case GXMSG_NETWORK_GET_IPCFG:
        {
            GxMsgProperty_NetDevIpcfg *dev_config = NULL;
            dev_config = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevIpcfg);
            app_network_dev_get_ipcfg(dev_config);
            break;
        }

        case GXMSG_NETWORK_SET_MODE:
        {
            GxMsgProperty_NetDevMode *dev_mode = NULL;
            dev_mode = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevMode);
            app_network_dev_set_mode(dev_mode);
            break;
        }

        case GXMSG_NETWORK_GET_MODE:
        {
            GxMsgProperty_NetDevMode *dev_mode = NULL;
            dev_mode = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevMode);
            app_network_dev_get_mode(dev_mode);
            break;
        }

#if (WIFI_SUPPORT > 0)
        case GXMSG_WIFI_SCAN_AP:
        {
            //printf("-----[%s]%d-----\n", __func__, __LINE__);
            ubyte64 *dev_name = NULL;
            dev_name = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            app_wifi_scan_ap(*dev_name);

            break;
        }

        case GXMSG_WIFI_GET_CUR_AP:
        {
            //printf("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_CurAp *cur_ap = NULL;
            cur_ap = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_CurAp);
            app_wifi_get_cur_ap(cur_ap);
            break;
        }

        case GXMSG_WIFI_SET_CUR_AP:
        {
            //printf("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_CurAp *cur_ap = NULL;
            cur_ap = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_CurAp);
            app_wifi_set_cur_ap(cur_ap);
            break;
        }

        case GXMSG_WIFI_DEL_CUR_AP:
        {
            //printf("-----[%s]%d-----\n", __func__, __LINE__);
            ubyte64 *wifi_dev = NULL;
            wifi_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            app_wifi_del_cur_ap(*wifi_dev);
            break;
        }
#endif

#if (MOD_3G_SUPPORT > 0)
		case GXMSG_3G_SET_PARAM:
        {
            printf("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_3G *tgParam = NULL;
            tgParam = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_3G);
			app_network_3g_set_param(tgParam);
            break;
        }

		case GXMSG_3G_GET_PARAM:
        {
            printf("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_3G *tgParam = NULL;
            tgParam = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_3G);
			app_network_3g_get_param(tgParam);
            break;
        }

        case GXMSG_3G_GET_CUR_OPERATOR:
        {
            printf("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_3G_Operator *operator = NULL;
            int value = 0;
            operator = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_3G_Operator);
            GxBus_ConfigGetInt(_3G_OPERATOR_KEY, &value, _3G_OPERATOR_DEFAULT);
            operator->operator_id = value;
            break;
        }

#ifdef ECOS_OS
        case GXMSG_USB3G_HOTPLUG_IN:
        {
            GxMsgProperty_Usb3gHotPlug *tg_plug = NULL;
            tg_plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_Usb3gHotPlug);
            if(tg_plug != NULL)
            {
                printf("[%s][%d], GXMSG_USB3G_HOTPLUG_IN, id = %d, err = %d\n", __func__, __LINE__, tg_plug->id, tg_plug->error);
                thiz_usb3g_hotplug_status = true;
            }
            break;
        }

        case GXMSG_USB3G_HOTPLUG_OUT:
        {
            GxMsgProperty_Usb3gHotPlug *tg_plug = NULL;
            tg_plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_Usb3gHotPlug);
            if(tg_plug != NULL)
            {
                printf("[%s][%d], GXMSG_USB3G_HOTPLUG_OUT, id = %d, err = %d\n", __func__, __LINE__, tg_plug->id, tg_plug->error);
                thiz_usb3g_hotplug_status = false;
            }
            break;
        }
#endif
#endif

#if (PPPOE_SUPPORT > 0)
        case GXMSG_PPPOE_SET_PARAM:
        {
            printf("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_PppoeCfg *pppoe_cfg = NULL;
            pppoe_cfg = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PppoeCfg);
			app_pppoe_set_param(pppoe_cfg);
            break;
        }

		case GXMSG_PPPOE_GET_PARAM:
        {
            printf("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_PppoeCfg *pppoe_cfg = NULL;
            pppoe_cfg = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PppoeCfg);
			app_pppoe_get_param(pppoe_cfg);
            break;
        }
#endif

        default:
            ret = GXMSG_OK;
            break;
    }

    return ret;
}

extern int mt7601_connect_status;
static void AppNetworkServiceConsole(handle_t self)
{
    bool valid_flag = false;
    IfDevList *p_dev_list = NULL;
    IfDevice *p_dev = NULL;

    if(GxCore_MutexTrylock(s_network_service_mutex) != GXCORE_SUCCESS)
    {
        GxCore_ThreadDelay(CONSOLE_DELAY_MS);
        return;
    }

#if (WIFI_SUPPORT > 0)
    static int sWifi = -1;
    static int sInit = -1;
    static int sPlug = -1;

    if(app_check_wifi_status() != sWifi || s_wifi_init_status != sInit || g_wifi_hotplug_status != sPlug) {
	    printf("@@@@@@@@@@ >> wifi status : %d init status : %d  hotplug : %d\n", 
			    app_check_wifi_status(), 
			    s_wifi_init_status,
			    g_wifi_hotplug_status);
	    sWifi = app_check_wifi_status();
	    sInit = s_wifi_init_status;
	    sPlug = g_wifi_hotplug_status;
    }

    if(s_wifi_init_status == -1 && 0 == app_check_wifi_status())
    {
        s_wifi_init_status = 1;
    }

    //if(s_wifi_init_status == -1 && -1 == app_check_wifi_status())
    if(g_wifi_hotplug_status == 0 && 0 == app_check_wifi_status())
    {
        s_wifi_init_status = -1;
        mt7601_connect_status = -1;
        GxBus_ConfigSet(WIFI_STATUS_KEY, "fail");
    }

    if(s_network_service_enable == false) //|| s_wifi_init_status == -1)
    {
        GxCore_MutexUnlock(s_network_service_mutex);
        GxCore_ThreadDelay(CONSOLE_DELAY_MS);
        return;
    }
#endif
    _network_check_all_dev();

    p_dev_list = if_dev_list;
    while(p_dev_list != NULL)
    {
        p_dev = &(p_dev_list->net_dev);

        if(GxCore_MutexTrylock(p_dev->dev_mutex) != GXCORE_SUCCESS)
        {
            p_dev_list = p_dev_list->next;
            continue;
        }

#if (MOD_3G_SUPPORT > 0)
        if(_if_dev_check_type(p_dev->dev_name) == IF_TYPE_3G)
        {
            valid_flag = _network_usb3g_lookup(NULL, 0);
        }
        else
#endif
        {
#ifdef ECOS_OS
#if (WIFI_SUPPORT > 0)
            if(_if_dev_check_type(p_dev->dev_name) == IF_TYPE_WIFI)
            {
                valid_flag = ((g_wifi_hotplug_status == 1) ? true : false);
            }
            else
#endif
#endif
                valid_flag = app_check_netlink(p_dev->dev_name);
        }

        if(valid_flag == false) //plug out
        {
#if (WIFI_SUPPORT > 0)
            if(_if_dev_check_type(p_dev->dev_name) == IF_TYPE_WIFI)
                s_wifi_init_status = -1;
#endif

            if((p_dev->dev_state != IF_STATE_INVALID) && (_check_prev_init_dev(p_dev->dev_name) == false))
            {
                if(p_dev->dev_ops->if_stop != NULL)
                    p_dev->dev_ops->if_stop(p_dev);
                p_dev->dev_state = IF_STATE_INVALID;
                p_dev->scanning_flag = false;
                if(p_dev->cur_flag == true)
                {
                    p_dev->cur_flag = false;
                    s_global_net_state = IF_STATE_IDLE;
                    s_network_dev_priority = MAX_PRIORITY;
                    printf("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
                }

                app_network_plug_out_report(p_dev->dev_name);
                s_dev_type_num[p_dev->dev_type]--;
            }
        }
        else
        {
            if(p_dev->dev_state == IF_STATE_INVALID) //plug in
            {
                if(s_dev_type_num[p_dev->dev_type] > 0)
                {
                    GxCore_MutexUnlock(p_dev->dev_mutex);
                    p_dev_list = p_dev_list->next;

                    continue;
                }

                s_dev_type_num[p_dev->dev_type]++;
                p_dev->dev_state = IF_STATE_IDLE;
                p_dev->user_flag = _check_dev_user_flag(p_dev->dev_name);
                app_network_plug_in_report(p_dev->dev_name);
            }

            if(p_dev->dev_state == IF_STATE_IDLE)
            {
                if((p_dev->user_flag == true)
                        && (p_dev->scanning_flag == false)
                        && (app_network_dev_compete(p_dev) == true))
                {
                    ubyte64 cur_dev = {0};
                    app_network_get_cur_dev(&cur_dev);
                    if(_check_prev_init_dev(cur_dev) == false)
                    {
                        if((p_dev->dev_ops->if_start != NULL)
                                && (p_dev->dev_ops->if_start(p_dev) == GXCORE_SUCCESS))
                        {
                            p_dev->dev_state = IF_STATE_START;
                            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
                        }
                    }
                }
            }
            else //active dev
            {
                //user disable
                if((_check_prev_init_dev(p_dev->dev_name) == false)
                        && ((p_dev->user_flag == false) && (p_dev->scanning_flag == false)))
                {
                    if(p_dev->dev_ops->if_stop != NULL)
                        p_dev->dev_ops->if_stop(p_dev);
                    p_dev->dev_state = IF_STATE_IDLE;

                    if(p_dev->cur_flag == true)
                    {
                        p_dev->cur_flag = false;
                        s_global_net_state = IF_STATE_IDLE;
                        s_network_dev_priority = MAX_PRIORITY;
                        printf("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
                    }

                    GxCore_MutexUnlock(p_dev->dev_mutex);
                    p_dev_list = p_dev_list->next;

                    app_network_state_report(p_dev->dev_name, p_dev->dev_state);

                    continue;
                }

                //useless dev
                if((p_dev->scanning_flag == false) && (app_network_dev_compete(p_dev) == false))
                {
                    if(p_dev->dev_ops->if_stop != NULL)
                        p_dev->dev_ops->if_stop(p_dev);
                    p_dev->dev_state = IF_STATE_IDLE;
                    p_dev->cur_flag = false;

                    GxCore_MutexUnlock(p_dev->dev_mutex);
                    p_dev_list = p_dev_list->next;

                    app_network_state_report(p_dev->dev_name, p_dev->dev_state);

                    continue;
                }

                if((p_dev->dev_state == IF_STATE_START)
                        || (p_dev->dev_state == IF_STATE_CONNECTING)
                        || (p_dev->dev_state == IF_STATE_CONNECTED))
                {
                    if(p_dev->dev_ops->if_monitor_connect != NULL)
                    {
                        IfFileRet ret = p_dev->dev_ops->if_monitor_connect(p_dev);
                        if((ret == IF_FILE_FAIL) || (ret == IF_FILE_REJ) || (ret == IF_FILE_NONE)) //failed
                        {
                            if((p_dev->scanning_flag == false) && (_check_prev_init_dev(p_dev->dev_name) == false))
                            {
                                //printf("~~~~~~[%s]%d, %s failed!!!!!\n", __func__, __LINE__, p_dev->dev_name);
                                if(p_dev->dev_ops->if_stop != NULL)
                                    p_dev->dev_ops->if_stop(p_dev);
                                if(ret == IF_FILE_REJ)
                                    p_dev->dev_state = IF_STATE_REJ;
                                else
                                    p_dev->dev_state = IF_STATE_IDLE;

                                if(p_dev->cur_flag == true)
                                {
                                    p_dev->cur_flag = false;
                                    s_global_net_state = IF_STATE_IDLE;
                                    s_network_dev_priority = MAX_PRIORITY;
                                    printf("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
                                }
                                app_network_state_report(p_dev->dev_name, p_dev->dev_state);
                            }
                        }
                        else if(ret == IF_FILE_OK) //ok
                        {
                            if(p_dev->dev_state != IF_STATE_CONNECTED)
                            {
                                printf("~~~~~~[%s]%d, %s ok!!!!!\n", __func__, __LINE__, p_dev->dev_name);
                                p_dev->dev_state = IF_STATE_CONNECTED;
                                p_dev->cur_flag = true;
                                s_global_net_state = IF_STATE_CONNECTED;
                                s_network_dev_priority = p_dev->dev_priority;
                                app_network_state_report(p_dev->dev_name, p_dev->dev_state);
                                printf("~~~~~~[%s]%d, s_global_net_state up!!!!!\n", __func__, __LINE__);
                            }
                        }
                        else if(ret == IF_FILE_WAIT)
                        {
                            if(p_dev->dev_state == IF_STATE_START)
                            {
                                printf("~~~~~~[%s]%d, %s connect!!!!!\n", __func__, __LINE__, p_dev->dev_name);
                                p_dev->dev_state = IF_STATE_CONNECTING;
                                app_network_state_report(p_dev->dev_name, p_dev->dev_state);
                            }
                        }
                        else//monitoring...
                        {
                            //printf("~~~~~~[%s]%d, %s monitoring!!!!!\n", __func__, __LINE__, p_dev->dev_name);
                        }
                    }
                }
            }
        }

        GxCore_MutexUnlock(p_dev->dev_mutex);
        p_dev_list = p_dev_list->next;
    }

    GxCore_MutexUnlock(s_network_service_mutex);
    GxCore_ThreadDelay(CONSOLE_DELAY_MS);
}

GxServiceClass app_network_service =
{
    "app network service",
    AppNetworkServiceInit,
    AppNetworkServiceDestroy,
    AppNetworkServiceRecvMsg,
    AppNetworkServiceConsole,
};

#endif
//#endif
