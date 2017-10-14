#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_utility.h"

#define STR_ID_SECTION_RECORD "Section Record"

enum TIME_AV_NAME 
{
	ITEM_TMS_FLAG = 0,
	ITEM_FILE_SIZE,
	ITEM_DURATION,
	ITEM_SECTION_RECORD,
	ITEM_DISK_INFO,
    ITEM_PVR_TOTAL
};

typedef enum
{
	TMS_OFF,
	TMS_ON
}PvrTmsFlag;

typedef enum
{
	SRECORD_OFF,
	SRECORD_ON
}PvrSectionFlag;

typedef enum
{
	FILE_SIZE_512M  =   512,
	FILE_SIZE_1G    =   1024,
	FILE_SIZE_2G    =   2048,
	FILE_SIZE_4G    =   4096,
}PvrFileSize;

typedef struct
{
	PvrTmsFlag      tms_flag;
    PvrFileSize     file_size;
    uint32_t        duration;   // seconds
	PvrSectionFlag  section_flag;
}PvrSetPara;

static SystemSettingOpt s_PvrSetOpt;
static SystemSettingItem s_PvrItem[ITEM_PVR_TOTAL];
static PvrSetPara s_PvrSetPara;
static  PopList pop_list;

static void app_pvr_set_result_para_get(PvrSetPara *ret_para)
{
    uint32_t sel_to_size[4] = {FILE_SIZE_512M,FILE_SIZE_1G,FILE_SIZE_2G,FILE_SIZE_4G};
	ret_para->tms_flag  = s_PvrItem[ITEM_TMS_FLAG].itemProperty.itemPropertyCmb.sel;
	ret_para->file_size = sel_to_size[s_PvrItem[ITEM_FILE_SIZE].itemProperty.itemPropertyCmb.sel];
    ret_para->duration = s_PvrItem[ITEM_DURATION].itemProperty.itemPropertyCmb.sel;
	ret_para->section_flag = s_PvrItem[ITEM_SECTION_RECORD].itemProperty.itemPropertyCmb.sel;
    
}

static bool app_pvr_set_para_change(PvrSetPara *ret_para)
{
	bool ret = FALSE;
	
	if(s_PvrSetPara.tms_flag != ret_para->tms_flag)
	{
		ret = TRUE;
	}

	if(s_PvrSetPara.file_size != ret_para->file_size)
	{
		ret = TRUE;
	}

	if(s_PvrSetPara.duration != ret_para->duration)
	{
		ret = TRUE;
	}
	if(s_PvrSetPara.section_flag != ret_para->section_flag)
	{
		ret = TRUE;
	}
	
	return ret;
}

static int app_pvr_set_tms_flag(int32_t tms)
{
    GxBus_ConfigSetInt(PVR_TIMESHIFT_KEY, tms);
    return 0;
}

static int app_pvr_set_file_size(int32_t size)
{
    GxMsgProperty_PlayerPVRConfig  pvr_cfg = {0};

    GxBus_ConfigSetInt(PVR_FILE_SIZE_KEY, size);

    pvr_cfg.volume_sizemb = size;

    app_send_msg_exec(GXMSG_PLAYER_PVR_CONFIG,&pvr_cfg);

	return 0;
}

static int app_pvr_set_duration_flag(uint32_t duration)
{
    GxBus_ConfigSetInt(PVR_DURATION_KEY, duration);

    // TODO: re-calculate pvr stop time

    return 0;
}

static int app_pvr_set_section_flag(int32_t section)
{
    GxBus_ConfigSetInt(PVR_SECTIONRECORD_KEY, section);
    return 0;
}

static int app_pvr_set_para_save(PvrSetPara *ret_para)
{
    app_pvr_set_tms_flag(ret_para->tms_flag);
    app_pvr_set_file_size(ret_para->file_size);
    app_pvr_set_duration_flag(ret_para->duration);
	app_pvr_set_section_flag(ret_para->section_flag);
    return 0;
}

static int app_pvr_disk_info_press_callback(unsigned short key)
{
	if(key == STBK_OK)
	{
#if 0
        if(FALSE == check_usb_status())
        {
			PopDlg  pop;
			memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_NO_DEVICE;
            pop.mode = POP_MODE_UNBLOCK;
            popdlg_create(&pop);

            return EVENT_TRANSFER_STOP;
        }
#endif
		GUI_CreateDialog("wnd_setting_dm");
	}
	return EVENT_TRANSFER_KEEPON;
}

static int app_pvr_set_tms_press_callback(int key)
{
    static char* s_TmsData[]= {"Off","On"};
	int cmb_sel =-1;

	if((key == STBK_OK) || (key == STBK_RIGHT))
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_TMS;
		pop_list.item_num = sizeof(s_TmsData)/sizeof(*s_TmsData);
		pop_list.item_content = s_TmsData;
		pop_list.sel = s_PvrItem[ITEM_TMS_FLAG].itemProperty.itemPropertyCmb.sel;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_2;
        pop_list.pos.x= 630;
        pop_list.pos.y= 136;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt1", "select", &cmb_sel);
	}

    if((key == STBK_LEFT) || (key == STBK_RIGHT))
        return EVENT_TRANSFER_STOP;

	return EVENT_TRANSFER_KEEPON;
}

