/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cmm_cascam_dialog1.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.10.05		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_config.h"
#include "app_cascam_pop.h"

#if CASCAM_SUPPORT
#define WND_CMM_DIALOG                  "wnd_cmm_dialog1"
#define XML_CMM_DIALOG_PATH             "ca/wnd_cmm_dialog1.xml"

#define TXT_CMM_DIALOG_TITLE            "txt_cmm_dialog1_title"
#define TXT_CMM_DIALOG_CONTENT1         "txt_cmm_dialog1_content1"
#define TXT_CMM_DIALOG_CONTENT2         "txt_cmm_dialog1_content2"
#define TXT_CMM_DIALOG_CONTENT3         "txt_cmm_dialog1_content3"
#define TXT_CMM_DIALOG_CONTENT4         "txt_cmm_dialog1_content4"
#define TXT_CMM_DIALOG_CONTENT_LONG     "txt_cmm_dialog1_content_long"
#define BOX_CMM_DIALOG_CONTROL          "box_cmm_dialog1_control"
#define BXI_CMM_DIALOG_CONTROL1         "bxi_cmm_dialog1_control1"
#define BXI_CMM_DIALOG_CONTROL2         "bxi_cmm_dialog1_control2"
#define BXI_CMM_DIALOG_CONTROL3         "bxi_cmm_dialog1_control3"
#define BTN_CMM_DIALOG_CONTROL1         "btn_cmm_dialog1_control1"
#define BTN_CMM_DIALOG_CONTROL2         "btn_cmm_dialog1_control2"
#define BTN_CMM_DIALOG_CONTROL3         "btn_cmm_dialog1_control3"

static CmmDialogClass thiz_dialog_para = {0};
static CmmDialogRetEnum thiz_dialog_ret = DIALOG_RET_NONE;
static event_list  *thiz_dialog_timer = NULL; 

static int _exit_dialog(void)
{
    if((DIALOG_RET_OK == thiz_dialog_ret)
        || (DIALOG_RET_YES == thiz_dialog_ret))
    {
        if(thiz_dialog_para.exit_cb)
        {
            thiz_dialog_para.exit_cb(thiz_dialog_ret);
        }
    }
    else if(DIALOG_RET_NO == thiz_dialog_ret)
    {
        // TODO: ...
    }
    memset(&thiz_dialog_para, 0, sizeof(thiz_dialog_para));
    thiz_dialog_ret = DIALOG_RET_NONE;

    if(thiz_dialog_timer != NULL)
    {
        remove_timer(thiz_dialog_timer);
        thiz_dialog_timer = NULL;
    }

    return 0;
}

static int _create_dialog(void)
{
    if(thiz_dialog_para.create_cb)
    {
        thiz_dialog_para.create_cb();
    }
    return 0;
}

static int _timeout_dialog(void *usrdata)
{
    if(thiz_dialog_para.timeout_cb)
    {
        thiz_dialog_para.timeout_cb();
    }
    GUI_EndDialog(WND_CMM_DIALOG);
    return 0;
}

void cmm_create_dialog1(CmmDialogClass *para)
{
    static char init_flag = 0;

    char *focus_widget = (char *)GUI_GetFocusWidget();
    if(NULL == para)
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }

    if(0 == init_flag)
    {
        GUI_LinkDialog(WND_CMM_DIALOG, XML_CMM_DIALOG_PATH);
        init_flag = 1;
    }
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_CMM_DIALOG))
    {// exist
        if(memcmp(para, &thiz_dialog_para, sizeof(CmmDialogClass)))
        {
            GUI_EndDialog(WND_CMM_DIALOG);
        }
        else
        {
            return ;
        }
    }
// init parameters
    memcpy(&thiz_dialog_para, para, sizeof(CmmDialogClass));
// init timer
    if(thiz_dialog_timer != NULL)
    {
        remove_timer(thiz_dialog_timer);
        thiz_dialog_timer = NULL;
    }
// create dialog
    app_create_dialog(WND_CMM_DIALOG);
// for no focus dialog
    if(DIALOG_NO_CONTROL == thiz_dialog_para.type)
    {
        GUI_SetInterface("flush", NULL);
        GUI_SetFocusWidget(focus_widget);
    }
// for create callback
    _create_dialog();
// for timeout timer
    if(0 < thiz_dialog_para.timeout_sec)
    {
        if (reset_timer(thiz_dialog_timer) != 0) 
        {
            thiz_dialog_timer = create_timer(_timeout_dialog, thiz_dialog_para.timeout_sec,  NULL, TIMER_REPEAT);
        }
    }
}

void cmm_exit_dialog1(void)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_CMM_DIALOG))
    {// exist
        GUI_EndDialog(WND_CMM_DIALOG);
    }
}
// ui
SIGNAL_HANDLER int On_wnd_cmm_dialog1_destroy(GuiWidget *widget, void *usrdata)
{
    _exit_dialog();
	return EVENT_TRANSFER_STOP;
}

