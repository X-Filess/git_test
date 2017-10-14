#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_utility.h"
#include "play_manage.h"
#include "full_screen.h"

#define WND_PVR_MEIDA "wnd_pvr_media_list"
#define LISTVIEW_PVR_MEIDA      "listview_pvr_list"
#define TXT_PVR_MEDIA_INFO_1 "text_pvr_midia_info_1"
#define TXT_PVR_MEDIA_INFO_2 "text_pvr_midia_info_2"
#define TXT_PVR_MEDIA_INFO_3 "text_pvr_midia_info_3"

#define PVR_PATH_LEN (11) // "/dev/usbxx" or "/dev/sdxx"
#define PVR_DIR_STR "/SunPvr"
#define PVR_INDEX_SUFFIX "dvr;DVR"
#define PVR_MEDIE_SUFFIX "ts;TS"
#define PVR_FIX_SIFFIX_LEN  (7) //.ts.dvr

#define IMG_DEL_TIP "img_pvr_media_tip_red"
#define TEXT_DEL_TIP "text_pvr_media_tip_red"
#define IMG_DEL_ALL_TIP "img_pvr_media_tip_blue"
#define TEXT_DEL_ALL_TIP "text_pvr_media_tip_blue"


typedef struct
{
    uint64_t size;
    time_t time;
}PvrFileInfo;

typedef struct
{
	char			path[500];
	char			dev[16];
	int			nents;
	GxDirent*	ents;
}DirPara;

static struct
{
    int total;
    int *record;
}s_pvr_media_map = {0};

HotplugPartition s_pvr_partition;

static DirPara s_pvr_dir;
static bool s_pvr_on_fullscreen = false;
static char s_pvr_dev_name[32] = {0};

extern void app_free_dir_ent(GxDirent **ents, int *nents);
/////free result extern/////
static char* _get_dvr_dir_path(int index)
{
    char *dir_path = NULL;
    int dir_len=1;
    int file_len;

    if((s_pvr_media_map.total == 0) || (s_pvr_media_map.record == NULL))
    {
        return NULL;
    }

    file_len = strlen(s_pvr_dir.ents[s_pvr_media_map.record[index]].fname);
    dir_len = file_len - PVR_FIX_SIFFIX_LEN;//xxx.ts.dvr
    dir_len += 1; // '/'
    dir_len += strlen(s_pvr_dir.path);
    dir_len += 1; // '\0'

    dir_path = (char*)GxCore_Malloc(dir_len * sizeof(char));
    if(dir_path != NULL)
    {
        memset(dir_path, 0, dir_len);
        strcpy(dir_path, s_pvr_dir.path);
        strcat(dir_path, "/");
        strncat(dir_path, s_pvr_dir.ents[s_pvr_media_map.record[index]].fname, file_len - PVR_FIX_SIFFIX_LEN);
    }
    return dir_path;
}

static status_t _get_pvr_media_info(int index, PvrFileInfo *Info)
{
    char *dvr_dir_path = NULL;
    DirPara dvr_dir_para;
    int i;
    status_t ret = GXCORE_ERROR;

    if((s_pvr_media_map.total == 0) || (s_pvr_media_map.record == NULL))
    {
        return GXCORE_ERROR;
    }

    dvr_dir_path = _get_dvr_dir_path(index);
    if(dvr_dir_path != NULL)
    {
        memset(&dvr_dir_para, 0, sizeof(dvr_dir_para));
        dvr_dir_para.nents  = GxCore_GetDir(dvr_dir_path, &(dvr_dir_para.ents), PVR_MEDIE_SUFFIX);
        GxCore_Free(dvr_dir_path);
        dvr_dir_path = NULL;

        if(dvr_dir_para.nents > 0)
        {
            Info->size = 0;
            for(i = 0; i < dvr_dir_para.nents; i++)
            {
                Info->size += dvr_dir_para.ents[i].fsize;
            }
            Info->time = s_pvr_dir.ents[s_pvr_media_map.record[index]].fmtime;
            ret = GXCORE_SUCCESS;
        }
        app_free_dir_ent(&(dvr_dir_para.ents), &(dvr_dir_para.nents));
    }

    return ret;
}

