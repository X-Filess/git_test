#include "app.h"
#include "module/app_time.h"
#include "app_wnd_system_setting_opt.h"
#include "module/app_play_control.h"
#include "app_module.h"
#include "app_book.h"

#define IMG_BAR_HALF_FOCUS_L "s_bar_half_focus_l.bmp"
#define IMG_BAR_HALF_FOCUS_R "s_bar_half_focus_r.bmp"
#define IMG_BAR_HALF_UNFOCUS_CHOICE "s_bar_half_unfocus_arrow.bmp"
#define IMG_BAR_HALF_FOCUS_CHOICE "s_bar_half_focus_arrow.bmp"

#define IMG_BAR_HALF_UNFOCUS "s_bar_half_unfocus.bmp"

//#define IMG_BACK "img_system_setting_BG"
//#define IMG_BACK_TOP "img_system_setting_BG_top"
#define TXT_BACK_TOP "text_system_setting_BG_top"

#define TEXT_SYS_TIME "text_system_setting_time"
#define TEXT_SYS_DATE "text_system_setting_ymd"

//#if MV_WIN_SUPPORT
#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
static char *s_item_chioce_ok[] =
{
	"img_system_setting_ok1",
	"img_system_setting_ok2",
	"img_system_setting_ok3",
	"img_system_setting_ok4",
	"img_system_setting_ok5",
	"img_system_setting_ok6",
	"img_system_setting_ok7",
	"img_system_setting_ok8",
	"img_system_setting_ok9",
	"img_system_setting_ok10"
};
//#define IMG_BAR_HALF_FOCUS_OK_ "s_bar_half_focus.bmp"
#else
#define IMG_BAR_HALF_FOCUS_OK_ "s_bar_half_focus_ok.bmp"
#endif

typedef enum
{
	SETTING_ITEM_1,
	SETTING_ITEM_2,
	SETTING_ITEM_3,
	SETTING_ITEM_4,
	SETTING_ITEM_5,
	SETTING_ITEM_6,
	SETTING_ITEM_7,
	SETTING_ITEM_8,
	SETTING_ITEM_9,
	SETTING_ITEM_10,
	SETTING_ITEM_MAX
}SettingOptSel;

static char *s_item_choice_back[] = 
{
	"img_system_setting_item_choice1",
	"img_system_setting_item_choice2",
	"img_system_setting_item_choice3",
	"img_system_setting_item_choice4",
	"img_system_setting_item_choice5",
	"img_system_setting_item_choice6",
	"img_system_setting_item_choice7",
	"img_system_setting_item_choice8",
	"img_system_setting_item_choice9",
	"img_system_setting_item_choice10",
};

static char *s_item_title[] = 
{
	"text_system_setting_item1",
	"text_system_setting_item2",
	"text_system_setting_item3",
	"text_system_setting_item4",
	"text_system_setting_item5",
	"text_system_setting_item6",
	"text_system_setting_item7",
	"text_system_setting_item8",
	"text_system_setting_item9",
	"text_system_setting_item10",
};

static char *s_item_content_cmb[] = 
{
	"cmb_system_setting_opt1",
	"cmb_system_setting_opt2",
	"cmb_system_setting_opt3",
	"cmb_system_setting_opt4",
	"cmb_system_setting_opt5",
	"cmb_system_setting_opt6",
	"cmb_system_setting_opt7",
	"cmb_system_setting_opt8",
	"cmb_system_setting_opt9",
	"cmb_system_setting_opt10",
};

static char *s_item_content_edit[] = 
{
	"edit_system_setting_opt1",
	"edit_system_setting_opt2",
	"edit_system_setting_opt3",
	"edit_system_setting_opt4",
	"edit_system_setting_opt5",
	"edit_system_setting_opt6",
	"edit_system_setting_opt7",
	"edit_system_setting_opt8",
	"edit_system_setting_opt9",
	"edit_system_setting_opt10"
};

static char *s_item_content_btn[] = 
{
	"btn_system_setting_opt1",
	"btn_system_setting_opt2",
	"btn_system_setting_opt3",
	"btn_system_setting_opt4",
	"btn_system_setting_opt5",
	"btn_system_setting_opt6",
	"btn_system_setting_opt7",
	"btn_system_setting_opt8",
	"btn_system_setting_opt9",
	"btn_system_setting_opt10",
};

static char *s_item_interval[] = 
{
	"img_system_setting_interval1",
	"img_system_setting_interval2",
	"img_system_setting_interval3",
	"img_system_setting_interval4",
	"img_system_setting_interval5",
	"img_system_setting_interval6",
	"img_system_setting_interval7",
	"img_system_setting_interval8",
	"img_system_setting_interval9",
	"img_system_setting_interval10",
};

static SystemSettingOpt *s_setting_opt = NULL;
static SettingOptSel s_sel = SETTING_ITEM_1;
static int s_item_total = 0;
static WndStatus s_system_setting_menu_state = WND_CANCLE;
static bool s_wnd_block = false;
static event_list* s_time_update_timer = NULL;

