/*****************************************************************************
*             CONFIDENTIAL                              
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009-2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :   app_pmt.c
* Author    :   shenbin
* Project   :   GoXceed -S
******************************************************************************
* Purpose   :   
******************************************************************************
* Release History:
  VERSION   Date              AUTHOR         Description
   0.0      2010.4.14              shenbin         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "app_module.h"
#include "module/si/si_pmt.h"
#include "gxsi.h"
#include "gxmsg.h"
#include "app_send_msg.h"
#include "module/config/gxconfig.h"
#include "app_default_params.h"
//#include "module/app_ca_manager.h"
//#include "cim_includes.h"
#include "gxcore.h"
#define NEQ_PARSE_TIME_MS    (24*60*60*1000UL)
#define PMT_DATA_FIFO_NUM    (1)

static handle_t s_pmt_fifo = -1;
static handle_t s_pmt_thread_handle = -1;
static handle_t s_pmt_mutex = -1;

extern void app_subt_change(void);
extern void app_track_set(PmtInfo* audio);
extern void app_audio_free(void);
/* Private functions ------------------------------------------------------ */
//#ifdef CI_ENABLE

uint8_t *s_pmt_sec_data = NULL;


#define MAX_AUDIO_TRACK     	(32)
#define MAX_CAS_COUNT     		(16)
#define MAX_MUIVIDEO_COUNT     	(6)
#define MAX_AUDIO_DES_COUNT     (6)
#define MPEG1_VIDEO_APP     	(0x01)
#define MPEG2_VIDEO_APP     	(0x02)
#define MPEG2_AUDIO1_APP		(0x03)
#define MPEG2_AUDIO2_APP		(0x04)
#define AAC_AUDIO1_APP 			(0x0f)
#define AAC_AUDIO2_APP          (0x11)
#define H264_VIDEO_APP      	(0x1b)
#define MPEG4_VIDEO_APP			(0x10)
#define	EAC3_AUDIO_APP			(0x06)
#define	LPCM_AUDIO_APP			(0x80)
#define	AC3_AUDIO_APP			(0x81)
#define	DTS_AUDIO_APP			(0x82)
#define	DOLBY_TRUEHD_AUDIO_APP  (0x83)
#define	AC3_PLUS_AUDIO_APP		(0x84)
#define	DTS_HD_AUDIO_APP		(0x85)
#define	DTS_MA_AUDIO_APP		(0x86)
#define	AC3_PLUS_SEC_AUDIO_APP  (0xa1)
#define	DTS_HD_SEC_AUDIO_APP	(0xa2)

//#define MULIFEED_PMT_ENABLE
#define MULIFEED_TODATA(data1,data2) 		 		((data1 & 0x1f)<< 8 ) |data2


typedef struct app_stream_info
{
	uint8_t stream_type;
	uint8_t descrip_tag;
	int16_t es_info_length;
}app_stream_info_t;

typedef struct video_info
{
	int8_t video_count;
	int8_t video_type;
	uint16_t video_id;
}video_info_t;

typedef struct audio_info
{
	uint16_t ad_audio_id;
	int8_t audio_count;
	int8_t audio_type;
	int8_t audio_lang_type;
	char iso_lang[4];
}audio_info_t;


typedef struct pmt_stream_para
{
	int16_t ac3_count;
	int16_t ca_count;
	int16_t cas_id[MAX_CAS_COUNT];
	audio_info_t audio_descriptor[MAX_AUDIO_DES_COUNT];
	app_stream_info_t descrip_info[MAX_AUDIO_TRACK]; 
	video_info_t v_info[MAX_MUIVIDEO_COUNT];
}pmt_info_stream_t;

static pmt_info_stream_t gs_PmtInfoPara;


static void app_pmt_thread(void* arg)
{
    static uint8_t section_data[sizeof(PmtInfo)] = {0};
    while(1)
    {
        if(app_fifo_read(s_pmt_fifo, (char*)section_data, sizeof(PmtInfo)) > 0)
        {
            g_AppPmt.analyse(&g_AppPmt, section_data);
        }
    }
}

int app_get_mult_audio_type(int mode, int *pRetIndex)
{
	int i = 0;
	if((mode >= MAX_AUDIO_TRACK)||(mode < 0) || (pRetIndex == NULL))
	{
		mode = 0;
	}
	for(i = mode; i < MAX_AUDIO_TRACK; i++)
	{
		if(gs_PmtInfoPara.descrip_info[i].stream_type == 0x6)
		{
			*pRetIndex = i;
			return gs_PmtInfoPara.descrip_info[i].descrip_tag;
		}
	}
	return -1;
}

int app_get_video_type_from_pmt(int sel)
{
	if(sel >= MAX_MUIVIDEO_COUNT)
	{
		return 0;
	}
	
	return gs_PmtInfoPara.v_info[sel].video_type;
}

int app_get_ac3_count_num(void)
{
	int ret_value = 0;

	ret_value = gs_PmtInfoPara.ac3_count;

	return ret_value;
}


uint16_t app_get_video_pid_status(int number,int *count)
{
	if((number >= MAX_MUIVIDEO_COUNT)||(number < 0))
	{
		number = 0;
	}

	if(number >gs_PmtInfoPara.v_info[0].video_count)
	{
		*count = 1;
		return 0;
	}

	*count = gs_PmtInfoPara.v_info[0].video_count;

	return gs_PmtInfoPara.v_info[number].video_id;

}

