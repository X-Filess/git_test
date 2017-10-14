#ifndef APP_NETVIDEO_PLAY_H
#define APP_NETVIDEO_PLAY_H

typedef struct
{
	void (*play_ok)(void);
	void (*play_exit)(void);
	void (*play_prev)(void);
	void (*play_next)(void);
	void (*play_restart)(uint64_t cur_time);
}NetVideoPlayCtrl;

typedef struct
{
	void (*get_url)(int);
	char *netvideo_title;
	int source_num;
	bool seek_support;
	NetVideoPlayCtrl play_ctrl;
}NetVideoPlayOps;

typedef enum
{
    PLAY_STATE_NONE,
    PLAY_STATE_LOAD,
    PLAY_STATE_RUNNING,
    PLAY_STATE_ERROR
}NetPlayState;

typedef enum
{
    PLAY_PLAYER_ERROR,
    PLAY_CONNECT_ERROR,
    PLAY_URL_NONE,
    PLAY_SYSTEM_BUSY,
    PLAY_USER_ERROR,
}NetPlayErrorType;

status_t app_net_video_play(NetVideoPlayOps *play_ops);
status_t app_net_video_start_play(char *url, NetPlayState play_state, int64_t start, bool show_info);
void app_net_video_set_error_str(char *errstr);
void app_net_video_switch_source(int index);
#endif
