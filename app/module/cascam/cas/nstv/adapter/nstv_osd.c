/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_osd.c
* Author    :   B.Z.
* Project   :	GoXceed
* Type      :   CA Module
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
VERSION	    Date			  AUTHOR         Description
1.0	      2015.09.01		   B.Z.			  creation
*****************************************************************************/
#include "../../../include/cascam_osd.h"
#include "../../../include/cascam_os.h"
#include "../../../include/cas/cascam_nstv.h"
#include "../../../include/cascam_types.h"
#include "../include/porting/CDCASS.h"
#include "../include/porting/cd_api.h"
#include "../include/nstv_platform.h"

#ifndef NULL
#define NULL (void*)(0)
#endif
typedef enum _NSTVOsdDisplayFlagEnum{
    DISPLAY_WITH_OK,
    DISPLAY_ALLWAYS,
    DISPLAY_CLEAN,
}NSTVOsdDisplayFlagEnum;

typedef struct _NSTVOsdErrorMessageClass{
    unsigned int error_code;
    char display_flag;
    char *strings;
}NSTVOsdErrorMessageClass;

static NSTVOsdErrorMessageClass thiz_error_table[] = {
    {0x00,         DISPLAY_CLEAN,        "Clean current display"         },
    {0x01,         DISPLAY_ALLWAYS,      NSTV_OSD_BADCARD                },
    {0x02,         DISPLAY_ALLWAYS,      NSTV_OSD_EXPICARD               },
    {0x03,         DISPLAY_ALLWAYS,      NSTV_OSD_INSERTCARD             },
    {0x04,         DISPLAY_ALLWAYS,      NSTV_OSD_NOOPER                 },
    {0x05,         DISPLAY_ALLWAYS,      NSTV_OSD_BLACKOUT               },
    {0x06,         DISPLAY_ALLWAYS,      NSTV_OSD_OUTWORKTIME            },
    {0x07,         DISPLAY_ALLWAYS,      NSTV_OSD_WATCHLEVEL             },
    {0x08,         DISPLAY_ALLWAYS,      NSTV_OSD_PAIRING                },
    {0x09,         DISPLAY_ALLWAYS,      NSTV_OSD_NOENTITLE              },
    {0x0a,         DISPLAY_ALLWAYS,      NSTV_OSD_DECRYPTFAIL            },
    {0x0b,         DISPLAY_ALLWAYS,      NSTV_OSD_NOMONEY                },
    {0x0c,         DISPLAY_ALLWAYS,      NSTV_OSD_ERRREGION              },
    {0x0d,         DISPLAY_ALLWAYS,      NSTV_OSD_NEEDFEED               },
    {0x0e,         DISPLAY_ALLWAYS,      NSTV_OSD_ERRCARD                },
    {0x0f,         DISPLAY_ALLWAYS,      NSTV_OSD_UPDATE                 },
    {0x10,         DISPLAY_ALLWAYS,      NSTV_OSD_LOWCARDVER             },
    {0x11,         DISPLAY_ALLWAYS,      NSTV_OSD_VIEWLOCK               },
    {0x12,         DISPLAY_ALLWAYS,      NSTV_OSD_MAXRESTART             },
    {0x13,         DISPLAY_ALLWAYS,      NSTV_OSD_FREEZE                 },
    {0x14,         DISPLAY_ALLWAYS,      NSTV_OSD_CALLBACK               },
    {0x15,         DISPLAY_ALLWAYS,      NSTV_OSD_CURTAIN                },
    {0x16,         DISPLAY_ALLWAYS,      NSTV_OSD_CARDTESTSTART          },
    {0x17,         DISPLAY_ALLWAYS,      NSTV_OSD_CARDTESTFAILD          },
    {0x18,         DISPLAY_ALLWAYS,      NSTV_OSD_CARDTESTSUCC           },
    {0x19,         DISPLAY_ALLWAYS,      NSTV_OSD_NOCALIBOPER            },
    {0x20,         DISPLAY_ALLWAYS,      NSTV_OSD_STBLOCKED              },
    {0x21,         DISPLAY_ALLWAYS,      NSTV_OSD_STBFREEZE              },
    {0x30,         DISPLAY_ALLWAYS,      NSTV_OSD_UNSUPPORTDEVICE        },
};

#define MAX_DISPLAY_SIZE (180*3 + 1)
typedef struct _NSTVOsdScreenDisplayInfoClass{
    char         content[MAX_DISPLAY_SIZE];// max len is 180 bytes
    char         org_content[MAX_DISPLAY_SIZE];// max len is 180 bytes
    //char         position;// 0: top; 1: bottom; 2: fullscreen; 3: halfscreen
}NSTVOsdScreenDisplayInfoClass;

