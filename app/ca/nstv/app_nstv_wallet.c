/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_wallet.c
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
#define WND_WALLET                  "wnd_nstv_wallet_history"
#define PATH_XML                    "ca/nstv/wnd_nstv_wallet_history.xml"
#define TXT_TITLE                   "txt_nstv_wallet_history_title"
#define LSV_WALLET                  "lsv_nstv_wallet_history_list"


static CASCAMNSTVWalletInfoClass thiz_wallet_info = {0};

static int _wallet_init(unsigned short operator_id)
{
    CASCAMDataGetClass data_get = {0};
    unsigned short wallet_count = 20;

    memset(&data_get, 0, sizeof(CASCAMDataGetClass));
    data_get.cas_flag = CASCAM_NSTV;
    data_get.main_id = WALLET_INFO;
    //data_get.sub_id = NSTV_OPERATOR_WALLET_COUNT;
    //wallet_count = operator_id;
    //data_get.data_buf = (void*)&wallet_count;
    //data_get.buf_len = sizeof(unsigned short);
    if (1)//(0 == CASCAM_Get_Property(&data_get))
    {
        printf("\033[32mnstv wallet info:\n\033[0m");
        //printf("\033[32mwallet  info count:\033[0m \033[33m%d\n\033[0m", wallet_count);
        //if(wallet_count > 0)
        {
            thiz_wallet_info.info_count = wallet_count;
            thiz_wallet_info.operator_id = operator_id;
            thiz_wallet_info.info_unit = GxCore_Mallocz(sizeof(CASCAMNSTVWalletUnitClass)*wallet_count);
            if(NULL == thiz_wallet_info.info_unit)
            {
               printf("\nCAS,error, %s, %d\n",__FUNCTION__,__LINE__);
               return -1;
            }
            data_get.sub_id = NSTV_WALLET_DATA;
            data_get.data_buf = (void*)&thiz_wallet_info;
            data_get.buf_len = sizeof(CASCAMNSTVWalletInfoClass);
            if(0 == CASCAM_Get_Property(&data_get))
            {
                int i = 0;
                for(i = 0; i < thiz_wallet_info.info_count; i++)
                {
                    printf("\033[33m%3d    %d    %d\n\033[0m", 
                            i+1, thiz_wallet_info.info_unit[i].credit_point,  
                            thiz_wallet_info.info_unit[i].used_point);
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
    if(thiz_wallet_info.info_unit)
    {
        GxCore_Free(thiz_wallet_info.info_unit);
    }
    memset(&thiz_wallet_info, 0, sizeof(thiz_wallet_info));
}

void app_nstv_wallet_exec(unsigned short operator_id)
{
    static char init_flag = 0;
    _wallet_init(operator_id);
    // create the UI
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_WALLET, PATH_XML);
    }
    app_create_dialog(WND_WALLET);
    GUI_SetProperty(TXT_TITLE, "string", NSTV_WALLET);
}

// for UI
SIGNAL_HANDLER int On_lsv_nstv_wallet_history_list_get_total(GuiWidget *widget, void *usrdata)
{
	return thiz_wallet_info.info_count;
}

SIGNAL_HANDLER int On_lsv_nstv_wallet_history_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = NULL;
    static char buf[10];
    static char buf1[10];
    static char buf2[20];
    static char buf3[20];
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
    memset(buf1, 0, sizeof(buf1));
    sprintf(buf1, "%d", thiz_wallet_info.info_unit[item->sel].wallet_id);
    item->string = buf1;
    item->x_offset = 0;
    item->image = NULL;
    // col 2
    item = item->next;
    memset(buf2, 0, sizeof(buf2));
    sprintf(buf2, "%u", thiz_wallet_info.info_unit[item->sel].credit_point);
    item->string = buf2;
    item->x_offset = 0;
    item->image = NULL;
    // col 3
    item = item->next;
    memset(buf3, 0, sizeof(buf3));
    sprintf(buf3, "%u", thiz_wallet_info.info_unit[item->sel].used_point);
    item->string = buf3;
    item->x_offset = 0;
    item->image = NULL;

	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int On_wnd_nstv_wallet_history_destroy(GuiWidget *widget, void *usrdata)
{
    _release_data();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_wallet_history_keypress(GuiWidget *widget, void *usrdata)
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
                    GUI_EndDialog(WND_WALLET);
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
SIGNAL_HANDLER int On_lsv_nstv_wallet_history_list_get_total(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_lsv_nstv_wallet_history_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_wallet_history_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_wallet_history_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
#endif
