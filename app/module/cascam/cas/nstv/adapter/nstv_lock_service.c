/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_lock_service.c
* Author    :   B.Z.
* Project   :	GoXceed
* Type      :   CA Module
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
VERSION	    Date			  AUTHOR         Description
1.0	      2016.04.22		   B.Z.			  creation
*****************************************************************************/
#include "../../../include/cas/cascam_nstv.h"
#include "../../../include/cascam_platform.h"
#include "../../../include/cascam_os.h"
#include "../../../include/cascam_osd.h"
#include "../../../include/cascam_types.h"
#include "../include/porting/CDCASS.h"

#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))
static unsigned int _clc_fre(unsigned long cd_fre_bcd)
{
    unsigned char high = 0;
    unsigned char low = 0;
    high = BCD2DEC((((cd_fre_bcd >> 24) & 0xff)));
    low = BCD2DEC(((cd_fre_bcd >> 16) & 0x00ff));
    return (high*100 + low);
}

static unsigned int _clc_sym(unsigned long cd_sym_bcd)
{
    unsigned char high = 0;
    unsigned short low = 0;
    high = BCD2DEC((((cd_sym_bcd >> 16) & 0x00ff)));
    low = BCD2DEC(((cd_sym_bcd >> 8) & 0xff))*10 + ((cd_sym_bcd >> 4) & 0x0f);
    return (high*1000 + low);
}

typedef struct _ProgInfoClass{
    unsigned short video_pid;
    unsigned short video_ecm_pid;
    unsigned char  video_code_type;
    unsigned short audio_pid;
    unsigned short audio_ecm_pid;
    unsigned char  audio_code_type;
}ProgInfoClass;

static unsigned int _deal_width_component(unsigned int count, SCDCAComponent *componet, ProgInfoClass* info)
{
    int ret = 0;
    int i = 0;
    int type = 0;
    int video_flag = 0;

    for(i = 0; i < count; i++)
    {
        ret = cascam_get_code_type(componet[i].m_CompType, &type, &video_flag);
        if(0 == ret)
        {
            if(video_flag > 0)
            {
                info->video_pid = componet[i].m_wCompPID;
                info->video_code_type = type;
                if(componet[i].m_wECMPID)
                {
                    info->video_ecm_pid = componet[i].m_wECMPID;
                }
                else
                {// not scrambler
                    info->video_ecm_pid = 0x1fff;
                }
            }
            else
            {
                info->audio_pid = componet[i].m_wCompPID;
                info->audio_code_type = type;
                if(componet[i].m_wECMPID)
                {
                    info->audio_ecm_pid = componet[i].m_wECMPID;
                }
                else
                {// not scrambler
                    info->audio_ecm_pid = 0x1fff;
                }
            }
        }
    }
    return ret;
}

static int _lock_notify(char flag, char* message)
{
    CASCAMOsdClass param = {0};
    param.notice_type = CASCAM_MESSAGE;
    param.property.message.main_id = NSTV_MESSAGE_LOCK_SERVICE;
    param.property.message.clean_flag = flag;
    param.property.message.message_data = message;
    cascam_modle_osd_exec(&param);
    return 0;
}

/* 频道锁定 */
//extern void CDSTBCA_LockService( const SCDCALockService* pLockService );
/* 解除频道锁定 */
//extern void CDSTBCA_UNLockService( void );

// param == NULL: unlock commond; param != NULL, lock common
static CASCAMNSTVLockServiceClass thiz_lock_service = {0};
void nstv_lock_service_control(void *param)
{
    SCDCALockService *info = NULL;
    ProgInfoClass av_info = {0};
    int ret = 0;
    if(NULL == param)
    {
        _lock_notify(1, NULL);
        //cascam_memset(&thiz_lock_service, 0, sizeof(CASCAMNSTVLockServiceClass));
    }
    else
    {
        info = (SCDCALockService*)param;
        if(/*(info->m_Modulation > 0) && (info->m_Modulation < 6) ||*/ (_clc_fre(info->m_dwFrequency) < 1000))
        {
            // qam, not dvbs/s2
            cascam_printf("\n\033[33m, cascam, error, %s, %d \033[0m\n", __func__,__LINE__);
            return ;

			// only for test
        //    info->m_dwFrequency = 0x40000000;
        //    info->m_symbol_rate = 0x275000;
        }
        //char FactoryPlayUrl[] = {"dvbs://fre:1030&polar:1&symbol:27500&22k:1&vpid:36&apid:35&pcrpid:36&vcodec:1&acodec:0&tuner:0&scramble:0"};
        thiz_lock_service.fre = _clc_fre(info->m_dwFrequency);
        thiz_lock_service.sym = _clc_sym(info->m_symbol_rate);
#if NSTV_AUTHENTICATION_SUPPORT
        thiz_lock_service.polar = 1;// 0 == 13v; 1 == 18v
#else
		thiz_lock_service.polar = 0;// 0 == 13v; 1 == 18v
#endif
        thiz_lock_service.pcr_pid = info->m_wPcrPid;
        ret = _deal_width_component(info->m_ComponentNum, info->m_CompArr, &av_info);
        if(ret == 0)
        {
            thiz_lock_service.video_pid = av_info.video_pid;
            thiz_lock_service.audio_pid = av_info.audio_pid;
            thiz_lock_service.video_type = av_info.video_code_type;//h.264
            thiz_lock_service.audio_type = av_info.audio_code_type;//mpeg2
            thiz_lock_service.video_ecm_pid = av_info.video_ecm_pid;
            thiz_lock_service.audio_ecm_pid = av_info.audio_ecm_pid;
            if((0x1fff == av_info.video_ecm_pid) && (0x1fff == av_info.audio_ecm_pid))
            {
                thiz_lock_service.scramble_flag = 0;//free
            }
            else
            {
                thiz_lock_service.scramble_flag = 1;
            }
            thiz_lock_service.force_flag = LOCK_SERVICE_FORCE;
            _lock_notify(0, (char*)&thiz_lock_service);
        }
        else
        {
            cascam_printf("\n\033[33m, cascam, error, %s, %d \033[0m\n", __func__,__LINE__);
        }
    }
}

