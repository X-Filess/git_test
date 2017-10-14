#include <string.h>
#include "gxavdev.h"
#include "include/descrambler.h"

#define DEMUX_DEVICE_ID                 (0)
#define MAX_DEMUX_SUPPORT	            (2)
#define MAGIC		                    (0x005a5359)

#define CHECK_HANDLE(handle)		do{\
											if(*(unsigned int*)handle != MAGIC)\
											{\
												printf("This is a wrong hanle!!!\n");\
												return 0;\
											}\
										}while(0)
/*#ifndef ASSERT
#define ASSERT(value)               do{\
                                            if(!value)\
                                            {\
                                                printf("CA DESCRAMBLER, wrong!!!!");\
                                                while(1);\
                                            }\
                                      }while(0)
#endif*/
struct descrambler_s
{
	unsigned int		magic;/*用于检测控制块的有效性*/
	int			        lock;/*打开的互斥锁句柄*/
	int	                h_devive;/*打开的设备句柄*/
	int	                h_demux;/*打开的demux模块句柄*/
	int	                h_slot;/*需要解扰的slot句柄,因为slot和pid是唯一对应的,为了用户使用方便,模块掩盖了slot句柄,统一用stream_pid来对应,保留待扩展用*/
	int	                descrambler_id;/*获取到的解扰器id*/
	unsigned int	    stream_pid;/*需要解扰的包pid值,用于对应slotid*/
};

static unsigned int GET_KEY_HIGH(const unsigned char *key_buf)
{
    if(key_buf == NULL)
        return 0;

    return ((unsigned int)(key_buf[3] + (key_buf[2] << 8)+ (key_buf[1] << 16) + (key_buf[0] << 24)));
}

static unsigned int GET_KEY_LOW(const unsigned char *key_buf)
{
    if(key_buf == NULL)
        return 0;

    return ((unsigned int)(key_buf[7] + (key_buf[6] << 8)+ (key_buf[5] << 16) + (key_buf[4] << 24)));
}

static void RESTORE_KEY_HIGHT(unsigned int key, unsigned char *key_buf)
{
    if(key_buf == NULL)
        return;

    key_buf[0] = (key >> 24) & 0xff;
    key_buf[1] = (key >> 16) & 0xff;
    key_buf[2] = (key >> 8) & 0xff;
    key_buf[3] = key & 0xff;
}

static void RESTORE_KEY_LOW(unsigned int key, unsigned char *key_buf)
{
    if(key_buf == NULL)
        return;

    key_buf[4] = (key >> 24) & 0xff;
    key_buf[5] = (key >> 16) & 0xff;
    key_buf[6] = (key >> 8) & 0xff;
    key_buf[7] = key & 0xff;
}

/**
* @brief      open a descrambler and return the handle
* @param []	DemuxID :demux id .0~1 is valid
* @return      handle == 0 failure;or sucess
*/
int Descrmb_Open(unsigned int  DemuxID)
{
	int                     ret;
	GxDemuxProperty_CA          property;
	struct descrambler_s*    p_descrambler = NULL;

	if(DemuxID>=MAX_DEMUX_SUPPORT)
	{
		printf("Input param err!\n");
		return 0;
	}

	p_descrambler = GxCore_Malloc(sizeof(struct descrambler_s));
	if(p_descrambler == NULL)
	{
		printf("Malloc failure\n");
		return 0;
	}

	memset(p_descrambler,0,sizeof(struct descrambler_s));
	p_descrambler->magic = MAGIC;
	GxCore_MutexCreate(&p_descrambler->lock);

	p_descrambler->h_devive = GxAvdev_CreateDevice(DEMUX_DEVICE_ID);
	if(p_descrambler->h_devive < 0)
	{
		GxCore_Free(p_descrambler);
		printf("Open device err\n");
		return 0;
	}


	p_descrambler->h_demux = GxAvdev_OpenModule(p_descrambler->h_devive, 
								GXAV_MOD_DEMUX, DemuxID);
	if(p_descrambler->h_demux < 0)
	{
		printf("Open module err\n");
		GxAvdev_DestroyDevice(p_descrambler->h_devive);
		GxCore_Free(p_descrambler);
		return 0;
	}

	ret = GxAVGetProperty(p_descrambler->h_devive,
							p_descrambler->h_demux,
							GxDemuxPropertyID_CAAlloc,
							&property,
							sizeof(GxDemuxProperty_CA));
	if(ret < 0)
	{
		printf("Allco descrambler err\n");
		GxAvdev_CloseModule(p_descrambler->h_devive, p_descrambler->h_demux);
		GxAvdev_DestroyDevice(p_descrambler->h_devive);
		GxCore_Free(p_descrambler);
		return 0;
	}

	p_descrambler->descrambler_id= property.ca_id;
	return (int)p_descrambler;
}

