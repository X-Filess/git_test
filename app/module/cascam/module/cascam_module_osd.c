/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cascam_module_osd.c
* Author    :	B.Z.
* Project   :	CA Module
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2015.09.22		   B.Z.			  Creation
*****************************************************************************/
#include "../include/cascam_types.h"
#include "../include/cascam_os.h"
#include "../include/cascam_osd.h"

// export
// for cascam
int cascam_modle_osd_exec(CASCAMOsdClass *para)
{
    int ret = 0;
    CASCAMOsdClass *p = NULL;
    if(NULL == para)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    p = cascam_malloc(sizeof(CASCAMOsdClass));
    if(NULL == p)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    cascam_memcpy(p, para, sizeof(CASCAMOsdClass));
    ret = cascam_osd_send_message(CASCAM_OSD_DRAW, (void*)p);
    if(ret)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        cascam_free(p);
        return -1;
    }
    return ret;
}

// for stb
int cascam_module_osd_draw(void *para)
{
    int ret = 0;
    CASCAMOsdClass *p = NULL;
    if(NULL == para)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    p = (CASCAMOsdClass*)para;
    if(CASCAM_DIALOG == p->notice_type)
    {// dialog

        if(CHOSE_YES_NO == p->property.pop_dialog.type)
        {
            if(p->property.pop_dialog.clean_flag)
            {
                cascam_osd_exit_dialog1();
                cascam_osd_exit_dialog2();
            }
            else
            {
                cascam_osd_exit_dialog4();
                if(p->property.pop_dialog.content_long)
                {
                    cascam_osd_pop_dialog1((void*)p);
                }
                else
                {
                    cascam_osd_pop_dialog2((void*)p);
                }
            }
        }
        else if(CHOSE_OK == p->property.pop_dialog.type)
        {
            if(p->property.pop_dialog.clean_flag)
            {
                cascam_osd_exit_dialog3();
            }
            else
            {
                cascam_osd_exit_dialog4();
                cascam_osd_pop_dialog3((void*)p);
            }
        }
        else if(INPUT_PASSWORD == p->property.pop_dialog.type)
        {
            if(p->property.pop_dialog.clean_flag)
            {
                cascam_osd_exit_dialog5();
            }
            else
            {
                cascam_osd_exit_dialog1();
                cascam_osd_exit_dialog2();
                cascam_osd_exit_dialog3();
                cascam_osd_pop_dialog5((void*)p);
            }
        }
        else if(ONLY_NOTICE == p->property.pop_dialog.type)
        {
            if(p->property.pop_dialog.clean_flag)
            {
                cascam_osd_exit_dialog4();
            }
            else
            {
                //cascam_osd_exit_dialog1();
                //cascam_osd_exit_dialog2();
                //cascam_osd_exit_dialog3();
                cascam_osd_pop_dialog4((void*)p);
            }
        }
        else if(COMPLEX_MODE == p->property.pop_dialog.type)
        {
            if(p->property.pop_dialog.clean_flag)
            {
                cascam_osd_exit_dialog10();
            }
            else
            {
                cascam_osd_exit_dialog1();
                cascam_osd_exit_dialog2();
                cascam_osd_exit_dialog3();
                cascam_osd_pop_dialog10((void*)p);
            }
        }
    }
    else if(CASCAM_NORMAL_NOTICE == p->notice_type)
    {// for rolling text
        if(p->property.text.clean_flag)
        {
            cascam_osd_exit_dialog6(p->property.text.position);
        }
        else
        {
            cascam_osd_pop_dialog6((void*)p);
            if(p->property.text.content)
            {
                cascam_free(p->property.text.content);
            }
        }
    }
    else if(CASCAM_EMAIL == p->notice_type)
    {
        if(p->property.mail.clean_flag)
        {
            CASCAMEmailNoticeTypeEnum type = p->property.mail.mail_state;
            cascam_osd_exit_dialog9((void*)&type);
        }
        else
        {
            cascam_osd_pop_dialog9((void*)p);
        }
    }
    else if(CASCAM_FINGER == p->notice_type)
    {
        if(p->property.fingerprint.clean_flag)
        {
            CASCAMOsdFingerTypeEnum type = p->property.fingerprint.type;
            cascam_osd_exit_dialog11((void*)&type);
        }
        else
        {
            cascam_osd_pop_dialog11((void*)p);
        }
    }
    else if(CASCAM_MESSAGE == p->notice_type)
    {
        if(p->property.message.clean_flag)
        {
            cascam_osd_exit_dialog12((void*)p);
        }
        else
        {
            cascam_osd_pop_dialog12((void*)p);
        }

    }
    cascam_free(para);
    return ret;
}

// for cascam
int cascam_module_osd_get_rolling_times(unsigned int *times, int style)
{
    int ret = 0;
    if(NULL == times)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    ret = cascam_osd_get_rolling_times(times, style);
    //ret = cascam_osd_send_message(CASCAM_OSD_GET_ROLLING_TIMES, (void*)times);
    //if(ret)
    //{
    //    cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
    //    return -1;
    //}
    return ret;
}

// for stb
#if 0
int cascam_module_osd_rolling_times(void *data)
{
    int ret = 0;
    unsigned int *p = NULL;
    if(NULL == data)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    p = (unsigned int*)data;
    ret = cascam_osd_get_rolling_times(p);
    return ret;
}
#endif
// for cascam
// transfer string to named multi-language by applation
int cascam_module_osd_transfer_string(CASCAMOsdTransferClass *para)
{
    int ret = 0;
    if(NULL == para)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }

#if 0
    ret = cascam_osd_send_message(CASCAM_OSD_TRANSFER_STR, (void*)para);
    if(ret)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
#else
    ret = cascam_osd_get_multi_language(para->in_str, &(para->out_str));
    if(ret)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }

#endif
    return ret;
}

int cascam_module_osd_transfer_message_exec(void *data)
{
    int ret = 0;
    CASCAMOsdTransferClass *p = NULL;
    if(NULL == data)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    p = (CASCAMOsdTransferClass*)data;
    ret = cascam_osd_get_multi_language(p->in_str, &(p->out_str));
    if(ret)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return ret;
}

