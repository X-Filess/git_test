/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_ota_loader_pop.c
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

#if (UPDATE_SUPPORT_FLAG > 0)
#include "app_module.h"

#define WND_OTA_LOADER_POP                  "wnd_ota_loader_pop"
#define XML_OTA_LOADER_POP_PATH             "ota_loader/wnd_ota_loader_pop.xml"

#define TXT_OTA_LOADER_POP_TITLE            "txt_ota_loader_pop_title"
#define TXT_OTA_LOADER_POP_CONTENT_LONG     "txt_ota_loader_pop_content_long"


extern int app_loader_ota_set_update_flag(unsigned char update_flag);

// static 
// export
int app_ota_loader_pop_exec(void)
{
    static char init_flag = 0;
    char *str = NULL;
    str = GxOem_GetValue("update", "update_status");
    if((NULL == str) || (strncmp(str, "failed", 6)))
    {
        return -1;
    }
    app_loader_ota_set_update_flag(0);// clear the flag
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_OTA_LOADER_POP, XML_OTA_LOADER_POP_PATH);
    }
    GUI_CreateDialog(WND_OTA_LOADER_POP);
    return 0;
}

SIGNAL_HANDLER int On_wnd_ota_loader_pop_destroy(GuiWidget *widget, void *usrdata)
{

    return 0;
}

SIGNAL_HANDLER int On_wnd_ota_loader_pop_create(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(TXT_OTA_LOADER_POP_CONTENT_LONG, "string", STR_ID_OTA_UPDATE_FAILED);
    return 0;
}

SIGNAL_HANDLER int On_wnd_ota_loader_pop_keypress(GuiWidget *widget, void *usrdata)
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
                case STBK_UP:
                case STBK_DOWN:
                    ret = EVENT_TRANSFER_STOP;
                    break;
                case STBK_OK:
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_OTA_LOADER_POP);
	                app_create_dialog("wnd_full_screen");
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

SIGNAL_HANDLER int On_wnd_ota_loader_pop_destroy(GuiWidget *widget, void *usrdata)
{
    return 0;
}

SIGNAL_HANDLER int On_wnd_ota_loader_pop_create(GuiWidget *widget, void *usrdata)
{
    return 0;
}

SIGNAL_HANDLER int On_wnd_ota_loader_pop_keypress(GuiWidget *widget, void *usrdata)
{
    return 0;
}
#endif

