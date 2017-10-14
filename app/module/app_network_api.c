#include "app.h"
#include "app_module.h"
#include "app_str_id.h"

#if NETWORK_SUPPORT
void app_network_ip_simplify(char *buf, char *ip)
{
	char *p;
	char tmp[16];
	int num[4] = {0, 0, 0, 0};

	strcpy(tmp, ip);
	num[0] = atoi(tmp);

	p = strchr(tmp, '.');
	p++;
	num[1] = atoi(p);

	p = strchr(p, '.');
	p++;
	num[2] = atoi(p);

	p = strchr(p, '.');
	p++;
	num[3] = atoi(p);

	sprintf(buf, "%03d.%03d.%03d.%03d", num[0], num[1], num[2], num[3]);
}
void app_network_ip_completion(char *ip)
{
	char *p;
	char buf[15];
	int num[4] = {0, 0, 0, 0};

	strcpy(buf, ip);

	num[0] = atoi(buf);

	p = strchr(buf, '.');
	if(p== NULL)
		goto END;
	p++;
	num[1] = atoi(p);

	p = strchr(p, '.');
	if(p== NULL)
		goto END;
	p++;
	num[2] = atoi(p);

	p = strchr(p, '.');
	if(p== NULL)
		goto END;
	p++;
	num[3] = atoi(p);

END:
	memset(ip, 0, 16);
	sprintf(ip, "%03d.%03d.%03d.%03d", num[0], num[1], num[2], num[3]);
}
status_t app_if_name_convert(const char *if_name, char *display_name)
{
    uint8_t num_offset = 0;
    int dev_num = 0;
    char *name_head = NULL;

    if((if_name == NULL) || (display_name == NULL))
    {
        return GXCORE_ERROR;
    }

    if(strncmp(if_name, "eth", 3) == 0) //wired
    {
        name_head = STR_ID_WIRED;
        num_offset = 3;
    }
    else if (strncmp(if_name, "ra", 2) == 0)
    {
        name_head = STR_ID_WIFI;
        num_offset = 2;
    }
    else if (strncmp(if_name, "wlan", 4) == 0)
    {
        name_head = STR_ID_WIFI;
        num_offset = 4;
    }
    else if (strncmp(if_name, "USB3G", 5) == 0)
    {
        name_head = STR_ID_3G;
        num_offset = 5;
    }
    else
    {
        strcpy(display_name, if_name);
    }

    if(name_head != NULL)
    {
        dev_num = atoi(if_name + num_offset);
        if(dev_num > 0)
        {
            sprintf(display_name, "%s-%d", name_head, dev_num);
        }
        else
        {
            sprintf(display_name, "%s", name_head);
        }
    }

    return GXCORE_SUCCESS;
}
#ifdef LINUX_OS

bool app_check_netlink(const char *if_name) //if_name: eth0,ra0... etc.
{
    bool ret = false;
    int skfd = -1;
    struct ifreq ifr;
    struct ethtool_value edata;
    char buffer[512] ={0};
    char cmd_buff[100] = {0};
    FILE *read_fp = NULL;
    int chars_read = 0;

    if((if_name == NULL) || (strlen(if_name) >= 10) || (strlen(if_name) == 0))
    {
        return false;
    }

    sprintf(cmd_buff, "ifconfig %s 2>/dev/null | grep Ethernet 2>/dev/null", if_name);
    read_fp = popen(cmd_buff, "r");
    if (read_fp != NULL)
    {
        chars_read = fread(buffer, sizeof(char), 511, read_fp);
        if (chars_read > 0)
        {
            ret = true;
        }
        else
        {
            //perror("xxxxxxxxxx fread == 0 xxxxxxxxxx");
        }
        pclose(read_fp);
    }
    else
    {
        //perror("xxxxxxxxxx popen == NULL xxxxxxxxxx");
    }

    if((ret == true) && (strncmp((char *)if_name, "eth", 3) == 0)) //wired
    {
        edata.cmd = ETHTOOL_GLINK;
        edata.data = 0;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
        ifr.ifr_data = (char *) &edata;

        if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0)
        {
            ret = false;
        }
        else
        {
            if(ioctl( skfd, SIOCETHTOOL, &ifr ) < 0)
            {
                ret = false;
            }

            if(edata.data != 1)
            {
                ret = false;
            }
            close(skfd);
        }
    }

    return ret;
}

