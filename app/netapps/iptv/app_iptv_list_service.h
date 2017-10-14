#ifndef __APP_IPTV_SERVICE_H__
#define __APP_IPTV_SERVICE_H__

#include "app_config.h"
#if IPTV_SUPPORT
#include <gx_apps.h>
#include <app_iptv_list_api.h>
#include "gxtype.h"

typedef enum
{
    IPTV_LIST_REMOTE = 0,
    IPTV_LIST_LOCAL,
    IPTV_LIST_FAV,
    IPTV_LIST_UNKOWN,
}IptvListSource;

typedef struct
{
    IptvListSource iptv_source;
    IptvListClass *iptv_list;
}AppMsg_IptvList;

status_t app_iptv_service_start(void);

#endif
#endif
