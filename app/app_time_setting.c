#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_utility.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_book.h"
#include <math.h>

#define HOUR_TO_SEC 3600

enum TIME_ITEM_NAME 
{
	ITEM_GMT = 0,
	ITEM_DATE,
	ITEM_TIME,
	ITEM_OFFSET,	
	ITEM_SUMMER,
	#if THREE_HOURS_AUTO_STANDBY
	ITEM_AUTO_STANDYB,
	#endif
	ITEM_TIME_TOTAL
};

#if THREE_HOURS_AUTO_STANDBY
enum _auto_standby
{
	AUTO_STANDBY_ON,
	AUTO_STANDBY_OFF
};
#endif

typedef enum
{
	GMT_ON,
	GMT_OFF
}TimeGMTSel;

typedef enum
{
	SUMMER_OFF,
	SUMMER_ON
}TimeSummerSel;

typedef struct
{
	enum _gmt_local gmt;
	time_t tm_sec;
	float zone;
	enum _summer summer;
	#if THREE_HOURS_AUTO_STANDBY
	enum _auto_standby standby;
	#endif
}SysTimePara;

static SystemSettingOpt s_time_setting_opt;
static SystemSettingItem s_time_item[ITEM_TIME_TOTAL];
static SysTimePara s_sys_time_para;

static char *s_time_date_str = NULL;
static char *s_time_hourmin_str = NULL;
static bool s_time_date_valid = false;


static time_t time_sec;
static float zone_change = 0 ;
static float zone_change_bak = 0;
static int summer_change = 0;
static int summer_change_bak = 0;


static void _time_date_change()
{
	char *str_temp = NULL;
	char *s_date_str = NULL;
	char *s_time_str = NULL;
	int  str_len = 0;
    float temp = 0;

	// change time_sec
    
	temp = (zone_change - zone_change_bak);
    if(temp < 0)
	{
        time_sec = time_sec - (unsigned int)(fabs(temp)*HOUR_TO_SEC);
    }
    else
    {
        time_sec = time_sec + (unsigned int)(fabs(temp)*HOUR_TO_SEC);
    }
	//time_sec = time_sec + (zone_change - zone_change_bak)*HOUR_TO_SEC;
	time_sec = time_sec + (summer_change - summer_change_bak)* HOUR_TO_SEC;

	// get and set date for edit2
	str_temp = app_time_to_date_edit_str(time_sec);
	if(str_temp != NULL)
	{
		str_len = strlen(str_temp);
		s_date_str = (char*)GxCore_Malloc(str_len + 1);
		if(s_date_str != NULL)
		{
			memset(s_date_str, 0 , str_len + 1);
			memcpy(s_date_str, str_temp, str_len);
		}
		GxCore_Free(str_temp);
		str_temp = NULL;
	}
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.string = s_date_str;
        app_system_set_item_property(ITEM_DATE, &s_time_item[ITEM_DATE].itemProperty);


	GxCore_Free(s_date_str);
	s_date_str = NULL;
	
	//   get and set time for edit3
	str_temp = app_time_to_hourmin_edit_str(time_sec);
	if(str_temp != NULL)
	{
		str_len = strlen(str_temp);
		s_time_str = (char*)GxCore_Malloc(str_len + 1);
		if(s_time_str != NULL)
		{
			memset(s_time_str, 0 , str_len + 1);
			memcpy(s_time_str, str_temp, str_len);
		}
		GxCore_Free(str_temp);
		str_temp =NULL;
	}
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.string = s_time_str;
        app_system_set_item_property(ITEM_TIME, &s_time_item[ITEM_TIME].itemProperty);

	GxCore_Free(s_time_str);
	s_time_str = NULL;
}
static TimeGMTSel app_gmt_to_sel(enum _gmt_local gmt)
{
	TimeGMTSel ret;
	switch(gmt)
	{
		case TIME_LOCAL:
			ret = GMT_OFF;
			break;

		case TIME_GMT:
			ret = GMT_ON;
			break;

		default:
			ret = GMT_OFF;
			break;
	}
	return ret;
}

