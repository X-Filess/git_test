#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "module/config/gxconfig.h"
#include "gxoem.h"
#include "gxupdate.h"
#include "app_utility.h"
#include "module/update/gxupdate_partition_file.h"
#include "module/update/gxupdate_partition_flash.h"
#include "module/update/gxupdate_protocol_usb.h"
#include "module/update/gxupdate_protocol_ts.h"
#include "module/update/gxupdate_protocol_serial.h"
#include "app_wnd_file_list.h"
#include <common/gx_hw_malloc.h>
#include <gxcore.h>

#ifdef ECOS_OS
#include "linux/zlib.h"
#endif
#ifdef LINUX_OS
#include "zlib.h"
#include "youtube_tools.h"
#endif

#define CA_VERIFY_SUPPORT 1
#define UPGRADE_DATA_SUPPORT 0
#define UPGRADE_P2P_SUPPORT 0
#define UPGRADE_DUMP_SUPPORT 0
#define FLASH_BIN_LENGTH (uint32_t)(4 * 1024 * 1024)
#define BIN_BLOCK_SIZE (uint32_t)(0x10000)
#define UPGRADE_SUFFIX "bin;BIN"
#define BOOT_SECTION_NAME "BOOT"
#define DATA_SECTION_NAME "DATA"
#define PRIVATE_SECTION_NAME "RESERVE"
#define DUNM_BIN_ALL "dump_all.bin"
//#define DUNM_BIN_USER "dump_user.bin"
//#define DUNM_BIN_BOOT "dump_boot.bin"
//#define DUNM_BIN_UIMAGE "dump_uImage.bin"
//#define DUNM_BIN_FS "dump_fs.bin"
//#define DUNM_BIN_DB "dump_db.bin"
//#define TEMP_SECT_BIN "/mnt/temp_sect.bin"

#define UPDATE_FAILED_OSD_ENGLISH  "\"File is incorrect\" (or) \"File[*.bin] could not be found in USB device!\""
static GxUpdate_ProtocolOps* protocol_list[] =
{
    &gxupdate_protocol_ts,
    &gxupdate_protocol_serial,
    &gxupdate_protocol_usb,
    NULL
};
static GxUpdate_PartitionOps* partition_list[] =
{
    &gxupdate_partition_flash,
    &gxupdate_partition_file,
    NULL
};
GxUpdate_ProtocolOps**  gxupdate_protocol_list = protocol_list;
GxUpdate_PartitionOps** gxupdate_partition_list = partition_list;

enum UPGRADE_ITEM_NAME
{
	ITEM_UPGRADE_TYPE = 0,
	ITEM_UPGRADE_SECT,
	ITEM_UPGRADE_PATH,
	ITEM_UPGRADE_EXEC,
	ITEM_UPGRADE_TOTAL
};

typedef enum
{
	UPGRADE_TYPE_U2S,
	#if (UPGRADE_P2P_SUPPORT == 1)
	UPGRADE_TYPE_S2S,
	#endif
	UPGRADE_TYPE_DUMP
}UpgradeTypeSel;

typedef enum
{
	UPGRADE_SECT_ALL,
	UPGRADE_SECT_APP,
#if UPGRADE_DATA_SUPPORT
	UPGRADE_SECT_DATA,
#endif
}UpgradeSectSel;

typedef struct
{
	UpgradeTypeSel type;
	UpgradeSectSel section;
	char *path;
}UpgradePara;

typedef enum
{
	MODE_UPGRADE,
	MODE_DUMP
}UsbUpgradeMode;

static char *s_temp_file = NULL;
static SystemSettingOpt s_upgrade_opt;
static SystemSettingItem s_upgrade_item[ITEM_UPGRADE_TOTAL];
static char *s_file_path = NULL;
static char *s_file_path_bak = NULL;
static event_list* sp_UpgradeTimeOut = NULL;

static enum
{
	UPGRADE_START = 0,
	UPGRADE_SUCCESS,
	UPGRADE_STOPPED
}s_upgade_state = UPGRADE_STOPPED;

extern int g_system_shell_ret_status;

extern int print_enable;
extern void app_update_sync_data(void);
extern void app_update_reboot(void);
#if CA_VERIFY_SUPPORT
extern int ca_check_need_verify(char *name);
extern int ca_verify_sign(char *buf, unsigned int size);
#endif

#define CRC_DEBUG
#ifdef CRC_DEBUG
static char *get_size_str(char *size_str, size_t size)
{
    if(size % 1024){
        sprintf(size_str, "%6d  B", size);
    }else{
        size /= 1024;		//k
        if(size % 1024){
            sprintf(size_str, "%6d KB", size);
        }else{
            size /= 1024; //m
            sprintf(size_str, "%6d MB", size);
        }
    }

    return size_str;
}

static char *crc32str(unsigned int crc)
{
    static char buf[9] = {0, };

    if (crc == 0)
        memset(buf, ' ', 8);
    else
        sprintf(buf, "%08X", crc);

    return buf;
}
#endif

static int char2int(unsigned char *data)
{
	return (data[0] << 24) + (data[1] << 16) + (data[2]  << 8) + data[3] ;
}

static int char2int16(uint8_t *data)
{
    return (data[0]  << 8) + data[1];
}

static int flash_table_read(struct partition *tbl, unsigned char *data, int len)
{
    int i;
    unsigned int new_crc;
    unsigned char *part;
    unsigned char *crc32_table = data + FLASH_PARTITION_ADDR_EX;
    unsigned char *part_version = data + FLASH_PARTITION_VERSION_ADDR;
    if (len < FLASH_PARTITION_SIZE)
        return -1;

    memset(tbl, 0, sizeof(struct partition));  // read table

    tbl->magic = char2int(data);
    tbl->count = data[4];
    tbl->crc_verify = char2int(data + FLASH_PARTITION_SIZE - 4);
    tbl->version = data[FLASH_PARTITION_SIZE - 5];
    tbl->crc32_enable = data[FLASH_PARTITION_SIZE - 6];
    tbl->write_protect = data[FLASH_PARTITION_SIZE - 7];

    if (tbl->count > FLASH_PARTITION_NUM) {
        printf("%s,%d partition_num=%d error\n", __func__, __LINE__, tbl->count);

        return -1;
    }

    part = data + 5;
    for (i = 0; i < tbl->count; i++, part += FLASH_PARTITION_HEAD_SIZE) {
        memcpy(tbl->tables[i].name, part, FLASH_PARTITION_NAME_SIZE);
        tbl->tables[i].total_size       = char2int(part +  8);
        tbl->tables[i].used_size        = char2int(part + 12);
        tbl->tables[i].start_addr       = char2int(part + 16);
        tbl->tables[i].file_system_type = part[20];
        tbl->tables[i].mode             = part[21] & 0x7F;
        tbl->tables[i].crc32_enable     = part[21] >> 7;

        tbl->tables[i].update_tags = part[23] & 0x03;
        tbl->tables[i].crc_verify = char2int(crc32_table + i * 4);
        tbl->tables[i].version = char2int16(part_version + i * 2);
    }
#ifdef CRC_DEBUG
    {
        char size_str1[32], size_str2[32];
        printf("Partition Version :  %d\n", tbl->version);
        printf("Partition Count   :  %d\n", tbl->count);
        printf("Write Protect     :  %s\n", tbl->write_protect ? "TRUE": "FALSE");
        printf("CRC32 Enable      :  %s\n", tbl->crc32_enable ? "TRUE": "FALSE");
        printf("Table CRC32       :  %X\n", tbl->crc_verify);

        printf("==========================================================\n");
        printf("%-8s%-10s%-9s%10s%11s%10s\n", "NAME",  "CRC32", "START", "SIZE", "USED", "Use%");
        printf("==========================================================\n");
        for (i=0; i < tbl->count; i++) {
            printf("%-8s%-10s%08x%11s%11s%8d %%\n",
                    tbl->tables[i].name,
                    crc32str(tbl->tables[i].crc_verify),
                    tbl->tables[i].start_addr,
                    get_size_str(size_str1, tbl->tables[i].total_size),
                    get_size_str(size_str2, tbl->tables[i].used_size),
                    tbl->tables[i].total_size > 0 ? tbl->tables[i].used_size * 100 / tbl->tables[i].total_size : 0
                  );
        }
        printf("----------------------------------------------------------\n\n");
    }
#endif
    new_crc = crc32(0, data, len - 4);
#ifdef CRC_DEBUG
    printf("------0x%X----------------------0x%X-----------\n\n", new_crc, tbl->crc_verify);
#endif
    if(new_crc != tbl->crc_verify)
        return 0;
    else
        return 1;
}

