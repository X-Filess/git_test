/*
 * =====================================================================================
 *
 *       Filename:  full_screen.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2011年08月26日 08时47分06秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#include "full_screen.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_pop.h"
#if CASCAM_SUPPORT
#include "app_cascam_pop.h"
#endif


#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
#define JPG_BG          "bg.jpg"
#else
#define JPG_BG          "menuback.jpg"
#endif

#define IMG_STATE       "img_full_state"
#define TXT_STATE       "txt_full_state"
#define TITLE_STATE     "txt_full_title"
#define IMG_REC         "img_full_rec"
#define IMG_TMS         "img_full_tms"

extern AppPlayOps g_AppPlayOps;
static event_list* sp_FullTimer = NULL;

char* tip_str[] = {
    STR_ID_NO_SIGNAL,
    STR_ID_LOCKED,
#if CASCAM_SUPPORT
    "CA Notice",
#endif
    STR_ID_SCRAMBLE,
    STR_ID_NO_PROGRAM,
    " "};

extern bool app_check_av_running(void);

static void full_mute_exec(FullArb* arb)
{
    GxMsgProperty_PlayerAudioMute player_mute;

    if (arb->state.mute == STATE_ON) 
    {
        player_mute = 0;
        arb->state.mute = STATE_OFF;
    }
    else 
    {
        player_mute = 1;
        arb->state.mute = STATE_ON;
    }

    app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
}


static void full_mute_draw(FullArb* arb)
{
#define IMG_MUTE    "img_full_mute"
#if (MINI_16_BITS_OSD_SUPPORT == 1)
#define IMG_MUTE_1    "img_full_mute1"	
#endif
    if (arb->state.mute == STATE_ON)
    {
#if (MINI_16_BITS_OSD_SUPPORT == 1)
		GUI_SetProperty(IMG_MUTE_1, "state", "show");
#endif
        GUI_SetProperty(IMG_MUTE, "state", "show");
    }
    else
    {
#if (MINI_16_BITS_OSD_SUPPORT == 1)
		GUI_SetProperty(IMG_MUTE_1, "state", "hide");
#endif
        GUI_SetProperty(IMG_MUTE, "state", "hide");
    }
    GUI_SetProperty(IMG_MUTE, "draw_now", NULL);
}

extern pvr_speed spd_val[5+1];//app_pvr_bar.c the same
static void full_speed_draw(FullArb* arb)
{
#define IMG_SPEED       "img_full_pause"
#define TXT_SPEED       "txt_full_speed"
#define FF_IMG          "s_pvr_ff.bmp"
#define FB_IMG          "s_pvr_fb.bmp"
    AppPvrOps *pvr = &g_AppPvrOps;  
    char buffer[6] = {0};

    if (pvr->spd != 0)
    {
        if(pvr->spd > 0)
            GUI_SetProperty(IMG_SPEED, "img", FF_IMG);
        else
            GUI_SetProperty(IMG_SPEED, "img", FB_IMG);
        sprintf(buffer,"X%d",spd_val[abs(pvr->spd)]);
        GUI_SetProperty(TXT_SPEED, "string", buffer);

        GUI_SetProperty(IMG_SPEED, "state", "show");
        GUI_SetProperty(TXT_SPEED, "state", "show");
    }
    else
    {
        GUI_SetProperty(IMG_SPEED, "state", "osd_trans_hide");
        GUI_SetProperty(TXT_SPEED, "state", "osd_trans_hide");
    }
//    GUI_SetProperty(IMG_SPEED, "draw_now", NULL);
}
   

static int32_t tms_flag_check(void)
{
    int32_t tms_flag = 0;

    GxBus_ConfigGetInt(PVR_TIMESHIFT_KEY, &tms_flag, PVR_TIMESHIFT_FLAG);
    return tms_flag;
}
static void event_to_pvr(void)
{
	GUI_Event event = {0};

    event.type = GUI_KEYDOWN;
    event.key.sym = STBK_PAUSE;

    app_create_dialog("wnd_pvr_bar");
    GUI_SetProperty("wnd_pvr_bar", "draw_now", NULL);
    GUI_SendEvent("wnd_pvr_bar", &event);
}

#define TMS_FLAG_CHECK()    tms_flag_check()
#define EVENT_TO_PVR()      event_to_pvr()
static void full_pause_exec(FullArb* arb)
{
    GxMsgProperty_PlayerPause player_pause;
    GxMsgProperty_PlayerResume player_resume;
    int freeze = 0;

    if(g_AppPvrOps.state != PVR_DUMMY)
    {
        GxMsgProperty_NodeByPosGet node_prog = {0};
        node_prog.node_type = NODE_PROG;
        node_prog.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
        if((g_AppPvrOps.state != PVR_TP_RECORD)
               && (node_prog.prog_data.id == g_AppPvrOps.env.prog_id)
            /*&&(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)*/)
            //Modified 2012-11-06 for Radio TMS
        {
            EVENT_TO_PVR();
        }
        else
        {
            goto NORMAL_PAUSE;
        }
    }
    else if ((TMS_FLAG_CHECK() == STATE_ON)
                && (g_AppPlayOps.normal_play.play_total > 0)
                && (g_AppPvrOps.usb_check(&g_AppPvrOps) == USB_OK)
                && (app_check_av_running() == true)
                &&(arb->state.pause == STATE_OFF))
    {
        //tms only start
        if((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
#if REDIO_RECODE_SUPPORT
			|| (g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
#endif
			)
        {
            EVENT_TO_PVR();
        }
        else
        {
            goto NORMAL_PAUSE;
        }
    }
    else
    {
NORMAL_PAUSE:
        if (arb->state.pause == STATE_ON)
        {
            GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);

            player_resume.player = PLAYER_FOR_NORMAL;
            app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
            arb->state.pause = STATE_OFF;
        }
        else
        {
            GxMsgProperty_PlayerTimeShift time_shift;

            freeze = 1;
            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);

            memset(&time_shift, 0, sizeof(GxMsgProperty_PlayerTimeShift));
            time_shift.enable = 0;
			time_shift.shift_dmxid = 1;
#if CA_SUPPORT
		{
			uint16_t ts_src = 0;
			uint16_t DemuxId = time_shift.shift_dmxid;
		    app_demux_param_adjust(&ts_src, &DemuxId, 0);
			time_shift.shift_dmxid = DemuxId;
		}
#endif
            app_send_msg_exec(GXMSG_PLAYER_TIME_SHIFT, (void*)(&time_shift));
            app_subt_pause();
            player_pause.player = PLAYER_FOR_NORMAL;
            app_send_msg_exec(GXMSG_PLAYER_PAUSE, (void *)(&player_pause));
            arb->state.pause = STATE_ON;

        }
    }

}

