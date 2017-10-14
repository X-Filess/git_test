#include "app_msg.h"
#include "app_send_msg.h"
#include "app_ota_protocol.h"
#include "app_ota_upgrade.h"
#include "gxoem.h"
#include "gui_core.h"
#include "app_default_params.h"
#ifdef LINUX_OS
#include <zlib.h>
#include "youtube_tools.h"
#else
#include <linux/zlib.h>
#endif
#include "app_config.h"
#include "app_str_id.h"
#include "app.h"
#include "app_module.h"
#include "app_ota_config.h"
#include <fcntl.h>

uint32_t g_nFlagOta = 0;
uint8_t g_chFlagEntryFlash = 0;

#if (OTA_SUPPORT > 0)

#define OTA_UPGRADE_PATH "/tmp/ota_update"

GxOTA_ProtoOTAData_t data_for_upgrade[10];
uint8_t g_chBissUpdate = 0;
uint8_t g_chDefaultDataUpdate = 0;
uint8_t g_chLogoUpdate = 0;
handle_t gx_ota_default_file = 0;
extern uint8_t *sp_SectionData;
// add in 20151203
extern GxHwMallocObj hw_malloc_block;

extern OtaDsmInfo_t OtaDsmInfo;
extern void app_epg_disable(void);
extern void app_ota_process_update(int percent);
extern void app_ota_message_update(char *message1, char *message2);
extern int app_ota_backup_oem(void);
extern int app_ota_backup_partition(struct partition_info * partition);
extern void app_ota_backup_fe(void);

static GxPartitionTable    table;
static uint32_t 	need_update_all = 0;
static char *		pcharName = NULL;

#define OEM_UPDATE_STATUS                       ("update_status")
#define OEM_UPDATE_SECTION                      ("update") 
#define OEM_UPDATE_FLAG                         ("flag")

#define SUB_FUNC
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
	unsigned char *part;
	unsigned char *crc32_table = data + FLASH_PARTITION_ADDR_EX;
    unsigned char *part_version = data + FLASH_PARTITION_VERSION_ADDR;
    if (len < FLASH_PARTITION_SIZE)
    {
        return -1;
    }

//	memcpy(tbl, data, FLASH_PARTITION_SIZE);  // read table

    tbl->magic = char2int(data);
    tbl->count = data[4];
    tbl->crc_verify = char2int(data + FLASH_PARTITION_SIZE - 4);
    tbl->version = data[FLASH_PARTITION_SIZE - 5];
    tbl->crc32_enable = data[FLASH_PARTITION_SIZE - 6];
    tbl->write_protect = data[FLASH_PARTITION_SIZE - 7];

    if (tbl->count > FLASH_PARTITION_NUM)
    {
        printf("%s,%d partition_num=%d error\n", __func__, __LINE__, tbl->count);

        return -1;
    }

    part = data + 5;
    for (i = 0; i < tbl->count; i++, part += FLASH_PARTITION_HEAD_SIZE)
    {
        memcpy(tbl->tables[i].name, part, FLASH_PARTITION_NAME_SIZE);
        tbl->tables[i].total_size       = char2int(part +  8);
        tbl->tables[i].used_size        = char2int(part + 12);
        tbl->tables[i].start_addr       = char2int(part + 16);
        tbl->tables[i].file_system_type = part[20];
        tbl->tables[i].mode             = part[21] & 0x7F;
        tbl->tables[i].crc32_enable     = part[21] >> 7;
        if (tbl->version >= PARTITION_VERSION)
        {
            tbl->tables[i].id            = part[22] & 0x7F;
            tbl->tables[i].write_protect = (part[22] >> 7) & 0x01;
        }
        else
        {
            tbl->tables[i].id = i;
            tbl->tables[i].write_protect = 0;
        }
        tbl->tables[i].update_tags = part[23] & 0x03;

        tbl->tables[i].crc_verify = char2int(crc32_table + i * 4);
        tbl->tables[i].version = char2int16(part_version + i * 2);
    }

    return 0;
}

