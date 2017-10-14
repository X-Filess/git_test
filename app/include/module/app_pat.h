#ifndef __APP_PAT_H__
#define __APP_PAT_H__

#include "gxcore.h"
#include "gxsi.h"

typedef struct app_pat AppPat;
struct app_pat
{
	uint32_t    ts_src;
	int32_t     subt_id;
	uint32_t    req_id;
	uint32_t    ver;
	void        (*start)(AppPat*);
	uint32_t    (*stop)(AppPat*);
	status_t    (*analyse)(AppPat*, GxMsgProperty_SiSubtableOk*); 
	void        (*cb)(PatInfo *pat_info);
};

extern AppPat       g_AppPat;
void app_pat_modify(void);

#endif
