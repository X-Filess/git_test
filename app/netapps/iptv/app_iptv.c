#include "app.h"
#if IPTV_SUPPORT
#include "app_module.h"
#include "app_iptv_play.h"
#include "app_iptv_play_list.h"
#include "app_wnd_file_list.h"
#include "app_iptv_list_service.h"
#include <gx_apps.h>

#ifdef ECOS_OS
#define IPTV_VIDEO_PIC    "/dvb/theme/image/netapps/default.png"
#define GIF_PATH "/dvb/theme/image/netapps/loading.gif"
#else
#define IPTV_VIDEO_PIC    WORK_PATH"/theme/image/netapps/default.png"
#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
#endif

#define IPTV_LIST_SOURCE_KEY    "netapps>iptv_list_source"

#define WND_IPTV    "wnd_iptv"
#define LIST_GROUP    "list_iptv_group"
#define TEXT_PAGE_NUM    "text_iptv_page"
#define IMG_ARROW_UP    "img_iptv_page_arrow_up"
#define IMG_ARROW_DOWN    "img_iptv_page_arrow_down"
#define IMG_GIF    "img_iptv_gif"
#define IMG_RED_KEY "img_iptv_red"
#define TEXT_RED_KEY "text_iptv_red"
#define IMG_GREEN_KEY "img_iptv_green"
#define TEXT_GREEN_KEY "text_iptv_green"
#define IMG_BLUE_KEY "img_iptv_blue"
#define TEXT_BLUE_KEY "text_iptv_blue"
#define IMG_YELLOW_KEY "img_iptv_yellow"
#define TEXT_YELLOW_KEY "text_iptv_yellow"
#define IMG_OK_KEY "img_iptv_ok"
#define TEXT_OK_KEY "text_iptv_ok"
#define TXT_POPUP    "txt_iptv_popup"
#define IMG_POPUP    "img_iptv_popup"
#define CMB_LIST_SOURCE    "cmb_iptv_list_source"

#define LISTVIEW_FOCUS_IMG    "s_bar_blue.bmp"
#define MAX_FUNC_TIP_LEN    (30)

#define MAX_PAGE_ITEM (6)

typedef enum
{
    IPTV_OBJ_GROUP = 0,
    IPTV_OBJ_PAGE,
    IPTV_OBJ_SOURCE,
    IPTV_OBJ_UNKNOW,
}IptvViewObj;

typedef struct
{
    char *prog_name;
    char *prog_pic;
    char *prog_url;
    IptvAdClass prog_ad;
}IptvProg;

static char *s_text_video_title[MAX_PAGE_ITEM] =
{
    "text_iptv_item_title1",
    "text_iptv_item_title2",
    "text_iptv_item_title3",
    "text_iptv_item_title4",
    "text_iptv_item_title5",
    "text_iptv_item_title6",
};

static char *s_img_video_focus[MAX_PAGE_ITEM] =
{
    "img_iptv_item_focus1",
    "img_iptv_item_focus2",
    "img_iptv_item_focus3",
    "img_iptv_item_focus4",
    "img_iptv_item_focus5",
    "img_iptv_item_focus6",
};

static char *s_img_video_unfocus[MAX_PAGE_ITEM] =
{
    "img_iptv_item_unfocus1",
    "img_iptv_item_unfocus2",
    "img_iptv_item_unfocus3",
    "img_iptv_item_unfocus4",
    "img_iptv_item_unfocus5",
    "img_iptv_item_unfocus6",
};

static event_list* s_iptv_gif_timer = NULL;
static event_list* s_iptv_popup_timer = NULL;
static int source_group_sel = 0;
static bool	first_set_ltv = false;
static bool pop_msg_on = false;

static IptvListClass *iptv_channellist = NULL;
static IptvListClass *iptv_grouplist = NULL;
static IptvListClass *iptv_sub_channellist = NULL;

static IptvViewObj s_iptv_obj = IPTV_OBJ_UNKNOW;
static int s_iptv_group_total = 0;
static int s_cur_group_sel = 0;
static char **s_iptv_group_title;
static int s_page_total = 0;
static int s_cur_page_num = 0;
static int s_page_prog_num = 0;
static int s_page_prog_sel = 0;
static IptvProg s_page_prog_item[MAX_PAGE_ITEM];

static int s_play_group_sel = 0;
static int s_play_prog_sel = 0;
//static char **s_play_group_title = NULL;
//static char **s_play_prog_title = NULL;
static IptvListSource s_iptv_list_source;
static bool s_first = false;

static char *s_file_path = NULL;
static char *s_file_path_bak = NULL;
#if 0
static void app_iptv_play_ok_ctrl(void);
static void app_iptv_play_exit_ctrl(void);
static void app_iptv_play_prev_ctrl(void);
static void app_iptv_play_next_ctrl(void);
#endif
static void app_iptv_exit(void);
static void app_iptv_clean_menu_info(void);
static void app_iptv_ok_press(void);
static status_t app_iptv_update_page(int page_num);
#if 0
static void app_iptv_list_ok_cb(int group_selint, int prog_sel);
static void app_iptv_list_exit_cb(void);
static status_t iptv_error_source_replay(void);
#endif
static void app_iptv_red_key_func(void);
static void app_iptv_green_key_func(void);
static void app_iptv_blue_key_func(void);
static status_t app_iptv_get_list(IptvListSource list_source);

static void iptv_list_destroy(void)
{
	if(iptv_channellist)
	{
		iptv_list_free(iptv_channellist);
		iptv_channellist = NULL;
	}
	if(iptv_grouplist)
	{
		iptv_list_free(iptv_grouplist);
		iptv_grouplist = NULL;
	}
	if(iptv_sub_channellist)
	{
		iptv_list_free(iptv_sub_channellist);
		iptv_sub_channellist = NULL;
	}
}