static void full_pause_draw(FullArb* arb)
{
#define IMG_PAUSE    "img_full_pause"
#define PAUSE_IMG    "s_pause.bmp"

    if (arb->state.pause == STATE_ON)
    {
		GUI_SetProperty(IMG_PAUSE, "img", PAUSE_IMG);
        GUI_SetProperty(IMG_PAUSE, "state", "show");
		GUI_SetProperty(TXT_SPEED, "string", " ");

    }
    else
    {
        GUI_SetProperty(IMG_PAUSE, "state", "osd_trans_hide");
		full_speed_draw(arb);
    }
    GUI_SetProperty(IMG_PAUSE, "draw_now", NULL);
}

static void full_tv_radio_exec(FullArb* arb)
{
#define STREAM_TYPE(play_ops)   play_ops.normal_play.view_info.stream_type
#define CUR_TV(play_ops)        play_ops.normal_play.view_info.tv_prog_cur
#define CUR_RADIO(play_ops)     play_ops.normal_play.view_info.radio_prog_cur
    if (STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_TV)
    {
        STREAM_TYPE(g_AppPlayOps) =  GXBUS_PM_PROG_RADIO;
        arb->state.tv = STATE_OFF;
    }
    else
    {
        STREAM_TYPE(g_AppPlayOps) =  GXBUS_PM_PROG_TV;
        arb->state.tv = STATE_ON;
    }

	if(arb->state.pause == STATE_ON)
	{
		arb->state.pause = STATE_OFF;
		full_pause_draw(arb);
	}

    g_AppPlayOps.program_stop();
    if (GXCORE_ERROR == g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info)))
    {
        // have no program , close player 
        #if RECALL_LIST_SUPPORT
	//g_AppPlayOps.add_recall_list(&(g_AppPlayOps.current_play));
	g_AppPlayOps.recall_play[0] = g_AppPlayOps.current_play;//g_recall_count
	#else
        g_AppPlayOps.recall_play = g_AppPlayOps.current_play;
	#endif
        g_AppPlayOps.current_play = g_AppPlayOps.normal_play;
    }
    else
    {
		// all list have, then play
		g_AppPlayOps.program_play(PLAY_TYPE_FORCE, g_AppPlayOps.normal_play.play_count);
	}
}