static void _pvr_media_display_size_str(uint64_t size, char* str)
{
    double display_size = 0.00;
    
    if(size >= (1024*1024*1024))
    {
        display_size = (double)size / (1024*1024*1024);
        sprintf(str, "%3.2f GB", display_size);
    }
    else if(size >= (1024*1024))
    {
        display_size = (double)size / (1024*1024);
        sprintf(str, "%3.2f MB", display_size);
    }
    else if(size >= (1024))
    {
        display_size = (double)size / (1024);
        sprintf(str, "%3.2f KB", display_size);
    }
    else
    {
        display_size = (double)size;
        sprintf(str, "%3.2f B", display_size);
    }
}

static void display_pvr_media_info(PvrFileInfo *Info)
{
    char str[50] = {0};
    char *tm_str = NULL;
    time_t time_sec = 0;

    memset(str, 0, sizeof(str));
    strcpy(str, s_pvr_partition.partition_name);
    strcat(str, " ");
    strcat(str, PVR_DIR_STR);
    GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", str);

    memset(str, 0, sizeof(str));
    _pvr_media_display_size_str(Info->size, str);
    GUI_SetProperty(TXT_PVR_MEDIA_INFO_2, "string", str);

    memset(str, 0, sizeof(str));
    time_sec = get_display_time_by_timezone(Info->time);
    tm_str = app_time_to_format_str(time_sec);
    if(tm_str != NULL)
    {
        strcpy(str, tm_str);
        GUI_SetProperty(TXT_PVR_MEDIA_INFO_3, "string", str);
        GxCore_Free(tm_str);
    }
}

#ifdef FILE_EDIT_VALID
static status_t _delete_pvr_media(int index)
{
    status_t ret = GXCORE_SUCCESS;
    int i = 0;
    char *path = NULL;
    int path_len = 0;
    DirPara dvr_dir_para;

    if((s_pvr_media_map.total == 0) || (s_pvr_media_map.record == NULL))
    {
        return GXCORE_ERROR;
    }
    
//////delete xxx.ts.dvr///////
    path_len = strlen(s_pvr_dir.path);
    path_len += 1;// '/'
    path_len += strlen(s_pvr_dir.ents[s_pvr_media_map.record[index]].fname);
    path_len += 1; // '\0'

    path = (char*)GxCore_Malloc(path_len);
    if(path != NULL)
    {
        memset(path, 0, path_len);
        strcpy(path, s_pvr_dir.path);
        strcat(path, "/");
        strcat(path, s_pvr_dir.ents[s_pvr_media_map.record[index]].fname);
        ret = GxCore_FileDelete(path);
        GxCore_Free(path);
        path = NULL;
    }

    path = _get_dvr_dir_path(index);
    if(path != NULL)
    {
        memset(&dvr_dir_para, 0, sizeof(dvr_dir_para));
        dvr_dir_para.nents  = GxCore_GetDir(path, &(dvr_dir_para.ents), NULL);

        path_len = strlen(path);
        if( dvr_dir_para.nents > 0)
        {
            char* sub_path = NULL;
            int sub_path_len = 0;
            for(i = 0; i < dvr_dir_para.nents; i++)
            {
                sub_path_len = path_len;
                sub_path_len += 1; // '/'
                sub_path_len += strlen(dvr_dir_para.ents[i].fname);
                sub_path_len += 1; // '\0'

                sub_path = (char*)GxCore_Malloc(sub_path_len);
                if(sub_path != NULL)
                {
                    memset(sub_path, 0, sub_path_len);
                    strcpy(sub_path, path);
                    strcat(sub_path, "/");
                    strcat(sub_path, dvr_dir_para.ents[i].fname);
                    ret = GxCore_FileDelete(sub_path);
                    GxCore_Free(sub_path);
                    sub_path = NULL;
                }
            }
        }
        app_free_dir_ent(&(dvr_dir_para.ents), &(dvr_dir_para.nents));
		if(GxCore_FileExists(path) == GXCORE_FILE_UNEXIST)
		{
			ret = GXCORE_SUCCESS;
		}
		else
		{
			ret = GxCore_Rmdir(path);
		}
        GxCore_Free(path);
        path = NULL;
    }

    return ret;
}
#endif

static void _play_pvr_media(int index)
{
    explorer_para explorer;

    if((s_pvr_media_map.total == 0) || (s_pvr_media_map.record == NULL))
        return;

    memset(&explorer, 0, sizeof(explorer_para));
    explorer.path = s_pvr_dir.path;
    explorer.suffix = PVR_INDEX_SUFFIX;
    explorer.nents = s_pvr_dir.nents;
    explorer.ents = s_pvr_dir.ents;

    play_list_init(PLAY_LIST_TYPE_MOVIE, &explorer, s_pvr_media_map.record[index]);
    GUI_CreateDialog("win_movie_view");
}