int app_get_ad_pid_and_type(int* ad_type,int *ad_pid)
{
	int index = 0;

	for(index=0; index < MAX_AUDIO_DES_COUNT; index++)
	{
		if((gs_PmtInfoPara.audio_descriptor[index].audio_lang_type==2)
			||(gs_PmtInfoPara.audio_descriptor[index].audio_lang_type==3))
		{
			*ad_type = gs_PmtInfoPara.audio_descriptor[index].audio_type;
			*ad_pid = gs_PmtInfoPara.audio_descriptor[index].ad_audio_id;
			break;
		}
	}

	return gs_PmtInfoPara.audio_descriptor[index].audio_lang_type;
}

static int app_play_audio_descriptor_program(void)
{
#ifdef AUDIO_DESCRIPTOR_SUPPORT
	int32_t ad_mode = 1;
	int ad_type = 0;
	int ad_pid = 0;
	int audio_status = 0;
	GxMsgProperty_PlayerAdAudio ad_audio = {0};
	
	ad_audio.player = PLAYER_FOR_NORMAL;

	GxBus_ConfigGetInt(AD_CONF_KEY, &ad_mode, AD_CONFIG);
	if(ad_mode)
	{
		audio_status = app_get_ad_pid_and_type(&ad_type,&ad_pid);
		if((audio_status == 2)||(audio_status == 3))
		{
			ad_audio.pid = ad_pid;
			ad_audio.type = ad_type;
			app_send_msg_exec(GXMSG_PLAYER_AD_AUDIO_ENABLE,(void*)&ad_audio);
		}
	}
	else
	{
		//app_send_msg_exec(GXMSG_PLAYER_AD_AUDIO_DISABLE,(void*)&ad_audio);
	}
#endif

	return 0;
}

int app_get_muli_audio_pid(int index,int *count)
{
	if(index >= MAX_AUDIO_DES_COUNT) 
	{
		index = 0;
	}

	*count = gs_PmtInfoPara.audio_descriptor[0].audio_count;

	return gs_PmtInfoPara.audio_descriptor[index].ad_audio_id ;

}

int app_pmt_get_muli_audio_type(int index)
{
	if(index >= MAX_AUDIO_DES_COUNT) 
	{
		index = 0;
	}

	return gs_PmtInfoPara.audio_descriptor[index].audio_type;
}

char *app_pmt_get_audio_lang_str(uint32_t index)
{
	char *str = NULL;
	str = (char *)gs_PmtInfoPara.audio_descriptor[index].iso_lang;
    return str;
}


int app_get_ca_system_pid_status(int mode,int *count)
{
	if((mode >= MAX_AUDIO_TRACK) || (mode < 0))
	{
		mode = 0;
	}

	if(mode >gs_PmtInfoPara.ca_count)
	{
		*count = 1;
		return 0;
	}

	*count = gs_PmtInfoPara.ca_count;

	return gs_PmtInfoPara.cas_id[mode];

}

static bool check_cas_pid(int16_t cas_id )
{
	int16_t j;

	for(j=0;j<gs_PmtInfoPara.ca_count;j++)
	{
		if(gs_PmtInfoPara.cas_id[j] == cas_id)
		{
			return FALSE;
		}
	}

	return TRUE;
}

static bool check_vide_pid(int16_t video_id )
{
	int16_t j;
	for(j=0;j<gs_PmtInfoPara.v_info[0].video_count;j++)
	{
		if(gs_PmtInfoPara.v_info[j].video_id== video_id)
		{
			return FALSE;
		}
	}
	return TRUE;
}

static bool check_audio_pid(int16_t audio_id )
{
	int16_t j;
	for(j=0;j<gs_PmtInfoPara.v_info[0].video_count;j++)
	{
		if(gs_PmtInfoPara.audio_descriptor[j].ad_audio_id== audio_id)
		{
			return FALSE;
		}
	}
	return TRUE;
}



