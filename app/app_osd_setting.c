#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"

/*
#define OSD_TRANS_30PERCENT 78*127/255
#define OSD_TRANS_40PERCENT 102*127/255
#define OSD_TRANS_50PERCENT 128*127/255
#define OSD_TRANS_60PERCENT 153*127/255
#define OSD_TRANS_70PERCENT 178*127/255
#define OSD_TRANS_80PERCENT 204*127/255
#define OSD_TRANS_90PERCENT 230*127/255
#define OSD_TRANS_100PERCENT 127
*/
enum OSD_ITEM_NAME 
{
	ITEM_TRANS = 0,
	ITEM_BAR_TIMEOUT,
	//ITEM_VOL_TIMEOUT,
	ITEM_FREEZE,
	ITEM_OSD_TOTAL
};

typedef enum
{
	OSD_TRANS_40 = 0,
	OSD_TRANS_50,
	OSD_TRANS_60,
	OSD_TRANS_70,
	OSD_TRANS_80,
	OSD_TRANS_90,
	OSD_TRANS_100
}OsdTransSel;

typedef enum
{
	OSD_TIMEOUT_3S,
	OSD_TIMEOUT_5S,
	OSD_TIMEOUT_7S
}OsdTimeOutSel;

typedef struct
{
	int osd_tran;
	int bar_timeout;
	int vol_timeout;
	int freeze_switch;
}SysOsdPara;

static SystemSettingOpt s_osd_setting_opt;
static SystemSettingItem s_osd_item[ITEM_OSD_TOTAL];
static SysOsdPara s_sys_osd_para;

static OsdTransSel app_osd_trans_to_sel(int trans)
{
	OsdTransSel sel = OSD_TRANS_40;

	switch(trans)
	{
		case OSD_TRANS_40PERCENT:
			sel = OSD_TRANS_100;
			break;

		case OSD_TRANS_50PERCENT:
			sel = OSD_TRANS_90;
			break;	

		case OSD_TRANS_60PERCENT:
			sel = OSD_TRANS_80;
			break;	
			
		case OSD_TRANS_70PERCENT:
			sel = OSD_TRANS_70;
			break;

		case OSD_TRANS_80PERCENT:
			sel = OSD_TRANS_60;
			break;	

		case OSD_TRANS_90PERCENT:
			sel = OSD_TRANS_50;
			break;	

		case OSD_TRANS_100PERCENT:
			sel = OSD_TRANS_40;
			break;	

		default:
			break;
	}
	
	return sel;
}

static int app_sel_to_osd_trans(OsdTransSel sel)
{
	int trans = OSD_TRANS_100PERCENT;

	switch(sel)
	{
		case OSD_TRANS_40:
			trans = OSD_TRANS_100PERCENT;
			break;

		case OSD_TRANS_50:
			trans = OSD_TRANS_90PERCENT;
			break;
			
		case OSD_TRANS_60:
			trans = OSD_TRANS_80PERCENT;
			break;

		case OSD_TRANS_70:
			trans = OSD_TRANS_70PERCENT;
			break;

		case OSD_TRANS_80:
			trans = OSD_TRANS_60PERCENT;
			break;
			
		case OSD_TRANS_90:
			trans = OSD_TRANS_50PERCENT;
			break;

		case OSD_TRANS_100:
			trans = OSD_TRANS_40PERCENT;
			break;

		default:
			break;
	}
	
	return trans;
}

static OsdTimeOutSel app_widget_timeout_to_sel(int sec)
{
	OsdTimeOutSel sel = OSD_TIMEOUT_3S;

	switch(sec)
	{
		case 3:
			sel = OSD_TIMEOUT_3S;
			break;

		case 5:
			sel = OSD_TIMEOUT_5S;
			break;	

		case 7:
			sel = OSD_TIMEOUT_7S;
			break;	

		default:
			break;
	}
	
	return sel;
}

static int app_sel_to_widget_timeout(OsdTimeOutSel sel)
{
	int sec = 3;

	switch(sel)
	{
		case OSD_TIMEOUT_3S:
			sec = 3;
			break;

		case OSD_TIMEOUT_5S:
			sec = 5;
			break;	

		case OSD_TIMEOUT_7S:
			sec = 7;
			break;	

		default:
			break;
	}
	
	return sec;
}

