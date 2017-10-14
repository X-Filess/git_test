#include "app.h"
#if (DLNA_SUPPORT > 0)
#include "jansson.h"
#include "dlna.h"

const static char *actions[DLNA_ACT_TOTAL] =
{
	"play",
	"stop",
	"pause",
	"resume",
	"vol_set",
	"vol_get",
	"seek",
	"get_transport_info",
	"get_position_info",
};

DlnaAction app_get_dlna_action(const char *action)
{
    DlnaAction ret = DLNA_ACT_TOTAL;
    int i = 0;

    if((action == NULL) || (strlen(action) == 0))
        return DLNA_ACT_TOTAL;

    for (i = 0; i < DLNA_ACT_TOTAL; i++)
    {
        if (strcmp(actions[i], action)==0)
        {
            ret = i;
            break;
        }
    }

    return ret;
}

bool ipc_from_dlna(char *action, char *url, char *urimetadata, int volume)
{
    int act = 0;

    //printf("**********[%s]%d action: %s, url: %s urimetadata: %s,volume: %d \n ", __func__, __LINE__, action, url,
    //        urimetadata, volume);
    act = app_get_dlna_action(action);

    switch(act)
    {
        case DLNA_PLAY:
            dplayer.play(url, urimetadata, 0);
            break;
        case DLNA_STOP:
            dplayer.stop();
            break;
        case DLNA_PAUSE:
            dplayer.pause();
            break;
        case DLNA_RESUME:
            dplayer.resume();
            break;
        case DLNA_VOLUME_SET:
            dplayer.vol_set(volume);
            break;

        default:
            return false;
    }
    return true;
}
#endif