#ifdef LINUX_OS
static int app_write_upgrade_para(char *buf, size_t sizes)
{
    char *p ;
    FILE *fp = NULL;
    long count = 0;

    p = buf;

    fp = fopen(OTA_UPGRADE_PATH, "w");
    if (NULL == fp)
    {
        return -1;
    }

    count = fwrite(p, 1, sizes, fp);
    if (count != sizes)
    {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    return 0;
}
#endif

static int app_ota_select_segment(void)
{
	//uint32_t box_sel;
    uint32_t value;
    int ret = 0;
    
    ret = app_ota_dsmcc_get_oui(&value);
    if(ret)
    {
        printf("\nMENU OTA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    OtaDsmInfo.m_nDsiOUI = value;

    ret = app_ota_dsmcc_get_hw_model(&value);
    if(ret)
    {
        printf("\nMENU OTA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
	OtaDsmInfo.m_nDsiHwModule	= (unsigned short)value;

    ret = app_ota_dsmcc_get_hw_version(&value);
    if(ret)
    {
        printf("\nMENU OTA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
	OtaDsmInfo.m_nDsiHwVersion	= (unsigned short)value;

    ret = app_ota_dsmcc_get_sw_model(&value);
    if(ret)
    {
        printf("\nMENU OTA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
	OtaDsmInfo.m_nDsiSwModule	= (unsigned short)value;

    ret = app_ota_dsmcc_get_sw_version(&value);
    if(ret)
    {
        printf("\nMENU OTA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
	OtaDsmInfo.m_nDsiSwVersion	= (unsigned short)value;
    return 0;
#if 0
	OtaDsmInfo.m_nDsiOUI		= 0x333333;//0x789ABC;
	OtaDsmInfo.m_nDsiHwModule	= 0x3333;//0x1234;
	OtaDsmInfo.m_nDsiHwVersion	= 0x3333;//0x5678;
	OtaDsmInfo.m_nDsiSwModule	= 0x3330;//0xADCB;
	OtaDsmInfo.m_nDsiSwVersion	= 0x1000;

	GUI_GetProperty("cmbox_ota_segment", "select", &box_sel);
	switch(box_sel)
	{
		case 0: // Segment for ALL
			need_update_all = 0xFF00;
			OtaDsmInfo.m_nDsiSwModule 	= 0x3330;//0xADCB;
			break;
		case 1:
			g_chDefaultDataUpdate = 1;
			OtaDsmInfo.m_nDsiSwModule 	= 0xDCBA;
			break;

		default:
			break;
	}
#endif
}


void app_ota_get_segment(void)
{
#ifdef ECOS_OS
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t find_part = 0;
    uint32_t need_update = 0;
    uint32_t nNeedUpdate[10] = {0};
    struct partition_info *param = NULL;
#else
    char cmd_buf[512] = {0};
#endif

    //struct partition_info *partition = NULL;
    GxPartitionTable    table_from_OTA;
    uint8_t     	*pUpdateData = NULL;

    memset(&table_from_OTA, 0, sizeof(GxPartitionTable));
    memset(&table, 0, sizeof(GxPartitionTable));
    memset(data_for_upgrade, 0, 10 * sizeof(GxOTA_ProtoOTAData_t));

    //Get the Partition table from Flash
    if (GxOem_PartitionTableGet(&table) != 0)
    {
        GxOtaPrintf("GxOem_PartitionTableGet  old Error!\n");
        return;
    }
    if (table.count == 0)
    {
        GxOtaPrintf("GxOem_PartitionTableGet  Old  Error!\n");
    }

    //Get the Partition table from the OTA Bin file
    pUpdateData = sp_SectionData;
    pUpdateData += 0x10000;//find the Table!
    if ((char2int(pUpdateData)) == PARTITION_MAGIC)
    {
        if (flash_table_read(&table_from_OTA, pUpdateData, FLASH_PARTITION_SIZE) != 0)
        {
            GxOtaPrintf("GxOem_PartitionTableGet  New  Error!\n");
            return;
        }
    }
    else
    {
        GxOtaPrintf("Cant find new partition table!\n");
        return;
    }
#ifdef ECOS_OS
    //Just print the Partition information
    GxOtaPrintf("Old partition:");
    GxCore_PartitionPrint(&table);
    GxOtaPrintf("New partition:");
    GxCore_PartitionPrint(&table_from_OTA);

    for (i = 0; i < table_from_OTA.count; i++)
    {
        find_part = 0;
        for (j = 0; j < table.count; j++)
        {
            if (strcmp(table_from_OTA.tables[i].name, table.tables[j].name) == 0)
            {
                find_part = 1;//find the partition
                GxOtaPrintf("Find partition: %s!\n", table_from_OTA.tables[i].name);
                break;
            }
        }

        if (0xFF00 != need_update_all) // Not ALL Segment
        {
            if (strcmp(table_from_OTA.tables[i].name, pcharName) != 0)
            {
                //Those partitions cann't be Update!
                GxOtaPrintf("filter part: %s\n", table_from_OTA.tables[i].name);
                continue;
            }
        }

        if (find_part)
        {
            //The Dst Addr is the old partition start Addr
            data_for_upgrade[need_update].dst = table.tables[j].start_addr;
            //The Src Addr is the Buffer Addr add the new Partition start addr
            data_for_upgrade[need_update].src = table_from_OTA.tables[i].start_addr + sp_SectionData;
            //Compare the Space of Partition for Update
            if (table_from_OTA.tables[i].total_size != table.tables[j].total_size)
            {
                GxOtaPrintf("New partition size is different from old partition total size ->%s!\n", \
                            table_from_OTA.tables[i].name);
                continue;
            }
            //Please ust the new partition total size for Update
            if (strcmp(table_from_OTA.tables[i].name, "LOGO") == 0)
            {
                data_for_upgrade[need_update].size = table_from_OTA.tables[i].total_size - 0x1000;    // 4K Private data
            }
            else
            {
                data_for_upgrade[need_update].size = table_from_OTA.tables[i].total_size;
            }

            GxOtaPrintf("need update : %s!data[i].src = 0x%08x| data[i].dst  = 0x%08x |data.size = %d, crc = %x\n", \
                        table_from_OTA.tables[i].name, data_for_upgrade[need_update].src, data_for_upgrade[need_update].dst, \
                        data_for_upgrade[need_update].size, \
                        table_from_OTA.tables[i].crc_verify);

            //Write the new partition to Flash!
            table.tables[i].used_size = table_from_OTA.tables[i].used_size;
            if (table.tables[i].crc32_enable == 1 && table.crc32_enable == 1)
            {
                table.tables[i].crc_verify = table_from_OTA.tables[i].crc_verify;
            }
            nNeedUpdate[need_update] = i;
            need_update ++;
        }
    }

    if (need_update > 1) //ALL
    {
        //param = &(table.tables[0]);
        //param->start_addr = 0x0;
        //param->total_size = OTA_FILE_TOTAL_SIZE;
        data_for_upgrade[0].src = sp_SectionData;
        data_for_upgrade[0].size = OTA_FILE_TOTAL_SIZE;
    }
    else//Other
    {
        param = &(table.tables[nNeedUpdate[0]]);
    }

    for (i = 0; i < need_update; i++)
    {
        param = &(table.tables[i]);
        printf("[%s]star_addr:0x%x size:0x%x\n", param->name, param->start_addr, param->total_size);
        GxCore_PartitionWrite(param, (void *)data_for_upgrade[i].src, data_for_upgrade[i].size, 0);
    }
    //GxCore_PartitionWrite(param,(void*)data_for_upgrade[0].src,data_for_upgrade[0].size);
    GxCore_PartitionTableSave(&table);
#else
    if (0 == app_write_upgrade_para((char *)data_for_upgrade[0].src, data_for_upgrade[0].size))
    {
        sprintf(cmd_buf, "usb_update all \"%s\"", OTA_UPGRADE_PATH);
        system_shell(cmd_buf, 600000, NULL, NULL, NULL);
    }
#endif

}

void app_ota_logo_update(void)
{
    uint32_t j = 0;
    struct partition_info *param = NULL;

    memset(&table, 0, sizeof(GxPartitionTable));
    memset(data_for_upgrade, 0, 10 * sizeof(GxOTA_ProtoOTAData_t));

    //Get the Partition table from Flash
    if (GxOem_PartitionTableGet(&table) != 0)
    {
        GxOtaPrintf("GxOem_PartitionTableGet  old Error!\n");
        return;
    }
    if (table.count == 0)
    {
        GxOtaPrintf("GxOem_PartitionTableGet  Old  Error!\n");
    }

    for (j = 0; j < table.count; j++)
    {
        if (strcmp("LOGO", table.tables[j].name) == 0)
        {
            GxOtaPrintf("Find partition: %s!\n", table.tables[j].name);
            break;
        }
    }

    //The Dst Addr is the old partition start Addr
    data_for_upgrade[0].dst = table.tables[j].start_addr;
    //The Src Addr is the Buffer Addr add the new Partition start addr
    data_for_upgrade[0].src = sp_SectionData;
    //Compare the Space of Partition for Update
    if (OtaDsmInfo.m_nDsiGroupSize > (table.tables[j].total_size - 0x1000))
    {
        GxOtaPrintf("New partition size is error old partition total size ->%s!\n", \
                    table.tables[j].name);
        return;
    }
    //Please ust the new partition total size for Update
    data_for_upgrade[0].size = table.tables[j].total_size - 0x1000;// 4K Private data

    GxOtaPrintf("need update : %s!data[i].src = 0x%08x| data[i].dst  = 0x%08x |data.size = %d, crc = %x\n", \
                table.tables[j].name, data_for_upgrade[0].src, data_for_upgrade[0].dst, \
                data_for_upgrade[0].size, \
                table.tables[j].crc_verify);

    //Write the new partition to Flash!
    table.tables[j].used_size = OtaDsmInfo.m_nDsiGroupSize;
    if (table.tables[j].crc32_enable == 1 && table.crc32_enable == 1)
    {
        table.tables[j].crc_verify = crc32(0, sp_SectionData, OtaDsmInfo.m_nDsiGroupSize);
    }

    param = &(table.tables[j]);
#ifdef ECOS_OS
    GxCore_PartitionWrite(param, (void *)data_for_upgrade[0].src, data_for_upgrade[0].size, 0);
    GxCore_PartitionTableSave(&table);
#endif
}

void app_ota_defaultdata_update(void)
{
    handle_t ret = GXCORE_FILE_UNEXIST;

    if (ret == GXCORE_FILE_UNEXIST)
    {
        gx_ota_default_file = GxCore_Open(OTA_DEFAULT_DATA_FILE_PATH, "w+");
    }
    else
    {
        gx_ota_default_file = GxCore_Open(OTA_DEFAULT_DATA_FILE_PATH, "w+");
    }
    if (gx_ota_default_file < 0)
    {
        GxOtaPrintf("--create ota Default file err!\n");
        return;
    }
    GxCore_Write(gx_ota_default_file, sp_SectionData, OtaDsmInfo.m_nDsiGroupSize, 1);
    GxCore_Close(gx_ota_default_file);

    GxBus_PmLoadDefault(SYS_MAX_SAT, SYS_MAX_TP, SYS_MAX_PROG, (uint8_t *)OTA_DEFAULT_DATA_FILE_PATH);
    GxCore_FileDelete(OTA_DEFAULT_DATA_FILE_PATH);
}

int app_ota_defaultdata_flush(unsigned char *data, unsigned int data_size)
{
	handle_t ret = GXCORE_FILE_UNEXIST;
    if((NULL == data) || (0 == data_size))
    {
        printf("\nMENU OTA, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

	if(ret == GXCORE_FILE_UNEXIST)
	{
		gx_ota_default_file = GxCore_Open(OTA_DEFAULT_DATA_FILE_PATH, "w+");
	}
	else
	{
		gx_ota_default_file = GxCore_Open(OTA_DEFAULT_DATA_FILE_PATH,"w+");
	}
	if(gx_ota_default_file<0)
	{
		GxOtaPrintf("--create ota Default file err!\n");
		return -1;
	}
	GxCore_Write(gx_ota_default_file, data, data_size, 1);
	GxCore_Close(gx_ota_default_file);

	GxBus_PmLoadDefault(SYS_MAX_SAT, SYS_MAX_TP, SYS_MAX_PROG, (uint8_t*)OTA_DEFAULT_DATA_FILE_PATH);
	GxCore_FileDelete(OTA_DEFAULT_DATA_FILE_PATH);
    return 0;
}

// add in 20151202
int app_ota_check_and_flush(unsigned int data_version)
{
#ifdef ECOS_OS
	uint32_t i=0;
	uint32_t j=0;
    uint32_t total_size = 0;
	uint32_t find_part = 0;
	uint32_t need_update = 0;
	uint32_t nNeedUpdate[10] = {0};
	struct partition_info* param = NULL;
	GxPartitionTable    table_from_OTA;
	uint8_t *	        pUpdateData = NULL;

	memset(&table_from_OTA,0,sizeof(GxPartitionTable));
	memset(&table,0,sizeof(GxPartitionTable));
	memset(data_for_upgrade, 0, 10*sizeof(GxOTA_ProtoOTAData_t));

	//Get the Partition table from Flash
	if(GxOem_PartitionTableGet(&table) != 0)
	{
		printf("GxOem_PartitionTableGet  old Error!\n");
		return -1;
	}
	if(table.count == 0)
    {
		GxOtaPrintf("GxOem_PartitionTableGet  Old  Error!\n");
        return -1;
    }
	//Get the Partition table from the OTA Bin file
	pUpdateData = sp_SectionData;
    for(i = 0; i < 4; i++)
    {
        if((char2int(pUpdateData)) == PARTITION_MAGIC)
        {
            if (flash_table_read(&table_from_OTA, pUpdateData, FLASH_PARTITION_SIZE) != 0)
            {
                printf("GxOem_PartitionTableGet  New  Error!\n");
                return -1;
            }
            else
            {
                break;
            }
        }
	    pUpdateData += 0x10000;//find the Table!
    }
    if(i == 4)
    {
        printf("\nMENU OTA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1; 
    }
#ifdef OTA_DEBUG
	//Just print the Partition information
	GxOtaPrintf("Old partition:");
	GxCore_PartitionPrint(&table);
	GxOtaPrintf("New partition:");
	GxCore_PartitionPrint(&table_from_OTA);
#endif

    app_ota_backup_oem();
    app_ota_backup_fe();

    if(1 == i)
    {// normal download_xxx.bin
        param = &(table.tables[0]);
        for(j = 0; j < table_from_OTA.count; j++)
        {
#if 0
            if(0 == strncmp(table_from_OTA.tables[j].name, "PRIVATE", 4))
            {//
             // 用户自定义段，例如ca等关键数据，出厂后就不能因为软件的菜
             // 单操作而修改，需要跳过更新。目较处理比较简单，有很强的局
             // 限性，就是PRIVATE的分区必须放到最该部分代码控制，检测到
             // PRIVATE分区后就返回了，那后面不做更新了。需要配合整机方案
             // 的flash.conf的配置。这里要特别注意，整机BIN文件的使用，会覆
             // 盖整个FLASH的内容，同样也会使得盒子功能异常，这边只能做到
             // 大小的检测，内容可以是完全不一样的。该判断，也只有在STB本机
             // 上的FLASH分区大小和结构和TS流中一致才有意义
                break;
            }
#endif
            total_size += table_from_OTA.tables[j].total_size;
        }

        GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_FLAG, "yes");
        GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_STATUS, "update_flash");
        GxOem_Save();
        GxOem_Destroy();
		GxCore_PartitionWrite(param, (void*)sp_SectionData, total_size, 0);
        GxOem_Init();
        GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_STATUS, "successful");
        GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_FLAG, "off");
        GxOem_Save();

        return 0;
    }
    else if (0 == i) 
    {// update bin, the table section num is 0
        // is all date section
        if((2 == table_from_OTA.count)
                && (0 == strncmp(table_from_OTA.tables[1].name, "ALL", 3)))
        {
            param = &(table.tables[0]);
            for(j = 0; j < table_from_OTA.count; j++)
            {
#if 0
                if(0 == strncmp(table_from_OTA.tables[j].name, "PRIVATE", 4))
                {//
                    // 用户自定义段，例如ca等关键数据，出厂后就不能因为软件的菜
                    // 单操作而修改，需要跳过更新。目较处理比较简单，有很强的局
                    // 限性，就是PRIVATE的分区必须放到最该部分代码控制，检测到
                    // PRIVATE分区后就返回了，那后面不做更新了。需要配合整机方案的
                    // flash.conf的配置。这里要特别注意，整机BIN文件的使用，会覆
                    // 盖整个FLASH的内容，同样也会使得盒子功能异常，这边只能做到
                    // 大小的检测，内容可以是完全不一样的。
                    break;
                }
#endif
                total_size += table_from_OTA.tables[j].total_size;
            }
            GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_FLAG, "yes");
            GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_STATUS, "update_flash");
            GxOem_Save();
            GxOem_Destroy();
            GxCore_PartitionWrite(param, (void*)(sp_SectionData + table_from_OTA.tables[1].start_addr), total_size, 0);
            GxOem_Init();
            GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_STATUS, "successful");
            GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_FLAG, "off");
            GxOem_Save();

            return 0;
        }
        for(i = 1; i < table_from_OTA.count; i++)
        {
            if(strncmp(table_from_OTA.tables[i].name,"DEFAULTDB", 9) == 0)
            {
                app_ota_defaultdata_flush((sp_SectionData + table_from_OTA.tables[i].start_addr), table_from_OTA.tables[i].used_size);
                continue;
            }
#if 0
            // for USER section
            if(strncmp(table_from_OTA.tables[i].name,"PRIVATE", 4) == 0)
            {
                continue;
            }
#endif
            find_part = 0;
            for(j = 0; j < table.count; j++)
            {
                if(strcmp(table_from_OTA.tables[i].name,table.tables[j].name) == 0)
                {
                    find_part = 1;//find the partition
                    GxOtaPrintf("Find partition: %s!\n",table_from_OTA.tables[i].name);
                    break;
                }
            }

            if(find_part)
            {
                //Compare the Space of Partition for Update
                if(table_from_OTA.tables[i].used_size > table.tables[j].total_size)
                {
                    GxOtaPrintf("New partition size is different from old partition total size ->%s!\n",\
                            table_from_OTA.tables[i].name);
                    continue;
                }
                //The Dst Addr is the old partition start Addr
                data_for_upgrade[need_update].dst = table.tables[j].start_addr;
                //The Src Addr is the Buffer Addr add the new Partition start addr
                data_for_upgrade[need_update].src = table_from_OTA.tables[i].start_addr + sp_SectionData;
                //Please ust the new partition total size for Update
                if(table_from_OTA.tables[i].total_size == table.tables[j].total_size)
                {
                    data_for_upgrade[need_update].size = table_from_OTA.tables[i].total_size;
                }
                else
                {
                    data_for_upgrade[need_update].size = table_from_OTA.tables[i].used_size;
                }
                // for table info modify
                table.tables[j].used_size = table_from_OTA.tables[i].used_size;
                table.tables[j].file_system_type = table_from_OTA.tables[i].file_system_type;
                table.tables[j].mode = table_from_OTA.tables[i].mode;
                if(table.tables[j].total_size == table_from_OTA.tables[i].total_size)
                {
                    table.tables[j].crc32_enable = table_from_OTA.tables[i].crc32_enable;
                }
                else
                {// force disable or you can generate a new CRC DATE....
                    table.tables[j].crc32_enable = 0;
                }
                table.tables[j].id = table_from_OTA.tables[i].id;
                table.tables[j].write_protect = table_from_OTA.tables[i].write_protect;
                table.tables[j].update_tags = table_from_OTA.tables[i].update_tags;
                // if "table.tables[j].total_size" != "table_from_OTA.tables[i].total_size", the crc32 is wrong
                table.tables[j].crc_verify= table_from_OTA.tables[i].crc_verify;
                table.tables[j].version = table_from_OTA.tables[i].version;

                GxOtaPrintf("need update : %s!data[i].src = 0x%08x| data[i].dst  = 0x%08x |data.size = %d, crc = %x\n",\
                        table_from_OTA.tables[i].name,data_for_upgrade[need_update].src,data_for_upgrade[need_update].dst,\
                        data_for_upgrade[need_update].size,\
                        table_from_OTA.tables[i].crc_verify);

                //Write the new partition to Flash!
                nNeedUpdate[need_update] = j;
                need_update++;
            }
        }
        //for flash
        // need destroy oem before write flash

        app_ota_backup_oem();

        GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_FLAG, "yes");
        GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_STATUS, "update_flash");
        GxOem_Save();
        GxOem_Destroy();
        app_ota_backup_oem();

        for(i = 0; i < need_update; i++)
        {
            //printf("\033[0m%s[%d]\n\033[0m", __func__, __LINE__);
            param = &(table.tables[nNeedUpdate[i]]);
            app_ota_backup_partition(param);

            //printf("\033[0m%s[%d]\n\033[0m", __func__, __LINE__);
            GxOem_Init();
            GxOem_SetValue("flash", "partition", param->name);
            GxOem_Save();
            GxOem_Destroy();
            //printf("\033[0m%s[%d]\n\033[0m", __func__, __LINE__);

		    //printf("[%s]star_addr:0x%x size:0x%x\n",param->name,param->start_addr,param->total_size);
            GxCore_PartitionWrite(param,(void*)data_for_upgrade[i].src, data_for_upgrade[i].size, 0);

            //printf("\033[0m%s[%d]\n\033[0m", __func__, __LINE__);
            GxOem_Init();
            GxOem_SetValue("flash", "partition", "none");
            GxOem_Save();
            GxOem_Destroy();
        }
        // modify the config file "variable_oem.ini", if the file is exsit
        GxOem_Init();
        GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_STATUS, "successful");
        GxOem_SetValue(OEM_UPDATE_SECTION, OEM_UPDATE_FLAG, "off");
        GxOem_Save();

        // for update flag, which is used in loader ota
        app_ota_set_update_flag(0);
        // for software version, which is used in next update
        app_ota_dsmcc_set_sw_version(data_version);
    }
#endif
#ifdef LINUX_OS
    printf("\nMENU OTA, error, %s, %d\n", __FUNCTION__,__LINE__);
#endif
    return 0;
}

void *app_ota_malloc(unsigned int size, void* hw_data)
{
    GxHwMallocObj *p = NULL;
    if((0 == size) || (NULL == hw_data))
    {
        printf("\nMENU OTA, error, %s, %d\n",__FUNCTION__,__LINE__);
        return NULL;
    }
    p = (GxHwMallocObj*)hw_data;
    memset(p, 0, sizeof(GxHwMallocObj));
    p->size = size;
    GxCore_HwMalloc(p, MALLOC_NO_CACHE);
    if(p->usr_p == NULL)
    {
        printf("\nMENU OTA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return NULL;
    }
    return ((void*)(p->usr_p));
}

void app_ota_free(void* hw_data)
{
    if((NULL == hw_data) || (NULL == ((GxHwMallocObj*)hw_data)->usr_p))
    {
        printf("\nMENU OTA, error, %s, %d\n",__FUNCTION__,__LINE__);
        return ;
    }
    GxCore_HwFree((GxHwMallocObj*)hw_data);
}

void app_ota_upgrade_entry(void)
{
	gxota_flash_unlock_t flash_lock;
	int fd;
    int ret = 0;

	ret = app_ota_select_segment();
    if(ret)
    {
        return ;
    }

	g_nFlagOta = 1;
	g_chFlagEntryFlash = 0;

	flash_lock.offset = 0;
	flash_lock.len = 0; // TODO
	fd = open("/dev/flash/0/0", O_RDWR);
	if(fd < 0)
		return;
	if(ioctl(fd, OTA_CYG_IO_GET_CONFIG_FLASH_UNLOCK, &flash_lock,sizeof(gxota_flash_unlock_t))<0)
    {
        close(fd);
		return;
    }
	close(fd);

	/* Initialize the protocol */
	app_ota_message_update(STR_ID_WAITING, STR_BLANK);
	app_ota_message_update(STR_ID_RECEIVE_DATA, STR_BLANK);
	app_epg_disable();
	//app_send_msg_exec(GXMSG_EPG_RELEASE, NULL);
	//app_send_msg_exec(GXMSG_EXTRA_RELEASE, NULL);
#if CASCAM_SUPPORT
    {
        CASCAMDemuxFlagClass flag;
        flag.Flags = DEMUX_ALL;
        CASCAM_Release_Demux(&flag);
    }
#endif

#if EMM_SUPPORT
    app_emm_release_all();
    app_cat_release();
#endif

#if OTA_MONITOR_SUPPORT
    app_ota_nit_release();
#endif

	g_AppOtaDsi.init();
	g_AppOtaDsi.start(&g_AppOtaDsi);

}

void app_ota_upgrade_exit(void)
{

	app_ota_process_update(0);
	app_ota_message_update(STR_BLANK, STR_BLANK);

	memset(&OtaDsmInfo,0,sizeof(OtaDsmInfo_t));

	if(sp_SectionData !=NULL)
	{
		//GxCore_Free(sp_SectionData);
        app_ota_free((void*)&hw_malloc_block);
		sp_SectionData = NULL;
	}
	g_nFlagOta = 0;
	g_chBissUpdate = 0;
	g_chDefaultDataUpdate = 0;
	g_chLogoUpdate = 0;
	if(1 == g_AppOtaDsi.enable)
		g_AppOtaDsi.stop(&g_AppOtaDsi);
	if(1 == g_AppOtaDii.enable)
		g_AppOtaDii.stop(&g_AppOtaDii);
	if(1 == g_AppOtaDdb.enable)
		g_AppOtaDdb.stop(&g_AppOtaDdb);

#if CASCAM_SUPPORT
        {
            CASCAMDemuxFlagClass flag;
            flag.Flags = DEMUX_CAT|DEMUX_NIT;
            CASCAM_Start_Demux(&flag, 0);
        }
#endif

#if EMM_SUPPORT
    app_cat_restart();
#endif

#if OTA_MONITOR_SUPPORT
    app_ota_nit_restart();
#endif

}

void app_ota_upgrade_ver_old(void)
{
    app_ota_message_update("Sorry! No new version!", STR_BLANK);
    GUI_SetInterface("flush", NULL);
	GxCore_ThreadDelay(200);
	app_ota_upgrade_exit();
	return;
}

#endif
