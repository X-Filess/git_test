#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "customer.h"

#ifdef CUSTOMER_1_SUPPORT
// for test
#define SMARTCARD_ID_ADDR   0x780
#define PROVIDER_ID_ADDR    0x788
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
    unsigned char temp1[8] = {0};
    unsigned char temp2[8] = {0};
    ret = chip_otp_device_read(CHIP_ID_ADDR, 8, temp1);
    function_des3(temp1, temp, 8, temp2, 1);
    memcpy(temp, temp2, 8);
    for(i = 0; i < 8; i++)
    {
        sprintf(&buffer[j++], "%x", (temp[i]>>4)&0xf);
        sprintf(&buffer[j++], "%x", temp[i]&0xf);
    }
    return ret;
}

int set_card_id(char *buffer, unsigned int bytes)
{
    int ret =0;
    int i = 0;
    unsigned char chip_id[8] = {0};
    unsigned char temp[8] = {0};
    unsigned char temp1[8] = {0};
    unsigned char buf[17] = {0};
    char alpha = 0;
    char bata = 0;
    
    if(NULL == buffer)
    {
        return -1;
    }
    // check
    ret = chip_otp_device_read(SMARTCARD_ID_ADDR, 8, temp);
    if((ret < 0) || (strncmp((char*)temp,(char*)temp1, 8) != 0))
    {// had been writed
        return -1;
    }
    memset(temp, 0, 8);

    ret = chip_otp_device_read(CHIP_ID_ADDR, 8, chip_id);
    if(ret)
    {
        return -1;
    }

    if(bytes < 16)
    {// set to 16bytes
        //sprintf(buf, "%016s", buffer);
        for(i = 0; i < (16-bytes); i++)
        {
            //strcat((char*)buf, (char*)'0');
            sprintf((char*)&buf[i], "%s", "0");
        }
        strcat((char*)buf, buffer);
    }
    else
    {
        memcpy(buf, buffer, 16);
    }

    // change string to hex
    for(i = 0; i < 16; i += 2)
    {
        //alpha = atoi(buf[i]);
        //bata  = atoi(buf[i+1]);
        //temp1[i/2] = ((alpha&0xf)<<4) + (bata&0xf);
        temp[0] = buf[i];
        temp[1] = '\0';
        alpha = strtol((const char*)temp, NULL, 16);
        temp[0] = buf[i+1];
        temp[1] = '\0';
        bata = strtol((const char*)temp, NULL, 16);
        temp1[i/2] = ((alpha&0xf)<<4) + (bata&0xf);
    }

    // encrypt by chip id
    function_des3(chip_id, temp1, 8, temp, 0);

    ret = chip_otp_device_write(SMARTCARD_ID_ADDR, 8, temp);
    if(ret)
    {
        return -1;
    }
// for check
    ret = chip_otp_device_read(SMARTCARD_ID_ADDR, 8, temp1);
    if((ret < 0) || (strncmp((char*)temp,(char*)temp1, 8) != 0))
    {// had been writed
        return -1;
    }

    return ret;
}

int get_provider_id(char *buffer, unsigned int bytes)
{
    int ret =0;
    int i = 0;
    int j = 0;
    unsigned char temp[8] = {0};

    if((bytes != 16) || (NULL == buffer))
    {
        return -1;
    }
    ret = chip_otp_device_read(PROVIDER_ID_ADDR, 8, temp);
    if(ret)
    {
        return -1;
    }

// test
    unsigned char temp1[8] = {0};
    unsigned char temp2[8] = {0};
    ret = chip_otp_device_read(CHIP_ID_ADDR, 8, temp1);
    if(ret)
    {
        return -1;
    }
    function_des3(temp1, temp, 8, temp2, 1);
    memcpy(temp, temp2, 8);

    for(i = 0; i < 8; i++)
    {
        sprintf(&buffer[j++], "%x", (temp[i]>>4)&0xf);
        sprintf(&buffer[j++], "%x", temp[i]&0xf);
    }
    return ret;
}

int set_provider_id(char *buffer, unsigned int bytes)
{
    int ret =0;
    unsigned char chip_id[8] = {0};
    unsigned char temp[8] = {0};
    int i = 0;
    unsigned char temp1[8] = {0};
    unsigned char buf[17] = {0}; 
    char alpha = 0;
    char bata = 0;

    if(NULL == buffer)
    {
        return -1;
    }
    // check
    ret = chip_otp_device_read(PROVIDER_ID_ADDR, 8, temp);

    if((ret) || (strncmp((char*)temp,(char*)temp1, 8) != 0))
    {
        return -1;
    }
    memset(temp, 0, 8);

    ret = chip_otp_device_read(CHIP_ID_ADDR, 8, chip_id);
    if(ret)
    {
        return -1;
    }

    if(bytes < 16)
    {// set to 16bytes
        //sprintf(buf, "%016s", buffer);
        for(i = 0; i < (16-bytes); i++)
        {
            //strcat((char*)buf, (char*)'0');
            sprintf((char*)&buf[i], "%s", "0");
        }
        strcat((char*)buf, buffer);

    }
    else
    {
        memcpy(buf, buffer, 16);
    }
    // change string to hex
    for(i = 0; i < 16; i += 2)
    {
        temp[0] = buf[i];
        temp[1] = '\0';
        alpha = strtol((const char*)temp, NULL, 16);
        temp[0] = buf[i+1];
        temp[1] = '\0';
        bata = strtol((const char*)temp, NULL, 16);
        temp1[i/2] = ((alpha&0xf)<<4) + (bata&0xf);

        //alpha = atoi(buf[i]);
        //bata  = atoi(buf[i+1]);
        //temp1[i/2] = ((alpha&0xf)<<4) + (bata&0xf);
    }

    // encrypt by chip id
    function_des3(chip_id, temp1, 8, temp, 0);

    ret = chip_otp_device_write(PROVIDER_ID_ADDR, 8, temp);
    if(ret)
    {
        return -1;
    }
    // check
    ret = chip_otp_device_read(PROVIDER_ID_ADDR, 8, temp1);
    if((ret) || (strncmp((char*)temp,(char*)temp1, 8) != 0))
    {
        return -1;
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
    char temp[8] = {0};
    char temp1[8] = {0};
    int ret = 0;
    ret = chip_otp_device_read(SMARTCARD_ID_ADDR, 8, (unsigned char*)temp);
    if(ret != 0)
    {
        return -1;
    }

    if(strncmp(temp,temp1, 8) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
#endif
