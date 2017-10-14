/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_card_parent_child.c
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
#include "../../module/cascam/include/cascam_os.h"  
#include "../../module/cascam/include/cascam_platform.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#if NSCAS_SUPPORT
#define WND_NSTV_CARD_PARENT_CHILD      "wnd_nstv_card_parent_child"
#define PATH_XML                        "ca/nstv/wnd_nstv_card_parent_child.xml"
#define TXT_TITLE                       "txt_nstv_card_parent_child_info_title"
#define TXT_CARD_PARENT_CHILD_INFO_L    "txt_nstv_card_parent_child_info_item1_l"
#define TXT_CARD_PARENT_CHILD_INFO_R    "txt_nstv_card_parent_child_info_item1_r"

static unsigned short g_operator_id = 0;

void app_nstv_card_parent_child_osd(unsigned char status)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_nstv_card_parent_child"))
    {
        return;
    }
	
    if(status == 1)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.mode = POP_MODE_UNBLOCK;
        pop.type = POP_TYPE_NO_BTN;
        pop.timeout_sec = 5;
        pop.str = NSTV_OSD_INSERT_CHILD;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
    }
    else
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.mode = POP_MODE_UNBLOCK;
        pop.type = POP_TYPE_NO_BTN;
        pop.timeout_sec = 5;
        pop.str = NSTV_OSD_BAD_PARENT;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
    }
}

static void _cddate_to_ymd(unsigned short cd_data,
                            unsigned short* year,
                            unsigned char* month,
                            unsigned char* day)
{
    unsigned int mjd = 0;
    unsigned char week = 0;
    if((NULL == year) || (NULL == month) || (NULL == day))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    cascam_ymd_to_mjd(&mjd, (2000-1900), 1, 1);
    mjd += cd_data;
    cascam_mjd_to_ymd(mjd, year, month, day, &week);
}

int app_nstv_card_parent_child_info_exec(unsigned short operator_id)
{
    static char init_flag = 0;
    
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_NSTV_CARD_PARENT_CHILD, PATH_XML);
    }

    app_create_dialog(WND_NSTV_CARD_PARENT_CHILD);
    // title
    GUI_SetProperty(TXT_TITLE, "string", NSTV_CARD_PARENT_CHILD);

    g_operator_id = operator_id;