static int _init_text_display(char *widget, CmmDialogStrClass para)
{
    if(NULL == widget)
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    if(0 != para.state)
    {
        GUI_SetProperty(widget, "state", "show");
        if(para.str)
        {
            GUI_SetProperty(widget, "string", para.str);
        }

        if(para.back_color)
        {
            GUI_SetProperty(widget, "backcolor", para.back_color);
        }

        if(para.fore_color)
        {
            GUI_SetProperty(widget, "forecolor", para.fore_color);
        }

        if(para.alignment)
        {
            GUI_SetProperty(widget, "alignment", para.alignment);
        }

        if(para.x_offset > 0)
        {

        }

        if(para.y_offset > 0)
        {

        }
    }
    else
    {
        GUI_SetProperty(widget, "state", "hide");
    }
    return 0;
}
SIGNAL_HANDLER int On_wnd_cmm_dialog1_create(GuiWidget *widget, void *usrdata)
{
    // for position, color, string, and alignment
    int sel = 0;
    if(DIALOG_YES_NO == thiz_dialog_para.type)
    {// hide ok
        GUI_SetProperty(BTN_CMM_DIALOG_CONTROL1, "state", "show");
        GUI_SetProperty(BTN_CMM_DIALOG_CONTROL2, "state", "hide");
        GUI_SetProperty(BTN_CMM_DIALOG_CONTROL3, "state", "show");
        GUI_SetProperty(BOX_CMM_DIALOG_CONTROL, "state", "show");
        sel = 2;
        GUI_SetProperty(BOX_CMM_DIALOG_CONTROL, "select",&sel);
        //GUI_SetFocusWidget(BTN_CMM_DIALOG_CONTROL3);
    }
    else if (DIALOG_OK == thiz_dialog_para.type)
    {// hide yes and no
        GUI_SetProperty(BTN_CMM_DIALOG_CONTROL1, "state", "hide");
        GUI_SetProperty(BTN_CMM_DIALOG_CONTROL2, "state", "show");
        GUI_SetProperty(BTN_CMM_DIALOG_CONTROL3, "state", "hide");
        GUI_SetProperty(BOX_CMM_DIALOG_CONTROL, "state", "show");
        sel = 1;
        GUI_SetProperty(BOX_CMM_DIALOG_CONTROL, "select",&sel);
        //GUI_SetFocusWidget(BTN_CMM_DIALOG_CONTROL2);
    }
    else
    {// hide all
        //GUI_SetProperty(BTN_CMM_DIALOG_CONTROL1, "state", "hide");
        //GUI_SetProperty(BTN_CMM_DIALOG_CONTROL2, "state", "hide");
        //GUI_SetProperty(BTN_CMM_DIALOG_CONTROL3, "state", "hide");
        GUI_SetProperty(BOX_CMM_DIALOG_CONTROL, "state", "hide");
    }
    // for dialog position
    if((0 == thiz_dialog_para.pos.x) && (0 == thiz_dialog_para.pos.y))
    {// default position
        ;
    }
    else
    {
        GUI_SetProperty(WND_CMM_DIALOG, "move_window_x", &thiz_dialog_para.pos.x);
        GUI_SetProperty(WND_CMM_DIALOG, "move_window_y", &thiz_dialog_para.pos.y);
    }
    // strings control
    _init_text_display(TXT_CMM_DIALOG_TITLE, thiz_dialog_para.title);
    _init_text_display(TXT_CMM_DIALOG_CONTENT1, thiz_dialog_para.content1);
    _init_text_display(TXT_CMM_DIALOG_CONTENT2, thiz_dialog_para.content2);
    _init_text_display(TXT_CMM_DIALOG_CONTENT3, thiz_dialog_para.content3);
    _init_text_display(TXT_CMM_DIALOG_CONTENT4, thiz_dialog_para.content4);
    _init_text_display(TXT_CMM_DIALOG_CONTENT_LONG, thiz_dialog_para.content_long); 

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_box_cmm_dialog1_control_keypress(GuiWidget *widget, void *usrdata)
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
                    if(DIALOG_YES_NO == thiz_dialog_para.type)
                    {
                        int sel = 0;
                        GUI_GetProperty(BOX_CMM_DIALOG_CONTROL, "select", &sel);
                        if(sel == 0)//yes
                        {
                            sel = 2;// no
                        }
                        else// no
                        {
                            sel = 0;
                        }
                        GUI_SetProperty(BOX_CMM_DIALOG_CONTROL, "select", &sel);
                    }
                    else if(DIALOG_OK == thiz_dialog_para.type)
                    {
                        int sel = 1;
                        GUI_SetProperty(BOX_CMM_DIALOG_CONTROL, "select", &sel);
                    }
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

SIGNAL_HANDLER int On_wnd_cmm_dialog1_keypress(GuiWidget *widget, void *usrdata)
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
				case STBK_OK:
                    if(DIALOG_NO_CONTROL != thiz_dialog_para.type)
                    {
                        int sel = 0;
                        GUI_GetProperty(BOX_CMM_DIALOG_CONTROL, "select", &sel);

                        thiz_dialog_ret = DIALOG_RET_NONE;
                        if(0 == sel)
                        {
                            thiz_dialog_ret = DIALOG_RET_YES; 
                        }
                        else if(1 == sel)
                        {
                            thiz_dialog_ret = DIALOG_RET_OK;
                        }
                        else if(2 == sel)
                        {
                            thiz_dialog_ret = DIALOG_RET_NO;
                        }
                        GUI_EndDialog(WND_CMM_DIALOG);
                    }
	                break;
			    case STBK_EXIT:
                case STBK_MENU:
                    if(DIALOG_NO_CONTROL != thiz_dialog_para.type)
                    {
                        thiz_dialog_ret = DIALOG_RET_NONE;
                        GUI_EndDialog(WND_CMM_DIALOG);
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
#else

SIGNAL_HANDLER int On_box_cmm_dialog1_control_keypress(GuiWidget *widget, void *usrdata)
{
    return 0;
}
SIGNAL_HANDLER int On_wnd_cmm_dialog1_destroy(GuiWidget *widget, void *usrdata)
{
    return 0;
}

SIGNAL_HANDLER int On_wnd_cmm_dialog1_create(GuiWidget *widget, void *usrdata)
{
    return 0;
}

SIGNAL_HANDLER int On_wnd_cmm_dialog1_keypress(GuiWidget *widget, void *usrdata)
{
    return 0;
}
#endif
