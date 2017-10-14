#ifndef __APP_ROOT_H__
#define __APP_ROOT_H__

#include <gxtype.h>
#include "gui_core.h"
#include "gui_timer.h"
#include "gui_key.h"
#include "gui_event.h"
#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

status_t app_wnd_main_menu(void);
status_t app_wnd_full_screen(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_ROOT_H__ */
