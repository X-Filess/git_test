#ifndef PORTING_COMMON_H
#define PORTING_COMMON_H

int common_thread_create(const char *ThreadName, 
                         int *ThreadID,
                         void (*EntryFunc)(void*),
                         void *Arg,
                         unsigned int StackSize,
                         unsigned int Priority);

int common_thread_delay(unsigned int Millisecond);

int common_thread_join(int ThreadID);

int common_thread_detach(void);

int common_queue_create(int *QueueID, unsigned int QueueDepth, unsigned int DataSize);

int common_queue_delete(int QueueID);

int common_queue_receive(int QueueID, 
                           void *DataBuf, 
                           unsigned int Size, 
                           unsigned int *SizeCopied,
                           int Timeout);

int common_queue_send(int QueueID, void *DataBuf, unsigned int Size, int Timeout);

int common_semaphore_create(int *SemId, unsigned int InitialValue);

int common_semaphore_post(int SemID);

int common_semaphore_wait(int SemID);

int common_semaphore_delete(int SemID);

int common_mutex_create(int *MutexID);

int common_mutex_delete(int MutexID);

int common_mutex_lock(int MutexID);

int common_mutex_unlock(int MutexID);

void *common_malloc(unsigned int Size);

void common_free(void* ptr);

unsigned int common_str2hex(unsigned int strlen, unsigned char *strbuf, unsigned char *numbuf);

int common_show_time(char *str);

void common_printf(char *format, ...);
#endif

