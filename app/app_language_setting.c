#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_config.h"
#include "app_epg.h"

#define LANG_NAME_MAX   (16)

enum TIME_AV_NAME 
{
	ITEM_MENU_LANG = 0,
	ITEM_AUDIO_1ST,
	ITEM_AUDIO_2ND,
	ITEM_SUB_LANG,
	ITEM_TTX_LANG,
	ITEM_EPG_LANG,
	ITEM_LANG_TOTAL
};

typedef enum
{
	LANG_PRIORITY_EN,
	LANG_PRIORITY_CHS
}LangPriority;

typedef enum
{
	LANG_EPG_EN,
	LANG_EPG_CHS
}LangEpg;

typedef struct
{
	uint32_t     menu_lang;
	uint32_t audio_1;
	uint32_t audio_2;
	uint32_t sub_1;
	uint32_t sub_2;
	uint32_t ttx_lang;
	uint32_t epg_lang;
}SysLangPara;

static SystemSettingOpt s_lang_setting_opt;
static SystemSettingItem s_lang_item[ITEM_LANG_TOTAL];
static SysLangPara s_sys_lang_para;
static char s_lang_array[LANG_MAX*LANG_NAME_MAX] = {0};
static char s_epg_lang_array[(LANG_MAX+1)*LANG_NAME_MAX] = {0};
static char s_ttx_lang_array[(LANG_MAX+1)*LANG_NAME_MAX] = {0};


static uint32_t _lang_to_sel(uint32_t lang)
{
    if(lang >= LANG_MAX)
        return 0;
    else
        return lang+1;
}

static uint32_t _sel_to_lang(uint32_t sel)
{
    if(sel == 0)
        return LANG_MAX;
    else
        return sel-1;
}

char *app_get_short_lang_str(uint32_t lang)
{
#define LANG_SHORT_ENG    "eng"
#define LANG_SHORT_CHI    "chi"
#define LANG_SHORT_VIE    "vie"
#define LANG_SHORT_ARA    "ara"
#define LANG_SHORT_RUS    "rus"
#define LANG_SHORT_FRA    "fra"
#define LANG_SHORT_POR    "por"
#define LANG_SHORT_TUR    "tur"
#define LANG_SHORT_SPA    "spa"
#define LANG_SHORT_TUR    "tur"
#define LANG_SHORT_ITA    "ita"
#define LANG_SHORT_POL    "pol"
#define LANG_SHORT_GER    "deu"//"ger"
#define LANG_SHORT_FAS    "fas"
#define LANG_SHORT_ALL    "all"

    char *str = NULL;
    if(lang >= LANG_MAX)
    {
        str = LANG_SHORT_ALL;
    }
    else
    {
        if(strcmp(g_LangName[lang], STR_ID_ENGLISH) == 0)
        {
            str = LANG_SHORT_ENG;
        }
        else if(strcmp(g_LangName[lang], STR_ID_CHINESES) == 0)
        {
            str = LANG_SHORT_CHI;
        }
        else if(strcmp(g_LangName[lang], STR_ID_VIETNAMESE) == 0)
        {
            str = LANG_SHORT_VIE;
        }
        else if(strcmp(g_LangName[lang], STR_ID_ARABIC) == 0)
        {
            str = LANG_SHORT_ARA;
        }
        else if(strcmp(g_LangName[lang], STR_ID_RUSSIAN) == 0)
        {
            str = LANG_SHORT_RUS;
        }
        else if(strcmp(g_LangName[lang], STR_ID_FRENCH) == 0)
        {
            str = LANG_SHORT_FRA;
        }
        else if(strcmp(g_LangName[lang], STR_ID_PORTUGUESE) == 0)
        {
            str = LANG_SHORT_POR;
        }
        else if(strcmp(g_LangName[lang], STR_ID_TURKISH) == 0)
        {
            str = LANG_SHORT_TUR;
        }
        else if(strcmp(g_LangName[lang], STR_ID_SPAIN) == 0)
        {
            str = LANG_SHORT_SPA;
        }
        else if(strcmp(g_LangName[lang], STR_ID_ITALIAN) == 0)
        {
            str = LANG_SHORT_ITA;
        }
        else if(strcmp(g_LangName[lang], STR_ID_POLSKI) == 0)
        {
            str = LANG_SHORT_POL;
        }
        else if(strcmp(g_LangName[lang], STR_ID_GERMAN) == 0)
        {
            str = LANG_SHORT_GER;
        }
        else if(strcmp(g_LangName[lang], STR_ID_FARSI) == 0)
        {
            str = LANG_SHORT_FAS;
        }
        else
        {
            str = LANG_SHORT_ALL;
        }
    }

    return str;
}

