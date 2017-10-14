/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	nstv_mail.c
* Author    :   B.Z.
* Project   :	GoXceed
* Type      :   CA Module
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
VERSION	    Date			  AUTHOR         Description
1.0	      2015.09.17		   B.Z.			  creation
*****************************************************************************/
#include "../../../include/cascam_platform.h"
#include "../../../include/cas/cascam_nstv.h"
#include "../../../include/cascam_os.h"
#include "../../../include/cascam_osd.h"
#include "../include/porting/cd_api.h"
#include "../include/porting/CDCASS.h"

static void _cdtime_to_ymdhms(unsigned int cdtime,
                            unsigned short *year, unsigned char *month, unsigned char *day,
                            unsigned char *hour, unsigned char *min, unsigned char *sec)
{
    time_t time = 0;
    struct tm *_tm = NULL;
    float time_zone = cascam_get_time_zone();
    if(time_zone < 0)
    {
        time = cdtime - (unsigned int)(fabs(time_zone)*60*60);
    }
    else
    {
        time = cdtime + (unsigned int)(fabs(time_zone)*60*60);
    }
    _tm = localtime((const time_t*)&time);
    if(_tm)
    {
        if(NULL != year)
        {
            *year = 1900 + _tm->tm_year;
        }
        if(NULL != month)
        {
            *month =  (_tm->tm_mon + 1);
        }
        if(NULL != day)
        {
            *day = _tm->tm_mday;
        }
        if(NULL != hour)
        {
            *hour = _tm->tm_hour;
        }
        if(NULL != min)
        {
            *min = _tm->tm_min;
        }
        if(NULL != sec)
        {
            *sec = _tm->tm_sec;
        }
    }
    else
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
    }
}

static SCDCAEmailHead thiz_email[CDCA_MAXNUM_EMAIL];
static unsigned char thiz_email_count = 0;

static int _sort_time_new_first(const void *a,const void* b)
{
    return ((SCDCAEmailHead*)a)->m_tCreateTime > ((SCDCAEmailHead*)b)->m_tCreateTime ? -1:1;
}

static int _email_init(void)
{
    int ret = 0;
    unsigned char count = CDCA_MAXNUM_EMAIL;
    unsigned char pos = 0;

    CDCASTB_RequestMaskBuffer();
    ret = CDCASTB_GetEmailHeads(thiz_email, &count, &pos);
    CDCASTB_RequestUpdateBuffer();
    if(ret == CDCA_RC_OK)
    {
        thiz_email_count = count;
        cascam_qsort(thiz_email, thiz_email_count, sizeof(SCDCAEmailHead), _sort_time_new_first);
    }
    else
    {
        cascam_printf("CASCAM, error, %s,%d\n",__func__, __LINE__);
        thiz_email_count = 0;
        return -1;
    }
    return 0;
}

