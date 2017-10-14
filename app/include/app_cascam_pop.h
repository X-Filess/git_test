#ifndef APP_CASCAM_POP_H
#define APP_CASCAM_POP_H

typedef enum _CmmDialogTypeEnum{
    DIALOG_NO_CONTROL,
    DIALOG_YES_NO,
    DIALOG_OK,
}CmmDialogTypeEnum;

typedef enum _CmmDialogRetEnum{
    DIALOG_RET_NONE,
    DIALOG_RET_YES,
    DIALOG_RET_OK,
    DIALOG_RET_NO,
}CmmDialogRetEnum;

typedef struct _CmmDialogPosClass{
    unsigned int x;
    unsigned int y;
}CmmDialogPosClass;

typedef struct _CmmDialogStrClass{
    char *str;
    unsigned int x_offset;
    unsigned int y_offset;
    char *back_color;// [#000000,#000000,#000000], RGB
    char *fore_color;// [#000000,#000000,#000000], RGB
    char *alignment; // hcentre|vcentre, left|vcentre, ...
    unsigned char state; // 1: show; 0: hide
}CmmDialogStrClass;

typedef int (*CreateFunc)(void);
typedef int (*ExitFunc)(CmmDialogRetEnum ret);
typedef int (*TimeoutFunc)(void);
typedef struct _CmmDialogClass{
    CmmDialogTypeEnum   type;
    CmmDialogStrClass   title;
    CmmDialogStrClass   content1;
    CmmDialogStrClass   content2;
    CmmDialogStrClass   content3;
    CmmDialogStrClass   content4;
    CmmDialogStrClass   content_long;
    CmmDialogPosClass   pos;        /* pos.x = 0, used hcentre & vcentre */
    CreateFunc          create_cb;
    ExitFunc            exit_cb;
    TimeoutFunc         timeout_cb;
    unsigned int        timeout_sec;
}CmmDialogClass;

typedef struct _CmmPasswordDlgClass{
    CmmDialogStrClass   title;
    CmmDialogStrClass   content1;
    CmmDialogStrClass   content2;
    CmmDialogStrClass   content3;
    CmmDialogPosClass   pos;        /* pos.x = 0, used hcentre & vcentre */
    ExitFunc            exit_cb;
    unsigned int        retry_times;
}CmmPasswordDlgClass;

typedef enum _CmmSubTypeEnum{
    PASSWORD_TYPE,
    CHOICE_TYPE,
    EDIT_TYPE,
    TEXT_MODE,
}CmmSubTypeEnum;

typedef struct _CmmWidgetClass{
    char *str_l;
    char *str_r;
    CmmSubTypeEnum type;
}CmmWidgetClass;

typedef struct _CmmComplexDlgClass{
    CmmDialogStrClass   title;
    CmmDialogStrClass   content1;
    CmmDialogStrClass   content2;
    CmmDialogStrClass   content3;
    CmmDialogStrClass   content4;
    CmmDialogPosClass   pos;        /* pos.x = 0, used hcentre & vcentre */
    CreateFunc          create_cb;
    ExitFunc            exit_cb;
    CmmWidgetClass      widget_ex;
    unsigned int        retry_times;// used in password mode
}CmmComplexDlgClass;

typedef struct _CmmOsdRollingClass{
    unsigned int x;
    unsigned int y;
    char *str;
    char *direction;
    int back_color;// [#000000,#000000,#000000], RGB
    int fore_color;// [#000000,#000000,#000000], RGB
    unsigned int pos;// 0: top; 1:bottom
    unsigned int duration;
    unsigned int repeat_times;
}CmmOsdRollingClass;

typedef struct _CmmNoticeClass{
    unsigned int x;
    unsigned int y;
    char *str;
}CmmNoticeClass;

typedef enum _CmmEmailNoticeTypeEnum{
    EMAIL_RECEIVE_NEW,
    EMAIL_BOX_FULL,
}CmmEmailNoticeTypeEnum;

