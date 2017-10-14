#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#define DEV_NAME                    "/dev/gxotp0"
#define CHIP_OTP_IOCTL_ADDR         123

static int thiz_device_fd = 0;

int chip_otp_device_open(void)
{
    if(thiz_device_fd > 0)
    {
        return 0;// have opend
    }
    thiz_device_fd = open(DEV_NAME, O_RDWR);
    if(thiz_device_fd <= 0)
    {// open failed
        return -1;
    }
    // open device success
    return 0;
}

void chip_otp_device_close(void)
{
    if(thiz_device_fd > 0)
    {
        close(thiz_device_fd);
        thiz_device_fd = 0;
    }
}

int chip_otp_device_read(unsigned int addr, unsigned int read_bytes, unsigned char *data)
{
    int ret = 0;
    unsigned start_addr = addr;
    if((0x7ff < addr) || (NULL == data))
    {
        return -1;
    }
    if(thiz_device_fd <= 0)
    {
        thiz_device_fd = open(DEV_NAME, O_RDWR);
        if(thiz_device_fd <= 0)
        {// open failed
            return -1;
        }
    }
    ret = ioctl(thiz_device_fd, CHIP_OTP_IOCTL_ADDR, &start_addr);
    if(0 != ret)
    {// set addr failed
        return -1;
    }
    ret = read(thiz_device_fd, data, read_bytes);
    if(ret <= 0)
    {
        return -1;
    }
    return 0;
}

int chip_otp_device_write(unsigned int addr, unsigned int write_bytes, unsigned char *data)
{
    int ret = 0;
    unsigned start_addr = addr;
    if((0x7ff < addr) || (NULL == data))
    {
        return -1;
    }
    if(thiz_device_fd <= 0)
    {
        thiz_device_fd = open(DEV_NAME, O_RDWR);
        if(thiz_device_fd <= 0)
        {// open failed
            return -1;
        }
    }
    ret = ioctl(thiz_device_fd, CHIP_OTP_IOCTL_ADDR, &start_addr);
    if(0 != ret)
    {// set addr failed
        return -1;
    }
    ret = write(thiz_device_fd, data, write_bytes);
    if(ret <= 0)
    {
        return -1;
    }
    return 0;
}
