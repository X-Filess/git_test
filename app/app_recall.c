#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "full_screen.h"


#define RECALL_LISTVIEW     "listview_recall"
#define WND_RECALL_LIST     "wnd_recall_list"
#define MAX_RECALL_CPUNT	16
/* Private variable------------------------------------------------------- */
/* widget macro -------------------------------------------------------- */

int g_recall_count = 0;


#if RECALL_LIST_SUPPORT

static GxBusPmViewInfo s_old_info = {0};
extern int app_play_get_recall_total_count(void);

void app_recall_list_set_play_num(int recall_num)
{
	int total_recall_num = 0;
	total_recall_num = app_play_get_recall_total_count();
	if(total_recall_num <= 1)
	{
		g_recall_count = 0;
	}
	else
	{
		g_recall_count = recall_num;
	}
	return;
}

SIGNAL_HANDLER int app_recall_create(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_NodeNumGet node = {0};
	int cur_sel = 0;

	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&s_old_info));
	node.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node);

	if(0 < node.node_num)
	{
		GUI_SetProperty(RECALL_LISTVIEW, "update_all", NULL);
		cur_sel = 0;
		GUI_SetProperty(RECALL_LISTVIEW, "select", (void*)(&cur_sel));
	}
	else
	{
		GUI_SetProperty(RECALL_LISTVIEW, "update_all", NULL);
		GUI_SetProperty(RECALL_LISTVIEW, "select", (void*)(&cur_sel));
	}
	
     	
    	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_recall_destroy(GuiWidget *widget, void *usrdata)
{

   	// app_create_dialog("wnd_channel_info");
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_recall_keypress(GuiWidget *widget, void *usrdata)
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
						ret = EVENT_TRANSFER_STOP;
					}
					break;
				case STBK_OK:
					GUI_GetProperty(RECALL_LISTVIEW, "select", &sel);
					g_recall_count = sel;//app_play_get_recall_total_count()-
					GUI_EndDialog(WND_RECALL_LIST);
					break;
				case STBK_EXIT:
				case STBK_MENU:
					{
						GUI_GetProperty(RECALL_LISTVIEW, "select", &sel);
						g_recall_count = sel;
						g_AppPlayOps.play_list_create(&s_old_info);
                        			GUI_EndDialog(WND_RECALL_LIST);
						 if(g_AppFullArb.state.tv == STATE_ON 
			                        || (event->key.sym == VK_BOOK_TRIGGER))
							    GUI_EndDialog("wnd_channel_info");			
							break;
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


#define RECALL_LISTVIEW_
SIGNAL_HANDLER int app_recall_list_get_total(GuiWidget *widget, void *usrdata)
{
	uint32_t total = 0;
	/*GxMsgProperty_NodeNumGet node_num_get = {0};
	node_num_get.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
	total = node_num_get.node_num;*/
	total = app_play_get_recall_total_count();

	if(total > MAX_RECALL_CPUNT)
	{
		total = MAX_RECALL_CPUNT;
	}
	return total;
}

SIGNAL_HANDLER int app_recall_list_get_data(GuiWidget *widget, void *usrdata)
{
    #define CHANNEL_NUM_LEN	    (10)
	ListItemPara* item = NULL;
	static GxMsgProperty_NodeByPosGet node;
	static char buffer[CHANNEL_NUM_LEN];
	int cur_sel = 0;

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	cur_sel = item->sel;//app_play_get_recall_total_count()-
	if(cur_sel <=0 || cur_sel > MAX_RECALL_CPUNT)
	{
		cur_sel = 0;
	}
	//g_AppPlayOps.play_list_create(&(g_AppPlayOps.recall_play[cur_sel].view_info));
	//col-0: num
	item->x_offset = 2;
	item->image = NULL;
	memset(buffer, 0, CHANNEL_NUM_LEN);
	sprintf(buffer, " %d", (item->sel+1));
	item->string = buffer;
	
	//col-1: channel name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	node.node_type = NODE_PROG;
	//node.pos = item->sel;
	if(g_AppPlayOps.recall_play[cur_sel].view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		node.id = g_AppPlayOps.recall_play[cur_sel].view_info.radio_prog_cur;	
	}
	else
	{
		node.id = g_AppPlayOps.recall_play[cur_sel].view_info.tv_prog_cur;
	}
	//get node
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
	item->string = (char*)node.prog_data.prog_name;

	  //col-2: $
	item = item->next;
	item->x_offset = 0;
	item->string = NULL;
	if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
	    item->image = "s_icon_money.bmp";
	}
	else
	{
	    item->image = NULL;
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_recall_list_keypress(GuiWidget *widget, void *usrdata)
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
					GUI_GetProperty(RECALL_LISTVIEW, "select", &sel);
					g_recall_count = sel;
					if (g_AppPvrOps.state != PVR_DUMMY)
					{
					    PopDlg pop;
					    pvr_state cur_pvr = PVR_DUMMY;
					    memset(&pop, 0, sizeof(PopDlg));
					    pop.type = POP_TYPE_YES_NO;

                            if(g_AppPvrOps.state == PVR_TMS_AND_REC)
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

                        GxBusPmViewInfo view_info = {0};
                        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

                        if(view_info.stream_type != g_AppPlayOps.recall_play[sel].view_info.stream_type)
                        {
                            if(view_info.stream_type != GXBUS_PM_PROG_TV)
                            {
                                g_AppFullArb.state.tv = STATE_ON;
                                g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
                            }else
                            {
                                g_AppFullArb.state.tv = STATE_OFF;
                                g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
                            }						
                        }
                        g_AppPlayOps.play_list_create(&(g_AppPlayOps.recall_play[sel].view_info));
                        g_AppPlayOps.program_play(PLAY_TYPE_FORCE, g_AppPlayOps.recall_play[sel].play_count);

                        g_AppFullArb.state.pause = STATE_OFF;
                        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                        GUI_EndDialog(WND_RECALL_LIST);
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

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

#else

SIGNAL_HANDLER int app_recall_create(GuiWidget *widget, void *usrdata)
{
    	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_recall_destroy(GuiWidget *widget, void *usrdata)
{

	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_recall_keypress(GuiWidget *widget, void *usrdata)
{
    	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_recall_list_get_total(GuiWidget *widget, void *usrdata)
{
	return 0;
}
SIGNAL_HANDLER int app_recall_list_get_data(GuiWidget *widget, void *usrdata)
{
    	return EVENT_TRANSFER_STOP;
}	
SIGNAL_HANDLER int app_recall_list_keypress(GuiWidget *widget, void *usrdata)
{
    	return EVENT_TRANSFER_STOP;
}	
#endif