typedef struct _CmmEmailNoticeClass{
    unsigned int x;
    unsigned int y;
    unsigned int flicker;
    CmmEmailNoticeTypeEnum type; 
}CmmEmailNoticeClass;

typedef enum _CmmFingerTypeEnum{
    CMM_STRING,
    CMM_QRCODE,
}CmmFingerTypeEnum;

typedef struct _CmmFingerprintClass{
    CmmFingerTypeEnum type;
    char random;
    union{
        struct{
            char *content;
        }string;
        struct{
            unsigned int row;
            unsigned int column;
        }QRCode;
    }fingerContent;
}CmmFingerprintClass;

typedef struct _CmmSuperMessageClass{
    char main_id;
    char sub_id;
    char* message_data;
}CmmSuperMessageClass;

typedef struct _CMMOSDClass{
    void (*CMMCreateDialog1) (CmmDialogClass *param);
    void (*CMMExitDialog1)(void);
    void (*CMMCreateDialog2) (CmmNoticeClass *param);
    void (*CMMExitDialog2)(void);
    void (*CMMCreateDialog3) (CmmOsdRollingClass *param);
    void (*CMMExitDialog3)(int style);
    void (*CMMCreateDialog4) (CmmPasswordDlgClass *param);
    void (*CMMExitDialog4)(void);
    void (*CMMCreateDialog5) (CmmEmailNoticeClass *param);
    void (*CMMExitDialog5)(CmmEmailNoticeTypeEnum type);
    void (*CMMCreateDialog6) (CmmFingerprintClass *param);
    void (*CMMExitDialog6)(CmmFingerTypeEnum type);
    void (*CMMCreateDialogx) (CmmSuperMessageClass *param);
    void (*CMMExitDialogx)(CmmSuperMessageClass *param);

    // rolling
    void (*CMMCheckRollingRectOverlap)(unsigned int x, unsigned int y, unsigned int w, unsigned h);
    void (*CMMGetRollingTimes)(unsigned int *times, int style);
    int  (*CMMCheckServiceLockStatus)(void);
    // finger
    void (*CMMCheckFingerRectOverlap)(unsigned int x, unsigned int y, unsigned int w, unsigned h);
    void (*CMMCheckFingerResume)(void);

  
}CMMOSDClass;

void cmm_cascam_create_dialog1(CmmDialogClass *para);
void cmm_cascam_exit_dialog1(void);
void cmm_cascam_create_dialog2(CmmNoticeClass *para);
void cmm_cascam_exit_dialog2(void);
void cmm_cascam_create_dialog3(CmmOsdRollingClass *para);
void cmm_cascam_exit_dialog3(int style);
void cmm_cascam_create_dialog4(CmmPasswordDlgClass *para);
void cmm_cascam_exit_dialog4(void);
void cmm_cascam_create_dialog5(CmmEmailNoticeClass *para);
void cmm_cascam_exit_dialog5(CmmEmailNoticeTypeEnum type);
void cmm_cascam_create_dialog6(CmmFingerprintClass *para);
void cmm_cascam_exit_dialog6(CmmFingerTypeEnum type);
//void cmm_cascam_create_dialog7(CmmComplexDlgClass *para);
//void cmm_cascam_exit_dialog7(void);
void cmm_cascam_create_dialogx(CmmSuperMessageClass *param);
void cmm_cascam_exit_dialogx(CmmSuperMessageClass *param);

void cmm_ca_dialog_func_init(CMMOSDClass func);

char cmm_ca_is_notice(void);
unsigned char *cmm_ca_get_notice_str(void);

void cmm_cascam_check_rolling_osd(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
void cmm_cascam_get_rolling_times(unsigned int *times, int style);
int cmm_cascam_check_lock_service_status(void);
void cmm_cascam_check_finger(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
void cmm_cascam_resume_finger(void);

#endif 
