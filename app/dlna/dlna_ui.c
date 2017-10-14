#include "app.h"
#include "app_module.h"
#if (DLNA_SUPPORT > 0)
#include "app_dlna.h"
#include "gxcore.h"

#define IMG_DLAN_TITLE		"img_dlna_title"
#define IMG_DLAN_TITLE_BK	"img_dlna_title_bk"
#define TXT_DLNA_NAME		"txt_dlna_name"

static bool s_dlna_flag = false;
static DlnaCB s_dlna_start = NULL;
static DlnaCB s_dlna_stop = NULL;
static char s_dlna_action[100] = {0};
static char s_dlna_url[MAX_DLNA_URL_LENGTH] = {0};
static char s_dlna_metadata[100] = {0};
static int s_dlna_vol = 0;
static bool s_dlna_trigger = false;
//extern int spectrum_redraw(void);
//extern int spectrum_stop(void);
extern bool ipc_from_dlna(char *action, char *url, char *urimetadata, int volume);
extern status_t dlna_player_stop(void);
/********************************************************************
 *  UI FOR DLNA
 ********************************************************************/
void app_dlna_start(void)
{
    if(s_dlna_flag == false)
    {
        system("gxdlna &");
        s_dlna_flag = true;
        printf("-----start dlna-----[line=%d]\n",__LINE__);
    }
}

void app_dlna_stop(void)
{
    system("killall gxdlna");
    s_dlna_flag = false;
    printf("-----stop dlna-----[line=%d]\n",__LINE__);
}

void app_dlan_restart(void)
{
    system("killall gxdlna");
    system("gxdlna &");
    s_dlna_flag = true;
    printf("-----restart dlna-----[line=%d]\n",__LINE__);
}

static int app_dlna_pop_exit_cb(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        if(s_dlna_start != NULL)
        {
            s_dlna_start();
            s_dlna_start = NULL;
        }
        GUI_CreateDialog("wnd_dlna");
        ipc_from_dlna(s_dlna_action, s_dlna_url, s_dlna_metadata, s_dlna_vol);
    }
    s_dlna_trigger = false;
    return 0;
}

void app_dlna_ui_exec(DlnaCB dlna_start, DlnaCB dlna_stop)
{
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_YES_NO;
    pop.str = "Enter DLNA ?";
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = app_dlna_pop_exit_cb;
    s_dlna_start = dlna_start;
    s_dlna_stop = dlna_stop;
    popdlg_create(&pop);
    s_dlna_trigger = true;
}

static void app_dlna_trigger(void)
{
    const char *focus_wnd = NULL;
    //printf("XXXXXXXXXXXXXX[%s]%d dlna_action: %s XXXXXXXXXXXXXX\n", __FILE__, __LINE__, s_dlna_action);
    if((GUI_CheckDialog("wnd_dlna") == GXCORE_SUCCESS) || (strcmp(s_dlna_action, "stop") == 0))
    {
        ipc_from_dlna(s_dlna_action, s_dlna_url, s_dlna_metadata, s_dlna_vol);
    }
    else
    {
        if((s_dlna_trigger == false) && (strcmp(s_dlna_action, "play") == 0))
        {
            popdlg_destroy();
            if(GUI_CheckDialog("wnd_passwd") == GXCORE_SUCCESS)
            {
                GUI_EndDialog("wnd_passwd");
                GUI_SetInterface("flush", NULL);
            }

            if((focus_wnd = GUI_GetFocusWindow()) != NULL)
            {
                static GUI_Event event;
                memset(&event, 0, sizeof(GUI_Event));
                event.type = GUI_KEYDOWN;
                event.key.sym = VK_DLNA_TRIGGER;
                GUI_SendEvent(focus_wnd, &event);
            }
        }
    }
}

void app_dlna_msg_proc(GxMessage *msg)
{
    if(msg == NULL)
        return;

    switch(msg->msg_id)
    {
        case GXMSG_NETWORK_STATE_REPORT:
            {
                GxMsgProperty_NetDevState *net_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
                if((net_state != NULL) && (net_state->dev_state == IF_STATE_CONNECTED))
                {
                    app_dlan_restart();
                }
                break;
            }

        case GXMSG_DLNA_TRIGGER:
            {
                GxMsgProperty_DlnaMsg *dlna_msg = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_DlnaMsg);
                if(dlna_msg != NULL)
                {
                    memset(s_dlna_action, 0, 100);
                    strncpy(s_dlna_action, dlna_msg->action, 99);

                    memset(s_dlna_url, 0, MAX_DLNA_URL_LENGTH);
                    strncpy(s_dlna_url, dlna_msg->url, MAX_DLNA_URL_LENGTH - 1);

                    memset(s_dlna_metadata, 0, 100);
                    strncpy(s_dlna_metadata, dlna_msg->metadata, 99);

                    s_dlna_vol = dlna_msg->volume;
                    app_dlna_trigger();
                }
                break;
            }

        default:
            break;
    }
}

void app_create_dlna_menu(void)
{
	GUI_CreateDialog("wnd_dlna");
}

void _show_dlna_title_menu(int status)
{
	if (status)
	{
		GUI_SetProperty(IMG_DLAN_TITLE,     "state", "show");
		GUI_SetProperty(IMG_DLAN_TITLE_BK,     "state", "show");
		GUI_SetProperty(TXT_DLNA_NAME,     "state", "show");
	}
	else
	{
		GUI_SetProperty(IMG_DLAN_TITLE,     "state", "hide");
		GUI_SetProperty(IMG_DLAN_TITLE_BK,     "state", "hide");
		GUI_SetProperty(TXT_DLNA_NAME,     "state", "hide");
	}
	return;
}

void _close_dlna_mp3_menu(void)
{
	//spectrum_stop();
	//spectrum_destroy();
	return;
}

SIGNAL_HANDLER int dlna_init(GuiWidget *widget, void *usrdata)
{
	_show_dlna_title_menu(1);
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int dlna_destroy(GuiWidget *widget, void *usrdata)
{
    dlna_player_stop();
	return EVENT_TRANSFER_STOP;
}

extern status_t dlna_player_play(const char* url, int start_time_ms);
SIGNAL_HANDLER int dlna_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch(event->type) {

        case GUI_KEYDOWN:
            {
                switch(event->key.sym)
                {
                    case VK_BOOK_TRIGGER:
                        GUI_EndDialog("after wnd_full_screen");
                        break;

                    case STBK_EXIT:
                        GUI_EndDialog("wnd_dlna");
                        if(s_dlna_stop != NULL)
                        {
                            s_dlna_stop();
                            s_dlna_stop = NULL;
                        }
                        break;

                    case STBK_VOLDOWN:
                    case STBK_VOLUP:
                    case STBK_LEFT:
                    case STBK_RIGHT:
                        GUI_CreateDialog("wnd_volume_value");
                        GUI_SendEvent("wnd_volume_value", event);
                        break;

                    default:
                        break;

                }
            }
    }

    return ret;
}
#else
SIGNAL_HANDLER int dlna_init(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int dlna_destroy(GuiWidget *widget, void *usrdata)
{
	//spectrum_stop();
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int dlna_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

#endif

SIGNAL_HANDLER  int app_dlna_canvas_init(const char* widgetname, void *usrdata)
{
	/*bean: canvas redraw lost static action*/
	//spectrum_redraw();
	return EVENT_TRANSFER_STOP;
}

