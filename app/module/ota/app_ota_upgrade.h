
#ifndef __APP_OTA_UPGRADE_H__
#define __APP_OTA_UPGRADE_H__

#include "app_config.h"

#if (OTA_SUPPORT > 0)

#define OEM_UPDATE_SECTION	                        ("update")
#define OEM_UPDATE_FLAG	                            ("flag")
#define OEM_UPDATE_FLAG_SECTION                     ("ota_update_section")
#define OTA_DEFAULT_DATA_FILE_PATH		            "/mnt/ota_default.db"

#ifdef ECOS_OS
#define OTA_FILE_TOTAL_SIZE		0x400000
#else
#define OTA_FILE_TOTAL_SIZE		0x800000
#endif

typedef enum
{
	OTA_SEC_ALL = 0,
	OTA_SEC_LOGO,
	OTA_SEC_V,
	OTA_SEC_I,
	OTA_SEC_OTA,
	OTA_SEC_KERNEL,
	OTA_SEC_DATA,
	OTA_SEC_PROG,
	OTA_SEC_TOTAL
}OtaSection;


typedef struct gxota_flash_erase {
    uint32_t    offset;
    size_t      len;
    int         flasherr;
    uint32_t    err_address;
}gxota_flash_erase_t;

typedef gxota_flash_erase_t gxota_flash_unlock_t;

#define OTA_CYG_IO_GET_CONFIG_FLASH_UNLOCK              0x603

//#define OTA_DEBUG
#ifdef OTA_DEBUG
#define GxOtaPrintf(...)  printf("[OTA]"__VA_ARGS__)
#else
#define GxOtaPrintf(...) //printf("[OTA]"__VA_ARGS__)
#endif
void app_ota_process_update(int percent);
void app_ota_message_update(char *message1, char *message2);
void app_ota_upgrade_entry(void);
void app_ota_upgrade_exit(void);
void app_ota_get_segment(void);
void app_ota_logo_update(void);
void app_ota_biss_update(void);
void app_ota_defaultdata_update(void);
int app_ota_check_and_flush(unsigned int data_version);
void *app_ota_malloc(unsigned int size, void* hw_data);
void app_ota_free(void* hw_data);

#endif
#endif