static enum _gmt_local app_sel_to_gmt(TimeGMTSel sel)
{
	enum _gmt_local ret;
	switch(sel)
	{
		case GMT_OFF:
			ret = TIME_LOCAL;
			break;

		case GMT_ON:
			ret = TIME_GMT;
			break;

		default:
			ret = TIME_LOCAL;
			break;
	}
	return ret;
}

static TimeSummerSel app_summer_to_sel(enum _summer summer)
{
	TimeGMTSel ret;
	switch(summer)
	{
		case TIME_SUMMER_OFF:
			ret = SUMMER_OFF;
			break;

		case TIME_SUMMER_ON:
			ret = SUMMER_ON;
			break;

		default:
			ret = SUMMER_OFF;
			break;
	}
	return ret;
}

static enum _gmt_local app_sel_to_summer(TimeSummerSel sel)
{
	enum _summer ret;
	switch(sel)
	{
		case SUMMER_OFF:
			ret = TIME_SUMMER_OFF;
			break;

		case SUMMER_ON:
			ret = TIME_SUMMER_ON;
			break;

		default:
			ret = TIME_SUMMER_OFF;
			break;
	}
	return ret;
}

static float thiz_zone_table[26] = {
  -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0,
  1,   2,   3,   4,  5,  6,  6.5, 7, 8,  9,  10, 11, 12,
};
#if 0
static int app_zone_to_sel(float zone)
{
    int i = 0;
    int count = sizeof(thiz_zone_table);
    for(i = 0; i < count; i++)
    {
        if(thiz_zone_table[i] == zone)
            break;
    }
	return i;
}

static float app_sel_to_zone(int sel)
{
	return thiz_zone_table[sel];
}
#endif

static int app_zone_to_sel(float zone)
{


	int i = 0;
    int count = sizeof(thiz_zone_table);
    for(i = 0; i < count; i++)
    {
        if(thiz_zone_table[i] == zone)
            break;
    }
	return i;
	//return zone + 12;
}

static float app_sel_to_zone(int sel)
{
	return thiz_zone_table[sel];
}

 static int app_time_zone_set_utc_string(int utc_mode)
{
	#define TEXT_SYS_SET_TIMEZONE_UTC 	"text_system_setting_timezone"
	#define IMG_SYS_SET_TIMEZONE_UTC_BAK 	"img_system_setting_timezone_back"
	GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"state","show");	
	GUI_SetProperty(IMG_SYS_SET_TIMEZONE_UTC_BAK,"state","show");	

	switch(utc_mode)
	{
		case 2:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_1000_);	
			break;
		case 3:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0900_);	
			break;
		case 4:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0800_);	
			break;
		case 5:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0700_);	
			break;
		case 6:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0600_);	
			break;
		case 7:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0500_);	
			break;
		case 8:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0400_);	
			break;
		case 9:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0300_);	
			break;
		case 12:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0000);	//
			break;
		case 13:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0100);	
			break;
		case 14:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0200);	
			break;
		case 15:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0300);	
			break;
		case 16:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0400);	
			break;
		case 17:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0500);	
			break;
		case 18:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0600);	
			break;
        case 19:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0630);	
			break;
		case 20:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0700);	
			break;
		case 21:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0800);	
			break;
		case 22:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_0900);	
			break;
		case 23:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_1000);	
			break;
		case 25:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",STR_ID_UTC_1200);	
			break;
		default:
			GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"state","hide");	
			GUI_SetProperty(IMG_SYS_SET_TIMEZONE_UTC_BAK,"state","hide");	
			break;
	}
	
	//GUI_SetInterface("flush",NULL);	

	//zone_change = utc_mode - 12 - s_sys_time_para.zone;
	zone_change = app_sel_to_zone(utc_mode) - s_sys_time_para.zone;
	_time_date_change();
	zone_change_bak = zone_change;
	return EVENT_TRANSFER_KEEPON;
}



