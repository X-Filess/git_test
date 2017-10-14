/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cascam_os_adapter.c
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
#include "gxcore.h"
#include "gxcore_hw_bsp.h"
#include "module/pm/gxpm_manage.h"

int cascam_thread_create(const char *ThreadName, 
                         int *ThreadID,
                         void (*EntryFunc)(void*),
                         void *Arg,
                         unsigned int StackSize,
                         unsigned int Priority)
{
	int ret = 0;
	ret = GxCore_ThreadCreate(ThreadName,ThreadID,EntryFunc,Arg,StackSize,Priority);
    return ret;
}

int cascam_thread_delay(unsigned int Millisecond)
{
    int ret = 0;
	ret = GxCore_ThreadDelay(Millisecond);
    return ret;
}

int cascam_thread_join(int ThreadID)
{
    int ret = 0;
	ret = GxCore_ThreadJoin(ThreadID);
    return ret;
}

int cascam_thead_detach(void)
{
    int ret = 0;
    ret = GxCore_ThreadDetach();
    return ret;
}

int cascam_queue_create(int *QueueID, unsigned int QueueDepth, unsigned int DataSize)
{
	int ret = 0;
    ret = GxCore_QueueCreate(QueueID,QueueDepth,DataSize);
    return ret;
}

int cascam_queue_delete(int QueueID)
{
    int ret = 0;
	ret = GxCore_QueueDelete(QueueID);
    return ret;
}

int cascam_queue_receive(int QueueID, 
                           void *DataBuf, 
                           unsigned int Size, 
                           unsigned int *SizeCopied,
                           int Timeout)
{// Timeout==0: means no timeout; Timeout==-1: special mode...
    int ret = -1;
    ret = GxCore_QueueGet(QueueID,DataBuf,Size,SizeCopied,Timeout);
    return ret;
}

int cascam_queue_send(int QueueID, void *DataBuf, unsigned int Size, int Timeout)
{// Timeout == 0: means no timeout; Timeout < 0: special mode...
    int ret = 0;
	ret = GxCore_QueuePut(QueueID,DataBuf,Size,Timeout);
    return ret;
}

int cascam_semaphore_create(int *SemId, unsigned int InitialValue)
{
    int ret = 0;
	ret = GxCore_SemCreate(SemId,InitialValue);
    return ret; 
}

int cascam_semaphore_post(int SemID)
{
    int ret = 0;
	ret = GxCore_SemPost(SemID);
    return ret;
}

int cascam_semaphore_wait(int SemID)
{
    int ret = 0;
	ret = GxCore_SemWait(SemID);
    return ret;
}

int cascam_semaphore_delete(int SemID)
{
    int ret = 0;
	ret = GxCore_SemDelete(SemID);
    return ret;
}

int cascam_mutex_create(int *MutexID)
{
    int ret = 0;
	ret = GxCore_MutexCreate(MutexID);
    return ret;
}

int cascam_mutex_delete(int MutexID)
{
    int ret = 0;
	ret = GxCore_MutexDelete(MutexID);
    return ret;
}

int cascam_mutex_lock(int MutexID)
{
    int ret = 0;
	ret = GxCore_MutexLock(MutexID);
    return ret;
}

int cascam_mutex_unlock(int MutexID)
{
    int ret = 0;
	ret = GxCore_MutexUnlock(MutexID);
    return ret;
}

void *cascam_malloc(unsigned int Size)
{
    return GxCore_Mallocz(Size);
}

void cascam_free(void* ptr)
{
    GxCore_Free(ptr);
}

void *cascam_memset(void *ptr, int ch, unsigned int n)
{
    return memset(ptr,ch,n);
}

void *cascam_memcpy(void *dest, const void *src, unsigned int n)
{
    //return memcpy(dest,src,n);
    return memmove(dest,src,n);
}

unsigned int cascam_strlen(char *s)
{
    return strlen(s);
}

int cascam_strcmp(const char *s1,const char *s2)
{
    return strcmp(s1,s2);
}

int cascam_strncmp(const char *s1,const char *s2, const unsigned int size)
{
    return strncmp(s1,s2, size);
}

void cascam_sprintf(char *buffer,char *format, ...)
{
#define BUFFER_LEN  256
	char Buf[BUFFER_LEN];
	va_list args;
	int i;
    memset(Buf,0, BUFFER_LEN);
	
	va_start(args, format);
	i = vsprintf(Buf, format, args);
	va_end(args);

	sprintf(buffer,"%s", Buf);
}

