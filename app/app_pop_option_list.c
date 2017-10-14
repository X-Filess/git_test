#include "app_pop.h"

#define WND_POPLIST "wnd_pop_option_list" 
#define TEXT_POPLIST_TITILE "text_pop_option_title"
#define LISTVIEW_POPLIST "listview_pop_option"

static enum
{
	POP_LIST_END,
	POP_LIST_START,
}s_pop_list_sate = POP_LIST_END;
static PopList *s_pop_list_param = NULL;
static int s_select_ret = -1;

SIGNAL_HANDLER int app_pop_option_create(GuiWidget *widget, void *usrdata)
{
	GUI_SetProperty(TEXT_POPLIST_TITILE, "string", (void*)(s_pop_list_param->title));
	GUI_SetProperty(LISTVIEW_POPLIST, "select", &s_pop_list_param->sel);
	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_option_destroy(GuiWidget *widget, void *usrdata)
{
	s_pop_list_param = NULL;
	app_block_msg_init(g_app_msg_self);
	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_option_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;

	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
            case VK_BOOK_TRIGGER:
            	s_select_ret = s_pop_list_param->sel;
				s_pop_list_sate = POP_LIST_END;

				GUI_EndDialog("after wnd_full_screen");
				break;

			case STBK_OK:
				GUI_GetProperty(LISTVIEW_POPLIST, "select", &s_select_ret);
				s_pop_list_sate = POP_LIST_END;
				break;

			case STBK_MENU:      
			case STBK_EXIT:
				s_select_ret = s_pop_list_param->sel;
				s_pop_list_sate = POP_LIST_END;
				break;
                    #if MINI_256_COLORS_OSD_SUPPORT
			case STBK_PAGE_UP:
			case STBK_PAGE_DOWN:
			{
				return EVENT_TRANSFER_STOP;
			}
		      #endif
			default:
				break;
		}
	}
	
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_pop_option_list_keypress(GuiWidget *widget, void *usrdata)
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
			switch(find_virtualkey(event->key.sym))
			{
				case STBK_PAGE_UP:
				{
					uint32_t sel;
					GUI_GetProperty(LISTVIEW_POPLIST, "select", &sel);
					if (sel == 0)
					{
						sel = s_pop_list_param->item_num-1;
						GUI_SetProperty(LISTVIEW_POPLIST, "select", &sel);
						ret =  EVENT_TRANSFER_STOP; 
					}
					else
					{
						ret =  EVENT_TRANSFER_KEEPON; 
					}
				}
				break;
				case STBK_PAGE_DOWN:
				{
					uint32_t sel;
					GUI_GetProperty(LISTVIEW_POPLIST, "select", &sel);
					if (s_pop_list_param->item_num == sel+1)
					{
						sel = 0;
						GUI_SetProperty(LISTVIEW_POPLIST, "select", &sel);
						ret =  EVENT_TRANSFER_STOP; 
					}
					else
					{
						ret = EVENT_TRANSFER_KEEPON; 
					}
				}
				break;
				case STBK_UP:
				case STBK_DOWN:
				case STBK_OK:
				case STBK_MENU:      
				case STBK_EXIT:
                if (GXCORE_SUCCESS == GUI_CheckDialog("wnd_tp_list"))
                {
	                GUI_SetProperty("img_tp_list_tip_green", "state", "show");
					GUI_SetProperty("text_tp_list_tip_green", "state", "show");
					GUI_SetProperty("img_tp_list_tip_info", "state", "show");
					GUI_SetProperty("text_tp_list_tip_info", "state", "show");
                }
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

SIGNAL_HANDLER int app_pop_option_list_get_total(GuiWidget *widget, void *usrdata)
{
	return s_pop_list_param->item_num;
}

SIGNAL_HANDLER int app_pop_option_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;
	static char num_buf[10] = {0};

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	//col-0: num
	item->x_offset = 0;
	item->image = NULL;

	if(s_pop_list_param->show_num == false)
	{
		item->string = NULL;
	}
	else
	{
		memset(num_buf, 0, sizeof(num_buf));
		sprintf(num_buf, " %d", (item->sel+1));
		item->string = num_buf;
	}
		
	//col-1: 
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	item->string = s_pop_list_param->item_content[item->sel];
	
	return EVENT_TRANSFER_STOP;
}

int poplist_create(PopList* pop_list)
{
#if(MINI_16_BITS_OSD_SUPPORT == 1)
#define WND_POPLIST_X 366
#define WND_POPLIST_Y 104
#else
#define WND_POPLIST_X 458
#define WND_POPLIST_Y 130
#endif

	int32_t pos_x = 0;
	int32_t pos_y = 0;

	if(pop_list == NULL)
	{
		return -1;
	}

	s_pop_list_param = pop_list;

	app_create_dialog(WND_POPLIST);
	if ((s_pop_list_param->pos.x==0)&&(s_pop_list_param->pos.y==0))
	{
		pos_x = WND_POPLIST_X; 
		pos_y = WND_POPLIST_Y;
	}
	else
	{
		pos_x = s_pop_list_param->pos.x;
		pos_y = s_pop_list_param->pos.y;
	}

	GUI_SetProperty(WND_POPLIST, "move_window_x", &pos_x);
	GUI_SetProperty(WND_POPLIST, "move_window_y", &pos_y);
	
	s_pop_list_sate = POP_LIST_START; 
	//app_msg_destroy(g_app_msg_self);
	app_block_msg_destroy(g_app_msg_self);   
#if 0
	while(s_pop_list_sate == POP_LIST_START)
	{
		GUI_Exec();               
		GxCore_ThreadDelay(50);
	}
#endif
	while(s_pop_list_sate == POP_LIST_START)
    {
        //GUI_Loop();
         GUI_LoopEvent();
        GxCore_ThreadDelay(50);
    }
	GUI_StartSchedule();
	
	GUI_EndDialog(WND_POPLIST);
	//GUI_Exec();
	GUI_SetInterface("flush",NULL);
	//app_msg_init(g_app_msg_self);
	return s_select_ret;
}

