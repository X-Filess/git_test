#ifndef __APP_DLNA_H__
#define __APP_DLNA_H__

#include "app_config.h"
#if (DLNA_SUPPORT > 0)

#define MAX_DLNA_URL_LENGTH    (512)
typedef struct{
    char action[100];
    char url[MAX_DLNA_URL_LENGTH];
    char metadata[100];
    int volume;
}GxMsgProperty_DlnaMsg;

typedef void (*DlnaCB)(void);
void app_dlna_ui_exec(DlnaCB dlna_start, DlnaCB dlna_stop);
#endif
#endif
