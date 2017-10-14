#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "module/config/gxconfig.h"
#include "app_utility.h"
#include "app_config.h"
#include <fcntl.h>

typedef enum {
    ITEM_SYSINFO_HW_INFO = 0,
    ITEM_SYSINFO_HW_VERSION,
    ITEM_SYSINFO_LOADER_VERSION,
    ITEM_SYSINFO_SW_VERSION,
    ITEM_SYSINFO_VER_DATE,
    ITEM_SYSINFO_SERIAL_NUM,
    ITEM_SYSINFO_STBID,
    ITEM_SYSINFO_MODULE_TYPE,
    ITEM_SYSINFO_MANUFACTURE,
    ITEM_SYSINFO_TOTAL
} SystemInfomationOptSel;

#define SYSTEM_INFO_TOTAL_ITEM		ITEM_SYSINFO_TOTAL - 2

#if NSCAS_SUPPORT
#if NSCAS_SMC_VERSION
#define LD_VER "1.0.0.0"
#define HW_VER "1.1.5.0"
#define SW_VER "1.1.5.0"
#else
#define LD_VER "1.0.0.0"
#define HW_VER "1.9.7.0"
#define SW_VER "1.9.7.0"
#endif
#endif

#define VERSION_LEN 64
#define DATE_TIME_LEN	24
#define SVN_VER_LEN	16

static char gStbID[16] ;
static SystemSettingOpt s_sysinfo_opt;
static SystemSettingItem s_sysinfo_item[ITEM_SYSINFO_TOTAL];
static SystemInfomationOptSel s_SystemInfoSel = ITEM_SYSINFO_MODULE_TYPE;

static char *s_system_item_title[] = {
    "text_system_info_litem1",
    "text_system_info_litem2",
    "text_system_info_litem3",
    "text_system_info_litem4",
    "text_system_info_litem5",
    "text_system_info_litem6",
    "text_system_info_litem7",
    "text_system_info_litem8",
    "text_system_info_litem9",
    "text_system_info_litem10",
    "text_system_info_litem11",
};

static char *s_item_content_btn_info[] = {
    "text_system_info_mitem1",
    "text_system_info_mitem2",
    "text_system_info_mitem3",
    "text_system_info_mitem4",
    "text_system_info_mitem5",
    "text_system_info_mitem6",
    "text_system_info_mitem7",
    "text_system_info_mitem8",
    "text_system_info_mitem9",
    "text_system_info_mitem10",
    "text_system_info_mitem11",
};

