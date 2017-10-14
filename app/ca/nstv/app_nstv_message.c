/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_nstv_message.c
* Author    :	B.Z.
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.04.10		   B.Z.			  Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_config.h"
#include "app_cascam_pop.h"

#if NSCAS_SUPPORT
extern void app_nstv_ippv_notify_create(void *param);
extern void app_nstv_ippv_notify_exit(char *param);
extern void app_nstv_action_notify(char type, char* notice);
extern void app_nstv_action_notify_exit(char type, char* notice);
extern void app_nstv_curtain_notify(char type, char* notice);
extern void app_nstv_curtain_notify_exit(char type, char* notice);
extern void app_nstv_lock_service_notify(char* param);
extern void app_nstv_lock_service_notify_exit(void);
extern void app_nstv_entitle_notify(char* param);
extern void app_nstv_detitle_notify(char type);
extern void app_nstv_detitle_notify_exit(void);
extern void app_nstv_card_update_notify(char* param);


void app_nstv_message_receive(CmmSuperMessageClass *param)
{
    if(NULL == param)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    switch(param->main_id)
    {
        case NSTV_MESSAGE_ENTITLE:
            app_nstv_entitle_notify(param->message_data);
            break;
        case NSTV_MESSAGE_DETITLE:
#if NSTV_AUTHENTICATION_SUPPORT > 0
            app_nstv_detitle_notify(param->sub_id);
#endif
            break;
        case NSTV_MESSAGE_CURTAIN:
            app_nstv_curtain_notify(param->sub_id, param->message_data);
            break;
        case NSTV_MESSAGE_ACTION:
            app_nstv_action_notify(param->sub_id, param->message_data);
            break;
        case NSTV_MESSAGE_LOCK_SERVICE:
            app_nstv_lock_service_notify(param->message_data);
            break;
        case NSTV_MESSAGE_IPPV:
            app_nstv_ippv_notify_create(param->message_data);
            break;
        case NSTV_MESSAGE_CARD_UPDATE:
            app_nstv_card_update_notify(param->message_data);
            break;
        default:
            printf("\nERROR, main id = %d, %s, %d\n", param->main_id, __FUNCTION__,__LINE__);
            break;
    }
}

void app_nstv_message_destroy(CmmSuperMessageClass *param)
{
    if(NULL == param)
    {
        printf("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    switch(param->main_id)
    {
        case NSTV_MESSAGE_ENTITLE:
            break;
        case NSTV_MESSAGE_DETITLE:
#if NSTV_AUTHENTICATION_SUPPORT > 0
            app_nstv_detitle_notify_exit();
#endif
            break;
        case NSTV_MESSAGE_CURTAIN:
            app_nstv_curtain_notify(param->sub_id, param->message_data);
            break;
        case NSTV_MESSAGE_ACTION:
            app_nstv_action_notify(param->sub_id, param->message_data);
            break;
        case NSTV_MESSAGE_LOCK_SERVICE:
            app_nstv_lock_service_notify_exit();
            break;
        case NSTV_MESSAGE_IPPV:
            app_nstv_ippv_notify_exit(param->message_data);
            break;
        default:
            printf("\nERROR, main id = %d, %s, %d\n", param->main_id, __FUNCTION__,__LINE__);
            break;
    }
}

#endif