static int app_audio_update_pmt(uint8_t *pPmtSection, int MaxLength)
{
    uint8_t *pPmt = NULL;
    int16_t Length = 0 , temp1 = 0, temp2 = 0,temp3 = 0, Flag = 0, Count = 0, IsVedio = 0, offset = 0;
    int16_t ca_num = 0;
    int16_t cas_id = 0;
    int16_t audio_num = 0;
    int16_t len_section = 0;
    int16_t len_sel = 0;
    uint8_t *pBuf = NULL;

    uint16_t video_pid = 0;
    uint16_t video_num = 0;


    if(NULL == pPmtSection)
    {
        printf("\n[Parameter Err] %s,%d\n",__FILE__,__LINE__);
        return -1;
    }

    pPmt = pPmtSection;
    Length = ((int16_t)((pPmtSection[10]<<8)|(pPmtSection[11])))&0x0FFF;
    if(Length > 1024)
    {
        printf("\n[Length Err] %s,%d\n",__FILE__,__LINE__);
        return -1;
    }
    pPmt = pPmt + 12;

    gs_PmtInfoPara.ac3_count = 0;
    gs_PmtInfoPara.ca_count =0;
    memset(&gs_PmtInfoPara, 0, sizeof(pmt_info_stream_t));

    while(Length > 0)
    {
        if((*pPmt == 0x09))// tag
        {// only for CA
            temp1 = *(pPmt+1);// CA descriptor length without the TAG and the length
            if(temp1 >= 4)
            {// find the CA data // common
                temp1 += 2;// CA descriptor length
                if(offset > MaxLength)
                {
                    //printf("\n[Parameter Err] %s,%d\n",__FILE__,__LINE__);
                    return -1;
                }
                Flag |= (1 << (Count++));
                offset += temp1;
                cas_id =  ((int16_t)pPmt[2]<<8|pPmt[3])&0xFFFF;
                if((check_cas_pid(cas_id))&&(ca_num<MAX_CAS_COUNT))
                {
                    gs_PmtInfoPara.cas_id[ca_num]= cas_id;
                    ca_num++;
                    gs_PmtInfoPara.ca_count++;

                }
#if 0
                printf("\n**********************[CA DATA]****************************\n");
                for(j = 0; j < temp1;j++)
                {
                    printf("%02x ",pPmt[j]);
                    if(((j + 1)%16) == 0)
                        printf("\n");
                }
                printf("\n**************************[END]****************************\n");
#endif
            }
        }
        pPmt++;
        Length -= *pPmt + 2;
        pPmt += *pPmt;
        pPmt++;
    }

    //OffsetCA1 = offset;
    temp1 = ((int16_t)((pPmtSection[10]<<8)|(pPmtSection[11])))&0x0FFF; // prog info length
    temp2 = (((int16_t)pPmtSection[1]<<8)|pPmtSection[2])&0x0FFF;// pmt section length

    pPmt = pPmtSection + 12 + temp1;// go to stream type
    // section length - program info length - offset[between section length to prog info length] - CRC
    Length = temp2 - (temp1 + 9 + 4);
    while((Length > 0) && (audio_num < MAX_AUDIO_TRACK))
    {
        // FOR STREAM -- AUDIO OR VIDEO
        IsVedio = -1;
        switch(pPmt[0])
        {
            case MPEG1_VIDEO_APP://MPEG1_VIDEO:
            case MPEG2_VIDEO_APP://MPEG2_VIDEO:
            case H264_VIDEO_APP://H264_VIDEO:
            case MPEG4_VIDEO_APP://MPEG4_VIDEO:
                if((offset + 3) > MaxLength)
                {
                    //printf("\n[Parameter Err] %s,%d\n",__FILE__,__LINE__);
                    return -1;
                }
                offset +=3;
                IsVedio = 1;

                video_pid = (uint16_t)MULIFEED_TODATA( pPmt[1],pPmt[2]);
                if(check_vide_pid(video_pid))
                {
                    if(gs_PmtInfoPara.v_info[0].video_count < MAX_MUIVIDEO_COUNT)
                    {
                        if((pPmt[0]==MPEG1_VIDEO_APP)||(pPmt[0]==MPEG2_VIDEO_APP))
                        {
                            gs_PmtInfoPara.v_info[video_num].video_type = GXBUS_PM_PROG_MPEG;
                        }
                        else if((pPmt[0]==H264_VIDEO_APP)||(pPmt[0]==MPEG4_VIDEO_APP))
                        {
                            gs_PmtInfoPara.v_info[video_num].video_type= GXBUS_PM_PROG_H264;
                        }
                        gs_PmtInfoPara.v_info[video_num].video_id= video_pid;
                        gs_PmtInfoPara.v_info[0].video_count ++;//((int16_t)pPmt[2]<<8|pPmt[3])&0xFFFF;//(((int16_t)((*(pPmt+1)++)&0x1f))<<8) + *pPmt++;
                        video_num++;
                    }
                    else
                    {
                        audio_num = MAX_AUDIO_TRACK+1;
                    }
                }

                break;

            case MPEG2_AUDIO1_APP:
            case MPEG2_AUDIO2_APP:
            case AAC_AUDIO1_APP:
            case AAC_AUDIO2_APP:
            case LPCM_AUDIO_APP:
            case AC3_AUDIO_APP:
            case DOLBY_TRUEHD_AUDIO_APP:
            case AC3_PLUS_AUDIO_APP:
            case AC3_PLUS_SEC_AUDIO_APP:
				{
				uint32_t iso_audio_type = 0;
				uint16_t audio_pid = 0;
				
                if((offset + 3) > MaxLength)
                {
                    printf("\n[Parameter Err] %s,%d\n",__FILE__,__LINE__);
                }
                offset +=3;
				audio_pid = (uint16_t)MULIFEED_TODATA( pPmt[1],pPmt[2]);

                gs_PmtInfoPara.descrip_info[audio_num].stream_type= pPmt[0];
                gs_PmtInfoPara.descrip_info[audio_num].es_info_length= pPmt[4];
                gs_PmtInfoPara.descrip_info[audio_num].descrip_tag= pPmt[5];
				pBuf = pPmt;
				

				if((gs_PmtInfoPara.audio_descriptor[0].audio_count < MAX_AUDIO_DES_COUNT)
					&&(check_audio_pid(audio_pid)))
                {
                    if(pPmt[0]==MPEG2_AUDIO1_APP)
                    {
                        gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_MPEG1;
                    }
					else if(pPmt[0]==MPEG2_VIDEO_APP)
					{
						gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_MPEG2;
					}
                    else if(pPmt[0]==AAC_AUDIO1_APP)
                    {
                        gs_PmtInfoPara.audio_descriptor[audio_num].audio_type  = GXBUS_PM_AUDIO_AAC_ADTS;
                    }
					else if(pPmt[0]==AAC_AUDIO2_APP)
                    {
                        gs_PmtInfoPara.audio_descriptor[audio_num].audio_type  = GXBUS_PM_AUDIO_AAC_LATM;
                    }
					else if(pPmt[0]==LPCM_AUDIO_APP)
                    {
                        gs_PmtInfoPara.audio_descriptor[audio_num].audio_type  = GXBUS_PM_AUDIO_LPCM;
                    }
                    else if(pPmt[0]==AC3_AUDIO_APP)
                    {
                        gs_PmtInfoPara.audio_descriptor[audio_num].audio_type= GXBUS_PM_AUDIO_AC3;
                    }
					else if(pPmt[0]==DOLBY_TRUEHD_AUDIO_APP)
                    {
                        gs_PmtInfoPara.audio_descriptor[audio_num].audio_type= GXBUS_PM_AUDIO_DOLBY_TRUEHD;
                    }
					else if(pPmt[0]==AC3_PLUS_AUDIO_APP)
                    {
                        gs_PmtInfoPara.audio_descriptor[audio_num].audio_type= GXBUS_PM_AUDIO_AC3_PLUS;
                    }
					else if(pPmt[0]==AC3_PLUS_SEC_AUDIO_APP)
                    {
                        gs_PmtInfoPara.audio_descriptor[audio_num].audio_type= GXBUS_PM_AUDIO_AC3_PLUS_SEC;
                    }
					len_section = ((int16_t)pPmt[3]<<8|pPmt[4])&0x0FFF;
	                len_sel = len_section;
	                pBuf += 5;
	                while(len_sel > 0)//descriptor
	                {
	                    if((*pBuf == 0x0a))// tag
	                    {
							pBuf += 2;
							memcpy(gs_PmtInfoPara.audio_descriptor[audio_num].iso_lang, pBuf, 3);
							pBuf += 3;
							iso_audio_type= *pBuf;
	                    }
	                    pBuf++;
	                    len_sel -= *pBuf + 2;
	                    pBuf += *pBuf;
	                    pBuf++;
	                }
					gs_PmtInfoPara.audio_descriptor[audio_num].audio_lang_type= iso_audio_type;
					gs_PmtInfoPara.audio_descriptor[audio_num].ad_audio_id=audio_pid ;
	                gs_PmtInfoPara.audio_descriptor[0].audio_count ++;
	                audio_num++;
	              }
                
             
            	}
                break;

            case EAC3_AUDIO_APP:
                if((offset + 3) > MaxLength)
                {
                    printf("\n[Parameter Err] %s,%d\n",__FILE__,__LINE__);
                    return -1;
                }
                offset +=3;
                pBuf = pPmt;
                len_section = ((int16_t)pPmt[3]<<8|pPmt[4])&0x0FFF;
                len_sel = len_section;
                pBuf += 5;
                while(len_sel > 0)//descriptor
                {
                    if((*pBuf == 0x0a))// tag
                    {
                    }
                    else if((*pBuf == 0x6a) || (*pBuf == 0x7a))
                    {
                        gs_PmtInfoPara.descrip_info[audio_num].stream_type = pPmt[0];
                        gs_PmtInfoPara.descrip_info[audio_num].es_info_length = pPmt[4];
                        gs_PmtInfoPara.descrip_info[audio_num].descrip_tag= *pBuf;
                        audio_num++;
                        gs_PmtInfoPara.ac3_count++;
                    }
                    pBuf++;
                    len_sel -= *pBuf + 2;
                    pBuf += *pBuf;
                    pBuf++;
                }
                IsVedio = 0;
                break;

            default:
                break;
        }

        if (IsVedio != -1)
        {
            if((offset + 2) > MaxLength)
            {
                printf("\n[Parameter Err]%s,%d\n",__FILE__,__LINE__);
                return -1;
            }
            offset += 2;
        }
        // for CA
        temp3 = ((int16_t)pPmt[3]<<8|pPmt[4])&0x0FFF;
        temp2 = temp3;
        pPmt += 5;
        while(temp2 > 0)// for ca descriptor
        {
            if((*pPmt == 0x09))// tag
            {// only for CA
                temp1 = *(pPmt+1);// CA descriptor length without the TAG and the length
                if(temp1 >= 4)
                {// find the CA data // common
                    temp1 += 2;// CA descriptor length

                    if((offset + temp1) > MaxLength)
                    {
                        printf("\n[Parameter Err] %s,%d\n",__FILE__,__LINE__);
                        return -1;
                    }
                    offset += temp1;

                    cas_id =  ((int16_t)pPmt[2]<<8|pPmt[3])&0xFFFF;
                    if((check_cas_pid(cas_id))&&(ca_num<MAX_CAS_COUNT))
                    {
                        gs_PmtInfoPara.cas_id[ca_num]= cas_id;
                        gs_PmtInfoPara.ca_count++;
                        ca_num++;
                    }
#if 0//def _CA_DEBUG_
                    printf("\n**********************[CA2 DATA]****************************\n");
                    for(j = 0; j < temp1;j++)
                    {
                        printf("%02x ",pPmt[j]);
                        if(((j + 1)%16) == 0)
                            printf("\n");
                    }
                    printf("\n**************************[END]*****************************\n");
#endif
                    // for length
                }

            }
            pPmt++;
            temp2 -= *pPmt + 2;
            pPmt += *pPmt;
            pPmt++;
        }
        Length -= (temp3+5);
    }

    return offset;
}