extern void frontend_mod_init_gx113x(unsigned int num, const char *profile);
extern status_t app_oem_get_list_value(app_oem_type type, char *buf, size_t size);
/************************************************************************/
/* strnset\u5c06\u4e00\u4e2a\u5b57\u7b26\u4e32\u7684\u6240\u6307\u5b9a\u4e2a\u6570\u7684\u6240\u6709\u5b57\u7b26\u90fd\u8bbe\u4e3a\u6307\u5b9a\u5b57\u7b26
char *strnset(char *str,char ch,unsigned n)*/
/************************************************************************/
#ifdef ECOS_OS
//a030a190
int _otp_read_chip_name(char *SnData, int rd_num)
{
    #ifdef GX3211
#define CHIP_SN_START_ADDR 0x348
#define CHIP_NAME_START_ADDR 0x350
    #endif

    #ifdef GX6605S
#define CHIP_SN_START_ADDR 0x148
#define CHIP_NAME_START_ADDR 0x150
    #endif

#define DEV_NAME "/dev/gxotp0"
#define OTP_IOCTL_ADDR	   123
#define OT_RDWR       ((1<<0)|(1<<1))

    unsigned int start_addr = CHIP_SN_START_ADDR;
    int fd = 0;
    int i = 0;
    char data[20] = {0};

    if ((SnData == NULL) && (rd_num < 8 || rd_num > 15)) {
        return -1;
    }

    fd = open(DEV_NAME, OT_RDWR);
    ioctl(fd, OTP_IOCTL_ADDR, &start_addr);
    read(fd, data, rd_num);
    memcpy(SnData, data, 8);

    printf("\n-sn -----start---- 0x%x\n", start_addr);
    for (i = 0; i < rd_num; i++) {
        printf("%x ", data[i]);
        if ((i + 1) % 10 == 0) {
            printf("\n");
        }
    }
    printf("\n-[%s] sn -----end---- \n", __FUNCTION__);
    start_addr = CHIP_NAME_START_ADDR;
    ioctl(fd, OTP_IOCTL_ADDR, &start_addr);
    read(fd, data, 16);

    printf("\n--name -----start**** 0x%x\n", start_addr);
    for (i = 0; i < 16; i++) {
        printf("%c", data[i]);
        if ((i + 1) % 10 == 0) {
            printf("\n");
        }
    }
    printf("\n-[%s]--name-----end---** \n", __FUNCTION__);

    start_addr = 0x101;
    ioctl(fd, OTP_IOCTL_ADDR, &start_addr);
    read(fd, data, 1);

    printf("\n -config -----start**** 0x%x\n", start_addr);

    printf("conifg=%x\n", data[0]);

    start_addr = 0x1ff;
    ioctl(fd, OTP_IOCTL_ADDR, &start_addr);
    read(fd, data, 1);

    printf("\n -section lock -----start**** 0x%x\n", start_addr);

    printf("section=%x\n", data[0]);

    #if 0
    start_addr = 0x180;
    memset(data, 0, 12);
    memcpy(data, "abcdef1234" , 12);
    GxOtp_Write(start_addr, data, 8);

    GxOtp_Read(start_addr, data, 8);
    printf("buf=%s\n", data);
    ioctl(fd, OTP_IOCTL_ADDR, &start_addr);
    read(fd, data, 16);

    printf("\n[chip6605s] --***** -----start**** 0x%x\n", start_addr);
    for (i = 0; i < 16; i++) {
        printf("%c", data[i]);
        if ((i + 1) % 10 == 0) {
            printf("\n");
        }
    }

    #endif
    return 0;
}

static char *strnset(char *str1, char ch, unsigned n)
{
    //	ASSERT(str1!=NULL);
    int index = 0;
    char *tmp = (char *) str1;

    while ((*tmp != '\0') || (index < n + 12)) { //while(n--&&*str1!='\0')
        index++;
        if (index == n) {
            *tmp = ch;
        }
        tmp++;
    }
    return str1;
}
#endif

/*************************************************************
		   <list value="0" display="|4:2:0x80:41:0:0xC6:@0:1:7:0:0:1"/>
27		   <list value="1" display="|4:2:0x80:41:0:0xC0:@0:1:7:0:0:1"/>
28		   <list value="2" display="|4:2:0x80:41:0:0xC6:@1:0:7:0:1:1"/>
29		   <list value="3" display="|4:2:0x80:41:0:0xC0:@1:1:7:0:0:1"/>
30		   <list value="4" display="|4:2:0x80:54:0:0xc2:@1:1:7:1:0:1"/>
31		   <list value="5" display="|4:2:0x80:55:0:0x18:@0:1:7:0:0:1"/>
32		   <list value="6" display="|4:2:0x80:41:0:0xC6:@1:1:7:0:0:1"/>
33		   <list value="7" display="|4:2:0x80:41:0:0xC6:@0:0:7:0:1:1"/>
34		   <list value="8" display="|8:0:0xe4:55:0:0x18:@1:1:2:0:0:1"/>
35		   <list value="9" display="|8:0:0xe4:41:0:0xC0:@0:1:2:0:0:1"/>
36		   <list value="10" display="|8:0:0xe4:55:0:0x18:@0:1:2:0:0:1"/>

**************************************************************/

