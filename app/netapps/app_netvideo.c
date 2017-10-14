#include "app.h"
#include "app_module.h"
#if NETAPPS_SUPPORT
#include "app_netvideo.h"
#include <gx_apps.h>

#ifdef ECOS_OS
#define GIF_PATH                 "/dvb/theme/image/netapps/loading.gif"
#define YOUTUBE_FAIL_VIDEO_PIC   "/dvb/theme/image/netapps/fail.jpg"
#define YOUTUBE_VIDEO_PIC        "/dvb/theme/image/netapps/default.png"
#else
#define GIF_PATH WORK_PATH       "theme/image/netapps/loading.gif"
#define YOUTUBE_FAIL_VIDEO_PIC   WORK_PATH"/theme/image/netapps/fail.jpg"
#define YOUTUBE_VIDEO_PIC        WORK_PATH"/theme/image/netapps/default.png"
#endif

#define WND_APPS_NETVIDEO    "wnd_apps_netvideo"
#define TEXT_TITLE           "text_netvideo_title"
#define IMG_LOGO             "img_netvideo_logo"
#define IMG_PAGE_UP          "img_netvideo_page_up"
#define IMG_PAGE_DOWN        "img_netvideo_page_down"
#define LIST_GROUP           "list_netvideo_group"
#define TEXT_PAGE_NUM        "text_netvideo_page"
#define IMG_GIF              "img_netvideo_gif"
#define TXT_POPUP            "txt_netvideo_popup"
#define IMG_POPUP            "img_netvideo_popup"
#define IMG_MENU_KEY         "img_netvideo_menu"
#define TEXT_MENU_KEY        "text_netvideo_menu"
#define IMG_RED_KEY          "img_netvideo_red"
#define TEXT_RED_KEY         "text_netvideo_red"
#define TXT_DATE             "text_netvideo_date" 
#define TXT_TIME             "text_netvideo_time" 

#define MAX_FUNC_TIP_LEN    (32)

typedef struct
{
    char tip[MAX_FUNC_TIP_LEN];
    void (*func)(void);
}FuncKey;

static unsigned int download_pic_handle[NETVIDEO_MAX_PAGE_ITEM];
static int pic_download_abort = 0;
static int pic_download_ok[NETVIDEO_MAX_PAGE_ITEM];
static int pic_show_ok[NETVIDEO_MAX_PAGE_ITEM];
static NetVideoItem *s_page_video_item = NULL;;

static char *s_text_video_duration[NETVIDEO_MAX_PAGE_ITEM] =
{
    "text_netvideo_item_duration1",
    "text_netvideo_item_duration2",
    "text_netvideo_item_duration3",
    "text_netvideo_item_duration4",
};

static char *s_text_video_title[NETVIDEO_MAX_PAGE_ITEM] =
{
    "text_netvideo_item_title1",
    "text_netvideo_item_title2",
    "text_netvideo_item_title3",
    "text_netvideo_item_title4",
};

static char *s_text_video_author[NETVIDEO_MAX_PAGE_ITEM] =
{
    "text_netvideo_item_author1",
    "text_netvideo_item_author2",
    "text_netvideo_item_author3",
    "text_netvideo_item_author4",
};

static char *s_text_video_viewcount[NETVIDEO_MAX_PAGE_ITEM] =
{
    "text_netvideo_viewcount1",
    "text_netvideo_viewcount2",
    "text_netvideo_viewcount3",
    "text_netvideo_viewcount4",
};

static char *s_img_video_focus[NETVIDEO_MAX_PAGE_ITEM] =
{
    "img_netvideo_item_focus1",
    "img_netvideo_item_focus2",
    "img_netvideo_item_focus3",
    "img_netvideo_item_focus4",
};

static char *s_img_video_unfocus[NETVIDEO_MAX_PAGE_ITEM] =
{
    "img_netvideo_item_unfocus1",
    "img_netvideo_item_unfocus2",
    "img_netvideo_item_unfocus3",
    "img_netvideo_item_unfocus4",
};

static struct
{
    int group_total;
    char **group_title;
    int group_sel;

    int page_total;
    int page_num;

    int page_video_num;
    NetVideoItem *page_video_item;
    int page_video_sel;
	
    FuncKey red_key;
    FuncKey menu_key;

    NetVideoObj video_obj;
    NetVideoMenuCb video_cb;
}s_video_snapshot;

