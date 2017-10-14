/*
 * =====================================================================================
 *
 *       Filename:  app_audio.c
 *
 *    Description:  select dvb subtitle or ttx subtitle
 *
 *        Version:  1.0
 *        Created:  2011年10月13日 10时14分49秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app.h"
#include "app_module.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_send_msg.h"
#include "full_screen.h"
#include "app_utility.h"
#include "app_audio.h"

#define SUBT_OFF_MODE_NUM    1
#define WND_AUDIO    "wnd_audio"
#define LV_AUDIO    "listview_audio"
#define CMB_AUDIO_TRACK "cmb_audio_track"

/*typedef struct
{
	uint32_t id;
	char *name;
	AudioCodecType audiotype;
}AudioList_t;

typedef struct
{
	AudioList_t *audioLangList;
	uint32_t audioLangCnt;
}AudioLang_t;*/

static AudioLang_t audioLang = {0,0};
static uint8_t audio_ac3_num = 0;
static handle_t s_audio_mutex = -1;

static void app_audio_init(void);

extern int app_get_ac3_count_num(void);
extern int app_get_mult_audio_type(int mode, uint8_t *a);
extern void app_show_channel_pid(GxBusPmDataProg *prog_data);
//extern void app_ca_set_cw(handle_t handle,AppCaDescramblerType type,uint8_t* OddKey,uint8_t* EvenKey);
//extern void app_ca_get_cw(handle_t handle,AppCaDescramblerType type,uint8_t* OddKey,uint8_t* EvenKey);
#ifdef AUTO_CHANGE_FIRST_AUDIO_SUPPORT
extern int app_set_first_audio_lang(void);
#endif

AudioLang_t* app_get_audio_lang(void)
{
    return &audioLang;
}

void app_audio_lang_free(AudioLang_t *audio_lang)
{
    if((audio_lang != NULL) && (NULL != audio_lang->audioLangList))
    {
        uint32_t i = 0;

        for(i=0; i<audio_lang->audioLangCnt; i++)
        {
            if(audio_lang->audioLangList[i].name != NULL)
            {
                GxCore_Free(audio_lang->audioLangList[i].name);
                audio_lang->audioLangList[i].name = NULL;
            }
        }
        GxCore_Free(audio_lang->audioLangList);
        audio_lang->audioLangList = NULL;
        audio_lang->audioLangCnt = 0;
    }
}

void app_audio_free(void)
{
    if(s_audio_mutex == -1)
    {
        GxCore_MutexCreate(&s_audio_mutex);
    }

    GxCore_MutexLock(s_audio_mutex);
    app_audio_lang_free(&audioLang);
    GxCore_MutexUnlock(s_audio_mutex);
    if(GUI_CheckDialog(WND_AUDIO) == GXCORE_SUCCESS)
    {
        GUI_SetProperty(LV_AUDIO,"update_all",NULL);
    }
}

static char *app_audio_get_str_duplication(uint8_t *array, uint32_t strLen)
{
	char *desStr = NULL;
	uint32_t i = 0;
	if(NULL == array || 0 == strLen)
		return NULL;
	
	desStr = GxCore_Malloc((strLen+1)*sizeof(char));
	for(i=0; i<strLen; i++)
	{
		desStr[i] = array[i];
	}
	desStr[strLen] = '\0';
	return desStr;
	
}

static uint32_t app_audio_get_array_len(uint8_t *array, uint32_t strLen)
{
	uint32_t i = 0;
	
	if(NULL == array)
		return 0;

	for(i = 0; i< strLen; i++)
	{
		if('\0' == array[i])
			break;
	}
	return i;
}

#define MPEG_1_AUDIO		(0x03)
#define MPEG_2_AUDIO		(0x04)
#define EAC3_AUDIO		    (0x06)
#define AC3_AUDIO			(0x81)
#define AAC_ADTS			(0x0f)
#define AAC_LATM			(0x11)

