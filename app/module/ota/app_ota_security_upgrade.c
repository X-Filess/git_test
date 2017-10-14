#include "app_msg.h"
#include "app_send_msg.h"
#include "app_ota_protocol.h"
#include "app_ota_upgrade.h"
#include "gxoem.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "module/app_nim.h"
#include "module/app_ioctl.h"

#define BACKUP_PARTITION_NAME ("BACKUP")
#define BACKUP_PARTITION_OEM_SIZE (64*1024)
#define GX_PARTITION_V_OEM ("V_OEM")

int app_ota_backup_partition(struct partition_info *partition)
{
    GxPartitionTable table = {0};
    struct partition_info *backup_partition = NULL;
    unsigned char *buf = NULL;

    if (GxOem_PartitionTableGet(&table) != 0) {
        printf("GxOem_PartitionTableGet Error!\n");
        return -1;
    }

    if (table.count == 0) {
        GxOtaPrintf("GxOem_PartitionTableGet Error!\n");
        return -1;
    }

    backup_partition = GxCore_PartitionGetByName(&table, BACKUP_PARTITION_NAME);
    if (NULL == backup_partition) {
        printf("get partition info failed\n");
        return -1;
    }

    if (partition->total_size > backup_partition->total_size) {
        printf("write error, size will exceed the partition %s %d\n", __FILE__, __LINE__);
    }

    buf = GxCore_Mallocz(partition->total_size);
    if (NULL == buf) {
        return -1;
    }

    GxCore_PartitionRead(partition, buf, partition->total_size, 0);
    GxCore_PartitionWrite(backup_partition, buf, partition->total_size, 0);
    return 0;
}

int app_ota_backup_oem(void)
{
    GxPartitionTable table = {0};
    struct partition_info *backup_partition = NULL;
    struct partition_info *voem_partition = NULL;
    unsigned int oem_in_backup_offset = 0;
    unsigned char *buf = NULL;

    if (GxOem_PartitionTableGet(&table) != 0) {
        printf("GxOem_PartitionTableGet Error!\n");
        return -1;
    }

    if (table.count == 0) {
        GxOtaPrintf("GxOem_PartitionTableGet Error!\n");
        return -1;
    }

    backup_partition = GxCore_PartitionGetByName(&table, BACKUP_PARTITION_NAME);
    if (NULL == backup_partition) {
        printf("get partition info failed\n");
        return -1;
    }

    voem_partition = GxCore_PartitionGetByName(&table, GX_PARTITION_V_OEM);
    if (NULL == voem_partition) {
        printf("get partition info failed\n");
        return -1;
    }

    buf = GxCore_Mallocz(BACKUP_PARTITION_OEM_SIZE);
    if (NULL == buf) {
        return -1;
    }

    GxCore_PartitionRead(voem_partition, buf, BACKUP_PARTITION_OEM_SIZE, 0);
    oem_in_backup_offset = backup_partition->total_size - BACKUP_PARTITION_OEM_SIZE;
    GxCore_PartitionWrite(backup_partition, (void *)buf, BACKUP_PARTITION_OEM_SIZE, oem_in_backup_offset);
    return 0;
}


/*以下为OEM中的信息，需要在ini文件中添加选项
用于OTA loader 等跟升级相关的模块交互信息*/
#define OTA_VARIABLE_FILE_SECTION			    ("variable_oem")
#define OTA_VARIABLE_FILE_VERSION               ("version")

#define OTA_SYSTEM_SECTION			            ("system")
#define OTA_SYSTEM_UPDATE_TYPE                  ("update_type")
#define OTA_SYSTEM_UPDATE_URL                   ("update_default_url")

#define OTA_SOFTWARE_SECTION                    ("software")
#define OTA_SOFTWARE_BOOTLOADER_VERSION         ("bootloader_version")
#define OTA_SOFTWARE_LOGO_VERSION               ("logo_version")
#define OTA_SOFTWARE_APPLICATION_VERSION        ("application_version")
#define OTA_SOFTWARE_ROOTFS_VERSION             ("rootfs_version")
#define OTA_SOFTWARE_DATA_VERSION               ("datafs_version")

#define OTA_UPDATE_SECTION                      ("update")
#define OTA_UPDATE_FLAG                         ("flag")
#define OTA_DMX_ID                              ("dmx_id")
#define OTA_DMX_TIMEOUT                         ("dmx_timeout")
#define OTA_DMX_SOURCE                          ("dmx_source")
#define OTA_DMX_PID                             ("dmx_pid")

#define OTA_FE_INIT_PARAM                       ("fe_init_param")
#define OTA_FE_MODULATION                       ("fe_modulation")
#define OTA_FE_REPEAT_TIMES                     ("fe_repeat_times")
#define OTA_REPEAT_TIMES                        ("ota_repeat_times")
#define OTA_FE_SYMBOLRATE                       ("fe_symbolrate")
#define OTA_FE_FREQUENCY                        ("fe_frequency")
// dvbs
#define OTA_FE_VOLTAGE                          ("fe_voltage")
#define OTA_FE_TONE                             ("fe_tone")
#define OTA_FE_DISEQC10                         ("fe_diseqc10")
// dvbt
#define OTA_FE_WOKEMODE                         ("fe_wokemode")
#define OTA_FE_BANDWIDTH                        ("fe_bandwidth")
// dvbc
#define OTA_FE_QAM                              ("fe_qam")

