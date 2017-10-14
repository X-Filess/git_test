#ifndef APP_EPG_H
#define APP_EPG_H
#include "service/gxepg.h"

#define MAX_EPG_DAY 7
#define EVENT_GET_NUM 1
#define EVENT_GET_MAX_SIZE 5000

typedef struct
{
	int cur_cnt;
	int day_cnt[MAX_EPG_DAY];
}EventCnt;

typedef struct
{
	uint8_t day;
	uint8_t valid;
	uint8_t sel;
	uint8_t virtualsel;
}EpgIndex;

typedef struct
{
	uint16_t ts_id;
	uint16_t orig_network_id;
	uint16_t service_id;
	uint32_t prog_id;
	int prog_pos;
}EpgProgInfo;

typedef struct epg_ops AppEpgOps;
struct epg_ops
{
	status_t (*event_cnt_update)(AppEpgOps*);
	status_t (*prog_info_update)(AppEpgOps*, EpgProgInfo*);
	int (*get_day_event_cnt)(AppEpgOps*, int);
	status_t (*get_day_event_info)(AppEpgOps*, EpgIndex*, GxEpgInfo*);
	int (*get_cur_event_cnt)(AppEpgOps*);
	status_t (*get_cur_event_info)(AppEpgOps*, GxEpgInfo*);
	status_t (*get_next_event_info)(AppEpgOps*, GxEpgInfo*);
	EventCnt event_cnt;
	EpgProgInfo prog_info;
};

void epg_ops_init(AppEpgOps * ops);

void app_epg_enable(uint32_t ts_src, uint32_t dmx_id);
void app_epg_disable(void);
void app_epg_info_clean(void);

extern uint32_t app_epg_get_cur_prog_tp_id(void);

#endif