// get parent child card info
    CASCAMNSTVParentChildInfoClass data_info;
    memset(&data_info, 0, sizeof(CASCAMNSTVParentChildInfoClass));
    data_info.wTvsID = operator_id;

    CASCAMDataGetClass data_set;
    memset(&data_set, 0, sizeof(CASCAMDataGetClass));
    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = PARENT_CHILD_INFO;
    data_set.sub_id = NSTV_CARD_PARENT_CHILD_INFO_GET;
    data_set.data_buf = (void*)&data_info;
    data_set.buf_len = sizeof(CASCAMNSTVParentChildInfoClass);

    if(0 == CASCAM_Get_Property(&data_set))
    {
        if(data_info.pbyChild == 0) // parent or normal card
        {
            if(data_info.pbIsCanFeed == 0)
            {
            //  example:
            //  char card_data[] = "parent card\nCan not feed\nPress F1 will feed card\n";
                char card_data[128] = {0};
				cascam_strcat(card_data, NSTV_OSD_PARENT);
				cascam_strcat(card_data, "\n");
				cascam_strcat(card_data, NSTV_OSD_FEED_NO);
				cascam_strcat(card_data, "\n");
				cascam_strcat(card_data, NSTV_OSD_F1_FEED);
				cascam_strcat(card_data, "\n");
                GUI_SetProperty(TXT_CARD_PARENT_CHILD_INFO_R, "string", card_data);
            }
            else
            {
                // example:
                // char card_data[] = "parent card\nCan feed\nPress F1 will feed card\n";
                // char card_data[] = "child card\nparent card sn: 8000302100000341\nfeed pear: 24 hours\nlast feed time:2016:12:12 23:10\nCan feed\nPress F1 will feed card\n";
                char card_data[128] = {0};
				cascam_strcat(card_data, NSTV_OSD_PARENT);
				cascam_strcat(card_data, "\n");
				cascam_strcat(card_data, NSTV_OSD_FEED_YES);
				cascam_strcat(card_data, "\n");
				cascam_strcat(card_data, NSTV_OSD_F1_FEED);
				cascam_strcat(card_data, "\n");
				GUI_SetProperty(TXT_CARD_PARENT_CHILD_INFO_R, "string", card_data);
            }
        }
        else if(data_info.pbyChild == 1)// child card
        {
            char child_info[512] = {0};
            cascam_strcat(child_info, NSTV_OSD_CHILD);

            char sn_info[64] = {0};
            snprintf(sn_info, sizeof(sn_info), "%s: %s \n", NSTV_OSD_PARENT_SN, data_info.pParentCardSN);
            cascam_strcat(child_info, sn_info);

            char delay_info[64] = {0};
            snprintf(delay_info, sizeof(delay_info), "%s: %d %s\n", NSTV_OSD_FEED_PEAR, data_info.pbyDelayTime, NSTV_OSD_FEED_HOURS);
            cascam_strcat(child_info, delay_info);

            if(data_info.pLastFeedTime == 0)
            {
                char temp[64] = {0};
				snprintf(temp, sizeof(temp), "%s: 0\n", NSTV_OSD_LAST_FEED_TIME);
                cascam_strcat(child_info, temp);
            }
            else
            {
                char last_feed_time_info[128] = {0};
                unsigned short year = 0;
                unsigned char month = 0;
                unsigned char day = 0;
                unsigned short hour = 0;
                unsigned short minute = 0;
                unsigned short second = 0;
				
                hour = ((data_info.pLastFeedTime) >> 11) & 0x1F;
                minute = ((data_info.pLastFeedTime) >> 5) & 0x3F;
                second = (data_info.pLastFeedTime) & 0x1F;
                _cddate_to_ymd(((data_info.pLastFeedTime) >> 16) & 0xFFFF, &year, &month, &day);
                snprintf(last_feed_time_info, sizeof(last_feed_time_info), "%s: %4d-%02d-%02d %02d:%02d\n",
                      NSTV_OSD_LAST_FEED_TIME, year, month, day, hour, minute);

				cascam_strcat(child_info, last_feed_time_info);
            }

            if(data_info.pbIsCanFeed == 0)
            {
                char temp[64] = {0};
				snprintf(temp, sizeof(temp), "%s\n%s\n", NSTV_OSD_FEED_NO, NSTV_OSD_F1_FEED);
				cascam_strcat(child_info, temp);
            }
            else
            {
                char temp[64] = {0};
				snprintf(temp, sizeof(temp), "%s\n%s\n", NSTV_OSD_FEED_YES, NSTV_OSD_F1_FEED);
				cascam_strcat(child_info, temp);
            }

            GUI_SetProperty(TXT_CARD_PARENT_CHILD_INFO_R, "string", child_info);
        }
        else // unknown card
        {
            GUI_SetProperty(TXT_CARD_PARENT_CHILD_INFO_R, "string", NSTV_OSD_FEED_UNKNOWN_CARD);
        }
    }
    else
    {
        GUI_SetProperty(TXT_CARD_PARENT_CHILD_INFO_R, "string", NSTV_OSD_FEED_NO_CARD);
    }

    return 0;
}

static int _child_save(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
    // step 3 
    // feed child card

        CASCAMNSTVParentChildInfoClass data_info;
        memset(&data_info, 0, sizeof(CASCAMNSTVParentChildInfoClass));
        data_info.wTvsID = g_operator_id;

        CASCAMDataSetClass data_set;
        memset(&data_set, 0, sizeof(CASCAMDataSetClass));
    
        data_set.cas_flag = CASCAM_NSTV;
        data_set.main_id = PARENT_CHILD_CONTROL;
        data_set.sub_id = NSTV_CARD_PARENT_CHILD_FEED;
        data_set.data_buf = (void*)&data_info;
        data_set.buf_len = sizeof(CASCAMNSTVParentChildInfoClass);

        int ret = CASCAM_Set_Property(&data_set);
        if(-2 == ret)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.mode = POP_MODE_BLOCK;
            pop.type = POP_TYPE_OK;
            pop.str = NSTV_OSD_FEED_NO_TIME;;
            pop.exit_cb = NULL;
            popdlg_create(&pop);
            return 0;
        }
        else if(0 == ret)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.mode = POP_MODE_BLOCK;
            pop.type = POP_TYPE_OK;
            pop.str = NSTV_OSD_FEED_SUCCESS;
            pop.exit_cb = NULL;
            popdlg_create(&pop);
        }
        else
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.mode = POP_MODE_BLOCK;
            pop.type = POP_TYPE_OK;
            pop.str = NSTV_OSD_FEED_FAIL;;
            pop.exit_cb = NULL;
            popdlg_create(&pop);
            return 0;
        }
    }

    return 0;
}

