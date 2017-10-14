/*
 * =====================================================================================
 *
 *       Filename:  app_time.c
 *
 *    Description:  set utc time to localtime,  provide multi-time mode 
 *
 *        Version:  1.0
 *        Created:  2011年09月14日 09时27分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app_module.h"
#include "module/si/si_pmt.h"
#include "gxsi.h"
#include "gxmsg.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "gui_core.h"
#include "module/config/gxconfig.h"
#include <math.h>

#define TIME_PRECISION  (3)

static void app_time_sync(AppTime *tm, time_t utc_time)
{
    GxTime sys_time;

    sys_time.seconds = utc_time;
    sys_time.microsecs = 0;
    GxCore_SetLocalTime(&sys_time);

    tm->seconds = sys_time.seconds;
}

static void app_time_init(AppTime *tm)
{
    app_time_sync(tm, SYS_DEFAULT_TIME_UTC);//back to 2001 .1.1 0:0
}

static int time_timeout_cb(void *usrdata)
{
    GxTime sys_time;
    GxCore_GetLocalTime(&sys_time);
    if(sys_time.seconds >= SYS_DEAD_TIME_SEC)
    {
       app_time_init((AppTime*)usrdata);
    }
    else
    {
    	((AppTime*)usrdata)->seconds = sys_time.seconds;
    }

    return 0;
}

static void app_time_stop(AppTime *tm)
{
    app_send_msg_exec(GXMSG_EXTRA_RELEASE, NULL);

    remove_timer(tm->timer);
    tm->timer = NULL;
}

static void app_time_start(AppTime *tm)
{
    int32_t value = 0;
    double zone = 0;

    if (tm == NULL) return;

    // time config
    GxBus_ConfigGetInt(TIME_DISP, &value, TIME_DISP_VALUE);
    tm->config.ymd = value;

    GxBus_ConfigGetInt(TIME_MODE, &value, TIME_MODE_VALUE);
    tm->config.gmt_local = value;

    //GxBus_ConfigGetInt(TIME_ZONE, &value, TIME_ZONE_VALUE);
    GxBus_ConfigGetDouble(TIME_ZONE, &zone, TIME_ZONE_VALUE);
    tm->config.zone = (float)zone;

    GxBus_ConfigGetInt(TIME_SUMMER, &value, TIME_SUMMER_VALUE);
    tm->config.summer = value;
	 

    if(tm->config.gmt_local == TIME_GMT)
    {
    	printf("enter %s:%d  tm->config.gmt_local  = %d \n",__func__,__LINE__,tm->config.gmt_local);
        GxMsgProperty_ExtraSyncTime time;
        time.ts_src = g_AppPlayOps.normal_play.ts_src;
        app_send_msg_exec(GXMSG_EXTRA_SYNC_TIME, (void *)(&time));
    }
    else
    {
        app_time_stop(tm);
    }

    if (reset_timer(tm->timer) != 0) 
    {
        tm->timer = create_timer(time_timeout_cb, 
                TIME_PRECISION*SEC_TO_MILLISEC, tm, TIMER_REPEAT);
    }
}

static void app_time_set(AppTime *tm, time_cfg *cfg)
{
    if (tm == NULL || cfg == NULL)      return;

    if (cfg->ymd < TIME_YMD_END)
    {
        tm->config.ymd = cfg->ymd;
        GxBus_ConfigSetInt(TIME_DISP, (int32_t)(tm->config.ymd));
    }
    if (cfg->gmt_local < TIME_GL_END)
    {
        tm->config.gmt_local = cfg->gmt_local;
        GxBus_ConfigSetInt(TIME_MODE, (int32_t)(tm->config.gmt_local));
    }
    if (cfg->zone >= -12 && cfg->zone <= 12)
    {
        tm->config.zone = cfg->zone;
        GxBus_ConfigSetDouble(TIME_ZONE, tm->config.zone);
#if CASCAM_SUPPORT
        CASCAM_Set_Zone((double)(cfg->zone));
#endif

    }

     if (cfg->summer< TIME_SUMMER_END)
    {
        tm->config.summer = cfg->summer;
        GxBus_ConfigSetInt(TIME_SUMMER, (int32_t)(tm->config.summer));
    }
}

static void app_time_get(AppTime *tm, char *tm_str, size_t str_len)
{
#define TIME_STR_LEM    (20)
    time_t seconds;
    struct tm _tm;

    if (str_len < TIME_STR_LEM)
    {
        printf("[time] buf too small\n");
        return;
    }
    //seconds = tm->seconds+tm->config.zone*3600;
	if(tm->config.zone > 0)
    {
        seconds = tm->seconds + (unsigned int)(tm->config.zone*3600);
    }
    else
    {
        seconds = tm->seconds - (unsigned int)(fabs(tm->config.zone)*3600);
    }
    seconds += tm->config.summer*3600;

    memcpy(&_tm, localtime(&(seconds)), sizeof(struct tm));
    memset(tm_str,0,TIME_STR_LEM*sizeof(char));

    if (tm->config.ymd == TIME_YMD)
    {
        sprintf(tm_str,"%d-%02d-%02d %02d:%02d", _tm.tm_year+1900,_tm.tm_mon+1,_tm.tm_mday,_tm.tm_hour,_tm.tm_min);
    }
    else if (tm->config.ymd == TIME_DMY)
    {
        sprintf(tm_str,"%02d-%02d-%d %02d:%02d", _tm.tm_mday,_tm.tm_mon+1,_tm.tm_year+1900,_tm.tm_hour,_tm.tm_min);
    }
    else
    {}

}

static time_t app_utc_get(AppTime *tm)
{
    return tm->seconds;
}

bool app_time_range_valid(time_t gmt_sec)
{
    if((gmt_sec >= SYS_INIT_TIME_SEC)
        && (gmt_sec < SYS_DEAD_TIME_SEC))
    {
        return true;
    }

    return false;
}

bool app_month_date_valid(uint16_t year, uint8_t mon, uint8_t day)
{
	bool ret = true;

	if(year < 1970 || year > 2037)
		return false;

	switch(mon)
	{
		case 4:
		case 6:
		case 9:
		case 11:
			if(day > 30)
			{
				ret = false;
			}
			break;

		case 2:
			if((year % 400 == 0)
				|| ((year % 4 == 0) && (year % 100 != 0)))
			{
				if(day > 29)
				{
					ret = false;
				}
			}
			else
			{
				if(day > 28)
				{
					ret = false;
				}
			}
			break;

		default:
			if(day > 31)
			{
				ret = false;
			}
			break;
	}

	return ret;
}

time_t app_mktime (time_t year, time_t mon, time_t day, time_t hour, time_t min, time_t sec)
{
    if (0 >= (int) (mon -= 2))
    {    /**//* 1..12 -> 11,12,1..10 */
        mon += 12;      /**//* Puts Feb last since it has leap day */
        year -= 1;
    }

    return (((
                 (time_t) (year/4 - year/100 + year/400 + 367*mon/12 + day) +
                 year*365 - 719499
             )*24 + hour /**//* now have hours */
            )*60 + min /**//* now have minutes */
           )*60 + sec; /**//* finally seconds */
}

