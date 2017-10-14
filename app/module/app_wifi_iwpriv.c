#include "app_config.h"

#ifndef LINUS_OS
#if NETWORK_SUPPORT && WIFI_SUPPORT

//#define __ECOS
#include <sys/types.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <pkgconf/system.h>

#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>

#include <cyg/io/eth/netdev.h>
#include <string.h>

#include <shell.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_arp.h>
#include <cyg/io/eth/eth_drv.h>

#include "gxcore.h"
#include "app.h"
#include "app_module.h"
#include <string.h>
#include <dhcp.h>

#define SIOCIWFIRSTPRIV 0x8BE0
#define RT_PRIV_IOCTL (SIOCIWFIRSTPRIV + 0x01)
#define RTPRIV_IOCTL_SET (SIOCIWFIRSTPRIV + 0x02)
#define RTPRIV_IOCTL_BBP (SIOCIWFIRSTPRIV + 0x03)
#define RTPRIV_IOCTL_MAC (SIOCIWFIRSTPRIV + 0x05)
#define RTPRIV_IOCTL_E2P (SIOCIWFIRSTPRIV + 0x07)
#define RTPRIV_IOCTL_STATISTICS (SIOCIWFIRSTPRIV + 0x09)
#define RTPRIV_IOCTL_GSITESURVEY (SIOCIWFIRSTPRIV + 0x0D)
#define RTPRIV_IOCTL_GET_MAC_TABLE (SIOCIWFIRSTPRIV + 0x0F)
#define RTPRIV_IOCTL_SHOW (SIOCIWFIRSTPRIV + 0x11)
#define RTPRIV_IOCTL_RF (SIOCIWFIRSTPRIV + 0x13)

#define RTPRIV_IOCTL_FLAG_UI 0x0001	/* Notidy this private cmd send by UI. */
#define RTPRIV_IOCTL_FLAG_NODUMPMSG 0x0002	/* Notify driver cannot dump msg to stdio/stdout when run this private ioctl cmd */
#define RTPRIV_IOCTL_FLAG_NOSPACE 0x0004	/* Notify driver didn't need copy msg to caller due to the caller didn't reserve space for this cmd */

extern char *strsep(char **s, const char *ct);

typedef struct
{
	const char*	cmd;					// Input command string
	int	(*func)(int argc, char* argv[]);
	const char*	msg;					// Help message
} COMMAND_TABLE;

// Define Linux ioctl relative structure, keep only necessary things
struct iw_point
{
	void *		pointer;
	unsigned short		length;
	unsigned short		flags;
};
	
union iwreq_data
{
	struct iw_point data;
};

struct iwreq {
	union
	{
		char    ifrn_name[IFNAMSIZ];    /* if name, e.g. "eth0" */
	} ifr_ifrn;
	
	union   iwreq_data      u;
};

typedef struct eth_drv_sc	*PNET_DEV;

#ifndef NULL
#define NULL	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

extern int rt28xx_sta_ioctl(
	PNET_DEV	pEndDev, 
	caddr_t		pData,
	int			cmd);

static char TempString[7168];
static handle_t s_aplist_mutex = -1;

