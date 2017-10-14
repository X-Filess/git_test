/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_update_info.c
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
#define WND_NSTV_UPDATE_INFO            "wnd_nstv_update_info"
#define PATH_XML                        "ca/nstv/wnd_nstv_update_info.xml"
#define TXT_TITLE                       "txt_nstv_update_info_title"
#define TXT_CARD_UPDATE_TIME_L          "txt_nstv_update_info_item1_l"
#define TXT_CARD_UPDATE_TIME_R          "txt_nstv_update_info_item1_r"
#define TXT_CARD_UPDATE_STATUS_L        "txt_nstv_update_info_item2_l"
#define TXT_CARD_UPDATE_STATUS_R        "txt_nstv_update_info_item2_r"

// process
#define WND_NSTV_UPDATE_NOTIFY          "wnd_nstv_update_process"
#define NOTIFY_PATH_XML                 "ca/nstv/wnd_nstv_update_process.xml"
#define TXT_CARD_UPDATE_STATUS          "txt_nstv_update_process_info2"
#define PRB_CARD_UPDATE_PROCESS         "prb_nstv_update_process"
#define TXT_CARD_UPDATE_PROCESS         "text_nstv_update_process_value"
// log file
#define UPDATE_LOG_FILE                 "/home/gx/card_update.log"
#define UPDATE_INFO_MAGIC               0x12345678

enum{
    CARD_UPDATE_NONE = 0,
    CARD_UPDATE_UPDATING,
    CARD_UPDATE_SUCCESS,
    CARD_UPDATE_FAILED,
};

