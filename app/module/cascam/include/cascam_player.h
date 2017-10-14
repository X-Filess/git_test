#ifndef __CASCAM_PLAYER_H__
#define __CASCAM_PLAYER_H__

#include "module/player/gxplayer_module.h"

typedef enum{
    CASCAM_PLAYER_STATUS_STOPPED              ,
    CASCAM_PLAYER_STATUS_PLAY_START           ,
    CASCAM_PLAYER_STATUS_PLAY_PAUSE           ,
    CASCAM_PLAYER_STATUS_PLAY_RUNNING         ,
    CASCAM_PLAYER_STATUS_PLAY_END             ,
}CASCAMPlayerStatus;

int cascam_player_get_vframe(unsigned long long *frame);
int cascam_player_get_status(CASCAMPlayerStatus *status);

#endif
