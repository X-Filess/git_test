/*****************************************************************************
* 			  CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009-2014, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_mulifeed.c
* Author    : 	gechq
* Project   :	mulifeed Foundation
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   1.0  	2014.06	          glen           Creation
*****************************************************************************/

//public header files
#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "full_screen.h"
//#include "app_utility.h"
//#include "app_default_params.h"
//#include "app_pop.h"

#define MAX_VIDEO_SOURCE_NAME	6
#define MULTIFEED_BUFFER_LENGTH	(MAX_VIDEO_SOURCE_NAME+20)


#define WND_VIDEO_MULTIFEED		    "wnd_video_multifeed"
#define WND_MULTIFEED_LV		    	    "listview_multifeed"

#define CMB_MULTIFEED_VIDEO_SOURCE  "cmbox_mutifeed_video_source"
#define BOX_MULTIFEED_POPUP_PARA	    "box_video_multifeed_para"

enum UNICABLE_SET_POPUP_BOX
{
	MULTIFEED_BOX_ITEM_VIDEO_PID = 0,
	MULTIFEED_BOX_ITEM_LANGUAGE,
	MULTIFEED_BOX_ITEM_AUDIO_TRACK
};

extern uint16_t app_get_video_pid_status(int number,int *count);
extern int app_play_program_user_info(GxMsgProperty_NodeModify node_modify);
extern int app_get_video_type_from_pmt(int sel);

static void app_multifeed_init_video_sourece(void)
{
#if 1
	int i = 0;
	int video_total = 0;
	uint16_t video_pid = 0;
	GxMsgProperty_NodeByPosGet node = {0};

	app_get_video_pid_status(0,&video_total);
	node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;
       app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
	   
	for(i = 0; i<video_total; i++)
	{	
		video_pid = app_get_video_pid_status(i,&video_total);
		if(node.prog_data.video_pid == video_pid)
		{
			GUI_SetProperty(WND_MULTIFEED_LV,"select",&i);
			return;
		}
	}
	GUI_SetFocusWidget(WND_MULTIFEED_LV);
#else
	int i = 0;
	int video_total = 0;
	uint16_t video_pid = 0;
	char *buffer_all = NULL;
	char buffer[MULTIFEED_BUFFER_LENGTH] = {0};
	GxMsgProperty_NodeByPosGet node = {0};

	app_get_video_pid_status(0,&video_total);
	node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;
       app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));	
	buffer_all = GxCore_Malloc(video_total * MULTIFEED_BUFFER_LENGTH);
	if(buffer_all == NULL)
	{
		return ;
	}
	memset(buffer_all, 0, video_total * MULTIFEED_BUFFER_LENGTH);
	//start build string
	strcat(buffer_all, "[");

	for(i = 0; i < video_total; i++)
	{
		video_pid = app_get_video_pid_status(i,&video_total);
		memset(buffer, 0, MULTIFEED_BUFFER_LENGTH);
		sprintf(buffer,"(%d/%d) %04d,", (i+1), video_total, video_pid);
		strcat(buffer_all, buffer);
		
		if(node.prog_data.video_pid== video_pid)
		{
			GUI_SetProperty(CMB_MULTIFEED_VIDEO_SOURCE,"select",&i);
		}
	}

	// instead last "," to "]", and finish the string build
	memcpy(buffer_all+strlen(buffer_all)-1, "]", 1);
	GUI_SetProperty(CMB_MULTIFEED_VIDEO_SOURCE,"content",buffer_all);

	GxCore_Free(buffer_all);
	
#endif
	return;
}

#if 0
static char *app_multifeed_get_video_str(int number)
{
	int video_total = 0;
	uint16_t video_pid = 0;
	char * str=NULL;

	char buffer[MULTIFEED_BUFFER_LENGTH] = {0};
	memset(buffer, 0, MULTIFEED_BUFFER_LENGTH);

	video_pid = app_get_video_pid_status(number,&video_total);
	memset(buffer, 0, MULTIFEED_BUFFER_LENGTH);
	sprintf(buffer,"(%d/%d) %s%d", (number+1), video_total, "Video",(number+1));
	str = buffer;
	return str;
}
#endif

