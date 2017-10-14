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
	int n_key;//虚拟键值
	pcb_stbk_fun pcb_fun;//其对应的处理函数
}stbk_fun_map_s;



/* BEGIN: Added by yingc, 2013/7/2 */
/*定义控件tab 索引*/
#define CONTROL_NAME_MAX   (50)
typedef struct
{
	uint32_t    u_n_tabindex;//控件对应的tab索引(自定义)
	char        s_controlname[CONTROL_NAME_MAX];//控件对应的名字(与xml中的name属性对应)
	//pcb_stbk_fun pcb_fun;//其对应的ok处理函数
}control_tab_s;
/* END:   Added by yingc, 2013/7/2   PN: */



#endif 
