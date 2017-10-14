#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "module/pm/gxprogram_manage_berkeley.h"
#include "module/gxtkgs.h"
#include "app_tkgs_upgrade.h"

#if TKGS_SUPPORT
enum TKGS_UPGRADE_ITEM_NAME 
{
	ITEM_OPERATE_MODE = 0,
	ITEM_LOCATION_SETTING,
	ITEM_PREFERRED_LIST,
	ITEM_START_UPGRADE,
	ITEM_HIDDEN_LOCATION,
	ITEM_VERSION,
	ITEM_TKGS_UPGRADE_TOTAL
};

extern int app_hidden_location_menu_exec(void);

static SystemSettingOpt s_tkgs_upgrade_opt;
static SystemSettingItem s_tkgs_upgrade_item[ITEM_TKGS_UPGRADE_TOTAL];
static GxTkgsOperatingMode s_tkgs_operateMode;
static GxTkgsOperatingMode s_tkgs_operateMode_bak;
static GxTkgsOperatingMode s_tkgs_operateMode_choice;
static char versionStr[10] = {0};

void app_tkgs_cantwork_popup(void)
{
	PopDlg  pop;
	memset(&pop, 0, sizeof(PopDlg));
	pop.type = POP_TYPE_OK;
	pop.str = STR_ID_TKGS_CANT_WORK;
	pop.mode = POP_MODE_UNBLOCK;
	pop.format = POP_FORMAT_DLG;
	popdlg_create(&pop);
}


void  app_tkgs_init_operatemode(void)
{
	GxTkgsOperatingMode mode = GX_TKGS_AUTOMATIC;
	app_send_msg_exec(GXMSG_TKGS_GET_OPERATING_MODE,&mode);
	s_tkgs_operateMode = mode;
}

GxTkgsOperatingMode app_tkgs_get_operatemode(void)
{
	return s_tkgs_operateMode;
}


void app_tkgs_set_operatemode_var(GxTkgsOperatingMode mode)
{
	s_tkgs_operateMode = mode;
}


void app_tkgs_set_operatemode(int mode)
{
	app_send_msg_exec(GXMSG_TKGS_SET_OPERATING_MODE,&mode);
	app_tkgs_set_operatemode_var((GxTkgsOperatingMode)mode);
}


#if 0
int app_tkgs_PmViewIsUpgrade(uint32_t operateMode)
{
	GxBusPmViewInfo sys = {0};
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)&sys);
	GxBus_PmViewInfoGet(&sys);
	app_tkgs_set_operatemode(operateMode);
	if(operateMode == GX_TKGS_AUTOMATIC)
	{
		if(sys.taxis_mode != TAXIS_MODE_LCN)
		{
			sys.taxis_mode = TAXIS_MODE_LCN;
			app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)&sys);
			g_AppPlayOps.normal_play.view_info.taxis_mode = TAXIS_MODE_LCN;
			return TRUE;
		}
	}
	else
	{
		if(sys.taxis_mode == TAXIS_MODE_LCN)
		{
			sys.taxis_mode = TAXIS_MODE_NON;
			app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)&sys);
			g_AppPlayOps.normal_play.view_info.taxis_mode =TAXIS_MODE_NON;
			return TRUE;
		}
		
	}
	return FALSE;
}

int32_t app_tkgs_pm_prog_check(GxBusPmDataProg* prog, GxBusPmProgSort* list)
{	
	if(s_tkgs_operateMode == GX_TKGS_OFF)
	{
		return 0;
	}
	else
	{
		AppProgExtInfo prog_ext_info = {0};
		
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(prog->id,sizeof(AppProgExtInfo),&prog_ext_info))
		{
			if(prog_ext_info.locator_id == TKGS_INVALID_LOCATOR)
			{
				return 0;
			}		
			
			if(prog_ext_info.logicnum != TKGS_INVALID_LCN)
			{
				return 0;
			}
		}
	}
	return -1;
}
#endif
static void app_tkgs_upgrade_para_get(void)
{
	app_tkgs_init_operatemode();
	s_tkgs_operateMode_bak = s_tkgs_operateMode;
	s_tkgs_operateMode_choice = s_tkgs_operateMode;
}

static int app_tkgs_upgrade_operate_mode_change_callback(int sel)
{
	s_tkgs_operateMode_choice = sel;
	return EVENT_TRANSFER_KEEPON;
}