#ifdef LINUX_OS
static event_list* s_system_setting_gif_timer = NULL;
static void app_system_setting_load_gif(void);
static void app_system_setting_free_gif(void);
static void app_system_setting_show_gif(void);
static void app_system_setting_hide_gif(void);
#endif
char * app_system_get_title()
{
	if (s_setting_opt != NULL)
		return s_setting_opt->menuTitle;
	else
		return NULL;
}
static void app_system_set_show_item(SettingOptSel sel)
{
#define NORMAL_FORE_COLOR "[text_color,text_color,text_color]"
#define DISABLE_FORE_COLOR "[text_disable_color,text_disable_color,text_disable_color]"
	char *item_widget = NULL;
	char *state = NULL;
	char *img = NULL;
	char *fore_color = NULL;

	GUI_SetProperty(s_item_choice_back[sel], "state", "show");
	GUI_SetProperty(s_item_title[sel], "state", "show");
	GUI_SetProperty(s_item_interval[sel], "state", "show");

	if(s_setting_opt->item[sel].itemType == ITEM_CHOICE)
	{
		item_widget = s_item_content_cmb[sel];
	}
	else if(s_setting_opt->item[sel].itemType == ITEM_EDIT)
	{
		item_widget = s_item_content_edit[sel];
	}
	else if(s_setting_opt->item[sel].itemType == ITEM_PUSH)
	{
		item_widget = s_item_content_btn[sel];
	}
	GUI_SetProperty(item_widget, "state",  "show");

	if(s_setting_opt->item[sel].itemStatus == ITEM_DISABLE)
	{
		state = "disable";
		fore_color = DISABLE_FORE_COLOR;
	}
	else
	{
		state = "enable";
		fore_color = NORMAL_FORE_COLOR;
	}
    img = IMG_BAR_HALF_UNFOCUS;

	GUI_SetProperty(item_widget, "state", state);
	GUI_SetProperty(s_item_choice_back[sel], "img", img);
	GUI_SetProperty(s_item_title[sel], "forecolor", fore_color);
	GUI_SetProperty(s_item_title[sel], "string", s_setting_opt->item[sel].itemTitle);
}

static void app_system_set_hide_item(SettingOptSel sel)
{
	GUI_SetProperty(s_item_title[sel], "state", "hide");
	GUI_SetProperty(s_item_content_cmb[sel], "state", "hide");
	GUI_SetProperty(s_item_content_edit[sel], "state", "hide");
	GUI_SetProperty(s_item_content_btn[sel], "state", "hide");
	GUI_SetProperty(s_item_choice_back[sel], "state", "hide");
	GUI_SetProperty(s_item_interval[sel], "state", "hide");
}

static SettingOptSel app_system_set_next_sel(SettingOptSel cur_sel)
{
	SettingOptSel sel = cur_sel;

	while(1)
	{
		(sel + 1 == s_item_total) ? sel = SETTING_ITEM_1 : sel++;

		if((s_setting_opt->item[sel].itemStatus == ITEM_NORMAL)
			|| (sel == cur_sel))
		{
			return sel;
		}
	}
}

static SettingOptSel app_system_set_last_sel(SettingOptSel cur_sel)
{
	SettingOptSel sel = cur_sel;
	
	while(1)
	{
		(sel  == SETTING_ITEM_1) ? sel = s_item_total - 1 : sel--;

		if((s_setting_opt->item[sel].itemStatus == ITEM_NORMAL)
			|| (sel == cur_sel))
		{
			return sel;
		}
	}
}

static void app_system_set_show_funckey(void)
{
	if(s_setting_opt->menuFuncKey.redKey.keyPressFun != NULL)
	{
		GUI_SetProperty("text_system_setting_red", "string", s_setting_opt->menuFuncKey.redKey.keyName);
		GUI_SetProperty("text_system_setting_red", "state", "show");
		GUI_SetProperty("img_system_setting_red", "state", "show");
	}

	if(s_setting_opt->menuFuncKey.greenKey.keyPressFun != NULL)
	{
		GUI_SetProperty("text_system_setting_green", "string", s_setting_opt->menuFuncKey.greenKey.keyName);
		GUI_SetProperty("text_system_setting_green", "state", "show");
		GUI_SetProperty("img_system_setting_green", "state", "show");
	}

	if(s_setting_opt->menuFuncKey.blueKey.keyPressFun != NULL)
	{
		GUI_SetProperty("text_system_setting_blue", "string", s_setting_opt->menuFuncKey.blueKey.keyName);
		GUI_SetProperty("text_system_setting_blue", "state", "show");
		GUI_SetProperty("img_system_setting_blue", "state", "show");
	}

	if(s_setting_opt->menuFuncKey.yellowKey.keyPressFun != NULL)
	{
		GUI_SetProperty("text_system_setting_yellow", "string", s_setting_opt->menuFuncKey.yellowKey.keyName);
		GUI_SetProperty("text_system_setting_yellow", "state", "show");
		GUI_SetProperty("img_system_setting_yellow", "state", "show");
	}
}

static int app_system_set_update_timer(void* usrdata)
{
	char sys_time_str[24] = {0};
	char time_str_temp[24] = {0};
	
	g_AppTime.time_get(&g_AppTime, sys_time_str, sizeof(sys_time_str)); 

	// time
	if((strlen(sys_time_str) > 24) || (strlen(sys_time_str) < 5))
	{
		printf("\nlenth error\n");
		return 1;
	}
	memcpy(time_str_temp,&sys_time_str[strlen(sys_time_str)-5],5);// 00:00
	GUI_SetProperty(TEXT_SYS_TIME, "string", time_str_temp);

	// ymd
	memcpy(time_str_temp,sys_time_str,strlen(sys_time_str));// 0000/00/00
	time_str_temp[strlen(sys_time_str)-5] = '\0';
	GUI_SetProperty(TEXT_SYS_DATE, "string", time_str_temp);

	return 0;
}

