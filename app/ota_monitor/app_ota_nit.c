#include "app_config.h"
#include "app_module.h"

#if OTA_MONITOR_SUPPORT

static struct app_ota_nit s_OtaNit = {
    .subt_id = -1,
    .timeout = 0,
    .ts_src  = 0,
    .demuxid = 0,
    .pid     = 0,
    .req_id  = 0,
    .ver     = 0xff,
};

static char thiz_nit_section_flag[256] = {0};

static status_t _nit_set_equal_filter(unsigned char version)
{
#define NEQ_PARSE_TIME_MS    (24*60*60*1000UL)
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiGet params_get = NULL;
    GxMsgProperty_SiModify params_modify = NULL;

    subt_detail.si_subtable_id = s_OtaNit.subt_id;
    params_get = &subt_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void *)(&params_get));

    // change to unmatch filter
    subt_detail.si_filter.match_depth = 6;
    subt_detail.si_filter.eq_or_neq = NEQ_MATCH;
    subt_detail.si_filter.match[5] = version << 1;
    subt_detail.si_filter.mask[5] = 0x3e;
    subt_detail.time_out = NEQ_PARSE_TIME_MS;

    params_modify = &subt_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_MODIFY, (void *)&params_modify);

    return GXCORE_SUCCESS;
}

typedef struct _NSTVUpgradeInfoClass {
    unsigned int oui;
    unsigned int fre;
    unsigned int sym;
    unsigned int pol;
    unsigned short pid;
    unsigned short download_type;
    unsigned char  upgrade_mode;
    unsigned int   hw_module;
    unsigned int   hw_version;
    unsigned int   sw_module;
    unsigned int   sw_version;
    unsigned int   sn_start;
    unsigned int   sn_end;
} NSTVUpgradeInfoClass;

typedef struct _NSTVOTAUpdateClass {
    int new_version;
    int update_type;//ex. 1:menual; 2:force
    int fre;// ex.11011
    int pol;// ex.0:H; 1:V
    int sym;// ex.30000
    unsigned short pid;//ex. 0x1f48
    int sn_start;//ex. 0x00000000
    int sn_end;//ex.0xffffffff
    int event_flag;//ex. 0:from nit; 1:from pid monitor
} NSTVOTAUpdateClass;

extern int app_ota_dsmcc_get_oui(unsigned int *data);
extern int app_ota_dsmcc_get_hw_model(unsigned int *data);
extern int app_ota_dsmcc_get_hw_version(unsigned int *data);
extern int app_ota_dsmcc_get_sw_model(unsigned int *data);
extern int app_ota_dsmcc_get_sw_version(unsigned int *data);

int check_system_upgrade_info(NSTVUpgradeInfoClass info_in)
{
    int ret = 0;
    NSTVUpgradeInfoClass info = {0};
    // oui
    ret = app_ota_dsmcc_get_oui(&(info.oui));
    if (ret < 0) {
        goto err;
    }
    if (info_in.oui != info.oui) {
        // not match
        return 1;
    }
    ret = app_ota_dsmcc_get_hw_model(&(info.hw_module));
    if (ret < 0) {
        goto err;
    }
    if (info_in.hw_module != info.hw_module) {
        // not match
        return 1;
    }
    ret = app_ota_dsmcc_get_hw_version(&(info.hw_version));
    if (ret < 0) {
        goto err;
    }
    if (info_in.hw_version != info.hw_version) {
        // not match
        return 1;
    }
    ret = app_ota_dsmcc_get_sw_model(&(info.sw_module));
    if (ret < 0) {
        goto err;
    }
    if (info_in.sw_module != info.sw_module) {
        // not match
        return 1;
    }
    ret = app_ota_dsmcc_get_sw_version(&(info.sw_version));
    if (ret < 0) {
        goto err;
    }

    if (info_in.upgrade_mode == 0) {
        // upper
        if (info_in.sw_version <= info.sw_version) {
            // not match
            return 1;
        }
    }
    else if (info_in.upgrade_mode == 1) {
        // equal
        if (info_in.sw_version != info.sw_version) {
            // not match
            return 1;
        }
    }
    else {
        // not equal
        if (info_in.sw_version == info.sw_version) {
            // not match
            return 1;
        }
    }

    return ret;

err:
    return -1;
}

