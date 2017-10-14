/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_detitle.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.04.10		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_config.h"

#if NSCAS_SUPPORT
#define WND_DETITLE                 "wnd_nstv_detitle_history"
#define PATH_XML                    "ca/nstv/wnd_nstv_detitle_history.xml"
#define TXT_TITLE                   "txt_nstv_detitle_history_title"
#define LSV_DETITLE                 "lsv_nstv_detitle_history_list"
#define IMG_DEL_ALL                 "img_nstv_detitle_history_list_item1"
#define TXT_DEL_ALL                 "txt_nstv_detitle_history_list_item1"
#define IMG_DEL                     "img_nstv_detitle_history_list_item2"
#define TXT_DEL                     "txt_nstv_detitle_history_list_item2"


static CASCAMNSTVDetitleInfoClass thiz_detitle_info = {0};

static int _detitle_init(unsigned short operator_id)
{
    CASCAMDataGetClass data_get = {0};
    unsigned short detitle_count = 5;

    memset(&data_get, 0, sizeof(CASCAMDataGetClass));
    data_get.cas_flag = CASCAM_NSTV;
    data_get.main_id = DETITLE_INFO;
    //data_get.sub_id = NSTV_OPERATOR_DETITLE_COUNT;
    //detitle_count = operator_id;// input and output
    //data_get.data_buf = (void*)&detitle_count;
    //data_get.buf_len = sizeof(unsigned short);
    if (1)//(0 == CASCAM_Get_Property(&data_get))
    {
        printf("\033[32mnstv detitle info:\n\033[0m");
        //printf("\033[32mdetitle  info count:\033[0m \033[33m%d\n\033[0m", detitle_count);
        //if(detitle_count > 0)
        {
            thiz_detitle_info.info_count = detitle_count;
            thiz_detitle_info.operator_id = operator_id;
            thiz_detitle_info.info_unit = GxCore_Mallocz(sizeof(CASCAMNSTVDetitleUnitClass)*detitle_count);
            if(NULL == thiz_detitle_info.info_unit)
            {
               printf("\nCAS,error, %s, %d\n",__FUNCTION__,__LINE__);
               return -1;
            }
            data_get.sub_id = NSTV_DETITLE_DATA;
            data_get.data_buf = (void*)&thiz_detitle_info;
            data_get.buf_len = sizeof(CASCAMNSTVDetitleInfoClass);
            if(0 == CASCAM_Get_Property(&data_get))
            {
                int i = 0;
                for(i = 0; i < thiz_detitle_info.info_count; i++)
                {
                    printf("\033[33m%3d    0x%x    %d\n\033[0m", 
                            i+1, thiz_detitle_info.info_unit[i].detitle_code,  
                            thiz_detitle_info.info_unit[i].read_flag);
                }
            }
            else
            {
                printf("\nCAS,error, %s, %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
         }
    }
    else
    {
        printf("\nfailed, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    return 0;
}

static void _release_data(void)
{// release the resource before exit the menu
    if(thiz_detitle_info.info_unit)
    {
        GxCore_Free(thiz_detitle_info.info_unit);
    }
    memset(&thiz_detitle_info, 0, sizeof(thiz_detitle_info));
}

static void _reflash_data(void)
{
    int sel = 0;
    int operator_id = thiz_detitle_info.operator_id;

    GUI_GetProperty(LSV_DETITLE, "select", &sel);
    _release_data();
    _detitle_init(operator_id);
    // for listview
    GUI_SetProperty(LSV_DETITLE, "update_all", NULL);
    if(0 == thiz_detitle_info.info_count)
    {
        GUI_SetProperty(IMG_DEL_ALL, "state", "hide");
        GUI_SetProperty(TXT_DEL_ALL, "state", "hide");
        GUI_SetProperty(IMG_DEL, "state", "hide");
        GUI_SetProperty(TXT_DEL, "state", "hide");
    }
    else
    {
        if(sel >= thiz_detitle_info.info_count)
        {
            sel = sel - 1;
        }
        GUI_SetProperty(LSV_DETITLE, "select", &sel);
    }
}

// delete all
static int _delete_all(void)
{
    int ret = 0;
    CASCAMDataSetClass data_set;
    CASCAMNSTVDetitleDeleteClass delete_param;

    memset(&data_set, 0, sizeof(CASCAMDataSetClass));
    memset(&delete_param, 0, sizeof(CASCAMNSTVDetitleDeleteClass));
    delete_param.operator_id = thiz_detitle_info.operator_id;
    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = DETITLE_CONTROL;
    data_set.sub_id = NSTV_DETITLE_DEL;
    data_set.data_buf = (void*)&delete_param;
    data_set.buf_len = sizeof(CASCAMNSTVDetitleDeleteClass);
    if(0 != CASCAM_Set_Property(&data_set))
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    _reflash_data();
    return ret;
}

static int _delete_by_detitle_code(int sel)
{
    int ret = 0;
    CASCAMDataSetClass data_set;
    CASCAMNSTVDetitleDeleteClass delete_param;

    memset(&data_set, 0, sizeof(CASCAMDataSetClass));
    memset(&delete_param, 0, sizeof(CASCAMNSTVDetitleDeleteClass));
    delete_param.operator_id = thiz_detitle_info.operator_id;
    delete_param.detitle_code = thiz_detitle_info.info_unit[sel].detitle_code;
    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = DETITLE_CONTROL;
    data_set.sub_id = NSTV_DETITLE_DEL;
    data_set.data_buf = (void*)&delete_param;
    data_set.buf_len = sizeof(CASCAMNSTVDetitleDeleteClass);
    if(0 != CASCAM_Set_Property(&data_set))
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    _reflash_data();
    return ret;
}


void app_nstv_detitle_exec(unsigned short operator_id)
{
    static char init_flag = 0;
    _detitle_init(operator_id);
    // create the UI
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_DETITLE, PATH_XML);
    }
    app_create_dialog(WND_DETITLE);
    GUI_SetProperty(TXT_TITLE, "string", NSTV_DETITLE);
}

// for notice
#define PROMPT_DETITLE_NOTICE_NEW_NAME    "prompt_ca_detitle_new"
#define WND_DETITLE_NOTICE_NEW            "ca_detitle_new_prompt"

static event_list  *thiz_query_flicker_timer = NULL; 
static int _query_flicker_timer(void *usrdata)
{
    static char flag = 0;
    if(0 == flag)
    {
        GUI_SetProperty(PROMPT_DETITLE_NOTICE_NEW_NAME, "state", "hide");
        flag = 1;
    }
    else
    {
        GUI_SetProperty(PROMPT_DETITLE_NOTICE_NEW_NAME, "state", "show");
        flag = 0;
    }
    return 0;
}

static int _create_flicker_timer(unsigned int duration_sec)
{
    if (reset_timer(thiz_query_flicker_timer) != 0)
    {
        thiz_query_flicker_timer = create_timer(_query_flicker_timer, duration_sec*1000, NULL, TIMER_REPEAT);
    }
    return 0;
}

static int _destroy_flicker_timer(void)
{
    int ret = 0;
    if(thiz_query_flicker_timer != NULL)
    {
        remove_timer(thiz_query_flicker_timer);
        thiz_query_flicker_timer = NULL;
    }

    return ret;
}

void app_nstv_detitle_notify(char type)
{
#define X1_DEFAUT 900
#define Y1_DEFAULT 250
    unsigned int x_pos = 0;
    unsigned int y_pos = 0;
    char *prompt_name = NULL;
    char *wnd_name = NULL;

    prompt_name = PROMPT_DETITLE_NOTICE_NEW_NAME;
    wnd_name = WND_DETITLE_NOTICE_NEW;
    x_pos = X1_DEFAUT;
    y_pos = Y1_DEFAULT;

    switch(type)
    {
        case NSTV_DETITLE_RECEIVE_NEW:
            if(GUI_CheckPrompt(prompt_name) == GXCORE_SUCCESS)
            {
                _destroy_flicker_timer();
                GUI_EndPrompt(prompt_name);
            }
            GUI_CreatePrompt(x_pos, y_pos, prompt_name, wnd_name, NULL, "ontop");

            break;
        case NSTV_DETITLE_SPACE_FULL:
            if(GUI_CheckPrompt(prompt_name) == GXCORE_SUCCESS)
            {
                _destroy_flicker_timer();
                GUI_EndPrompt(prompt_name);
            }
            GUI_CreatePrompt(x_pos, y_pos, prompt_name, wnd_name, NULL, "ontop");
            _create_flicker_timer(1);// 1s
            break;
        default:
            printf("\nNSTV, error, %s, %d\n", __FUNCTION__,__LINE__);
            break;
    }
}

void app_nstv_detitle_notify_exit(void)
{// for hide the flag
    if(GUI_CheckPrompt(PROMPT_DETITLE_NOTICE_NEW_NAME) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(PROMPT_DETITLE_NOTICE_NEW_NAME);
    }
    _destroy_flicker_timer();
}


// for UI
SIGNAL_HANDLER int On_lsv_nstv_detitle_history_list_get_total(GuiWidget *widget, void *usrdata)
{
	return thiz_detitle_info.info_count;
}

SIGNAL_HANDLER int On_lsv_nstv_detitle_history_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = NULL;
    static char buf[10] ;
    static char buf2[20];
    item = (ListItemPara*)usrdata;
    if(NULL == item)
    {
        return GXCORE_ERROR;
    }
    if(0 > item->sel)
    {
        return GXCORE_ERROR;
    }
    // col 0
    memset(buf, 0, sizeof(buf));
    sprintf(buf,"%d", (item->sel + 1));
    item->string = buf;
    item->x_offset = 2;
    item->image = NULL;
    // col 1
    item = item->next;
    if(thiz_detitle_info.info_unit[item->sel].read_flag)
        item->string = NSTV_READ;
    else
        item->string = NSTV_UNREAD;
    item->x_offset = 0;
    item->image = NULL;
    // col 2
    item = item->next;
    memset(buf2, 0, sizeof(buf2));
    sprintf(buf2, "0x%x", thiz_detitle_info.info_unit[item->sel].detitle_code);
    item->string = buf2;
    item->x_offset = 0;
    item->image = NULL;

	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int On_wnd_nstv_detitle_history_destroy(GuiWidget *widget, void *usrdata)
{
    _release_data();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_detitle_history_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    int sel = 0;

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
                case STBK_RED:
                    _delete_all();
                    break;
                case STBK_YELLOW:
                    GUI_GetProperty(LSV_DETITLE, "select", &sel);
                    _delete_by_detitle_code(sel);
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_DETITLE);
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
SIGNAL_HANDLER int On_lsv_nstv_detitle_history_list_get_total(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_lsv_nstv_detitle_history_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_detitle_history_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_detitle_history_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#endif