static AudioCodecType app_audio_type(uint8_t  stream_type, uint8_t tag_index)
{
    AudioCodecType ret = AUDIO_CODEC_UNKNOWN;
#if 1//ndef USER_BUS_PMT_INFO
    int audio_type = 0;
    uint8_t audio_index = 0;
#endif

    switch(stream_type)
    {
        case MPEG_1_AUDIO:
            ret = AUDIO_CODEC_MPEG1;
            break;
        case MPEG_2_AUDIO:
            ret = AUDIO_CODEC_MPEG2;
            break;

		//case AC3_AUDIO:
			//ret = AUDIO_CODEC_AC3;
			//break;

        case EAC3_AUDIO:
		#if 0//def USER_BUS_PMT_INFO
		ret = AUDIO_CODEC_EAC3;
		#else
         if( (audio_type = app_get_mult_audio_type(audio_ac3_num,&audio_index))  != -1)
		{
			audio_ac3_num = audio_index + 1;
		}
            

            if((audio_type == 0x6a) || (audio_type == 0x7a)) 
            {
                ret = AUDIO_CODEC_EAC3;
            }
			else if(audio_type == AC3_AUDIO)
			{
				
				ret = AUDIO_CODEC_AC3;
			}
            else  if((audio_type == 0x59)||(audio_type == 0x45)
                    ||(audio_type == 0x46) || (audio_type < 0))
            {
                ret = AUDIO_CODEC_UNKNOWN;
            }
	     #endif
            break;

		case AAC_LATM:
			ret = AUDIO_CODEC_AAC_LATM;
			break;

		case AAC_ADTS:
			ret = AUDIO_CODEC_AAC_ADTS;
			break;
			
		default:
			ret = AUDIO_CODEC_UNKNOWN;
			break;
	}
	
	
	return ret;
}

int app_audio_prog_track_get_mode(void)
{
	int track_sel = 0;
	GxMsgProperty_NodeByPosGet node = {0};
	
	node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
	{	
		
		if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_MONO)
		{
			track_sel = 0;
		}
		else if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_LEFT)
		{
			track_sel = 1;
		}
		else if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_RIGHT)
		{
			track_sel = 2;
		}
		else
		{
			track_sel = 3;
		}
		
	}

	return track_sel;
}

int app_audio_prog_track_set_mode(int value,uint8_t mode)
{
	GxMsgProperty_NodeByPosGet node = {0};
    GxMsgProperty_NodeModify node_modify = {0};
	GxBusPmDataProgAduioTrack prog_track = AUDIO_TRACK_MONO;
	
	node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
	{
		node_modify.node_type = NODE_PROG;
		node_modify.prog_data = node.prog_data;	
		if(value == 0)
		{
			prog_track = GXBUS_PM_PROG_AUDIO_MODE_MONO;
		}
		else if(value == 1)
		{
			prog_track =GXBUS_PM_PROG_AUDIO_MODE_LEFT;
		}
		else if(value == 2)
		{
			prog_track =GXBUS_PM_PROG_AUDIO_MODE_RIGHT;
		}
		else
		{
			prog_track =GXBUS_PM_PROG_AUDIO_MODE_STEREO;
		}
		
		node_modify.prog_data.audio_mode = prog_track;
		
		if(0xfe == mode)
		{
			node_modify.prog_data.audio_volume = value;
		}
		app_send_msg_exec(GXMSG_PM_NODE_MODIFY, (void *)(&node_modify));
		
	}

	return 0;
}

