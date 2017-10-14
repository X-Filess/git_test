#include "app.h"
#if YOUTUBE_SUPPORT
#include "app_pop.h"
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_netvideo.h"
#include "app_netvideo_play.h"
#include "app_wnd_file_list.h"
#include "app_netapps_service.h"
#include "app_netapps_common.h"
#include <gx_youtube.h>

#define YOUTUBE_LOGO    "youtubelogo.bmp"
#define YOUTUBE_SAVE_FREE(x) if(x){GxCore_Free(x);x=NULL;}
#define LIST_GROUP    "list_netvideo_group"
#define TEXT_MENU_KEY "text_netvideo_menu"
#define TEXT_STR_CATEGORY "Category"
#define TEXT_STR_CHANNEL "Channel"
#define YOUTUBE_REGIONSTR_LEN 2048
#define YOUTUBE_RESSTR_LEN 128
#define YOUTUBE_AUTHOR_LEN 64
#define YOUTUBE_VIEWCOUNT_LEN 32
#define YOUTUBE_DURATION_LEN 16

typedef void(* youtube_thread_func)(void);

typedef enum
{
	GROUP_CATEGORY = 0,
	GROUP_CHANNEL,
	GROUP_MAX
}YoutubeGroupType;

typedef enum 
{
	YOUTUBE_SETTING_REGION=0,
	YOUTUBE_SETTING_RESOLUTION,
	YOUTUBE_SETTING_MAX,
}youtube_setting_itm;

typedef struct youtube_pagepos_s
{
	int cur_page;
	int sel;
	int total_page;
}youtube_pagepos_t;

typedef struct youtube_grouppos_s
{
	int cur_group;
	int sel_group;
	int total_group;
}youtube_grouppos_t;

typedef struct youtube_channel_s
{
	const char *ui_name;
	const char *channel_id;
}youtube_channel_t;

typedef struct
{
	char region;
	char resolution;
}youtube_setting_t;

typedef struct
{
	char youtube_author[YOUTUBE_AUTHOR_LEN];
	char youtube_viewcnt[YOUTUBE_VIEWCOUNT_LEN];
	char youtube_duration[YOUTUBE_DURATION_LEN];
}YoutubeVideoItem;

typedef struct
{
	int cur_page_num;
	char *nextPageToken;
	char *prevPageToken;
}YoutubePageInfo;

static NetVideoObj s_video_obj = OBJ_VIDEO_GROUP;
static youtube_pagepos_t youtube_page_pos = {0};
static youtube_grouppos_t youtube_group_pos = {0};
static NetVideoItem s_page_video_item[NETVIDEO_MAX_PAGE_ITEM];
static YoutubeVideoItem *youtube_video_item = NULL;

static char **s_yb_group_title = NULL;;
static char *youtube_url = NULL;
static char *youtube_search_keyword = NULL;
static bool is_page_get_info = false;
static int is_youtube_download = 0;
static int youtube_data_finish = 0;
static unsigned int youtube_data_handle = 0;
static unsigned int youtube_geturl_handle = 0;

static YoutubeGroupType s_youtube_group_type = GROUP_CATEGORY;
static gx_youtube_videolist_t *youtube_videolist = NULL;
static gx_youtube_urllist_t *youtube_urllist = NULL;
static event_list* youtube_videodata_timer = NULL;
static SystemSettingOpt  *youtube_setting_opt = NULL;
static SystemSettingItem *youtube_setting_item = NULL;
static youtube_setting_t *youtube_setting = NULL;
static char *youtube_region_array = NULL;
static char *youtube_resolution_array = NULL;
static YoutubePageInfo youtube_pageinfo;
static gxapps_ret_t youtube_ret_status = GXAPPS_SUCCESS;

static char* youtube_category_str[YOUTUBE_CATEGORY_MAX]=
{
	"Trending", //YOUTUBE_CAGEGORY_TRENDING
	"Music", //YOUTUBE_CAGEGORY_MUSIC
	"Autos & Vehicles", //YOUTUBE_CAGEGORY_AUTOS
	"Comedy", //YOUTUBE_CAGEGORY_COMEDY
	"Entertainment", //YOUTUBE_CAGEGORY_ENTERTAINMENT
	"Gaming", //YOUTUBE_CAGEGORY_GAMING
	"People & Blogs", //YOUTUBE_CAGEGORY_PEOPLE
	"Pets & Animals", //YOUTUBE_CAGEGORY_PETS
	"Science & Technology", //YOUTUBE_CAGEGORY_SCIENCE
	"Sports", //YOUTUBE_CAGEGORY_SPORTS
	"Howto & Style", //YOUTUBE_CAGEGORY_HOWTO
	"Travel & Events", //YOUTUBE_CAGEGORY_TRAVEL
};
#define YOUTUBE_CATEGORY_NUM 	(sizeof(youtube_category_str)/sizeof(char *))