// fre = 10973, sym=24500, pol=v
// A1,2B,00,02,43,0B,01,09,73,00,07,65,A0,02,45,00,00,11,12,19,00,00,00,01,00,00,00,01,00,00,00,01,00,00,00,02,00,00,00,00,FF,FF,FF,FF,00
static int _descriptor_update(char *data)
{
    unsigned int oui = 0;
    unsigned char descriptor_len = 0;
    unsigned char loop_len = 0;
    unsigned char satellite_descriptoe_len = 0;
    unsigned int fre = 0;
    unsigned int sym = 0;
    unsigned int pol = 0;
    unsigned short pid = 0;
    unsigned char  download_type = 0;
    unsigned char private_date_len = 0;
    unsigned int  hw_module = 0;
    unsigned int  hw_version = 0;
    unsigned int  sw_module = 0;
    unsigned int  sw_version = 0;
    unsigned int  sn_start = 0;
    unsigned int  sn_end = 0;
    unsigned char update_mode = 0;
    char *p = data;

    descriptor_len = data[1];
    oui = ((data[2] << 8) | data[3]);
    loop_len = data[5] + 2;
    if ((loop_len > 12) && (data[4] == 0x43)) {
        //satellite delivery system descriptor
        p = data + 4;
        satellite_descriptoe_len = p[1];
        // fre
        fre = (((((p[2] >> 4) & 0xf) * 10 + (p[2] & 0xf)) * 1000000)
               + ((((p[3] >> 4) & 0xf) * 10 + (p[3] & 0xf)) * 10000)
               + ((((p[4] >> 4) & 0xf) * 10 + (p[4] & 0xf)) * 100)
               + (((p[5] >> 4) & 0xf) * 10 + (p[5] & 0xf))) / 100;
        p = p + 8;
        // pol
        pol = ((p[0] >> 5) & 0x3);// h:00; v:01
        // sym
        sym = (((((p[1] >> 4) & 0xf) * 10 + (p[1] & 0xf)) * 10000)
               + ((((p[2] >> 4) & 0xf) * 10 + (p[2] & 0xf)) * 100)
               + ((((p[3] >> 4) & 0xf) * 10 + (p[3] & 0xf))));
    }
    p = data + 4 + loop_len;
    // pid
    pid = (((p[0] << 8) | (p[1] & 0xf8)) >> 3);
    // download type
    download_type = (p[1] & 0x7); //1: menu; 2:force
    // private data len
    private_date_len = p[2];
    if (private_date_len > 24) {
        p = p + 3;
        // hw module
        hw_module = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
        // hw version
        hw_version = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
        // sw module
        sw_module = (p[8] << 24) | (p[9] << 16) | (p[10] << 8) | p[11];
        // sw version
        sw_version = (p[12] << 24) | (p[13] << 16) | (p[14] << 8) | p[15];
        // sn start
        sn_start = (p[16] << 24) | (p[17] << 16) | (p[18] << 8) | p[19];
        // sn end
        sn_end = (p[20] << 24) | (p[21] << 16) | (p[22] << 8) | p[23];
        // update mode: 0: upper need upgrade; 1: equal need upgrade; 2: not
        // equal need upgrade
        update_mode = p[24];
    }
    //check need update or not
    NSTVUpgradeInfoClass info = {0};
    info.oui = oui;
    info.fre = fre;
    info.sym = sym;
    info.pol = pol;
    info.pid = pid;
    info.hw_module = hw_module;
    info.hw_version = hw_version;
    info.sw_module = sw_module;
    info.sw_version = sw_version;
    info.sn_start = sn_start;
    info.sn_end   = sn_end;
    info.download_type = download_type;
    info.upgrade_mode = update_mode;
    if (0 == check_system_upgrade_info(info)) {
        extern int app_ota_ui_msg_send(void *param);

        printf("\033[31m>>>>> Detect OTA update flag %d from NIT!!!\n\033[0m", download_type);
        NSTVOTAUpdateClass param = {0};
        param.new_version = sw_version;
        param.fre = fre;
        param.pol = pol;
        param.sym = sym;
        param.pid = pid;
        param.update_type = download_type;
        param.sn_start = sn_start;
        param.sn_end = sn_end;
        param.event_flag = 0;
        
        app_ota_ui_msg_send(&param);
    }
    return 0;
}

