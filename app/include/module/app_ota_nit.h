#ifndef __APP_OTA_NIT_H__
#define __APP_OTA_NIT_H__

#include "gxcore.h"
#include "app_module.h"
#include "gxsi.h"
#include "gxmsg.h"
#include "app_send_msg.h"

struct app_ota_nit
{
    uint16_t ts_src;
	uint32_t demuxid;
    uint32_t pid;
    uint32_t timeout;

    int16_t  subt_id;
    uint32_t req_id;
    uint32_t ver;
};

int app_ota_nit_start(struct app_ota_nit *ota_nit);
int app_ota_nit_restart(void);
int app_ota_nit_release(void);
int app_ota_nit_get(GxMsgProperty_SiSubtableOk *data);

#endif