#define MAX_DISPLAY_STYLE  4
static  NSTVOsdScreenDisplayInfoClass  thiz_screen_display_info[MAX_DISPLAY_STYLE];// top, bottom, half-screen, full-screen
extern int nstv_osd_get_program_status(void);

#define  _COUNT (sizeof(thiz_error_table)/sizeof(NSTVOsdErrorMessageClass))
static int _get_error_info(unsigned int err_code, char* display_flag, char** display_str)
{
    int i = 0;
    if(NULL == display_str)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    for(i = 0; i < _COUNT; i++)
    {
        if(thiz_error_table[i].error_code == err_code)
        {
            *display_str = thiz_error_table[i].strings;
            *display_flag = thiz_error_table[i].display_flag;
            return 0;
        }
    }
    return -1;

}

/*****************************************************************************************/
// osd notice
// export
static unsigned char thiz_record_ecm_pid = 0;
int nstv_osd_message_notify(unsigned char ecm_pid, unsigned int error_code)
{
    char *err_str = NULL;
    CASCAMOsdClass para = {0};
    int ret = 0;
    char flag = 0;
// 20160115
    //if(0 == nstv_osd_get_program_status())
    //{// discard it
    //    return 0;
    //}

    ret = _get_error_info(error_code, &flag, &err_str);
    if(0 > ret)
    {
        return 0;//do not process
    }
    if(0 == error_code)
    {
        // clear the error notice
        if(ecm_pid == thiz_record_ecm_pid)
        {
            thiz_record_ecm_pid = 0;
        }
        else
        {// discard it
            return 0;
        }
    }
    else
    {// record the operate code
        thiz_record_ecm_pid = ecm_pid;
    }
    para.font_color.rgb  = 0x00ffffff;//white
    para.back_color.rgb  = 0x00000001;//black

    if(DISPLAY_ALLWAYS == flag)
    {
        para.notice_type = CASCAM_DIALOG;
        para.property.pop_dialog.type = ONLY_NOTICE;
        para.property.pop_dialog.title = "Notice";
        para.property.pop_dialog.content_long = err_str;
        para.property.pop_dialog.content_long_x_offset = 0;
        para.property.pop_dialog.content_long_align = MIDDLE_MIDDLE;
    }
    else if(DISPLAY_WITH_OK == flag)
    {
        para.notice_type = CASCAM_DIALOG;
        para.property.pop_dialog.type = CHOSE_OK;
        para.property.pop_dialog.title = "Notice";
        para.property.pop_dialog.content_long = err_str;
        para.property.pop_dialog.content_long_x_offset = 0;
        para.property.pop_dialog.content_long_align = MIDDLE_MIDDLE;
    }
    else if(DISPLAY_CLEAN == flag)
    {
        para.notice_type = CASCAM_DIALOG;
        para.property.pop_dialog.type = ONLY_NOTICE;
        para.property.pop_dialog.clean_flag = 1;
    }
    cascam_modle_osd_exec(&para);

    return ret;
}

/*********************************************************************************/
// osd screen, rolling message
#if ROLLING_COMPLETE_EXIT
static int thiz_rolling_complete_times[MAX_DISPLAY_STYLE] = {0, 0, 0, 0};
#endif

static char* _delete_enter(char *str, unsigned int *bytes)
{
    char *temp = NULL;
    unsigned int size = 0;
    int i = 0;
    unsigned int real_size = 0;
    if((NULL == str) ||(0 ==  (size = cascam_strlen(str))) || (NULL == bytes))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return NULL;
    }
    size = *bytes;
    temp = cascam_malloc(size + 1);
    if(NULL == temp)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return NULL;
    }

    for(i = 0; i < size; i++)
    {
        if((str[i] < 0x80))
        {
            if((str[i] == 0x0d)// '\n'
                    || (str[i] == 0x0a))// '\r'
            {
                continue;
            }
        }
        temp[real_size++] = str[i];
    }
    if(real_size > 0)
    {
        //cascam_memcpy(str, temp, real_size);
        temp[real_size] = '\0';
        *bytes = real_size+1;
    }
    else
    {
        *bytes = size;
    }
    //cascam_free(temp);
    return temp;
}

