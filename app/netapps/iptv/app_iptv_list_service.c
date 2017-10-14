#include "app.h"
#include "app_module.h"

#if IPTV_SUPPORT
#include "app_iptv_list_service.h"

#define IPTV_SERVICE_NAME    "iptv list service"

static handle_t thiz_get_remote_channels_thread = 0;
static handle_t thiz_get_ad_pic_thread = 0;
static status_t AppIptvServiceInit(handle_t self,int priority_offset);
static void AppIptvServiceDestroy(handle_t self);
static GxMsgStatus AppIptvServiceRecvMsg(handle_t self, GxMessage * Msg);

GxServiceClass app_iptv_service =
{
    IPTV_SERVICE_NAME,
    AppIptvServiceInit,
    AppIptvServiceDestroy,
    AppIptvServiceRecvMsg,
    NULL,
};

static void _get_remote_channels_proc(void *arg)
{
    AppMsg_IptvList iptv_list;
    char username[CLOUD_MAX_ID_LEN + 1] = {0};
    char passwd[CLOUD_MAX_PASSWD_LEN + 1] = {0};
    char chipid[8] = {0};

    GxCore_ThreadDetach();

    if(app_chip_sn_get(chipid, 8) == 0)
    {
        GxBus_ConfigGet(CLOUD_ID_KEY, username, CLOUD_MAX_ID_LEN,  CLOUD_DEFAULT_ID);
        GxBus_ConfigGet(CLOUD_PASSWD_KEY, passwd, CLOUD_MAX_PASSWD_LEN, CLOUD_DEFAULT_PASSWD);

        memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
        iptv_list.iptv_source = IPTV_LIST_REMOTE;
        iptv_list.iptv_list = app_iptv_api_get_remote_list(username, passwd, chipid);
    }

    thiz_get_remote_channels_thread = 0;
    app_send_msg_exec(APPMSG_IPTV_LIST_DONE, &iptv_list);
}

static status_t _app_iptv_service_get_remote_list(void)
{
    if(thiz_get_remote_channels_thread > 0)
    {
        printf("[%s][%d]!!!!!!!!!! busy busy busy!!!!!!!!! \n", __func__, __LINE__);
        return GXCORE_ERROR;
    }

    return GxCore_ThreadCreate("iptv_get_remote_channels", &thiz_get_remote_channels_thread, _get_remote_channels_proc,
            NULL, 8 * 1024, GXOS_DEFAULT_PRIORITY);
}

static status_t app_iptv_service_get_local_list(void)
{
    AppMsg_IptvList iptv_list;

    memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
    iptv_list.iptv_source = IPTV_LIST_LOCAL;
    iptv_list.iptv_list = app_iptv_api_get_local_list();

    app_send_msg_exec(APPMSG_IPTV_LIST_DONE, &iptv_list);

    return GXCORE_SUCCESS;
}

static status_t app_iptv_service_get_fav_list(void)
{
    AppMsg_IptvList iptv_list;

    memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
    iptv_list.iptv_source = IPTV_LIST_FAV;
    iptv_list.iptv_list = app_iptv_api_get_fav_list();

    app_send_msg_exec(APPMSG_IPTV_LIST_DONE, &iptv_list);

    return GXCORE_SUCCESS;
}

static status_t app_iptv_service_get_list(IptvListSource list_source)
{
    status_t ret = GXCORE_ERROR;

    switch(list_source)
    {
        case IPTV_LIST_REMOTE:
            ret = _app_iptv_service_get_remote_list();
            break;

        case IPTV_LIST_LOCAL:
            ret = app_iptv_service_get_local_list();
            break;

        case IPTV_LIST_FAV:
            ret = app_iptv_service_get_fav_list();
            break;

        default:
            {
                AppMsg_IptvList iptv_list;
                memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
                iptv_list.iptv_source = IPTV_LIST_UNKOWN;
                app_send_msg_exec(APPMSG_IPTV_LIST_DONE, &iptv_list);
            }
            break;
    }

    return ret;
}

