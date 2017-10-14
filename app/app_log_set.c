#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_utility.h"

#include "gxhotplug.h"
#include "comm_log.h"


#define PARTITION_NAME_LIST_LEN_MAX (100)

typedef enum
{
	LOG_ITEM_OUT_PUT_TYPE=0,
	LOG_ITEM_DISK_SELECT,
	LOG_ITEM_TOTAL,
}LogSetItemType;

//extern int gn_view_list_hide;

//static explorer_para* lpExplorerView=NULL;
static HotplugPartitionList* lpPartitionList=NULL;
static char lsPartionNameList[PARTITION_NAME_LIST_LEN_MAX]={0};

#define IMG_SYSTEM_SETTING_GIF	"img_system_setting_gif"



static SystemSettingOpt lLogSetOpt;
static SystemSettingItem lLogSetItem[LOG_ITEM_TOTAL];

#define OUT_PUT_TYPE_CONSOLE		"CONSOLE"
#define OUT_PUT_TYPE_DISK		"DISK"
#define OUT_PUT_TYPE_NETWORK		"NETWORK"

#define NOT_FOUND_DISK "No Disk"

#ifdef LINUX_OS

static char lsOutPutType[] ="[CONSOLE,DISK,FLASH,NETWORK,NULL]";
#else
static char lsOutPutType[] ="[CONSOLE,DISK,FLASH,NULL]";

#endif

void GetPartNameList();
















void app_log_set_init_callback()
{

#if 0/* BEGIN: Deleted by yingc, 2013/12/13 */
	GUI_SetProperty(TXT_VIEW_LIST, "state", "hide");
	GUI_SetProperty(IMG_VIEW_LIST_OK, "state", "hide");
#endif/* END:   Deleted by yingc, 2013/12/13   PN: */
}




int app_log_set_exit_callback(ExitType exit_type)
{
	//PvrSetPara para_ret;
	int ret = 0;
	int cmb_sel=0;
	int nDiskSel=0;

	char partition[50] = {0};
	int nOutPutType=0;

	//char sOutPutType[5] = {0};


	PopDlgRet pop_status = POP_VAL_CANCEL;


	GUI_GetProperty("cmb_system_setting_opt1","select",&cmb_sel);
	if(cmb_sel==LOG_OUT_PUT_DISK)
	{
		if(strstr(lsPartionNameList,NOT_FOUND_DISK))
		{
			goto LOGSETEXITCALLBACK;
		}
		GetPartNameList();
		GUI_GetProperty("cmb_system_setting_opt2","select",&nDiskSel);
		if(NULL==lpPartitionList || lpPartitionList->partition_num<1 || nDiskSel>lpPartitionList->partition_num-1)
		{
			goto LOGSETEXITCALLBACK;
		}

		if(NULL==lpPartitionList->partition && (NULL==&(lpPartitionList->partition[nDiskSel])))
		{
			goto LOGSETEXITCALLBACK;
		}
		GxBus_ConfigGet(LOG_DISK_SELECT, partition, sizeof(partition), "tmp");
		#if 0/* BEGIN: Deleted by yingc, 2013/12/16 */
		printf("lpPartitionList->partition[nDiskSel].partition_name=%s\n",lpPartitionList->partition[nDiskSel].partition_name);
		printf("lpPartitionList->partition[nDiskSel].partition_entry=%s\n",lpPartitionList->partition[nDiskSel].partition_entry);
		printf("lpPartitionList->partition[nDiskSel].dev_name=%s\n",lpPartitionList->partition[nDiskSel].dev_name);
		#endif/* END:   Deleted by yingc, 2013/12/16   PN: */
		if(!(strcmp(partition,lpPartitionList->partition[nDiskSel].partition_entry)))
		{
			goto LOGSETEXITCALLBACK;
		}	
		else
		{
			PopDlg  pop;
			memset(&pop, 0, sizeof(PopDlg));
			pop.type = POP_TYPE_YES_NO;
			pop.str = STR_ID_SAVE_INFO;

			pop_status = popdlg_create(&pop);
			if(pop_status == POP_VAL_OK)
			{
#if 0/* BEGIN: Deleted by yingc, 2013/12/16 */
				itoa(cmb_sel,sOutPutType,10);
				GxBus_ConfigSet(LOG_OUT_PUT_TYPE, sOutPutType);
#endif/* END:   Deleted by yingc, 2013/12/16   PN: */
				GxBus_ConfigSetInt(LOG_OUT_PUT_TYPE, cmb_sel);

				GxBus_ConfigSet(LOG_DISK_SELECT, lpPartitionList->partition[nDiskSel].partition_entry);
				ret =1;
			}

		}		
	}else
	{
		GxBus_ConfigGetInt(LOG_OUT_PUT_TYPE,&nOutPutType,0);
		if(nOutPutType!=cmb_sel)
		{
			PopDlg  pop;
			memset(&pop, 0, sizeof(PopDlg));
			pop.type = POP_TYPE_YES_NO;
			pop.str = STR_ID_SAVE_INFO;

			pop_status = popdlg_create(&pop);
			if(pop_status == POP_VAL_OK)
			{
				//itoa(cmb_sel,sOutPutType,10);
				GxBus_ConfigSetInt(LOG_OUT_PUT_TYPE, cmb_sel);
			}
		}
		printf("-----ddddd---\n");
	}




LOGSETEXITCALLBACK:
	g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);

	if (g_AppPlayOps.normal_play.play_total != 0)
	{
		// deal for recall , current will copy to recall
#if RECALL_LIST_SUPPORT
		//extern int g_recall_count;
		g_AppPlayOps.current_play = g_AppPlayOps.recall_play[0];
#else
		g_AppPlayOps.current_play = g_AppPlayOps.recall_play;
#endif
		g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT,
				g_AppPlayOps.normal_play.play_count);
	}

	return ret;
}