static void full_tv_radio_draw(FullArb* arb)
{
    static uint32_t tv_state_bak = ~((uint32_t)0);

    if(tv_state_bak != arb->state.tv)
    {
        tv_state_bak = arb->state.tv;
		GUI_SetInterface("video_enable", NULL);
        GxCore_ThreadDelay(500);//wait for player set spp
        
        if(arb->state.tv == STATE_OFF)
        {
            GUI_SetInterface("clear_image", NULL);
            GUI_SetInterface("flush", NULL);
            GUI_SetProperty("wnd_full_screen", "back_ground", (void*)(JPG_BG));
            GUI_SetInterface("video_disable", NULL);
        }
        else
        {
            if ((arb->state.subt == STATE_OFF)
                || ( g_AppTtxSubt.ttx_num  + g_AppTtxSubt.subt_num ==0))
            {
                GUI_SetProperty("wnd_full_screen", "back_ground", "null");
                GUI_SetInterface("flush", NULL);
                GUI_SetInterface("video_enable", NULL);
            }
        }
    }
    
	//  if(arb->state.pause == STATE_ON)
	//  {
	//    arb->state.pause = STATE_OFF;
	//  }
}

static void full_recall_exec(FullArb* arb)
{
#define CURRENT_TYPE(play_ops)  play_ops.current_play.view_info.stream_type
#if RECALL_LIST_SUPPORT
extern int app_play_get_recall_total_count(void);
extern void app_recall_list_set_play_num(int recall_num);
  #define RECALL_TYPE(play_ops)   play_ops.recall_play[1].view_info.stream_type//g_recall_count
	if (GUI_CheckDialog("wnd_channel_info") != GXCORE_SUCCESS)
	{
	//	app_recall_list_set_play_num(1);  //--[2013-07-03]
	}
	app_play_get_recall_total_count();//排除已删除节目.--[2013-07-03]
    g_AppPlayOps.recall_play[1].view_info.skip_view_switch = g_AppPlayOps.normal_play.view_info.skip_view_switch;
    if (GXCORE_ERROR == g_AppPlayOps.play_list_create(&(g_AppPlayOps.recall_play[1].view_info)))//g_recall_count
    {
            // have no program , close player 
            g_AppPlayOps.play_list_create(&(g_AppPlayOps.current_play.view_info));
            return;
    }
#else
    #define RECALL_TYPE(play_ops)   play_ops.recall_play.view_info.stream_type
    g_AppPlayOps.recall_play.view_info.skip_view_switch = g_AppPlayOps.normal_play.view_info.skip_view_switch;
    if (GXCORE_ERROR == g_AppPlayOps.play_list_create(&(g_AppPlayOps.recall_play.view_info)))
    {
            // have no program , close player 
            g_AppPlayOps.play_list_create(&(g_AppPlayOps.current_play.view_info));
            return;
    }
#endif

    // have program , start play 
    if (CURRENT_TYPE(g_AppPlayOps) != RECALL_TYPE(g_AppPlayOps))
    {
    	if (RECALL_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_TV)
    	{
    		arb->state.tv = STATE_ON;
    	}
    	else
    	{
    		arb->state.tv = STATE_OFF;
    	}
    }
#if RECALL_LIST_SUPPORT
    g_AppPlayOps.program_play(PLAY_TYPE_FORCE, g_AppPlayOps.recall_play[1].play_count);//g_recall_count
#else
	if(g_AppPlayOps.recall_play.play_count >= g_AppPlayOps.normal_play.play_total)
		g_AppPlayOps.recall_play.play_count = 0;
    g_AppPlayOps.program_play(PLAY_TYPE_FORCE, g_AppPlayOps.recall_play.play_count);
#endif
}