static int _nit_section_is_finished(unsigned char last_section)
{
    int ret = 0;
    int i = 0;
    for (i = 0; i <= last_section; i++) {
        if (0 == thiz_nit_section_flag[i]) {
            break;
        }
    }
    if ((i == last_section) || (0 == last_section)) {
        ret = 1;
    }
    return ret;
}

static int _network_description_parse(char *descriptor)
{
    int descriptor_len = (descriptor[1] + 2);
    switch (descriptor[0]) {
        case 0xA1:
            _descriptor_update(descriptor);
            break;
    }
    return descriptor_len;
}

static int _transport_description_parse(char *descriptor)
{
    int descriptor_len = (descriptor[1] + 2);

    switch (descriptor[0]) {
        case 0xA1:
            _descriptor_update(descriptor);
            break;
    }
    return descriptor_len;
}

/*
 * @return PRIVATE_SECTION_OK   :GXMSG_SI_SECTION_OK
 *         PRIVATE_SUBTABLE_OK  :GXMSG_SI_SUBTABLE_OK
 */
static private_parse_status _nit_filter_callback(unsigned char *Section, size_t Size)
{
    unsigned char               version = 0;
    unsigned short              descriptor_length = 0;
    const unsigned char        *data = Section;
    char                       *p = NULL;
    int                         len = 0;
    int                         len1 = 0;
    unsigned char               section_num = 0;
    unsigned char               last_section_num = 0;
    unsigned short              network_des_len = 0;
    unsigned short              transport_loop_len = 0;
    unsigned short              transport_des_len = 0;

    //if ((NULL == data) || (0x40 != data[0]) || (1 == cascam_module_psi_section_crc32_check((unsigned char *)data))) {
    if ((NULL == data) || (0x40 != data[0])) {
        printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return PRIVATE_SECTION_OK;
    }

    //printf("\033[31m>>>>> Get NIT data!!!\n\033[0m");

    // data parse
    version = ((data[5] & 0x3e) >> 1);
    section_num = data[6];
    if (1 == thiz_nit_section_flag[section_num]) {
        // the section has been processed
        return PRIVATE_SECTION_OK;
    }
    thiz_nit_section_flag[section_num] = 1;
    last_section_num = data[7];
    // for descriptor 1
    network_des_len = (((data[8] & 0xf) << 8) | data[9]);
    p = (char *)(&data[10]);
    if (network_des_len > 0) {
        while (network_des_len > len) {
            descriptor_length = _network_description_parse(p);
            len += descriptor_length;
            p += descriptor_length;
        }
    }
    // for transport_steam_loop
    p = (char *)(data + 10 + network_des_len);
    transport_loop_len = ((p[0] & 0xf) << 8) | p[1];
    if (transport_loop_len > 0) {
        len = 0;
        p += 2;// point to transport stream id
        while (transport_loop_len > len) {
            len += 6 + ((p[4] & 0xf) | p[5]);
            transport_des_len = ((p[4] & 0xf) | p[5]);
            if (transport_des_len) {
                p  += 6;
                len1 = 0;
                while (transport_des_len > len1) {
                    descriptor_length = _transport_description_parse(p);
                    len1 += descriptor_length;
                    p += descriptor_length;
                }
            }
        }
    }

    if (_nit_section_is_finished(last_section_num)) {
        memset(thiz_nit_section_flag, 0, 256);
    }

    return PRIVATE_SECTION_OK;
}