static int app_upgrade_crc_check(char *path)
{
	unsigned char buf[FLASH_PARTITION_SIZE] = {0};
	struct partition tbl;
	FILE *file;
	int ret = 0;
	uint32_t magic  = 0;
    bool table_flag = false;

	file = fopen(path, "r");
	if (file == 0)
	{
		printf("----------file open err.\n");
		return 0;
	}

    ret = fread(buf, 1, FLASH_PARTITION_SIZE, file);
    while(ret == FLASH_PARTITION_SIZE)
    {
        magic = char2int(buf);
        if(magic == PARTITION_MAGIC)
        {
            table_flag = true;
            break;
        }

        memset(buf, 0, FLASH_PARTITION_SIZE);
        ret = fread(buf, 1, FLASH_PARTITION_SIZE, file);
    }

	if(table_flag == true)
	{
		if (flash_table_read(&tbl, buf, FLASH_PARTITION_SIZE))
		{
            if(tbl.crc32_enable > 0)
            {
                int i = 0;
                unsigned char *data = NULL;
                GxHwMallocObj hw_data;
                unsigned int new_crc;

                for(i = 0; i < tbl.count; i++)
                {
                    if((i != 1) && (tbl.tables[i].crc32_enable != 0))
                    {
                        fseek(file, tbl.tables[i].start_addr, SEEK_SET);
                        memset(&hw_data, 0, sizeof(GxHwMallocObj));
                        hw_data.size = tbl.tables[i].used_size;
                        GxCore_HwMalloc(&hw_data,MALLOC_NO_CACHE);
                        if(hw_data.usr_p == NULL)
                        {
                            ret = 0;
                            goto END;
                        }

                        data = (unsigned char*)hw_data.usr_p;
                        ret = fread(data, 1, tbl.tables[i].used_size, file);
                        if (ret !=  tbl.tables[i].used_size)
                        {
                            printf("(1)------read failed %d.\n", ret);
                            GxCore_HwFree(&hw_data);
                            data = NULL;
                            ret =0;
                            goto END;
                        }
                        new_crc = crc32(0, data,  tbl.tables[i].used_size);
                        GxCore_HwFree(&hw_data);
                        data = NULL;
#ifdef CRC_DEBUG
                        printf("------0x%X----------------------0x%X-----------\n\n", new_crc, tbl.tables[i].crc_verify);
#endif
                        if(new_crc != tbl.tables[i].crc_verify)
                        {
                            ret = 0;
                            goto END;
                        }
                    }
                }
            }
			ret = 1;
		}
		else
			ret =0;
	}
	else
	{
		ret = 0;
	}
END:
	fclose(file);
	return ret;
}

#if CA_VERIFY_SUPPORT
// ret: 0-failed; 1-success
extern int device_chip_otp_open(void);
extern void device_chip_otp_close(void);
extern int device_chip_otp_read(unsigned int addr, unsigned int read_bytes, unsigned char *data);
static int app_upgrade_verify_check(char *path)
{
	unsigned char buf[FLASH_PARTITION_SIZE] = {0};
	struct partition tbl;
	FILE *file;
	int ret = 0;
	uint32_t magic  = 0;
    bool table_flag = false;
// 添加芯片安全启动状态检测，理论上开启了安全启动就必须要进行签名校验，目前代码
// 实现的方式是，当为非安全启动模式下就不做签名校验，方便方案开发阶段的调试。
    {
        unsigned char ads_status = 0;
        if(0 == device_chip_otp_open())
        {
            if(0 == device_chip_otp_read(0x313, 1, &ads_status))
            {
                if((ads_status&0x7) == 0x6)
                {
                    device_chip_otp_close();
                }
                else
                {
                    printf("\nno adveranced scurity start, need not verify\n");
                    device_chip_otp_close();
                    return 1;
                }
            }
            else
            {
                printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
                device_chip_otp_close();
                return 0;
            }
        }
        else
        {
            printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
            device_chip_otp_close();
            return 0;
        }
    }
	file = fopen(path, "r");
	if (file == 0)
	{
		printf("----------file open err.\n");
		return 0;
	}

    ret = fread(buf, 1, FLASH_PARTITION_SIZE, file);
    while(ret == FLASH_PARTITION_SIZE)
    {
        magic = char2int(buf);
        if(magic == PARTITION_MAGIC)
        {
            table_flag = true;
            break;
        }

        memset(buf, 0, FLASH_PARTITION_SIZE);
        ret = fread(buf, 1, FLASH_PARTITION_SIZE, file);
    }

	if(table_flag == true)
	{
		if (flash_table_read(&tbl, buf, FLASH_PARTITION_SIZE))
        {
            int i = 0;
            char *data = NULL;
            GxHwMallocObj hw_data;

            for(i = 0; i < tbl.count; i++)
            {
                if((i != 1)&& (ca_check_need_verify(tbl.tables[i].name)))
                {
                    fseek(file, tbl.tables[i].start_addr, SEEK_SET);
                    memset(&hw_data, 0, sizeof(GxHwMallocObj));
                    hw_data.size = tbl.tables[i].total_size;
                    GxCore_HwMalloc(&hw_data,MALLOC_NO_CACHE);
                    if(hw_data.usr_p == NULL)
                    {
                        ret = 0;
                        goto END;
                    }

                    data = (char*)hw_data.usr_p;
                    ret = fread(data, 1, tbl.tables[i].total_size, file);
                    if (ret !=  tbl.tables[i].total_size)
                    {
                        printf("\nbreak, %s, %d------read failed %d.\n", __func__, __LINE__,ret);
                        GxCore_HwFree(&hw_data);
                        data = NULL;
                        ret =0;
                        goto END;
                    }
                    printf("\n\033[35mCA VERIFY, name = %s \033[0m\n", tbl.tables[i].name);
                    if(0 == ca_verify_sign(data, tbl.tables[i].total_size))
                    {// success
                        ret = 1;
                    }
                    else
                    {// failed
                        ret = 0;
                    }
                    GxCore_HwFree(&hw_data);
                    data = NULL;
                    if(ret == 0)
                    {
                        goto END;
                    }
                }
            }
            ret = 1;
        }
		//else
        //{
		//	ret =0;
        //}
	}
	//else
	//{
	//	ret = 0;
	//}
END:
	fclose(file);
	return ret;
}
#endif