static int BadSignalCount = 0;
static void full_state_exec(FullArb* arb)
{
    GxMsgProperty_NodeByPosGet prog_node = {0};
    AppFrontend_LockState lock;
    int led_lock_state = 0;

    if (g_AppPvrOps.state == PVR_RECORD
            || g_AppPvrOps.state == PVR_TMS_AND_REC)
    {
        // pvr rec state get
        GxMsgProperty_NodeByPosGet node;
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
        if (g_AppPvrOps.env.prog_id == node.prog_data.id)
        {
            arb->state.rec = STATE_ON;
        }
        else if(GUI_CheckDialog("wnd_find") != GXCORE_SUCCESS)
        {
            arb->state.rec = STATE_OFF;
        }
    }
    else if(g_AppPvrOps.state == PVR_TP_RECORD)
    {
        arb->state.rec = STATE_ON;
    }
    else
    {
        arb->state.rec = STATE_OFF;
    }

    if (g_AppPvrOps.state == PVR_TIMESHIFT
            || g_AppPvrOps.state == PVR_TMS_AND_REC)
    {
        arb->state.tms = STATE_ON;
    }
    else
    {
        arb->state.tms = STATE_OFF;
    }

    if (g_AppPlayOps.normal_play.play_total == 0)
    {
        arb->tip = FULL_STATE_NOT_EXIST;
        return;
    }

    app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);
    //g_AppNim.property_get(&g_AppNim, AppFrontendID_Status, &status, 
    //								sizeof(AppFrontend_Status));

    if (lock == FRONTEND_UNLOCK)
    {
		if(BadSignalCount++ >= 3)
		{
			//if(BadSignalCount == 4)
			//{
				//GxPlayer_MediaVideoHide("player1");
				//BadSignalFlag = true;
			//}
			arb->tip = FULL_STATE_NO_SIGNAL;
			led_lock_state = 0;
			ioctl(bsp_panel_handle, PANEL_SHOW_LOCK, &led_lock_state);
		}
		else
		{
			if(arb->tip == FULL_STATE_NOT_EXIST)
			{
				arb->tip = FULL_STATE_DUMMY;
			}
		}
	}
    else if(lock == -1)
    {
        // locking tp now, quit
        led_lock_state = 0;//locking status close the lock led add by leixj 120307
        ioctl(bsp_panel_handle, PANEL_SHOW_LOCK, &led_lock_state);
    }
    else
    {
        BadSignalCount = 0;
	//	if(BadSignalFlag == true)
	//	{
		//	GxPlayer_MediaVideoShow("player1");
		//	BadSignalFlag = false;
	//	}
#if 0
        if(s_StateDelay >= 1)
        {
            //g_AppNim.diseqc_set(&g_AppNim);
            // TODO: set diseqc
            if(TempCount++ >= 2)
            {
                extern void app_play_set_diseqc(uint32_t sat_id, uint16_t tp_id, bool ForceFlag);
                GxMsgProperty_NodeByPosGet node;
                GxMsgProperty_NodeNumGet prog_num = {0};

                prog_num.node_type = NODE_PROG;
                app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &prog_num);
                if(prog_num.node_num)
                {
                    node.node_type = NODE_PROG;
                    node.pos = g_AppPlayOps.normal_play.play_count;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&node);
                    app_play_set_diseqc(node.prog_data.sat_id,node.prog_data.tp_id,true);
                }
                s_StateDelay = 0;
                TempCount = 0;
            }

        }
        else
        {
            TempCount = 0;
        }
#endif
        led_lock_state = 1;
        ioctl(bsp_panel_handle, PANEL_SHOW_LOCK, &led_lock_state);

        if(g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK)
        {
            arb->tip = FULL_STATE_LOCKED;
            return;
        }

        // get current program info
        prog_node.node_type = NODE_PROG;
        prog_node.pos = g_AppPlayOps.normal_play.play_count;
        if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node)))
        {
            return;
        }

#if CASCAM_SUPPORT
       // yanfuqiang: FTA channel change to scramble channel, CA message not show
        if(cmm_ca_is_notice())
        {
            arb->tip = FULL_STATE_CA;
            tip_str[FULL_STATE_CA] = (char *)cmm_ca_get_notice_str();
            return;
        }
#endif
        
        if(arb->scramble_check() == FALSE)
        {
            arb->tip = FULL_STATE_DUMMY;
        }
        else
        {
            arb->tip = FULL_STATE_SCRAMBLE;
        }
    }
}

//judge whether state change ,update txt_channel_edit_signal only when change
static bool channeledit_fullstate_change(FullArb *arb)
{
	bool ret = false;
	static int channeledit_fullstate_bak = -1;

	if(channeledit_fullstate_bak != (int)arb->tip)
	{
		ret = true;
		channeledit_fullstate_bak = (int)arb->tip;
	}

	return ret; 
}

