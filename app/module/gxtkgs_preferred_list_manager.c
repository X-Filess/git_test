/************************************************************
Copyright (C), 2014, GX S&T Co., Ltd.
FileName   :	gxtkgs_preferred_list_manager.c
Author     : 
Version    : 	1.0
Date       :
Description:
Version    :
History    :
Date                Author      Modification
2014.06.17     liqg         create
***********************************************************/
/* Includes --------------------------------------------------------------- */
#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "sys_setting_manage.h"
#include "app_utility.h"
#include "app_config.h"
#include "gxcore.h"

#include "gxtkgs.h"
#if TKGS_SUPPORT

#ifdef LINUX_OS
#define FILE_MODE  "w+"
#else
#define FILE_MODE  "w+"
#endif


#define GX_TKGS_PREFERRED_LIST_FILE_NAME "/home/gx/tkgs_preferred_list_db"
#define TKGS_USER_PREFERRED_LIST_NAME	"tkgs>preflistname"

static status_t GxBus_TkgsPreferredFreeList(void);

/*��ʼ�����*/
//static uint32_t gx_tkgs_prefer_list_init_flag = 0;

/*���������*/
static handle_t gx_tkgs_prefer_list_mutex_handle = -1;

/*current preferred list*/
static GxTkgsPreferredList *tkgsPreferredList = NULL;


/**
 * @brief 	�����TKGS��������������������б�
 * @return   	GXCORE_SUCCESS:ִ������
 				GXCORE_ERROR:ִ��ʧ��
 */
