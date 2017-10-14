#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_config.h"

#define UART_SUPPORT 0
#if (UART_SUPPORT > 0)
enum TIME_AV_NAME
{
	ITEM_UART_CONTROL = 0,
	ITEM_SERIAL_BAUD_RATE,
	ITEM_UART_TOTAL
};
typedef enum
{
	UART_PROTOCOL_DISABLE = 0,
	UART_PROTOCOL_ENABLE ,
}UART_CONTROL;

typedef enum
{
	LOWER_LIMIT = 0,// if the baud is 115200, the ouput is 112500
	UPPER_LIMIT,	// if the baud is 115200, the ouput is 120550
}BAUD_RATE_ADJUST;

typedef struct
{
	UART_CONTROL     UartControlSwitch;
	BAUD_RATE_ADJUST BaudRateAdjust;// only support two level to adjust
}UARTPara;

static SystemSettingOpt sg_UartSettingOpt;
static SystemSettingItem sg_UartSettingItem[ITEM_UART_TOTAL];
static UARTPara sg_UartPara;


static void app_uart_setting_para_get(void)
{
	int init_value = 0;
	
	GxBus_ConfigGetInt(UART_MODE_KEY, &init_value, UART_PROTOCOL_SWITCH);
	sg_UartPara.UartControlSwitch = init_value;

	GxBus_ConfigGetInt(UART_BAUD_KEY, &init_value, UART_BAUD_RATE_LIMIT);
	sg_UartPara.BaudRateAdjust = init_value;
}

static void app_uart_setting_result_para_get(UARTPara *ret_para)
{
    ret_para->UartControlSwitch = sg_UartSettingItem[ITEM_UART_CONTROL].itemProperty.itemPropertyCmb.sel;
    ret_para->BaudRateAdjust = sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemProperty.itemPropertyCmb.sel;
}

static bool app_uart_setting_para_change(UARTPara *ret_para)
{
	bool ret = FALSE;

	if(sg_UartPara.UartControlSwitch != ret_para->UartControlSwitch)
	{
		ret = TRUE;
	}

	if(sg_UartPara.BaudRateAdjust != ret_para->BaudRateAdjust)
	{
		ret = TRUE;
	}
	
	return ret;
}

static int app_uart_setting_para_save(UARTPara *ret_para)
{
	int init_value = 0;

    init_value = ret_para->UartControlSwitch;
    GxBus_ConfigSetInt(UART_MODE_KEY,init_value);

    init_value = ret_para->BaudRateAdjust;
    GxBus_ConfigSetInt(UART_BAUD_KEY,init_value); 
	
	if(UART_PROTOCOL_DISABLE == ret_para->UartControlSwitch)
	{
        // TODO: 20160302
	}
	else
	{
        // TODO: 20160302
	}

	return 0;
}

static int app_uart_setting_exit_callback(ExitType exit_type) 
{
	UARTPara para_ret;
	int ret = 0;

	app_uart_setting_result_para_get(&para_ret);
	if(app_uart_setting_para_change(&para_ret) == TRUE)
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
	    pop.str = STR_ID_SAVE_INFO;
		if(popdlg_create(&pop) == POP_VAL_OK)
		{
			app_uart_setting_para_save(&para_ret);
			ret = 1;
		}

	}
	return ret;
}


static void app_uart_setting_uart_switch_item_init(void)
{
	sg_UartSettingItem[ITEM_UART_CONTROL].itemTitle = "Uart Protocol Enable";
	sg_UartSettingItem[ITEM_UART_CONTROL].itemType = ITEM_CHOICE;
	sg_UartSettingItem[ITEM_UART_CONTROL].itemProperty.itemPropertyCmb.content= "[Off,On]";
	sg_UartSettingItem[ITEM_UART_CONTROL].itemProperty.itemPropertyCmb.sel = sg_UartPara.UartControlSwitch;
	sg_UartSettingItem[ITEM_UART_CONTROL].itemCallback.cmbCallback.CmbChange= NULL;
	sg_UartSettingItem[ITEM_UART_CONTROL].itemStatus = ITEM_NORMAL;
}

static void app_uart_setting_baud_limit_item_init(void)
{
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemTitle = "Baud Rate Adjust";
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemType = ITEM_CHOICE;
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemProperty.itemPropertyCmb.content= "[Lower Limit,Upper Limit]";
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemProperty.itemPropertyCmb.sel = sg_UartPara.BaudRateAdjust;
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemCallback.cmbCallback.CmbChange= NULL;
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemStatus = ITEM_NORMAL;
}


void app_uart_menu_exec(void)
{

	memset(&sg_UartSettingOpt, 0 ,sizeof(sg_UartSettingOpt));
	
	app_uart_setting_para_get();

	sg_UartSettingOpt.menuTitle = STR_ID_UART;
	sg_UartSettingOpt.titleImage = "s_title_left_utility.bmp";
	sg_UartSettingOpt.itemNum = ITEM_UART_TOTAL;
	sg_UartSettingOpt.timeDisplay = TIP_HIDE;
	sg_UartSettingOpt.item = sg_UartSettingItem;
	
	app_uart_setting_uart_switch_item_init();
	app_uart_setting_baud_limit_item_init();

	sg_UartSettingOpt.exit = app_uart_setting_exit_callback;

	app_system_set_create(&sg_UartSettingOpt);
}
#endif


