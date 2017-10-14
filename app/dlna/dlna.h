#include "gxcore.h"

typedef enum
{
	DLNA_PLAY,
	DLNA_STOP,
	DLNA_PAUSE,
	DLNA_RESUME,
	DLNA_VOLUME_SET,
	DLNA_VOLUME_GET,
	DLNA_SEEK,
	DLNA_GET_TRANSPORT_INFO,
	DLNA_GET_POSITION_INFO,
	DLNA_ACT_TOTAL,
}DlnaAction;

struct dlna_player
{
	status_t (*play)(const char* url, const char* uriMetadata, int start_time_ms);
	status_t (*stop)(void);
	status_t (*pause)(void);
	status_t (*resume)(void);
	status_t (*vol_set)(int);
	status_t (*vol_get)(int*);
	status_t (*seek)(int32_t time_s);
	status_t (*get_player_status)(PlayerStatusInfo* info);
	status_t (*get_position_info)(uint64_t* current_ms, uint64_t* total_ms);
};

struct dlna_controller
{
	status_t (*play_start)(void);
	status_t (*play_resume)(void);
	status_t (*play_stop)(void);
	status_t (*play_pause)(void);
	status_t (*set_volume)(int);
	status_t (*get_volume)(int*);
	status_t (*get_transport_info)(const char*);
};

extern struct dlna_player dplayer;
extern struct dlna_controller dcontroller;
DlnaAction app_get_dlna_action(const char *action);