static void app_osd_system_para_get(void)
{
    int init_value = 0;

    GxBus_ConfigGetInt(TRANS_LEVEL_KEY, &init_value, TRANS_LEVEL);
    s_sys_osd_para.osd_tran= init_value;

    GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);
    s_sys_osd_para.bar_timeout= init_value;

    GxBus_ConfigGetInt(VOLUMEBAR_TIMEOUT_KEY, &init_value, VOLUMEBAR_TIMEOUT);
    s_sys_osd_para.vol_timeout= init_value;

    GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
    s_sys_osd_para.freeze_switch= init_value;

}

static void app_osd_result_para_get(SysOsdPara *ret_para)
{
    ret_para->osd_tran = app_sel_to_osd_trans(s_osd_item[ITEM_TRANS].itemProperty.itemPropertyCmb.sel);
    ret_para->bar_timeout = app_sel_to_widget_timeout(s_osd_item[ITEM_BAR_TIMEOUT].itemProperty.itemPropertyCmb.sel);
    ret_para->vol_timeout = s_sys_osd_para.vol_timeout;
    ret_para->freeze_switch= s_osd_item[ITEM_FREEZE].itemProperty.itemPropertyCmb.sel;
}

static bool app_osd_setting_para_change(SysOsdPara *ret_para)
{
	bool ret = FALSE;

	if(s_sys_osd_para.osd_tran!= ret_para->osd_tran)
	{
		ret = TRUE;
	}

	if(s_sys_osd_para.bar_timeout!= ret_para->bar_timeout)
	{
		ret = TRUE;
	}

	if(s_sys_osd_para.vol_timeout!= ret_para->vol_timeout)
	{
		ret = TRUE;
	}

	if(s_sys_osd_para.freeze_switch!= ret_para->freeze_switch)
	{
		ret = TRUE;
	}

    return ret;
}

static int app_osd_setting_para_save(SysOsdPara *ret_para)
{
    int init_value = 0;

    init_value = ret_para->osd_tran;
    GxBus_ConfigSetInt(TRANS_LEVEL_KEY, init_value);

    init_value = ret_para->bar_timeout;
    GxBus_ConfigSetInt(INFOBAR_TIMEOUT_KEY,init_value);

    init_value = ret_para->vol_timeout;
    GxBus_ConfigSetInt(VOLUMEBAR_TIMEOUT_KEY,init_value);

    init_value = ret_para->freeze_switch;
    GxBus_ConfigSetInt(FREEZE_SWITCH,init_value);
    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

	return 0;
}

static int app_osd_setting_exit_callback(ExitType exit_type)
{
	SysOsdPara para_ret = {0};
	int ret = 0;

	if(exit_type == EXIT_ABANDON)
	{
		GUI_SetInterface("osd_alpha_global", &s_sys_osd_para.osd_tran);
	}
	else
	{
		app_osd_result_para_get(&para_ret);
		if(app_osd_setting_para_change(&para_ret) == true)
		{
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.str = STR_ID_SAVE_INFO;
			if(popdlg_create(&pop) == POP_VAL_OK)
			{
				app_osd_setting_para_save(&para_ret);
				ret =1;
			}
			else
			{
				GUI_SetInterface("osd_alpha_global", &s_sys_osd_para.osd_tran);
			}
		}
	}
	return ret;
}

static int app_osd_trans_change_callback(int sel)
{
	int trans = OSD_TRANS_100PERCENT;

	trans = app_sel_to_osd_trans(sel);
	GUI_SetInterface("osd_alpha_global", &trans);
	//GUI_SetInterface("osd_alpha", &trans);

	return EVENT_TRANSFER_KEEPON;
}

static void app_osd_setting_trans_item_init(void)
{
	s_osd_item[ITEM_TRANS].itemTitle = STR_ID_TRAN;
	s_osd_item[ITEM_TRANS].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_TRANS].itemProperty.itemPropertyCmb.content= "[0%,10%,20%,30%,40%,50%,60%]";//"[40%,50%,60%,70%,80%,90%,100%]";
	s_osd_item[ITEM_TRANS].itemProperty.itemPropertyCmb.sel = app_osd_trans_to_sel(s_sys_osd_para.osd_tran);
	s_osd_item[ITEM_TRANS].itemCallback.cmbCallback.CmbChange= app_osd_trans_change_callback;
	s_osd_item[ITEM_TRANS].itemStatus = ITEM_NORMAL;
}