#ifdef ECOS_OS
static status_t app_get_partition_info(UpgradeSectSel section, GxUpdate_ConfigFlash *conf)
{
	GxPartitionTable *table_info = NULL;
	status_t ret = GXCORE_SUCCESS;
	int i, j;

	table_info = GxCore_Malloc(sizeof(GxPartitionTable));
	if(NULL == table_info)
		return  GXCORE_ERROR;

	memset(table_info,0,sizeof(GxPartitionTable));
	if(GxOem_PartitionTableGet(table_info) < 0)
	{
		GxCore_Free(table_info);
		return GXCORE_ERROR;
	}

	if(UPGRADE_SECT_ALL == section)
	{
		conf->start_addr = 0;
		conf->size = 0;

		for(i = 0; i < table_info->count; i++)
		{
			if(table_info->tables[i].name != NULL
				&& 0 == strcmp(table_info->tables[i].name, BOOT_SECTION_NAME))
			{
				conf->start_addr = table_info->tables[i].start_addr;
				for(j = i; j < table_info->count; j++)
				{
                    if(0 == strcmp(table_info->tables[j].name, PRIVATE_SECTION_NAME))
                    {// 用户自定义段，例如ca等关键数据，出厂后就不能因为软件的菜
                     // 单操作而修改，需要跳过更新目前处理比较简单，有很强的局限
                     // 性，就是PRIVATE的分区必须放到最后，就如该部分代码控制，
                     // 检测到user分区后就返回了，那后面的数据就不做更新了。
                        break;
                    }
					conf->size += table_info->tables[j].total_size;
				}
                break;
			}
		}
	}
    else if(UPGRADE_SECT_APP == section)
	{
		conf->start_addr = 0;
		conf->size = 0;

		for(i = 0; i < table_info->count; i++)
		{
			if(table_info->tables[i].name != NULL
				&& 0 == strcmp(table_info->tables[i].name, BOOT_SECTION_NAME))
			{
				conf->start_addr = table_info->tables[i].start_addr;
				for(j = i; j < table_info->count; j++)
				{
                    if((0 == strcmp(table_info->tables[j].name, DATA_SECTION_NAME))
                        ||(0 == strcmp(table_info->tables[j].name, PRIVATE_SECTION_NAME)))
                    {// 数据段，目前的实现也很有局限性，需要FLASH的分区配置配合，
                     // 目前的设定是数据段之前的都定义为APP相关，所以就如上述ALL
                     // 段的PRIVATE分区的处理。PRIVATE分区也不能包含在APP中，当然
                     // 目前的定义关系是这样的。具体PRIVATE的用途有整机方案规格定
                     // 义，只是需要和flash.conf相互配合使用。【同样OTA的升级部分
                     // 也需要同样注意】
                        break;
                    }

					conf->size += table_info->tables[j].total_size;
				}
                break;
			}
		}
	}
#if UPGRADE_DATA_SUPPORT
    else if(UPGRADE_SECT_DATA == section)
    {
        bool isFind = false;

        for(i = 0; i < table_info->count; i++)
        {
            if(table_info->tables[i].name != NULL
                    && 0 == strcmp(table_info->tables[i].name, DATA_SECTION_NAME))
            {
                conf->start_addr = table_info->tables[i].start_addr;
                conf->size = table_info->tables[i].total_size;
                isFind = true;
                break;
            }
        }

        if(isFind == false) //can't find partition
            ret = GXCORE_ERROR;
    }
    else
#endif
    {
        ret = GXCORE_ERROR;
    }
	GxCore_Free(table_info);
	return ret;
}

#if 0
static status_t app_copy_sect2ramfs(const char *file_path, const char *partition_name)
{
    char *temp = NULL;
    handle_t file_fd = 0;
    struct partition file_partiton;
    uint32_t sect_addr = 0;
    uint32_t sect_size = 0;
    uint32_t i = 0;
    status_t ret = GXCORE_ERROR;

    if((file_path == NULL) || (partition_name == NULL))
        return GXCORE_ERROR;

    memset(&file_partiton, 0, sizeof(struct partition));
    if(GxCore_PartitionInit(&file_partiton, NULL, file_path) == NULL) // bin file partition
        return GXCORE_ERROR;

    for(i = 0; i < file_partiton.count; i++)
    {
        if(strcmp(file_partiton.tables[i].name, partition_name) == 0)
        {
            sect_addr = file_partiton.tables[i].start_addr;
            sect_size = file_partiton.tables[i].total_size;
            break;
        }
    }

    if(i >= file_partiton.count) //can't find partiton
    {
        return GXCORE_ERROR;
    }

    if((temp = (char*)GxCore_Calloc(sect_size, 1)) == NULL)
        return GXCORE_ERROR;

    if((file_fd = GxCore_Open(file_path, "r")) < 0)
    {
        GxCore_Free(temp);
        return GXCORE_ERROR;
    }

    ret = GxCore_ReadAt(file_fd, (void*)temp, sect_size, 1, sect_addr);
    GxCore_Close(file_fd);
    file_fd = 0;
    if(ret == GXCORE_ERROR)
    {
        GxCore_Free(temp);
        return GXCORE_ERROR;
    }

    if((file_fd = GxCore_Open(TEMP_SECT_BIN, "w+")) < 0)
    {
        GxCore_Free(temp);
        return GXCORE_ERROR;
    }

    ret = GxCore_Write(file_fd, (void*)temp, sect_size, 1);
    GxCore_Free(temp);
    GxCore_Close(file_fd);
    file_fd = 0;
    if(ret > 0)
        ret = GXCORE_SUCCESS;
    else
        ret = GXCORE_ERROR;

    return ret;
}
#endif

static status_t app_copy_sect(const char *bin_file, const char *partition_name, const char *dest_file)
{
    char *temp = NULL;
    handle_t src_fd = 0;
    handle_t dest_fd = 0;
    struct partition file_partiton;
    uint32_t sect_addr = 0;
    uint32_t sect_size = 0;
    uint32_t i = 0;
    status_t ret = GXCORE_ERROR;

    if((bin_file == NULL) || (partition_name == NULL) || (dest_file == NULL))
        return GXCORE_ERROR;

    memset(&file_partiton, 0, sizeof(struct partition));
    if(GxCore_PartitionInit(&file_partiton, NULL, bin_file) == NULL) // bin file partition
        return GXCORE_ERROR;

    for(i = 0; i < file_partiton.count; i++)
    {
        if(strcmp(file_partiton.tables[i].name, partition_name) == 0)
        {
            sect_addr = file_partiton.tables[i].start_addr;
            sect_size = file_partiton.tables[i].total_size;
            break;
        }
    }

    if(i >= file_partiton.count) //can't find partiton
        return GXCORE_ERROR;

    if((temp = (char*)GxCore_Calloc(BIN_BLOCK_SIZE, 1)) == NULL)
        return GXCORE_ERROR;

    if((src_fd = GxCore_Open(bin_file, "r")) < 0)
    {
        GxCore_Free(temp);
        return GXCORE_ERROR;
    }

    if((dest_fd = GxCore_Open(dest_file, "w+")) < 0)
    {
        GxCore_Free(temp);
        GxCore_Close(src_fd);
        return GXCORE_ERROR;
    }

    GxCore_Seek(src_fd, sect_addr, GX_SEEK_SET);
    for(i = 0; i < sect_size; i += BIN_BLOCK_SIZE)
    {
        ret = GxCore_Read(src_fd, (void*)temp, 1, BIN_BLOCK_SIZE);
        if(ret <= 0)
            break;
        ret = GxCore_Write(dest_fd, (void*)temp, 1, BIN_BLOCK_SIZE);
        if(ret <= 0)
            break;
    }

    GxCore_Free(temp);
    GxCore_Close(dest_fd);
    GxCore_Close(src_fd);

    if(ret > 0)
        return GXCORE_SUCCESS;
    else
        return GXCORE_ERROR;
}