static void ch_ed_state_draw(FullArb* arb)
{
#define CH_ED_SIGNAL        "txt_channel_edit_signal"

	char *pstr = NULL;
	static uint8_t play_delay = 0;

	//if (GUI_CheckDialog("wnd_keyboard_language") == GXCORE_SUCCESS
	//	|| GUI_CheckDialog("wnd_pop_tip") == GXCORE_SUCCESS)
	//if (GUI_CheckDialog("wnd_pop_tip") == GXCORE_SUCCESS)
	//{
	//	return;
	//}

	GUI_GetProperty(CH_ED_SIGNAL, "string", &pstr);

	if((arb->tip == FULL_STATE_DUMMY) && (arb->state.tv != STATE_ON))
	{
		if((pstr == NULL) || (strcmp(pstr,"Radio")))
		{
			GUI_SetProperty(CH_ED_SIGNAL, "string", "Radio");
			if (GUI_CheckDialog("wnd_keyboard_language") == GXCORE_SUCCESS)
				GUI_SetProperty("wnd_keyboard_language","update",NULL);
		}
	}
	else if((pstr == NULL) || (strcmp(pstr,tip_str[arb->tip])))
	{
		if (GUI_CheckDialog("wnd_keyboard_language") != GXCORE_SUCCESS)
			GUI_SetProperty(CH_ED_SIGNAL, "string", tip_str[arb->tip]);
	}

	if (arb->tip == FULL_STATE_DUMMY)
	{
		channeledit_fullstate_change(arb);//update epg_fullstate_bak
		if((arb->state.tv == STATE_ON) && (app_check_av_running() == true))//
		{
			if(play_delay >= 0)
			{
			//	GUI_SetProperty(CH_ED_SIGNAL, "state", "osd_trans_hide");
				GUI_SetProperty(CH_ED_SIGNAL, "state", "hide");//错误41561 用OSDhide 会导致小键盘被截断
				GUI_SetInterface("video_on", NULL);
				play_delay = 0;
			}
			else
			{
				play_delay++;
			}
		}
		else
		{
			//GUI_SetProperty(CH_ED_SIGNAL, "string", "Radio");
			//GUI_SetProperty(CH_ED_SIGNAL, "state", "show");
		}
	}
	else
	{
		GUI_SetProperty(CH_ED_SIGNAL, "state", "show");
		if(channeledit_fullstate_change(arb) == true)
		{
			printf("\n################ line:%d   [ channeledit_fullstate have changed ]\n",__LINE__);
			if(arb->state.tv == STATE_ON)
			{
				GUI_SetInterface("video_off", NULL);
			}

			if (GUI_CheckDialog("wnd_keyboard_language") == GXCORE_SUCCESS)
				GUI_SetProperty("wnd_keyboard_language","update",NULL);
			if (GUI_CheckDialog("wnd_pop_tip") == GXCORE_SUCCESS)
				GUI_SetProperty("wnd_pop_tip","update",NULL);
		}

		if(GUI_CheckDialog("wnd_pop_book") == GXCORE_SUCCESS)
			GUI_SetProperty("wnd_pop_book","update",NULL);
	}
}

//judge whether state change ,update txt_epg_signal only when change
static bool epg_fullstate_change(FullArb *arb)
{
	bool ret = false;
	static int epg_fullstate_bak = -1;

	if(epg_fullstate_bak != (int)arb->tip)
	{
		ret = true;
		epg_fullstate_bak = (int)arb->tip;
	}

	return ret;
}

static void epg_state_draw(FullArb* arb)
{
#define EPG_SIGNAL        "txt_epg_signal"

	char *pstr = NULL;
	static uint8_t play_delay = 0;

	if (GUI_CheckDialog("wnd_keyboard_language") == GXCORE_SUCCESS
			|| GUI_CheckDialog("wnd_pop_tip") == GXCORE_SUCCESS)
	{
		return;
	}

	//if(strcmp(BTN_EPG_DETAILS_BACK, GUI_GetFocusWidget()) == 0)
	//return;

	GUI_GetProperty(EPG_SIGNAL, "string", &pstr);

	if((arb->tip == FULL_STATE_DUMMY) && (arb->state.tv != STATE_ON))
	{
		if((pstr == NULL) || (strcmp(pstr,"Radio")))
		{
			GUI_SetProperty(EPG_SIGNAL, "string", "Radio");
			GUI_SetProperty(EPG_SIGNAL, "state", "show");
		}
	}
	else if((pstr == NULL) || (strcmp(pstr,tip_str[arb->tip])))
	{
		GUI_SetProperty(EPG_SIGNAL, "string", tip_str[arb->tip]);
		//avoid tip_str msg text cut part of displayed passwd bar  qingyzh add 20160825
		if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_passwd"))
		{
			GUI_SetProperty("wnd_passwd","update",NULL);
		}
		//add end
	}
    if (arb->tip == FULL_STATE_DUMMY)
	{
		epg_fullstate_change(arb);//update epg_fullstate_bak
		if((arb->state.tv == STATE_ON) && (app_check_av_running() == true))//
		{
			if(play_delay >= 0)
			{
				//GUI_SetProperty(EPG_SIGNAL, "state", "osd_trans_hide");
				GUI_SetProperty(EPG_SIGNAL, "state", "hide");
				GUI_SetInterface("video_on", NULL);
				play_delay = 0;
			}
			else
			{
				play_delay++;
			}
		}
		else
		{
			//GUI_SetProperty(EPG_SIGNAL, "string", "Radio");
			//GUI_SetProperty(EPG_SIGNAL, "state", "show");
		}
	}
	else
	{
		if(arb->state.tv == STATE_ON)
		{
			GUI_SetInterface("video_off", NULL);
		}
		GUI_SetProperty(EPG_SIGNAL, "state", "show");
		if(0 == strcasecmp("wnd_epg", GUI_GetFocusWindow()))//when password created not update
			GUI_SetProperty(EPG_SIGNAL,"update",NULL);
		if(GUI_CheckDialog("wnd_pop_book") == GXCORE_SUCCESS)
			GUI_SetProperty("wnd_pop_book","update",NULL);
	}
}

