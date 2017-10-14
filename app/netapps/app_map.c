#include "app.h"
#if MAP_SUPPORT
#include "app_module.h"
#include "app_msg.h"
#include "gui_core.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_netapps_service.h"
#include <ctype.h>
#include <gx_apps.h>
#include <gx_googlemaps.h>

#ifdef ECOS_OS
#define MAP_IMAGE_PATH "/mnt/map.jpg"
#else
#define MAP_IMAGE_PATH "/tmp/map/map.jpg"
#endif

#define GOOGLEMAPS_MAP_WIDTH 640
#define GOOGLEMAPS_MAP_HEIGHT 320
#define GOOGLEMAPS_DEFAULT_ZOOMLEVEL GOOGLEMAPS_ZOOMLEVEL_15

#define MAP_SEARCH_MAX_ADDRESS (16)
#define MAP_SEARCHLIST_BUFLEN (128)
#define WND_MAP 		"wnd_map"
#define IMG_MAP_PIC       "img_map_pic"
#define TEXT_MAP_COORDINATE "text_map_coordinate"
#define IMG_MAP_SLIDERBAR_OK "img_map_sliderbar_bk"
#define MAP_SLIDERBAR "map_sliderbar"
#define MAP_LANGSTR_LEN 2048

static void app_map_tip_show(char *buf, int time_out);
SIGNAL_HANDLER int app_map_search_list_get_total(const char* widgetname, void *usrdata);

typedef void(* map_thread_func)(void);

typedef struct
{
	int language;
	int roadtype;
}map_setting_t;

typedef enum 
{
	MAP_SETTING_LANGUAGE=0,
	MAP_SETTING_MAPTYPE,
	MAP_SETTING_MAX,
}map_setting_itm;

static int is_map_download = 0;
static event_list* map_popup_timer = NULL;
static event_list* map_picupdate_timer = NULL;
static event_list* map_data_timer = NULL;
static int map_search_finish = 0;
static gx_googlemaps_addresslist_t *map_search_addresslist = NULL;
static char *map_search_keyword = NULL;
static double map_latitude = 0;
static double map_longitude = 0;
static int map_zoom_level = GOOGLEMAPS_DEFAULT_ZOOMLEVEL;
static int map_msg_on = 0;
static char map_search_result[MAP_SEARCH_MAX_ADDRESS][MAP_SEARCHLIST_BUFLEN];
static char *map_lang_array = NULL;
static SystemSettingOpt  *map_setting_opt = NULL;
static SystemSettingItem *map_setting_item = NULL;
static map_setting_t *map_setting = NULL;
static gxapps_ret_t map_ret_status = GXAPPS_SUCCESS;
static int map_need_refresh = 0;
static unsigned int map_data_handle = 0;
static unsigned int map_pic_download_handle = 0;
static int map_pic_download_abort = 0;
static int map_pic_download_ok = 0;
static int map_pic_show_ok = 0;

static char* map_language_str[GOOGLEMAPS_LANGUAGE_MAX]=
{
	"English", //GOOGLEMAPS_LANGUAGE_ENGLISH
	"French", //GOOGLEMAPS_LANGUAGE_FRENCH
	"German", //GOOGLEMAPS_LANGUAGE_GERMAN
	"Spanish", //GOOGLEMAPS_LANGUAGE_SPANISH
	"Portuguese", //GOOGLEMAPS_LANGUAGE_PORTUGUESE
	"Italian", //GOOGLEMAPS_LANGUAGE_ITALIAN
	"Arabic", //GOOGLEMAPS_LANGUAGE_ARABIC
	"Turkish", //GOOGLEMAPS_LANGUAGE_TURKISH
	"Russian", //GOOGLEMAPS_LANGUAGE_RUSSIA
	"Polish", //GOOGLEMAPS_LANGUAGE_POLISH
	"Dutch", //GOOGLEMAPS_LANGUAGE_DUTCH
	"Slovak", //GOOGLEMAPS_LANGUAGE_SLOVAK
	"Czech", //GOOGLEMAPS_LANGUAGE_CZECH
	"Hungarian", //GOOGLEMAPS_LANGUAGE_HUNGARIAN
	"Greek", //GOOGLEMAPS_LANGUAGE_GREEK
	"Korean", //GOOGLEMAPS_LANGUAGE_KOREAN
	"Vietnamese", //GOOGLEMAPS_LANGUAGE_VIETNAMESE
	"Thai", //GOOGLEMAPS_LANGUAGE_THAI
	"Malay", //GOOGLEMAPS_LANGUAGE_MALAY
	"Indonesian", //GOOGLEMAPS_LANGUAGE_INDONESIAN
	"Bulgarian", //GOOGLEMAPS_LANGUAGE_BULGARIAN
	"Lithuanian", //GOOGLEMAPS_LANGUAGE_LITHUANIAN
	"Persian", //GOOGLEMAPS_LANGUAGE_PERSIAN
};
#define MAP_LANG_NUM 	(sizeof(map_language_str)/sizeof(char *))

