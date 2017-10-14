#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "channel_edit.h"

#define MAX_PASSWD_LEN 4

enum LOCK_ITEM_NAME
{
    ITEM_MENU_LOCK = 0,
    ITEM_CHANNEL_LOCK,
    ITEM_PASSWD_BUTTON,
    ITEM_OLD_PASSWD,
    ITEM_NEW_PASSWD,
    ITEM_CONFIRM_PASSWD,
    ITEM_LOCK_TOTAL
};

typedef enum
{
    MENU_LOCK_OFF,
    MENU_LOCK_ON
}MenuLock;

typedef enum
{
    CHANNEL_LOCK_OFF,
    CHANNEL_LOCK_ON
}ChannelLock;

typedef struct
{
    MenuLock menu_lock;
    ChannelLock channel_lock;
    char passwd[MAX_PASSWD_LEN+1];
}SysLockPara;

static SystemSettingOpt s_lock_setting_opt;
static SystemSettingItem s_lock_item[ITEM_LOCK_TOTAL];
static SysLockPara s_sys_lock_para;
static char *s_old_pwd = NULL;
static char *s_new_pwd = NULL;
static char *s_confirm_pwd = NULL;
static  PopList pop_list;

static void app_lock_system_para_get(void)
{
    int init_value = 0;

    GxBus_ConfigGetInt(MENU_LOCK_KEY, &init_value, MENU_LOCK);
    s_sys_lock_para.menu_lock= init_value;

    GxBus_ConfigGetInt(CHANNEL_LOCK_KEY, &init_value, CHANNEL_LOCK);
    s_sys_lock_para.channel_lock= init_value;

    memset(s_sys_lock_para.passwd, '\0', sizeof(s_sys_lock_para.passwd));
}

static void app_lock_result_para_get(SysLockPara *ret_para)
{
    ret_para->menu_lock= s_lock_item[ITEM_MENU_LOCK].itemProperty.itemPropertyCmb.sel;
    ret_para->channel_lock= s_lock_item[ITEM_CHANNEL_LOCK].itemProperty.itemPropertyCmb.sel;
}

static bool app_lock_setting_para_change(SysLockPara *ret_para)
{
    bool ret = FALSE;

    if(s_sys_lock_para.menu_lock!= ret_para->menu_lock)
    {
        ret = TRUE;
    }

    if(s_sys_lock_para.channel_lock!= ret_para->channel_lock)
    {
        ret = TRUE;
    }

    return ret;
}

static int app_lock_setting_para_save(SysLockPara *ret_para)
{
    int init_value = 0;

    init_value = ret_para->menu_lock;
    GxBus_ConfigSetInt(MENU_LOCK_KEY,init_value);

    init_value = ret_para->channel_lock;
    GxBus_ConfigSetInt(CHANNEL_LOCK_KEY,init_value);

    return 0;
}

static void app_lock_exit_passwd_edit(int cur_sel)
{
    app_system_set_item_state_update(ITEM_MENU_LOCK, ITEM_NORMAL);
    app_system_set_item_state_update(ITEM_CHANNEL_LOCK, ITEM_NORMAL);
    app_system_set_item_state_update(ITEM_PASSWD_BUTTON, ITEM_NORMAL);
    app_system_set_item_state_update(ITEM_OLD_PASSWD, ITEM_HIDE);
    app_system_set_item_state_update(ITEM_NEW_PASSWD, ITEM_HIDE);
    app_system_set_item_state_update(ITEM_CONFIRM_PASSWD, ITEM_HIDE);

    app_system_set_unfocus_item(cur_sel);
    app_system_set_focus_item(ITEM_PASSWD_BUTTON);
}

static int app_lock_setting_exit_callback(ExitType exit_type)
{
    SysLockPara para_ret;
    int ret = 0;

    if(exit_type == EXIT_ABANDON)//error:#40662
    {
    }
    else
    {
        app_lock_result_para_get(&para_ret);
        if(app_lock_setting_para_change(&para_ret) == TRUE)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.str = STR_ID_SAVE_INFO;
            if(popdlg_create(&pop) == POP_VAL_OK)
            {
                app_lock_setting_para_save(&para_ret);
                ret = 1;
            }
        }
    }
    return ret;
}


