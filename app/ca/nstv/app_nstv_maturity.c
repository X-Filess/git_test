/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_maturity.c
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

#define WND_MATURITY       "wnd_nstv_maturity"
#define XML_MATURITY       "ca/nstv/wnd_nstv_maturity.xml"
#define BOX_MATURITY       "box_nstv_maturity"
#define TXT_MATURITY_TITLE "txt_nstv_maturity_title"
#define CMB_MATURITY_LEVEL "cmb_nstv_maturity_level"
#define EDIT_MATURITY_PIN  "edit_nstv_maturity_pin"
#define BXI_MATURITY_1     "bxi_nstv_maturity_1"
#define BXI_MATURITY_2     "bxi_nstv_maturity_2"

#define NSTV_PIN_KEY         "nscas>pin"

#define BMP_BAR_LONG_FOCUS   "s_bar_long_focus.bmp"
#define BMP_BAR_LONG_UNFOCUS ""

static char thiz_level_table[] = {4,5,6,7,8,9,10,11,12,13,14,15,16,17,18};
static int _get_level_index(char level)
{
    int i = 0;
    unsigned int count = sizeof(thiz_level_table);
    for(i = 0; i < count; i++)
    {
        if(thiz_level_table[i] == level)
        {// find it
            return i;
        }
    }
    return -1;
}