///should free return value outside
static char *app_sel_to_priority_lang_str(uint32_t lang_1, uint32_t lang_2)
{
	char *str1 = NULL;
	char *str2 = NULL;
	int len1 = 0;
	int len2 = 0;
	char *result = NULL;

	str1 = app_get_short_lang_str(lang_1);
	len1= strlen(str1);
	
	str2 = app_get_short_lang_str(lang_2);
	len2= strlen(str2);
	
	result = (char *)GxCore_Malloc(len1 + len2 + 2);
	if(result != NULL)
	{
		memset(result, 0, len1 + len2 + 2);
		memcpy(result, str1, len1);
		result[len1] = '_';
		memcpy(result + len1 + 1 , str2, len2);
	}
	return result;
}

static void app_lang_system_para_get(void)
{
    int init_value = 0;

    GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
    s_sys_lang_para.menu_lang= init_value;

    GxBus_ConfigGetInt(AUDIO_1ST_KEY, &init_value, OSD_LANG);
    s_sys_lang_para.audio_1= init_value;

    GxBus_ConfigGetInt(AUDIO_2ND_KEY, &init_value, OSD_LANG);
    s_sys_lang_para.audio_2= init_value;

    GxBus_ConfigGetInt(SUBTILE_1ST_KEY, &init_value, OSD_LANG);
    s_sys_lang_para.sub_1 = init_value;

    //GxBus_ConfigGetInt(SUBTILE_2ND_KEY, &init_value, OSD_LANG);
    //s_sys_lang_para.sub_2 = init_value;

    GxBus_ConfigGetInt(TTX_LANG_KEY, &init_value, TTX_LANG);
    s_sys_lang_para.ttx_lang= _lang_to_sel(init_value);

    GxBus_ConfigGetInt(EPG_LANG_KEY, &init_value, EPG_LANG);
    s_sys_lang_para.epg_lang= _lang_to_sel(init_value);
}

static void app_lang_result_para_get(SysLangPara *ret_para)
{
    ret_para->menu_lang = s_lang_item[ITEM_MENU_LANG].itemProperty.itemPropertyCmb.sel;
    ret_para->audio_1 = s_lang_item[ITEM_AUDIO_1ST].itemProperty.itemPropertyCmb.sel;
    ret_para->audio_2 = s_lang_item[ITEM_AUDIO_2ND].itemProperty.itemPropertyCmb.sel;
    ret_para->sub_1 = s_lang_item[ITEM_SUB_LANG].itemProperty.itemPropertyCmb.sel;
    //ret_para->sub_2 = s_lang_item[ITEM_SUB_2ND].itemProperty.itemPropertyCmb.sel;
    ret_para->ttx_lang = s_lang_item[ITEM_TTX_LANG].itemProperty.itemPropertyCmb.sel;
    ret_para->epg_lang = s_lang_item[ITEM_EPG_LANG].itemProperty.itemPropertyCmb.sel;
}

static bool app_lang_setting_para_change(SysLangPara *ret_para)
{
	bool ret = FALSE;

	if(s_sys_lang_para.menu_lang != ret_para->menu_lang)
	{
		ret = TRUE;
	}

	if(s_sys_lang_para.audio_1 != ret_para->audio_1)
	{
		ret = TRUE;
	}

	if(s_sys_lang_para.audio_2 != ret_para->audio_2)
	{
		ret = TRUE;
	}

	if(s_sys_lang_para.sub_1 != ret_para->sub_1)
	{
		ret = TRUE;
	}

	//if(s_sys_lang_para.sub_2 != ret_para->sub_2)
	//{
	//	ret = TRUE;
	//}

	if(s_sys_lang_para.ttx_lang != ret_para->ttx_lang)
	{
		ret = TRUE;
	}

	if(s_sys_lang_para.epg_lang!= ret_para->epg_lang)
	{
		ret = TRUE;
	}

	return ret;
}

