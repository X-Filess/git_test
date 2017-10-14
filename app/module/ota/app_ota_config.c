/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_ota_config.c
* Author    :	B.Z.
* Project   :	OTA
* Type      :   Source Code
******************************************************************************
* Description  :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.12.02		   B.Z.			  Creation
*****************************************************************************/

#include "app_config.h"
#if (OTA_SUPPORT || OTA_MONITOR_SUPPORT)
#include "module/config/gxconfig.h"
#include "gxoem.h"

#define OTA_DSMCC
#define OTA_OEM_SECTION                             ("update")
#define OTA_OEM_FLAG                                ("flag")
#define OTA_OEM_STATUS                              ("update_status")
// data versions
#define OTA_DSMCC_PID                               ("dmx_pid")
#define OTA_DSMCC_FRE                               ("fe_frequency")
#define OTA_DSMCC_SYM                               ("fe_symbolrate")
#define OTA_DSMCC_POL                               ("fe_voltage")
#define OTA_DSMCC_OUI					            ("ota_OUI")
#define OTA_DSMCC_HWMODEL					        ("ota_HwModel")
#define OTA_DSMCC_HWVER					            ("ota_HwVer")
#define OTA_DSMCC_SWMODEL					        ("ota_SwModel")
#define OTA_DSMCC_SWVER					            ("ota_SwVer")
// menu
#define OTA_MENU_UPDATE_FLAG                        ("ota>flag")
#define OTA_MENU_DEFAULT_FLAG                       (0)
#define OTA_MENU_DSMCC_PID                          ("ota>dmx_pid")
#define OTA_MENU_DEFAULT_PID                        (0x3a)
#define OTA_MENU_DSMCC_FRE                          ("ota>fre")
#define OTA_MENU_DEFAULT_FRE                        (1150000)
#define OTA_MENU_DSMCC_SYM                          ("ota>sym")
#define OTA_MENU_DEFAULT_SYM                        (27500000)
#define OTA_MENU_DSMCC_POL                          ("ota>pol")
#define OTA_MENU_DEFAULT_POL                        (1)// 0: 18V; 1: 13V

#define OTA_MENU_DSMCC_OUI                          ("ota>dsmcc_oui")
#define OTA_MENU_DEFAULT_OUI                        (0x333333)
#define OTA_MENU_DSMCC_HWMODEL                      ("ota>dsmcc_hwmodel")
#define OTA_MENU_DEFAULT_HWMODEL                    (0x3333)
#define OTA_MENU_DSMCC_HWVER                        ("ota>dsmcc_hwver")
#define OTA_MENU_DEFAULT_HWVER                      (0x3333)
#define OTA_MENU_DSMCC_SWMODEL                      ("ota>dsmcc_swmodel")
#define OTA_MENU_DEFAULT_SWMODEL                    (0x3330)
#define OTA_MENU_DSMCC_SWVER                        ("ota>dsmcc_swver")
#define OTA_MENU_DEFAULT_SWVER                      (0x0000)
#define OTA_MENU_DSMCC_DETECT_SWVER                 ("ota>dsmcc_detect_swver")
#define OTA_MENU_DEFAULT_DETECT_SWVER               (0x0000)

#ifdef ECOS_OS
int app_ota_dsmcc_get_oui(unsigned int *data)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == data)
    {
        *data = 0xffffffff;
        return -1;
    }

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_OUI);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_OUI, &value, OTA_MENU_DEFAULT_OUI);
        *data = (unsigned int)value;
        return 0;
    }
    sscanf((str+2), "%x", data);
    return ret;
}

int app_ota_dsmcc_get_hw_model(unsigned int *data)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == data)
    {
        *data = 0xffff;
        return -1;
    }

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_HWMODEL);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_HWMODEL, &value, OTA_MENU_DEFAULT_HWMODEL);
        *data = (unsigned short)value;
        return 0;
    }

    sscanf((str+2), "%x", data);
    return ret;
}

int app_ota_dsmcc_get_hw_version(unsigned int *data)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == data)
    {
        *data = 0xffff;
        return -1;
    }

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_HWVER);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_HWVER, &value, OTA_MENU_DEFAULT_HWVER);
        *data = (unsigned short)value;
        return 0;
    }

    sscanf((str+2), "%x", data);
    return ret;
}