char *cascam_strcat(char *s1, char *s2)
{
    return strcat(s1,s2);
}

char *cascam_strncat(char *s1, const char *s2, unsigned int bytes)
{
    return strncat(s1,s2, bytes);
}

void cascam_qsort(void *base,int nelem,int width,int (*fcmp)(const void *,const void *))
{
    qsort(base, nelem, width, fcmp);
}

unsigned int cascam_get_tick_time(void)
{// 
	GxTime TickTime = {0};
    GxCore_GetTickTime(&TickTime);
    return TickTime.seconds;
}

// time
// change zone to seconds
extern  int app_get_current_time(int *utc_time, int *timezone_min, int *daylight_min, unsigned int *daylight_flag);
float cascam_get_time_zone(void)
{
    int utc_time = 0;
    int timezone_min = 0;
    int daylight_min = 0;
    unsigned int daylight_flag = 0;

    app_get_current_time(&utc_time, &timezone_min, &daylight_min, &daylight_flag);
    return (float)(timezone_min/60);
}

int cascam_get_current_time(int *utc_time, int *timezone_min, int *daylight_min, unsigned int *daylight_flag)
{
    return app_get_current_time(utc_time, timezone_min, daylight_min, daylight_flag);
}

// timer 1
typedef void (*timer1)(unsigned long data);
int cascam_timer1_create(timer1 func,unsigned long data, unsigned long trigger, unsigned long interval)
{
    int TimerId = 0;
    TimerId = gx_rtc_timer_create(func,data,trigger,interval);
    return TimerId;
}

int cascam_time1_delete(int TimerID)
{
    int ret = 0;
	ret = gx_rtc_timer_delete(TimerID);
    return ret;
}

// timer2
int cascam_timer2_create(timer1 func,unsigned long data, unsigned long trigger, unsigned long interval)
{
    int TimerId = 0;
	// TODO:
    return TimerId;
}

int cascam_time2_delete(int TimerID)
{
    int ret = 0;
	// TODO:
    return ret;
}


int cascam_show_time(char *str)
{
	SHOWTIME(str);
    return 0;
}

void cascam_printf(char *format, ...)
{
#define BUFFER_LEN  256
	char Buf[BUFFER_LEN];
	va_list args;
    memset(Buf,0, BUFFER_LEN);
	
	va_start(args, format);
	vsnprintf(Buf, BUFFER_LEN, format, args);
	va_end(args);

	printf("%s", Buf);
}

#define MPEG1_VIDEO     (0x01)
#define MPEG2_VIDEO     (0x02)
#define MPEG4_VIDEO     (0x10)
#define H264_VIDEO      (0x1b)
#define H265_VIDEO      (0x24)
#define AVS_VIDEO       (0x42)
#define MPEG4_VIDEO     (0x10)
#define MPEG2_AUDIO1    (0x03)
#define MPEG2_AUDIO2    (0x04)
#define AAC_AUDIO1      (0x0f)
#define	AAC_AUDIO2		(0x11)
#define A52_AUDIO       (0x81)
#define AC3_PLUS        (0x84)
int cascam_get_code_type(unsigned int cas_code_type, int *out_type, int *video_flag)
{
    int ret = 0;
    switch(cas_code_type)
    {
        case MPEG2_AUDIO1:
            *out_type = GXBUS_PM_AUDIO_MPEG1;
            *video_flag = 0;
            break;
        case MPEG2_AUDIO2:
            *out_type = GXBUS_PM_AUDIO_MPEG2;
            *video_flag = 0;
            break;
        case AAC_AUDIO1:
            *out_type = GXBUS_PM_AUDIO_AAC_ADTS;
            *video_flag = 0;
            break;
        case AAC_AUDIO2:
            *out_type = GXBUS_PM_AUDIO_AAC_LATM;
            *video_flag = 0;
            break;
        case A52_AUDIO:
            *out_type = GXBUS_PM_AUDIO_AC3;
            *video_flag = 0;
            break;
        case AC3_PLUS:
            *out_type = GXBUS_PM_AUDIO_AC3_PLUS;
            *video_flag = 0;
            break;
        case MPEG1_VIDEO:
        case MPEG2_VIDEO:
            *out_type = GXBUS_PM_PROG_MPEG;
            *video_flag = 1;
            break;
        case MPEG4_VIDEO:
            *out_type = GXBUS_PM_PROG_MPEG4;
            *video_flag = 1;
            break;
        case AVS_VIDEO:
            *out_type = GXBUS_PM_PROG_AVS;
            *video_flag = 1;
            break;
        case H264_VIDEO:
            *out_type = GXBUS_PM_PROG_H264;
            *video_flag = 1;
            break;
        case H265_VIDEO:
            *out_type = GXBUS_PM_PROG_H265;
            *video_flag = 1;
            break;
        default:
            printf("\n\033[33mCASCAM, can not support type: %d\033[0m\n", cas_code_type);
            ret = -1;
            break;
    }
    return ret;
}