static int app_lang_setting_para_save(SysLangPara *ret_para)
{
    int init_value = 0;
    char * priority_lang = NULL;

	if(s_sys_lang_para.menu_lang != ret_para->menu_lang)
    {
        init_value = ret_para->menu_lang;
        GxBus_ConfigSetInt(OSD_LANG_KEY,init_value);
    }

	if(s_sys_lang_para.audio_1 != ret_para->audio_1)
    {
        init_value = ret_para->audio_1;
        GxBus_ConfigSetInt(AUDIO_1ST_KEY,init_value);
    }

	if(s_sys_lang_para.audio_2 != ret_para->audio_2)
    {
        init_value = ret_para->audio_2;
        GxBus_ConfigSetInt(AUDIO_2ND_KEY,init_value);
    }

    if((s_sys_lang_para.audio_1 != ret_para->audio_1) || (s_sys_lang_para.audio_2 != ret_para->audio_2))
    {
        priority_lang = app_sel_to_priority_lang_str(ret_para->audio_1, ret_para->audio_2);
        if(priority_lang != NULL)
        {
            GxBus_ConfigSet(PRIORITY_LANG_KEY, priority_lang);
            GxCore_Free(priority_lang);
            priority_lang = NULL;
        }
    }

	if(s_sys_lang_para.sub_1 != ret_para->sub_1)
    {
        init_value = ret_para->sub_1;
        GxBus_ConfigSetInt(SUBTILE_1ST_KEY,init_value);
    }

#if 0
    if(s_sys_lang_para.sub_2 != ret_para->sub_2)
    {
        init_value = ret_para->sub_2;
        GxBus_ConfigSetInt(SUBTILE_2ND_KEY,init_value);
    }

    if((s_sys_lang_para.sub_1 != ret_para->sub_1) || (s_sys_lang_para.sub_2 != ret_para->sub_2))
    {
        priority_lang = app_sel_to_priority_lang_str(ret_para->sub_1, ret_para->sub_1);//Correct! It's double!
        if(priority_lang != NULL)
        {
            GxBus_ConfigSet(PRIORITY_LANG_KEY, priority_lang);
            GxCore_Free(priority_lang);
            priority_lang = NULL;
        }
    }
#endif

	if(s_sys_lang_para.ttx_lang != ret_para->ttx_lang)
    {
        init_value = _sel_to_lang(ret_para->ttx_lang);
        GxBus_ConfigSetInt(TTX_LANG_KEY,init_value);
    }

	if(s_sys_lang_para.epg_lang!= ret_para->epg_lang)
    {
        //app_epg_disable();
        init_value = _sel_to_lang(ret_para->epg_lang);
        GxBus_ConfigSetInt(EPG_LANG_KEY,init_value);
        //app_epg_enable(g_AppPlayOps.normal_play.ts_src,TS_2_DMX);
    }

    return 0;
}

static int app_lang_setting_exit_callback(ExitType exit_type)
{
    SysLangPara para_ret;
    int ret = 0;

    if(exit_type == EXIT_ABANDON)
    {
        GUI_SetInterface("osd_language",g_LangName[s_sys_lang_para.menu_lang]);  
    }
    else
    {
        app_lang_result_para_get(&para_ret);
        if(app_lang_setting_para_change(&para_ret) == TRUE)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.str = STR_ID_SAVE_INFO;
            if(popdlg_create(&pop) == POP_VAL_OK)
            {
                app_lang_setting_para_save(&para_ret);
                ret = 1;
            }
            else
            {
                //printf("language : %s\n",g_LangName[s_sys_lang_para.menu_lang]);
                GUI_SetInterface("osd_language",g_LangName[s_sys_lang_para.menu_lang]);  
            }
        }
    }
    return ret;
}

static int app_menu_lang_set_font_type(const char *widget_name,int sel)
{
#if MINI_256_COLORS_OSD_SUPPORT

	 if(strcmp(g_LangName[sel],"Thailand")==0)
	 {
		 GUI_SetProperty(widget_name, "font", "thai_main");
	 }
	 else
	 {
		 GUI_SetProperty(widget_name, "font", "font_main");
	 }
#endif
	return 0;
}

int app_menu_lang_change_callback(int sel)
{
	printf("language : %s\n",g_LangName[sel]);
	GUI_SetInterface("osd_language",g_LangName[sel]);
	app_menu_lang_set_font_type("text_system_setting_title",sel);//cmb_system_setting_opt1
	app_system_set_wnd_update();

	return EVENT_TRANSFER_KEEPON;
}

