/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cascam_osd_adapter.c
* Author    :	B.Z.
* Project   :	CA Module
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.08.25		   B.Z.			  Creation
*****************************************************************************/
#include "gdi_core.h"
#include "gxbus.h"
#include "app_cascam_pop.h"
#include "app_msg.h"
#include "../include/cascam_public.h"
#include "../include/cascam_osd.h"

static char *thiz_alignment[] = {
    NULL, //ALIGN_NONE
    "left|top",//LEFT_TOP
    "left|vcentre",//LEFT_MIDDLE
    "left|bottom",//LEFT_DOWN
    "hcentre|top",//MIDDLE_TOP
    "hcentre|vcentre",//MIDDLE_MIDDLE
    "hcentre|bottom",//MIDDLE_DOWN
    "right|top",//RIGHT_TOP
    "right|vcentre",//RIGHT_MIDDLE
    "right|bottom",//RIGHT_DOWN
};

static char *thiz_text_mode[] = {
    "static",
    "roll_r2l",
    "roll_l2r",
    "fix_r2l",
    "roll_b2t",
    "roll_t2b",
    "automatic",
};
// type1
/*****************************************
 * ------------------------------------- *
 * |             Title                 | *
 * ------------------------------------- *
 * ------------------------------------- *
 * |                                   | *
 * |             CONTENT               | *
 * |                                   | *
 * ------------------------------------- *
 *                                       *
 * -------------           ------------- *
 * |     Y     |           |     N     | *
 * -------------           ------------- *
 * ***************************************/
int cascam_osd_pop_dialog1(void *param)
{
    int ret = 0;
    CmmDialogClass para = {0};
    CASCAMOsdClass *p = NULL;

    if(NULL == param)
    {
        return -1;
    }
    p = (CASCAMOsdClass *)param;
    para.type = DIALOG_YES_NO;
    para.title.state = 1;
    para.title.str = p->property.pop_dialog.title;
    para.title.alignment = thiz_alignment[p->property.pop_dialog.title_align];
    para.content_long.state = 1;
    para.content_long.str = p->property.pop_dialog.content_long;

    cmm_cascam_create_dialog1(&para);
    return ret;
}

void cascam_osd_exit_dialog1(void)
{
    cmm_cascam_exit_dialog1();
}

// type2
/*****************************************
 * ------------------------------------- *
 * |             Title                 | *
 * ------------------------------------- *
 * ------------------------------------- *
 * |             content1              | *
 * |             content2              | *
 * |             content3              | *
 * |             content4              | *
 * ------------------------------------- *
 *                                       *
 * -------------           ------------- *
 * |     Y     |           |     N     | *
 * -------------           ------------- *
 * ***************************************/
int cascam_osd_pop_dialog2(void *param)
{
    int ret = 0;
    CmmDialogClass para = {0};
    CASCAMOsdClass *p = NULL;

    if(NULL == param)
    {
        return -1;
    }
    p = (CASCAMOsdClass *)param;
    para.type = DIALOG_YES_NO;
    if(p->property.pop_dialog.title)
    {
        para.title.state = 1;
        para.title.str = p->property.pop_dialog.title;
        para.title.alignment = thiz_alignment[p->property.pop_dialog.title_align];
    }
    if(p->property.pop_dialog.content1)
    {
        para.content1.state = 1;
        para.content1.str = p->property.pop_dialog.content1;
        para.content1.alignment = thiz_alignment[p->property.pop_dialog.content1_align];
    }
    if(p->property.pop_dialog.content2)
    {
        para.content2.state = 1;
        para.content2.str = p->property.pop_dialog.content2;
        para.content2.alignment = thiz_alignment[p->property.pop_dialog.content2_align];
    }
    if(p->property.pop_dialog.content3)
    {
        para.content3.state = 1;
        para.content3.str = p->property.pop_dialog.content3;
        para.content3.alignment = thiz_alignment[p->property.pop_dialog.content3_align];
    }
    if(p->property.pop_dialog.content4)
    {
        para.content4.state = 1;
        para.content4.str = p->property.pop_dialog.content4;
        para.content4.alignment = thiz_alignment[p->property.pop_dialog.content4_align];
    }
    para.exit_cb = (ExitFunc)(p->exit);
    
    cmm_cascam_create_dialog1(&para);

    return ret;
}

void cascam_osd_exit_dialog2(void)
{
    cmm_cascam_exit_dialog1();
}