int app_get_current_time(int *utc_time, int *timezone_min, int *daylight_min, unsigned int *daylight_flag)
{
    GxTime time;
    double time_zone = 0;
    int value = 0;

    GxCore_GetLocalTime(&time);
    GxBus_ConfigGetDouble(TIME_ZONE, &time_zone, TIME_ZONE_VALUE);
    *utc_time = time.seconds;
    *timezone_min = time_zone*60;
    GxBus_ConfigGetInt(TIME_SUMMER, &value, TIME_SUMMER_VALUE);
    if(value)
    {
        *daylight_min = 60;
        *daylight_flag = 1;
    }
    else
    {
        *daylight_min = 0;
        *daylight_flag = 0;
    }
    return 0;
}

int app_time_get_local_time(unsigned short *year, unsigned char *month, unsigned char *day,
                                    unsigned char *hour, unsigned char *minute, unsigned char *second)
{
    time_t seconds;
    struct tm _tm;
    GxTime time;
    double time_zone = 0;
    int value = 0;

    if ((NULL == year) || (NULL ==month) || (NULL == day)
            || (NULL == hour) || (NULL == minute) || (NULL == second))
    {
        printf("\nTime, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    GxCore_GetLocalTime(&time);
    GxBus_ConfigGetDouble(TIME_ZONE, &time_zone, TIME_ZONE_VALUE);
    seconds = time.seconds + (int)(time_zone*3600);
    GxBus_ConfigGetInt(TIME_SUMMER, &value, TIME_SUMMER_VALUE);
    if(value)
    {
        seconds += 3600;
    }
    memcpy(&_tm, localtime(&(seconds)), sizeof(struct tm));

    *year = _tm.tm_year + 1900;
    *month = _tm.tm_mon + 1;
    *day = _tm.tm_mday;
    *hour = _tm.tm_hour;
    *minute = _tm.tm_min;
    *second = _tm.tm_sec;
    return 0;
}



AppTime g_AppTime = 
{
    .start          = app_time_start,
    .stop           = app_time_stop,
    .sync           = app_time_sync,
    .mode_set       = app_time_set,
    .time_get       = app_time_get,
    .utc_get        = app_utc_get,
    .init              = app_time_init,
    .time_sync_cb   = NULL,
};