static int app_system_set_init(SystemSettingOpt *settingOpt)
{
	int sel;

	if(settingOpt == NULL)
		return -1;
		
	s_setting_opt = settingOpt;
	//s_wnd_open_count++;
	if(GXCORE_SUCCESS != GUI_CheckDialog("wnd_system_setting"))
	{
		GUI_CreateDialog("wnd_system_setting");
		app_system_reset_unfocus_image();
		GUI_SetProperty("text_system_setting_timezone","state","hide"/*"osd_trans_hide"*/);
		GUI_SetProperty("img_system_setting_timezone_back","state","hide"/*"osd_trans_hide"*/);
	}
	else
	{
		for(sel = 0; sel < SETTING_ITEM_MAX; sel++)
		{
			app_system_set_hide_item(sel);
		}
	}

	if(s_setting_opt->titleImage != NULL)
	{
		GUI_SetProperty("img_system_setting_title", "img", s_setting_opt->titleImage);
	}
	else
	{
		GUI_SetProperty("img_system_setting_title", "img", "s_title_left_setting.bmp");
	}

	if(s_setting_opt->backGround == BG_PLAY_RPOG)
	{
		//GUI_SetProperty(IMG_BACK_TOP, "state", "show");
        //GUI_SetProperty("text_system_setting_title","backcolor","[#FF00FF,#FF00FF,#FF00FF]");

        GUI_SetProperty(TXT_BACK_TOP, "backcolor", "[#3a3d4a,#3a3d4a,#3a3d4a]");
	}
	else
	{
		//GUI_SetProperty(IMG_BACK, "state", "show");
        //GUI_SetProperty("text_system_setting_title","backcolor","[#17174A,#17174A,#17174A]");

        GUI_SetProperty(TXT_BACK_TOP, "backcolor", "[#17174A,#17174A,#17174A]");
	}

	if(s_setting_opt->topTipDisplay == TIP_SHOW)
	{
		GUI_SetProperty("text_system_setting_title", "string", s_setting_opt->menuTitle);
		GUI_SetProperty("text_system_setting_title", "state", "show");
		GUI_SetProperty("img_system_setting_title", "state", "show");
	}

	if(s_setting_opt->bottmTipDisplay == TIP_SHOW)
	{
		GUI_SetProperty("img_system_setting_bottom", "state", "show");
	}

	if(s_setting_opt->itemNum > SETTING_ITEM_MAX)
	{
		s_item_total = SETTING_ITEM_MAX;
	}	
	else
	{
		s_item_total = s_setting_opt->itemNum;
	}
		
	for(sel = 0; sel < s_item_total; sel++)
	{
		app_system_set_item_property(sel, &s_setting_opt->item[sel].itemProperty);
		app_system_set_item_state_update(sel, s_setting_opt->item[sel].itemStatus);
	}

	s_sel = SETTING_ITEM_1;
	app_system_set_focus_item(s_sel);
	app_system_set_show_funckey();
	app_lang_set_menu_font_type("text_system_setting_title");
	if(s_setting_opt->timeDisplay == TIP_SHOW)
	{
		app_system_set_update_timer(NULL);
		GUI_SetProperty(TEXT_SYS_TIME, "state", "show");
		GUI_SetProperty(TEXT_SYS_DATE, "state", "show");
		s_time_update_timer = create_timer(app_system_set_update_timer, 1000, NULL, TIMER_REPEAT);
	}

#ifdef LINUX_OS
	if(s_setting_opt->gifDisplay == GIF_SHOW)
	{
		app_system_setting_load_gif();
	}
#endif
	return 0;
}

#ifdef LINUX_OS
static void app_system_setting_load_gif(void)
{
#define GIF_PATH WORK_PATH"theme/image/youtube/loading.gif"
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty("img_system_setting_gif", "load_img", GIF_PATH);
	GUI_SetProperty("img_system_setting_gif", "init_gif_alu_mode", &alu);
}
static void app_system_setting_free_gif(void)
{
	GUI_SetProperty("img_system_setting_gif", "load_img", NULL);
	GUI_SetProperty("img_system_setting_gif", "state", "hide");
	if(s_system_setting_gif_timer != NULL)
	{
		remove_timer(s_system_setting_gif_timer);
		s_system_setting_gif_timer = NULL;
	}
}

static int app_system_setting_draw_gif(void* usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty("img_system_setting_gif", "draw_gif", &alu);
	return 0;
}
static void app_system_setting_show_gif(void)
{
	GUI_SetProperty("img_system_setting_gif", "state", "show");
	if(0 != reset_timer(s_system_setting_gif_timer))
	{
		s_system_setting_gif_timer = create_timer(app_system_setting_draw_gif, 100, NULL, TIMER_REPEAT);
	}
}
static void app_system_setting_hide_gif(void)
{
	//remove_timer(s_system_setting_gif_timer);
	//s_system_setting_gif_timer = NULL;

	if(s_system_setting_gif_timer != NULL)
		timer_stop(s_system_setting_gif_timer);

	GUI_SetProperty("img_system_setting_gif", "state", "hide");
}

void app_system_setting_gif_show(void)
{
	app_system_setting_show_gif();
}
void app_system_setting_gif_hide(void)
{
	app_system_setting_hide_gif();
}
#endif