char *app_inet_ntoa(struct in_addr in)
{
    static char s_add_buffer[18] = {0};
    unsigned char *bytes = (unsigned char *) &in;
    memset(s_add_buffer, 0 , 18);
    sprintf (s_add_buffer, "%03d.%03d.%03d.%03d",
            bytes[0], bytes[1], bytes[2], bytes[3]);
    return s_add_buffer;
}

status_t app_get_ip_addr(const char *if_name, char *ipaddr)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *sa = NULL;

    if((if_name == NULL) || (ipaddr == NULL))
    {
        return GXCORE_ERROR;
    }

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return GXCORE_ERROR;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0)
    {
        close(skfd);
        return GXCORE_ERROR;
    }

	sa = (struct sockaddr_in *)(&ifr.ifr_addr);
    strcpy(ipaddr, app_inet_ntoa(sa->sin_addr));

	close(skfd);
    return GXCORE_SUCCESS;
}

status_t app_set_ip_addr(const char *if_name, const char *ipaddr)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *sa = NULL;

    if((if_name == NULL) || (ipaddr == NULL))
    {
        return GXCORE_ERROR;
    }

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return GXCORE_ERROR;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
	sa = (struct sockaddr_in*)&ifr.ifr_addr;
	sa->sin_family = AF_INET;
	inet_aton(ipaddr, &sa->sin_addr);
    if (ioctl(skfd, SIOCSIFADDR, &ifr) < 0)
    {
        close(skfd);
        return GXCORE_ERROR;
    }

	close(skfd);
    return GXCORE_SUCCESS;
}

status_t app_get_netmask(const char *if_name, char *netmask)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *sa = NULL;

    if((if_name == NULL) || (netmask == NULL))
    {
        return GXCORE_ERROR;
    }

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return GXCORE_ERROR;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    if (ioctl(skfd, SIOCGIFNETMASK, &ifr) < 0)
    {
        close(skfd);
        return GXCORE_ERROR;
    }
	sa = (struct sockaddr_in *)(&ifr.ifr_netmask);
    strcpy(netmask, app_inet_ntoa(sa->sin_addr));

	close(skfd);
    return GXCORE_SUCCESS;
}

status_t app_set_netmask(const char *if_name, const char *netmask)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *sa = NULL;

    if((if_name == NULL) || (netmask == NULL))
    {
        return GXCORE_ERROR;
    }

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return GXCORE_ERROR;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
	sa = (struct sockaddr_in*)&ifr.ifr_netmask;
	sa->sin_family = AF_INET;
	inet_aton(netmask, &sa->sin_addr);
    if (ioctl(skfd, SIOCSIFNETMASK, &ifr) < 0)
    {
        close(skfd);
        return GXCORE_ERROR;
    }

	close(skfd);
    return GXCORE_SUCCESS;
}

status_t app_get_bcast_addr(const char *if_name, char *bcast)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *sa = NULL;

    if((if_name == NULL) || (bcast == NULL))
    {
        return GXCORE_ERROR;
    }

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return GXCORE_ERROR;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    if (ioctl(skfd, SIOCGIFBRDADDR, &ifr) < 0)
    {
        close(skfd);
        return GXCORE_ERROR;
    }
	sa = (struct sockaddr_in *)(&ifr.ifr_broadaddr);
    strcpy(bcast, app_inet_ntoa(sa->sin_addr));

	close(skfd);
    return GXCORE_SUCCESS;
}

status_t app_set_bcast_addr(const char *if_name, const char *bcast)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *sa = NULL;

    if((if_name == NULL) || (bcast == NULL))
    {
        return GXCORE_ERROR;
    }

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return GXCORE_ERROR;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
	sa = (struct sockaddr_in*)&ifr.ifr_broadaddr;
	sa->sin_family = AF_INET;
	inet_aton(bcast, &sa->sin_addr);
    if (ioctl(skfd, SIOCGIFBRDADDR, &ifr) < 0)
    {
        close(skfd);
        return GXCORE_ERROR;
    }

	close(skfd);
    return GXCORE_SUCCESS;
}

#if 0
status_t app_get_mac_addr(const char *if_name, char *mac)
{
    int skfd;
    struct ifreq ifr;

    if((if_name == NULL) || (mac == NULL))
    {
        return GXCORE_ERROR;
    }

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return GXCORE_ERROR;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0)
    {
        close(skfd);
        return GXCORE_ERROR;
    }
	sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char)ifr.ifr_hwaddr.sa_data[0],
            (unsigned char)ifr.ifr_hwaddr.sa_data[1],
            (unsigned char)ifr.ifr_hwaddr.sa_data[2],
            (unsigned char)ifr.ifr_hwaddr.sa_data[3],
            (unsigned char)ifr.ifr_hwaddr.sa_data[4],
            (unsigned char)ifr.ifr_hwaddr.sa_data[5]);

	close(skfd);
    return GXCORE_SUCCESS;
}
#endif