//#if MV_WIN_SUPPORT
static int app_lang_menu_lang_pop(int sel)
{
	PopList pop_list;
	int ret =-1;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_MENU_LANG;
	pop_list.item_num = LANG_MAX;
	pop_list.item_content = g_LangName;
	pop_list.sel = sel;
	pop_list.show_num= false;

	ret = poplist_create(&pop_list);

	return ret;
}

static int app_menu_lang_cmb_press_callback(int key)
{
	//PopList pop_list;
	int cmb_sel =-1;

	if(key == STBK_OK)
	{
		GUI_GetProperty("cmb_system_setting_opt1", "select", &cmb_sel);
		cmb_sel = app_lang_menu_lang_pop(cmb_sel);
		GUI_SetProperty("cmb_system_setting_opt1", "select", &cmb_sel);
		app_menu_lang_set_font_type("text_system_setting_title",cmb_sel);
		app_menu_lang_set_font_type("text_main_menu_title",cmb_sel);
	}
	return EVENT_TRANSFER_KEEPON;
}

static int app_lang_setting_audio_1_cmb_press_callback(int key)
{
	PopList pop_list;
	int cmb_sel =-1;

	app_system_set_item_data_update(ITEM_AUDIO_1ST);
	if(key == STBK_OK)
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_AUDIO_LANG_1;
		pop_list.item_num = LANG_MAX; 
		pop_list.item_content = g_LangName;
		pop_list.sel = s_lang_item[ITEM_AUDIO_1ST].itemProperty.itemPropertyCmb.sel;//s_sys_lang_para.audio_1;
		pop_list.show_num= false;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt2", "select", &cmb_sel);
	}

	return EVENT_TRANSFER_KEEPON;
}

static int app_lang_setting_audio_2_cmb_press_callback(int key)
{
	PopList pop_list;
	int cmb_sel =-1;

	app_system_set_item_data_update(ITEM_AUDIO_2ND);
	if(key == STBK_OK)
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_AUDIO_LANG_2;
		pop_list.item_num = LANG_MAX; 
		pop_list.item_content = g_LangName;
		pop_list.sel = s_lang_item[ITEM_AUDIO_2ND].itemProperty.itemPropertyCmb.sel;//s_sys_lang_para.audio_2;
		pop_list.show_num= false;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt3", "select", &cmb_sel);
	}

	return EVENT_TRANSFER_KEEPON;
}

static int app_lang_setting_sub_lang_cmb_press_callback(int key)
{
	PopList pop_list;
	int cmb_sel =-1;

	app_system_set_item_data_update(ITEM_SUB_LANG);
	if(key == STBK_OK)
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_SUB_LANG;
		pop_list.item_num = LANG_MAX; 
		pop_list.item_content = g_LangName;
		pop_list.sel = s_lang_item[ITEM_SUB_LANG].itemProperty.itemPropertyCmb.sel;//s_sys_lang_para.sub_1;
		pop_list.show_num= false;
		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt4", "select", &cmb_sel);
	}

	return EVENT_TRANSFER_KEEPON;
}

static int app_lang_setting_ttx_cmb_press_callback(int key)
{
	PopList pop_list;
	int cmb_sel =-1;
	static char * s_ttx_lang[LANG_MAX+1]={};
	int i = 0;

	app_system_set_item_data_update(ITEM_TTX_LANG);
	if(key == STBK_OK)
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_TTX_LANG;
		pop_list.item_num = LANG_MAX+1; 
        for(i = 0; i < LANG_MAX; i++)
		{
			s_ttx_lang[_lang_to_sel(i)] = g_LangName[i];
		}
		s_ttx_lang[_lang_to_sel(LANG_MAX)] = STR_ID_AUTO;
		pop_list.item_content = s_ttx_lang;
		pop_list.sel = s_lang_item[ITEM_TTX_LANG].itemProperty.itemPropertyCmb.sel;//s_sys_lang_para.ttx_lang;
		pop_list.show_num= false;
		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt5", "select", &cmb_sel);
	}

	return EVENT_TRANSFER_KEEPON;
}