static int _channel_lock_change_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
    if(ret->result ==  PASSWD_ERROR)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_ERR_PASSWD;
        popdlg_create(&pop);
    }
    else if(ret->result ==  PASSWD_CANCEL)
    {
    }
    else
    {
        s_lock_item[ITEM_CHANNEL_LOCK].itemProperty.itemPropertyCmb.sel = CHANNEL_LOCK_OFF;
        app_system_set_item_property(ITEM_CHANNEL_LOCK, &s_lock_item[ITEM_CHANNEL_LOCK].itemProperty);
        ;
    }

    return 0;
}

static int _menu_lock_change_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
    if(ret->result ==  PASSWD_ERROR)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_ERR_PASSWD;
        popdlg_create(&pop);
    }
    else if(ret->result ==  PASSWD_CANCEL)
    {
    }
    else
    {
        s_lock_item[ITEM_MENU_LOCK].itemProperty.itemPropertyCmb.sel = MENU_LOCK_OFF;
        app_system_set_item_property(ITEM_MENU_LOCK, &s_lock_item[ITEM_MENU_LOCK].itemProperty); 
    }

    return 0;
}

static int app_menu_lock_change_cb(int sel)
{
    int ret =EVENT_TRANSFER_KEEPON;
    if( (sel == MENU_LOCK_OFF) && (s_sys_lock_para.menu_lock == MENU_LOCK_ON) )
    {
        s_lock_item[ITEM_MENU_LOCK].itemProperty.itemPropertyCmb.sel = s_sys_lock_para.menu_lock;
        app_system_set_item_property(ITEM_MENU_LOCK, &s_lock_item[ITEM_MENU_LOCK].itemProperty); 
        PasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.passwd_key_msg = KEY_MSG_STOP;
        passwd_dlg.passwd_exit_cb = _menu_lock_change_passwd_dlg_cb;
        passwd_dlg_create(&passwd_dlg);
        ret = EVENT_TRANSFER_STOP;
    }
    return ret;

}

static int app_channel_lock_change_cb(int sel)
{
    int ret = EVENT_TRANSFER_KEEPON;
    if((sel == CHANNEL_LOCK_OFF) && (s_sys_lock_para.channel_lock == CHANNEL_LOCK_ON))
    {
        s_lock_item[ITEM_CHANNEL_LOCK].itemProperty.itemPropertyCmb.sel = s_sys_lock_para.channel_lock;
        app_system_set_item_property(ITEM_CHANNEL_LOCK, &s_lock_item[ITEM_CHANNEL_LOCK].itemProperty);

        PasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.passwd_key_msg = KEY_MSG_STOP;
        passwd_dlg.passwd_exit_cb = _channel_lock_change_passwd_dlg_cb;
        passwd_dlg_create(&passwd_dlg);
        ret = EVENT_TRANSFER_STOP;
    }
    return ret;
}