static status_t app_pvr_meida_subsuffix_match(char *path)
{ // match ".ts.dvr"
    int i=0, point=0, len=0;
    int lastPoint[2] = {0};
    char *subSuffix = NULL;

    if(path == NULL)
        return GXCORE_ERROR;

    len = strlen(path);
    if(len < PVR_FIX_SIFFIX_LEN)
        return GXCORE_ERROR;

    for(i=(len-1); i>=0; i--)
    {
        if('.' == path[i])
        {
            lastPoint[point] = i;
            point ++;
            if(point == 2)
                break;
        }
    }

    len = lastPoint[0]-lastPoint[1] -1;
    subSuffix = GxCore_Malloc((len+1) * sizeof(char));
    memset(subSuffix, 0, (len+1));
    strncpy(subSuffix, (path+lastPoint[1]+1), len);

    if((strcmp("TS", subSuffix) == 0) || (strcmp("ts", subSuffix) == 0))
        return GXCORE_SUCCESS;

    return GXCORE_ERROR;
}

static status_t app_pvr_media_update(void)
{
    char partition[PVR_PATH_LEN + 1] = {0};
    int i = 0;
    int num = 0;
    uint32_t dev_len;
    char *p = NULL;
    HotplugPartitionList *partition_list = NULL;
    status_t ret = GXCORE_ERROR;

    memset(&s_pvr_media_map, 0, sizeof(s_pvr_media_map));
    memset(&s_pvr_partition, 0, sizeof(HotplugPartition));

    g_AppPvrOps.tms_delete(&g_AppPvrOps);
    partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if((NULL != partition_list) && (0 < partition_list->partition_num))
    {
        GxBus_ConfigGet(PVR_PARTITION_KEY, partition, PVR_PATH_LEN, PVR_PARTITION);
        for(i = 0; i < partition_list->partition_num; i++)
        {
            if(strcmp(partition, partition_list->partition[i].dev_name) == 0)
            {
                break;
            }
        }
        if(i >= partition_list->partition_num)
        {
            i = 0;
        }
        memcpy(&s_pvr_partition,  &partition_list->partition[i], sizeof(HotplugPartition));

        memset(s_pvr_dev_name, 0, sizeof(s_pvr_dev_name));
        strcpy(s_pvr_dev_name, s_pvr_partition.dev_name);

        dev_len = strlen(s_pvr_dev_name);
        p = s_pvr_dev_name + dev_len - 1;

        if(*p >= '0' && *p <= '9') //remove partition num
            *p = '\0';

        app_free_dir_ent(&(s_pvr_dir.ents), &(s_pvr_dir.nents));
        memset(&s_pvr_dir, 0, sizeof(s_pvr_dir));

		char dir_name[10] = {0};
#ifdef LINUX_OS
		//获取真实的GxPvr目录的名字（主要解决存在的目录名字大小写的差异问题）
		extern status_t _get_pvr_dir_name(const char *path);
		if(_get_pvr_dir_name(s_pvr_partition.partition_entry) == GXCORE_ERROR) {
			strcpy(dir_name, PVR_DIR_STR);
		}else{
			GxBus_ConfigGet(PVR_TRUE_DIR_NAME_KEY, dir_name, 6, PVR_TRUE_DIR_NAME);
		}
#else
		strcpy(dir_name, PVR_DIR_STR);
#endif
        strcpy(s_pvr_dir.path, s_pvr_partition.partition_entry);
        strcat(s_pvr_dir.path, dir_name);

        s_pvr_dir.nents = GxCore_GetDir(s_pvr_dir.path, &(s_pvr_dir.ents), PVR_INDEX_SUFFIX);
        if(s_pvr_dir.nents > 0)
        {
            GxCore_SortDir(s_pvr_dir.ents, s_pvr_dir.nents, NULL);

            for(i = 0; i < s_pvr_dir.nents; i++)
            {
                if(s_pvr_dir.ents[i].ftype == GX_FILE_REGULAR)
                {
                    if(app_pvr_meida_subsuffix_match(s_pvr_dir.ents[i].fname) == GXCORE_SUCCESS)
                    {
                        s_pvr_media_map.total++;
                    }
                }
            }

            if(s_pvr_media_map.total > 0)
            {
                s_pvr_media_map.record = (int*)GxCore_Malloc(s_pvr_media_map.total * sizeof(int));
                if(NULL !=  s_pvr_media_map.record)
                {
                    num = 0;
                    for(i = 0; i < s_pvr_dir.nents; i++)
                    {
                        if(s_pvr_dir.ents[i].ftype== GX_FILE_REGULAR)
                        {
                            if(app_pvr_meida_subsuffix_match(s_pvr_dir.ents[i].fname) == GXCORE_SUCCESS)
                                s_pvr_media_map.record[num++] = i;
                        }
                    }
                }
            }
        }
        else
        {
            app_free_dir_ent(&(s_pvr_dir.ents), &(s_pvr_dir.nents));
        }

        ret = GXCORE_SUCCESS;
    }

    return ret;
}