static event_list* s_netvideo_gif_timer = NULL;
static event_list* s_netvideo_popup_timer = NULL;
static event_list* s_netvideo_picupdate_timer = NULL;
static event_list* s_netvideo_time_timer = NULL;
static int source_group_sel = 0;
static bool	first_set_ltv = false;
static bool pop_msg_on = false;

int app_net_video_draw_gif(void* usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
	return 0;
}

static void _net_video_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
	GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _net_video_free_gif(void)
{
	GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

void app_net_video_show_gif(void)
{
	GUI_SetProperty(IMG_GIF, "state", "show");
	if(0 != reset_timer(s_netvideo_gif_timer))
	{
		s_netvideo_gif_timer = create_timer(app_net_video_draw_gif, 100, NULL, TIMER_REPEAT);
	}
}

void app_net_video_hide_gif(void)
{
	remove_timer(s_netvideo_gif_timer);
	s_netvideo_gif_timer = NULL;
	GUI_SetProperty(IMG_GIF, "state", "hide");
}

void app_net_video_hide_popup_msg(void)
{
	if(s_netvideo_popup_timer)
	{
		remove_timer(s_netvideo_popup_timer);
		s_netvideo_popup_timer = NULL;
	}

	pop_msg_on = false;
	GUI_SetProperty(TXT_POPUP, "state", "hide");
	GUI_SetProperty(IMG_POPUP, "state", "hide");
}

static int _net_video_popup_timer_timeout(void *userdata)
{
	app_net_video_hide_popup_msg();
	return 0;
}

void app_net_video_show_popup_msg(char* pMsg, int time_out)
{
	GUI_SetInterface("flush", NULL);
	pop_msg_on = true;
	GUI_SetProperty(TXT_POPUP, "string", pMsg);
	GUI_SetProperty(IMG_POPUP, "state", "show");
	GUI_SetProperty(TXT_POPUP, "state", "show");

	if(time_out > 0)
	{
		if (reset_timer(s_netvideo_popup_timer) != 0)
		{
			s_netvideo_popup_timer = create_timer(_net_video_popup_timer_timeout, time_out, NULL, TIMER_ONCE);
		}
	}
}

status_t app_net_video_group_update(int group_total, char **group_title)
{
    s_video_snapshot.group_total = group_total;
    s_video_snapshot.group_title = group_title;

    GUI_SetProperty(LIST_GROUP, "update_all", NULL);

    return GXCORE_SUCCESS;
}

static void _net_video_show_item(int index, NetVideoItem *video_item)
{
    if((index >= NETVIDEO_MAX_PAGE_ITEM) || (index < 0) || (video_item == NULL))
        return;

    //duration
    if(video_item->video_duration != NULL)
    {
        GUI_SetProperty(s_text_video_duration[index], "string", video_item->video_duration);
        GUI_SetProperty(s_text_video_duration[index], "state", "show");
    }
	
    //title
    if(video_item->video_title != NULL)
    {
        GUI_SetProperty(s_text_video_title[index], "string", video_item->video_title);
        GUI_SetProperty(s_text_video_title[index], "state", "show");
    }

    //Author
    if(video_item->video_author != NULL)
    {
        GUI_SetProperty(s_text_video_author[index], "string", video_item->video_author);
        GUI_SetProperty(s_text_video_author[index], "state", "show");
    }

    //ViewCount
    if(video_item->video_viewcnt != NULL)
    {
        GUI_SetProperty(s_text_video_viewcount[index], "string", video_item->video_viewcnt);
        GUI_SetProperty(s_text_video_viewcount[index], "state", "show");
    }

    //pic
	GUI_SetProperty(s_img_video_unfocus[index], "load_scal_img", YOUTUBE_VIDEO_PIC);
	GUI_SetProperty(s_img_video_unfocus[index], "state", "show");
}

static void _net_video_hide_item(int index)
{
    if((index >= NETVIDEO_MAX_PAGE_ITEM) || (index < 0))
        return;

    GUI_SetProperty(s_text_video_duration[index], "state", "hide");
    GUI_SetProperty(s_text_video_title[index], "state", "hide");
    GUI_SetProperty(s_text_video_author[index], "state", "hide");
    GUI_SetProperty(s_text_video_viewcount[index], "state", "hide");
    GUI_SetProperty(s_img_video_unfocus[index], "state", "hide");
}

void app_net_video_clear_all_item(void)
{
	int i = 0;

	for(i=0; i<NETVIDEO_MAX_PAGE_ITEM; i++)
	{
		GUI_SetProperty(s_text_video_duration[i], "state", "hide");
		GUI_SetProperty(s_text_video_title[i], "string", " ");
		GUI_SetProperty(s_text_video_author[i], "string", " ");
		GUI_SetProperty(s_text_video_viewcount[i], "string", " ");
		GUI_SetProperty(s_img_video_unfocus[i], "load_scal_img", YOUTUBE_VIDEO_PIC);
		GUI_SetProperty(s_img_video_unfocus[i], "state", "show");
	}
}

static void _net_video_focus_item(int index)
{
    if((index >= 0) && (index < NETVIDEO_MAX_PAGE_ITEM))
        GUI_SetProperty(s_img_video_focus[index], "state", "show");
}

static void _net_video_unfocus_item(int index)
{
    if((index >= 0) && (index < NETVIDEO_MAX_PAGE_ITEM))
        GUI_SetProperty(s_img_video_focus[index], "state", "hide");
}

status_t app_net_video_page_pic_updata(int index, char *pic_url)
{
    if((index >= NETVIDEO_MAX_PAGE_ITEM) || (index < 0))
		return GXCORE_ERROR;

    if(pic_url != NULL)
    {
        GUI_SetProperty(s_img_video_unfocus[index], "load_scal_img", pic_url);
        GUI_SetProperty(s_img_video_unfocus[index], "state", "show");
    }
    return GXCORE_SUCCESS;
}

status_t app_net_video_page_item_update(int video_num, NetVideoItem *video_item)
{
    int i = 0;
    int cur_sel = 0;

    s_video_snapshot.page_video_num = video_num;
    s_video_snapshot.page_video_item = video_item;
    s_page_video_item = video_item;

    cur_sel = s_video_snapshot.page_video_sel;
    _net_video_unfocus_item(cur_sel);
    for(i = 0; i < s_video_snapshot.page_video_num; i++)
    {
        _net_video_show_item(i, video_item + i);
    }
    for(; i < NETVIDEO_MAX_PAGE_ITEM; i++)
    {
        _net_video_hide_item(i);
    }

    if(cur_sel >= s_video_snapshot.page_video_num)
    {
        cur_sel = s_video_snapshot.page_video_num - 1;
    }

    if(s_video_snapshot.video_obj == OBJ_VIDEO_PAGE)
    {
        _net_video_focus_item(cur_sel);
    }

    if(cur_sel != s_video_snapshot.page_video_sel)
    {
        s_video_snapshot.page_video_sel = cur_sel;
        if(s_video_snapshot.video_cb.video_sel_change != NULL)
        {
            s_video_snapshot.video_cb.video_sel_change(s_video_snapshot.page_video_sel);
        }
    }

    return GXCORE_SUCCESS;
}

status_t app_net_video_page_info_update(int page_total, int page_num)
{
	char buf[64] = {0};

	s_video_snapshot.page_total = page_total;
	s_video_snapshot.page_num = page_num;

	if(s_video_snapshot.page_total > 0)
	{
		sprintf(buf, "[ %d / %d ]", s_video_snapshot.page_num + 1, s_video_snapshot.page_total);
		GUI_SetProperty(TEXT_PAGE_NUM, "string", buf);
		GUI_SetProperty(TEXT_PAGE_NUM, "state", "show");
	}
	else
	{
		GUI_SetProperty(TEXT_PAGE_NUM, "state", "hide");
	}

	if(s_video_snapshot.page_total <= 1)
	{
		GUI_SetProperty(IMG_PAGE_UP, "state", "hide");
		GUI_SetProperty(IMG_PAGE_DOWN, "state", "hide");
	}
	else
	{
		if(s_video_snapshot.page_num >= s_video_snapshot.page_total - 1) //last page
		{
			GUI_SetProperty(IMG_PAGE_UP, "state", "show");
			GUI_SetProperty(IMG_PAGE_DOWN, "state", "hide");
		}
		else if(s_video_snapshot.page_num == 0)//first page
		{
			GUI_SetProperty(IMG_PAGE_UP, "state", "hide");
			GUI_SetProperty(IMG_PAGE_DOWN, "state", "show");
		}
		else
		{
			GUI_SetProperty(IMG_PAGE_UP, "state", "show");
			GUI_SetProperty(IMG_PAGE_DOWN, "state", "show");
		}
	}

	return GXCORE_SUCCESS;
}

status_t app_net_video_create(char *menu_title, char *menu_logo, NetVideoMenuCb *video_cb, int group_sel)
{
    memset(&s_video_snapshot, 0, sizeof(s_video_snapshot));

    if(video_cb != NULL)
    {
        memcpy(&s_video_snapshot.video_cb, video_cb, sizeof(NetVideoMenuCb));
    }

	source_group_sel = group_sel;
    if(GUI_CheckDialog(WND_APPS_NETVIDEO) == GXCORE_ERROR)
    {
        GUI_CreateDialog(WND_APPS_NETVIDEO);
    }

    GUI_SetProperty(TEXT_TITLE, "string", menu_title);
    GUI_SetProperty(IMG_LOGO, "img", menu_logo);

    return GXCORE_SUCCESS;
}

status_t app_net_video_set_obj(NetVideoObj obj)
{
    int act = -1;
    if(obj >= OBJ_VIDEO_TOTAL)
        return GXCORE_ERROR;

    if(obj == s_video_snapshot.video_obj)
        return GXCORE_SUCCESS;

    if(obj == OBJ_VIDEO_GROUP)
    {
        _net_video_unfocus_item(s_video_snapshot.page_video_sel);
        GUI_SetProperty(LIST_GROUP, "active", &act);
        s_video_snapshot.video_obj = OBJ_VIDEO_GROUP;
        if(s_video_snapshot.video_cb.obj_change != NULL)
        {
            s_video_snapshot.video_cb.obj_change(s_video_snapshot.video_obj);
        }
    }
    else if(obj == OBJ_VIDEO_PAGE)
    {
        if(s_video_snapshot.page_video_num > 0) //focus video item
        {
            s_video_snapshot.page_video_sel = 0;
            s_video_snapshot.video_obj = OBJ_VIDEO_PAGE;
            _net_video_focus_item(s_video_snapshot.page_video_sel);
            GUI_SetProperty(LIST_GROUP, "active", &s_video_snapshot.group_sel);

            if(s_video_snapshot.video_cb.obj_change != NULL)
            {
                s_video_snapshot.video_cb.obj_change(s_video_snapshot.video_obj);
            }

            if(s_video_snapshot.video_cb.video_sel_change != NULL)
            {
                s_video_snapshot.video_cb.video_sel_change(s_video_snapshot.page_video_sel);
            }
        }
    }

    return GXCORE_SUCCESS;
}

status_t app_net_video_enable_menu_key(char *tip, void (*func)(void))
{
    memset(s_video_snapshot.menu_key.tip, 0, sizeof(s_video_snapshot.menu_key.tip));
    if(tip != NULL)
    {
        strncpy(s_video_snapshot.menu_key.tip, tip, MAX_FUNC_TIP_LEN);
    }
    s_video_snapshot.menu_key.func = func;

    GUI_SetProperty(IMG_MENU_KEY, "state", "show");
    GUI_SetProperty(TEXT_MENU_KEY, "state", "show");
    GUI_SetProperty(TEXT_MENU_KEY, "string", s_video_snapshot.menu_key.tip);

    return GXCORE_SUCCESS;
}

status_t app_net_video_disable_menu_key(void)
{
    memset(s_video_snapshot.menu_key.tip, 0, sizeof(s_video_snapshot.menu_key.tip));
    s_video_snapshot.menu_key.func = NULL;

    GUI_SetProperty(IMG_MENU_KEY, "string", s_video_snapshot.menu_key.tip);
    GUI_SetProperty(TEXT_MENU_KEY, "state", "hide");
    GUI_SetProperty(TEXT_MENU_KEY, "state", "hide");

    return GXCORE_SUCCESS;
}

status_t app_net_video_enable_red_key(char *tip, void (*func)(void))
{
    memset(s_video_snapshot.red_key.tip, 0, sizeof(s_video_snapshot.red_key.tip));
    if(tip != NULL)
    {
        strncpy(s_video_snapshot.red_key.tip, tip, MAX_FUNC_TIP_LEN);
    }
    s_video_snapshot.red_key.func = func;

    GUI_SetProperty(IMG_RED_KEY, "state", "show");
    GUI_SetProperty(TEXT_RED_KEY, "state", "show");
    GUI_SetProperty(TEXT_RED_KEY, "string", s_video_snapshot.red_key.tip);

    return GXCORE_SUCCESS;
}

status_t app_net_video_disable_red_key(void)
{
    memset(s_video_snapshot.red_key.tip, 0, sizeof(s_video_snapshot.red_key.tip));
    s_video_snapshot.red_key.func = NULL;

    GUI_SetProperty(IMG_RED_KEY, "string", s_video_snapshot.red_key.tip);
    GUI_SetProperty(TEXT_RED_KEY, "state", "hide");
    GUI_SetProperty(TEXT_RED_KEY, "state", "hide");

    return GXCORE_SUCCESS;
}

static int app_pic_update_timer_timeout(void *userdata)
{
	int i=0;
	bool b_all_ok = true;
	char image_path[NETVIDEO_IMAGE_PATH_LEN];
	
	if((s_video_snapshot.page_video_num<=0)||(s_video_snapshot.page_video_item == NULL))
	{
		return 0;
	}
	
	for(i = 0; i < s_video_snapshot.page_video_num; i++)
	{
		if(pic_show_ok[i] == 1)
		{
			continue;
		}
		b_all_ok = false;
		if(pic_download_ok[i] == 1)
		{
			snprintf(image_path,sizeof(image_path),"%s%d%s",NETVIDEO_IMAGE_PATH_PREFIX,i, NETVIDEO_IMAGE_PATH_SUFFIX);
			if(GXCORE_FILE_UNEXIST != GxCore_FileExists(image_path))
			{
				app_net_video_page_pic_updata(i, image_path);
			}
			else
			{
				if(!pop_msg_on)
				{
					app_net_video_page_pic_updata(i, YOUTUBE_FAIL_VIDEO_PIC);
				}
			}
			pic_show_ok[i] = 1;
		}
		else if(pic_download_ok[i] == -1)
		{
			if(!pop_msg_on)
			{
				app_net_video_page_pic_updata(i, YOUTUBE_FAIL_VIDEO_PIC);
			}
			pic_show_ok[i] = 1;
		}
	}

	if(b_all_ok)
	{
		app_net_video_hide_gif();
		GUI_SetInterface("flush",NULL);
		timer_stop(s_netvideo_picupdate_timer);
		s_netvideo_picupdate_timer = NULL;
	}

	return 0;
}

static void app_netvideo_update_timer_stop(void)
{
	if(s_netvideo_picupdate_timer)
	{
		remove_timer(s_netvideo_picupdate_timer);
		s_netvideo_picupdate_timer = NULL;
	}
}

static void app_netvideo_update_timer_reset(void)
{
	if (reset_timer(s_netvideo_picupdate_timer) != 0)
	{
		s_netvideo_picupdate_timer = create_timer(app_pic_update_timer_timeout, 300, NULL, TIMER_REPEAT);
	}
}

static void app_netvideo_pic_cb(int message, int body)
{
	gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;
	unsigned int handle = 0;
	int i = 0;
	
	if(message == GXCURL_STATE_COMPLETE)
	{
		if(pic_download_abort)
			return;
		handle = callback_data->handle;
		if(callback_data->handle>0)
		{
			for(i=0;i<NETVIDEO_MAX_PAGE_ITEM;i++)
			{
				if(download_pic_handle[i] ==callback_data->handle)
				{
					if(callback_data->complete_data.retcode == GXAPPS_SUCCESS)
					{
						pic_download_ok[i] = 1;
					}
					else
					{
						pic_download_ok[i] = -1;
					}
					break;
				}
			}
		}
	}
}

static void netvideo_delete_files(void)
{
	int i=0;
	char image_path[NETVIDEO_IMAGE_PATH_LEN];

	for(i=0; i<NETVIDEO_MAX_PAGE_ITEM; i++)
	{
		snprintf(image_path,sizeof(image_path),"%s%d%s",NETVIDEO_IMAGE_PATH_PREFIX,i, NETVIDEO_IMAGE_PATH_SUFFIX);
		if(GXCORE_FILE_EXIST == GxCore_FileExists(image_path))
		{
			GxCore_FileDelete(image_path);
		}
	}
}

void netvideo_pic_download_stop(void)
{
	int i=0;

	app_netvideo_update_timer_stop();
	memset(&pic_download_ok, 0, sizeof(pic_download_ok));
	memset(&pic_show_ok, 0, sizeof(pic_show_ok));
	pic_download_abort= 1;
	for(i=0; i<NETVIDEO_MAX_PAGE_ITEM; i++)
	{
		if(download_pic_handle[i]!=0)
		{
			gxapps_curl_async_abort(download_pic_handle[i]);
			download_pic_handle[i] = 0;
		}
	}
	netvideo_delete_files();
	pic_download_abort= 0;
}

void netvideo_pic_download_start(void)
{
	int i=0;
	char image_path[NETVIDEO_IMAGE_PATH_LEN];

	netvideo_pic_download_stop();
	app_netvideo_update_timer_reset();
	if((s_video_snapshot.page_video_num>0)&&(s_video_snapshot.page_video_item))
	{
		for(i=0; i<s_video_snapshot.page_video_num; i++)
		{
			if(pic_download_abort == 0)
			{
				if(s_video_snapshot.page_video_item[i].video_pic)
				{
					snprintf(image_path,sizeof(image_path),"%s%d%s",NETVIDEO_IMAGE_PATH_PREFIX,i, NETVIDEO_IMAGE_PATH_SUFFIX);
					download_pic_handle[i] = gxapps_curl_async_download(s_video_snapshot.page_video_item[i].video_pic, image_path, app_netvideo_pic_cb);
				}
				else
				{
					pic_download_ok[i] = -1;
				}
			}
		}
	}
}

static int app_netvideo_time_update(void* userdata)
{
	char sys_time_str[24] = {0};
	char time_str_temp[24] = {0};
	
	g_AppTime.time_get(&g_AppTime, sys_time_str, sizeof(sys_time_str)); 
	// time
	if((strlen(sys_time_str) > 24) || (strlen(sys_time_str) < 5))
	{
		printf("\nlenth error\n");
		return 1;
	}
	memcpy(time_str_temp,&sys_time_str[strlen(sys_time_str)-5],5);// 00:00
	GUI_SetProperty(TXT_TIME, "string", time_str_temp);

	// ymd
	memcpy(time_str_temp,sys_time_str,strlen(sys_time_str));// 0000/00/00
	time_str_temp[strlen(sys_time_str)-5] = '\0';
	GUI_SetProperty(TXT_DATE, "string", time_str_temp);
	return 0;
}

void app_netvideo_time_timer_stop(void)
{
	if(s_netvideo_time_timer)
	{
		remove_timer(s_netvideo_time_timer);
		s_netvideo_time_timer = NULL;
	}
}
void app_netvideo_time_timer_reset(void)
{
	app_netvideo_time_update(NULL);
	if( reset_timer(s_netvideo_time_timer) != 0 )
	{
		s_netvideo_time_timer = create_timer(app_netvideo_time_update,1000,NULL,TIMER_REPEAT);
	}
}
SIGNAL_HANDLER int app_netvideo_create(GuiWidget *widget, void *usrdata)
{
	app_netvideo_time_timer_reset();
	pop_msg_on = false;
	first_set_ltv = true;
	s_page_video_item = NULL;
	memset(download_pic_handle, 0, sizeof(download_pic_handle));
	memset(&pic_download_ok, 0, sizeof(pic_download_ok));
	memset(&pic_show_ok, 0, sizeof(pic_show_ok));
	pic_download_abort = 0;
	_net_video_load_gif();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_destroy(GuiWidget *widget, void *usrdata)
{
	app_netvideo_time_timer_stop();
	netvideo_pic_download_stop();
	memset(&s_video_snapshot, 0, sizeof(s_video_snapshot));
	app_net_video_hide_gif();
	app_net_video_hide_popup_msg();
	_net_video_free_gif();
	s_page_video_item = NULL;

    return EVENT_TRANSFER_STOP;
}

static void _net_video_up_keypress(void)
{
	if(s_video_snapshot.page_video_sel == 0)
	{
		if(s_video_snapshot.page_num > 0)
		{
			if(s_video_snapshot.video_cb.video_busy!= NULL)
			{
				if(s_video_snapshot.video_cb.video_busy())
				{
					app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);    
					return;
				}
			}
			_net_video_unfocus_item(s_video_snapshot.page_video_sel);
			s_video_snapshot.page_video_sel = NETVIDEO_MAX_PAGE_ITEM - 1;
			_net_video_focus_item(s_video_snapshot.page_video_sel);
			if(s_video_snapshot.video_cb.video_sel_change != NULL)
			{
				s_video_snapshot.video_cb.video_sel_change(s_video_snapshot.page_video_sel);
			}

			s_video_snapshot.page_num--;
			if(s_video_snapshot.video_cb.video_page_change != NULL)
			{
				s_video_snapshot.video_cb.video_page_change(s_video_snapshot.page_num);
			}
			app_net_video_page_info_update(s_video_snapshot.page_total, s_video_snapshot.page_num);
		}
	}
	else if(s_video_snapshot.page_video_sel > 0)
	{
		_net_video_unfocus_item(s_video_snapshot.page_video_sel);
		s_video_snapshot.page_video_sel--;
		_net_video_focus_item(s_video_snapshot.page_video_sel);
		if(s_video_snapshot.video_cb.video_sel_change != NULL)
		{
			s_video_snapshot.video_cb.video_sel_change(s_video_snapshot.page_video_sel);
		}
	}
}

static void _net_video_down_keypress(void)
{
	if(s_video_snapshot.page_video_sel == s_video_snapshot.page_video_num - 1)
	{
		if(s_video_snapshot.page_num < s_video_snapshot.page_total - 1)
		{
			if(s_video_snapshot.video_cb.video_busy!= NULL)
			{
				if(s_video_snapshot.video_cb.video_busy())
				{
					app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);    
					return;
				}
			}
			_net_video_unfocus_item(s_video_snapshot.page_video_sel);
			s_video_snapshot.page_video_sel = 0;
			_net_video_focus_item(s_video_snapshot.page_video_sel);
			if(s_video_snapshot.video_cb.video_sel_change != NULL)
			{
				s_video_snapshot.video_cb.video_sel_change(s_video_snapshot.page_video_sel);
			}

			s_video_snapshot.page_num++;
			if(s_video_snapshot.video_cb.video_page_change != NULL)
			{
				s_video_snapshot.video_cb.video_page_change(s_video_snapshot.page_num);
			}
			app_net_video_page_info_update(s_video_snapshot.page_total, s_video_snapshot.page_num);
		}
	}
	else if(s_video_snapshot.page_video_sel >= 0)
	{
		_net_video_unfocus_item(s_video_snapshot.page_video_sel);
		s_video_snapshot.page_video_sel++;
		_net_video_focus_item(s_video_snapshot.page_video_sel);
		if(s_video_snapshot.video_cb.video_sel_change != NULL)
		{
			s_video_snapshot.video_cb.video_sel_change(s_video_snapshot.page_video_sel);
		}
	}
}