static int _record_osd_screen_data(unsigned char style, char* message, NSTVOsdScreenDisplayInfoClass *p)
{
    int ret = 0;
    unsigned int new_len = cascam_strlen(message);
    unsigned int bytes = new_len*3 + 1;
    char *utf8_str = NULL;
    char *org_temp = NULL;

    utf8_str = cascam_malloc(new_len*3 + 1);
    if(NULL == utf8_str)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
    }
    else
    {
        org_temp = _delete_enter(message, &new_len);
        if(NULL == org_temp)
        {
            cascam_memcpy(p->content, message, new_len);
            cascam_free(utf8_str);
            return ret; 
        }
        if(0 == cascam_osd_change_to_utf8(org_temp, new_len, utf8_str, &bytes))
        {
            new_len = bytes > (MAX_DISPLAY_SIZE - 1)? (MAX_DISPLAY_SIZE - 1): bytes;//
        }
        else
        {
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        }
        cascam_free(org_temp);
    }
    if(NULL == utf8_str)
    {
        cascam_memcpy(p->content, message, new_len);
    }
    else
    {
        cascam_memcpy(p->content, utf8_str, new_len);
        cascam_free(utf8_str);
    }
    return ret;
}

//
static int thiz_thread[MAX_DISPLAY_STYLE] = {0};;
static void _wait_task(void* args)
{
    unsigned int rolling_times = 0;
    int style = *(int*)args;
    CASCAMOsdClass param = {0};
    unsigned int str_len = 0;

    cascam_thead_detach();
    while(1)
    {
        if(thiz_thread[style] == 0)
        {
            break;
        }
        cascam_module_osd_get_rolling_times(&rolling_times, style);
#if ROLLING_COMPLETE_EXIT
        if(rolling_times == thiz_rolling_complete_times[style])
#else
        if(rolling_times > 0)
#endif
        {
            param.notice_type = CASCAM_NORMAL_NOTICE;
            str_len = cascam_strlen(thiz_screen_display_info[style].content);
            if(str_len == 0)
            {// delete
                break;
            }
            param.property.text.content = cascam_malloc(str_len + 1);
            if(param.property.text.content == NULL)
            {
                cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
                break;
            }
            cascam_memcpy(param.property.text.content, thiz_screen_display_info[style].content, str_len);
            param.property.text.position = style;// 0: top, 1:bottom, 2:fullscreen, 3:halfscreen
            param.property.text.type = CASCAM_ROLL_R2L;
            param.property.text.align = MIDDLE_MIDDLE;
            param.repeat_times = 0xffffffff;// repeat
            cascam_modle_osd_exec(&param);
            break;
        }
#if ROLLING_COMPLETE_EXIT
        cascam_thread_delay(200);
#else
        cascam_thread_delay(1000);
#endif
    }

#if ROLLING_COMPLETE_EXIT
    thiz_rolling_complete_times[style] = 0;
#endif
    thiz_thread[style] = 0;
}

static int _create_wait_thread(int index)
{
    if(thiz_thread[index] == 0)
    {
        static int style = 0;
        style = index;
        cascam_thread_create("screen_display",&thiz_thread[index], _wait_task, (void*)&style, 10 * 1024, 10);
    }
    return 0;
}

