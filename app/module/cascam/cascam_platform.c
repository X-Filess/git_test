/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cascam_platform.c
* Author    :	B.Z.
* Project   :	CA Module
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.08.27		   B.Z.			  Creation
*****************************************************************************/
//#include "include/cascam_types.h"
#include "include/cascam_public.h"
#include "include/cascam_os.h"
#include "include/cascam_osd.h"

extern int cascam_start_demux(CASCAMDemuxFlagClass *pFlags, unsigned short pid);
extern int cascam_release_demux(CASCAMDemuxFlagClass *pFlags);
extern int cascam_change_frequence(void);

static CASCAMPlatformClass thiz_control[CASCAM_MAX_CAS];
static CASCAMCASTableClass *thiz_table = NULL;
static char *thiz_CAS[CASCAM_MAX_CAS] ={
    "WANFA",
    "TNT",
    "CMB",
    "CDCAS",
    "DHD",
    "PRIVATE",
};

// for regist
static int _cas_register(CASCAMControlBlockClass *CAS)
{
    int IsFind = 0;
    CASCAMCASTableClass *pTemp = NULL;
    if((NULL == CAS) || (NULL == CAS->CASName) || (0 == cascam_strlen(CAS->CASName)))
    {
        cascam_printf("\nCASCAM, error, %s,%d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    if(NULL == thiz_table)
    {
        thiz_table = cascam_malloc(sizeof(CASCAMCASTableClass));
        if(NULL == thiz_table)
        {
           cascam_printf("\nCASCAM, error, %s,%d\n",__FUNCTION__,__LINE__); 
           return -1;
        }
        cascam_memcpy(&(thiz_table->CASBlock_class), CAS, sizeof(CASCAMControlBlockClass));
        thiz_table->pPreview = NULL;
		thiz_table->pNext = NULL;
        return 0;
    }
    pTemp = thiz_table;
    while(pTemp)
    {
        if(0 == cascam_strcmp(pTemp->CASBlock_class.CASName,CAS->CASName))
        {
            IsFind = 1;
            break;// the cas have been registed
        }
        if(pTemp->pNext)
            pTemp = pTemp->pNext;
        else
            break;
    }
    if(0 == IsFind)
    {// can not find the CAS, need add it
       pTemp->pNext = cascam_malloc(sizeof(CASCAMCASTableClass));
       if(NULL == pTemp->pNext)
       {
           cascam_printf("\nCASCAM, error, %s,%d\n",__FUNCTION__,__LINE__); 
           return -1;
       }
       cascam_memcpy(&(pTemp->pNext->CASBlock_class), CAS, sizeof(CASCAMControlBlockClass));
	   pTemp->pNext->pPreview = pTemp;
       pTemp->pNext->pNext = NULL;
       return 0;
    }
    cascam_printf("\nCASCAM, waring, the CAS have been registed\n");
    return 0;
}

static CASCAMCASTableClass* _cas_find(char *CASName)
{
    CASCAMCASTableClass *pTemp = NULL;

    if(NULL == CASName)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__, __LINE__);
        return NULL;
    }
    pTemp = thiz_table;
    while(pTemp)
    {
        if(0 == cascam_strcmp(CASName, pTemp->CASBlock_class.CASName))
        {// find  it
            return pTemp;
        }
        pTemp = pTemp->pNext;
    }
    return NULL;// can not find it
}

int cascam_cas_release(void)
{
    // TODO:
    return 0;
}

static int _add_cas(char *CASName)
{
    static int i = 0;
    CASCAMCASTableClass *pTemp = NULL;
    if((pTemp = _cas_find(CASName)))
    {
        if(i >= CASCAM_MAX_CAS)
        {
            cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
            return -1;
        }
        cascam_memcpy(&(thiz_control[i++].CASCAMBlock_class),&(pTemp->CASBlock_class),sizeof(CASCAMControlBlockClass));
        return 0;
    }
    cascam_printf("\nCASCAM, error, not support the cas (%s)\n",CASName);
    return -1;
}