static int app_lock_passwd_button_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;
    int init_value = 0;

    switch(key)
    {
        case STBK_OK:

            app_system_set_item_state_update(ITEM_MENU_LOCK, ITEM_DISABLE);
            app_system_set_item_state_update(ITEM_CHANNEL_LOCK, ITEM_DISABLE);
            app_system_set_item_state_update(ITEM_PASSWD_BUTTON, ITEM_DISABLE);

            app_system_set_item_property(ITEM_OLD_PASSWD, &s_lock_item[ITEM_OLD_PASSWD].itemProperty);
            app_system_set_item_state_update(ITEM_OLD_PASSWD, ITEM_NORMAL);

            app_system_set_item_property(ITEM_NEW_PASSWD, &s_lock_item[ITEM_NEW_PASSWD].itemProperty);
            app_system_set_item_state_update(ITEM_NEW_PASSWD, ITEM_NORMAL);

            app_system_set_item_property(ITEM_CONFIRM_PASSWD, &s_lock_item[ITEM_CONFIRM_PASSWD].itemProperty);
            app_system_set_item_state_update(ITEM_CONFIRM_PASSWD, ITEM_NORMAL);

            app_system_set_unfocus_item(ITEM_PASSWD_BUTTON);
            app_system_set_focus_item(ITEM_OLD_PASSWD);

            GxBus_ConfigGetInt(PASSWORD_KEY0, &init_value, PASSWORD_1);
            s_sys_lock_para.passwd[0] = init_value;
            GxBus_ConfigGetInt(PASSWORD_KEY1, &init_value, PASSWORD_2);
            s_sys_lock_para.passwd[1] = init_value;
            GxBus_ConfigGetInt(PASSWORD_KEY2, &init_value, PASSWORD_3);
            s_sys_lock_para.passwd[2] = init_value;
            GxBus_ConfigGetInt(PASSWORD_KEY3, &init_value, PASSWORD_4);
            s_sys_lock_para.passwd[3] = init_value;

            ret = EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }

    return ret;
}

static int app_lock_old_passwd_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;

    switch(key)
    {
        case STBK_EXIT:
            app_lock_exit_passwd_edit(ITEM_OLD_PASSWD);
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_UP:
        case STBK_DOWN:
            ret = EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }

    return ret;
}

static int app_lock_new_passwd_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;

    switch(key)
    {
        case STBK_EXIT:
            app_lock_exit_passwd_edit(ITEM_NEW_PASSWD);
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_UP:
        case STBK_DOWN:
            ret = EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }

    return ret;
}

static int app_lock_confirm_passwd_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;

    switch(key)
    {
        case STBK_EXIT:
            app_lock_exit_passwd_edit(ITEM_CONFIRM_PASSWD);
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_UP:
        case STBK_DOWN:
            ret = EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }

    return ret;
}

static int app_lock_old_passwd_reachend_callback(char *string)
{
    if(string != NULL)
    {
        s_old_pwd = string;

        if ((strcmp(s_old_pwd, s_sys_lock_para.passwd) == 0)
                || (strcmp(s_old_pwd, GRNERAL_PASSWD) == 0))
        {
            app_system_set_unfocus_item(ITEM_OLD_PASSWD);
            app_system_set_focus_item(ITEM_NEW_PASSWD);
        }
        else
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_ERR_PASSWD;
            popdlg_create(&pop);
            app_system_set_item_property(ITEM_OLD_PASSWD, &s_lock_item[ITEM_OLD_PASSWD].itemProperty);
        }
    }

    return EVENT_TRANSFER_KEEPON;
}

static int app_lock_new_passwd_reachend_callback(char *string)
{
    if(string != NULL)
    {
        s_new_pwd = string;

        app_system_set_unfocus_item(ITEM_NEW_PASSWD);
        app_system_set_focus_item(ITEM_CONFIRM_PASSWD);
    }

    return EVENT_TRANSFER_KEEPON;
}