static void app_tkgs_upgrade_operate_mode_item_init(void)
{
	s_tkgs_upgrade_item[ITEM_OPERATE_MODE].itemTitle = STR_ID_OPERATE_MODE;
	s_tkgs_upgrade_item[ITEM_OPERATE_MODE].itemType = ITEM_CHOICE;
	s_tkgs_upgrade_item[ITEM_OPERATE_MODE].itemProperty.itemPropertyCmb.content= "[Auto,Customer,Off]";
	s_tkgs_upgrade_item[ITEM_OPERATE_MODE].itemProperty.itemPropertyCmb.sel = s_tkgs_operateMode_choice;
	s_tkgs_upgrade_item[ITEM_OPERATE_MODE].itemCallback.cmbCallback.CmbChange= app_tkgs_upgrade_operate_mode_change_callback;
	s_tkgs_upgrade_item[ITEM_OPERATE_MODE].itemStatus = ITEM_NORMAL;
}

extern int app_visible_location_menu_exec(void);
static int app_tkgs_upgrade_visible_location_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
		app_visible_location_menu_exec();
		ret = EVENT_TRANSFER_STOP;
		break;
	}
	return ret;
}

static void app_tkgs_upgrade_visible_location_item_init(void)
{
	s_tkgs_upgrade_item[ITEM_LOCATION_SETTING].itemTitle = STR_ID_VISIABLE_LOCATION;
	s_tkgs_upgrade_item[ITEM_LOCATION_SETTING].itemType = ITEM_PUSH;
	s_tkgs_upgrade_item[ITEM_LOCATION_SETTING].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
	s_tkgs_upgrade_item[ITEM_LOCATION_SETTING].itemCallback.btnCallback.BtnPress = app_tkgs_upgrade_visible_location_callback;
	s_tkgs_upgrade_item[ITEM_LOCATION_SETTING].itemStatus = ITEM_NORMAL;
}

static int app_tkgs_upgrade_hidden_location_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
		app_hidden_location_menu_exec();
		ret = EVENT_TRANSFER_STOP;
		break;
	}
	return ret;
}

static void app_tkgs_upgrade_hidden_location_item_init(void)
{
	s_tkgs_upgrade_item[ITEM_HIDDEN_LOCATION].itemTitle = STR_ID_HIDDEN_LOCATION;
	s_tkgs_upgrade_item[ITEM_HIDDEN_LOCATION].itemType = ITEM_PUSH;
	s_tkgs_upgrade_item[ITEM_HIDDEN_LOCATION].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
	s_tkgs_upgrade_item[ITEM_HIDDEN_LOCATION].itemCallback.btnCallback.BtnPress = app_tkgs_upgrade_hidden_location_callback;
	s_tkgs_upgrade_item[ITEM_HIDDEN_LOCATION].itemStatus = ITEM_NORMAL;
}

extern int app_preferred_list_menu_exec(void);
static int app_tkgs_upgrade_preferred_list_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
		app_preferred_list_menu_exec();
		ret = EVENT_TRANSFER_STOP;
		break;
	}
	return ret;
}

static void app_tkgs_upgrade_preferred_list_item_init(void)
{
	s_tkgs_upgrade_item[ITEM_PREFERRED_LIST].itemTitle = STR_ID_PREFERRED_LIST;
	s_tkgs_upgrade_item[ITEM_PREFERRED_LIST].itemType = ITEM_PUSH;
	s_tkgs_upgrade_item[ITEM_PREFERRED_LIST].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
	s_tkgs_upgrade_item[ITEM_PREFERRED_LIST].itemCallback.btnCallback.BtnPress = app_tkgs_upgrade_preferred_list_callback;
	s_tkgs_upgrade_item[ITEM_PREFERRED_LIST].itemStatus = ITEM_NORMAL;
}

static int app_tkgs_upgrade_start_upgrade_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
		{
			GxTkgsLocation s_tkgsLocation;
			
			if(s_tkgs_operateMode_bak != s_tkgs_operateMode_choice)
			{
				s_tkgs_operateMode_bak = s_tkgs_operateMode_choice;
				app_tkgs_set_operatemode(s_tkgs_operateMode_choice);
			//	app_tkgs_PmViewIsUpgrade(s_tkgs_operateMode_choice);//2014.09.25, del
			}
			s_tkgsLocation.visible_num = 0;
			s_tkgsLocation.invisible_num = 0;
			app_send_msg_exec(GXMSG_TKGS_GET_LOCATION,&s_tkgsLocation);
			
			if((s_tkgsLocation.visible_num>0)
				||(s_tkgsLocation.invisible_num>0))
			{
				app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_MENU);
				app_tkgs_upgrade_process_menu_exec();
			}
			else
			{
				PopDlg  pop;
				memset(&pop, 0, sizeof(PopDlg));
				pop.type = POP_TYPE_OK;
				pop.str = STR_ID_TKGS_SET_LOCATION;
				pop.mode = POP_MODE_UNBLOCK;
				pop.format = POP_FORMAT_DLG;
				popdlg_create(&pop);
			}
			ret = EVENT_TRANSFER_STOP;
		}
		break;
	}
	return ret;
	
}