// different protocol used
//#define OTA_DMX_TABLEID                       ("dmx_tableid")

// only for menu ota
#define OTA_MENU_DMX_PID                         "ota>dmx_pid"
#define OTA_DEFAULT_DMX_ID                       58 // 0x3a
#define OTA_MENU_UPDATE_FLAG                    "ota>flag"
#define OTA_DEFAULT_UPDATE_FLAG                  0
#define OTA_MENU_TIMEOUT                        "ota>dmx_timeout"
#define OTA_DEFAULT_TIMEOUT                      10000
#define OTA_MENU_SOURCE                         "ota>dmx_source"
#define OTA_DEFAULT_SOURCE                       0// DEMUX_TS1
#define OTA_MENU_MODULATION                     "ota>modulation"
#define OTA_DEFAULT_MODULATION                   0// DVBS/S2
#define OTA_MENU_QUERY_LOCK_TIMES               "ota>query_lock_times"
#define OTA_DEFAULT_QUERY_LOCK_TIMES             12288// 0X3000
#define OTA_MENU_TRY_LOCK_TIMES                 "ota>try_lock_times"
#define OTA_DEFAULT_LOCK_TIMES                   3
#define OTA_MENU_FRE                            "ota>fre"
#define OTA_DEFAULT_FRE                          1150000
#define OTA_MENU_SYM                            "ota>sym"
#define OTA_DEFAULT_SYM                          27500000
#define OTA_MENU_VOLTAGE                        "ota>voltage"
#define OTA_DEFAULT_VOLTAGE                      1// 18V
#define OTA_MENU_22K                            "ota>22k"
#define OTA_DEFAULT_22K                          0// ON
#define OTA_MENU_DISEQC10                       "ota>diseqc10"
#define OTA_DEFAULT_DISEQC10                     4// port4
// FOR DVBC
#define OTA_MENU_QAM                            "ota>qam"
#define OTA_DEFAULT_QAM                          0
// FOR DVBT2
#define OTA_MENU_WORKMODE                       "ota>work_mode"
#define OTA_DEFAULT_WORKMODE                     0
#define OTA_MENU_BANDWIDTH                      "ota>bandwidth"
#define OTA_DEFAULT_BANDWIDTH                    0// BANDWIDTH_8_MHZ

static int _set_ts_source(unsigned char ts_source)
{
    int ret = 0;
    char *str = NULL;
    unsigned char temp = 0;

    if (2 < ts_source) {
        return -1;
    }

    str = GxOem_GetValue(OTA_UPDATE_SECTION, OTA_DMX_SOURCE);
    if (NULL == str) {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_SOURCE, &value, OTA_DEFAULT_SOURCE);
        if (ts_source != value) {
            GxBus_ConfigSetInt(OTA_MENU_SOURCE, ts_source);
        }
        return 0;
    }
    if (0 == strncmp("DEMUX_TS1", str, strlen(str))) {
        temp = 0;
    }
    else if (0 == strncmp("DEMUX_TS2", str, strlen(str))) {
        temp = 1;
    }
    else if (0 == strncmp("DEMUX_TS3", str, strlen(str))) {
        temp = 2;
    }

    if (temp != ts_source) {
        char *buf[3] = {"DEMUX_TS1", "DEMUX_TS2", "DEMUX_TS3"};
        GxOem_SetValue(OTA_UPDATE_SECTION, OTA_DMX_SOURCE, buf[ts_source]);
        GxOem_Save();
    }
    return ret;
}

static int _set_fre(unsigned int fre)
{
    int ret = 0;
    char *str = NULL;

    str = GxOem_GetValue(OTA_UPDATE_SECTION, OTA_FE_FREQUENCY);

    if (NULL == str) {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_FRE, &value, OTA_DEFAULT_FRE);
        if (fre != value) {
            GxBus_ConfigSetInt(OTA_MENU_FRE, fre);
        }
        return 0;
    }

    if (fre != atoi(str)) {
        char buf[15] = {0};
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%d",  fre);// ex. 1150000
        GxOem_SetValue(OTA_UPDATE_SECTION, OTA_FE_FREQUENCY, buf);
        GxOem_Save();
    }
    return ret;
}

static int _set_sym(unsigned int sym)
{
    int ret = 0;
    char *str = NULL;

    str = GxOem_GetValue(OTA_UPDATE_SECTION, OTA_FE_SYMBOLRATE);
    if (NULL == str) {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_SYM, &value, OTA_DEFAULT_SYM);
        if (sym != value) {
            GxBus_ConfigSetInt(OTA_MENU_SYM, sym);
        }
        return 0;
    }

    if (sym != atoi(str)) {
        char buf[15] = {0};
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%d",  sym);// ex. 27500000
        GxOem_SetValue(OTA_UPDATE_SECTION, OTA_FE_SYMBOLRATE, buf);
        GxOem_Save();
    }
    return ret;
}

