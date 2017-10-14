/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_utility.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.06.03		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_config.h"
#include "full_screen.h"

#if NSCAS_SUPPORT
void app_nstv_exit_to_full_screen(void)
{
    GUI_EndDialog("after wnd_full_screen");
    if (g_AppPlayOps.normal_play.play_total != 0)
    {
#if RECALL_LIST_SUPPORT
        g_AppPlayOps.current_play = g_AppPlayOps.recall_play[0];//g_recall_count
#else
        g_AppPlayOps.current_play = g_AppPlayOps.recall_play;
#endif
        g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT,
                g_AppPlayOps.normal_play.play_count);
        if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
            g_AppFullArb.tip = FULL_STATE_LOCKED;
        else
            g_AppFullArb.tip = FULL_STATE_DUMMY;
        //if(GUI_CheckDialog("wnd_passwd") != GXCORE_SUCCESS)
        //{
        //
        //}
    }
}

#endif