void app_mod_tuner_config_init(void)
{
    #ifdef ECOS_OS
    extern void frontend_mod_init_gx3211(unsigned int num, const char *profile);
    extern void frontend_mod_init_gx1001(unsigned int num, const char *profile);
    extern void frontend_mod_init_gx1211(unsigned int num, const char *profile);
    extern void frontend_mod_init_gx1801(unsigned int num, const char *profile);

    int ret = 0;
    static char tuner_cfg[VERSION_LEN];
    char letter = '&';

    memset(tuner_cfg, 0, VERSION_LEN);
    //ret = app_oem_get_list_value(OEM_TUNER_CONFIG, tuner_cfg, VERSION_LEN);
    ret = app_oem_get(OEM_TUNER_CONFIG, tuner_cfg, VERSION_LEN);
    #if ((DEMOD_DVB_S > 0) && (DEMOD_DTMB > 0))
    frontend_mod_init_gx3211(2, "|8:0:0xe4:70:0:0xf4:&0:1:2:0:0:1|9:1:0x60:73:0:0xf4:&0:1:2:0:0:1");

    //#elif (DEMOD_DVB_T > 0 && DEMOD_DVB_S > 0 && DEMOD_DVB_C > 0)
    #elif (DEMOD_DVB_COMBO > 0)
    strcat(tuner_cfg, "|0:0:0xe4:90:");
    #if (DEMOD_DVB_COMBO_S > 0)
    strcat(tuner_cfg, "70:0:0xF4:");
    #endif

    #if (DEMOD_DVB_COMBO_T > 0)
    strcat(tuner_cfg, "71:0:0xF4:");
    #endif

    #if (DEMOD_DVB_COMBO_C > 0)
    strcat(tuner_cfg, "72:0:0xF4:");
    #endif
    strcat(tuner_cfg, "#:&0:1:2:0:0");

    printf("______------_____=%s,[line=%d]\n", tuner_cfg, __LINE__);
    frontend_mod_init_gx1801(1, tuner_cfg);

    #elif (DEMOD_DVB_S > 0)

    #ifdef GX3211
    #if DOUBLE_S2_SUPPORT
    //frontend_mod_init_gx3211(2,"|7:0:0xe4:70:1:0xf4:&1:1:2:0:0|8:2:0x60:71:1:0xf4:&1:1:2:0:0");
    frontend_mod_init_gx3211(2, "|8:0:0xe4:41:0:0xc0:&0:1:2:0:0:1|4:1:0xd0:41:1:0xc0:&1:1:8:0:0:0"); //For DVBS Dobble Tuner
    #else
    strnset(tuner_cfg, letter, 21);
    if ((GXCORE_SUCCESS != ret) || (tuner_cfg == NULL)) {
        strncpy(tuner_cfg, "|8:0:0xe4:55:0:0x18:&1:1:2:0:0:1", VERSION_LEN);
        printf("______---3211---_____=%s,[line=%d]\n", tuner_cfg, __LINE__);
    }
    frontend_mod_init_gx3211(1, tuner_cfg);
    //frontend_mod_init_gx3211(1,"|8:0:0xe4:55:0:0x18:&1:1:2:0:0:1");
    #endif
    #endif

    #ifdef GX6605S
    #if DOUBLE_S2_SUPPORT
    frontend_mod_init_gx3211(2, "|8:0:0xe4:41:0:0xc0:&0:1:2:0:0:1|4:1:0xd0:41:1:0xc0:&1:1:8:0:0:0"); //For DVBS Dobble Tuner
    #else
    strnset(tuner_cfg, letter, 21);
    if ((GXCORE_SUCCESS != ret) || (tuner_cfg == NULL)) {
        strncpy(tuner_cfg, "|8:0:0xe4:55:0:0x18:&1:1:2:0:0:1", VERSION_LEN);
        printf("______---gx6605s---_____=%s,[line=%d]\n", tuner_cfg, __LINE__);
    }

    frontend_mod_init_gx3211(1, tuner_cfg);
    //frontend_mod_init_gx3211(1,"|8:0:0xe4:55:0:0x18:&1:1:2:0:0:1");
    //frontend_mod_init_gx3211(1,"|8:0:0xe4:41:0:0xC6:&1:1:2:0:0:1");
    #endif
    #endif

    #ifdef GX3201
    extern void frontend_mod_init_gx113x(unsigned int num, const char *profile);
    // char letter = '&';

    //if (GXCORE_SUCCESS != app_oem_get_list_value(OEM_TUNER_CONFIG, tuner_cfg, VERSION_LEN))
    if (GXCORE_SUCCESS != ret) {
        strncpy(tuner_cfg, MOD_TUNER, VERSION_LEN);
        printf("______---222---_____=%s,[line=%d]\n", tuner_cfg, __LINE__);
    }

    printf("string before strnset: %s\n", tuner_cfg);
    strnset(tuner_cfg, letter, 21);
    printf("string after strnset: %s\n", tuner_cfg);
    frontend_mod_init_gx113x(1, tuner_cfg);
    #endif

    #elif (DEMOD_DVB_C > 0)
    strncpy(tuner_cfg, "|0:2:0x18:53:0:0xc0:&0:1:0:0:1", VERSION_LEN);
    frontend_mod_init_gx1001(1, tuner_cfg);

    #elif (DEMOD_DVB_T > 0)
    strncpy(tuner_cfg, "|6:0:0xe4:56:0:0x34:&0:1:0:0:1", VERSION_LEN);
    frontend_mod_init_gx1211(1, tuner_cfg); //T2
    #endif
    #endif

    _otp_read_chip_name(tuner_cfg, 8);

}
#if FACTORY_TEST_SUPPORT
char *app_system_info_get_sw_ver(void)
{
    return SW_VER;
}
#endif

