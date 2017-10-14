/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_ippv.c
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
#define WND_IPPV                    "wnd_nstv_ippv_history"
#define PATH_XML                    "ca/nstv/wnd_nstv_ippv_history.xml"
#define TXT_TITLE                   "txt_nstv_ippv_history_title"
#define LSV_IPPV                    "lsv_nstv_ippv_history_list"


static CASCAMNSTVIPPVInfoClass thiz_ippv_info = {0};

static int _ippv_init(unsigned short operator_id)
{
    CASCAMDataGetClass data_get = {0};
    unsigned short ippv_count = 300;

    memset(&data_get, 0, sizeof(CASCAMDataGetClass));
    data_get.cas_flag = CASCAM_NSTV;
    data_get.main_id = IPPV_INFO;
    //data_get.sub_id = NSTV_OPERATOR_ippv_COUNT;
    //data_get.data_buf = (void*)&ippv_count;
    //data_get.buf_len = sizeof(unsigned short);
    if (1)//(0 == CASCAM_Get_Property(&data_get))
    {
        printf("\033[32mnstv entitle info:\n\033[0m");
        //printf("\033[32mentitlement info count:\033[0m \033[33m%d\n\033[0m", ippv_count);
        //if(ippv_count > 0)
        {
            thiz_ippv_info.info_count = ippv_count;
            thiz_ippv_info.operator_id = operator_id;
            thiz_ippv_info.info_unit = GxCore_Mallocz(sizeof(CASCAMNSTVIPPVUnitClass)*ippv_count);
            if(NULL == thiz_ippv_info.info_unit)
            {
               printf("\nCAS,error, %s, %d\n",__FUNCTION__,__LINE__);
               return -1;
            }
            data_get.sub_id = NSTV_IPPV_DATA;
            data_get.data_buf = (void*)&thiz_ippv_info;
            data_get.buf_len = sizeof(CASCAMNSTVIPPVInfoClass);
            if(0 == CASCAM_Get_Property(&data_get))
            {
#if 0
                int i = 0;
                for(i = 0; i < thiz_ippv_info.info_count; i++)
                {
                        printf("\033[33m%3d    %s     %d    %d    %d    %s\n\033[0m", 
                            i+1, thiz_ippv_info.info_unit[i].product_id,  
                            thiz_ippv_info.info_unit[i].product_status, 
                            thiz_ippv_info.info_unit[i].record_flag,
                            thiz_ippv_info.info_unit[i].product_price,
                            thiz_ippv_info.info_unit[i].expired_time);
                }
#endif
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
    if(thiz_ippv_info.info_unit)
    {
        GxCore_Free(thiz_ippv_info.info_unit);
    }
    memset(&thiz_ippv_info, 0, sizeof(thiz_ippv_info));
}

void app_nstv_ippv_exec(unsigned short operator_id)
{
    static char init_flag = 0;
    _ippv_init(operator_id);
    // create the UI
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_IPPV, PATH_XML);
    }
    app_create_dialog(WND_IPPV);
    GUI_SetProperty(TXT_TITLE, "string", NSTV_IPPV_INFO);
}


// for UI
SIGNAL_HANDLER int On_lsv_nstv_ippv_history_list_get_total(GuiWidget *widget, void *usrdata)
{
	return thiz_ippv_info.info_count;
}

SIGNAL_HANDLER int On_lsv_nstv_ippv_history_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = NULL;
    static char buf[10] ;
    static char buf1[10] ;
    static char buf2[10] ;
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
    item->string = thiz_ippv_info.info_unit[item->sel].product_id;
    item->x_offset = 0;
    item->image = NULL;
    // col 2
    item = item->next;
    memset(buf1, 0, sizeof(buf1));
    sprintf(buf1,"%d", (thiz_ippv_info.info_unit[item->sel].slot_id));
    item->string = buf1;
    item->x_offset = 0;
    item->image = NULL;
    // col 3
    item = item->next;
    if(thiz_ippv_info.info_unit[item->sel].product_status == 0)
        item->string = NSTV_UNKNOW;
    else if(thiz_ippv_info.info_unit[item->sel].product_status == 1)
        item->string = NSTV_BOOKING;
    else if(thiz_ippv_info.info_unit[item->sel].product_status == 2)
        item->string = NSTV_EXPIRED;
    else
        item->string = NSTV_VIEWED;
    item->x_offset = 0;
    item->image = NULL;
    // col 4
    item = item->next;
    if(thiz_ippv_info.info_unit[item->sel].record_flag)
        item->string = NSTV_ALLOW;
    else
        item->string = NSTV_FORBIDDEN;
    item->x_offset = 0;
    item->image = NULL;
    // col 5
    item = item->next;
    memset(buf2, 0, sizeof(buf2));
    sprintf(buf2, "%u", thiz_ippv_info.info_unit[item->sel].product_price);
    item->string = buf2;
    item->x_offset = 0;
    item->image = NULL;
    // col 6
    item = item->next;
    item->string = thiz_ippv_info.info_unit[item->sel].expired_time;
    item->x_offset = 0;
    item->image = NULL;

	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int On_wnd_nstv_ippv_history_destroy(GuiWidget *widget, void *usrdata)
{
    _release_data();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_ippv_history_keypress(GuiWidget *widget, void *usrdata)
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
                    GUI_EndDialog(WND_IPPV);
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
SIGNAL_HANDLER int On_lsv_nstv_ippv_history_list_get_total(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_lsv_nstv_ippv_history_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_ippv_history_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_ippv_history_keypress(GuiWidget *widget, void *usrdata)
{
    return  EVENT_TRANSFER_STOP;
}
#endif
