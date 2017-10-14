#include "app_rf_modu.h"
#include <stdio.h>

#ifdef ECOS_OS

extern void *gx_i2c_open(unsigned int id);
extern int gx_i2c_set(void *dev, unsigned int busid, unsigned int chip_addr, unsigned int address_width, int send_noack, int send_stop);
extern int gx_i2c_tx(void *dev, unsigned int reg_address, const unsigned char *tx_data, unsigned int count);
extern int gx_i2c_rx(void *dev, unsigned int reg_address, unsigned char *rx_data, unsigned int count);
extern int gx_i2c_close(void *dev);

static void *rf_i2c = NULL;
static unsigned char s_rfmodu_channel_no = 25;
static RfModuMode s_rfmodu_mode = RFMODU_MODE_NTSC;

static int _rf_i2c_init(unsigned int id)
{
    rf_i2c = gx_i2c_open(id);

    if (rf_i2c == NULL) {
        return -1;
    }

    return 0;
}

static int _rf_i2c_uninit(void)
{
    return gx_i2c_close(rf_i2c);
}

static int _rf_i2c_write(unsigned char *buf, unsigned char size)
{
    if (NULL == buf || 0 == size) {
        return -1;
    }

    gx_i2c_set(rf_i2c, 0, RFMODU_I2C_WRITE_ADDRESS, 1, 0, 1);
    if (gx_i2c_tx(rf_i2c, RFMODU_I2C_WRITE_ADDRESS, buf, size) == size) {
        return 0;
    }
    else {
        return -1;
    }

    return -1;
}

static int _rf_i2c_read(unsigned char *buf, unsigned char size)
{
    if (NULL == buf || 0 == size) {
        return -1;
    }

    gx_i2c_set(rf_i2c, 0, RFMODU_I2C_READ_ADDRESS, 1, 0, 1);
    if (gx_i2c_rx(rf_i2c, RFMODU_I2C_READ_ADDRESS, buf, size) == size) {
        return 0;
    }
    else {
        return -1;
    }
}

unsigned short rf_modu_ch_to_freq(unsigned char chRfChannel, RfModuMode mode)
{
    unsigned short wFreq;
    wFreq = (chRfChannel - 21);
    wFreq *= 8;
    wFreq += RFMODU_MIN_FREQ;

    if (wFreq < RFMODU_MIN_FREQ) {
        wFreq = RFMODU_MIN_FREQ;
    }
    if (wFreq > RFMODU_MAX_FREQ) {
        wFreq = RFMODU_MAX_FREQ;
    }

    return wFreq;

}

static int rf_modu_setup(RfModuPara *pRfModuPara)
{
    unsigned char chI2cBuff[RFMODU_NUMBER_OF_BYTES + 1];
    unsigned char chWriteI2cTimes;
    int iDivider;

    if (pRfModuPara->channel < 21 || pRfModuPara->channel > 69) {
        return -1;
    }

    switch (pRfModuPara->mode) {
        case RFMODU_MODE_PAL_I :
            iDivider = (int)((471250 + (pRfModuPara->channel - 21) * 8000)) / 250.0;
            chI2cBuff[1] = 0x50;
            break;
        case RFMODU_MODE_PAL_DK:
            iDivider = (int)((471250 + (pRfModuPara->channel - 21) * 8000)) / 250.0;
            chI2cBuff[1] = 0x58;
            break;
        case RFMODU_MODE_PAL_BG:
            iDivider = (int)((471250 + (pRfModuPara->channel - 21) * 8000)) / 250.0;
            chI2cBuff[1] = 0x48;
            break;
        case RFMODU_MODE_NTSC:
            iDivider = (int)((471250 + (pRfModuPara->channel - 21) * 8000)) / 250.0;
            chI2cBuff[1] = 0x40;
            break;
        default:
            iDivider = 0;
            chI2cBuff[1] = 0x48;
            break;
    }

    printf("\033[31mchChannelNo:%d[%dhz]\n\033[0m",
           pRfModuPara->channel,
           471250 + (pRfModuPara->channel - 21) * 8000);

    chI2cBuff[0] = 0x80;
    if (pRfModuPara->power == RFMODU_POWER_OFF) {
        iDivider = 0;
        chI2cBuff[0] = 0x90;
    }

    chI2cBuff[2] = (unsigned char)((iDivider >> 6) & 0x3F);
    chI2cBuff[3] = (unsigned char)((iDivider << 2) & 0xfc) ;

    //printf("Byte0:0x%x, Byte1:0x%x, Byte2:0x%x, Byte3:0x%x\n", chI2cBuff[0], chI2cBuff[1], chI2cBuff[2], chI2cBuff[3]);
    for (chWriteI2cTimes = 0; chWriteI2cTimes < 3; chWriteI2cTimes++) {
        gx_i2c_set(rf_i2c, 0, RFMODU_I2C_WRITE_ADDRESS, 1, 0, 1);
        gx_i2c_tx(rf_i2c, chI2cBuff[0], chI2cBuff + 1, 3);
    }
    return 0;
}

int app_rf_modu_set_power(RfModuPower power)
{
    RfModuPara paras = {0};
    paras.power = power;
    paras.mode = s_rfmodu_mode;
    paras.channel = s_rfmodu_channel_no;

    rf_modu_setup(&paras);
    return 0;
}

int app_rf_modu_set_mode(RfModuMode mode)
{
    RfModuPara paras = {0};
    paras.power = RFMODU_POWER_ON;
    paras.mode = mode;
    paras.channel = s_rfmodu_channel_no;

    s_rfmodu_mode = mode;
    rf_modu_setup(&paras);
    return 0;
}

RfModuMode app_rf_modu_get_mode(void)
{
    return s_rfmodu_mode;
}

int app_rf_modu_set_channel(unsigned char channel)
{
    RfModuPara paras = {0};
    paras.power = RFMODU_POWER_ON;
    paras.mode = s_rfmodu_mode;
    paras.channel = channel;

    s_rfmodu_channel_no = channel;
    rf_modu_setup(&paras);
    return 0;
}

unsigned char app_rf_modu_get_channel(void)
{
    return s_rfmodu_channel_no;
}

int app_rf_modulator_working(void)
{
    if (0 != _rf_i2c_init(RFMODU_I2C_ID)) {
        return -1;
    }

    if (0 != app_rf_modu_set_power(RFMODU_POWER_ON)) {
        return -1;
    }

    if (0 != app_rf_modu_set_mode(RFMODU_MODE_PAL_DK)) {
        return -1;
    }

    if (0 != app_rf_modu_set_channel(s_rfmodu_channel_no)) {
        return -1;
    }

    //if (0 != _rf_i2c_uninit()) {
    //    return -1;
    //}

    return 0;
}

#endif
