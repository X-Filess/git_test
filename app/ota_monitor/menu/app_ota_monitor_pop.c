/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_ota_monitor_pop.c
* Author    :	B.Z.
* Project   :	OTA
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.11.11		   B.Z.			  Creation
*****************************************************************************/
#include "app_config.h"
#include "app.h"

#if (OTA_MONITOR_SUPPORT > 0)
#include "app_module.h"
#include "module/ota/app_ota_config.h"

#define WND_OTA_MONITOR_POP                  "wnd_ota_monitor_pop"
#define XML_OTA_MONITOR_POP_PATH             "ota/wnd_ota_monitor_pop.xml"

#define TXT_OTA_MONITOR_POP_TITLE            "txt_ota_monitor_pop_title"
#define TXT_OTA_MONITOR_POP_CONTENT_LONG     "txt_ota_monitor_pop_content_long"
#define BTN_OTA_MONITOR_POP_CONTROL1         "btn_ota_monitor_pop_control1"
#define BTN_OTA_MONITOR_POP_CONTROL2         "btn_ota_monitor_pop_control2"

// static 
static int thiz_new_version = 0;
static OTAUpdateClass thiz_ota_update = {0};

static int _update_immediately(void)
{// if have loader ota, will reboot now, otherwise go to menu ota
    int ret = 0;
#if (UPDATE_SUPPORT_FLAG > 0)
extern void app_update_reboot(void);
    // use loader ota
	//GxOem_SetValue("update", "flag", "yes");
    //GxOem_Save();
    if(thiz_ota_update.event_flag == 0)
    {// need update the fre
        unsigned int pid = 0;
        unsigned int fre = 0;
        unsigned int sym = 0;
        unsigned int pol = 0;
        unsigned int temp = 0;
        // pid
        ret = app_ota_get_data_pid(&pid);
        if((0 == ret) && (pid != thiz_ota_update.pid) && (thiz_ota_update.pid != 8191))
        {// set pid
            app_ota_set_data_pid(thiz_ota_update.pid);
        }
        else if((0 != ret) && ((thiz_ota_update.pid == 0) || (thiz_ota_update.pid != 8191)))
        {
            printf("\nWARNING, %s, %d, no pid\n", __func__,__LINE__);
            return -1;
        }
        // fre
        ret = app_ota_get_fre(&fre);
        if((thiz_ota_update.fre > 10000) && (thiz_ota_update.fre < 11700))
        {
            temp = (thiz_ota_update.fre - 9750)*1000;
        }
        else if(thiz_ota_update.fre > 11700)
        {
            temp = (thiz_ota_update.fre - 10600)*1000;
        }
        else
        {
            temp = (5150 - thiz_ota_update.fre)*1000;
        }
        if((0 == ret) && (fre != temp) && (thiz_ota_update.fre != 0))
        {
            app_ota_set_fre(temp);
        }
        else
        {
            printf("\nWARNING, %s, %d, no fre\n", __func__,__LINE__);
        }
        // pol
        ret = app_ota_get_pol(&pol);
        if((0 == ret) && (pol != thiz_ota_update.pol) && (thiz_ota_update.pol < 3))
        {
            app_ota_set_pol(thiz_ota_update.pol);
        }
        else
        {
            printf("\nWARNING, %s, %d, no pol\n", __func__,__LINE__);
        }
        // sym
        ret = app_ota_get_sym(&sym);
        if((0 == ret) && (sym != (thiz_ota_update.sym*1000)) && (0 != thiz_ota_update.sym))
        {
            app_ota_set_sym((thiz_ota_update.sym*1000));
        }
        else
        {
            printf("\nWARNING, %s, %d, no sym\n", __func__,__LINE__);
        }
    }
    app_ota_set_new_detect_version(thiz_new_version);
    app_ota_set_update_flag(1);
    app_update_reboot();
#endif
#if (OTA_SUPPORT > 0)
    //GxBus_ConfigSetInt("ota>flag", 0);
#if 0
    if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_main_menu"))
    {
        GUI_EndDialog("after wnd_main_menu");
    }
    else if (GXCORE_SUCCESS == GUI_CheckDialog("wnd_full_screen"))
    {
        // CHECK PVR

        // CHECK PLAY
    }
    app_create_dialog("wnd_ota_upgrade");
#endif
#endif
    return ret;
}

