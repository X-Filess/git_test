/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_mail.c
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
#include "app.h"
#include "app_module.h"
#include "app_config.h"

#if NSCAS_SUPPORT

#define WND_MAIL                "wnd_nstv_mail_list"
#define PATH_XML                "ca/nstv/wnd_nstv_mail_list.xml"
#define TXT_MAIL_TITLE          "txt_nstv_mail_list_title"
#define TXT_MAIL_TOTAL_NUM      "txt_nstv_mail_list_total_num"
#define TXT_MAIL_UNREAD_NUM     "txt_nstv_mail_list_unread_num"
#define LSV_MAIL_LIST           "lsv_nstv_mail_list_list"
#define TXT_DELETE_ALL          "txt_nstv_mail_list_item1"
#define TXT_DELETE              "txt_nstv_mail_list_item2"
#define TXT_READ                "txt_nstv_mail_list_item3"
#define IMG_DELETE_ALL          "img_nstv_mail_list_item1"
#define IMG_DELETE              "img_nstv_mail_list_item2"
#define IMG_READ                "img_nstv_mail_list_item3"
static int gFullscreen_flag =0;
// OSD
void app_nstv_mail_detail_exec(char *subject, unsigned int pos);

static int _delete_all_mail(void)
{
    CASCAMDataSetClass data_set = {0};
    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = EMAIL_CONTROL;
    data_set.sub_id = NSTV_MAIL_DEL_ALL;
    data_set.data_buf = NULL;
    data_set.buf_len = 0;
    if(0 != CASCAM_Set_Property(&data_set))
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    return 0;
}

static int _delete_mail_by_pos(unsigned short pos)
{
    CASCAMDataSetClass data_set = {0};
    unsigned char mail_pos = pos;
    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = EMAIL_CONTROL;
    data_set.sub_id = NSTV_MAIL_DEL_BY_POS;
    data_set.data_buf = (void*)&mail_pos;
    data_set.buf_len = 1;
    if(0 != CASCAM_Set_Property(&data_set))
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    return 0;
}

static CASCAMNSTVMailInfoClass thiz_mail_info = {0};