static void app_module_item_init(void)
{
    s_sysinfo_item[ITEM_SYSINFO_MODULE_TYPE].itemTitle = "Machine Type";
    s_sysinfo_item[ITEM_SYSINFO_MODULE_TYPE].itemType = ITEM_PUSH;
    s_sysinfo_item[ITEM_SYSINFO_MODULE_TYPE].itemProperty.itemPropertyBtn.string = "NL-5101E";
    s_sysinfo_item[ITEM_SYSINFO_MODULE_TYPE].itemCallback.btnCallback.BtnPress = NULL;
    s_sysinfo_item[ITEM_SYSINFO_MODULE_TYPE].itemStatus = ITEM_NORMAL;
}

static void app_manu_item_init(void)
{
    s_sysinfo_item[ITEM_SYSINFO_MANUFACTURE].itemTitle = "Manufacture";
    s_sysinfo_item[ITEM_SYSINFO_MANUFACTURE].itemType = ITEM_PUSH;
    s_sysinfo_item[ITEM_SYSINFO_MANUFACTURE].itemProperty.itemPropertyBtn.string = "06";
    s_sysinfo_item[ITEM_SYSINFO_MANUFACTURE].itemCallback.btnCallback.BtnPress = NULL;
    s_sysinfo_item[ITEM_SYSINFO_MANUFACTURE].itemStatus = ITEM_NORMAL;
}

static void app_hwinfor_item_init(void)
{

    s_sysinfo_item[ITEM_SYSINFO_HW_INFO].itemTitle = "Hardware Info";
    s_sysinfo_item[ITEM_SYSINFO_HW_INFO].itemType = ITEM_PUSH;
    s_sysinfo_item[ITEM_SYSINFO_HW_INFO].itemProperty.itemPropertyBtn.string = "8M Flash/128M SDRAM";
    s_sysinfo_item[ITEM_SYSINFO_HW_INFO].itemCallback.btnCallback.BtnPress = NULL;
    s_sysinfo_item[ITEM_SYSINFO_HW_INFO].itemStatus = ITEM_NORMAL;
    //memcpy(s_item_content_text_info[ITEM_SYSINFO_DEFAULTDB], date_buf, DATE_TIME_LEN);
}

