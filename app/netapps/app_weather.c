#include "app.h"
#if WEATHER_SUPPORT
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
#include <gx_weather.h>

#define WEATHER_FORECAST_DAYNUM (5)
#define WEATHER_MAX_PIC_ITEM (WEATHER_FORECAST_DAYNUM+1)
#ifdef ECOS_OS
#define WEATHER_IMAGE_PATH_PREFIX "/mnt/weather_"
#else
#define WEATHER_IMAGE_PATH_PREFIX "/tmp/weather/"
#endif
#define WEATHER_IMAGE_PATH_SUFFIX ".png"
#define WEATHER_BIG_IMG_URL_SUFFIX "-l.png"
#define WEATHER_SMALL_IMG_URL_SUFFIX "-m.png"
#ifdef ECOS_OS
#define WEATHER_DEFAULT_CITYLIST_PATH "/dvb/theme/citylist.xml"
#else
#define WEATHER_DEFAULT_CITYLIST_PATH WORK_PATH"theme/citylist.xml"
#endif
#define WEATHER_USER_CITYLIST_PATH "/home/gx/weather_usercity.xml"

#define WEATHER_SEARCH_MAX_CITY (16)
#define WEATHER_SEARCHLIST_BUFLEN (128)
#define WEATHER_IMAGE_PATH_LEN 128
#define WEATHER_PIC_URL_PREFIX "http://vortex.accuweather.com/adc2010/images/icons-numbered/"
#define WND_WEATHER 		"wnd_weather"
#define WEATHER_LANGSTR_LEN 2048
#define WEATHER_MULTI_THREAD

SIGNAL_HANDLER int app_weather_search_list_get_total(const char* widgetname, void *usrdata);

typedef void(* weather_thread_func)(void);

typedef struct
{
	int language;
	int metric;
}weather_setting_t;

typedef enum 
{
	WEATHER_SETTING_LANGUAGE=0,
	WEATHER_SETTING_TIME,
	WEATHER_SETTING_MAX,
}weather_setting_itm;

typedef enum 
{
	WEATHER_SEARCH=0,
	WEATHER_GETDATA,
}weather_api_type;

static int is_weather_download = 0;
static int weather_data_valid = 0;
static gx_weather_data_t *weather_forecast_data = NULL;
static event_list* weather_popup_timer = NULL;
static event_list* weather_picupdate_timer = NULL;
static event_list* weather_data_timer = NULL;
static int weather_data_finish = 0;
static int weather_search_finish = 0;
static gx_list_t *weather_citylist = NULL;
static gx_weather_citylist_t *weather_search_citylist = NULL;
static char *weather_search_keyword = NULL;
static int weather_current_city = 0;
static int weather_msg_on = 0;
static char weather_search_result[WEATHER_SEARCH_MAX_CITY][WEATHER_SEARCHLIST_BUFLEN];
static char *weather_lang_array = NULL;
static SystemSettingOpt  *weather_setting_opt = NULL;
static SystemSettingItem *weather_setting_item = NULL;
static weather_setting_t *weather_setting = NULL;
static gxapps_ret_t weather_ret_status = GXAPPS_SUCCESS;
static int weather_need_refresh = 0;
static unsigned int weather_data_handle = 0;
static unsigned int weather_pic_download_handle[WEATHER_MAX_PIC_ITEM];
static int weather_pic_download_abort = 0;
static int weather_pic_download_ok[WEATHER_MAX_PIC_ITEM];
static int weather_pic_show_ok[WEATHER_MAX_PIC_ITEM];

static char *weather_cf_bmp[2][2] = {
	{"weather_fah_30.bmp","weather_fah_60.bmp"},
	{"weather_deg_30.bmp","weather_deg_60.bmp"}
};

static char *weather_forecast_day[WEATHER_FORECAST_DAYNUM][5] = {
	{"text_weather_date0_week", "text_weather_date0_date", "text_weather_date0_temp",  "img_weather_small_ssd0"},
	{"text_weather_date1_week", "text_weather_date1_date", "text_weather_date1_temp",  "img_weather_small_ssd1"},
	{"text_weather_date2_week", "text_weather_date2_date", "text_weather_date2_temp",  "img_weather_small_ssd2"},
	{"text_weather_date3_week", "text_weather_date3_date", "text_weather_date3_temp",  "img_weather_small_ssd3"},
	{"text_weather_date4_week", "text_weather_date4_date", "text_weather_date4_temp",  "img_weather_small_ssd4"}
};

static char *weather_pic_img[WEATHER_MAX_PIC_ITEM] =
{
	"img_weather_info_pic",
	"img_weather_date0",
	"img_weather_date1",
	"img_weather_date2",
	"img_weather_date3",
	"img_weather_date4",
};

static char* weather_language_str[WEATHER_LAN_MAX]=
{
	"English", //WEATHER_LAN_ENGLISH
	"French", //WEATHER_LAN_FRENCH
	"German", //WEATHER_LAN_GERMAN
	"Spanish", //WEATHER_LAN_SPANISH
	"Portuguese", //WEATHER_LAN_PORTUGUESE
	"Italian", //WEATHER_LAN_ITALIAN
	"Arabic", //WEATHER_LAN_ARABIC
	"Turkish", //WEATHER_LAN_TURKISH
	"Russian", //WEATHER_LAN_RUSSIAN
	"Polish", //WEATHER_LAN_POLISH
	"Dutch", //WEATHER_LAN_DUTCH
	"Slovak", //WEATHER_LAN_SLOVAK
	"Romanian", //WEATHER_LAN_ROMANIAN
	"Czech", //WEATHER_LAN_CZECH
	"Hungarian", //WEATHER_LAN_HUNGARIAN
	"Catalan", //WEATHER_LAN_CATALAN
	"Hebrew", //WEATHER_LAN_HEBREW
	"Danish", //WEATHER_LAN_DANISH
	"Norwegian", //WEATHER_LAN_NORWEGIAN
	"Swedish", //WEATHER_LAN_SWEDISH
	"Finnish", //WEATHER_LAN_FINNISH
	"Greek", //WEATHER_LAN_GREEK
	"Korean", //WEATHER_LAN_KOREAN
	"Vietnamese", //WEATHER_LAN_VIETNAMESE
	"Thai", //WEATHER_LAN_THAI
	"Malay", //WEATHER_LAN_MALAY
	"Tagalog", //WEATHER_LAN_TAGALOG
	"Indonesian", //WEATHER_LAN_INDONESIAN
	"Slovenian", //WEATHER_LAN_SLOVENIAN
	"Ukrainian", //WEATHER_LAN_UKRAINIAN
	"Bulgarian", //WEATHER_LAN_BULGARIAN
	"Estonian", //WEATHER_LAN_ESTONIAN
	"Croatian", //WEATHER_LAN_CROTIAN
	"Kazakh", //WEATHER_LAN_KAZAKH
	"Lithuanian", //WEATHER_LAN_LITHUANIAN
	"Latvian", //WEATHER_LAN_LATVIAN
	"Macedonian", //WEATHER_LAN_MACEDONIAN
	"Serbian", //WEATHER_LAN_SERBIAN
};
#define WEATHER_LANG_NUM 	(sizeof(weather_language_str)/sizeof(char *))