// type3
/*****************************************          *****************************************
 * ------------------------------------- *          * ------------------------------------- *
 * |             Title                 | *          * |             Title                 | *
 * ------------------------------------- *          * ------------------------------------- *
 * ------------------------------------- *          * ------------------------------------- *
 * |                                   | *          * |             CONTENT1              | *
 * |             CONTENT               | *          * |             CONTENT2              | *
 * |                                   | *          * |             CONTENT3              | *
 * |                                   | *          * |             CONTENT4              | *
 * ------------------------------------- *          * ------------------------------------- *
 *                                       *          *                                       *
 *             -------------             *          *             -------------             *
 *             |    OK     |             *          *             |    OK     |             *
 *             -------------             *          *             -------------             *
 * ***************************************          * ***************************************/
int cascam_osd_pop_dialog3(void *param)
{
    int ret = 0;
    CmmDialogClass para = {0};
    CASCAMOsdClass *p = NULL;

    if(NULL == param)
    {
        return -1;
    }
    p = (CASCAMOsdClass *)param;
    para.type = DIALOG_OK;
    if(p->property.pop_dialog.title)
    {
        para.title.state = 1;
        para.title.str = p->property.pop_dialog.title;
        para.title.alignment = thiz_alignment[p->property.pop_dialog.title_align];
    }
    if(p->property.pop_dialog.content1)
    {
        para.content1.state = 1;
        para.content1.str = p->property.pop_dialog.content1;
        para.content1.alignment = thiz_alignment[p->property.pop_dialog.content1_align];
    }
    if(p->property.pop_dialog.content2)
    {
        para.content2.state = 1;
        para.content2.str = p->property.pop_dialog.content2;
        para.content2.alignment = thiz_alignment[p->property.pop_dialog.content2_align];
    }
    if(p->property.pop_dialog.content3)
    {
        para.content3.state = 1;
        para.content3.str = p->property.pop_dialog.content3;
        para.content3.alignment = thiz_alignment[p->property.pop_dialog.content3_align];
    }
    if(p->property.pop_dialog.content4)
    {
        para.content4.state = 1;
        para.content4.str = p->property.pop_dialog.content4;
        para.content4.alignment = thiz_alignment[p->property.pop_dialog.content4_align];
    }

    if(p->property.pop_dialog.content_long)
    {
        para.content_long.state = 1;
        para.content_long.str = p->property.pop_dialog.content_long;
    }

    cmm_cascam_create_dialog1(&para);

    return ret;
}

void cascam_osd_exit_dialog3(void)
{
    cmm_cascam_exit_dialog1();
}

// type4
/*****************************************
 * ------------------------------------- *
 * |                                   | *
 * |                                   | *
 * |                                   | *
 * |             Notice                | *
 * |                                   | *
 * |                                   | *
 * |                                   | *
 * |                                   | *
 * ------------------------------------- *
 *                                       *
 * ***************************************/
int cascam_osd_pop_dialog4(void *param)
{
    int ret = 0;
    CmmNoticeClass para = {0};
    CASCAMOsdClass *p = NULL;

    if(NULL == param)
    {
        return -1;
    }
    p = (CASCAMOsdClass *)param;
    para.x = p->rect.x;
    para.y = p->rect.y;
    para.str = p->property.pop_dialog.content_long;
    //para.content_long.state = 1;
    //para.content_long.str = p->property.pop_dialog.content_long;

    cmm_cascam_create_dialog2(&para);

    return ret;
}

void cascam_osd_exit_dialog4(void)
{
    cmm_cascam_exit_dialog2();
}

// type5
/*****************************************
 * ------------------------------------- *
 * |             Title                 | *
 * ------------------------------------- *
 * ------------------------------------- *
 * |             content1              | *
 * |             content2              | *
 * ------------------------------------- *
 * ------------------------------------- *
 * |            * * * * * *            | *
 * ------------------------------------- *
 * -------------           ------------- *
 * |     Y     |           |     N     | *
 * -------------           ------------- *
 * ***************************************/
