#include "app.h"
#include "file_view.h"
#include "play_movie.h"
#include "play_manage.h"
#include "pmp_dvb_subt.h"
#include "pmp_psi.h"
#include "stdlib.h"

extern status_t app_movie_play_config(void);
extern status_t app_movie_psi_info_init(char *file_path);
extern int app_movie_dvb_subt_cnt(void);

static char *get_tsfile_full_path(char *ts_dvr, char *ts_file)
{
    int dvr_len = 0;
    int file_len = 0;
    char *str = NULL;
    char *p = NULL;

    if((ts_dvr == NULL) || (strlen(ts_dvr) == 0))
        return NULL;

    if((ts_file == NULL) || (strlen(ts_file) == 0))
        return NULL;

    dvr_len = strlen(ts_dvr);
    str = (char*)GxCore_Calloc(dvr_len + 1, 1);
    if(str == NULL)
        goto err_ret;

    memmove(str, ts_dvr, dvr_len);
    p = strrchr(str, '.');
    if((p == NULL) || (strncmp(p, ".dvr", 4) != 0))
        goto err_ret;

    *p = '\0';
    p = strrchr(str, '.');
    if((p == NULL) || (strncmp(p, ".ts", 3) != 0))
        goto err_ret;

    *p = '\0';

    dvr_len -= 7; //strlen(".ts.dvr");
    file_len = strlen(ts_file) + dvr_len + 1;
    p = (char*)GxCore_Realloc(str, file_len + 1);
    if(p == NULL)
        goto err_ret;

    str = p;
    strcat(str, "/");
    strcat(str, ts_file);

    return str;

err_ret:
    if(str != NULL)
    {
        GxCore_Free(str);
        str = NULL;
    }
    return NULL;
}

status_t play_movie(uint32_t file_no, int start_time_ms)
{
    play_list* list = NULL;
    char *path = NULL;
    char *ts_path = NULL;

    if(app_movie_dvb_subt_cnt() > 0)
    {
        movie_dvb_subt_stop();
    }
    else
    {
        subtitle_destroy();
    }

    list = play_list_get(PLAY_LIST_TYPE_MOVIE);
    if(NULL == list) return GXCORE_ERROR;

    if(file_no >= list->nents)
    {
        return GXCORE_ERROR;
    }

    list->play_no = file_no;
    path = explorer_static_path_strcat(list->path, list->ents[file_no]);
    if(NULL == path) return GXCORE_ERROR;

    char *p = strrchr(path, '.');
    if((p != NULL) && (strncmp(p, ".dvr", 4) == 0))
    {
        ts_path = get_tsfile_full_path(path, "0000.ts");
        if(ts_path != NULL)
        {
            app_movie_psi_info_init(ts_path);
            GxCore_Free(ts_path);
            app_movie_play_config();
        }
    }
    else
    {
        app_movie_psi_info_init(path);
        app_movie_play_config();
    }
    printf("[PLAY] movie: %s, start: %d\n", path, start_time_ms);

#ifdef MENCENT_FREEE_SPACE
    GUI_SetInterface("free_space", "fragment|spp|osd|back_osd");
    GxCore_HwCleanCache();
#endif
    play_av_by_url(path, start_time_ms);

    if(app_movie_dvb_subt_cnt() == 0)
    {
#define TEXT_SUBT                       "movie_view_canvas_subt"
        /*subt create*/
        subtitle_create(TEXT_SUBT);
    }

    return GXCORE_SUCCESS;
}

