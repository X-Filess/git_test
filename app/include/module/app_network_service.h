#ifndef __APP_NETWORK_H__
#define __APP_NETWORK_H__

//#ifdef LINUX_OS
#if NETWORK_SUPPORT

#include "app.h"

#define MAX_NET_DEV_NUM (10)
#define MAX_AP_NUM (50)

typedef enum
{
    IF_TYPE_WIRED = 0,
#if (WIFI_SUPPORT > 0)
    IF_TYPE_WIFI,
#endif
#if (MOD_3G_SUPPORT > 0)
    IF_TYPE_3G,
#endif
    IF_TYPE_UNKOWN
}IfType;

typedef enum
{
    IF_STATE_INVALID = (1 << 0),
    IF_STATE_IDLE = (1 << 1),
    IF_STATE_START = (1 << 2),
    IF_STATE_CONNECTING = (1 << 3),
    IF_STATE_CONNECTED = (1 << 4),
    IF_STATE_REJ = (1 << 5),
}IfState;

typedef enum
{
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_SHARED = (1<<0),
    WIFI_AUTH_WPA1PSK = (1<<1),
    WIFI_AUTH_WPA2PSK = (1<<2),
    WIFI_AUTH_WPA1 = (1<<3),
    WIFI_AUTH_WPA2 = (1<<4),
}WifiAuthType;

typedef enum
{
    WIFI_ENCRYP_NONE = 0,
    WIFI_ENCRYP_WEP = (1<<0),
    WIFI_ENCRYP_TKIP = (1<<1),
    WIFI_ENCRYP_AES = (1<<2),
}WifiEncrypType;

typedef struct
{
    ubyte64 ap_essid;
    ubyte32 ap_mac;
    ubyte32 ap_psk;
    unsigned char ap_quality;
    unsigned short encryption;
}ApInfo;

typedef struct
{
    unsigned int dev_num;
    ubyte64 dev_name[MAX_NET_DEV_NUM];
}GxMsgProperty_NetDevList;

typedef struct
{
    ubyte64 dev_name;
    IfState dev_state;
}GxMsgProperty_NetDevState;

typedef struct
{
    ubyte64 dev_name;
    IfType dev_type;
}GxMsgProperty_NetDevType;

typedef struct
{
    ubyte64 dev_name;
    char ip_type;
    ubyte32 ip_addr;
    ubyte32 net_mask;
    ubyte32 gate_way;
    ubyte32 dns1;
    ubyte32 dns2;
}GxMsgProperty_NetDevIpcfg;

typedef struct
{
    ubyte64 dev_name;
    unsigned char mode;
}GxMsgProperty_NetDevMode;

typedef struct
{
    ubyte64 dev_name;
    ubyte32 dev_mac;
}GxMsgProperty_NetDevMac;

typedef struct
{
    ubyte64 wifi_dev;
    unsigned int ap_num;
    ApInfo *ap_info;
}GxMsgProperty_ApList;

typedef struct
{
    ubyte64 wifi_dev;
    ubyte32 ap_essid;
    ubyte32 ap_mac;
    ubyte32 ap_psk;
}GxMsgProperty_CurAp;

typedef struct
{
	ubyte64 dev_name;
    unsigned char operator_id; //operator
    ubyte32 apn;
    ubyte32 phone;
    ubyte32 username;
    ubyte32 password;
}GxMsgProperty_3G;

typedef struct
{
    unsigned char operator_id;
}GxMsgProperty_3G_Operator;

typedef struct
{
	ubyte64 dev_name;
    unsigned char mode;
    ubyte32 username;
    ubyte32 passwd;
    ubyte32 dns1;
    ubyte32 dns2;
}GxMsgProperty_PppoeCfg;

#endif
//#endif
#endif