static int _get_thread_status(int index)
{
    if(thiz_thread[index] != 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int _screen_display_deal_with(unsigned char style, char* message)
{
    int ret = 0;
    NSTVOsdScreenDisplayInfoClass temp;
    unsigned int record_len = cascam_strlen(thiz_screen_display_info[style].org_content);
    unsigned int new_len = 0;
    unsigned int rolling_times = 0;

    if(_get_thread_status(style))
    {// thread is running
        return 1;
    }
    cascam_memset(&temp, 0, sizeof(NSTVOsdScreenDisplayInfoClass));

    _record_osd_screen_data(style, message, &temp);
    new_len = cascam_strlen(temp.content);
    if(0 == new_len)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }

    cascam_module_osd_get_rolling_times(&rolling_times, style);
#if ROLLING_COMPLETE_EXIT 
    if(rolling_times != 0xffffffff)
#else
    if(rolling_times < 1)
#endif
    {
        if((cascam_strlen(message) != record_len)
            || (cascam_strncmp(message, thiz_screen_display_info[style].org_content, record_len)))
        {
            // new osd data
            cascam_memset(&thiz_screen_display_info[style], 0, sizeof(NSTVOsdScreenDisplayInfoClass));
            cascam_memcpy(thiz_screen_display_info[style].content, temp.content, new_len);
            cascam_memcpy(thiz_screen_display_info[style].org_content, message, cascam_strlen(message));

#if ROLLING_COMPLETE_EXIT 
            thiz_rolling_complete_times[style] = (rolling_times + 1);
#endif
            _create_wait_thread(style);
            return 1;
        }
        else
        {// same, drop
            return 2;
        }
    }

    cascam_memset(&thiz_screen_display_info[style], 0, sizeof(NSTVOsdScreenDisplayInfoClass));
    cascam_memcpy(thiz_screen_display_info[style].content, temp.content, new_len);
    cascam_memcpy(thiz_screen_display_info[style].org_content, message, cascam_strlen(message));
    return ret;
}

int nstv_osd_screen_display_notify(unsigned char style, char* message)
{
    int ret = 0;
    int style_index = 0;
    int str_len = 0;

    CASCAMOsdClass param = {0};

    if((NULL == message) || (style > 4) || (style == 0))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    style_index = style - 1;

    if(_screen_display_deal_with(style_index, message) != 0)
    {
        //
        return 0;
    }

    switch(style)
    {
        case 1:// CDCA_OSD_TOP
            param.property.text.position = 0;
            param.property.text.type = CASCAM_ROLL_R2L;
            break;
        case 2:// CDCA_OSD_BOTTOM
            param.property.text.position = 1;
            param.property.text.type = CASCAM_ROLL_R2L;
            break;
        case 3:// CDCA_OSD_FULLSCREEN
            param.property.text.position = 0; //2
            param.property.text.type = CASCAM_ROLL_R2L;//CASCAM_AUTOMATIC;
            cascam_printf("\nCASCAM, CDCA_OSD_FULLSCREEN, %s, %d\n",__FUNCTION__,__LINE__);
            break;
        case 4:// CDCA_OSD_HALFSCREEN
            param.property.text.position = 0; //3
            param.property.text.type = CASCAM_ROLL_R2L;//CASCAM_AUTOMATIC;
            cascam_printf("\nCASCAM, CDCA_OSD_HALFSCREEN, %s, %d\n",__FUNCTION__,__LINE__);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
            return -1;
    }
    param.notice_type = CASCAM_NORMAL_NOTICE;
    str_len = cascam_strlen(thiz_screen_display_info[style_index].content);
    param.property.text.content = cascam_malloc(str_len + 1);
    if(param.property.text.content == NULL)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    cascam_memcpy(param.property.text.content, thiz_screen_display_info[style_index].content, str_len);
    param.property.text.align = MIDDLE_MIDDLE;
    param.repeat_times = 0xffffffff;// repeat allways
    cascam_modle_osd_exec(&param);

    return ret;
}

int nstv_osd_screen_display_hide(int style)
{
    int ret = 0;
    int index = 0;
    CASCAMOsdClass param = {0};
    switch(style)
    {
        case 1:// CDCA_OSD_TOP
            param.property.text.position = 0;
            index = 0;
            break;
        case 2:// CDCA_OSD_BOTTOM
            param.property.text.position = 1;
            index = 1;
            break;
        case 3:// CDCA_OSD_FULLSCREEN
            param.property.text.position = 0; //2
            index = 0;
            cascam_printf("\nCASCAM, CDCA_OSD_FULLSCREEN, %s, %d\n",__FUNCTION__,__LINE__);
            break;
        case 4:// CDCA_OSD_HALFSCREEN
            param.property.text.position = 0; //3
            index = 0;
            cascam_printf("\nCASCAM, CDCA_OSD_HALFSCREEN, %s, %d\n",__FUNCTION__,__LINE__);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
            return -1;
    }
    // exit the wait thread
    thiz_thread[index] = 0;
    cascam_memset(&thiz_screen_display_info[index], 0, sizeof(NSTVOsdScreenDisplayInfoClass));
    param.notice_type = CASCAM_NORMAL_NOTICE;
    param.property.text.clean_flag = 1;
    cascam_modle_osd_exec(&param);
    return ret;
}

#if NSCAS_SMC_VERSION
/******************************************************************************************/
// ippv notify
//extern CDCA_U16 CDCASTB_StopIPPVBuyDlg( CDCA_BOOL       bBuyProgram,
//                                        CDCA_U16        wEcmPid,
//                                        const CDCA_U8*  pbyPinCode,
//                                        const SCDCAIPPVPrice* pPrice );
//
static CASCAMNSTVIPPVBuyReturnClass thiz_ippv_return = {0};
static char* _ippx_exit(char buy, CASCAMNSTVIPPVBuyClass *info)
{
    char *notice_ptr = NULL;
    int ret = 0;
    unsigned char pin[6] = {0};

    if(NULL == info)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    thiz_ippv_return.status = 1;
    if(1 == buy)// buy
    {
        SCDCAIPPVPrice price = {0};
        price.m_wPrice = info->price;
        price.m_byPriceCode = info->price_code;
        pin[0] = info->pin[0] - 0x30;
        pin[1] = info->pin[1] - 0x30;
        pin[2] = info->pin[2] - 0x30;
        pin[3] = info->pin[3] - 0x30;
        pin[4] = info->pin[4] - 0x30;
        pin[5] = info->pin[5] - 0x30;

        GxCas_CdStopIppv stop;
        memset(&stop, 0, sizeof(GxCas_CdStopIppv));
        stop.buy_flag = 1;
        stop.ecm_pid = info->ecm_pid;
        memcpy(stop.pin, pin, 6);
        memcpy(&stop.price, &price, sizeof(SCDCAIPPVPrice));
        ret = GxCas_Set(GXCAS_CD_STOP_IPPV, (void *)&stop);
        switch(ret)
        {
            case CDCA_RC_OK:
				if(info->ippt_flag == 0)
                {
                    notice_ptr =  NSTV_OSD_PURCHASED;
                }
                else
                {
                    notice_ptr =  NSTV_OSD_PURCHASED_IPPT;
                }
                

                thiz_ippv_return.status = 0;
                break;
            case CDCA_RC_PIN_INVALID:
                notice_ptr = NSTV_OSD_INVALID_PIN;
                break;
            case CDCA_RC_PROG_STATUS_INVALID:
                break;
            case CDCA_RC_CARD_NO_ROOM:
                if(info->ippt_flag == 0)
                {
                    notice_ptr = NSTV_OSD_NO_ROOM;
                }
                else
                {
                    notice_ptr = NSTV_OSD_NO_ROOM_IPPT;
                }
                break;
            case CDCA_RC_DATA_NOT_FIND:
                break;
            case CDCA_RC_POINTER_INVALID:
                break;
            default:
                break;
        }
    }
    else // not buy
    {
        GxCas_CdStopIppv stop;
        memset(&stop, 0, sizeof(GxCas_CdStopIppv));
        stop.buy_flag = 0;
        stop.ecm_pid = info->ecm_pid;
        GxCas_Set(GXCAS_CD_STOP_IPPV, (void *)&stop);
        thiz_ippv_return.status = 0;
    }
    thiz_ippv_return.notice = notice_ptr;
    return (char*)&thiz_ippv_return;
}

///* IPPV节目通知 */
//extern void CDSTBCA_StartIppvBuyDlg( CDCA_U8                 byMessageType,
//                                    CDCA_U16                wEcmPid,
//                                    const SCDCAIppvBuyInfo* pIppvProgram  );

int nstv_osd_ippx_notify(unsigned char type, unsigned short ecm_pid, void* param)
{
#define  BUFFER_LEN 50
    CASCAMOsdClass para = {0};
    CASCAMNSTVIPPVNotifyClass info;
    SCDCAIppvBuyInfo*  ippv_param = NULL;
    int i = 0;
    static CASCAMNSTVIPPVNotifyClass thiz_ippv_notify = {0};

    if( NULL == param)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    ippv_param = (SCDCAIppvBuyInfo*)param;
// 20160115
//    if(0 == nstv_osd_get_program_status())
//    {// discard it
//        return 0;
//    }
    cascam_memset(&info, 0, sizeof(CASCAMNSTVIPPVNotifyClass));
    switch(type)
    {
        case 0x00:// CDCA_IPPV_FREEVIEWED_SEGMENT
            info.ippt_flag = 0;
            info.message = NSTV_OSD_IPPV_STAGE1;
            info.time.expired_date = ippv_param->m_wIPPVTime.m_wExpiredDate;
            break;
        case 0x01:// CDCA_IPPV_PAYVIEWED_SEGMEN
            info.ippt_flag = 0;
            info.message = NSTV_OSD_IPPV_STAGE2;
            info.time.expired_date = ippv_param->m_wIPPVTime.m_wExpiredDate;
            break;
        case 0x02:// CDCA_IPPT_PAYVIEWED_SEGMENT
            info.ippt_flag = 1;
            info.message = NSTV_OSD_IPPT_NOTIFY;
            info.time.interval_min = ippv_param->m_wIPPVTime.m_wIntervalMin;
            break;
    }

// TODO: 20160423
    info.product_id = ippv_param->m_dwProductID;
    info.operator_id = ippv_param->m_wTvsID;
    info.wallet_id = ippv_param->m_bySlotID;
    info.ecm_id = ecm_pid;
    info.buy_price.unit_count = ippv_param->m_byPriceNum;
    for(i = 0; i < info.buy_price.unit_count; i++)
    {
        info.buy_price.unit_info[i].price = ippv_param->m_Price[i].m_wPrice;
        info.buy_price.unit_info[i].price_code = ippv_param->m_Price[i].m_byPriceCode;
    }

    info.exit_func = _ippx_exit;
    cascam_memcpy(&thiz_ippv_notify, &info, sizeof(CASCAMNSTVIPPVNotifyClass));

    para.notice_type = CASCAM_MESSAGE;
    para.property.message.main_id = NSTV_MESSAGE_IPPV;
    para.property.message.message_data = (char*)&thiz_ippv_notify;

    cascam_modle_osd_exec(&para);
	return 0;
}

///* 隐藏IPPV对话框 */
//extern void CDSTBCA_HideIPPVDlg(CDCA_U16 wEcmPid);

static unsigned short thiz_ippx_hide_pid = 0;
int nstv_osd_ippx_hide(unsigned short ecm_pid)
{
    int ret = 0;
    CASCAMOsdClass param = {0};

    if(ecm_pid == 0x1fff)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
    }
    param.notice_type = CASCAM_MESSAGE;
    param.property.message.main_id = NSTV_MESSAGE_IPPV;
    param.property.message.clean_flag = 1;
    thiz_ippx_hide_pid = ecm_pid;
    param.property.message.message_data = (char*)&thiz_ippx_hide_pid;
    cascam_modle_osd_exec(&param);

    return ret;
}
#endif

