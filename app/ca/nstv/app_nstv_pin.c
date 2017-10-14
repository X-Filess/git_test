/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_pin.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.09.27		   B.Z.			  Creation
*****************************************************************************/
#include "app_module.h"
#include "app_config.h"
#include "app_pop.h"

#if NSCAS_SUPPORT
#define WID_PIN                     "wnd_nstv_pin"
#define XML_PIN                     "ca/nstv/wnd_nstv_pin.xml"
#define BOX_PIN                     "box_nstv_pin"
#define TXT_PIN_TITLE               "txt_nstv_pin_title"
#define EDIT_PIN_OLD                "edit_nstv_pin_old"
#define EDIT_PIN_NEW                "edit_nstv_pin_new"
#define EDIT_PIN_CONFIRM            "edit_nstv_pin_confirm"
#define BXI_pin_1                   "bxi_nstv_pin_1"
#define BXI_pin_2                   "bxi_nstv_pin_2"
#define BXI_pin_3                   "bxi_nstv_pin_3"

#define NSTV_PIN_KEY                "nscas>pin"

#define BMP_BAR_LONG_FOCUS          "s_bar_long_focus.bmp"
#define BMP_BAR_LONG_UNFOCUS        ""

static int _set_pin(unsigned char *old_pin, unsigned char *new_pin)
{
    CASCAMDataSetClass data_set;
    CASCAMNSTVPINControlClass pin_info;

    if((NULL == old_pin) || (NULL == new_pin))
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    memset(&pin_info, 0, sizeof(CASCAMNSTVPINControlClass));
    memset(&data_set, 0, sizeof(CASCAMDataSetClass));
    // old pin
    memcpy(pin_info.old_pin, old_pin, 6);
    // new pin
    memcpy(pin_info.new_pin, new_pin, 6);

    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = PIN_CONTROL;
    data_set.sub_id = NSTV_PIN_SET;
    data_set.data_buf = (void*)&pin_info;
    data_set.buf_len = sizeof(CASCAMNSTVPINControlClass);
    if(0 == CASCAM_Set_Property(&data_set))
    {
        unsigned char old_pin_temp[7] = {0};
		unsigned char new_pin_temp[7] = {0};
		memcpy(old_pin_temp, old_pin, 6);
		memcpy(new_pin_temp, new_pin, 6);
        printf("\n--------------------------------------------\n");
        printf("\033[32m pin control:\n\033[0m");
        printf("\033[32m pin:\033[0m \033[33mold_pin = %s, new_pin = %s\n\033[0m", old_pin_temp, new_pin_temp);

        // notice the format, now use the strings just like "000000"
        // GxBus_ConfigSet(NSTV_PIN_KEY, (const char*)new_pin);
        GUI_SetProperty(EDIT_PIN_OLD, "clear", NULL);
        GUI_SetProperty(EDIT_PIN_NEW, "clear", NULL);
        GUI_SetProperty(EDIT_PIN_CONFIRM, "clear", NULL);
        return 0;
    }
//
   GUI_SetProperty(EDIT_PIN_OLD, "clear", NULL);
   GUI_SetProperty(EDIT_PIN_NEW, "clear", NULL);
   GUI_SetProperty(EDIT_PIN_CONFIRM, "clear", NULL);
    return -1;

}

static int _pin_save(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {// need to save
        char *old_pin = NULL;
        char *new_pin = NULL;

        GUI_GetProperty(EDIT_PIN_OLD, "string", &old_pin);
        GUI_GetProperty(EDIT_PIN_NEW, "string", &new_pin);

        if(_set_pin((unsigned char*)old_pin, (unsigned char*)new_pin) < 0)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.mode = POP_MODE_BLOCK;
            pop.type = POP_TYPE_OK;
            pop.str = NSTV_OSD_INVALID_PIN;
            popdlg_create(&pop);
            return -1;
        }
        else
        {
            PopDlg  pop;

            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.str = NSTV_OSD_CHANGE_PIN;
            pop.timeout_sec = 1;
            popdlg_create(&pop);

        }
    }

   return 0;
}

