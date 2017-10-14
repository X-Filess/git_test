/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_operator.c
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
#include "full_screen.h"

#if NSCAS_SUPPORT
#define WND_OPERATOR                "wnd_nstv_operator"
#define PATH_XML                    "ca/nstv/wnd_nstv_operator.xml"
#define WND_OPERATOR_SUB            "wnd_nstv_operator_sub"
#define PATH_XML_SUB                "ca/nstv/wnd_nstv_operator_sub.xml"

#define TXT_TITLE                   "txt_nstv_operator_title"
#define LSV_OPERATOR                "lsv_nstv_operator_list"
#define TXT_OK                      "txt_nstv_operator_list_item1"
#define IMG_OK                      "img_nstv_operator_list_item1"

#define TXT_TITLE_SUB               "txt_nstv_operator_sub_title"
#define LSV_OPERATOR_SUB            "lsv_nstv_operator_sub_list"


static CASCAMNSTVOperatorInfoClass thiz_operator_info = {0};
static unsigned short thiz_operator_id = 0;
static int _operator_init(void)
{
    CASCAMDataGetClass data_get;
    unsigned short operator_count = 0;

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
            thiz_operator_info.operator_count = operator_count;
            thiz_operator_info.operator_info = GxCore_Mallocz(sizeof(CASCAMNSTVOperatorUnitClass)*operator_count);
            if(NULL == thiz_operator_info.operator_info)
            {
               printf("\nCAS,error, %s, %d\n",__FUNCTION__,__LINE__);
               return -1;
            }
            data_get.sub_id = NSTV_OPERATOR_DATA;
            data_get.data_buf = (void*)&thiz_operator_info;
            data_get.buf_len = sizeof(CASCAMNSTVOperatorInfoClass);
            if(0 == CASCAM_Get_Property(&data_get))
            {
                int i = 0;
                for(i = 0; i < thiz_operator_info.operator_count; i++)
                {
                    printf("\033[33m%3d    %s     %s\n\033[0m", 
                            i+1, thiz_operator_info.operator_info[i].operator_id,  
                            thiz_operator_info.operator_info[i].operator_des);
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
    if(thiz_operator_info.operator_info)
    {
        GxCore_Free(thiz_operator_info.operator_info);
    }
    memset(&thiz_operator_info, 0, sizeof(thiz_operator_info));
    thiz_operator_id = 0xffff;
}

void app_nstv_operator_exec(void)
{
    static char init_flag = 0;
    _operator_init();
    // create the UI
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_OPERATOR, PATH_XML);
    }
    app_create_dialog(WND_OPERATOR);
    GUI_SetProperty(TXT_TITLE, "string", NSTV_OPERATOR_TITLE);
    if(0 == thiz_operator_info.operator_count)
    {
        GUI_SetProperty(TXT_OK, "state", "hide");
        GUI_SetProperty(IMG_OK, "state", "hide");
    }
    else
    {
        GUI_SetProperty(TXT_OK, "state", "show");
        GUI_SetProperty(IMG_OK, "state", "show");
    }
}

static void _nstv_operator_sub_exec(unsigned int sel)
{
    static char init_flag = 0;
    // create the UI
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_OPERATOR_SUB, PATH_XML_SUB);
    }
    thiz_operator_id = atoi((const char*)(thiz_operator_info.operator_info[sel].operator_id));
    printf("\n thiz operator id = %d\n", thiz_operator_id);
    app_create_dialog(WND_OPERATOR_SUB);
    GUI_SetProperty(TXT_TITLE_SUB, "string", NSTV_OPERATOR_TITLE);
}

// MAIN MENU
SIGNAL_HANDLER int On_lsv_nstv_operator_list_get_total(GuiWidget *widget, void *usrdata)
{
	return thiz_operator_info.operator_count;
}