void app_track_set(PmtInfo* audio)
{
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t audioCnt = 0;
	#ifdef USER_BUS_PMT_INFO
	uint8_t num = 0;
	uint8_t NormalCount = 0;
	uint8_t Ac3Count = 0;
	uint8_t EAc3Count = 0;
	uint8_t FindAudio = 0;
	#endif
	AudioCodecType audioType = AUDIO_CODEC_UNKNOWN;

    GxCore_MutexLock(s_audio_mutex);
    app_audio_lang_free(&audioLang);

    if(NULL == audio ||0 == audio->stream_count)
    {
        GxCore_MutexUnlock(s_audio_mutex);
        return;
    }

    audio_ac3_num = 0;

	for(i = 0; i<audio->stream_count; i++)
	{
		if((MPEG_1_AUDIO == audio->stream_info[i].stream_type)
			|| (MPEG_2_AUDIO == audio->stream_info[i].stream_type)
			//|| (AC3_AUDIO == audio->stream_info[i].stream_type)
			|| (AAC_ADTS == audio->stream_info[i].stream_type)
			|| (AAC_LATM == audio->stream_info[i].stream_type))
		{
			audioCnt++;
		}
	}

	#ifdef USER_BUS_PMT_INFO
	audioCnt+= audio->eac3_count + audio->ac3_count;
	#else
	audioCnt+= app_get_ac3_count_num();
	#endif

	audioLang.audioLangCnt = audioCnt;

    if(0 == audioLang.audioLangCnt)
    {
        GxCore_MutexUnlock(s_audio_mutex);
        return;
    }

    audioLang.audioLangList = GxCore_Calloc(audioLang.audioLangCnt, sizeof(AudioList_t));
    if(NULL == audioLang.audioLangList)
    {
        audioLang.audioLangCnt = 0;
        GxCore_MutexUnlock(s_audio_mutex);
        return;
    }

	for(i = 0; i<audio->stream_count; i++)
	{
		audioType = app_audio_type(audio->stream_info[i].stream_type,audioCnt);
		
		if(audioType != AUDIO_CODEC_UNKNOWN)
		{
#ifdef USER_BUS_PMT_INFO
			if(audioType != AUDIO_CODEC_EAC3)
			{
				audioLang.audioLangList[j].audiotype = audioType;
				audioLang.audioLangList[j].id = audio->stream_info[i].elem_pid;
				if((NULL != audio->stream_info[i].iso639_info)
					&&(0 != app_audio_get_array_len(audio->stream_info[i].iso639_info->iso639,3)))
				{
					audioLang.audioLangList[j].name = app_audio_get_str_duplication(audio->stream_info[i].iso639_info->iso639,3);
				}
				else
				{
					#define MAX_ID_LEN    6
					char buf[MAX_ID_LEN] = {0};

					if(audioType == AUDIO_CODEC_AC3)
					{
						Ac3Count++;
						sprintf(buf, "%d",Ac3Count);
						buf[MAX_ID_LEN-1] = '\0';
						audioLang.audioLangList[j].name = get_string_strcat("Dobly",buf,NULL);
					}
					else
					{
						NormalCount++;
						sprintf(buf, "%d",NormalCount);
						buf[MAX_ID_LEN-1] = '\0';
						audioLang.audioLangList[j].name = get_string_strcat("audio",buf,NULL);
					}
				}
				j++;
			}
			else 
			{
				FindAudio = 0;
				for(num=0; num<audio->ac3_count;num++)
				{
					if(((Ac3Descriptor*)audio->ac3_info[num])->elem_pid == audio->stream_info[i].elem_pid)	
					{
						audioLang.audioLangList[j].audiotype = AUDIO_CODEC_AC3;
						audioLang.audioLangList[j].id = ((Ac3Descriptor*)audio->ac3_info[num])->elem_pid;
						FindAudio = 1;
						break;
					}
				}
				
				if(num == audio->ac3_count)
				{
					for(num=0; num<audio->eac3_count;num++)
					{
						if(((Eac3Descriptor*)audio->eac3_info[num])->elem_pid == audio->stream_info[i].elem_pid)	
						{
							audioLang.audioLangList[j].audiotype = AUDIO_CODEC_EAC3;
							audioLang.audioLangList[j].id = ((Eac3Descriptor*)audio->eac3_info[num])->elem_pid;
							FindAudio = 1;
							break;
						}
					}
				}
				
				if(FindAudio == 1)
				{// find the audio, AC3 OR EAC3
					if((NULL != audio->stream_info[i].iso639_info)
							&&(0 != app_audio_get_array_len(audio->stream_info[i].iso639_info->iso639,3)))
					{
						audioLang.audioLangList[j].name = app_audio_get_str_duplication(audio->stream_info[i].iso639_info->iso639,3);
					}
					else
					{
		            			#define MAX_ID_LEN    6
						char buf[MAX_ID_LEN] = {0};

						if(audioType == AUDIO_CODEC_AC3 )
						{
							Ac3Count++;
							sprintf(buf, "%d",Ac3Count);
							buf[MAX_ID_LEN-1] = '\0';
							audioLang.audioLangList[j].name = get_string_strcat("Dobly",buf,NULL);
						}
						else if (audioType == AUDIO_CODEC_EAC3)
						{
							EAc3Count++;
							sprintf(buf, "%d",EAc3Count);
							buf[MAX_ID_LEN-1] = '\0';
							audioLang.audioLangList[j].name = get_string_strcat("Dobly+",buf,NULL);
						}
						else
						{
							NormalCount++;
							sprintf(buf, "%d",NormalCount);
							buf[MAX_ID_LEN-1] = '\0';
							audioLang.audioLangList[j].name = get_string_strcat("audio",buf,NULL);
						}
					}
					j++;
				}				
			}
#else	
			audioLang.audioLangList[j].audiotype = audioType;
			audioLang.audioLangList[j].id = audio->stream_info[i].elem_pid;
			if((NULL != audio->stream_info[i].iso639_info)
				&&(0 != app_audio_get_array_len(audio->stream_info[i].iso639_info->iso639,3)))
			{
				audioLang.audioLangList[j].name = app_audio_get_str_duplication(audio->stream_info[i].iso639_info->iso639,3);
			}
			else
			{
            			#define MAX_ID_LEN    6
				char buf[MAX_ID_LEN] = {0};
				sprintf(buf, "%d",j);
				buf[MAX_ID_LEN-1] = '\0';

		                if((audioType == AUDIO_CODEC_AC3) || (audioType == AUDIO_CODEC_EAC3))
		                {
				         audioLang.audioLangList[j].name = get_string_strcat("Dobly",buf,NULL);
		                }
		                else
		                {
					   audioLang.audioLangList[j].name = get_string_strcat("audio",buf,NULL);
		                }
			}
#endif
			
		}
	}
	
#ifdef AUTO_CHANGE_FIRST_AUDIO_SUPPORT
	app_set_first_audio_lang();
#endif

    #ifdef RECOVER_AUDIO_ID_TYPE
	GxMsgProperty_NodeByPosGet node = {0};
	node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
	{	
		for(i = 0; i < audioLang.audioLangCnt; i++)
		{		
			if(audioLang.audioLangList[i].id == node.prog_data.cur_audio_pid)
			{
				break;
			}
		}
		if(i >= audioLang.audioLangCnt) //can't find track
		{
			GxMsgProperty_NodeModify  modify_node = {0};
			GxMsgProperty_PlayerAudioSwitch audioSwitch = {0};
		
			audioSwitch.player = PLAYER_FOR_NORMAL;
			audioSwitch.pid = audioLang.audioLangList[0].id;
			audioSwitch.type = audioLang.audioLangList[0].audiotype;
			app_send_msg_exec(GXMSG_PLAYER_AUDIO_SWITCH, &audioSwitch);

			memcpy(&(modify_node.prog_data),&(node.prog_data),sizeof(GxBusPmDataProg));
			modify_node.node_type = NODE_PROG;
			modify_node.prog_data.cur_audio_pid = audioLang.audioLangList[0].id;
			if(AUDIO_CODEC_AC3 == audioLang.audioLangList[0].audiotype)
			{
				modify_node.prog_data.cur_audio_type = GXBUS_PM_AUDIO_AC3;
            }
            else if(AUDIO_CODEC_EAC3 == audioLang.audioLangList[0].audiotype)
			{
				modify_node.prog_data.cur_audio_type = GXBUS_PM_AUDIO_EAC3;
			}
			else if(AUDIO_CODEC_MPEG1 = audioLang.audioLangList[0].audiotype)
			{
				modify_node.prog_data.cur_audio_type = GXBUS_PM_AUDIO_MPEG1;
			}
            else if(AUDIO_CODEC_MPEG2 = audioLang.audioLangList[0].audiotype)
			{
				modify_node.prog_data.cur_audio_type = GXBUS_PM_AUDIO_MPEG2;
			}
			
			app_send_msg_exec(GXMSG_PM_NODE_MODIFY, (void *)(&modify_node));

			if (GXCORE_SUCCESS == GUI_CheckDialog("wnd_channel_edit"))
				app_show_channel_pid(&modify_node.prog_data);
		}
	}
	#endif


    GxCore_MutexUnlock(s_audio_mutex);
	if(GUI_CheckDialog(WND_AUDIO) == GXCORE_SUCCESS)
	{
		GUI_SetProperty(LV_AUDIO,"update_all",NULL);
		app_audio_init();
	}
}


