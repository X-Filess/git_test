#ifndef __APP_TKGS_BACKGROUND_UPGRADE_H__
#define __APP_TKGS_BACKGROUND_UPGRADE_H__

#include "gxcore.h"
#include "gxsi.h"
#include "module/gxtkgs.h"

#if TKGS_SUPPORT

#define TKGS_BACKUPGRADE_STATUS_FALSE 0x00
#define TKGS_BACKUPGRADE_START 		0x1
#define TKGS_BACKUPGRADE_RUNNING		0x2
#define TKGS_BACKUPGRADE_STOP			0x4
#define TKGS_BACKUPGRADE_PAUSE		0x8

#define TKGS_BACKUPGRADE_PAT_CREATE 	0x200
#define TKGS_BACKUPGRADE_PAT_TIMEOUT 	0x400
#define TKGS_BACKUPGRADE_PAT_RECEIVED	0x800
#define TKGS_BACKUPGRADE_PAT_STOPING	0x1000
#define TKGS_BACKUPGRADE_PAT_RELEASED	0x2000

#define TKGS_BACKUPGRADE_PMT_CREATE 	0x80000
#define TKGS_BACKUPGRADE_PMT_TIMEOUT	0x100000
#define TKGS_BACKUPGRADE_PMT_RECEIVED	0x200000
#define TKGS_BACKUPGRADE_PMT_STOPING	0x400000
#define TKGS_BACKUPGRADE_PMT_RELEASED	0x800000



/*pat�������*/
typedef struct {
	uint16_t                prog_number;	///<�൱��service id
	uint16_t                pmt_pid;
	uint32_t 				pat_version;
} GxSearchPatBody;

#define FILTER_MAX 64

struct app_tkgsBackUpgrade
{
	//external, only start need
	uint32_t    search_status;	//����״̬
	uint32_t    ts_src;
	uint32_t    sat_id;
	uint32_t    tp_id;
	uint32_t    search_demux_id;
	//internal
	GxSearchIdBody    search_pat_subtable_id;///<pat��subtable id
	uint32_t      search_pat_time_out;///<patt�ĳ�ʱʱ��
	GxSearchPatBody    *search_pat_body;
	uint32_t    search_pat_body_count;
	uint32_t    search_pat_finish_flag;
	
	/*pmt*/
	GxSearchIdBody  search_pmt_subtable_id[FILTER_MAX];///<��¼pmt��subtable id
	uint32_t        search_pmt_time_out;///<pmt�ĳ�ʱʱ��
	GxSearchPmtBody  search_pmt_body;
	uint32_t search_pmt_count;	///<���ڽ�����pmt������,���Ӧ�õ���search_pat_body_count
	uint32_t search_pmt_filter_count;	///<���ڽ���pmt��filter������,������Ӧ�õ���search_pat_body_count
	uint32_t search_pmt_finish_count;	///<�����õ�pmt������,���Ӧ�õ���search_pat_body_count

	uint32_t         search_stream_body_count;
	GxBusPmDataStream *search_stream_body ;

	
	uint32_t tkgs_component_flag;
	uint32_t backUpgradeFlag;
	uint16_t pid;
	status_t    (*init)(uint32_t sat_id,uint32_t tp_id, uint32_t ts_src);
	status_t    (*start)(void);
	status_t    (*stop)(void);
	status_t  (*get)(GxParseResult * );
	status_t  (*timeout)(GxParseResult *);
	//private_table_parser pmt_parse_fun;
      private_parse_status (*cb)(uint8_t *p_section_data, uint32_t len);
};
typedef struct app_tkgsBackUpgrade AppTkgsBackUpgrade;

extern AppTkgsBackUpgrade g_AppTkgsBackUpgrade;
#endif
#endif