static int app_lock_confirm_passwd_reachend_callback(char *string)
{
    PopDlg  pop;

    memset(&pop, 0, sizeof(PopDlg));

    if((string != NULL) && (s_new_pwd != NULL))
    {
        s_confirm_pwd = string;

        //check pwd
        if(strcmp(s_new_pwd, s_confirm_pwd) == 0)
        {
            if(strcmp(s_old_pwd,s_new_pwd) == 0)
            {
                pop.type = POP_TYPE_OK;
                pop.str = "The same password, please re-enter.";
                popdlg_create(&pop);

                app_system_set_item_property(ITEM_NEW_PASSWD, &s_lock_item[ITEM_NEW_PASSWD].itemProperty);
                app_system_set_item_property(ITEM_CONFIRM_PASSWD, &s_lock_item[ITEM_CONFIRM_PASSWD].itemProperty);
                app_system_set_unfocus_item(ITEM_CONFIRM_PASSWD);
                app_system_set_focus_item(ITEM_NEW_PASSWD);
                if(GUI_CheckDialog("wnd_pop_book") == GXCORE_SUCCESS)
                {
                    GUI_SetFocusWidget("wnd_pop_book");
                }

            }
            else
            {
                pop.type = POP_TYPE_NO_BTN;
                pop.str = STR_ID_SUCCESS;
                pop.timeout_sec = 1;
                popdlg_create(&pop);

                GxBus_ConfigSetInt(PASSWORD_KEY0, s_confirm_pwd[0]);
                GxBus_ConfigSetInt(PASSWORD_KEY1, s_confirm_pwd[1]);
                GxBus_ConfigSetInt(PASSWORD_KEY2, s_confirm_pwd[2]);
                GxBus_ConfigSetInt(PASSWORD_KEY3, s_confirm_pwd[3]);

                app_lock_exit_passwd_edit(ITEM_CONFIRM_PASSWD);

            }
        }
        else
        {
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_ERR_PASSWD;
            popdlg_create(&pop);

            app_system_set_item_property(ITEM_NEW_PASSWD, &s_lock_item[ITEM_NEW_PASSWD].itemProperty);
            app_system_set_item_property(ITEM_CONFIRM_PASSWD, &s_lock_item[ITEM_CONFIRM_PASSWD].itemProperty);
            //			if(0 == strcasecmp("wnd_system_setting", GUI_GetFocusWidget()))
            //			{
            app_system_set_unfocus_item(ITEM_CONFIRM_PASSWD);
            app_system_set_focus_item(ITEM_NEW_PASSWD);
            //			}
            if(GUI_CheckDialog("wnd_pop_book") == GXCORE_SUCCESS)
            {
                GUI_SetFocusWidget("wnd_pop_book");
            }
        }
    }

    return EVENT_TRANSFER_KEEPON;
}

static int app_menu_lock_press_cb(int key)
{
    static char* s_MenuData[]= {"Off","On"};
    int cmb_sel =-1;

    if(key == STBK_OK)
    {
        memset(&pop_list, 0, sizeof(PopList));
        pop_list.title = STR_ID_MENU_LOCK;
        pop_list.item_num = sizeof(s_MenuData)/sizeof(*s_MenuData);
        pop_list.item_content = s_MenuData;
        pop_list.sel = s_lock_item[ITEM_MENU_LOCK].itemProperty.itemPropertyCmb.sel;
        pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_2;
        pop_list.pos.x= 630;
        pop_list.pos.y= 136;

        cmb_sel = poplist_create(&pop_list);
        GUI_SetProperty("cmb_system_setting_opt1", "select", &cmb_sel);
    }
    return EVENT_TRANSFER_KEEPON;
}

static int app_channel_lock_press_cb(int key)
{
    static char* s_ChData[]= {"Off","On"};
    int cmb_sel =-1;

    if((key == STBK_OK) || (key == STBK_RIGHT))
    {
        memset(&pop_list, 0, sizeof(PopList));
        pop_list.title = STR_ID_CH_LOCK;
        pop_list.item_num = sizeof(s_ChData)/sizeof(*s_ChData);
        pop_list.item_content = s_ChData;
        pop_list.sel = s_lock_item[ITEM_CHANNEL_LOCK].itemProperty.itemPropertyCmb.sel;
        pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_2;
        pop_list.pos.x= 630;
        pop_list.pos.y= 167;

        cmb_sel = poplist_create(&pop_list);
        GUI_SetProperty("cmb_system_setting_opt2", "select", &cmb_sel);
    }

    if((key == STBK_LEFT) || (key == STBK_RIGHT))
        return EVENT_TRANSFER_STOP;

    return EVENT_TRANSFER_KEEPON;
}