#ifdef ECOS_OS
#define GIF_PATH "/dvb/theme/image/netapps/loading.gif"
#else
#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
#endif

static void weather_pic_download_stop(void);
static void app_weather_refresh_stop(void);

static void weather_thread_body(void* arg)
{
	weather_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (weather_thread_func)arg;
	if(func)
	{
		func();
	}
}

void create_weather_thread(weather_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("weather", &thread_id, weather_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static void weather_citylist_init(void)
{
	if(GXCORE_FILE_EXIST == GxCore_FileExists(WEATHER_USER_CITYLIST_PATH))
	{
		weather_citylist = gx_get_list_from_file(WEATHER_USER_CITYLIST_PATH);
	}
	if(!weather_citylist)
	{
		if(GXCORE_FILE_EXIST == GxCore_FileExists(WEATHER_DEFAULT_CITYLIST_PATH))
		{
			weather_citylist = gx_get_list_from_file(WEATHER_DEFAULT_CITYLIST_PATH);
		}
		if(!weather_citylist)
		{
			weather_citylist = gx_list_new();
		}
		gx_list_save_to_file(weather_citylist, WEATHER_USER_CITYLIST_PATH);
	}
}

static void weather_citylist_destroy(void)
{
	if(weather_citylist)
	{
		gx_list_save_to_file(weather_citylist, WEATHER_USER_CITYLIST_PATH);
		gx_list_free(weather_citylist);
		weather_citylist = NULL;
	}
}

void weather_factory_reset(void)
{
	if(GXCORE_FILE_EXIST == GxCore_FileExists(WEATHER_USER_CITYLIST_PATH))
	{
		GxCore_FileDelete(WEATHER_USER_CITYLIST_PATH);
	}
}

static void weather_search_citylist_destroy(void)
{
	if(weather_search_citylist)
	{
		gx_weather_free_citylist(weather_search_citylist);
		weather_search_citylist = NULL;
	}
}

static void app_weather_hide_gif(void)
{
	GUI_SetProperty("img_weather_gif", "state", "hide");
}


static void app_weather_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty("img_weather_gif", "load_img", GIF_PATH);
	GUI_SetProperty("img_weather_gif", "init_gif_alu_mode", &alu);
}



static void app_weather_tip_hide(void)
{
	weather_msg_on = 0;
	GUI_SetProperty("img_weather_tip_bk", "state", "hide");
	GUI_SetProperty("text_weather_tip_status", "state", "hide");
	if(weather_popup_timer)
	{
		remove_timer(weather_popup_timer);
		weather_popup_timer = NULL;
	}
	GUI_SetProperty(WND_WEATHER, "update", NULL);
}

static int weather_setting_change_callback(int sel)
{	
	return EVENT_TRANSFER_KEEPON;
}
static int weather_setting_cmb_press_callback(int key)
{
	int cmb_sel = -1;
	PopList pop_list;
	if(key == STBK_OK)
	{
		GUI_GetProperty("cmb_system_setting_opt1", "select", &cmb_sel);
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = "Language";
		pop_list.item_num = WEATHER_LANG_NUM;
		pop_list.item_content = weather_language_str;
		pop_list.sel = cmb_sel;
		pop_list.show_num= false;
		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt1", "select", &cmb_sel);

	}
	return EVENT_TRANSFER_KEEPON;

}

static void weather_setting_init(void)
{
	weather_setting_opt=(SystemSettingOpt *)GxCore_Calloc(1, sizeof(SystemSettingOpt));
	weather_setting_item=(SystemSettingItem *)GxCore_Calloc(1, sizeof(SystemSettingItem)*WEATHER_SETTING_MAX);
	weather_setting=(weather_setting_t *)GxCore_Calloc(1, sizeof(weather_setting_t));
	weather_lang_array = GxCore_Calloc(1, WEATHER_LANGSTR_LEN);
}

static void weather_setting_destroy(void)
{
	if(weather_setting_opt)
	{
		GxCore_Free(weather_setting_opt);
		weather_setting_opt=NULL;
	}

	
	if(weather_setting_item)
	{
		GxCore_Free(weather_setting_item);
		weather_setting_item=NULL;
	}
	
	if(weather_setting)
	{
		GxCore_Free(weather_setting);
		weather_setting=NULL;
	}

	if(weather_lang_array)
	{
		GxCore_Free(weather_lang_array);
		weather_lang_array=NULL;
	}
}

static void weather_setting_get(void)
{
	int value = 0;
	if(!weather_setting)
	{
		return;
	}

	GxBus_ConfigGetInt(WEATHER_LANGUAGE_KEY, &value, WEATHER_DEFAULT_LANGUAGE);
	if((value<0)||(value>=WEATHER_LAN_MAX))
	{
		value = 0;
	}
	weather_setting->language= value;

	GxBus_ConfigGetInt(WEATHER_METRIC_KEY, &value, WEATHER_DEFAULT_METRIC);
	if((value<0)||(value>METRIC_CELSIUS))
	{
		value = 0;
	}
	weather_setting->metric = value;
}

static int weather_setting_save(weather_setting_t *ret_para)
{	
	int value = 0;   
	
	value = ret_para->language;
	if((value<0)||(value>=WEATHER_LAN_MAX))
	{
		value = 0;
	}
	GxBus_ConfigSetInt(WEATHER_LANGUAGE_KEY,value);
	weather_setting->language= value;
	
	value = ret_para->metric;
	if((value<0)||(value>METRIC_CELSIUS))
	{
		value = 0;
	}
	GxBus_ConfigSetInt(WEATHER_METRIC_KEY,value);
	weather_setting->metric= value;
	
	return 0;
}

static void weather_currentcity_get(void)
{
	GxBus_ConfigGetInt(WEATHER_CURRENTCITY_KEY, &weather_current_city, WEATHER_DEFAULT_CURCITY);
	if(weather_citylist->item_num<=0)
	{
		weather_current_city = 0;
	}
	else if(weather_current_city>=weather_citylist->item_num)
	{
		weather_current_city = weather_citylist->item_num-1;
	}
}
static void weather_currentcity_save(void)
{
	GxBus_ConfigSetInt(WEATHER_CURRENTCITY_KEY,weather_current_city);
}
static void weather_setting_all(void)
{
	if(weather_setting)
	{
		gx_weather_set_language(weather_setting->language);
		gx_weather_set_metric(weather_setting->metric);
	}
}

static void weather_setting_result_get(weather_setting_t *ret_para)
{    
	ret_para->language= weather_setting_item[WEATHER_SETTING_LANGUAGE].itemProperty.itemPropertyCmb.sel;
	ret_para->metric = weather_setting_item[WEATHER_SETTING_TIME].itemProperty.itemPropertyCmb.sel;
}

static int weather_setting_change(weather_setting_t *para)
{	
	if((weather_setting->language == para->language)
		&&(weather_setting->metric == para->metric))
	{		
		return 0;	
	}
	else
	{
		return 1;
	}
}

static int weather_setting_exit_cb(ExitType Type)
{
	weather_setting_t para;
	int ret = 0;

	weather_setting_result_get(&para);
	if(weather_setting_change(&para))
	{
		PopDlg  pop;
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_YES_NO;
		pop.str = STR_ID_SAVE_INFO;
                if(popdlg_create(&pop) == POP_VAL_OK)
		{
			weather_setting_save(&para);
			weather_setting_all();
			ret = 1;
		}
	}
	return ret;
}

static void weather_setting_item_init()
{
	int i = 0;
	
	memset(weather_lang_array, 0, WEATHER_LANGSTR_LEN);
	strcpy(weather_lang_array, "[");
	for(i=0; i<WEATHER_LANG_NUM; i++)
	{
		strcat(weather_lang_array, weather_language_str[i]);
		if(i == (WEATHER_LANG_NUM-1))
		{
			strcat(weather_lang_array, "]");
		}
		else
		{
			strcat(weather_lang_array, ",");
		}
	}
	
	weather_setting_item[WEATHER_SETTING_LANGUAGE].itemTitle = "Language";
	weather_setting_item[WEATHER_SETTING_LANGUAGE].itemType = ITEM_CHOICE;
	weather_setting_item[WEATHER_SETTING_LANGUAGE].itemProperty.itemPropertyCmb.content= weather_lang_array;
	weather_setting_item[WEATHER_SETTING_LANGUAGE].itemProperty.itemPropertyCmb.sel = weather_setting->language;
	weather_setting_item[WEATHER_SETTING_LANGUAGE].itemCallback.cmbCallback.CmbChange= weather_setting_change_callback;
	weather_setting_item[WEATHER_SETTING_LANGUAGE].itemCallback.cmbCallback.CmbPress= weather_setting_cmb_press_callback;
	weather_setting_item[WEATHER_SETTING_LANGUAGE].itemStatus = ITEM_NORMAL;

	weather_setting_item[WEATHER_SETTING_TIME].itemTitle = "Metric";
	weather_setting_item[WEATHER_SETTING_TIME].itemType = ITEM_CHOICE;
	weather_setting_item[WEATHER_SETTING_TIME].itemProperty.itemPropertyCmb.content= "[Fahrenheit,Celsius]";
	weather_setting_item[WEATHER_SETTING_TIME].itemProperty.itemPropertyCmb.sel = weather_setting->metric;
	weather_setting_item[WEATHER_SETTING_TIME].itemCallback.cmbCallback.CmbChange= weather_setting_change_callback;
	weather_setting_item[WEATHER_SETTING_TIME].itemCallback.cmbCallback.CmbPress= NULL;//weather_setting_cmb_press_callback;
	weather_setting_item[WEATHER_SETTING_TIME].itemStatus = ITEM_NORMAL;	

}

static void weather_create_setting_menu(void)
{
	weather_need_refresh = 1;
	app_weather_refresh_stop();
	weather_pic_download_stop();
	memset(weather_setting_opt, 0 ,sizeof(SystemSettingOpt));		
	memset(weather_setting_item, 0 ,sizeof(SystemSettingItem)*WEATHER_SETTING_MAX);	
	memset(weather_setting, 0 ,sizeof(weather_setting_t));	
	
	weather_setting_get();
	weather_setting_item_init();
	weather_setting_opt->menuTitle = "Weather Setting";
	weather_setting_opt->itemNum = WEATHER_SETTING_MAX;
	weather_setting_opt->item = weather_setting_item;
	weather_setting_opt->exit = weather_setting_exit_cb;
	app_system_set_create(weather_setting_opt);
}


static status_t weather_pic_update(int index, char *pic_url)
{
	char* focus_win = NULL;

	if((index >= WEATHER_MAX_PIC_ITEM) || (index < 0))
		return GXCORE_ERROR;
	
	focus_win = (char*)GUI_GetFocusWindow();
	if(!focus_win) 
		return GXCORE_ERROR;
	if(strcasecmp(focus_win, WND_WEATHER))
		return GXCORE_ERROR;
	
	if(pic_url)
	{
		GUI_SetProperty(weather_pic_img[index], "load_scal_img", pic_url);
		GUI_SetProperty(weather_pic_img[index], "state", "show");
	}
	else
	{
		GUI_SetProperty(weather_pic_img[index], "state", "hide");
	}

	return GXCORE_SUCCESS;
}

void app_weather_clear_all(void)
{
	int i = 0;

	GUI_SetProperty("img_weather_info_city_arr_l", "state", "hide");
	GUI_SetProperty("img_weather_info_city_arr_r", "state", "hide");
	GUI_SetProperty("text_weather_info_city", "state", "hide");
	GUI_SetProperty("text_weather_info_description", "state", "hide");
	GUI_SetProperty("text_weather_info_temp", "state", "hide");
	GUI_SetProperty("text_weather_info_publish", "state", "hide");
	GUI_SetProperty("img_weather_big_ssd", "state", "hide");

	GUI_SetProperty("text_weather_info_humidity", "state", "hide");
	GUI_SetProperty("text_weather_info_visibility", "state", "hide");
	GUI_SetProperty("text_weather_info_wind", "state", "hide");
	GUI_SetProperty("text_weather_info_pressure", "state", "hide");
	GUI_SetProperty("text_weather_info0", "state", "hide");
	GUI_SetProperty("text_weather_info1", "state", "hide");
	GUI_SetProperty("text_weather_info2", "state", "hide");
	GUI_SetProperty("text_weather_info3", "state", "hide");

	for(i = 0; i < WEATHER_FORECAST_DAYNUM; i++)
	{
		GUI_SetProperty(weather_forecast_day[i][0], "state", "hide");
		GUI_SetProperty(weather_forecast_day[i][1], "state", "hide");
		GUI_SetProperty(weather_forecast_day[i][2], "state", "hide");
		GUI_SetProperty(weather_forecast_day[i][3], "state", "hide");
	}
	for(i = 0; i < WEATHER_MAX_PIC_ITEM; i++)
	{
		GUI_SetProperty(weather_pic_img[i], "state", "hide");
		GUI_SetProperty(weather_pic_img[i], "state", "hide");
		GUI_SetProperty(weather_pic_img[i], "state", "hide");
		GUI_SetProperty(weather_pic_img[i], "state", "hide");
	}
	GUI_SetProperty(WND_WEATHER, "update", NULL);
}


static int weather_popup_timer_timeout(void *userdata)
{
	app_weather_tip_hide();
	return 0;
}

static int weather_pic_update_timer_timeout(void *userdata)
{
	int i=0;
	bool b_all_ok = true;
	char image_path[WEATHER_IMAGE_PATH_LEN];
	
	if((!weather_forecast_data)||(weather_forecast_data->forecast_daynum<=0))
	{
		return 0;
	}
	
	for(i = 0; i < WEATHER_MAX_PIC_ITEM; i++)
	{
		if(weather_pic_show_ok[i] == 1)
		{
			continue;
		}
		b_all_ok = false;
		if(weather_pic_download_ok[i] == 1)
		{
			snprintf(image_path,sizeof(image_path),"%s%d%s",WEATHER_IMAGE_PATH_PREFIX,i, WEATHER_IMAGE_PATH_SUFFIX);
			if(GXCORE_FILE_UNEXIST != GxCore_FileExists(image_path))
			{
				weather_pic_update(i, image_path);
			}
			else
			{
				weather_pic_update(i, NULL);
			}
			weather_pic_show_ok[i] = 1;
		}
	}

	if(b_all_ok)
	{
		app_weather_hide_gif();
		GUI_SetInterface("flush",NULL);
		timer_stop(weather_picupdate_timer);
		weather_picupdate_timer = NULL;
	}

	return 0;
}

static void weather_update_timer_stop(void)
{
	if(weather_picupdate_timer)
	{
		remove_timer(weather_picupdate_timer);
		weather_picupdate_timer = NULL;
	}
}

static void weather_update_timer_reset(void)
{
	if (reset_timer(weather_picupdate_timer) != 0)
	{
		weather_picupdate_timer = create_timer(weather_pic_update_timer_timeout, 300, NULL, TIMER_REPEAT);
	}
}

static void weather_delete_files(void)
{
	int i=0;
	char image_path[WEATHER_IMAGE_PATH_LEN];

	for(i=0; i<WEATHER_MAX_PIC_ITEM; i++)
	{
		snprintf(image_path,sizeof(image_path),"%s%d%s",WEATHER_IMAGE_PATH_PREFIX,i, WEATHER_IMAGE_PATH_SUFFIX);
		if(GXCORE_FILE_EXIST == GxCore_FileExists(image_path))
		{
			GxCore_FileDelete(image_path);
		}
	}
}

static void weather_pic_download_stop(void)
{
	int i=0;

	weather_update_timer_stop();
	memset(&weather_pic_download_ok, 0, sizeof(weather_pic_download_ok));
	memset(&weather_pic_show_ok, 0, sizeof(weather_pic_show_ok));
	weather_pic_download_abort= 1;
	for(i=0; i<WEATHER_MAX_PIC_ITEM; i++)
	{
		if(weather_pic_download_handle[i]!=0)
		{
			gxapps_curl_async_abort(weather_pic_download_handle[i]);
			weather_pic_download_handle[i] = 0;
		}
	}
	weather_delete_files();
	weather_pic_download_abort= 0;
}

#ifdef WEATHER_MULTI_THREAD
static void weahter_download_pic_cb(int message, int body)
{
	gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;
	unsigned int handle = 0;
	int i = 0;
	
	if(message == GXCURL_STATE_COMPLETE)
	{
		if(weather_pic_download_abort)
			return;
		handle = callback_data->handle;
		if(callback_data->handle>0)
		{
			for(i=0;i<WEATHER_MAX_PIC_ITEM;i++)
			{
				if(weather_pic_download_handle[i] ==callback_data->handle)
				{
					weather_pic_download_ok[i] = 1;
					break;
				}
			}
		}
	}
}

static void weather_pic_download_start(void)
{
	int i=0;
	char image_path[WEATHER_IMAGE_PATH_LEN];
	char image_url[WEATHER_IMAGE_PATH_LEN];

	weather_pic_download_stop();
	weather_update_timer_reset();
	if(weather_forecast_data&&(weather_forecast_data->forecast_daynum>0))
	{
		for(i=0; i<WEATHER_MAX_PIC_ITEM; i++)
		{
			if(weather_pic_download_abort == 0)
			{
				if((i==0)&&(weather_forecast_data->currentcondition->weathericon))
				{
					snprintf(image_path,sizeof(image_path),"%s%d%s",WEATHER_IMAGE_PATH_PREFIX,0, WEATHER_IMAGE_PATH_SUFFIX);
					snprintf(image_url,sizeof(image_url),"%s%s%s",WEATHER_PIC_URL_PREFIX,weather_forecast_data->currentcondition->weathericon, WEATHER_BIG_IMG_URL_SUFFIX);
					weather_pic_download_handle[i] = gxapps_curl_async_download(image_url, image_path, weahter_download_pic_cb);
				}
				else if((i>0)&&(weather_forecast_data->forecast_data[i-1].weathericon))
				{
					snprintf(image_path,sizeof(image_path),"%s%d%s",WEATHER_IMAGE_PATH_PREFIX,i, WEATHER_IMAGE_PATH_SUFFIX);
					snprintf(image_url,sizeof(image_url),"%s%s%s",WEATHER_PIC_URL_PREFIX,weather_forecast_data->forecast_data[i-1].weathericon, WEATHER_SMALL_IMG_URL_SUFFIX);
					weather_pic_download_handle[i] = gxapps_curl_async_download(image_url, image_path, weahter_download_pic_cb);
				}
				else
				{
					weather_pic_download_ok[i] = 1;
				}
			}
		}
	}
}
#else
static void weahter_download_pic_cb(int message, int body);
#ifdef ECOS_OS
extern int app_check_wifi_status(void);
#endif
static void weather_download_pic(int index)
{
	char image_path[WEATHER_IMAGE_PATH_LEN];
	char image_url[WEATHER_IMAGE_PATH_LEN];
	if((weather_pic_download_abort == 0)&&(index>=0)&&(index<WEATHER_MAX_PIC_ITEM))
	{
	#ifdef ECOS_OS
		if(app_check_wifi_status()<=0)
		{
			weather_pic_download_ok[index] = 1;
			weather_download_pic(index+1);
			return;
		}
	#endif
		if((index==0)&&(weather_forecast_data->currentcondition->weathericon))
		{
			snprintf(image_path,sizeof(image_path),"%s%d%s",WEATHER_IMAGE_PATH_PREFIX,0, WEATHER_IMAGE_PATH_SUFFIX);
			snprintf(image_url,sizeof(image_url),"%s%s%s",WEATHER_PIC_URL_PREFIX,weather_forecast_data->currentcondition->weathericon, WEATHER_BIG_IMG_URL_SUFFIX);
			weather_pic_download_handle[index] = gxapps_curl_async_download(image_url, image_path, weahter_download_pic_cb);
		}
		else if((index>0)&&(weather_forecast_data->forecast_data[index-1].weathericon))
		{
			snprintf(image_path,sizeof(image_path),"%s%d%s",WEATHER_IMAGE_PATH_PREFIX,index, WEATHER_IMAGE_PATH_SUFFIX);
			snprintf(image_url,sizeof(image_url),"%s%s%s",WEATHER_PIC_URL_PREFIX,weather_forecast_data->forecast_data[index-1].weathericon, WEATHER_SMALL_IMG_URL_SUFFIX);
			weather_pic_download_handle[index] = gxapps_curl_async_download(image_url, image_path, weahter_download_pic_cb);
		}
		else
		{
			weather_pic_download_ok[index] = 1;
			weather_download_pic(index+1);
		}
	}
}
static void weahter_download_pic_cb(int message, int body)
{
	gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;
	unsigned int handle = 0;
	int i = 0;
	
	if(message == GXCURL_STATE_COMPLETE)
	{
		if(weather_pic_download_abort)
			return;
		handle = callback_data->handle;
		if(callback_data->handle>0)
		{
			for(i=0;i<WEATHER_MAX_PIC_ITEM;i++)
			{
				if(weather_pic_download_handle[i] ==callback_data->handle)
				{
					weather_pic_download_ok[i] = 1;
					weather_download_pic(i+1);
					break;
				}
			}
		}
	}
}

static void weather_pic_download_start(void)
{
	weather_pic_download_stop();
	weather_update_timer_reset();
	if(weather_forecast_data&&(weather_forecast_data->forecast_daynum>0))
	{
		weather_download_pic(0);		
	}
}
#endif

static void app_weather_tip_show(char *buf, int time_out)
{
	weather_msg_on = 1;
	GUI_SetProperty("img_weather_tip_bk", "state", "show");
	GUI_SetProperty("text_weather_tip_status", "string", buf);
	GUI_SetProperty("text_weather_tip_status", "state", "show");
	if(time_out > 0)
	{
		if (reset_timer(weather_popup_timer) != 0)
		{
			weather_popup_timer = create_timer(weather_popup_timer_timeout, time_out, NULL, TIMER_ONCE);
		}
	}
}

static void app_weather_show_errormsg(int type)
{
	if(weather_ret_status==GXAPPS_SERVER_ERROR)
	{
		if(type == WEATHER_GETDATA)
		{
			app_weather_tip_show("No data for this city!", 1000);
		}
		else
		{
			app_weather_tip_show("Can not find this city!", 1000);
		}
	}
	else if(weather_ret_status==GXAPPS_NETWORK_ERROR)
	{
		app_weather_tip_show("Network error!", 1000);
	}
	else if(weather_ret_status==GXAPPS_INTERNAL_ERROR)
	{
		app_weather_tip_show("Internal error!", 1000);
	}
	else if(weather_ret_status==GXAPPS_UNKNOWN_ERROR)
	{
		app_weather_tip_show("Unknown error!", 1000);
	}
}

static void weather_show_init_city_info(void)
{
	char buf[128];
	
	if((weather_citylist->item_num>0)&&(weather_current_city<weather_citylist->item_num))
	{
		snprintf(buf, sizeof(buf), "(%d/%d) %s", weather_current_city+1, weather_citylist->item_num, weather_citylist->items[weather_current_city].key);
		GUI_SetProperty("text_weather_info_city", "string", buf);
		GUI_SetProperty("text_weather_info_city", "state", "show");
		GUI_SetProperty("img_weather_info_city_arr_l", "state", "show");
		GUI_SetProperty("img_weather_info_city_arr_r", "state", "show");
	}
}

static void app_weather_info_update(void)
{
	int i = 0;
	char buf[128];

	snprintf(buf, sizeof(buf), "(%d/%d) %s,%s,%s", weather_current_city+1, weather_citylist->item_num, weather_forecast_data->location->city, weather_forecast_data->location->province, weather_forecast_data->location->country);
	GUI_SetProperty("text_weather_info_city", "string", buf);
	GUI_SetProperty("text_weather_info_city", "state", "show");
	GUI_SetProperty("text_weather_info_description", "string", weather_forecast_data->currentcondition->weathertext);
	GUI_SetProperty("text_weather_info_description", "state", "show");
	GUI_SetProperty("text_weather_info_temp", "string", weather_forecast_data->currentcondition->temperature);
	GUI_SetProperty("text_weather_info_temp", "state", "show");
	GUI_SetProperty("text_weather_info_publish", "string", weather_forecast_data->currentcondition->observationtime);
	GUI_SetProperty("text_weather_info_publish", "state", "show");
	GUI_SetProperty("img_weather_big_ssd", "img", weather_cf_bmp[weather_setting->metric][1]);
	GUI_SetProperty("img_weather_big_ssd", "state", "show");

	GUI_SetProperty("text_weather_info0", "state", "show");
	GUI_SetProperty("text_weather_info1", "state", "show");
	GUI_SetProperty("text_weather_info2", "state", "show");
	GUI_SetProperty("text_weather_info3", "state", "show");
	GUI_SetProperty("text_weather_info_humidity", "string", weather_forecast_data->currentcondition->humidity);
	GUI_SetProperty("text_weather_info_humidity", "state", "show");
	if(weather_setting->metric == 1)
	{
		snprintf(buf, sizeof(buf), "%skm", weather_forecast_data->currentcondition->visibility);
	}
	else
	{
		 snprintf(buf, sizeof(buf), "%smi", weather_forecast_data->currentcondition->visibility);
	}
	GUI_SetProperty("text_weather_info_visibility", "string", buf);
	GUI_SetProperty("text_weather_info_visibility", "state", "show");
	if(isalpha(weather_forecast_data->currentcondition->winddirection[0]))
	{
		if(weather_setting->metric == 1)
		{
			snprintf(buf, sizeof(buf), "%s at %skm/h", weather_forecast_data->currentcondition->winddirection,weather_forecast_data->currentcondition->windspeed);
		}
		else
		{
			snprintf(buf, sizeof(buf), "%s at %smi/h", weather_forecast_data->currentcondition->winddirection,weather_forecast_data->currentcondition->windspeed);
		}
	}
	else
	{
		if(weather_setting->metric == 1)
		{
			snprintf(buf, sizeof(buf), "%s%c  at %skm/h", weather_forecast_data->currentcondition->winddirection,0xb0, weather_forecast_data->currentcondition->windspeed);
		}
		else
		{
			snprintf(buf, sizeof(buf), "%s%c  at %smi/h", weather_forecast_data->currentcondition->winddirection,0xb0, weather_forecast_data->currentcondition->windspeed);
		}
	}
	GUI_SetProperty("text_weather_info_wind", "string", buf);
	GUI_SetProperty("text_weather_info_wind", "state", "show");
	if(weather_setting->metric == 1)
	{
		snprintf(buf, sizeof(buf), "%skpa", weather_forecast_data->currentcondition->pressure);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%sinHg", weather_forecast_data->currentcondition->pressure);
	}
	GUI_SetProperty("text_weather_info_pressure", "string", buf);
	GUI_SetProperty("text_weather_info_pressure", "state", "show");
	
	for(i = 0; i < WEATHER_FORECAST_DAYNUM; i++)
	{
		GUI_SetProperty(weather_forecast_day[i][0], "string", weather_forecast_data->forecast_data[i].weekday);
		GUI_SetProperty(weather_forecast_day[i][0], "state", "show");
		GUI_SetProperty(weather_forecast_day[i][1], "string", weather_forecast_data->forecast_data[i].obsdate);
		GUI_SetProperty(weather_forecast_day[i][1], "state", "show");
		snprintf(buf, sizeof(buf), "%s~%s", weather_forecast_data->forecast_data[i].lowtemperature,weather_forecast_data->forecast_data[i].hightemperature);
		GUI_SetProperty(weather_forecast_day[i][2], "string", buf);
		GUI_SetProperty(weather_forecast_day[i][2], "state", "show");
		GUI_SetProperty(weather_forecast_day[i][3], "img", weather_cf_bmp[weather_setting->metric][0]);
		GUI_SetProperty(weather_forecast_day[i][3], "state", "show");
	}
}

static void app_weather_refresh_stop(void)
{
	weather_data_finish = 0;
}
static void app_weather_refresh_start(void)
{
	weather_data_finish = 1;
}

static int weather_refresh_timeout(void)
{	
	if(weather_data_valid)	
	{		
		app_weather_tip_hide();		
		app_weather_info_update();		
		weather_pic_download_start();	
	}	
	else	
	{		
		app_weather_show_errormsg(WEATHER_GETDATA);    	
	}	
	app_weather_refresh_stop();	
	return 0;
}

static void app_weather_search_refresh_stop(void)
{
	weather_search_finish = 0;
}
static void app_weather_search_refresh_start(void)
{
	weather_search_finish = 1;
}

static void app_weather_search_show(void)
{
    int sel = 0;
	GUI_SetProperty("img_weather_search_bk", "state", "show");
	GUI_SetProperty("text_weather_search_title", "state", "show");
	GUI_SetProperty("list_weather_search", "state", "show");
	GUI_SetFocusWidget("list_weather_search");
    GUI_SetProperty("list_weather_search", "select", &sel);
	GUI_SetProperty("list_weather_search", "update_all", NULL);
}
static void app_weather_search_hide(void)
{
	GUI_SetProperty("img_weather_search_bk", "state", "hide");
	GUI_SetProperty("text_weather_search_title", "state", "hide");
	GUI_SetProperty("list_weather_search", "state", "hide");
}

static int weather_search_timeout(void)
{
	if(weather_search_citylist&&(weather_search_citylist->cities)&&(weather_search_citylist->city_num>0))
	{
		app_weather_tip_hide();
		app_weather_search_show();
	}
	else
	{
		app_weather_show_errormsg(WEATHER_SEARCH);    
	}
	app_weather_search_refresh_stop();
	return 0;
}

static void app_weather_data_timer_stop(void)
{
	if(weather_data_timer)
	{
		remove_timer(weather_data_timer);
		weather_data_timer = NULL;
	}
}

static int weather_data_timer_timeout(void *usrdata)
{
	if(GUI_CheckDialog("wnd_pop_book") != GXCORE_SUCCESS)
	{
		if(weather_search_finish)
		{
			weather_search_timeout();
		}
		if(weather_data_finish)
		{
			weather_refresh_timeout();
		}
	}
	return 0;
}

static void app_weather_data_timer_start(void)
{
	app_weather_data_timer_stop();
	weather_data_timer = create_timer(weather_data_timer_timeout, 10, NULL, TIMER_REPEAT);
}


static int app_weather_busy(void)
{
	return is_weather_download;
}

static void weather_free_forecastdata(void)
{
	if(weather_forecast_data)
	{
		gx_weather_free_data(weather_forecast_data);
		weather_forecast_data = NULL;
	}
}

static void weather_start_get_data(void)
{
	unsigned int handle = 0;
	gx_weather_data_t *data = NULL;
	gxapps_ret_t ret = GXAPPS_SUCCESS;
	
	is_weather_download = 1;
	weather_data_handle++;
	handle = weather_data_handle;
	ret = gx_weather_get_data(weather_citylist->items[weather_current_city].value, &data);
	if(handle == weather_data_handle)
	{
		weather_ret_status = ret;
		if(weather_ret_status == GXAPPS_SUCCESS)
		{
			weather_data_valid = 1;
			weather_forecast_data = data;		
		}
		app_weather_refresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_weather_free_data(data);
		}
	}
	is_weather_download = 0;
}