static int _find_cas(int incount, int* pArry, int *pOutcount)
{
	int *p = pArry;
	int i = 0;
	unsigned int count = 0;
	if((NULL == pArry) || (NULL == pOutcount))
	{
		cascam_printf("\nCASCAM, error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	for(i = 0; i < CASCAM_MAX_CAS; i++)
	{
		if(NULL != thiz_control[i].CASCAMBlock_class.CASName)
		{
			p[count] = (int)(&thiz_control[i].CASCAMBlock_class);
			count++;
			if(count >= incount)
			{
				cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
				break;
			}
		}
	}
	*pOutcount = count;
	return 0;	
}

// adapter
static CASCAMProgramParaClass thiz_program_parameter = {0};

static int _record_program_parameter(CASCAMProgramParaClass* pParams)
{
	if(NULL == pParams)
	{
		cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	cascam_memcpy(&thiz_program_parameter, pParams,sizeof(CASCAMProgramParaClass));
	return 0;
}
static int _clear_program_parameter(void)
{
	cascam_memset(&thiz_program_parameter, 0,sizeof(CASCAMProgramParaClass));
	return 0;
}

int cascam_update(void)
{
    // TODO:
    return 0;
}

int cascam_init(void)
{
    int ret = 0;   
    int count = 0;
    int i = 0;
    int CASAddr[CASCAM_MAX_CAS] = {0};
    CASCAMControlBlockClass *pTemp = NULL;
	//_clear_program_parameter();
	
    ret = _find_cas(CASCAM_MAX_CAS,CASAddr,&count);
    if(ret < 0)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < count; i++)
    {
        pTemp = (CASCAMControlBlockClass*)CASAddr[i];
        if(pTemp->init)
            pTemp->init();
    }
    return 0;
}

int cascam_close(void)
{
    int ret = 0;   
    int count = 0;
    int i = 0;
    int CASAddr[CASCAM_MAX_CAS] = {0};
    CASCAMControlBlockClass *pTemp = NULL;
    
    ret = _find_cas(CASCAM_MAX_CAS,CASAddr,&count);
    if(ret < 0)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < count; i++)
    {
        pTemp = (CASCAMControlBlockClass*)CASAddr[i];
        if(pTemp->close)
            pTemp->close();
    }

	_clear_program_parameter();
    return 0;
}

int cascam_change_prog(CASCAMProgramParaClass* pParams)
{
    int ret = 0;
    int count = 0;
    int i = 0;
    int CASAddr[CASCAM_MAX_CAS] = {0};
    CASCAMControlBlockClass *pTemp = NULL;
    
    if(NULL == pParams)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    ret = _find_cas(CASCAM_MAX_CAS,CASAddr,&count);
    if(ret < 0)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < count; i++)
    {
        pTemp = (CASCAMControlBlockClass*)CASAddr[i];
        if(pTemp->change_program)
        {
            ret = pTemp->change_program(pParams);
            if(ret < 0)
            {
                cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
	_record_program_parameter(pParams);
    return 0;
}

int cascam_release_demux(CASCAMDemuxFlagClass *pFlags)
{
    int ret = 0;
    int count = 0;
    int i = 0;
    int CASAddr[CASCAM_MAX_CAS] = {0};
    CASCAMControlBlockClass *pTemp = NULL;
    
    if(NULL == pFlags)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    ret = _find_cas(CASCAM_MAX_CAS,CASAddr,&count);
    if(ret < 0)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < count; i++)
    {
        pTemp = (CASCAMControlBlockClass*)CASAddr[i];
        if(pTemp->demux_class.release_demux)
        {
            ret = pTemp->demux_class.release_demux(pFlags);
            if(ret < 0)
            {
                cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
    return 0;
}

int cascam_start_demux(CASCAMDemuxFlagClass *pFlags, unsigned short pid)
{
    int ret = 0;
    int count = 0;
    int i = 0;
    int CASAddr[CASCAM_MAX_CAS] = {0};
    CASCAMControlBlockClass *pTemp = NULL;
    
    if(NULL == pFlags)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    ret = _find_cas(CASCAM_MAX_CAS,CASAddr,&count);
    if(ret < 0)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < count; i++)
    {
        pTemp = (CASCAMControlBlockClass*)CASAddr[i];
        if(pTemp->demux_class.start_demux)
        {
            ret = pTemp->demux_class.start_demux(pFlags, pid);
            if(ret < 0)
            {
                cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
    return 0;
}

int cascam_change_frequence(void)
{
	int ret = 0;
    int count = 0;
    int i = 0;
    int CASAddr[CASCAM_MAX_CAS] = {0};
    CASCAMControlBlockClass *pTemp = NULL;

    ret = _find_cas(CASCAM_MAX_CAS,CASAddr,&count);
    if(ret < 0)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < count; i++)
    {
        pTemp = (CASCAMControlBlockClass*)CASAddr[i];
        if(pTemp->change_frequency)
        {
            ret = pTemp->change_frequency();
            if(ret < 0)
            {
                cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
	return 0;
}

int cascam_set_zone(double zone)
{
	int ret = 0;
    int count = 0;
    int i = 0;
    int CASAddr[CASCAM_MAX_CAS] = {0};
    CASCAMControlBlockClass *pTemp = NULL;

    ret = _find_cas(CASCAM_MAX_CAS,CASAddr,&count);
    if(ret < 0)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < count; i++)
    {
        pTemp = (CASCAMControlBlockClass*)CASAddr[i];
        if(pTemp->set_zone)
        {
            ret = pTemp->set_zone(zone);
            if(ret < 0)
            {
                cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
        }
    }
	return 0;
}

static unsigned  int thiz_cas_id[CASCAM_MAX_CAS] = {0};
int cascam_get_cas_id(unsigned int **ppID, unsigned char *pCount)
{
	int ret = 0;
    int count = 0;
    int i = 0;
    int cas_count = 0;
    int CASAddr[CASCAM_MAX_CAS] = {0};
    CASCAMControlBlockClass *pTemp = NULL;

    if((NULL == ppID) || (NULL == pCount))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    cascam_memset(thiz_cas_id, 0, sizeof(thiz_cas_id));

    ret = _find_cas(CASCAM_MAX_CAS,CASAddr,&count);
    if(ret < 0)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < count; i++)
    {
        pTemp = (CASCAMControlBlockClass*)CASAddr[i];
        if(pTemp->get_cas_id)
        {
            ret = pTemp->get_cas_id();
            thiz_cas_id[cas_count++] = ret;
        }
    }

    *ppID = thiz_cas_id;
    *pCount = cas_count;
	return 0;
}

int cascam_is_vaild_cas(unsigned short CASID)
{
        int ret = 0;
    int count = 0;
    int i = 0;
    int CASAddr[CASCAM_MAX_CAS] = {0};
    CASCAMControlBlockClass *pTemp = NULL;

    ret = _find_cas(CASCAM_MAX_CAS,CASAddr,&count);
    if(ret < 0)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < count; i++)
    {
        pTemp = (CASCAMControlBlockClass*)CASAddr[i];
        if(pTemp->is_vaild_cas)
        {
            ret = pTemp->is_vaild_cas(CASID);
            if(ret == 1)
            {
                return 1;
            }
        }
    }
    return ret;
}

// data transfer
int cascam_set_property(CASCAMDataSetClass *SetProperty)
{
    CASCAMCASTableClass *pTemp = NULL;
    if(NULL == SetProperty)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    if(SetProperty->cas_flag < CASCAM_TOTAL)
    {
        if((pTemp = _cas_find(thiz_CAS[SetProperty->cas_flag])))
        {
            if(pTemp->CASBlock_class.set_property)
            {
                return pTemp->CASBlock_class.set_property(SetProperty->main_id, SetProperty->sub_id, SetProperty->data_buf, SetProperty->buf_len);
            }
        }
        else
        {
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
            return -1;
        }
    }
    else
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    return 0;
}

int cascam_get_property(CASCAMDataGetClass *GetProperty)
{
    CASCAMCASTableClass *pTemp = NULL;
    if(NULL == GetProperty)
    {
        return -1;
    }
    if(GetProperty->cas_flag < CASCAM_TOTAL)
    {
        if((pTemp = _cas_find(thiz_CAS[GetProperty->cas_flag])))
        {
            if(pTemp->CASBlock_class.get_property)
            {
                return pTemp->CASBlock_class.get_property(GetProperty->main_id, GetProperty->sub_id, GetProperty->data_buf, GetProperty->buf_len);
            }
        }
        else
        {
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
            return -1;
        }
    }
    else
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    return 0;
}

// base info

// export for app
int CASCAM_Init(CASCAMInitParaClass Paras)
{
    int i = 0;
    int flags = 0;
    int ret = 0;

    if(0 == Paras.CASFlag)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
// init the data
    for(i = 0; i < CASCAM_MAX_CAS; i++)
    {
	    // add cas to table
        if((Paras.CASFlag & (1 << i)) != 0)
        {
            ret = _add_cas(thiz_CAS[i]);
            if(ret >= 0)
            {
                flags |= ret;
            }
            else
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
            }
        }
    }

	// initialize the cas soft
	cascam_init();
	return 0;
}

int CASCAM_Change_Program(CASCAMProgramParaClass *pProgPara)
{
	return cascam_change_prog(pProgPara);
}

int CASCAM_Release_Demux(CASCAMDemuxFlagClass *pFlags)
{
	return cascam_release_demux(pFlags);
}

int CASCAM_Start_Demux(CASCAMDemuxFlagClass *pFlags, unsigned short pid)
{
	return cascam_start_demux(pFlags, pid);
}

int CASCAM_Change_Frequence(void)
{
	return cascam_change_frequence();
}

int CASCAM_Set_Zone(double zone)
{
	return cascam_set_zone(zone);
}

int CASCAM_Register_CAS(CASCAMControlBlockClass *CAS)
{
    return _cas_register(CAS);
}

int CASCAM_Get_CAS_ID(unsigned int **ppID, unsigned char *pCount)
{
    return cascam_get_cas_id(ppID, pCount);
}

int CASCAM_Is_Vaild_CAS(unsigned short CASID)
{
    return cascam_is_vaild_cas(CASID);
}

int CASCAM_Get_Property(CASCAMDataGetClass *GetProperty)
{
   return cascam_get_property(GetProperty);
}

int CASCAM_Set_Property(CASCAMDataSetClass *SetProperty)
{
   return cascam_set_property(SetProperty);
}
// for osd
int CASCAM_Do_Event(CASCAMOSDEventClass *para)
{//
    int ret = 0;
    if(NULL == para)
    {
        return -1;
    }

    switch(para->type)
    {
        case CASCAM_OSD_DRAW:
            ret = cascam_module_osd_draw(para->data);
            break;
        //case CASCAM_OSD_GET_ROLLING_TIMES:
        //    ret = cascam_module_osd_rolling_times(para->data);
        //    break;
        case CASCAM_OSD_TRANSFER_STR:
            ret = cascam_module_osd_transfer_message_exec(para->data);
            break;
        case CASCAM_OSD_EMAIL_FULL:
            break;
        case CASCAM_OSD_EMAIL_NEW:
            break;
        default:
            break;
    }
    return ret;
}

void CASCAM_Osd_Clean(void)
{
    cascam_osd_exit_dialog1();
    cascam_osd_exit_dialog2();
    cascam_osd_exit_dialog3();
    cascam_osd_exit_dialog4();
    cascam_osd_exit_dialog5();
    //cascam_osd_exit_dialog6(-1);
}

// base information
//