status_t app_get_gateway(const char *if_name, char *gateway)
{
    FILE *read_fp;
    char cmdline[60] = {0};
    char temp_buf[18] = {0};
    char temp_buf2[18] = {0};
    char *p1 = NULL;
    char *p2 = NULL;
    char len = 0;

    if((if_name == NULL) || (gateway == NULL))
    {
        return GXCORE_ERROR;
    }

    strcpy(cmdline, "route -n | grep ");
    strcat(cmdline, if_name);
    strcat(cmdline, " | grep UG | awk '{print $2}'");

    read_fp = popen(cmdline, "r");
    if (read_fp != NULL)
    {
        if(fread(temp_buf, 1, 17, read_fp) > 0)
        {
            p1 = temp_buf;
            p2 = p1;
            while(p1 != NULL)
            {
               if((*p1 == '.') || (*p1 == '\0') || (*p1 == '\n'))
               {
                   len = p1 - p2;
                   if(len == 3)
                   {
                   }
                   else if(len == 2)
                   {
                       strcat(temp_buf2, "0");
                   }
                   else if(len == 1)
                   {
                       strcat(temp_buf2, "00");
                   }
                   else
                   {
                       return GXCORE_ERROR;
                   }
                   strncat(temp_buf2, p2, len);

                   if(*p1 == '.')
                   {
                       strcat(temp_buf2, ".");
                       p2 = p1 + 1;
                   }
                   else
                   {
                       break;
                   }
               }
               p1++;
            }
		    pclose(read_fp);
            strcpy(gateway, temp_buf2);
            gateway[strlen(temp_buf2)] = '\0';
			return GXCORE_SUCCESS;
        }
        pclose(read_fp);
    }
	return GXCORE_ERROR;
}

status_t app_if_up(const char *if_name)
{
    char cmd_buff[40] = {0};
    FILE *read_fp = NULL;

    if((if_name == NULL) || (strlen(if_name) >= 10) || (strlen(if_name) == 0))
        return GXCORE_ERROR;

    sprintf(cmd_buff, "ifconfig %s up", if_name);
    read_fp = popen(cmd_buff, "r");
    if (read_fp != NULL)
    {
        pclose(read_fp);
    }
    return GXCORE_SUCCESS;
}

status_t app_if_down(const char *if_name)
{
    char cmd_buff[40] = {0};
    FILE *read_fp = NULL;

    if((if_name == NULL) || (strlen(if_name) >= 10) || (strlen(if_name) == 0))
        return GXCORE_ERROR;

    sprintf(cmd_buff, "ifconfig %s down", if_name);
    read_fp = popen(cmd_buff, "r");
    if (read_fp != NULL)
    {
        pclose(read_fp);
    }

    return GXCORE_SUCCESS;
}

status_t app_if_name_convert(const char *if_name, char *display_name)
{
    uint8_t num_offset = 0;
    int dev_num = 0;
    char *name_head = NULL;

    if((if_name == NULL) || (display_name == NULL))
    {
        return GXCORE_ERROR;
    }

    if(strncmp(if_name, "eth", 3) == 0) //wired
    {
        name_head = STR_ID_WIRED;
        num_offset = 3;
    }
    else if (strncmp(if_name, "ra", 2) == 0)
    {
        name_head = STR_ID_WIFI;
        num_offset = 2;
    }
    else if (strncmp(if_name, "wlan", 4) == 0)
    {
        name_head = STR_ID_WIFI;
        num_offset = 4;
    }
    else if (strncmp(if_name, "USB3G", 5) == 0)
    {
        name_head = STR_ID_3G;
        num_offset = 5;
    }
    else
    {
        strcpy(display_name, if_name);
    }

    if(name_head != NULL)
    {
        dev_num = atoi(if_name + num_offset);
        if(dev_num > 0)
        {
            sprintf(display_name, "%s-%d", name_head, dev_num);
        }
        else
        {
            sprintf(display_name, "%s", name_head);
        }
    }

    return GXCORE_SUCCESS;
}




#else



status_t app_get_gateway(const char *if_name, char *gateway)
{
    //TODO
    return GXCORE_ERROR;
}

bool app_check_netlink(const char *if_name) //if_name: eth0,ra0... etc.
{
    //TODO
    return true;
}
#endif
#endif

