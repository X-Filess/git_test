#include "pmp_setting.h"
#include "pmp_dvb_subt.h"
#include "play_manage.h"

static PlayerSubtitle *thiz_subt_handle = NULL;
status_t movie_dvb_subt_stop(void)
{
    if(thiz_subt_handle != NULL)
    {
        GxPlayer_MediaSubUnLoad(PMP_PLAYER_AV, thiz_subt_handle);
        thiz_subt_handle = NULL;
    }

    return GXCORE_SUCCESS;
}

status_t movie_dvb_subt_start(uint8_t subt_index)
{
    PlayerSubPara subpara;

    movie_dvb_subt_stop();

    memset(&subpara, 0 , sizeof(PlayerSubPara));
    subpara.type = PLAYER_SUB_TYPE_DVB;
    subpara.render = PLAYER_SUB_RENDER_SPP;
    subpara.pid.pid = subt_index;
    thiz_subt_handle = GxPlayer_MediaSubLoad(PMP_PLAYER_AV, &subpara);

    pmpset_set_int(PMPSET_MOVIE_SUBT_VISIBILITY, PMPSET_TONE_ON);

    return GXCORE_SUCCESS;
}

status_t movie_dvb_subt_switch(uint8_t subt_index)
{
    PlayerSubPID pid = {0};

    if(thiz_subt_handle == NULL)
        return GXCORE_ERROR;

    pid.pid = subt_index;

    return GxPlayer_MediaSubSwitch(PMP_PLAYER_AV, thiz_subt_handle, pid);
}

status_t movie_dvb_subt_hide(void)
{
    if(thiz_subt_handle == NULL)
        return GXCORE_ERROR;

    return GxPlayer_MediaSubHide(PMP_PLAYER_AV, thiz_subt_handle);
}

status_t movie_dvb_subt_show(void)
{
    if(thiz_subt_handle == NULL)
        return GXCORE_ERROR;

    return GxPlayer_MediaSubShow(PMP_PLAYER_AV, thiz_subt_handle);
}

