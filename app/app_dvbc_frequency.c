#include "app.h"
#if (DEMOD_DVB_C > 0)
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "app_pop.h"

extern int app_win_check_fre_vaild(uint32_t fre);
static void app_search_setting_frequency_save(void)
{
    int32_t value = 0;
    int32_t fre = 0;
    char*   string;

    GUI_GetProperty("edit_search_main_freq", "string", &string);
    fre = atoi(string);

    GxBus_ConfigGetInt(DVBC_FREQ_KEY, &value, DVBC_FREQ_VALUE);
    if(fre != value)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_SAVE_INFO;
        if(popdlg_create(&pop) == POP_VAL_OK)
        {
            GxBus_ConfigSetInt(DVBC_FREQ_KEY, fre);
        }
    }
}

SIGNAL_HANDLER int app_search_setting_frequency_create(GuiWidget *widget, void *usrdata)
{

    int32_t value = 0;
    char App_Fre[7];//Æµµã

    GxBus_ConfigGetInt(DVBC_FREQ_KEY, &value, DVBC_FREQ_VALUE);
    memset(App_Fre,0,7);
    sprintf(App_Fre,"%03d", value);
    GUI_SetProperty("edit_search_main_freq", "string", App_Fre);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_setting_frequency_destroy(GuiWidget *widget, void *usrdata)
{


    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_setting_frequency_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
	int32_t fre = 0;
    char*   string;
	int32_t value = 0;
	char App_Fre[7];//Æµµã

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
				case STBK_0:
                case STBK_1:
                case STBK_2:
                case STBK_3:
                case STBK_4:
                case STBK_5:
                case STBK_6:
                case STBK_7:
                case STBK_8:
                case STBK_9:
                    GUI_GetProperty("edit_search_main_freq", "string", &string);
    				fre = atoi(string);
                    if (FALSE ==  app_win_check_fre_vaild(fre))
                    {
					    GxBus_ConfigGetInt(DVBC_FREQ_KEY, &value, DVBC_FREQ_VALUE);
					    memset(App_Fre,0,7);
					    sprintf(App_Fre,"%03d", value);
					    GUI_SetProperty("edit_search_main_freq", "string", App_Fre);
                    }
					break;
                case STBK_MENU:
                case STBK_EXIT:
					GUI_GetProperty("edit_search_main_freq", "string", &string);
    				fre = atoi(string);
                    if (TRUE ==  app_win_check_fre_vaild(fre))
                    	app_search_setting_frequency_save();
					
                    GUI_EndDialog("wnd_search_setting_frequency");

#if (DEMOD_DVB_COMBO > 0)    
                    GUI_SendEvent("wnd_system_setting", event);
#endif	
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}
#else //DEMOD_DVB_C
SIGNAL_HANDLER int app_search_setting_frequency_create(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_search_setting_frequency_destroy(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
SIGNAL_HANDLER int app_search_setting_frequency_keypress(GuiWidget *widget, void *usrdata)
{return EVENT_TRANSFER_STOP;}
#endif