static void app_time_system_para_get(void)
{
	int init_value = 0;
	double zone = 0;
	time_t sec;
	time_t temp_sec; 

	sec = g_AppTime.utc_get(&g_AppTime);
	temp_sec = get_display_time_by_timezone(sec);
	s_sys_time_para.tm_sec = temp_sec - temp_sec % 60;
	
    	GxBus_ConfigGetInt(TIME_MODE, &init_value, TIME_MODE_VALUE);
    	s_sys_time_para.gmt = init_value;

    	GxBus_ConfigGetDouble(TIME_ZONE, &zone, TIME_ZONE_VALUE);
    	s_sys_time_para.zone = (float)zone;

    	GxBus_ConfigGetInt(TIME_SUMMER, &init_value, TIME_SUMMER_VALUE);
    	s_sys_time_para.summer = init_value;

	#if THREE_HOURS_AUTO_STANDBY
	GxBus_ConfigGetInt(TIME_AUTO_STANDBY, &init_value, TIME_AUTO_STANDBY_VALUE);
    	s_sys_time_para.standby= init_value;
	#endif

	// get the start time_sec
	time_sec = s_sys_time_para.tm_sec;
		
} 

static void app_time_result_para_get(SysTimePara *ret_para)
{
	time_t date;
	time_t time;
	char* datestring;
    char* timestring;

	GUI_GetProperty("edit_system_setting_opt2","string",&datestring);
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.string = datestring;

	if(app_edit_str_to_date_sec(s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.string, &date) == GXCORE_SUCCESS)
	{
		GUI_GetProperty("edit_system_setting_opt3","string",&timestring);
        s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.string = timestring;
		time = app_edit_str_to_hourmin_sec(s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.string);
		ret_para->tm_sec = date + time;

		ret_para->gmt = app_sel_to_gmt(s_time_item[ITEM_GMT].itemProperty.itemPropertyCmb.sel);
		ret_para->zone = app_sel_to_zone(s_time_item[ITEM_OFFSET].itemProperty.itemPropertyCmb.sel);
		ret_para->summer = app_sel_to_summer(s_time_item[ITEM_SUMMER].itemProperty.itemPropertyCmb.sel);
#if THREE_HOURS_AUTO_STANDBY
		ret_para->standby= app_sel_to_summer(s_time_item[ITEM_AUTO_STANDYB].itemProperty.itemPropertyCmb.sel);
#endif
		if(ret_para->gmt == TIME_LOCAL)
		{
            if(ret_para->zone < 0)
            {
			    time = ret_para->tm_sec + (unsigned int)(fabs(ret_para->zone) * HOUR_TO_SEC);
            }
            else
            {
			    time = ret_para->tm_sec - (unsigned int)(fabs(ret_para->zone) * HOUR_TO_SEC);
            }
			time -= ret_para->summer* HOUR_TO_SEC;

			s_time_date_valid = app_time_range_valid(time);
		}
		else
		{
			s_time_date_valid = true;
		}
	} 
	else
	{
		s_time_date_valid = false;
	}

	if(s_time_date_valid == false)
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
       	pop.type = POP_TYPE_OK;
		pop.str = STR_ID_ERR_DATE;
       	pop.mode = POP_MODE_BLOCK;
       	popdlg_create(&pop);
	}
}

static bool app_time_setting_para_change(SysTimePara *ret_para)
{
	bool ret = FALSE;

	if(s_time_date_valid == false)
		return ret;
	
	if(s_sys_time_para.gmt != ret_para->gmt)
	{
		ret = TRUE;
	}

	if(s_sys_time_para.zone != ret_para->zone)
	{
		ret = TRUE;
	}

	if(s_sys_time_para.summer!= ret_para->summer)
	{
		ret = TRUE;
	}

	if(ret_para->gmt == TIME_LOCAL)
	{
		if(s_sys_time_para.tm_sec!= ret_para->tm_sec)
		{
			ret = TRUE;
		}
	}
	#if THREE_HOURS_AUTO_STANDBY
	if(s_sys_time_para.standby!= ret_para->standby)
	{
		ret = TRUE;
	}
	#endif
	
	return ret;
}

#if THREE_HOURS_AUTO_STANDBY
extern void app_set_auto_standby_time(int mode);
static int app_auto_standby_para_save(SysTimePara *ret_para)
{
	int standby_value = 0;
	
	standby_value = ret_para->standby;
    	GxBus_ConfigSetInt(TIME_AUTO_STANDBY,standby_value); 
	app_set_auto_standby_time(standby_value);
	return 0;
}
#endif