#if MINI_16BIT_WIN8_OSD_SUPPORT
static void main_menu_state_draw(FullArb* arb)
{
#define MAINMENU_SIGNAL        "txt_main_menu_video"


	char *pstr = NULL;
	static uint8_t play_delay = 0;

	if(GUI_CheckDialog("wnd_pop_tip") == GXCORE_SUCCESS ||
		GUI_CheckDialog("wnd_menu_sub_list") == GXCORE_SUCCESS ) //for bug 56084  qingyzh add 20160826
	{
		return;
	}

	GUI_GetProperty(MAINMENU_SIGNAL, "string", &pstr);

	if((arb->tip == FULL_STATE_DUMMY) && (arb->state.tv != STATE_ON))
	{
		if((pstr == NULL) || (strcmp(pstr,"Radio")))
		{
			GUI_SetProperty(MAINMENU_SIGNAL, "string", "Radio");
			GUI_SetProperty(MAINMENU_SIGNAL, "state", "show");
		}
	}
	else if((pstr == NULL) || (strcmp(pstr,tip_str[arb->tip])))
	{
		GUI_SetProperty(MAINMENU_SIGNAL, "string", tip_str[arb->tip]);
	}
    if (arb->tip == FULL_STATE_DUMMY)
	{
		channeledit_fullstate_change(arb);
		if((arb->state.tv == STATE_ON) && (app_check_av_running() == true))//
		{
			if(play_delay >= 0)
			{
				//GUI_SetProperty(MAINMENU_SIGNAL, "state", "osd_trans_hide");
				GUI_SetProperty(MAINMENU_SIGNAL, "state", "hide");
				GUI_SetInterface("video_on", NULL);
				play_delay = 0;
			}
			else
			{
				play_delay++;
			}
		}

	}
	else
	{
		//if(channeledit_fullstate_change(arb) == true) //rm it because when channeledit didn't change the main menu tv state txt will not flash.
		{
			
			if(arb->state.tv == STATE_ON)
			{
				GUI_SetInterface("video_off", NULL);
			}
			GUI_SetProperty(MAINMENU_SIGNAL, "state", "show");
			

		}
		/*if(arb->state.tv == STATE_ON)
		{
			GUI_SetInterface("video_off", NULL);
		}


		{
			GUI_SetProperty(MAINMENU_SIGNAL, "state", "show");
			if(0 == strcasecmp("wnd_main_menu", GUI_GetFocusWindow()))//when password created not update
				GUI_SetProperty(MAINMENU_SIGNAL,"update",NULL);
		}*/
	}
}