static int _get_mail_count(unsigned short *count)
{
    int ret = 0;
    if(NULL == count)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    ret = GxCas_Get(GXCAS_CD_GET_EMAIL_COUNT, (void *)count);
    if(ret != CDCA_RC_OK)
    {
        cascam_printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return ret;
}


static int _change_str_to_utf8(char* in_str, unsigned int in_len, char *out_str, unsigned int *out_len)
{
    unsigned int size = 0;
    if((NULL == in_str) || (0 == in_str) || (NULL == out_str) || (NULL == out_len) || (0 == *out_len))
    {
        cascam_printf("\nCASCAM, error\n");
        return -1;
    }
    size = *out_len;
    if(0 == cascam_osd_change_to_utf8(in_str, in_len, out_str, &size))
    {
        *out_len = size;
    }
    else
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    return 0;
}

static int _get_mail_head(CASCAMNSTVMailInfoClass *out_data)
{
    int ret = 0;
    unsigned int mail_count = 0;
    int i = 0;
    unsigned int temp = 0;
    unsigned int bytes = 0;
    
    GxCas_CdGetMailHeadInfo mail_head;

    if((NULL == out_data) || (0 == out_data->mail_total))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    cascam_memset(&mail_head, 0, sizeof(GxCas_CdGetMailHeadInfo));
    mail_head.mail_head = cascam_malloc(100*sizeof(GxCas_CdGetHeadInfo));
    if(NULL == mail_head.mail_head)
    {
        cascam_printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    mail_head.mail_total = out_data->mail_total;
    ret = GxCas_Get(GXCAS_CD_GET_EMAIL_HEAD, (void *)&mail_head);
    if(ret != CDCA_RC_OK)
    {
        cascam_printf("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        cascam_free(mail_head.mail_head);
        return -1;
    }

    mail_count = out_data->mail_total;
    mail_count = mail_count > mail_head.mail_total? mail_head.mail_total:mail_count;
    for(i = 0; i < mail_count; i++)
    {
        bytes = cascam_strlen(mail_head.mail_head[i].title);
        if(0 < bytes)
        {
            // change to utf8
            unsigned int utf8_size = MAIL_TITLE_SIZE;
            char *utf8_str = cascam_malloc(utf8_size);
            if(NULL != utf8_str)
            {
                if(_change_str_to_utf8(mail_head.mail_head[i].title, bytes, utf8_str, &utf8_size) == 0)
                {
                    cascam_memcpy(out_data->mail_head[i].title, utf8_str, utf8_size);
                }
                else
                {
                    cascam_memcpy(out_data->mail_head[i].title, mail_head.mail_head[i].title, bytes);
                }
                cascam_free(utf8_str);
                utf8_str = NULL;
            }
            else
            {
                cascam_memcpy(out_data->mail_head[i].title, mail_head.mail_head[i].title, bytes);
            }
        }
        cascam_memcpy(out_data->mail_head[i].create_time, mail_head.mail_head[i].create_time, sizeof(mail_head.mail_head[i].create_time));
        out_data->mail_head[i].read_status =  mail_head.mail_head[i].read_status;// 0: read; 1:unread
        if(mail_head.mail_head[i].read_status)
        {
            temp++;
        }
        out_data->mail_head[i].importance =  mail_head.mail_head[i].importance;// 0: normal; 1: important
        out_data->mail_head[i].mail_id =  mail_head.mail_head[i].mail_id;
        
    }
    out_data->mail_total = i;
    out_data->mail_unread_count = temp;
    cascam_free(mail_head.mail_head);
    return ret;
}

static int _get_mail_body(CASCAMNSTVMailBodyClass *out_info)
{
    int ret = 0;
    unsigned int bytes = (MAIL_BODY_SIZE - 1);

    if(NULL == out_info)
    {
        cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    GxCas_CdGetMailContent cd_mail_content = {0};
    cd_mail_content.pos = out_info->pos;
    ret = GxCas_Get(GXCAS_CD_GET_EMAIL_CONTENT, (void *)&cd_mail_content);
    if (ret != CDCA_RC_OK) {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    bytes =  bytes > cascam_strlen(cd_mail_content.email_content.m_szEmail) ? cascam_strlen(cd_mail_content.email_content.m_szEmail): bytes;
    if(0 < bytes)
    {
        // change to utf8
        unsigned int utf8_size = MAIL_BODY_SIZE;
        char* utf8_str = cascam_malloc(utf8_size);
        if(NULL != utf8_str)
        {
            if(_change_str_to_utf8(cd_mail_content.email_content.m_szEmail, bytes, utf8_str, &utf8_size) == 0)
            {
                cascam_memcpy(out_info->body, utf8_str, utf8_size);
            }
            else
            {
                cascam_memcpy(out_info->body, cd_mail_content.email_content.m_szEmail, bytes);
            }
            cascam_free(utf8_str);
            utf8_str = NULL;
        }
        else
        {
            cascam_memcpy(out_info->body, cd_mail_content.email_content.m_szEmail, bytes);
        }
    }
    return ret;
}

// for mail edit
static int _mail_delete_by_pos(unsigned char *pos, unsigned short count)
{
    int ret = 0;
    // pos need linearity, such as 0,1,2,3; count=4
    GxCas_CdDelMail param = {0};
    param.pos = *(unsigned char *)pos;
    param.count = count;
    ret = GxCas_Set(GXCAS_CD_DELETE_EMAIL, (void *)&param);
    return ret;
}

static int _mail_delete_all(void)
{
    int ret = 0;
    GxCas_CdDelMail del = {0};
    del.pos = 0xff;
    ret = GxCas_Set(GXCAS_CD_DELETE_EMAIL, (void *)&del);
    return ret;
}


// export function
// 1: full, 0: have space
int nstv_mail_check_box_is_full(void)
{
    // TODO: 20160418
    if(thiz_email_count == CDCA_MAXNUM_EMAIL)
    {
        return 1;
    }
    return 0;
}

int nstv_mail_check_have_unread_mail(void)
{
    int i = 0;
    for(i = 0; i < thiz_email_count; i++)
    {
        if(thiz_email[i].m_bNewEmail)
            return 1;
    }
    return 0;
}


int nstv_mail_information_get(unsigned int sub_id, void *property, unsigned int len)
{
    int ret = 0;
    if((NULL == property) || (0 == len))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_MAIL_GET_COUNT:
            if(len < sizeof(unsigned short))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
                return -1;
            }
            ret = _get_mail_count((unsigned short*)property);
            break;
        case NSTV_MAIL_GET_DATA_HEAD:
            if(len != sizeof(CASCAMNSTVMailInfoClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
            ret = _get_mail_head((CASCAMNSTVMailInfoClass*)property);
            break;
        case NSTV_MAIL_GET_DATA_BODY:
            if(len != sizeof(CASCAMNSTVMailBodyClass))
            {
                cascam_printf("\nCASCAM, error, %s, %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
            ret = _get_mail_body((CASCAMNSTVMailBodyClass*)property);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }

    return ret;
}

int nstv_mail_information_edit(unsigned int sub_id, void *property, unsigned int len)
{
    int ret = 0;
    if(((NULL == property) || (0 == len))
            && (NSTV_MAIL_DEL_ALL != sub_id))
    {
        cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__,__LINE__);
        return -1;
    }
    switch(sub_id)
    {
        case NSTV_MAIL_DEL_ALL:
            ret = _mail_delete_all();
            break;
        case NSTV_MAIL_DEL_BY_POS:
            ret = _mail_delete_by_pos((unsigned char*)property, len);
            break;
        default:
            cascam_printf("\nCASCAM, error, %s, %d\n", __FUNCTION__, __LINE__);
            break;
    }
    return ret;
}

