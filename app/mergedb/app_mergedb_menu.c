#include "app.h"
#include "app_pop.h"
#include <gxcore.h>
#include "app_wnd_file_list.h"
#if MERGE_DB_SUPPORT

#ifdef LINUX_OS
#define FILE_MODE "w+"
#define GX_DEFAULT_DB_PATH	"/home/root/default_data.db"
#else
#define FILE_MODE "a+"//"rc"
#define GX_DEFAULT_DB_PATH	"/default_data.db"
#endif
#define DB_SUFFIX "db;DB"
#define DEFAULT_DB "/default_data.db"
#define GX_PM_VERIFY_FILE_PATH "/home/gx/verify.db"

enum
{
	EXPORT_USER_DB,
	EXPORT_DEF_DB,
};

typedef enum
{
	MERGE_DB_TYPE,
	MERGE_EXPORT_SWITCH,
	MERGE_DB_PATH,
	MERGE_DB_OK,
}GxMergeDB;

typedef enum
{
	MERGE_DB_EXPORT,
	MERGE_DB_INPUT,
}MergeDbTypeSel; 

enum gx_pm_verify_flag
{
	GX_PM_VERIFY_ERR = 0,
	GX_PM_VERIFY_OK
};
static char *s_file_path_bak = NULL;
static char *s_file_path = NULL;

extern void app_reboot(void);
extern status_t GxBus_PmCreateDefaultDb(int8_t* file_patch);
static status_t app_pm_verify_write(enum gx_pm_verify_flag flag)
{
	handle_t gx_pm_verify_file = 0;
	static int old_flag = -1;
	int32_t buf = 0;
	handle_t ret = GXCORE_FILE_UNEXIST;
	ret =  GxCore_FileExists  (GX_PM_VERIFY_FILE_PATH);
	
	if (old_flag == flag)
		return GXCORE_SUCCESS;
	old_flag = flag;

	if(ret == GXCORE_FILE_UNEXIST)
	{
		gx_pm_verify_file = GxCore_Open(GX_PM_VERIFY_FILE_PATH, FILE_MODE);
		if(gx_pm_verify_file<0)
		{
			return GXCORE_ERROR;
		}
		GxCore_WriteAt(gx_pm_verify_file,&buf,1,4,sizeof(enum gx_pm_verify_flag));//第一次创建文件，修改记录清零
	}
	else
	{
		gx_pm_verify_file = GxCore_Open(GX_PM_VERIFY_FILE_PATH,FILE_MODE);
	}
	if(gx_pm_verify_file<0)
	{
		return GXCORE_ERROR;
	}
	if(flag == GX_PM_VERIFY_OK)
		{
		/*每进行一次成功的更新，记录号加一*/
		GxCore_ReadAt(gx_pm_verify_file,&buf,1,4,sizeof(enum gx_pm_verify_flag));
		buf +=1;
		GxCore_WriteAt(gx_pm_verify_file,&buf,1,4,sizeof(enum gx_pm_verify_flag));
		}
	
	GxCore_WriteAt(gx_pm_verify_file,&flag,1,sizeof(enum gx_pm_verify_flag),0);
//	GX_PM_FS_WRITE(gx_pm_verify_file, &flag, 1,sizeof(enum gx_pm_verify_flag));
	GxCore_Close(gx_pm_verify_file);
	return GXCORE_SUCCESS;
	}