/**
* @brief        close a descrambler release resourse
* @param []	    Descrambler :handle of descrambler,get via Descrmb_Open();
* @return       int  >= 0, sucess.<0 failure
*/
int Descrmb_Close(int Descrambler)
{
	struct descrambler_s* p_descrambler = (struct descrambler_s*)Descrambler;
	GxDemuxProperty_CA       p;
	int ret;

	CHECK_HANDLE(Descrambler);

	GxCore_MutexLock(p_descrambler->lock);

	p.ca_id = p_descrambler->descrambler_id;
	ret = GxAVSetProperty(p_descrambler->h_devive,
						p_descrambler->h_demux,
						GxDemuxPropertyID_CAFree,
						&p,
						sizeof(GxDemuxProperty_CA));
	if(ret < 0)
	{
		printf("Free descrambler err\n");
	    GxCore_MutexUnlock(p_descrambler->lock);
        return -1;
	}
	GxAvdev_CloseModule(p_descrambler->h_devive, p_descrambler->h_demux);
	GxAvdev_DestroyDevice(p_descrambler->h_devive);
	/*改变魔数值,防止释放后被非法调用*/
	p_descrambler->magic = 0;
	GxCore_MutexUnlock(p_descrambler->lock);
	GxCore_MutexDelete(p_descrambler->lock);
	GxCore_Free(p_descrambler);


	return 0;
}

/**
* @brief    tell descrambler that which pid channel need to bind. the pid will used to find the
		    id of slot,then descrambler can descramble the stream.
* @param []	Descrambler :handle of descrambler,get via Descrmb_Open();
* @param []	StreamPID :The pid of stream;
* @return   int  >= 0, sucess.<0 failure
*/
int Descrmb_SetStreamPID(int Descrambler, unsigned short StreamPID)
{
	struct descrambler_s*        p_descrambler;
	CHECK_HANDLE(Descrambler);
	p_descrambler = (struct descrambler_s*)Descrambler;

//	ASSERT(StreamPID <= 0x1FFF);

	GxCore_MutexLock(p_descrambler->lock);
	p_descrambler->stream_pid = StreamPID;
	GxCore_MutexUnlock(p_descrambler->lock);
	return 0;
}

/**
* @brief      Send odd key to descrambler.
* @param []	Descrambler :handle of descrambler,get via Descrmb_Open();
* @param [in] OddKey :buffer of odd key
* @param [] KeyLen :length of key
* @param [] DecryptFlag: DMX_CA_DES_MODE(DES MODE), 0(CAS2 MODE)
 * @return      int  >= 0, sucess.<0 failure
*/
int Descrmb_SetOddKey(  int Descrambler, IN const unsigned char* OddKey, unsigned int KeyLen, int DecryptFlag)
{
	int                         status;
	GxDemuxProperty_CA              ca_property = {0,};
	GxDemuxProperty_SlotQueryByPid  slot_property = {0,};
	struct descrambler_s*        p_descrambler;
	CHECK_HANDLE(Descrambler);
	p_descrambler = (struct descrambler_s*)Descrambler;

//	ASSERT(OddKey != NULL);

	GxCore_MutexLock(p_descrambler->lock);
	slot_property.pid           = p_descrambler->stream_pid;
	status = GxAVGetProperty(p_descrambler->h_devive,
							p_descrambler->h_demux,
							GxDemuxPropertyID_SlotQueryByPid,
							&slot_property,
							sizeof(GxDemuxProperty_SlotQueryByPid));
	if(status>=0)
	{
		ca_property.ca_id           = p_descrambler->descrambler_id;
		ca_property.slot_id         = slot_property.slot_id;
		ca_property.flags           = DMX_CA_KEY_ODD | DecryptFlag;
		ca_property.odd_key_high    = GET_KEY_HIGH( OddKey );
		ca_property.odd_key_low     = GET_KEY_LOW( OddKey );

		status = GxAVSetProperty(   p_descrambler->h_devive,
								p_descrambler->h_demux,
								GxDemuxPropertyID_CAConfig,
								&ca_property,
								sizeof(GxDemuxProperty_CA) );
		if(status< 0)
		{
			printf("Set key err!\n");
			GxCore_MutexUnlock(p_descrambler->lock);
			return -1;
		}
		GxCore_MutexUnlock(p_descrambler->lock);
		return 0;
	}
	printf("Can't find slot id via pid\n");
	GxCore_MutexUnlock(p_descrambler->lock);
	return -1;
}