static private_parse_status pmt_private_function(uint8_t *p_section_data, uint32_t len)
{
    if(GxCore_MutexTrylock(s_pmt_mutex) == GXCORE_SUCCESS)
    {
        if(s_pmt_sec_data != NULL)
        {
            GxCore_Free(s_pmt_sec_data);
            s_pmt_sec_data = NULL;
        }
        if((s_pmt_sec_data = (uint8_t*)GxCore_Calloc(1, len)) != NULL)
        {
            memcpy(s_pmt_sec_data, p_section_data, len);
        }
        GxCore_MutexUnlock(s_pmt_mutex);
    }
#if 0 //pmt data log
    int i = 0;
    printf("\n**************************** PMT DATE len = %d******************************\n", len);
    for(i = 0; i < len; i++)
    {
        printf("%02x ",p_section_data[i]);
        if(((i + 1)%16) == 0)
            printf("\n");
    }
    printf("\n***************************************************************\n");
#endif

#ifdef CI_ENABLE
    cim_update_pmt(p_section_data, UPDATE_PMT_CIM_0);
#endif

    app_audio_update_pmt(p_section_data,len);
    return PRIVATE_SUBTABLE_OK;
}
//#endif

/* Exported functions ----------------------------------------------------- */
// return release pmt pid, error return 0
static uint32_t app_pmt_stop(AppPmt *pmt)
{
	app_audio_free();

#if CASCAM_SUPPORT
{
    CASCAMDemuxFlagClass flag;
    flag.Flags = DEMUX_ECM;
    CASCAM_Release_Demux(&flag);
}
#endif

#if ECM_SUPPORT > 0	  
	g_AppEcm.stop(&g_AppEcm);
#endif
	// for ttx subtitle
	g_AppTtxSubt.ttx_pre_process_close(&g_AppTtxSubt);
	g_AppTtxSubt.clean(&g_AppTtxSubt);
	
    GxCore_MutexLock(s_pmt_mutex);
    pmt->ver = 0xff;
    if(s_pmt_sec_data != NULL)
    {
        GxCore_Free(s_pmt_sec_data);
        s_pmt_sec_data = NULL;
    }
    if (pmt->subt_id != -1)
    {
        GxSubTableDetail subt_detail = {0};
        GxMsgProperty_SiGet si_get;
        GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)pmt->subt_id;

        subt_detail.si_subtable_id = pmt->subt_id;
        si_get = &subt_detail;
        app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void*)&si_get);

        app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
        pmt->subt_id = -1;
        GxCore_MutexUnlock(s_pmt_mutex);		
        return subt_detail.si_filter.pid;
    }
    GxCore_MutexUnlock(s_pmt_mutex);	
    return 0;
}