static void _net_video_left_keypress(void)
{
	int act = -1;
	_net_video_unfocus_item(s_video_snapshot.page_video_sel);
	GUI_SetProperty(LIST_GROUP, "active", &act);
	s_video_snapshot.video_obj = OBJ_VIDEO_GROUP;
	if(s_video_snapshot.video_cb.obj_change != NULL)
	{
		s_video_snapshot.video_cb.obj_change(s_video_snapshot.video_obj);
	}
}

static void _net_video_right_keypress(void)
{

}

static void _net_video_pageup_keypress(void)
{
	if(s_video_snapshot.video_cb.video_busy!= NULL)
	{
		if(s_video_snapshot.video_cb.video_busy())
		{
			app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);    
			return;
		}
	}
	if(s_video_snapshot.page_num > 0)
	{
		s_video_snapshot.page_num--;
		if(s_video_snapshot.video_cb.video_page_change != NULL)
		{
			s_video_snapshot.video_cb.video_page_change(s_video_snapshot.page_num);
		}
		app_net_video_page_info_update(s_video_snapshot.page_total, s_video_snapshot.page_num);
	}
	else if(s_video_snapshot.page_num == 0)
	{
		if(s_video_snapshot.page_video_sel > 0)
		{
			_net_video_unfocus_item(s_video_snapshot.page_video_sel);
			s_video_snapshot.page_video_sel = 0;
			_net_video_focus_item(s_video_snapshot.page_video_sel);
			if(s_video_snapshot.video_cb.video_sel_change != NULL)
			{
				s_video_snapshot.video_cb.video_sel_change(s_video_snapshot.page_video_sel);
			}
		}
	}
}