#ifdef ECOS_OS
int _export_default_db(char* file_path)
{
#define FILE_BUF_SIZE (1024)
	int ret = GXCORE_ERROR;
	handle_t default_db_file = 0;
	handle_t export_file = 0;
	uint64_t write_size = 0,read_size = 0;
	GxFileInfo info;
	char buf[FILE_BUF_SIZE];
	
	if(file_path == NULL)
	{
		return GXCORE_ERROR;
	}
	
	export_file = GxCore_Open((const char*)file_path, "w");
	if(export_file<0)
	{
		goto end;
	}

	default_db_file = GxCore_Open(GX_DEFAULT_DB_PATH, "r");
	if(default_db_file<0)
	{
		goto end;
	}
	
	GxCore_GetFileInfo(GX_DEFAULT_DB_PATH, &info);

	while(info.size_by_bytes - write_size >0)
	{
		uint32_t size = 0;
		
		read_size = ((info.size_by_bytes - write_size) > FILE_BUF_SIZE)?FILE_BUF_SIZE:(info.size_by_bytes - write_size);
		memset(buf, 0, FILE_BUF_SIZE);
		size = GxCore_Read(default_db_file,buf,1,read_size);
		
		size = GxCore_Write(export_file,buf,1,size);
		write_size += size;
	}
	
	if(write_size == info.size_by_bytes)
	{
		ret = GXCORE_SUCCESS;
	}
end:
	if(-1!=export_file)
		GxCore_Close(export_file);
	if(-1!=default_db_file)
		GxCore_Close(default_db_file);
	
	return ret;
}
#endif

static status_t export_db(void )
{
    uint32_t cmb_sel = 0;
    char buf[100] = {0};
    int ret = GXCORE_ERROR;
    PopDlg  pop;

    if(s_file_path == NULL)
	{
		memset(&pop, 0, sizeof(PopDlg));
		pop.str = STR_ID_ERR_PATH;
		pop.type = POP_TYPE_OK;
		pop.format = POP_FORMAT_DLG;
		pop.mode = POP_MODE_UNBLOCK;
		pop.timeout_sec= 3;
		popdlg_create(&pop);
		GUI_SetInterface("flush", NULL);
		return GXCORE_ERROR;
	}

    memset(buf, 0, sizeof(buf));
    GUI_GetProperty("cmb_export_switch", "select", &cmb_sel);
    if(cmb_sel == EXPORT_USER_DB)
    {
        sprintf(buf,"%s%s",s_file_path,DEFAULT_DB);
        ret = GxBus_PmCreateDefaultDb((int8_t*)buf);
    }
    else if(cmb_sel == EXPORT_DEF_DB)
    {
#ifdef LINUX_OS
        sprintf(buf,"cp %s %s%s",GX_DEFAULT_DB_PATH,s_file_path,DEFAULT_DB);
        ret = system(buf);
        if(ret != -1 && ret !=127)
        {
            ret = GXCORE_SUCCESS;
        }
#else
        sprintf(buf,"%s%s",s_file_path,DEFAULT_DB);
        ret = _export_default_db(buf);
#endif
    }
#ifdef LINUX_OS
    system("sync");
#endif
    memset(&pop, 0, sizeof(PopDlg));
    if(ret == GXCORE_SUCCESS)
    {
        pop.str = "Successed !";
    }
    else
    {
        pop.str = "Not Successed !";
    }

    pop.type = POP_TYPE_OK;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec= 3;
    popdlg_create(&pop);
    GUI_SetInterface("flush", NULL);

    return ret;
}

static void inport_db(void)
{
    int32_t Ret=0;
    PopDlg  pop;
    int32_t init_value;

    if(s_file_path == NULL)
	{
		memset(&pop, 0, sizeof(PopDlg));
		pop.str = STR_ID_ERR_PATH;
		pop.type = POP_TYPE_OK;
		pop.format = POP_FORMAT_DLG;
		pop.mode = POP_MODE_UNBLOCK;
		pop.timeout_sec= 3;
		popdlg_create(&pop);
		GUI_SetInterface("flush", NULL);
        return ;
	}

    app_send_msg_exec(GXMSG_PM_CLOSE,NULL);
    GxBus_PmLock();
    app_pm_verify_write(GX_PM_VERIFY_ERR);
    Ret = GxBus_PmDbaseInit(SYS_MAX_SAT, SYS_MAX_TP, SYS_MAX_PROG, (uint8_t*)s_file_path);
	init_value = 0;
	GxBus_ConfigSetInt(FAV_INIT_KEY, init_value);
    GxBus_PmUnlock();

    if(Ret != 0)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.str = "Not Successed !";
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec= 3;
        popdlg_create(&pop);
        GUI_SetInterface("flush", NULL);
        app_pm_verify_write(GX_PM_VERIFY_OK);
    }
    else
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.str = "Import Successed!Reboot...";
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec= 3;
        popdlg_create(&pop);
        GUI_SetInterface("flush", NULL);
        GxCore_ThreadDelay(1000);
        app_reboot();
    }
}

