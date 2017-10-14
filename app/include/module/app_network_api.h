#ifndef APP_NETWORK_API_H
#define APP_NETWORK_API_H

#include "app_config.h"
#include <gxcore.h>
#if NETWORK_SUPPORT
status_t app_if_name_convert(const char *if_name, char *display_name);
void app_network_ip_completion(char *ip);
void app_network_ip_simplify(char *buf, char *ip);
#ifdef LINUX_OS

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

char *app_inet_ntoa(struct in_addr in);
void app_get_ethernet_mac(char* pMac);
void app_set_ethernet_mac(const char* pMac);
bool app_check_netlink(const char *if_name); //if_name: eth0,ra0... etc.
status_t app_get_ip_addr(const char *if_name, char *ipaddr);
status_t app_set_ip_addr(const char *if_name, const char *ipaddr);
status_t app_get_netmask(const char *if_name, char *netmask);
status_t app_set_netmask(const char *if_name, const char *netmask);
status_t app_get_bcast_addr(const char *if_name, char *bcast);
status_t app_set_bcast_addr(const char *if_name, const char *bcast);
status_t app_get_gateway(const char *if_name, char *gateway);
#else
bool app_check_netlink(const char *if_name); //if_name: eth0,ra0... etc.
status_t app_get_gateway(const char *if_name, char *gateway);
#endif
#endif
#endif
