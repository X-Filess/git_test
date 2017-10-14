#include "app.h"
#if (PPPOE_SUPPORT > 0)
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_utility.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_book.h"


#define PPPOE_EDIT_LEN 32

#define STR_ID_USER "User"
#define STR_ID_PASSWORD "Password"

enum PPPOE_ITEM_NAME
{
	ITEM_PPPOE_MODE,
	ITEM_PPPOE_USERNAME,
	ITEM_PPPOE_PASSWORD,
    ITEM_PPPOE_DNS1,
    ITEM_PPPOE_DNS2,
	ITEM_PPPOE_SAVE,
	ITEM_PPPOE_TOTAL
};

typedef struct str_pppoe
{
    char mode;
	char username[PPPOE_EDIT_LEN + 1];
	char password[PPPOE_EDIT_LEN + 1];
    ubyte32 dns1;
    ubyte32 dns2;
}PppoeData;

static PppoeData s_pppoe_data;
static PppoeData s_pppoe_data_trace;
static SystemSettingOpt s_pppoe_opt;
static SystemSettingItem s_pppoe_item[ITEM_PPPOE_TOTAL];
static ubyte64 s_pppoe_dev = {0};

static bool app_pppoe_cfg_change(void)
{
    if(s_pppoe_data_trace.mode != s_pppoe_data.mode)
        return true;

    if(s_pppoe_data_trace.mode == 1)
    {
        if(strcmp(s_pppoe_data_trace.username, s_pppoe_data.username) != 0)
            return true;

        if(strcmp(s_pppoe_data_trace.password, s_pppoe_data.password) != 0)
            return true;

        if(strcmp(s_pppoe_data_trace.dns1, s_pppoe_data.dns1) != 0)
            return true;

        if(strcmp(s_pppoe_data_trace.dns2, s_pppoe_data.dns2) != 0)
            return true;
    }

    return false;
}

static void app_pppoe_cfg_save(void)
{
	GxMsgProperty_PppoeCfg pppoe_cfg;

    memset(&pppoe_cfg, 0, sizeof(GxMsgProperty_PppoeCfg));
    strcpy(pppoe_cfg.dev_name, s_pppoe_dev);
    pppoe_cfg.mode = s_pppoe_data_trace.mode;
    strcpy(pppoe_cfg.username, s_pppoe_data_trace.username);
    strcpy(pppoe_cfg.passwd, s_pppoe_data_trace.password);
    app_network_ip_simplify(pppoe_cfg.dns1, s_pppoe_data_trace.dns1);
    app_network_ip_simplify(pppoe_cfg.dns2, s_pppoe_data_trace.dns2);
    printf(" === [%s]%d mode: %d, user: %s, passwd: %s, dns1: %s, dns2: %s === \n",
            __func__, __LINE__, pppoe_cfg.mode, pppoe_cfg.username, pppoe_cfg.passwd, pppoe_cfg.dns1, pppoe_cfg.dns2);
    app_send_msg_exec(GXMSG_PPPOE_SET_PARAM, &pppoe_cfg);
	//同步两个数据，使得在界面中多次保存操作时正确
    memcpy(&s_pppoe_data, &s_pppoe_data_trace, sizeof(PppoeData));
}

static void app_pppoe_username_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;

    memset(s_pppoe_data_trace.username, 0, strlen(s_pppoe_data_trace.username) + 1);
    strncpy(s_pppoe_data_trace.username, data->out_name, strlen(data->out_name));
    //printf(" === [%s]%d user: %s=== \n", __func__, __LINE__, s_pppoe_data_trace.username);
    s_pppoe_item[ITEM_PPPOE_USERNAME].itemProperty.itemPropertyBtn.string = s_pppoe_data_trace.username;
    app_system_set_item_property(ITEM_PPPOE_USERNAME, &s_pppoe_item[ITEM_PPPOE_USERNAME].itemProperty);
}

static void app_pppoe_password_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;

    memset(s_pppoe_data_trace.password, 0, strlen(s_pppoe_data_trace.password) + 1);
    strncpy(s_pppoe_data_trace.password, data->out_name, strlen(data->out_name));
    //printf(" === [%s]%d pwd: %s=== \n", __func__, __LINE__, s_pppoe_data_trace.password);
    //printf("data->out_name: %s \n",data->out_name);
}

