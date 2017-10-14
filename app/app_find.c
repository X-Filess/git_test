#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "full_screen.h"
#if TKGS_SUPPORT
#include "module/gxtkgs.h"
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
#endif

#define TXT_TITLE   "txt_find_title"
#define FIND_LISTVIEW     "listview_find"

/* Private variable------------------------------------------------------- */
/* widget macro -------------------------------------------------------- */

static GxBusPmViewInfo old_info = {0};
static uint8_t sFindNum = 0;
uint32_t ExitFlag = 0;
static void  prog_list_find_program(char *strFind)
{
	GxBusPmViewInfo view_info = {0};

	if(NULL == strFind)
		return;

	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info)))
	{
		int i = 0;
		int len = strlen(strFind);

		view_info.taxis_mode = TAXIS_MODE_LETTER;

		if(MAX_CMP_NAME < len)
		{
			len  = MAX_CMP_NAME;
		}

		if(0 == len)
		{
			view_info = old_info;
		}
		else
		{
			memset(view_info.letter,0,MAX_CMP_NAME*sizeof(uint8_t));
			for(i = 0; i<len; i++)
			{
				view_info.letter[i] = strFind[i];
			}
		}

        app_send_msg_exec(GXMSG_PM_VIEWINFO_SET,  (void *)(&view_info));
	}
}

void app_find_old_info_change(void)
{
    old_info.pip_switch = 0;
    old_info.pip_main_tp_id = 0;
}

static void find_keyboard_release_cb(PopKeyboard *data)
{
    if((data->in_ret == POP_VAL_CANCEL) || (data->out_name == NULL) || (sFindNum == 0))
    {
        g_AppPlayOps.play_list_create(&old_info);
        GUI_EndDialog("wnd_find");
    }
}

static void find_keyboard_change_cb(PopKeyboard *data)
{
    GxMsgProperty_NodeNumGet node = {0};
	int cur_sel = 0;

	prog_list_find_program(data->out_name);

	node.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node);

	sFindNum = node.node_num;

	if(0 < node.node_num)
	{
	    GUI_SetProperty(FIND_LISTVIEW, "update_all", NULL);
		cur_sel = 0;
	    GUI_SetProperty(FIND_LISTVIEW, "select", (void*)(&cur_sel));
	}
	else
	{
	    GUI_SetProperty(FIND_LISTVIEW, "update_all", NULL);
		GUI_SetProperty(FIND_LISTVIEW, "select", (void*)(&cur_sel));
	}
}