static status_t app_merge_db_path_button_press(MergeDbTypeSel type)
{
    status_t ret = GXCORE_ERROR;
    if(1)
    {
        WndStatus get_path_ret = WND_CANCLE;

        if(check_usb_status() == false)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_INSERT_USB;
            pop.mode = POP_MODE_UNBLOCK;
            popdlg_create(&pop);
            GUI_SetInterface("flush", NULL);
        }
        else
        {
            FileListParam file_para;
            memset(&file_para, 0, sizeof(file_para));

            file_para.cur_path = s_file_path_bak;
            file_para.dest_path = &s_file_path;
            file_para.suffix = DB_SUFFIX;
            if(type == MERGE_DB_INPUT)
            {
                file_para.dest_mode = DEST_MODE_FILE;
                get_path_ret = app_get_file_path_dlg(&file_para);
            }
            else if(type == MERGE_DB_EXPORT)
            {
                file_para.dest_mode = DEST_MODE_DIR;
                get_path_ret = app_get_file_path_dlg(&file_para);
            }

            if(get_path_ret == WND_OK)
            {
                if(s_file_path != NULL)
                {
                    int str_len = 0;

                    //s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = s_file_path;
                    //app_system_set_item_property(ITEM_UPGRADE_PATH, &s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty);
                    printf("\n[func:%s] s_file_path:(%s)\n",__func__,s_file_path);
                    GUI_SetProperty("btn_merge_db_filepath", "string", (void*)s_file_path);

                    ////bak////
                    if(s_file_path_bak != NULL)
                    {
                        GxCore_Free(s_file_path_bak);
                        s_file_path_bak = NULL;
                    }
                    str_len = strlen(s_file_path);

                    s_file_path_bak = (char *)GxCore_Malloc(str_len + 1);
                    if(s_file_path_bak != NULL)
                    {
                        memcpy(s_file_path_bak, s_file_path, str_len);
                        s_file_path_bak[str_len] = '\0';
                    }
                }
            }

            ret = GXCORE_SUCCESS;
        }
    }
    return ret;
}

#endif

