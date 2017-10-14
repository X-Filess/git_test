/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_entitlement.c
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
#define WND_ENTITLEMENT             "wnd_nstv_entitlement_history"
#define PATH_XML                    "ca/nstv/wnd_nstv_entitlement_history.xml"
#define TXT_TITLE                   "txt_nstv_entitlement_history_title"
#define LSV_ENTITLEMENT             "lsv_nstv_entitlement_history_list"


static CASCAMNSTVEntitlementInfoClass thiz_entitlement_info = {0};

static int _entitlement_init(unsigned short operator_id)
{
    CASCAMDataGetClass data_get = {0};
    unsigned short entitle_count = 300;

    memset(&data_get, 0, sizeof(CASCAMDataGetClass));
    data_get.cas_flag = CASCAM_NSTV;
    data_get.main_id = ENTITLEMENT_INFO;
    //data_get.sub_id = NSTV_OPERATOR_ENTITLEMENT_COUNT;
    //entitle_count = operator_id;// input and output
    //data_get.data_buf = (void*)&entitle_count;
    //data_get.buf_len = sizeof(unsigned short);
    if (1)//(0 == CASCAM_Get_Property(&data_get))
    {
        printf("\033[32mnstv entitle info:\n\033[0m");
        //printf("\033[32mentitlement info count:\033[0m \033[33m%d\n\033[0m", entitle_count);
        //if(entitle_count > 0)
        {
            thiz_entitlement_info.info_count = entitle_count;
            thiz_entitlement_info.operator_id = operator_id;
            thiz_entitlement_info.info_unit = GxCore_Mallocz(sizeof(CASCAMNSTVEntitlementUnitClass)*entitle_count);
            if(NULL == thiz_entitlement_info.info_unit)
            {
               printf("\nCAS,error, %s, %d\n",__FUNCTION__,__LINE__);
               return -1;
            }
            data_get.sub_id = NSTV_ENTITLEMENT_DATA;
            data_get.data_buf = (void*)&thiz_entitlement_info;
            data_get.buf_len = sizeof(CASCAMNSTVEntitlementInfoClass);
            if(0 == CASCAM_Get_Property(&data_get))
            {
                int i = 0;
                for(i = 0; i < thiz_entitlement_info.info_count; i++)
                {
                    printf("\033[33m%3d    %s     %s    %s    %d\n\033[0m", 
                            i+1, thiz_entitlement_info.info_unit[i].product_id,  
                            thiz_entitlement_info.info_unit[i].start_time, 
                            thiz_entitlement_info.info_unit[i].end_time,
                            thiz_entitlement_info.info_unit[i].is_support_record);
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
    if(thiz_entitlement_info.info_unit)
    {
        GxCore_Free(thiz_entitlement_info.info_unit);
    }
    memset(&thiz_entitlement_info, 0, sizeof(thiz_entitlement_info));
}

void app_nstv_entitlement_exec(unsigned short operator_id)
{
    static char init_flag = 0;
    _entitlement_init(operator_id);
    // create the UI
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_ENTITLEMENT, PATH_XML);
    }
    app_create_dialog(WND_ENTITLEMENT);
    GUI_SetProperty(TXT_TITLE, "string", NSTV_ENTITLEMENT_INFO);
}

// for entitle notify
void app_nstv_entitle_notify(char* param)
{
   NSTVEntitlementMessageClass *p = NULL; 
   if(NULL == param)
   {
        printf("\nnstv, error, %s, %d\n", __func__,__LINE__);
        return ;
   }
   if(GUI_CheckDialog(WND_ENTITLEMENT) == GXCORE_SUCCESS)
   {
        p = (NSTVEntitlementMessageClass*)param;
        if(thiz_entitlement_info.operator_id == p->operator_id)
        {// match, reflash
            _release_data();
            _entitlement_init(p->operator_id);
            GUI_SetProperty(LSV_ENTITLEMENT, "update_all", NULL);
        }
    }
}

#if 0
// 0 or -1: no entitlement or error; 1: find the entitlenment
int app_nstv_check_entitlement(void)
{
    CASCAMDataGetClass data_get;
    unsigned short operator_count = 0;
    CASCAMNSTVOperatorInfoClass operator_info = {0};

    memset(&data_get, 0, sizeof(CASCAMDataGetClass));
    data_get.cas_flag = CASCAM_NSTV;
    data_get.main_id = OPERATOR_INFO;
    data_get.sub_id = NSTV_OPERATOR_INFO_COUNT;
    data_get.data_buf = (void*)&operator_count;
    data_get.buf_len = sizeof(unsigned short);
    if(0 == CASCAM_Get_Property(&data_get))
    {
        printf("\033[32mNSTV operator info:\n\033[0m");
        printf("\033[32mOperator info count:\033[0m \033[33m%d\n\033[0m", operator_count);
        if(operator_count > 0)
        {
            operator_info.operator_count = operator_count;
            operator_info.operator_info = GxCore_Mallocz(sizeof(CASCAMNSTVOperatorUnitClass)*operator_count);
            if(NULL == operator_info.operator_info)
            {
               printf("\nCAS,error, %s, %d\n",__FUNCTION__,__LINE__);
               return -1;
            }
            data_get.sub_id = NSTV_OPERATOR_DATA;
            data_get.data_buf = (void*)&operator_info;
            data_get.buf_len = sizeof(CASCAMNSTVOperatorInfoClass);
            if(0 == CASCAM_Get_Property(&data_get))
            {
                int i = 0;
                unsigned short entitle_count;
                for(i = 0; i < operator_info.operator_count; i++)
                {
                    //printf("\033[33m%3d    %s     %s\n\033[0m", 
                    //        i+1, operator_info.operator_info[i].operator_id,  
                    //        operator_info.operator_info[i].operator_des);

                    memset(&data_get, 0, sizeof(CASCAMDataGetClass));
                    data_get.cas_flag = CASCAM_NSTV;
                    data_get.main_id = ENTITLEMENT_INFO;
                    data_get.sub_id = NSTV_OPERATOR_ENTITLEMENT_COUNT;
                    entitle_count = atoi(operator_info.operator_info[i].operator_id);// input and output
                    data_get.data_buf = (void*)&entitle_count;
                    data_get.buf_len = sizeof(unsigned short);
                    if(0 == CASCAM_Get_Property(&data_get))
                    {
                        if(entitle_count > 0)
                        {// find entitlement history info
                            GxCore_Free(operator_info.operator_info);
                            return 1;
                        }
                    }
                }
                GxCore_Free(operator_info.operator_info);
            }
            else
            {
                printf("\nCAS,error, %s, %d\n",__FUNCTION__,__LINE__);
                GxCore_Free(operator_info.operator_info);
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
#endif

// for UI
SIGNAL_HANDLER int On_lsv_nstv_entitlement_history_list_get_total(GuiWidget *widget, void *usrdata)
{
	return thiz_entitlement_info.info_count;
}

SIGNAL_HANDLER int On_lsv_nstv_entitlement_history_list_get_data(GuiWidget *widget, void *usrdata)
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
    // col 0
    memset(buf, 0, sizeof(buf));
    sprintf(buf,"%d", (item->sel + 1));
    item->string = buf;
    item->x_offset = 2;
    item->image = NULL;
    // col 1
    item = item->next;
    item->string = thiz_entitlement_info.info_unit[item->sel].product_id;
    item->x_offset = 0;
    item->image = NULL;
    // col 2
    item = item->next;
    if(thiz_entitlement_info.info_unit[item->sel].is_support_record)
        item->string = NSTV_ALLOW;
    else
        item->string = NSTV_FORBIDDEN;
    item->x_offset = 0;
    item->image = NULL;
    // col 3
    item = item->next;
    item->string = thiz_entitlement_info.info_unit[item->sel].start_time;
    item->x_offset = 0;
    item->image = NULL;
    // col 4
    item = item->next;
    item->string = thiz_entitlement_info.info_unit[item->sel].end_time;
    item->x_offset = 0;
    item->image = NULL;

	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int On_wnd_nstv_entitlement_history_destroy(GuiWidget *widget, void *usrdata)
{
    _release_data();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_entitlement_history_keypress(GuiWidget *widget, void *usrdata)
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
                    GUI_EndDialog(WND_ENTITLEMENT);
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
SIGNAL_HANDLER int On_lsv_nstv_entitlement_history_list_get_total(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_lsv_nstv_entitlement_history_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_entitlement_history_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_entitlement_history_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

#endif