static void _feed_proccess(void)
{
    // step 1
    // read parent card data
    CASCAMNSTVParentChildInfoClass data_info;
    memset(&data_info, 0, sizeof(CASCAMNSTVParentChildInfoClass));
    data_info.wTvsID = g_operator_id;

    CASCAMDataSetClass data_set;
    memset(&data_set, 0, sizeof(CASCAMDataSetClass));
    
    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = PARENT_CHILD_CONTROL;
    data_set.sub_id = NSTV_CARD_PARENT_CHILD_DATA_GET;
    data_set.data_buf = (void*)&data_info;
    data_set.buf_len = sizeof(CASCAMNSTVParentChildInfoClass);

    if(0 != CASCAM_Set_Property(&data_set))
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.mode = POP_MODE_BLOCK;
        pop.type = POP_TYPE_OK;
        pop.str = NSTV_OSD_BAD_PARENT;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return;
    }

    // step 2
    // show "please insert child card"
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.mode = POP_MODE_BLOCK;
    pop.type = POP_TYPE_OK;
    pop.str = NSTV_OSD_INSERT_CHILD;
    pop.exit_cb = _child_save;
    popdlg_create(&pop);
}

static int _parent_save(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        _feed_proccess();
    }

    return 0;
}

static void _card_feed(void)
{
    CASCAMNSTVParentChildInfoClass data_info;
    memset(&data_info, 0, sizeof(CASCAMNSTVParentChildInfoClass));
    data_info.wTvsID = g_operator_id;
    CASCAMDataGetClass data_set;
    memset(&data_set, 0, sizeof(CASCAMDataGetClass));
    data_set.cas_flag = CASCAM_NSTV;
    data_set.main_id = PARENT_CHILD_INFO;
    data_set.sub_id = NSTV_CARD_PARENT_CHILD_INFO_GET;
    data_set.data_buf = (void*)&data_info;
    data_set.buf_len = sizeof(CASCAMNSTVParentChildInfoClass);

    if(0 != CASCAM_Get_Property(&data_set))
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.mode = POP_MODE_BLOCK;
        pop.type = POP_TYPE_OK;
        pop.str = NSTV_OSD_FEED_NO_CARD;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return;
    }

    unsigned char pbyIsChild = data_info.pbyChild; // 0: parent 1: child
    unsigned char pbIsCanFeed = data_info.pbIsCanFeed; // 0: can not feed 1: can feed

    if(0 == pbyIsChild) // parent card first
    {
        _feed_proccess();
    }
    else if(1 == pbyIsChild) // child card first
    {
        if(0 == pbIsCanFeed) // this time no need feed
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.mode = POP_MODE_BLOCK;
            pop.type = POP_TYPE_OK;
            pop.str = NSTV_OSD_FEED_NO_TIME;
            pop.exit_cb = NULL;
            popdlg_create(&pop);
            return;
        }
        else // feed child card
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.mode = POP_MODE_BLOCK;
            pop.type = POP_TYPE_OK;
            pop.str = NSTV_OSD_INSERT_PARENT;
            pop.exit_cb = _parent_save;
            popdlg_create(&pop);
        }
    }
    else
    {
        printf("_card_feed error \n");
    }
}

// UI
SIGNAL_HANDLER int app_nstv_card_parent_child_keypress(GuiWidget *widget, void *usrdata)
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
                        GUI_EndDialog(WND_NSTV_CARD_PARENT_CHILD);
                    }
                    break;
                case STBK_RED:
                    _card_feed();
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
SIGNAL_HANDLER int app_nstv_card_parent_child_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
#endif