void app_create_pvr_media_menu(void)
{
    if(app_pvr_media_update() == GXCORE_SUCCESS)
    {
        GUI_CreateDialog(WND_PVR_MEIDA);
        s_pvr_on_fullscreen = false;
    }
    else
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_INSERT_USB;
        pop.mode = POP_MODE_UNBLOCK;
        popdlg_create(&pop);
    }
}

void app_create_pvr_media_fullsreeen(void)
{
     PopDlg pop = {0};

    if (g_AppPvrOps.state != PVR_DUMMY)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_STOP_PVR_FIRST;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec = 3;
        popdlg_create(&pop);    
    }
    else
    {
        if(app_pvr_media_update() == GXCORE_SUCCESS)
        {
			g_AppFullArb.timer_stop();
            GUI_SetProperty("txt_full_state", "state", "hide");
            GUI_SetProperty("img_full_state", "state", "hide");
            GUI_SetProperty("txt_full_title", "state", "hide");
			if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
				GUI_EndDialog("wnd_channel_info");
            g_AppFullArb.state.pause = STATE_OFF;
            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
            g_AppPlayOps.program_stop();
            GUI_CreateDialog(WND_PVR_MEIDA);
            s_pvr_on_fullscreen = true;
        }
        else
        {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_INSERT_USB;
            pop.mode = POP_MODE_UNBLOCK;
            popdlg_create(&pop);
        }
    }
}

void app_pvr_media_quit(void)
{
	if(s_pvr_on_fullscreen == true)
	{
	    GUI_EndDialog("after wnd_full_screen");
	    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
	    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
	    GUI_SetProperty("wnd_full_screen","draw_now",NULL);

	    if (g_AppPlayOps.normal_play.play_total != 0)
	    {
	        g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
	        //GUI_CreateDialog("wnd_channel_info");
	    }
	    s_pvr_on_fullscreen = false;
	}
	else
	{
#if MINI_16BIT_WIN8_OSD_SUPPORT
		//GUI_EndDialog("after wnd_main_menu");
	   GUI_EndDialog("after win_media_centre");
#else
        GUI_EndDialog("after wnd_main_menu");

#if MV_WIN_SUPPORT
	    //extern void app_main_item_menu_create(int value);	
        int sel = 0;
#ifdef ECOS_OS
	#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
		sel = 4;
	#else
		sel = 2;
	#endif
#else
		sel = 1;
#endif
	    //app_main_item_menu_create(sel);
#endif
#endif
	}
}