static void app_iptv_list_init(IptvListClass *list)
{
    iptv_list_destroy();
    iptv_channellist = list;
    if(iptv_channellist && (iptv_channellist->iptv_item_num > 0))
    {
        iptv_grouplist = iptv_get_group_list(iptv_channellist);
    }
}

static void app_iptv_update_group_channellist(void)
{
    s_page_total = 0;
    if(iptv_channellist && (iptv_channellist->iptv_item_num > 0))
    {
        if(s_cur_group_sel == 0)
        {
            s_page_total = iptv_channellist->iptv_item_num / MAX_PAGE_ITEM;
            if(iptv_channellist->iptv_item_num % MAX_PAGE_ITEM > 0)
                s_page_total++;
        }
        else if((s_cur_group_sel>0)
                && (iptv_grouplist)
                && (iptv_grouplist->iptv_item_num>0)
                && ((s_cur_group_sel-1) < iptv_grouplist->iptv_item_num))
        {
            if(iptv_sub_channellist)
            {
                iptv_list_free(iptv_sub_channellist);
                iptv_sub_channellist = NULL;
            }
            iptv_sub_channellist = iptv_get_list_by_type(iptv_channellist, iptv_grouplist->iptv_item_list[s_cur_group_sel-1].iptv_key);
            if(iptv_sub_channellist && (iptv_sub_channellist->iptv_item_num > 0))
            {
                s_page_total = iptv_sub_channellist->iptv_item_num / MAX_PAGE_ITEM;
                if(iptv_sub_channellist->iptv_item_num % MAX_PAGE_ITEM > 0)
                    s_page_total++;
            }
        }
    }
}

static int app_iptv_draw_gif(void* usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
	return 0;
}