void app_system_set_item_property(int sel, ItemProPerty *property)
{
	if(sel >= s_item_total)
		return;
		
	if(s_setting_opt->item[sel].itemType == ITEM_CHOICE)
	{
		if(property->itemPropertyCmb.content != NULL)
		{
			GUI_SetProperty(s_item_content_cmb[sel], "content", property->itemPropertyCmb.content);
		}

		GUI_SetProperty(s_item_content_cmb[sel], "select", &(property->itemPropertyCmb.sel));
	}
	else if(s_setting_opt->item[sel].itemType == ITEM_EDIT)
	{
		GUI_SetProperty(s_item_content_edit[sel], "clear", NULL);
		
		if(property->itemPropertyEdit.format != NULL)
		{
			GUI_SetProperty(s_item_content_edit[sel], "format", property->itemPropertyEdit.format);
		}

		if(property->itemPropertyEdit.maxlen != NULL)
		{
			GUI_SetProperty(s_item_content_edit[sel], "maxlen", property->itemPropertyEdit.maxlen);
		}

		if(property->itemPropertyEdit.intaglio != NULL)
		{
			GUI_SetProperty(s_item_content_edit[sel], "intaglio", property->itemPropertyEdit.intaglio);
		}
		else
		{
			GUI_SetProperty(s_item_content_edit[sel], "intaglio", "");
		}

		if(property->itemPropertyEdit.default_intaglio != NULL)
		{
			GUI_SetProperty(s_item_content_edit[sel], "default_intaglio", property->itemPropertyEdit.default_intaglio);
		}
		else
		{
			GUI_SetProperty(s_item_content_edit[sel], "default_intaglio", "");
		}

		if(property->itemPropertyEdit.string != NULL)
		{
			GUI_SetProperty(s_item_content_edit[sel], "string", property->itemPropertyEdit.string);
		}
	}
	else if(s_setting_opt->item[sel].itemType == ITEM_PUSH)
	{
		if(property->itemPropertyBtn.string != NULL)
		{
			GUI_SetProperty(s_item_content_btn[sel], "string", property->itemPropertyBtn.string);
		}
	}
	memmove(&s_setting_opt->item[sel].itemProperty, property, sizeof(ItemProPerty));
}

void app_system_set_item_title(int sel, char *new_title)
{
	s_setting_opt->item[sel].itemTitle = new_title;
	GUI_SetProperty(s_item_title[sel], "string", s_setting_opt->item[sel].itemTitle);
}

void app_system_set_item_data_update(int sel)
{
	if(s_setting_opt->item[sel].itemType == ITEM_CHOICE)
	{
		GUI_GetProperty(s_item_content_cmb[sel], "select", &s_setting_opt->item[sel].itemProperty.itemPropertyCmb.sel);
	}
	
	if(s_setting_opt->item[sel].itemType == ITEM_EDIT)
	{
		GUI_GetProperty(s_item_content_edit[sel], "string", &s_setting_opt->item[sel].itemProperty.itemPropertyEdit.string);
	}

	if(s_setting_opt->item[sel].itemType == ITEM_PUSH)
	{
		GUI_GetProperty(s_item_content_btn[sel], "string", &s_setting_opt->item[sel].itemProperty.itemPropertyBtn.string);
	}
}

void app_system_set_item_state_update(int sel, ItemDisplayStatus state)
{
	if(sel >= s_item_total)
		return;
		
	s_setting_opt->item[sel].itemStatus = state;
	
	if(state == ITEM_HIDE)
	{
		app_system_set_hide_item(sel);
	}
	else
	{
		app_system_set_show_item(sel);
	}
}

void app_system_set_focus_item(int sel)
{
#define SS_FOCUS_FORE_COLOR "[text_focus_color,text_color,text_disable_color]"
	int item_sel = 0;
	char *img_name = IMG_BAR_HALF_FOCUS_L;

	if(s_setting_opt->item[sel].itemStatus == ITEM_NORMAL)
	{
		if(s_setting_opt->item[sel].itemType == ITEM_CHOICE)
		{
			GUI_SetProperty(s_item_content_cmb[sel], "state", "focus");
			#if MV_WIN_SUPPORT
			GUI_GetProperty("wnd_main_menu_item_listview", "select", &item_sel);
			#else
			GUI_GetProperty("listview_main_menu_opt", "select", &item_sel);//
			#endif

			if(0 != strcmp(s_setting_opt->menuTitle, STR_ID_FIRMWARE_UPGRADE))
			{
				if((2 == item_sel)||((3==item_sel)&&(0==sel)))
				{
                #if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
					GUI_SetProperty(s_item_chioce_ok[sel], "state", "show");
                #else
					img_name = IMG_BAR_HALF_FOCUS_OK_;
                #endif
				}
			}
		}
		else if(s_setting_opt->item[sel].itemType == ITEM_EDIT)
		{
			GUI_SetProperty(s_item_content_edit[sel], "state", "focus");
		}
		else if(s_setting_opt->item[sel].itemType == ITEM_PUSH)
		{
			GUI_SetProperty(s_item_content_btn[sel], "state", "focus");
		}
		//#if MV_WIN_SUPPORT
		GUI_SetProperty(s_item_choice_back[sel], "img", img_name);
		//#else
		//GUI_SetProperty(s_item_choice_back[sel], "img", IMG_BAR_HALF_FOCUS_L);
		//#endif
		GUI_SetProperty(s_item_title[sel], "string", s_setting_opt->item[sel].itemTitle);
        GUI_SetProperty(s_item_title[sel], "forecolor", SS_FOCUS_FORE_COLOR);

		s_sel = sel;
	}
}

void app_system_set_unfocus_item(int sel)
{
#define SS_UNFOCUS_FORE_COLOR "[text_color,text_color,text_disable_color]"
	if(s_setting_opt->item[sel].itemStatus == ITEM_NORMAL)
	{
		GUI_SetProperty(s_item_choice_back[sel], "img", IMG_BAR_HALF_UNFOCUS);
		GUI_SetProperty(s_item_title[sel], "string", s_setting_opt->item[sel].itemTitle);
        GUI_SetProperty(s_item_title[sel], "forecolor", SS_UNFOCUS_FORE_COLOR);
#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
		GUI_SetProperty(s_item_chioce_ok[sel], "state", "hide");
#endif
	}
}

void app_system_set_wnd_update(void)
{
	GUI_SetProperty("wnd_system_setting", "update", NULL);
}

