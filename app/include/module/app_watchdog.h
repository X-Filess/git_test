#ifndef __APP_WATCHDOG_H__
#define __APP_WATCHDOG_H__


#ifdef LINUX_OS
status_t app_watchdog_init(time_t timeout_sec);
status_t app_watchdog_feed(void);
status_t app_watchdog_close(void);
#endif

#endif