static void app_hw_item_init(void)
{
    s_sysinfo_item[ITEM_SYSINFO_HW_VERSION].itemTitle = STR_ID_HARDWARE_VER;
    s_sysinfo_item[ITEM_SYSINFO_HW_VERSION].itemType = ITEM_PUSH;
    s_sysinfo_item[ITEM_SYSINFO_HW_VERSION].itemProperty.itemPropertyBtn.string = HW_VER;
    s_sysinfo_item[ITEM_SYSINFO_HW_VERSION].itemCallback.btnCallback.BtnPress = NULL;
    s_sysinfo_item[ITEM_SYSINFO_HW_VERSION].itemStatus = ITEM_NORMAL;
    //memcpy(s_item_content_text_info[ITEM_SYSINFO_DEFAULTDB], date_buf, DATE_TIME_LEN);
}

static void app_sw_item_init(char *soft_buf)
{
    static char sw_version[VERSION_LEN];
    //static char date_buf[DATE_TIME_LEN];
    //char svn_buf[SVN_VER_LEN];

    memset(sw_version, 0, VERSION_LEN);
    //memset(svn_buf, 0, SVN_VER_LEN);

    if (soft_buf != NULL) {
        strcat(sw_version, soft_buf);
    }
    //sprintf(svn_buf,"_SVN%04d", SOFEWARE_VER);
    //strcat(sw_version,svn_buf);

    s_sysinfo_item[ITEM_SYSINFO_SW_VERSION].itemTitle = STR_ID_SOFLTWARE_VER;
    s_sysinfo_item[ITEM_SYSINFO_SW_VERSION].itemType = ITEM_PUSH;
    s_sysinfo_item[ITEM_SYSINFO_SW_VERSION].itemProperty.itemPropertyBtn.string = sw_version;
    s_sysinfo_item[ITEM_SYSINFO_SW_VERSION].itemCallback.btnCallback.BtnPress = NULL;
    s_sysinfo_item[ITEM_SYSINFO_SW_VERSION].itemStatus = ITEM_NORMAL;
}

static void app_loader_version_item_init(void)
{
    s_sysinfo_item[ITEM_SYSINFO_LOADER_VERSION].itemTitle = STR_ID_LOADER_VER;
    s_sysinfo_item[ITEM_SYSINFO_LOADER_VERSION].itemType = ITEM_PUSH;
    s_sysinfo_item[ITEM_SYSINFO_LOADER_VERSION].itemProperty.itemPropertyBtn.string = LD_VER;
    s_sysinfo_item[ITEM_SYSINFO_LOADER_VERSION].itemCallback.btnCallback.BtnPress = NULL;
    s_sysinfo_item[ITEM_SYSINFO_LOADER_VERSION].itemStatus = ITEM_NORMAL;
    //memcpy(s_item_content_text_info[ITEM_SYSINFO_DEFAULTDB], date_buf, DATE_TIME_LEN);
}

static void app_date_item_init(void)
{
    static char date_buf[DATE_TIME_LEN];

    int value[4];

    memset(date_buf, 0, DATE_TIME_LEN);
    sprintf(date_buf, " %d", SOFEWARE_TIME);
    value[0] = SOFEWARE_TIME / 10000;
    value[1] = (SOFEWARE_TIME - value[0] * 10000) / 100;
    value[2] = SOFEWARE_TIME % 100;
    sprintf(date_buf, "%04d-%02d-%02d %s", value[0], value[1], value[2], __TIME__);

    s_sysinfo_item[ITEM_SYSINFO_VER_DATE].itemTitle = STR_ID_BUILD_TIME;

    s_sysinfo_item[ITEM_SYSINFO_VER_DATE].itemType = ITEM_PUSH;
    s_sysinfo_item[ITEM_SYSINFO_VER_DATE].itemProperty.itemPropertyBtn.string = date_buf;
    s_sysinfo_item[ITEM_SYSINFO_VER_DATE].itemCallback.btnCallback.BtnPress = NULL;
    s_sysinfo_item[ITEM_SYSINFO_VER_DATE].itemStatus = ITEM_NORMAL;
}