static int app_time_setting_para_save(SysTimePara *ret_para)
{
	time_cfg time = {0};

	time.ymd = TIME_YMD;
	time.gmt_local = ret_para->gmt;
	time.zone = ret_para->zone;
	time.summer = ret_para->summer;
	g_AppTime.mode_set(&g_AppTime, &time);
	
	if(TIME_LOCAL == ret_para->gmt)
	{
		time_t sec;
		
		sec = get_local_time_by_timezone(ret_para->tm_sec);
		g_AppTime.sync(&g_AppTime, sec+3);
		g_AppBook.enable();
	}

	g_AppTime.start(&g_AppTime);
	app_time_system_para_get();

	return 0;
}

static int app_time_setting_exit_callback(ExitType exit_type)
{
	SysTimePara para_ret = {0};
	int ret = 0;
    if (EXIT_ABANDON != exit_type)
    {
        app_time_result_para_get(&para_ret);
        if(app_time_setting_para_change(&para_ret) == TRUE)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.str = STR_ID_SAVE_INFO;
            if(popdlg_create(&pop) == POP_VAL_OK)
            {
                app_time_setting_para_save(&para_ret);
#if THREE_HOURS_AUTO_STANDBY
                app_auto_standby_para_save(&para_ret);
#endif
                ret = 1;
            }
        }
    }

    if(s_time_date_str != NULL)
    {
        GxCore_Free(s_time_date_str);
        s_time_date_str = NULL;
    }

    if(s_time_hourmin_str != NULL)
    {
        GxCore_Free(s_time_hourmin_str);
        s_time_hourmin_str = NULL;
    }

    return ret;
}

static int app_time_zone_set_utc_keypress(int key)
{
    SysTimePara para_ret = {0};
    int ret = 0;
    if (key == STBK_OK)
    {
        app_time_result_para_get(&para_ret);
        if (app_time_setting_para_change(&para_ret) == TRUE)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.str = STR_ID_SAVE_INFO;
            if(popdlg_create(&pop) == POP_VAL_OK)
            {
                app_time_setting_para_save(&para_ret);
#if THREE_HOURS_AUTO_STANDBY
                app_auto_standby_para_save(&para_ret);
#endif
                ret = 1;
            }
        }
    }   
	return EVENT_TRANSFER_KEEPON;
	
}

static int app_time_gmt_change_callback(int sel)
{
	if(sel == GMT_ON)
	{
		app_system_set_item_state_update(ITEM_DATE, ITEM_DISABLE);
		app_system_set_item_state_update(ITEM_TIME, ITEM_DISABLE);
		app_system_set_item_state_update(ITEM_OFFSET, ITEM_NORMAL);//20140113
		app_system_set_item_state_update(ITEM_SUMMER, ITEM_NORMAL);//20140113
#if THREE_HOURS_AUTO_STANDBY
		app_system_set_item_state_update(ITEM_AUTO_STANDYB, ITEM_DISABLE);//20140113
#endif
	}
	else
	{
		app_system_set_item_state_update(ITEM_DATE, ITEM_NORMAL);
		app_system_set_item_state_update(ITEM_TIME, ITEM_NORMAL);
		app_system_set_item_state_update(ITEM_OFFSET, ITEM_DISABLE);//20140113
		app_system_set_item_state_update(ITEM_SUMMER, ITEM_DISABLE);//20140113
		#if THREE_HOURS_AUTO_STANDBY
		app_system_set_item_state_update(ITEM_AUTO_STANDYB, ITEM_NORMAL);//20140113
		#endif
	}

	return EVENT_TRANSFER_KEEPON;
}

