#ifndef __APP_NETAPPS_SERVICE_H__
#define __APP_NETAPPS_SERVICE_H__

typedef void(* gx_netapps_service_func)(void);

void gx_netapps_service_init(void);

void gx_netapps_service_destroy(void);

int gx_netapps_service_push(gx_netapps_service_func func);

#endif