static int app_lang_setting_epg_cmb_press_callback(int key)
{
	PopList pop_list;
	int cmb_sel =-1;
	static char * s_epg_lang[LANG_MAX+1]={};
	int i = 0;
	
	app_system_set_item_data_update(ITEM_EPG_LANG);
	if(key == STBK_OK)
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_EPG_LANG;
		pop_list.item_num = LANG_MAX +1; 
		for(i = 0; i < LANG_MAX; i++)
		{
			s_epg_lang[_lang_to_sel(i)] = g_LangName[i];
		}
		s_epg_lang[_lang_to_sel(LANG_MAX)] = STR_ID_ALL;
		pop_list.item_content = s_epg_lang;
		pop_list.sel = s_lang_item[ITEM_EPG_LANG].itemProperty.itemPropertyCmb.sel;//s_sys_lang_para.epg_lang;
		pop_list.show_num= false;
		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt6", "select", &cmb_sel);
	}

	return EVENT_TRANSFER_KEEPON;
}
//#endif
static void app_lang_setting_menu_lang_item_init(void)
{
	s_lang_item[ITEM_MENU_LANG].itemTitle = STR_ID_MENU_LANG;
	s_lang_item[ITEM_MENU_LANG].itemType = ITEM_CHOICE;
	s_lang_item[ITEM_MENU_LANG].itemProperty.itemPropertyCmb.content= s_lang_array;//"[English,Chinese]";
	s_lang_item[ITEM_MENU_LANG].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.menu_lang;
	s_lang_item[ITEM_MENU_LANG].itemCallback.cmbCallback.CmbChange= app_menu_lang_change_callback;
	//#if MV_WIN_SUPPORT
	s_lang_item[ITEM_MENU_LANG].itemCallback.cmbCallback.CmbPress= app_menu_lang_cmb_press_callback;
	//#endif
	s_lang_item[ITEM_MENU_LANG].itemStatus = ITEM_NORMAL;
}

static void app_lang_setting_audio_1_item_init(void)
{
	s_lang_item[ITEM_AUDIO_1ST].itemTitle = STR_ID_AUDIO_LANG_1;
	s_lang_item[ITEM_AUDIO_1ST].itemType = ITEM_CHOICE;
	s_lang_item[ITEM_AUDIO_1ST].itemProperty.itemPropertyCmb.content= s_lang_array;
	s_lang_item[ITEM_AUDIO_1ST].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.audio_1;
	//#if MV_WIN_SUPPORT
	s_lang_item[ITEM_AUDIO_1ST].itemCallback.cmbCallback.CmbPress= app_lang_setting_audio_1_cmb_press_callback;
	//#endif
	s_lang_item[ITEM_AUDIO_1ST].itemCallback.cmbCallback.CmbChange= NULL;
	s_lang_item[ITEM_AUDIO_1ST].itemStatus = ITEM_NORMAL;
}

static void app_lang_setting_audio_2_item_init(void)
{
	s_lang_item[ITEM_AUDIO_2ND].itemTitle = STR_ID_AUDIO_LANG_2;
	s_lang_item[ITEM_AUDIO_2ND].itemType = ITEM_CHOICE;
	s_lang_item[ITEM_AUDIO_2ND].itemProperty.itemPropertyCmb.content= s_lang_array;
	s_lang_item[ITEM_AUDIO_2ND].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.audio_2;
	//#if MV_WIN_SUPPORT
	s_lang_item[ITEM_AUDIO_2ND].itemCallback.cmbCallback.CmbPress= app_lang_setting_audio_2_cmb_press_callback;
	//#endif
	s_lang_item[ITEM_AUDIO_2ND].itemCallback.cmbCallback.CmbChange= NULL;
	s_lang_item[ITEM_AUDIO_2ND].itemStatus = ITEM_NORMAL;
}

static void app_lang_setting_sub_lang_item_init(void)
{
	s_lang_item[ITEM_SUB_LANG].itemTitle = STR_ID_SUB_LANG;
	s_lang_item[ITEM_SUB_LANG].itemType = ITEM_CHOICE;
	s_lang_item[ITEM_SUB_LANG].itemProperty.itemPropertyCmb.content= s_lang_array;
	s_lang_item[ITEM_SUB_LANG].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.sub_1;
	s_lang_item[ITEM_SUB_LANG].itemCallback.cmbCallback.CmbChange= NULL;
	//#if MV_WIN_SUPPORT
	s_lang_item[ITEM_SUB_LANG].itemCallback.cmbCallback.CmbPress= app_lang_setting_sub_lang_cmb_press_callback;
	//#endif
	s_lang_item[ITEM_SUB_LANG].itemStatus = ITEM_NORMAL;
}

