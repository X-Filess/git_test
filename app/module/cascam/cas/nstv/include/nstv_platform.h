/**
 *
 * @file        nstv_platform.h
 * @brief
 * @version     1.1.0
 * @date        04/17/2016 09:47:49 AM
 * @author      B.Z.
 *
 */
#include "../../../include/cascam_types.h"


int nstv_basic_information_get(int sub_id, void *property, unsigned int len);

int nstv_entitle_information_get(unsigned int sub_id, void *property, unsigned int len);

int nstv_mail_information_get(unsigned int sub_id, void *property, unsigned int len);

int nstv_mail_check_box_is_full(void);

int nstv_mail_check_have_unread_mail(void);

int nstv_maturity_get(int sub_id, void* param, unsigned int len);

int nstv_pin_get(int sub_id, void* param, unsigned int len);

int nstv_work_time_get(int sub_id, void* param, unsigned int len);

int nstv_wallet_information_get(unsigned int sub_id, void *property, unsigned int len);

int nstv_ippv_information_get(unsigned int sub_id, void *property, unsigned int len);

int nstv_detitle_information_get(unsigned int sub_id, void *property, unsigned int len);

int nstv_detitle_check_is_full(void);

int nstv_detitle_check_have_unread_record(void);

int nstv_features_information_get(unsigned int sub_id, void *property, unsigned int len);

int nstv_operator_information_get(unsigned int sub_id, void *property, unsigned int len);

int nstv_card_update_get(int sub_id, void* param, unsigned int len);

int nstv_mail_information_edit(unsigned int sub_id, void *property, unsigned int len);

int nstv_maturity_set(int sub_id, void* param, unsigned int len);

int nstv_pin_set(int sub_id, void* param, unsigned int len);

int nstv_work_time_set(int sub_id, void* param, unsigned int len);

int nstv_detitle_information_edit(unsigned int sub_id, void *property, unsigned int len);

int nstv_card_parent_child_information_get(int sub_id, void* param, unsigned int len);

int nstv_parent_child_set(unsigned int sub_id, void *property, unsigned int len);

// osd
int nstv_osd_entitlement_notify(unsigned short operator_id);

void nstv_osd_detitle_notify(unsigned char detitle_status);

int nstv_osd_ippx_notify(unsigned char type, unsigned short ecm_pid, void* param);

int nstv_osd_ippx_hide(unsigned short ecm_pid);

void nstv_osd_mail_notify(unsigned char show_flag, unsigned int mail_id);

int nstv_osd_screen_display_notify(unsigned char style, char* message);

int nstv_osd_screen_display_hide(int style);

void nstv_lock_service_control(void *param);

int nstv_osd_message_notify(unsigned char ecm_pid, unsigned int error_code);

int nstv_osd_fingerprint_notify(unsigned short ecm_pid, char* msg);

void nstv_osd_curtain_notify(unsigned short ecm_pid, unsigned short code);

void nstv_action_notify(unsigned short ecm_pid, unsigned short code);

int nstv_mail_check_box_is_full(void);

int nstv_mail_check_have_unread_mail(void);

int nstv_detitle_check_is_full(void);

int nstv_detitle_check_have_unread_record(void);

void nstv_osd_mail_notice_init(void);

void nstv_osd_card_update_notify(unsigned char mark, unsigned char progress);

void app_nstv_card_parent_child_osd(unsigned char status);


