/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_features.c
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
#define WND_FEATURES                "wnd_nstv_features_history"
#define PATH_XML                    "ca/nstv/wnd_nstv_features_history.xml"
#define TXT_TITLE                   "txt_nstv_features_history_title"
#define LSV_FEATURES                "lsv_nstv_features_history_list"
#define FEATURE_ID                  "hdr_nstv_features_history_features_id"

static CASCAMNSTVFeaturesInfoClass thiz_features_info = {0};

static int _features_init(unsigned short operator_id)
{
    CASCAMDataGetClass data_get = {0};
    unsigned short features_count = 18;

    memset(&data_get, 0, sizeof(CASCAMDataGetClass));
    data_get.cas_flag = CASCAM_NSTV;
    data_get.main_id = FEATURES_INFO;
    //data_get.sub_id = NSTV_OPERATOR_features_COUNT;
    //data_get.data_buf = (void*)&features_count;
    //data_get.buf_len = sizeof(unsigned short);
    if (1)//(0 == CASCAM_Get_Property(&data_get))
    {
        printf("\033[32mnstv features info:\n\033[0m");
        //printf("\033[32mfeatures info count:\033[0m \033[33m%d\n\033[0m", features_count);
        //if(features_count > 0)
        {
            thiz_features_info.info_count = features_count;
            thiz_features_info.operator_id = operator_id;
            thiz_features_info.info_unit = GxCore_Mallocz(sizeof(CASCAMNSTVFeaturesUnitClass)*features_count);
            if(NULL == thiz_features_info.info_unit)
            {
               printf("\nCAS,error, %s, %d\n",__FUNCTION__,__LINE__);
               return -1;
            }
            data_get.sub_id = NSTV_FEATURES_DATA;
            data_get.data_buf = (void*)&thiz_features_info;
            data_get.buf_len = sizeof(CASCAMNSTVFeaturesInfoClass);
            if(0 == CASCAM_Get_Property(&data_get))
            {
                int i = 0;
                for(i = 0; i < thiz_features_info.info_count; i++)
                {
                     printf("\033[33m%3d    %d     %d    %d    %d\n\033[0m",
                            i+1, thiz_features_info.info_unit[i].area_code,
                            thiz_features_info.info_unit[i].bouquet_id,
                            thiz_features_info.info_unit[i].features_id,
                            thiz_features_info.info_unit[i].features_data);
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
    if(thiz_features_info.info_unit)
    {
        GxCore_Free(thiz_features_info.info_unit);
    }
    memset(&thiz_features_info, 0, sizeof(thiz_features_info));
}

void app_nstv_features_exec(unsigned short operator_id)
{
    static char init_flag = 0;
    _features_init(operator_id);
    // create the UI
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_FEATURES, PATH_XML);
    }
    app_create_dialog(WND_FEATURES);
    GUI_SetProperty(TXT_TITLE, "string", NSTV_FEATURES_INFO);

#if NSCAS_SMC_VERSION
#else
    GUI_SetProperty(FEATURE_ID, "state", "hide");
#endif
}


// for UI
SIGNAL_HANDLER int On_lsv_nstv_features_history_list_get_total(GuiWidget *widget, void *usrdata)
{
	return thiz_features_info.info_count;
}

SIGNAL_HANDLER int On_lsv_nstv_features_history_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = NULL;
    static char buf[10] ;
    static char buf1[30] ;
    static char buf2[30] ;
    static char buf3[30] ;
    //static char buf4[30] ;
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
    sprintf(buf1,"%u", thiz_features_info.info_unit[item->sel].area_code);
    item->string = buf1;
    item->x_offset = 0;
    item->image = NULL;
    // col 2
    item = item->next;
    memset(buf2, 0, sizeof(buf2));
    sprintf(buf2,"%u", thiz_features_info.info_unit[item->sel].bouquet_id);
    item->string = buf2;
    item->x_offset = 0;
    item->image = NULL;

#if NSCAS_SMC_VERSION
    // col 3
    item = item->next;
    memset(buf3, 0, sizeof(buf3));
    //sprintf(buf3,"%u", thiz_features_info.info_unit[item->sel].features_id);
    sprintf(buf3,"%u", thiz_features_info.info_unit[item->sel].features_data);
    item->string = buf3;
    item->x_offset = 0;
    item->image = NULL;
#endif

#if 0
    // col 4
    item = item->next;
    memset(buf4, 0, sizeof(buf4));
    sprintf(buf4,"%u", thiz_features_info.info_unit[item->sel].features_data);
    item->string = buf4;
    item->x_offset = 0;
    item->image = NULL;
#endif
	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int On_wnd_nstv_features_history_destroy(GuiWidget *widget, void *usrdata)
{
    _release_data();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_features_history_keypress(GuiWidget *widget, void *usrdata)
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
                    GUI_EndDialog(WND_FEATURES);
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
SIGNAL_HANDLER int On_lsv_nstv_features_history_list_get_total(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_lsv_nstv_features_history_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_features_history_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_features_history_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

#endif
