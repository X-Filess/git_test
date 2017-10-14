/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_work_time.c
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
#define WND_WORK_TIME                    "wnd_nstv_work_time"
#define XML_WORK_TIME                    "ca/nstv/wnd_nstv_work_time.xml"
#define BOX_WORK_TIME                    "box_nstv_work_time"
#define TXT_WORK_TIME_TITLE              "txt_nstv_work_time_title"
#define EDIT_WORK_TIME_START             "edit_nstv_work_time_start"
#define EDIT_WORK_TIME_END               "edit_nstv_work_time_end"
#define EDIT_WORK_TIME_PIN               "edit_nstv_work_time_pin"
#define BXI_WORK_TIME_1                  "bxi_nstv_work_time_1"
#define BXI_WORK_TIME_2                  "bxi_nstv_work_time_2"
#define BXI_WORK_TIME_3                  "bxi_nstv_work_time_3"

#define NSTV_PIN_KEY                    "nscas>pin"

#define BMP_BAR_LONG_FOCUS              "s_bar_long_focus.bmp"
#define BMP_BAR_LONG_UNFOCUS            ""
static char *thiz_item_widget[] = {
    BXI_WORK_TIME_1,
    BXI_WORK_TIME_2,
};

extern status_t app_nstv_passwd_dlg(PasswdDlg *passwd_dlg);