void app_system_set_item_event(int sel, GUI_Event *event)
{
    static GUI_Event eve = {0};

    if(event == NULL)
    {
        memset(&eve, 0, sizeof(GUI_Event));
        return;
    }

    char *item = NULL;

    if(s_setting_opt->item[sel].itemType == ITEM_CHOICE)
    {
        item = s_item_content_cmb[sel];
    }
    else if(s_setting_opt->item[sel].itemType == ITEM_EDIT)
    {
        item = s_item_content_edit[sel];
    }
    else if(s_setting_opt->item[sel].itemType == ITEM_PUSH)
    {
        item = s_item_content_btn[sel];
    }
    else
    {
        memset(&eve, 0, sizeof(GUI_Event));
        return;
    }

    memcpy(&eve, event, sizeof(GUI_Event));
    GUI_SendEvent(item, &eve);
}

void app_system_set_wnd_event(GUI_Event *event)
{
    static GUI_Event eve = {0};

    if(event == NULL)
    {
        memset(&eve, 0, sizeof(GUI_Event));
        return;
    }
 
    memcpy(&eve, event, sizeof(GUI_Event));
    GUI_SendEvent("wnd_system_setting", &eve);
}

//gui 阻塞方式，适合需要返回值的情况
static bool s_sys_block_book = false;
WndStatus app_system_set_block_create(SystemSettingOpt *settingOpt)
{
	if(app_system_set_init(settingOpt) < 0)
	{
		if(s_time_update_timer != NULL)
		{
			remove_timer(s_time_update_timer);
			s_time_update_timer = NULL;
		}
		return WND_CANCLE;
	}
	s_wnd_block = true;
	s_system_setting_menu_state = WND_EXEC;

	app_block_msg_destroy(g_app_msg_self);
	while(s_system_setting_menu_state == WND_EXEC)
	{
		GUI_LoopEvent();
		GxCore_ThreadDelay(50);
	}
	GUI_StartSchedule();
	if(s_sys_block_book == true)
	{
		s_sys_block_book = false;
	}
	else
	{
		app_block_msg_init(g_app_msg_self);
		GUI_EndDialog("wnd_system_setting");
		GUI_SetInterface("flush",NULL);
	}
	return s_system_setting_menu_state;
}

void app_system_reset_unfocus_image(void)
{
	if(s_setting_opt->item[s_sel].itemType == ITEM_CHOICE)
	{
#if (MINI_16_BITS_OSD_SUPPORT)
		GUI_SetProperty(s_item_content_cmb[s_sel], "unfocus_img", IMG_BAR_HALF_UNFOCUS_CHOICE);
#else
		GUI_SetProperty(s_item_content_cmb[s_sel], "unfocus_img", IMG_BAR_HALF_UNFOCUS);
#endif
	}
	else if(s_setting_opt->item[s_sel].itemType == ITEM_EDIT)
	{
		GUI_SetProperty(s_item_content_edit[s_sel], "unfocus_img", IMG_BAR_HALF_UNFOCUS);
	}
	else if(s_setting_opt->item[s_sel].itemType == ITEM_PUSH)
	{
		GUI_SetProperty(s_item_content_btn[s_sel], "unfocus_img", IMG_BAR_HALF_UNFOCUS);
	}
	return ;
}

WndStatus app_system_set_create(SystemSettingOpt *settingOpt)
{
	WndStatus ret = WND_OK;

	if(app_system_set_init(settingOpt) < 0)
	{
		if(s_time_update_timer != NULL)
		{
			remove_timer(s_time_update_timer);
			s_time_update_timer = NULL;
		}
		ret = WND_CANCLE;
	}
	s_wnd_block = false;
	return ret;
}

void app_system_set_destroy(ExitType exit_type)
{
	WndStatus menu_ret = WND_CANCLE;
	int i;

	for(i = 0; i < s_item_total; i++)
	{
		app_system_set_item_data_update(i);
	};
	menu_ret = WND_CANCLE;	
	if(s_setting_opt->exit != NULL)
	{
		if(s_setting_opt->exit(exit_type) == 1)
		{
			menu_ret = WND_OK;
		}
	}
	if(s_time_update_timer != NULL)
	{
		remove_timer(s_time_update_timer);
		s_time_update_timer = NULL;
	}
	if(s_wnd_block)
	{
		s_system_setting_menu_state = menu_ret;
	}
	else
	{
		GUI_EndDialog("wnd_system_setting");
		GUI_SetProperty("wnd_system_setting","draw_now",NULL);
	}
}