#define FIND_LIST
SIGNAL_HANDLER int app_find_create(GuiWidget *widget, void *usrdata)
{
#if(MINI_16_BITS_OSD_SUPPORT == 1)
#define POS_X 400 
#define POS_Y 80
#else
#define POS_X 500 
#define POS_Y 100
#endif
	static PopKeyboard keyboard;
	ExitFlag = 0;

	memset(&keyboard, 0, sizeof(PopKeyboard));
	keyboard.in_name    = NULL;
	keyboard.max_num = MAX_PROG_NAME - 1;
    keyboard.out_name   = NULL;
    keyboard.change_cb  = find_keyboard_change_cb;
    keyboard.release_cb = find_keyboard_release_cb;
    keyboard.usr_data   = NULL;
	keyboard.pos.x = POS_X;
	keyboard.pos.y = POS_Y;

	multi_language_keyboard_create(&keyboard);


	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&old_info));

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_find_destroy(GuiWidget *widget, void *usrdata)
{
	ExitFlag = 0;
    app_create_dialog("wnd_channel_info");
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_find_keypress(GuiWidget *widget, void *usrdata)
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
			switch(event->key.sym)
			{
				case STBK_DOWN:
				case STBK_UP:
				case STBK_PAGE_UP:
				case STBK_PAGE_DOWN:
					{
						ret = EVENT_TRANSFER_STOP;
					}
					break;

				case STBK_EXIT:
				case STBK_MENU:
					{
						g_AppPlayOps.play_list_create(&old_info);
                        GUI_EndDialog("wnd_find");
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

SIGNAL_HANDLER int app_find_got_focus(GuiWidget *widget, void *usrdata)
{

	return EVENT_TRANSFER_STOP;
}

void app_end_find_extern(void)
{
	if(ExitFlag)
	{
		g_AppPlayOps.play_list_create(&old_info);
        if (GXCORE_SUCCESS == GUI_CheckDialog("wnd_find"))
		{
			GUI_EndDialog("wnd_find");
            app_create_dialog("wnd_channel_info");
		}
	}
}

#define FIND_LISTVIEW_

SIGNAL_HANDLER int app_find_list_change(GuiWidget *widget, void *usrdata)
{	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_find_list_get_total(GuiWidget *widget, void *usrdata)
{
    uint32_t total = 0;
	GxMsgProperty_NodeNumGet node_num_get = {0};

	node_num_get.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));

	total = node_num_get.node_num;

    return total;
}

SIGNAL_HANDLER int app_find_list_get_data(GuiWidget *widget, void *usrdata)
{
    #define CHANNEL_NUM_LEN	    (10)
	#if TKGS_SUPPORT
	AppProgExtInfo prog_ext_info = {{0}};
	#endif
	ListItemPara* item = NULL;
	static GxMsgProperty_NodeByPosGet node;
	static char buffer[CHANNEL_NUM_LEN];

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	node.node_type = NODE_PROG;
	node.pos = item->sel;
	//get node
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

	//col-0: num
	item->x_offset = 2;
	item->image = NULL;
	memset(buffer, 0, CHANNEL_NUM_LEN);
	#if TKGS_SUPPORT
	if(GX_TKGS_OFF != app_tkgs_get_operatemode())
	{
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
		{
			sprintf(buffer, "%d", prog_ext_info.tkgs.logicnum);
		}
		else
		{
			sprintf(buffer, "%d", (item->sel+1));
		}		
	}
	else
	{
		sprintf(buffer, "%d", (item->sel+1));
	}
	#else
	sprintf(buffer, "%03d", (item->sel+1));
	#endif
	item->string = buffer;

	//col-1: channel name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;

	if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
		item->string = "$";
	}
	else
	{
		item->string = NULL;
	}
	
	//col-2: channel name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	item->string = (char*)node.prog_data.prog_name;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_find_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint32_t sel=0;

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
				case STBK_DOWN:
				case STBK_UP:
				case STBK_PAGE_UP:
				case STBK_PAGE_DOWN:
					{
						ret = EVENT_TRANSFER_KEEPON;
					}
					break;
					
				case STBK_LEFT:
				case STBK_RIGHT:
					
					break;

				case STBK_EXIT:
				case STBK_MENU:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				case STBK_OK:
                {
                    GUI_GetProperty(FIND_LISTVIEW, "select", &sel);
                    if (g_AppPvrOps.state != PVR_DUMMY)
                    {
                        PopDlg pop;
                        pvr_state cur_pvr = PVR_DUMMY;
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
                            pop.str = STR_ID_STOP_TMS_INFO;
                            cur_pvr = PVR_TIMESHIFT;
                        }

                        if(cur_pvr != PVR_DUMMY)
                        {
                            if (POP_VAL_OK != popdlg_create(&pop))
                            {
                                break;
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

                    GxMsgProperty_NodeByPosGet node_pos = {0};
                    int service_id, frequency, symbol_rate, i;

                    node_pos.node_type = NODE_PROG;
                    node_pos.pos = sel;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node_pos));

                    if(node_pos.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
                    {
                        GxMsgProperty_NodeByIdGet node_tp = {0};
                        node_tp.node_type = NODE_TP;
                        node_tp.id = node_pos.prog_data.tp_id;
                        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

                        frequency = node_tp.tp_data.frequency;
                        symbol_rate = node_tp.tp_data.tp_s.symbol_rate;

                        service_id = node_pos.prog_data.service_id;

                        g_AppPlayOps.play_list_create(&old_info);

                        for(i=0; i<g_AppPlayOps.normal_play.play_total; i++)
                        {
                            node_pos.node_type = NODE_PROG;
                            node_pos.pos = i;
                            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node_pos));

                            node_tp.node_type = NODE_TP;
                            node_tp.id = node_pos.prog_data.tp_id;
                            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

                            if(node_pos.prog_data.service_id == service_id && node_tp.tp_data.frequency == frequency 
                                    && node_tp.tp_data.tp_s.symbol_rate == symbol_rate)
                            {
                                g_AppPlayOps.program_play(PLAY_MODE_POINT, i);
                                //printf("-----find the program: 0x%04x\n", service_id);
                                break;
                            }
                        }
                    }
                    else
                    {
                        g_AppPlayOps.play_list_create(&old_info);
                        if(app_get_prog_pos_by_id(&g_AppPlayOps.normal_play.view_info, node_pos.prog_data.id, &g_AppPlayOps.normal_play.play_count) == GXCORE_ERROR)
                        {
                            g_AppPlayOps.normal_play.play_count = 0;
                        }
                        g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
                    }

                    g_AppFullArb.state.pause = STATE_OFF;
                    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                    GUI_EndDialog("wnd_find");
                }
				break;

                case STBK_SAT:	
                case STBK_FAV:
#if CA_SUPPORT
					if(app_extend_control_pvr_change())
					{
						//can not change the program
						PopDlg pop;
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_NO_BTN;
						pop.format = POP_FORMAT_DLG;
						pop.str = STR_ID_STOP_PVR_FIRST;
						pop.mode = POP_MODE_UNBLOCK;
						pop.creat_cb = NULL;
						pop.exit_cb = NULL;
						pop.timeout_sec= 3;
						popdlg_create(&pop);
						break;
					}
#endif
                    ret = EVENT_TRANSFER_KEEPON; 
                    break;
                //case STBK_PAGE_UP:
                //    {
                //        uint32_t sel;
                //        GUI_GetProperty(FIND_LISTVIEW, "select", &sel);
                //        if (sel == 0)
                //        {
                //            sel = app_find_list_get_total(NULL,NULL)-1;
                //            GUI_SetProperty(FIND_LISTVIEW, "select", &sel);
                //            ret = EVENT_TRANSFER_STOP; 
                //        }
                //        else
                //        {
                //            ret = EVENT_TRANSFER_KEEPON; 
                //        }
                //    }
                // case STBK_PAGE_DOWN:
                //    {
                //        uint32_t sel;
                //        GUI_GetProperty(FIND_LISTVIEW, "select", &sel);
                //        if (app_channel_list_list_get_total(NULL,NULL)
                //                == sel+1)
                //        {
                //            sel = 0;
                //            GUI_SetProperty(FIND_LISTVIEW, "select", &sel);
                //            ret = EVENT_TRANSFER_STOP; 
                //        }
                //        else
                //        {
                //            ret = EVENT_TRANSFER_KEEPON; 
                //        }
                //    }
                //    break;
				default:
					break;
			}
		default:
			break;
	}

	return ret;
}




