#include "app.h"
#if (DEMOD_DVB_T > 0)
#include "full_screen.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_dvbt2_search.h"
#include "app_wnd_system_setting_opt.h"
#include "app_module.h"

#define LANG_NAME_MAX   (16)

enum BOX_GUIDE_ITEM_NAME
{
    T2_GUIDE_BOX_ITEM_LANG = 0,
    T2_GUIDE_BOX_ITEM_COUNTRY,
    T2_GUIDE_BOX_ITEM_FTA,
    T2_GUIDE_BOX_ITEM_SEARCH,
};

#define CMB_T2_INSTALLATION_COUNTRY	"cmb_t2_installation_country"
#define CMB_T2_INSTALLATION_LANGUAGE	"cmb_t2_installation_language"
#define CMB_T2_INSTALLATION_MODE	"cmb_t2_installation_searchsel"

#define BOX_T2_INSTALLATION_OPT		"box_t2_installation_opt"

SIGNAL_HANDLER int app_t2_installation_create(const char* widgetname, void *usrdata)
{
    static char s_country_array[ITEM_COUNTRY_MAX*MAX_COUNTRY_NAME_LEN] = {0};
    int i=0;
    strcpy(s_country_array, "[");
    for (i=0; i<ITEM_COUNTRY_MAX; i++)
    {
        strcat(s_country_array, AppDvbt2GetCountryName(i));
        strcat(s_country_array, ",");
    }
    strcat(s_country_array, "]");

    GUI_SetProperty(CMB_T2_INSTALLATION_COUNTRY,"content",s_country_array);

    char s_lang_array[LANG_MAX*LANG_NAME_MAX] = {0};
    strcpy(s_lang_array, "[");
    for (i=0; i<ITEM_COUNTRY_MAX; i++)
    {
        strcat(s_lang_array, g_LangName[i]);
        strcat(s_lang_array, ",");
    }
    strcat(s_lang_array, "]");

    GUI_SetProperty(CMB_T2_INSTALLATION_LANGUAGE,"content",s_lang_array);


    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_t2_installation_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    uint32_t box_sel;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case STBK_EXIT:
                case STBK_MENU:
                    {
                        GUI_EndDialog("wnd_t2_installation_guide");
                        GUI_SetInterface("flush",NULL);

                        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);//目的处理进节目编辑后退出导致主界面变灰的问题
                        g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);

                        if (g_AppPlayOps.normal_play.play_total != 0)
                        {
                            // deal for recall , current will copy to recall
#if RECALL_LIST_SUPPORT
                            //extern int g_recall_count;
                            g_AppPlayOps.current_play = g_AppPlayOps.recall_play[0];//g_recall_count
#else
                            g_AppPlayOps.current_play = g_AppPlayOps.recall_play;
#endif
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
                    }
                    break;

                case STBK_OK:
                    {
                        GUI_GetProperty(BOX_T2_INSTALLATION_OPT, "select", &box_sel);
                        if(T2_GUIDE_BOX_ITEM_SEARCH == box_sel)
                        {
                            uint8_t sel = 0;
                            uint8_t modes = 0;
                            GUI_GetProperty(CMB_T2_INSTALLATION_MODE, "select", &sel);
                            if(0 == sel)//no: all
                                modes = GX_SEARCH_FTA_CAS_ALL;
                            else if(1 == sel)//yes: fta only
                                modes = GX_SEARCH_FTA;

                            GUI_EndDialog("wnd_t2_installation_guide");
                            AppT2AutoSearchStart(modes);
                        }
                    }
                    break;

                default:
                    break;
            }

        default:
            break;
    }
    return ret;
}


SIGNAL_HANDLER int app_language_cmbox_change(GuiWidget *widget, void *usrdata)
{
    //printf("language : %s\n",g_LangName[sel]);
    int sel = 0;
    GUI_GetProperty(CMB_T2_INSTALLATION_LANGUAGE, "select", &sel);
    GUI_SetInterface("osd_language",g_LangName[sel]);
    app_system_set_wnd_update();
    GxBus_ConfigSetInt(OSD_LANG_KEY,sel);	//save
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_country_cmbox_change(GuiWidget *widget, void *usrdata)
{
    //printf("language : %s\n",g_LangName[sel]);
    int sel = 0;
    GUI_GetProperty(CMB_T2_INSTALLATION_COUNTRY, "select", &sel);
    AppDvbt2SetCountry(sel);

    return EVENT_TRANSFER_KEEPON;
}

#else //DEMOD_DVB_T
SIGNAL_HANDLER int app_t2_installation_create(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_t2_installation_box_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_language_cmbox_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_country_cmbox_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

#endif