/***********************************************************************************/
// mail
static int _mail_notify(CASCAMEmailNoticeTypeEnum type, char clean_flag, char flicker)
{
    CASCAMOsdClass param = {0};
    param.notice_type = CASCAM_EMAIL;
    param.property.mail.clean_flag = clean_flag;
    param.property.mail.mail_state = type;
    param.property.mail.flicker = flicker;
    cascam_modle_osd_exec(&param);
    return 0;
}

void nstv_osd_mail_notice_init(void)
{// reboot stb init
    if(nstv_mail_check_box_is_full() == 1)
    {
        _mail_notify(CASCAM_EMAIL_RECEIVE_NEW, 0, 1);// show full state
    }
    else if(nstv_mail_check_have_unread_mail() == 1)
    {
        _mail_notify(CASCAM_EMAIL_RECEIVE_NEW, 0, 0);// show read state
    }
}

void nstv_osd_mail_notify(unsigned char show_flag, unsigned int mail_id)
{
    switch(show_flag)
    {
        case 0x00:// CDCA_Email_IconHide
            _mail_notify(CASCAM_EMAIL_RECEIVE_NEW, 1, 0);// hide
            break;
        case 0x01:// CDCA_Email_New
            _mail_notify(CASCAM_EMAIL_RECEIVE_NEW, 0, 0);// show
            break;
        case 0x02:// CDCA_Email_SpaceExhaust
            _mail_notify(CASCAM_EMAIL_RECEIVE_NEW, 0, 1);// flicker
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
            break;
    }
}