static void app_lang_setting_ttx_lang_item_init(void)
{
	s_lang_item[ITEM_TTX_LANG].itemTitle = STR_ID_TTX_LANG;
	s_lang_item[ITEM_TTX_LANG].itemType = ITEM_CHOICE;
	s_lang_item[ITEM_TTX_LANG].itemProperty.itemPropertyCmb.content= s_ttx_lang_array;
	s_lang_item[ITEM_TTX_LANG].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.ttx_lang;
	//#if MV_WIN_SUPPORT
	s_lang_item[ITEM_TTX_LANG].itemCallback.cmbCallback.CmbPress= app_lang_setting_ttx_cmb_press_callback;
	//#endif
	s_lang_item[ITEM_TTX_LANG].itemCallback.cmbCallback.CmbChange= NULL;
	s_lang_item[ITEM_TTX_LANG].itemStatus = ITEM_NORMAL;
}

static void app_lang_setting_epg_lang_item_init(void)
{
	s_lang_item[ITEM_EPG_LANG].itemTitle = STR_ID_EPG_LANG;
	s_lang_item[ITEM_EPG_LANG].itemType = ITEM_CHOICE;
	s_lang_item[ITEM_EPG_LANG].itemProperty.itemPropertyCmb.content= s_epg_lang_array;
	s_lang_item[ITEM_EPG_LANG].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.epg_lang;
	//#if MV_WIN_SUPPORT
	s_lang_item[ITEM_EPG_LANG].itemCallback.cmbCallback.CmbPress= app_lang_setting_epg_cmb_press_callback;
	//#endif
	s_lang_item[ITEM_EPG_LANG].itemCallback.cmbCallback.CmbChange= NULL;
	s_lang_item[ITEM_EPG_LANG].itemStatus = ITEM_NORMAL;
}

void app_lang_setting_menu_exec(void)
{
#define ALL_LANG_STR "All"
#define AUTO_LANG_STR "Auto"
	uint32_t i;

	memset(s_lang_array, 0, LANG_MAX * LANG_NAME_MAX);
	memset(s_epg_lang_array, 0, (LANG_MAX + 1) * LANG_NAME_MAX);
	memset(s_ttx_lang_array, 0, (LANG_MAX + 1) * LANG_NAME_MAX);
	strcpy(s_lang_array, "[");
	strcpy(s_epg_lang_array, "[");
	strcat(s_epg_lang_array, ALL_LANG_STR);
	strcpy(s_ttx_lang_array, "[");
    strcat(s_ttx_lang_array, AUTO_LANG_STR);
	for (i=0; i<LANG_MAX; i++)
	{
		strcat(s_lang_array, g_LangName[i]);
		strcat(s_lang_array, ",");
		
		strcat(s_epg_lang_array, ",");
		strcat(s_epg_lang_array, g_LangName[i]);

		strcat(s_ttx_lang_array, ",");
        strcat(s_ttx_lang_array, g_LangName[i]);
	}
	strcat(s_lang_array, "]");
	strcat(s_epg_lang_array, "]");
	strcat(s_ttx_lang_array, "]");
    
	memset(&s_lang_setting_opt, 0 ,sizeof(s_lang_setting_opt));
	
	app_lang_system_para_get();

	s_lang_setting_opt.menuTitle = STR_ID_LANG;
	s_lang_setting_opt.itemNum = ITEM_LANG_TOTAL;
	s_lang_setting_opt.timeDisplay = TIP_HIDE;
	s_lang_setting_opt.item = s_lang_item;
	
	app_lang_setting_menu_lang_item_init();
	app_lang_setting_audio_1_item_init();
	app_lang_setting_audio_2_item_init();
	app_lang_setting_sub_lang_item_init();
	app_lang_setting_ttx_lang_item_init();
	app_lang_setting_epg_lang_item_init();

	s_lang_setting_opt.exit = app_lang_setting_exit_callback;

	app_system_set_create(&s_lang_setting_opt);
}

int app_lang_get_menu_lang_status(void)
{
	int init_value = 0;
	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
    //	s_sys_lang_para.menu_lang= init_value;
	return init_value;
}

int app_lang_set_menu_font_type(const char *widget_name)
{	
#if (MINI_256_COLORS_OSD_SUPPORT||MINI_16_BITS_OSD_SUPPORT)
			 
	 if(strcmp(g_LangName[app_lang_get_menu_lang_status()],"Thailand")==0)
	 {
		 GUI_SetProperty(widget_name, "font", "thai_main");
	 }
	 else
	 {
		 GUI_SetProperty(widget_name, "font", "font_main");
	 }
#endif
	return 0;
}