static int _set_22k(unsigned char tone)
{
    int ret = 0;
    char *str = NULL;
    unsigned char temp = 0;

    if (1 < tone) {
        return -1;
    }

    str = GxOem_GetValue(OTA_UPDATE_SECTION, OTA_FE_TONE);
    if (NULL == str) {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_22K, &value, OTA_DEFAULT_22K);
        if (tone != value) {
            GxBus_ConfigSetInt(OTA_MENU_22K, tone);
        }
        return 0;
    }
    if (0 == strncmp("ON", str, 2)) {
        temp = 0;
    }
    else {
        temp = 1;
    }
    if (temp != tone) {
        char *buf[2] = {"ON", "OFF"};
        GxOem_SetValue(OTA_UPDATE_SECTION, OTA_FE_TONE, buf[tone]);
        GxOem_Save();
    }
    return ret;
}

static int _set_diseqc10(unsigned char diseqc10)
{
    int ret = 0;
    char *str = NULL;
    unsigned char temp = 0 ;

    if (4 < diseqc10) {
        return -1;
    }

    str = GxOem_GetValue(OTA_UPDATE_SECTION, OTA_FE_DISEQC10);
    if (NULL == str) {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_DISEQC10, &value, OTA_DEFAULT_DISEQC10);
        if (diseqc10 != value) {
            GxBus_ConfigSetInt(OTA_MENU_DISEQC10, diseqc10);
        }
        return 0;
    }
    if (strncmp(str, "OFF", 3) == 0) {
        temp = 0;
    }
    else if (strncmp(str, "PORT1", 5) == 0) {
        temp = 1;
    }
    else if (strncmp(str, "PORT2", 5) == 0) {
        temp = 2;
    }
    else if (strncmp(str, "PORT3", 5) == 0) {
        temp = 3;
    }
    else if (strncmp(str, "PORT4", 5) == 0) {
        temp = 4;
    }
    if (diseqc10 != temp) {
        char *buf[5] = {"OFF", "PORT1", "PORT2" , "PORT3", "PORT4"};
        GxOem_SetValue(OTA_UPDATE_SECTION, OTA_FE_DISEQC10, buf[diseqc10]);
        GxOem_Save();
    }
    return ret;
}

static int _set_voltage(unsigned char voltage)
{
    int ret = 0;
    char *str = NULL;
    unsigned char temp = 0;

    if (2 < voltage) {
        return -1;
    }

    str = GxOem_GetValue(OTA_UPDATE_SECTION, OTA_FE_VOLTAGE);
    if (NULL == str) {
        int value = 0;
        GxBus_ConfigGetInt(OTA_MENU_VOLTAGE, &value, OTA_DEFAULT_VOLTAGE);
        if (voltage != value) {
            GxBus_ConfigSetInt(OTA_MENU_VOLTAGE, voltage);
        }
        return 0;
    }
    if (0 == strncmp("18V", str, 3)) {
        temp = 1;
    }
    else if (0 == strncmp("13V", str, 3)) {
        temp = 0;
    }
    else {
        temp = 2;
    }
    if (temp != voltage) {
        char *buf[3] = {"13V", "18V", "OFF"};
        GxOem_SetValue(OTA_UPDATE_SECTION, OTA_FE_VOLTAGE, buf[voltage]);
        GxOem_Save();
    }
    return ret;
}

static int _set_frontend_init_param(char *init_param)
{
    int ret = 0;
    char *str = NULL;

    if (NULL == init_param) {
        return -1;
    }

    str = GxOem_GetValue(OTA_UPDATE_SECTION, OTA_FE_INIT_PARAM);
    if (NULL == str) {
        return -1;
    }

    if (strlen(str) == 0) {
        ret = -1;
    }
    else {
        if (strncmp(init_param, str, strlen(str))) {
            GxOem_SetValue(OTA_UPDATE_SECTION, OTA_FE_INIT_PARAM, init_param);
            GxOem_Save();
        }
    }

    return ret;
}

void app_ota_backup_fe(void)
{
    // pid, ts_source
    AppFrontend_Param params = {0};
    AppFrontend_Config cfg = {0};

    if (GXCORE_ERROR == app_ioctl(0, FRONTEND_PARAM_GET, &params)) {
        printf("\nOTA MONITOR, %s, %d\n", __FUNCTION__, __LINE__);
        return;
    }

    if (params.type == FRONTEND_DVB_S) {
        _set_fre(params.fre * 1000);
        _set_sym(params.symb * 1000);
        _set_voltage(params.polar);
        _set_22k(params.sat22k);
        _set_diseqc10(params.diseqc10);
    }

    if (GXCORE_ERROR == app_ioctl(0, FRONTEND_CONFIG_GET, &cfg)) {
        printf("\nOTA MONITOR, %s, %d\n", __FUNCTION__, __LINE__);
        return;
    }
    _set_ts_source(cfg.ts_src);
}