int iwpriv(char *dev, char *pcmd, char *paras)
{
    int cmd = -1;
    cyg_netdevtab_entry_t *t;
    struct eth_drv_sc *sc;
    struct iwreq param;
    int found = 0;

    if (!pcmd || !dev)
        goto error_exit;

    /*show command*/
    printf("\033[31m");
    printf("iwpriv ");
    printf("%s ", dev);
    printf("%s ", pcmd);
    if(paras)
        printf("%s ", paras);
    printf("\033[0m\n");

    if ((strcmp(dev, "ra0") != 0 )
            && (strcmp(dev, "ra1") != 0 )
            && (strcmp(dev, "ra2") != 0 )
            && (strcmp(dev, "ra3") != 0 )
            && (strcmp(dev, "ra4") != 0 )
            && (strcmp(dev, "ra5") != 0 )
            && (strcmp(dev, "ra6") != 0 )
            && (strcmp(dev, "ra7") != 0 )
            && (strcmp(dev, "wds0") != 0 )
            && (strcmp(dev, "apcli0") != 0 )
            && (strcmp(dev, "mesh0") != 0 )
       )
        goto error_exit;

    memset(&param, 0, sizeof(struct iwreq));

    if ( strcmp(pcmd, "set") == 0 )
    {
        cmd = RTPRIV_IOCTL_SET;
    } 
    else if ( strcmp(pcmd, "show") == 0 )
    {
        cmd = RTPRIV_IOCTL_SHOW;
    } 
    else if ( strcmp(pcmd, "e2p") == 0 )
    {
        cmd = RTPRIV_IOCTL_E2P;
    } 
    else if ( strcmp(pcmd, "bbp") == 0 )
    {
        cmd = RTPRIV_IOCTL_BBP;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    } 
    else if ( strcmp(pcmd, "macaddr") == 0 )
    {
        cmd = SIOCGIFHWADDR;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    }
    else if ( strcmp(pcmd, "mac") == 0 )
    {
        cmd = RTPRIV_IOCTL_MAC;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    } 
    else if ( strcmp(pcmd, "rf") == 0 )
    {
        cmd = RTPRIV_IOCTL_RF;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    } 
    else if (( strcmp(pcmd, "GetSiteSurvey") == 0 ) ||( strcmp(pcmd, "get_site_survey") == 0 ))
    {
        cmd = RTPRIV_IOCTL_GSITESURVEY;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    }                 
    else if ( strcmp(pcmd, "stat") == 0 )
    {
        cmd = RTPRIV_IOCTL_STATISTICS;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    }
    else 
    {
        goto error_exit;
    }

    for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
    {
        sc = (struct eth_drv_sc *)t->device_instance;
        if (strcmp(sc->dev_name, dev) == 0) {
            found =1;
            break;
        }
    }

    if ( found == 1 )
    {
        if(-1 == s_aplist_mutex)
            GxCore_MutexCreate(&s_aplist_mutex);

        GxCore_MutexLock(s_aplist_mutex);
        int original_length = 0;
        //int i =0;

        //char *TempString=NULL;
        //TempString =malloc(7168);//7*1024
        //if(TempString==NULL){printf("Not enough memory for CmdIwpriv!\n");return FALSE;}	    
        memset(TempString, 0, sizeof(TempString));
        TempString[0] = '\0';

        if (paras)
        {              
            sprintf(&TempString[0], "%s", paras);
            original_length = strlen(paras);
        }

        param.u.data.pointer = &TempString[0];  
        param.u.data.length = strlen(TempString);
        //printf("cmd:%s [%d][%d]",TempString,param.u.data.length,original_length);
        rt28xx_sta_ioctl(sc, (caddr_t)&param, cmd);
        if (param.u.data.length != original_length)
        {
            TempString[param.u.data.length] = '\0';
            //for (i =0; i < param.u.data.length;i ++)
            //    diag_printf("%c",TempString[i]);
            //diag_printf("%s", TempString);
        }
        GxCore_MutexUnlock(s_aplist_mutex);

        //free(TempString);
        return TRUE;			
    }	

error_exit:
    printf("usage: iwpriv <interface> set|show|e2p|bbp|macaddr|mac|rf <command>\n");
    printf("usage: iwpriv <interface> stat|GetSiteSurvey\n");
    return FALSE;    
}



/*
 * sprintf(msg+strlen(msg),"%-4s%-33s%-20s%-23s%-9s%-7s%-7s%-3s\n",
 *        "Ch", "SSID", "BSSID", "Security", "Siganl(%)", "W-Mode", " ExtCH"," NT");
 */

#define SSID_LEN   (33)
#define MAC_LEN    (20)
#define SECU_LEN   (23)
#define SIG_LEN    (9)
#define RESULT_LEN (4+SSID_LEN+MAC_LEN+SECU_LEN+SIG_LEN+7+7+3+10)  //10:tmp

