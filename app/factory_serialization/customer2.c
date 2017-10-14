#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "customer.h"

#ifdef CUSTOMER_2_SUPPORT

#define DSN_ID_ADDR         0x780
#define SMARTCARD_ID_ADDR   0x784
#define CHIP_ID_ADDR        0x348
#define LOCK_ADDR           0x3ff //control bit is 7

extern int chip_otp_device_read(unsigned int addr, unsigned int read_bytes, unsigned char *data);
extern int chip_otp_device_write(unsigned int addr, unsigned int write_bytes, unsigned char *data);
extern char function_des3(unsigned char *pucKey, unsigned char *pucSourceData, unsigned int uiLen, unsigned char *pucDestOut, unsigned char ucMode);

int get_card_id(char *buffer, unsigned int bytes)
{
    int ret =0;
    int i = 0;
    unsigned char temp[8] = {0};
    int j = 0;
    if((bytes != 16) || (NULL == buffer))
    {
        return -1;
    }
    ret = chip_otp_device_read(SMARTCARD_ID_ADDR, 8, temp);
    if(ret < 0)
    {
        return -1;
    }

    for(i = 0; i < 8; i++)
    {
        sprintf(&buffer[j++], "%x", (temp[i]>>4)&0xf);
        sprintf(&buffer[j++], "%x", temp[i]&0xf);
    }
    return ret;
}

int set_card_id(char *buffer, unsigned int bytes)
{
    return 0;
}

int get_provider_id(char *buffer, unsigned int bytes)
{
    return 0;
}

int set_provider_id(char *buffer, unsigned int bytes)
{
    return 0;
}


int get_dsn_id(char *buffer, unsigned int bytes)
{
    int ret =0;
    int i = 0;
    int j = 0;
    unsigned char temp[8] = {0};

    if((bytes != 16) || (NULL == buffer))
    {
        return -1;
    }
    ret = chip_otp_device_read(DSN_ID_ADDR, 4, temp);
    if(ret)
    {
        return -1;
    }

    for(i = 0; i < 4; i++)
    {
        sprintf(&buffer[j++], "%x", (temp[i]>>4)&0xf);
        sprintf(&buffer[j++], "%x", temp[i]&0xf);
    }
    return ret;
}

int get_chip_id(char *buffer, unsigned int bytes)
{
    int ret =0;
    int i = 0;
    int j = 0;
    unsigned char temp[8] = {0};

    if((NULL == buffer)|| (16 != bytes))
    {
        return -1;
    }
    ret = chip_otp_device_read(CHIP_ID_ADDR, 8, temp);
    if(ret)
    {
        return -1;
    }

    for(i = 0; i < 8; i++)
    {
        sprintf(&buffer[j++], "%x", (temp[i]>>4)&0xf);
        sprintf(&buffer[j++], "%x", temp[i]&0xf);
    }

    return ret;
}

int lock_otp(void)
{
    int ret = 0;
    unsigned char lock_data = 0;

    ret = chip_otp_device_read(LOCK_ADDR, 1, &lock_data);
    if(ret)
    {
        return -1;
    }
    lock_data |= (1<<7);
    ret = chip_otp_device_write(LOCK_ADDR, 1, &lock_data);
    if(ret)
    {
        return -1;
    }
    return ret;
}

//return, 1: need serialization; 0: need not serialization; -1: error
int get_factory_serialization_flag(void)
{
    return 0;
}
#endif