static void _net_video_pagedown_keypress(void)
{
	if(s_video_snapshot.video_cb.video_busy!= NULL)
	{
		if(s_video_snapshot.video_cb.video_busy())
		{
			app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);    
			return;
		}
	}
	if(s_video_snapshot.page_num < s_video_snapshot.page_total - 1)
	{
		s_video_snapshot.page_num++;
		if(s_video_snapshot.video_cb.video_page_change != NULL)
		{
			s_video_snapshot.video_cb.video_page_change(s_video_snapshot.page_num);
		}
		app_net_video_page_info_update(s_video_snapshot.page_total, s_video_snapshot.page_num);
	}
	else if(s_video_snapshot.page_num == s_video_snapshot.page_total - 1)
	{
		if(s_video_snapshot.page_video_sel < s_video_snapshot.page_video_num - 1)
		{
			_net_video_unfocus_item(s_video_snapshot.page_video_sel);
			s_video_snapshot.page_video_sel = s_video_snapshot.page_video_num - 1;
			_net_video_focus_item(s_video_snapshot.page_video_sel);
			if(s_video_snapshot.video_cb.video_sel_change != NULL)
			{
				s_video_snapshot.video_cb.video_sel_change(s_video_snapshot.page_video_sel);
			}
		}
	}
}

