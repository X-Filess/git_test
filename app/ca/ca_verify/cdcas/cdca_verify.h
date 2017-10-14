/*================================================================================
- Description   : RSA verify API for Novel-SuperTV secure boot
- Version       : 4.7
- Author        : GYW
- Date          : 2015-2-13
=================================================================================*/
#ifndef CDCA_VERIFY_H
#define CDCA_VERIFY_H


/*
 ע��:
 1.USING_TEST_KEY2���ڲ��Խ׶�Ӧ�ö��嵽project�ı��������ļ���;
 2.ʹ�ò���KEY2ǩ����bin�ļ�����������������������Ŀ��;
 3.��Ҫǩ��ʽ��app֮ǰ��Ҫ�������Ӳ�������ʽMarketID�����ӦKEY2(������cdca_key2.inc��)����ȥ����USING_TEST_KEY2.
*/
//#define USING_TEST_KEY2  1 //�Ƿ�ʹ�ò���KEY2


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
 app_header��ʽ:
  header v1(4B)  : app_length(4B)
  header v2(12B) : app_name(8B) + app_length(4B)
  header v3(16B) : app_name(8B) + app_length(4B) + type(1B) + static_attr(1B) + dynamic_attr(2B)
 ǩ����ʽ:
  app_to_sign.bin   : app_header + app_image
  app_signed.bin    : signature(256B) + app_to_sign.bin
 ǩ���㷨:
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
 * �����Ӳ���App���뾵�����ǩ����֤��API.
 * �ն˳��̵�bootloader��OTAloader����ֻ�д�API������֤ͨ���󣬲��ܼ�����������������.
 * ����ֵ:
    RE_OK  ��֤ͨ��;
    other  ��֤ʧ��.
 * ˵��: 
 *  ������һ��API����ѡ��ʹ��. һ��ֱ�ӵ���CDCA_Verify_One()����.
 */

//----------------------------------Verify API------------------------------------
/* 
 * ����:
    buf        app_signed.bin
    len        length of app_signed.bin
*/
extern int CDCA_Verify_One(const unsigned char *buf, unsigned int len);

/* 
 * ����:
    sig_area    N_SigArea struct(signature + app_header)
    image       app_image
*/
extern int CDCA_Verify_Split(const N_SigArea *sig_area, const unsigned char *image);


/* 
 * ����:
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
