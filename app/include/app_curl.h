#ifndef __APP_CURL_H__
#define __APP_CURL_H__

#include "app_utility.h"
#include "curl.h"



typedef size_t (*CurlWriteCB)(char *pcBuf,	int  nSize, int  nItems,	void *pOutStream) ;
typedef CURLcode CurlCode;
__BEGIN_DECLS

//size_t CurlWriteToFile(char *pcBuf,int  nSize,int  nItems,void *pOutStream);
size_t CurlWriteToFile(char *buffer,size_t  size,size_t  nitems,void *outstream);
//CurlCode CurlDownLoad(char *pUrl,char *pSavePath,CurlWriteCB cbWriteFun);
CurlCode CurlDownLoad(char *pUrl,char *pSavePath,curl_write_callback cbWriteFun);
CurlCode CurlHttpsDownLoad(char *pUrl,char *pSavePath,curl_write_callback cbWriteFun);


__END_DECLS

#endif