#ifdef ECOS_OS
#define GIF_PATH "/dvb/theme/image/netapps/loading.gif"
#else
#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
#endif

static void map_pic_download_stop(void);

static void map_thread_body(void* arg)
{
	map_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (map_thread_func)arg;
	if(func)
	{
		func();
	}
}

void create_map_thread(map_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("map", &thread_id, map_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static void map_search_addresslist_destroy(void)
{
	if(map_search_addresslist)
	{
		gx_googlemaps_free_addlist(map_search_addresslist);
		map_search_addresslist = NULL;
	}
}

static void app_map_tip_hide(void)
{
	map_msg_on = 0;
	GUI_SetProperty("img_map_tip_bk", "state", "hide");
	GUI_SetProperty("text_map_tip_status", "state", "hide");
	if(map_popup_timer)
	{
		remove_timer(map_popup_timer);
		map_popup_timer = NULL;
	}
	GUI_SetProperty(WND_MAP, "update", NULL);
}

static int map_setting_change_callback(int sel)
{	
	return EVENT_TRANSFER_KEEPON;
}
static int map_setting_cmb_press_callback(int key)
{
	return EVENT_TRANSFER_KEEPON;

}

static void map_setting_init(void)
{
	map_setting_opt=(SystemSettingOpt *)GxCore_Calloc(1, sizeof(SystemSettingOpt));
	map_setting_item=(SystemSettingItem *)GxCore_Calloc(1, sizeof(SystemSettingItem)*MAP_SETTING_MAX);
	map_setting=(map_setting_t *)GxCore_Calloc(1, sizeof(map_setting_t));
	map_lang_array = GxCore_Calloc(1, MAP_LANGSTR_LEN);
	map_zoom_level = GOOGLEMAPS_DEFAULT_ZOOMLEVEL;
	gx_googlemaps_set_zoomlevel(GOOGLEMAPS_DEFAULT_ZOOMLEVEL);
	gx_googlemaps_set_size(GOOGLEMAPS_MAP_WIDTH, GOOGLEMAPS_MAP_HEIGHT);
	gx_googlemaps_set_coordinate(map_latitude, map_longitude);
}

static void map_setting_destroy(void)
{
	if(map_setting_opt)
	{
		GxCore_Free(map_setting_opt);
		map_setting_opt=NULL;
	}

	if(map_setting_item)
	{
		GxCore_Free(map_setting_item);
		map_setting_item=NULL;
	}
	
	if(map_setting)
	{
		GxCore_Free(map_setting);
		map_setting=NULL;
	}

	if(map_lang_array)
	{
		GxCore_Free(map_lang_array);
		map_lang_array=NULL;
	}
}

static void map_setting_get(void)
{
	int value = 0;
	if(!map_setting)
	{
		return;
	}

	GxBus_ConfigGetInt(MAP_LANGUAGE_KEY, &value, MAP_DEFAULT_LANGUAGE);
	if((value<0)||(value>=GOOGLEMAPS_LANGUAGE_MAX))
	{
		value = 0;
	}
	map_setting->language= value;

	GxBus_ConfigGetInt(MAP_MAPTYPE_KEY, &value, MAP_DEFAULT_MAPTYPE);
	if((value<0)||(value>=GOOGLEMAPS_MAPTYPE_MAX))
	{
		value = 0;
	}
	map_setting->roadtype = value;
}

static int map_setting_save(map_setting_t *ret_para)
{	
	int value = 0;   
	
	value = ret_para->language;
	if((value<0)||(value>=GOOGLEMAPS_LANGUAGE_MAX))
	{
		value = 0;
	}
	GxBus_ConfigSetInt(MAP_LANGUAGE_KEY,value);
	map_setting->language= value;
	
	value = ret_para->roadtype;
	if((value<0)||(value>=GOOGLEMAPS_MAPTYPE_MAX))
	{
		value = 0;
	}
	GxBus_ConfigSetInt(MAP_MAPTYPE_KEY,value);
	map_setting->roadtype= value;
	
	return 0;
}

static void map_latlng_get(void)
{
	GxBus_ConfigGetDouble(MAP_LATITUDE_KEY, &map_latitude, MAP_DEFAULT_LATITUDE);
	GxBus_ConfigGetDouble(MAP_LONGITUDE_KEY, &map_longitude, MAP_DEFAULT_LONGITUDE);
}

static void map_latlng_save(void)
{
	GxBus_ConfigSetDouble(MAP_LATITUDE_KEY,map_latitude);
	GxBus_ConfigSetDouble(MAP_LONGITUDE_KEY,map_longitude);
}

static void map_setting_all(void)
{
	if(map_setting)
	{
		gx_googlemaps_set_language(map_setting->language);
		gx_googlemaps_set_maptype(map_setting->roadtype);
	}
}

static void map_setting_result_get(map_setting_t *ret_para)
{    
	ret_para->language= map_setting_item[MAP_SETTING_LANGUAGE].itemProperty.itemPropertyCmb.sel;
	ret_para->roadtype = map_setting_item[MAP_SETTING_MAPTYPE].itemProperty.itemPropertyCmb.sel;
}

static int map_setting_change(map_setting_t *para)
{	
	if((map_setting->language == para->language)
		&&(map_setting->roadtype == para->roadtype))
	{		
		return 0;	
	}
	else
	{
		return 1;
	}
}

static int map_setting_exit_cb(ExitType Type)
{
	map_setting_t para;
	int ret = 0;

	map_setting_result_get(&para);
	if(map_setting_change(&para))
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_YES_NO;
		pop.str = STR_ID_SAVE_INFO;
		if(popdlg_create(&pop) == POP_VAL_OK)
		{
			map_setting_save(&para);
			map_setting_all();
			ret = 1;
		}
	}
	return ret;
}

static void map_setting_item_init()
{
	int i = 0;
	
	memset(map_lang_array, 0, MAP_LANGSTR_LEN);
	strcpy(map_lang_array, "[");
	for(i=0; i<MAP_LANG_NUM; i++)
	{
		strcat(map_lang_array, map_language_str[i]);
		if(i == (MAP_LANG_NUM-1))
		{
			strcat(map_lang_array, "]");
		}
		else
		{
			strcat(map_lang_array, ",");
		}
	}
	
	map_setting_item[MAP_SETTING_LANGUAGE].itemTitle = "Language";
	map_setting_item[MAP_SETTING_LANGUAGE].itemType = ITEM_CHOICE;
	map_setting_item[MAP_SETTING_LANGUAGE].itemProperty.itemPropertyCmb.content= map_lang_array;
	map_setting_item[MAP_SETTING_LANGUAGE].itemProperty.itemPropertyCmb.sel = map_setting->language;
	map_setting_item[MAP_SETTING_LANGUAGE].itemCallback.cmbCallback.CmbChange= map_setting_change_callback;
	map_setting_item[MAP_SETTING_LANGUAGE].itemCallback.cmbCallback.CmbPress= map_setting_cmb_press_callback;
	map_setting_item[MAP_SETTING_LANGUAGE].itemStatus = ITEM_NORMAL;

	map_setting_item[MAP_SETTING_MAPTYPE].itemTitle = "Map Type";
	map_setting_item[MAP_SETTING_MAPTYPE].itemType = ITEM_CHOICE;
	map_setting_item[MAP_SETTING_MAPTYPE].itemProperty.itemPropertyCmb.content= "[Roadmap,Satellite,Terrain,Hybrid]";
	map_setting_item[MAP_SETTING_MAPTYPE].itemProperty.itemPropertyCmb.sel = map_setting->roadtype;
	map_setting_item[MAP_SETTING_MAPTYPE].itemCallback.cmbCallback.CmbChange= map_setting_change_callback;
	map_setting_item[MAP_SETTING_MAPTYPE].itemCallback.cmbCallback.CmbPress= map_setting_cmb_press_callback;
	map_setting_item[MAP_SETTING_MAPTYPE].itemStatus = ITEM_NORMAL;	

}

static void map_create_setting_menu(void)
{
	map_need_refresh = 1;
	map_pic_download_stop();
	memset(map_setting_opt, 0 ,sizeof(SystemSettingOpt));		
	memset(map_setting_item, 0 ,sizeof(SystemSettingItem)*MAP_SETTING_MAX);	
	memset(map_setting, 0 ,sizeof(map_setting_t));	
	
	map_setting_get();
	map_setting_item_init();
	map_setting_opt->menuTitle = "Map Setting";
	map_setting_opt->itemNum = MAP_SETTING_MAX;
	map_setting_opt->item = map_setting_item;
	map_setting_opt->exit = map_setting_exit_cb;
	app_system_set_create(map_setting_opt);
}


static status_t map_pic_update(char *pic_url)
{
	char* focus_win = NULL;
	
	focus_win = (char*)GUI_GetFocusWindow();
	if(!focus_win) 
		return GXCORE_ERROR;
	if(strcasecmp(focus_win, WND_MAP))
		return GXCORE_ERROR;
	
	if(pic_url)
	{
		GUI_SetProperty(IMG_MAP_PIC, "load_scal_img", pic_url);
		GUI_SetProperty(IMG_MAP_PIC, "state", "show");
	}
	else
	{
		GUI_SetProperty(IMG_MAP_PIC, "state", "hide");
	}

	return GXCORE_SUCCESS;
}

static status_t map_info_update(int show)
{
	char buf[64] = {0};
	if(show)
	{
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%f,%f", map_latitude, map_longitude);
		GUI_SetProperty(TEXT_MAP_COORDINATE, "state", "show");
		GUI_SetProperty(TEXT_MAP_COORDINATE, "string", buf);
		GUI_SetProperty(IMG_MAP_SLIDERBAR_OK, "state", "show");
		GUI_SetProperty(MAP_SLIDERBAR, "state", "show");
		GUI_SetProperty(MAP_SLIDERBAR, "value", &map_zoom_level);
		GUI_SetFocusWidget(MAP_SLIDERBAR);
	}
	else
	{
		GUI_SetProperty(TEXT_MAP_COORDINATE, "state", "hide");
		GUI_SetProperty(MAP_SLIDERBAR, "state", "hide");
		GUI_SetProperty(IMG_MAP_SLIDERBAR_OK, "state", "hide");
	}
	return GXCORE_SUCCESS;
}

void app_map_clear_all(void)
{
	GUI_SetProperty(MAP_SLIDERBAR, "state", "hide");
	GUI_SetProperty(IMG_MAP_SLIDERBAR_OK, "state", "hide");
	GUI_SetProperty(TEXT_MAP_COORDINATE, "state", "hide");
	GUI_SetProperty(IMG_MAP_PIC, "state", "hide");
	GUI_SetProperty(WND_MAP, "update", NULL);
}


static int map_popup_timer_timeout(void *userdata)
{
	app_map_tip_hide();
	return 0;
}

static int map_pic_update_timer_timeout(void *userdata)
{
	if(map_pic_show_ok==1)
	{
		return 0;
	}
	
	if(map_pic_download_ok == 1)
	{
		if(GXCORE_FILE_UNEXIST != GxCore_FileExists(MAP_IMAGE_PATH))
		{
			map_pic_update(MAP_IMAGE_PATH);
			map_info_update(1);
			GUI_SetProperty(WND_MAP, "update", NULL);
		}
		else
		{
			map_pic_update(NULL);
			map_info_update(0);
			app_map_tip_show("Network error!", 1000);
		}
		map_pic_show_ok = 1;
		timer_stop(map_picupdate_timer);
		map_picupdate_timer = NULL;
	}

	return 0;
}

static void map_update_timer_stop(void)
{
	if(map_picupdate_timer)
	{
		remove_timer(map_picupdate_timer);
		map_picupdate_timer = NULL;
	}
}

static void map_update_timer_reset(void)
{
	if (reset_timer(map_picupdate_timer) != 0)
	{
		map_picupdate_timer = create_timer(map_pic_update_timer_timeout, 300, NULL, TIMER_REPEAT);
	}
}

static void map_delete_files(void)
{
	if(GXCORE_FILE_EXIST == GxCore_FileExists(MAP_IMAGE_PATH))
	{
		GxCore_FileDelete(MAP_IMAGE_PATH);
	}
}

static void map_pic_download_stop(void)
{
	map_update_timer_stop();
	map_pic_download_ok = 0;
	map_pic_show_ok = 0;
	map_pic_download_abort= 1;
	if(map_pic_download_handle!=0)
	{
		gxapps_curl_async_abort(map_pic_download_handle);
		map_pic_download_handle = 0;
	}
	map_delete_files();
	map_pic_download_abort= 0;
}

static void map_download_pic_cb(int message, int body);
#ifdef ECOS_OS
extern int app_check_wifi_status(void);
#endif
static void map_download_pic(void)
{
	char image_url[256];
	if(map_pic_download_abort == 0)
	{
	#ifdef ECOS_OS
		if(app_check_wifi_status()<=0)
		{
			map_pic_download_ok = 1;
			return;
		}
	#endif
		memset(image_url, 0, sizeof(image_url));
		gx_googlemaps_get_mapurl(image_url);
		map_pic_download_handle = gxapps_curl_async_download(image_url, MAP_IMAGE_PATH, map_download_pic_cb);
	}
}

static void map_download_pic_cb(int message, int body)
{
	gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;
	unsigned int handle = 0;
	
	if(message == GXCURL_STATE_COMPLETE)
	{
		if(map_pic_download_abort)
			return;
		handle = callback_data->handle;
		if(callback_data->handle>0)
		{
			if(map_pic_download_handle ==callback_data->handle)
			{
				map_pic_download_ok = 1;
			}
		}
	}
}

static void map_pic_download_start(void)
{
	map_pic_download_stop();
	map_update_timer_reset();
	map_download_pic();		
}

static void app_map_tip_show(char *buf, int time_out)
{
	map_msg_on = 1;
	GUI_SetProperty("img_map_tip_bk", "state", "show");
	GUI_SetProperty("text_map_tip_status", "string", buf);
	GUI_SetProperty("text_map_tip_status", "state", "show");
	if(time_out > 0)
	{
		if (reset_timer(map_popup_timer) != 0)
		{
			map_popup_timer = create_timer(map_popup_timer_timeout, time_out, NULL, TIMER_ONCE);
		}
	}
}

static void app_map_show_errormsg(void)
{
	if(map_ret_status==GXAPPS_SERVER_ERROR)
	{
		app_map_tip_show("Can not find this address!", 1000);
	}
	else if(map_ret_status==GXAPPS_NETWORK_ERROR)
	{
		app_map_tip_show("Network error!", 1000);
	}
	else if(map_ret_status==GXAPPS_INTERNAL_ERROR)
	{
		app_map_tip_show("Internal error!", 1000);
	}
	else if(map_ret_status==GXAPPS_UNKNOWN_ERROR)
	{
		app_map_tip_show("Unknown error!", 1000);
	}
}

static void app_map_search_refresh_stop(void)
{
	map_search_finish = 0;
}
static void app_map_search_refresh_start(void)
{
	map_search_finish = 1;
}

static void app_map_search_show(void)
{
	int sel = 0;
	GUI_SetProperty("img_map_search_bk", "state", "show");
	GUI_SetProperty("text_map_search_title", "state", "show");
	GUI_SetProperty("list_map_search", "state", "show");
	GUI_SetFocusWidget("list_map_search");
	GUI_SetProperty("list_map_search", "select", &sel);
	GUI_SetProperty("list_map_search", "update_all", NULL);
}

static void app_map_search_hide(void)
{
	GUI_SetProperty("img_map_search_bk", "state", "hide");
	GUI_SetProperty("text_map_search_title", "state", "hide");
	GUI_SetProperty("list_map_search", "state", "hide");
}

static int map_search_timeout(void)
{
	if(map_search_addresslist&&(map_search_addresslist->addresses)&&(map_search_addresslist->address_num>0))
	{
		app_map_tip_hide();
		app_map_search_show();
	}
	else
	{
		app_map_show_errormsg();    
	}
	app_map_search_refresh_stop();
	return 0;
}

static void app_map_data_timer_stop(void)
{
	if(map_data_timer)
	{
		remove_timer(map_data_timer);
		map_data_timer = NULL;
	}
}

static int map_data_timer_timeout(void *usrdata)
{
	if(GUI_CheckDialog("wnd_pop_book") != GXCORE_SUCCESS)
	{
		if(map_search_finish)
		{
			map_search_timeout();
		}
	}
	return 0;
}

static void app_map_data_timer_start(void)
{
	app_map_data_timer_stop();
	map_data_timer = create_timer(map_data_timer_timeout, 10, NULL, TIMER_REPEAT);
}


static int app_map_busy(void)
{
	return is_map_download;
}

static void map_start_search_address(void)
{
	unsigned int handle = 0;
	gx_googlemaps_addresslist_t *addrlist = NULL;
	gxapps_ret_t ret = GXAPPS_SUCCESS;
	
	is_map_download = 1;
	map_data_handle++;
	handle = map_data_handle;
	map_search_addresslist_destroy();
	ret = gx_googlemaps_search_address(map_search_keyword, &addrlist);
	if(handle == map_data_handle)
	{
		map_ret_status = ret;
		if(map_ret_status == GXAPPS_SUCCESS)
		{
			map_search_addresslist = addrlist;		
		}
		app_map_search_refresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_googlemaps_free_addlist(addrlist);
		}
	}
	is_map_download = 0;
}