SIGNAL_HANDLER int app_system_set_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;
	WndStatus menu_ret = WND_CANCLE;
	int i;
	
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_UP:
				app_system_set_unfocus_item(s_sel);
				s_sel = app_system_set_last_sel(s_sel);
				app_system_set_focus_item(s_sel);
				break;

			case STBK_DOWN:
				app_system_set_unfocus_item(s_sel);
				s_sel = app_system_set_next_sel(s_sel);
				app_system_set_focus_item(s_sel);
				break;

			case VK_BOOK_TRIGGER:
				if(s_setting_opt->exit != NULL)
				{
					s_setting_opt->exit(EXIT_ABANDON);
				}
				if(s_time_update_timer != NULL)
				{
					remove_timer(s_time_update_timer);
					s_time_update_timer = NULL;
				}
				if(s_wnd_block)
				{
					app_block_msg_init(g_app_msg_self);
					s_sys_block_book = true;
					s_system_setting_menu_state = WND_CANCLE;
					if(GUI_CheckDialog("wnd_epg") != GXCORE_SUCCESS)
					{
						GUI_EndDialog("after wnd_full_screen");
					}
					else
					{
						GUI_EndDialog("after wnd_full_screen");
						PlayerWindow video_wnd = {0, 0, 1024, 576};
						GUI_SetInterface("flush", NULL);
						g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
						GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
					}
				}
				else
				{
					GUI_EndDialog("after wnd_full_screen");
				}
				break;
				
			case STBK_EXIT:
			case STBK_MENU:
				for(i = 0; i < s_item_total; i++)
				{
					app_system_set_item_data_update(i);
				}
				if(s_setting_opt->exit != NULL)
				{
					if(s_setting_opt->exit(EXIT_NORMAL) == 1)
					{
						menu_ret = WND_OK;
					}
				}
				if(s_time_update_timer != NULL)
				{
					remove_timer(s_time_update_timer);
					s_time_update_timer = NULL;
				}
				if(s_wnd_block)
				{
					s_system_setting_menu_state = menu_ret;
				}
				else
				{
					GUI_EndDialog("wnd_system_setting");
					GUI_SetProperty("wnd_system_setting","draw_now",NULL);
				}
			#ifdef LINUX_OS
				app_system_setting_free_gif();
			#endif
				break;

			case STBK_RED:
				if(s_setting_opt->menuFuncKey.redKey.keyPressFun != NULL)
				{
					s_setting_opt->menuFuncKey.redKey.keyPressFun();
				}
				break;

			case STBK_GREEN:
				if(s_setting_opt->menuFuncKey.greenKey.keyPressFun != NULL)
				{
					s_setting_opt->menuFuncKey.greenKey.keyPressFun();
				}
				break;

			case STBK_BLUE:
				if(s_setting_opt->menuFuncKey.blueKey.keyPressFun != NULL)
				{
					s_setting_opt->menuFuncKey.blueKey.keyPressFun();
				}
				break;

			case STBK_YELLOW:
				if(s_setting_opt->menuFuncKey.yellowKey.keyPressFun != NULL)
				{
					s_setting_opt->menuFuncKey.yellowKey.keyPressFun();
				}
				break;	

			default:
				break;
		}		
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_got_focus(GuiWidget *widget, void *usrdata)
{
	app_system_reset_unfocus_image();
	if(s_setting_opt->timeDisplay == TIP_SHOW)
	{
		s_time_update_timer = create_timer(app_system_set_update_timer, 1000, NULL, TIMER_REPEAT);
	}
    	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_system_set_lost_focus(GuiWidget *widget, void *usrdata)
{
	if(s_time_update_timer != NULL)
	{
		remove_timer(s_time_update_timer);
		s_time_update_timer = NULL;
	}
	if(s_setting_opt->item[s_sel].itemType == ITEM_CHOICE)
	{
	#if (MINI_16_BITS_OSD_SUPPORT)
		GUI_SetProperty(s_item_content_cmb[s_sel], "unfocus_img", IMG_BAR_HALF_FOCUS_CHOICE);
	#else
		GUI_SetProperty(s_item_content_cmb[s_sel], "unfocus_img", IMG_BAR_HALF_FOCUS_R);
	#endif
	}
	else if(s_setting_opt->item[s_sel].itemType == ITEM_EDIT)
	{
		GUI_SetProperty(s_item_content_edit[s_sel], "unfocus_img", IMG_BAR_HALF_FOCUS_R);
	}
	else if(s_setting_opt->item[s_sel].itemType == ITEM_PUSH)
	{
		GUI_SetProperty(s_item_content_btn[s_sel], "unfocus_img", IMG_BAR_HALF_FOCUS_R);
	}
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_system_set_btn1_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_1) && (s_setting_opt->item[SETTING_ITEM_1].itemCallback.btnCallback.BtnPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_1].itemCallback.btnCallback.BtnPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_btn2_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_2) && (s_setting_opt->item[SETTING_ITEM_2].itemCallback.btnCallback.BtnPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_2].itemCallback.btnCallback.BtnPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_btn3_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_3) && (s_setting_opt->item[SETTING_ITEM_3].itemCallback.btnCallback.BtnPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_3].itemCallback.btnCallback.BtnPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_btn4_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_4) && (s_setting_opt->item[SETTING_ITEM_4].itemCallback.btnCallback.BtnPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_4].itemCallback.btnCallback.BtnPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_btn5_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_5) && (s_setting_opt->item[SETTING_ITEM_5].itemCallback.btnCallback.BtnPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_5].itemCallback.btnCallback.BtnPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_btn6_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_6) && (s_setting_opt->item[SETTING_ITEM_6].itemCallback.btnCallback.BtnPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_6].itemCallback.btnCallback.BtnPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_btn7_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_7) && (s_setting_opt->item[SETTING_ITEM_7].itemCallback.btnCallback.BtnPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_7].itemCallback.btnCallback.BtnPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_btn8_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_8) && (s_setting_opt->item[SETTING_ITEM_8].itemCallback.btnCallback.BtnPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_8].itemCallback.btnCallback.BtnPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_btn9_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_9) && (s_setting_opt->item[SETTING_ITEM_9].itemCallback.btnCallback.BtnPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_9].itemCallback.btnCallback.BtnPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_btn10_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_10) && (s_setting_opt->item[SETTING_ITEM_10].itemCallback.btnCallback.BtnPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_10].itemCallback.btnCallback.BtnPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_edit1_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_1) && (s_setting_opt->item[SETTING_ITEM_1].itemCallback.editCallback.EditPress!= NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_1].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_system_set_edit2_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_2) && (s_setting_opt->item[SETTING_ITEM_2].itemCallback.editCallback.EditPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_2].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_system_set_edit3_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_3) && (s_setting_opt->item[SETTING_ITEM_3].itemCallback.editCallback.EditPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_3].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_system_set_edit4_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_4) && (s_setting_opt->item[SETTING_ITEM_4].itemCallback.editCallback.EditPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_4].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_system_set_edit5_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_5) && (s_setting_opt->item[SETTING_ITEM_5].itemCallback.editCallback.EditPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_5].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_system_set_edit6_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_6) && (s_setting_opt->item[SETTING_ITEM_6].itemCallback.editCallback.EditPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_6].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_system_set_edit7_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_7) && (s_setting_opt->item[SETTING_ITEM_7].itemCallback.editCallback.EditPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_7].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_system_set_edit8_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_8) && (s_setting_opt->item[SETTING_ITEM_8].itemCallback.editCallback.EditPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_8].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_system_set_edit9_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_9) && (s_setting_opt->item[SETTING_ITEM_9].itemCallback.editCallback.EditPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_9].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_system_set_edit10_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_10) && (s_setting_opt->item[SETTING_ITEM_10].itemCallback.editCallback.EditPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_10].itemCallback.editCallback.EditPress(event->key.sym);
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_system_set_edit1_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_item_total > SETTING_ITEM_1) && (s_setting_opt->item[SETTING_ITEM_1].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_item_content_edit[SETTING_ITEM_1], "string", &s);
		ret = s_setting_opt->item[SETTING_ITEM_1].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_edit2_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_item_total > SETTING_ITEM_2) && (s_setting_opt->item[SETTING_ITEM_2].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_item_content_edit[SETTING_ITEM_2], "string", &s);
		ret = s_setting_opt->item[SETTING_ITEM_2].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_edit3_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_item_total > SETTING_ITEM_3) && (s_setting_opt->item[SETTING_ITEM_3].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_item_content_edit[SETTING_ITEM_3], "string", &s);
		ret = s_setting_opt->item[SETTING_ITEM_3].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_edit4_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_item_total > SETTING_ITEM_4) && (s_setting_opt->item[SETTING_ITEM_4].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_item_content_edit[SETTING_ITEM_4], "string", &s);
		ret = s_setting_opt->item[SETTING_ITEM_4].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_edit5_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_item_total > SETTING_ITEM_5) && (s_setting_opt->item[SETTING_ITEM_5].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_item_content_edit[SETTING_ITEM_5], "string", &s);
		ret = s_setting_opt->item[SETTING_ITEM_5].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_edit6_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_item_total > SETTING_ITEM_6) && (s_setting_opt->item[SETTING_ITEM_6].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_item_content_edit[SETTING_ITEM_6], "string", &s);
		ret = s_setting_opt->item[SETTING_ITEM_6].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_edit7_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_item_total > SETTING_ITEM_7) && (s_setting_opt->item[SETTING_ITEM_7].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_item_content_edit[SETTING_ITEM_7], "string", &s);
		ret = s_setting_opt->item[SETTING_ITEM_7].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_edit8_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_item_total > SETTING_ITEM_8) && (s_setting_opt->item[SETTING_ITEM_8].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_item_content_edit[SETTING_ITEM_8], "string", &s);
		ret = s_setting_opt->item[SETTING_ITEM_8].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_edit9_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_item_total > SETTING_ITEM_9) && (s_setting_opt->item[SETTING_ITEM_9].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_item_content_edit[SETTING_ITEM_9], "string", &s);
		ret = s_setting_opt->item[SETTING_ITEM_9].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_edit10_reach_end(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	if((s_item_total > SETTING_ITEM_10) && (s_setting_opt->item[SETTING_ITEM_10].itemCallback.editCallback.EditReachEnd!= NULL))
	{
		char *s = NULL;
		GUI_GetProperty(s_item_content_edit[SETTING_ITEM_10], "string", &s);
		ret = s_setting_opt->item[SETTING_ITEM_10].itemCallback.editCallback.EditReachEnd(s);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb1_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;

	if((s_item_total > SETTING_ITEM_1) && (s_setting_opt->item[SETTING_ITEM_1].itemCallback.cmbCallback.CmbChange!= NULL))
	{
		int sel = 0;
		GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_1], "select", &sel);
		ret = s_setting_opt->item[SETTING_ITEM_1].itemCallback.cmbCallback.CmbChange(sel);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb2_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
    
	GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_2], "select", &s_setting_opt->item[SETTING_ITEM_2].itemProperty.itemPropertyCmb.sel);

	if((s_item_total > SETTING_ITEM_2) && (s_setting_opt->item[SETTING_ITEM_2].itemCallback.cmbCallback.CmbChange!= NULL))
	{
		int sel = 0;
		GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_2], "select", &sel);
		ret = s_setting_opt->item[SETTING_ITEM_2].itemCallback.cmbCallback.CmbChange(sel);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb3_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;

	GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_3], "select", &s_setting_opt->item[SETTING_ITEM_3].itemProperty.itemPropertyCmb.sel);

	if((s_item_total > SETTING_ITEM_3) && (s_setting_opt->item[SETTING_ITEM_3].itemCallback.cmbCallback.CmbChange!= NULL))
	{
		int sel = 0;
		GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_3], "select", &sel);
		ret = s_setting_opt->item[SETTING_ITEM_3].itemCallback.cmbCallback.CmbChange(sel);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb4_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;

	GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_4], "select", &s_setting_opt->item[SETTING_ITEM_4].itemProperty.itemPropertyCmb.sel);

	if((s_item_total > SETTING_ITEM_4) && (s_setting_opt->item[SETTING_ITEM_4].itemCallback.cmbCallback.CmbChange!= NULL))
	{
		int sel = 0;
		GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_4], "select", &sel);
		ret = s_setting_opt->item[SETTING_ITEM_4].itemCallback.cmbCallback.CmbChange(sel);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb5_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;

	GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_5], "select", &s_setting_opt->item[SETTING_ITEM_5].itemProperty.itemPropertyCmb.sel);

	if((s_item_total > SETTING_ITEM_5) && (s_setting_opt->item[SETTING_ITEM_5].itemCallback.cmbCallback.CmbChange!= NULL))
	{
		int sel = 0;
		GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_5], "select", &sel);
		ret = s_setting_opt->item[SETTING_ITEM_5].itemCallback.cmbCallback.CmbChange(sel);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb6_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;

	GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_6], "select", &s_setting_opt->item[SETTING_ITEM_6].itemProperty.itemPropertyCmb.sel);

	if((s_item_total > SETTING_ITEM_6) && (s_setting_opt->item[SETTING_ITEM_6].itemCallback.cmbCallback.CmbChange!= NULL))
	{
		int sel = 0;
		GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_6], "select", &sel);
		ret = s_setting_opt->item[SETTING_ITEM_6].itemCallback.cmbCallback.CmbChange(sel);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb7_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
    
	GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_7], "select", &s_setting_opt->item[SETTING_ITEM_7].itemProperty.itemPropertyCmb.sel);

	if((s_item_total > SETTING_ITEM_7) && (s_setting_opt->item[SETTING_ITEM_7].itemCallback.cmbCallback.CmbChange!= NULL))
	{
		int sel = 0;
		GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_7], "select", &sel);
		ret = s_setting_opt->item[SETTING_ITEM_7].itemCallback.cmbCallback.CmbChange(sel);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb8_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
    
	GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_8], "select", &s_setting_opt->item[SETTING_ITEM_8].itemProperty.itemPropertyCmb.sel);

	if((s_item_total > SETTING_ITEM_8) && (s_setting_opt->item[SETTING_ITEM_8].itemCallback.cmbCallback.CmbChange!= NULL))
	{
		int sel = 0;
		GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_8], "select", &sel);
		ret = s_setting_opt->item[SETTING_ITEM_8].itemCallback.cmbCallback.CmbChange(sel);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb9_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
    
	GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_9], "select", &s_setting_opt->item[SETTING_ITEM_9].itemProperty.itemPropertyCmb.sel);

	if((s_item_total > SETTING_ITEM_9) && (s_setting_opt->item[SETTING_ITEM_6].itemCallback.cmbCallback.CmbChange!= NULL))
	{
		int sel = 0;
		GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_9], "select", &sel);
		ret = s_setting_opt->item[SETTING_ITEM_9].itemCallback.cmbCallback.CmbChange(sel);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb10_change(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
    
	GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_10], "select", &s_setting_opt->item[SETTING_ITEM_10].itemProperty.itemPropertyCmb.sel);
    
	if((s_item_total > SETTING_ITEM_10) && (s_setting_opt->item[SETTING_ITEM_10].itemCallback.cmbCallback.CmbChange!= NULL))
	{
		int sel = 0;
		GUI_GetProperty(s_item_content_cmb[SETTING_ITEM_10], "select", &sel);
		ret = s_setting_opt->item[SETTING_ITEM_10].itemCallback.cmbCallback.CmbChange(sel);
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb1_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type) 
	{
		//#if MV_WIN_SUPPORT
		if((s_item_total > SETTING_ITEM_1) && (s_setting_opt->item[SETTING_ITEM_1].itemCallback.cmbCallback.CmbPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_1].itemCallback.cmbCallback.CmbPress(event->key.sym);
		}
		//#endif
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb2_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		//#if MV_WIN_SUPPORT
		if((s_item_total > SETTING_ITEM_2) && (s_setting_opt->item[SETTING_ITEM_2].itemCallback.cmbCallback.CmbPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_2].itemCallback.cmbCallback.CmbPress(event->key.sym);
		}
		//#endif
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb3_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		//#if MV_WIN_SUPPORT
		if((s_item_total > SETTING_ITEM_3) && (s_setting_opt->item[SETTING_ITEM_3].itemCallback.cmbCallback.CmbPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_3].itemCallback.cmbCallback.CmbPress(event->key.sym);
		}
		//#endif
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb4_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
		
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
	//	#if MV_WIN_SUPPORT
		if((s_item_total > SETTING_ITEM_4) && (s_setting_opt->item[SETTING_ITEM_4].itemCallback.cmbCallback.CmbPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_4].itemCallback.cmbCallback.CmbPress(event->key.sym);
		}
	//	#endif
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb5_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
	event = (GUI_Event *)usrdata;
	//#if MV_WIN_SUPPORT
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_5) && (s_setting_opt->item[SETTING_ITEM_5].itemCallback.cmbCallback.CmbPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_5].itemCallback.cmbCallback.CmbPress(event->key.sym);
		}
	}
	//#endif
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb6_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
	
	event = (GUI_Event *)usrdata;
	//#if MV_WIN_SUPPORT	
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_6) && (s_setting_opt->item[SETTING_ITEM_6].itemCallback.cmbCallback.CmbPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_6].itemCallback.cmbCallback.CmbPress(event->key.sym);
		}
	}
	//#endif
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb7_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
	
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_7) && (s_setting_opt->item[SETTING_ITEM_7].itemCallback.cmbCallback.CmbPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_7].itemCallback.cmbCallback.CmbPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb8_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
	
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_8) && (s_setting_opt->item[SETTING_ITEM_8].itemCallback.cmbCallback.CmbPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_8].itemCallback.cmbCallback.CmbPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb9_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
	
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_9) && (s_setting_opt->item[SETTING_ITEM_9].itemCallback.cmbCallback.CmbPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_9].itemCallback.cmbCallback.CmbPress(event->key.sym);
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_system_set_cmb10_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
	
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if((s_item_total > SETTING_ITEM_10) && (s_setting_opt->item[SETTING_ITEM_10].itemCallback.cmbCallback.CmbPress != NULL))
		{
			ret = s_setting_opt->item[SETTING_ITEM_10].itemCallback.cmbCallback.CmbPress(event->key.sym);
		}
	}
	return ret;
}