static int app_pvr_set_file_press_callback(int key)
{
    static char* s_FileData[]= {"512M","1G","2G","4G"};
	int cmb_sel =-1;

	if((key == STBK_OK) || (key == STBK_RIGHT))
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_TS_FILE_SIZE;
		pop_list.item_num = sizeof(s_FileData)/sizeof(*s_FileData);
		pop_list.item_content = s_FileData;
		pop_list.sel = s_PvrItem[ITEM_FILE_SIZE].itemProperty.itemPropertyCmb.sel;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_3;
        pop_list.pos.x= 630;
        pop_list.pos.y= 167;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt2", "select", &cmb_sel);
	}

    if((key == STBK_LEFT) || (key == STBK_RIGHT))
        return EVENT_TRANSFER_STOP;

	return EVENT_TRANSFER_KEEPON;
}

static int app_pvr_set_duration_press_callback(int key)
{
    static char* s_DurData[]= {"2h00min","2h30min","3h00min",\
"3h30min","4h00min","4h30min","5h00min","5h30min","6h00min","6h30min","7h00min",\
"7h30min","8h00min","8h30min","9h00min","9h30min","10h00min","10h30min","11h00min",\
"11h30min","12h00min"};
	int cmb_sel =-1;

	if((key == STBK_OK) || (key == STBK_RIGHT))
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_REC_DURA;
		pop_list.item_num = sizeof(s_DurData)/sizeof(*s_DurData);
		pop_list.item_content = s_DurData;
		pop_list.sel = s_PvrItem[ITEM_DURATION].itemProperty.itemPropertyCmb.sel;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_5;
        pop_list.pos.x= 630;
        pop_list.pos.y= 198;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt3", "select", &cmb_sel);
	}

    if((key == STBK_LEFT) || (key == STBK_RIGHT))
        return EVENT_TRANSFER_STOP;

	return EVENT_TRANSFER_KEEPON;
}

static int app_pvr_set_section_press_callback(int key)
{
    static char* s_SectionData[]= {"Off","On"};
	int cmb_sel =-1;

	if((key == STBK_OK) || (key == STBK_RIGHT))
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_SECTION_RECORD;
		pop_list.item_num = sizeof(s_SectionData)/sizeof(*s_SectionData);
		pop_list.item_content = s_SectionData;
		pop_list.sel = s_PvrItem[ITEM_SECTION_RECORD].itemProperty.itemPropertyCmb.sel;
		pop_list.show_num= false;
        pop_list.line_num= POPLIST_LINE_2;
        pop_list.pos.x= 630;
        pop_list.pos.y= 229;

		cmb_sel = poplist_create(&pop_list);
		GUI_SetProperty("cmb_system_setting_opt4", "select", &cmb_sel);
	}

    if((key == STBK_LEFT) || (key == STBK_RIGHT))
        return EVENT_TRANSFER_STOP;

	return EVENT_TRANSFER_KEEPON;
}

static void app_pvr_set_tms_flag_item_init(void)
{
    int32_t tms_flag=0;

	s_PvrItem[ITEM_TMS_FLAG].itemTitle = STR_ID_TMS;
	s_PvrItem[ITEM_TMS_FLAG].itemType = ITEM_CHOICE;
	s_PvrItem[ITEM_TMS_FLAG].itemProperty.itemPropertyCmb.content="[Off,On]";

	GxBus_ConfigGetInt(PVR_TIMESHIFT_KEY, &tms_flag, PVR_TIMESHIFT_FLAG);
	s_PvrItem[ITEM_TMS_FLAG].itemProperty.itemPropertyCmb.sel = tms_flag;
	s_PvrItem[ITEM_TMS_FLAG].itemCallback.cmbCallback.CmbChange= NULL;
	s_PvrItem[ITEM_TMS_FLAG].itemCallback.cmbCallback.CmbPress = app_pvr_set_tms_press_callback;
	s_PvrItem[ITEM_TMS_FLAG].itemStatus = ITEM_NORMAL;

	s_PvrSetPara.tms_flag = tms_flag;
}

static void app_pvr_set_file_size_item_init(void)
{
    int32_t file_size=0;
    int32_t size_to_sel[] = {0,1,2,0,3};
    //int32_t size_to_sel[] = {0,1,0,2};

	s_PvrItem[ITEM_FILE_SIZE].itemTitle = STR_ID_TS_FILE_SIZE;
	s_PvrItem[ITEM_FILE_SIZE].itemType = ITEM_CHOICE;
	s_PvrItem[ITEM_FILE_SIZE].itemProperty.itemPropertyCmb.content= "[512M,1G,2G,4G]";

	GxBus_ConfigGetInt(PVR_FILE_SIZE_KEY, &file_size, PVR_FILE_SIZE_VALUE);
	s_PvrItem[ITEM_FILE_SIZE].itemProperty.itemPropertyCmb.sel = size_to_sel[file_size/1024];
	//s_PvrItem[ITEM_FILE_SIZE].itemProperty.itemPropertyCmb.sel = size_to_sel[file_size/1025];
	s_PvrItem[ITEM_FILE_SIZE].itemCallback.cmbCallback.CmbChange= NULL;
	s_PvrItem[ITEM_FILE_SIZE].itemCallback.cmbCallback.CmbPress = app_pvr_set_file_press_callback;
	s_PvrItem[ITEM_FILE_SIZE].itemStatus = ITEM_NORMAL;

	s_PvrSetPara.file_size = file_size;
}

