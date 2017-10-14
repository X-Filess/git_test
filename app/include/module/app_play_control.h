/*
 * =====================================================================================
 *
 *       Filename:  app_play_control.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2011��09��14�� 15ʱ08��28��
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef __APP_PLAY_CONTROL_H__
#define  __APP_PLAY_CONTROL_H__

#include "gxcore.h"
#include "module/pm/gxprogram_manage_berkeley.h"
#include "module/player/gxplayer_module.h"

#if RECALL_LIST_SUPPORT
#define MAX_RECALL_NUM		16
#endif

/**
 * play mode
 * normal play next program: PLAY_TYPE_NORMAL|PLAY_MODE_NEXT
 * normal play program 9: PLAY_TYPE_NORMAL|PLAY_MODE_POINT, 9
 */
typedef enum
{
	PLAY_TYPE_NORMAL = 1,
//	PLAY_TYPE_PIP =1<<1,
	PLAY_TYPE_FORCE = 1<<2,

	PLAY_MODE_NEXT = 1<<3,
	PLAY_MODE_PREV = 1<<4,
	PLAY_MODE_POINT = 1<<5,

	PLAY_MODE_UNLOCK = 1<<6, // enter passwd success
	PLAY_MODE_WITH_REC = 1<<7, //play with record
	PLAY_MODE_WITH_TMS = 1<<8, //play with record
}AppPlayMode;

/**
  * current program play status
  */
typedef enum
{
	PLAY_STATUS_NORAML = 1,
	PLAY_STATUS_SCRAMBLE = 1<<1,
	PLAY_STATUS_LOCKED = 1<<2,
}AppProgStatus;

typedef enum
{
    PLAY_KEY_DUMMY,
    PLAY_KEY_LOCK,
    PLAY_KEY_UNLOCK
}AppPlayKey;

/**
 * recored the every mode's play info
 */
typedef struct
{
    uint32_t tuner;
    uint32_t ts_src;
    uint32_t dmx_id;
	GxBusPmViewInfo view_info;
	int32_t play_count;		// current play No.
	int32_t play_total;		// total number in list
    AppPlayKey key;
    AppPlayKey rec;
}PlayInfo;

typedef struct
{
	AppPlayMode mode;
	int32_t pos;
}PlayStatus;

typedef struct
{
    uint32_t width;
    uint32_t height;
}VideoSolution;

/**
 * play control, can realize all play here
 */
typedef struct
{
	PlayInfo normal_play;		// program play in normal screen

	PlayInfo current_play;		// current in noraml screen
	#if RECALL_LIST_SUPPORT
	PlayInfo recall_play[MAX_RECALL_NUM];		// recall in noraml screen
	#else
	PlayInfo recall_play;		// recall in noraml screen
	#endif

	void (*program_play_init)(void);

	status_t (*play_list_create)(GxBusPmViewInfo*);	// create play list, prepare the environment for play
	void (*program_play)(AppPlayMode, int32_t);	// play a program by pos in the list, 
											// when play mode set "PLAY_MODE_POINT",uint32_t in use
	void (*program_stop)(void);											
	void (*play_window_set)(AppPlayMode, PlayerWindow*);	// set player show window
	
	// app fill the follow function
	void (*poppwd_create)(int32_t);			// pop passwd create
	void (*set_rec_time)(uint32_t);
	#if RECALL_LIST_SUPPORT
	status_t (*add_recall_list)(PlayInfo*);
	#endif
}AppPlayOps;


extern AppPlayOps   g_AppPlayOps;

typedef struct
{
	GxBusPmViewInfo 	old_view_info;
        uint32_t                old_tv_state;
}StreamState;
/*
����������channel eidte����������
*/
extern StreamState g_OldStreamState;

extern status_t app_get_prog_pos_by_id(GxBusPmViewInfo *pm_view, uint32_t prog_id, int32_t *prog_pos);
#endif

