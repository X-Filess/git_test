/*================================================================================
- Description   : RSA verify API for Novel-SuperTV secure boot
- Version       : 4.7
- Author        : GYW
- Date          : 2015-2-13
=================================================================================*/
#ifndef CDCA_VERIFY_H
#define CDCA_VERIFY_H


/*
 注意:
 1.USING_TEST_KEY2宏在测试阶段应该定义到project的编译配置文件中;
 2.使用测试KEY2签出的bin文件不能用于量产出货等商用目的;
 3.在要签正式版app之前，要向永新视博申请正式MarketID及其对应KEY2(定义在cdca_key2.inc中)，并去除宏USING_TEST_KEY2.
*/
//#define USING_TEST_KEY2  1 //是否使用测试KEY2


//==================================Macro Definition==============================
// Return Error code
#define RE_OK 23130 //=0x5A5A

#define RE_DATA             (-1)
#define RE_DIGEST_ALGORITHM (-2)
#define RE_INVALID_LEN      (-3)
#define RE_INVALID_PADDING  (-4)


#define SHA1_HASH_SIZE 20

// The current used header version
#define USING_APP_HEADER_VER  3



#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char RSA_Public_Modulus[];
extern const unsigned char RSA_Public_Exponent[];

//==================================Signature Data Structure======================
/*
 app_header格式:
  header v1(4B)  : app_length(4B)
  header v2(12B) : app_name(8B) + app_length(4B)
  header v3(16B) : app_name(8B) + app_length(4B) + type(1B) + static_attr(1B) + dynamic_attr(2B)
 签名格式:
  app_to_sign.bin   : app_header + app_image
  app_signed.bin    : signature(256B) + app_to_sign.bin
 签名算法:
  RSASSA-PKCS1_v1_5/SHA1, 2048bit
*/
#if (USING_APP_HEADER_VER == 1)
typedef struct
{
    unsigned char	sig[256];
    unsigned int	length; /*length of app_image*/
} N_SigArea;
#elif (USING_APP_HEADER_VER == 2)
typedef struct
{
    unsigned char	sig[256];
    char			name[8];
    unsigned int	length; /*length of app_image*/
} N_SigArea;
#elif (USING_APP_HEADER_VER == 3)
typedef struct
{
    unsigned char	sig[256];
    char			name[8];
    unsigned int	length; /*length of app_image*/
    unsigned char   type;
    unsigned char   static_attr;
    unsigned short  dynamic_attr;
} N_SigArea;
#endif


//==================================Function Interface============================

/* 
 * 永新视博对App代码镜像进行签名验证的API.
 * 终端厂商的bootloader和OTAloader程序只有此API返回验证通过后，才能继续启动或下载流程.
 * 返回值:
    RE_OK  验证通过;
    other  验证失败.
 * 说明: 
 *  以下有一组API，请选择使用. 一般直接调用CDCA_Verify_One()即可.
 */

//----------------------------------Verify API------------------------------------
/* 
 * 参数:
    buf        app_signed.bin
    len        length of app_signed.bin
*/
extern int CDCA_Verify_One(const unsigned char *buf, unsigned int len);

/* 
 * 参数:
    sig_area    N_SigArea struct(signature + app_header)
    image       app_image
*/
extern int CDCA_Verify_Split(const N_SigArea *sig_area, const unsigned char *image);


/* 
 * 参数:
    digest     sha1 of app_to_sign.bin
    signature  signature of of app_to_sign.bin
*/
extern int CDCA_Verify_Digest(const unsigned char *digest, const unsigned char *signature);


//----------------------------------SHA1 API--------------------------------------
extern void CDCA_sha1(const unsigned char *data, unsigned int len, unsigned char digest[SHA1_HASH_SIZE]);
typedef struct
{
    unsigned long total[2];	/*!< number of bytes processed	*/
    unsigned long state[5];	/*!< intermediate digest state	*/
    unsigned char buffer[64];	/*!< data block being processed */
} N_sha1_context;
extern void CDCA_sha1_starts(N_sha1_context *ctx);
extern void CDCA_sha1_update(N_sha1_context *ctx, const unsigned char *input, int ilen);
extern void CDCA_sha1_finish(N_sha1_context *ctx, unsigned char output[SHA1_HASH_SIZE]);


#ifdef __cplusplus
}
#endif

#endif /* CDCA_VERIFY_H */

/*EOF*/