int cascam_osd_pop_dialog5(void *param)
{
    int ret = 0;
    CmmPasswordDlgClass para = {{0},};
    CASCAMOsdClass *p = NULL;

    if(NULL == param)
    {
        return -1;
    }
    p = (CASCAMOsdClass *)param;
    if(p->property.pop_dialog.title)
    {
        para.title.state = 1;
        para.title.str = p->property.pop_dialog.title;
        para.title.alignment = thiz_alignment[p->property.pop_dialog.title_align];
    }
    if(p->property.pop_dialog.content1)
    {
        para.content1.state = 1;
        para.content1.str = p->property.pop_dialog.content1;
        para.content1.alignment = thiz_alignment[p->property.pop_dialog.content1_align];
    }
    if(p->property.pop_dialog.content2)
    {
        para.content2.state = 1;
        para.content2.str = p->property.pop_dialog.content2;
        para.content2.alignment = thiz_alignment[p->property.pop_dialog.content2_align];
    }
    if(p->property.pop_dialog.content3)
    {
        para.content3.state = 1;
        para.content3.str = p->property.pop_dialog.content3;
        para.content3.alignment = thiz_alignment[p->property.pop_dialog.content3_align];
    }
    para.exit_cb = (ExitFunc)p->exit;
    cmm_cascam_create_dialog4(&para);

    return ret;
}

void cascam_osd_exit_dialog5(void)
{
    cmm_cascam_exit_dialog4();
}

// type6
/*****************************************   *****************************************
 * ------------------------------------- *   *                                       *
 * |              <----                | *   *                                       *
 * ------------------------------------- *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   * ------------------------------------- *
 *                                       *   * |              <----                | *
 *                                       *   * ------------------------------------- *
 * ***************************************   *****************************************/ 
/*****************************************   *****************************************
 * ------------------------------------- *   *                                       *
 * |              ---->                | *   *                                       *
 * ------------------------------------- *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   * ------------------------------------- *
 *                                       *   * |              ---->                | *
 *                                       *   * ------------------------------------- *
 * ***************************************   *****************************************/ 


int cascam_osd_pop_dialog6(void *param)
{
    int ret = 0;
    CmmOsdRollingClass para = {0};
    CASCAMOsdClass *p = NULL;

    if(NULL == param)
    {
        return -1;
    }
    p = (CASCAMOsdClass *)param;
    para.x = p->rect.x;
    para.y = p->rect.y;
    para.back_color = p->back_color.rgb;
    para.fore_color = p->font_color.rgb;
    para.str = p->property.text.content;
    para.direction = thiz_text_mode[p->property.text.type];
    para.duration = p->duration;
    para.repeat_times = p->repeat_times;
    para.pos = p->property.text.position;
    cmm_cascam_create_dialog3(&para);

    return ret;
}

void cascam_osd_exit_dialog6(int style)
{
    cmm_cascam_exit_dialog3(style);
}

// type7
/*****************************************   *****************************************
 * -----                                 *   *                                 ----- *
 * |   |                                 *   *                                 |   | *
 * | ^ |                                 *   *                                 | ^ | *
 * | | |                                 *   *                                 | | | *
 * | | |                                 *   *                                 | | | *
 * | | |                                 *   *                                 | | | *
 * |   |                                 *   *                                 |   | *
 * |   |                                 *   *                                 |   | *
 * |   |                                 *   *                                 |   | *
 * |   |                                 *   *                                 |   | *
 * -----                                 *   *                                 ----- *
 * ***************************************   *****************************************/ 
int cascam_osd_pop_dialog7(void *param)
{
    int ret = 0;

    return ret;
}

// type8
/*****************************************   *****************************************
 * -----                                 *   *                                 ----- *
 * |   |                                 *   *                                 |   | *
 * |   |                                 *   *                                 |   | *
 * | | |                                 *   *                                 | | | *
 * | | |                                 *   *                                 | | | *
 * | | |                                 *   *                                 | | | *
 * | v |                                 *   *                                 | v | *
 * |   |                                 *   *                                 |   | *
 * |   |                                 *   *                                 |   | *
 * |   |                                 *   *                                 |   | *
 * -----                                 *   *                                 ----- *
 * ***************************************   *****************************************/ 
int cascam_osd_pop_dialog8(void *param)
{
    int ret = 0;

    return ret;
}

/*****************************************   *****************************************
 *                              ---      *   *                                       *
 *                              | |      *   *                                       *
 *                              ---      *   *                               ---     *
 *                                       *   *                               | |     *
 *                                       *   *                               ---     *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 *                                       *   *                                       *
 * ***************************************   *****************************************/ 

int cascam_osd_pop_dialog9(void *param)// email tips
{
    int ret = 0;
    CmmEmailNoticeClass para = {0};
    CASCAMOsdClass *p = NULL;

    if(NULL == param)
    {
        return -1;
    }
    p = (CASCAMOsdClass *)param;
    para.type = p->property.mail.mail_state;
    para.flicker = p->property.mail.flicker;
    cmm_cascam_create_dialog5(&para);

    return ret;
}

