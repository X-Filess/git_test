#include "app.h"
#include "register.h"
#include "uart.h"
#include "customer.h"

#define log_printf printf
// memory
static unsigned char thiz_receive_memory[MAX_CMDBUF_SIZE*2] = {0};
static int thiz_memory_read_pos	= 0;
static int thiz_memory_write_pos	= 0;
static int thiz_uart_thread = 0;

int uart_reply(char *cmd, char *str, unsigned int error_code, char *result);

static int __find_first_keyword(char *cmd, char *keyword)
{
	int len = 0;
	char buffer[MAX_CMDBUF_SIZE] = {0};
	if((cmd == NULL) || (keyword == NULL))
	{
		log_printf("\nUART TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	len = strlen(cmd);
	if((len == 0) || (len >= MAX_CMDBUF_SIZE))
	{
		log_printf("\nUART TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if((cmd[0] < 'a') || (cmd[0] > 'z'))
		sscanf(cmd, "%*[^a-z]%s",buffer);
	else
		sscanf(cmd, "%s", buffer);
//	printf("\ncmd = %s, keyword = %s\n", cmd, buffer);
	len = strlen(buffer);
	if((len == 0) || (len >= MAX_CMDBUF_SIZE))
	{
		log_printf("\nUART TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	memcpy(keyword, buffer, len);
	return 0;
}

/***************************************************************************
* Function		:parse_line
* Description	:get command param num, command name and params
* Arguments		:[in]line
				 [in]argv[]
* Return 		: return param num -- argc
* Others		:argv[0]--name  argv[1]--params1  argv[2]--params2
****************************************************************************/
static int __parse_line (char *line, char *argv[])
{
	int nargs = 0;
	//log_printf("Line: %s|\n",line);
	while (nargs < CONFIG_SYS_MAXARGS) {
		 //skip any white space 
		while ((*line == ' ') || (*line == '\t')) {
			++line;
		}

		if(*line == '\n'){
			*line = '\0';
		}

		if (*line == '\0') {	// end of line, no more args	
			argv[nargs] = NULL;
			return (nargs);
		}

		argv[nargs++] = line;	// begin of argument string	

		// find end of string 
		while (*line && (*line != ' ') && (*line != '\t') && (*line != '\n')) {
			++line;
		}

		if(*line == '\n'){
			*line = '\0';
		}

		if (*line == '\0' ) {	// end of line, no more args	
			argv[nargs] = NULL;
			return (nargs);
		}

		*line++ = '\0';		//terminate current arg	 
	}

	log_printf("** Too many args (max. %d) **\n", CONFIG_SYS_MAXARGS);

	return (nargs);
}

static int _uart_test_one_cmd_proccess(char *cmd, int cmdlen)
{
	signed int ret = 0;
	char buffer[MAX_CMDBUF_SIZE] = {0};
	char *argv[CONFIG_SYS_MAXARGS + 1] = {0};
	int argc = 0;
	cmd_t *cmd_ops = NULL;
		
	ret = __find_first_keyword(cmd,buffer);
	if(ret < 0)
	{
		// cmd is wrong
		log_printf("\nUART TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		uart_reply("find_firt_keywords", "failed: the cmd is not support!", 2, NULL);
       goto err;
		//return -1;
	}
	cmd_ops = find_uart_cmd(buffer);
	if(cmd_ops == NULL)
	{
		// cmd is wrong, cmd is not support
		log_printf("\nUART TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		uart_reply("find_cmd", "failed: the cmd is not support!", 2, NULL);
		goto err;
      //return -1;
	}
	if(cmd_ops->cmd != NULL)
	{
		memset(buffer, 0, MAX_CMDBUF_SIZE);
		memcpy(buffer, cmd, strlen(cmd));
		argc = __parse_line(buffer, argv);
		if((argc == 0) || (argc >= CONFIG_SYS_MAXARGS))
		{
			// cmd is wrong, cmd is not support
			log_printf("\nUART TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
			uart_reply(cmd_ops->name, "failed: parameter is wrong!", 2, NULL);
			goto err;
           //return -1;
		}
		ret = cmd_ops->cmd(cmd_ops, 0, argc, argv, cmd);
		if(ret < 0)
		{
			// cmd execute failed
			log_printf("\nUART TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
			goto err;
          //  return -1;
		}
	}
	else
	{
		// cmd is wrong, cmd is not support
		log_printf("\nUART TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		uart_reply(cmd_ops->name, "failed: have no function to deal with the cmd!", 1, NULL);
		goto err;
       // return -1;
	}
	return 0;

err:
#ifdef ECOS_OS
    // for gx uart module, reset the uart receive fifo
    {// NOTICE: the register address is different from different chips
        int i = 1000;
        *(volatile unsigned int*)0xa0400010 |=(1<<4);
        while(i--);
        *(volatile unsigned int*)0xa0400010 &=~(1<<4);
    }
#endif

    return -1;
}



static int _data_receive(unsigned char* MemBuffer, int MemSize,
											 int *pReadPos,int *pWritePos,
											 unsigned char *ReceiveData,int ReceiveSize)
{
	if((MemBuffer == NULL) 
		|| (ReceiveData == NULL) 
		|| (pReadPos == NULL)
		|| (pWritePos == NULL)
		|| (MemSize*2 < (ReceiveSize + *pWritePos)) 
		|| (MemSize < ReceiveSize)
		|| (ReceiveSize <= 0))
	{
		log_printf("\nPROTOCOL, parameter error\n");
		return -1;
	}

	if((*pWritePos + ReceiveSize) > MemSize)
	{
		memcpy((MemBuffer + *pWritePos), ReceiveData, (MemSize - *pWritePos));
		memcpy(MemBuffer,(ReceiveData + (MemSize - *pWritePos)),(ReceiveSize - (MemSize - *pWritePos)));
		*pWritePos = ReceiveSize - (MemSize - *pWritePos);
		if(*pWritePos >= *pReadPos)
		{
			*pWritePos = 0;
			*pReadPos = 0;
			log_printf("\nPROTOCOL, buff is full\n");
			return -2;
		}
	}
	else
	{
		memcpy((MemBuffer + *pWritePos), ReceiveData, ReceiveSize);
		if((*pWritePos + ReceiveSize) == MemSize)
			*pWritePos = 0;
		else
			*pWritePos +=  ReceiveSize;
	}
	return 0;
}

static int _data_dealwith(unsigned char* MemBuffer, int MemSize,
											 int *pReadPos,int *pWritePos,
											 int ByteToDo)
{
	char *pData 							= NULL;
	char *pTemp 							= NULL;
	char *pTempParser						= NULL;
	char *p									= NULL;
	int ByteToDealWith 						= ByteToDo;
	int RealDoSize 							= 0;
	int Ret									= 0;
	int Length								= 0;
	int i									= 0;

	if((MemBuffer == NULL)
		|| (pReadPos == NULL)
		|| (pWritePos == NULL)
		|| (MemSize < *pReadPos)
		|| (MemSize < ByteToDealWith)
		|| (ByteToDealWith <= 0))
	{
		log_printf("\nSK PROTOCOL, parameter error\n");
		return -1;
	}
	if(*pReadPos == *pWritePos)
	{
		return -1;
	}
	log_printf("\nPROTOCOL, Read = %d, Write = %d\n",*pReadPos,*pWritePos);

	if(*pReadPos > *pWritePos)
	{
		if(((MemSize - *pReadPos) + *pWritePos) < ByteToDealWith)
			ByteToDealWith = (MemSize - *pReadPos) + *pWritePos;
	}
	else
	{
		if((*pWritePos - *pReadPos) < ByteToDealWith)
			ByteToDealWith = *pWritePos - *pReadPos;
	}

	pTemp = calloc(1,ByteToDealWith+1);
	if(pTemp == NULL)
	{
		log_printf("PROTOCOL, calloc error\n");
		return -1;
	}

	pTempParser = calloc(1,ByteToDealWith+1);
	if(pTempParser == NULL)
	{
		log_printf("PROTOCOL, calloc error\n");
		free(pTemp);
		return -1;
	}

	if((*pReadPos + ByteToDealWith) <= MemSize)
	{
		pData = (char*)MemBuffer + *pReadPos;
		memcpy(pTemp,pData,ByteToDealWith);
	}
	else
	{
		pData = (char*)MemBuffer + *pReadPos;
		memcpy(pTemp,pData,(MemSize - *pReadPos));
		pData = (char*)MemBuffer;
		memcpy((pTemp + (MemSize - *pReadPos)),pData,(ByteToDealWith - (MemSize - *pReadPos)));
	}
	// do the data
	pData = pTemp;
	p = strstr((char*)pData,CMD_END_FLAG);
	if(p == NULL)
	{
		//pTemp += ByteToDealWith;
		log_printf("PROTOCOL, bad data...\n");
		goto BACK;
	}
	while(1)
	{
		memset(pTempParser, 0, ByteToDealWith+1);
		Length = p - pData + (strlen(CMD_END_FLAG));
		memcpy(pTempParser,pData,Length);

		for(i = 0; i < Length; i++)
		{
			if((pTempParser[i] == '\r')
                || (pTempParser[i] == '\n'))
				pTempParser[i] = ' ';
		}
		Ret = _uart_test_one_cmd_proccess(pTempParser,Length);
		if(Ret < 0)
		{
			//log_printf("\nPROTOCOL, parser cmd failed...\n");
			//break;
		}
		pData += Length;
		p = strstr((char*)pData,CMD_END_FLAG);
		if(p == NULL)
		{
			log_printf("\nPROTOCOL, not enough data to deal with...\n");
			break;
		}
	}

BACK:	
	// successfull, set the read position
	RealDoSize = pData - pTemp;
	if((*pReadPos + RealDoSize) > MemSize)
	{
		*pReadPos = (*pReadPos + RealDoSize) - MemSize;
	}
	else if((*pReadPos + RealDoSize) == MemSize)
	{
		*pReadPos = 0;
	}
	else
	{
		*pReadPos += RealDoSize;
	}
	free(pTemp);
	free(pTempParser);
	return 0;
}

static void _uart_console(void *arg)
{
	unsigned char Buffer[MAX_CMDBUF_SIZE] = {0};
	int ByteRealRead = 0;

    GxCore_ThreadDetach();
    while(1)
    {
        memset(Buffer, 0, MAX_CMDBUF_SIZE);
        ByteRealRead = 0;
	    if((ByteRealRead = uart_device_receive(Buffer,MAX_CMDBUF_SIZE)) > 0)
	    {// read data
		    // deal with the cmd
		    if(_data_receive(thiz_receive_memory, MAX_CMDBUF_SIZE*2, 
										&thiz_memory_read_pos, &thiz_memory_write_pos,
										Buffer, ByteRealRead) == 0)
		    {
			    _data_dealwith(thiz_receive_memory, MAX_CMDBUF_SIZE*2, 
										&thiz_memory_read_pos, &thiz_memory_write_pos,
										MAX_CMDBUF_SIZE*2);
		    }
	    }
        if(thiz_uart_thread == -1)
        {
            thiz_uart_thread = 0;
            break;
        }
#ifdef ECOS_OS
        GxCore_ThreadDelay(100);
#endif
    }
}


// 
static int _mutex_create(int *MutexID)
{
    int ret = 0;
    if(*MutexID > 0)
    {
        return -1;
    }
	ret = GxCore_MutexCreate(MutexID);
    return ret;
}

static int _mutex_delete(int MutexID)
{
    int ret = 0;
    if(MutexID > 0)
    {
	    ret = GxCore_MutexDelete(MutexID);
    }
    return ret;
}

static int _mutex_lock(int MutexID)
{
    int ret = 0;
    if(MutexID)
    {
	    ret = GxCore_MutexLock(MutexID);
    }
    return ret;
}

static int _mutex_unlock(int MutexID)
{
    int ret = 0;
    if(MutexID)
    {
	    ret = GxCore_MutexUnlock(MutexID);
    }
    return ret;
}

#define MESSAGE_BUFF_COUNT      4
#define MESSAGE_LEN_MAX        512
typedef struct _UartMessageClass
{
   char messages[MESSAGE_LEN_MAX];
   unsigned int message_len;
}UartMessageClass;
static UartMessageClass thiz_messages[MESSAGE_BUFF_COUNT] = {{{0},0}, {{0},0}, {{0},0}, {{0},0}};

//static char thiz_uart_messages[MESSAGE_LEN_MAX] = {0};
//static unsigned int thiz_uart_message_len = 0;
static int thiz_mutex = 0;
static int _replay_message_record(char *str, unsigned int bytes)
{
    unsigned int len = 0;
    int i = 0;
    if((NULL == str) && (0 == bytes))
    {
        return -1;
    }
    len = bytes > MESSAGE_LEN_MAX? MESSAGE_LEN_MAX: bytes;
// need mutex...
    _mutex_lock(thiz_mutex);
    for(i = 0; i < MESSAGE_BUFF_COUNT; i++)
    {
        if(0 == thiz_messages[i].message_len)
        {
            thiz_messages[i].message_len = len;
            memcpy(thiz_messages[i].messages, str, len);
            break;
        }
    }
    //thiz_uart_message_len = len;
    //memcpy(thiz_uart_messages, str, len);
    _mutex_unlock(thiz_mutex);
    return 0;
}

int factory_serialization_get_message(char* string, unsigned int data_bytes)
{
    unsigned int len = 0;
    int i = 0;

    _mutex_lock(thiz_mutex);
    if((NULL == string) /*|| (0 == thiz_uart_message_len)*/)
    {
        _mutex_unlock(thiz_mutex);
        return 0;
    }
    for(i = 0; i < MESSAGE_BUFF_COUNT; i++)
    {
        if(0 < thiz_messages[i].message_len)
        {
            len = thiz_messages[i].message_len;
            len = len > data_bytes? data_bytes: len;
            memcpy(string, thiz_messages[i].messages, len);
            thiz_messages[i].message_len = 0;
            break;
        }
    }
    //len = thiz_uart_message_len;
    //len = len > data_bytes? data_bytes: len;
    //memcpy(string, thiz_uart_messages, len);
    //thiz_uart_message_len = 0;
    _mutex_unlock(thiz_mutex);
    return len;
}

int uart_reply(char *cmd, char *str, unsigned int error_code, char *result)
{
#define REPLAY_CMD_LEN  MAX_CMDBUF_SIZE*2
	char buffer[REPLAY_CMD_LEN] = {0};
    int ret = 0;
	if((cmd == NULL) || (str == NULL))
	{
		log_printf("\nreply failed!!!\n");
		
	}
	if(result != NULL)
	{
		sprintf(buffer, "code:%d ", error_code);
		strcat(buffer, "data: ");
		strcat(buffer, result);
		strcat(buffer, " message: ");
	}
	else
	{
		sprintf(buffer, "code:%d message: ", error_code);
	}
    strcat(buffer, "reply ");
	strcat(buffer, cmd);
	strcat(buffer, " ");
	if((strlen(buffer) + strlen(str)) >= (REPLAY_CMD_LEN - 2))
	{
		char *p = NULL;
		p = strchr(str,':');
		if(p == NULL)
		{
			strcat(buffer, "failed");
		}
		else
		{
			memcpy(buffer+strlen(buffer), str, (REPLAY_CMD_LEN - strlen(buffer)));
		}
			
	}
	else
	{
		strcat(buffer, str);
	}
	strcat(buffer, "\r\n");

	ret = uart_device_send((unsigned char*)buffer,strlen(buffer));
	if(ret < 0)
	{
		log_printf("\nUART TEST, error, [%s, %d]!\n", __FUNCTION__, __LINE__);
		return -1;
	}
    //
    _replay_message_record(buffer, strlen(buffer));
	return 0;
}

extern int get_factory_serialization_flag(void);
extern int chip_otp_device_open(void);
extern int chip_otp_device_close(void);
#ifdef CUSTOMER_1_SUPPORT
extern cmd_t cmd_get_card_id;
extern cmd_t cmd_set_card_id;
extern cmd_t cmd_get_provider_id;
extern cmd_t cmd_set_provider_id;
extern cmd_t cmd_get_chip_id;
extern cmd_t cmd_lock_otp;
#endif
#ifdef CUSTOMER_2_SUPPORT
extern cmd_t cmd_get_card_id;
extern cmd_t cmd_get_dsn_id;
extern cmd_t cmd_get_chip_id;
#endif
void uart_module_init(unsigned int baudrate)
{
    int ret = 0;
    static char register_flag = 0;
    if(0 == thiz_uart_thread)
    {
        if(0 == register_flag)
        {
#ifdef CUSTOMER_1_SUPPORT
            register_uart_cmd(&cmd_get_card_id);
            register_uart_cmd(&cmd_set_card_id);
            register_uart_cmd(&cmd_get_provider_id);
            register_uart_cmd(&cmd_set_provider_id);
            register_uart_cmd(&cmd_get_chip_id);
            register_uart_cmd(&cmd_lock_otp);
#endif
#ifdef CUSTOMER_2_SUPPORT
            register_uart_cmd(&cmd_get_card_id);
            register_uart_cmd(&cmd_get_dsn_id);
            register_uart_cmd(&cmd_get_chip_id);
#endif
            register_flag = 1;
        }
        ret = uart_device_init(baudrate/*115200*/);
        if(ret < 0)
        {
            return ;
        }
        // 
        thiz_memory_read_pos = 0;
        thiz_memory_write_pos = 0;

        // create thread
        GxCore_ThreadCreate("uart console", &thiz_uart_thread, _uart_console, NULL, 50*1024, 10);
        if((0 == thiz_uart_thread) || (-1 == thiz_uart_thread))
        {
            return;
        }
    }
}

void uart_module_destroy(void)
{
    thiz_uart_thread = -1;
    while(thiz_uart_thread)
    {
        GxCore_ThreadDelay(100);
    }
    uart_device_close();
}

int factory_serialization_enter(int force_flag)
{
    if((1 == force_flag) || (1 == get_factory_serialization_flag()))
    {
        if(0 == thiz_mutex)
        {
            _mutex_create(&thiz_mutex);
            uart_module_init(115200);
            chip_otp_device_open();
        }
        return 1;
    }
    else
    {
        printf("\nDo not serializ again\n");
        return 0;
    }
}

void factory_serialization_exit(void)
{
    uart_module_destroy();
    chip_otp_device_close();
    _mutex_delete(thiz_mutex);
    thiz_mutex = 0;
}