int app_ota_dsmcc_get_sw_model(unsigned int *data)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == data)
    {
        *data = 0xffff;
        return -1;
    }

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_SWMODEL);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_SWMODEL, &value, OTA_MENU_DEFAULT_SWMODEL);
        *data = (unsigned short)value;
        return 0;
    }

    sscanf((str+2), "%x", data);
    return ret;
}

int app_ota_dsmcc_get_sw_version(unsigned int *data)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == data)
    {
        *data = 0xffff;
        return -1;
    }

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_SWVER);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_SWVER, &value, OTA_MENU_DEFAULT_SWVER);
        *data = (unsigned short)value;
        return 0;
    }

    sscanf((str+2), "%x", data);
    return ret;
}

int app_ota_dsmcc_set_sw_version(unsigned int data)
{
    int ret = 0;
    char buf[9] = {0};
    char *str = NULL;

    if(data > 0xffff)
    {
        return -1;
    }
    memset(buf, 0, 9);
    sprintf(buf, "0x%04x",  data);

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_SWVER);
    if(NULL == str)
    {
        // no oem file, use config.ini file
    }
    else
    {
        if(strncmp(str, buf, 6))
        {
            GxOem_SetValue(OTA_OEM_SECTION, OTA_DSMCC_SWVER, buf);
            GxOem_Save();
        }
    }
    GxBus_ConfigSetInt(OTA_MENU_DSMCC_SWVER, data);
    return ret;
}

// common
int app_ota_get_data_pid(unsigned int *pid)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == pid)
    {
        *pid = 0x1fff;
        return -1;
    }

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_PID);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_PID, &value, OTA_MENU_DEFAULT_PID);
        *pid = (unsigned short)value;
        return 0;
    }
    if((*str == '0')
            && ((*(str+1) == 'x') || (*(str+1) == 'X')))
    {
        sscanf((str+2), "%x", (unsigned int*)pid);
    }
    else
    {
        sscanf(str, "%d", (unsigned int*)pid);
    }
    return ret;
}

int app_ota_set_data_pid(unsigned short pid)
{
    char buf[10] = {0};
    unsigned int record_pid = 0;
    if(0x1fff == pid)
    {
        printf("\nOTA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    app_ota_get_data_pid(&record_pid);
    if(record_pid != pid)
    {
        sprintf(buf, "0x%x", pid);
        GxOem_SetValue(OTA_OEM_SECTION, OTA_DSMCC_PID, buf);
        GxOem_Save();
        GxBus_ConfigSetInt(OTA_MENU_DSMCC_PID, pid);
    }
    return 0;
}

// fre
int app_ota_get_fre(unsigned int *fre)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == fre)
    {
        return -1;
    }

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_FRE);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_FRE, &value, OTA_MENU_DEFAULT_FRE);
        *fre = (unsigned short)value;
        return 0;
    }
    if((*str == '0')
            && ((*(str+1) == 'x') || (*(str+1) == 'X')))
    {
        sscanf((str+2), "%x", (unsigned int*)fre);
    }
    else
    {
        sscanf(str, "%d", (unsigned int*)fre);
    }
    return ret;
}

int app_ota_set_fre(unsigned int fre)
{
    char buf[10] = {0};
    unsigned int record_fre= 0;

    app_ota_get_fre(&record_fre);
    if(record_fre != fre)
    {
        sprintf(buf, "%d", fre);
        GxOem_SetValue(OTA_OEM_SECTION, OTA_DSMCC_FRE, buf);
        GxOem_Save();
        GxBus_ConfigSetInt(OTA_MENU_DSMCC_FRE, fre);
    }
    return 0;
}

// sym
int app_ota_get_sym(unsigned int *sym)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == sym)
    {
        return -1;
    }

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_SYM);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_SYM, &value, OTA_MENU_DEFAULT_SYM);
        *sym = (unsigned short)value;
        return 0;
    }
    if((*str == '0')
            && ((*(str+1) == 'x') || (*(str+1) == 'X')))
    {
        sscanf((str+2), "%x", (unsigned int*)sym);
    }
    else
    {
        sscanf(str, "%d", (unsigned int*)sym);
    }
    return ret;
}