static int _mail_init_info(void)
{
    CASCAMDataGetClass data_get = {0};
    unsigned short mail_count = 0;
    data_get.cas_flag = CASCAM_NSTV;
    data_get.main_id = EMAIL_INFO;
    data_get.sub_id = NSTV_MAIL_GET_COUNT;
    data_get.data_buf = (void*)&mail_count;
    data_get.buf_len = sizeof(unsigned short);
    if(0 == CASCAM_Get_Property(&data_get))
    {
        printf("\033[32mwnafa email info:\n\033[0m");
        printf("\033[32memail count:\033[0m \033[33m%d\n\033[0m", mail_count);
        if(mail_count > 0)
        {
            data_get.sub_id = NSTV_MAIL_GET_DATA_HEAD;
            thiz_mail_info.mail_total = mail_count;
            thiz_mail_info.mail_head = GxCore_Mallocz(sizeof(CASCAMNSTVMailHeadClass)*mail_count);
            if(NULL == thiz_mail_info.mail_head)
            {
                printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
            data_get.data_buf = (void*)&thiz_mail_info;
            data_get.buf_len = sizeof(CASCAMNSTVMailInfoClass);
            if(0 == CASCAM_Get_Property(&data_get))
            {
                 printf("\033[32memail unread count:\033[0m \033[33m%d\n\033[0m", thiz_mail_info.mail_unread_count);
#if 0
                 int i = 0;
                 for(i = 0; i < thiz_mail_info.mail_total; i++)
                 {
                     printf("\033[33m%3d    %d     %s    %s    %d    %d\n\033[0m",
                             i+1, thiz_mail_info.mail_head[i].read_status,
                             thiz_mail_info.mail_head[i].create_time, 
                             thiz_mail_info.mail_head[i].title,
                             thiz_mail_info.mail_head[i].importance,
                             thiz_mail_info.mail_head[i].mail_id);
                 }
#endif
            }
            else
            {
                printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
    else
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    return 0;
}

static int _mail_body_get_by_pos(unsigned int pos, char *buf, unsigned int buf_size)
{
    int ret = 0;
    CASCAMNSTVMailBodyClass *info;
    CASCAMDataGetClass data_get = {0};
    unsigned int bytes = 0;

    if((NULL == buf) || (0 == buf_size))
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    info = GxCore_Mallocz(sizeof(CASCAMNSTVMailBodyClass));
    if(NULL == info)
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    info->pos = pos;
    data_get.cas_flag = CASCAM_NSTV;
    data_get.main_id = EMAIL_INFO;
    data_get.sub_id = NSTV_MAIL_GET_DATA_BODY;
    data_get.data_buf = (void*)info;
    data_get.buf_len = sizeof(CASCAMNSTVMailBodyClass);
    if(0 != CASCAM_Get_Property(&data_get))
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    bytes = strlen(info->body);
    bytes = (buf_size > bytes)? bytes:buf_size;
    memcpy(buf, info->body, bytes);
    GxCore_Free(info);
    return ret;
}

static void _release_data(void)
{
   if(thiz_mail_info.mail_head)
   {
        GxCore_Free(thiz_mail_info.mail_head);
   }
   memset(&thiz_mail_info, 0, sizeof(thiz_mail_info));
}

static void _reflash_data(void)
{
    char buf[50] = {0};
    int sel = 0;
    
    GUI_GetProperty(LSV_MAIL_LIST, "select", &sel);
    _release_data();
    _mail_init_info();
    // for text display
    sprintf(buf, "%d", thiz_mail_info.mail_total);
    GUI_SetProperty(TXT_MAIL_TOTAL_NUM, "string", buf);
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%d",thiz_mail_info.mail_unread_count);
    GUI_SetProperty(TXT_MAIL_UNREAD_NUM, "string", buf);
    // for listview
    GUI_SetProperty(LSV_MAIL_LIST, "update_all", NULL);
    if(0 == thiz_mail_info.mail_total)
    {
        GUI_SetProperty(TXT_DELETE_ALL, "state", "hide");
        GUI_SetProperty(TXT_DELETE, "state", "hide");
        GUI_SetProperty(TXT_READ, "state", "hide");
        GUI_SetProperty(IMG_DELETE_ALL, "state", "hide");
        GUI_SetProperty(IMG_DELETE, "state", "hide");
        GUI_SetProperty(IMG_READ, "state", "hide");
    }
    else
    {
        if(sel >= thiz_mail_info.mail_total)
        {
            sel = sel - 1;
        }
        GUI_SetProperty(LSV_MAIL_LIST, "select", &sel);
    }
}

// for check dialog
static int _delete_one_mail_check(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        int sel = 0;
        GUI_GetProperty(LSV_MAIL_LIST, "select", &sel);

        _delete_mail_by_pos(sel);
        _reflash_data();

        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.str = NSTV_OSD_DELETE_MAIL;
        pop.timeout_sec = 1;
        popdlg_create(&pop);
    }
    return 0;
}

static int _nstv_mail_delete_one(void)
{
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.mode = POP_MODE_UNBLOCK;
    pop.type = POP_TYPE_YES_NO;
    pop.str = STR_ID_SURE_DELETE;
    pop.exit_cb = _delete_one_mail_check;
    popdlg_create(&pop);
    return 0;
}

static int _delete_all_mail_check(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        _delete_all_mail();
        _reflash_data();

        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.str = NSTV_OSD_DELETE_MAIL_ALL;
        pop.timeout_sec = 1;
        popdlg_create(&pop);
    }
    return 0;
}

static int _nstv_mail_delete_all(void)
{
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.mode = POP_MODE_UNBLOCK;
    pop.type = POP_TYPE_YES_NO;
    pop.str = STR_ID_SURE_DELETE;
    pop.exit_cb = _delete_all_mail_check;
    popdlg_create(&pop);
    return 0;
}


static int _get_mail_count(void)
{
    return thiz_mail_info.mail_total;
}

void app_nstv_mail_exec(int fullscreen_flag)
{
    char buf[50] = {0};
    static char init_flag = 0;
    if(_mail_init_info() < 0)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return ;
    }
    // TODO: for UI
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_MAIL, PATH_XML);
    }
    gFullscreen_flag = fullscreen_flag;
    app_create_dialog(WND_MAIL);
    GUI_SetProperty(TXT_MAIL_TITLE, "string", "Mail List");
    GUI_SetProperty(TXT_DELETE_ALL, "string", NSTV_DELETE_ALL_MAILS);
    GUI_SetProperty(TXT_DELETE, "string", STR_ID_DELETE);
    GUI_SetProperty(TXT_READ, "string", NSTV_READ);
    //
    sprintf(buf, "%d", thiz_mail_info.mail_total);
    GUI_SetProperty(TXT_MAIL_TOTAL_NUM, "string", buf);
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%d", thiz_mail_info.mail_unread_count);
    GUI_SetProperty(TXT_MAIL_UNREAD_NUM, "string", buf);

}

SIGNAL_HANDLER int On_lsv_nstv_mail_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	return thiz_mail_info.mail_total;
}