int app_video_multifeed_fullscreen(void)
{
	if (g_AppPlayOps.normal_play.play_total != 0)
	{
		GxMsgProperty_NodeByPosGet node = {0};
		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
		{
			if ((g_AppPvrOps.state != PVR_DUMMY)
			&&(node.prog_data.id == g_AppPvrOps.env.prog_id))
			{
				PopDlg pop;
				memset(&pop, 0, sizeof(PopDlg));
				pop.type = POP_TYPE_NO_BTN;
				pop.format = POP_FORMAT_DLG;
				pop.str = STR_ID_STOP_PVR_FIRST;
				pop.mode = POP_MODE_UNBLOCK;
				pop.timeout_sec = 3;
				popdlg_create(&pop);
				return 1;
			}
			GUI_SetProperty("txt_full_state", "state", "hide");
			GUI_SetProperty("img_full_state", "state", "hide");
			GUI_SetProperty("txt_full_title", "state", "hide");

			if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
				GUI_EndDialog("wnd_channel_info");

			app_create_dialog(WND_VIDEO_MULTIFEED);
		}
	}
	return 0;
}


SIGNAL_HANDLER int app_multifeed_list_get_total(GuiWidget *widget, void *usrdata)
{
   int video_total = 0;
   app_get_video_pid_status(0,&video_total);
   return video_total;
}

SIGNAL_HANDLER int app_multifeed_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;
	int video_total = 0;
	uint16_t video_pid = 0;
	static char buffer[MULTIFEED_BUFFER_LENGTH] = {0};
	
	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	// col-0: num
	item->x_offset = 0;
	item->image = NULL;
	item->string = NULL;

	//col-1: channel name
	item = item->next;
	item->image = NULL;

	
	memset(buffer, 0, MULTIFEED_BUFFER_LENGTH);

	video_pid = app_get_video_pid_status(item->sel,&video_total);
	memset(buffer, 0, MULTIFEED_BUFFER_LENGTH);
	sprintf(buffer,"(%d/%d) %s%d", (item->sel+1), video_total, "Video",(item->sel+1));


	item->string = buffer;//app_multifeed_get_video_str(item->sel);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_multifeed_list_keypress(GuiWidget *widget, void *usrdata)
{
   GUI_Event *event = NULL;
   uint32_t box_sel = 0;
   int ret = EVENT_TRANSFER_KEEPON;
   uint16_t video_pid = 0;
   int video_total = 0;
   GxMsgProperty_NodeByPosGet node = {0};
   GxMsgProperty_NodeModify node_modify = {0};

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					GUI_EndDialog(WND_VIDEO_MULTIFEED);
					break;

				case STBK_EXIT:
				case STBK_MENU: 		   
					GUI_EndDialog(WND_VIDEO_MULTIFEED);
					app_create_dialog("wnd_channel_info");
				   #if MINI_256_COLORS_OSD_SUPPORT
					ret = EVENT_TRANSFER_STOP;
					   #endif
					break;

				case STBK_OK:
				   	GUI_GetProperty(WND_MULTIFEED_LV,"select",&box_sel);
				   	video_pid = app_get_video_pid_status(box_sel,&video_total);
					
					node.node_type = NODE_PROG;
					node.pos = g_AppPlayOps.normal_play.play_count;
					if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
					{
					   node_modify.node_type = NODE_PROG;
					   memcpy(&node_modify.prog_data, &node.prog_data, sizeof(GxBusPmDataProg));
					   node_modify.prog_data.video_pid= video_pid;
					   node_modify.prog_data.video_type = app_get_video_type_from_pmt(box_sel);
					  // if(memcmp(&node.prog_data, &node_modify.prog_data, sizeof(GxBusPmDataProg)) != 0)
					   if(node.prog_data.video_pid!=video_pid)
					   {
						   if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
						   {
							   //g_AppPlayOps.program_stop();
							   //g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
							   app_play_program_user_info(node_modify);
						   }
					   } 
					}			

					GUI_EndDialog(WND_VIDEO_MULTIFEED);
					app_create_dialog("wnd_channel_info");
					return EVENT_TRANSFER_STOP;
					break;
				case STBK_LEFT:
				case STBK_RIGHT:
					return EVENT_TRANSFER_STOP;
					break;
				default:
					break;
			}
	}

	return ret;
}

SIGNAL_HANDLER int app_video_multifeed_create(GuiWidget *widget, void *usrdata)
{
	app_multifeed_init_video_sourece();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_video_multifeed_destroy(GuiWidget *widget, void *usrdata)
{

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_video_multifeed_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;

	GUI_Event *event = NULL;


	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case STBK_EXIT:
				case STBK_MENU:
					{
						GUI_EndDialog(WND_VIDEO_MULTIFEED);
					}
					break;

				case STBK_OK:
			
					break;
					
				default:
					break;
			}
		default:
			break;
	}

	return ret;
}


