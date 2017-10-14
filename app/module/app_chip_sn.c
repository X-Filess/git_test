/*
 * =====================================================================================
 *
 *       Filename:  otp_chip.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/17/2013 01:26:45 PM
 *       Revision:  none
 *       Compiler:  gcc
 
 *         Author:  Z.Z.R 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <stdio.h> 

#include "app_config.h"

#ifdef ECOS_OS
#define GX_OTP_BASE 0xA0F80000

extern int _otp_read_chip_name(char *SnData, int rd_num);
/*
 * OTP control registe, register used when CPU access OTP
 * OTP config register
 * OTP state register
 */
#define OTP_CON_REG (*(volatile unsigned int*)(GX_OTP_BASE + 0x80))
#define OTP_CFG_REG (*(volatile unsigned int*)(GX_OTP_BASE + 0x84))
#define OTP_STA_REG (*(volatile unsigned int*)(GX_OTP_BASE + 0x88))
#ifdef GX3201
static void otp_read_hard(unsigned int start_addr, int rd_num, char *data)
{
	int i;
	unsigned int otp_con_reg;
	otp_con_reg = 0x43000000 | (0x3fff & start_addr);
	for (i = 0; i < rd_num; i++)
	{
		/* check if OTP Controller is busy now; 1 for busy */
		while(OTP_STA_REG & 0x100);
		/* set READEN */
		OTP_CON_REG = otp_con_reg | (0x1 << 14);
		/* clear READEN */
		OTP_CON_REG = otp_con_reg;
		/* chekc if OTP data is ready */
		while(!(OTP_STA_REG & 0x200));
		data[i] = (unsigned char)(OTP_STA_REG & 0xFF);
		otp_con_reg++;
	}
	/* set all control signals to be invalid, address to 0x300(default) */
	/* otp_con_reg = 0x43000300; */
}
#endif

// the chip sn address: 0x338; the sn length:8byte
int app_chip_sn_get(char *SnData, int BufferLen)
{

	char buffer[8] = {0};
#ifdef GX3201
	if((SnData == NULL) &&(BufferLen <8))
	{
		return -1;
	}

	otp_read_hard(0x338,8,buffer);
#else
	_otp_read_chip_name(buffer,8);
#endif

	memcpy(SnData, buffer,8);
	{
		int i =0;
		printf("\n**********start************len=%d\n",BufferLen);
		for(i=0;i<8;i++)
		{
			printf("0x%x ",buffer[i]);
		}
		printf("\n**********end************\n");
	}


	return 0;
}
#endif

#ifdef LINUX_OS
#include <gxos/gxcore_os.h>
// file path: rootfs/home/gx/sbin/disp_chipsn.sh
// buildroot file parth: buildroot/templete/(project name)/home/gx/sbin/disp_chipsn.sh
#define READ_SN_SHELL_CMD     "disp_chipsn.sh -a 0x338 -n 8"
#define SN_RECORD_FILE	      "/proc/gx_otp"
// the chip sn address: 0x338; the sn length:8byte
int app_chip_sn_get(char *SnData, int BufferLen)
{
    char buffer[8] = {0};
    FILE *fp = NULL;
    int ret = 0;
    if((SnData == NULL) &&(BufferLen <8))
    {
        return -1;
    }

    system(READ_SN_SHELL_CMD);
    fp = fopen(SN_RECORD_FILE,"r");
    if(fp == NULL)
    {
        printf("\nCHIP SN, open file failed...\n");
        return -1;
    }
    ret = fread(buffer, 1, 8, fp);
    if(ret != 8)
    {
        printf("\nCHIP SN, read sn failed...\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    memcpy(SnData, buffer,8);
    return 0;
}
#endif