static int app_pppoe_username_btn_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;
	static PopKeyboard keyboard;

	if(key == STBK_OK)
	{
		memset(&keyboard, 0, sizeof(PopKeyboard));
		keyboard.in_name    = s_pppoe_data.username;
		keyboard.max_num = PPPOE_EDIT_LEN;
		keyboard.out_name   = NULL;
		keyboard.change_cb  = NULL;
		keyboard.release_cb = app_pppoe_username_keyboard_proc;
		keyboard.usr_data   = NULL;
		keyboard.pos.x = 500;
		multi_language_keyboard_create(&keyboard);
		ret = EVENT_TRANSFER_STOP;
	}
	return ret;
}

static int app_pppoe_password_btn_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;
	static PopKeyboard keyboard;

	if(key == STBK_OK)
    {
        memset(&keyboard, 0, sizeof(PopKeyboard));
        keyboard.in_name    = s_pppoe_data.password;
        keyboard.max_num = PPPOE_EDIT_LEN;
        keyboard.out_name   = NULL;
        keyboard.change_cb  = NULL;
        keyboard.release_cb = app_pppoe_password_keyboard_proc;
        keyboard.usr_data   = NULL;
        keyboard.pos.x = 500;
        multi_language_keyboard_create(&keyboard);
        ret = EVENT_TRANSFER_STOP;
    }
	return ret;
}

static int app_pppoe_save_press_callback(unsigned short key)
{
    if(key == STBK_OK)
    {
        app_system_set_item_data_update(ITEM_PPPOE_MODE);
        s_pppoe_data_trace.mode = s_pppoe_item[ITEM_PPPOE_MODE].itemProperty.itemPropertyCmb.sel;

        app_system_set_item_data_update(ITEM_PPPOE_DNS1);
        strcpy(s_pppoe_data_trace.dns1, s_pppoe_item[ITEM_PPPOE_DNS1].itemProperty.itemPropertyEdit.string);

        app_system_set_item_data_update(ITEM_PPPOE_DNS2);
        strcpy(s_pppoe_data_trace.dns2, s_pppoe_item[ITEM_PPPOE_DNS2].itemProperty.itemPropertyEdit.string);

        if(app_pppoe_cfg_change() == true)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.mode = POP_MODE_UNBLOCK;
            pop.str = STR_ID_WAITING;
            pop.timeout_sec = 1;
            popdlg_create(&pop);
            app_pppoe_cfg_save();
        }
    }
    return EVENT_TRANSFER_KEEPON;
}

static int app_pppoe_mode_change_callback(int sel)
{
    if(sel == 0)
    {
        app_system_set_item_state_update(ITEM_PPPOE_USERNAME, ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_PPPOE_PASSWORD, ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_PPPOE_DNS1, ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_PPPOE_DNS2, ITEM_DISABLE);
    }
    else
    {
        app_system_set_item_state_update(ITEM_PPPOE_USERNAME, ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_PPPOE_PASSWORD, ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_PPPOE_DNS1, ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_PPPOE_DNS2, ITEM_NORMAL);
    }
    return EVENT_TRANSFER_KEEPON;
}


static int app_pppoe_setting_exit_callback(ExitType exit_type)
{
    int ret = 0;
#if 0
    s_pppoe_data_trace.mode = s_pppoe_item[ITEM_PPPOE_MODE].itemProperty.itemPropertyCmb.sel;
    //strcpy(s_pppoe_data_trace.username, s_pppoe_item[ITEM_PPPOE_USERNAME].itemProperty.itemPropertyEdit.string);
    //strcpy(s_pppoe_data_trace.password, s_pppoe_item[ITEM_PPPOE_PASSWORD].itemProperty.itemPropertyEdit.string);
    strcpy(s_pppoe_data_trace.dns1, s_pppoe_item[ITEM_PPPOE_DNS1].itemProperty.itemPropertyEdit.string);
    strcpy(s_pppoe_data_trace.dns2, s_pppoe_item[ITEM_PPPOE_DNS2].itemProperty.itemPropertyEdit.string);

    if(app_pppoe_cfg_change() == true)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_SAVE_INFO;
        if(popdlg_create(&pop) == POP_VAL_OK)
        {
            app_pppoe_cfg_save();
            ret = 1;
        }
    }
#endif
    return ret;
}

