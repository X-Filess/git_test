/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_ota_ui.c
* Author    :	B.Z.
* Project   :	OTA
* Type      :   Source Code
******************************************************************************
* Description:
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.11.23		   B.Z.			  Creation
*****************************************************************************/

#include "app_config.h"
#if (OTA_MONITOR_SUPPORT > 0)// menu ota or loader ota

#include "app_module.h"
#include "gxbus.h"
#include "app_msg.h"

int app_ota_ui_msg_send(void *param)
{
    int ret = 0;
    GxMessage *msg = NULL;
    OTAUpdateClass *para = NULL;
    if (NULL == param) {
        return -1;
    }
    msg = GxBus_MessageNew(GXMSG_OTA_EVENT);
    if (NULL == msg) {
        return -1;
    }
    para = GxBus_GetMsgPropertyPtr(msg, OTAUpdateClass);
    if (NULL == para) {
        return -1;
    }
    memcpy(para, param, sizeof(OTAUpdateClass));
    GxBus_MessageSend(msg);
    return ret;
}

int app_ota_ui_msg_recv(void *param)
{
    extern int app_ota_monitor_pop_exec(void *param);
    int ret = 0;
    app_ota_monitor_pop_exec(param);
    return ret;
}
#endif