static void weather_start_search_city(void)
{
	unsigned int handle = 0;
	gx_weather_citylist_t *citilist = NULL;
	gxapps_ret_t ret = GXAPPS_SUCCESS;
	
	is_weather_download = 1;
	weather_data_handle++;
	handle = weather_data_handle;
	weather_search_citylist_destroy();
	ret = gx_weather_search_city(weather_search_keyword, &citilist);
	if(handle == weather_data_handle)
	{
		weather_ret_status = ret;
		if(weather_ret_status == GXAPPS_SUCCESS)
		{
			weather_search_citylist = citilist;		
		}
		app_weather_search_refresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_weather_free_citylist(citilist);
		}
	}
	is_weather_download = 0;
}

static void app_weather_update(void)
{
	if(weather_citylist->item_num <= 0)
	{
		app_weather_tip_show("No City!", 1000);    
		return;
	}
	if(weather_current_city>=weather_citylist->item_num)
	{
		weather_current_city = weather_citylist->item_num-1;
	}
	weather_data_valid = 0;
	weather_free_forecastdata();
	weather_show_init_city_info();
	app_weather_tip_show("Updating...", 0);
	GUI_SetInterface("flush",NULL);
	if(app_weather_busy())
	{
		app_weather_tip_show("System Busy, Please Wait!", 1000);    
	}
	else
	{
		create_weather_thread(weather_start_get_data);
	}
}

