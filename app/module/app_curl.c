
#ifdef LINUX_OS
#include "app_curl.h"
#include "comm_log.h"
#include "app_config.h"

#if YOUTUBE_ADVANCED

__BEGIN_DECLS

//size_t CurlWriteToFile(char *pcBuf,int  nSize,int  nItems,void *pOutStream)
size_t CurlWriteToFile(char *buffer,size_t  size,size_t  nitems,void *outstream)
{
	size_t ret = 0;
	size_t sizes = size * nitems;
	FILE *fp = (FILE *)(outstream);
	if(NULL==fp)
	{
		printf("--CurlWriteToFile fp is null--\n");
		return -1;
	}

	ret = fwrite(buffer,1,sizes,fp);

	return ret;
}

//CurlCode CurlDownLoad(char *pUrl,char *pSavePath,CurlWriteCB cbWriteFun)
CurlCode CurlDownLoad(char *pUrl,char *pSavePath,curl_write_callback cbWriteFun)
{
	CurlCode eRet=CURL_LAST;//CURLE_OK
	FILE* pFile=NULL;
	CURL* pCurl=NULL;

	if(NULL==pUrl || NULL==pSavePath)
	{
		return eRet;
	}

	pCurl = curl_easy_init();
	if (NULL == pCurl)
	{
		printf("--get a easy handle failed.--");
		goto CURLDOWNLOAD;
	}

	if(NULL==(pFile = fopen(pSavePath, "wb" )))
	{
		printf("--fopen pSavePath failed.--");
		goto CURLDOWNLOAD;
	}

#if 0
	//ÉèÖÃÁ¬½Ó³¬Ê±Ê±¼ä
	if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 10)))
	{
		COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_CONNECTTIMEOUT");
		goto CURLDOWNLOAD;
	}
	if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 10)))
	{
		COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_TIMEOUT");
		goto CURLDOWNLOAD;
	}
#endif

    if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1))) //disable DNS resolve timeout, it will cause crash (alarm + siglongjmp)
	{
		COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_NOSIGNAL");
		goto CURLDOWNLOAD;
	}

    if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_URL, pUrl)))
	{
		COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_URL");
		goto CURLDOWNLOAD;
	}

	if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, (void*)pFile)))
	{
		COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_WRITEDATA");
		goto CURLDOWNLOAD;
	}

	//ÉèÖÃ»Øµ÷º¯Êý
	if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, cbWriteFun)))
	{
		COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_WRITEFUNCTION");
		goto CURLDOWNLOAD;
	}

	if(CURLE_OK!=(eRet =curl_easy_perform(pCurl)))
	{
		COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"curl_easy_perform");
		goto CURLDOWNLOAD;
	}

CURLDOWNLOAD:
    if(NULL!=pCurl)
    {
        curl_easy_cleanup(pCurl);
        pCurl = NULL;
    }
	if(NULL!=pFile)
    {
        fclose(pFile);
        pFile = NULL;
    }

	return eRet;
}

CurlCode CurlHttpsDownLoad(char *pUrl,char *pSavePath,curl_write_callback cbWriteFun)
{
    CurlCode eRet=CURL_LAST;//CURLE_OK
    FILE* pFile=NULL;
    CURL* pCurl=NULL;

    if(NULL==pUrl || NULL==pSavePath)
    {
        return eRet;
    }
    	if(CURLE_OK!=(eRet =curl_global_init(CURL_GLOBAL_SSL)))//³õÊ¼»¯libcurl
    	{
    		COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"curl_global_init");
    		goto CURLHTTPSDOWNLOAD;
    	}
    pCurl = curl_easy_init();
    if (NULL == pCurl)
    {
        printf("--get a easy handle failed.--");
        goto CURLHTTPSDOWNLOAD;
    }
    if(NULL==(pFile = fopen(pSavePath, "wb" )))
    {
        printf("--fopen pSavePath failed.--");
        goto CURLHTTPSDOWNLOAD;
    }
#if 0
    //ÉèÖÃÁ¬½Ó³¬Ê±Ê±¼ä
    if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 10)))
    {
        COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_CONNECTTIMEOUT");
        goto CURLHTTPSDOWNLOAD;
    }
    if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 20)))
    {
        COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_TIMEOUT");
        goto CURLHTTPSDOWNLOAD;
    }
#endif
    if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_URL, pUrl)))
    {
        COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_URL");
        goto CURLHTTPSDOWNLOAD;
    }	

    if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, false)))
    {
        COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_SSL_VERIFYPEER");
        goto CURLHTTPSDOWNLOAD;
    }
    //	if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_CAPATH, "/dvb/ssl/certs")))
    //	{
    //		COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_SSL_VERIFYPEER");
    //		goto CURLHTTPSDOWNLOAD;
    //	}	


    if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_VERBOSE,0L)))
    {
        COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_VERBOSE");
        goto CURLHTTPSDOWNLOAD;
    }		
    //if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_CAINFO,"/dvb/cacert.pem")))
    //if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_CAINFO,"/dvb/ssl/certs/ca-certificates.crt")))
    //	if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_CAINFO,"/dvb/ssl/certs/cacert.pem")))
    //	{
    //		COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_CAINFO");
    //		goto CURLHTTPSDOWNLOAD;
    //	}	
    if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, false)))
    {
        COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_SSL_VERIFYHOST");
        goto CURLHTTPSDOWNLOAD;
    }			
    if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, (void*)pFile)))
    {
        COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_WRITEDATA");
        goto CURLHTTPSDOWNLOAD;
    }		
    //ÉèÖÃ»Øµ÷º¯Êý
    if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, cbWriteFun)))
    {
        COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_WRITEFUNCTION");
        goto CURLHTTPSDOWNLOAD;
    }		
    //	if(CURLE_OK!=(eRet =curl_easy_setopt(pCurl, CURLOPT_CERTINFO, 1L)))
    //	{
    //		COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"CURLOPT_CERTINFO");
    //		goto CURLHTTPSDOWNLOAD;
    //	}	


    if(CURLE_OK!=(eRet =curl_easy_perform(pCurl)))
    {
        COMM_LOG("curl",LOG_ERROR,"curl error[%d]:%s",eRet,"curl_easy_perform");
        goto CURLHTTPSDOWNLOAD;
    }	
    //      union {
    //        struct curl_slist    *to_info;
    //        struct curl_certinfo *to_certinfo;
    //      } ptr;
    //
    //      ptr.to_info = NULL;
    //
    //      eRet = curl_easy_getinfo(pCurl, CURLINFO_CERTINFO, &ptr.to_info);
    //
    //      if(!eRet && ptr.to_info) {
    //        int i;
    //
    //        printf("%d certs!\n", ptr.to_certinfo->num_of_certs);
    //
    //        for(i = 0; i < ptr.to_certinfo->num_of_certs; i++) {
    //          struct curl_slist *slist;
    //
    //          for(slist = ptr.to_certinfo->certinfo[i]; slist; slist = slist->next)
    //            printf("%s\n", slist->data);
    //
    //        }   
    //      }   	


CURLHTTPSDOWNLOAD:
    if(NULL!=pCurl)curl_easy_cleanup(pCurl);pCurl=NULL;
    if(NULL!=pFile)fclose(pFile);pFile=NULL;

    return eRet;
}

__END_DECLS
#endif
#endif