// ts_src TS1-0 TS2-1 TS3-2...
static void app_pmt_start(AppPmt *pmt)
{
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_NodeByPosGet node;
    GxMsgProperty_SiCreate  params_create;
    GxMsgProperty_SiStart params_start;

    if(s_pmt_mutex == -1)
    {
        GxCore_MutexCreate(&s_pmt_mutex);
    }

    if(s_pmt_fifo == -1)
    {
        app_fifo_create(&s_pmt_fifo, sizeof(PmtInfo), PMT_DATA_FIFO_NUM);
        if(s_pmt_thread_handle == -1)
        {
            GxCore_ThreadCreate("app_pmt_proc", &s_pmt_thread_handle, app_pmt_thread, NULL, 10*1024, GXOS_DEFAULT_PRIORITY);
        }
    }

    if (pmt->subt_id != -1)
    {
        pmt->stop(pmt);
    }

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&node);

    pmt->pid = node.prog_data.pmt_pid;

    // create pmt filter
    subt_detail.ts_src = (uint16_t)pmt->ts_src;

    AppFrontend_Config config = {0};
    app_ioctl(subt_detail.ts_src, FRONTEND_CONFIG_GET, &config);

    subt_detail.demux_id = config.dmx_id;// TODO: 20120416

    // add in 20121212
#if CA_SUPPORT
    app_demux_param_adjust(&subt_detail.ts_src, &subt_detail.demux_id, 0);
#endif
    subt_detail.si_filter.pid = (uint16_t)pmt->pid;
    subt_detail.si_filter.match_depth = 5;
    subt_detail.si_filter.eq_or_neq = EQ_MATCH;
    subt_detail.si_filter.match[0] = PMT_TID;
    subt_detail.si_filter.mask[0] = 0xff;
    subt_detail.si_filter.match[3] = (node.prog_data.service_id >> 8);
    subt_detail.si_filter.mask[3] = 0xff;
    subt_detail.si_filter.match[4] = (node.prog_data.service_id & 0xff);
    subt_detail.si_filter.mask[4] = 0xff;
    subt_detail.si_filter.soft_filter = SOFT_ON;
    subt_detail.time_out = 5000000;

#if 1
    subt_detail.table_parse_cfg.mode = PARSE_WITH_STANDARD;
    subt_detail.table_parse_cfg.table_parse_fun = pmt_private_function;
#else
    subt_detail.table_parse_cfg.mode = PARSE_STANDARD_ONLY;
    subt_detail.table_parse_cfg.table_parse_fun = NULL;
#endif

    GxCore_MutexLock(s_pmt_mutex);

    if(s_pmt_sec_data != NULL)
    {
        GxCore_Free(s_pmt_sec_data);
        s_pmt_sec_data = NULL;
    }

    params_create = &subt_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);

    // get the return "si_subtable_id"
    pmt->subt_id = subt_detail.si_subtable_id;
    pmt->req_id = subt_detail.request_id;

    params_start = pmt->subt_id;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);

    pmt->ver = 0xff;
    GxCore_MutexUnlock(s_pmt_mutex);

// recoder 
    pmt->dmx_id = subt_detail.demux_id;
#if OTA_MONITOR_SUPPORT
{
    GxMsgProperty_NodeByIdGet nodeId;
    int type = -1;
    nodeId.node_type = NODE_SAT;
    nodeId.id = node.prog_data.sat_id;
    if(GXCORE_ERROR != app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void*)&nodeId))
    {
        if(nodeId.sat_data.type == GXBUS_PM_SAT_S)
        {
            type = 0;//OTA_QPSK;
        }
        else if(nodeId.sat_data.type == GXBUS_PM_SAT_C)
        {// TODO: not support now
            //type = OTA_QAM;
        }
        else if((nodeId.sat_data.type == GXBUS_PM_SAT_T)
                || (nodeId.sat_data.type == GXBUS_PM_SAT_DVBT2))
        {// TODO: not support now
            //type = OTA_OFDM;
        }
        else if(nodeId.sat_data.type == GXBUS_PM_SAT_DTMB)
        {// TODO: not support now
            //type = OTA_DTMB;
        }
        else if(nodeId.sat_data.type == GXBUS_PM_SAT_ATSC_C)
        {// TODO: not support

        }
        else if(nodeId.sat_data.type == GXBUS_PM_SAT_ATSC_T)
        {// TODO: not support

        }
    }
}
#endif
}

