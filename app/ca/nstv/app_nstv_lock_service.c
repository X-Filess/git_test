/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_lock_service.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.04.24		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_config.h"
#include "app_cascam_pop.h"

#if NSCAS_SUPPORT

static char thiz_nstv_force_lock_status = 0;
int app_nstv_chaeck_force_lock_status(void)
{
    return thiz_nstv_force_lock_status;
}

static char thiz_lock_service_url[PLAYER_URL_LONG] = {0};
void app_nstv_lock_service_notify(char* param)
{
    CASCAMNSTVLockServiceClass *p = NULL;
    int ret = 0;

    if(NULL == param)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }
    p = (CASCAMNSTVLockServiceClass*)param;
    // check tp is exist or not
    GxBusPmDataSat sat_arry={0};
    unsigned short tp_id = 0;
    unsigned int fre = 0;
    
    ret = GxBus_PmSatsGetByPos(0,1,&sat_arry);
    if(ret < 0)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }
    fre = p->fre;
    // frequency deal width
    if(((sat_arry.sat_s.lnb1 == sat_arry.sat_s.lnb2) || (sat_arry.sat_s.lnb2 == 0))
            && (sat_arry.sat_s.lnb1 >= 9750))
    {// KU single
        fre = sat_arry.sat_s.lnb1 + p->fre;
    }
    else if(((sat_arry.sat_s.lnb1 == sat_arry.sat_s.lnb2) || (sat_arry.sat_s.lnb2 == 0))
            && (sat_arry.sat_s.lnb1 < 9750))
    {// c single
        fre = sat_arry.sat_s.lnb1 - p->fre;
    }

    ret = GxBus_PmTpExistChek(sat_arry.id, p->fre, p->sym, (p->polar == 0)?1:0/*V=1*/, 0/*used in cable solution*/,&tp_id);
    if(ret < 0)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }
    // UI CONREOL
    GUI_EndDialog("after wnd_full_screen");

    if(ret > 0)
    {// exist the tp, need to check the program is exist or not
        // check program
        unsigned int count = 0;
        GxBusPmDataProg prog = {0};
        int i = 0;
        count = GxBus_PmProgNumGet();
        for(i = 0; i < count; i++)
        {
            GxBus_PmProgGetByPos(i, 1, &prog);
            if ((tp_id == prog.tp_id)
                    &&(p->video_pid == prog.video_pid)
                    &&(p->audio_pid == prog.cur_audio_pid)
                    /*&&(p->video_type == prog.video_type)*/
                    /*&&(p->audio_type == prog.cur_audio_type)*/)
            {// find the same service
                // TODO: need modify, notic the view group
                extern AppPlayOps g_AppPlayOps;
                g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, i);
                break;
            }
        }
        if(i == count)
        {
            ret = 0;
        }
    }

    if(ret == 0)
    {// not exist, need to force to play the service
        GxMsgProperty_PlayerPlay play = {0};
        PlayerWindow video_rect = {0, 0, APP_XRES, APP_YRES};

        play.player = PLAYER_FOR_NORMAL;
        memset(thiz_lock_service_url, 0, sizeof(thiz_lock_service_url));
        sprintf(thiz_lock_service_url, "dvbs://fre:%d&polar:%d&symbol:%d&vpid:%d&apid:%d&pcrpid:%d&vcodec:%d&acodec:%d&tuner:0&tsid:0&dmxid:0&scramble:%d&progid:0",
                fre, (p->polar == 1?0:1)/*p->polar*/, p->sym, p->video_pid, p->audio_pid, p->pcr_pid, p->video_type, p->audio_type, p->scramble_flag);
        memcpy(play.url, thiz_lock_service_url, sizeof(thiz_lock_service_url));

        memcpy(&play.window, &video_rect, sizeof(PlayerWindow));
        {
            CASCAMDemuxFlagClass flag;
            flag.Flags = DEMUX_ECM;
            CASCAM_Release_Demux(&flag);
        }
        printf("\n\033[36mLOCK SERVICE, url = %s\n", play.url);
        GxPlayer_MediaPlay(play.player, play.url, 0, 0, &play.window);
        {
            CASCAMProgramParaClass para = {0};

            para.DemuxID = 0;
            para.ScrambleFlag = p->scramble_flag;
            para.VideoPid = p->video_pid;
            para.AudioPid = p->audio_pid;
            para.VideoECMPid = p->video_ecm_pid;
            para.AudioECMPid = p->audio_ecm_pid;

            CASCAM_Change_Program(&para); 

            CASCAMDemuxFlagClass flag;
            flag.Flags = DEMUX_ECM;
            CASCAM_Start_Demux(&flag, 0);
            CASCAM_Change_Frequence();
        }
    }

    if(LOCK_SERVICE_FORCE == p->force_flag)
    {// force lock
        thiz_nstv_force_lock_status = 1;
    }
    else
    {
        thiz_nstv_force_lock_status = 0;
    }

}
void app_nstv_lock_service_notify_exit(void)
{
    thiz_nstv_force_lock_status = 0;
}
#endif