static void app_weather_refresh(void)
{
	app_weather_refresh_stop();
	weather_pic_download_stop();
	app_weather_clear_all();
	weather_currentcity_save();
	app_weather_update();
}

static int app_weather_is_city_exist(char *cityid)
{
	int exist = 0;
	int i = 0;

	if(weather_citylist&&(weather_citylist->item_num>0)&&cityid)
	{
		for(i=0; i<weather_citylist->item_num; i++)
		{
			if(0 == strcmp(weather_citylist->items[i].value, cityid))
			{
				exist = 1;
				break;
			}
		}
	}
	return exist;
}

void weather_search_full_keyboard_proc(PopKeyboard *data)
{
	 if(data->in_ret == POP_VAL_CANCEL)
        	return;

   	if(data->out_name == NULL)
       	     return;

	if(weather_search_keyword)
	{
		free(weather_search_keyword);
		weather_search_keyword = NULL;
	}
	weather_search_keyword = strdup(data->out_name);
	app_weather_tip_show("Searching...", 0);
	GUI_SetInterface("flush",NULL);
	if(app_weather_busy())
	{
		app_weather_tip_show("System Busy, Please Wait!", 1000);    
	}
	else
	{
		create_weather_thread(weather_start_search_city);
	}
}

static int app_weather_delete_cb(PopDlgRet ret)
{
	if(ret == POP_VAL_OK)
	{
		gx_list_delete_item(weather_citylist, weather_current_city);
		gx_list_save_to_file(weather_citylist, WEATHER_USER_CITYLIST_PATH);
		if(weather_citylist->item_num<=0)
		{
			weather_current_city = 0;
		}
		else if(weather_current_city>=weather_citylist->item_num)
		{
			weather_current_city = weather_citylist->item_num-1;
		}
		app_weather_refresh();
	}
	return 0;
}

