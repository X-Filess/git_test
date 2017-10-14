/*****************************************************************************
* 			  CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009-2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_full_screen.c
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.3.8	           shenbin         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "gui_core.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "app_default_params.h"
#include "full_screen.h"
#if TKGS_SUPPORT
#include "module/gxtkgs.h"
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
#endif
extern int app_pvr_stop_cb(PopDlgRet ret);


#define TEXT_NUMBER	"txt_number"
#define PROG_NUM_MAX  (4)

#if CMM_SUPPORT
#define CMB_MENU_KEY         (7788)
#define CMP_MENU_KEY         (7799)
#endif

#define TUNER_CONFIG_KEY       (9998)
#define KEY_SCANCODE_KEY       (7789)
#define MERGE_DB_KEY           (7790)

#define TP_RECORD_KEY           (7791)

#define FACTORY_TEST_MENU_KEY  (9999)
#define LOG_SET_MENU_KEY       (8765)
#define LOG_SET_SUPPORT (1)


/* static variable ------------------------------------------------------ */
static event_list * sp_TimerNumber = NULL;
static char key_buf[PROG_NUM_MAX + 1];
static uint32_t key_num = 0;

/* Private functions ------------------------------------------------------ */

void number_set(unsigned short key_value)
{
	switch (key_value)
	{
		case STBK_1:
			strcat(key_buf, "1");
			break;
		case STBK_2:
			strcat(key_buf, "2");
			break;
		case STBK_3:
			strcat(key_buf, "3");
			break;
		case STBK_4:
			strcat(key_buf, "4");
			break;
		case STBK_5:
			strcat(key_buf, "5");
			break;
		case STBK_6:
			strcat(key_buf, "6");
			break;
		case STBK_7:
			strcat(key_buf, "7");
			break;
		case STBK_8:
			strcat(key_buf, "8");
			break;
		case STBK_9:
			strcat(key_buf, "9");
			break;
		case STBK_0:
			strcat(key_buf, "0");
			break;
		default:
			break;
	}
}

//extern void _channel_info_show(void);
#if TKGS_SUPPORT
int app_t2_lcn_num_switch(int32_t num)
{
	GxMsgProperty_NodeByPosGet node_prog = {0};
	int i = 0;
	PopDlg pop = {0};
	bool find_logicnum = false;
	#if TKGS_SUPPORT
	AppProgExtInfo prog_ext_info = {{0}};
	#endif
	
	for(i=0;i<g_AppPlayOps.normal_play.play_total;i++)
	{
		node_prog.node_type = NODE_PROG;
		node_prog.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
		#if TKGS_SUPPORT
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node_prog.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
		{
			if(num == prog_ext_info.tkgs.logicnum)
			{
				find_logicnum = true;
				return i;
			}
		}
		#else
		if(num == node_prog.prog_data.logicnum)
		{
			find_logicnum = true;
			return i;
		}
		#endif
	}
	
	if(!find_logicnum)
	{
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.mode = POP_MODE_BLOCK;
		pop.str = STR_ID_NO_PROGRAM;
		pop.timeout_sec = 3;
		popdlg_create(&pop);
		return GXCORE_ERROR;
	}

    return 0;
}
#endif

static bool number_special_key_check(int num_value)
{
	if(0
#if CMM_SUPPORT
            ||(num_value == CMB_MENU_KEY)
            ||(num_value == CMP_MENU_KEY)
#endif
#if FACTORY_TEST_SUPPORT	
                ||(num_value == FACTORY_TEST_MENU_KEY) 
#endif	
#if LOG_SET_SUPPORT	
                || (num_value == LOG_SET_MENU_KEY)
#endif	
#if KEY_SCANCODE_SUPPORT	
                || (num_value == KEY_SCANCODE_KEY) 
#endif	
#if MERGE_DB_SUPPORT	
                || (num_value == MERGE_DB_KEY) 
#endif	
#if 1	
                || (num_value == TP_RECORD_KEY)
#endif
				|| (num_value == TUNER_CONFIG_KEY)
			)
		return true;
	else
		return false;
}

