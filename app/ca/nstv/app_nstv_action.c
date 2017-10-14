/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_action.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.04.24		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_config.h"
#include "app_cascam_pop.h"

#if NSCAS_SUPPORT
extern void app_sysinfo_menu_exec(void);

#if 0
static int _action_pop_exit(PopDlgRet ret)
{
    extern void app_reboot(void);
    app_reboot();
    return 0;
}
static void _action_reboot_stb(char *message)
{
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.mode = POP_MODE_UNBLOCK;
    pop.type = POP_TYPE_NO_BTN;
    pop.timeout_sec = 3;
    pop.show_time = true;
    pop.str = message;
    pop.exit_cb = _action_pop_exit;
    popdlg_create(&pop);
}
#endif
#if 0
static int _action_pop_exit(PopDlgRet ret)
{
    cmm_exit_dialog2();
    return 0;
}
static void _action_time_notice(char *message)
{
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.mode = POP_MODE_UNBLOCK;
    pop.type = POP_TYPE_NO_BTN;
    pop.timeout_sec = 3;
    pop.show_time = true;
    pop.str = message;
    pop.exit_cb = _action_pop_exit;
    popdlg_create(&pop);
}
#endif

static void _action_notice(char *message)
{
    CmmNoticeClass notice = {0};
    notice.str = message;
    cmm_cascam_create_dialog2(&notice);
}

static void _action_notice_clear(void)
{
    cmm_cascam_exit_dialog2();
}


void app_nstv_action_notify(char type, char* notice)
{
    CASCAMNSTVActionTypeEnum action_type = (CASCAMNSTVActionTypeEnum)type;
    switch(action_type)
    {
        case NSTV_ACTION_REBOOT:/*重启机顶盒*/
            //_action_reboot_stb(notice);
            _action_notice(notice);
            break;
        case NSTV_ACTION_FREEZE:/*冻结机顶盒*/
            _action_notice(notice);
            break;
        case NSTV_ACTION_RESEARCH:/*重新搜索节目*/
            // TODO:
            break;
        case NSTV_ACTION_UPGRADE:/*机顶盒程序升级*/
            _action_notice(notice);
            break;
        case NSTV_ACTION_UNFREEZE:/*解除冻结机顶盒*/
            //_action_time_notice(notice);
            _action_notice_clear();
            break;
        case NSTV_ACTION_INIT_STB:/*初始化机顶盒*/
            break;
        case NSTV_ACTION_DISPLAY_SYSTEM_INFO:/*显示系统信息*/
            app_sysinfo_menu_exec();
            break;
        case NSTV_ACTION_DISABLE_EPG_AD:/*禁用EPG广告信息功能*/
            break;
        case NSTV_ACTION_ENABLE_EPG_AD:/*恢复EPG广告信息功能*/
            break;
        case NSTV_ACTION_FINISH_CARD_IN:/*插卡处理完成*/
            break;
        default:
            printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }

}
void app_nstv_action_notify_exit(char type, char* notice)
{
    _action_notice_clear();
}

#endif