static youtube_channel_t youtube_channel_array[]=
{
	{"Vevo", "UC2pmfLm7iq6Ov1UwYrWYkZA"},
	{"BBC", "UCCj956IF62FbT7Gouszaj9w"},
	{"BBC News", "UC16niRr50-MSBwiO3YDb3RA"},
	{"BBCWorldwide", "UC2ccm1GajfSujz7T18d7cKA"},
	{"Ellen Show", "UCp0hYYBW6IMayGgR-WeoCvQ"},
	{"Top Gear", "UCjOl2AUblVmg2rA_cRgZkFg"},
	{"Sky News", "UCoMdktPbSTixAyNGwb-UYkQ"},
	{"Sky Sports", "UCE97AW7eR8VVbVPBy4cCLKg"},
	{"Bein Sport Officiel", "UCfj4kQ6_mYO5r4hzX5KloVw"},
	{"Al Jazeera English", "UCNye-wNBqNL5ZzHSJj3l8Bg"},
	{"MBC GROUP", "UCy2CIDKYZqqbeJ0NHNcJ9sQ"},
	{"MBC The Voice", "UCBgVoRRoRuIpVwD6CfoBRag"},
	{"MBC MASR", "UCnFKsbAof9fRv614I4wJX_w"},
	{"Shahid VOD", "UCokgtRLAWKNPdfFMaSfLLbA"},
	{"Arabs Got Talent", "UCcbFqNpZnEeV276ZmOqN2dg"},
	{"Arab Idol", "UCt8w6KPEDkf_Qe1FreHCU0A"},
	{"OSN", "UCBP0RPjVRrOC8Ob4q4IHAXQ"},
	{"Al Jazeera Arabic", "UCfiwzLy-8yKzIbsmZTzxDgw"},
	{"BBC Arabic News", "UCelk6aHijZq-GJBBB9YpReA"},
	{"CBC Drama", "UC3Dci3BzZXDo4jw4dU8KqWg"},
};
#define YOUTUBE_CHANNEL_NUM (sizeof(youtube_channel_array)/sizeof(youtube_channel_t))

static char* youtube_region_str[]=
{
	"All", //YOUTUBE_REGION_ALL
	"Algeria", //YOUTUBE_REGION_ALGERIA
	"Argentina", //YOUTUBE_REGION_ARGENTINA
	"Australia", //YOUTUBE_REGION_AUSTRALIA
	"Bahrain", //YOUTUBE_REGION_BAHRAIN
	"Brazil", //YOUTUBE_REGION_BRAZIL
	"Canada", //YOUTUBE_REGION_CANADA
	"Czech Republic", //YOUTUBE_REGION_CZECHREPUBLIC
	"Egypt", //YOUTUBE_REGION_EGYPT
	"France", //YOUTUBE_REGION_FRANCE
	"Germany", //YOUTUBE_REGION_GERMANY
	"Greece", //YOUTUBE_REGION_GREECE
	"Hong Kong", //YOUTUBE_REGION_HONGKONG
	"India", //YOUTUBE_REGION_INDIA
	"Indonesia", //YOUTUBE_REGION_INDONESIA
	"Iraq", //YOUTUBE_REGION_IRAQ
	"Ireland", //YOUTUBE_REGION_IRELAND
	"Israel", //YOUTUBE_REGION_ISRAEL
	"Italy", //YOUTUBE_REGION_ITALY
	"Japan", //YOUTUBE_REGION_JAPAN
	"Jordan", //YOUTUBE_REGION_JORDAN
	"Kenya", //YOUTUBE_REGION_KENYA
	"Kuwait", //YOUTUBE_REGION_KUWAIT
	"Lebanon", //YOUTUBE_REGION_LEBANON
	"Libya", //YOUTUBE_REGION_LIBYA
	"Malaysia", //YOUTUBE_REGION_MALAYSIA
	"Mexico", //YOUTUBE_REGION_MEXICO
	"Morocco", //YOUTUBE_REGION_MOROCCO
	"Netherlands", //YOUTUBE_REGION_NETHERLANDS
	"New Zealand", //YOUTUBE_REGION_NEWZEALAND
	"Norway", //YOUTUBE_REGION_NORWAY
	"Oman", //YOUTUBE_REGION_OMAN
	"Philippines", //YOUTUBE_REGION_PHILIPPINES
	"Poland", //YOUTUBE_REGION_POLAND
	"Portugal", //YOUTUBE_REGION_PORTUGAL
	"Qatar", //YOUTUBE_REGION_QATAR
	"Romania", //YOUTUBE_REGION_ROMANIA
	"Russia", //YOUTUBE_REGION_RUSSIA
	"Saudi Arabia", //YOUTUBE_REGION_SAUDIARABIA
	"South Africa", //YOUTUBE_REGION_SOUTHAFRICA
	"South Korea", //YOUTUBE_REGION_SOUTHKOREA
	"Spain", //YOUTUBE_REGION_SPAIN
	"Sweden", //YOUTUBE_REGION_SWEDEN
	"Taiwan", //YOUTUBE_REGION_TAIWAN
	"Thailand", //YOUTUBE_REGION_THAILAND
	"Tunisia", //YOUTUBE_REGION_TUNISIA
	"Turkey", //YOUTUBE_REGION_TURKEY
	"United Arab Emirates", //YOUTUBE_REGION_UNITEDARABEMIRATES
	"United Kingdom", //YOUTUBE_REGION_UNITEDKINGDOM
	"United States", //YOUTUBE_REGION_UNITEDSTATES
	"Vietnam", //YOUTUBE_REGION_VIETNAM
	"Yemen", //YOUTUBE_REGION_YEMEN
};
#define YOUTUBE_REGION_NUM 	(sizeof(youtube_region_str)/sizeof(char *))

static char* youtube_resolution_str[YOUTUBE_RES_MAX]=
{
	"360P", //YOUTUBE_RES_360P
	"480P", //YOUTUBE_RES_480P
	"720P", //YOUTUBE_RES_720P
	"1080P", //YOUTUBE_RES_1080P
};
#define YOUTUBE_RESOLUTION_NUM 	(sizeof(youtube_resolution_str)/sizeof(char *))