static int number_timeout(void* data)
{
    int32_t num_value;
    PopDlg pop = {0};
    pvr_state cur_pvr = PVR_DUMMY;
    GxMsgProperty_NodeByPosGet node_prog_tag = {0};
    GxMsgProperty_NodeByPosGet node_prog_tmp = {0};
    int tag_pos = 0;
    int i;

    // get number
    num_value = atoi(key_buf);

    GUI_EndDialog("wnd_number");
    GUI_SetProperty("wnd_number", "draw_now", NULL);

#if TKGS_SUPPORT
    if(num_value == 0)
        return GXCORE_ERROR;
	
	 if(!number_special_key_check(num_value))
{
    if(GX_TKGS_OFF != app_tkgs_get_operatemode())
    {
        tag_pos = app_t2_lcn_num_switch(num_value);
        if (tag_pos == GXCORE_ERROR)
        {
            return GXCORE_ERROR;
        }
    }
    else
    {
         if (num_value > g_AppPlayOps.normal_play.play_total)
    {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.mode = POP_MODE_BLOCK;
            pop.str = STR_ID_NO_PROGRAM;
            pop.timeout_sec = 3;

            popdlg_create(&pop);

            return GXCORE_ERROR;
    }
       tag_pos = num_value - 1;
    }
}
#else
    if (num_value > g_AppPlayOps.normal_play.play_total || num_value == 0)
    {
         if(number_special_key_check(num_value))
        {
            ;
        }
        else
        {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.mode = POP_MODE_BLOCK;
            pop.str = STR_ID_NO_PROGRAM;
            pop.timeout_sec = 3;

            popdlg_create(&pop);

            return GXCORE_ERROR;
        }
    }
	else
	{
        tag_pos = num_value - 1;
    }

#endif

        if(number_special_key_check(num_value))
        {
            ;
        }
        else
        {
           // tag_pos = num_value - 1;
            node_prog_tag.node_type = NODE_PROG;
            node_prog_tag.pos = tag_pos;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tag);
        }

    if (g_AppPvrOps.state != PVR_DUMMY)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;

        if(g_AppPvrOps.state == PVR_TP_RECORD)
        {
            pop.str = STR_ID_STOP_REC_INFO;
            cur_pvr = PVR_TP_RECORD;
        }
        else if(g_AppPvrOps.state == PVR_TMS_AND_REC)
        {
#if CA_SUPPORT
            if(app_extend_control_pvr_change())
            {
                pop.str = STR_ID_STOP_REC_INFO;
                cur_pvr = PVR_RECORD;
            }
            else
#endif
            {
                pop.str = STR_ID_STOP_TMS_INFO;
                cur_pvr = PVR_TIMESHIFT;
            }
        }
        else if(g_AppPvrOps.state == PVR_RECORD)
        {
#if CA_SUPPORT
            if(app_extend_control_pvr_change())
            {
                pop.str = STR_ID_STOP_REC_INFO;
                cur_pvr = PVR_RECORD;
            }
#endif
        }
        else
        {
#if CMM_SUPPORT
            if((num_value != CMB_MENU_KEY)
                    || (num_value != CMP_MENU_KEY))
            {
                pop.str = STR_ID_STOP_TMS_INFO;
                cur_pvr = PVR_TIMESHIFT;
            }
#endif
        }

        if(cur_pvr != PVR_DUMMY)
        {
            if (POP_VAL_OK != popdlg_create(&pop))
            {
                return GXCORE_ERROR;
            }
            if(cur_pvr == PVR_TIMESHIFT)
            {
                app_pvr_tms_stop();
            }
            else
            {
                app_pvr_stop();
            }
        }
    }

    switch(num_value)
    {
#if CMM_SUPPORT
        case CMB_MENU_KEY:
            {
                //extern void app_cmb_create_exec(void);
                //g_AppFullArb.timer_stop();
                //app_cmb_create_exec();
                break;
            }

        case CMP_MENU_KEY:
            {
                //extern void app_cmp_create_exec(void);
                //g_AppFullArb.timer_stop();
                //app_cmp_create_exec();
                break;
            }
#endif
#if FACTORY_TEST_SUPPORT
        case FACTORY_TEST_MENU_KEY:
            {
                g_AppFullArb.timer_stop();
                g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                g_AppPlayOps.program_stop();
                extern void app_factory_test_menu_exec(void);
                app_factory_test_menu_exec();
                break;
            }
#endif

#if LOG_SET_SUPPORT
        case LOG_SET_MENU_KEY:
            {
                g_AppFullArb.timer_stop();
                g_AppPlayOps.program_stop();//为了防止图片显示过慢(沈从则给出解释，帧存不够)by gcyin
                extern void app_log_set_menu_exec(void);
                app_log_set_menu_exec();
                break;
            }
#endif

#if KEY_SCANCODE_SUPPORT
        case KEY_SCANCODE_KEY:
            {
                g_AppFullArb.timer_stop();
                app_create_dialog("wnd_key_scancode");
                break;
            }
#endif

#if MERGE_DB_SUPPORT
        case MERGE_DB_KEY:
            {
                g_AppFullArb.timer_stop();
                app_create_dialog("wnd_merge_db");
                break;
            }
#endif

#if PVR_RECORD_TP_SUPPORT
        case TP_RECORD_KEY:
            {
                usb_check_state usb_state = USB_OK;

                if (g_AppPlayOps.normal_play.play_total == 0)
                    break;

                if(g_AppPvrOps.state == PVR_DUMMY)
                {
                    PopDlg pop;
                    // check usb state
                    usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
                    if(usb_state == USB_ERROR)
                    {
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.format = POP_FORMAT_DLG;
                        pop.str = STR_ID_INSERT_USB;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.creat_cb = NULL;
                        pop.exit_cb = NULL;
                        pop.timeout_sec= 3;
                        popdlg_create(&pop);

                        break;
                    }

                    if(usb_state == USB_NO_SPACE)
                    {
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.format = POP_FORMAT_DLG;
                        pop.str = STR_ID_NO_SPACE;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.creat_cb = NULL;
                        pop.exit_cb = NULL;
                        pop.timeout_sec= 3;
                        popdlg_create(&pop);

                        break;
                    }

                    app_tp_rec_start(0);
                    app_create_dialog("wnd_pvr_bar");
                    GUI_SetProperty("wnd_pvr_bar", "draw_now", NULL);
                }
                else
                {
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_NO_BTN;
                    pop.format = POP_FORMAT_DLG;
                    pop.str = STR_ID_STOP_PVR_FIRST;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.timeout_sec = 3;
                    popdlg_create(&pop);
                }

                break;
            }
#endif
		case TUNER_CONFIG_KEY:
			GUI_CreateDialog("wnd_tuner_config");
			break;

        default:
            {
                if(cur_pvr == PVR_RECORD)
                {
                    for(i = 0; i < g_AppPlayOps.normal_play.play_total; i++)
                    {
                        node_prog_tmp.node_type = NODE_PROG;
                        node_prog_tmp.pos = i;
                        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tmp);
                        if(node_prog_tmp.prog_data.id == node_prog_tag.prog_data.id)
                        {
                            tag_pos = i;
                            break;
                        }
                    }
                }
                // player prog by pos, pos from 0 and num from 1, so need del 1
                g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, tag_pos);
                g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);

                /*if(GUI_CheckDialog("wnd_passwd") != GXCORE_SUCCESS)
                  {
                  if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
                  _channel_info_show();
                  else
                  app_create_dialog("wnd_channel_info");
                  }*/
                break;
            }
    }
    return GXCORE_SUCCESS;
}