void GetPartNameList()
{
	HotplugPartition* partition = NULL;


	int i=0,n_len=0;

	lpPartitionList = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);

	partition = &(lpPartitionList->partition[0]);
	memset(lsPartionNameList,0,sizeof(lsPartionNameList));
	n_len+=snprintf((lsPartionNameList)+n_len,PARTITION_NAME_LIST_LEN_MAX-n_len,"%s","[");

	if(NULL!=lpPartitionList)
	{

		if(lpPartitionList->partition_num>0)
		{

			for(i=0;i<lpPartitionList->partition_num;i++)
			{
				if(NULL==(&(lpPartitionList->partition[i])))goto GETPARTNAMELIST;
				if(i==lpPartitionList->partition_num-1)
				{
					n_len+=snprintf((lsPartionNameList)+n_len,PARTITION_NAME_LIST_LEN_MAX-n_len,"%s",lpPartitionList->partition[i].partition_entry);
				}
				else
				{
					n_len+=snprintf((lsPartionNameList)+n_len,PARTITION_NAME_LIST_LEN_MAX-n_len,"%s,",lpPartitionList->partition[i].partition_entry);
				}
			}	

		}
		else
		{
			n_len+=snprintf((lsPartionNameList)+n_len,PARTITION_NAME_LIST_LEN_MAX-n_len,"%s",NOT_FOUND_DISK);
		}

	}
	else
	{
		n_len+=snprintf((lsPartionNameList)+n_len,PARTITION_NAME_LIST_LEN_MAX-n_len,"%s",NOT_FOUND_DISK);
	}
	n_len+=snprintf((lsPartionNameList)+n_len,PARTITION_NAME_LIST_LEN_MAX-n_len,"%s","]");

GETPARTNAMELIST:


	return;
}

static int app_out_put_sel_change_callback(int sel)
{
	#define TXT_INFO "text_system_setting_timezone"
	GUI_SetProperty(TXT_INFO,"state","show");
	char sSavePath[50]={0};
	if(sel != LOG_OUT_PUT_DISK)
	{
		app_system_set_item_state_update(LOG_ITEM_DISK_SELECT, ITEM_DISABLE);
		if(sel==LOG_OUT_PUT_NULL)
		{
			GUI_SetProperty(TXT_INFO,"string","don't output log!");	
			return EVENT_TRANSFER_KEEPON;
		}
		if(sel==LOG_OUT_PUT_CONSOLE)
		{
			GUI_SetProperty(TXT_INFO,"state","hide");	
			return EVENT_TRANSFER_KEEPON;
		}		
		
#ifdef LINUX_OS
		if(sel==LOG_OUT_PUT_SOCKET)
		{
			GUI_SetProperty(TXT_INFO,"state","hide");
		}
#else
		snprintf(sSavePath,sizeof(sSavePath),"%s%s",LOG_FILE_DEFAULT_FLASH_PATH,LOG_FILE_DEFAULT_NAME);
		GUI_SetProperty(TXT_INFO,"string",sSavePath);	

#endif			

	}
	else
	{
		app_system_set_item_state_update(LOG_ITEM_DISK_SELECT, ITEM_NORMAL);
		snprintf(sSavePath,sizeof(sSavePath),"%s",LOG_FILE_DEFAULT_NAME);
		GUI_SetProperty(TXT_INFO,"string",sSavePath);	
	}
	return EVENT_TRANSFER_KEEPON;
}



