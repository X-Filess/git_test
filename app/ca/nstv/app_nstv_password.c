#include "app_pop.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"

#define WND_NSTV_PASSWD "wnd_nstv_passwd"
#define BTN_PASSWD_INPUT_1 "btn_nstv_passwd_input_1"
#define BTN_PASSWD_INPUT_2 "btn_nstv_passwd_input_2"
#define BTN_PASSWD_INPUT_3 "btn_nstv_passwd_input_3"
#define BTN_PASSWD_INPUT_4 "btn_nstv_passwd_input_4"
#define BTN_PASSWD_INPUT_5 "btn_nstv_passwd_input_5"
#define BTN_PASSWD_INPUT_6 "btn_nstv_passwd_input_6"
#define STR_INPUTTED_FLAG "*"

#define PASSWD_LENGTH 6

static char *s_btn_input[PASSWD_LENGTH] = {
    BTN_PASSWD_INPUT_1,
    BTN_PASSWD_INPUT_2,
    BTN_PASSWD_INPUT_3,
    BTN_PASSWD_INPUT_4,
    BTN_PASSWD_INPUT_5,
    BTN_PASSWD_INPUT_6
};

static struct {
    char passwd_buf[PASSWD_LENGTH];
    int cur_cursor;
} s_passwd_data = {{0}, 0};

static PasswdDlg s_passwd_dlg;

static bool _passwd_check(void)
{
    #if 0
    char sys_passwd[PASSWD_LENGTH] = {0};
    int init_value = 0;
    int i = 0;
    bool ret = true;

    GxBus_ConfigGetInt(PASSWORD_KEY0, &init_value, PASSWORD_1);
    sys_passwd[0] = init_value;
    GxBus_ConfigGetInt(PASSWORD_KEY1, &init_value, PASSWORD_2);
    sys_passwd[1] = init_value;
    GxBus_ConfigGetInt(PASSWORD_KEY2, &init_value, PASSWORD_3);
    sys_passwd[2] = init_value;
    GxBus_ConfigGetInt(PASSWORD_KEY3, &init_value, PASSWORD_4);
    sys_passwd[3] = init_value;

    for (i = 0; i < PASSWD_LENGTH; i++) {
        if (sys_passwd[i] != s_passwd_data.passwd_buf[i]) {
            ret = false;
            break;
        }
    }

    if (ret == false) {
        char *general = GRNERAL_PASSWD;

        ret = true;
        for (i = 0; i < PASSWD_LENGTH; i++) {
            if (general[i] != s_passwd_data.passwd_buf[i]) {
                ret = false;
                break;
            }
        }
    }

    return ret;
    #else
    return 0;
    #endif
}

static status_t _set_passwd_cursor_back(void)
{
    status_t ret = GXCORE_SUCCESS;

    if (s_passwd_data.cur_cursor > 0) {
        s_passwd_data.cur_cursor--;
        GUI_SetFocusWidget(s_btn_input[s_passwd_data.cur_cursor]);
    }

    return ret;
}

static status_t _set_passwd_cursor_next(void)
{
    status_t ret = GXCORE_SUCCESS;

    s_passwd_data.cur_cursor++;

    if (s_passwd_data.cur_cursor <= PASSWD_LENGTH - 1) {
        GUI_SetFocusWidget(s_btn_input[s_passwd_data.cur_cursor]);
    }
    else {
        ret = GXCORE_ERROR;
    }
    return ret;
}

static status_t _input_cursor_passwd(uint32_t key_val)
{
    status_t ret = GXCORE_SUCCESS;

    switch (key_val) {
        case STBK_0:
            s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '0';
            break;

        case STBK_1:
            s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '1';
            break;

        case STBK_2:
            s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '2';
            break;

        case STBK_3:
            s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '3';
            break;

        case STBK_4:
            s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '4';
            break;

        case STBK_5:
            s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '5';
            break;

        case STBK_6:
            s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '6';
            break;

        case STBK_7:
            s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '7';
            break;

        case STBK_8:
            s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '8';
            break;

        case STBK_9:
            s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '9';
            break;

        default:
            ret = GXCORE_ERROR;
            break;
    }

    if (ret == GXCORE_SUCCESS) {
        GUI_SetProperty(s_btn_input[s_passwd_data.cur_cursor], "string", STR_INPUTTED_FLAG);
    }

    return ret;
}