#define MAINMENU_STATE_DRAW(arb)  main_menu_state_draw(arb)
#endif
#define PLAY_KEY_EXIST()    false
#define CH_ED_STATE_DRAW(arb)  ch_ed_state_draw(arb)
#define EPG_STATE_DRAW(arb)  epg_state_draw(arb)
static void full_state_draw(FullArb* arb)
{
    const char *wnd = GUI_GetFocusWindow();
    if(NULL != wnd)
    {
        // signal state , wnd add here
        if (GXCORE_SUCCESS == GUI_CheckDialog("wnd_channel_edit") || GXCORE_SUCCESS == GUI_CheckDialog("wnd_epg"))//(0 == strcmp(wnd,"wnd_channel_edit"))
        {
			if(GXCORE_SUCCESS == GUI_CheckDialog("wnd_channel_edit"))
				CH_ED_STATE_DRAW(arb);
			else
				EPG_STATE_DRAW(arb);
            return;
        }
		
#if MINI_16BIT_WIN8_OSD_SUPPORT
		else if (GXCORE_SUCCESS == GUI_CheckDialog("wnd_main_menu") )
        {
			MAINMENU_STATE_DRAW(arb);
            return;
        }
#endif

    }

    if(GUI_CheckDialog("wnd_epg") == GXCORE_SUCCESS)
        return;

          // rec here
    if (arb->state.rec == STATE_ON)
    {
        GUI_SetProperty(IMG_REC, "state", "show");
    }
    else
    {
        //从原来是在键盘框不在时用osd_trans_hide,先改为hide,就算键盘窗口存在也
        //不会出现把键盘消一块
        GUI_SetProperty(IMG_REC, "state", "osd_trans_hide");
    }

     // tms here
    if (arb->state.tms == STATE_ON)
    {
        GUI_SetProperty(IMG_TMS, "state", "show");
    }
    else
    {
        GUI_SetProperty(IMG_TMS, "state", "osd_trans_hide");
    }

    if (GUI_CheckDialog("wnd_passwd") != GXCORE_SUCCESS
        && GUI_CheckDialog("wnd_pop_tip") != GXCORE_SUCCESS
        && GUI_CheckDialog("wnd_channel_info_signal") != GXCORE_SUCCESS
#if CASCAM_SUPPORT
        && (GUI_CheckDialog("wnd_cmm_dialog1") != GXCORE_SUCCESS)
        && (GUI_CheckDialog("wnd_cmm_dialog4") != GXCORE_SUCCESS)
        && (GUI_CheckPrompt("prompt_ca_notice") != GXCORE_SUCCESS)
#endif
        )
    {
        if(NULL != wnd)
        {
            // signal state , wnd add here
            if(!(0 == strcmp(wnd,"wnd_full_screen")
                || 0 == strcmp(wnd,"wnd_channel_info")
                || 0 == strcmp(wnd,"wnd_volume_value")
                || 0 == strcmp(wnd,"wnd_number")
                || 0 == strcmp(wnd,"wnd_pvr_bar")
                || 0 == strcmp(wnd,"wnd_channel_list")
                || 0 == strcmp(wnd,"wnd_sleep_set")))
            {
                char *pstr = NULL;
                GUI_GetProperty(TXT_STATE, "string", &pstr);
                if((NULL == pstr) || (0 != strncmp(pstr, tip_str[FULL_STATE_DUMMY], strlen(pstr))))
                {
                    GUI_SetProperty(TXT_STATE, "string", tip_str[FULL_STATE_DUMMY]);
                }
                GUI_SetProperty(TXT_STATE, "state", "hide");
                GUI_SetProperty(IMG_STATE, "state", "hide");
                GUI_SetProperty(TITLE_STATE, "state", "hide");
                return;
            }
        }

        if(0 != strcmp(wnd,"wnd_channel_list"))
        {
            if (arb->tip == FULL_STATE_DUMMY)
            {
                GUI_SetProperty(IMG_STATE, "state", "osd_trans_hide");
                GUI_SetProperty(TITLE_STATE, "state", "osd_trans_hide");
                GUI_SetProperty(TXT_STATE, "state", "osd_trans_hide");
            }
            else
            {
                if (!PLAY_KEY_EXIST())
                {
                    char *pstr = NULL;
                    GUI_GetProperty(TXT_STATE, "string", &pstr);
                    if((NULL == pstr) || (0 != strncmp(pstr, tip_str[arb->tip], strlen(pstr))))
                    {
                        GUI_SetProperty(TXT_STATE, "string", tip_str[arb->tip]);
                    }
                    GUI_SetProperty(TXT_STATE, "state", "show");
                    GUI_SetProperty(IMG_STATE, "state", "show");
                    GUI_SetProperty(TITLE_STATE, "state", "show");
                }
            }
        }
    }
	else
    {
            char *pstr = NULL;
            GUI_GetProperty(TXT_STATE, "string", &pstr);
            if((NULL == pstr) || (0 != strncmp(pstr, tip_str[FULL_STATE_DUMMY], strlen(pstr))))
            {
                GUI_SetProperty(TXT_STATE, "string", tip_str[FULL_STATE_DUMMY]);
            }
            GUI_SetProperty(TXT_STATE, "state", "hide");
            GUI_SetProperty(IMG_STATE, "state", "hide");
            GUI_SetProperty(TITLE_STATE, "state", "hide");
    }
}