static void _get_ad_pic_proc(void *arg)
{
    char username[CLOUD_MAX_ID_LEN + 1] = {0};
    char passwd[CLOUD_MAX_PASSWD_LEN + 1] = {0};
    char chipid[8] = {0};
    char *pic_name = *(char**)arg;
    char *pic_path = NULL;

    GxCore_ThreadDetach();

    if(pic_name == NULL)
        return;

    if(app_chip_sn_get(chipid, 8) == 0)
    {
        GxBus_ConfigGet(CLOUD_ID_KEY, username, CLOUD_MAX_ID_LEN,  CLOUD_DEFAULT_ID);
        GxBus_ConfigGet(CLOUD_PASSWD_KEY, passwd, CLOUD_MAX_PASSWD_LEN, CLOUD_DEFAULT_PASSWD);
        pic_path = app_iptv_api_download_ad_pic(username, passwd, chipid, pic_name);
    }

    thiz_get_ad_pic_thread = 0;
    app_send_msg_exec(APPMSG_IPTV_AD_PIC_DONE, &pic_path);
}

static status_t _app_iptv_service_get_ad_pic(char *pic_name)
{
    static char *pic = NULL;

    pic = pic_name;
    if(pic == NULL)
    {
        return GXCORE_ERROR;
    }

    if(thiz_get_ad_pic_thread > 0)
    {
        printf("[%s][%d]!!!!!!!!!! busy busy busy!!!!!!!!! \n", __func__, __LINE__);
        return GXCORE_ERROR;
    }

    return GxCore_ThreadCreate("iptv_get_ad_pic", &thiz_get_ad_pic_thread, _get_ad_pic_proc,
            &pic, 8 * 1024, GXOS_DEFAULT_PRIORITY);
}

static status_t AppIptvServiceInit(handle_t self,int priority_offset)
{
    handle_t sch;

    GxBus_MessageRegister(APPMSG_IPTV_GET_LIST, sizeof(AppMsg_IptvList));
    GxBus_MessageRegister(APPMSG_IPTV_LIST_DONE, sizeof(AppMsg_IptvList));
    GxBus_MessageRegister(APPMSG_IPTV_UPDATE_LOCAL_LIST, sizeof(char*));
    GxBus_MessageRegister(APPMSG_IPTV_GET_AD_PIC, sizeof(char*));
    GxBus_MessageRegister(APPMSG_IPTV_AD_PIC_DONE, sizeof(char*));

    GxBus_MessageListen(self, APPMSG_IPTV_GET_LIST);
    GxBus_MessageListen(self, APPMSG_IPTV_UPDATE_LOCAL_LIST);
    GxBus_MessageListen(self, APPMSG_IPTV_GET_AD_PIC);

    sch = GxBus_SchedulerCreate("IptvMsgScheduler", GXBUS_SCHED_MSG,
                                    1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

    return GXCORE_SUCCESS;
}

static void AppIptvServiceDestroy(handle_t self)
{
    GxBus_MessageUnListen(self, APPMSG_IPTV_GET_LIST);

    GxBus_ServiceUnlink(self);

    return;
}

static GxMsgStatus AppIptvServiceRecvMsg(handle_t self, GxMessage * msg)
{
    GxMsgStatus status = GXMSG_OK;

    switch (msg->msg_id)
    {
        case APPMSG_IPTV_GET_LIST:
            {
                AppMsg_IptvList *iptv_list = NULL;
                iptv_list = GxBus_GetMsgPropertyPtr(msg, AppMsg_IptvList);
                app_iptv_service_get_list(iptv_list->iptv_source);
            }
            break;

        case APPMSG_IPTV_UPDATE_LOCAL_LIST:
            {
                char **file_path = GxBus_GetMsgPropertyPtr(msg, char*);
                if(file_path != NULL)
                    app_iptv_service_update_local_list(*file_path);
            }
            break;

        case APPMSG_IPTV_GET_AD_PIC:
            {
                char **pic_name = GxBus_GetMsgPropertyPtr(msg, char*);
                if(pic_name != NULL)
                {
                    _app_iptv_service_get_ad_pic(*pic_name);
                }
            }
            break;

        default:
            break;
    }

    return status;
}

status_t app_iptv_service_start(void)
{
    extern handle_t g_app_msg_self;
    status_t ret = GXCORE_ERROR;
    if(GxBus_ServiceFindByName(IPTV_SERVICE_NAME) == GXCORE_INVALID_POINTER)
    {
        GxBus_ServiceCreate(&app_iptv_service);
        GxBus_MessageListen(g_app_msg_self, APPMSG_IPTV_LIST_DONE);
        GxBus_MessageListen(g_app_msg_self, APPMSG_IPTV_AD_PIC_DONE);
        ret = GXCORE_SUCCESS;
    }
    return ret;
}

#endif