static status_t app_youtube_update_videodata(void);

static void app_youtube_show_error_msg(void)
{
	if(youtube_ret_status==GXAPPS_SERVER_ERROR)
	{
		app_net_video_show_popup_msg("No results!", 1000);
	}
	else if(youtube_ret_status==GXAPPS_NETWORK_ERROR)
	{
		app_net_video_show_popup_msg("Network error!", 1000);
	}
	else if(youtube_ret_status==GXAPPS_INTERNAL_ERROR)
	{
		app_net_video_show_popup_msg("Internal error!", 1000);
	}
	else if(youtube_ret_status==GXAPPS_UNKNOWN_ERROR)
	{
		app_net_video_show_popup_msg("Unknown error!", 1000);
	}
}

static void youtube_thread_body(void* arg)
{
	youtube_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (youtube_thread_func)arg;
	if(func)
	{
		func();
	}
}

void create_youtube_thread(youtube_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("youtube", &thread_id, youtube_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static int app_youtube_video_busy(void)
{
	return is_youtube_download;
}

static void app_youtube_pageinfo_init(void)
{
	memset(&youtube_pageinfo, 0, sizeof(YoutubePageInfo));
}

static void app_youtube_pageinfo_set(int page_num , char *next, char *prev)
{
	youtube_pageinfo.cur_page_num = page_num;
	if(youtube_pageinfo.nextPageToken)
	{
		free(youtube_pageinfo.nextPageToken);
		youtube_pageinfo.nextPageToken = NULL;
	}
	if(youtube_pageinfo.prevPageToken)
	{
		free(youtube_pageinfo.prevPageToken);
		youtube_pageinfo.prevPageToken = NULL;
	}
	if(next)
	{
		youtube_pageinfo.nextPageToken = strdup(next);
	}
	if(prev)
	{
		youtube_pageinfo.prevPageToken = strdup(prev);
	}
}
static void app_youtube_pageinfo_free(void)
{
	app_youtube_pageinfo_set(0, NULL, NULL);
}

static void app_youtube_itemdata_init(void)
{
	if(!youtube_video_item)
	{
		youtube_video_item = GxCore_Calloc(NETVIDEO_MAX_PAGE_ITEM, sizeof(YoutubeVideoItem));
	}

}
static void app_youtube_itemdata_free(void)
{
	if(youtube_video_item)
	{
		GxCore_Free(youtube_video_item);
		youtube_video_item = NULL;
	}
}

static void app_youtube_clean_menu_info(void)
{
	youtube_page_pos.cur_page = 0;
	youtube_page_pos.sel = 0;
	youtube_page_pos.total_page = 0;

	youtube_group_pos.cur_group = 1;
	youtube_group_pos.sel_group = 1;
	youtube_group_pos.total_group = 0;
	is_page_get_info = false;
}

static void youtube_setting_init(void)
{
	youtube_setting_opt=(SystemSettingOpt *)GxCore_Calloc(1, sizeof(SystemSettingOpt));
	youtube_setting_item=(SystemSettingItem *)GxCore_Calloc(1, sizeof(SystemSettingItem)*YOUTUBE_SETTING_MAX);
	youtube_setting=(youtube_setting_t *)GxCore_Calloc(1, sizeof(youtube_setting_t));
	youtube_region_array = GxCore_Calloc(1, YOUTUBE_REGIONSTR_LEN);
	youtube_resolution_array = GxCore_Calloc(1, YOUTUBE_RESSTR_LEN);
}

static void youtube_setting_destroy(void)
{
	if(youtube_setting_opt)
	{
		GxCore_Free(youtube_setting_opt);
		youtube_setting_opt=NULL;
	}

	if(youtube_setting_item)
	{
		GxCore_Free(youtube_setting_item);
		youtube_setting_item=NULL;
	}
	
	if(youtube_setting)
	{
		GxCore_Free(youtube_setting);
		youtube_setting=NULL;
	}

	if(youtube_region_array)
	{
		GxCore_Free(youtube_region_array);
		youtube_region_array=NULL;
	}
	
	if(youtube_resolution_array)
	{
		GxCore_Free(youtube_resolution_array);
		youtube_resolution_array=NULL;
	}
}

static void youtube_setting_get(void)
{
	int value = 0;
	if(!youtube_setting)
	{
		return;
	}

	GxBus_ConfigGetInt(YOUTUBE_REGION_KEY, &value, YOUTUBE_DEFAULT_REGION);
	if((value<0)||(value>=YOUTUBE_REGION_NUM))
	{
		value = 0;
	}
	youtube_setting->region= value;

	GxBus_ConfigGetInt(YOUTUBE_RESOLUTION_KEY, &value, YOUTUBE_DEFAULT_RESOLUTION);
	if((value<0)||(value>=YOUTUBE_RES_MAX))
	{
		value = 0;
	}
	youtube_setting->resolution= value;
}

static int youtube_setting_save(youtube_setting_t *ret_para)
{	
	int value = 0;   
	
	value = ret_para->region;
	if((value<0)||(value>=YOUTUBE_REGION_NUM))
	{
		value = 0;
	}
	GxBus_ConfigSetInt(YOUTUBE_REGION_KEY,value);
	youtube_setting->region= value;
	
	value = ret_para->resolution;
	if((value<0)||(value>=YOUTUBE_RES_MAX))
	{
		value = 0;
	}
	GxBus_ConfigSetInt(YOUTUBE_RESOLUTION_KEY,value);
	youtube_setting->resolution= value;
	
	return 0;
}

static void youtube_setting_all(void)
{
	if(youtube_setting)
	{
		gx_youtube_set_region(youtube_setting->region);
	}
}

static void youtube_setting_result_get(youtube_setting_t *ret_para)
{    
	ret_para->region= youtube_setting_item[YOUTUBE_SETTING_REGION].itemProperty.itemPropertyCmb.sel;
	ret_para->resolution= youtube_setting_item[YOUTUBE_SETTING_RESOLUTION].itemProperty.itemPropertyCmb.sel;
}

static int youtube_setting_change(youtube_setting_t *para)
{	
	if((youtube_setting->region == para->region)
		&&(youtube_setting->resolution == para->resolution))
	{		
		return 0;	
	}
	else
	{
		return 1;
	}
}

static int youtube_setting_exit_cb(ExitType Type)
{
	youtube_setting_t para;
	int ret = 0;

	youtube_setting_result_get(&para);
	if(youtube_setting_change(&para))
	{
		PopDlg  pop;
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_YES_NO;
        	pop.str = STR_ID_SAVE_INFO;
                if(popdlg_create(&pop) == POP_VAL_OK)
		{
			youtube_setting_save(&para);
			youtube_setting_all();
			ret = 1;
		}
	}
	app_netvideo_time_timer_reset();
	return ret;
}

static int youtube_change_callback(int sel)
{	
	return EVENT_TRANSFER_KEEPON;
}
static int youtube_cmb_press_callback(int key)
{

	return EVENT_TRANSFER_KEEPON;

}
static void youtube_setting_item_init()
{
	int i = 0;
	
	memset(youtube_region_array, 0, YOUTUBE_REGIONSTR_LEN);
	strcpy(youtube_region_array, "[");
	for(i=0; i<YOUTUBE_REGION_NUM; i++)
	{
		strcat(youtube_region_array, youtube_region_str[i]);
		if(i == (YOUTUBE_REGION_NUM-1))
		{
			strcat(youtube_region_array, "]");
		}
		else
		{
			strcat(youtube_region_array, ",");
		}
	}

	memset(youtube_resolution_array, 0, YOUTUBE_RESSTR_LEN);
	strcpy(youtube_resolution_array, "[");
	for(i=0; i<YOUTUBE_RESOLUTION_NUM; i++)
	{
		strcat(youtube_resolution_array, youtube_resolution_str[i]);
		if(i == (YOUTUBE_RESOLUTION_NUM-1))
		{
			strcat(youtube_resolution_array, "]");
		}
		else
		{
			strcat(youtube_resolution_array, ",");
		}
	}
	
	youtube_setting_item[YOUTUBE_SETTING_REGION].itemTitle = "Region";
	youtube_setting_item[YOUTUBE_SETTING_REGION].itemType = ITEM_CHOICE;
	youtube_setting_item[YOUTUBE_SETTING_REGION].itemProperty.itemPropertyCmb.content= youtube_region_array;
	youtube_setting_item[YOUTUBE_SETTING_REGION].itemProperty.itemPropertyCmb.sel = youtube_setting->region;
	youtube_setting_item[YOUTUBE_SETTING_REGION].itemCallback.cmbCallback.CmbChange= youtube_change_callback;
	youtube_setting_item[YOUTUBE_SETTING_REGION].itemCallback.cmbCallback.CmbPress= youtube_cmb_press_callback;
	youtube_setting_item[YOUTUBE_SETTING_REGION].itemStatus = ITEM_NORMAL;

	youtube_setting_item[YOUTUBE_SETTING_RESOLUTION].itemTitle = "Resolution";
	youtube_setting_item[YOUTUBE_SETTING_RESOLUTION].itemType = ITEM_CHOICE;
	youtube_setting_item[YOUTUBE_SETTING_RESOLUTION].itemProperty.itemPropertyCmb.content= youtube_resolution_array;
	youtube_setting_item[YOUTUBE_SETTING_RESOLUTION].itemProperty.itemPropertyCmb.sel = youtube_setting->resolution;
	youtube_setting_item[YOUTUBE_SETTING_RESOLUTION].itemCallback.cmbCallback.CmbChange= youtube_change_callback;
	youtube_setting_item[YOUTUBE_SETTING_RESOLUTION].itemCallback.cmbCallback.CmbPress= youtube_cmb_press_callback;
	youtube_setting_item[YOUTUBE_SETTING_RESOLUTION].itemStatus = ITEM_NORMAL;
}

static void youtube_create_setting_menu(void)
{
	memset(youtube_setting_opt, 0 ,sizeof(SystemSettingOpt));		
	memset(youtube_setting_item, 0 ,sizeof(SystemSettingItem)*YOUTUBE_SETTING_MAX);	
	memset(youtube_setting, 0 ,sizeof(youtube_setting_t));	
	
	youtube_setting_get();
	youtube_setting_item_init();
	youtube_setting_opt->menuTitle = "Youtube Setting";
	youtube_setting_opt->itemNum = YOUTUBE_SETTING_MAX;
	youtube_setting_opt->item = youtube_setting_item;
	youtube_setting_opt->exit = youtube_setting_exit_cb;
	app_system_set_create(youtube_setting_opt);
}

static void app_youtube_refresh_stop(void)
{
	youtube_data_finish = 0;
}
static void app_youtube_refresh_start(void)
{
	youtube_data_finish = 1;
}

static int youtube_videodata_timeout(void)
{
	app_net_video_hide_popup_msg();
	app_youtube_update_videodata();
	netvideo_pic_download_start();
	app_youtube_refresh_stop();
	return 0;
}

static void app_youtube_data_timer_stop(void)
{
	if(youtube_videodata_timer)
	{
		remove_timer(youtube_videodata_timer);
		youtube_videodata_timer = NULL;
	}
}

static int youtube_data_timer_timeout(void *usrdata)
{
	if(youtube_data_finish &&  (GUI_CheckDialog("wnd_pop_book") != GXCORE_SUCCESS))
	{
		youtube_videodata_timeout();
	}
	return 0;
}

static void app_youtube_data_timer_start(void)
{
	app_youtube_data_timer_stop();
	youtube_videodata_timer = create_timer(youtube_data_timer_timeout, 10, NULL, TIMER_REPEAT);
}

static void youtube_free_videodata(void)
{
	if(youtube_videolist)
	{
		gx_youtube_free_videolist(youtube_videolist);
		youtube_videolist = NULL;
	}
}
static void youtube_free_urldata(void)
{
	if(youtube_urllist)
	{
		gx_youtube_free_urllist(youtube_urllist);
		youtube_urllist = NULL;
	}
}

static char *youtube_get_page_token(void)
{
	char *page_token = NULL;

	if(youtube_page_pos.cur_page == 0)
	{
		page_token = NULL;
		app_youtube_pageinfo_set(0, NULL, NULL);
	}
	else if((youtube_page_pos.cur_page>youtube_pageinfo.cur_page_num)&&youtube_pageinfo.nextPageToken)
	{
		page_token = youtube_pageinfo.nextPageToken;
	}
	else if((youtube_page_pos.cur_page<youtube_pageinfo.cur_page_num)&&youtube_pageinfo.prevPageToken)
	{
		page_token = youtube_pageinfo.prevPageToken;
	}
	return page_token;
}

static void youtube_start_category_video_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_youtube_videolist_t *data = NULL;
	
	is_youtube_download = 1;
	youtube_data_handle++;
	handle = youtube_data_handle;
	ret = gx_youtube_get_category_videos(youtube_group_pos.sel_group-1, youtube_get_page_token(), NETVIDEO_MAX_PAGE_ITEM, &data);
	if(handle == youtube_data_handle)
	{
		youtube_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			youtube_videolist = data;
			youtube_page_pos.total_page = youtube_videolist->video_total_num / NETVIDEO_MAX_PAGE_ITEM;
			if( youtube_videolist->video_total_num % NETVIDEO_MAX_PAGE_ITEM > 0)
				youtube_page_pos.total_page++;
			//allen for youtube API V3 bug
			if(youtube_page_pos.total_page>13)
				youtube_page_pos.total_page = 13;
			is_page_get_info = true;
			app_youtube_pageinfo_set(youtube_page_pos.cur_page, youtube_videolist->nextPageToken, youtube_videolist->prevPageToken);
		}
		app_youtube_refresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_youtube_free_videolist(data);
		}
	}
	is_youtube_download = 0;
}

