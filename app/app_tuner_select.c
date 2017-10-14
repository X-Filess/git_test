/*
 * =====================================================================================
 *
 *       Filename:  app_tuner_select.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2011年12月09日 09时34分49秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app.h"
#if (DEMOD_DVB_S > 0)

#include "app_send_msg.h"
#include "app_module.h"
#include "app_pop.h"

#define BTN_T1  "btn_tuner_select1"
#define BTN_T2  "btn_tuner_select2"
#define WND_T1  "wnd_antenna_setting"
#define WND_T2  "wnd_antenna_setting"

static uint32_t s_CurTuner = 0;
static int8_t sg_Buffer[256] = {0};
uint32_t app_tuner_cur_tuner_get(void)
{
    return s_CurTuner;
}

void app_tuner_cur_tuner_set(uint32_t CurTuner)
{
    s_CurTuner = CurTuner;
}

uint32_t app_tuner_sat_num_get(uint32_t tuner)
{
    GxMsgProperty_NodeNumGet node_num = {0};
    GxMsgProperty_NodeByPosGet node_sat = {0};
    uint32_t sat_count = 0;
    uint32_t i;

    node_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);

    // both tuner have none sat
    if (node_num.node_num == 0)
    {
        return 0;
    }

    // check current sat
    for (i=0; i<node_num.node_num; i++)
    {
        node_sat.node_type = NODE_SAT;
        node_sat.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);

        if (tuner != node_sat.sat_data.tuner)
        {
            continue;
        }

        sat_count++;
    }

    return sat_count;
}

SIGNAL_HANDLER int app_tuner_select_create(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(BTN_T1, "string", "Tuner1");
    GUI_SetProperty(BTN_T2, "string", "Tuner2");
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tuner_select_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tuner_select_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    uint32_t sat_total = 0;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
				case VK_BOOK_TRIGGER:
					GUI_EndDialog("after wnd_full_screen");
					break;
#if DLNA_SUPPORT
                case VK_DLNA_TRIGGER:
                    app_dlna_ui_exec(NULL, NULL);
                    break;
#endif
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog("wnd_tuner_select");
                    break;

                case STBK_OK:
                    if (strcmp(GUI_GetFocusWidget(), BTN_T1) == 0)
                    {
                        sat_total = app_tuner_sat_num_get(0);
                        if(sat_total > 0)
                        {
                            s_CurTuner = 0;
                            app_ioctl(0, FRONTEND_DEMUX_CFG, NULL);
                            //GUI_EndDialog("wnd_tuner_select");
                            GUI_CreateDialog((const char*)sg_Buffer);
                        }
                        else
                        {
                            PopDlg pop;
                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_NO_BTN;
                            pop.format = POP_FORMAT_DLG;
                            pop.str = STR_ID_NO_SAT;
                            pop.title = NULL;
                            pop.mode = POP_MODE_UNBLOCK;
                            pop.creat_cb = NULL;
                            pop.exit_cb = NULL;
                            pop.timeout_sec = 3;
                            popdlg_create(&pop);
                        }
                    }
                    else
                    {
                        sat_total = app_tuner_sat_num_get(1);
                        if(sat_total > 0)
                        {
                            s_CurTuner = 1;
                            app_ioctl(1, FRONTEND_DEMUX_CFG, NULL);
                            GUI_CreateDialog((const char*)sg_Buffer);
                        }
                        else
                        {
                            PopDlg pop;
                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_NO_BTN;
                            pop.format = POP_FORMAT_DLG;
                            pop.str = STR_ID_NO_SAT;
                            pop.title = NULL;
                            pop.mode = POP_MODE_UNBLOCK;
                            pop.creat_cb = NULL;
                            pop.exit_cb = NULL;
                            pop.timeout_sec = 3;
                            popdlg_create(&pop);
                        }

                    }
                    break;

                case STBK_UP:
                case STBK_DOWN:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }

    return ret;
}

int app_tuner_select_create_dlg(char *Widget)
{
	if((NULL == Widget) || (strlen(Widget) >= 256))
		return -1;
	memset(sg_Buffer,0,256);
	memcpy(sg_Buffer,Widget,strlen(Widget));
	GUI_CreateDialog("wnd_tuner_select");
	return 0;
}

#else //DEMOD_DVB_S
SIGNAL_HANDLER int app_tuner_select_create(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tuner_select_destroy(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_tuner_select_keypress(GuiWidget *widget, void *usrdata)
{ return EVENT_TRANSFER_STOP;}
#endif
