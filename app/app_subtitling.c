/*
 * =====================================================================================
 *
 *       Filename:  app_subtitling.c
 *
 *    Description:  select dvb subtitle or ttx subtitle
 *
 *        Version:  1.0
 *        Created:  2011年10月13日 10时14分49秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app.h"
#include "app_module.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_send_msg.h"
#include "full_screen.h"

#define SUBT_OFF_MODE_NUM    1
#define WND_SUBT    "wnd_subtitling"
#define LV_SUBT    "listview_subtitling"

typedef struct subtitling Subtitling;
struct subtitling
{
    int32_t     sel;
    uint32_t    mode;
    char(*lang)[8];
    char(*lang_list)[8];
    void (*magz_open)(void);
    status_t (*open)(Subtitling *, uint32_t);
    void (*close)(Subtitling *);
    void (*change)(Subtitling *);
    void (*pause)(Subtitling *);
    void (*resume)(Subtitling *);

#define SUBT_MODE_OFF   (0)
#define SUBT_MODE_TTX   (1)
#define SUBT_MODE_DVB   (2)
};

extern Subtitling g_AppSubtitling;
static event_list *timer_refresh_list = NULL;
static char s_ch_setfocus = 0;//0:Don't do set sel for LV_SUBT 1:do

static int _check_char_validity(uint8_t ch_str)//language code from ISO-8859-1
{
    uint8_t tmp = 0;

    tmp = (ch_str & 0xf0) >> 4;
    if ((tmp == 0) || (tmp == 1) || (tmp == 8) || (tmp == 9))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int _refresh_list(void *usrdata)
{
    uint32_t            i = 0;
    TtxSubtOps          *p = &g_AppTtxSubt;
    Subtitling          *p_sub = &g_AppSubtitling;
    uint32_t            size = sizeof(char *[8]) * (p->ttx_num + p->subt_num + SUBT_OFF_MODE_NUM);
    static uint32_t     last_lang = 0;
    uint32_t            lang = 0;
    int32_t             sel = 0;
    char                lang_code[3] = {0};

    p_sub->lang = GxCore_Malloc(size);
    if (p_sub->lang == NULL)
    {
        return EVENT_TRANSFER_STOP;
    }

    memset(p_sub->lang, 0, size);
    lang++;

    strncpy((char *)p_sub->lang, "OFF", 8);
    memcpy(lang_code, p->content[lang - 1].lang, sizeof(lang_code));

    for (i = 0; i < p->ttx_num; i++)
    {
        if (_check_char_validity(lang_code[0]))
        {
            lang_code[0] = 0x20;
        }
        if (_check_char_validity(lang_code[1]))
        {
            lang_code[1] = 0x20;
        }
        if (_check_char_validity(lang_code[2]))
        {
            lang_code[2] = 0x20;
        }

        sprintf((char *)(p_sub->lang + lang), "%c%c%c(T)", lang_code[0], lang_code[1], lang_code[2]);
        lang++;
    }
    for (i = 0; i < p->subt_num; i++)
    {
        if (_check_char_validity(lang_code[0]))
        {
            lang_code[0] = 0x20;
        }
        if (_check_char_validity(lang_code[1]))
        {
            lang_code[1] = 0x20;
        }
        if (_check_char_validity(lang_code[2]))
        {
            lang_code[2] = 0x20;
        }

        sprintf((char *)(p_sub->lang + lang), "%c%c%c(D)", lang_code[0], lang_code[1], lang_code[2]);
        lang++;
    }

    if (s_ch_setfocus == 1 || last_lang != lang)
    {
        if (lang == 1)
        {
            sel = 0;
            strncpy((char *)p_sub->lang, STR_ID_NO_DATA, 8);
        }
        else
        {
            GxBus_ConfigGetInt(SUBTITLE_LANG_KEY, &sel, SUBTITLE_LANG_VALUE);
            if (lang <= sel)
            {
                sel = 1;
            }
        }
        GUI_SetProperty(LV_SUBT, "select", &sel);
        GUI_SetProperty(LV_SUBT, "update_all", NULL);
    }

    s_ch_setfocus = 0;
    last_lang = lang;

    if ((timer_refresh_list != NULL) && (p->sync_flag == 1))
    {
        remove_timer(timer_refresh_list);
        timer_refresh_list = NULL;
    }

    return GXCORE_SUCCESS;
}

static void app_subt_ini_save(Subtitling *p_sub)
{
    int32_t init_value;

    init_value = p_sub->mode;
    GxBus_ConfigSetInt(SUBTITLE_MODE_KEY, init_value);
    init_value = p_sub->sel;
    GxBus_ConfigSetInt(SUBTITLE_LANG_KEY, init_value);
}
#if 0
static int app_subt_lang_set_first(int sel)
{
    int i = 0;
    char *str1 = NULL;
    TtxSubtOps *p = &g_AppTtxSubt;

    uint32_t lang = 0;
    char(*tmp_lang)[8];
    uint32_t size = sizeof(char *[8]) * (p->ttx_num + p->subt_num + SUBT_OFF_MODE_NUM);

    tmp_lang = GxCore_Malloc(size);
    if (tmp_lang == NULL)
    {
        return EVENT_TRANSFER_STOP;
    }
    memset(tmp_lang, 0, size);

    lang++;
    for (i = 0; i < p->ttx_num; i++)
    {
        if (_check_char_validity(p->content[lang - 1].lang[0]))
        {
            p->content[lang - 1].lang[0] = 0x20;
        }
        if (_check_char_validity(p->content[lang - 1].lang[1]))
        {
            p->content[lang - 1].lang[1] = 0x20;
        }
        if (_check_char_validity(p->content[lang - 1].lang[2]))
        {
            p->content[lang - 1].lang[2] = 0x20;
        }

        sprintf((char *)(tmp_lang + lang), "%c%c%c", p->content[lang - 1].lang[0],
                p->content[lang - 1].lang[1], p->content[lang - 1].lang[2]);
        lang++;
    }
    for (i = 0; i < p->subt_num; i++)
    {
        if (_check_char_validity(p->content[lang - 1].lang[0]))
        {
            p->content[lang - 1].lang[0] = 0x20;
        }
        if (_check_char_validity(p->content[lang - 1].lang[1]))
        {
            p->content[lang - 1].lang[1] = 0x20;
        }
        if (_check_char_validity(p->content[lang - 1].lang[2]))
        {
            p->content[lang - 1].lang[2] = 0x20;
        }

        sprintf((char *)(tmp_lang + lang), "%c%c%c", p->content[lang - 1].lang[0],
                p->content[lang - 1].lang[1], p->content[lang - 1].lang[2]);
        lang++;
    }

    extern char *app_get_short_lang_str(uint32_t lang);
    str1 = app_get_short_lang_str(sel);
    for (i = 1; i < p->ttx_num + p->subt_num + SUBT_OFF_MODE_NUM; i++)
    {
        if (strcmp(str1, tmp_lang[i]) == 0)
        {
            GxCore_Free(tmp_lang);
            return i;
        }
    }
    GxCore_Free(tmp_lang);
    return -1;
}
#endif
void app_ttx_magz_open(void)
{
    PopDlg pop = {0};
    GxMsgProperty_NodeByPosGet node;
    TtxSubtOps *p_ttx = &g_AppTtxSubt;

    if (g_AppPlayOps.normal_play.play_total > 0)
    {
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

        if (node.prog_data.ttx_flag == GXBUS_PM_PROG_BOOL_ENABLE)
        {
            if ((g_AppPvrOps.state == PVR_TIMESHIFT)
                    || (g_AppPvrOps.state == PVR_TMS_AND_REC))
            {
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_NO_BTN;
                pop.format = POP_FORMAT_DLG;
                pop.str = STR_ID_STOP_PVR_FIRST;
                pop.mode = POP_MODE_UNBLOCK;
                pop.timeout_sec = 3;
                popdlg_create(&pop);
            }
            else
            {
                app_subt_pause();

                GUI_EndDialog("wnd_channel_info");
                GUI_EndDialog("wnd_volume_value");

                g_AppFullArb.timer_stop();
                GUI_SetProperty("txt_full_state", "state", "hide");
                GUI_SetProperty("img_full_state", "state", "hide");
                GUI_SetProperty("txt_full_title", "state", "hide");
                GUI_SetInterface("flush", NULL);

                if (p_ttx->ttx_magz_open(p_ttx) != GXCORE_SUCCESS)
                {
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_OK;
                    pop.format = POP_FORMAT_DLG;
                    pop.str = STR_ID_NO_TELTEXT;
                    pop.mode = POP_MODE_UNBLOCK;
                    popdlg_create(&pop);
                    app_subt_resume();
                }
            }
        }
        else
        {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_NO_TELTEXT;
            pop.mode = POP_MODE_UNBLOCK;
            popdlg_create(&pop);
        }
    }
}

void app_subt_change(void)
{
    if (GUI_CheckDialog("wnd_main_menu") == GXCORE_SUCCESS
            || (GUI_CheckDialog("wnd_channel_edit") == GXCORE_SUCCESS)
            || (GUI_CheckDialog("wnd_epg") == GXCORE_SUCCESS)
            || g_AppFullArb.state.tv == STATE_OFF)
    {
        return;
    }

    g_AppSubtitling.change(&g_AppSubtitling);
}

void app_subt_pause(void)
{
    g_AppSubtitling.pause(&g_AppSubtitling);
}

void app_subt_resume(void)
{
    TtxSubtOps *p = &g_AppTtxSubt;

    if (p->ttx_num + p->subt_num > 0)
    {
        //GxCore_ThreadDelay(500);
        g_AppSubtitling.resume(&g_AppSubtitling);
    }
}

static void app_subtitling_save_extinfo(uint32_t sel)
{
    TtxSubtOps *p = &g_AppTtxSubt;
    AppProgExtInfo prog_ext_info = {{0}};
    GxMsgProperty_NodeByPosGet node = {0};

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;

    if (g_AppFullArb.state.pause == STATE_ON)
    {
        return;
    }

    if (sel > p->ttx_num + p->subt_num || sel == 0)
    {
        return;
    }

    if (GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        memcpy(&prog_ext_info.subt, &p->content[sel - SUBT_OFF_MODE_NUM], sizeof(ttx_subt));
        GxBus_PmProgExtInfoAdd(node.prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info);
        GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);
    }

    return;
}

static status_t app_subtitling_open(Subtitling *p_sub, uint32_t sel)
{
    status_t ret = GXCORE_ERROR;
    TtxSubtOps *p = &g_AppTtxSubt;

    if (g_AppFullArb.state.pause == STATE_ON)
    {
        return GXCORE_ERROR;
    }

    if (sel > p->ttx_num + p->subt_num || sel == 0)
    {
        // sel = 0 mean MODE_OFF
        return GXCORE_ERROR;
    }

    //exit the pre-process for release the TTX memory for subt
    g_AppTtxSubt.ttx_pre_process_close(&g_AppTtxSubt);

    if (p_sub->mode == SUBT_MODE_TTX)
    {
        p->ttx_subt_close(p);
    }

    if (p_sub->mode == SUBT_MODE_DVB)
    {
        p->dvb_subt_close(p);
    }

    if (sel > p->ttx_num)
    {
        // dvb_subt
        ret = p->dvb_subt_open(p, sel - SUBT_OFF_MODE_NUM - p->ttx_num);
        if (ret == GXCORE_SUCCESS)
        {
            p_sub->mode = SUBT_MODE_DVB;
        }
    }
    else
    {
        // ttx_subt
        ret = p->ttx_subt_open(p, sel - SUBT_OFF_MODE_NUM);
        if (ret == GXCORE_SUCCESS)
        {
            p_sub->mode = SUBT_MODE_TTX;
        }
    }

    if (ret == GXCORE_SUCCESS)
    {
        g_AppFullArb.state.subt = STATE_ON;

        p_sub->sel = sel;
        app_subt_ini_save(p_sub);
    }

    return ret;
}

static void app_subtitling_close(Subtitling *p_sub)
{
    TtxSubtOps *p = &g_AppTtxSubt;

    if (p_sub->mode == SUBT_MODE_TTX)
    {
        p->ttx_subt_close(p);
    }

    if (p_sub->mode == SUBT_MODE_DVB)
    {
        p->dvb_subt_close(p);
    }

    p_sub->sel = 0;
    p_sub->mode = SUBT_MODE_OFF;
    //p->subt_num = 0;
    //p->ttx_num= 0;
    app_subt_ini_save(p_sub);

    g_AppFullArb.state.subt = STATE_OFF;

    //restart the pre-process, if APP support the function.
    g_AppTtxSubt.ttx_pre_process_init(&g_AppTtxSubt);
}

static void app_subtitling_change(Subtitling *p_sub)
{
    int32_t mode = 0;
    int32_t lang = 0;
    uint32_t i = 0;
    AppProgExtInfo prog_ext_info = {{0}};
    GxMsgProperty_NodeByPosGet node = {0};

    TtxSubtOps *p = &g_AppTtxSubt;
    uint32_t lang_total = p->ttx_num + p->subt_num;
    if (lang_total == 0)
    {
        return;
    }

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;

    GxBus_ConfigGetInt(SUBTITLE_MODE_KEY, &mode, SUBTITLE_MODE_VALUE);
    if (mode != SUBTITLE_MODE_VALUE)
    {
        if (GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
        {
            if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info))
            {
                for (i = 0; i < lang_total; i++)
                {
                    if (i < p->ttx_num)
                    {
                        if (prog_ext_info.subt.elem_pid == p->content[i].elem_pid
                                && strncmp((const char *)prog_ext_info.subt.lang, (const char *)p->content[i].lang, sizeof(prog_ext_info.subt.lang)) == 0
                                && prog_ext_info.subt.ttx.magazine == p->content[i].ttx.magazine
                                && prog_ext_info.subt.ttx.page == p->content[i].ttx.page)
                        {
                            break;
                        }

                    }
                    else
                    {
                        if (prog_ext_info.subt.elem_pid == p->content[i].elem_pid
                                && strncmp((const char *)prog_ext_info.subt.lang, (const char *)p->content[i].lang, sizeof(prog_ext_info.subt.lang)) == 0
                                && prog_ext_info.subt.subt.composite_page_id == p->content[i].subt.composite_page_id
                                && prog_ext_info.subt.subt.ancillary_page_id == p->content[i].subt.ancillary_page_id)
                        {
                            break;
                        }

                    }
                }

                lang = (i == lang_total) ? 0 : i;
            }
            else
            {
                lang = 0;
            }
        }
        GxBus_ConfigSetInt(SUBTITLE_LANG_KEY, lang + SUBT_OFF_MODE_NUM);

        p_sub->open(p_sub, lang + SUBT_OFF_MODE_NUM);
    }
}

static void app_subtitling_pause(Subtitling *p_sub)
{
    TtxSubtOps *p = &g_AppTtxSubt;

    if (p_sub->mode == SUBT_MODE_TTX)
    {
        p->ttx_subt_close(p);
    }

    if (p_sub->mode == SUBT_MODE_DVB)
    {
        p->dvb_subt_close(p);
    }
}

static void app_subtitling_resume(Subtitling *p_sub)
{
    int32_t lang = 0;
    TtxSubtOps *p = &g_AppTtxSubt;

    if (p_sub->mode != SUBT_MODE_OFF
            && p->ttx_num + p->subt_num > lang)
    {
        GxBus_ConfigGetInt(SUBTITLE_LANG_KEY, &lang, SUBTITLE_LANG_VALUE);
        p_sub->open(p_sub, lang);
    }
}

SIGNAL_HANDLER int app_subtitling_list_get_total(GuiWidget *widget, void *usrdata)
{
    TtxSubtOps *p = &g_AppTtxSubt;

    return p->ttx_num + p->subt_num + SUBT_OFF_MODE_NUM;
}

SIGNAL_HANDLER int app_subtitling_list_get_data(GuiWidget *widget, void *usrdata)
{
#define SUB_NUM_LEN     (10)
    static char buffer[SUB_NUM_LEN];
    ListItemPara *item = NULL;
    Subtitling  *p_sub = &g_AppSubtitling;

    item = (ListItemPara *)usrdata;
    if (NULL == item)
    {
        return GXCORE_ERROR;
    }
    if (0 > item->sel)
    {
        return GXCORE_ERROR;
    }

    // col-0: num
    item->x_offset = 0;
    item->image = NULL;
    if (item->sel > 0)
    {
        memset(buffer, 0, SUB_NUM_LEN);
        sprintf(buffer, "%d", item->sel);
        item->string = buffer;
    }
    else
    {
        item->string = NULL;
    }

    //col-1: channel name
    item = item->next;
    item->image = NULL;
    item->string = p_sub->lang[item->sel];

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_subtitling_list_create(GuiWidget *widget, void *usrdata)
{
    TtxSubtOps          *p = &g_AppTtxSubt;

    s_ch_setfocus = 1;
    _refresh_list(NULL);

    if (p->sync_flag == 0)
    {
        timer_refresh_list = create_timer(_refresh_list, 2000, 0, TIMER_REPEAT);
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_subtitling_list_destroy(GuiWidget *widget, void *usrdata)
{
    Subtitling  *p_sub = &g_AppSubtitling;

    GxCore_Free(p_sub->lang);

    if (timer_refresh_list)
    {
        remove_timer(timer_refresh_list);
        timer_refresh_list = NULL;
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_subtitling_list_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    Subtitling  *p_sub = &g_AppSubtitling;
    int ret = EVENT_TRANSFER_KEEPON;
    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch (event->key.sym)
            {
#if DLNA_SUPPORT
                case VK_DLNA_TRIGGER:
                    GUI_EndDialog(WND_SUBT);
                    GUI_SendEvent("wnd_full_screen", event);
                    break;
#endif
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_SUBT);
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_SUBT);
                    GUI_SetProperty("wnd_full_screen", "update", NULL);
                    app_create_dialog("wnd_channel_info");
#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
                    ret = EVENT_TRANSFER_STOP;
#endif
                    break;

                case STBK_OK:
                {
                    int32_t sel;
                    TtxSubtOps *p = &g_AppTtxSubt;
                    uint32_t total = p->ttx_num + p->subt_num;

                    GUI_GetProperty(LV_SUBT, "select", &sel);

                    if (total != 0)
                    {
                        if (sel == 0)
                        {
                            p_sub->close(p_sub);
                        }
                        else
                        {
                            app_subtitling_save_extinfo(sel);
                            p_sub->open(p_sub, sel);
                        }
                    }

                    GUI_EndDialog(WND_SUBT);
                    GUI_SetProperty("wnd_full_screen", "update", NULL);
                }
                break;
                default:
                    break;
            }
    }

    return ret;
}

SIGNAL_HANDLER int app_subtitling_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_subtitling_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_subtitling_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_KEYDOWN:
            switch (event->key.sym)
            {
#if DLNA_SUPPORT
                case VK_DLNA_TRIGGER:
                    GUI_EndDialog(WND_SUBT);
                    GUI_SendEvent("wnd_full_screen", event);
                    break;
#endif
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_SUBT);
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

Subtitling g_AppSubtitling =
{
    .magz_open          = app_ttx_magz_open,
    .open    = app_subtitling_open,
    .close   = app_subtitling_close,
    .change  = app_subtitling_change,
    .pause   = app_subtitling_pause,
    .resume  = app_subtitling_resume,

};
