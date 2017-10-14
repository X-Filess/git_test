/**
 *
 * @file        app_common_search.h
 * @brief
 * @version     1.1.0
 * @date        10/18/2012 09:40:49 AM
 * @author      zhouhuaming (zhouhm), zhuohm@nationalchip.com
 *
 */
#ifndef __APP_CABLE_SEARCH__H__
#define __APP_CABLE_SEARCH__H__
#ifdef __cplusplus
extern "C" {
#endif
#include <gxtype.h>

#define CABLE_NETWORK_NAME          ("Cable(DVB-C)")

typedef struct{
	uint32_t 	cable_annex_type;
	uint32_t	cable_qam_type;
	uint32_t	cable_fre;
	uint32_t	cable_sym;
}ClassCableSearch;

extern int CableSearchFrontendSetAnnexType(uint32_t annex_type);
extern int CableSearchFrontendLockTp(uint32_t qam_type, uint32_t fre, uint32_t symbol_rate);
extern void CableSearchStartFilterNit(void);
extern void CableSearchStopFilterNit(void);
extern int CableSearchSortNitTp(void);
extern int CableSearchStartAutoSearch(void);

#ifdef __cplusplus
}
#endif
#endif /*__APP_CABLE_SEARCH__H__*/

