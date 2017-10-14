/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_card_pair_info.c
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
#include "app.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#if NSCAS_SUPPORT
#define WND_NSTV_CARD_PAIR_INFO         "wnd_nstv_card_pair_info"
#define PATH_XML                        "ca/nstv/wnd_nstv_card_pair_info.xml"
#define TXT_TITLE                       "txt_nstv_card_pair_info_title"
#define TXT_CARD_PAIR_INFO_L            "txt_nstv_card_pair_info_item1_l"
#define TXT_CARD_PAIR_INFO_R            "txt_nstv_card_pair_info_item1_r"

void app_nstv_card_pair_info_exec(void)
{
    static char init_flag = 0;
    
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_NSTV_CARD_PAIR_INFO, PATH_XML);
    }

    app_create_dialog(WND_NSTV_CARD_PAIR_INFO);
    // title
    GUI_SetProperty(TXT_TITLE, "string", NSTV_CARD_PAIR_INFO);

	CASCAMNSTVBasicInfoClass base_info;
    CASCAMDataGetClass data_get;
	memset(&base_info, 0, sizeof(CASCAMNSTVBasicInfoClass));
    memset(&data_get, 0, sizeof(CASCAMDataGetClass));
    data_get.cas_flag = CASCAM_NSTV;
    data_get.main_id = BASIC_INFO;
    data_get.sub_id = NSTV_BASIC_ALL;
    data_get.data_buf = (void*)&base_info;
    data_get.buf_len = sizeof(CASCAMNSTVBasicInfoClass);
	if(0 == CASCAM_Get_Property(&data_get))
    {
        printf("\n--------------------------------------------\n");
        printf("\033[32mnstv basic info:\n\033[0m");
        printf("\033[32mcard sn:\033[0m \033[33m%s\n\033[0m", base_info.card_sn);
        printf("\033[32mstb id:\033[0m \033[33m %s\n\033[0m", base_info.stb_id);
        printf("\033[32mstb match status:\033[0m \033[33m %s\n\033[0m", base_info.stb_match_status);
        printf("\033[32mcas version:\033[0m \033[33m %s\n\033[0m", base_info.cas_version);
        printf("\033[32mwork time:\033[0m \033[33m %s\n\033[0m", base_info.work_time);
        printf("\033[32mwatch rating:\033[0m \033[33m %s\n\033[0m", base_info.watch_rating);
        printf("\033[32moperator id:\033[0m \033[33m %s\n\033[0m", base_info.operator_id);
        printf("\033[32mmarket id:\033[0m \033[33m %s\n\033[0m", base_info.market_id);
        printf("\033[32mjtag status:\033[0m \033[33m %s\n\033[0m", base_info.jtag_status);
        printf("\033[32msecurity start status:\033[0m \033[33m %s\033[0m", base_info.security_start);
        printf("\n--------------------------------------------\n");

		GUI_SetProperty(TXT_CARD_PAIR_INFO_R, "string", base_info.stb_match_status);
	}
}

// UI
SIGNAL_HANDLER int app_nstv_card_pair_info_keypress(GuiWidget *widget, void *usrdata)
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
                        GUI_EndDialog(WND_NSTV_CARD_PAIR_INFO);
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
SIGNAL_HANDLER int app_nstv_card_pair_info_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
#endif