static char thiz_stb_sn[16] = {'0', '6', '0', '3', '2', '0', '1', '6', '0', '0', '0', '0', '0', '0', '0', '1'};
static void app_sn_item_init(void)
{
    extern int device_chip_otp_open(void);
    extern void device_chip_otp_close(void);
    extern int device_chip_otp_read(unsigned int addr, unsigned int read_bytes, unsigned char *data);

    s_sysinfo_item[ITEM_SYSINFO_SERIAL_NUM].itemTitle = "SN";
    s_sysinfo_item[ITEM_SYSINFO_SERIAL_NUM].itemType = ITEM_PUSH;
    // get stb sn
    {
        unsigned char buf[8] = {0x06, 0x03, 0x20, 0x16, 0x00, 0x00, 0x00, 0x01};
        if (0 == device_chip_otp_open()) {
            if (0 == device_chip_otp_read(0x780, 8, buf)) {
                sprintf(thiz_stb_sn, "%x%02x%02x%02x%02x%02x%02x%02x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
            }
            else {
                printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
            }
            device_chip_otp_close();
        }
        else {
            printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        }

    }
    s_sysinfo_item[ITEM_SYSINFO_SERIAL_NUM].itemProperty.itemPropertyBtn.string = thiz_stb_sn/*"603201600000001"*/;
    s_sysinfo_item[ITEM_SYSINFO_SERIAL_NUM].itemCallback.btnCallback.BtnPress = NULL;
    s_sysinfo_item[ITEM_SYSINFO_SERIAL_NUM].itemStatus = ITEM_NORMAL;
}

static void app_stb_id_item_init(void)
{
    s_sysinfo_item[ITEM_SYSINFO_STBID].itemTitle = "STB ID";
    s_sysinfo_item[ITEM_SYSINFO_STBID].itemType = ITEM_PUSH;
    s_sysinfo_item[ITEM_SYSINFO_STBID].itemProperty.itemPropertyBtn.string = gStbID;
    s_sysinfo_item[ITEM_SYSINFO_STBID].itemCallback.btnCallback.BtnPress = NULL;
    s_sysinfo_item[ITEM_SYSINFO_STBID].itemStatus = ITEM_NORMAL;
}

static void app_system_info_item_property(int sel, ItemProPerty *property)
{
    if (sel >= SYSTEM_INFO_TOTAL_ITEM) {
        return;
    }

    if (s_sysinfo_opt.item[sel].itemType == ITEM_PUSH) {
        if (property->itemPropertyBtn.string != NULL) {
            GUI_SetProperty(s_item_content_btn_info[sel], "string", property->itemPropertyBtn.string);
        }
    }
    GUI_SetProperty(s_system_item_title[sel], "string", s_sysinfo_opt.item[sel].itemTitle);

}

static void app_system_info_focus_item(int sel)
{
    if (s_sysinfo_opt.item[sel].itemStatus == ITEM_NORMAL) {
        GUI_SetProperty(s_system_item_title[sel], "string", s_sysinfo_opt.item[sel].itemTitle);

        GUI_SetProperty(s_item_content_btn_info[sel], "state", "focus");

        s_SystemInfoSel = sel;
    }
}

static int app_system_info_init(SystemSettingOpt *settingOpt)
{
    int sel;
    int n_item_total  = 0;
    if (settingOpt == NULL) {
        return -1;
    }

    GUI_CreateDialog("wnd_system_information");
    GUI_SetProperty("text_system_info_title", "string", settingOpt->menuTitle);
    app_lang_set_menu_font_type("text_system_info_title");

    if (settingOpt->itemNum > SYSTEM_INFO_TOTAL_ITEM) {
        n_item_total = SYSTEM_INFO_TOTAL_ITEM;
    }
    else {
        n_item_total = settingOpt->itemNum;
    }

    for (sel = 0; sel < n_item_total; sel++) {
        app_system_info_item_property(sel, &settingOpt->item[sel].itemProperty);

    }
    app_system_info_focus_item(s_SystemInfoSel);
    return 0;
}

int app_systeminfor_get_chip_sn(unsigned char *SnData, int BufferLen)
{
#define PUBLIC_ID1_STA_REG (*(volatile unsigned int*)(0xA030a560))
#define PUBLIC_ID2_STA_REG (*(volatile unsigned int*)(0xA030a564))
    unsigned char buffer[8] = {0};
    unsigned int reg_data = 0;
    if ((SnData == NULL) && (BufferLen < 8)) {
        return -1;
    }
    reg_data = PUBLIC_ID1_STA_REG;
    buffer[0] = reg_data & 0xff;
    buffer[1] = (reg_data >> 8) & 0xff;
    buffer[2] = (reg_data >> 16) & 0xff;
    buffer[3] = (reg_data >> 24) & 0xff;
    reg_data = PUBLIC_ID2_STA_REG;
    buffer[4] = reg_data & 0xff;
    buffer[5] = (reg_data >> 8) & 0xff;
    buffer[6] = (reg_data >> 16) & 0xff;
    buffer[7] = (reg_data >> 24) & 0xff;
    memcpy(SnData, buffer, 8);
    return 0;
}

void app_sysinfo_menu_exec(void)
{
    memset(&s_sysinfo_opt, 0 , sizeof(s_sysinfo_opt));

    s_sysinfo_opt.menuTitle = STR_ID_SYSTEM_INFO;
    s_sysinfo_opt.titleImage = "s_title_left_utility.bmp";
    s_sysinfo_opt.itemNum = SYSTEM_INFO_TOTAL_ITEM;
    s_sysinfo_opt.timeDisplay = TIP_HIDE;
    s_sysinfo_opt.item = s_sysinfo_item;

    // stb id
    #if NSCAS_SUPPORT
    {
        extern int app_nstv_get_stb_id(char *stb_id, unsigned int *size);
        unsigned int size = sizeof(gStbID);// must initialize to buffer size
        app_nstv_get_stb_id(gStbID, &size);
    }
    #endif

    //app_module_item_init();
    //app_manu_item_init();
    app_hwinfor_item_init();
    app_hw_item_init();
    app_sw_item_init(SW_VER);
    app_loader_version_item_init();
    app_date_item_init();
    app_sn_item_init();
    app_stb_id_item_init();
    s_sysinfo_opt.exit = NULL;

    app_system_info_init(&s_sysinfo_opt);
}

#if FACTORY_SERIALIZATION_SUPPORT
void app_serialization_mode(int key)
{
#define DEFAULT_ENTRY_KEY_LEN 4
    // key code is 8815
    int key_code[DEFAULT_ENTRY_KEY_LEN] = {STBK_8, STBK_8, STBK_1, STBK_5};
    static int correct_code_num = 0;
    if ((key_code[correct_code_num] == key)) {
        if (++correct_code_num == DEFAULT_ENTRY_KEY_LEN) {
            correct_code_num = 0;
            // enter the serialization mode
            extern int app_factory_serialization(int forced_flag);
            app_factory_serialization(1);
        }
    }
    else {
        correct_code_num = 0;
    }
}
#endif

SIGNAL_HANDLER int app_system_info_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch (event->type) {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            #if FACTORY_SERIALIZATION_SUPPORT
            app_serialization_mode(find_virtualkey_ex(event->key.scancode, event->key.sym));
            #endif
            switch (find_virtualkey_ex(event->key.scancode, event->key.sym)) {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog("wnd_system_information");
                    GUI_SetProperty("wnd_system_information", "draw_now", NULL);
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_system_info_got_focus(GuiWidget *widget, void *usrdata)
{

    if (s_sysinfo_opt.item[s_SystemInfoSel].itemType == ITEM_PUSH) {
        //GUI_SetProperty(s_item_content_btn_info[s_SystemInfoSel], "unfocus_img", IMG_BAR_HALF_UNFOCUS);
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_info_lost_focus(GuiWidget *widget, void *usrdata)
{
    if (s_sysinfo_opt.item[s_SystemInfoSel].itemType == ITEM_PUSH) {
        //GUI_SetProperty(s_item_content_btn_info[s_SystemInfoSel], "unfocus_img", IMG_BAR_HALF_FOCUS_R);
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_info_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;

    return ret;
}