status_t app_pvr_media_hotplug(char *hotplug_dev)
{
    if((s_pvr_dev_name != NULL)
        && (hotplug_dev != NULL)
        && strncmp(s_pvr_dev_name, hotplug_dev,strlen(hotplug_dev)) == 0)
    {
        if(GUI_CheckDialog("win_movie_view") == GXCORE_SUCCESS) //play with pvr media list
        {
            GxMsgProperty_PlayerStop player_stop;
            player_stop.player = PMP_PLAYER_AV;
            app_send_msg_exec(GXMSG_PLAYER_STOP, &player_stop);
        }
        app_pvr_media_quit();
    }
    
    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int app_pvr_media_create(GuiWidget *widget, void *usrdata)
{
    int sel = 0;
    PvrFileInfo file_info;
    int32_t init_value = 0;

    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);
    app_lang_set_menu_font_type("text_pvr_meida_title");
	
    if(s_pvr_media_map.total > 0)
    {
        GUI_SetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
        memset(&file_info, 0, sizeof(PvrFileInfo));
        if( _get_pvr_media_info(sel, &file_info) == GXCORE_SUCCESS)
        {
            display_pvr_media_info(&file_info);
        }
		else
		{
			GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", STR_NO_PVR_FILE);
			GUI_SetProperty(TXT_PVR_MEDIA_INFO_2, "string", " ");
			GUI_SetProperty(TXT_PVR_MEDIA_INFO_3, "string", " ");
		}
    }
    else
    {
        GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", STR_NO_PVR_FILE);
    }

#ifdef FILE_EDIT_VALID
	GUI_SetProperty(IMG_DEL_TIP, "state","show");
	GUI_SetProperty(TEXT_DEL_TIP, "state","show");
	GUI_SetProperty(IMG_DEL_ALL_TIP, "state","show");
	GUI_SetProperty(TEXT_DEL_ALL_TIP, "state","show");
#endif

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_destroy(GuiWidget *widget, void *usrdata)
{
    int32_t init_value = 0;
    pmpset_exit();
    if(s_pvr_media_map.record != NULL)
    {
        GxCore_Free(s_pvr_media_map.record);
        s_pvr_media_map.record= NULL;
    }
    s_pvr_media_map.total = 0;

    app_free_dir_ent(&(s_pvr_dir.ents), &(s_pvr_dir.nents));
    memset(&s_pvr_dir, 0, sizeof(s_pvr_dir));

    GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);
    
    return EVENT_TRANSFER_STOP;
}

#ifdef FILE_EDIT_VALID
static int _delete_pvr_media_poptip_cb(PopDlgRet ret)
{
    int sel = 0;
    status_t result = GXCORE_SUCCESS;
    GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
    result = _delete_pvr_media(sel);
    app_pvr_media_update();
    GUI_SetProperty(LISTVIEW_PVR_MEIDA, "update_all", NULL);
    if (result == GXCORE_ERROR)
    {
	PopDlg  pop = {0};
	pop.type = POP_TYPE_NO_BTN;
	pop.format = POP_FORMAT_DLG; 
	pop.str = "Delete failed!";
	pop.timeout_sec = 2;
	popdlg_create(&pop);	
    }
    return 0;
}
#endif

#ifdef FILE_EDIT_VALID
static int _deletei_all_pvr_media_poptip_cb(PopDlgRet ret)
{
    int sel = 0;
    status_t result = GXCORE_SUCCESS;
    int total = s_pvr_media_map.total;	
	PopDlg  pop = {0};
	pop.type = POP_TYPE_NO_BTN;
	pop.mode = POP_MODE_UNBLOCK;
	pop.str = STR_ID_DELETING;
	pop.timeout_sec = 1000;
	popdlg_create(&pop);	
    while(total)
    {
        result = _delete_pvr_media(sel);
        app_pvr_media_update();
        total--;
        if (result == GXCORE_ERROR)
        	{
		sel ++;
        	}
    }
	pop.type = POP_TYPE_NO_BTN;
	pop.mode = POP_MODE_UNBLOCK;
	pop.str = STR_ID_DELETING;
	pop.timeout_sec = 0;
	popdlg_create(&pop);	
    GUI_SetProperty(LISTVIEW_PVR_MEIDA, "update_all", NULL);
    return 0;
}
#endif

SIGNAL_HANDLER int app_pvr_media_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int sel = 0;

    
    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
         switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
         {
#if (DLNA_SUPPORT > 0)
             case VK_DLNA_TRIGGER:
                 app_dlna_ui_exec(NULL, NULL);
                 break;
#endif
             case VK_BOOK_TRIGGER:
                 GUI_EndDialog("after wnd_full_screen");
                 break;

             case STBK_EXIT:
             case STBK_MENU:
                 app_pvr_media_quit();
                 break;

             case STBK_RED:
                 {
#ifdef FILE_EDIT_VALID
                     PvrFileInfo file_info;
                     PopDlg  pop = {0};
                     if(s_pvr_media_map.total == 0)
                         break;

                     GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);

                     memset(&pop, 0, sizeof(PopDlg));
                     pop.type = POP_TYPE_YES_NO;
                     pop.str = STR_ID_SURE_DELETE;
                     if(popdlg_create(&pop) == POP_VAL_OK)
                     {
                         memset(&pop, 0, sizeof(PopDlg));
                         pop.type = POP_TYPE_NO_BTN;
                         pop.format = POP_FORMAT_DLG; 
                         pop.str = STR_ID_DELETING;
                         pop.exit_cb= _delete_pvr_media_poptip_cb;
                         pop.timeout_sec = 1;
                         popdlg_create(&pop);
                     }
                     else
                     {
                         break;
                     }	

                     if(s_pvr_media_map.total == 0)
                     {
                         sel =-1;
                         GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", STR_NO_PVR_FILE);
                         GUI_SetProperty(TXT_PVR_MEDIA_INFO_2, "string", " ");
                         GUI_SetProperty(TXT_PVR_MEDIA_INFO_3, "string", " ");
                     }
                     else if(sel >= s_pvr_media_map.total)
                     {
                         sel = s_pvr_media_map.total - 1;
                     }
                     GUI_SetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);

                     if( _get_pvr_media_info(sel, &file_info) == GXCORE_SUCCESS)
                     {
                         display_pvr_media_info(&file_info);
                     }
					 else
					 {
						GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", STR_NO_PVR_FILE);
						GUI_SetProperty(TXT_PVR_MEDIA_INFO_2, "string", " ");
						GUI_SetProperty(TXT_PVR_MEDIA_INFO_3, "string", " ");
					 }