static status_t app_usb_upgrade_config(UpgradePara *ret_para)
{
    GxMsgProperty_UpdateOpen updateOpen;
    GxMsgProperty_UpdateProtocolSelect protSel;
    GxUpdate_ConfigUsbTerminalType type;
    GxMsgProperty_UpdatePartitionSelect partSel;
    GxUpdate_IoCtrl ctrl;
    GxUpdate_SelectUsbFile info;
    GxUpdate_ConfigFlash config;
    int32_t len = 0;
    int32_t state = GXUPDATE_OK - 1;

    if(NULL == ret_para->path)
    {
        return GXCORE_ERROR;
    }

    memset(&info, 0, sizeof(GxUpdate_SelectUsbFile));
    if(UPGRADE_TYPE_U2S== ret_para->type)
    {
#if UPGRADE_DATA_SUPPORT
        if(ret_para->section == UPGRADE_SECT_DATA)
        {
            s_temp_file = (char*)GxCore_Calloc(strlen(ret_para->path) + 6, 1);
            if(s_temp_file == NULL)
                return GXCORE_ERROR;

            //for mini memory update solution, make temp data file under the same usb directory
            strcat(s_temp_file, ret_para->path);
            strcat(s_temp_file, ".user");

            if(app_copy_sect(ret_para->path, DATA_SECTION_NAME, s_temp_file) == GXCORE_ERROR)
                return GXCORE_ERROR;

            strlcpy(info.file, s_temp_file, GXUPDATE_MAX_FILE_PATH_LENGTH);
        }
        else
#endif
        {
            strlcpy(info.file, ret_para->path, GXUPDATE_MAX_FILE_PATH_LENGTH);
        }

        type.type = GXUPDATE_CLIENT;
    }
    else if(UPGRADE_TYPE_DUMP == ret_para->type)
    {
        len = strlen(ret_para->path);
        if((len >= (GXUPDATE_MAX_FILE_PATH_LENGTH - 15)) || (len <= 0))
        {
            return GXCORE_ERROR;
        }
        sprintf(info.file, "%s/", ret_para->path);
        strcat(info.file, DUNM_BIN_ALL);
        type.type = GXUPDATE_SERVER;
    }

    memset(&updateOpen, 0, sizeof(GxMsgProperty_UpdateOpen));
    memcpy(&updateOpen, "NULL", sizeof(GxMsgProperty_UpdateOpen));
    app_send_msg_exec(GXMSG_UPDATE_OPEN, &updateOpen);

    memset(&protSel,0,sizeof(GxMsgProperty_UpdateProtocolSelect));
    memcpy(&protSel, GXUPDATE_PROTOCOL_USB, sizeof(GxMsgProperty_UpdateProtocolSelect));
    app_send_msg_exec(GXMSG_UPDATE_PROTOCOL_SELECT, &protSel);

    ctrl.key = GXUPDATE_USB_SELECT_TERMINAL_TYPE;
    ctrl.buf = &type;
    ctrl.size = sizeof(GxUpdate_ConfigUsbTerminalType);
    app_send_msg_exec(GXMSG_UPDATE_PROTOCOL_CTRL, (void*)&ctrl);

#if 0
    if((ret_para->section == UPGRADE_SECT_ALL) && (ret_para->type == UPGRADE_TYPE_U2S))
    {
        ctrl.key = GXUPDATE_USB_SET_CRC; //crc
        ctrl.buf = NULL;
        ctrl.size = 0;
        //app_send_msg_exec(GXMSG_UPDATE_PROTOCOL_CTRL, (void*)&ctrl); //disable for section upgrade
    }
#endif

    ctrl.key = GXUPDATE_SELECT_FILE_NAME;
    ctrl.buf = &info;
    ctrl.size = sizeof(GxUpdate_SelectUsbFile);
    app_send_msg_exec(GXMSG_UPDATE_PROTOCOL_CTRL, (void*)&ctrl);

    state = GxUpdate_StreamGetStatus();
    if(state < GXUPDATE_OK)
    {
        return GXCORE_ERROR;
    }

	if(UPGRADE_TYPE_DUMP != ret_para->type)
	{
		app_update_sync_data();
	}
    memset(&partSel,0,sizeof(GxMsgProperty_UpdatePartitionSelect));
    memcpy(&partSel,GXUPDATE_PARTITION_FLASH, sizeof(GxMsgProperty_UpdatePartitionSelect));
    app_send_msg_exec(GXMSG_UPDATE_PARTITION_SELECT, &partSel);

    memset(&config,0,sizeof(GxUpdate_ConfigFlash));
    app_get_partition_info(ret_para->section, &config);
    ctrl.key = GXUPDATE_CONFIG_FLASH;
    ctrl.buf = &config;
    ctrl.size = sizeof(GxUpdate_ConfigFlash);
    app_send_msg_exec(GXMSG_UPDATE_PARTITION_CTRL, (void*)&ctrl);

    return GXCORE_SUCCESS;
}

#if (UPGRADE_P2P_SUPPORT == 1)
static status_t app_serial_upgrade_config(UpgradePara *ret_para)
{
    GxMsgProperty_UpdateOpen updateOpen;
    GxMsgProperty_UpdateProtocolSelect protSel;
    GxUpdate_ConfigUsbTerminalType type;
    GxMsgProperty_UpdatePartitionSelect partSel;
    GxUpdate_IoCtrl ctrl;
    GxUpdate_SerialInfo info;
    GxUpdate_ConfigFlash config;
    GxUpdate_SerialPlatform platform = {0};

    memset(&updateOpen,0,sizeof(GxMsgProperty_UpdateOpen));
    memcpy(&updateOpen,"NULL",sizeof(GxMsgProperty_UpdateOpen));
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_UPDATE_OPEN, &updateOpen))
    {
        return GXCORE_ERROR;
    }

    memset(&protSel,0,sizeof(GxMsgProperty_UpdateProtocolSelect));
    memcpy(&protSel,GXUPDATE_PROTOCOL_SERIAL,sizeof(GxMsgProperty_UpdateProtocolSelect));
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_UPDATE_PROTOCOL_SELECT, &protSel))
    {
        return GXCORE_ERROR;
    }

    type.type = GXUPDATE_SERVER;
    ctrl.key = GXUPDATE_SERIAL_SELECT_TERMINAL_TYPE;
    ctrl.buf = &type;
    ctrl.size = sizeof(GxUpdate_ConfigSerialTerminalType);
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_UPDATE_PROTOCOL_CTRL, (void*)&ctrl))
    {
        return GXCORE_ERROR;
    }

    info.baud = SERIAL_BAUD_115200;
    info.parity = SERIAL_PARITY_NONE;
    info.stop = SERIAL_STOP_1;
    info.word_length = SERIAL_WORD_LENGTH_8;
    info.flags = 0;
    ctrl.key = GXUPDATE_SET_SERIAL_INFO;
    ctrl.buf = &info;
    ctrl.size = sizeof(GxUpdate_SerialInfo);
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_UPDATE_PROTOCOL_CTRL, (void*)&ctrl))
    {
        return GXCORE_ERROR;
    }

    memset(&partSel,0,sizeof(GxMsgProperty_UpdatePartitionSelect));
    memcpy(&partSel,GXUPDATE_PARTITION_FLASH,sizeof(GxMsgProperty_UpdatePartitionSelect));
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_UPDATE_PARTITION_SELECT, &partSel))
    {
        return GXCORE_ERROR;
    }

    memset(&config,0,sizeof(GxUpdate_ConfigFlash));
    app_get_partition_info(ret_para->section, &config);
    ctrl.key = GXUPDATE_CONFIG_FLASH;
    ctrl.buf = &config;
    ctrl.size = sizeof(GxUpdate_ConfigFlash);
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_UPDATE_PARTITION_CTRL, (void*)&ctrl))
    {
        return GXCORE_ERROR;
    }

    platform.nWork_Mode       = 0;
    platform.nChip_Type       = 0x20;
    platform.nXtal            = 27000;
    platform.nFlash_Interface = 1;
    platform.nDRAM_Type       = 1;
    platform.nDRAM_Size       = 2;
    platform.nMix_Type        = 0;
    platform.nSection_Start   = config.start_addr;
    platform.nSection_Len     = config.start_addr + config.size;
    platform.nFlash_Model     = 0x91;
    platform.nSdramModel      = 0;
    platform.bFastModel = 1;

    ctrl.key = GXUPDATE_SET_CONFIG_PLATFORM;
    ctrl.buf = &platform;
    ctrl.size = sizeof(GxUpdate_SerialPlatform);
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_UPDATE_PROTOCOL_CTRL, (void*)&ctrl))
    {
        return GXCORE_ERROR;
    }

    return GXCORE_SUCCESS;
}
#endif
#endif