static void youtube_start_channel_video_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_youtube_videolist_t *data = NULL;
	
	is_youtube_download = 1;
	youtube_data_handle++;
	handle = youtube_data_handle;
	ret = gx_youtube_get_channel_videos((char *)youtube_channel_array[youtube_group_pos.sel_group-1].channel_id, youtube_get_page_token(), NETVIDEO_MAX_PAGE_ITEM, &data);
	if(handle == youtube_data_handle)
	{
		youtube_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			youtube_videolist = data;
			youtube_page_pos.total_page = youtube_videolist->video_total_num / NETVIDEO_MAX_PAGE_ITEM;
			if( youtube_videolist->video_total_num % NETVIDEO_MAX_PAGE_ITEM > 0)
				youtube_page_pos.total_page++;
			is_page_get_info = true;
			app_youtube_pageinfo_set(youtube_page_pos.cur_page, youtube_videolist->nextPageToken, youtube_videolist->prevPageToken);
		}
		app_youtube_refresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_youtube_free_videolist(data);
		}
	}
	is_youtube_download = 0;
}

static void youtube_start_search_video_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_youtube_videolist_t *data = NULL;
	
	is_youtube_download = 1;
	youtube_data_handle++;
	handle = youtube_data_handle;
	ret = gx_youtube_search_videos(youtube_search_keyword, youtube_get_page_token(), NETVIDEO_MAX_PAGE_ITEM, &data);
	if(handle == youtube_data_handle)
	{
		youtube_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			youtube_videolist = data;
			youtube_page_pos.total_page = youtube_videolist->video_total_num / NETVIDEO_MAX_PAGE_ITEM;
			if( youtube_videolist->video_total_num % NETVIDEO_MAX_PAGE_ITEM > 0)
				youtube_page_pos.total_page++;
			is_page_get_info = true;
			app_youtube_pageinfo_set(youtube_page_pos.cur_page, youtube_videolist->nextPageToken, youtube_videolist->prevPageToken);
		}
		app_youtube_refresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_youtube_free_videolist(data);
		}
	}
	is_youtube_download = 0;
}