SIGNAL_HANDLER int app_weather_search_list_keypress(const char* widgetname, void *usrdata)
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
				app_weather_search_hide();
				break;
			case STBK_OK:
				{
					int sel = 0;
					GUI_GetProperty("list_weather_search", "select", &sel);
					if(app_weather_is_city_exist(weather_search_citylist->cities[sel].cityid))
					{
						app_weather_search_hide();
						app_weather_tip_show("City already exist!", 1000);    
					}
					else
					{
						gx_list_add_item(weather_citylist, weather_search_citylist->cities[sel].city, weather_search_citylist->cities[sel].cityid, NULL);
						gx_list_save_to_file(weather_citylist, WEATHER_USER_CITYLIST_PATH);
						app_weather_search_hide();
						weather_current_city = weather_citylist->item_num-1;
						app_weather_refresh();
					}
				}
				break;
            case STBK_PAGE_UP:
                {
                    int sel = 0;
                    GUI_GetProperty("list_weather_search", "select", &sel);
                    if(0 == sel)
                    {
                        sel = app_weather_search_list_get_total(NULL, NULL) - 1;
                        GUI_SetProperty("list_weather_search", "select", &sel);

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
                    GUI_GetProperty("list_weather_search", "select", &sel);
                    if(sel == (app_weather_search_list_get_total(NULL, NULL) - 1))
                    {
                        sel = 0;
                        GUI_SetProperty("list_weather_search", "select", &sel);

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

SIGNAL_HANDLER int app_weather_search_list_get_total(const char* widgetname, void *usrdata)
{
	int ret = 0;
	if(weather_search_citylist&&(weather_search_citylist->city_num>0))
	{
		if(weather_search_citylist->city_num>WEATHER_SEARCH_MAX_CITY)
		{
			ret = WEATHER_SEARCH_MAX_CITY;
		}
		else
		{
			ret = weather_search_citylist->city_num;
		}
	}
	return ret;
}
SIGNAL_HANDLER int app_weather_search_list_get_data(const char* widgetname, void *usrdata)
{	
	ListItemPara* item = NULL;
	item = (ListItemPara*)usrdata;
	if(NULL == item) 	
		return GXCORE_ERROR;
	if((item->sel<0)||(item->sel>=WEATHER_SEARCH_MAX_CITY)
		||(!weather_search_citylist)||(weather_search_citylist->city_num<=0)
		||(item->sel>=weather_search_citylist->city_num))
		return GXCORE_ERROR;
	snprintf(weather_search_result[item->sel], WEATHER_SEARCHLIST_BUFLEN, "%s,%s,%s",
		weather_search_citylist->cities[item->sel].city,
		weather_search_citylist->cities[item->sel].province,
		weather_search_citylist->cities[item->sel].country);
	item->string = weather_search_result[item->sel];
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_weather_keypress(const char* widgetname, void *usrdata)
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
				if(weather_msg_on)
				{
					weather_data_handle++;
					app_weather_tip_hide();
				}
				else
				{
					GUI_EndDialog(WND_WEATHER);
				}
				break;
			case STBK_LEFT:
				if((weather_msg_on == 0)&&(weather_citylist->item_num>0))
				{
					if(app_weather_busy())
					{
						app_weather_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						if(weather_current_city == 0)
						{
							weather_current_city = weather_citylist->item_num-1;
						}
						else
						{
							weather_current_city--;
						}
						app_weather_refresh();
					}
				}
				break;
			case STBK_RIGHT:
				if((weather_msg_on == 0)&&(weather_citylist->item_num>0))
				{
					if(app_weather_busy())
					{
						app_weather_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						if(weather_current_city == weather_citylist->item_num-1)
						{
							weather_current_city = 0;
						}
						else
						{
							weather_current_city++;
						}
						app_weather_refresh();
					}
				}
				break;	
			case STBK_RED:
				if((weather_msg_on == 0)&&(weather_citylist->item_num>0)&&(weather_current_city<weather_citylist->item_num))
				{
					if(app_weather_busy())
					{
						app_weather_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						app_weather_refresh();
					}
				}
				break;
			case STBK_GREEN:
				if(weather_msg_on==0)
				{
					app_weather_refresh_stop();
					weather_pic_download_stop();
					static PopKeyboard keyboard;
					memset(&keyboard, 0, sizeof(PopKeyboard));
					keyboard.in_name    = NULL;
					keyboard.max_num = 120;
					keyboard.out_name   = NULL;
					keyboard.change_cb  = NULL;
					keyboard.release_cb = weather_search_full_keyboard_proc;
					keyboard.usr_data   = NULL;
					keyboard.pos.x = 500;
					multi_language_keyboard_create(&keyboard);
				}
				break;
			case STBK_YELLOW:
				if((weather_msg_on==0)&&(weather_citylist->item_num>0))
				{
					if(app_weather_busy())
					{
						app_weather_tip_show("System Busy, Please Wait!", 1000);    
					}
					else
					{
						PopDlg  pop;
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_YES_NO;
	        				pop.format = POP_FORMAT_DLG;
	       				pop.str = "Sure to delete this city?";
	       				pop.title = "Delete City";
						pop.mode = POP_MODE_UNBLOCK;
						pop.exit_cb = app_weather_delete_cb;
						//pop.default_ret = POP_VAL_CANCEL;
						app_weather_refresh_stop();
						weather_pic_download_stop();
						popdlg_create(&pop);
					}
				}
				break;
			case STBK_BLUE:
				if(weather_msg_on==0)
				{
					weather_create_setting_menu();
				}
				break;
			default:
				break;
		}		
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_weather_create(const char* widgetname, void *usrdata)
{
	app_weather_data_timer_start();
	app_weather_load_gif();
	weather_citylist_init();
	weather_currentcity_get();
	weather_msg_on = 0;
	weather_setting_init();
	weather_setting_get();
	weather_setting_all();
	app_weather_clear_all();
	app_weather_update();
	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_weather_destroy(const char* widgetname, void *usrdata)
{
    is_weather_download = 0;
	weather_data_handle++;
	app_weather_refresh_stop();
	app_weather_search_refresh_stop();
	app_weather_data_timer_stop();
	app_weather_tip_hide();
	weather_pic_download_stop();
	weather_free_forecastdata();
	weather_citylist_destroy();
	weather_search_citylist_destroy();
	if(weather_search_keyword)
	{
		free(weather_search_keyword);
		weather_search_keyword = NULL;
	}
	weather_setting_destroy();
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_weather_got_focus(GuiWidget *widget, void *usrdata)
{
	if(weather_need_refresh)
	{
		weather_need_refresh = 0;
		app_weather_refresh();
	}
	return EVENT_TRANSFER_STOP;
}
#else
SIGNAL_HANDLER int app_weather_search_list_keypress(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_weather_search_list_get_total(const char* widgetname, void *usrdata)
{	
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_weather_search_list_get_data(const char* widgetname, void *usrdata)
{	
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_weather_keypress(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_weather_create(const char* widgetname, void *usrdata)
{	
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_weather_destroy(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_weather_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#endif