int app_ota_set_sym(unsigned int sym)
{
    char buf[10] = {0};
    unsigned int record_sym= 0;

    app_ota_get_sym(&record_sym);
    if(record_sym != sym)
    {
        sprintf(buf, "%d", sym);
        GxOem_SetValue(OTA_OEM_SECTION, OTA_DSMCC_SYM, buf);
        GxOem_Save();
        GxBus_ConfigSetInt(OTA_MENU_DSMCC_SYM, sym);
    }
    return 0;
}

// pol
int app_ota_get_pol(unsigned int *pol)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == pol)
    {
        return -1;
    }

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_POL);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_POL, &value, OTA_MENU_DEFAULT_POL);
        *pol = (unsigned short)value;
        return 0;
    }
    if(0 == (strncmp(str, "13V", 3)))
    {
        *pol = 1;
    }
    else if(0 == (strncmp(str, "18V", 3)))
    {
        *pol = 0;
    }
    else
    {
        *pol = 2;
    }
    return ret;
}

int app_ota_set_pol(unsigned int pol)
{
    char buf[10] = {0};
    unsigned int record_pol= 0;

    app_ota_get_pol(&record_pol);
    if(record_pol != pol)
    {
        if(pol == 0)
        {
            sprintf(buf, "%s", "18V");
        }
        else if(pol == 1)
        {
            sprintf(buf, "%s", "13V");
        }
        else
        {
            sprintf(buf, "%s", "OFF");
        }
        GxOem_SetValue(OTA_OEM_SECTION, OTA_DSMCC_POL, buf);
        GxOem_Save();
        GxBus_ConfigSetInt(OTA_MENU_DSMCC_POL, pol);
    }
    return 0;
}


int app_loader_ota_get_update_flag(unsigned char *update_flag)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == update_flag)
    {
        return -1;
    }

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_OEM_FLAG);
    if (NULL == str)
    {
        return -1;
    }

    if(0 == strncmp(str, "yes", 3))
    {
        *update_flag = 1;
    }
    else
    {
        *update_flag = 0;
    }
    return ret;
}

int app_menu_ota_get_update_flag(unsigned char *update_flag)
{
    int ret = 0;
    int value = 0;
    GxBus_ConfigGetInt(OTA_MENU_UPDATE_FLAG, &value, OTA_MENU_DEFAULT_FLAG);
    *update_flag = (unsigned char)value;
    return ret;
}

int app_ota_get_update_flag(unsigned char *update_flag)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == update_flag)
    {
        return -1;
    }

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_OEM_FLAG);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_UPDATE_FLAG, &value, OTA_MENU_DEFAULT_FLAG);
        *update_flag = (unsigned char)value;
        return 0;
    }

    if(0 == strncmp(str, "yes", 3))
    {
        *update_flag = 1;
    }
    else
    {
        *update_flag = 0;
    }
    return ret;
}

int app_loader_ota_set_update_flag(unsigned char update_flag)
{
    int ret = 0;
    char *str = NULL;

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_OEM_FLAG);
    if (NULL == str)
    {
        // no oem file
        return -1;
    }
    else
    {
        if(update_flag)
        {
            GxOem_SetValue(OTA_OEM_SECTION, OTA_OEM_FLAG, "yes");
        }
        else
        {
            GxOem_SetValue(OTA_OEM_SECTION, OTA_OEM_FLAG, "no");
        }
        GxOem_SetValue(OTA_OEM_SECTION, OTA_OEM_STATUS, "none");
        GxOem_Save();
    }
    return ret;
}

int app_menu_ota_set_update_flag(unsigned char update_flag)
{
    int ret = 0;
    GxBus_ConfigSetInt(OTA_MENU_UPDATE_FLAG, update_flag);
    return ret;
}

int app_ota_set_update_flag(unsigned char update_flag)
{
    int ret = 0;
    char *str = NULL;

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_OEM_FLAG);
    if (NULL == str)
    {
        // no oem file
    }
    else
    {
        if(update_flag)
        {
            GxOem_SetValue(OTA_OEM_SECTION, OTA_OEM_FLAG, "yes");
        }
        else
        {
            GxOem_SetValue(OTA_OEM_SECTION, OTA_OEM_FLAG, "no");
        }
        GxOem_SetValue(OTA_OEM_SECTION, OTA_OEM_STATUS, "none");
        GxOem_Save();
    }
    GxBus_ConfigSetInt(OTA_MENU_UPDATE_FLAG, update_flag);

    return ret;
}