void _youtube_search_full_keyboard_proc(PopKeyboard *data)
{
	 if(data->in_ret == POP_VAL_CANCEL)
	 {
		return;
	 }

	if((data->out_name == NULL)||strlen(data->out_name)==0)
	{
		return;
	}
	YOUTUBE_SAVE_FREE(youtube_search_keyword);
	youtube_search_keyword = strdup(data->out_name);
	if(app_youtube_video_busy())
	{
		app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);    
	}
	else
	{
		app_net_video_show_popup_msg("Updating...", 0);
		create_youtube_thread(youtube_start_search_video_feed);
	}
}

static void _youtube_search(void)
{
    static PopKeyboard keyboard;

    memset(&keyboard, 0, sizeof(PopKeyboard));
    keyboard.in_name    = NULL;
    keyboard.max_num = 127;
    keyboard.out_name   = NULL;
    keyboard.change_cb  = NULL;
    keyboard.release_cb = _youtube_search_full_keyboard_proc;
    keyboard.usr_data   = NULL;
    keyboard.pos.x = 500;
    multi_language_keyboard_create(&keyboard);
}

static status_t app_youtube_update_group(int group)
{
	int i= 0;
	int group_num = 0;
	YOUTUBE_SAVE_FREE(s_yb_group_title);
	if(group == GROUP_CHANNEL)
	{
		s_youtube_group_type = GROUP_CHANNEL;
		group_num = 1 + YOUTUBE_CHANNEL_NUM;
		s_yb_group_title = (char**)GxCore_Calloc(group_num, sizeof(char*));
		if(s_yb_group_title == NULL)
		{
			return GXCORE_ERROR;
		}

		s_yb_group_title[0] = STR_ID_SEARCH;
		for(i=1; i<group_num; i++)
		{
			s_yb_group_title[i] = (char *)youtube_channel_array[i-1].ui_name;
		}
		GUI_SetProperty(TEXT_MENU_KEY, "string", TEXT_STR_CATEGORY);
	}
	else
	{
		s_youtube_group_type = GROUP_CATEGORY;
		group_num = 1 + YOUTUBE_CATEGORY_NUM;
		s_yb_group_title = (char**)GxCore_Calloc(group_num, sizeof(char*));
		if(s_yb_group_title == NULL)
		{
			return GXCORE_ERROR;
		}

		s_yb_group_title[0] = STR_ID_SEARCH;
		for(i=1; i<group_num; i++)
		{
			s_yb_group_title[i] = youtube_category_str[i-1];
		}
		GUI_SetProperty(TEXT_MENU_KEY, "string", TEXT_STR_CHANNEL);
	}
	youtube_group_pos.total_group = group_num;
	app_net_video_group_update(youtube_group_pos.total_group, s_yb_group_title);
	
	return GXCORE_SUCCESS;
}