status_t GxBus_TkgsPreferredListSave(GxTkgsPreferredList *list)
{
	int size=0;
	int i, j;
	int serviceNum=0;
	GxTkgsPreferredList *preferredList = NULL;
	handle_t file;
	
	if (list == NULL)
	{
		return GXCORE_ERROR;
	}
	
    if(gx_tkgs_prefer_list_mutex_handle == -1)
    {
        GxCore_MutexCreate(&gx_tkgs_prefer_list_mutex_handle);
    }

	GxCore_MutexLock(gx_tkgs_prefer_list_mutex_handle);
	if ((list->listNum == 0) || (list->prefer == NULL))
	{
		GxCore_FileDelete(GX_TKGS_PREFERRED_LIST_FILE_NAME);
		return GXCORE_SUCCESS;
	}

	size = sizeof(GxTkgsPreferredList)+list->listNum*sizeof(GxTkgsPrefer);
	for(i=0; i<list->listNum; i++)
	{
		size += (list->prefer[i].serviceNum*sizeof(GxTkgsService));
	}
	preferredList = (GxTkgsPreferredList *)GxCore_Malloc(size);
	if (preferredList == NULL)
	{
		GxCore_MutexUnlock(gx_tkgs_prefer_list_mutex_handle);
		return GXCORE_ERROR;
	}
	preferredList->prefer = (GxTkgsPrefer*)((void*)preferredList+sizeof(GxTkgsPreferredList));
	
	preferredList->listNum = list->listNum;
	for (i=0; i<list->listNum; i++)
	{
		preferredList->prefer[i] = list->prefer[i];
		preferredList->prefer[i].list = (GxTkgsService*)((void*)preferredList+sizeof(GxTkgsPreferredList) 
			+ preferredList->listNum*sizeof(GxTkgsPrefer)
			+ serviceNum*sizeof(GxTkgsService));
		serviceNum += preferredList->prefer[i].serviceNum;
		for (j=0; j<preferredList->prefer[i].serviceNum; j++)
		{
			preferredList->prefer[i].list[j] = list->prefer[i].list[j];
		}
	}

	file = GxCore_Open(GX_TKGS_PREFERRED_LIST_FILE_NAME, FILE_MODE);
	if(file<0)
	{
		//#ifdef GX_BUS_PM_DBUG
		//GX_BUS_PM_VIEW_INFO_PRINTF("[SYS INFO]---GxBus_PmViewInfoModify open file err!\n");
		//#endif
		GxCore_MutexUnlock(gx_tkgs_prefer_list_mutex_handle);
		return GXCORE_ERROR;
	}
	
	i = GxCore_Write(file, preferredList, 1,size);
	//#ifdef GX_BUS_PM_DBUG
	//SYS_INFO_PRINTF("[SYS INFO]---write ret = %d!\n",i);
	//#endif
	GxCore_Close(file);
	GxCore_MutexUnlock(gx_tkgs_prefer_list_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 	�ͷ����е��б�
 * @return   	GXCORE_SUCCESS:ִ������
 				GXCORE_ERROR:ִ��ʧ��
 */
static status_t GxBus_TkgsPreferredFreeList(void)
{
	int i;
	
	if (tkgsPreferredList != NULL)
	{
		if (tkgsPreferredList->prefer != NULL)
		{
			for(i=0; i<tkgsPreferredList->listNum; i++)			
			{
				GxCore_Free(tkgsPreferredList->prefer[i].list);
			}
		}
		GxCore_Free(tkgsPreferredList);
		tkgsPreferredList = NULL;
	}

	return GXCORE_SUCCESS;
}


/**
 * @brief 	��ȡ�����б�
 * @return   	GXCORE_SUCCESS:ִ������
 				GXCORE_ERROR:ִ��ʧ��
 */
status_t GxBus_TkgsPreferredListGet(GxTkgsPreferredList *list)
{
	size_t real_size = 0;
	handle_t ret = GXCORE_FILE_UNEXIST;
	handle_t file;
	uint32_t listSize = 0;
	int i;
	GxTkgsPreferredList tempList = {0, NULL};

    if(gx_tkgs_prefer_list_mutex_handle == -1)
    {
        GxCore_MutexCreate(&gx_tkgs_prefer_list_mutex_handle);
    }

	GxBus_TkgsPreferredFreeList();

	GxCore_MutexLock(gx_tkgs_prefer_list_mutex_handle);
	ret =  GxCore_FileExists  (GX_TKGS_PREFERRED_LIST_FILE_NAME);
	if(ret == GXCORE_FILE_UNEXIST)
	{
		TKGS_ERRO_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		GxCore_MutexUnlock(gx_tkgs_prefer_list_mutex_handle);
		return GXCORE_ERROR;
	}

	file = GxCore_Open(GX_TKGS_PREFERRED_LIST_FILE_NAME, "r"/*FILE_MODE*/);
	if(file<0)
	{
		TKGS_ERRO_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		GxCore_MutexUnlock(gx_tkgs_prefer_list_mutex_handle);
		return GXCORE_ERROR;
	}

	/*get list number*/
	real_size = GxCore_Read(file, &tempList, 1,sizeof(GxTkgsPreferredList));
	if((real_size != sizeof(GxTkgsPreferredList)) ||(tempList.listNum==0))
	{
		TKGS_ERRO_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		GxCore_Close(file);
		GxCore_MutexUnlock(gx_tkgs_prefer_list_mutex_handle);
		return GXCORE_ERROR;
	}

	/*get prefer*/
	tkgsPreferredList = (GxTkgsPreferredList *)GxCore_Malloc(sizeof(GxTkgsPreferredList)+sizeof(GxTkgsPrefer)*tempList.listNum);
	if (tkgsPreferredList == NULL)
	{
		TKGS_ERRO_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		GxCore_Close(file);
		GxCore_MutexUnlock(gx_tkgs_prefer_list_mutex_handle);
		return GXCORE_ERROR;
	}
	tkgsPreferredList->listNum = tempList.listNum;
	tkgsPreferredList->prefer = (GxTkgsPrefer*)((void*)tkgsPreferredList+sizeof(GxTkgsPreferredList));
	real_size = GxCore_Read(file, tkgsPreferredList->prefer, 1,sizeof(GxTkgsPrefer)*tkgsPreferredList->listNum);
	if(real_size != sizeof(GxTkgsPrefer)*tkgsPreferredList->listNum)
	{
		TKGS_ERRO_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
		GxCore_Close(file);
		GxCore_MutexUnlock(gx_tkgs_prefer_list_mutex_handle);
		GxBus_TkgsPreferredFreeList();
		return GXCORE_ERROR;
	}

	//��ȡ�����б�
	for (i=0; i<tkgsPreferredList->listNum; i++)
	{
		listSize = (tkgsPreferredList->prefer[i].serviceNum * sizeof(GxTkgsService));
		tkgsPreferredList->prefer[i].list = NULL;
		tkgsPreferredList->prefer[i].list = (GxTkgsService*)GxCore_Malloc(listSize);
		if (tkgsPreferredList->prefer[i].list == NULL)
		{
			TKGS_ERRO_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
			GxCore_Close(file);
			GxCore_MutexUnlock(gx_tkgs_prefer_list_mutex_handle);
			GxBus_TkgsPreferredFreeList();
			return GXCORE_ERROR;	
		}
		real_size = GxCore_Read(file, tkgsPreferredList->prefer[i].list, 1,listSize);
		if(real_size != listSize)
		{
			TKGS_ERRO_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
			GxCore_Close(file);
			GxCore_MutexUnlock(gx_tkgs_prefer_list_mutex_handle);
			GxBus_TkgsPreferredFreeList();
			return GXCORE_ERROR;
		}
	}

	GxCore_Close(file);
	GxCore_MutexUnlock(gx_tkgs_prefer_list_mutex_handle);
	
//	*list = tkgsPreferredList;
	list->listNum = tkgsPreferredList->listNum;
	list->prefer = tkgsPreferredList->prefer;
	TKGS_DEBUG_PRINTF("===%s, %d\n", __FUNCTION__, __LINE__);
	return GXCORE_SUCCESS;
}


/**
 * @brief 	�����û�ѡ����б�����
 * @return   	GXCORE_SUCCESS:ִ������
 				GXCORE_ERROR:ִ��ʧ��
 */
status_t GxBus_TkgsUserPreferredListNameSet(char *listName)
{
	if (listName == NULL)
	{
		return GXCORE_ERROR;
	}
	
	GxBus_ConfigSet(TKGS_USER_PREFERRED_LIST_NAME, listName);
	return GXCORE_SUCCESS;
}

/**
 * @brief 	��ȡ�û�ѡ����б�����
 * @return   	GXCORE_SUCCESS:ִ������
 				GXCORE_ERROR:ִ��ʧ��
 */
status_t GxBus_TkgsUserPreferredListNameGet(char *listName)
{
	if (listName == NULL)
	{
		return GXCORE_ERROR;
	}
	
	GxBus_ConfigGet(TKGS_USER_PREFERRED_LIST_NAME,listName,TKGS_PREFER_NAME_LEN,"default");

	return GXCORE_SUCCESS;
}
#endif