static CASCAMNSTVCardUpdateInfoClass thiz_update_info = {0};
static int _update_history_init(CASCAMNSTVCardUpdateInfoClass *info)
{
    CASCAMNSTVCardUpdateInfoClass temp = {0};
    temp.magic = UPDATE_INFO_MAGIC;
    int size = 0;
    int real_size = 0;
    FILE *pf = NULL;
    if(access(UPDATE_LOG_FILE, F_OK) != 0)
    {// not exist
        pf = fopen(UPDATE_LOG_FILE, "w");//只写模式创建文件
        if(NULL == pf)
        {
            printf("\nCAS CARD UPDATE, ERROR, %s, %d\n", __FUNCTION__, __LINE__);
            return -1;
        }

        size = sizeof(CASCAMNSTVCardUpdateInfoClass);
        if(NULL != info)
        {
           memcpy(&temp, info, size);
        }
        fseek(pf, 0, SEEK_SET);
        real_size = fwrite((unsigned char *)&temp, size, 1, pf);
        if(real_size != 1)
        {
            printf("\nCAS CARD UPDATE, ERROR, %s, %d\n", __FUNCTION__, __LINE__);
            goto err;
        }
    }
    else
    { 
        if(NULL != info)
        {
            pf = fopen(UPDATE_LOG_FILE,"w");//只写模式打开
            if(NULL == pf)
            {
                printf("\nCAS CARD UPDATE, ERROR, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }

            size = sizeof(CASCAMNSTVCardUpdateInfoClass);
            fseek(pf, 0, SEEK_SET);
            real_size = fwrite((unsigned char *)info, size, 1, pf);
            if(real_size != 1)
            {
                printf("\nCAS CARD UPDATE, ERROR, %s, %d\n", __FUNCTION__, __LINE__);
                goto err;
            }
        }
        else
        { 
            pf = fopen(UPDATE_LOG_FILE,"r");//只读模式打开
            if(NULL == pf)
            {
                printf("\nCAS CARD UPDATE, ERROR, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            size = sizeof(CASCAMNSTVCardUpdateInfoClass);
            fseek(pf, 0, SEEK_SET);
            real_size = fread((unsigned char*)&temp, size, 1, pf);
            if((real_size != 1) || (temp.magic != UPDATE_INFO_MAGIC))
            {
                printf("\nCAS CARD UPDATE, ERROR, %s, %d\n", __FUNCTION__, __LINE__);
                //goto err;
                temp.update_status = 3;// failed
            }
        }
    }
#if 0
    // init
    if(temp.update_status > 0)
    {
        //ex. 0: no update information; 1: updating; 2: update success; 3: update failed
        switch(temp.update_status)
        {
            case 1:
                GUI_SetProperty(TXT_CARD_UPDATE_STATUS_R, "string", NSTV_OSD_PATCHING);
                break;
            case 2:
                GUI_SetProperty(TXT_CARD_UPDATE_STATUS_R, "string", NSTV_OSD_UPDATE_SUCCESS);
                break;
            case 3:
                GUI_SetProperty(TXT_CARD_UPDATE_STATUS_R, "string", NSTV_OSD_UPDATE_FAIL);
                break;
            default:
                break;
        }
    }
    if(strlen((const char*)temp.update_time))
    {
        GUI_SetProperty(TXT_CARD_UPDATE_TIME_R, "string", temp.update_time);
    }
#endif
    memcpy(&thiz_update_info, &temp, sizeof(temp));
    fclose(pf);
    return 0;
err:
    memcpy(&thiz_update_info, &temp, sizeof(temp));
    if(NULL != pf)
    {
        fclose(pf);
    }
    return -1;
}

void app_nstv_update_info_exec(void)
{
    static char init_flag = 0;

    _update_history_init(NULL);
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_NSTV_UPDATE_INFO, PATH_XML);
    }

    app_create_dialog(WND_NSTV_UPDATE_INFO);
    // title
    GUI_SetProperty(TXT_TITLE, "string", NSTV_CARD_UPDATE);
    // init
    if(thiz_update_info.update_status > 0)
    {
        //ex. 0: no update information; 1: updating; 2: update success; 3: update failed
        switch(thiz_update_info.update_status)
        {
            case 1:
                GUI_SetProperty(TXT_CARD_UPDATE_STATUS_R, "string", NSTV_OSD_PATCHING);
                break;
            case 2:
                GUI_SetProperty(TXT_CARD_UPDATE_STATUS_R, "string", NSTV_OSD_UPDATE_SUCCESS);
                break;
            case 3:
                GUI_SetProperty(TXT_CARD_UPDATE_STATUS_R, "string", NSTV_OSD_UPDATE_FAIL);
                break;
            default:
                GUI_SetProperty(TXT_CARD_UPDATE_STATUS_R, "string", NSTV_OSD_UPDATE_FAIL);
                break;
        }
    }
    if(strlen((const char*)thiz_update_info.update_time))
    {
        GUI_SetProperty(TXT_CARD_UPDATE_TIME_R, "string", thiz_update_info.update_time);
    }
}

// for process
static event_list*  thiz_update_timer= NULL;
static void _update_history(unsigned char status)
{
extern int app_time_get_local_time(unsigned short *year, unsigned char *month, unsigned char *day,
                                    unsigned char *hour, unsigned char *minute, unsigned char *second);
    CASCAMNSTVCardUpdateInfoClass info = {0};
    unsigned short year = 0;
    unsigned char month = 0;
    unsigned char day = 0;
    unsigned char hour = 0;
    unsigned char min = 0;
    unsigned char sec = 0;
    //status
    info.update_status = status;
    app_time_get_local_time(&year, &month, &day, &hour, &min, &sec);
    sprintf((char*)info.update_time,"%4d-%02d-%02d %02d:%02d",year,month,day,hour,min);
    info.magic = UPDATE_INFO_MAGIC;
    _update_history_init(&info);
}

static int _exit_update_process(void *userdata)
{
    if(thiz_update_timer)
    {
        remove_timer(thiz_update_timer);
        thiz_update_timer = NULL;
    }
    _update_history(CARD_UPDATE_SUCCESS);
    GUI_EndDialog(WND_NSTV_UPDATE_NOTIFY);
    return 0;
}


void app_nstv_card_update_notify(char *param)
{
    static char init_flag = 0;
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_NSTV_UPDATE_NOTIFY, NOTIFY_PATH_XML);
    }
    if(GUI_CheckDialog(WND_NSTV_UPDATE_NOTIFY) != GXCORE_SUCCESS)
    {
        _update_history(CARD_UPDATE_FAILED);
        app_create_dialog(WND_NSTV_UPDATE_NOTIFY);
    }
    // init param
    CASCAMNSTVCardUpdateClass *p = NULL;
    if(NULL != param)
    {
        char buf[10] = {0};
        p = (CASCAMNSTVCardUpdateClass*)param;
        switch(p->mark)
        {
            case 1://receive data
                GUI_SetProperty(TXT_CARD_UPDATE_STATUS, "string", NSTV_OSD_RECEIVE_DATA);
                break;
            case 2:// updating
                GUI_SetProperty(TXT_CARD_UPDATE_STATUS, "string", NSTV_OSD_PATCHING);
                break;
            default:
                printf("\nCAS, update process...\n");
                break;
        }
#if 0
        if((p->mark == 2) && (p->process > 100))
        {
#define MAX_MARK_VALUE 2
            int value = ((p->process - 100)*100/MAX_MARK_VALUE);
            GUI_SetProperty(PRB_CARD_UPDATE_PROCESS, "value", &value);
            sprintf(buf, "%d%%", value);
            GUI_SetProperty(TXT_CARD_UPDATE_PROCESS, "string", buf);
            if(value == 100)
            {
                GUI_SetProperty(TXT_CARD_UPDATE_STATUS, "string", NSTV_OSD_UPDATE_SUCCESS);
                // end dialog
                // create a timer
                if (thiz_update_timer == NULL)
                {
                    thiz_update_timer = create_timer(_exit_update_process, 500, NULL, TIMER_REPEAT);
                }
            }
        }
        else
#endif
        {
            int value = p->process;
            if(value > 100)
            {
                value = 100;
            }
            GUI_SetProperty(PRB_CARD_UPDATE_PROCESS, "value", &value);
            sprintf(buf, "%d%%", value);
            GUI_SetProperty(TXT_CARD_UPDATE_PROCESS, "string", buf);
            if((p->mark == 1) && (p->process >100))
            {
                if(thiz_update_timer != NULL)
                {
                    remove_timer(thiz_update_timer);
                    thiz_update_timer = NULL;
                }

                GUI_EndDialog(WND_NSTV_UPDATE_NOTIFY);
            }
            else if((p->mark == 2) && (p->process >= 100))
            {
                GUI_SetProperty(TXT_CARD_UPDATE_STATUS, "string", NSTV_OSD_UPDATE_SUCCESS);
                // end dialog
                // create a timer
                if (thiz_update_timer == NULL)
                {
                    thiz_update_timer = create_timer(_exit_update_process, 500, NULL, TIMER_REPEAT);
                }

            }
        }
    }
    else
    {
        GUI_EndDialog(WND_NSTV_UPDATE_NOTIFY);
        printf("\nERROR, %s, %d\n", __func__, __LINE__);
    }
}

// UI
SIGNAL_HANDLER int app_nstv_update_info_keypress(GuiWidget *widget, void *usrdata)
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
                        GUI_EndDialog(WND_NSTV_UPDATE_INFO);
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
SIGNAL_HANDLER int app_nstv_update_info_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
#endif