static status_t app_youtube_update_videodata(void)
{
	int i = 0;
	int curPageProgNum = 0;

	if(is_page_get_info&&youtube_videolist)
	{
		curPageProgNum = youtube_videolist->video_num;

		memset(youtube_video_item, 0, NETVIDEO_MAX_PAGE_ITEM*sizeof(YoutubeVideoItem));
		for(i = 0; i < curPageProgNum; i++)
		{
			if(strlen(youtube_videolist->videos[i].duration)<=5)
			{
				snprintf(youtube_video_item[i].youtube_duration, YOUTUBE_DURATION_LEN, "00:%5s", youtube_videolist->videos[i].duration);
			}
			else
			{
				snprintf(youtube_video_item[i].youtube_duration, YOUTUBE_DURATION_LEN, "%8s", youtube_videolist->videos[i].duration);
			}
			s_page_video_item[i].video_duration = youtube_video_item[i].youtube_duration;
			s_page_video_item[i].video_title = youtube_videolist->videos[i].title;
			s_page_video_item[i].video_pic = youtube_videolist->videos[i].pic_url;
			snprintf(youtube_video_item[i].youtube_author, YOUTUBE_AUTHOR_LEN, "by %s", youtube_videolist->videos[i].author);
			s_page_video_item[i].video_author  = youtube_video_item[i].youtube_author;
			snprintf(youtube_video_item[i].youtube_viewcnt, YOUTUBE_VIEWCOUNT_LEN, "%s views", youtube_videolist->videos[i].viewcount);
			s_page_video_item[i].video_viewcnt = youtube_video_item[i].youtube_viewcnt;
		}
		app_net_video_page_item_update(curPageProgNum, s_page_video_item);
		app_net_video_page_info_update(youtube_page_pos.total_page, youtube_page_pos.cur_page);
	}
	else
	{
		app_net_video_hide_gif();
		for(i = 0; i < NETVIDEO_MAX_PAGE_ITEM; i++)
		{
			s_page_video_item[i].video_duration = NULL;
			s_page_video_item[i].video_title   = "Update failed!";
			s_page_video_item[i].video_pic     = NULL;
			s_page_video_item[i].video_author  = NULL;
			s_page_video_item[i].video_viewcnt = NULL;
		}
		app_net_video_page_item_update(NETVIDEO_MAX_PAGE_ITEM, s_page_video_item);
		app_youtube_show_error_msg();
		youtube_page_pos.cur_page = youtube_pageinfo.cur_page_num;
		app_net_video_page_info_update(youtube_page_pos.total_page, youtube_page_pos.cur_page);
	}

	return GXCORE_SUCCESS;
}

