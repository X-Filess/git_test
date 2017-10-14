#ifndef __APP_SDT_H__
#define __APP_SDT_H__

#include "gxcore.h"
#include "gxsi.h"

typedef struct app_sdt AppSdt;
struct app_sdt
{
	uint32_t    ts_src;
	int32_t     subt_id;
	uint32_t    req_id;
	uint32_t    ver;
	void        (*start)(AppSdt*);
	uint32_t    (*stop)(AppSdt*);
	status_t    (*analyse)(AppSdt*, GxMsgProperty_SiSubtableOk*); 
	void        (*cb)(SdtInfo *sdt_info);
};

extern AppSdt       g_AppSdt;
void app_Sdt_modify(void);

#endif