#ifdef ECOS_OS
// gx3211
#define GX_OTP_BASE 0xA0F80000
/*
 * OTP control registe, register used when CPU access OTP
 * OTP config register
 * OTP state register
 */
#define OTP_CON_REG (*(volatile unsigned int*)(GX_OTP_BASE + 0x80))
#define OTP_CFG_REG (*(volatile unsigned int*)(GX_OTP_BASE + 0x84))
#define OTP_STA_REG (*(volatile unsigned int*)(GX_OTP_BASE + 0x88))

static void _otp_read_hard(unsigned int start_addr, int rd_num, unsigned char *data)
{
	int i;
    volatile int j = 100;
	unsigned int otp_con_reg;
	// set all control signals to be invalid, address to 0x1800(default)
    otp_con_reg = 0x00000000;
    //check if OTP Controller is ready
    while(!(OTP_STA_REG & (1 << 10)));
	for (i = 0; i < rd_num; i++)
	{
        otp_con_reg = (0x7ff & (start_addr + i)) << 3;
		/* check if OTP Controller is busy now; 1 for busy */
		while(OTP_STA_REG & 0x100);
		/* set READEN */
		OTP_CON_REG = otp_con_reg | (0x1 << 14);
        /* delay*/
        j = 100;
        while(j--);
		/* clear READEN */
		OTP_CON_REG &= ~(0x1 << 14);
		/* chekc if OTP data is ready */
		while(!(OTP_STA_REG & 0x200));
		data[i] = (unsigned char)(OTP_STA_REG & 0xFF);
		OTP_CON_REG = 0x00000000;
	}
    // set all control signals to be invalid, address to 0x1800(default)
	OTP_CON_REG = 0x00000000;
    printf("\n%s, %d\n", __FUNCTION__,__LINE__);
}

int cascam_get_chip_name(unsigned char *NameData, int BufferLen)
{
#define CHIP_NAME1_STA_REG (*(volatile unsigned int*)(0xA030a190))
#define CHIP_NAME2_STA_REG (*(volatile unsigned int*)(0xA030a194))
#define CHIP_NAME3_STA_REG (*(volatile unsigned int*)(0xA030a198))

    unsigned char buffer[12] = {0};
    unsigned int reg_data = 0;
	if((NameData == NULL) &&(BufferLen <12))
	{
		return -1;
	}
    reg_data = CHIP_NAME1_STA_REG;
    buffer[0] = reg_data&0xff;
    buffer[1] = (reg_data >> 8)&0xff;
    buffer[2] = (reg_data >> 16)&0xff;
    buffer[3] = (reg_data >> 24)&0xff;
    reg_data = CHIP_NAME2_STA_REG;
    buffer[4] = reg_data&0xff;
    buffer[5] = (reg_data >> 8)&0xff;
    buffer[6] = (reg_data >> 16)&0xff;
    buffer[7] = (reg_data >> 24)&0xff;
    reg_data = CHIP_NAME3_STA_REG;
    buffer[8] = reg_data&0xff;
    buffer[9] = (reg_data >> 8)&0xff;
    buffer[10] = (reg_data >> 16)&0xff;
    buffer[11] = (reg_data >> 24)&0xff;

	memcpy(NameData, buffer,12);
	return 0;
}