static void app_pvr_set_duration_item_init(void)
{
#define SHOW_TIME_LEN   (8)
    int32_t duration =0;

	s_PvrItem[ITEM_DURATION].itemTitle = STR_ID_REC_DURA;
	s_PvrItem[ITEM_DURATION].itemType = ITEM_CHOICE;
	s_PvrItem[ITEM_DURATION].itemProperty.itemPropertyCmb.content= "[2h00min,2h30min,3h00min,\
3h30min,4h00min,4h30min,5h00min,5h30min,6h00min,6h30min,7h00min,\
7h30min,8h00min,8h30min,9h00min,9h30min,10h00min,10h30min,11h00min,\
11h30min,12h00min]";

	GxBus_ConfigGetInt(PVR_DURATION_KEY, &duration, PVR_DURATION_VALUE);
	s_PvrItem[ITEM_DURATION].itemProperty.itemPropertyCmb.sel = duration;
	s_PvrItem[ITEM_DURATION].itemCallback.cmbCallback.CmbChange= NULL;
	s_PvrItem[ITEM_DURATION].itemCallback.cmbCallback.CmbPress = app_pvr_set_duration_press_callback;
	s_PvrItem[ITEM_DURATION].itemStatus = ITEM_NORMAL;

	s_PvrSetPara.duration = (uint32_t)duration;
}

static void app_pvr_disk_info_item_init(void)
{
	s_PvrItem[ITEM_DISK_INFO].itemTitle = STR_ID_DISK_INFO;
	s_PvrItem[ITEM_DISK_INFO].itemType = ITEM_PUSH;
	s_PvrItem[ITEM_DISK_INFO].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	s_PvrItem[ITEM_DISK_INFO].itemCallback.btnCallback.BtnPress= app_pvr_disk_info_press_callback;
	s_PvrItem[ITEM_DISK_INFO].itemStatus = ITEM_NORMAL;
}

static void app_pvr_section_record_init(void)
{
	int32_t section_flag=0;
	s_PvrItem[ITEM_SECTION_RECORD].itemTitle = STR_ID_SECTION_RECORD;
	s_PvrItem[ITEM_SECTION_RECORD].itemType = ITEM_CHOICE;
	s_PvrItem[ITEM_SECTION_RECORD].itemProperty.itemPropertyCmb.content="[Off,On]";
	GxBus_ConfigGetInt(PVR_SECTIONRECORD_KEY, &section_flag, PVR_SECTIONRECORD_FLAG);
	s_PvrItem[ITEM_SECTION_RECORD].itemProperty.itemPropertyCmb.sel = section_flag;
	s_PvrItem[ITEM_SECTION_RECORD].itemCallback.cmbCallback.CmbChange= NULL;
	s_PvrItem[ITEM_SECTION_RECORD].itemCallback.cmbCallback.CmbPress = app_pvr_set_section_press_callback;
	s_PvrItem[ITEM_SECTION_RECORD].itemStatus = ITEM_NORMAL;
	s_PvrSetPara.section_flag = section_flag;
}


static int app_pvr_set_exit_callback(ExitType exit_type)
{
    PvrSetPara para_ret;
    int ret = 0;

    app_pvr_set_result_para_get(&para_ret);
    if(app_pvr_set_para_change(&para_ret) == TRUE)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_SAVE_INFO;
        if(popdlg_create(&pop) == POP_VAL_OK)
        {
            app_pvr_set_para_save(&para_ret);
            ret =1;
        }
    }
    return ret;
}

void app_pvr_set_menu_exec(void)
{
	memset(&s_PvrSetOpt, 0 ,sizeof(SystemSettingOpt));
	s_PvrSetOpt.menuTitle = STR_ID_PVR_MANAGE;
#if( MINI_16_BITS_OSD_SUPPORT)
	s_PvrSetOpt.titleImage = "s_menu_icon_media3.bmp";
#else
	s_PvrSetOpt.titleImage = "s_title_left_media.bmp";
#endif
	s_PvrSetOpt.itemNum = ITEM_PVR_TOTAL;
	s_PvrSetOpt.timeDisplay = TIP_HIDE;
	s_PvrSetOpt.item = s_PvrItem;

	app_pvr_set_tms_flag_item_init();
	app_pvr_set_file_size_item_init();
    app_pvr_set_duration_item_init();
	app_pvr_disk_info_item_init();
	app_pvr_section_record_init();

	s_PvrSetOpt.exit = app_pvr_set_exit_callback;

	app_system_set_create(&s_PvrSetOpt);
}