#ifdef LINUX_OS
static uint32_t s_step_num = 0;
static uint32_t s_update_rate = 0;
#define USB_UPDATE_PROGRESS "/tmp/usb_progress"
void usb_update_proc(void* userdata)
{
#define MAX_COUNT_LEN 16
    char count_buf[MAX_COUNT_LEN];

    int len = 0;
    int ibegin=0, iend=0;
    FILE *fp = NULL;

    if(100 == s_update_rate)
    {
        return;
    }

    s_step_num++;

    //printf("------------------------s_step_num: %d\n", s_step_num);
    if(0 == s_step_num % 10)
    {
        if(GXCORE_FILE_UNEXIST == GxCore_FileExists(USB_UPDATE_PROGRESS))
        {
            return;
        }

        fp = fopen(USB_UPDATE_PROGRESS, "r");
        if(NULL == fp)
        {
            return;
        }

        len = MAX_COUNT_LEN;
        fgets_own(count_buf, &len, fp);
        if(len > 0)
        {
            ibegin = atoi(count_buf);
        }

        len = MAX_COUNT_LEN;
        fgets_own(count_buf, &len, fp);
        if(len > 0)
        {
            iend = atoi(count_buf);
        }

        fclose(fp);

        //printf("++++++++++++++++++++++--%d, begin:%d, end:%d\n", s_update_rate, ibegin, iend);
        if(100 == ibegin && 100 == iend)
        {
            s_update_rate = 100;
            GUI_SetProperty("progbar_upgrade_process", "value", (void*)&s_update_rate);

            GUI_SetProperty("txt_upgrade_process_info1", "string", STR_UPGRADE_SUCCESS);
            GUI_SetProperty("txt_upgrade_process_info2", "string", STR_REBOOT_NOW);

            remove_timer(sp_UpgradeTimeOut);
            sp_UpgradeTimeOut = NULL;

            return;
        }

        if(s_update_rate < ibegin)
        {
            s_update_rate = ibegin;
        }
        else if(s_update_rate < iend)
        {
            s_update_rate++;
        }

        if(s_update_rate>=100)
            s_update_rate = 99;
        GUI_SetProperty("progbar_upgrade_process", "value", (void*)&s_update_rate);
        reset_timer(sp_UpgradeTimeOut);
    }
}

void usb_update_ok(void* userdata)
{
	s_update_rate = 100;
	GUI_SetProperty("progbar_upgrade_process", "value", (void*)&s_update_rate);

    remove_timer(sp_UpgradeTimeOut);
    sp_UpgradeTimeOut = NULL;

	if(0 == g_system_shell_ret_status)
	{
#if 0 //it's a risk, maybe read ttf data from updated flash;

		GUI_SetProperty("txt_upgrade_process_info1", "string", STR_UPGRADE_SUCCESS);
		GUI_SetProperty("txt_upgrade_process_info2", "string", STR_REBOOT_NOW);

        GUI_SetInterface("flush",NULL);
        GxCore_ThreadDelay(2000);
#endif
        app_update_reboot();

		s_upgade_state = UPGRADE_SUCCESS;
	}
	else
    {
#if NETWORK_SUPPORT
        app_send_msg_exec(GXMSG_NETWORK_START, NULL);
#if (BOX_SERVICE > 0)
        app_send_msg_exec(GXMSG_BOX_ENABLE, (void *)NULL);
#endif
#endif
        GUI_SetProperty("txt_upgrade_process_info1", "string", STR_ID_ERR_OCCUR);
        GUI_SetProperty("txt_upgrade_process_info2", "string", UPDATE_FAILED_OSD_ENGLISH);

        GUI_SetProperty("img_upgrade_process_tip", "state", "show");
        GUI_SetProperty("btn_upgrade_process_ok", "state", "show");

        s_upgade_state = UPGRADE_STOPPED;
    }
}

void usb_dump_ok(void* userdata)
{
	s_update_rate = 100;
	GUI_SetProperty("progbar_upgrade_process", "value", (void*)&s_update_rate);

	if(0 == g_system_shell_ret_status)
	{
		GUI_SetProperty("txt_upgrade_process_info1", "string", STR_DUMP_SUCCESS);
		GUI_SetProperty("txt_upgrade_process_info2", "string", NULL);

		s_upgade_state = UPGRADE_SUCCESS;
	}
	else
	{
		GUI_SetProperty("txt_upgrade_process_info1", "string", STR_ID_ERR_OCCUR);
		GUI_SetProperty("txt_upgrade_process_info2", "string", NULL);

		s_upgade_state = UPGRADE_STOPPED;
	}

	remove_timer(sp_UpgradeTimeOut);
	sp_UpgradeTimeOut = NULL;

	GUI_SetProperty("img_upgrade_process_tip", "state", "show");
	GUI_SetProperty("btn_upgrade_process_ok", "state", "show");
}
#endif

static bool _check_upgrade_file_valid(UpgradePara *ret_para)
{
	bool ret = true;

    if(ret_para->path == NULL)
        return false;

    if(app_upgrade_crc_check(ret_para->path) == 0)
        return false;
#if CA_VERIFY_SUPPORT
    if(app_upgrade_verify_check(ret_para->path) == 0)
        return false;
#endif
#if 0
	else
	{
		if(ret_para->path != NULL)
		{
			GxUpdate_ConfigFlash config;
			GxFileInfo upgrade_file;

			if(app_get_partition_info(ret_para->section, &config) == GXCORE_SUCCESS)
			{
				memset(&upgrade_file, 0, sizeof(GxFileInfo));
				GxCore_GetFileInfo(ret_para->path, &upgrade_file);

				if(upgrade_file.size_by_bytes <= config.size)
				{
					char *filename = NULL;

					filename = strrchr(ret_para->path, '/');
					if(filename != NULL)
					{
						char *dest_name = NULL;
						switch(ret_para->section)
						{
							case UPGRADE_SECT_APP:
								dest_name = DUNM_BIN_USER;
								break;

							default:
								return false;
						}

						if(0 == strcmp(filename + 1, dest_name))
						{
							ret = true;
						}
					}
				}
			}
		}
	}
#endif
	return ret;
}
#if UPGRADE_AUTO_SELECT_SUPPORT
#define  UPGRADE_FILE  "download_ecos.bin"
static bool upgrade_check_name(const char* check_file, const char* play_file)
{
	int check_file_len = 0;
	int play_file_len = 0;
	int base_file_name_len = 0;
	int base_lrc_name_len = 0;
	int i = 0;

	if(NULL == check_file || NULL == play_file)
	{
		return FALSE;
	}

	check_file_len = strlen(check_file);
	play_file_len = strlen(play_file);

	// get play file's name length without suffix
	for(i = play_file_len - 1; i >= 0 ; i--)
	{
		if('.' == play_file[i])
		{	
			break;
		}
	}
	if(0 == i) return FALSE;
	base_file_name_len = i;

	// get lrc file's name length without suffix
	for(i = check_file_len- 1; i >= 0 ; i--)
	{
		if('.' == check_file[i])
		{	
			break;
		}
	}
	if(0 == i) return FALSE;
	base_lrc_name_len = i;
	
	if(base_file_name_len != base_lrc_name_len)
	{
		return FALSE;
	}

	if(check_file_len <= base_file_name_len)
	{
		return FALSE;
	}
	
	if(0 == strncasecmp((const char*)check_file,play_file, base_file_name_len))
	{
		return TRUE;
	}

	return FALSE;
}
 void app_upgrade_file_auto_select(void)
{
#define MAX_PATH_LEN             256

    explorer_para* explorer_upgradefile = NULL;
	static char upgradebuffer[MAX_PATH_LEN];

    HotplugPartitionList* partition_list = NULL;
    GxDirent* ent = NULL;
    int count = 0;
    int i = 0;

    partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    	if(partition_list != NULL)
        {

	explorer_upgradefile = explorer_opendir(partition_list->partition[0].partition_entry, UPGRADE_SUFFIX);
	if(NULL == explorer_upgradefile) return ;


	count = explorer_upgradefile->nents;
	for(i = 0; i < count; i++)
	{
		ent = explorer_upgradefile->ents + i;
		if(NULL == ent)
		{
			explorer_closedir(explorer_upgradefile);
			return ;
		}

		if(GX_FILE_REGULAR == ent->ftype)
		{

			if(upgrade_check_name(ent->fname, UPGRADE_FILE))
            {
                printf("ent->fname = %s  \n",ent->fname);

                sprintf(upgradebuffer,"%s/%s",partition_list->partition[0].partition_entry,ent->fname);
                s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = upgradebuffer;
                app_system_set_item_property(ITEM_UPGRADE_PATH, &s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty);

                explorer_closedir(explorer_upgradefile);
                return ;
            }
		}
	}

	explorer_closedir(explorer_upgradefile);

        }   
        	return ;

}
#endif
static void app_upgrade_para_get(UpgradePara *ret_para)
{
    ret_para->type= s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.sel;
    ret_para->section= s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty.itemPropertyCmb.sel;

    if((strcmp(s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string, STR_ID_PRESS_OK) == 0)
            || (strcmp(s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string, "") == 0))
    {
        ret_para->path = NULL;
    }
    else
    {
        ret_para->path = s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string;
    }
}