status_t play_movie_ctrol(play_movie_ctrol_state ctrol)
{
	status_t ret = GXCORE_ERROR;
	play_list* list = NULL;
	uint32_t file_no = 0;
	uint32_t file_count = 0;
	GxMessage *msg;


	list = play_list_get(PLAY_LIST_TYPE_MOVIE);
	if(NULL == list) return GXCORE_ERROR;
	
	file_no = list->play_no;
	file_count = list->nents;
	if(0 == file_count)
	{
		return GXCORE_ERROR;
	}


	if(PLAY_MOVIE_CTROL_PREVIOUS == ctrol)
	{
		if(0 == file_count) return GXCORE_SUCCESS;

		if(1 == file_count)
		{
			file_no = 0;
		}
		else if(file_no <=0)
		{
			file_no = file_count  - 1;
		}
		else
		{
			file_no--;
		}
		ret = play_movie(file_no, 0);
	}
	else if(PLAY_MOVIE_CTROL_NEXT == ctrol)
	{
		if(0 == file_count) return GXCORE_SUCCESS;
		
		if(1 == file_count)
		{
			file_no = 0;
		}
		else if(file_no >= file_count -1)
		{
			file_no = 0;
		}
		else
		{
			file_no++;
		}
		ret = play_movie(file_no, 0);
	}
	else if(PLAY_MOVIE_CTROL_PLAY == ctrol)
	{
		play_movie(list->play_no, 0);
	}
	else if(PLAY_MOVIE_CTROL_PAUSE == ctrol)
	{
		msg = GxBus_MessageNew(GXMSG_PLAYER_PAUSE);
		APP_CHECK_P(msg, GXCORE_ERROR);
		
		GxMsgProperty_PlayerPause *pause;
		pause = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerPause);
		APP_CHECK_P(pause, GXCORE_ERROR);
		pause->player = PMP_PLAYER_AV;

		GxBus_MessageSendWait(msg);
		GxBus_MessageFree(msg);
	}
	else if(PLAY_MOVIE_CTROL_RESUME == ctrol)
	{
		msg = GxBus_MessageNew(GXMSG_PLAYER_RESUME);
		APP_CHECK_P(msg, GXCORE_ERROR);
		
		GxMsgProperty_PlayerResume *resume;
		resume = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerResume);
		APP_CHECK_P(resume, GXCORE_ERROR);
		resume->player = PMP_PLAYER_AV;

		GxBus_MessageSendWait(msg);
		GxBus_MessageFree(msg);
	}	
	else if(PLAY_MOVIE_CTROL_STOP == ctrol)
	{
		msg = GxBus_MessageNew(GXMSG_PLAYER_STOP);
		APP_CHECK_P(msg, GXCORE_ERROR);
		
		GxMsgProperty_PlayerStop *stop;
		stop = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerStop);
		APP_CHECK_P(stop, GXCORE_ERROR);
		stop->player = PMP_PLAYER_AV;

		GxBus_MessageSendWait(msg);
		GxBus_MessageFree(msg);
	}
	else if(PLAY_MOVIE_CTROL_RANDOM== ctrol)
	{
		int random_no = rand();
		random_no = random_no%file_count;
		if(random_no == list->play_no)
		{
			random_no = random_no+1%file_count;
		}
		ret = play_movie(random_no, 0);
	}
	else
	{
		return GXCORE_ERROR;
	}
	
	return GXCORE_SUCCESS;	
}


status_t play_movie_speed(play_movie_ctrol_state ctrol, play_movie_speed_state speed)
{
	GxMsgProperty_PlayerSpeed player_speed;
	GxMessage *msg;
	
	if(PLAY_MOVIE_SPEED_X1 == speed)
	{
		player_speed.speed = (float)speed;
	}
	else
	{
		if(PLAY_MOVIE_CTROL_BACKWARD== ctrol)
		{
			player_speed.speed = (0 - (float)speed);
		}
		else if(PLAY_MOVIE_CTROL_FORWARD== ctrol)
		{
			player_speed.speed = (float)speed;
		}
		else
		{
			player_speed.speed = (float)(PLAY_MOVIE_SPEED_X1);
		}
	}

	printf("start speed %f\n", player_speed.speed);

	msg = GxBus_MessageNew(GXMSG_PLAYER_SPEED);
	APP_CHECK_P(msg, GXCORE_ERROR);
		
	GxMsgProperty_PlayerSpeed *spd;
	spd = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerSpeed);
	APP_CHECK_P(spd, GXCORE_ERROR);
	spd->player = PMP_PLAYER_AV;
	spd->speed = player_speed.speed;
	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);
	
	return GXCORE_SUCCESS;
}

status_t play_movie_zoom(play_movie_zoom_state zoom)
{
	return GXCORE_SUCCESS;
}


status_t play_movie_get_info(play_movie_info *info)
{
	play_list* list = NULL;

	list = play_list_get(PLAY_LIST_TYPE_MOVIE);
	if(NULL == list) return GXCORE_ERROR;

	// TODO:
	memset(info, 0, sizeof(play_movie_info));
	info->no = list->play_no;
	info->path = list->path;
	info->name = list->ents[list->play_no];

	return GXCORE_SUCCESS;
}