static void app_audio_init(void)
{
	int init_value = 0;
	int i = 0;
	GxMsgProperty_NodeByPosGet node = {0};
	uint32_t selectPid = 0;

	GxBus_ConfigGetInt(SOUND_MODE_KEY, &init_value, SOUND_MODE);
	//GUI_SetProperty(CMB_AUDIO_TRACK,"select",&init_value);	

	node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;

	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
			selectPid = node.prog_data.cur_audio_pid;

	if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_MONO)
	{
		init_value = 0;
	}
	else if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_LEFT)
	{
		init_value = 1;
	}
	else if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_RIGHT)
	{
		init_value = 2;
	}
	else
	{
		init_value = 3;
	}
	GUI_SetProperty(CMB_AUDIO_TRACK,"select",&init_value);
    GxCore_MutexLock(s_audio_mutex);
	for(i = 0; i<audioLang.audioLangCnt; i++)
	{		
		if(audioLang.audioLangList[i].id == selectPid)
		{
            GxCore_MutexUnlock(s_audio_mutex);
			GUI_SetProperty(LV_AUDIO,"select",&i);
			return;
		}
	}
    GxCore_MutexUnlock(s_audio_mutex);
}

SIGNAL_HANDLER int app_audio_list_get_total(GuiWidget *widget, void *usrdata)
{
    return audioLang.audioLangCnt;
}