int app_osd_setting_trans_main_menu_exec(void)
{
	int init_value = 0, sel = 0;

    GxBus_ConfigGetInt(TRANS_LEVEL_KEY, &init_value, TRANS_LEVEL);
	sel = app_osd_trans_to_sel(init_value);

    return sel;
}

void app_osd_setting_trans_main_menu_change_save(int sel)
{
	int trans = OSD_TRANS_100PERCENT;

	trans = app_sel_to_osd_trans(sel);
	GUI_SetInterface("osd_alpha_global", &trans);
    GxBus_ConfigSetInt(TRANS_LEVEL_KEY, trans);
}

static void app_osd_setting_bar_timeout_item_init(void)
{
	s_osd_item[ITEM_BAR_TIMEOUT].itemTitle = STR_ID_OSD_TIMEOUT;
	s_osd_item[ITEM_BAR_TIMEOUT].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_BAR_TIMEOUT].itemProperty.itemPropertyCmb.content= "[3s,5s,7s]";
	s_osd_item[ITEM_BAR_TIMEOUT].itemProperty.itemPropertyCmb.sel = app_widget_timeout_to_sel(s_sys_osd_para.bar_timeout);
	s_osd_item[ITEM_BAR_TIMEOUT].itemCallback.cmbCallback.CmbChange= NULL;
	s_osd_item[ITEM_BAR_TIMEOUT].itemStatus = ITEM_NORMAL;
}

/*static void app_osd_setting_vol_timeout_item_init(void)
{
	s_osd_item[ITEM_VOL_TIMEOUT].itemTitle = STR_ID_VOL_TIMEOUT;
	s_osd_item[ITEM_VOL_TIMEOUT].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_VOL_TIMEOUT].itemProperty.itemPropertyCmb.content= "[3s,5s,7s]";
	s_osd_item[ITEM_VOL_TIMEOUT].itemProperty.itemPropertyCmb.sel = app_widget_timeout_to_sel(s_sys_osd_para.vol_timeout);
	s_osd_item[ITEM_VOL_TIMEOUT].itemCallback.cmbCallback.CmbChange= NULL;
	s_osd_item[ITEM_VOL_TIMEOUT].itemStatus = ITEM_NORMAL;
}*/

static void app_osd_setting_freeze_switch_item_init(void)
{
	s_osd_item[ITEM_FREEZE].itemTitle = STR_ID_FREEZE;
	s_osd_item[ITEM_FREEZE].itemType = ITEM_CHOICE;
#if 0//MINI_256_COLORS_OSD_SUPPORT
	s_osd_item[ITEM_FREEZE].itemProperty.itemPropertyCmb.content= "[Black]";
#else
	s_osd_item[ITEM_FREEZE].itemProperty.itemPropertyCmb.content= "[Black,Freeze]";
#endif
	s_osd_item[ITEM_FREEZE].itemProperty.itemPropertyCmb.sel = s_sys_osd_para.freeze_switch;
	s_osd_item[ITEM_FREEZE].itemCallback.cmbCallback.CmbChange= NULL;
	s_osd_item[ITEM_FREEZE].itemStatus = ITEM_NORMAL;
}

void app_osd_setting_menu_exec(void)
{
	memset(&s_osd_setting_opt, 0 ,sizeof(s_osd_setting_opt));
	
	app_osd_system_para_get();

	s_osd_setting_opt.menuTitle = STR_ID_OSD_SET;
	s_osd_setting_opt.itemNum = ITEM_OSD_TOTAL;
	s_osd_setting_opt.timeDisplay = TIP_HIDE;
	s_osd_setting_opt.item = s_osd_item;
	
	app_osd_setting_trans_item_init();
	app_osd_setting_bar_timeout_item_init();
	//app_osd_setting_vol_timeout_item_init();
	app_osd_setting_freeze_switch_item_init();

	s_osd_setting_opt.exit = app_osd_setting_exit_callback;

	app_system_set_create(&s_osd_setting_opt);
}