// 20151207

int app_ota_get_notice_flag(unsigned char* notice_flag, int new_version)
{
#if (OTA_MONITOR_SUPPORT  > 0)
    int ret = 0;
    int menu_detect_version = 0;

    if (NULL == notice_flag)
    {
        return -1;
    }

    if(-1 != new_version)
    {
        menu_detect_version = new_version;
    }
    else
    {
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_DETECT_SWVER, &menu_detect_version, OTA_MENU_DEFAULT_DETECT_SWVER);
    }
#if (UPDATE_SUPPORT_FLAG > 0)
    char *str = NULL;
    int loader_ota_version = 0;
    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_SWVER);
    if (NULL == str)
    {
        return -1;
    }
    sscanf((str+2), "%x", &loader_ota_version);
    if(loader_ota_version >= menu_detect_version)
    {
        *notice_flag = 0;// old version
    }
    else
    {
        *notice_flag = 1;// new version, need notice
    }
    return ret;
#elif (OTA_SUPPORT > 0)
    int menu_ota_version = 0;
    GxBus_ConfigGetInt(OTA_MENU_DSMCC_SWVER, &menu_ota_version, OTA_MENU_DEFAULT_SWVER);
    if(menu_ota_version >= menu_detect_version)
    {
        *notice_flag = 0;// old version
    }
    else
    {
        *notice_flag = 1;// new version
    }
#endif
#else
    *notice_flag = 0;// do not notice new version
#endif
    return ret;
}

int app_ota_set_new_detect_version(unsigned short detected_version)
{
    int value = 0;
    GxBus_ConfigGetInt(OTA_MENU_DSMCC_DETECT_SWVER, &value, OTA_MENU_DEFAULT_DETECT_SWVER);
    if(value < detected_version)
    {
        GxBus_ConfigSetInt(OTA_MENU_DSMCC_DETECT_SWVER, detected_version);
    }
    return 0;
}

static int thiz_ota_menual_flag = 0;
int app_ota_set_menual_flag(void)
{
    int ret = 0;
    thiz_ota_menual_flag = 1;
    return ret;
}

int app_ota_get_manual_flag(void)
{
    return thiz_ota_menual_flag;
}

int app_ota_sync_init(void)
{
    int ret = 0;
 #if (UPDATE_SUPPORT_FLAG && OTA_MONITOR_SUPPORT)
    char *str = NULL;
    int menu_detect_version = 0;
    int loader_ota_version = 0;
    unsigned char flag = 0;

    str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_SWVER);
    if (NULL == str)
    {
        return -1;
    }
    sscanf((str+2), "%x", &loader_ota_version);

    GxBus_ConfigGetInt(OTA_MENU_DSMCC_DETECT_SWVER, &menu_detect_version, OTA_MENU_DEFAULT_DETECT_SWVER);
    //printf("\nMENU DETECT VERSION = %d, loader version = %d\n", menu_detect_version, loader_ota_version);
    if(menu_detect_version < loader_ota_version)
    {// loader ota had updated to the newest version. need sync the menu ota params.
        GxBus_ConfigSetInt(OTA_MENU_DSMCC_DETECT_SWVER, loader_ota_version);
#if (OTA_SUPPORT > 0)
        GxBus_ConfigSetInt(OTA_MENU_DSMCC_SWVER, loader_ota_version);
        GxBus_ConfigSetInt(OTA_MENU_UPDATE_FLAG, 0);
#endif
    }
    else
    {
        app_ota_get_update_flag(&flag);
        //printf("\nlaoder flag = %d\n", flag);
        if(flag == 0)
        {
            GxBus_ConfigSetInt(OTA_MENU_DSMCC_DETECT_SWVER, loader_ota_version);
        }
        ;// update by manual
    }
#endif
    return ret;
}
#else
int app_ota_dsmcc_get_oui(unsigned int *data)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == data)
    {
        *data = 0xffffffff;
        return -1;
    }

    //str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_OUI);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_OUI, &value, OTA_MENU_DEFAULT_OUI);
        *data = (unsigned int)value;
        return 0;
    }
    sscanf((str+2), "%x", data);
    return ret;
}