/*
*此变量用于保存密码校验结果
*/
SIGNAL_HANDLER int app_nstv_passwd_dlg_create(GuiWidget *widget, void *usrdata)
{
    memset(s_passwd_data.passwd_buf, 0, sizeof(s_passwd_data.passwd_buf));
    s_passwd_data.cur_cursor = 0;
    GUI_SetFocusWidget(s_btn_input[s_passwd_data.cur_cursor]);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_nstv_passwd_dlg_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_nstv_passwd_dlg_keypress(GuiWidget *widget, void *usrdata)
{
    extern int app_channel_info_get_prog_lock_flag(void);
    //int index = 0;
    GUI_Event *event = NULL;
    PopPwdRet passwd_ret = {PASSWD_CANCEL, NULL};
    int ret = EVENT_TRANSFER_STOP;
    const char *parent = NULL;

    event = (GUI_Event *)usrdata;
    if (GUI_KEYDOWN ==  event->type) {
        passwd_ret.event = event;

        switch (event->key.sym) {
            case STBK_EXIT:
            case STBK_MENU:
                GUI_EndDialog(WND_NSTV_PASSWD);
                if (s_passwd_dlg.passwd_exit_cb != NULL) {
                    passwd_ret.result = PASSWD_CANCEL;
                    s_passwd_dlg.passwd_exit_cb(&passwd_ret, s_passwd_dlg.usr_data);
                }
                break;

            case STBK_LEFT:
                if (s_passwd_data.cur_cursor > 0) {
                    GUI_SetProperty(s_btn_input[s_passwd_data.cur_cursor - 1], "string", " ");
                }
                _set_passwd_cursor_back();
                break;

            case STBK_RIGHT:
                break;

            case STBK_UP:
            case STBK_DOWN:
            case STBK_PAGE_UP:
            case STBK_PAGE_DOWN:
                if (s_passwd_dlg.passwd_key_msg == KEY_MSG_KEEPON) {
                    GUI_EndDialog(WND_NSTV_PASSWD);

                    parent = GUI_GetFocusWidget();
                    if (parent != NULL) {
                        GUI_SendEvent(parent, event);
                    }
                }
                break;
            default:
                if (_input_cursor_passwd(event->key.sym) == GXCORE_SUCCESS) {
                    if (_set_passwd_cursor_next() == GXCORE_ERROR) { //end
                        if (s_passwd_dlg.passwd_check_cb != NULL) {
                            if (s_passwd_dlg.passwd_check_cb(s_passwd_data.passwd_buf, PASSWD_LENGTH) == true) {
                                passwd_ret.result = PASSWD_OK;
                            }
                            else {
                                passwd_ret.result = PASSWD_ERROR;
                            }
                        }
                        else {
                            if (_passwd_check() == true) {
                                passwd_ret.result = PASSWD_OK;
                            }
                            else {
                                passwd_ret.result = PASSWD_ERROR;
                            }
                        }

                        GUI_EndDialog(WND_NSTV_PASSWD);
                        if (s_passwd_dlg.passwd_exit_cb != NULL) {
                            s_passwd_dlg.passwd_exit_cb(&passwd_ret, s_passwd_dlg.usr_data);
                        }
                    }
                }
                else {
                    ret = EVENT_TRANSFER_KEEPON;
                }
                break;
        }
    }

    return ret;
}

status_t app_nstv_passwd_dlg(PasswdDlg *passwd_dlg)
{
    #if ( MINI_16_BITS_OSD_SUPPORT == 1)
#define WND_PASSWD_X 320
#define WND_PASSWD_Y 152
    #else
#define WND_PASSWD_X 400
#define WND_PASSWD_Y 190
    #endif

    int32_t pos_x = 0;
    int32_t pos_y = 0;

    if (passwd_dlg == NULL) {
        return GXCORE_ERROR;
    }

    memcpy(&s_passwd_dlg, passwd_dlg, sizeof(PasswdDlg));
    memset(&s_passwd_data, 0, sizeof(s_passwd_data));

    app_create_dialog(WND_NSTV_PASSWD);

    if ((passwd_dlg->pos.x == 0) && (passwd_dlg->pos.y == 0)) {
        pos_x = WND_PASSWD_X;
        pos_y = WND_PASSWD_Y;
    }
    else {
        pos_x = passwd_dlg->pos.x;
        pos_y = passwd_dlg->pos.y;
    }

    GUI_SetProperty(WND_NSTV_PASSWD, "move_window_x", &pos_x);
    GUI_SetProperty(WND_NSTV_PASSWD, "move_window_y", &pos_y);

    return GXCORE_SUCCESS;
}