static int app_time_gmt_keypress_callback(int key)
{
    SysTimePara para_ret = {0};
    int ret = 0;
    printf("key == %d \n",key);
    if (key == STBK_OK)
    {
        app_time_result_para_get(&para_ret);
        if (app_time_setting_para_change(&para_ret) == TRUE)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.str = STR_ID_SAVE_INFO;
            if(popdlg_create(&pop) == POP_VAL_OK)
            {
                app_time_setting_para_save(&para_ret);
#if THREE_HOURS_AUTO_STANDBY
                app_auto_standby_para_save(&para_ret);
#endif
                ret = 1;
            }
            else
            {
                s_time_item[ITEM_GMT].itemProperty.itemPropertyCmb.sel = app_gmt_to_sel(s_sys_time_para.gmt);
                app_system_set_item_property(ITEM_GMT, &s_time_item[ITEM_GMT].itemProperty);
            }
        }
    }   
	return EVENT_TRANSFER_KEEPON;
	
}

static int app_time_summer_change_callback(int sel)
{

	if(s_sys_time_para.summer == TIME_SUMMER_OFF)
	{
		if(sel == SUMMER_ON)
		{
			summer_change = 1;
			_time_date_change();
			summer_change_bak = summer_change;
		}
		else
		{
			summer_change = 0;
			_time_date_change();
			summer_change_bak = summer_change;
		}
	}
	else if(s_sys_time_para.summer == TIME_SUMMER_ON)
	{
		if(sel == SUMMER_OFF)
		{
			summer_change = -1;
			_time_date_change();
			summer_change_bak = summer_change;
		}
		else
		{
			summer_change = 0;
			_time_date_change();
			summer_change_bak = summer_change;
		}
	}
	
	return EVENT_TRANSFER_KEEPON;
}

static int app_time_summer_keypress_callback(int key)
{
    SysTimePara para_ret = {0};
    int ret = 0;
    if (key == STBK_OK)
    {
        app_time_result_para_get(&para_ret);
        if (app_time_setting_para_change(&para_ret) == TRUE)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.str = STR_ID_SAVE_INFO;
            if(popdlg_create(&pop) == POP_VAL_OK)
            {
                app_time_setting_para_save(&para_ret);
#if THREE_HOURS_AUTO_STANDBY
                app_auto_standby_para_save(&para_ret);
#endif
                ret = 1;
            }
             else
            {
                s_time_item[ITEM_SUMMER].itemProperty.itemPropertyCmb.sel = app_summer_to_sel(s_sys_time_para.summer);
                app_system_set_item_property(ITEM_SUMMER, &s_time_item[ITEM_SUMMER].itemProperty);
            }
        }
    }   
	return EVENT_TRANSFER_KEEPON;
	
}

static void app_time_setting_gmt_item_init(void)
{
	s_time_item[ITEM_GMT].itemTitle = STR_ID_MODE;
	s_time_item[ITEM_GMT].itemType = ITEM_CHOICE;
	s_time_item[ITEM_GMT].itemProperty.itemPropertyCmb.content= "[Auto,Manual]";
	s_time_item[ITEM_GMT].itemProperty.itemPropertyCmb.sel = app_gmt_to_sel(s_sys_time_para.gmt);
	s_time_item[ITEM_GMT].itemCallback.cmbCallback.CmbChange= app_time_gmt_change_callback;
    s_time_item[ITEM_GMT].itemCallback.cmbCallback.CmbPress= app_time_gmt_keypress_callback;

	s_time_item[ITEM_GMT].itemStatus = ITEM_NORMAL;
}

