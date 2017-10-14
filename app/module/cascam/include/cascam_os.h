/**
 *
 * @file        cascam_os.h
 * @brief
 * @version     1.1.0
 * @date        08/25/2015 09:47:49 AM
 * @author      B.Z., 88156088@nationalchip.com
 *
 */

#include <assert.h>
#include <time.h>
#include <math.h>

int cascam_thread_create(const char *ThreadName, 
                         int *ThreadID,
                         void (*EntryFunc)(void*),
                         void *Arg,
                         unsigned int StackSize,
                         unsigned int Priority);

int cascam_thread_delay(unsigned int Millisecond);

int cascam_thread_join(int ThreadID);

int cascam_thead_detach(void);

int cascam_queue_create(int *QueueID, unsigned int QueueDepth, unsigned int DataSize);

int cascam_queue_delete(int QueueID);

int cascam_queue_receive(int QueueID, 
                           void *DataBuf, 
                           unsigned int Size, 
                           unsigned int *SizeCopied,
                           int Timeout);

int cascam_queue_send(int QueueID, void *DataBuf, unsigned int Size, int Timeout);

int cascam_semaphore_create(int *SemId, unsigned int InitialValue);

int cascam_semaphore_post(int SemID);

int cascam_semaphore_wait(int SemID);

int cascam_semaphore_delete(int SemID);

int cascam_mutex_create(int *MutexID);

int cascam_mutex_delete(int MutexID);

int cascam_mutex_lock(int MutexID);

int cascam_mutex_unlock(int MutexID);

void *cascam_malloc(unsigned int Size);

void cascam_free(void* ptr);

void *cascam_memset(void *ptr, int ch, unsigned int n);

void *cascam_memcpy(void *dest, const void *src, unsigned int n);

unsigned int cascam_strlen(char *s);

int cascam_strcmp(const char *s1,const char *s2);

int cascam_strncmp(const char *s1,const char *s2, const unsigned int size);

char *cascam_strcat(char *s1,char *s2);

char *cascam_strncat(char *s1,const char *s2, unsigned int bytes);

void cascam_qsort(void *base,int nelem,int width,int (*fcmp)(const void *,const void *));

unsigned int cascam_get_tick_time(void);

float cascam_get_time_zone(void);

int cascam_get_current_time(int *utc_time, int *timezone_min, int *daylight_min, unsigned int *daylight_flag);

int cascam_get_code_type(unsigned int cas_code_type, int *out_type, int *video_flag);

// timer 1
typedef void (*timer1)(unsigned long data);
int cascam_timer1_create(timer1 func,unsigned long data, unsigned long trigger, unsigned long interval);

int cascam_time1_delete(int TimerID);

// timer2
int cascam_timer2_create(timer1 func,unsigned long data, unsigned long trigger, unsigned long interval);

int cascam_time2_delete(int TimerID);

void cascam_printf(char *format, ...);

void cascam_sprintf(char *buffer,char *format, ...);

int cascam_show_time(char *str);

// crc32
unsigned int cascam_crc32(unsigned char* pBuffer, unsigned int nSize);

//des3
int cascam_des3(unsigned char *pucKey, unsigned int key_len, unsigned char *pucSourceData, unsigned int data_len, unsigned char *pucDestOut, unsigned char ucMode);

int cascam_get_chip_sn(unsigned char *SnData, int BufferLen);

int cascam_get_card_id(unsigned char *CardData, int BufferLen);

int cascam_get_dsn(unsigned char *DsnData, int BufferLen);

int cascam_get_chip_name(unsigned char *NameData, int BufferLen);

int cascam_get_market_id(unsigned char *MarketData, int BufferLen);

int cascam_get_jtag_status(int *is_support);

int cascam_get_ads_status(int *is_support);