/*******************************************************************************************/
// entitlement notify
// void CDSTBCA_EntitleChanged(CDCA_U16 wTvsID)
static NSTVEntitlementMessageClass thiz_entitlement_notify = {0};
int nstv_osd_entitlement_notify(unsigned short operator_id)
{
    int ret = 0;
    CASCAMOsdClass para = {0};

    para.notice_type = CASCAM_MESSAGE;
    para.property.message.main_id = NSTV_MESSAGE_ENTITLE;
    thiz_entitlement_notify.operator_id = operator_id;
    para.property.message.message_data = (char*)&thiz_entitlement_notify;

    cascam_modle_osd_exec(&para);
    return ret;
}

/*******************************************************************************************/
// detitle notify
// extern void CDSTBCA_DetitleReceived( CDCA_U8 bstatus );
// CDCASTB_GetDetitleReaded(CDCA_U16 wTvsID)
static int _detitle_notify(char id, char clean_flag, char flicker)
{
    CASCAMOsdClass param = {0};
    param.notice_type = CASCAM_MESSAGE;
    param.property.message.main_id = NSTV_MESSAGE_DETITLE;
    param.property.message.clean_flag = clean_flag;
    param.property.message.message_id = id;
    param.property.message.flicker = flicker;
    cascam_modle_osd_exec(&param);
    return 0;
}

void nstv_osd_detitle_notify(unsigned char detitle_status)
{
    switch(detitle_status)
    {
        case 0x00:// CDCA_Detitle_All_Read
            _detitle_notify(NSTV_DETITLE_RECEIVE_NEW, 1, 0);// hide
            break;
        case 0x01:// CDCA_Detitle_Received
            _detitle_notify(NSTV_DETITLE_RECEIVE_NEW, 0, 0);// show
            break;
        case 0x02:// CDCA_Detitle_Space_Small
            _detitle_notify(NSTV_DETITLE_SPACE_FULL, 0, 1);// flicker
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d, %d\n", __FUNCTION__,__LINE__, detitle_status);
            break;
    }
}