int app_ota_dsmcc_get_hw_model(unsigned int *data)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == data)
    {
        *data = 0xffff;
        return -1;
    }

    //str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_HWMODEL);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_HWMODEL, &value, OTA_MENU_DEFAULT_HWMODEL);
        *data = (unsigned short)value;
        return 0;
    }

    sscanf((str+2), "%x", data);
    return ret;
}

int app_ota_dsmcc_get_hw_version(unsigned int *data)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == data)
    {
        *data = 0xffff;
        return -1;
    }

    //str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_HWVER);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_HWVER, &value, OTA_MENU_DEFAULT_HWVER);
        *data = (unsigned short)value;
        return 0;
    }

    sscanf((str+2), "%x", data);
    return ret;
}

int app_ota_dsmcc_get_sw_model(unsigned int *data)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == data)
    {
        *data = 0xffff;
        return -1;
    }

    //str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_SWMODEL);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_SWMODEL, &value, OTA_MENU_DEFAULT_SWMODEL);
        *data = (unsigned short)value;
        return 0;
    }

    sscanf((str+2), "%x", data);
    return ret;
}

int app_ota_dsmcc_get_sw_version(unsigned int *data)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == data)
    {
        *data = 0xffff;
        return -1;
    }

    //str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_SWVER);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_SWVER, &value, OTA_MENU_DEFAULT_SWVER);
        *data = (unsigned short)value;
        return 0;
    }

    sscanf((str+2), "%x", data);
    return ret;
}

int app_ota_dsmcc_set_sw_version(unsigned int data)
{
    int ret = 0;
    char buf[9] = {0};
    char *str = NULL;

    if(data > 0xffff)
    {
        return -1;
    }
    memset(buf, 0, 9);
    sprintf(buf, "0x%04x",  data);

    //str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_SWVER);
    if(NULL == str)
    {
        // no oem file, use config.ini file
    }
    else
    {
        if(strncmp(str, buf, 6))
        {
            GxOem_SetValue(OTA_OEM_SECTION, OTA_DSMCC_SWVER, buf);
            GxOem_Save();
        }
    }
    GxBus_ConfigSetInt(OTA_MENU_DSMCC_SWVER, data);
    return ret;
}

// common
int app_ota_get_data_pid(unsigned short *pid)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == pid)
    {
        *pid = 0x1fff;
        return -1;
    }

    //str = GxOem_GetValue(OTA_OEM_SECTION, OTA_DSMCC_PID);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DSMCC_PID, &value, OTA_MENU_DEFAULT_PID);
        *pid = (unsigned short)value;
        return 0;
    }

    sscanf((str+2), "%x", (unsigned int*)pid);
    return ret;
}

int app_ota_get_update_flag(unsigned char *update_flag)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == update_flag)
    {
        return -1;
    }

    //str = GxOem_GetValue(OTA_OEM_SECTION, OTA_OEM_FLAG);
    if (NULL == str)
    {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_UPDATE_FLAG, &value, OTA_MENU_DEFAULT_FLAG);
        *update_flag = (unsigned char)value;
        return 0;
    }

    if(strncmp(str, "yes", 3))
    {
        *update_flag = 1;
    }
    else
    {
        *update_flag = 0;
    }

    return ret;
}

int app_ota_set_update_flag(unsigned char update_flag)
{
    int ret = 0;
    char *str = NULL;

    //str = GxOem_GetValue(OTA_OEM_SECTION, OTA_OEM_FLAG);
    if (NULL == str)
    {
        // no oem file
    }
    else
    {
        if(update_flag)
        {
            GxOem_SetValue(OTA_OEM_SECTION, OTA_OEM_FLAG, "yes");
        }
        else
        {
            GxOem_SetValue(OTA_OEM_SECTION, OTA_OEM_FLAG, "no");
        }
        GxOem_SetValue(OTA_OEM_SECTION, OTA_OEM_STATUS, "none");
        GxOem_Save();
    }
    GxBus_ConfigSetInt(OTA_MENU_UPDATE_FLAG, update_flag);

    return ret;
}
#endif
#endif
