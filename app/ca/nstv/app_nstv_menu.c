/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_menu.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.09.27		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#include "app_config.h"
#include "app_cascam_pop.h"

#if NSCAS_SUPPORT

extern void app_nstv_baseinfo_exec(void);
extern void app_nstv_operator_exec(void);
extern void app_nstv_maturity_exec(void);
extern void app_nstv_work_time_exec(void);
extern void app_nstv_pin_exec(void);
extern void app_nstv_mail_exec(int fullscreen_flag);
extern void app_nstv_update_info_exec(void);
extern void app_nstv_card_pair_info_exec(void);

enum
{
    ITEM_BASEINFO,
    ITEM_OPERATOR,
	ITEM_MATURITY,
#if NSCAS_SMC_VERSION
	ITEM_WORK_TIME,
#endif
    ITEM_PIN_CONTROL,
    ITEM_EMAIL,
#if NSCAS_SMC_VERSION
    ITEM_UPDATE_INFO,
    ITEM_CARD_PAIR_INFO,
#endif
    ITEM_NSTV_TOTAL,
};

static SystemSettingOpt thiz_nstv_setting_opt;
static SystemSettingItem thiz_nstv_setting_item[ITEM_NSTV_TOTAL];

static int _baseinfo_item_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
			app_nstv_baseinfo_exec();
			break;
		default:
			break;
	}
	return ret;
}

static void _baseinfo_item_init(void)
{
	thiz_nstv_setting_item[ITEM_BASEINFO].itemTitle = "Basic Information";
	thiz_nstv_setting_item[ITEM_BASEINFO].itemType = ITEM_PUSH;
	thiz_nstv_setting_item[ITEM_BASEINFO].itemProperty.itemPropertyBtn.string = "Press Ok";
	thiz_nstv_setting_item[ITEM_BASEINFO].itemCallback.btnCallback.BtnPress = _baseinfo_item_press_callback;
	thiz_nstv_setting_item[ITEM_BASEINFO].itemStatus = ITEM_NORMAL;
}

static int _operator_item_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
			app_nstv_operator_exec();
			break;
		default:
			break;
	}
	return ret;
}

static void _operator_item_init(void)
{
	thiz_nstv_setting_item[ITEM_OPERATOR].itemTitle = "Operator Information";
	thiz_nstv_setting_item[ITEM_OPERATOR].itemType = ITEM_PUSH;
	thiz_nstv_setting_item[ITEM_OPERATOR].itemProperty.itemPropertyBtn.string = "Press Ok";
	thiz_nstv_setting_item[ITEM_OPERATOR].itemCallback.btnCallback.BtnPress = _operator_item_press_callback;
	thiz_nstv_setting_item[ITEM_OPERATOR].itemStatus = ITEM_NORMAL;
}

static int _maturity_item_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
			app_nstv_maturity_exec();
			break;
		default:
			break;
	}
	return ret;
}

static void _maturity_item_init(void)
{
	thiz_nstv_setting_item[ITEM_MATURITY].itemTitle = "Maturity Setting";
	thiz_nstv_setting_item[ITEM_MATURITY].itemType = ITEM_PUSH;
	thiz_nstv_setting_item[ITEM_MATURITY].itemProperty.itemPropertyBtn.string = "Press Ok";
	thiz_nstv_setting_item[ITEM_MATURITY].itemCallback.btnCallback.BtnPress = _maturity_item_press_callback;
	thiz_nstv_setting_item[ITEM_MATURITY].itemStatus = ITEM_NORMAL;
}

#if NSCAS_SMC_VERSION
static int _work_time_item_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
			app_nstv_work_time_exec();
			break;
		default:
			break;
	}
	return ret;
}

static void _work_time_item_init(void)
{
	thiz_nstv_setting_item[ITEM_WORK_TIME].itemTitle = "Work Time Setting";
	thiz_nstv_setting_item[ITEM_WORK_TIME].itemType = ITEM_PUSH;
	thiz_nstv_setting_item[ITEM_WORK_TIME].itemProperty.itemPropertyBtn.string = "Press Ok";
	thiz_nstv_setting_item[ITEM_WORK_TIME].itemCallback.btnCallback.BtnPress = _work_time_item_press_callback;
	thiz_nstv_setting_item[ITEM_WORK_TIME].itemStatus = ITEM_NORMAL;
}
#endif

static int _pin_control_item_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
			app_nstv_pin_exec();
			break;
		default:
			break;
	}
	return ret;
}

static void _pin_control_item_init(void)
{
	thiz_nstv_setting_item[ITEM_PIN_CONTROL].itemTitle = "PIN Setting";
	thiz_nstv_setting_item[ITEM_PIN_CONTROL].itemType = ITEM_PUSH;
	thiz_nstv_setting_item[ITEM_PIN_CONTROL].itemProperty.itemPropertyBtn.string = "Press Ok";
	thiz_nstv_setting_item[ITEM_PIN_CONTROL].itemCallback.btnCallback.BtnPress = _pin_control_item_press_callback;
	thiz_nstv_setting_item[ITEM_PIN_CONTROL].itemStatus = ITEM_NORMAL;
}


static int _email_item_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
			app_nstv_mail_exec(0);
			break;
		default:
			break;
	}
	return ret;
}