static int _get_work_time(int *start_time, int *end_time)
{
    CASCAMDataGetClass data_set;
    CASCAMNSTVWorkTimeClass work_time_info;

    if((NULL == start_time) || (NULL == end_time))
    {
        printf("\nERROR, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    memset(&work_time_info, 0, sizeof(CASCAMNSTVWorkTimeClass));
    memset(&data_set, 0, sizeof(CASCAMDataGetClass));
    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = WORK_TIME_INFO;
    data_set.sub_id = NSTV_WORK_TIME_GET;
    data_set.data_buf = (void*)&work_time_info;
    data_set.buf_len = sizeof(CASCAMNSTVWorkTimeClass);
    if(0 == CASCAM_Get_Property(&data_set))
    {
        printf("\n--------------------------------------------\n");
        printf("\033[32mnstv maturity info:\n\033[0m");
        printf("\033[32m watch level:\033[0m \033[33mstart = %06x, end = %06x\n\033[0m", work_time_info.start_time, work_time_info.end_time);
        *start_time = work_time_info.start_time;
        *end_time = work_time_info.end_time;
        return 0;
    }
    return -1;
}

// BCD CODE: start_time(0x235959), means 23:59:59
static int _set_work_time(char* pin,int start_time, int end_time)
{
    CASCAMDataSetClass data_set;
    CASCAMNSTVWorkTimeClass work_time_info;

    memset(&work_time_info, 0, sizeof(CASCAMNSTVWorkTimeClass));
    memset(&data_set, 0, sizeof(CASCAMDataSetClass));
    // start_time
    work_time_info.start_time = start_time;
    // end_time
    work_time_info.end_time = end_time;
    // PIN
    memcpy(work_time_info.pin, pin, 6);

    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = WORK_TIME_CONTROL;
    data_set.sub_id = NSTV_WORK_TIME_SET;
    data_set.data_buf = (void*)&work_time_info;
    data_set.buf_len = sizeof(CASCAMNSTVWorkTimeClass);
    if(0 == CASCAM_Set_Property(&data_set))
    {
        printf("\n--------------------------------------------\n");
        printf("\033[32m work time info:\n\033[0m");
        printf("\033[32m start time:\033[0m \033[33mstart_time = %06x, end_time = %06x\n\033[0m", work_time_info.start_time, work_time_info.end_time);
        printf("\033[32m pin:\033[0m \033[33m%s, pin[0] = %x\n\033[0m", work_time_info.pin, work_time_info.pin[0]);
        return 0;
    }
    return -1;

}
// 0x235959 -> 23, 59, 59
static void  _bcd_to_hms(unsigned int   bcd,
                            unsigned char  *a,
                            unsigned char  *b,
                            unsigned char  *c)
{
    *a =(((bcd >> 20) & 0xf)*10) + ((bcd >> 16) & 0xf);

    *b = (((bcd >> 12) & 0xf)*10) + ((bcd >> 8) & 0xf);

    *c = (((bcd >> 4) & 0xf)*10) + ((bcd >> 0) & 0xf);
}

//23, 59, 59 -> 0x235959
static void  _hms_to_bcd(unsigned int   *bcd,
                            unsigned char  a,
                            unsigned char  b,
                            unsigned char  c)
{
    unsigned short bcd_high_word = 0;
    unsigned short bcd_low_word  = 0;

    bcd_high_word =((((a/10)%10) & 0xf) << 4)
                    | ((a%10) & 0xf);
    bcd_low_word = (((b/10) & 0xf) << 12)
        | (((b%10) & 0xf) << 8)
        | (((c/10) & 0xf) << 4)
        | ((c%10) & 0xf);
    *bcd = (bcd_high_word << 16) | bcd_low_word;
}

static int _time_str_2_bcd(char *str, int *bcd_out)
{
    int hh = 0;
    int mm = 0;
    int ss = 0;
    if((NULL == str) || (NULL == bcd_out))
    {
        printf("\nERROR, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    sscanf(str, "%d:%d:%d", &hh, &mm, &ss);
    _hms_to_bcd((unsigned int*)bcd_out, hh, mm, ss);
    //printf("\n%s, str = %s, hh = %d, mm = %d, ss = %d, bcd = %06x\n", __FUNCTION__,str, hh, mm, ss, *bcd_out);
    return 0;
}


static bool _work_time_passwd_check_cb( char *passwd_data,int passwd_len)
{
    char *start = NULL;
    char *end = NULL;
    int start_time = 0;
    int end_time = 0;
    int time1 = 0;
    int time2 = 0;
    bool ret = true; 

    if (passwd_data == NULL)
    {
        return false;
    }
    _get_work_time(&time1, &time2);
    GUI_GetProperty(EDIT_WORK_TIME_START, "string", &start);
    GUI_GetProperty(EDIT_WORK_TIME_END, "string", &end);

    // HH:MM:SS change to 0xHHMMSS
    _time_str_2_bcd(start, &start_time);
    _time_str_2_bcd(end, &end_time);


    if (_set_work_time(passwd_data,start_time, end_time) < 0)
    {
        ret =  false;
    }
    else
    {
        ret =  true;
    }

    return ret;
}
static int _work_time_passwd_exit_cb(PopPwdRet *ret, void *usr_data)
{
	if(ret->result ==  PASSWD_OK)
	{
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.str = NSTV_OSD_CHANGE_TIME;
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
void _nstv_work_time_passwd_dlg(void)
{
    PasswdDlg passwd_dlg;

    memset(&passwd_dlg, 0, sizeof(PasswdDlg));
    passwd_dlg.passwd_check_cb =_work_time_passwd_check_cb ;
    passwd_dlg.passwd_exit_cb = _work_time_passwd_exit_cb;

    app_nstv_passwd_dlg(&passwd_dlg);
}

static int _work_time_save_exit(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {// need to save
       _nstv_work_time_passwd_dlg();
    }
    else
    {
    }
   return 0;
}

static int _nstv_work_time_check_save(void)
{
    char *start = NULL;
    char *end = NULL;
    int start_time = 0;
    int end_time = 0;
    int time1 = 0;
    int time2 = 0;
       
    _get_work_time(&time1, &time2);
    GUI_GetProperty(EDIT_WORK_TIME_START, "string", &start);
    GUI_GetProperty(EDIT_WORK_TIME_END, "string", &end);

     // HH:MM:SS change to 0xHHMMSS
     _time_str_2_bcd(start, &start_time);
     _time_str_2_bcd(end, &end_time);


// 双秒
    if((start_time != time1) || (end_time != time2))
    {// need to save
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.mode = POP_MODE_BLOCK;
        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_SAVE_CHANGED;
        pop.exit_cb = _work_time_save_exit;
        popdlg_create(&pop);
    }
    return 0;
}

static void _create_work_time_menu(void)
{
    static char init_flag = 0;
    int start_time = 0;
    int end_time = 0;
    unsigned char hh = 0;
    unsigned char mm = 0;
    unsigned char ss = 0;
    char buf[10] = {0};

    if(0 == init_flag)
    {
        GUI_LinkDialog(WND_WORK_TIME, XML_WORK_TIME);
        init_flag = 1;
    }
    app_create_dialog(WND_WORK_TIME);

    _get_work_time(&start_time, &end_time);
    // start time
    _bcd_to_hms(start_time, &hh, &mm, &ss);
    sprintf(buf, "%02d:%02d:%02d", hh, mm, ss);
    GUI_SetProperty(EDIT_WORK_TIME_START, "string", buf);
    // end time
    memset(buf, 0, sizeof(buf));
    _bcd_to_hms(end_time, &hh, &mm, &ss);
    sprintf(buf, "%02d:%02d:%02d", hh, mm, ss);
    GUI_SetProperty(EDIT_WORK_TIME_END, "string", buf);
    // pin
    GUI_SetProperty(EDIT_WORK_TIME_PIN, "default_intaglio", "-");
    GUI_SetProperty(EDIT_WORK_TIME_PIN, "intaglio", "*");


}

void app_nstv_work_time_exec(void)
{
    _create_work_time_menu();
}

// for ui
SIGNAL_HANDLER int On_wnd_nstv_work_time_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}



SIGNAL_HANDLER int On_wnd_nstv_work_time_got_focus(GuiWidget *widget, void *usrdata)
{
    int box_sel = 0;
    GUI_GetProperty(BOX_WORK_TIME, "select", &box_sel);
	GUI_SetProperty(thiz_item_widget[box_sel], "unfocus_image", BMP_BAR_LONG_UNFOCUS);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_work_time_lost_focus(GuiWidget *widget, void *usrdata)
{
    int box_sel = 0;
    GUI_GetProperty(BOX_WORK_TIME, "select", &box_sel);
	GUI_SetProperty(thiz_item_widget[box_sel], "unfocus_image", BMP_BAR_LONG_FOCUS);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_work_time_keypress(GuiWidget *widget, void *usrdata)
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
                    GUI_EndDialog(WND_WORK_TIME);
                    break;
                case STBK_OK:
                    _nstv_work_time_check_save();
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
SIGNAL_HANDLER int On_wnd_nstv_work_time_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_work_time_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_work_time_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_work_time_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}


#endif