static int app_time_date_keypress_callback(unsigned short key)
{
    SysTimePara para_ret = {0};
    int ret = 0;
    if (key == STBK_OK)
    {
        app_time_result_para_get(&para_ret);
        if (app_time_setting_para_change(&para_ret) == TRUE)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.str = STR_ID_SAVE_INFO;
            if(popdlg_create(&pop) == POP_VAL_OK)
            {
                app_time_setting_para_save(&para_ret);
#if THREE_HOURS_AUTO_STANDBY
                app_auto_standby_para_save(&para_ret);
#endif
                ret = 1;
            }
        }
    }   
	return EVENT_TRANSFER_KEEPON;
	
}
static int app_time_time_keypress_callback(unsigned short key)
{
    SysTimePara para_ret = {0};
    int ret = 0;
    if (key == STBK_OK)
    {
        app_time_result_para_get(&para_ret);
        if (app_time_setting_para_change(&para_ret) == TRUE)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.str = STR_ID_SAVE_INFO;
            if(popdlg_create(&pop) == POP_VAL_OK)
            {
                app_time_setting_para_save(&para_ret);
#if THREE_HOURS_AUTO_STANDBY
                app_auto_standby_para_save(&para_ret);
#endif
                ret = 1;
            }
        }
    }   
	return EVENT_TRANSFER_KEEPON;
	
}
static void app_time_setting_date_item_init(void)
{
	char *str_temp = NULL;
	int str_len = 0;
	
	s_time_item[ITEM_DATE].itemTitle = STR_ID_DATE;
	s_time_item[ITEM_DATE].itemType = ITEM_EDIT;
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.format = "edit_date";
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.maxlen = "10";
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.intaglio = NULL;
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.default_intaglio = NULL;

	if(s_time_date_str != NULL)
	{
		GxCore_Free(s_time_date_str);
		s_time_date_str = NULL;
	}
	str_temp = app_time_to_date_edit_str(s_sys_time_para.tm_sec);
	if(str_temp != NULL)
	{
		str_len = strlen(str_temp);
		s_time_date_str = (char*)GxCore_Malloc(str_len + 1);
		if(s_time_date_str != NULL)
		{
			memset(s_time_date_str, 0 , str_len + 1);
			memcpy(s_time_date_str, str_temp, str_len);
		}
		GxCore_Free(str_temp);
		str_temp =NULL;
	}
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.string = s_time_date_str;

	s_time_item[ITEM_DATE].itemCallback.editCallback.EditReachEnd= NULL;
    s_time_item[ITEM_DATE].itemCallback.editCallback.EditPress = app_time_date_keypress_callback;
	if(s_sys_time_para.gmt == TIME_GMT)
	{
		s_time_item[ITEM_DATE].itemStatus = ITEM_DISABLE;
	}
	else
	{
		s_time_item[ITEM_DATE].itemStatus = ITEM_NORMAL;
	}
}

static void app_time_setting_time_item_init(void)
{
	char *str_temp = NULL;
	int str_len = 0;
	
	s_time_item[ITEM_TIME].itemTitle = STR_ID_TIME;
	s_time_item[ITEM_TIME].itemType = ITEM_EDIT;
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.format = "edit_time";
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.maxlen = "5";
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.intaglio = NULL;
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.default_intaglio = NULL;

	if(s_time_hourmin_str != NULL)
	{
		GxCore_Free(s_time_hourmin_str);
		s_time_hourmin_str = NULL;
	}
	str_temp = app_time_to_hourmin_edit_str(s_sys_time_para.tm_sec);
	if(str_temp != NULL)
	{
		str_len = strlen(str_temp);
		s_time_hourmin_str = (char*)GxCore_Malloc(str_len + 1);
		if(s_time_hourmin_str != NULL)
		{
			memset(s_time_hourmin_str, 0 , str_len + 1);
			memcpy(s_time_hourmin_str, str_temp, str_len);
		}
		GxCore_Free(str_temp);
		str_temp =NULL;
	}
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.string = s_time_hourmin_str;

	s_time_item[ITEM_TIME].itemCallback.editCallback.EditReachEnd= NULL;
	s_time_item[ITEM_TIME].itemCallback.editCallback.EditPress= NULL;
    s_time_item[ITEM_TIME].itemCallback.editCallback.EditPress= app_time_time_keypress_callback;
	if(s_sys_time_para.gmt == TIME_GMT)
	{
		s_time_item[ITEM_TIME].itemStatus = ITEM_DISABLE;
	}
	else
	{
		s_time_item[ITEM_TIME].itemStatus = ITEM_NORMAL;
	}
}