void app_upgrade_clear_path()
{	
	 UpgradePara upgrade_para = {0};
	app_upgrade_para_get(&upgrade_para);
	  if((upgrade_para.path == NULL)
                        || (GxCore_FileExists(upgrade_para.path) == GXCORE_FILE_UNEXIST))
	  {
		s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
		if(GUI_CheckDialog("wnd_pop_tip") != GXCORE_SUCCESS && GUI_CheckDialog("wnd_file_list") != GXCORE_SUCCESS) 
		{
			app_system_set_item_property(ITEM_UPGRADE_PATH, &s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty);
		}
		 		
	  }
}

static int app_upgrade_exit_callback(ExitType exit_type)
{
	int ret = 1;

	if(s_file_path_bak != NULL)
	{
		GxCore_Free(s_file_path_bak);
		s_file_path_bak = NULL;
	}

	s_file_path =NULL;

	return ret;
}

static int app_upgrade_type_change_callback(int sel)
{
	s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
	app_system_set_item_property(ITEM_UPGRADE_PATH, &s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty);

    s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty.itemPropertyCmb.sel = 0;
	app_system_set_item_property(ITEM_UPGRADE_SECT, &(s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty));

	if(sel == UPGRADE_TYPE_U2S)
	{
        app_system_set_item_state_update(ITEM_UPGRADE_SECT, ITEM_NORMAL);
	}
#if (UPGRADE_DUMP_SUPPORT == 1)
    else if(sel == UPGRADE_TYPE_DUMP)
    {
        app_system_set_item_state_update(ITEM_UPGRADE_SECT, ITEM_DISABLE);
    }
#endif
#if (UPGRADE_P2P_SUPPORT == 1)
	else if(sel == UPGRADE_TYPE_S2S)
	{
		app_system_set_item_state_update(ITEM_UPGRADE_PATH, ITEM_DISABLE);
	}
#endif

	s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.sel = sel;
	if(s_file_path_bak != NULL)
	{
		GxCore_Free(s_file_path_bak);
		s_file_path_bak = NULL;
	}

	s_file_path =NULL;


	return EVENT_TRANSFER_STOP;
}

static int app_upgrade_section_change_callback(int sel)
{
	s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty.itemPropertyCmb.sel = sel;
	return EVENT_TRANSFER_STOP;
}

static int app_upgrade_path_button_press_callback(unsigned short key)
{
    if(key == STBK_OK)
    {
        WndStatus get_path_ret = WND_CANCLE;
        UpgradeTypeSel type = s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.sel;

        if(check_usb_status() == false)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_INSERT_USB;
            pop.mode = POP_MODE_UNBLOCK;
            popdlg_create(&pop);
        }
        else
        {
            FileListParam file_para;
            memset(&file_para, 0, sizeof(file_para));

            file_para.cur_path = s_file_path_bak;
            file_para.dest_path = &s_file_path;
            file_para.suffix = UPGRADE_SUFFIX;
            if(type == UPGRADE_TYPE_U2S)
            {
                file_para.dest_mode = DEST_MODE_FILE;
                get_path_ret = app_get_file_path_dlg(&file_para);
            }
            else if(type == UPGRADE_TYPE_DUMP)
            {
                file_para.dest_mode = DEST_MODE_DIR;
                get_path_ret = app_get_file_path_dlg(&file_para);
            }

            if(get_path_ret == WND_OK)
            {
                if(s_file_path != NULL)
                {
                    int str_len = 0;

                    s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = s_file_path;
                    app_system_set_item_property(ITEM_UPGRADE_PATH, &s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty);

                    ////bak////
                    if(s_file_path_bak != NULL)
                    {
                        GxCore_Free(s_file_path_bak);
                        s_file_path_bak = NULL;
                    }

                    str_len = strlen(s_file_path);
                    s_file_path_bak = (char *)GxCore_Malloc(str_len + 1);
                    if(s_file_path_bak != NULL)
                    {
                        memcpy(s_file_path_bak, s_file_path, str_len);
                        s_file_path_bak[str_len] = '\0';
                    }
                }
            }
        }
    }
    return EVENT_TRANSFER_KEEPON;
}

