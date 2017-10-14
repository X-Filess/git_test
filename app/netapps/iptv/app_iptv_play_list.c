#include "app.h"
#if IPTV_SUPPORT
#include "app_iptv_play_list.h"

#define WND_IPTV_LIST    "wnd_iptv_list"
#define LIST_PROG    "listview_iptvlist_list"
#define CMB_GROUP    "cmb_iptv_list_group"

static struct
{
	int group_total;
	char **group_title;
	int prog_total;
	char **prog_title;
	IPTVListCb list_cb;
}s_list_snapshot;

status_t app_iptv_list_create(IPTVListCb *list_cb)
{
	memset(&s_list_snapshot, 0, sizeof(s_list_snapshot));
	if(list_cb != NULL)
	{
		memcpy(&s_list_snapshot.list_cb, list_cb, sizeof(IPTVListCb));
	}

	GUI_CreateDialog(WND_IPTV_LIST);
	GUI_SetFocusWidget(LIST_PROG);

	return GXCORE_SUCCESS;
}

status_t app_iptv_list_group_update(int group_total, char **group_title)
{
	char *content_str = NULL;
	int content_len = 0;
	int i = 0;

	if((group_total <= 0) || (group_title == NULL))
	{
		s_list_snapshot.group_total = 0;
		s_list_snapshot.group_title = NULL;
		GUI_SetProperty(CMB_GROUP, "content", "[]");
	}
	else
	{
		content_str = (char*)GxCore_Calloc(group_total * 32 + 3, 1);
		if(content_str != NULL)
		{
			s_list_snapshot.group_total = group_total;
			s_list_snapshot.group_title = group_title;

			content_str[0] = '[';
			for(i = 0; i < group_total; i++)
			{
				content_len++;
				strncpy(content_str + content_len, group_title[i], 31);
				content_len += strlen(group_title[i]);
				content_str[content_len] = ',';
			}
			content_str[content_len] = ']';
			GUI_SetProperty(CMB_GROUP, "content", content_str);

			GxCore_Free(content_str);
			content_str = NULL;
		}
	}

	return GXCORE_SUCCESS;
}

status_t app_iptv_list_prog_update(int prog_total, char **prog_title)
{
	if((prog_total <= 0) || (prog_title == NULL))
	{
		s_list_snapshot.prog_total = 0;
		s_list_snapshot.prog_title = NULL;
	}
	else
	{
		s_list_snapshot.prog_total = prog_total;
		s_list_snapshot.prog_title = prog_title;
	}

	GUI_SetProperty(LIST_PROG, "update_all", NULL);

	return GXCORE_SUCCESS;
}

void app_iptv_list_set_group_sel(int group_sel)
{
	int sel = group_sel;
	GUI_SetProperty(CMB_GROUP, "select", &sel);
}

void app_iptv_list_set_prog_sel(int prog_sel)
{
	int sel = prog_sel;
	GUI_SetProperty(LIST_PROG, "select", &sel);
}

SIGNAL_HANDLER int app_iptv_list_group_change(GuiWidget *widget, void *usrdata)
{
	int sel = 0;

	if(s_list_snapshot.list_cb.group_change != NULL)
	{
		GUI_GetProperty(CMB_GROUP, "select", &sel);
		s_list_snapshot.list_cb.group_change(sel);
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptvlist_list_get_total(GuiWidget *widget, void *usrdata)
{
	return s_list_snapshot.prog_total;
}

SIGNAL_HANDLER int app_iptvlist_list_get_data(GuiWidget *widget, void *usrdata)
{
	static char  buffer[10] = {0};
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return EVENT_TRANSFER_STOP;
	if(0 > item->sel)
		return EVENT_TRANSFER_STOP;
	if(s_list_snapshot.group_title == NULL)
		return EVENT_TRANSFER_STOP;

	item->x_offset = 2;
	item->image = NULL;
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", (item->sel+1));
	item->string = buffer;

	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	item->string = s_list_snapshot.prog_title[item->sel];;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptvlist_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	void (*exit_cb)(void);

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case VK_BOOK_TRIGGER:
			break;

			case STBK_MENU:
			case STBK_EXIT:
				exit_cb = s_list_snapshot.list_cb.list_exit;
				memset(&s_list_snapshot, 0, sizeof(s_list_snapshot));
				GUI_EndDialog(WND_IPTV_LIST);
				if(exit_cb != NULL)
				{
					exit_cb();
				}
			break;

			case STBK_OK:
				if(s_list_snapshot.list_cb.list_ok != NULL)
				{
					int g_sel = 0;
					int p_sel = 0;
					GUI_GetProperty(CMB_GROUP, "select", &g_sel);
					GUI_GetProperty(LIST_PROG, "select", &p_sel);
					s_list_snapshot.list_cb.list_ok(g_sel, p_sel);
				}
			break;

			case STBK_LEFT:
			case STBK_RIGHT:
				GUI_SendEvent(CMB_GROUP, event);
			break;

			case STBK_UP:
			case STBK_DOWN:
			case STBK_PAGE_UP:
			case STBK_PAGE_DOWN:
				ret = EVENT_TRANSFER_KEEPON;
			break;

			default:
			break;
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_iptvlist_list_change(GuiWidget *widget, void *usrdata)
{
	int sel = -1;
	if(s_list_snapshot.list_cb.prog_change != NULL)
	{
		GUI_GetProperty(LIST_PROG, "select", &sel);
		s_list_snapshot.list_cb.prog_change(sel);
	}
	return EVENT_TRANSFER_STOP;
}

#else
SIGNAL_HANDLER int app_iptv_list_group_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptvlist_list_get_total(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptvlist_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptvlist_list_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptvlist_list_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#endif
