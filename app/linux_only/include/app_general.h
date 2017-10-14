#ifndef APP_GENERAL_H
#define APP_GENERAL_H

//#ifdef LINUX_OS
//#include "app_utility.h"
#include "comm_log.h"
//#endif

#if defined(__cplusplus)
#define	__BEGIN_DECLS	extern "C" {
#define	__END_DECLS	}
#else
#define	__BEGIN_DECLS
#define	__END_DECLS
#endif

typedef int (*pcb_stbk_fun)(GUI_Event *event) ;

typedef struct {
	int n_key;//�����ֵ
	pcb_stbk_fun pcb_fun;//���Ӧ�Ĵ�����
}stbk_fun_map_s;



/* BEGIN: Added by yingc, 2013/7/2 */
/*����ؼ�tab ����*/
#define CONTROL_NAME_MAX   (50)
typedef struct
{
	uint32_t    u_n_tabindex;//�ؼ���Ӧ��tab����(�Զ���)
	char        s_controlname[CONTROL_NAME_MAX];//�ؼ���Ӧ������(��xml�е�name���Զ�Ӧ)
	//pcb_stbk_fun pcb_fun;//���Ӧ��ok������
}control_tab_s;
/* END:   Added by yingc, 2013/7/2   PN: */



#endif 