static void app_tkgs_upgrade_start_upgrade_item_init(void)
{
	s_tkgs_upgrade_item[ITEM_START_UPGRADE].itemTitle = STR_ID_START_UPDATE;
	s_tkgs_upgrade_item[ITEM_START_UPGRADE].itemType = ITEM_PUSH;
	s_tkgs_upgrade_item[ITEM_START_UPGRADE].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
	s_tkgs_upgrade_item[ITEM_START_UPGRADE].itemCallback.btnCallback.BtnPress = app_tkgs_upgrade_start_upgrade_callback;
	s_tkgs_upgrade_item[ITEM_START_UPGRADE].itemStatus = ITEM_NORMAL;
}

static int app_tkgs_clear_version_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;
	int version = 0;
	ItemProPerty property;

	switch(key)
	{
		case STBK_OK:
		{
			app_send_msg_exec(GXMSG_TKGS_TEST_RESET_VERNUM, NULL);
			app_send_msg_exec(GXMSG_TKGS_TEST_GET_VERNUM, &version);
			sprintf(versionStr, "%d", version);
			property.itemPropertyBtn.string = versionStr;
			app_system_set_item_property(ITEM_VERSION,&property);
			
			ret = EVENT_TRANSFER_STOP;
		}
		break;
	}
	return ret;
	
}

static void app_tkgs_version_item_init(void)
{
	int version = 0;
	
	app_send_msg_exec(GXMSG_TKGS_TEST_GET_VERNUM, &version);
	sprintf(versionStr, "%d", version);
	s_tkgs_upgrade_item[ITEM_VERSION].itemTitle = STR_ID_TKGS_VERSION;
	s_tkgs_upgrade_item[ITEM_VERSION].itemType = ITEM_PUSH;
	s_tkgs_upgrade_item[ITEM_VERSION].itemProperty.itemPropertyBtn.string = versionStr;
	s_tkgs_upgrade_item[ITEM_VERSION].itemCallback.btnCallback.BtnPress = app_tkgs_clear_version_callback;
	s_tkgs_upgrade_item[ITEM_VERSION].itemStatus = ITEM_NORMAL;
}


static int app_tkgs_upgrade_exit_callback(ExitType exit_type)
{
	int ret = 0;

	if(exit_type == EXIT_ABANDON)
	{
		;
	}
	else
	{
		
		if(s_tkgs_operateMode_choice != s_tkgs_operateMode_bak)
		{
#if 0	//2014.09.25, del	
			PopDlg  pop;
			memset(&pop, 0, sizeof(PopDlg));
	        	pop.type = POP_TYPE_YES_NO;
	       	pop.str = STR_ID_SAVE_INFO;
			if(popdlg_create(&pop) == POP_VAL_OK)
			{
				app_tkgs_set_operatemode(s_tkgs_operateMode_choice);
				if(app_tkgs_PmViewIsUpgrade(s_tkgs_operateMode_choice))
				{
					GxBus_PmViewInfoMemClear();
					GxBus_PmProgListRebulid();	
					
				}
				ret =1;
			}
#else
		app_tkgs_set_operatemode(s_tkgs_operateMode_choice);	
#endif			
		}
	}
	return ret;
}

void app_tkgs_update_ver_disp(void)
{
	int version = 0;
	ItemProPerty property;

	app_send_msg_exec(GXMSG_TKGS_TEST_GET_VERNUM, &version);
	sprintf(versionStr, "%d", version);
	property.itemPropertyBtn.string = versionStr;
	app_system_set_item_property(ITEM_VERSION,&property);
}

void app_init_tkgs_info()
{
	uint8_t pos= 0;
	app_send_msg_exec(GXMSG_TKGS_GET_PREFERRED_LIST,&pos);
	app_tkgs_set_operatemode(GX_TKGS_AUTOMATIC);
}



void app_tkgs_upgrade_menu_exec(void)
{
	memset(&s_tkgs_upgrade_opt, 0 ,sizeof(s_tkgs_upgrade_opt));
	
	app_tkgs_upgrade_para_get();

	s_tkgs_upgrade_opt.menuTitle = STR_ID_TKGS_UPGRATE;
	s_tkgs_upgrade_opt.itemNum = ITEM_TKGS_UPGRADE_TOTAL;
	s_tkgs_upgrade_opt.timeDisplay = TIP_HIDE;
	s_tkgs_upgrade_opt.item = s_tkgs_upgrade_item;
	app_tkgs_upgrade_operate_mode_item_init();
	app_tkgs_upgrade_visible_location_item_init();
	app_tkgs_upgrade_preferred_list_item_init();
	app_tkgs_upgrade_start_upgrade_item_init();
	app_tkgs_upgrade_hidden_location_item_init();
	app_tkgs_version_item_init();
	s_tkgs_upgrade_opt.exit = app_tkgs_upgrade_exit_callback;
	app_system_set_create(&s_tkgs_upgrade_opt);
}

#endif