static void app_lock_setting_menu_item_init(void)
{
    s_lock_item[ITEM_MENU_LOCK].itemTitle = STR_ID_MENU_LOCK;
    s_lock_item[ITEM_MENU_LOCK].itemType = ITEM_CHOICE;
    s_lock_item[ITEM_MENU_LOCK].itemProperty.itemPropertyCmb.content= "[Off,On]";
    s_lock_item[ITEM_MENU_LOCK].itemProperty.itemPropertyCmb.sel = s_sys_lock_para.menu_lock;
    s_lock_item[ITEM_MENU_LOCK].itemCallback.cmbCallback.CmbChange = app_menu_lock_change_cb;
    s_lock_item[ITEM_MENU_LOCK].itemCallback.cmbCallback.CmbPress = app_menu_lock_press_cb;
    s_lock_item[ITEM_MENU_LOCK].itemStatus = ITEM_NORMAL;
}

static void app_lock_setting_channel_item_init(void)
{
    s_lock_item[ITEM_CHANNEL_LOCK].itemTitle = STR_ID_CH_LOCK;
    s_lock_item[ITEM_CHANNEL_LOCK].itemType = ITEM_CHOICE;
    s_lock_item[ITEM_CHANNEL_LOCK].itemProperty.itemPropertyCmb.content= "[Off,On]";
    s_lock_item[ITEM_CHANNEL_LOCK].itemProperty.itemPropertyCmb.sel = s_sys_lock_para.channel_lock;
    s_lock_item[ITEM_CHANNEL_LOCK].itemCallback.cmbCallback.CmbChange = app_channel_lock_change_cb;
    s_lock_item[ITEM_CHANNEL_LOCK].itemCallback.cmbCallback.CmbPress = app_channel_lock_press_cb;
    /*only play program need input password*/
    //s_lock_item[ITEM_CHANNEL_LOCK].itemCallback.cmbCallback.CmbChange= NULL;
    s_lock_item[ITEM_CHANNEL_LOCK].itemStatus = ITEM_NORMAL;
}

static void app_lock_setting_passwd_button_item_init(void)
{
    s_lock_item[ITEM_PASSWD_BUTTON].itemTitle = STR_ID_CHANGE_PASSWD;
    s_lock_item[ITEM_PASSWD_BUTTON].itemType = ITEM_PUSH;
    s_lock_item[ITEM_PASSWD_BUTTON].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
    s_lock_item[ITEM_PASSWD_BUTTON].itemCallback.btnCallback.BtnPress= app_lock_passwd_button_press_callback;
    s_lock_item[ITEM_PASSWD_BUTTON].itemStatus = ITEM_NORMAL;
}

static void app_lock_setting_old_passwd_item_init(void)
{
    int pwd_len = MAX_PASSWD_LEN;
    static char edit_len[2] = {0};
    sprintf(edit_len , "%d", pwd_len);

    s_lock_item[ITEM_OLD_PASSWD].itemTitle = STR_ID_OLD_PASSWD;
    s_lock_item[ITEM_OLD_PASSWD].itemType = ITEM_EDIT;
    s_lock_item[ITEM_OLD_PASSWD].itemProperty.itemPropertyEdit.string= NULL;
    s_lock_item[ITEM_OLD_PASSWD].itemProperty.itemPropertyEdit.maxlen = edit_len;
    s_lock_item[ITEM_OLD_PASSWD].itemProperty.itemPropertyEdit.format = "edit_digit";
    s_lock_item[ITEM_OLD_PASSWD].itemProperty.itemPropertyEdit.intaglio = "*";
    s_lock_item[ITEM_OLD_PASSWD].itemProperty.itemPropertyEdit.default_intaglio = "-";
    s_lock_item[ITEM_OLD_PASSWD].itemCallback.editCallback.EditReachEnd= app_lock_old_passwd_reachend_callback;
    s_lock_item[ITEM_OLD_PASSWD].itemCallback.editCallback.EditPress= app_lock_old_passwd_press_callback;
    s_lock_item[ITEM_OLD_PASSWD].itemStatus = ITEM_HIDE;
}