/*int app_tolower(int ch)
{
	return (ch >= 'A' && ch <= 'Z') ? ch - 'A' + 'a' : ch;
}*/

#ifdef AUTO_CHANGE_FIRST_AUDIO_SUPPORT
extern int app_get_muli_audio_pid(int index,int *count);
extern char *app_pmt_get_audio_lang_str(uint32_t index);
extern int app_pmt_get_muli_audio_type(int index);
int  app_lang_check_audio_lang(uint16_t* audio_pid,
                                             GxBusPmDataProgAudioType* type,GxMsgProperty_NodeByPosGet* node)
{
    uint32_t i = 0;
    uint16_t select_auio_pid = 0;
    int8_t lang_buff[60];
    int8_t lang[3][4];
    int8_t flag = -1;//-1代表还没有赋值，0代表没有找到优先语言，1代表找到了最优语言，2代表着找到了第二优语言
    int8_t* p = 0;
    uint8_t count = 0;
	int audio_count = 0;

    memset(lang,0,3*4);
    if(NULL ==  GxBus_ConfigGet(PRIORITY_LANG_KEY,(char*)lang_buff,60,PRIORITY_LANG_VALUE))
    {
       strcpy((char*)lang_buff,PRIORITY_LANG_VALUE);
    }
   
    for(p=lang_buff; ;)
    {
        if(*p!='\0'&&count!=2 )
        {
            memcpy(lang[count],p,3);
            count++;
            p+=3;
        }
        if(*p!='\0'&&count!=2 )
        {
            p++;
        }
        else
        {
            break;
        }
    }
	
	app_get_muli_audio_pid(0,&audio_count);
	select_auio_pid = node->prog_data.cur_audio_pid;
	*audio_pid = node->prog_data.cur_audio_pid;
	*type = node->prog_data.cur_audio_type;

	for(i=0; i<audio_count; i++)
	{
		if(app_pmt_get_muli_audio_type(i)<= GXBUS_PM_AUDIO_DTS_HD_SEC)
		{  
            if(-1 == flag)
            {
                *audio_pid = node->prog_data.cur_audio_pid;
                *type = node->prog_data.cur_audio_type;
                flag = 0;
            }
        
            if(0 == strcmp((const char*)(app_pmt_get_audio_lang_str(i)),(const char*)(lang[0])))
            {
                *audio_pid = app_get_muli_audio_pid(i,&audio_count);
                *type = app_pmt_get_muli_audio_type(i);
                flag = 1;
                break;
            }
            else if(0 == strcmp((const char*)(app_pmt_get_audio_lang_str(i)),(const char*)(lang[1])))
            {
                *audio_pid = app_get_muli_audio_pid(i,&audio_count);
                *type = app_pmt_get_muli_audio_type(i); 
                flag = 2;
            }
		}
	}



	if((select_auio_pid != *audio_pid)&&(*audio_pid!=0))
	{
		
#if 1
	{
		GxMsgProperty_NodeModify node_modify = {0};
		node_modify.node_type = NODE_PROG;
		memcpy(&node_modify.prog_data, &node->prog_data, sizeof(GxBusPmDataProg));
		node_modify.prog_data.cur_audio_pid= *audio_pid;
		node_modify.prog_data.cur_audio_type= *type;
		

		if(memcmp(&node->prog_data, &node_modify.prog_data, sizeof(GxBusPmDataProg)) != 0)
		{
			if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
			{
				printf("\n-------modify program error\n");
			}
		}
	}
#endif                        
	}
	
	return 0;
}

int app_set_first_audio_lang(void)
{
	GxMsgProperty_PlayerAudioSwitch audioSwitch = {0};
	GxBusPmDataProgAudioType type = 0xff;
	uint16_t cur_audio_pid = 0;
	

	GxMsgProperty_NodeByPosGet prog_node = {0};
	prog_node.node_type = NODE_PROG;
	prog_node.pos = g_AppPlayOps.normal_play.play_count;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node)))
	{
		app_lang_check_audio_lang(&cur_audio_pid,&type,&prog_node);
		audioSwitch.player = PLAYER_FOR_NORMAL;
		audioSwitch.pid = cur_audio_pid;
		audioSwitch.type = type;
		app_send_msg_exec(GXMSG_PLAYER_AUDIO_SWITCH, &audioSwitch);
	}
		
	return 0;
}
#endif

