/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	porting_common.c
* Author    :	B.Z.
* Project   :	CM Module
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.03.04		   B.Z.			  Creation
*****************************************************************************/
#include "gxcore.h"
int common_thread_create(const char *ThreadName, 
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

int common_thread_delay(unsigned int Millisecond)
{
    int ret = 0;
	ret = GxCore_ThreadDelay(Millisecond);
    return ret;
}

int common_thread_join(int ThreadID)
{
    int ret = 0;
	ret = GxCore_ThreadJoin(ThreadID);
    return ret;
}

int common_thread_detach(void)
{
    int ret = 0;
    ret = GxCore_ThreadDetach();
    return ret;
}

int common_queue_create(int *QueueID, unsigned int QueueDepth, unsigned int DataSize)
{
	int ret = 0;
    ret = GxCore_QueueCreate(QueueID,QueueDepth,DataSize);
    return ret;
}

int common_queue_delete(int QueueID)
{
    int ret = 0;
	ret = GxCore_QueueDelete(QueueID);
    return ret;
}

int common_queue_receive(int QueueID, 
                           void *DataBuf, 
                           unsigned int Size, 
                           unsigned int *SizeCopied,
                           int Timeout)
{// Timeout==0: means no timeout; Timeout==-1: special mode...
    int ret = -1;
    ret = GxCore_QueueGet(QueueID,DataBuf,Size,SizeCopied,Timeout);
    return ret;
}

int common_queue_send(int QueueID, void *DataBuf, unsigned int Size, int Timeout)
{// Timeout == 0: means no timeout; Timeout < 0: special mode...
    int ret = 0;
	ret = GxCore_QueuePut(QueueID,DataBuf,Size,Timeout);
    return ret;
}

int common_semaphore_create(int *SemId, unsigned int InitialValue)
{
    int ret = 0;
	ret = GxCore_SemCreate(SemId,InitialValue);
    return ret; 
}

int common_semaphore_post(int SemID)
{
    int ret = 0;
	ret = GxCore_SemPost(SemID);
    return ret;
}

int common_semaphore_wait(int SemID)
{
    int ret = 0;
	ret = GxCore_SemWait(SemID);
    return ret;
}

int common_semaphore_delete(int SemID)
{
    int ret = 0;
	ret = GxCore_SemDelete(SemID);
    return ret;
}

int common_mutex_create(int *MutexID)
{
    int ret = 0;
	ret = GxCore_MutexCreate(MutexID);
    return ret;
}

int common_mutex_delete(int MutexID)
{
    int ret = 0;
	ret = GxCore_MutexDelete(MutexID);
    return ret;
}

int common_mutex_lock(int MutexID)
{
    int ret = 0;
	ret = GxCore_MutexLock(MutexID);
    return ret;
}

int common_mutex_unlock(int MutexID)
{
    int ret = 0;
	ret = GxCore_MutexUnlock(MutexID);
    return ret;
}

void *common_malloc(unsigned int Size)
{
    return GxCore_Mallocz(Size);
}

void common_free(void* ptr)
{
    GxCore_Free(ptr);
}

static unsigned char _ascii2hex(unsigned char ascii)
{
    if (ascii >= '0' && ascii <= '9')
    {
        return ascii - 0x30;
    }
    else if (ascii >= 'a' && ascii <= 'f')
    {
        return ascii - 0x61 + 0xa;
    }
    else if (ascii >= 'A' && ascii <= 'F')
    {
        return ascii - 0x41 + 0xa;
    }
    return 0;
}

unsigned int common_str2hex(unsigned int strlen, unsigned char *strbuf, unsigned char *numbuf)
{
    unsigned int hexlen = 0;
    unsigned int i;
    unsigned char num;

    for (i = 0; i < strlen / 2; i++)
    {
        num = _ascii2hex(strbuf[2 * i]) & 0x0f;
        num <<= 4;
        num |= _ascii2hex(strbuf[2 * i + 1]) & 0x0f;
        numbuf[hexlen++] = num;
    }
    return hexlen;
}


int common_show_time(char *str)
{
	SHOWTIME(str);
    return 0;
}

void common_printf(char *format, ...)
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

