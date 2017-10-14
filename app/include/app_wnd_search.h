#ifndef APP_SEARCH_H
#define APP_SEARCH_H

#include "app.h"

typedef enum
{
	SEARCH_SAT,
	SEARCH_BLIND_ONLY,
	SEARCH_TP,
	SEARCH_PID,
	SEARCH_CABLE,
    SEARCH_DVBT2,
    SEARCH_DTMB,
	SEARCH_NONE
}SearchType;

typedef struct
{
	uint32_t id_num;
	uint32_t *id_array;
}SearchIdSet;

#if (DEMOD_DVB_S > 0)
WndStatus app_search_opt_dlg(SearchType type, SearchIdSet *id);
void app_search_start(uint32_t tuner);

int app_antenna_setting_init(int SatId, int TpSel ,int sat_SatSel);
int app_satellite_list_init(int SatSel);
int app_tp_list_init(int SatSel, int TpSel);

uint32_t app_tuner_cur_tuner_get(void);
void app_tuner_cur_tuner_set(uint32_t CurTuner);
#endif

#endif