static void app_lock_setting_new_passwd_item_init(void)
{
    int pwd_len = MAX_PASSWD_LEN;
    static char edit_len[2] = {0};
    sprintf(edit_len , "%d", pwd_len);

    s_lock_item[ITEM_NEW_PASSWD].itemTitle = STR_ID_NEW_PASSWD;
    s_lock_item[ITEM_NEW_PASSWD].itemType = ITEM_EDIT;
    s_lock_item[ITEM_NEW_PASSWD].itemProperty.itemPropertyEdit.string= NULL;
    s_lock_item[ITEM_NEW_PASSWD].itemProperty.itemPropertyEdit.maxlen = edit_len;
    s_lock_item[ITEM_NEW_PASSWD].itemProperty.itemPropertyEdit.format = "edit_digit";
    s_lock_item[ITEM_NEW_PASSWD].itemProperty.itemPropertyEdit.intaglio = "*";
    s_lock_item[ITEM_NEW_PASSWD].itemProperty.itemPropertyEdit.default_intaglio = "-";
    s_lock_item[ITEM_NEW_PASSWD].itemCallback.editCallback.EditReachEnd= app_lock_new_passwd_reachend_callback;
    s_lock_item[ITEM_NEW_PASSWD].itemCallback.editCallback.EditPress= app_lock_new_passwd_press_callback;
    s_lock_item[ITEM_NEW_PASSWD].itemStatus = ITEM_HIDE;
}

static void app_lock_setting_confirm_passwd_item_init(void)
{
    int pwd_len = MAX_PASSWD_LEN;
    static char edit_len[2] = {0};
    sprintf(edit_len , "%d", pwd_len);

    s_lock_item[ITEM_CONFIRM_PASSWD].itemTitle = STR_ID_CON_PASSWD;
    s_lock_item[ITEM_CONFIRM_PASSWD].itemType = ITEM_EDIT;
    s_lock_item[ITEM_CONFIRM_PASSWD].itemProperty.itemPropertyEdit.string= NULL;
    s_lock_item[ITEM_CONFIRM_PASSWD].itemProperty.itemPropertyEdit.maxlen = edit_len;
    s_lock_item[ITEM_CONFIRM_PASSWD].itemProperty.itemPropertyEdit.format = "edit_digit";
    s_lock_item[ITEM_CONFIRM_PASSWD].itemProperty.itemPropertyEdit.intaglio = "*";
    s_lock_item[ITEM_CONFIRM_PASSWD].itemProperty.itemPropertyEdit.default_intaglio = "-";
    s_lock_item[ITEM_CONFIRM_PASSWD].itemCallback.editCallback.EditReachEnd= app_lock_confirm_passwd_reachend_callback;
    s_lock_item[ITEM_CONFIRM_PASSWD].itemCallback.editCallback.EditPress= app_lock_confirm_passwd_press_callback;
    s_lock_item[ITEM_CONFIRM_PASSWD].itemStatus = ITEM_HIDE;
}

void app_lock_setting_menu_exec(void)
{
    memset(&s_lock_setting_opt, 0 ,sizeof(s_lock_setting_opt));
    app_lock_system_para_get();

    s_lock_setting_opt.menuTitle = STR_ID_LOCK_CTRL;
    s_lock_setting_opt.itemNum = ITEM_LOCK_TOTAL;
    s_lock_setting_opt.timeDisplay = TIP_HIDE;
    s_lock_setting_opt.item = s_lock_item;

    app_lock_setting_menu_item_init();
    app_lock_setting_channel_item_init();
    app_lock_setting_passwd_button_item_init();
    app_lock_setting_old_passwd_item_init();
    app_lock_setting_new_passwd_item_init();
    app_lock_setting_confirm_passwd_item_init();

    s_lock_setting_opt.exit = app_lock_setting_exit_callback;

    app_system_set_create(&s_lock_setting_opt);
}



