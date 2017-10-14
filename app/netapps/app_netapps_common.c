#include "app.h"
#include "app_module.h"
#if NETAPPS_SUPPORT
#include "app_netapps_common.h"
void gx_netapps_app_init(void)
{
	g_AppPlayOps.program_stop();
	g_AppPvrOps.tms_delete(&g_AppPvrOps); 
}

void gx_netapps_app_destroy(void)
{

}
#endif