static status_t app_pmt_get(AppPmt *pmt, GxMsgProperty_SiSubtableOk *data)
{
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiGet params_get = NULL;
    GxMsgProperty_SiModify params_modify = NULL;
    PmtInfo *pmt_info = (PmtInfo*)data->parsed_data;

    if (data->si_subtable_id == pmt->subt_id
            && data->table_id == PMT_TID
            && data->request_id == pmt->req_id)
    {
        APP_PRINT("app_pmt_get ver orgi: %d   cur: %d\n", pmt->ver, pmt_info->si_info.ver_number);
        if(pmt->ver != pmt_info->si_info.ver_number)
        {
            app_fifo_write(s_pmt_fifo, (char*)data->parsed_data, sizeof(PmtInfo));
            GxCore_MutexLock(s_pmt_mutex);
            pmt->ver = pmt_info->si_info.ver_number;
            if(pmt->subt_id != -1)
            {
                // get original pmt info
                subt_detail.si_subtable_id = pmt->subt_id;
                params_get = &subt_detail;
                app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void*)(&params_get));

                // change to unmatch filter
                subt_detail.si_filter.match_depth = 6;
                subt_detail.si_filter.eq_or_neq = NEQ_MATCH;
                subt_detail.si_filter.match[5] = pmt->ver<<1;
                subt_detail.si_filter.mask[5] = 0x3e;
                subt_detail.time_out = NEQ_PARSE_TIME_MS;

                params_modify = &subt_detail;
                app_send_msg_exec(GXMSG_SI_SUBTABLE_MODIFY, (void*)&params_modify);
            }
            GxCore_MutexUnlock(s_pmt_mutex);
        }

        GxBus_SiParserBufFree(data->parsed_data);
        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

// test
#if CASCAM_SUPPORT
static unsigned int thiz_ecm_count = 0;
#define MAX_ECM_CAS     12
typedef struct _ECMInfoClass{
    unsigned short pid;
    unsigned short cas_pid[MAX_ECM_CAS];
    unsigned short scrambler_type[MAX_ECM_CAS];
    unsigned int   cas_count;
}ECMInfoClass;
static ECMInfoClass thiz_ecm_info[MAX_ECM_CAS];
#define _COMMON 0
#define _VIDEO  1
#define _AUDIO  2
#define _TTX    3
#define _SUBT   4

static int _check_ECMINFO(int32_t ECMPID,int32_t CASPID,int32_t DescrambType)
{
	uint32_t i;
	uint32_t j;
	if(ECMPID == 0x1fff)
	{
		APP_PRINT("---[PARAMETER ERRER]\n");
		return FALSE;
	}
	for(i=0;i<thiz_ecm_count;i++)
	{
		if(thiz_ecm_info[i].pid == ECMPID)
		{
			for(j=0;j<thiz_ecm_info[i].cas_count;j++)
			{
				if(thiz_ecm_info[i].cas_pid[j] == CASPID)
				{
					if(thiz_ecm_info[i].scrambler_type[j] != DescrambType)
						thiz_ecm_info[i].scrambler_type[j] = _COMMON;
					return FALSE;//same ecmpid & same casid & different scrambletype
				}
			}
			//add the new cassystem
			if(thiz_ecm_info[i].cas_count < MAX_ECM_CAS)
			{
				thiz_ecm_info[i].cas_pid[thiz_ecm_info[i].cas_count] = CASPID;
				thiz_ecm_info[i].scrambler_type[thiz_ecm_info[i].cas_count] = DescrambType;
				thiz_ecm_info[i].cas_count++;
			}
			else
				APP_PRINT("---[too many casid]\n");
			return FALSE;
		}
	}
	// add new ecmid
	if(thiz_ecm_count < MAX_ECM_CAS)
	{
		thiz_ecm_info[thiz_ecm_count].pid = ECMPID;
		thiz_ecm_info[thiz_ecm_count].cas_pid[0] = CASPID;
		thiz_ecm_info[thiz_ecm_count].scrambler_type[0] = DescrambType;
		thiz_ecm_info[thiz_ecm_count].cas_count++;
		thiz_ecm_count++;
	}
	else
		APP_PRINT("---[too many ecmid]\n");

	return TRUE;
}

