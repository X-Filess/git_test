#ifndef __APP_RF_MODU_H__
#define __APP_RF_MODU_H__

#include "app_config.h"

#define ACTIVE_RF_FMMC017

#define RFMODU_I2C_WRITE_ADDRESS 0xCA
#define RFMODU_I2C_READ_ADDRESS  0xCB
#define RFMODU_NUMBER_OF_BYTES   4
#define RFMODU_MIN_FREQ          471
#define RFMODU_MAX_FREQ          855
#define RFMODU_I2C_ID            (1)    //I2C1

typedef enum
{
    RFMODU_MODE_PAL_I = 0,  //6.0Mhz
    RFMODU_MODE_PAL_DK,     //6.5Mhz
    RFMODU_MODE_PAL_BG,     //5.5Mhz
    RFMODU_MODE_NTSC        //4.5Mhz
}RfModuMode;

typedef enum
{
    RFMODU_POWER_ON = 0,
    RFMODU_POWER_OFF
}RfModuPower;

typedef struct
{
    RfModuPower power;
    RfModuMode mode;
    unsigned char channel;
}RfModuPara;

int app_rf_modu_set_power(RfModuPower power);

int app_rf_modu_set_mode(RfModuMode mode);
RfModuMode app_rf_modu_get_mode(void);

int app_rf_modu_set_channel(unsigned char channel);
unsigned char app_rf_modu_get_channel(void);

#endif
