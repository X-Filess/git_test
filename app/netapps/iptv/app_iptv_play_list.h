#ifndef APP_IPTV_LIST_H
#define APP_IPTV_LIST_H

typedef struct
{
    void (*group_change)(int group);
    void (*prog_change)(int prog);
    void (*list_ok)(int group, int prog);
    void (*list_exit)(void);
}IPTVListCb;

status_t app_iptv_list_create(IPTVListCb *list_cb);
status_t app_iptv_list_group_update(int group_total, char **group_title);
status_t app_iptv_list_prog_update(int prog_total, char **prog_title);
void app_iptv_list_set_group_sel(int group_sel);
void app_iptv_list_set_prog_sel(int prog_sel);
#endif