int cascam_get_chip_sn(unsigned char *SnData, int BufferLen)
{
#define PUBLIC_ID1_STA_REG (*(volatile unsigned int*)(0xA030a560))
#define PUBLIC_ID2_STA_REG (*(volatile unsigned int*)(0xA030a564))
    unsigned char buffer[8] = {0};
    unsigned int reg_data = 0;
	if((SnData == NULL) &&(BufferLen <8))
	{
		return -1;
	}
    reg_data = PUBLIC_ID1_STA_REG;
    buffer[0] = reg_data&0xff;
    buffer[1] = (reg_data >> 8)&0xff;
    buffer[2] = (reg_data >> 16)&0xff;
    buffer[3] = (reg_data >> 24)&0xff;
    reg_data = PUBLIC_ID2_STA_REG;
    buffer[4] = reg_data&0xff;
    buffer[5] = (reg_data >> 8)&0xff;
    buffer[6] = (reg_data >> 16)&0xff;
    buffer[7] = (reg_data >> 24)&0xff;
	memcpy(SnData, buffer,8);
	return 0;
}

int cascam_get_dsn(unsigned char *DsnData, int BufferLen)
{
	unsigned char buffer[4] = {0};
	if((DsnData == NULL) &&(BufferLen <4))
	{
		return -1;
	}
	_otp_read_hard(0x780,4,buffer);
	memcpy(DsnData, buffer,4);
	return 0;
}

int cascam_get_card_id(unsigned char *CardData, int BufferLen)
{
	unsigned char buffer[8] = {0};
	if((CardData == NULL) &&(BufferLen <8))
	{
		return -1;
	}
	_otp_read_hard(0x784,8,buffer);
	memcpy(CardData, buffer,8);
	return 0;
}

int cascam_get_market_id(unsigned char *MarketData, int BufferLen)
{
	unsigned char buffer[8] = {0};
	if((MarketData == NULL) &&(BufferLen < 4))
	{
		return -1;
	}
	_otp_read_hard(0x390,4,buffer);
	memcpy(MarketData, buffer,4);
	return 0;
}

// is_support: 1-support; 0-not support
int cascam_get_jtag_status(int *is_support)
{
    unsigned char jtag_mode = 0;
	if(NULL == is_support)
	{
		return -1;
	}
	_otp_read_hard(0x313, 1, &jtag_mode);
    if((((jtag_mode >> 3)&0x7) == 0x6)
        || (((jtag_mode >> 3)&0x7) == 0x4))
    {
        *is_support = 0;
    }
    else
    {
        *is_support = 1;
    }
	return 0;
}

// is_support: 1-support; 0-not support
// adveranced sucurity start support
int cascam_get_ads_status(int *is_support)
{
    unsigned char ads_status = 0;
	if(NULL == is_support)
	{
		return -1;
	}
	_otp_read_hard(0x313, 1, &ads_status);
    if((ads_status&0x7) == 0x6)
    {
        *is_support = 1;
    }
    else
    {
        *is_support = 0;
    }
	return 0;
}

#if 0 // gx3201
#define GX_OTP_BASE 0xA0F80000
/*
 * OTP control registe, register used when CPU access OTP
 * OTP config register
 * OTP state register
 */
#define OTP_CON_REG (*(volatile unsigned int*)(GX_OTP_BASE + 0x80))
#define OTP_CFG_REG (*(volatile unsigned int*)(GX_OTP_BASE + 0x84))
#define OTP_STA_REG (*(volatile unsigned int*)(GX_OTP_BASE + 0x88))

void _otp_read_hard(unsigned int start_addr, int rd_num, unsigned char *data)
{
	int i;
	unsigned int otp_con_reg;
	otp_con_reg = 0x43000000 | (0x3fff & start_addr);
	for (i = 0; i < rd_num; i++)
	{
		/* check if OTP Controller is busy now; 1 for busy */
		while(OTP_STA_REG & 0x100);
		/* set READEN */
		OTP_CON_REG = otp_con_reg | (0x1 << 14);
		/* clear READEN */
		OTP_CON_REG = otp_con_reg;
		/* chekc if OTP data is ready */
		while(!(OTP_STA_REG & 0x200));
		data[i] = (unsigned char)(OTP_STA_REG & 0xFF);
		otp_con_reg++;
	}
	/* set all control signals to be invalid, address to 0x300(default) */
	/* otp_con_reg = 0x43000300; */
}

// the chip sn address: 0x338; the sn length:8byte
int cascam_get_chip_sn(unsigned char *SnData, int BufferLen)
{
	unsigned char buffer[8] = {0};
	if((SnData == NULL) &&(BufferLen <8))
	{
		return -1;
	}
	_otp_read_hard(0x338,8,buffer);
	memcpy(SnData, buffer,8);
	return 0;
}
#endif
#endif