static int _update_later(void)
{// if have loader ota, will update next reboot.
    int ret = 0;
#if (UPDATE_SUPPORT_FLAG > 0)
	//GxOem_SetValue("update", "flag", "yes");
    //GxOem_Save();
#endif
#if (OTA_SUPPORT > 0) // menu ota 
    //GxBus_ConfigSetInt("ota>flag", 0);// clear flag
#endif
    app_ota_set_menual_flag();
    return ret;
}

// export
int app_ota_monitor_pop_exec(void *param)
{
    static char init_flag = 0;
    if(NULL == param)
    {
        printf("\nERROR, %s, %d\n", __func__, __LINE__);
        return -1;
    }
    OTAUpdateClass *p = (OTAUpdateClass*)param;
    memcpy(&thiz_ota_update, p, sizeof(OTAUpdateClass));
    thiz_new_version = thiz_ota_update.new_version;
    if( p->update_type == 2)// force
    {
        _update_immediately();
        return 0;
    }
    else
    {
        if(app_ota_get_manual_flag())
        {
            return 0;
        }
    }
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_OTA_MONITOR_POP, XML_OTA_MONITOR_POP_PATH);
    }
    if(GXCORE_SUCCESS != GUI_CheckDialog(WND_OTA_MONITOR_POP))
    {
        app_create_dialog(WND_OTA_MONITOR_POP);
    }
    return 0;
}

#if (UPDATE_SUPPORT_FLAG > 0)
static int thiz_focus_item = 1;
static char *thiz_control_widget[] = {
    BTN_OTA_MONITOR_POP_CONTROL1,
    BTN_OTA_MONITOR_POP_CONTROL2,
};
#endif

SIGNAL_HANDLER int On_wnd_ota_monitor_pop_destroy(GuiWidget *widget, void *usrdata)
{
    return 0;
}

SIGNAL_HANDLER int On_wnd_ota_monitor_pop_create(GuiWidget *widget, void *usrdata)
{
#if (UPDATE_SUPPORT_FLAG > 0)
    GUI_SetProperty(TXT_OTA_MONITOR_POP_CONTENT_LONG, "string", STR_ID_OTA_MONITOR_NOTICE);
    thiz_focus_item = 1;
    GUI_SetFocusWidget(thiz_control_widget[thiz_focus_item]);
#else// only for menu OTA
    GUI_SetProperty(TXT_OTA_MONITOR_POP_CONTENT_LONG, "string", STR_ID_OTA_MONITOR_NOTICE1);
    GUI_SetProperty(BTN_OTA_MONITOR_POP_CONTROL1, "state", "hide");
    GUI_SetProperty(BTN_OTA_MONITOR_POP_CONTROL2, "string", "Ok");
    GUI_SetFocusWidget(BTN_OTA_MONITOR_POP_CONTROL2);
#endif
    return 0;
}

SIGNAL_HANDLER int On_wnd_ota_monitor_pop_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
                case STBK_LEFT:
                case STBK_RIGHT:
#if (UPDATE_SUPPORT_FLAG > 0)
                    if(0 == thiz_focus_item)
                    {
                        thiz_focus_item = 1;
                    }
                    else
                    {
                        thiz_focus_item = 0;
                    }
                    GUI_SetFocusWidget(thiz_control_widget[thiz_focus_item]);
#endif
                    ret = EVENT_TRANSFER_STOP;
                    break;
                case STBK_OK:
#if (UPDATE_SUPPORT_FLAG > 0)
                    if(0 == thiz_focus_item)// immediately
                    {
                        _update_immediately();
                    }
                    else
                    {
                        GUI_EndDialog(WND_OTA_MONITOR_POP);
                        _update_later();
                    }
#else
                    GUI_EndDialog(WND_OTA_MONITOR_POP);
                     _update_later();

#endif
                    ret = EVENT_TRANSFER_STOP;
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_OTA_MONITOR_POP);
                    _update_later();
                    ret = EVENT_TRANSFER_STOP;
                    break;
                case STBK_UP:
                case STBK_DOWN:
                    ret = EVENT_TRANSFER_STOP;
                    break;
				default:
					break;
			}
		default:
			break;
	}
	return ret;
}
#else

SIGNAL_HANDLER int On_wnd_ota_monitor_pop_destroy(GuiWidget *widget, void *usrdata)
{
    return 0;
}

SIGNAL_HANDLER int On_wnd_ota_monitor_pop_create(GuiWidget *widget, void *usrdata)
{
    return 0;
}

SIGNAL_HANDLER int On_wnd_ota_monitor_pop_keypress(GuiWidget *widget, void *usrdata)
{
    return 0;
}
#endif