static void app_pppoe_setting_mode_item_init(void)
{
	s_pppoe_item[ITEM_PPPOE_MODE].itemTitle = STR_ID_PPPOE;
	s_pppoe_item[ITEM_PPPOE_MODE].itemType = ITEM_CHOICE;
	s_pppoe_item[ITEM_PPPOE_MODE].itemProperty.itemPropertyCmb.content= "[Off,On]";
	s_pppoe_item[ITEM_PPPOE_MODE].itemProperty.itemPropertyCmb.sel = s_pppoe_data.mode;
	s_pppoe_item[ITEM_PPPOE_MODE].itemCallback.cmbCallback.CmbChange = app_pppoe_mode_change_callback;
	s_pppoe_item[ITEM_PPPOE_MODE].itemStatus = ITEM_NORMAL;
}

static void app_pppoe_setting_user_item_init(void)
{
	s_pppoe_item[ITEM_PPPOE_USERNAME].itemTitle = STR_ID_USER;
	s_pppoe_item[ITEM_PPPOE_USERNAME].itemType = ITEM_PUSH;
	s_pppoe_item[ITEM_PPPOE_USERNAME].itemProperty.itemPropertyBtn.string = s_pppoe_data.username;
	s_pppoe_item[ITEM_PPPOE_USERNAME].itemCallback.btnCallback.BtnPress= app_pppoe_username_btn_callback;
    if(s_pppoe_data.mode == 0)
        s_pppoe_item[ITEM_PPPOE_USERNAME].itemStatus = ITEM_DISABLE;
    else
        s_pppoe_item[ITEM_PPPOE_USERNAME].itemStatus = ITEM_NORMAL;
}

static void app_pppoe_setting_password_item_init(void)
{
	s_pppoe_item[ITEM_PPPOE_PASSWORD].itemTitle = STR_ID_PASSWORD;
	s_pppoe_item[ITEM_PPPOE_PASSWORD].itemType = ITEM_PUSH;
	s_pppoe_item[ITEM_PPPOE_PASSWORD].itemProperty.itemPropertyBtn.string = "********";
	s_pppoe_item[ITEM_PPPOE_PASSWORD].itemCallback.btnCallback.BtnPress= app_pppoe_password_btn_callback;
    if(s_pppoe_data.mode == 0)
        s_pppoe_item[ITEM_PPPOE_PASSWORD].itemStatus = ITEM_DISABLE;
    else
        s_pppoe_item[ITEM_PPPOE_PASSWORD].itemStatus = ITEM_NORMAL;
}

static void app_pppoe_setting_dns1_item_init(void)
{
    s_pppoe_item[ITEM_PPPOE_DNS1].itemTitle = STR_ID_DNS1;
    s_pppoe_item[ITEM_PPPOE_DNS1].itemType = ITEM_EDIT;
    s_pppoe_item[ITEM_PPPOE_DNS1].itemProperty.itemPropertyEdit.format = "edit_ip";
    s_pppoe_item[ITEM_PPPOE_DNS1].itemProperty.itemPropertyEdit.maxlen = "15";
    s_pppoe_item[ITEM_PPPOE_DNS1].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_pppoe_item[ITEM_PPPOE_DNS1].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    s_pppoe_item[ITEM_PPPOE_DNS1].itemProperty.itemPropertyEdit.string = s_pppoe_data.dns1;
    s_pppoe_item[ITEM_PPPOE_DNS1].itemCallback.editCallback.EditReachEnd= NULL;
    s_pppoe_item[ITEM_PPPOE_DNS1].itemCallback.editCallback.EditPress= NULL;
    if(s_pppoe_data.mode == 0)
        s_pppoe_item[ITEM_PPPOE_DNS1].itemStatus = ITEM_DISABLE;
    else
        s_pppoe_item[ITEM_PPPOE_DNS1].itemStatus = ITEM_NORMAL;
}