static void _email_item_init(void)
{
	thiz_nstv_setting_item[ITEM_EMAIL].itemTitle = "Email List";
	thiz_nstv_setting_item[ITEM_EMAIL].itemType = ITEM_PUSH;
	thiz_nstv_setting_item[ITEM_EMAIL].itemProperty.itemPropertyBtn.string = "Press Ok";
	thiz_nstv_setting_item[ITEM_EMAIL].itemCallback.btnCallback.BtnPress = _email_item_press_callback;
	thiz_nstv_setting_item[ITEM_EMAIL].itemStatus = ITEM_NORMAL;
}

#if NSCAS_SMC_VERSION
static int _update_info_item_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
			app_nstv_update_info_exec();
			break;
		default:
			break;
	}
	return ret;
}

static void _update_info_item_init(void)
{
	thiz_nstv_setting_item[ITEM_UPDATE_INFO].itemTitle = "Card Update Information";
	thiz_nstv_setting_item[ITEM_UPDATE_INFO].itemType = ITEM_PUSH;
	thiz_nstv_setting_item[ITEM_UPDATE_INFO].itemProperty.itemPropertyBtn.string = "Press Ok";
	thiz_nstv_setting_item[ITEM_UPDATE_INFO].itemCallback.btnCallback.BtnPress = _update_info_item_press_callback;
	thiz_nstv_setting_item[ITEM_UPDATE_INFO].itemStatus = ITEM_NORMAL;
}

static int _card_pair_info_item_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
			app_nstv_card_pair_info_exec();
			break;
		default:
			break;
	}
	return ret;
}

static void _crad_pair_info_item_init(void)
{
	thiz_nstv_setting_item[ITEM_CARD_PAIR_INFO].itemTitle = "Card Pair Information";
	thiz_nstv_setting_item[ITEM_CARD_PAIR_INFO].itemType = ITEM_PUSH;
	thiz_nstv_setting_item[ITEM_CARD_PAIR_INFO].itemProperty.itemPropertyBtn.string = "Press Ok";
	thiz_nstv_setting_item[ITEM_CARD_PAIR_INFO].itemCallback.btnCallback.BtnPress = _card_pair_info_item_press_callback;
	thiz_nstv_setting_item[ITEM_CARD_PAIR_INFO].itemStatus = ITEM_NORMAL;
}

#endif
void app_nstv_menu_exec(void)
{
    memset(&thiz_nstv_setting_opt, 0 ,sizeof(thiz_nstv_setting_opt));
    thiz_nstv_setting_opt.menuTitle = STR_ID_SPECIAL;
    thiz_nstv_setting_opt.titleImage = "s_title_left_utility.bmp";
    thiz_nstv_setting_opt.itemNum = ITEM_NSTV_TOTAL;
    thiz_nstv_setting_opt.timeDisplay = TIP_HIDE;
    thiz_nstv_setting_opt.item = thiz_nstv_setting_item;
    thiz_nstv_setting_opt.exit = NULL;

    _baseinfo_item_init();
    _operator_item_init();
    _maturity_item_init();
#if NSCAS_SMC_VERSION
    _work_time_item_init();
#endif
    _pin_control_item_init();
    _email_item_init();
#if NSCAS_SMC_VERSION
    _update_info_item_init();
    _crad_pair_info_item_init();
#endif

    app_system_set_create(&thiz_nstv_setting_opt);
}

void app_nstv_init_dialog(void)
{
extern void app_nstv_rolling_osd_create(CmmOsdRollingClass *param);
extern void app_nstv_rolling_osd_exit(int style);
extern void app_nstv_osd_check_rolling_osd(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
extern void app_nstv_osd_get_rolling_times(unsigned int *times, int style);
extern void app_nstv_fingerprint_create(CmmFingerprintClass *param);
extern void app_nstv_fingerprint_exit(CmmFingerTypeEnum type);
//extern void app_nstv_ippv_notify_create(CmmComplexClass *param);
//extern void app_nstv_ippv_notify_exit(int type);
extern void app_nstv_message_receive(CmmSuperMessageClass *param);
extern void app_nstv_message_destroy(CmmSuperMessageClass *param);
extern int app_nstv_chaeck_force_lock_status(void);
extern void app_nstv_osd_check_for_hide(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
extern void app_nstv_fingerprint_water_mark_reshow(void);

    CMMOSDClass func = {0};
    func.CMMCreateDialog3 = app_nstv_rolling_osd_create;
    func.CMMExitDialog3 = app_nstv_rolling_osd_exit;
    func.CMMCheckRollingRectOverlap = app_nstv_osd_check_rolling_osd;
    func.CMMGetRollingTimes = app_nstv_osd_get_rolling_times;
    func.CMMCreateDialog6   = app_nstv_fingerprint_create;
    func.CMMExitDialog6     = app_nstv_fingerprint_exit;
    func.CMMCreateDialogx   = app_nstv_message_receive;
    func.CMMExitDialogx     = app_nstv_message_destroy;
    func.CMMCheckServiceLockStatus = app_nstv_chaeck_force_lock_status;
    func.CMMCheckFingerRectOverlap = app_nstv_osd_check_for_hide;
    func.CMMCheckFingerResume = app_nstv_fingerprint_water_mark_reshow;

    cmm_ca_dialog_func_init(func);
}
#endif

