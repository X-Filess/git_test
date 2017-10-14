/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cmm_cascam_dialog4.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.10.11		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_config.h"
#include "app_cascam_pop.h"

#if CASCAM_SUPPORT
#define WND_CMM_DIALOG                  "wnd_cmm_dialog4"
#define XML_CMM_DIALOG_PATH             "ca/wnd_cmm_dialog4.xml"

#define TXT_CMM_DIALOG_TITLE            "txt_cmm_dialog4_title"
#define TXT_CMM_DIALOG_CONTENT1         "txt_cmm_dialog4_content1"
#define TXT_CMM_DIALOG_CONTENT2         "txt_cmm_dialog4_content2"
#define TXT_CMM_DIALOG_CONTENT3         "txt_cmm_dialog4_content3"
#define BTN_CMM_DIALOG_PASSWORD1        "btn_cmm_dialog4_password1"
#define BTN_CMM_DIALOG_PASSWORD2        "btn_cmm_dialog4_password2"
#define BTN_CMM_DIALOG_PASSWORD3        "btn_cmm_dialog4_password3"
#define BTN_CMM_DIALOG_PASSWORD4        "btn_cmm_dialog4_password4"

#define STR_INPUTTED_FLAG "*"
#define PASSWD_LENGTH 4

static char *thiz_password_widget[PASSWD_LENGTH] = {
	BTN_CMM_DIALOG_PASSWORD1,
	BTN_CMM_DIALOG_PASSWORD2,
	BTN_CMM_DIALOG_PASSWORD3,
	BTN_CMM_DIALOG_PASSWORD4
};

static struct{
	char passwd_buf[PASSWD_LENGTH];
	int cur_cursor;
}thiz_password_info = {{0}, 0};

static bool _passwd_check(void)
{
	char sys_passwd[PASSWD_LENGTH] = {0};
	int init_value = 0;
	int i = 0;
	bool ret = true;

	GxBus_ConfigGetInt(PASSWORD_KEY0, &init_value, PASSWORD_1);
	sys_passwd[0] = init_value;
	GxBus_ConfigGetInt(PASSWORD_KEY1, &init_value, PASSWORD_2);
	sys_passwd[1] = init_value;
	GxBus_ConfigGetInt(PASSWORD_KEY2, &init_value, PASSWORD_3);
	sys_passwd[2] = init_value;
	GxBus_ConfigGetInt(PASSWORD_KEY3, &init_value, PASSWORD_4);
	sys_passwd[3] = init_value;

	for(i = 0; i < PASSWD_LENGTH; i++)
	{
		if(sys_passwd[i] != thiz_password_info.passwd_buf[i])
		{
			ret = false;
			break;
		}
	}

	if(ret == false)
	{
		char *general = GRNERAL_PASSWD;

		ret = true;
		for(i = 0; i < PASSWD_LENGTH; i++)
		{
			if(general[i] != thiz_password_info.passwd_buf[i])
			{
				ret = false;
				break;
			}
		}
	}

	return ret;
}

static int _set_passwd_cursor_back(void)
{
	int ret = 0;

	if(thiz_password_info.cur_cursor > 0)
	{
		thiz_password_info.cur_cursor--;
		GUI_SetFocusWidget(thiz_password_widget[thiz_password_info.cur_cursor]);
	}
	return ret;
}

static int _set_passwd_cursor_next(void)
{
	int ret = 0;


	if(thiz_password_info.cur_cursor < (PASSWD_LENGTH -1))
	{
	    thiz_password_info.cur_cursor++;
		GUI_SetFocusWidget(thiz_password_widget[thiz_password_info.cur_cursor]);
	}
	else
	{
		ret = -1;
	}
	return ret;
}

static int _input_cursor_passwd(unsigned int key_val)
{
	int ret = 0;

	switch(key_val)
	{
		case STBK_0:
			thiz_password_info.passwd_buf[thiz_password_info.cur_cursor] = '0';
			break;

		case STBK_1:
			thiz_password_info.passwd_buf[thiz_password_info.cur_cursor] = '1';
			break;

		case STBK_2:
			thiz_password_info.passwd_buf[thiz_password_info.cur_cursor] = '2';
			break;

		case STBK_3:
			thiz_password_info.passwd_buf[thiz_password_info.cur_cursor] = '3';
			break;

		case STBK_4:
			thiz_password_info.passwd_buf[thiz_password_info.cur_cursor] = '4';
			break;

		case STBK_5:
			thiz_password_info.passwd_buf[thiz_password_info.cur_cursor] = '5';
			break;

		case STBK_6:
			thiz_password_info.passwd_buf[thiz_password_info.cur_cursor] = '6';
			break;

		case STBK_7:
			thiz_password_info.passwd_buf[thiz_password_info.cur_cursor] = '7';
			break;

		case STBK_8:
			thiz_password_info.passwd_buf[thiz_password_info.cur_cursor] = '8';
			break;

		case STBK_9:
			thiz_password_info.passwd_buf[thiz_password_info.cur_cursor] = '9';
			break;

		default:
			ret = -1;
			break;
	}

	if(0 == ret)
	{
		GUI_SetProperty(thiz_password_widget[thiz_password_info.cur_cursor], "string", STR_INPUTTED_FLAG);
	}
	return ret;
}