static void _ecm_store_info(void *pInfo)
{
	int32_t i = 0;
	int32_t j = 0;
	PmtInfo* pmt_info = (PmtInfo*)pInfo;
	CaDescriptor *p = NULL;
	if(pmt_info == NULL)
	{
		APP_PRINT("---[parameter error]\n");
		return;
	}

    //clear the data of the ecm info
	thiz_ecm_count = 0;
    for (i=0; i < MAX_ECM_CAS; i++)
    {
        thiz_ecm_info[i].pid = 0;
		for (j=0; j< MAX_ECM_CAS; j++)
		{
			thiz_ecm_info[i].cas_pid[j] = 0;
			thiz_ecm_info[i].scrambler_type[j] = 0;
		}
		thiz_ecm_info[i].cas_count = 0;
    }
	// for ca des1
	if((pmt_info->ca_count != 0)&&(pmt_info->ca_info != NULL))
	{
		APP_PRINT("---[ca_count][%d]\n",pmt_info->ca_count);

		for(i = 0; i < pmt_info->ca_count; i++)
		{
			p = (CaDescriptor*)(pmt_info->ca_info[i]);
			APP_PRINT("---[ca_pid = %x]-[ca_system_id = %x]\n",p->ca_pid,p->ca_system_id);
			_check_ECMINFO(p->ca_pid,p->ca_system_id,_COMMON);
		}
	}

	// for ca des2
	if((pmt_info->ca_count2 != 0)&&(pmt_info->ca_info2 != NULL))
	{
		#define MPEG1_VIDEO     (0x01)
		#define MPEG2_VIDEO     (0x02)
		#define H264_VIDEO      (0x1b)
		#define H265_VIDEO      (0x24)
		#define AVS_VIDEO       (0x42)
		#define MPEG4_VIDEO     (0x10)
		#define MPEG2_AUDIO1    (0x03)
		#define MPEG2_AUDIO2    (0x04)
		#define AAC_AUDIO1      (0x0f)
		#define	AAC_AUDIO2		(0x11)
		#define A52_AUDIO       (0x81)
		uint32_t DescrambleType = _COMMON;
		uint32_t j = 0;
		APP_PRINT("---[ca_count2][%d]\n",pmt_info->ca_count2);
		
		for(i = 0; i < pmt_info->ca_count2; i++)
		{
			p = (CaDescriptor*)(pmt_info->ca_info2[i]);
			for(j = 0; j < pmt_info->stream_count; j++)
			{
				if(pmt_info->stream_info[j].elem_pid == p->elem_pid)
				{
					 if((MPEG1_VIDEO == pmt_info->stream_info[j].stream_type)
                        || (MPEG2_VIDEO == pmt_info->stream_info[j].stream_type)
                        || (H264_VIDEO == pmt_info->stream_info[j].stream_type)
                        || (H265_VIDEO == pmt_info->stream_info[j].stream_type)
                        || (AVS_VIDEO == pmt_info->stream_info[j].stream_type)
                        || (MPEG4_VIDEO == pmt_info->stream_info[j].stream_type))
                        DescrambleType = _VIDEO;
					 else if((MPEG2_AUDIO1 == pmt_info->stream_info[j].stream_type)
                        || (MPEG2_AUDIO2 == pmt_info->stream_info[j].stream_type)
                        || (AAC_AUDIO1 == pmt_info->stream_info[j].stream_type)
                        || (AAC_AUDIO2 == pmt_info->stream_info[j].stream_type)
                        || (A52_AUDIO == pmt_info->stream_info[j].stream_type))
                        DescrambleType = _AUDIO;
					 else
						APP_PRINT("---[can not support type]\n");
					 break;
				}
			}
            // TODO:20160301
            for(j = 0; j < pmt_info->ac3_count; j++)
            {
                Ac3Descriptor *temp = NULL;
                temp = (Ac3Descriptor*)(pmt_info->ac3_info[j]);
				if(temp->elem_pid == p->elem_pid)
                {
                    DescrambleType = _AUDIO;
                    break;
                }
            }
            for(j = 0; j < pmt_info->eac3_count; j++)
            {
                Eac3Descriptor *temp = NULL;
                temp = (Eac3Descriptor*)(pmt_info->eac3_info[j]);
				if(temp->elem_pid == p->elem_pid)
                {
                    DescrambleType = _AUDIO;
                    break;
                }
            }
            for(j = 0; j < pmt_info->ttx_count; j++)
            {
                TeletextDescriptor *temp = NULL;
                temp = (TeletextDescriptor*)(pmt_info->ttx_info[j]);
				if(temp->elem_pid == p->elem_pid)
                {
                    DescrambleType = _TTX;
                    break;
                }
            }
            for(j = 0; j < pmt_info->subt_count; j++)
            {
                SubtDescriptor *temp = NULL;
                temp = (SubtDescriptor*)(pmt_info->subt_info[j]);
				if(temp->elem_pid == p->elem_pid)
                {
                    DescrambleType = _SUBT;
                    break;
                }
            }
			//
			APP_PRINT("---[ca_pid2 = %x]-[ca_system_id2 = %x]-[DescrambleType = %d]\n",p->ca_pid,p->ca_system_id,DescrambleType);
			if(DescrambleType != _COMMON)
				_check_ECMINFO(p->ca_pid,p->ca_system_id,DescrambleType);
		}
	}

	//test
	APP_PRINT("---------start[%d]----------\n",thiz_ecm_count);
	for(i = 0; i<thiz_ecm_count;i++)
	{
		uint32_t j = 0;
		APP_PRINT("--ECMPID[%x]--CASCOUNT[%x]--\n",thiz_ecm_info[i].pid,thiz_ecm_info[i].cas_count);
		for(j = 0; j<thiz_ecm_info[i].cas_count;j++)
			APP_PRINT("--CASPID[%x]--TYPE[%d]--\n",thiz_ecm_info[i].cas_pid[j],thiz_ecm_info[i].scrambler_type[j]);
		APP_PRINT("\n---------end----------\n");
	}
	APP_PRINT("---[%s exit]\n", __FUNCTION__);

}
static int _get_ecm_data(PmtInfo *pmt_info, unsigned short *pVideoEcmPid, unsigned short *pAudioEcmPid, unsigned short *pCasPid)
{
    int i = 0;
    int j = 0;

    *pVideoEcmPid = 0x1fff;
    *pAudioEcmPid = 0x1fff;

    thiz_ecm_count = 0;
    memset(&thiz_ecm_info, 0, MAX_ECM_CAS*sizeof(ECMInfoClass));

    _ecm_store_info(pmt_info);
    for(i = 0; i < thiz_ecm_count; i++)
    {
       for(j = 0; j < thiz_ecm_info[i].cas_count; j++)
       {
            //if(CASCAM_Get_CAS_ID() == thiz_ecm_info[i].cas_pid[j])
            if(1 == CASCAM_Is_Vaild_CAS(thiz_ecm_info[i].cas_pid[j]))
            {
                if(thiz_ecm_info[i].scrambler_type[j] == _COMMON)
                {
                    *pVideoEcmPid = thiz_ecm_info[i].pid;
                    *pAudioEcmPid = thiz_ecm_info[i].pid;
                }
                else if(thiz_ecm_info[i].scrambler_type[j] == _VIDEO)
                {
                    *pVideoEcmPid = thiz_ecm_info[i].pid;
                }
                else if(thiz_ecm_info[i].scrambler_type[j] == _AUDIO)
                {
                    *pAudioEcmPid = thiz_ecm_info[i].pid;
                }
                *pCasPid = thiz_ecm_info[i].cas_pid[j];
            }
       }
    }
    return 0;
}
#endif