SIGNAL_HANDLER int On_lsv_nstv_operator_list_get_data(GuiWidget *widget, void *usrdata)
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
    item->string = (char*)(thiz_operator_info.operator_info[item->sel].operator_id);
    item->x_offset = 0;
    item->image = NULL;
    // col 2
    item = item->next;
    item->string = (char*)(thiz_operator_info.operator_info[item->sel].operator_des);
    item->x_offset = 0;
    item->image = NULL;
	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int On_wnd_nstv_operator_destroy(GuiWidget *widget, void *usrdata)
{
    _release_data();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_operator_keypress(GuiWidget *widget, void *usrdata)
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
                case STBK_OK:
                    if(thiz_operator_info.operator_count > 0)
                    {
                        GUI_GetProperty(LSV_OPERATOR, "select", &sel);
                        _nstv_operator_sub_exec(sel);
                    }
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_OPERATOR);
                    break;
                default:
                    break;
            }
        default:
            break;
    }
    return ret;

}

// SUB MENU
extern int app_nstv_entitlement_exec(unsigned short operator_id);
extern int app_nstv_detitle_exec(unsigned short operator_id);
extern int app_nstv_wallet_exec(unsigned short operator_id);
extern int app_nstv_ippv_exec(unsigned short operator_id);
extern int app_nstv_features_exec(unsigned short operator_id);
extern int app_nstv_card_parent_child_info_exec(unsigned short operator_id);

typedef struct _NSTVOperatorSubMenuClass{
    char * sub_menu_name;
    int (*EntryFunc) (unsigned short operator_id);
}NSTVOperatorSubMenuClass;
static  NSTVOperatorSubMenuClass thiz_nstv_operator_sub[] =
{
    {NSTV_ENTITLEMENT,  app_nstv_entitlement_exec},

#if NSCAS_SMC_VERSION
    {NSTV_DETITLE,      app_nstv_detitle_exec},
    {NSTV_WALLET,       app_nstv_wallet_exec},
    {NSTV_IPPV,         app_nstv_ippv_exec},
#endif

    {NSTV_FEATURES,     app_nstv_features_exec},
    
#if NSCAS_SMC_VERSION
    {NSTV_CARD_PARENT_CHILD,	app_nstv_card_parent_child_info_exec},
#endif
};

#define GET_SUBMENU_COUNT(count) {count = (sizeof(thiz_nstv_operator_sub)/sizeof(NSTVOperatorSubMenuClass));}
SIGNAL_HANDLER int On_lsv_nstv_operator_sub_list_get_total(GuiWidget *widget, void *usrdata)
{
    unsigned int count = 0;
    GET_SUBMENU_COUNT(count);
	return count;
}

SIGNAL_HANDLER int On_lsv_nstv_operator_sub_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = NULL;
    //char buf[50] ;
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
    //memset(buf, 0, sizeof(buf));
    //memcpy(buf, thiz_nstv_operator_sub[item->sel].sub_menu_name, strlen(thiz_nstv_operator_sub[item->sel].sub_menu_name));
    //printf("\n operator sub menu name = %s\n", buf);
    item->string = thiz_nstv_operator_sub[item->sel].sub_menu_name;
    item->x_offset = 2;
    item->image = NULL;
    // col 1
    item = item->next;
    item->string = "Press Ok";
    item->x_offset = 0;
    item->image = NULL;
	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int On_wnd_nstv_operator_sub_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_operator_sub_keypress(GuiWidget *widget, void *usrdata)
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
                case STBK_OK:
                    // TODO: 20160410
                    GUI_GetProperty(LSV_OPERATOR_SUB, "select", &sel);
                    if(thiz_nstv_operator_sub[sel].EntryFunc)
                    {
                        thiz_nstv_operator_sub[sel].EntryFunc(thiz_operator_id);
                    }
                    else
                    {
                        printf("\n\033[39m %s no entry function\033[0m\n", thiz_nstv_operator_sub[sel].sub_menu_name);
                    }
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_OPERATOR_SUB);
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
SIGNAL_HANDLER int On_lsv_nstv_operator_list_get_total(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_lsv_nstv_operator_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_operator_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_operator_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_lsv_nstv_operator_sub_list_get_total(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_lsv_nstv_operator_sub_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_operator_sub_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_nstv_operator_sub_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#endif