#endif
                 }
                 break;
             case STBK_BLUE:
                 {
#ifdef FILE_EDIT_VALID
                     PvrFileInfo file_info;
                     PopDlg  pop = {0};
                     if(s_pvr_media_map.total == 0)
                         break;

                     GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);

                     memset(&pop, 0, sizeof(PopDlg));
                     pop.type = POP_TYPE_YES_NO;
                     pop.str = STR_ID_SURE_DELETE_ALL;
                     if(popdlg_create(&pop) == POP_VAL_OK)
                     {
                         memset(&pop, 0, sizeof(PopDlg));
                         pop.type = POP_TYPE_NO_BTN;
                         pop.format = POP_FORMAT_DLG; 
						pop.str = STR_ID_DELETING;
                         pop.exit_cb = _deletei_all_pvr_media_poptip_cb;
                         pop.timeout_sec = 1;
                         popdlg_create(&pop);
                     }
                     else
                     {
                         break;
                     }	

                     if(s_pvr_media_map.total == 0)
                     {
                         sel =-1;
                         GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", STR_NO_PVR_FILE);
                         GUI_SetProperty(TXT_PVR_MEDIA_INFO_2, "string", " ");
                         GUI_SetProperty(TXT_PVR_MEDIA_INFO_3, "string", " ");
                     }
                     else if(sel >= s_pvr_media_map.total)
                     {
                         sel = s_pvr_media_map.total - 1;
                     }
                     GUI_SetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);

                     if( _get_pvr_media_info(sel, &file_info) == GXCORE_SUCCESS)
                     {
                         display_pvr_media_info(&file_info);
                     }
					 else
					 {
						 GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", STR_NO_PVR_FILE);
						 GUI_SetProperty(TXT_PVR_MEDIA_INFO_2, "string", " ");
						 GUI_SetProperty(TXT_PVR_MEDIA_INFO_3, "string", " ");
					 }
#endif
                 }
                 break;

             case STBK_OK:
                 GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);
                 _play_pvr_media(sel);
                 break;

             default:
                 break;
         }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_list_get_total(GuiWidget *widget, void *usrdata)
{
    return s_pvr_media_map.total;
}

SIGNAL_HANDLER int app_pvr_media_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;
    if(s_pvr_media_map.record == NULL)
        return GXCORE_ERROR;

    //col-0: channel name
    item->image = NULL;
    item->string = s_pvr_dir.ents[s_pvr_media_map.record[item->sel]].fname;

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_media_list_change(GuiWidget *widget, void *usrdata)
{
    int sel;
    PvrFileInfo file_info;
    
    GUI_GetProperty(LISTVIEW_PVR_MEIDA, "select", &sel);

    memset(&file_info, 0, sizeof(PvrFileInfo));
    if( _get_pvr_media_info(sel, &file_info) == GXCORE_SUCCESS)
    {
        display_pvr_media_info(&file_info);
    }
	else
	{
		GUI_SetProperty(TXT_PVR_MEDIA_INFO_1, "string", STR_NO_PVR_FILE);
		GUI_SetProperty(TXT_PVR_MEDIA_INFO_2, "string", " ");
		GUI_SetProperty(TXT_PVR_MEDIA_INFO_3, "string", " ");
	}
    return EVENT_TRANSFER_STOP;
}