static void _iptv_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
	GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _iptv_free_gif(void)
{
	GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void app_iptv_show_gif(void)
{
	GUI_SetProperty(IMG_GIF, "state", "show");
	if(0 != reset_timer(s_iptv_gif_timer))
	{
		s_iptv_gif_timer = create_timer(app_iptv_draw_gif, 100, NULL, TIMER_REPEAT);
	}
}

static void app_iptv_hide_gif(void)
{
	remove_timer(s_iptv_gif_timer);
	s_iptv_gif_timer = NULL;
	GUI_SetProperty(IMG_GIF, "state", "hide");
}

static void app_iptv_hide_popup_msg(void)
{
	if(s_iptv_popup_timer)
	{
		remove_timer(s_iptv_popup_timer);
		s_iptv_popup_timer = NULL;
	}

	pop_msg_on = false;
	GUI_SetProperty(TXT_POPUP, "state", "hide");
	GUI_SetProperty(IMG_POPUP, "state", "hide");
}

static int _iptv_popup_timer_timeout(void *userdata)
{
	app_iptv_hide_popup_msg();
	return 0;
}

static void app_iptv_show_popup_msg(char* pMsg, int time_out)
{
	pop_msg_on = true;
	GUI_SetProperty(TXT_POPUP, "string", pMsg);
	GUI_SetProperty(IMG_POPUP, "state", "show");
	GUI_SetProperty(TXT_POPUP, "state", "show");

	if(time_out > 0)
	{
		if (reset_timer(s_iptv_popup_timer) != 0)
		{
			s_iptv_popup_timer = create_timer(_iptv_popup_timer_timeout, time_out, NULL, TIMER_ONCE);
		}
	}
}

static void _iptv_show_item(int index, IptvProg *iptv_item)
{
	if((index >= MAX_PAGE_ITEM) || (index < 0) || (iptv_item == NULL))
		return;

	//title
	if(iptv_item->prog_name != NULL)
	{
		GUI_SetProperty(s_text_video_title[index], "string", iptv_item->prog_name);
		GUI_SetProperty(s_text_video_title[index], "state", "show");
	}

	//pic
	if(iptv_item->prog_url != NULL)
	{
		GUI_SetProperty(s_img_video_unfocus[index], "load_scal_img", iptv_item->prog_pic);
		GUI_SetProperty(s_img_video_unfocus[index], "state", "show");
	}
}

static void _iptv_hide_item(int index)
{
	if((index >= MAX_PAGE_ITEM) || (index < 0))
		return;

	GUI_SetProperty(s_text_video_title[index], "state", "hide");
	GUI_SetProperty(s_img_video_unfocus[index], "state", "hide");
}

#if 0
static void app_iptv_clear_all_item(char *default_pic)
{
	int i = 0;
	for(i=0; i<MAX_PAGE_ITEM; i++)
	{
		GUI_SetProperty(s_text_video_title[i], "string", " ");
		GUI_SetProperty(s_img_video_unfocus[i], "load_scal_img", default_pic);
		GUI_SetProperty(s_img_video_unfocus[i], "state", "show");
	}
}
#endif

static void _iptv_focus_item(int index)
{
	if((index >= 0) && (index < MAX_PAGE_ITEM))
		GUI_SetProperty(s_img_video_focus[index], "state", "show");
}

static void _iptv_unfocus_item(int index)
{
	if((index >= 0) && (index < MAX_PAGE_ITEM))
		GUI_SetProperty(s_img_video_focus[index], "state", "hide");
}

#if 0
static status_t app_iptv_page_pic_updata(int index, char *pic_url)
{
	if((index >= MAX_PAGE_ITEM) || (index < 0))
		return GXCORE_ERROR;

	if(pic_url != NULL)
	{
		GUI_SetProperty(s_img_video_unfocus[index], "load_scal_img", pic_url);
		GUI_SetProperty(s_img_video_unfocus[index], "state", "show");
	}
	return GXCORE_SUCCESS;
}
#endif

static status_t app_iptv_page_item_update(int iptv_num, IptvProg *iptv_item)
{
	int i = 0;
	int cur_sel = 0;

	cur_sel = s_page_prog_sel;
	_iptv_unfocus_item(cur_sel);
	for(i = 0; i < iptv_num; i++)
	{
		_iptv_show_item(i, iptv_item + i);
	}
	for(; i < MAX_PAGE_ITEM; i++)
	{
		_iptv_hide_item(i);
	}

	if(cur_sel >= iptv_num)
	{
		cur_sel = iptv_num - 1;
	}

	if(s_iptv_obj == IPTV_OBJ_PAGE)
	{
		_iptv_focus_item(cur_sel);
	}

	if(cur_sel != s_page_prog_sel)
	{
		s_page_prog_sel = cur_sel;
	}

	return GXCORE_SUCCESS;
}

static status_t app_iptv_page_info_update(void)
{
	char buf[64] = {0};

	if(s_page_total > 0)
	{
		sprintf(buf, "[ %d / %d ]", s_cur_page_num + 1, s_page_total);
		GUI_SetProperty(TEXT_PAGE_NUM, "string", buf);
		GUI_SetProperty(TEXT_PAGE_NUM, "state", "show");
	}
	else
	{
		GUI_SetProperty(TEXT_PAGE_NUM, "state", "hide");
	}

	if(s_page_total <= 1)
	{
		GUI_SetProperty(IMG_ARROW_UP, "state", "hide");
		GUI_SetProperty(IMG_ARROW_DOWN, "state", "hide");
	}
	else
	{
		if(s_cur_page_num >= s_page_total - 1) //last page
		{
			GUI_SetProperty(IMG_ARROW_UP, "state", "show");
			GUI_SetProperty(IMG_ARROW_DOWN, "state", "hide");
		}
		else if(s_cur_page_num == 0)//first page
		{
			GUI_SetProperty(IMG_ARROW_UP, "state", "hide");
			GUI_SetProperty(IMG_ARROW_DOWN, "state", "show");
		}
		else
		{
			GUI_SetProperty(IMG_ARROW_UP, "state", "show");
			GUI_SetProperty(IMG_ARROW_DOWN, "state", "show");
		}
	}

	return GXCORE_SUCCESS;
}

static void app_iptv_info_tip_draw(char *blue_inf, char *green_inf, char *red_inf, char *ok_inf)
{
    if(blue_inf != NULL)
    {
        GUI_SetProperty(IMG_BLUE_KEY, "state", "show");
        GUI_SetProperty(TEXT_BLUE_KEY, "state", "show");
        GUI_SetProperty(TEXT_BLUE_KEY, "string", blue_inf);
    }
    else
    {
        GUI_SetProperty(IMG_BLUE_KEY, "state", "hide");
        GUI_SetProperty(TEXT_BLUE_KEY, "state", "hide");
    }

    if(green_inf != NULL)
    {
#if 0
        GUI_SetProperty(IMG_GREEN_KEY, "state", "show");
        GUI_SetProperty(TEXT_GREEN_KEY, "state", "show");
        GUI_SetProperty(TEXT_GREEN_KEY, "string", green_inf);
#else
        GUI_SetProperty(IMG_GREEN_KEY, "state", "hide");
        GUI_SetProperty(TEXT_GREEN_KEY, "state", "hide");
#endif
    }
    else
    {
        GUI_SetProperty(IMG_GREEN_KEY, "state", "hide");
        GUI_SetProperty(TEXT_GREEN_KEY, "state", "hide");
    }

    if(red_inf != NULL)
    {
#if 0
        GUI_SetProperty(IMG_RED_KEY, "state", "show");
        GUI_SetProperty(TEXT_RED_KEY, "state", "show");
        GUI_SetProperty(TEXT_RED_KEY, "string", red_inf);
#else
        GUI_SetProperty(IMG_RED_KEY, "state", "hide");
        GUI_SetProperty(TEXT_RED_KEY, "state", "hide");
#endif
    }
    else
    {
        GUI_SetProperty(IMG_RED_KEY, "state", "hide");
        GUI_SetProperty(TEXT_RED_KEY, "state", "hide");
    }

    if(ok_inf != NULL)
    {
        GUI_SetProperty(IMG_OK_KEY, "state", "show");
        GUI_SetProperty(TEXT_OK_KEY, "state", "show");
        GUI_SetProperty(TEXT_OK_KEY, "string", ok_inf);
    }
    else
    {
        GUI_SetProperty(IMG_OK_KEY, "state", "hide");
        GUI_SetProperty(TEXT_OK_KEY, "state", "hide");
    }
}

static void app_iptv_listtype_info_tip(void)
{
    switch(s_iptv_list_source)
    {
        case IPTV_LIST_REMOTE:
            app_iptv_info_tip_draw(STR_ID_REFRESH, NULL, NULL, NULL);
            break;

        case IPTV_LIST_LOCAL:
            app_iptv_info_tip_draw(STR_ID_ADD_USB, NULL, NULL, NULL);
            break;

        case IPTV_LIST_FAV:
            app_iptv_info_tip_draw(NULL, NULL, NULL, NULL);
            break;

        default:
            break;
    }
}

static void app_iptv_group_info_tip(void)
{
    switch(s_iptv_list_source)
    {
        case IPTV_LIST_REMOTE:
            app_iptv_info_tip_draw(STR_ID_REFRESH, NULL, NULL, STR_ID_SELECT);
            break;

        case IPTV_LIST_LOCAL:
            app_iptv_info_tip_draw(STR_ID_ADD_USB, NULL, NULL, STR_ID_SELECT);
            break;

        case IPTV_LIST_FAV:
            app_iptv_info_tip_draw(NULL, NULL, NULL, STR_ID_SELECT);
            break;

        default:
            break;
    }
}

static void app_iptv_page_info_tip(void)
{
    switch(s_iptv_list_source)
    {
        case IPTV_LIST_REMOTE:
            app_iptv_info_tip_draw(STR_ID_REFRESH, STR_ID_FAV, STR_ID_DELETE, STR_ID_PLAY);
            break;

        case IPTV_LIST_LOCAL:
            app_iptv_info_tip_draw(STR_ID_ADD_USB, STR_ID_FAV, STR_ID_DELETE, STR_ID_PLAY);
            break;

        case IPTV_LIST_FAV:
            app_iptv_info_tip_draw(NULL, NULL, STR_ID_DELETE, STR_ID_PLAY);
            break;

        default:
            break;
    }
}

static void app_iptv_null_info_tip(void)
{
    switch(s_iptv_list_source)
    {
        case IPTV_LIST_REMOTE:
            app_iptv_info_tip_draw(STR_ID_REFRESH, NULL, NULL, NULL);
            break;

        case IPTV_LIST_LOCAL:
            app_iptv_info_tip_draw(STR_ID_ADD_USB, NULL, NULL, NULL);
            break;

        case IPTV_LIST_FAV:
            app_iptv_info_tip_draw(NULL, NULL, NULL, NULL);
            break;

        default:
            break;
    }
}

static void app_iptv_info_tip_update(void)
{
    switch(s_iptv_obj)
    {
        case IPTV_OBJ_SOURCE:
            app_iptv_listtype_info_tip();
            break;

        case IPTV_OBJ_GROUP:
            app_iptv_group_info_tip();
            break;

        case IPTV_OBJ_PAGE:
            app_iptv_page_info_tip();
            break;

        default:
            app_iptv_null_info_tip();
            break;
    }
}

static status_t app_iptv_set_obj(IptvViewObj obj)
{
	if(obj >= IPTV_OBJ_UNKNOW)
		return GXCORE_ERROR;

	if(obj == IPTV_OBJ_GROUP)
	{
        GUI_SetFocusWidget(LIST_GROUP);
		_iptv_unfocus_item(s_page_prog_sel);
		GUI_SetProperty(LIST_GROUP, "focus_img", LISTVIEW_FOCUS_IMG);
		GUI_SetProperty(LIST_GROUP, "select", &s_cur_group_sel);
		s_iptv_obj = IPTV_OBJ_GROUP;
	}
	else if(obj == IPTV_OBJ_PAGE)
	{
		if(s_page_prog_num > 0) //focus prog item
		{
            GUI_SetFocusWidget(LIST_GROUP);
			GUI_SetProperty(LIST_GROUP, "focus_img", "");
			GUI_SetProperty(LIST_GROUP, "select", &s_cur_group_sel);
			s_page_prog_sel = 0;
			_iptv_focus_item(s_page_prog_sel);
			s_iptv_obj = IPTV_OBJ_PAGE;
		}
	}
    else if(obj == IPTV_OBJ_SOURCE)
    {
		_iptv_unfocus_item(s_page_prog_sel);
		GUI_SetProperty(LIST_GROUP, "focus_img", "");
        GUI_SetFocusWidget(CMB_LIST_SOURCE);
		s_iptv_obj = IPTV_OBJ_SOURCE;
    }

    app_iptv_info_tip_update();

	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int app_iptv_create(GuiWidget *widget, void *usrdata)
{
	pop_msg_on = false;
	first_set_ltv = true;
	_iptv_load_gif();
	GUI_SetProperty(LIST_GROUP, "focus_img", LISTVIEW_FOCUS_IMG);
    GUI_SetProperty(CMB_LIST_SOURCE, "select", (int*)&s_iptv_list_source);
    if(IPTV_LIST_REMOTE == s_iptv_list_source)
    {
        app_iptv_show_popup_msg("Updating...", 0);
    }

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_destroy(GuiWidget *widget, void *usrdata)
{
	app_iptv_hide_gif();
	app_iptv_hide_popup_msg();
	_iptv_free_gif();
	return EVENT_TRANSFER_STOP;
}

static void _iptv_up_keypress(void)
{
	int val = MAX_PAGE_ITEM / 2;

	if((s_page_prog_sel >= val) && (s_page_prog_sel < MAX_PAGE_ITEM))
	{
		_iptv_unfocus_item(s_page_prog_sel);
		s_page_prog_sel -= val;
		_iptv_focus_item(s_page_prog_sel);
	}
    else if(s_page_prog_sel < val)
    {
        if(s_cur_page_num > 0)
        {
            s_cur_page_num--;
            app_iptv_update_page(s_cur_page_num);
        }
    }
}

static void _iptv_down_keypress(void)
{
	int val = MAX_PAGE_ITEM / 2;
	int sel = s_page_prog_sel + val;
	if(sel < s_page_prog_num)
	{
		_iptv_unfocus_item(s_page_prog_sel);
		s_page_prog_sel = sel;
		_iptv_focus_item(s_page_prog_sel);
	}
    else
    {
        if(s_cur_page_num < s_page_total - 1)
        {
            s_cur_page_num++;
            app_iptv_update_page(s_cur_page_num);
        }
    }
}

static void _iptv_left_keypress(void)
{
    if((s_page_prog_sel == 0) || (s_page_prog_sel == MAX_PAGE_ITEM / 2))
    {
        app_iptv_set_obj(IPTV_OBJ_GROUP);
    }
    else
    {
        if(s_page_prog_sel > 0)
        {
            _iptv_unfocus_item(s_page_prog_sel);
            s_page_prog_sel--;
            _iptv_focus_item(s_page_prog_sel);
        }
    }
}

static void _iptv_right_keypress(void)
{
	if(s_page_prog_sel == s_page_prog_num - 1)
	{
		if(s_cur_page_num < s_page_total - 1)
		{
			_iptv_unfocus_item(s_page_prog_sel);
			s_page_prog_sel = 0;
			_iptv_focus_item(s_page_prog_sel);

			s_cur_page_num++;
			app_iptv_update_page(s_cur_page_num);
		}
	}
	else if(s_page_prog_sel >= 0)
	{
		_iptv_unfocus_item(s_page_prog_sel);
		s_page_prog_sel++;
		_iptv_focus_item(s_page_prog_sel);
	}
}

static void _iptv_pageup_keypress(void)
{
	if(s_cur_page_num > 0)
	{
		s_cur_page_num--;
		app_iptv_update_page(s_cur_page_num);
	}
	else if(s_cur_page_num == 0)
	{
		if(s_page_prog_sel > 0)
		{
			_iptv_unfocus_item(s_page_prog_sel);
			s_page_prog_sel = 0;
			_iptv_focus_item(s_page_prog_sel);
		}
    }
}

static void _iptv_pagedown_keypress(void)
{
	if(s_cur_page_num < s_page_total - 1)
	{
		s_cur_page_num++;
		app_iptv_update_page(s_cur_page_num);
	}
	else if(s_cur_page_num == s_page_total - 1)
	{
		if(s_page_prog_sel < s_page_prog_num - 1)
		{
			_iptv_unfocus_item(s_page_prog_sel);
			s_page_prog_sel = s_page_prog_num - 1;
			_iptv_focus_item(s_page_prog_sel);
		}
	}
}

SIGNAL_HANDLER int app_iptv_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;

            case STBK_EXIT:
                if(pop_msg_on)
                {
                    app_iptv_hide_popup_msg();
                }
                else
                {
                    app_iptv_exit();
                }
                break;

            case STBK_MENU:
                if(pop_msg_on)
                {
                    app_iptv_hide_popup_msg();
                }
                else
                {
                    if(s_iptv_obj == IPTV_OBJ_PAGE)
                    {
                        s_cur_page_num = 0;
                        app_iptv_update_page(s_cur_page_num);
                        app_iptv_set_obj(IPTV_OBJ_GROUP);
                    }
                    else if(s_iptv_obj == IPTV_OBJ_GROUP)
                    {
                        app_iptv_set_obj(IPTV_OBJ_SOURCE);
                    }
                    else
                    {
                        app_iptv_exit();
                    }
                }
                break;

            case STBK_OK:
                if(!pop_msg_on)
                {
                    app_iptv_ok_press();
                }
                break;

            case STBK_RED:
                if(!pop_msg_on)
                {
                    app_iptv_red_key_func();
                }
                break;

            case STBK_GREEN:
                if(!pop_msg_on)
                {
                    app_iptv_green_key_func();
                }
                break;

            case STBK_BLUE:
                app_iptv_blue_key_func();
                break;

            case STBK_UP:
                if(!pop_msg_on)
                {
                    _iptv_up_keypress();
                }
                break;

            case STBK_DOWN:
                if(!pop_msg_on)
                {
                    _iptv_down_keypress();
                }
                break;

            case STBK_LEFT:
                if(!pop_msg_on)
                {
                    _iptv_left_keypress();
                }
                break;

            case STBK_RIGHT:
                if(!pop_msg_on)
                {
                    _iptv_right_keypress();
                }
                break;

            case STBK_PAGE_UP:
                if(!pop_msg_on)
                {
                    _iptv_pageup_keypress();
                }
                break;

            case STBK_PAGE_DOWN:
                if(!pop_msg_on)
                {
                    _iptv_pagedown_keypress();
                }
                break;

            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

static int _group_list_keypress_proc(int key_val)
{
    int ret = EVENT_TRANSFER_KEEPON;
    int sel = 0;

    switch(key_val)
    {
        case STBK_RIGHT:
            app_iptv_set_obj(IPTV_OBJ_PAGE);
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_OK:
            GUI_GetProperty(LIST_GROUP, "select", &sel);
            if(s_cur_group_sel != sel)
            {
                s_cur_group_sel = sel;
                GUI_SetProperty(LIST_GROUP, "active", &s_cur_group_sel);
                s_cur_page_num = 0;
                app_iptv_update_group_channellist();
                app_iptv_update_page(s_cur_page_num);
            }
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_UP:
            GUI_GetProperty(LIST_GROUP, "select", &sel);
            if((sel == 0) || (sel == -1))
            {
                app_iptv_set_obj(IPTV_OBJ_SOURCE);
                ret = EVENT_TRANSFER_STOP;
            }
            break;

        case STBK_DOWN:
            GUI_GetProperty(LIST_GROUP, "select", &sel);
            if((sel >= s_iptv_group_total - 1) || (sel == -1))
            {
                app_iptv_set_obj(IPTV_OBJ_SOURCE);
                ret = EVENT_TRANSFER_STOP;
            }

            break;

        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_iptv_group_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN == event->type)
	{
		if((pop_msg_on) || (s_iptv_obj == IPTV_OBJ_PAGE))
		{
			GUI_SendEvent(WND_IPTV, event); // proc by app_iptv_keypress
			ret = EVENT_TRANSFER_STOP;
		}
		else
		{
			int key_val = find_virtualkey_ex(event->key.scancode, event->key.sym);
			ret =  _group_list_keypress_proc(key_val);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_iptv_group_list_get_total(GuiWidget *widget, void *usrdata)
{
	return s_iptv_group_total;
}

SIGNAL_HANDLER int app_iptv_group_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return EVENT_TRANSFER_STOP;
	if(0 > item->sel)
		return EVENT_TRANSFER_STOP;
	if(s_iptv_group_title == NULL)
		return EVENT_TRANSFER_STOP;

	//col-0: channel name
	item->image = NULL;
	item->string = s_iptv_group_title[item->sel];

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_group_list_change(GuiWidget *widget, void *usrdata)
{
	int sel = -1;

	if(0)//(first_set_ltv)
	{
		sel = source_group_sel;
		GUI_SetProperty(LIST_GROUP, "select", &sel);
		first_set_ltv = false;
	}
	GUI_GetProperty(LIST_GROUP, "select", &sel);
	//app_iptv_group_change_cb(sel);

	return EVENT_TRANSFER_STOP;
}

static status_t app_iptv_set_list_source(IptvListSource list_source)
{
    if(list_source != s_iptv_list_source)
    {
        s_iptv_list_source = list_source;
        iptv_list_destroy();
        app_iptv_clean_menu_info();
        app_iptv_update_page(s_cur_page_num);
        app_iptv_info_tip_update();
        app_iptv_show_gif();
        app_iptv_get_list(s_iptv_list_source);
    }

    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int app_iptv_list_source_cmb_change(GuiWidget *widget, void *usrdata)
{
    int sel = 0;
    int ret = EVENT_TRANSFER_STOP;
    GUI_GetProperty(CMB_LIST_SOURCE, "select", &sel);
    app_iptv_set_list_source(sel);
    return ret;
}

SIGNAL_HANDLER int app_iptv_list_source_cmb_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int ret = EVENT_TRANSFER_KEEPON;
    int sel = 0;

    event = (GUI_Event *)usrdata;
    if(pop_msg_on)
    {
        GUI_SendEvent(WND_IPTV, event); // proc by app_iptv_keypress
        return EVENT_TRANSFER_STOP;
    }

    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case STBK_UP:
                if(s_iptv_group_total > 0)
                {
                    app_iptv_set_obj(IPTV_OBJ_GROUP);
                    sel = s_iptv_group_total - 1;
                    GUI_SetProperty(LIST_GROUP, "select", &sel);
                }
                ret = EVENT_TRANSFER_STOP;
                break;

            case STBK_DOWN:
                if(s_iptv_group_total > 0)
                {
                    app_iptv_set_obj(IPTV_OBJ_GROUP);
                    GUI_SetProperty(LIST_GROUP, "select", &sel);
                    ret = EVENT_TRANSFER_STOP;
                }
                break;

            default:
                break;
        }
    }

    return ret;
}

#if 0
static status_t _iptv_prog_cmp(IptvProg *src_prog, IptvProg *dest_prog)
{
	if((src_prog == NULL) || (src_prog->prog_name == NULL) || (src_prog->prog_url == NULL))
		return GXCORE_ERROR;

	if((dest_prog == NULL) || (dest_prog->prog_name == NULL) || (dest_prog->prog_url == NULL))
		return GXCORE_ERROR;

	if(strcmp(src_prog->prog_name, dest_prog->prog_name) == 0)
	{
		if(strcmp(src_prog->prog_url, dest_prog->prog_url) == 0)
			return GXCORE_SUCCESS;
	}

	return GXCORE_ERROR;
}
#endif

static status_t app_iptv_update_group(void)
{
    int i = 0;
    int group_num = 0;

    if(s_iptv_group_title != NULL)
    {
        GxCore_Free(s_iptv_group_title);
        s_iptv_group_title = NULL;
    }

    if(iptv_grouplist && iptv_grouplist->iptv_item_num > 0)
    {
        group_num = 1 + iptv_grouplist->iptv_item_num;
        s_iptv_group_title = (char**)GxCore_Calloc(group_num, sizeof(char*));
        if(s_iptv_group_title == NULL)
        {
            return GXCORE_ERROR;
        }

        s_iptv_group_title[0] = STR_ID_ALL;
        for(i=0; i<iptv_grouplist->iptv_item_num; i++)
        {
            s_iptv_group_title[i+1] = iptv_grouplist->iptv_item_list[i].iptv_key;
        }
    }
    else
    {
        group_num = 1;
        s_iptv_group_title = (char**)GxCore_Calloc(group_num, sizeof(char*));
        if(s_iptv_group_title == NULL)
        {
            return GXCORE_ERROR;
        }

        s_iptv_group_title[0] = STR_ID_ALL;
    }

    s_iptv_group_total = group_num;
    GUI_SetProperty(LIST_GROUP, "update_all", NULL);
    if(s_iptv_group_total > 0)
    {
        GUI_SetProperty(LIST_GROUP, "active", &s_cur_group_sel);
        if(s_iptv_obj == IPTV_OBJ_GROUP)
        {
            GUI_SetProperty(LIST_GROUP, "select", &s_cur_group_sel);
        }
    }

    return GXCORE_SUCCESS;
}

static status_t app_iptv_update_page(int page_num)
{
	int i = 0;
	int start_index = 0;

	if(page_num < 0)
        return GXCORE_ERROR;

	s_page_prog_num = 0;
	start_index = page_num * MAX_PAGE_ITEM;
	memset(s_page_prog_item, 0, sizeof(IptvProg) * MAX_PAGE_ITEM);
	if(iptv_channellist&&(iptv_channellist->iptv_item_num > 0) && (page_num < s_page_total))
	{
		i = 0;
		if(s_cur_group_sel == 0) //all
		{
			while((i<MAX_PAGE_ITEM)&&((start_index+i)<iptv_channellist->iptv_item_num))
			{
				s_page_prog_item[i].prog_name = iptv_channellist->iptv_item_list[start_index+i].iptv_key;
				s_page_prog_item[i].prog_url = iptv_channellist->iptv_item_list[start_index+i].iptv_value;
				s_page_prog_item[i].prog_pic = IPTV_VIDEO_PIC;
				s_page_prog_item[i].prog_ad = iptv_channellist->iptv_item_list[start_index+i].iptv_ad;
				i++;
			}
			s_page_prog_num = i;
		}
		else if((iptv_sub_channellist)&&(iptv_sub_channellist->iptv_item_num>0))
		{
			while((i<MAX_PAGE_ITEM)&&((start_index+i)<iptv_sub_channellist->iptv_item_num))
			{
				s_page_prog_item[i].prog_name = iptv_sub_channellist->iptv_item_list[start_index+i].iptv_key;
				s_page_prog_item[i].prog_url = iptv_sub_channellist->iptv_item_list[start_index+i].iptv_value;
				s_page_prog_item[i].prog_pic = IPTV_VIDEO_PIC;
				s_page_prog_item[i].prog_ad = iptv_sub_channellist->iptv_item_list[start_index+i].iptv_ad;
				i++;
			}
			s_page_prog_num = i;
		}
	}

	app_iptv_page_info_update();
	app_iptv_page_item_update(s_page_prog_num, s_page_prog_item);

	return GXCORE_SUCCESS;
}

static void app_iptv_clean_menu_info(void)
{
	if(s_iptv_group_title != NULL)
	{
		GxCore_Free(s_iptv_group_title);
		s_iptv_group_title = NULL;
	}
	memset(s_page_prog_item, 0, sizeof(IptvProg) * MAX_PAGE_ITEM);
	s_iptv_group_total = 0;
	s_cur_group_sel = 0;
	s_page_total = 0;
	s_cur_page_num = 0;
	s_page_prog_num = 0;
	s_page_prog_sel = 0;
}

static void app_iptv_exit(void)
{
    GUI_EndDialog(WND_IPTV);
    if(s_file_path_bak != NULL)
    {
        GxCore_Free(s_file_path_bak);
        s_file_path_bak = NULL;
    }

    iptv_list_destroy();
    app_iptv_clean_menu_info();
    GxBus_ConfigSetInt(IPTV_LIST_SOURCE_KEY, s_iptv_list_source);
}

#if 0
static void app_iptv_get_url(void)
{
	return;
}

static void app_iptv_play_restart_ctrl(uint64_t cur_time)
{
	//iptv_error_source_replay();
}
#endif

static status_t app_iptv_try_play(char *prog_name, char *prog_url, IptvAdClass *ad)
{
	IPTVPlayOps play_ops;

    if(prog_url == NULL)
        return GXCORE_SUCCESS;

	memset(&play_ops, 0, sizeof(IPTVPlayOps));

    play_ops.prog_name = prog_name;
	play_ops.prog_url = prog_url;
    if((ad != NULL) && (ad->ad_value != NULL) && (ad->ad_type != NULL))
    {
        play_ops.pre_media.pre_media_name = ad->ad_name;
        play_ops.pre_media.pre_media_url = ad->ad_value;
        if(strcmp(ad->ad_type, "av") == 0)
        {
            play_ops.pre_media.pre_media_type = IPTV_PRE_AV;
        }
        else if(strcmp(ad->ad_type, "picture") == 0)
        {
            play_ops.pre_media.pre_media_type = IPTV_PRE_PIC;
        }
        else
        {
            play_ops.pre_media.pre_media_type = IPTV_PRE_NONE;
        }
    }
#if 0
	play_ops.play_ctrl.play_ok = app_iptv_play_ok_ctrl;
	play_ops.play_ctrl.play_exit = app_iptv_play_exit_ctrl;
	play_ops.play_ctrl.play_prev = app_iptv_play_prev_ctrl;
	play_ops.play_ctrl.play_next = app_iptv_play_next_ctrl;
	//play_ops.play_ctrl.play_restart = app_iptv_play_restart_ctrl;
#endif
    return app_iptv_play(&play_ops);
}

static void app_iptv_ok_press(void)
{
    app_iptv_hide_gif();
    if(s_iptv_obj == IPTV_OBJ_PAGE)
    {
#if 1
        if(app_iptv_try_play(s_page_prog_item[s_page_prog_sel].prog_name,
                    s_page_prog_item[s_page_prog_sel].prog_url, &s_page_prog_item[s_page_prog_sel].prog_ad) == GXCORE_SUCCESS)
        {
            s_play_group_sel = s_cur_group_sel;
            s_play_prog_sel = s_cur_page_num * MAX_PAGE_ITEM + s_page_prog_sel;
#if 0
            if(s_play_group_sel == 0)
            {
                group_node = s_iptv_data.group_list;
                while((i < s_cur_group_sel) && (group_node != NULL))
                {
                    prog_cnt += group_node->iptv_group.prog_num;
                    group_node = group_node->next;
                    i++;
                }

                s_play_prog_sel = prog_cnt + play_sel;
            }
            else
            {
                s_play_prog_sel = s_cur_page_num * MAX_PAGE_ITEM + s_page_prog_sel;
            }
#endif
        }
#endif
    }
}

//delete function
static status_t app_iptv_update_local_list(void)
{
#define UPDATE_FILE_SUFFFIX    "xml;XML"
    int str_len = 0;
    WndStatus get_path_ret = WND_CANCLE;

    if(check_usb_status() == false)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_INSERT_USB;
        pop.mode = POP_MODE_UNBLOCK;
        popdlg_create(&pop);
    }
    else
    {
        FileListParam file_para;
        memset(&file_para, 0, sizeof(file_para));

        file_para.cur_path = s_file_path_bak;
        file_para.dest_path = &s_file_path;
        file_para.suffix = UPDATE_FILE_SUFFFIX;
        file_para.dest_mode = DEST_MODE_FILE;

        get_path_ret = app_get_file_path_dlg(&file_para);

        if(get_path_ret == WND_OK)
        {
            if(s_file_path != NULL)
            {
                ////backup the path...////
                if(s_file_path_bak != NULL)
                {
                    GxCore_Free(s_file_path_bak);
                    s_file_path_bak = NULL;
                }

                str_len = strlen(s_file_path);
                s_file_path_bak = (char *)GxCore_Malloc(str_len + 1);
                if(s_file_path_bak != NULL)
                {
                    memcpy(s_file_path_bak, s_file_path, str_len);
                    s_file_path_bak[str_len] = '\0';
                }

                app_iptv_show_gif();

                app_send_msg_exec(APPMSG_IPTV_UPDATE_LOCAL_LIST, &s_file_path);

                if(IPTV_OBJ_PAGE == s_iptv_obj)
                    app_iptv_set_obj(IPTV_OBJ_GROUP);

                iptv_list_destroy();
                app_iptv_clean_menu_info();
                app_iptv_update_page(s_cur_page_num);
                app_iptv_get_list(s_iptv_list_source);
            }
        }
    }

    return GXCORE_SUCCESS;
}

static void app_iptv_red_key_func(void)
{
}

static void app_iptv_green_key_func(void)
{
}

static void app_iptv_blue_key_func(void)
{
    app_iptv_hide_popup_msg();
    switch(s_iptv_list_source)
    {
        case IPTV_LIST_REMOTE:
            app_iptv_show_gif();

            if(IPTV_OBJ_PAGE == s_iptv_obj)
                app_iptv_set_obj(IPTV_OBJ_GROUP);

            iptv_list_destroy();
            app_iptv_clean_menu_info();
            app_iptv_update_page(s_cur_page_num);

            app_iptv_get_list(s_iptv_list_source);
            app_iptv_show_popup_msg("Updating...", 0);
            break;

        case IPTV_LIST_LOCAL:
            app_iptv_update_local_list();
            break;

        default:
            break;
    }
}

int app_iptv_browser_msg_proc(GxMessage *msg)
{
    if(msg == NULL)
        return EVENT_TRANSFER_STOP;

    switch(msg->msg_id)
    {
        case APPMSG_IPTV_LIST_DONE:
            {
                AppMsg_IptvList *iptv_list = GxBus_GetMsgPropertyPtr(msg, AppMsg_IptvList);
                app_iptv_hide_gif();
                if(iptv_list->iptv_list != NULL)
                {
                    app_iptv_hide_popup_msg();
                    if((s_iptv_list_source == iptv_list->iptv_source) && (strcmp(GUI_GetFocusWindow(), WND_IPTV) == 0))
                    {
                        if(iptv_list->iptv_list != NULL)
                        {
                            app_iptv_list_init(iptv_list->iptv_list);
                            app_iptv_update_group();
                            app_iptv_update_group_channellist();
                            app_iptv_update_page(s_cur_page_num);
                            if((s_first == true) && (s_iptv_group_total > 0))
                                app_iptv_set_obj(IPTV_OBJ_GROUP);
                        }
                    }
                    else
                    {
                        if(iptv_list->iptv_list != NULL)
                            iptv_list_free(iptv_list->iptv_list);
                    }
                }
                else
                {
                    if(s_iptv_list_source == IPTV_LIST_REMOTE)
                        app_iptv_show_popup_msg(STR_ID_SERVER_FAIL, 1000);

                    if(s_iptv_obj != IPTV_OBJ_SOURCE)
                        app_iptv_set_obj(IPTV_OBJ_SOURCE);
                }
                s_first = false;
            }
            break;

        case APPMSG_IPTV_AD_PIC_DONE:
            {
                char **pic_path = GxBus_GetMsgPropertyPtr(msg, char*);
                if(*pic_path != NULL)
                    GxCore_FileDelete(*pic_path);
            }
            break;

        case GXMSG_NETWORK_STATE_REPORT:
            {
                GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
                if((iptv_channellist == NULL)
                        &&(s_iptv_list_source == IPTV_LIST_REMOTE)
                        &&(dev_state->dev_state == IF_STATE_CONNECTED))
                {
                    app_iptv_hide_popup_msg();
                    app_iptv_show_gif();
                    app_iptv_get_list(s_iptv_list_source);
                }
            }
            break;

        default:
            break;
    }
    return EVENT_TRANSFER_STOP;
}

static status_t app_iptv_get_list(IptvListSource list_source)
{
    AppMsg_IptvList iptv_list;

    memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
    iptv_list.iptv_source = list_source;
    return app_send_msg_exec(APPMSG_IPTV_GET_LIST, &iptv_list);
}

status_t app_create_iptv_menu(void)
{
    app_iptv_service_start();
    app_iptv_clean_menu_info();

    GxBus_ConfigGetInt(IPTV_LIST_SOURCE_KEY, (int*)&s_iptv_list_source, IPTV_LIST_REMOTE);
    source_group_sel = s_cur_group_sel;

    if(GUI_CheckDialog(WND_IPTV) == GXCORE_ERROR)
    {
        GUI_CreateDialog(WND_IPTV);
    }

    s_first = true;
    app_iptv_set_obj(IPTV_OBJ_SOURCE);
    GUI_SetInterface("flush", NULL);

    app_iptv_show_gif();
    app_iptv_get_list(s_iptv_list_source);

    return GXCORE_SUCCESS;
}


#if 0
static void app_iptv_play_ok_ctrl(void)
{
#if 0
    //create prog list
    IPTVListCb play_list_cb;

    memset(&play_list_cb, 0, sizeof(IPTVListCb));

    play_list_cb.group_change = app_iptv_list_group_change;
    play_list_cb.prog_change = app_iptv_list_prog_change;
    play_list_cb.list_ok = app_iptv_list_ok_cb;
    play_list_cb.list_exit = app_iptv_list_exit_cb;
    app_iptv_list_create(&play_list_cb);
#endif
}

static void app_iptv_play_exit_ctrl(void)
{
    int group_sel = 0;
    int prog_sel = 0;

    group_sel = s_play_group_sel;
    prog_sel = s_play_prog_sel;

    if(s_cur_group_sel != group_sel)
    {
        s_cur_group_sel = group_sel;
    }

    s_cur_page_num = prog_sel / MAX_PAGE_ITEM;
    s_page_prog_sel = prog_sel % MAX_PAGE_ITEM;
    app_iptv_update_page(s_cur_page_num);
}

static void app_iptv_play_prev_ctrl(void)
{
}

static void app_iptv_play_next_ctrl(void)
{
}

static void app_iptv_list_ok_cb(int group, int prog)
{
}

static void app_iptv_list_exit_cb(void)
{
    if(s_play_group_title != NULL)
    {
        GxCore_Free(s_play_group_title);
        s_play_group_title = NULL;
    }

    if(s_play_prog_title != NULL)
    {
        GxCore_Free(s_play_prog_title);
        s_play_prog_title = NULL;
    }
}

void iptv_data_log(void)
{
}

static status_t iptv_error_source_replay(void)
{
	return GXCORE_SUCCESS;
}
#endif
#else
SIGNAL_HANDLER int app_iptv_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_group_list_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_group_list_get_total(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_group_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_group_list_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_list_source_cmb_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_list_source_cmb_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#endif