static CmmPasswordDlgClass thiz_password_dlg_para ;
static CmmDialogRetEnum thiz_password_dlg_ret = DIALOG_RET_NONE;

static int _exit_dialog(void)
{
    if((DIALOG_RET_OK == thiz_password_dlg_ret)
        || (DIALOG_RET_YES == thiz_password_dlg_ret))
    {
        if(thiz_password_dlg_para.exit_cb)
        {
            thiz_password_dlg_para.exit_cb(thiz_password_dlg_ret);
        }
    }
    else if(DIALOG_RET_NO == thiz_password_dlg_ret)
    {
        // TODO: ...
    }
    memset(&thiz_password_dlg_para, 0, sizeof(thiz_password_dlg_para));
    thiz_password_dlg_ret = DIALOG_RET_NONE;
    return 0;
}

void cmm_create_dialog4(CmmPasswordDlgClass *para)
{
    static char init_flag = 0;

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
        if(memcmp(para, &thiz_password_dlg_para, sizeof(CmmPasswordDlgClass)))
        {
            GUI_EndDialog(WND_CMM_DIALOG);
        }
        else
        {
            return ;
        }
    }
// init parameters
    memcpy(&thiz_password_dlg_para, para, sizeof(CmmPasswordDlgClass));
    memset(& thiz_password_info, 0, sizeof(thiz_password_info));
// create dialog
    app_create_dialog(WND_CMM_DIALOG);
}

void cmm_exit_dialog4(void)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_CMM_DIALOG))
    {// exist
        GUI_EndDialog(WND_CMM_DIALOG);
    }
}

// for ui
SIGNAL_HANDLER int On_wnd_cmm_dialog4_destroy(GuiWidget *widget, void *usrdata)
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

SIGNAL_HANDLER int On_wnd_cmm_dialog4_create(GuiWidget *widget, void *usrdata)
{
    // for dialog position
    if((0 == thiz_password_dlg_para.pos.x) && (0 == thiz_password_dlg_para.pos.y))
    {// default position
        ;
    }
    else
    {
        GUI_SetProperty(WND_CMM_DIALOG, "move_window_x", &thiz_password_dlg_para.pos.x);
        GUI_SetProperty(WND_CMM_DIALOG, "move_window_y", &thiz_password_dlg_para.pos.y);
    }
    // strings control
    _init_text_display(TXT_CMM_DIALOG_TITLE, thiz_password_dlg_para.title);
    _init_text_display(TXT_CMM_DIALOG_CONTENT1, thiz_password_dlg_para.content1);
    _init_text_display(TXT_CMM_DIALOG_CONTENT2, thiz_password_dlg_para.content2);
    _init_text_display(TXT_CMM_DIALOG_CONTENT3, thiz_password_dlg_para.content3);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_cmm_dialog4_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    int key = 0;
	GUI_Event *event = NULL;
    char *parent = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
            key = find_virtualkey_ex(event->key.scancode,event->key.sym);
			switch(key)
			{
#if DLNA_SUPPORT
                case VK_DLNA_TRIGGER:
                    app_dlna_ui_exec(NULL, NULL);
                    break;
#endif
                case VK_BOOK_TRIGGER:
					GUI_EndDialog("after wnd_full_screen");
					break;
                case STBK_LEFT:
                    {
                        char *str = NULL;
                        GUI_GetProperty(thiz_password_widget[thiz_password_info.cur_cursor], "string", &str);
                        if((NULL != str) 
                            && (0 == strncmp(str, STR_INPUTTED_FLAG, 1)))
                        {
                            GUI_SetProperty(thiz_password_widget[thiz_password_info.cur_cursor], "string", " ");
                        }
                    }
                    _set_passwd_cursor_back();
                    break;
				case STBK_EXIT:
                case STBK_MENU:
                    thiz_password_dlg_ret = DIALOG_RET_NONE;
                    GUI_EndDialog(WND_CMM_DIALOG);
                    break;
                case STBK_UP:
			    case STBK_DOWN:
			    case STBK_PAGE_UP:
			    case STBK_PAGE_DOWN:
                    thiz_password_dlg_ret = DIALOG_RET_NONE;
                    GUI_EndDialog(WND_CMM_DIALOG);
					parent = (char *)GUI_GetFocusWidget();
					if(parent != NULL)
					{
						GUI_SendEvent((const char*)parent, event);
					}
				    break;
				default:
                    if(0 == _input_cursor_passwd(key))
                    {
                        if(0 != _set_passwd_cursor_next())
					    {
						    if(_passwd_check() == true)
						    {
                                thiz_password_dlg_ret = DIALOG_RET_OK;
						    }
						    else
						    {
                                thiz_password_dlg_ret = DIALOG_RET_NONE;
						    }
                            GUI_EndDialog(WND_CMM_DIALOG);
					    }
                    }
					break;
			}
		default:
			break;
	}
	return ret;
}
#else

SIGNAL_HANDLER int On_wnd_cmm_dialog4_create(GuiWidget *widget, void *usrdata)
{
    return 0;
}

SIGNAL_HANDLER int On_wnd_cmm_dialog4_keypress(GuiWidget *widget, void *usrdata)
{
    return 0;
}

SIGNAL_HANDLER int On_wnd_cmm_dialog4_destroy(GuiWidget *widget, void *usrdata)
{
    return 0;
}
#endif