/* Exported functions ----------------------------------------------------- */
SIGNAL_HANDLER int app_number_create(const char* widgetname, void *usrdata)
{
	if (reset_timer(sp_TimerNumber) != 0)
	{
		sp_TimerNumber = create_timer(number_timeout, 1500, NULL, TIMER_ONCE);
	}

	key_num = 0;
	memset(key_buf, 0, PROG_NUM_MAX + 1);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_number_destroy(const char* widgetname, void *usrdata)
{
	remove_timer(sp_TimerNumber);
	sp_TimerNumber = NULL;

	key_num = 0;
	memset(key_buf, 0, PROG_NUM_MAX + 1);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_number_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_1:
			case STBK_2:
			case STBK_3:
			case STBK_4:
			case STBK_5:
			case STBK_6:
			case STBK_7:
			case STBK_8:
			case STBK_9:
			case STBK_0:
				reset_timer(sp_TimerNumber);
				if (key_num >= PROG_NUM_MAX)
				{
					key_num = 0;
					memset(key_buf, 0, PROG_NUM_MAX + 1);
				}

				key_num++;
				number_set(event->key.sym);
				GUI_SetProperty(TEXT_NUMBER, "string", key_buf);
				break;

			case STBK_OK:
				number_timeout(NULL);
				break;
			case STBK_EXIT:
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("wnd_number");
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

/* End of file -------------------------------------------------------------*/


