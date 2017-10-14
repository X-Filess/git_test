#include "app.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "sys_setting_manage.h"
#include "app_pop.h"

enum COLOR_ITEM_NAME
{
	ITEM_BRIGHT = 0,
	ITEM_SATURATION,
	ITEM_CONTRAST,
	ITEM_COLOR_TOTAL,
};

static SystemSettingOpt s_color_setting_opt;
static SystemSettingItem s_color_item[ITEM_COLOR_TOTAL];
static VideoColor s_sys_color_para;

static int app_color_to_sel(int color)
{
	int ret;
	if(color < 20)
      color = 20;
	ret = color/10-2;
	return ret;
}

static int app_sel_to_color(int sel)
{
	int ret;
	ret = sel*10+20;
	return ret;
}

////////// sel[0,10] <==> contrast[10, 100] ////////
static int app_contrast_to_sel(int contrast)
{
	int ret;
	ret = (contrast - 10) / 9;

	ret = app_color_to_sel(contrast);
	return ret;
}

static int app_sel_to_contrast(int sel)
{
	int ret;
	ret = sel * 9 + 10;
	ret = app_sel_to_color(sel);
	return ret;
}

static void app_color_system_para_get(void)
{
	GxBus_ConfigGetInt(BRIGHTNESS_KEY, &s_sys_color_para.brightness, BRIGHTNESS);
	GxBus_ConfigGetInt(SATURATION_KEY, &s_sys_color_para.saturation, SATURATION);
	GxBus_ConfigGetInt(CONTRAST_KEY, &s_sys_color_para.contrast, CONTRAST);
}

static void app_color_result_para_get(VideoColor *ret_para)
{
    ret_para->brightness = app_sel_to_color(s_color_item[ITEM_BRIGHT].itemProperty.itemPropertyCmb.sel);
    ret_para->saturation = app_sel_to_color(s_color_item[ITEM_SATURATION].itemProperty.itemPropertyCmb.sel);
    ret_para->contrast = app_sel_to_contrast(s_color_item[ITEM_CONTRAST].itemProperty.itemPropertyCmb.sel);
}

static bool app_color_setting_para_change(VideoColor *ret_para)
{
	bool ret = FALSE;

	if(s_sys_color_para.brightness != ret_para->brightness)
	{
		ret = TRUE;
	}

	if(s_sys_color_para.saturation != ret_para->saturation)
	{
		ret = TRUE;
	}

	if(s_sys_color_para.contrast!= ret_para->contrast)
	{
		ret = TRUE;
	}

	return ret;
}

static int app_color_setting_para_save(VideoColor *ret_para)
{
	GxBus_ConfigSetInt(BRIGHTNESS_KEY, ret_para->brightness);
	GxBus_ConfigSetInt(SATURATION_KEY, ret_para->saturation);
	GxBus_ConfigSetInt(CONTRAST_KEY, ret_para->contrast);
	return 0;
}

static int app_color_setting_exit_callback(ExitType exit_type)
{
	VideoColor para_ret = {0};
	int ret = 0;

	if(exit_type == EXIT_ABANDON)
	{
		system_setting_set_screen_color(s_sys_color_para.saturation ,s_sys_color_para.brightness, s_sys_color_para.contrast);
	}
	else
    {
        app_color_result_para_get(&para_ret);
        if(app_color_setting_para_change(&para_ret) == TRUE)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.str = STR_ID_SAVE_INFO;
            if(popdlg_create(&pop) == POP_VAL_OK)
            {
                app_color_setting_para_save(&para_ret);
                ret = 1;
            }
            else
            {
                system_setting_set_screen_color(s_sys_color_para.saturation ,s_sys_color_para.brightness, s_sys_color_para.contrast);
            }
        }
    }

	return ret;
}

