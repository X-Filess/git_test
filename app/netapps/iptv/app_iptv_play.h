#ifndef APP_IPTV_PLAY_H
#define APP_IPTV_PLAY_H

typedef struct _IPTVPlayCtrl
{
	void (*play_ok)(void);
	void (*play_exit)(void);
	void (*play_prev)(void);
	void (*play_next)(void);
	void (*play_restart)(uint64_t cur_time);
}IPTVPlayCtrl;

typedef enum _IPTVPreMediaTypeEnum
{
    IPTV_PRE_NONE = 0,
    IPTV_PRE_AV,
    IPTV_PRE_PIC,
}IPTVPreMediaTypeEnum;

typedef struct _IPTVPreMeidaClass //eg. AD
{
    char *pre_media_name;
    IPTVPreMediaTypeEnum pre_media_type;
    char *pre_media_url;
}IPTVPreMediaClass;

typedef struct _IPTVPlayOps
{
	char *prog_name;
	char *prog_url;
    IPTVPreMediaClass pre_media;
	IPTVPlayCtrl play_ctrl;
}IPTVPlayOps;

typedef enum _IPTVPlayState
{
    PLAY_STATE_NONE,
    PLAY_STATE_LOAD,
    PLAY_STATE_RUNNING,
    PLAY_STATE_ERROR
}IPTVPlayState;

status_t app_iptv_play(IPTVPlayOps *play_ops);
#endif