/*******************************************************************************************/
// curtain notify
// extern void  CDSTBCA_ShowCurtainNotify( CDCA_U16 wEcmPID,CDCA_U16  wCurtainCode);
// 
static int _curtain_notify(char id, char clean_flag, char *message)
{
    CASCAMOsdClass param = {0};
    param.notice_type = CASCAM_MESSAGE;
    param.property.message.main_id = NSTV_MESSAGE_CURTAIN;
    param.property.message.clean_flag = clean_flag;
    param.property.message.message_id = id;
    param.property.message.message_data = message;
    cascam_modle_osd_exec(&param);
    return 0;
}

void nstv_osd_curtain_notify(unsigned short ecm_pid, unsigned short code)
{
    switch(code&0xff)
    {
        case 0x00:// CDCA_CURTAIN_CANCLE
            _curtain_notify(NSTV_CURTAIN_CANCEL, 1, NSTV_OSD_CURTAIN);// hide
            break;
        case 0x01:// CDCA_CURTAIN_OK
            _curtain_notify(NSTV_CURTAIN_OK, 0, NSTV_OSD_CURTAIN);// show
            break;
        case 0x02:// CDCA_CURTAIN_TOTTIME_ERROR
            _curtain_notify(NSTV_CURTAIN_TOTTIME_ERROR, 0, NSTV_OSD_CURTAIN);// show
            break;
        case 0x03:// CDCA_CURTAIN_WATCHTIME_ERROR
            _curtain_notify(NSTV_CURTAIN_WATCHTIME_ERROR, 0, NSTV_OSD_CURTAIN);// show
            break;
        case 0x04:// CDCA_CURTAIN_TOTCNT_ERROR
            _curtain_notify(NSTV_CURTAIN_TOTCNT_ERROR, 0, NSTV_OSD_CURTAIN);// show
            break;
        case 0x05:// CDCA_CURTAIN_ROOM_ERROR
            _curtain_notify(NSTV_CURTAIN_ROOM_ERROR, 0, NSTV_OSD_CURTAIN);// show
            break;
        case 0x06:// CDCA_CURTAIN_PARAM_ERROR
            _curtain_notify(NSTV_CURTAIN_PARAM_ERROR, 0, NSTV_OSD_CURTAIN);// show
            break;
        case 0x07:// CDCA_CURTAIN_TIME_ERROR
            _curtain_notify(NSTV_CURTAIN_TIME_ERROR, 0, NSTV_OSD_CURTAIN);// show
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
            break;
    }
}

/*******************************************************************************************/
// nstv action notify
// extern void  CDSTBCA_ActionRequestExt(CDCA_U16 wTVSID,
//                                          CDCA_U8 byActionType,
//                                          CDCA_U8 byLen,
//                                          CDCA_U8 *pbyData);
static int _action_notify(char id, char clean_flag, char *message)
{
    CASCAMOsdClass param = {0};
    param.notice_type = CASCAM_MESSAGE;
    param.property.message.main_id = NSTV_MESSAGE_ACTION;
    param.property.message.clean_flag = clean_flag;
    param.property.message.message_id = id;
    param.property.message.message_data = message;
    cascam_modle_osd_exec(&param);
    return 0;
}