static void app_pppoe_setting_dns2_item_init(void)
{
    s_pppoe_item[ITEM_PPPOE_DNS2].itemTitle = STR_ID_DNS2;
    s_pppoe_item[ITEM_PPPOE_DNS2].itemType = ITEM_EDIT;
    s_pppoe_item[ITEM_PPPOE_DNS2].itemProperty.itemPropertyEdit.format = "edit_ip";
    s_pppoe_item[ITEM_PPPOE_DNS2].itemProperty.itemPropertyEdit.maxlen = "15";
    s_pppoe_item[ITEM_PPPOE_DNS2].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_pppoe_item[ITEM_PPPOE_DNS2].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    s_pppoe_item[ITEM_PPPOE_DNS2].itemProperty.itemPropertyEdit.string = s_pppoe_data.dns2;
    s_pppoe_item[ITEM_PPPOE_DNS2].itemCallback.editCallback.EditReachEnd= NULL;
    s_pppoe_item[ITEM_PPPOE_DNS2].itemCallback.editCallback.EditPress= NULL;
    if(s_pppoe_data.mode == 0)
        s_pppoe_item[ITEM_PPPOE_DNS2].itemStatus = ITEM_DISABLE;
    else
        s_pppoe_item[ITEM_PPPOE_DNS2].itemStatus = ITEM_NORMAL;
}

static void app_pppoe_setting_save_item_init(void)
{
	s_pppoe_item[ITEM_PPPOE_SAVE].itemTitle = STR_ID_SAVE;
	s_pppoe_item[ITEM_PPPOE_SAVE].itemType = ITEM_PUSH;
	s_pppoe_item[ITEM_PPPOE_SAVE].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	s_pppoe_item[ITEM_PPPOE_SAVE].itemCallback.btnCallback.BtnPress = app_pppoe_save_press_callback;
}

void app_create_pppoe_menu(ubyte64 *pppoe_dev)
{
    GxMsgProperty_PppoeCfg pppoe_cfg;

    if((pppoe_dev == NULL) || (*pppoe_dev == NULL) || (strlen(*pppoe_dev) == 0))
    {
        return;
    }
    memset(s_pppoe_dev, 0, sizeof(ubyte64));
    strcpy(s_pppoe_dev, *pppoe_dev);
    memset(&pppoe_cfg, 0, sizeof(GxMsgProperty_PppoeCfg));
    strcpy(pppoe_cfg.dev_name, s_pppoe_dev);
    app_send_msg_exec(GXMSG_PPPOE_GET_PARAM, &pppoe_cfg);

    s_pppoe_data.mode = pppoe_cfg.mode;
    if(pppoe_cfg.mode > 1)
        pppoe_cfg.mode = 1;

    if(strlen(pppoe_cfg.username) > 0)
        strncpy(s_pppoe_data.username, pppoe_cfg.username, sizeof(s_pppoe_data.username) - 1);
    else
        strcpy(s_pppoe_data.username, "TEST");

    if(strlen(pppoe_cfg.passwd) > 0)
        strncpy(s_pppoe_data.password, pppoe_cfg.passwd, sizeof(s_pppoe_data.password) - 1);
    else
        strcpy(s_pppoe_data.password, "123456");

    if(strlen(pppoe_cfg.dns1) > 0)
    {
        app_network_ip_completion(pppoe_cfg.dns1);
        strncpy(s_pppoe_data.dns1, pppoe_cfg.dns1, sizeof(s_pppoe_data.dns1) - 1);
    }
    else
        strcpy(s_pppoe_data.dns1, "008.008.008.008");

    if(strlen(pppoe_cfg.dns2) > 0)
    {
        app_network_ip_completion(pppoe_cfg.dns2);
        strncpy(s_pppoe_data.dns2, pppoe_cfg.dns2, sizeof(s_pppoe_data.dns2) - 1);
    }
    else
        strcpy(s_pppoe_data.dns2, "208.067.222.222");

    memcpy(&s_pppoe_data_trace, &s_pppoe_data, sizeof(PppoeData));

	memset(&s_pppoe_opt, 0 ,sizeof(s_pppoe_opt));
	s_pppoe_opt.menuTitle = STR_ID_PPPOE;
	s_pppoe_opt.itemNum = ITEM_PPPOE_TOTAL;
	s_pppoe_opt.timeDisplay = TIP_HIDE;
	s_pppoe_opt.item = s_pppoe_item;

    app_pppoe_setting_mode_item_init();
	app_pppoe_setting_user_item_init();
	app_pppoe_setting_password_item_init();
    app_pppoe_setting_dns1_item_init();
    app_pppoe_setting_dns2_item_init();
	app_pppoe_setting_save_item_init();
	s_pppoe_opt.exit = app_pppoe_setting_exit_callback;

	app_system_set_create(&s_pppoe_opt);
}
#endif