/* BEGIN: Added by yingc, 2013/8/16 */
void app_log_set_out_put_item_init(void)
{
	//int i=0;
	char sOutPutType[5] = {0};
	int nOutPutType=0;


	lLogSetItem[LOG_ITEM_OUT_PUT_TYPE].itemProperty.itemPropertyCmb.content=lsOutPutType;

	lLogSetItem[LOG_ITEM_OUT_PUT_TYPE].itemTitle = STR_ID_OUT_PUT_TYPE;
	lLogSetItem[LOG_ITEM_OUT_PUT_TYPE].itemType = ITEM_CHOICE;

	lLogSetItem[LOG_ITEM_OUT_PUT_TYPE].itemCallback.cmbCallback.CmbPress= NULL;


	GxBus_ConfigGet(LOG_OUT_PUT_TYPE, sOutPutType, sizeof(sOutPutType), "0");
	nOutPutType=atoi(sOutPutType);
	lLogSetItem[LOG_ITEM_OUT_PUT_TYPE].itemProperty.itemPropertyCmb.sel = nOutPutType;
	//lLogSetItem[ITEM_FILE_SIZE].itemProperty.itemPropertyCmb.sel = size_to_sel[file_size/1025];
	lLogSetItem[LOG_ITEM_OUT_PUT_TYPE].itemCallback.cmbCallback.CmbChange= app_out_put_sel_change_callback;
	lLogSetItem[LOG_ITEM_OUT_PUT_TYPE].itemStatus = ITEM_NORMAL;	

}


void app_log_set_disk_select_item_init(void)
{
	char sOutPutType[5] = {0};
	int nOutPutType=0;

	lLogSetItem[LOG_ITEM_DISK_SELECT].itemTitle = STR_ID_DISK_SELECT;
	lLogSetItem[LOG_ITEM_DISK_SELECT].itemType = ITEM_CHOICE;
	lLogSetItem[LOG_ITEM_DISK_SELECT].itemProperty.itemPropertyCmb.content=lsPartionNameList;
	lLogSetItem[LOG_ITEM_DISK_SELECT].itemCallback.cmbCallback.CmbPress= NULL;



	//lLogSetItem[ITEM_FILE_SIZE].itemProperty.itemPropertyCmb.sel = size_to_sel[file_size/1025];
	lLogSetItem[LOG_ITEM_DISK_SELECT].itemCallback.cmbCallback.CmbChange= NULL;

	GxBus_ConfigGet(LOG_OUT_PUT_TYPE, sOutPutType, sizeof(sOutPutType), "0");
	nOutPutType=atoi(sOutPutType);	
	if(nOutPutType==LOG_OUT_PUT_DISK)
	{
		lLogSetItem[LOG_ITEM_DISK_SELECT].itemStatus = ITEM_NORMAL;	
		lLogSetItem[LOG_ITEM_DISK_SELECT].itemProperty.itemPropertyCmb.sel = 0;
	}else
	{
		lLogSetItem[LOG_ITEM_DISK_SELECT].itemStatus = ITEM_DISABLE;
		lLogSetItem[LOG_ITEM_DISK_SELECT].itemProperty.itemPropertyCmb.sel = 0;
	}

	//s_PvrSetPara.file_size = file_size;
}



void app_log_set_menu_exec(void)
{
	memset(&lLogSetOpt, 0 ,sizeof(SystemSettingOpt));
	lLogSetOpt.menuTitle =STR_ID_LOG_SET;
	//lLogSetOpt.titleImage = "";
	lLogSetOpt.itemNum = LOG_ITEM_TOTAL;
	//lLogSetOpt.timeDisplay = TIP_HIDE;
	lLogSetOpt.item = lLogSetItem;

	GetPartNameList();

	app_log_set_out_put_item_init();
	app_log_set_disk_select_item_init();

	lLogSetOpt.exit = app_log_set_exit_callback;
	//lLogSetOpt.init = app_log_set_init_callback;
	//gn_view_list_hide=1;
	app_system_set_create(&lLogSetOpt);
}