/**
* @brief      Send even key to descrambler.
* @param []	Descrambler :handle of descrambler,get via Descrmb_Open();
* @param [in] EvenKey :buffer of even key
* @param [] KeyLen :length of key
* @param [] DecryptFlag: DMX_CA_DES_MODE(DES MODE), 0(CAS2 MODE)
 * @return      int  >= 0, sucess.<0 failure
*/
int Descrmb_SetEvenKey( int Descrambler, IN const unsigned char*  EvenKey, unsigned int KeyLen, int DecryptFlag)
{
	int                         status;
	GxDemuxProperty_CA              ca_property = {0,};
	GxDemuxProperty_SlotQueryByPid  slot_property = {0,};
	struct descrambler_s*        p_descrambler;
	CHECK_HANDLE(Descrambler);
	p_descrambler = (struct descrambler_s*)Descrambler;

//	ASSERT(EvenKey != NULL);

	GxCore_MutexLock(p_descrambler->lock);
	slot_property.pid           = p_descrambler->stream_pid;
	status = GxAVGetProperty(p_descrambler->h_devive,
							p_descrambler->h_demux,
							GxDemuxPropertyID_SlotQueryByPid,
							&slot_property,
							sizeof(GxDemuxProperty_SlotQueryByPid));
	if(status>=0)
	{
		ca_property.ca_id           = p_descrambler->descrambler_id;
		ca_property.slot_id         = slot_property.slot_id;
		ca_property.flags           = DMX_CA_KEY_EVEN | DecryptFlag;
		ca_property.even_key_high   = GET_KEY_HIGH( EvenKey );
		ca_property.even_key_low    = GET_KEY_LOW( EvenKey );

		status = GxAVSetProperty(   p_descrambler->h_devive,
								p_descrambler->h_demux,
								GxDemuxPropertyID_CAConfig,
								&ca_property,
								sizeof(GxDemuxProperty_CA) );
		if(status< 0)
		{
			printf("Set key err!\n");
			GxCore_MutexUnlock(p_descrambler->lock);
			return -1;
		}
		GxCore_MutexUnlock(p_descrambler->lock);
		return 0;
	}
	printf("Can't find slot id via pid\n");
	GxCore_MutexUnlock(p_descrambler->lock);
	return -1;
}


/**
* @brief      Send cw (odd key & even key) to descrambler.
* @param []	Descrambler :handle of descrambler,get via Descrmb_Open();
* @param [in] OddKey :buffer of odd key
* @param [in] EvenKey :buffer of even key
* @param [] KeyLen :length of key
* @param [] DecryptFlag: DMX_CA_DES_MODE(DES MODE), 0(CAS2 MODE)
* @return      int  >= 0, sucess.<0 failure
* @note		we sugest to use this interface, user can save even key or odd key ,and send them to
			descrambler together.
*/
int Descrmb_SetCW(int Descrambler,
                                const unsigned char* OddKey,
                                const unsigned char* EvenKey,
                                unsigned int KeyLen,
                                int DecryptFlag)
{
	int                         status;
	GxDemuxProperty_CA              ca_property ={0,};
	GxDemuxProperty_SlotQueryByPid  slot_property = {0,};
	struct descrambler_s*        p_descrambler = NULL;
	CHECK_HANDLE(Descrambler);

	p_descrambler = (struct descrambler_s*)Descrambler;

//	ASSERT(Descrambler != 0);
//	ASSERT(EvenKey != NULL);
//	ASSERT(OddKey != NULL);
//	ASSERT(KeyLen <= 8);

	GxCore_MutexLock(p_descrambler->lock);
	slot_property.pid           = p_descrambler->stream_pid;

	status = GxAVGetProperty(p_descrambler->h_devive,
							p_descrambler->h_demux,
							GxDemuxPropertyID_SlotQueryByPid,
							&slot_property,
							sizeof(GxDemuxProperty_SlotQueryByPid));

	if(status >= 0)
	{
		ca_property.ca_id           = p_descrambler->descrambler_id;
		ca_property.slot_id         = slot_property.slot_id;
		ca_property.flags           = DMX_CA_KEY_EVEN | DMX_CA_KEY_ODD | DecryptFlag;

		/*寄存器大端，库大端*/
		ca_property.odd_key_high    = GET_KEY_HIGH(OddKey);
		ca_property.odd_key_low     = GET_KEY_LOW(OddKey);
		ca_property.even_key_high   = GET_KEY_HIGH(EvenKey);
		ca_property.even_key_low    = GET_KEY_LOW(EvenKey);

		status = GxAVSetProperty(   p_descrambler->h_devive,
								p_descrambler->h_demux,
								GxDemuxPropertyID_CAConfig,
								&ca_property,
								sizeof(GxDemuxProperty_CA) );
		if(status< 0)
		{
			printf("Set key err!\n");
			GxCore_MutexUnlock(p_descrambler->lock);
			return -1;
		}
		GxCore_MutexUnlock(p_descrambler->lock);
		return 0;
	}
	printf("Can't find slot id via pid\n");
	GxCore_MutexUnlock(p_descrambler->lock);
	return -1;
}