static int app_color_change_brighness_cb(int sel)
{
	VideoColor cur_para = {0};

	app_system_set_item_data_update(ITEM_BRIGHT);
//	app_system_set_item_data_update(ITEM_SATURATION);
//	app_system_set_item_data_update(ITEM_CONTRAST);
	app_color_result_para_get(&cur_para);
	system_setting_set_screen_color(cur_para.saturation ,cur_para.brightness, cur_para.contrast);

	return EVENT_TRANSFER_KEEPON;
}
static int app_color_change_saturation_cb(int sel)
{
	VideoColor cur_para = {0};

//	app_system_set_item_data_update(ITEM_BRIGHT);
	app_system_set_item_data_update(ITEM_SATURATION);
//	app_system_set_item_data_update(ITEM_CONTRAST);
	app_color_result_para_get(&cur_para);
	system_setting_set_screen_color(cur_para.saturation ,cur_para.brightness, cur_para.contrast);

	return EVENT_TRANSFER_KEEPON;
}

static int app_color_change_contrast_cb(int sel)
{
	VideoColor cur_para = {0};

//	app_system_set_item_data_update(ITEM_BRIGHT);
//	app_system_set_item_data_update(ITEM_SATURATION);
	app_system_set_item_data_update(ITEM_CONTRAST);
	app_color_result_para_get(&cur_para);
	system_setting_set_screen_color(cur_para.saturation ,cur_para.brightness, cur_para.contrast);

	return EVENT_TRANSFER_KEEPON;
}

static void app_color_bright_item_init(void)
{
	s_color_item[ITEM_BRIGHT].itemTitle = STR_ID_BRIGHT;
	s_color_item[ITEM_BRIGHT].itemType = ITEM_CHOICE;
	s_color_item[ITEM_BRIGHT].itemProperty.itemPropertyCmb.content= "[20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_color_item[ITEM_BRIGHT].itemProperty.itemPropertyCmb.sel = app_color_to_sel(s_sys_color_para.brightness);
	s_color_item[ITEM_BRIGHT].itemCallback.cmbCallback.CmbChange= app_color_change_brighness_cb;
	s_color_item[ITEM_BRIGHT].itemStatus = ITEM_NORMAL;
}

static void app_color_saturation_item_init(void)
{
	s_color_item[ITEM_SATURATION].itemTitle = STR_ID_CHROMA;
	s_color_item[ITEM_SATURATION].itemType = ITEM_CHOICE;
	s_color_item[ITEM_SATURATION].itemProperty.itemPropertyCmb.content= "[20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_color_item[ITEM_SATURATION].itemProperty.itemPropertyCmb.sel = app_color_to_sel(s_sys_color_para.saturation);
	s_color_item[ITEM_SATURATION].itemCallback.cmbCallback.CmbChange = app_color_change_saturation_cb;
	s_color_item[ITEM_SATURATION].itemStatus = ITEM_NORMAL;
}

static void app_color_contrast_item_init(void)
{
	s_color_item[ITEM_CONTRAST].itemTitle = STR_ID_CONTRAST;
	s_color_item[ITEM_CONTRAST].itemType = ITEM_CHOICE;
	s_color_item[ITEM_CONTRAST].itemProperty.itemPropertyCmb.content="[20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_color_item[ITEM_CONTRAST].itemProperty.itemPropertyCmb.sel = app_contrast_to_sel(s_sys_color_para.contrast);
	s_color_item[ITEM_CONTRAST].itemCallback.cmbCallback.CmbChange = app_color_change_contrast_cb;
	s_color_item[ITEM_CONTRAST].itemStatus = ITEM_NORMAL;
}

void app_color_setting_menu_exec(void)
{
	memset(&s_color_setting_opt, 0 ,sizeof(s_color_setting_opt));
	app_color_system_para_get();

	s_color_setting_opt.menuTitle = STR_ID_COLOR_SET;
	s_color_setting_opt.itemNum = ITEM_COLOR_TOTAL;
	s_color_setting_opt.timeDisplay = TIP_HIDE;
	s_color_setting_opt.item = s_color_item;

	app_color_bright_item_init();
	app_color_saturation_item_init();
	app_color_contrast_item_init();

	s_color_setting_opt.exit = app_color_setting_exit_callback;

	app_system_set_create(&s_color_setting_opt);

}

