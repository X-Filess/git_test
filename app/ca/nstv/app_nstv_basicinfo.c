/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_baseinfo.c
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

#if NSCAS_SUPPORT

#if NSCAS_SMC_VERSION
#define PATH_XML                        "ca/nstv/wnd_nstv_baseinfo.xml"
#define WND_NSTV_BASEINFO               "wnd_nstv_baseinfo"
#define TXT_TITLE                       "txt_nstv_baseinfo_title"
#define TXT_CARD_SN_L                   "txt_nstv_baseinfo_item1_l"
#define TXT_CARD_SN_R                   "txt_nstv_baseinfo_item1_r"
#define TXT_STB_ID_L                    "txt_nstv_baseinfo_item2_l"
#define TXT_STB_ID_R                    "txt_nstv_baseinfo_item2_r"
#define TXT_STB_MATCH_STATUS_L          "txt_nstv_baseinfo_item3_l"
#define TXT_STB_MATCH_STATUS_R          "txt_nstv_baseinfo_item3_r"
#define TXT_CAS_VERSION_L               "txt_nstv_baseinfo_item4_l"
#define TXT_CAS_VERSION_R               "txt_nstv_baseinfo_item4_r"
#define TXT_WORK_TIME_L                 "txt_nstv_baseinfo_item5_l"
#define TXT_WORK_TIME_R                 "txt_nstv_baseinfo_item5_r"
#define TXT_WATCH_RATING_L              "txt_nstv_baseinfo_item6_l"
#define TXT_WATCH_RATING_R              "txt_nstv_baseinfo_item6_r"
#define TXT_OPERATOR_ID_L               "txt_nstv_baseinfo_item7_l"
#define TXT_OPERATOR_ID_R               "txt_nstv_baseinfo_item7_r"
#define TXT_MARKET_ID_L                 "txt_nstv_baseinfo_item8_l"
#define TXT_MARKET_ID_R                 "txt_nstv_baseinfo_item8_r"
#define TXT_JTAG_STATUS_L               "txt_nstv_baseinfo_item9_l"
#define TXT_JTAG_STATUS_R               "txt_nstv_baseinfo_item9_r"
#define TXT_SECURITY_START_L            "txt_nstv_baseinfo_item10_l"
#define TXT_SECURITY_START_R            "txt_nstv_baseinfo_item10_r"
#else
#define PATH_XML                        "ca/nstv/wnd_nstv_baseinfo_nocard.xml"
#define WND_NSTV_BASEINFO               "wnd_nstv_baseinfo_nocard"
#define TXT_TITLE                       "txt_nstvnocard_baseinfo_title"
#define TXT_STB_ID_L                    "txt_nstvnocard_baseinfo_item2_l"
#define TXT_STB_ID_R                    "txt_nstvnocard_baseinfo_item2_r"
#define TXT_CAS_VERSION_L               "txt_nstvnocard_baseinfo_item4_l"
#define TXT_CAS_VERSION_R               "txt_nstvnocard_baseinfo_item4_r"
#define TXT_WATCH_RATING_L              "txt_nstvnocard_baseinfo_item6_l"
#define TXT_WATCH_RATING_R              "txt_nstvnocard_baseinfo_item6_r"
#define TXT_OPERATOR_ID_L               "txt_nstvnocard_baseinfo_item7_l"
#define TXT_OPERATOR_ID_R               "txt_nstvnocard_baseinfo_item7_r"
#define TXT_MARKET_ID_L                 "txt_nstvnocard_baseinfo_item8_l"
#define TXT_MARKET_ID_R                 "txt_nstvnocard_baseinfo_item8_r"
#define TXT_JTAG_STATUS_L               "txt_nstvnocard_baseinfo_item9_l"
#define TXT_JTAG_STATUS_R               "txt_nstvnocard_baseinfo_item9_r"
#define TXT_SECURITY_START_L            "txt_nstvnocard_baseinfo_item10_l"
#define TXT_SECURITY_START_R            "txt_nstvnocard_baseinfo_item10_r"
#endif


SIGNAL_HANDLER int app_nstv_baseinfo_keypress(GuiWidget *widget, void *usrdata)
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
                        GUI_EndDialog(WND_NSTV_BASEINFO);
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