static void app_map_refresh(int clear)
{
	map_pic_download_stop();
	if(clear)
	{
		app_map_clear_all();
	}
	map_pic_download_start();
}

void map_search_full_keyboard_proc(PopKeyboard *data)
{
	 if(data->in_ret == POP_VAL_CANCEL)
        	return;

   	if(data->out_name == NULL)
       	     return;

	if(map_search_keyword)
	{
		free(map_search_keyword);
		map_search_keyword = NULL;
	}
	map_search_keyword = strdup(data->out_name);
	app_map_tip_show("Searching...", 0);
	if(app_map_busy())
	{
		app_map_tip_show("System Busy, Please Wait!", 1000);    
	}
	else
	{
		create_map_thread(map_start_search_address);
	}
}

SIGNAL_HANDLER int app_map_search_list_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{	
			case STBK_UP:
			case STBK_DOWN:
				GUI_SetInterface("flush",NULL);
				ret = EVENT_TRANSFER_KEEPON;
				break;
			case STBK_EXIT:
			case STBK_MENU:
				app_map_search_hide();
				break;
			case STBK_OK:
				{
					int sel = 0;
					GUI_GetProperty("list_map_search", "select", &sel);
					map_latitude = map_search_addresslist->addresses[sel].latitude;
					map_longitude = map_search_addresslist->addresses[sel].longitude;
					gx_googlemaps_set_coordinate(map_latitude, map_longitude);
					map_latlng_save();
					app_map_search_hide();
					app_map_refresh(1);
				}
				break;
            case STBK_PAGE_UP:
                {
                    int sel = 0;
                    GUI_GetProperty("list_map_search", "select", &sel);
                    if(0 == sel)
                    {
                        sel = app_map_search_list_get_total(NULL, NULL) - 1;
                        GUI_SetProperty("list_map_search", "select", &sel);

                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;
                }
            case STBK_PAGE_DOWN:
                {
                    int sel = 0;
                    GUI_GetProperty("list_map_search", "select", &sel);
                    if(sel == (app_map_search_list_get_total(NULL, NULL) - 1))
                    {
                        sel = 0;
                        GUI_SetProperty("list_map_search", "select", &sel);

                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;
                }
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_map_search_list_get_total(const char* widgetname, void *usrdata)
{
	int ret = 0;
	if(map_search_addresslist&&(map_search_addresslist->address_num>0))
	{
		if(map_search_addresslist->address_num>MAP_SEARCH_MAX_ADDRESS)
		{
			ret = MAP_SEARCH_MAX_ADDRESS;
		}
		else
		{
			ret = map_search_addresslist->address_num;
		}
	}
	return ret;
}
SIGNAL_HANDLER int app_map_search_list_get_data(const char* widgetname, void *usrdata)
{	
	ListItemPara* item = NULL;
	item = (ListItemPara*)usrdata;
	if(NULL == item) 	
		return GXCORE_ERROR;
	if((item->sel<0)||(item->sel>=MAP_SEARCH_MAX_ADDRESS)
		||(!map_search_addresslist)||(map_search_addresslist->address_num<=0)
		||(item->sel>=map_search_addresslist->address_num))
		return GXCORE_ERROR;
	snprintf(map_search_result[item->sel], MAP_SEARCHLIST_BUFLEN, "%s,%f,%f",
		map_search_addresslist->addresses[item->sel].address,
		map_search_addresslist->addresses[item->sel].latitude,
		map_search_addresslist->addresses[item->sel].longitude);
	item->string = map_search_result[item->sel];
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_map_sliderbar_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;
	
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		uint16_t key = find_virtualkey_ex(event->key.scancode,event->key.sym);
		switch(key)
		{
			case STBK_LEFT:
				if(map_msg_on == 0)
				{
					if(app_map_busy())
					{
						app_map_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						gx_googlemaps_move(GOOGLEMAPS_MOVE_RIGHT);
						gx_googlemaps_get_coordinate(&map_latitude, &map_longitude);
						app_map_refresh(0);
					}
				}
				break;
			case STBK_RIGHT:
				if(map_msg_on == 0)
				{
					if(app_map_busy())
					{
						app_map_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						gx_googlemaps_move(GOOGLEMAPS_MOVE_LEFT);
						gx_googlemaps_get_coordinate(&map_latitude, &map_longitude);
						app_map_refresh(0);
					}
				}
				break;	
			default:
				ret = EVENT_TRANSFER_KEEPON;
				break;
		}		
	}
	return ret;
}
SIGNAL_HANDLER int app_map_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		uint16_t key = find_virtualkey_ex(event->key.scancode,event->key.sym);
		switch(key)
		{
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				break;
			case STBK_MENU:
			case STBK_EXIT:
				if(map_msg_on)
				{
					map_data_handle++;
					app_map_tip_hide();
				}
				else
				{
					GUI_EndDialog(WND_MAP);
				}
				break;
			case STBK_UP:
				if(map_msg_on == 0)
				{
					if(app_map_busy())
					{
						app_map_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						gx_googlemaps_move(GOOGLEMAPS_MOVE_UP);
						gx_googlemaps_get_coordinate(&map_latitude, &map_longitude);
						app_map_refresh(0);
					}
				}
				break;
			case STBK_DOWN:
				if(map_msg_on == 0)
				{
					if(app_map_busy())
					{
						app_map_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						gx_googlemaps_move(GOOGLEMAPS_MOVE_DN);
						gx_googlemaps_get_coordinate(&map_latitude, &map_longitude);
						app_map_refresh(0);
					}
				}
				break;
			case STBK_LEFT:
				if(map_msg_on == 0)
				{
					if(app_map_busy())
					{
						app_map_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						gx_googlemaps_move(GOOGLEMAPS_MOVE_RIGHT);
						gx_googlemaps_get_coordinate(&map_latitude, &map_longitude);
						app_map_refresh(0);
					}
				}
				break;
			case STBK_RIGHT:
				if(map_msg_on == 0)
				{
					if(app_map_busy())
					{
						app_map_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						gx_googlemaps_move(GOOGLEMAPS_MOVE_LEFT);
						gx_googlemaps_get_coordinate(&map_latitude, &map_longitude);
						app_map_refresh(0);
					}
				}
				break;	
			case STBK_RED:
				if(map_msg_on == 0)
				{
					map_pic_download_stop();
					static PopKeyboard keyboard;
					memset(&keyboard, 0, sizeof(PopKeyboard));
					keyboard.in_name    = NULL;
					keyboard.max_num = 120;
					keyboard.out_name   = NULL;
					keyboard.change_cb  = NULL;
					keyboard.release_cb = map_search_full_keyboard_proc;
					keyboard.usr_data   = NULL;
					keyboard.pos.x = 500;
					multi_language_keyboard_create(&keyboard);
				}
				break;
			case STBK_GREEN:
				if(map_msg_on == 0)
				{
					if(app_map_busy())
					{
						app_map_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						if(map_zoom_level>GOOGLEMAPS_ZOOMLEVEL_1)
						{
							map_zoom_level--;
							gx_googlemaps_set_zoomlevel(map_zoom_level);
							app_map_refresh(0);
						}
					}
				}
				break;
			case STBK_YELLOW:
				if(map_msg_on==0)
				{
					if(app_map_busy())
					{
						app_map_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						if(map_zoom_level<GOOGLEMAPS_ZOOMLEVEL_21)
						{
							map_zoom_level++;
							gx_googlemaps_set_zoomlevel(map_zoom_level);
							app_map_refresh(0);
						}
					}
				}
				break;
			case STBK_BLUE:
				if(map_msg_on==0)
				{
					map_create_setting_menu();
				}
				break;
			default:
				break;
		}		
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_map_create(const char* widgetname, void *usrdata)
{
	app_map_data_timer_start();
	map_latlng_get();
	map_msg_on = 0;
	map_setting_init();
	map_setting_get();
	map_setting_all();
	app_map_clear_all();
	map_pic_download_start();
	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_map_destroy(const char* widgetname, void *usrdata)
{
	is_map_download = 0;
	map_data_handle++;
	app_map_search_refresh_stop();
	app_map_data_timer_stop();
	app_map_tip_hide();
	map_pic_download_stop();
	map_search_addresslist_destroy();
	if(map_search_keyword)
	{
		free(map_search_keyword);
		map_search_keyword = NULL;
	}
	map_setting_destroy();
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_map_got_focus(GuiWidget *widget, void *usrdata)
{
	if(map_need_refresh)
	{
		map_need_refresh = 0;
		app_map_refresh(1);
	}
	return EVENT_TRANSFER_STOP;
}
#else
SIGNAL_HANDLER int app_map_search_list_keypress(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_map_search_list_get_total(const char* widgetname, void *usrdata)
{	
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_map_search_list_get_data(const char* widgetname, void *usrdata)
{	
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_map_sliderbar_keypress(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_map_keypress(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_map_create(const char* widgetname, void *usrdata)
{	
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_map_destroy(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_map_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#endif