int Descrmb_GetCW(int Descrambler,
                                unsigned char* OddKey,
                                unsigned char* EvenKey)
{
	int                         status;
	GxDemuxProperty_CA              ca_property ={0,};
	GxDemuxProperty_SlotQueryByPid  slot_property = {0,};
	struct descrambler_s*        p_descrambler = NULL;
	CHECK_HANDLE(Descrambler);

	p_descrambler = (struct descrambler_s*)Descrambler;

//	ASSERT(Descrambler != 0);
//	ASSERT(EvenKey != NULL);
//	ASSERT(OddKey != NULL);

	GxCore_MutexLock(p_descrambler->lock);
	slot_property.pid           = p_descrambler->stream_pid;

	status = GxAVGetProperty(p_descrambler->h_devive,
							p_descrambler->h_demux,
							GxDemuxPropertyID_SlotQueryByPid,
							&slot_property,
							sizeof(GxDemuxProperty_SlotQueryByPid));

	if(status >= 0)
	{
		ca_property.ca_id           = p_descrambler->descrambler_id;
		ca_property.slot_id         = slot_property.slot_id;
		ca_property.flags           = DMX_CA_KEY_EVEN | DMX_CA_KEY_ODD;

		status = GxAVGetProperty(p_descrambler->h_devive,
								p_descrambler->h_demux,
								GxDemuxPropertyID_CAConfig,
								&ca_property,
								sizeof(GxDemuxProperty_CA) );

        RESTORE_KEY_HIGHT(ca_property.odd_key_high,OddKey);
        RESTORE_KEY_LOW(ca_property.odd_key_low,OddKey);
        RESTORE_KEY_HIGHT(ca_property.even_key_high,EvenKey);
        RESTORE_KEY_LOW(ca_property.even_key_low,EvenKey);

		if(status< 0)
		{
			printf("Get key err!\n");
			GxCore_MutexUnlock(p_descrambler->lock);
			return -1;
		}
		GxCore_MutexUnlock(p_descrambler->lock);
		return 0;
	}
	printf("Can't find slot id via pid\n");
	GxCore_MutexUnlock(p_descrambler->lock);
	return -1;
}

//set dsk/dck/dcw/vcw/mcw dcw字节序规则奇+偶
//1 : dcw
//2 : dck / dcw
//3 : dsk / dck / dcw
//4 : dsk / dck / dcw / vcw
//5 : dsk / dck / dcw / vcw / mcw
//
static void _reverse(IN unsigned char *key, IN unsigned int key_len, OUT unsigned char *buf)
{
    if((NULL == key) || (0 == key_len) || (NULL == buf))
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return;
    }
    int i = 0;
    for(i = 0; i < key_len; i++)
    {
        buf[i] = key[(key_len - i - 1)];
    }
}

int Descrmb_MTCSet(int Descrambler,
                                enum mtc_api_desc_type   des_type,
                                /*struct mtc_api_input_s*/void *keys,
                                enum mtc_arith_type arith_type,
                                enum mtc_arith_mode arith_mode)
{
    struct descrambler_s*        p_descrambler = NULL;
    struct mtc_api_input_s *data = NULL;
    struct mtc_api_input_s  para = {0};
    int i = 0;
    int ret = 0;
   
	CHECK_HANDLE(Descrambler);
	p_descrambler = (struct descrambler_s*)Descrambler;

//	ASSERT(Descrambler != 0);
    if(NULL == keys)
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
    data = (struct mtc_api_input_s*)keys;
    if(1 > data->num)
    {
        printf("\nCAS, error, %s, %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

	GxCore_MutexLock(p_descrambler->lock);
    para.num = data->num;
    for(i = 0; i < data->num; i++)
    {
        if(0 >= data->size[i])
        {
            printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
            goto exit;
        }
        para.buf[i] = GxCore_Malloc(data->size[i]);
        if(NULL == para.buf[i])
        {
            printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
            goto exit;
        }
        _reverse(data->buf[i], data->size[i], para.buf[i]);
        para.size[i] = data->size[i];
    }
    ret = GxMtc_CwDecrypt(&para, des_type, p_descrambler->descrambler_id, arith_type, arith_mode);
exit:
    GxCore_MutexUnlock(p_descrambler->lock);
    for(i = 0; i < data->num; i++)
    {
        if(para.buf[i])
        {
            GxCore_Free(para.buf[i]);
        }
    }
    return ret;
}


