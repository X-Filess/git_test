/**
 *
 * @file        cascam_osd.h
 * @brief
 * @version     1.1.0
 * @date        08/25/2015 09:47:49 AM
 * @author      B.Z., 88156088@nationalchip.com
 *
 *
**/

typedef struct _CASCAMOsdRectClass{
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
}CASCAMOsdRectClass;

typedef struct _CASCAMOsdColorClass{
    unsigned int rgb;//0xxxrrggbb
    unsigned char color_index;// for 8bpp
}CASCAMOsdColorClass;

typedef enum _CASCAMOsdTextTypeEnum{
    CASCAM_STATIC,
    CASCAM_ROLL_R2L,
    CASCAM_ROLL_L2R,
    CASCAM_LONGER_R2L,
    CASCAM_ROLL_B2U,
    CASCAM_ROLL_U2B,
    CASCAM_AUTOMATIC,
}CASCAMOsdTextTypeEnum;

typedef enum _CASCAMOsdDialogExitType{
    CASCAM_EXIT_NONE,
    CASCAM_EXIT_YES,
    CASCAM_EXIT_OK,
    CASCAM_EXIT_NO,
}CASCAMOsdDialogExitType;

typedef enum _CASCAMOsdNoticeTypeEnum{
    CASCAM_DIALOG,
    CASCAM_NORMAL_NOTICE,//for rolling text
    CASCAM_EMAIL,// for new email notice
    CASCAM_FINGER,
    CASCAM_MESSAGE,
}CASCAMOsdNoticeTypeEnum;

typedef enum _CASCAMOsdDialogTypeEnum{
    CHOSE_YES_NO,
    CHOSE_OK,
    INPUT_PASSWORD,
    ONLY_NOTICE,
    COMPLEX_MODE,
}CASCAMOsdDialogTypeEnum;

typedef enum _CASCAMEmailNoticeTypeEnum{
    CASCAM_EMAIL_RECEIVE_NEW,
    CASCAM_EMAIL_BOX_FULL,
}CASCAMEmailNoticeTypeEnum;

typedef enum _CASCAMOsdAlignEnum{
    ALIGN_NONE,
    LEFT_TOP,
    LEFT_MIDDLE,
    LEFT_DOWN,
    MIDDLE_TOP,
    MIDDLE_MIDDLE,
    MIDDLE_DOWN,
    RIGHT_TOP,
    RIGHT_MIDDLE,
    RIGHT_DOWN,
}CASCAMOsdAlignEnum;

typedef enum _CASCAMOsdFingerTypeEnum{
    CASCAM_STRING,
    CASCAM_QRCODE,
}CASCAMOsdFingerTypeEnum;

typedef int (*CASCAMOsdExitFunc)(CASCAMOsdDialogExitType ret);

typedef struct _CASCAMOsdClass{
    CASCAMOsdNoticeTypeEnum notice_type;
    CASCAMOsdRectClass rect;
    CASCAMOsdColorClass font_color;
    CASCAMOsdColorClass back_color;
    union{
        struct{
            CASCAMOsdDialogTypeEnum type;
            char *title;
            int  title_x_offset;
            CASCAMOsdAlignEnum  title_align;
            char *content1;
            int  content1_x_offset;
            CASCAMOsdAlignEnum  content1_align;
            char *content2;
            int  content2_x_offset;
            CASCAMOsdAlignEnum  content2_align;
            char *content3;
            int  content3_x_offset;
            CASCAMOsdAlignEnum  content3_align;
            char *content4;
            int  content4_x_offset;
            CASCAMOsdAlignEnum  content4_align;

            char *content_long;// one line is not enough
            int  content_long_x_offset;
            CASCAMOsdAlignEnum  content_long_align;

            // complex
            char complex_type; // 0: none; 1: password; 2: choice; 3: edit; 4:text;
            char *str_left; // text
            char *str_right; // rule:"str1,str2,str3...." for choice;  initialize display for edit
            // clean flag
            char clean_flag; // 1: only clean the dialog
        }pop_dialog;

        struct{
            CASCAMOsdTextTypeEnum type;
            char *content;
            CASCAMOsdAlignEnum  align;
            char position;// top = 0; bottom = 1;
            char clean_flag; // 1: only clean the notice
        }text;

        struct{
            CASCAMEmailNoticeTypeEnum mail_state;// is a flag
            char flicker;// 1: flicker; 0:static
            char clean_flag;// 1: only clean the mail notice
        }mail;

        struct{
            CASCAMOsdFingerTypeEnum type;
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
            char clean_flag;// 1: only clean the finger
        }fingerprint;

        struct{
            char main_id;
            char message_id;// define by cas
            char *message_data;// private struct
            char flicker;// 1: flicker; 0: static
            char clean_flag;//1: clean status
        }message;

    }property;

    unsigned int duration;//seconds
    unsigned int repeat_times;// counts

    CASCAMOsdExitFunc exit;
    CASCAMOsdExitFunc timeout;
}CASCAMOsdClass;

// for string transfer
typedef struct _CASCAMOsdTransferClass{
    char *in_str;
    char *out_str;
}CASCAMOsdTransferClass;


int cascam_osd_pop_dialog1(void *param);

void cascam_osd_exit_dialog1();

int cascam_osd_pop_dialog2(void *param);

void cascam_osd_exit_dialog2();

int cascam_osd_pop_dialog3(void *param);

void cascam_osd_exit_dialog3();

int cascam_osd_pop_dialog4(void *param);

void cascam_osd_exit_dialog4();

int cascam_osd_pop_dialog5(void *param);

void cascam_osd_exit_dialog5();

int cascam_osd_pop_dialog6(void *param);

void cascam_osd_exit_dialog6(int style);

int cascam_osd_pop_dialog9(void *param);

void cascam_osd_exit_dialog9(void *param);

int cascam_osd_pop_dialog10(void *param);

void cascam_osd_exit_dialog10(void);

int cascam_osd_pop_dialog11(void *param);

void cascam_osd_exit_dialog11(void *param);

int cascam_osd_pop_dialog12(void *info);

void cascam_osd_exit_dialog12(void *info);

int cascam_osd_change_to_utf8(char * in_str, unsigned int in_len, char *out_str, unsigned int *out_len);

int cascam_osd_send_message(int type, void* para);

int cascam_module_osd_draw(void *para);

int cascam_module_osd_get_rolling_times(unsigned int *times, int style);

int cascam_osd_get_rolling_times(unsigned int *times, int style);

//int cascam_module_osd_rolling_times(void *data);

int cascam_modle_osd_exec(CASCAMOsdClass *para);

int cascam_osd_get_multi_language(char* in_str, char** out_str);

int cascam_module_osd_transfer_string(CASCAMOsdTransferClass *para);

int cascam_module_osd_transfer_message_exec(void *data);