#if NSCAS_SMC_VERSION
int app_nstv_get_card_id(char* card_id, unsigned int *size)
{
    CASCAMNSTVBasicInfoClass base_info;
    CASCAMDataGetClass data_get;
    if((NULL == card_id) || (NULL == size) || (sizeof(base_info.card_sn) > *size))
    {
        printf("\nNSTV, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }

    memset(&base_info, 0, sizeof(CASCAMNSTVBasicInfoClass));
    memset(&data_get, 0, sizeof(CASCAMDataGetClass));
    data_get.cas_flag = CASCAM_NSTV;
    data_get.main_id = BASIC_INFO;
    data_get.sub_id = NSTV_BASIC_ALL;
    data_get.data_buf = (void*)&base_info;
    data_get.buf_len = sizeof(CASCAMNSTVBasicInfoClass);
    if(0 == CASCAM_Get_Property(&data_get))
    {
        *size = sizeof(base_info.card_sn);
        memcpy(card_id, base_info.card_sn, *size);
        return 0;
    }
    return -1;
}
#endif

int app_nstv_get_stb_id(char* stb_id, unsigned int *size)
{
    CASCAMNSTVBasicInfoClass base_info;
    CASCAMDataGetClass data_get;
    if((NULL == stb_id) || (NULL == size) || (sizeof(base_info.stb_id) > *size))
    {
        printf("\nNSTV, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }

    memset(&base_info, 0, sizeof(CASCAMNSTVBasicInfoClass));
    memset(&data_get, 0, sizeof(CASCAMDataGetClass));
    data_get.cas_flag = CASCAM_NSTV;
    data_get.main_id = BASIC_INFO;
    data_get.sub_id = NSTV_BASIC_ALL;
    data_get.data_buf = (void*)&base_info;
    data_get.buf_len = sizeof(CASCAMNSTVBasicInfoClass);
    if(0 == CASCAM_Get_Property(&data_get))
    {
        *size = sizeof(base_info.stb_id);
        memcpy(stb_id, base_info.stb_id, *size);
        return 0;
    }
    return -1;
}

void app_nstv_baseinfo_exec(void)
{
    CASCAMNSTVBasicInfoClass base_info;
    CASCAMDataGetClass data_get;
    static char init_flag = 0;

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
        printf("\033[32mjtag status:\033[0m \033[33m %s\033[0m", base_info.jtag_status);
        printf("\033[32msecurity start status:\033[0m \033[33m %s\033[0m", base_info.security_start);
        printf("\n--------------------------------------------\n");
        // TODO: for UI
        if(0 == init_flag)
        {
            init_flag = 1;
            GUI_LinkDialog(WND_NSTV_BASEINFO, PATH_XML);
        }

        app_create_dialog(WND_NSTV_BASEINFO);
        // title
        GUI_SetProperty(TXT_TITLE, "string", NSTV_BASIC_INFO);

#if NSCAS_SMC_VERSION
        // card sn
        GUI_SetProperty(TXT_CARD_SN_L, "string", NSTV_CARD_SN);
        GUI_SetProperty(TXT_CARD_SN_R, "string", base_info.card_sn);
        // stb match status
        GUI_SetProperty(TXT_STB_MATCH_STATUS_L, "string", NSTV_STB_MATCH_STATUS);
        GUI_SetProperty(TXT_STB_MATCH_STATUS_R, "string", base_info.stb_match_status);
        // work time
        GUI_SetProperty(TXT_WORK_TIME_L, "string", NSTV_WORK_TIME);
        GUI_SetProperty(TXT_WORK_TIME_R, "string", base_info.work_time);
#endif
        // stb id
        GUI_SetProperty(TXT_STB_ID_L, "string", NSTV_STB_ID);
        GUI_SetProperty(TXT_STB_ID_R, "string", base_info.stb_id);
        // cas version
        GUI_SetProperty(TXT_CAS_VERSION_L, "string", NSTV_CAS_VERSION);
        GUI_SetProperty(TXT_CAS_VERSION_R, "string", base_info.cas_version);
        // watch rating
        GUI_SetProperty(TXT_WATCH_RATING_L, "string", NSTV_WATCH_RATING);
        GUI_SetProperty(TXT_WATCH_RATING_R, "string", base_info.watch_rating);
        // operator id
        GUI_SetProperty(TXT_OPERATOR_ID_L, "string", NSTV_OPERATOR_ID);
        GUI_SetProperty(TXT_OPERATOR_ID_R, "string", base_info.operator_id);
        // market id
        GUI_SetProperty(TXT_MARKET_ID_L, "string", NSTV_MARKET_ID);
        GUI_SetProperty(TXT_MARKET_ID_R, "string", base_info.market_id);
        // jtag status
        GUI_SetProperty(TXT_JTAG_STATUS_L, "string", NSTV_JTAG_STATUS);
        GUI_SetProperty(TXT_JTAG_STATUS_R, "string", base_info.jtag_status);
        // security start
        GUI_SetProperty(TXT_SECURITY_START_L, "string", NSTV_SECURITY_START_STATUS);
        GUI_SetProperty(TXT_SECURITY_START_R, "string", base_info.security_start);
    }
}
#else
SIGNAL_HANDLER int app_nstv_baseinfo_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
#endif