static int app_upgrade_exec_button_press_callback(unsigned short key)
{
    UpgradePara upgrade_para = {0};
    PopDlg  pop = {0};
    status_t ret = GXCORE_ERROR;
#ifdef LINUX_OS
    char cmd_buf[512] = {0};
#endif

    if(key == STBK_OK)
    {
        app_upgrade_para_get(&upgrade_para);
        if((upgrade_para.type == UPGRADE_TYPE_U2S) || (upgrade_para.type == UPGRADE_TYPE_DUMP))
        {
            if(check_usb_status() == false)
            {
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_OK;
                pop.str = STR_ID_NO_DEVICE;
                pop.mode = POP_MODE_UNBLOCK;
                popdlg_create(&pop);
            }
            else
            {
                if((upgrade_para.path == NULL)
                        || (GxCore_FileExists(upgrade_para.path) == GXCORE_FILE_UNEXIST))
                {
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_OK;
                    pop.str = STR_ID_ERR_PATH;
                    pop.mode = POP_MODE_UNBLOCK;
                    popdlg_create(&pop);
                }
                else
                {
                    if(upgrade_para.type == UPGRADE_TYPE_DUMP) // dump
                    {
                        char *partiton = get_usb_url_partition(upgrade_para.path);
                        if(partiton != NULL)
                        {
#ifdef LINUX_OS
                            if(get_usb_partition_space(partiton) <= 4*FLASH_BIN_LENGTH) //reserve 16M
                            {
                                memset(&pop, 0, sizeof(PopDlg));
                                pop.type = POP_TYPE_OK;
                                pop.str = STR_ID_NO_SPACE;
                                pop.mode = POP_MODE_UNBLOCK;
                                popdlg_create(&pop);
                            }
                            else
                            {
                                if(UPGRADE_SECT_ALL == upgrade_para.section)
                                {
                                    sprintf(cmd_buf, "usb_dump all \"%s/%s\"", upgrade_para.path, DUNM_BIN_ALL);
                                }
                                else //always "ALL"
                                {
                                    sprintf(cmd_buf, "usb_dump all \"%s/%s\"", upgrade_para.path, DUNM_BIN_ALL);
                                }
                                s_step_num = 0;
                                s_update_rate = 0;

                                g_system_shell_ret_status = 0;
                                system_shell(cmd_buf, 600000, usb_update_proc, usb_dump_ok, NULL);

                                ret = GXCORE_SUCCESS;
                            }
#else
                            if(get_usb_partition_space(partiton) <= FLASH_BIN_LENGTH)
                            {
                                memset(&pop, 0, sizeof(PopDlg));
                                pop.type = POP_TYPE_OK;
                                pop.str = STR_ID_NO_SPACE;
                                pop.mode = POP_MODE_UNBLOCK;
                                popdlg_create(&pop);
                            }
                            else
                            {
                                ret = app_usb_upgrade_config(&upgrade_para);
                            }
#endif
                            GxCore_Free(partiton);
                            partiton = NULL;
                        }
                    }
                    else  //update
                    {
                        if(_check_upgrade_file_valid(&upgrade_para) == true)
                        {
                            // for demux control, off the lnb power
                            AppFrontend_PolarState Polar = FRONTEND_POLAR_OFF;
                            app_ioctl(0, FRONTEND_POLAR_SET, &Polar);
                            app_ioctl(1, FRONTEND_POLAR_SET, &Polar);
#if CASCAM_SUPPORT
                            {
                                CASCAMDemuxFlagClass flag;
                                flag.Flags = DEMUX_ALL;
                                CASCAM_Release_Demux(&flag);
                            }
#endif

#if NETWORK_SUPPORT
                           // app_send_msg_exec(GXMSG_NETWORK_STOP, NULL);
#endif

#ifdef ECOS_OS
                            ret = app_usb_upgrade_config(&upgrade_para);
#endif

#ifdef LINUX_OS
#if (BOX_SERVICE > 0)
                            GxMsgProperty_BoxRecordControl box_rec = {0};
                            box_rec.option = BOX_RECORD_ALL_STOP;
                            app_send_msg_exec(GXMSG_BOX_RECORD_CONTROL, (void *)(&box_rec));
                            app_send_msg_exec(GXMSG_BOX_DISABLE, (void *)NULL);
#endif

                            if(UPGRADE_SECT_ALL == upgrade_para.section)
                            {
                                sprintf(cmd_buf, "usb_update all \"%s\"", upgrade_para.path);
                            }
                            else if(UPGRADE_SECT_APP == upgrade_para.section)
                            {
                                sprintf(cmd_buf, "usb_update app \"%s\"", upgrade_para.path);
                            }
#if UPGRADE_DATA_SUPPORT
                            else if(UPGRADE_SECT_DATA == upgrade_para.section)
                            {
                                sprintf(cmd_buf, "usb_update data \"%s\"", upgrade_para.path);
                            }
#endif
                            else
                            {
                                sprintf(cmd_buf, "usb_update all \"%s\"", upgrade_para.path);
                            }

                            s_step_num = 0;
                            s_update_rate = 0;
                            system_shell(cmd_buf, 600000, usb_update_proc, usb_update_ok, NULL);

                            ret = GXCORE_SUCCESS;
#endif
                        }
                    }
                    if(ret == GXCORE_ERROR)
                    {
#ifdef ECOS_OS
                        if(s_temp_file != NULL)
                        {
                            if(GxCore_FileExists(s_temp_file) == GXCORE_FILE_EXIST)
                                GxCore_FileDelete(s_temp_file);
                            GxCore_Free(s_temp_file);
                            s_temp_file = NULL;
                        }
#endif
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_OK;
                        pop.str = STR_ID_ERR_OCCUR;
                        pop.mode = POP_MODE_UNBLOCK;
                        popdlg_create(&pop);
                    }
                }
            }
        }
#if (UPGRADE_P2P_SUPPORT == 1)
        else if((upgrade_para.type == UPGRADE_TYPE_S2S))
        {
			// TODO: 20160303
            print_enable = 0;
            ret = app_serial_upgrade_config(&upgrade_para);
        }
#endif

        if(ret == GXCORE_SUCCESS)
        {
            GUI_CreateDialog("wnd_upgrade_process");

#ifndef LINUX_OS
            app_send_msg_exec(GXMSG_UPDATE_START, NULL);
#endif
        }
    }
    return EVENT_TRANSFER_KEEPON;
}

static void app_upgrade_type_item_init(void)
{
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemTitle = STR_ID_UPGRADE_TYPE;
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemType = ITEM_CHOICE;
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemCallback.cmbCallback.CmbChange= app_upgrade_type_change_callback;
#if (UPGRADE_P2P_SUPPORT == 1)
#if (UPGRADE_DUMP_SUPPORT == 1)
    s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.content= "[USB Upgrade,STB to STB,Dump]";
#else
    s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.content= "[USB Upgrade,STB to STB]";
#endif
#else
#if (UPGRADE_DUMP_SUPPORT == 1)
    s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.content= "[USB Upgrade,Dump]";
#else
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemCallback.cmbCallback.CmbChange= NULL;
    s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.content= "[USB Upgrade]";
#endif
#endif
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.sel = UPGRADE_TYPE_U2S;
	s_upgrade_item[ITEM_UPGRADE_TYPE].itemStatus = ITEM_NORMAL;
}

static void app_upgrade_sect_item_init(void)
{
	s_upgrade_item[ITEM_UPGRADE_SECT].itemTitle = STR_ID_SECTION;
	s_upgrade_item[ITEM_UPGRADE_SECT].itemType = ITEM_CHOICE;
#if UPGRADE_DATA_SUPPORT
	s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty.itemPropertyCmb.content= "[All,App,User]";
#else
	s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty.itemPropertyCmb.content= "[All,App]";
#endif
	s_upgrade_item[ITEM_UPGRADE_SECT].itemProperty.itemPropertyCmb.sel = UPGRADE_SECT_ALL;
	s_upgrade_item[ITEM_UPGRADE_SECT].itemCallback.cmbCallback.CmbChange= app_upgrade_section_change_callback;
	s_upgrade_item[ITEM_UPGRADE_SECT].itemStatus = ITEM_NORMAL;
}

static void app_upgrade_path_item_init(void)
{
	s_upgrade_item[ITEM_UPGRADE_PATH].itemTitle = STR_ID_FILE_PATH;
	s_upgrade_item[ITEM_UPGRADE_PATH].itemType = ITEM_PUSH;
	s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	s_upgrade_item[ITEM_UPGRADE_PATH].itemCallback.btnCallback.BtnPress= app_upgrade_path_button_press_callback;
	s_upgrade_item[ITEM_UPGRADE_PATH].itemStatus = ITEM_NORMAL;
}

static void app_upgrade_exec_item_init(void)
{
	s_upgrade_item[ITEM_UPGRADE_EXEC].itemTitle = STR_ID_START;
	s_upgrade_item[ITEM_UPGRADE_EXEC].itemType = ITEM_PUSH;
	s_upgrade_item[ITEM_UPGRADE_EXEC].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	s_upgrade_item[ITEM_UPGRADE_EXEC].itemCallback.btnCallback.BtnPress= app_upgrade_exec_button_press_callback;
	s_upgrade_item[ITEM_UPGRADE_EXEC].itemStatus = ITEM_NORMAL;
}

void app_upgrade_menu_exec(void)
{
	s_file_path =NULL;
	s_file_path_bak = NULL;

	memset(&s_upgrade_opt, 0 ,sizeof(s_upgrade_opt));

	s_upgrade_opt.menuTitle = STR_ID_FIRMWARE_UPGRADE;
	s_upgrade_opt.titleImage = "s_title_left_utility.bmp";
	s_upgrade_opt.itemNum = ITEM_UPGRADE_TOTAL;
	s_upgrade_opt.timeDisplay = TIP_HIDE;
	s_upgrade_opt.item = s_upgrade_item;

	app_upgrade_type_item_init();
	app_upgrade_sect_item_init();
	app_upgrade_path_item_init();
	app_upgrade_exec_item_init();

	s_upgrade_opt.exit = app_upgrade_exit_callback;

	app_system_set_create(&s_upgrade_opt);
        app_upgrade_file_auto_select();

}