static void app_youtube_play_exit_ctrl(void)
{
	youtube_geturl_handle++;
}

static void app_youtube_play_restart_ctrl(uint64_t cur_time)
{
	app_net_video_start_play(youtube_url, PLAY_STATE_LOAD, cur_time, false);
}

static int app_youtube_get_url_index(gx_youtube_urllist_t *urllist, int setting_res)
{
	int i=0;
	int j=0;

	for(i=setting_res; i>=0; i--)
	{
		for(j=0; j<urllist->url_num; j++)
		{
			if(urllist->urls[j].resolution == i)
			{
				return j;
			}
		}
	}
	return 0;
}
static void app_youtube_play_get_url(int source_index)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	int youtube_resolution = 0;
	int resolution_index = 0;
	unsigned int handle = 0;
	char *url = NULL;
	gx_youtube_urllist_t *urllist = NULL;

	youtube_geturl_handle++;
	handle = youtube_geturl_handle;
	YOUTUBE_SAVE_FREE(youtube_url);
	if(youtube_videolist&&(youtube_videolist->videos)
		&&(youtube_videolist->video_num>youtube_page_pos.sel))
	{
		youtube_free_urldata();
		if(source_index == 1)
		{
			ret = gx_youtube_get_video_urllist_combo(youtube_videolist->videos[youtube_page_pos.sel].video_id, &urllist);
		}
		else
		{
			ret = gx_youtube_get_video_urllist_from_server(youtube_videolist->videos[youtube_page_pos.sel].video_id, &urllist);
		}
		if(ret == GXAPPS_SUCCESS)
		{
			if(urllist&&urllist->urls&&(urllist->url_num>0))
			{
				youtube_resolution = youtube_setting->resolution;
				if((youtube_resolution<0)||(youtube_resolution>=YOUTUBE_RES_MAX))
				{
					youtube_resolution = 0;
				}
				resolution_index = app_youtube_get_url_index(urllist, youtube_resolution);
				url = strdup(urllist->urls[resolution_index].video_url);
				printf("play url:%s\n", url);
			}
			else
			{
				printf("url from server is null\n");
				ret = GXAPPS_SERVER_ERROR;
			}
		}
		else
		{
			if(ret == GXAPPS_INTERNAL_ERROR)
			{
				printf("internal error\n");
			}
			else if(ret == GXAPPS_NETWORK_ERROR)
			{
				printf("network error\n");
			}
			else if(ret == GXAPPS_SERVER_ERROR)
			{
				printf("data from server is invalid\n");
			}
			else
			{
				printf("unknown error\n");
			}
		}
	}

	if(handle != youtube_geturl_handle)
	{
		printf("player has exited!\n");
		if(ret == GXAPPS_SUCCESS)
		{
			gx_youtube_free_urllist(urllist);
		}
		if(url)
		{
			free(url);
		}
		return;
	}

	youtube_url = url;
	youtube_urllist = urllist;
	if((ret == GXAPPS_SUCCESS)&&youtube_url)
	{
		app_net_video_start_play(youtube_url, PLAY_STATE_LOAD, 0, false);
	}
	else
	{
		if((source_index == 0)&&(ret != GXAPPS_NETWORK_ERROR))
		{
			app_net_video_switch_source(1);
		}
		else
		{
			if(ret == GXAPPS_NETWORK_ERROR)
			{
				app_net_video_set_error_str("Network error!");
			}
			else if(ret == GXAPPS_SERVER_ERROR)
			{
				app_net_video_set_error_str("The uploader has not made this video available in your country!");
			}
			else
			{
				app_net_video_set_error_str("Internal error!");
			}
			app_net_video_start_play(NULL, PLAY_STATE_ERROR, 0, false);
		}
	}
}

static status_t app_youtube_play(char *prog_name)
{
	NetVideoPlayOps play_ops;
	memset(&play_ops, 0, sizeof(NetVideoPlayOps));

	play_ops.get_url             = app_youtube_play_get_url;
	play_ops.netvideo_title         = prog_name;
	play_ops.seek_support        = true;
	play_ops.source_num = 2;
	play_ops.play_ctrl.play_ok   = NULL;
	play_ops.play_ctrl.play_exit = app_youtube_play_exit_ctrl;
	play_ops.play_ctrl.play_prev = NULL;
	play_ops.play_ctrl.play_next = NULL;
	play_ops.play_ctrl.play_restart = app_youtube_play_restart_ctrl;

	return app_net_video_play(&play_ops);
}

static void app_youtube_obj_change_cb(NetVideoObj obj)
{
	s_video_obj = obj;
}

static void app_youtube_group_change_cb(int group_sel)
{
	youtube_group_pos.cur_group = group_sel;
}

static void app_youtube_page_change_cb(int page_num)
{
	if((page_num >= 0) && (page_num < youtube_page_pos.total_page))
	{
		is_page_get_info = false;
		netvideo_pic_download_stop();
		app_youtube_refresh_stop();
		app_net_video_clear_all_item();
		youtube_free_videodata();
		youtube_page_pos.cur_page = page_num;
		if(youtube_group_pos.sel_group == 0) //search
		{
			if(app_youtube_video_busy())
			{
				app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);    
			}
			else
			{
				app_net_video_show_popup_msg("Updating...", 0);
				create_youtube_thread(youtube_start_search_video_feed);
			}
		}
		else 
		{
			if(s_youtube_group_type == GROUP_CHANNEL)
			{
				if(app_youtube_video_busy())
				{
					app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);    
				}
				else
				{
					app_net_video_show_popup_msg("Updating...", 0);
					create_youtube_thread(youtube_start_channel_video_feed);
				}
			}
			else
			{
				if(app_youtube_video_busy())
				{
					app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);    
				}
				else
				{
					app_net_video_show_popup_msg("Updating...", 0);
					create_youtube_thread(youtube_start_category_video_feed);
				}
			}
		}
	}
}