SIGNAL_HANDLER int app_audio_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;
    
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
    GxCore_MutexLock(s_audio_mutex);
    if(audioLang.audioLangList[item->sel].name !=NULL)
    {
   	 item->string = audioLang.audioLangList[item->sel].name;//get_str_duplication(audioLang.audioLangList[item->sel].name, buf);
    }
    GxCore_MutexUnlock(s_audio_mutex);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_audio_list_create(GuiWidget *widget, void *usrdata)
{
	
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_audio_list_destroy(GuiWidget *widget, void *usrdata)
{
    
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_audio_track_change(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel = 0;
	#if 1
	GUI_GetProperty(CMB_AUDIO_TRACK, "select", &box_sel);	
	app_send_msg_exec(GXMSG_PLAYER_AUDIO_TRACK, &box_sel);
	#else
	static GxMsgProperty_PlayerAudioTrack track = AUDIO_TRACK_MONO;
	GUI_GetProperty(CMB_AUDIO_TRACK, "select", &box_sel);		
	track = box_sel;	
	GxBus_ConfigSetInt(SOUND_MODE_KEY, box_sel);
	app_send_msg_exec(GXMSG_PLAYER_AUDIO_TRACK, &track);
	#endif
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_audio_list_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    uint32_t box_sel = 0;
    int ret = EVENT_TRANSFER_KEEPON;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_AUDIO);
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    GUI_GetProperty(CMB_AUDIO_TRACK, "select", &box_sel);	
                    app_audio_prog_track_set_mode(box_sel,0);
                    GUI_EndDialog(WND_AUDIO);
                    GUI_SetInterface("flush",NULL);
                    app_create_dialog("wnd_channel_info");
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_OK:
                    {
                        GxCore_MutexLock(s_audio_mutex);
                        if(audioLang.audioLangCnt == 0)
                        {
                            GxCore_MutexUnlock(s_audio_mutex);
                            break;
                        }
                        GxMsgProperty_NodeByPosGet node = {0};
                        int sel = 0;
                        uint8_t cur_audio_type = GXBUS_PM_AUDIO_MPEG1;

                        GUI_GetProperty(LV_AUDIO,"select",&sel);
                        AudioCodecType audiotype = AUDIO_CODEC_UNKNOWN;
						node.node_type = NODE_PROG;
						node.pos = g_AppPlayOps.normal_play.play_count;
						if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
						{
							GxMsgProperty_NodeModify  modify_node = {0};
							modify_node.node_type = NODE_PROG;
							memcpy(&(modify_node.prog_data),&(node.prog_data),sizeof(GxBusPmDataProg));
							
							GxMsgProperty_PlayerAudioSwitch audioSwitch;
							memset(&audioSwitch ,0,sizeof(GxMsgProperty_PlayerAudioSwitch));
							
							audioSwitch.player = PLAYER_FOR_NORMAL;
							audioSwitch.pid = audioLang.audioLangList[sel].id;
							audiotype = audioLang.audioLangList[sel].audiotype;
							if(AUDIO_CODEC_AC3 == audiotype)
							{
								audioSwitch.type = AUDIO_CODEC_AC3;
								cur_audio_type = GXBUS_PM_AUDIO_AC3;
							}
                       else if(AUDIO_CODEC_EAC3 == audiotype)
							{
								audioSwitch.type = AUDIO_CODEC_EAC3;
								cur_audio_type = GXBUS_PM_AUDIO_EAC3;
                            }
							else if(AUDIO_CODEC_AAC_LATM == audiotype)
							{
								 audioSwitch.type = AUDIO_CODEC_AAC_LATM;
								 cur_audio_type = GXBUS_PM_AUDIO_AAC_LATM;
							}
                            else if(AUDIO_CODEC_MPEG1 == audiotype)
                            {
                                audioSwitch.type = audiotype;
                                cur_audio_type = GXBUS_PM_AUDIO_MPEG1;
                            }
                            else if(AUDIO_CODEC_MPEG2 == audiotype)
                            {
                                audioSwitch.type = audiotype;
                                cur_audio_type = GXBUS_PM_AUDIO_MPEG2;
                            }

                            if(audioLang.audioLangList[sel].id != node.prog_data.cur_audio_pid)
                            {
                                app_send_msg_exec(GXMSG_PLAYER_AUDIO_SWITCH, &audioSwitch);
#if CA_SUPPORT
                                {
                                    app_extend_change_audio(node.prog_data.cur_audio_pid,audioLang.audioLangList[sel].id, CONTROL_NORMAL);
                                    //app_descrambler_set_cw(handle, even, odd, 0);// TODO: 0 for cas2, 128 for 3des
                                    // TODO: 
                                }
#endif
#if CASCAM_SUPPORT
                                extern void app_cascam_change_audio(unsigned short audio_pid);
                                app_cascam_change_audio(audioLang.audioLangList[sel].id);
#endif
                                modify_node.prog_data.cur_audio_pid = audioLang.audioLangList[sel].id;
                                modify_node.prog_data.cur_audio_type = cur_audio_type;
                                app_send_msg_exec(GXMSG_PM_NODE_MODIFY, (void *)(&modify_node));
                            }
						}
                        GxCore_MutexUnlock(s_audio_mutex);
                    }

                    GUI_GetProperty(CMB_AUDIO_TRACK, "select", &box_sel);	
                    app_audio_prog_track_set_mode(box_sel,0);
                    GUI_EndDialog(WND_AUDIO);
                    GUI_SetInterface("flush",NULL);
                    app_create_dialog("wnd_channel_info");
					return EVENT_TRANSFER_STOP;
                    break;
				case STBK_LEFT:
				case STBK_RIGHT:
					GUI_SendEvent(CMB_AUDIO_TRACK, event);
					return EVENT_TRANSFER_STOP;
					break;
                default:
                    break;
            }
    }

    return ret;
}

SIGNAL_HANDLER int app_audio_create(GuiWidget *widget, void *usrdata)
{
    if(s_audio_mutex == -1)
    {
        GxCore_MutexCreate(&s_audio_mutex);
    }
	app_audio_init();	

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_audio_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_audio_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
#if (DLNA_SUPPORT > 0)
                case VK_DLNA_TRIGGER:
                    GUI_EndDialog(WND_AUDIO);
                    GUI_SendEvent("wnd_full_screen", event);
                    break;
#endif
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_AUDIO);
                    break;

                case STBK_UP:
                case STBK_DOWN:
                    //GUI_SendEvent(LV_AUDIO, event);
                    return EVENT_TRANSFER_STOP;
                case STBK_LEFT:
                case STBK_RIGHT:
                    //GUI_SendEvent(CMB_AUDIO_TRACK, event);
                    return EVENT_TRANSFER_STOP;


                case STBK_EXIT:

                    break;

                default:

                    break;
            }
        default:
            break;
    }

    return ret;
}