void cascam_osd_exit_dialog9(void *param)
{
    CmmEmailNoticeTypeEnum type = 0;

    if(NULL == param)
    {
        return;
    }
    type = *(CASCAMEmailNoticeTypeEnum *)param;
    cmm_cascam_exit_dialog5(type);
}
// type10
/*****************************************          *****************************************
 * ------------------------------------- *          * ------------------------------------- *
 * |             Title                 | *          * |             Title                 | *
 * ------------------------------------- *          * ------------------------------------- *
 * ------------------------------------- *          * ------------------------------------- *
 * |             content1              | *          * |             content1              | *
 * |             content2              | *          * |             content2              | *
 * |             content3              | *          * |             content3              | *
 * |             content4              | *          * |             content4              | *
 * ------------------------------------- *          * ------------------------------------- *
 * ------------------------------------- *          * ------------------------------------- *
 * |            * * * * * *            | *          * |  content5            <          > | *
 * ------------------------------------- *          * ------------------------------------- *
 * -------------           ------------- *          * -------------           ------------- *
 * |     Y     |           |     N     | *          * |     Y     |           |     N     | *
 * -------------           ------------- *          * -------------           ------------- *
 * ***************************************          * ***************************************/
int cascam_osd_pop_dialog10(void *param)
{
    int ret = 0;
    CmmComplexDlgClass para;
    CASCAMOsdClass *p = NULL;

    if(NULL == param)
    {
        return -1;
    }

    memset(&para, 0, sizeof(CmmComplexDlgClass));
    p = (CASCAMOsdClass *)param;
    if(p->property.pop_dialog.title)
    {
        para.title.state = 1;
        para.title.str = p->property.pop_dialog.title;
        para.title.alignment = thiz_alignment[p->property.pop_dialog.title_align];
    }
    if(p->property.pop_dialog.content1)
    {
        para.content1.state = 1;
        para.content1.str = p->property.pop_dialog.content1;
        para.content1.alignment = thiz_alignment[p->property.pop_dialog.content1_align];
    }
    if(p->property.pop_dialog.content2)
    {
        para.content2.state = 1;
        para.content2.str = p->property.pop_dialog.content2;
        para.content2.alignment = thiz_alignment[p->property.pop_dialog.content2_align];
    }
    if(p->property.pop_dialog.content3)
    {
        para.content3.state = 1;
        para.content3.str = p->property.pop_dialog.content3;
        para.content3.alignment = thiz_alignment[p->property.pop_dialog.content3_align];
    }
    if(p->property.pop_dialog.content4)
    {
        para.content4.state = 1;
        para.content4.str = p->property.pop_dialog.content4;
        para.content4.alignment = thiz_alignment[p->property.pop_dialog.content4_align];
    }
    if(p->exit)
    {
        para.exit_cb = (ExitFunc)p->exit;
    }
    if(p->property.pop_dialog.complex_type > 0)
    {
        if(p->property.pop_dialog.str_left)
        {
            para.widget_ex.str_l = p->property.pop_dialog.str_left;
        }
        if(p->property.pop_dialog.str_right)
        {
            para.widget_ex.str_r = p->property.pop_dialog.str_right;
        }
        para.widget_ex.type = (p->property.pop_dialog.complex_type - 1);

    }
    //cmm_cascam_create_dialog7(&para);
    return ret;
}

void cascam_osd_exit_dialog10(void)
{
    //cmm_cascam_exit_dialog7();
}

// type 11
/*****************************************
 *                                       *
 *                             random    *
 *                                       *
 *                                       *
 *      random                           *
 *                                       *
 *                                       *
 *                                       *
 *                 random                *
 *                                       *
 *                                       *
 * ***************************************/

int cascam_osd_pop_dialog11(void *param)
{
    int ret = 0;
    CmmFingerprintClass para = {0};
    CASCAMOsdClass *p = NULL;

    if(NULL == param)
    {
        return -1;
    }
    p = (CASCAMOsdClass *)param;
    para.type = p->property.fingerprint.type;
    para.random = p->property.fingerprint.random;
    if(p->property.fingerprint.type == CASCAM_STRING)
    {
        para.fingerContent.string.content =  p->property.fingerprint.fingerContent.string.content;
        para.type = CMM_STRING;
    }
    else
    {
        para.fingerContent.QRCode.row = p->property.fingerprint.fingerContent.QRCode.row;
        para.fingerContent.QRCode.column = p->property.fingerprint.fingerContent.QRCode.column;
        para.type = CMM_QRCODE;
    }
    cmm_cascam_create_dialog6(&para);

    return ret;
}