SIGNAL_HANDLER int app_netvideo_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    void (*exit_cb)(void) = NULL;

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
					app_net_video_hide_popup_msg();
				}
				else
				{
					exit_cb = s_video_snapshot.video_cb.video_exit;
					GUI_EndDialog(WND_APPS_NETVIDEO);
					if(exit_cb != NULL)
					{
						exit_cb();
					}
				}
                break;

            case STBK_MENU:
				if(!pop_msg_on)
				{
		                  if(s_video_snapshot.menu_key.func != NULL)
		                  {
		                    s_video_snapshot.menu_key.func();
		                  }
				}
                break;

            case STBK_RED:
				if(!pop_msg_on)
				{
					if(s_video_snapshot.red_key.func != NULL)
					{
						app_netvideo_time_timer_stop();
						s_video_snapshot.red_key.func();
					}
				}
                break;
				
            case STBK_OK:
				if(!pop_msg_on)
				{
					if(s_video_snapshot.video_cb.ok_press != NULL)
					{
						app_netvideo_time_timer_stop();
						s_video_snapshot.video_cb.ok_press();
					}
				}
                break;

            case STBK_UP:
				if(!pop_msg_on)
				{
					_net_video_up_keypress();
				}
                break;

            case STBK_DOWN:
				if(!pop_msg_on)
				{
					_net_video_down_keypress();
				}
                break;

            case STBK_LEFT:
				if(!pop_msg_on)
				{
					_net_video_left_keypress();
				}
                break;

            case STBK_RIGHT:
				if(!pop_msg_on)
				{
					_net_video_right_keypress();
				}
                break;

            case STBK_PAGE_UP:
				if(!pop_msg_on)
				{
					_net_video_pageup_keypress();
				}
                break;

            case STBK_PAGE_DOWN:
				if(!pop_msg_on)
				{
					_net_video_pagedown_keypress();
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

    switch(key_val)
    {
        case STBK_RIGHT:
            app_net_video_set_obj(OBJ_VIDEO_PAGE);
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_OK:
            if(s_video_snapshot.video_cb.ok_press != NULL)
            {
                s_video_snapshot.video_cb.ok_press();
            }
            ret = EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_netvideo_group_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        if(s_video_snapshot.video_obj == OBJ_VIDEO_PAGE)
        {
            GUI_SendEvent(WND_APPS_NETVIDEO, event); // proc by app_netvideo_keypress
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

SIGNAL_HANDLER int app_netvideo_group_list_get_total(GuiWidget *widget, void *usrdata)
{
	return s_video_snapshot.group_total;
}

SIGNAL_HANDLER int app_netvideo_group_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return EVENT_TRANSFER_STOP;
	if(0 > item->sel)
		return EVENT_TRANSFER_STOP;
	if(s_video_snapshot.group_title == NULL)
		return EVENT_TRANSFER_STOP;

	//col-0: channel name
	item->image = NULL;
	item->string = s_video_snapshot.group_title[item->sel];

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_group_list_change(GuiWidget *widget, void *usrdata)
{
	int sel = -1;

	if(first_set_ltv)
	{
		sel = source_group_sel;
		GUI_SetProperty(LIST_GROUP, "select", &sel);
		first_set_ltv = false;
	}
	GUI_GetProperty(LIST_GROUP, "select", &sel);
	s_video_snapshot.group_sel = sel;
	if(s_video_snapshot.video_cb.video_group_change != NULL)
	{
		s_video_snapshot.video_cb.video_group_change(s_video_snapshot.group_sel);
	}

	return EVENT_TRANSFER_STOP;
}

#else
SIGNAL_HANDLER int app_netvideo_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_netvideo_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_netvideo_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_netvideo_group_list_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_netvideo_group_list_get_total(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_netvideo_group_list_get_data(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_netvideo_group_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
#endif