static int full_state_timer(void* usrdata)
{
    FullArb *arb = &g_AppFullArb;

    arb->exec[EVENT_STATE](arb);
    arb->draw[EVENT_STATE](arb);

	return 0;
}

static void full_timer_start(void)
{
    if (reset_timer(sp_FullTimer) != 0) 
    {
        // reset time failed, do
        sp_FullTimer = create_timer(full_state_timer, 300, NULL, TIMER_REPEAT);
    }
}

static void full_timer_stop(void)
{
    timer_stop(sp_FullTimer);
}

// flag 0 : get scramble status 1 : set scramble   2 : set free
static bool full_scramble_manage(uint32_t flag)
{
extern int app_get_descrambler_delay_flag(void);
	static uint32_t scramble = 2;

	if (flag == 0)
	{
		if((scramble == 1)
#if 1
            &&(app_get_descrambler_delay_flag() == 1)
#endif
             )
        {
            return TRUE;
        }
		else if (scramble == 2)    	return FALSE;
		else    return FALSE;
	}
	else if (flag == 1 || flag == 2)
	{
		scramble = flag;
		return TRUE;
	}
	else
	{
		return TRUE;
	}

	return TRUE;
}

static bool full_scramble_check(void)
{
    return full_scramble_manage(0);
}
static void full_scramble_set(uint32_t flag)
{
    full_scramble_manage(flag);
}


static void full_arbitrate_init(FullArb* arb)
{
    int32_t init_value = 0;

    if (g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        arb->state.tv       = STATE_ON; 
    }
    else
    {
        arb->state.tv       = STATE_OFF; 
    }
    arb->state.mute     = STATE_OFF;
    arb->state.pause    = STATE_OFF;
    arb->state.rec      = STATE_OFF;
    arb->state.tms      = STATE_OFF;
    arb->state.ttx      = STATE_OFF;

    GxBus_ConfigGetInt(SUBTITLE_MODE_KEY, &init_value, SUBTITLE_MODE_VALUE);
    (init_value==0)?(arb->state.subt=STATE_OFF):(arb->state.subt=STATE_ON);

    arb->tip            = FULL_STATE_DUMMY;//TODO  !!! MUST get !!!!!!! shenbin 

    arb->draw[EVENT_TV_RADIO]   = full_tv_radio_draw;
    arb->exec[EVENT_TV_RADIO]   = full_tv_radio_exec;

    arb->draw[EVENT_MUTE]       = full_mute_draw;
    arb->exec[EVENT_MUTE]       = full_mute_exec;

    arb->draw[EVENT_PAUSE]      = full_pause_draw;
    arb->exec[EVENT_PAUSE]      = full_pause_exec;

    arb->exec[EVENT_STATE]      = full_state_exec;
    arb->draw[EVENT_STATE]      = full_state_draw;

    arb->draw[EVENT_RECALL]     = full_tv_radio_draw;
    arb->exec[EVENT_RECALL]     = full_recall_exec;

    arb->draw[EVENT_SPEED]      = full_speed_draw;
    arb->exec[EVENT_SPEED]      = NULL;

}


FullArb g_AppFullArb = 
{
    .init               = full_arbitrate_init,
    .timer_start        = full_timer_start,
    .timer_stop         = full_timer_stop,
    .scramble_check     = full_scramble_check,
    .scramble_set       = full_scramble_set,
};

//////////for media center////////////
void app_save_mute_state(int value)
{
    if(value == 0)
    {
        g_AppFullArb.state.mute = STATE_OFF;
    }
    else
    {
        g_AppFullArb.state.mute = STATE_ON;
    }
}

int app_get_mute_state(void)
{
    pmpset_mute ret;
    
    if(g_AppFullArb.state.mute == STATE_OFF)
    {
        ret = 0;
    }
    else
    {
       ret = 1;
    }
   
    return ret;
}

int app_get_motor_state_in_fullscreen(void)
{
	int value = 0;

	 if((GUI_CheckDialog("wnd_channel_list") == GXCORE_SUCCESS
		|| GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS
		|| GUI_CheckDialog("wnd_full_screen") == GXCORE_SUCCESS)
		&& (GUI_CheckDialog("wnd_main_menu") != GXCORE_SUCCESS))
	 {
		value = 1;
	 }
	 else
	 {
		value = 0;
	 }
	 
	 return value;
}