void cascam_osd_exit_dialog11(void *param)
{
    CmmFingerTypeEnum type = 0;

    if(NULL == param)
    {
        return;
    }
    type = *(CASCAMOsdFingerTypeEnum *)param;
    cmm_cascam_exit_dialog6(type);
}

// message turnel
int cascam_osd_pop_dialog12(void *info)
{
    int ret = 0;
    CASCAMOsdClass *p = NULL;
    CmmSuperMessageClass param = {0};

    if(NULL == info)
    {
        return -1;
    }
    p = (CASCAMOsdClass *)info;
    param.main_id = p->property.message.main_id;
    param.sub_id = p->property.message.message_id;
    param.message_data = p->property.message.message_data;

    cmm_cascam_create_dialogx(&param);

    return ret;
}

void cascam_osd_exit_dialog12(void *info)
{
    CmmSuperMessageClass param = {0};
    CASCAMOsdClass *p = NULL;

    if(NULL == info)
    {
        return;
    }
    p = (CASCAMOsdClass *)info;
    param.main_id = p->property.message.main_id;
    param.sub_id = p->property.message.message_id;
    param.message_data = p->property.message.message_data;

    cmm_cascam_exit_dialogx(&param);
}


int cascam_osd_change_to_utf8(char * in_str, unsigned int in_len, char *out_str, unsigned int *out_len)
{
    unsigned char *pstr = NULL;
    unsigned int size = 0;
    int ret = 0;

    if((NULL == in_str) || (0 == in_len) || (NULL == out_str) || (NULL == out_len) || (0 == *out_len))
    {
        return -1;
    }

    ret = gxgdi_iconv((const unsigned char *) in_str, &pstr, in_len, &size, NULL);
    if(ret)
    {
        return -1;
    }
    size = size > *out_len ? *out_len:size;
    memcpy(out_str, pstr, size);
    *out_len = size;
    GxCore_Free(pstr);
    return ret;
}

int cascam_osd_get_multi_language(char* in_str, char** out_str)
{
    if((NULL == in_str) || (0 == strlen(in_str)) || (NULL == out_str))
    {
        return -1;
    }
    *out_str = (char*)GXGDI_I18N_String((const char*)in_str);
    return 0;
}

int cascam_osd_get_rolling_times(unsigned int *times, int style)
{
    int ret  = 0;
    cmm_cascam_get_rolling_times(times, style);
    return ret;
}

int cascam_osd_send_message(int type, void *para)
{
#if CASCAM_SUPPORT
    GxMessage *msg = NULL;

	CASCAMOSDEventClass *dialog_para;
	msg = GxBus_MessageNew(GXMSG_CASCAM_EVENT);
    if(NULL == msg)
    {
        return -1;
    }
	dialog_para = GxBus_GetMsgPropertyPtr(msg, CASCAMOSDEventClass);
    if(NULL == dialog_para)
    {
        return -1;
    }
    dialog_para->type = type;
    dialog_para->data = para;
	//GxBus_MessageSendWait(msg);
	GxBus_MessageSend(msg);
	//GxBus_MessageFree(msg);
#endif
    return 0;
}

#if 0
int cascam_osd_send_message(CASCAMOsdClass *para)
{
    GxMessage *msg = NULL;

	CASCAMOSDEventClass *dialog_para;
	msg = GxBus_MessageNew(GXMSG_CASCAM_EVENT);
    if(NULL == msg)
    {
        return -1;
    }
	dialog_para = GxBus_GetMsgPropertyPtr(msg, CASCAMOSDEventClass);
    if(NULL == dialog_para)
    {
        return -1;
    }
    dialog_para->type = 0;
    dialog_para->data = para;
	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);
    return 0;
}

int cascam_osd_get_rolling_times(int *rolling_times)
{
    GxMessage *msg = NULL;

	CASCAMOSDEventClass *dialog_para;
	msg = GxBus_MessageNew(GXMSG_CASCAM_EVENT);
    if(NULL == msg)
    {
        return -1;
    }
	dialog_para = GxBus_GetMsgPropertyPtr(msg, CASCAMOSDEventClass);
    if(NULL == dialog_para)
    {
        return -1;
    }
    dialog_para->type = 1;
    dialog_para->data = rolling_times;
	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);
    return 0;
}
#endif