static int _nit_record(struct app_ota_nit *nit)
{
    memcpy(&s_OtaNit, nit, sizeof(struct app_ota_nit));
    return 0;
}

static int _nit_config(struct app_ota_nit *nit)
{
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiCreate  params_create = {0};
    GxMsgProperty_SiStart params_start = {0};

    if (s_OtaNit.subt_id != -1) {
        app_ota_nit_release();
    }

    if (nit == NULL) {
        printf("[%s] No params!\n", __func__);
        return -1;
    }

    subt_detail.si_filter.pid = 0x10;
    subt_detail.si_filter.match_depth = 1;
    subt_detail.si_filter.eq_or_neq = EQ_MATCH;
    subt_detail.si_filter.match[0] = 0x40;
    subt_detail.si_filter.mask[0] = 0xff;
    subt_detail.si_filter.crc = CRC_OFF;
    subt_detail.time_out = 0xFFFFFFFF;

    subt_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
    subt_detail.table_parse_cfg.table_parse_fun = _nit_filter_callback;

    params_create = &subt_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void *)&params_create);

    nit->subt_id = subt_detail.si_subtable_id;
    nit->req_id = subt_detail.request_id;

    params_start = nit->subt_id;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void *)&params_start);

    APP_PRINT("---[app_ota_nit_start][start]---\n");
    APP_PRINT("---[ts_src = %d][pid = 0x%x][subt_id = %d][req_id = %d]---\n", subt_detail.ts_src, subt_detail.si_filter.pid, nit->subt_id, nit->req_id);
    APP_PRINT("---[app_ota_nit_start][end]---\n");

    nit->ver = 0xff;
    return 0;
}

int app_ota_nit_start(struct app_ota_nit *nit)
{
    memset(thiz_nit_section_flag, 0, 256);
    if (0 == _nit_config(nit)) {
        _nit_record(nit);
    }
    return 0;
}

int app_ota_nit_restart(void)
{
    struct app_ota_nit *nit = &s_OtaNit;

    memset(thiz_nit_section_flag, 0, 256);
    if (0 == _nit_config(nit)) {
        _nit_record(nit);
    }
    return 0;
}

static int _nit_stop(void)
{
    struct app_ota_nit *nit = &s_OtaNit;
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiGet si_get = {0};
    GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)nit->subt_id;

    if (nit->subt_id == -1) {
        printf("[%s] nit has been released!\n", __func__);
        return -1;
    }

    nit->ver = 0xff;

    APP_PRINT("---[app_ota_nit_releasei id :%d]---\n", nit->subt_id);
    subt_detail.si_subtable_id = nit->subt_id;
    si_get = &subt_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void *)&si_get);

    app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void *)&params_release);
    nit->subt_id = -1;

    return subt_detail.si_filter.pid;
}

int app_ota_nit_release(void)
{
    return _nit_stop();
}

int app_ota_nit_get(GxMsgProperty_SiSubtableOk *data)
{
    unsigned char version = 0;

    if (data != NULL) {
        if (data->table_id == 0x10
                && data->parsed_data[0] == 0x40
                && data->si_subtable_id == s_OtaNit.subt_id
                && data->request_id == s_OtaNit.req_id) {

            //printf("\033[33m Get GXMSG_SI_SUBTABLE_OK, NIT!!\033[0m\n");

            version = ((data->parsed_data[5] & 0x3e) >> 1);

            memset(thiz_nit_section_flag, 0, 256);
            return _nit_set_equal_filter(version);
        }
    }

    return -1;
}

#endif
