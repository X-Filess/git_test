#include "app_config.h"
#include "app.h"

#if LNB_SHORTCUT_SUPPORT

#include "app_module.h"

#define WND_LNB_SHORTCUT_POP                  "wnd_lnb_shortcut_pop"
#define XML_LNB_SHORTCUT_POP_PATH             "shortcut_monitor/wnd_lnb_shortcut_pop.xml"

#define TXT_LNB_SHORTCUT_POP_TITLE            "txt_lnb_shortcut_pop_title"
#define TXT_LNB_SHORTCUT_POP_CONTENT_LONG     "txt_lnb_shortcut_pop_content_long"

// export
int app_lnb_shortcut_pop_exec(void)
{
    static char init_flag = 0;
    //
    if (0 == init_flag) {
        init_flag = 1;
        GUI_LinkDialog(WND_LNB_SHORTCUT_POP, XML_LNB_SHORTCUT_POP_PATH);
    }
    if (GUI_CheckDialog(WND_LNB_SHORTCUT_POP) != GXCORE_SUCCESS) {
        GUI_CreateDialog(WND_LNB_SHORTCUT_POP);
    }
    else
        GUI_EndDialog(WND_LNB_SHORTCUT_POP);
    return 0;
}

// shortcut ui notice
static void shortcut_ui_notice(void)
{
    GxMessage *msg = NULL;

    msg = GxBus_MessageNew(GXMSG_LNB_SHORTCUT_EVENT);
    if (NULL == msg) {
        printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return;
    }
    GxBus_MessageSend(msg);
}

static void _close_lnbshort(void)
{
	GpioData _gpio;
	_gpio.port = V_GPIO_LNB_A;
	ioctl(bsp_gpio_handle, GPIO_OUTPUT, &_gpio);
	ioctl(bsp_gpio_handle, GPIO_LOW, &_gpio);
}

static void _open_lnb(void)
{
	GpioData _gpio;
	_gpio.port = V_GPIO_LNB_A;
	ioctl(bsp_gpio_handle, GPIO_OUTPUT, &_gpio);
	ioctl(bsp_gpio_handle, GPIO_HIGH, &_gpio);
}

static unsigned int thiz_shortcut_flag = 0;
//static char thiz_count = 0;

static void _shortcut_console(void *arg)
{
    GpioData _gpio = {0};
    _gpio.port = V_GPIO_LNB_SHORT;
    thiz_shortcut_flag = 0;
    GxCore_ThreadDetach();

    while (1) {
        if (ioctl(bsp_gpio_handle, GPIO_READ, &_gpio) < 0) {
            printf("\nerror, %s, %d\n", __FUNCTION__, __LINE__);
            GxCore_ThreadDelay(100);
            continue;
        }
        if (0 == _gpio.value) {
            if (thiz_shortcut_flag == 0) {
                _close_lnbshort();
                // UI NOTICE
                printf(">>>>> \033[31m Detect LNB short circuit happen!\033[0m <<<<<\n");
                shortcut_ui_notice();
                thiz_shortcut_flag = 1;
            }
        }
        else if (1 == _gpio.value) {
            // normal mode
            //open
            if (thiz_shortcut_flag == 1) {
                _open_lnb();
                printf(">>>>> \033[31m Detect LNB short circuit disappear!\033[0m <<<<<\n");
                shortcut_ui_notice();
                thiz_shortcut_flag = 0;
            }
        }
        GxCore_ThreadDelay(100);
    }
}

void lnb_shortcut_control(void)
{
    int handle = 0;
    GxCore_ThreadCreate("shortcut console", &handle, _shortcut_console, NULL, 16 * 1024, 10);
}

// for short cut control
SIGNAL_HANDLER int On_wnd_lnb_shortcut_pop_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_lnb_shortcut_pop_create(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(TXT_LNB_SHORTCUT_POP_CONTENT_LONG, "string", "Short occured to LNB connection Make LNB connection correct then Restart the Machine");
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_lnb_shortcut_pop_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch (event->type) {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
                ret = EVENT_TRANSFER_STOP;
                break;
        default:
            break;
    }
    return ret;
}
#else

SIGNAL_HANDLER int On_wnd_lnb_shortcut_pop_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_lnb_shortcut_pop_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_lnb_shortcut_pop_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
#endif
