#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_config.h"

#if CASCAM_SUPPORT

enum ITEM_NAME 
{
#if NSCAS_SUPPORT
    ITEM_NSTV,
#endif
	ITEM_CAS_TOTAL
};

static SystemSettingOpt thiz_cascam_setting_opt;
static SystemSettingItem thiz_cascam_setting_item[ITEM_CAS_TOTAL];

#if NSCAS_SUPPORT
extern void app_nstv_menu_exec(void);
static int _nstv_item_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
			app_nstv_menu_exec();
			break;
		default:
			break;
	}
	return ret;
}
static void _nstv_item_init(void)
{
	thiz_cascam_setting_item[ITEM_NSTV].itemTitle = "NSTV CAS";
	thiz_cascam_setting_item[ITEM_NSTV].itemType = ITEM_PUSH;
	thiz_cascam_setting_item[ITEM_NSTV].itemProperty.itemPropertyBtn.string = "Press Ok";
	thiz_cascam_setting_item[ITEM_NSTV].itemCallback.btnCallback.BtnPress = _nstv_item_press_callback;
	thiz_cascam_setting_item[ITEM_NSTV].itemStatus = ITEM_NORMAL;
}
#endif

static void _single_cas_enter(CASCAMCASEnum type)
{
    switch(type)
    {
#if NSCAS_SUPPORT
        case CASCAM_NSTV:
            app_nstv_menu_exec();
            break;
#endif
        default:
            break;
    }
}

void app_ca_menu_exec(void)
{
	if(ITEM_CAS_TOTAL == 0)
	{
		printf("\n-------no one need to setting----\n");
		return;
	}

    if(1)// need count
    {
#if NSCAS_SUPPORT
        _single_cas_enter(CASCAM_NSTV);
#endif
    }
    else
    {// multi cas enter
	    memset(&thiz_cascam_setting_opt, 0 ,sizeof(thiz_cascam_setting_opt));
	    thiz_cascam_setting_opt.menuTitle = STR_ID_SPECIAL;
	    thiz_cascam_setting_opt.titleImage = "s_title_left_utility.bmp";
	    thiz_cascam_setting_opt.itemNum = ITEM_CAS_TOTAL;
	    thiz_cascam_setting_opt.timeDisplay = TIP_HIDE;
	    thiz_cascam_setting_opt.item = thiz_cascam_setting_item;
        thiz_cascam_setting_opt.exit = NULL;

#if NSCAS_SUPPORT
        _nstv_item_init();
#endif
	    app_system_set_create(&thiz_cascam_setting_opt);
    }
}
#endif