UpgradeTypeSel app_get_upgrade_type(void)
{
	return s_upgrade_item[ITEM_UPGRADE_TYPE].itemProperty.itemPropertyCmb.sel;
}

#define PROGBAR__________________

static int _timer_upgrade_timeout(void *userdata)
{
	remove_timer(sp_UpgradeTimeOut);
	sp_UpgradeTimeOut = NULL;

	s_upgade_state = UPGRADE_STOPPED;
	//app_send_msg_exec(GXMSG_UPDATE_STOP,NULL);
	GUI_SetProperty("txt_upgrade_process_info1", "string", STR_TIMEOUT);

	if(app_get_upgrade_type() == UPGRADE_TYPE_U2S
		||app_get_upgrade_type() == UPGRADE_TYPE_DUMP)
	{
		GUI_SetProperty("txt_upgrade_process_info2", "string", NULL);
	}
	#if (UPGRADE_P2P_SUPPORT == 1)
	else
	{
		GUI_SetProperty("txt_upgrade_process_info2", "string", STR_REBOOT_NOW);
	}
	#endif
	GUI_SetProperty("img_upgrade_process_tip", "state", "show");
	GUI_SetProperty("btn_upgrade_process_ok", "state", "show");

	return 0;
}

SIGNAL_HANDLER int app_upgrade_progcess_create(GuiWidget * widget, void *usrdata)
{
    int rate = 1;
    time_t timeout = 80000;

    GUI_SetProperty("progbar_upgrade_process", "value", (void*)&rate);
    UpgradeTypeSel type = app_get_upgrade_type();
    if(type == UPGRADE_TYPE_DUMP)
    {
        GUI_SetProperty("text_upgrade_process_tip", "string", STR_ID_DUMP);
    }
    else
    {
        GUI_SetProperty("text_upgrade_process_tip", "string", STR_UPGRADE);
#if (UPGRADE_P2P_SUPPORT == 1)
        if(type == UPGRADE_TYPE_S2S)
        {
            timeout = 60000;
        }
#endif
    }

    GUI_SetProperty("txt_upgrade_process_info1", "string", STR_ID_WAITING);
    GUI_SetProperty("txt_upgrade_process_info2", "string", STR_DONT_CUT_PWER);

    s_upgade_state = UPGRADE_START;

    if (reset_timer(sp_UpgradeTimeOut) != 0)
    {
        sp_UpgradeTimeOut = create_timer(_timer_upgrade_timeout, timeout, NULL, TIMER_ONCE);
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_upgrade_progcess_destroy(GuiWidget *widget, void *usrdata)
{
	remove_timer(sp_UpgradeTimeOut);
	sp_UpgradeTimeOut = NULL;
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_upgrade_progcess_service(GuiWidget * widget, void *usrdata)
{
    GUI_Event *event = NULL;
    GxMessage *msg = NULL;
    GxMsgProperty_UpdateStatus* p;
    uint32_t rate;
    UpgradeTypeSel update_type;

    event = (GUI_Event *)usrdata;
    msg = (GxMessage *)event->msg.service_msg;

    update_type = app_get_upgrade_type();

    if(event->type == GUI_SERVICE_MSG)
    {
        if(msg->msg_id == GXMSG_UPDATE_STATUS)
        {
            p = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_UpdateStatus);
            if(p->type == GXUPDATE_STATUS_PRERCENT)
            {
                rate = (p->percent)/100;
#if (UPGRADE_P2P_SUPPORT == 1)
                if(update_type == UPGRADE_TYPE_S2S)
                    rate += 3;
#endif
                GUI_SetProperty("progbar_upgrade_process", "value", (void*)&rate);
                reset_timer(sp_UpgradeTimeOut);
            }
            else if(p->type == GXUPDATE_STATUS_ERROR)
            {
#ifdef ECOS_OS
                if(s_temp_file != NULL)
                {
                    if(GxCore_FileExists(s_temp_file) == GXCORE_FILE_EXIST)
                        GxCore_FileDelete(s_temp_file);
                    GxCore_Free(s_temp_file);
                    s_temp_file = NULL;
                }
#endif
                if(p->error < GXUPDATE_OK)
                {
                    PopDlg  pop;
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_OK;
                    pop.str = STR_ID_ERR_OCCUR;
                    pop.mode = POP_MODE_UNBLOCK;
                    popdlg_create(&pop);
                }
                else if(p->error == GXUPDATE_OK)
                {
                    rate = 100;
                    GUI_SetProperty("progbar_upgrade_process", "value", (void*)&rate);

                    if(update_type == UPGRADE_TYPE_DUMP)
                    {
                        GUI_SetProperty("txt_upgrade_process_info1", "string", STR_DUMP_SUCCESS);
                        GUI_SetProperty("txt_upgrade_process_info2", "string", NULL);
                    }
#if (UPGRADE_P2P_SUPPORT == 1)
                    else if(update_type == UPGRADE_TYPE_S2S)
                    {
                        GUI_SetProperty("txt_upgrade_process_info1", "string", STR_UPGRADE_SUCCESS);
                        GUI_SetProperty("txt_upgrade_process_info2", "string", NULL);
                    }
#endif
                    else
                    {
                        GUI_SetProperty("txt_upgrade_process_info1", "string", STR_UPGRADE_SUCCESS);
                        GUI_SetProperty("txt_upgrade_process_info2", "string", STR_REBOOT_NOW);
                    }

                    s_upgade_state = UPGRADE_SUCCESS;
                    remove_timer(sp_UpgradeTimeOut);
                    sp_UpgradeTimeOut = NULL;

#if 1 //auto reboot after upgrade...
                    if(update_type == UPGRADE_TYPE_U2S)
                    {
                        GUI_SetInterface("flush",NULL);
                        GxCore_ThreadDelay(2000);
                        app_update_reboot();
                    }
#endif
                }
                else if(p->error == GXUPDATE_STOP)
                {
                    s_upgade_state = UPGRADE_STOPPED;

                    remove_timer(sp_UpgradeTimeOut);
                    sp_UpgradeTimeOut = NULL;
                }
                GUI_SetProperty("img_upgrade_process_tip", "state", "show");
                GUI_SetProperty("btn_upgrade_process_ok", "state", "show");
            }

            return EVENT_TRANSFER_STOP;
        }
    }

    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_upgrade_progcess_keypress(GuiWidget* widgetname, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    UpgradeTypeSel update_type;

    update_type = app_get_upgrade_type();

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case STBK_OK:
                    if(s_upgade_state == UPGRADE_STOPPED)
                    {
                        if(update_type == UPGRADE_TYPE_U2S || update_type == UPGRADE_TYPE_DUMP)
                        {
                            GUI_EndDialog("wnd_upgrade_process");
                            app_system_set_focus_item(ITEM_UPGRADE_EXEC);
                        }
#if (UPGRADE_P2P_SUPPORT == 1)
                        else
                        {
                            //reboot
                            app_update_reboot();
                        }
#endif
                    }
                    else if(s_upgade_state == UPGRADE_SUCCESS)
                    {
                        if(update_type == UPGRADE_TYPE_U2S)
                        {
                            //reboot
                            app_update_reboot();
                        }
                        else
                        {
                            GUI_EndDialog("wnd_upgrade_process");
                            app_system_set_focus_item(ITEM_UPGRADE_EXEC);

#if (UPGRADE_P2P_SUPPORT == 1)
                            if(update_type == UPGRADE_TYPE_S2S)
                            {
                                // TODO: 20160302
                            }
#endif
			s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
			app_system_set_item_property(ITEM_UPGRADE_PATH, &s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty);
		     }
                    }
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