SIGNAL_HANDLER  int app_merge_db_create(const char* widgetname, void *usrdata)
{
#if MERGE_DB_SUPPORT

	s_file_path =NULL;
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_merge_db_destroy(const char* widgetname, void *usrdata)
{
#if MERGE_DB_SUPPORT

	if(s_file_path_bak != NULL)
	{
		GxCore_Free(s_file_path_bak);
		s_file_path_bak = NULL;
	}

	s_file_path =NULL;
#endif
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER  int app_merge_db_keypress(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_merge_db_path_press(const char* widgetname, void *usrdata)
{
#if MERGE_DB_SUPPORT

	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	uint32_t cmb_sel = 0;

	if(event->type == GUI_KEYDOWN)
	{
		switch(event->key.sym)
		{
			case STBK_OK:
				GUI_GetProperty("cmb_merge_db_type", "select", &cmb_sel);
				app_merge_db_path_button_press(cmb_sel);
				break;
			default:
				break;
		}
	}
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_merge_db_start_press(const char* widgetname, void *usrdata)
{
#if MERGE_DB_SUPPORT

	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	uint32_t cmb_sel = 0;

	if(event->type == GUI_KEYDOWN)
	{
		switch(event->key.sym)
		{
			case STBK_OK:

				if(check_usb_status() == false)
				{
					PopDlg  pop;
					memset(&pop, 0, sizeof(PopDlg));
					pop.type = POP_TYPE_OK;
					pop.str = STR_ID_INSERT_USB;
					pop.mode = POP_MODE_UNBLOCK;
					popdlg_create(&pop);
					GUI_SetInterface("flush", NULL);
					break;
				}

				GUI_GetProperty("cmb_merge_db_type", "select", &cmb_sel);
				if(MERGE_DB_EXPORT == cmb_sel)
				{
					export_db();
				}
				else if(MERGE_DB_INPUT== cmb_sel)
				{
					inport_db();
				}
				break;
			default:
				break;
		}
	}
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_merge_db_box_keypress(const char* widgetname, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
#if MERGE_DB_SUPPORT
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	uint32_t box_sel;

	if(event->type == GUI_KEYDOWN)
	{
		switch(event->key.sym)
		{
			case STBK_MENU:
			case STBK_EXIT:
				GUI_SetProperty("cmb_export_switch", "content", "[User DB,Default DB]");
				GUI_SetProperty("boxitem_export_switch", "state", "enable");
				GUI_EndDialog("wnd_merge_db");
				break;
			
			case STBK_OK:
				break;

			case STBK_UP:
				GUI_GetProperty("box_merge_db", "select", &box_sel);
				if(MERGE_DB_TYPE == box_sel)
				{
					box_sel = MERGE_DB_OK; 
					GUI_SetProperty("box_merge_db", "select", &box_sel);
					printf("\n[func:%s] , box_sel:%d\n",__func__,box_sel);
				}
				else
				{
					box_sel--;	
					GUI_SetProperty("box_merge_db", "select", &box_sel);
				}
				break;

			case STBK_DOWN:
				GUI_GetProperty("box_merge_db", "select", &box_sel);
				if(MERGE_DB_OK== box_sel)
				{
					box_sel = MERGE_DB_TYPE; 
					GUI_SetProperty("box_merge_db", "select", &box_sel);
				}
				else if(MERGE_DB_TYPE== box_sel)
				{
					uint32_t cmb_sel = 0;
	
					GUI_GetProperty("cmb_merge_db_type", "select", &cmb_sel);
					if(MERGE_DB_EXPORT == cmb_sel)
					{
						box_sel = MERGE_EXPORT_SWITCH; 
					}
					else
					{
						box_sel = MERGE_DB_PATH; 
					}
					GUI_SetProperty("box_merge_db", "select", &box_sel);
				}
				else
				{
					box_sel++;	
					GUI_SetProperty("box_merge_db", "select", &box_sel);
				}
				break;
			case STBK_RIGHT:
			case STBK_LEFT:
				GUI_GetProperty("box_merge_db", "select", &box_sel);
				if(MERGE_DB_TYPE== box_sel)
				{
					uint32_t cmb_sel = 0;
	
					if(s_file_path_bak != NULL)
					{
						GxCore_Free(s_file_path_bak);
						s_file_path_bak = NULL;
					}
					s_file_path =NULL;
					GUI_SetProperty("btn_merge_db_filepath", "string", STR_ID_PRESS_OK);
					
					GUI_GetProperty("cmb_merge_db_type", "select", &cmb_sel);
					if(MERGE_DB_EXPORT == cmb_sel)
					{
						GUI_SetProperty("cmb_export_switch", "content", "[User DB,Default DB]");
						GUI_SetProperty("boxitem_export_switch", "state", "enable");
					}
					else if(MERGE_DB_INPUT== cmb_sel)
					{
						GUI_SetProperty("cmb_export_switch",  "content", "[User DB]");
						GUI_SetProperty("boxitem_export_switch", "state", "disable");
					}
				}
				ret = EVENT_TRANSFER_KEEPON;
				break;
				
			default:
				break;
		}
	}
#endif
	return ret;
}