static void app_time_setting_zone_item_init(void)
{
	s_time_item[ITEM_OFFSET].itemTitle = STR_ID_TIME_ZONE;
	s_time_item[ITEM_OFFSET].itemType = ITEM_CHOICE;

	s_time_item[ITEM_OFFSET].itemProperty.itemPropertyCmb.content= "[-12,-11,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,0,+1,+2,+3,+4,+5,+6,+6.5,+7,+8,+9,+10,+11,+12]";
	s_time_item[ITEM_OFFSET].itemProperty.itemPropertyCmb.sel = app_zone_to_sel(s_sys_time_para.zone);
	s_time_item[ITEM_OFFSET].itemCallback.cmbCallback.CmbChange= app_time_zone_set_utc_string;
    s_time_item[ITEM_OFFSET].itemCallback.cmbCallback.CmbPress= app_time_zone_set_utc_keypress;
	s_time_item[ITEM_OFFSET].itemStatus = ITEM_NORMAL;
}

static void app_time_setting_summer_item_init(void)
{
	s_time_item[ITEM_SUMMER].itemTitle = STR_ID_TIME_SUMMER;
	s_time_item[ITEM_SUMMER].itemType = ITEM_CHOICE;
	s_time_item[ITEM_SUMMER].itemProperty.itemPropertyCmb.content= "[Off,On]";
	s_time_item[ITEM_SUMMER].itemProperty.itemPropertyCmb.sel = app_summer_to_sel(s_sys_time_para.summer);
	s_time_item[ITEM_SUMMER].itemCallback.cmbCallback.CmbChange = app_time_summer_change_callback;
    s_time_item[ITEM_SUMMER].itemCallback.cmbCallback.CmbPress= app_time_summer_keypress_callback;
	s_time_item[ITEM_SUMMER].itemStatus = ITEM_NORMAL;
}

#if THREE_HOURS_AUTO_STANDBY
static int app_auto_standby_to_sel(int summer)
{
	int ret=0;
	ret = summer;
	app_set_auto_standby_time(summer);
	return ret;
}



static void app_time_setting_auto_standby_item_init(void)
{
	s_time_item[ITEM_AUTO_STANDYB].itemTitle = STR_ID_AUTO_STANDBY;
	s_time_item[ITEM_AUTO_STANDYB].itemType = ITEM_CHOICE;
	s_time_item[ITEM_AUTO_STANDYB].itemProperty.itemPropertyCmb.content= "[Off,On]";
	s_time_item[ITEM_AUTO_STANDYB].itemProperty.itemPropertyCmb.sel = app_auto_standby_to_sel(s_sys_time_para.standby);
	s_time_item[ITEM_AUTO_STANDYB].itemCallback.cmbCallback.CmbChange = NULL;
	s_time_item[ITEM_AUTO_STANDYB].itemStatus = ITEM_NORMAL;
	//------20140113
	if(s_sys_time_para.gmt == TIME_GMT)
	{
		s_time_item[ITEM_AUTO_STANDYB].itemStatus = ITEM_DISABLE;
	}
	else
	{
		s_time_item[ITEM_AUTO_STANDYB].itemStatus = ITEM_NORMAL;
	}
}

int app_auto_standby_status(void)
{
	int value = 0;
	GxBus_ConfigGetInt(TIME_AUTO_STANDBY, &value, TIME_AUTO_STANDBY_VALUE);
	return value;
}
#endif
void app_time_setting_menu_exec(void)
{
	zone_change = 0 ;
	zone_change_bak = 0;
	summer_change = 0;
	summer_change_bak = 0;

	memset(&s_time_setting_opt, 0 ,sizeof(s_time_setting_opt));
	app_time_system_para_get();

	s_time_setting_opt.menuTitle = STR_ID_TIME_SET;
	s_time_setting_opt.itemNum = ITEM_TIME_TOTAL;
	s_time_setting_opt.timeDisplay = TIP_HIDE;
	s_time_setting_opt.item = s_time_item;
	
	app_time_setting_gmt_item_init();
	app_time_setting_date_item_init();
	app_time_setting_time_item_init();
	app_time_setting_zone_item_init();
	app_time_setting_summer_item_init();
	#if THREE_HOURS_AUTO_STANDBY
	app_time_setting_auto_standby_item_init();
	#endif

	s_time_setting_opt.exit = app_time_setting_exit_callback;

	app_system_set_create(&s_time_setting_opt);

	GUI_SetProperty("text_system_setting_timezone","state","show");	
	GUI_SetProperty("img_system_setting_timezone_back","state","show");	
}

