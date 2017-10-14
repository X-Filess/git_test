#include "app_module.h"

#ifdef LINUX_OS

#include <linux/watchdog.h>

#define WDT_DEV "/dev/watchdog"
static int s_watchdog_fd = -1;

status_t app_watchdog_init(time_t timeout_sec)
{
    int timeout = timeout_sec;

    if(s_watchdog_fd == -1)
    {
        if((s_watchdog_fd = open(WDT_DEV, O_RDWR)) <= 0)
        {
            s_watchdog_fd = -1;
            printf("[%s][%d] open watchdog err !!!\n", __func__, __LINE__);
            return GXCORE_ERROR;
        }
        //printf("[%s][%d] open watchdog %d success, %d sec!!!\n", __func__, __LINE__, s_watchdog_fd, timeout);
        ioctl(s_watchdog_fd, WDIOS_ENABLECARD, NULL);
    }

    ioctl(s_watchdog_fd, WDIOC_SETTIMEOUT, &timeout);

    return GXCORE_SUCCESS;
}

status_t app_watchdog_feed(void)
{
    if(s_watchdog_fd == -1)
    {
        return GXCORE_ERROR;
    }

    ioctl(s_watchdog_fd, WDIOC_KEEPALIVE, NULL);

    return GXCORE_SUCCESS;
}

status_t app_watchdog_close(void)
{
    if(s_watchdog_fd != -1)
    {
        //printf("[%s][%d] close watchdog !!!\n", __func__, __LINE__);
        ioctl(s_watchdog_fd, WDIOS_DISABLECARD, NULL);
        close(s_watchdog_fd);
        s_watchdog_fd = -1;
    }

    return GXCORE_SUCCESS;
}

#endif

