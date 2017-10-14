#ifndef __APP_NETVIDEO_H__
#define __APP_NETVIDEO_H__

#define NETVIDEO_MAX_PAGE_ITEM (4)
#ifdef ECOS_OS
#define NETVIDEO_IMAGE_PATH_PREFIX "/mnt/youtube_"
#else
#define NETVIDEO_IMAGE_PATH_PREFIX "/tmp/youtube/nv_"
#endif
#define NETVIDEO_IMAGE_PATH_SUFFIX ".jpg"
#define NETVIDEO_IMAGE_PATH_LEN 256

typedef enum
{
	OBJ_VIDEO_GROUP = 0,
	OBJ_VIDEO_PAGE,
	OBJ_VIDEO_TOTAL,
}NetVideoObj;

typedef struct
{
	char *video_duration;
	char *video_title;
	char *video_author;
	char *video_viewcnt;
	char *video_pic;
}NetVideoItem;

typedef struct
{
	int (*video_busy)(void);
	void (*video_page_change)(int page_num);
	void (*video_exit)(void);
	void (*video_sel_change)(int video_sel);
	void (*video_group_change)(int group_sel);
	void (*obj_change)(NetVideoObj obj);
	void (*ok_press)(void);
}NetVideoMenuCb;

void app_netvideo_time_timer_stop(void);
void app_netvideo_time_timer_reset(void);
status_t app_net_video_create(char *menu_title, char *menu_logo, NetVideoMenuCb *video_cb, int group_sel);
status_t app_net_video_group_update(int group_total, char **group_title);
status_t app_net_video_page_item_update(int video_num, NetVideoItem *video_item);
status_t app_net_video_page_info_update(int page_total, int cur_page_num);
status_t app_net_video_set_obj(NetVideoObj obj);
status_t app_net_video_enable_menu_key(char *tip, void (*func)(void));
status_t app_net_video_disable_menu_key(void);
status_t app_net_video_enable_red_key(char *tip, void (*func)(void));
status_t app_net_video_disable_red_key(void);
void netvideo_pic_download_stop(void);
void netvideo_pic_download_start(void);
void app_net_video_show_gif(void);
void app_net_video_hide_gif(void);
void app_net_video_show_popup_msg(char* pMsg, int time_out);
status_t app_net_video_page_pic_updata(int index, char *pic_url);
void app_net_video_hide_popup_msg(void);
void app_net_video_clear_all_item(void);
int app_net_video_draw_gif(void* usrdata);

#endif