SIGNAL_HANDLER int On_lsv_nstv_mail_list_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = NULL;
    static char buf[10] ;
    item = (ListItemPara*)usrdata;
    if(NULL == item)
    {
        return GXCORE_ERROR;
    }
    if(0 > item->sel)
    {
        return GXCORE_ERROR;
    }
    // col 0, No.
    memset(buf, 0, sizeof(buf));
    sprintf(buf,"%d", (item->sel + 1));
    item->string = buf;
    item->x_offset = 2;
    item->image = NULL;
    // col 1, importance
    item = item->next;
    if(thiz_mail_info.mail_head[item->sel].importance > 0)
    {
        item->string = NSTV_IMPORTANT;//"Important";
    }
    else
    {
        item->string = NSTV_NORMAL;//"Normal";
    }
    item->x_offset = 0;
    item->image = NULL;
    // col 2, read status
    item = item->next;
    if(thiz_mail_info.mail_head[item->sel].read_status == 1)
    {
        item->string = "No";
    }
    else
    {
        item->string = "Yes";
    }
    item->x_offset = 0;
    item->image = NULL;
    // col 3, create time
    item = item->next;
    item->string = thiz_mail_info.mail_head[item->sel].create_time;
    item->x_offset = 0;
    item->image = NULL;
    // col 4, title
    item = item->next;
    item->string = thiz_mail_info.mail_head[item->sel].title;
    item->x_offset = 0;
    item->image = NULL;

	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int On_wnd_nstv_mail_list_destroy(GuiWidget *widget, void *usrdata)
{
    _release_data();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_mail_list_got_focus(GuiWidget *widget, void *usrdata)
{
    _reflash_data();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_mail_list_keypress(GuiWidget *widget, void *usrdata)
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
                    if(0 < _get_mail_count())
                    {
                        // TODO: email body
                        unsigned short sel;
                        GUI_GetProperty(LSV_MAIL_LIST, "select", &sel);
                        app_nstv_mail_detail_exec(thiz_mail_info.mail_head[sel].title, sel);
                    }
					break;
				case STBK_GREEN:
                    if(0 < _get_mail_count())
                    {
                        _nstv_mail_delete_one();
                    }
					break;
				case STBK_RED:
                    if(0 < _get_mail_count())
                    {
                        //_delete_all_mail();
                        //_reflash_data();
                        _nstv_mail_delete_all();
                    }
					break;
				case STBK_BLUE:
	                break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_MAIL);
                    if(gFullscreen_flag == 1)
                    {
                        GUI_CreateDialog("wnd_channel_info");
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

#define EMAIL_DETAIL
#define WND_MAIL_DETAIL            "wnd_nstv_mail_detail"
#define PATH_MAIL_DETAIL           "ca/nstv/wnd_nstv_mail_detail.xml"
#define TXT_MAIL_DETAIL_SUBJECT    "txt_nstv_mail_detail_title"
#define NTP_MAIL_DETAIL_CONTENT    "ntp_nstv_mail_detail_content"

static char* thiz_email_detail_content = NULL;
static void _release_email_detail_data(void)
{
   if(thiz_email_detail_content) 
   {
        GxCore_Free(thiz_email_detail_content);
        thiz_email_detail_content = NULL;
   }
}

void app_nstv_mail_detail_exec(char *subject, unsigned int pos)
{
    int ret = 0;
    static int init_flag = 0;

    if(NULL == subject)
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return;
    }
    if(thiz_email_detail_content)
    {
        GxCore_Free(thiz_email_detail_content);
        thiz_email_detail_content = NULL;
    }
    thiz_email_detail_content = GxCore_Mallocz(MAIL_BODY_SIZE);
    if(NULL == thiz_email_detail_content)
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return ;
    }
    ret = _mail_body_get_by_pos(pos, thiz_email_detail_content, EMAIL_BODY_SIZE);
    if(0 > ret)
    {
        GxCore_Free(thiz_email_detail_content);
        thiz_email_detail_content = NULL;
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return ;
    }
    if(init_flag == 0)
    {
        GUI_LinkDialog(WND_MAIL_DETAIL, PATH_MAIL_DETAIL);
        init_flag = 1;
    }
    app_create_dialog(WND_MAIL_DETAIL);
    // init data
    GUI_SetProperty(TXT_MAIL_DETAIL_SUBJECT, "string", subject);
    GUI_SetProperty(NTP_MAIL_DETAIL_CONTENT, "string", thiz_email_detail_content);
}


SIGNAL_HANDLER int On_wnd_nstv_mail_detail_destroy(GuiWidget *widget, void *usrdata)
{
    _release_email_detail_data();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_mail_detail_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
    unsigned int value = 1;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
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
                case STBK_UP:
                    GUI_SetProperty(NTP_MAIL_DETAIL_CONTENT, "line_up", &value);
                    break;
                case STBK_DOWN:
                    GUI_SetProperty(NTP_MAIL_DETAIL_CONTENT, "line_down", &value);
                    break;
                case STBK_PAGE_UP:
                    GUI_SetProperty(NTP_MAIL_DETAIL_CONTENT, "page_up", &value);
                    break;
                case STBK_PAGE_DOWN:
                    GUI_SetProperty(NTP_MAIL_DETAIL_CONTENT, "page_down", &value);
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_MAIL_DETAIL);
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
SIGNAL_HANDLER int On_lsv_nstv_mail_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_lsv_nstv_mail_list_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_mail_list_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_mail_list_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_mail_list_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_mail_detail_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_mail_detail_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#endif