//#define CDCA_ACTIONREQUEST_RESTARTSTB                     0       /* 重启机顶盒 */
//#define CDCA_ACTIONREQUEST_FREEZESTB                      1       /* 冻结机顶盒 *define*/
//#define CDCA_ACTIONREQUEST_SEARCHCHANNEL                  2       /* 重新搜索节目 */
//#define CDCA_ACTIONREQUEST_STBUPGRADE                     3       /* 机顶盒程序升级 */
//#define CDCA_ACTIONREQUEST_UNFREEZESTB                    4       /* 解除冻结机顶盒 */
//#define CDCA_ACTIONREQUEST_INITIALIZESTB                  5       /* 初始化机顶盒 */
//#define CDCA_ACTIONREQUEST_INITIALIZESTBSHOWSYSTEMINFO    6       /* 显示系统信息 */
//#define CDCA_ACTIONREQUEST_DISABLEEPGADINFO               201     /* 禁用EPG广告信息功能 */
//#define CDCA_ACTIONREQUEST_ENABLEEPGADINFO                202     /* 恢复EPG广告信息功能 */
//#define CDCA_ACTIONREQUEST_CARDINSERTFINISH               255     /* 插卡处理完成 */
//
void nstv_action_notify(unsigned short operator_pid, unsigned short code)
{
    switch(code&0xff)
    {
        case 0x00:// CDCA_ACTIONREQUEST_RESTARTSTB
            _action_notify(NSTV_ACTION_REBOOT, 0, NSTV_OSD_RESTART);
            break;
        case 0x01:// CDCA_ACTIONREQUEST_FREEZESTB
            _action_notify(NSTV_ACTION_FREEZE, 0, NSTV_OSD_STBFREEZE);
            break;
        case 0x02:// CDCA_ACTIONREQUEST_SEARCHCHANNEL
            _action_notify(NSTV_ACTION_RESEARCH, 0, NULL);
            break;
        case 0x03:// CDCA_ACTIONREQUEST_STBUPGRADE
            _action_notify(NSTV_ACTION_UPGRADE, 0, NSTV_OSD_UPGRADE_STB);
            break;
        case 0x04:// CDCA_ACTIONREQUEST_UNFREEZESTB
            _action_notify(NSTV_ACTION_UNFREEZE, 1, NULL);
            break;
        case 0x05:// CDCA_ACTIONREQUEST_INITIALIZESTB
            _action_notify(NSTV_ACTION_INIT_STB, 0, NULL);
            break;
        case 0x06:// CDCA_ACTIONREQUEST_INITIALIZESTBSHOWSYSTEMINFO
            _action_notify(NSTV_ACTION_DISPLAY_SYSTEM_INFO, 0, NULL);
            break;
        case 201:// CDCA_ACTIONREQUEST_DISABLEEPGADINFO
            _action_notify(NSTV_ACTION_DISABLE_EPG_AD, 0, NULL);
            break;
        case 202:// CDCA_ACTIONREQUEST_ENABLEEPGADINFO
            _action_notify(NSTV_ACTION_ENABLE_EPG_AD, 1, NULL);
            break;
        case 203:// CDCA_ACTIONREQUEST_CARDINSERTFINISH
            _action_notify(NSTV_ACTION_FINISH_CARD_IN, 0, NULL);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d, code_type = %d\n", __FUNCTION__,__LINE__, code&0xff);
            break;
    }
}

/*******************************************************************************************/
// card update notify
// extern void CDSTBCA_ShowProgressStrip(CDCA_U8 byProgress,CDCA_U8 byMark);
// #define CDCA_SCALE_RECEIVEPATCH   1     /* 升级数据接收中 */
// #define CDCA_SCALE_PATCHING       2     /* 智能卡升级中 */
static int _card_update_notify(char* message)
{
    CASCAMOsdClass param = {0};
    param.notice_type = CASCAM_MESSAGE;
    param.property.message.main_id = NSTV_MESSAGE_CARD_UPDATE;
    param.property.message.message_data = message;
    cascam_modle_osd_exec(&param);
    return 0;
}

static CASCAMNSTVCardUpdateClass thiz_card_update_info = {0};
void nstv_osd_card_update_notify(unsigned char mark, unsigned char progress)
{
    switch(mark)
    {
        case 0x01:// CDCA_SCALE_RECEIVEPATCH
        case 0x02:// CDCA_SCALE_PATCHING
            thiz_card_update_info.mark = mark;
            thiz_card_update_info.process = progress;
            _card_update_notify((char*)&thiz_card_update_info);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d, %d\n", __FUNCTION__,__LINE__, mark);
            break;
    }
}

/*******************************************************************************************/
// fingerprint notify
// extern void CDSTBCA_ShowFingerMessageExt(CDCA_U16 wEcmPID, char* fingerMsg );
static char thiz_finger_string[100] = {0};
int nstv_osd_fingerprint_notify(unsigned short ecm_pid, char* msg)
{
    int ret = 0;
    CASCAMOsdClass param = {0};

    // TODO: 20160420

    param.notice_type = CASCAM_FINGER;
    if(NULL == msg)
    {// clear finger display
        param.property.fingerprint.clean_flag = 1;
    }
    else
    {
        unsigned int len = cascam_strlen(msg);
        cascam_memset(thiz_finger_string, 0, sizeof(thiz_finger_string));
        len = len >= 100? 99:len;
        cascam_memcpy(thiz_finger_string, msg, len);
        param.property.fingerprint.fingerContent.string.content = thiz_finger_string;
        param.property.fingerprint.random = 1;
        param.property.fingerprint.type = CASCAM_STRING;
    }
    cascam_modle_osd_exec(&param);
    return ret;
}

// for change program
int nstv_osd_screen_display_data_clean(void)
{
    //cascam_memset(thiz_screen_display_info, 0, sizeof(thiz_screen_display_info));
    return 0;
}

static int thiz_is_matched_program = 0;
int nstv_osd_record_program_status(int is_support)
{
    int ret = 0;
    thiz_is_matched_program = is_support;
    return ret;
}

int nstv_osd_get_program_status(void)
{
    if(thiz_is_matched_program > 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


