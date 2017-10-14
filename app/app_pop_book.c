/*
 * =====================================================================================
 *
 *       Filename:  app_pop_tip.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2011年08月18日 09时47分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app.h"
#include "app_utility.h"
#include "app_module.h"
#include "app_book.h"
#define WND_POPDLG_BOOK          "wnd_pop_book"
#define TXT_BOOK_TITLE           "txt_pop_book_title"
#define IMG_BOOK_OPT             "img_pop_book_opt"
#define BMP_OK              "s_bar_choice_blue4.bmp"
#define BMP_CANCLE          "s_bar_choice_blue4_l.bmp"
#define TXT_BOOK_TIP             "txt_pop_book"
#define TXT_BOOK_SEC         "txt_pop_second"
#define STR_ID_30_SEC         "30"

static event_list* sp_CountTimer = NULL;
static unsigned char count = 30;

static WndStatus       s_book_pop_state        = WND_EXEC;

static int timer_power_off_refresh_tip(void *userdata)
{
	char buffer[5] = {0};
	count--;
	sprintf(buffer,"%d", count);
	GUI_SetProperty(TXT_BOOK_SEC, "string", buffer);
	if(count == 0)
	{
//		GUI_SetProperty(TXT_BOOK_SEC, "state", "hide");
		s_book_pop_state = WND_OK;
		remove_timer(sp_CountTimer);
		sp_CountTimer = NULL;
		count = 30;
	}
	return 0;
}

SIGNAL_HANDLER  int app_book_popdlg_set_service(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_book_popdlg_create(GuiWidget *widget, void *usrdata)
{
	GUI_SetProperty(IMG_BOOK_OPT, "img", BMP_OK);
	GUI_SetFocusWidget("btn_pop_book_ok");
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_book_popdlg_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_book_popdlg_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_OK:
				if(0 == strcasecmp("btn_pop_book_ok", GUI_GetFocusWidget()))
				{
					s_book_pop_state = WND_OK;
				}
				else
				{
					s_book_pop_state = WND_CANCLE;
				}
				break;
			case STBK_LEFT:
			case STBK_RIGHT:
				if(0 == strcasecmp("btn_pop_book_ok", GUI_GetFocusWidget()))
				{
					GUI_SetProperty(IMG_BOOK_OPT, "img", BMP_CANCLE);
					GUI_SetFocusWidget("btn_pop_book_cancel");
				}
				else
				{
					GUI_SetProperty(IMG_BOOK_OPT, "img", BMP_OK);
					GUI_SetFocusWidget("btn_pop_book_ok");
				}
				break;
			case STBK_MENU:
			case STBK_EXIT:
				s_book_pop_state = WND_CANCLE;
				break;

			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

WndStatus book_popdlg_create(BookType book_type)
{
	s_book_pop_state = WND_EXEC;
	app_msg_destroy(g_app_msg_self);
	app_create_dialog("wnd_pop_book");
	if(book_type == BOOKTYPE_POWOFF)
	{
		GUI_SetProperty(TXT_BOOK_TIP, "string", STR_ID_POWER_OFF_TIP);
		GUI_SetProperty(TXT_BOOK_SEC, "string", STR_ID_30_SEC);
		GUI_SetProperty(TXT_BOOK_TITLE, "string", STR_ID_POWER_OFF);
		APP_TIMER_ADD(sp_CountTimer,timer_power_off_refresh_tip,1000,TIMER_REPEAT);

	}
	if(book_type == BOOKTYPE_PVR || book_type == BOOKTYPE_PLAY)
	{
		GUI_SetProperty(TXT_BOOK_TIP, "string", STR_ID_BOOK_ARRIVE_INFO);
		GUI_SetProperty(TXT_BOOK_SEC, "string", STR_ID_30_SEC);
		GUI_SetProperty(TXT_BOOK_TITLE, "string", STR_ID_BOOK);
		APP_TIMER_ADD(sp_CountTimer,timer_power_off_refresh_tip,1000,TIMER_REPEAT);
	}
	while(s_book_pop_state == WND_EXEC)
	{
		GUI_LoopEvent();
		GxCore_ThreadDelay(50);
	}
	GUI_StartSchedule();
//	GUI_SetProperty(TXT_BOOK_SEC, "state", "hide");
	GUI_EndDialog("wnd_pop_book");
	GUI_SetInterface("flush",NULL);
	app_msg_init(g_app_msg_self);

	APP_TIMER_REMOVE(sp_CountTimer);

	count = 30;

	return s_book_pop_state;
}