static int _passwd_save(void)
{
    char *old_pin = NULL;
    char *new_pin = NULL;
    char *confirm_pin = NULL;
    // level
    GUI_GetProperty(EDIT_PIN_OLD, "string", &old_pin);
    GUI_GetProperty(EDIT_PIN_NEW, "string", &new_pin);
    GUI_GetProperty(EDIT_PIN_CONFIRM, "string", &confirm_pin);
    if(((NULL == old_pin) || (0 == strlen(old_pin)))
                && ((NULL == new_pin) || (0 == strlen(new_pin)))
                && ((NULL == confirm_pin) || (0 == strlen(confirm_pin))))
    {
        // no modify
        return 0;
    }

    if((strlen(old_pin) != 6)
            || (strncmp(new_pin, confirm_pin, 6) != 0)
            || (strlen(new_pin) != 6)
            || (strlen(confirm_pin) != 6))
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.mode = POP_MODE_BLOCK;
        pop.type = POP_TYPE_OK;
        pop.str = NSTV_OSD_INVALID_PIN;
        popdlg_create(&pop);
        GUI_SetProperty(EDIT_PIN_OLD, "clear", NULL);
        GUI_SetProperty(EDIT_PIN_NEW, "clear", NULL);
        GUI_SetProperty(EDIT_PIN_CONFIRM, "clear", NULL);
        return -1;
    }

    if(strncmp(old_pin, new_pin, 6))
    {// need to save
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.mode = POP_MODE_BLOCK;
        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_SAVE_INFO;
        pop.exit_cb = _pin_save;
        popdlg_create(&pop);
        return 1;
    }
    return 0;
}

static void _create_pin_menu(void)
{
    static char init_flag = 0;

    if(0 == init_flag)
    {
        GUI_LinkDialog(WID_PIN, XML_PIN);
        init_flag = 1;
    }
    app_create_dialog(WID_PIN);

    //
    GUI_SetProperty(EDIT_PIN_OLD, "default_intaglio", "-");
    GUI_SetProperty(EDIT_PIN_OLD, "intaglio", "*");
    //
    GUI_SetProperty(EDIT_PIN_NEW, "default_intaglio", "-");
    GUI_SetProperty(EDIT_PIN_NEW, "intaglio", "*");
    //
    GUI_SetProperty(EDIT_PIN_CONFIRM, "default_intaglio", "-");
    GUI_SetProperty(EDIT_PIN_CONFIRM, "intaglio", "*");


}

void app_nstv_pin_exec(void)
{
    _create_pin_menu();
}

// for ui
SIGNAL_HANDLER int On_wnd_nstv_pin_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

static char *thiz_item_widget[] = {
    BXI_pin_1,
    BXI_pin_2,
    BXI_pin_3,
};

SIGNAL_HANDLER int On_wnd_nstv_pin_got_focus(GuiWidget *widget, void *usrdata)
{
    int box_sel = 0;
    GUI_GetProperty(BOX_PIN, "select", &box_sel);
	GUI_SetProperty(thiz_item_widget[box_sel], "unfocus_image", BMP_BAR_LONG_UNFOCUS);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_pin_lost_focus(GuiWidget *widget, void *usrdata)
{
    int box_sel = 0;
    GUI_GetProperty(BOX_PIN, "select", &box_sel);
	GUI_SetProperty(thiz_item_widget[box_sel], "unfocus_image", BMP_BAR_LONG_FOCUS);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_pin_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
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
#if DLNA_SUPPORT
                case VK_DLNA_TRIGGER:
                    app_dlna_ui_exec(NULL, NULL);
                    break;
#endif
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    {
                        GUI_EndDialog(WID_PIN);
                    }
                    break;
                case STBK_OK:
                    {
                        _passwd_save();

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
SIGNAL_HANDLER int On_wnd_nstv_pin_old_reach_end(GuiWidget *widget, void *usrdata)
{
        int box_sel = 1;
        GUI_SetProperty(BOX_PIN, "select", &box_sel);
        return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int On_wnd_nstv_pin_new_reach_end(GuiWidget *widget, void *usrdata)
{
        int box_sel = 2;
        GUI_SetProperty(BOX_PIN, "select", &box_sel);
        return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int On_wnd_nstv_pin_confirmpasswd_reach_end(GuiWidget *widget, void *usrdata)
{
    //_passwd_save();
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int On_wnd_nstv_pin_confirmpasswd_keypress(GuiWidget *widget, void *usrdata)
{

	return EVENT_TRANSFER_KEEPON;
}

#else
SIGNAL_HANDLER int On_wnd_nstv_pin_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_pin_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_pin_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_pin_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_pin_confirmpasswd_reach_end(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_pin_confirmpasswd_keypress(GuiWidget *widget, void *usrdata)
{

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_pin_old_reach_end(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_KEEPON;
}
SIGNAL_HANDLER int On_wnd_nstv_pin_new_reach_end(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_KEEPON;
}
#endif