static void app_youtube_video_change_cb(int video_sel)
{
	if(youtube_page_pos.sel != video_sel)
	{
		youtube_page_pos.sel = video_sel;
	}
}

static void app_youtube_menu_exit_cb(void)
{
	youtube_data_handle++;
	app_youtube_refresh_stop();
	app_youtube_data_timer_stop();
	YOUTUBE_SAVE_FREE(s_yb_group_title);
	YOUTUBE_SAVE_FREE(youtube_url);
	YOUTUBE_SAVE_FREE(youtube_search_keyword);
	app_youtube_clean_menu_info();
	youtube_free_videodata();
	youtube_free_urldata();
	youtube_setting_destroy();
	app_youtube_itemdata_free();
	app_youtube_pageinfo_free();
	gx_netapps_app_destroy();
}

static void app_youtube_ok_press_cb(void)
{
	if(app_youtube_video_busy())
	{
		app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);
		return;
	}
	if(s_video_obj == OBJ_VIDEO_GROUP)
	{
		is_page_get_info = false;
		netvideo_pic_download_stop();
		app_youtube_refresh_stop();
		app_net_video_clear_all_item();
		youtube_free_videodata();
		app_net_video_page_item_update(0, NULL);
		app_net_video_page_info_update(0, 0);
		youtube_group_pos.sel_group = youtube_group_pos.cur_group;
		youtube_page_pos.cur_page = 0;
		youtube_page_pos.sel = 0;
		youtube_page_pos.total_page = 0;
		if(youtube_group_pos.cur_group == 0) //search
		{
			app_net_video_hide_popup_msg();
			_youtube_search();
		}
		else 
		{
			app_net_video_show_popup_msg("Updating...", 0);
			if(s_youtube_group_type == GROUP_CHANNEL)
			{
				create_youtube_thread(youtube_start_channel_video_feed);
			}
			else
			{
				create_youtube_thread(youtube_start_category_video_feed);
			}
		}
	}
	else if((s_video_obj == OBJ_VIDEO_PAGE)&&is_page_get_info
		&&youtube_videolist&&(youtube_videolist->videos)
		&&(youtube_videolist->video_num>youtube_page_pos.sel))
	{
		app_net_video_hide_gif();
		netvideo_pic_download_stop();
		app_youtube_refresh_stop();
		app_youtube_play(youtube_videolist->videos[youtube_page_pos.sel].title); 
	}
}

static void app_youtube_menu_key_func(void)
{
	int sel = 1;
	if(app_youtube_video_busy())
	{
		app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);    
		return;
	}
	app_net_video_set_obj(OBJ_VIDEO_GROUP);
	GUI_SetProperty(LIST_GROUP, "select", &sel);
	netvideo_pic_download_stop();
	app_youtube_clean_menu_info();
	app_youtube_refresh_stop();
	app_net_video_clear_all_item();
	app_net_video_page_item_update(0, NULL);
	app_net_video_page_info_update(0, 0);
	youtube_free_videodata();
	if(s_youtube_group_type == GROUP_CHANNEL)
	{
		app_youtube_update_group(GROUP_CATEGORY);
	}
	else
	{
		app_youtube_update_group(GROUP_CHANNEL);
	}
}

static void app_youtube_red_key_func(void)
{
	if(app_youtube_video_busy())
	{
		app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);    
		return;
	}
	netvideo_pic_download_stop();
	app_youtube_refresh_stop();
	youtube_create_setting_menu();
}

status_t app_create_youtube_menu(void)
{
	NetVideoMenuCb video_cb;
	gx_netapps_app_init();
	app_youtube_data_timer_start();
	app_youtube_pageinfo_init();
	app_youtube_itemdata_init();
	app_youtube_clean_menu_info();
	youtube_setting_init();
	youtube_setting_get();
	youtube_setting_all();
	app_youtube_obj_change_cb(OBJ_VIDEO_GROUP);
	memset(&video_cb, 0, sizeof(NetVideoMenuCb));
	video_cb.obj_change         = app_youtube_obj_change_cb;
	video_cb.video_group_change = app_youtube_group_change_cb;
	video_cb.video_page_change  = app_youtube_page_change_cb;
	video_cb.video_sel_change   = app_youtube_video_change_cb;
	video_cb.video_exit         = app_youtube_menu_exit_cb;
	video_cb.ok_press           = app_youtube_ok_press_cb;
	video_cb.video_busy        = app_youtube_video_busy;
	app_net_video_create(STR_ID_NET_YOUTUBE, YOUTUBE_LOGO, &video_cb, youtube_group_pos.cur_group);

	app_net_video_enable_menu_key(TEXT_STR_CHANNEL, app_youtube_menu_key_func);
	app_net_video_enable_red_key("Setting", app_youtube_red_key_func);
	
	GUI_SetInterface("flush", NULL);
	app_youtube_update_group(GROUP_CATEGORY);
	app_youtube_ok_press_cb();
	
	return GXCORE_SUCCESS;
}

#endif

