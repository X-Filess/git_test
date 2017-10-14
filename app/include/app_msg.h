/*****************************************************************************
* 			  CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_msg.h
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.2.27	            shenbin         creation
*****************************************************************************/

#ifndef __APP_MSG_H__
#define __APP_MSG_H__
#include "gxmsg.h"
#include "service/gxplayer.h"
#include "service/gxextra.h"
#include "service/gxfrontend.h"
#include "service/gxepg.h"
#include "service/gxsearch.h"
#include "service/gxsi.h"
#include "app_config.h"

typedef enum
{
    GXMSG_PM_OPEN = GXMSG_TOTAL_NUM,
    GXMSG_PM_CLOSE,
    GXMSG_PM_NODE_NUM_GET,
    GXMSG_PM_NODE_BY_POS_GET,
    GXMSG_PM_MULTI_NODE_BY_POS_GET,
    GXMSG_PM_NODE_BY_ID_GET,
    GXMSG_PM_NODE_ADD,
    GXMSG_PM_NODE_MODIFY,
    GXMSG_PM_NODE_DELETE,
    GXMSG_PM_NODE_GET_URL,
    GXMSG_PM_LOAD_DEFAULT,

    GXMSG_PM_PLAY_BY_POS,
    GXMSG_PM_PLAY_BY_ID,
    GXMSG_TUNER_COPY,
    GXMSG_PM_VIEWINFO_SET,
    GXMSG_PM_VIEWINFO_GET,

    GXMSG_INTERFACE_HELP,
    GXMSG_INTERFACE_LOCK_TS,
    GXMSG_CHECK_PM_EXIST,
    GXMSG_EMPTY_SERVICE_MSG,

    GXMSG_GET_MEDIA_TIME,
    GXMSG_AV_OUTPUT_OFF,

    GXMSG_SUBTITLE_INIT,
    GXMSG_SUBTITLE_DESTROY,
    GXMSG_SUBTITLE_HIDE,
    GXMSG_SUBTITLE_SHOW,
    GXMSG_SUBTITLE_DRAW,
    GXMSG_SUBTITLE_CLEAR,

#if NETWORK_SUPPORT
    GXMSG_NETWORK_START,
    GXMSG_NETWORK_STOP,

    GXMSG_NETWORK_DEV_LIST,

    GXMSG_NETWORK_GET_STATE,
    GXMSG_NETWORK_STATE_REPORT,

    GXMSG_NETWORK_GET_TYPE,

    GXMSG_NETWORK_GET_CUR_DEV,

    GXMSG_NETWORK_PLUG_IN,
    GXMSG_NETWORK_PLUG_OUT,

    GXMSG_NETWORK_SET_MAC,
    GXMSG_NETWORK_GET_MAC,

    GXMSG_NETWORK_SET_IPCFG,
    GXMSG_NETWORK_GET_IPCFG,

    GXMSG_NETWORK_SET_MODE,
    GXMSG_NETWORK_GET_MODE,

    GXMSG_WIFI_SCAN_AP,
    GXMSG_WIFI_SCAN_AP_OVER,

    GXMSG_WIFI_GET_CUR_AP,
    GXMSG_WIFI_SET_CUR_AP,
    GXMSG_WIFI_DEL_CUR_AP,

    GXMSG_3G_SET_PARAM,
    GXMSG_3G_GET_PARAM,
    GXMSG_3G_GET_CUR_OPERATOR,

    GXMSG_PPPOE_SET_PARAM,
    GXMSG_PPPOE_GET_PARAM,
#if (DLNA_SUPPORT > 0)
    GXMSG_DLNA_TRIGGER,
#endif

#if IPTV_SUPPORT
    APPMSG_IPTV_GET_LIST,
    APPMSG_IPTV_LIST_DONE,
    APPMSG_IPTV_UPDATE_LOCAL_LIST,
    APPMSG_IPTV_GET_AD_PIC,
    APPMSG_IPTV_AD_PIC_DONE,
#endif
#endif

#if TKGS_SUPPORT
/*TKGS msg*/
GXMSG_TKGS_CHECK_BY_PMT,//通过检查PMT中是否有TKGS component descriptor，判断该频点是否是包含TKGS数据的频点， 同步消息;参数：PMT表数据
GXMSG_TKGS_CHECK_BY_TP,//对比保存的location list检查该TP是否是包含TKGS数据的频点， 同步消息；参数：TP id
GXMSG_TKGS_START_VERSION_CHECK, //启动TKGS版本检查，看是否有新的升级， 异步消息;参数：GxTkgsUpdateParm
//GXMSG_TKGS_VERSION_CHECK_FINISH,//版本检查结果,APP处理; 参数：GxTkgsUpdateStatus
GXMSG_TKGS_START_UPDATE,//启动升级， 异步消息；参数：GxTkgsUpdateParm
GXMSG_TKGS_STOP,//强制停止TKGS已经启动的版本检查或升级，异步消息；参数:无
GXMSG_TKGS_UPDATE_STATUS, //升级结束，参数:GxTkgsUpdateMsg, 如果有TKGS消息，需要马上展示给用户，APP处理
GXMSG_TKGS_SET_LOCATION,//设置visible location, 同步消息
GXMSG_TKGS_GET_LOCATION,//获取visible location 列表
//GXMSG_TKGS_LOCATION_SET_HIDDEN_LIST,//设置hidden list， 同步消息
GXMSG_TKGS_SET_OPERATING_MODE,	//设置模式：KGS Off，Customizable，Automatic， 同步消息
GXMSG_TKGS_GET_OPERATING_MODE,	//获取设置的模式：KGS Off，Customizable，Automatic， 同步消息
GXMSG_TKGS_GET_PREFERRED_LISTS, //获取prefered list, 从TKGS数据中获取service list, 供用户选择，同步消息；参数：GxTkgsPreferredList
GXMSG_TKGS_SET_PREFERRED_LIST,//设置用户选择的列表，同步消息; 参数：字符串， service list name
GXMSG_TKGS_GET_PREFERRED_LIST,//获取用户选择的列表，同步消息；参数：字符串，service list name
GXMSG_TKGS_BLOCKING_CHECK,//检查节目是否是block节目，同步消息；参数: 节目ID
//测试相关消息
GXMSG_TKGS_TEST_GET_VERNUM, //获取TKGS表的版本号; 同步消息；参数：版本号
GXMSG_TKGS_TEST_RESET_VERNUM,	//清除机顶盒中保存的TKGS表版本号; 同步消息；参数：无
GXMSG_TKGS_TEST_CLEAR_HIDDEN_LOCATIONS_LIST,	//清除hidden location 列表；同步消息；参数：无
GXMSG_TKGS_TEST_CLEAR_VISIBLE_LOCATIONS_LIST,	//清除visible location 列表；同步消息；参数：无
#endif

#if CASCAM_SUPPORT
    GXMSG_CASCAM_EVENT,
#endif
#if OTA_MONITOR_SUPPORT
    GXMSG_OTA_EVENT,
#endif
#if LNB_SHORTCUT_SUPPORT
    GXMSG_LNB_SHORTCUT_EVENT,
#endif
    GXMAX_MSG_NUM
}AppMsgId;

typedef struct {
	PlayerWindow rect;
	int type;
	int handle;
	int num;
	void* data;
}GxMsgProperty_AppSubtitle;

typedef struct _OTAUpdateClass{
    int new_version;
    int update_type;//ex. 1:menual; 2:force
    int fre;// ex.11011
    int pol;// ex.0:H; 1:V
    int sym;// ex.30000
    unsigned short pid;//ex. 0x1f48
    int sn_start;//ex. 0x00000000
    int sn_end;//ex.0xffffffff
    int event_flag;//ex. 0:from nit; 1:from pid monitor
}OTAUpdateClass;

#define APP_PRINT(...)     printf(__VA_ARGS__)
#endif