static int _get_maturity_rating(int *rating)
{
    CASCAMDataGetClass data_set;
    CASCAMNSTVWatchLevelClass rating_info;

    if(NULL == rating)
    {
        printf("\nERROR, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    memset(&rating_info, 0, sizeof(CASCAMNSTVWatchLevelClass));
    memset(&data_set, 0, sizeof(CASCAMDataGetClass));
    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = PARENTAL_INFO;
    data_set.sub_id = NSTV_WATCH_LEVEL_GET;
    data_set.data_buf = (void*)&rating_info;
    data_set.buf_len = sizeof(CASCAMNSTVWatchLevelClass);
    if(0 == CASCAM_Get_Property(&data_set))
    {
        printf("\n--------------------------------------------\n");
        printf("\033[32mnstv maturity info:\n\033[0m");
        printf("\033[32m watch level:\033[0m \033[33m%d\n\033[0m", rating_info.watch_level);
        *rating = rating_info.watch_level;
        return 0;
    }
    return -1;
}

static int _set_maturity_rating(char *passwd,unsigned int rating)
{
    CASCAMDataSetClass data_set;
    CASCAMNSTVWatchLevelClass rating_info;

    if (passwd == NULL)
    {
        return -1;
    }
    memset(&rating_info, 0, sizeof(CASCAMNSTVWatchLevelClass));
    memset(&data_set, 0, sizeof(CASCAMDataSetClass));
    // rating
    rating_info.watch_level = rating;

    memcpy(rating_info.pin, passwd, 6);
    //printf("\n%s: \033[38m pin = %s, pin[0] = %x\033[0m\n", __FUNCTION__,str, rating_info.pin[0]);

    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = PARENTAL_CONTROL;
    data_set.sub_id = NSTV_WATCH_LEVEL_SET;
    data_set.data_buf = (void*)&rating_info;
    data_set.buf_len = sizeof(CASCAMNSTVWatchLevelClass);
    if(0 == CASCAM_Set_Property(&data_set))
    {
        printf("\n--------------------------------------------\n");
        printf("\033[32mnstv maturity info:\n\033[0m");
        printf("\033[32m watch level:\033[0m \033[33m%d\n\033[0m", rating_info.watch_level);
        printf("\033[32m pin:\033[0m \033[33m%s, pin[0] = %x\n\033[0m", rating_info.pin, rating_info.pin[0]);
        return 0;
    }
    return -1;

}


static bool _maturity_passwd_check_cb( char *passwd_data,int passwd_len)
{
    int sel = 0;

    bool ret = true; 

    if (passwd_data == NULL)
    {
        return false;
    }
    GUI_GetProperty(CMB_MATURITY_LEVEL, "select", &sel);


    if(_set_maturity_rating(passwd_data,thiz_level_table[sel]) < 0)
    {
        ret =  false;
    }
    else
    {
        ret =  true;
    }

    return ret;
}

static int _maturity_passwd_exit_cb(PopPwdRet *ret, void *usr_data)
{
	if(ret->result ==  PASSWD_OK)
	{
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.str = NSTV_OSD_CHANGE_RATING;
            pop.timeout_sec = 1;
            popdlg_create(&pop);
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.mode = POP_MODE_BLOCK;
            pop.type = POP_TYPE_OK;
            pop.str = NSTV_OSD_INVALID_PIN;
            popdlg_create(&pop);
            return -1;
	}

	return 0;	
}

extern status_t app_nstv_passwd_dlg(PasswdDlg *passwd_dlg);
void _nstv_maturity_passwd_dlg(void)
{
    PasswdDlg passwd_dlg;

    memset(&passwd_dlg, 0, sizeof(PasswdDlg));
    passwd_dlg.passwd_check_cb =_maturity_passwd_check_cb ;
    passwd_dlg.passwd_exit_cb = _maturity_passwd_exit_cb;

    app_nstv_passwd_dlg(&passwd_dlg);
}

static int _maturity_save_exit(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {// need to save
       _nstv_maturity_passwd_dlg();
    }
    else
    {
    }
   return 0;
}

static int _nstv_maturity_check_save(void)
    {
    int level  = 0;
    int value1 = 0;

    // level
    _get_maturity_rating(&level);
    GUI_GetProperty(CMB_MATURITY_LEVEL, "select", &value1);


    if(level != thiz_level_table[value1])
    {// need to save
          PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.mode = POP_MODE_BLOCK;
        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_SAVE_CHANGED;
        pop.exit_cb = _maturity_save_exit;
        popdlg_create(&pop);
        return 1;
    }
    return 0;
}


static void _create_maturity_menu(void)
{
    static char init_flag = 0;
    int value = 0;

    if(0 == init_flag)
    {
        GUI_LinkDialog(WND_MATURITY, XML_MATURITY);
        init_flag = 1;
    }
    app_create_dialog(WND_MATURITY);
    //level
    _get_maturity_rating(&value);
    value = _get_level_index(value);
    if(value >= 0)
    {
        GUI_SetProperty(CMB_MATURITY_LEVEL, "select", &value);
    }
    else
    {
        printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
    }
    GUI_SetProperty(EDIT_MATURITY_PIN, "default_intaglio", "-");
    GUI_SetProperty(EDIT_MATURITY_PIN, "intaglio", "*");
}

void app_nstv_maturity_exec(void)
{
    _create_maturity_menu();
}

// for ui
SIGNAL_HANDLER int On_wnd_nstv_maturity_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_maturity_got_focus(GuiWidget *widget, void *usrdata)
{
    int box_sel = 0;
    GUI_GetProperty(BOX_MATURITY, "select", &box_sel);
    if(box_sel)
    {
	    GUI_SetProperty(BXI_MATURITY_2, "unfocus_image", BMP_BAR_LONG_UNFOCUS);
    }
    else
    {
	    GUI_SetProperty(BXI_MATURITY_1, "unfocus_image", BMP_BAR_LONG_UNFOCUS);
    }

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_maturity_lost_focus(GuiWidget *widget, void *usrdata)
{
    int box_sel = 0;
    GUI_GetProperty(BOX_MATURITY, "select", &box_sel);
    if(box_sel)
    {
	    GUI_SetProperty(BXI_MATURITY_2, "unfocus_image", BMP_BAR_LONG_FOCUS);
    }
    else
    {
	    GUI_SetProperty(BXI_MATURITY_1, "unfocus_image", BMP_BAR_LONG_FOCUS);
    }
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_maturity_keypress(GuiWidget *widget, void *usrdata)
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
                        GUI_EndDialog(WND_MATURITY);
                    }
                    break;
                case STBK_OK:
                    {
                        _nstv_maturity_check_save();
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
SIGNAL_HANDLER int On_wnd_nstv_maturity_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_maturity_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_maturity_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_maturity_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}


#endif
