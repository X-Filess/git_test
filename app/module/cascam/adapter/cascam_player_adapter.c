#include "../include/cascam_player.h"

int cascam_player_get_vframe(unsigned long long *frame)
{
    PlayerProgInfo info = {{0},};

    if (0 == GxPlayer_MediaGetProgInfoByName("player1", &info)) {
        *frame = info.video_stat.play_frame_cnt;
        return 0;
    }

    return -1;
}

int cascam_player_get_status(CASCAMPlayerStatus *status)
{
    PlayerStatusInfo info = {0};

    if (0 == GxPlayer_MediaGetStatus("player1", &info)) {
        if (PLAYER_STATUS_STOPPED == info.status) {
            *status = CASCAM_PLAYER_STATUS_STOPPED;
            return 0;
        }
        else if (PLAYER_STATUS_PLAY_START == info.status) {
            *status = CASCAM_PLAYER_STATUS_PLAY_START;
            return 0;
        }
        else if (PLAYER_STATUS_PLAY_PAUSE == info.status) {
            *status = CASCAM_PLAYER_STATUS_PLAY_PAUSE;
            return 0;
        }
        else if (PLAYER_STATUS_PLAY_RUNNING == info.status) {
            *status = CASCAM_PLAYER_STATUS_PLAY_RUNNING;
            return 0;
        }
        else if (PLAYER_STATUS_PLAY_END == info.status) {
            *status = CASCAM_PLAYER_STATUS_PLAY_END;
            return 0;
        }
        else {
            return -1;
        }
    }

    return -1;
}