#if CASCAM_SUPPORT
static CASCAMProgramParaClass thiz_cascam_prog_info = {0};
void app_cascam_change_audio(unsigned short audio_pid)
{
    if(0x1fff == audio_pid)
    {
        return ;
    }
    thiz_cascam_prog_info.AudioPid = audio_pid;
    CASCAM_Change_Program(&thiz_cascam_prog_info); 
}
#endif

static status_t app_pmt_analyse(AppPmt *pmt, uint8_t *section_data)
{
    PmtInfo *pmt_info = (PmtInfo*)section_data;
    uint32_t i;

    APP_PRINT("app_pmt_get ver orgi: %d   cur: %d\n", pmt->ver, pmt_info->si_info.ver_number);
#if CASCAM_SUPPORT
{
        GxMsgProperty_NodeByPosGet prog_node;
        prog_node.node_type = NODE_PROG;
        prog_node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&prog_node);
       
        CASCAMProgramParaClass para;

        _get_ecm_data(pmt_info, &para.VideoECMPid, &para.AudioECMPid, &para.CasPid);
        
        para.DemuxID = 0;
        para.ScrambleFlag = prog_node.prog_data.scramble_flag;
        para.VideoPid = prog_node.prog_data.video_pid;
        para.AudioPid = prog_node.prog_data.cur_audio_pid;
        para.ServiceID = prog_node.prog_data.service_id;
        para.PMTPid = prog_node.prog_data.pmt_pid;
        //para.VideoECMPid = prog_node.prog_data.ecm_pid_v;
        //para.AudioECMPid = prog_node.prog_data.cur_audio_ecm_pid;
        //para.CasPid = prog_node.prog_data.cas_id;
        para.NetworkID = prog_node.prog_data.original_id;
        para.StreamID = prog_node.prog_data.ts_id;

        memcpy(&thiz_cascam_prog_info, &para, sizeof(CASCAMProgramParaClass));
        CASCAM_Change_Program(&para); 
        CASCAMDemuxFlagClass flag;
        flag.Flags = DEMUX_ECM;
        CASCAM_Start_Demux(&flag, 0);
}
#endif
#if CA_SUPPORT > 0
    GxMsgProperty_NodeByPosGet prog_node;

    prog_node.node_type = NODE_PROG;
    prog_node.pos = g_AppPlayOps.normal_play.play_count;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&prog_node))
    {
        app_extend_send_service(prog_node.prog_data.id,
                                prog_node.prog_data.sat_id,
                                prog_node.prog_data.tp_id,
                                pmt->dmx_id,
                                CONTROL_NORMAL);
    }
#endif

#if ECM_SUPPORT > 0
    GxMsgProperty_NodeByPosGet node;

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&node);
    g_AppEcm.DemuxId = pmt->dmx_id;//0;// TODO: 20120702
    g_AppEcm.CaManager.ProgId = node.prog_data.id;
    g_AppEcm.CaManager.Type = CONTROL_NORMAL;
    g_AppEcm.storeinfo((void*)pmt_info,&g_AppEcm);
    // add in 20121212
#if CA_SUPPORT
{
    uint16_t DemuxId = 0;
    app_demux_param_adjust(&g_AppEcm.ts_src, &DemuxId, 0);
    g_AppEcm.DemuxId = DemuxId;
}
#endif
    g_AppEcm.start(&g_AppEcm);
#endif

    g_AppTtxSubt.sync(&g_AppTtxSubt, pmt_info);
    // app function embeded module
    for(i = 0; i<pmt_info->stream_count; i++)
    {
        // only video start subt
        if((MPEG1_VIDEO_APP == pmt_info->stream_info[i].stream_type)
                || (MPEG2_VIDEO_APP == pmt_info->stream_info[i].stream_type)
                || (H264_VIDEO_APP == pmt_info->stream_info[i].stream_type))
        {
            app_subt_change();
        }
    }
	// restart the ttx ree-process, if APP support the pre-process function
	g_AppTtxSubt.ttx_pre_process_init(&g_AppTtxSubt);
	
    app_track_set(pmt_info);

	app_play_audio_descriptor_program();

    return GXCORE_SUCCESS;
}

AppPmt g_AppPmt =
{
    .subt_id    = -1,
    .req_id     = 0,
    .ver        = 0xff,
    .start      = app_pmt_start,
    .stop       = app_pmt_stop,
    .get        = app_pmt_get,
    .analyse    = app_pmt_analyse,
    .cb         = NULL,
};

/* End of file -------------------------------------------------------------*/


