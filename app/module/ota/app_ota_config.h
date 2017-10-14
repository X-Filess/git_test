#ifndef APP_OTA_CONFIG_H
#define APP_OTA_CONFIG_H

int app_ota_dsmcc_get_oui(unsigned int *data);

int app_ota_dsmcc_get_hw_model(unsigned int *data);

int app_ota_dsmcc_get_hw_version(unsigned int *data);

int app_ota_dsmcc_get_sw_model(unsigned int *data);

int app_ota_dsmcc_get_sw_version(unsigned int *data);

int app_ota_dsmcc_set_sw_version(unsigned int data);

int app_ota_get_data_pid(unsigned int *pid);

int app_ota_set_data_pid(unsigned short pid);

int app_ota_get_fre(unsigned int *fre);

int app_ota_set_fre(unsigned int fre);

int app_ota_get_sym(unsigned int *sym);

int app_ota_set_sym(unsigned int sym);

int app_ota_get_pol(unsigned int *pol);

int app_ota_set_pol(unsigned int pol);

int app_ota_get_update_flag(unsigned char *upate_flag);

int app_loader_ota_get_update_flag(unsigned char *update_flag);

int app_menu_ota_get_update_flag(unsigned char *update_flag);

int app_ota_set_update_flag(unsigned char update_flag);

int app_loader_ota_set_update_flag(unsigned char update_flag);

int app_menu_ota_set_update_flag(unsigned char update_flag);

int app_ota_get_notice_flag(unsigned char* notice_flag, int new_version);

int app_ota_set_new_detect_version(unsigned short detected_version);

int app_ota_sync_init(void);

int app_ota_set_menual_flag(void);

int app_ota_get_manual_flag(void);

#endif