unsigned short app_check_security(char * pSecu)
{
    unsigned char chAuth = 0;
    unsigned char chEncr = 0;
    int i, j;  

    char data[SECU_LEN + 1] = {0};  
    char tmp[SECU_LEN + 1] = {0};  
    char * p = NULL;

    strcpy(data, pSecu);  
    for(i = 0, j = 0; data[i] != '\0'; i++){  
        if(data[i] != '/'){        //删除字符'/'  
            data[j++] = data[i];  
        }  
    }  
    data[j] = '\0';  
    //printf("%s,len : %d\n", data, strlen(data));  

    if(strncmp(data, "WEP", strlen("WEP")) == 0 && strlen(data) == strlen("WEP"))
    {
        chAuth = WIFI_AUTH_SHARED;
        chEncr = WIFI_ENCRYP_WEP;
    }
    else if(strncmp(pSecu, "NONE", strlen("NONE")) == 0 && strlen(data) == strlen("NONE"))
    {
        chAuth = WIFI_AUTH_OPEN;
        chEncr = WIFI_ENCRYP_NONE;
    }
    else
    {
        //AUTH TYPE
        if((p = strstr(data, "WPA2PSK")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA2PSK;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPA2PSK"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //printf("%s,len : %d\n", data, strlen(data));  
        }
        if((p = strstr(data, "WPA1PSK")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA1PSK;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPA1PSK"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //printf("%s,len : %d\n", data, strlen(data));  
        }
        if((p = strstr(data, "WPAPSK")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA1PSK;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPAPSK"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //printf("%s,len : %d\n", data, strlen(data));  
        }
        if((p = strstr(data, "WPA2")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA2;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPA2"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //printf("%s,len : %d\n", data, strlen(data));  
        }
        if((p = strstr(data, "WPA1")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA1;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPA1"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //printf("%s,len : %d\n", data, strlen(data));  
        }
        if((p = strstr(data, "WPA")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA1;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPA"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //printf("%s,len : %d\n", data, strlen(data));  
        }

        //ENCRYP TYPE
        if((p = strstr(data, "AES")) != NULL)
        {
            chEncr |= WIFI_ENCRYP_AES;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("AES"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //printf("%s,len : %d\n", data, strlen(data));  
        }
        if((p = strstr(data, "TKIP")) != NULL)
        {
            chEncr |= WIFI_ENCRYP_TKIP;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("TKIP"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //printf("%s,len : %d\n", data, strlen(data));  
        }
    }

    //printf("Encrpy Mode : 0x%x 0x%x\n", chAuth, chEncr);
    return (chAuth << 8) | chEncr;
}

unsigned int app_get_aplist(ApInfo *ap_data, unsigned int max_ap_num)
{
    char *tok = NULL;
    char *s = NULL;
    char name[SSID_LEN + 1] = {0};
    char mac[MAC_LEN + 1] = {0};
    char signal[SIG_LEN + 1] = {0};
    char secu[SECU_LEN + 1] = {0};
    unsigned int nApNum = 0;
    unsigned int nLine = 0;
    int i = 0;

    iwpriv("ra0", "set", "SiteSurvey");
    GxCore_ThreadDelay(1000);
    iwpriv("ra0", "GetSiteSurvey", NULL);

    GxCore_MutexLock(s_aplist_mutex);

    s = strdup(TempString);  
    for(tok = strsep(&s, "\n"); tok != NULL; tok = strsep(&s, "\n"))
    {  
        if(strlen(tok) == 0)
            continue;

        printf("\033[33m%s\n\033[0m", tok);
        if(nLine == 0)  //title
        {
            nLine++;
            continue;
        }

        sscanf(tok, "%*4c%33c%20s%23s%9s%*7c%*7c%*3c", name, mac, secu, signal);
        strcpy(ap_data[nApNum].ap_mac, mac);
        ap_data[nApNum].ap_quality = atoi(signal);
        ap_data[nApNum].encryption = app_check_security(secu);

        i = strlen(name) - 1;
        while(name[i] == ' ')
        {
            i--;
        }
        name[i + 1] = '\0'; 
        strcpy(ap_data[nApNum].ap_essid, name);

        //printf("name : %s\n", name);
        memset(name, 0, sizeof(name));
        //printf("mac : %s\n", mac);
        memset(mac, 0, sizeof(mac));
        //printf("secu : %s[0x%x]\n", secu, ap_data[nApNum].encryption);
        memset(secu, 0, sizeof(secu));
        //printf("signal : %s\n", signal);
        memset(signal, 0, sizeof(signal));

        nApNum++;
        if(nApNum == max_ap_num)
            break;
        nLine++;
    }  
    printf("\n");  
    GxCore_MutexUnlock(s_aplist_mutex);

    return nApNum;
}

status_t app_get_security(char *mac, unsigned short *encryption)
{
    status_t ret = GXCORE_ERROR;
    unsigned int ap_num = 0;
    unsigned int i = 0;
    ApInfo *ap_info = NULL;
    static unsigned int nTryCount;

    ap_info = GxCore_Malloc(sizeof(ApInfo)*MAX_AP_NUM);
    if(NULL == ap_info)
        return GXCORE_ERROR;
    memset(ap_info, 0, sizeof(ApInfo)*MAX_AP_NUM);

    nTryCount = 5;
    while(nTryCount > 0 && ret == GXCORE_ERROR)
    {
        if(nTryCount != 5)
            printf("\033[32mTry to get ap security again!!\n\033[0m");
        ap_num = app_get_aplist(ap_info, MAX_AP_NUM);
        for(i = 0; i < ap_num; i++)
        {
            if(strcmp(mac, ap_info[i].ap_mac) == 0)
            {
                *encryption = ap_info[i].encryption;
                ret = GXCORE_SUCCESS;
                break;
            }
        }
        nTryCount--;
    }

    GxCore_Free(ap_info);
    ap_info = NULL;

    return ret;
}

extern int mt7601_connect_status;
status_t _wifi_start(char *wifi_dev, char *essid, char *mac, char *psk)
{
    char cmd[50] = {0}; 
    unsigned short nEncr = 0;
    unsigned char chAuth = 0, chEncr = 0;
    if(wifi_dev == NULL || essid == NULL || mac == NULL)
        return GXCORE_ERROR;

    printf("\033[35m[%s]Name : %s MAC : %s PSK : %s\n\033[0m", wifi_dev, essid, mac, psk);

    if(GXCORE_SUCCESS == app_get_security(mac, &nEncr))
    {
        iwpriv(wifi_dev, "set", "NetworkType=Infra");
        printf("encryption : 0x%x\n", nEncr);
        chAuth = (unsigned char)((nEncr & 0xFF00) >> 8);
        chEncr = (unsigned char)(nEncr & 0x00FF);

        if((chAuth & WIFI_AUTH_WPA2PSK) == WIFI_AUTH_WPA2PSK) {
            iwpriv(wifi_dev, "set", "AuthMode=WPA2PSK");
        } else {
            if((chAuth & WIFI_AUTH_WPA1PSK) == WIFI_AUTH_WPA1PSK) {
                iwpriv(wifi_dev, "set", "AuthMode=WPAPSK");
            } else {
                if((chAuth & WIFI_AUTH_SHARED) == WIFI_AUTH_SHARED) {
                    iwpriv(wifi_dev, "set", "AuthMode=SHARED");
                } else {
                    if((chAuth & WIFI_AUTH_OPEN) == WIFI_AUTH_OPEN) {
                        iwpriv(wifi_dev, "set", "AuthMode=OPEN");
                    }
                }
            }
        }

        //memset(cmd, 0, sizeof(cmd));
        //snprintf(cmd, sizeof(cmd), "SSID=%s", essid);
        //iwpriv(wifi_dev, "set", cmd);

        if((chEncr & WIFI_ENCRYP_AES) == WIFI_ENCRYP_AES) {
            iwpriv(wifi_dev, "set", "EncrypType=AES");
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "WPAPSK=%s", psk);
            iwpriv(wifi_dev, "set", cmd);
        } else {
            if((chEncr & WIFI_ENCRYP_TKIP) == WIFI_ENCRYP_TKIP) {
                iwpriv(wifi_dev, "set", "EncrypType=TKIP");
                memset(cmd, 0, sizeof(cmd));
                snprintf(cmd, sizeof(cmd), "WPAPSK=%s", psk);
                iwpriv(wifi_dev, "set", cmd);
            } else {
                if((chEncr & WIFI_ENCRYP_WEP) == WIFI_ENCRYP_WEP) {
                    iwpriv(wifi_dev, "set", "EncrypType=WEP");
                    iwpriv(wifi_dev, "set", "DefaultKeyID=1");
                    memset(cmd, 0, sizeof(cmd));
                    snprintf(cmd, sizeof(cmd), "Key1=%s", psk);
                    iwpriv(wifi_dev, "set", cmd);
                } else {
                    if((chEncr & WIFI_ENCRYP_NONE) == WIFI_ENCRYP_NONE) {
                        iwpriv(wifi_dev, "set", "EncrypType=NONE");
                    }
                }
            }
        }

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "SSID=%s", essid);
        iwpriv(wifi_dev, "set", cmd);

        printf("\033[31mset wifi status = 0 !!\033[0m\n");
        mt7601_connect_status = 0;
        GxBus_ConfigSet(WIFI_STATUS_KEY, "wait");

        return GXCORE_SUCCESS;
    }

    GxCore_ThreadDelay(100);
    printf("\033[31mCann't find current wifi !!\033[0m\n");
    GxBus_ConfigSet(WIFI_STATUS_KEY, "reject");

    return GXCORE_ERROR;
}

int app_check_wifi_status(void)
{
    return mt7601_connect_status;
}

int app_get_wifi_mac(char * mac)
{
    iwpriv("ra0", "macaddr", NULL);

    GxCore_MutexLock(s_aplist_mutex);

    memcpy(mac, TempString, sizeof("00:00:00:00:00:00")); //00:00:00:00:00:00

    GxCore_MutexUnlock(s_aplist_mutex);

    return 0;
}

#if 0
extern struct bootp ra0_bootp_data;
extern cyg_uint8 ra0_dhcpstate;
bool app_check_dhcp_status(void)
{
    return (ra0_dhcpstate == DHCPSTATE_BOUND) ? true : false;
}

char * app_get_dhcp_ip(void)
{
    return inet_ntoa(ra0_bootp_data.bp_yiaddr);
}

/*
 *
            case TAG_SUBNET_MASK:
            case TAG_GATEWAY:
            case TAG_DOMAIN_SERVER:
 *
 */
static char dns[(3*4+3)*2 + 3] = {0}; //[xxx.xxx.xxx.xxx,xxx.xxx.xxx.xxx]
char * app_get_dhcp_msg(unsigned char nTag)
{
    int i, len;
    unsigned char *op, optover;
    char  *ap = 0;
    struct in_addr addr[32];
    unsigned int length;
    struct bootp *bp = &ra0_bootp_data;
    static int nDnsNum = 0;
    
    optover = 0; // See whether sname and file are overridden for options
    length = sizeof(optover);
    (void)get_bootp_option( bp, TAG_DHCP_OPTOVER, &optover, &length );
    if (bp->bp_vend[0]) {
        op = &bp->bp_vend[4];
        while (*op != TAG_END) {
            switch (*op) {
            case TAG_PAD:
                op++;
                continue;
            case TAG_SUBNET_MASK:
            case TAG_GATEWAY:
            case TAG_IP_BROADCAST:
            case TAG_DOMAIN_SERVER:
                ap = (char *)&addr[0];
                len = *(op+1);
                for (i = 0;  i < len;  i++) {
                    *ap++ = *(op+i+2);
                }

                ap = (char *)&addr[0];

                if ((*op == TAG_SUBNET_MASK || *op == TAG_GATEWAY || *op == TAG_IP_BROADCAST) && (*op == nTag))
                    return inet_ntoa(*(struct in_addr *)ap);

                if(*op == TAG_DOMAIN_SERVER && *op == nTag)
                {
                    memset(dns, 0, sizeof(dns));
                    while (len > 0) {
                        if(nDnsNum < 2)
                            strcat(dns, inet_ntoa(*(struct in_addr *)ap));
                        //printf("%s", inet_ntoa(*(struct in_addr *)ap));
                        len -= sizeof(struct in_addr);
                        ap += sizeof(struct in_addr);
                        nDnsNum++;
                        if (len && nDnsNum == 1) strcat(dns, ",");
                    }
                    nDnsNum = 0;
                    return dns;
                }

                while (len > 0) {
                    //printf("%s", inet_ntoa(*(struct in_addr *)ap));
                    len -= sizeof(struct in_addr);
                    ap += sizeof(struct in_addr);
                    //if (len) printf(", ");
                }

                break;

            default:
                break;
            }                
            op += *(op+1)+2;
        }
    }
    return NULL;
}

#endif

#endif
#endif

