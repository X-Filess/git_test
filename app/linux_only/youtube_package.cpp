#include "youtube_package.h"
#include "app_xml_config.h"
#include "app_config.h"

#if YOUTUBE_ADVANCED

#define YTB_SWAP      "Swap"
#define YTB_SPLICE    "splice"
#define YTB_SLICE     "slice"
#define YTB_REVERSE   "reverse"

YtbDecrypt* YtbDecrypt::lInstance=0;
YtbDecrypt::GarbageCollect YtbDecrypt::lGarbage;

typedef map<string,string> UrlMemMap;

bool CreateTcpSocketByHost(char *pcHost,int nPort,int *pnOutSock)
{
#define IPV6_SUPPORT (0)
	bool bRet=false;
	int  nAddrInfoPort = 0;
	char sAddrInfoIp[INET6_ADDRSTRLEN] = {0};
	char sServiceName[10]={0};
	struct addrinfo *pAddrInfoResult= NULL;
	struct sockaddr_in *pSockAddrIn= NULL;
	struct addrinfo hints={0};

	if(nPort<=0 || NULL==pcHost)return bRet;

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	snprintf(sServiceName,sizeof(sServiceName),"%d",nPort);
	if(getaddrinfo(pcHost, sServiceName, &hints, &pAddrInfoResult)){
		printf("tcp: %s error, getaddrinfo return null\n",__FUNCTION__);
		return bRet;
	}   

HOST_FOREACH:
	if(pAddrInfoResult == NULL){
		printf("cannot find hostname\n");
		freeaddrinfo(pAddrInfoResult);
		return bRet;
	}   
	pSockAddrIn = (struct sockaddr_in*)pAddrInfoResult->ai_addr;
	if(!IPV6_SUPPORT && pAddrInfoResult->ai_family==AF_INET6)
	{
		printf("ipv6 is not support!!\n");
		pAddrInfoResult = pAddrInfoResult->ai_next;
		goto HOST_FOREACH;		
	}

	nAddrInfoPort = ntohs(pSockAddrIn->sin_port);
	if(nAddrInfoPort != nPort){
		printf("nAddrInfoPort:%d nPort:%d\n",nAddrInfoPort,nPort);
		pAddrInfoResult = pAddrInfoResult->ai_next;
		goto HOST_FOREACH;
	}

	if(false==InetNtop(pAddrInfoResult->ai_family,&pSockAddrIn->sin_addr,sAddrInfoIp,sizeof(sAddrInfoIp)))
	{
		pAddrInfoResult = pAddrInfoResult->ai_next;
		goto HOST_FOREACH;		
	}
	if(false==Socket(pAddrInfoResult->ai_family,SOCK_STREAM,0,pnOutSock))
	{
		pAddrInfoResult = pAddrInfoResult->ai_next;
		goto HOST_FOREACH;				
	}
	if(false==Connect(*pnOutSock,(const struct sockaddr *)pSockAddrIn,sizeof(struct sockaddr)))
	{
		pAddrInfoResult = pAddrInfoResult->ai_next;
		goto HOST_FOREACH;				
	}	
	bRet=true;	
	freeaddrinfo(pAddrInfoResult);
	return bRet;
}

#if 0
static status_t YtbGetPlayerConfig(const char *pcSrc,char **ppcJsonPlayerConfig)
{
	status_t ret=GXCORE_ERROR;//GXCORE_SUCCESS
	pcre  *pPcre=NULL;
	const char *pcError=NULL;
	int  nErrOffset;
	int  nOutVector[8]={0};
	int  nRc=0;
	char  sPattern [] = ";ytplayer.config = ({.*?});";    
	if(NULL==pcSrc)return ret;
	pPcre = pcre_compile(sPattern,       // pattern, 输入参数,将要被编译的字符串形式的正则表达式
			0,            // options, 输入参数,用来指定编译时的一些选项
			&pcError,       // errptr, 输出参数,用来输出错误信息
			&nErrOffset,   // erroffset, 输出参数,pattern中出错位置的偏移量
			NULL);        // tableptr, 输入参数,用来指定字符表,一般情况用NULL	
	if (pPcre == NULL) {                 //如果编译失败,返回错误信息
		COMM_LOG("youtube",LOG_ERROR,"PCRE compilation failed at offset %d: %s/n", nErrOffset, pcError);
		//printf("---gcyin:%s---[%d]--\n",__FILE__,__LINE__);
		return ret;
	}	
	nRc = pcre_exec(pPcre,            // code, 输入参数,用pcre_compile编译好的正则表达结构的指针
			NULL,          // extra, 输入参数,用来向pcre_exec传一些额外的数据信息的结构的指针
			pcSrc,           // subject, 输入参数,要被用来匹配的字符串
			strlen(pcSrc),  // length, 输入参数, 要被用来匹配的字符串的指针
			0,             // startoffset, 输入参数,用来指定subject从什么位置开始被匹配的偏移量
			0,             // options, 输入参数, 用来指定匹配过程中的一些选项
			nOutVector,       // ovector, 输出参数,用来返回匹配位置偏移量的数组
			ARRAY_LEN(nOutVector));    // ovecsize, 输入参数, 用来返回匹配位置偏移量的数组的最大大小
	// 返回值：匹配成功返回非负数,没有匹配返回负数
	//    if (nRc < 0) {                     //如果没有匹配,返回错误信息
	//        if (nRc == PCRE_ERROR_NOMATCH) printf("Sorry, no match .../n");
	//        else printf("Matching error %d/n", nRc);
	//		goto YTBGETPLAYERCONFIG;
	//    }
	//printf("/nOK, has matched .../n/n");   //没有出错,已经匹配
	if(nRc>1)
	{
		const char *pcStrStart = pcSrc + nOutVector[2];
		int nStrLen = nOutVector[3] - nOutVector[2];
		if(NULL==((*ppcJsonPlayerConfig)=((char *)GxCore_Malloc(nStrLen+1))))
		{
			printf("--GxCore_Malloc  is error--\n");
			goto YTBGETPLAYERCONFIG;
		}else
		{
			memset(*ppcJsonPlayerConfig,0,nStrLen+1);
			memcpy(*ppcJsonPlayerConfig,pcStrStart,nStrLen);
		}

	}else{
		if (nRc == PCRE_ERROR_NOMATCH) printf("Sorry, no match .../n");
		else printf("Matching error %d\n", nRc);
		goto YTBGETPLAYERCONFIG;		
	}
	ret=GXCORE_SUCCESS;

YTBGETPLAYERCONFIG:
	if(ret!=GXCORE_SUCCESS)
	{
		GXCORE_FREE(*ppcJsonPlayerConfig);
	}
	if(NULL!=pPcre)pcre_free(pPcre);pPcre=NULL; // 编译正则表达式re 释放内存	

	return ret;
}
#endif

static status_t YtbGetPlayerConfigQuick(string &sWatchContent,string &sJsonPlayerOut)
{
#define PLAYER_CONFIG_LEN_MAX (40960)  //假定player config json 数据最大值
	status_t ret=GXCORE_ERROR;

	string::size_type nStart,nEnd;
	int nFlag=0;
	string sPattern= ";ytplayer.config = ";   
	if(sWatchContent.empty())return ret;
	nStart=sWatchContent.find(sPattern);
	if(nStart<sWatchContent.length())
	{
		nStart=nStart+sPattern.length();
		nEnd=sWatchContent.find("};",nStart);
		if(nEnd>=sWatchContent.length())
		{
			nFlag=1;
			nEnd=sWatchContent.find(";</",nStart);
		}
		if(nEnd<sWatchContent.length())
		{
			sJsonPlayerOut=sWatchContent.substr(nStart,nEnd-nStart);
			if(nFlag==0)sJsonPlayerOut.append("}");
			if(sJsonPlayerOut.length()>PLAYER_CONFIG_LEN_MAX)
			{
				COMM_LOG("youtube",LOG_ERROR,"nStart=%d,nEnd=%d,%s",nStart,nEnd,"sJsonPlayerOut.length is too long");
				sJsonPlayerOut="";
				goto YTBGETPLAYERCONFIGQUICK;                
			}
		}else
		{
			COMM_LOG("youtube",LOG_ERROR,"nStart=%d,nEnd=%d,%s",nStart,nEnd,"not find proper sJsonPlayerOut");
			goto YTBGETPLAYERCONFIGQUICK;
		}           
	}else
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","not find ;ytplayer.config");
		goto YTBGETPLAYERCONFIGQUICK;
	}      

	COMM_LOG("youtube",LOG_DEBUG,"%s","find sJsonPlayerOut ...");
	ret=GXCORE_SUCCESS;

YTBGETPLAYERCONFIGQUICK:	
	return ret;
}


static status_t YtbGetVideoUrlList(const string sVideoId,string &sWatchContent,vector<UrlMemMap> &vUrlListMap)
{
	status_t ret=GXCORE_ERROR;//GXCORE_SUCCESS
	string sJsonPlayer;     
	json_t *pRoot=NULL,*pArgs=NULL,*pEncodedUrl=NULL;
	const char *pcEncodedUrl=NULL;
	const char *pTmp=NULL;
	vector<string> vUrlList;
	YtbDecrypt *pDecrypt=NULL;
	//vector<UrlMemMap> vUrlListMap;
	//bool bFirst=true;

	if(sWatchContent.empty())return ret;

	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	if(GXCORE_SUCCESS!=YtbGetPlayerConfigQuick(sWatchContent,sJsonPlayer))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","YtbGetPlayerConfigQuick is error");
		//printf("---gcyin:%s---[%d]--\n",__FILE__,__LINE__);
		goto YTBGETVIDEOURLLIST;
	}	

	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	if(NULL==(pRoot=json_loads((char *)(sJsonPlayer.c_str()),0,NULL)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","json file load failed");
		COMM_LOG("youtube",LOG_ERROR,"sJsonPlayer=%s",sJsonPlayer.c_str());
		goto YTBGETVIDEOURLLIST;
	}

	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	pArgs= json_object_get(pRoot, "args");
	if(!json_is_object(pArgs))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","not an obj");
		goto YTBGETVIDEOURLLIST;
	}
	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	pEncodedUrl = json_object_get(pArgs, "url_encoded_fmt_stream_map");//url_encoded_fmt_stream_map
	if(!json_is_string(pEncodedUrl))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","not an string");
		goto YTBGETVIDEOURLLIST;
	}
	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	pcEncodedUrl=json_string_value(pEncodedUrl);
	if(IS_EMPTY_OR_NULL(pcEncodedUrl))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","pEncodedUrl string is empty");
		goto YTBGETVIDEOURLLIST;		
	}

	pTmp= strtok ((char *)pcEncodedUrl,","); //分割字符串
	while(pTmp)
	{
		vUrlList.push_back(pTmp);//在容器尾部加入一个数据
		pTmp = strtok(NULL,","); //指向下一个指针
	}
	if(vUrlList.size()==0)
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","vUrlList.size is zero");
		goto YTBGETVIDEOURLLIST;			
	}

	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	for(vector<string>::iterator iterUrl= vUrlList.begin();iterUrl != vUrlList.end();iterUrl++)
	{
		//printf("iterUrl=%s\n\n\n",(*iterUrl).c_str());
		if((*iterUrl).find("&")==string::npos)
		{
			vUrlList.erase(iterUrl);
		}
		vector<string> vKeyAndValueList;
		pTmp= strtok((char *)(*iterUrl).c_str(),"&"); //分割字符串
		while(pTmp)
		{
			vKeyAndValueList.push_back(pTmp);//在容器尾部加入一个数据
			pTmp = strtok(NULL,"&"); //指向下一个指针
		}
		UrlMemMap memMap;
		for(vector<string>::iterator iterEqual= vKeyAndValueList.begin();iterEqual != vKeyAndValueList.end();iterEqual++)
		{
			//printf("iterEqual=%s\n\n\n",(*iterEqual).c_str());
			if((*iterEqual).find("=")==string::npos)
			{
				vKeyAndValueList.erase(iterEqual);
				continue;
			}	

			char *pTmp0=NULL,*pTmp1=NULL;
			pTmp0= strtok((char *)(*iterEqual).c_str(),"="); //分割字符串	
			pTmp1= strtok(NULL, "=");
			//printf("pTmp0=%s----pTmp1=%s\n\n",pTmp0,pTmp1);
			if (pTmp1) 
			{
				//memMap.insert(pair<string,string>(pTmp0,pTmp1));
				memMap[pTmp0]=UrlDecode(pTmp1);
			}else
			{
				//memMap.insert(pair<string,string>(pTmp0,""));
				memMap[pTmp0]="";
			}
		}
		//printf("memMap.size()=%u\n\n",memMap.size());
		vUrlListMap.push_back(memMap);

	}

	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	//YtbGetDecryptJsContent(pcSrc,ttt);
	//decrypt

	for(unsigned int i=0;i<vUrlListMap.size();i++)
	{
		//map<string,string>::iterator iter=vUrlListMap[i].find("sig");
		if(vUrlListMap[i].find("sig")!=vUrlListMap[i].end())
		{
			vUrlListMap[i]["url"]=vUrlListMap[i]["url"]+"&signature="+vUrlListMap[i]["sig"];
		}
		else if(vUrlListMap[i].find("s")!=vUrlListMap[i].end())
		{
			string sEncryptedSig=vUrlListMap[i]["s"];
			pDecrypt=YtbDecrypt::GetInstance();

			if(NULL==pDecrypt || false==pDecrypt->GetInitStatus())
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","pDecrypt is error");
				goto YTBGETVIDEOURLLIST;                    
			}
			else
			{
				string sDecryptSig;
				if(GXCORE_SUCCESS!=pDecrypt->GetDecryptSignature(sVideoId,sEncryptedSig,sDecryptSig))
				{
					COMM_LOG("youtube",LOG_ERROR,"%s","GetDecryptSignature is error");
					goto YTBGETVIDEOURLLIST;                          
				}
				vUrlListMap[i]["url"]=vUrlListMap[i]["url"]+"&signature="+sDecryptSig;
			}
		}
	}	

	for(unsigned int i=0;i<vUrlListMap.size();i++)
	{
		for(map<string,string>::iterator iter= vUrlListMap[i].begin();iter != vUrlListMap[i].end();iter++)
		{
			StringReplace(vUrlListMap[i]["url"],"\n","");
			//printf("first:%s\t\tsecond:%s\n",iter->first.c_str(),iter->second.c_str());
		}
		//printf("------\n");
	}	

	ret=GXCORE_SUCCESS;

YTBGETVIDEOURLLIST:
	json_decref(pRoot);

	return ret;
}

status_t YtbGetUrlByVideoId(const char *pcVideoId,char *pcUrlOut)
{
	status_t ret=GXCORE_ERROR;//GXCORE_SUCCESS
	char sWatchUrl[128]={0};
	string sWatchContent,sSrcTmp;
	vector<UrlMemMap> vUrlListMap;
	int fd,len;
	char *pcBufMap=NULL;
	if(NULL==pcVideoId)return ret;//s.ytimg.com\/yts\/jsbin\/html5player-vflG49soT.js

	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	snprintf(sWatchUrl,sizeof(sWatchUrl),YTB_WATCH_URL_FORMATS,pcVideoId);
	if(CURLE_OK!=CurlHttpsDownLoad(sWatchUrl, (char *)(YTB_WATCH_FILE_PATH),CurlWriteToFile))
	{
		COMM_LOG("youtube",LOG_ERROR,"curl get %s error",sWatchUrl);
		return ret;
	}

	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	YtbDecrypt::GetInstance();//解密前置
	printf("jiexi0--\n");

	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	if(0>=(fd = open(YTB_WATCH_FILE_PATH,O_RDONLY)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","open file is error");
		return ret;
	}
	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	len = lseek(fd,0,SEEK_END);
	if((char*)-1 == (pcBufMap = (char *) mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","mmap is error");
		return ret;
	}
	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	sWatchContent=pcBufMap;
	munmap((void *)pcBufMap,len);
	close(fd);
	COMM_LOG("youtube",LOG_DEBUG,"%s","");  

#if 0
	ifstream filein(YTB_WATCH_FILE_PATH);
	while(!filein.eof())
	{
		getline(filein, sSrcTmp);
		sWatchContent.append(sSrcTmp);
	} 
	filein.close();
#endif
	COMM_LOG("youtube",LOG_DEBUG,"sWatchContent.length=%d",sWatchContent.length());  
	if(sWatchContent.empty())
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","sWatchContent is error");
		return ret;        
	}

	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	if(GXCORE_SUCCESS!=YtbGetVideoUrlList(pcVideoId,sWatchContent,vUrlListMap))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","YtbGetVideoUrlList is error");
		//printf("---gcyin:%s---[%d]--\n",__FILE__,__LINE__);
		return ret;
	} 
	int nFormatFlag=0;
	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	for(unsigned int i=0;i<vUrlListMap.size();i++)
	{
		if(vUrlListMap[i]["type"].find("mp4;")<vUrlListMap[i]["type"].length())
		{
			sprintf(pcUrlOut,"%s",vUrlListMap[i]["url"].c_str());
			nFormatFlag=1;
			break;
		}
	}
	if(nFormatFlag!=1)sprintf(pcUrlOut,"%s",vUrlListMap[0]["url"].c_str());

	COMM_LOG("youtube",LOG_DEBUG,"%s","");  
	//vUrlListMap[i]["url"]
	printf("jiexi11111\n");
	ret=GXCORE_SUCCESS;
	return ret;
}
status_t YtbDecrypt::GetDecryptFileContent(string &sServerFilePath,string &sContent,const string &sFileSavePath)
{
	status_t ret=GXCORE_ERROR;//GXCORE_SUCCESS

	string sSrcTmp;
	ifstream filein;
	int fd,len;
	char *pcBufMap=NULL;
	if(sServerFilePath.empty())return ret;
	if(sServerFilePath.find("//")==0)sServerFilePath="https:"+sServerFilePath;

	if(CURLE_OK!=CurlHttpsDownLoad((char *)(sServerFilePath.c_str()), (char *)(sFileSavePath.c_str()),CurlWriteToFile))
	{
		COMM_LOG("youtube",LOG_ERROR,"curl get %s error",sServerFilePath.c_str());
		goto GETDECRYPTFILECONTENT;
	}
	sContent="";


	if(0>=(fd = open(sFileSavePath.c_str(),O_RDONLY)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","open file is error");
		goto GETDECRYPTFILECONTENT;
	}
	len = lseek(fd,0,SEEK_END);
	if(NULL==(pcBufMap= (char *) mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","mmap is error");
		goto GETDECRYPTFILECONTENT;
	}
	sContent=pcBufMap;
	munmap((void *)pcBufMap,len);
	close(fd);

#if 0
	filein.open(sFileSavePath.c_str());
	while(!filein.eof())
	{
		getline(filein, sSrcTmp);
		sContent.append(sSrcTmp);
	} 
	filein.close();
#endif
	COMM_LOG("youtube",LOG_DEBUG,"sContent.length=%d",sContent.length());

	if(sContent.empty())
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","sContent is false");
		goto GETDECRYPTFILECONTENT;
	}    

	ret=GXCORE_SUCCESS;
GETDECRYPTFILECONTENT:

	return ret;  
}
//status_t YtbDecrypt::GetWatchFileContent(const string &sDecryptVideoId,string &sWatchContent)
//{
//    status_t ret=GXCORE_ERROR;//GXCORE_SUCCESS
//
//    char sWatchUrl[128]={0};
//    string sSrcTmp; 
//    CurlCode eCurl=CURL_LAST;  
//
//    snprintf(sWatchUrl,sizeof(sWatchUrl),YTB_WATCH_URL_FORMATS,sDecryptVideoId.c_str());
//    if(CURLE_OK!=(eCurl=CurlHttpsDownLoad(sWatchUrl, (char *)(YTB_WATCH_INNER_FILE_PATH),CurlWriteToFile)))
//    {
//        COMM_LOG("youtube",LOG_ERROR,"curl get %s error,err code=%d",sWatchUrl,eCurl);
//        return ret;                
//    }
//
//    ifstream filein(YTB_WATCH_FILE_PATH);
//    while(!filein.eof())
//    {
//        getline(filein, sSrcTmp);
//        sWatchContent.append(sSrcTmp);
//    } 
//    filein.close();
//    StringReplace(sWatchContent,"\n","");  
//    COMM_LOG("youtube",LOG_DEBUG,"sWatchContent.length=%d",sWatchContent.length());  
//    if(sWatchContent.empty())
//    {
//        COMM_LOG("youtube",LOG_ERROR,"%s","sWatchContent is error");
//        return ret;        
//    }
//GETWATCHFILECONTENT:
//    ret=GXCORE_SUCCESS;
//    return ret;     
//}

status_t YtbDecrypt::GetWatchFileContent(const string &sDecryptVideoId,string &sWatchContent,const string &sFileSavePath)
{
	status_t ret=GXCORE_ERROR;//GXCORE_SUCCESS

	char sWatchUrl[128]={0};
	string sSrcTmp; 
	CurlCode eCurl=CURL_LAST;
	ifstream filein;  
	int fd,len;
	char *pcBufMap=NULL;
	if(sDecryptVideoId.empty())return ret;
	snprintf(sWatchUrl,sizeof(sWatchUrl),YTB_WATCH_URL_FORMATS,sDecryptVideoId.c_str());
	if(CURLE_OK!=(eCurl=CurlHttpsDownLoad(sWatchUrl, (char *)(sFileSavePath.c_str()),CurlWriteToFile)))
	{
		COMM_LOG("youtube",LOG_ERROR,"curl get %s error,err code=%d",sWatchUrl,eCurl);
		goto GETWATCHFILECONTENT;                
	}

	if(0>=(fd = open(sFileSavePath.c_str(),O_RDONLY)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","open file is error");
		goto GETWATCHFILECONTENT;                
	}
	len = lseek(fd,0,SEEK_END);
	pcBufMap= (char *) mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0);
	if(pcBufMap == NULL || (int)pcBufMap == -1)
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","mmap is error");
		goto GETWATCHFILECONTENT;                
	}
	sWatchContent=pcBufMap;
	munmap((void *)pcBufMap,len);
	close(fd);

#if 0
	filein.open(sFileSavePath.c_str());
	while(!filein.eof())
	{
		getline(filein, sSrcTmp);
		sWatchContent.append(sSrcTmp);
	} 
	filein.close();
#endif
	//StringReplace(sWatchContent,"\n","");  
	COMM_LOG("youtube",LOG_DEBUG,"sWatchContent.length=%d,sFileSavePath=%s,sWatchUrl=%s",sWatchContent.length(),sFileSavePath.c_str(),sWatchUrl);  
	if(sWatchContent.empty())
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","sWatchContent is error");
		goto GETWATCHFILECONTENT;        
	}
	ret=GXCORE_SUCCESS;
GETWATCHFILECONTENT:

	return ret;     
}

status_t YtbDecrypt::GetWatchFileContent(string &sWatchContent)
{//注：目前的处理方式,无法保证当其加密方式改变时能立即就能处理,有一定时间间隔
	//若处理的话,会有大的改动(不只是加密这一块)
	//妥协的做法是将定时器在不影响其他性能的话适当缩短
#define DECRYPT_VIDEOID_LEN_MAX (20)  //假定video id最大值
	status_t ret=GXCORE_ERROR;//GXCORE_SUCCESS
	string::size_type nStart,nEnd;
	string sPattern= "http://www.youtube.com/watch?v=";  
	string sDecryptVideoId,sXmlContent;
	vector<string> vVideoId;
	char sWatchUrl[128]={0};
	string sSrcTmp;
	CurlCode eCurl=CURL_LAST;
	ifstream decryptin;
	ifstream filein;
	int fd,len;
	char *pcBufMap=NULL;

	//if(sDecryptVideoId.empty())return ret;

	eCurl=CurlHttpsDownLoad((char*)(YTB_DECRYPT_VIDEOID_HTTP_PATH), (char*)(YTB_DECRYPT_VIDEOID_SAVE_PATH),CurlWriteToFile);
	if(CURLE_OK!=eCurl)
	{
		COMM_LOG("youtube",LOG_ERROR,"curl get %s error,err code=%d",YTB_DECRYPT_VIDEOID_HTTP_PATH,eCurl);
		return ret;  
	}  
	decryptin.open(YTB_DECRYPT_VIDEOID_SAVE_PATH);
	while(!decryptin.eof())
	{
		getline(decryptin, sSrcTmp);
		sXmlContent.append(sSrcTmp);
	} 
	decryptin.close();  
	ClearVector(vVideoId);
	sWatchContent="";
	//StringReplace(sXmlContent,"\n","");  
	nStart=sXmlContent.find(sPattern);
	if(nStart<sXmlContent.length())
	{
		nStart=nStart+sPattern.length();
		nEnd=sXmlContent.find("&amp",nStart);
		if(nEnd<sXmlContent.length())
		{
			sDecryptVideoId=sXmlContent.substr(nStart,nEnd-nStart);
			if(sDecryptVideoId.length()>DECRYPT_VIDEOID_LEN_MAX)
			{
				COMM_LOG("youtube",LOG_ERROR,"nStart=%d,nEnd=%d,%s",nStart,nEnd,"sDecryptVideoId.length is too long");
				sDecryptVideoId="";
				goto GETWATCHFILECONTENT;                
			}
		}else
		{
			COMM_LOG("youtube",LOG_ERROR,"nStart=%d,nEnd=%d,%s",nStart,nEnd,"not find proper sDecryptVideoId");
			goto GETWATCHFILECONTENT;
		}           
	}else
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","not find sDecryptVideoId");
		goto GETWATCHFILECONTENT;
	}             

	vVideoId.push_back(sDecryptVideoId);
	if(vVideoId.empty())return ret;//s.ytimg.com\/yts\/jsbin\/html5player-vflG49soT.js

	for(unsigned int i=0;i<vVideoId.size();i++)
	{//此处简单做处理,仅获取一个有效的videoid
		snprintf(sWatchUrl,sizeof(sWatchUrl),YTB_WATCH_URL_FORMATS,vVideoId[i].c_str());
		eCurl=CurlHttpsDownLoad(sWatchUrl, (char *)(YTB_WATCH_INNER_FILE_PATH),CurlWriteToFile);
		if(CURLE_OK==eCurl)
		{
			break;
		}else
		{
			COMM_LOG("youtube",LOG_ERROR,"curl get %s error,err code=%d",sWatchUrl,eCurl);
			return ret;         
		}        
	}

	if(0>=(fd = open(YTB_WATCH_FILE_PATH,O_RDONLY)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","open file is error");
		return ret;
	}
	len = lseek(fd,0,SEEK_END);
	if((char *)-1 == (pcBufMap= (char *) mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","mmap is error");
		return ret;
	}
	sWatchContent=pcBufMap;
	munmap((void *)pcBufMap,len);
	close(fd);
	printf("jiexi0--\n");
#if 0
	filein.open(YTB_WATCH_FILE_PATH);
	while(!filein.eof())
	{
		getline(filein, sSrcTmp);
		sWatchContent.append(sSrcTmp);
	} 
	filein.close();
#endif
	//StringReplace(sWatchContent,"\n","");  
	COMM_LOG("youtube",LOG_DEBUG,"sWatchContent.length=%d",sWatchContent.length());  
	if(sWatchContent.empty())
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","sWatchContent is error");
		return ret;        
	}
GETWATCHFILECONTENT:
	ret=GXCORE_SUCCESS;
	return ret; 
}

status_t YtbDecrypt::GetPlayerConfigQuick(const string &sWatchContent,string &sJsonPlayerConfig)
{
#define PLAYER_CONFIG_LEN_MAX (40960)  //假定player config json 数据最大值
	status_t ret=GXCORE_ERROR;

	string::size_type nStart,nEnd;
	int nFlag=0;
	string sPattern= ";ytplayer.config = ";   
	if(sWatchContent.empty())return ret;
	nStart=sWatchContent.find(sPattern);
	if(nStart<sWatchContent.length())
	{
		nStart=nStart+sPattern.length();
		nEnd=sWatchContent.find("};",nStart);
		if(nEnd>=sWatchContent.length())
		{
			nFlag=1;
			nEnd=sWatchContent.find(";</",nStart);
		}
		if(nEnd<sWatchContent.length())
		{
			sJsonPlayerConfig=sWatchContent.substr(nStart,nEnd-nStart);
			if(nFlag==0)sJsonPlayerConfig.append("}");
			if(sJsonPlayerConfig.length()>PLAYER_CONFIG_LEN_MAX)
			{
				COMM_LOG("youtube",LOG_ERROR,"nStart=%d,nEnd=%d,%s",nStart,nEnd,"sJsonPlayerConfig.length is too long");
				sJsonPlayerConfig="";
				goto GETPLAYERCONFIGQUICK;                
			}
		}else
		{
			COMM_LOG("youtube",LOG_ERROR,"nStart=%d,nEnd=%d,%s",nStart,nEnd,"not find proper sJsonPlayerConfig");
			goto GETPLAYERCONFIGQUICK;
		}           
	}else
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","not find ;ytplayer.config");
		goto GETPLAYERCONFIGQUICK;
	}      

	COMM_LOG("youtube",LOG_DEBUG,"%s","find sJsonPlayerConfig ...");
	ret=GXCORE_SUCCESS;

GETPLAYERCONFIGQUICK:	
	return ret;    

}



status_t YtbDecrypt::GetEncryptSigQuick(const string &sJsonPlayerConfig,string &sEncryptSig)
{
#define ENCRYPT_SIG_LEN_MAX (128)  //假定s最大值
	status_t ret=GXCORE_ERROR;//GXCORE_SUCCESS  
	string::size_type nStart,nEnd;
	string sPattern= "&s=";  
	string sEncodedUrl;
	json_t *pRoot=NULL,*pArgs=NULL,*pEncodedUrl=NULL;

	sEncryptSig="";
	if(sJsonPlayerConfig.empty())return ret;

	if(NULL==(pRoot=json_loads((char *)(sJsonPlayerConfig.c_str()),0,NULL)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","json file load failed");
		goto GETENCRYPTSIGQUICK;
	}
	pArgs= json_object_get(pRoot, "args");
	if(!json_is_object(pArgs))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","not an obj");
		goto GETENCRYPTSIGQUICK;
	}
	pEncodedUrl = json_object_get(pArgs, "url_encoded_fmt_stream_map");//url_encoded_fmt_stream_map
	if(!json_is_string(pEncodedUrl))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","not an string");
		goto GETENCRYPTSIGQUICK;
	}
	sEncodedUrl=json_string_value(pEncodedUrl);

	if(sEncodedUrl.empty())return ret;
	if(0==(nStart=sEncodedUrl.find("s=")))
	{//sEncodedUrl is start with "s="
		sPattern="s=";
	}else
	{
		sPattern="&s=";
		nStart=sEncodedUrl.find(sPattern);
	}
	if(nStart<sEncodedUrl.length())
	{
		nStart=nStart+sPattern.length();
		string::size_type nEnd0,nEnd1;
		nEnd0=sEncodedUrl.find("&",nStart);//may be '&' or ',',otherwise
		nEnd1=sEncodedUrl.find(",",nStart);//may be '&' or ',',otherwise
		nEnd=(nEnd0<nEnd1)?nEnd0:nEnd1;//we want the smaller value
		if(nEnd<sEncodedUrl.length())
		{
			sEncryptSig=sEncodedUrl.substr(nStart,nEnd-nStart);
			if(sEncryptSig.length()>ENCRYPT_SIG_LEN_MAX)
			{
				COMM_LOG("youtube",LOG_ERROR,"nStart=%d,nEnd=%d,%s",nStart,nEnd,"sEncryptSig.length is too long");
				COMM_LOG("youtube",LOG_ERROR,"sEncryptSig=%s",sEncryptSig.c_str());
				sEncryptSig="";
				goto GETENCRYPTSIGQUICK;                
			}
		}else
		{
			COMM_LOG("youtube",LOG_ERROR,"nStart=%d,nEnd=%d,%s",nStart,nEnd,"not find proper sEncryptSig");
			goto GETENCRYPTSIGQUICK;
		}           
	}else
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","not find sEncryptSig");
		COMM_LOG("youtube",LOG_ERROR,"sEncodedUrl=%s",sEncodedUrl.c_str());
		goto GETENCRYPTSIGQUICK;
	}       


	ret=GXCORE_SUCCESS;

GETENCRYPTSIGQUICK:
	json_decref(pRoot);

	return ret;
}

status_t YtbDecrypt::GetDecryptFileServerPath(const string &sJsonPlayerConfig,string &sServerFilePath)
{
	status_t ret=GXCORE_ERROR;//GXCORE_SUCCESS    
	json_t *pRoot=NULL,*pAssets=NULL,*pJsPath=NULL;

	sServerFilePath="";
	if(sJsonPlayerConfig.empty())return ret;

	if(NULL==(pRoot=json_loads((char *)(sJsonPlayerConfig.c_str()),0,NULL)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","json file load failed");
		goto GETDECRYPTFILESERVERPATH;
	}
	pAssets= json_object_get(pRoot, "assets");
	if(!json_is_object(pAssets))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","pAssets not an obj");
		goto GETDECRYPTFILESERVERPATH;
	}
	pJsPath = json_object_get(pAssets, "js");//url_encoded_fmt_stream_map
	if(!json_is_string(pJsPath))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","pJsPath not an string");
		goto GETDECRYPTFILESERVERPATH;
	}
	sServerFilePath=json_string_value(pJsPath);	
	ret=GXCORE_SUCCESS;

GETDECRYPTFILESERVERPATH:
	json_decref(pRoot);

	return ret;    
}

//status_t YtbDecrypt::GenDecryptIndexVector()
//{
//    string sSrcTmp;
//    //char *pcFileContent=NULL;
//    if(sServerFilePath.empty())return;
//    if(sServerFilePath.find("//")==0)sServerFilePath="https:"+sServerFilePath;
//
//    if(CURLE_OK!=CurlHttpsDownLoad((char *)(sServerFilePath.c_str()), (char *)(YTB_DECRYPTED_JS_FILE_PATH),CurlWriteToFile))
//    {
//        COMM_LOG("youtube",LOG_ERROR,"curl get %s error",sServerFilePath.c_str());
//        return;
//    }
//    ifstream filein(YTB_DECRYPTED_JS_FILE_PATH);
//    while(!filein.eof())
//    {
//        getline(filein, sSrcTmp);
//        sJsContent.append(sSrcTmp);
//    } 
//
//    filein.close();
//    COMM_LOG("youtube",LOG_DEBUG,"sJsContent.length=%d",sJsContent.length());
//
//    if(sJsContent.length()>0)
//    {
//        if(GXCORE_SUCCESS!=GetEntryFunNameQuick())
//        {
//            COMM_LOG("youtube",LOG_ERROR,"%s","GetEntryFunNameQuick is false");
//        }
//    }    
//    for(unsigned int i=0;i<sEncryptSig.length();i++)
//    {
//        vDecryptIndex.push_back(i);
//    }	
//    vector<string>  vArgs,vCode;
//    //if(GXCORE_SUCCESS!=ExtractFunction(sEntryFunName,vArgs,vCode))
//    if(GXCORE_SUCCESS!=ExtractFunctionQuick(sEntryFunName,vArgs,vCode))
//    {
//        COMM_LOG("youtube",LOG_ERROR,"%s","ExtractFunction is error");
//        return ret;            
//    }
//    if(GXCORE_SUCCESS!=Resf(vArgs,vCode))
//    {
//        COMM_LOG("youtube",LOG_ERROR,"%s","Resf is error");
//        return ret;            
//    }    
//
//}

status_t YtbDecrypt::SaveDecryptIndexVector()
{
	status_t ret=GXCORE_ERROR;
	string sDecrypt;
	static string lsDecrypt;
	ofstream fileout;
	unsigned int unCount=0;
	pthread_mutex_lock(&m_MutexDecryptFile);

	if(m_mDecryptIndex.empty())
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","m_mDecryptIndex is empty");
		goto SAVEDECRYPTINDEXVECTOR;
	}    


	COMM_LOG("youtube",LOG_DEBUG,"m_mDecryptIndex.size()=%u",m_mDecryptIndex.size());                                             
	foreach(m_mDecryptIndex,it)
	{
		COMM_LOG("youtube",LOG_DEBUG,"it->first=%u,it->second.size()=%u",it->first,it->second.size());
		unCount++;
		if(it->first >0 && it->second.size()>0 && it->first<=FUZZY_SIG_LEN_MAX && it->second.size()<FUZZY_SIG_LEN_MAX)
		{
			sDecrypt=sDecrypt+ToString(it->first)+"=";
			for(unsigned i=0;i<it->second.size();i++)
			{
				if(i==it->second.size()-1)
					sDecrypt=sDecrypt+ToString(it->second[i]);
				else
					sDecrypt=sDecrypt+ToString(it->second[i])+",";
			} 
			if(unCount!=m_mDecryptIndex.size())
				sDecrypt=sDecrypt+"\n";          
		}else
		{
			//m_mDecryptIndex.erase(it);
			COMM_LOG("youtube",LOG_DEBUG,"%s","m_mDecryptIndex remove element");
		}
	}
	COMM_LOG("youtube",LOG_DEBUG,"%s","");                                             
	if(lsDecrypt==sDecrypt)                                                                                   
	{//this means we don't need to update the file                                                         
		COMM_LOG("youtube",LOG_DEBUG,"%s","lsDecrypt==sDecrypt");                                             
	}else                                                                                                     
	{                                                                                                         
		fileout.open(DECRYPT_INDEX_SAVE_FILE_PATH);
		if(!fileout)
		{
			COMM_LOG("youtube",LOG_ERROR,"%s","fileout is null");
			goto SAVEDECRYPTINDEXVECTOR;        
		}
		lsDecrypt=sDecrypt;                                                                                   
		fileout << sDecrypt <<endl;
		fileout.close();
	} 

	ret=GXCORE_SUCCESS;
SAVEDECRYPTINDEXVECTOR:
	pthread_mutex_unlock(&m_MutexDecryptFile);
	return ret;
}
status_t YtbDecrypt::LoadDecryptIndexVector()
{
	//文件内容格式：length=2,3,5,1...（包含多行）
	status_t ret=GXCORE_ERROR;
	string sDecrypt;
	const char *pTmp0=NULL,*pTmp1=NULL,*pTmp=NULL;
	pthread_mutex_lock(&m_MutexDecryptFile);
	ifstream filein(DECRYPT_INDEX_SAVE_FILE_PATH);
	if(!filein)
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","filein is null");
		goto LOADDECRYPTINDEXVECTOR;        
	}
	while(!filein.eof())
	{
		getline(filein, sDecrypt);
		StringReplace(sDecrypt,"\n","");
		if(sDecrypt.empty())break;
		COMM_LOG("youtube",LOG_DEBUG,"sDecrypt=%s",sDecrypt.c_str());                                             
		//unsigned int unKey=0;
		vector<unsigned int> vDecryptIndex;
		if(sDecrypt.find("=")<sDecrypt.length() && sDecrypt.find(",")<sDecrypt.length())
		{
			pTmp0= strtok ((char *)sDecrypt.c_str(),"="); //分割字符串
			pTmp1 = strtok(NULL,"="); //指向下一个指针

			pTmp= strtok ((char *)pTmp1,","); //分割字符串
			while(pTmp)
			{
				vDecryptIndex.push_back(FromString<unsigned int>(pTmp));//在容器尾部加入一个数据
				pTmp = strtok(NULL,","); //指向下一个指针
			} 
			if(FromString<unsigned int>(pTmp0) >0 && vDecryptIndex.size()>0)
				m_mDecryptIndex[FromString<unsigned int>(pTmp0)]=vDecryptIndex;      
		}else
		{
			COMM_LOG("youtube",LOG_ERROR,"%s","load decrypt index is error");
			goto LOADDECRYPTINDEXVECTOR;          
		}        
	}
	filein.close();  

	ret=GXCORE_SUCCESS;

LOADDECRYPTINDEXVECTOR:

	m_vVideoIdTimerTmp=m_vVideoIdTimer;
	pthread_mutex_unlock(&m_MutexDecryptFile);
	return ret;
}
status_t YtbDecrypt::SaveVideoIdVector()
{
	COMM_LOG("youtube",LOG_DEBUG,"%s","");                                             
	status_t ret=GXCORE_ERROR;
	GXxmlDocPtr doc=NULL;
	GXxmlNodePtr group=NULL,xndTmp1=NULL,xndTmp2=NULL,xndTmp3=NULL,root=NULL;
	string sVideoIdContents;
	static string lsVideoIdContents;
	unsigned int unCount=0;

	if(m_vVideoIdTimer.empty())return ret;
	foreach(m_vVideoIdTimer,it)
	{
		unCount++;
		if(unCount==m_vVideoIdTimer.size())
			sVideoIdContents=sVideoIdContents+(*it);
		else
			sVideoIdContents=sVideoIdContents+(*it)+",";
	}
	if(sVideoIdContents==lsVideoIdContents)
	{
		COMM_LOG("youtube",LOG_DEBUG,"%s","sVideoIdContents==lsVideoIdContents");                                             
		return GXCORE_SUCCESS;
	}
	COMM_LOG("youtube",LOG_DEBUG,"%s","");                                             
	if(NULL==(doc = GXxmlParseFile(XML_CONFIG_PATH)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","GXxml parse error");
		goto SAVEVIDEOIDVECTOR;
	}
	if(NULL==(root = doc->root))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","root is null");
		goto SAVEVIDEOIDVECTOR;
	}    
	if (0 !=GXxmlStrcmp(root->name, (GXxmlChar *)(XML_GLOBAL_CONFIG)))                                                               
	{                                                                                                         
		COMM_LOG("youtube",LOG_ERROR,"%s","XML_GLOBAL_CONFIG is error");
		goto SAVEVIDEOIDVECTOR;                                                                                    
	}                                                                                                         
	// group                                                                                                  
	group = root->childs;                                                                                                                                         
	while (NULL != group)                                                                                     
	{  
		if(0==GXxmlStrcmp(group->name,(GXxmlChar *)XML_INTERNET))
		{
			xndTmp1=group->childs;
			while(NULL!=xndTmp1)
			{
				if(0==GXxmlStrcmp(xndTmp1->name,(GXxmlChar *)XML_YOUTUBE))
				{
					xndTmp2=xndTmp1->childs;
					while(NULL!=xndTmp2)
					{
						if(0==GXxmlStrcmp(xndTmp2->name,(GXxmlChar *)XML_YOUTUBEDECRYPT))
						{
							xndTmp3=xndTmp2->childs;
							while(NULL!=xndTmp3)
							{
								if(0==GXxmlStrcmp(xndTmp3->name,(GXxmlChar *)XML_DECRYPT_VIDEOID))
								{
									GXxmlNodeSetContent(xndTmp3,(GXxmlChar *)(sVideoIdContents.c_str()));
									break;
								}
								xndTmp3=xndTmp3->next;
							}
							break;
						}
						xndTmp2=xndTmp2->next;
					}
					break;
				}
				xndTmp1=xndTmp1->next;
			}
			break;
		}else
		{
			group = group->next;   
		}		                                        
	}

	ret=GXCORE_SUCCESS;
SAVEVIDEOIDVECTOR:
	if(doc)
	{
		GXxmlSaveFile(XML_CONFIG_PATH,doc);
		GXxmlFreeDoc(doc);
		doc=NULL;
	}
	return ret;
}
status_t YtbDecrypt::LoadVideoIdVector()
{
	status_t ret=GXCORE_ERROR;
	GXxmlDocPtr doc=NULL;
	GXxmlChar *pVideoIdValue=NULL,*pIntervalValue=NULL;
	GXxmlNodePtr group=NULL,xndTmp1=NULL,xndTmp2=NULL,xndTmp3=NULL,root=NULL;
	string sVideoId,sInterval;
	const char *pTmp=NULL;

	if(NULL==(doc = GXxmlParseFile(XML_CONFIG_PATH)))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","GXxml parse error");
		goto LOADVIDEOIDVECTOR;
	}
	if(NULL==(root = doc->root))
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","root is null");
		goto LOADVIDEOIDVECTOR;
	}    
	if (0 !=GXxmlStrcmp(root->name, (GXxmlChar *)(XML_GLOBAL_CONFIG)))                                                               
	{                                                                                                         
		COMM_LOG("youtube",LOG_ERROR,"%s","XML_GLOBAL_CONFIG is error");
		goto LOADVIDEOIDVECTOR;                                                                                    
	}                                                                                                         
	// group                                                                                                  
	group = root->childs;                                                                                                                                         
	while (NULL != group)                                                                                     
	{  
		if(0==GXxmlStrcmp(group->name,(GXxmlChar *)XML_INTERNET))
		{
			xndTmp1=group->childs;
			while(NULL!=xndTmp1)
			{
				if(0==GXxmlStrcmp(xndTmp1->name,(GXxmlChar *)XML_YOUTUBE))
				{
					xndTmp2=xndTmp1->childs;
					while(NULL!=xndTmp2)
					{
						if(0==GXxmlStrcmp(xndTmp2->name,(GXxmlChar *)XML_YOUTUBEDECRYPT))
						{
							xndTmp3=xndTmp2->childs;
							while(NULL!=xndTmp3)
							{
								if(0==GXxmlStrcmp(xndTmp3->name,(GXxmlChar *)XML_DECRYPT_VIDEOID))
								{
									if(NULL==(pVideoIdValue=GXxmlNodeGetContent(xndTmp3)))
									{
										COMM_LOG("youtube",LOG_ERROR,"%s","pVideoIdValue is error");
										goto LOADVIDEOIDVECTOR;                                              
									}
								}
								if(0==GXxmlStrcmp(xndTmp3->name,(GXxmlChar *)XML_UPDATE_TIME_INTERVAL))
								{
									if(NULL==(pIntervalValue=GXxmlNodeGetContent(xndTmp3)))
									{
										COMM_LOG("youtube",LOG_ERROR,"%s","pIntervalValue is error");
										goto LOADVIDEOIDVECTOR;                                              
									}
								}   
								if(NULL!=pVideoIdValue && NULL!=pIntervalValue)break;
								xndTmp3=xndTmp3->next;
							}
							break;
						}
						xndTmp2=xndTmp2->next;
					}
					break;
				}
				xndTmp1=xndTmp1->next;
			}
			break;
		}else
		{
			group = group->next;   
		}		                                        
	}
	sVideoId=(char *)pVideoIdValue;
	sInterval=(char *)pIntervalValue;
	if(sVideoId.empty() || sInterval.empty())
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","sVideoId or sInterval is error");
		goto LOADVIDEOIDVECTOR;          
	}
	ClearVector(m_vVideoIdTimerTmp);
	StringReplace(sVideoId,"\n","");
	pTmp= strtok ((char *)sVideoId.c_str(),","); //分割字符串
	while(pTmp)
	{
		m_vVideoIdTimerTmp.push_back(pTmp);//在容器尾部加入一个数据
		if(m_vVideoIdTimerTmp.size()==VIDEO_ID_TIMER_VECTOR_SIZE_MAX)break;
		pTmp = strtok(NULL,","); //指向下一个指针
	}   
	unTimeInterval= FromString<unsigned int>(sInterval);
	if(unTimeInterval<=0 || m_vVideoIdTimerTmp.empty())
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","unTimeInterval or vVideoId is error");
		goto LOADVIDEOIDVECTOR;         
	}

	ret=GXCORE_SUCCESS;
LOADVIDEOIDVECTOR:
	if(doc)
	{
		GXxmlFreeDoc(doc);
		doc=NULL;
	}
	GXCORE_FREE(pVideoIdValue);
	GXCORE_FREE(pIntervalValue);

	m_vVideoIdTimer=m_vVideoIdTimerTmp;
	return ret;
}
void *GetDecryptIndexThread(void *pArg)
{
	struct timeval timeout;
	YtbDecrypt *pYtbDecrypt=(YtbDecrypt *)pArg;
	if(NULL==pYtbDecrypt)return NULL;

	pthread_detach(pthread_self());

	while(1)
	{
		//timeout.tv_sec = pYtbDecrypt->unTimeInterval;
		//pYtbDecrypt->unTimeInterval=120;
		if(pYtbDecrypt->unTimeInterval<=0)
		{
			pYtbDecrypt->unTimeInterval=30*60;
		}
		timeout.tv_sec =pYtbDecrypt->unTimeInterval;
		timeout.tv_usec = 0;  

		pthread_mutex_lock(&pYtbDecrypt->m_MutexVideoIdTimer);  
		pYtbDecrypt->m_vVideoIdTimer=pYtbDecrypt->m_vVideoIdTimerTmp;
		pthread_mutex_unlock(&pYtbDecrypt->m_MutexVideoIdTimer);

		foreach(pYtbDecrypt->m_vVideoIdTimer,it)
		{
			COMM_LOG("youtube",LOG_DEBUG,"%s--m_vVideoIdTimer.size()=%u--","GetDecryptIndexThread is run",pYtbDecrypt->m_vVideoIdTimer.size());
			status_t ret=GXCORE_ERROR;
			string sWatchContent,sEntryFunName,sServerFilePath,sEncryptSig,sJsonPlayerConfig,sJsContent, sFunVarName;
			vector<string>  vArgs,vCode, vSubFunName, vSubFun;
			vector<unsigned int> vDecryptIndex;            
			if(GXCORE_SUCCESS!=pYtbDecrypt->GetWatchFileContent((*it),sWatchContent))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","GetWatchFileContent is error");
				goto GETDYDECRYPTINDEXTHREAD;            
			}            
			if(GXCORE_SUCCESS!=pYtbDecrypt->GetPlayerConfigQuick(sWatchContent,sJsonPlayerConfig))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","GetPlayerConfigQuick is error");
				goto GETDYDECRYPTINDEXTHREAD;            
			}  
			if(GXCORE_SUCCESS!=pYtbDecrypt->GetEncryptSigQuick(sJsonPlayerConfig,sEncryptSig))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","GetEncryptSigQuick is error");
				goto GETDYDECRYPTINDEXTHREAD;            
			}              
			if(GXCORE_SUCCESS!=pYtbDecrypt->GetDecryptFileServerPath(sJsonPlayerConfig,sServerFilePath))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","GetPlayerConfigQuick is error");
				goto GETDYDECRYPTINDEXTHREAD;            
			} 
			if(GXCORE_SUCCESS!=pYtbDecrypt->GetDecryptFileContent(sServerFilePath,sJsContent))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","GetDecryptFileContent is error");
				goto GETDYDECRYPTINDEXTHREAD;            
			}  
			if(GXCORE_SUCCESS!=pYtbDecrypt->GetEntryFunNameQuick(sJsContent,sEntryFunName))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","GetDecryptFileContent is error");
				goto GETDYDECRYPTINDEXTHREAD;            
			} 
			if(GXCORE_SUCCESS != pYtbDecrypt->GetFunVarNameQuick(sJsContent, sEntryFunName, sFunVarName))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s"," GetFunVarNameQuick is error");
				goto GETDYDECRYPTINDEXTHREAD;            
			} 
			if(GXCORE_SUCCESS != pYtbDecrypt->GetSubFunNameQuick(sJsContent, sFunVarName, vSubFunName, vSubFun))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s"," GetSubFunNameQuick is error");
				goto GETDYDECRYPTINDEXTHREAD;            
			} 
			for(unsigned int i=0;i<sEncryptSig.length();i++)
			{
				vDecryptIndex.push_back(i);
			}	            
			if(GXCORE_SUCCESS!=pYtbDecrypt->ExtractFunctionQuick(sJsContent,sEntryFunName,vArgs,vCode))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","ExtractFunction is error");
				goto GETDYDECRYPTINDEXTHREAD;           
			}
			if(GXCORE_SUCCESS!=pYtbDecrypt->Resf(vArgs,vCode,vDecryptIndex, vSubFunName, vSubFun, sFunVarName))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","Resf is error");
				goto GETDYDECRYPTINDEXTHREAD;             
			}  
			ret=GXCORE_SUCCESS;
GETDYDECRYPTINDEXTHREAD:
			if(GXCORE_SUCCESS==ret)
			{
				pthread_mutex_lock(&pYtbDecrypt->m_MutexDecryptIndexMap);  
				pYtbDecrypt->m_mDecryptIndex[sEncryptSig.length()]=vDecryptIndex;
				COMM_LOG("youtube",LOG_DEBUG,"%s","");
				if(GXCORE_SUCCESS!=pYtbDecrypt->SaveDecryptIndexVector())
				{
					COMM_LOG("youtube",LOG_ERROR,"%s","SaveDecryptIndexVector is error"); 
				}
				if(GXCORE_SUCCESS!=pYtbDecrypt->SaveVideoIdVector())
				{
					COMM_LOG("youtube",LOG_ERROR,"%s","SaveVideoIdVector is error"); 
				}                    
				pthread_mutex_unlock(&pYtbDecrypt->m_MutexDecryptIndexMap);               
			}            
		} 
		select( 0, NULL, NULL, NULL, &timeout);        
	}
	//pthread_mutex_lock(&pImp->m_MutexPreLoad);
	//pthread_mutex_unlock(&pImp->m_MutexPreLoad);
	pthread_exit(NULL);    
}

void *StudyDecryptIndexThread(void *pArg)
{
	status_t ret=GXCORE_ERROR;

	YtbDecrypt *pYtbDecrypt=(YtbDecrypt *)pArg;
	if(NULL==pYtbDecrypt)return NULL;

	pthread_detach(pthread_self());

	while(1)
	{
		string sWatchContent,sEntryFunName,sServerFilePath,sJsonPlayerConfig,sJsContent, sFunVarName;
		vector<string>  vArgs,vCode, vSubFunName, vSubFun;
		vector<unsigned int> vDecryptIndex;
		bool bHasFound=false;
		ret=GXCORE_ERROR;


		pthread_mutex_lock(&pYtbDecrypt->m_MutexVideoIdStudy);  
		string sVideoIdStudy=pYtbDecrypt->m_sVideoIdStudy;
		unsigned int unVideoIdStudyLen=pYtbDecrypt->m_unVideoIdStudyLen;
		pthread_mutex_unlock(&pYtbDecrypt->m_MutexVideoIdStudy);       

		if(unVideoIdStudyLen<DECRYPT_VIDEO_ID_LEN_MAX && !(sVideoIdStudy.empty()))//如果没有要立即获取的
		{
			sleep(1);//休眠，让出时间片，使其能够尽快播放
			COMM_LOG("youtube",LOG_DEBUG,"%s","StudyDecryptIndexThread is run");
			if(GXCORE_SUCCESS!=pYtbDecrypt->GetWatchFileContent(sVideoIdStudy,sWatchContent,YTB_WATCH_STUDY_FILE_PATH))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","GetWatchFileContent is error");
				goto STUDYDECRYPTINDEXTHREAD;            
			}            
			if(GXCORE_SUCCESS!=pYtbDecrypt->GetPlayerConfigQuick(sWatchContent,sJsonPlayerConfig))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","GetPlayerConfigQuick is error");
				goto STUDYDECRYPTINDEXTHREAD;            
			}  
			if(GXCORE_SUCCESS!=pYtbDecrypt->GetDecryptFileServerPath(sJsonPlayerConfig,sServerFilePath))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","GetPlayerConfigQuick is error");
				goto STUDYDECRYPTINDEXTHREAD;            
			} 
			if(GXCORE_SUCCESS!=pYtbDecrypt->GetDecryptFileContent(sServerFilePath,sJsContent,YTB_DECRYPTED_JS_STUDY_FILE_PATH))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","GetDecryptFileContent is error");
				goto STUDYDECRYPTINDEXTHREAD;            
			}  
			if(GXCORE_SUCCESS!=pYtbDecrypt->GetEntryFunNameQuick(sJsContent,sEntryFunName))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","GetDecryptFileContent is error");
				goto STUDYDECRYPTINDEXTHREAD;            
			} 
			if(GXCORE_SUCCESS != pYtbDecrypt->GetFunVarNameQuick(sJsContent, sEntryFunName, sFunVarName))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s"," GetFunVarNameQuick is error");
				goto STUDYDECRYPTINDEXTHREAD;            
			} 
			if(GXCORE_SUCCESS != pYtbDecrypt->GetSubFunNameQuick(sJsContent, sFunVarName, vSubFunName, vSubFun))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s"," GetSubFunNameQuick is error");
				goto STUDYDECRYPTINDEXTHREAD;            
			} 
			for(unsigned int i=0;i<unVideoIdStudyLen;i++)
			{
				vDecryptIndex.push_back(i);
			}	            
			if(GXCORE_SUCCESS!=pYtbDecrypt->ExtractFunctionQuick(sJsContent,sEntryFunName,vArgs,vCode))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","ExtractFunction is error");
				goto STUDYDECRYPTINDEXTHREAD;           
			}
			if(GXCORE_SUCCESS!=pYtbDecrypt->Resf(vArgs,vCode,vDecryptIndex, vSubFunName, vSubFun, sFunVarName))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","Resf is error");
				goto STUDYDECRYPTINDEXTHREAD;             
			}  
			COMM_LOG("youtube",LOG_DEBUG,"%s","");
			ret=  GXCORE_SUCCESS;             
		} 
STUDYDECRYPTINDEXTHREAD:
		if(GXCORE_SUCCESS==ret)
		{
			pthread_mutex_lock(&pYtbDecrypt->m_MutexDecryptIndexMap);  

			pYtbDecrypt->m_mDecryptIndex[unVideoIdStudyLen]=vDecryptIndex;
			COMM_LOG("youtube",LOG_DEBUG,"unVideoIdStudyLen=%u,vDecryptIndex.size()=%u",unVideoIdStudyLen,vDecryptIndex.size());
			if(GXCORE_SUCCESS!=pYtbDecrypt->SaveDecryptIndexVector())
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","SaveDecryptIndexVector is error"); 
			}
			if(GXCORE_SUCCESS!=pYtbDecrypt->SaveVideoIdVector())
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","SaveVideoIdVector is error"); 
			}            


			pthread_mutex_unlock(&pYtbDecrypt->m_MutexDecryptIndexMap);               
		}
		pthread_mutex_lock(&pYtbDecrypt->m_MutexVideoIdTimer);  
		if(!sVideoIdStudy.empty())
		{
			foreach(pYtbDecrypt->m_vVideoIdTimerTmp,it)
			{
				if(*it==pYtbDecrypt->m_sVideoIdStudy)
				{
					bHasFound=true;
					break;
				}
			}
			if(false==bHasFound)
			{
				if(pYtbDecrypt->m_vVideoIdTimerTmp.size()<VIDEO_ID_TIMER_VECTOR_SIZE_MAX)
				{
					pYtbDecrypt->m_vVideoIdTimerTmp.push_back(sVideoIdStudy);
				}else
				{
					while(pYtbDecrypt->m_vVideoIdTimerTmp.size()>=VIDEO_ID_TIMER_VECTOR_SIZE_MAX)
						pYtbDecrypt->m_vVideoIdTimerTmp.erase(pYtbDecrypt->m_vVideoIdTimerTmp.begin());
					pYtbDecrypt->m_vVideoIdTimerTmp.push_back(sVideoIdStudy);
				}                    
			}
		}

		//pYtbDecrypt->m_vVideoIdTimer=pYtbDecrypt->m_vVideoIdTimerTmp;
		pthread_mutex_unlock(&pYtbDecrypt->m_MutexVideoIdTimer);  


		pthread_mutex_lock(&pYtbDecrypt->m_MutexVideoIdStudy);  
		pYtbDecrypt->m_sVideoIdStudy="";
		pYtbDecrypt->m_unVideoIdStudyLen=0; 
		pthread_mutex_unlock(&pYtbDecrypt->m_MutexVideoIdStudy);   
		//pthread_mutex_unlock(&pYtbDecrypt->m_MutexStudyAndTimer);
		usleep(500*1000);//0.5s
	}
	//pthread_mutex_lock(&pImp->m_MutexPreLoad);
	//pthread_mutex_unlock(&pImp->m_MutexPreLoad);
	pthread_exit(NULL);    
}

YtbDecrypt::YtbDecrypt()
{
	bInitStatus=false;
	//bHasCached=false;

	if(GXCORE_SUCCESS!=LoadDecryptIndexVector())
	{//load即使失败，但不影响全局，流程继续往下走
		COMM_LOG("youtube",LOG_ERROR,"%s","LoadDecryptIndexVector is error");            
	}  
	if(GXCORE_SUCCESS!=LoadVideoIdVector())
	{//load即使失败，但不影响全局，流程继续往下走
		COMM_LOG("youtube",LOG_ERROR,"%s","LoadVideoIdVector is error");            
	}   



	//mutex init
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&m_MutexDecryptFile, &attr);
	//pthread_mutex_init(&m_MutexStudyAndTimer, &attr);
	pthread_mutex_init(&m_MutexDecryptIndexMap, &attr);
	pthread_mutex_init(&m_MutexVideoIdStudy, &attr);

	//m_CondStudyAndTimer=PTHREAD_COND_INITIALIZER;
	//create pthread
	pthread_t thread;
	pthread_attr_t tThreadAttr;
	size_t unStackSize=0;

	pthread_attr_init(&tThreadAttr);
	unStackSize=1024*100;
	pthread_attr_setstacksize(&tThreadAttr,unStackSize);

	COMM_LOG("youtube",LOG_DEBUG,"%s","pthread_create GetDecryptIndexThread");
	if(0!=(pthread_create(&thread,&tThreadAttr,GetDecryptIndexThread,this)))
	{    
		COMM_LOG("youtube",LOG_ERROR,"%s",strerror(errno));
		return;
	} 
	if(0!=(pthread_create(&thread,&tThreadAttr,StudyDecryptIndexThread,this)))
	{    
		COMM_LOG("youtube",LOG_ERROR,"%s",strerror(errno));
		return;
	}     

	bInitStatus=true;    

	pthread_attr_destroy(&tThreadAttr);
}
YtbDecrypt::~YtbDecrypt()
{
	pthread_mutex_destroy(&m_MutexVideoIdStudy);
	pthread_mutex_destroy(&m_MutexVideoIdTimer);
	pthread_mutex_destroy(&m_MutexDecryptIndexMap);
	pthread_mutex_destroy(&m_MutexDecryptFile);
}

status_t YtbDecrypt::GetDecryptSignature(const string &sDecryptVideoId,const string &sEncryptSig,string &sDecryptSig)
{
	status_t ret=GXCORE_ERROR;

	if(false==bInitStatus)return ret;

	pthread_mutex_lock(&m_MutexDecryptIndexMap);  
	map<unsigned int,vector<unsigned int> >::iterator it=m_mDecryptIndex.find(sEncryptSig.length());
	if(it!=m_mDecryptIndex.end())
	{
		vector<unsigned int> vDecryptIndex=m_mDecryptIndex[sEncryptSig.length()];
		if(!vDecryptIndex.empty())
		{//做过缓存,直接用vDecryptIndex进行解密
			unsigned int i=0;
			for(i=0;i<vDecryptIndex.size();i++)
			{
				if(vDecryptIndex[i]>=sEncryptSig.length())
				{//若数据处理正确,此流程不可能进入
					COMM_LOG("youtube",LOG_ERROR,"vDecryptIndex[%u]=%u,sEncryptSig.length()=%u,%s",i,vDecryptIndex[i],sEncryptSig.length(),"vDecryptIndex is error");
					m_mDecryptIndex.erase(it);
					break;
				}
				sDecryptSig=sDecryptSig+sEncryptSig.substr(vDecryptIndex[i],1);
			}
			if(i==vDecryptIndex.size())
			{
				ret=GXCORE_SUCCESS;
			}
		}else{
			COMM_LOG("youtube",LOG_ERROR,"%s","vDecryptIndex is empty");
			m_mDecryptIndex.erase(it);           
		}
	}else
	{//TODO
	}
	pthread_mutex_unlock(&m_MutexDecryptIndexMap); 

	pthread_mutex_lock(&m_MutexVideoIdStudy);  
	if(m_sVideoIdStudy.empty())
	{ 
		m_sVideoIdStudy=sDecryptVideoId;//后台会一直在获取
		m_unVideoIdStudyLen=sEncryptSig.length();
		COMM_LOG("youtube",LOG_DEBUG,"m_sVideoIdStudy=%s,m_unVideoIdStudyLen=%u",m_sVideoIdStudy.c_str(),m_unVideoIdStudyLen);
	}    
	pthread_mutex_unlock(&m_MutexVideoIdStudy);        


	return ret;
}

status_t YtbDecrypt::Slice(string sSlice,vector<unsigned int> &vDecryptIndex)
{
	status_t ret=GXCORE_ERROR;
	pcre  *pPcre=NULL;
	const char *pcError=NULL;
	int  nErrOffset;
	int  nOutVector[30]={0};
	int  nRc=0; 
	string sPattern;

	sPattern="slice\\((?P<idx>.*)\\)";
	if(sSlice.empty())return ret;
	pPcre = pcre_compile(sPattern.c_str(),       // pattern, 输入参数,将要被编译的字符串形式的正则表达式
			0,            // options, 输入参数,用来指定编译时的一些选项
			&pcError,       // errptr, 输出参数,用来输出错误信息
			&nErrOffset,   // erroffset, 输出参数,pattern中出错位置的偏移量
			NULL);        // tableptr, 输入参数,用来指定字符表,一般情况用NULL	
	if (pPcre == NULL) {                 //如果编译失败,返回错误信息
		COMM_LOG("youtube",LOG_ERROR,"PCRE compilation failed at offset %d: %s/n", nErrOffset, pcError);
		return ret;
	}	
	nRc = pcre_exec(pPcre,            // code, 输入参数,用pcre_compile编译好的正则表达结构的指针
			NULL,          // extra, 输入参数,用来向pcre_exec传一些额外的数据信息的结构的指针
			sSlice.c_str(),           // subject, 输入参数,要被用来匹配的字符串
			sSlice.length(),  // length, 输入参数, 要被用来匹配的字符串的指针
			0,             // startoffset, 输入参数,用来指定subject从什么位置开始被匹配的偏移量
			0,             // options, 输入参数, 用来指定匹配过程中的一些选项
			nOutVector,       // ovector, 输出参数,用来返回匹配位置偏移量的数组
			ARRAY_LEN(nOutVector));    // ovecsize, 输入参数, 用来返回匹配位置偏移量的数组的最大大小
	// 返回值：匹配成功返回非负数,没有匹配返回负数

	//printf("/nOK, has matched .../n/n");   //没有出错,已经匹配
	if(nRc==2)
	{
		string sSliceIndex;
		int nSubLength=0,nSliceIndex=0;
		nSubLength = nOutVector[3] - nOutVector[2];
		sSliceIndex=sSlice.substr(nOutVector[2],nSubLength);
		if(sSliceIndex.empty())
		{
			COMM_LOG("youtube",LOG_ERROR,"%s","sSliceIndex is empty");
			goto SLICE;            
		}
		if(0>(nSliceIndex=atoi(sSliceIndex.c_str())))
		{
			COMM_LOG("youtube",LOG_ERROR,"%s","sSliceIndex is empty");
			goto SLICE;   
		}
		printf("nSliceIndex=%d\n",nSliceIndex);
		COMM_LOG("youtube",LOG_DEBUG,"%s","");
		if(nSliceIndex<(int)(vDecryptIndex.size()))
		{
			for(int i=0;i<nSliceIndex;i++)
			{
				vDecryptIndex.erase(vDecryptIndex.begin());
				COMM_LOG("youtube",LOG_DEBUG,"%s","");
			}
		}else
		{
			COMM_LOG("youtube",LOG_ERROR,"%s","sSliceIndex is out of range");
			goto SLICE;         
		}

	}else{
		if (nRc == PCRE_ERROR_NOMATCH) printf("Sorry, no match ...\n");
		else printf("Matching error %d\n", nRc);
		goto SLICE;		
	}
	ret=GXCORE_SUCCESS;

SLICE:

	if(NULL!=pPcre)pcre_free(pPcre);pPcre=NULL; // 编译正则表达式re 释放内存		
	return ret;
}

status_t YtbDecrypt::Swap(string sSwap,vector<unsigned int> &vDecryptIndex)
{
	status_t ret=GXCORE_ERROR;
#if 0
	pcre  *pPcre=NULL;
	const char *pcError=NULL;
	int  nErrOffset;
	int  nOutVector[30]={0};
	int  nRc=0; 
#endif
	const char *pTmp=NULL;
	string sPattern= "(";   
	if(sSwap.empty())return ret;

#if 0
	sPattern="(?P<func>[a-zA-Z0-9]+)\\((?P<args>[a-z0-9,]+)\\)$";
	pPcre = pcre_compile(sPattern.c_str(),       // pattern, 输入参数,将要被编译的字符串形式的正则表达式
			0,            // options, 输入参数,用来指定编译时的一些选项
			&pcError,       // errptr, 输出参数,用来输出错误信息
			&nErrOffset,   // erroffset, 输出参数,pattern中出错位置的偏移量
			NULL);        // tableptr, 输入参数,用来指定字符表,一般情况用NULL	
	if (pPcre == NULL) {                 //如果编译失败,返回错误信息
		COMM_LOG("youtube",LOG_ERROR,"PCRE compilation failed at offset %d: %s/n", nErrOffset, pcError);
		return ret;
	}	
	nRc = pcre_exec(pPcre,            // code, 输入参数,用pcre_compile编译好的正则表达结构的指针
			NULL,          // extra, 输入参数,用来向pcre_exec传一些额外的数据信息的结构的指针
			sSwap.c_str(),           // subject, 输入参数,要被用来匹配的字符串
			sSwap.length(),  // length, 输入参数, 要被用来匹配的字符串的指针
			0,             // startoffset, 输入参数,用来指定subject从什么位置开始被匹配的偏移量
			0,             // options, 输入参数, 用来指定匹配过程中的一些选项
			nOutVector,       // ovector, 输出参数,用来返回匹配位置偏移量的数组
			ARRAY_LEN(nOutVector));    // ovecsize, 输入参数, 用来返回匹配位置偏移量的数组的最大大小
	// 返回值：匹配成功返回非负数,没有匹配返回负数

	//printf("/nOK, has matched .../n/n");   //没有出错,已经匹配
	if(nRc==3)
	{
#endif
		string sSwapIndex;
		int    nSwapIndex=0;
		string::size_type nStart,nEnd;

		nStart = sSwap.find(sPattern);
		if(nStart < sSwap.length())
		{
			nStart += sPattern.length();
			nEnd = sSwap.find(")", nStart);
			if(nEnd < sSwap.length())
			{
				sSwapIndex = sSwap.substr(nStart, (nEnd-nStart));

				if(sSwapIndex.find(",")>=sSwapIndex.length())//a,18
				{
					COMM_LOG("youtube",LOG_ERROR,"%s","sSwapIndex is empty");
					goto SWAP;            
				}
				pTmp = strtok((char *)sSwapIndex.c_str(),","); //分割字符串
				pTmp = strtok(NULL,",");
				if(NULL==pTmp)
				{
					COMM_LOG("youtube",LOG_ERROR,"%s","sSwapIndex is empty");
					goto SWAP;              
				}
				if(0>(nSwapIndex=atoi(pTmp)))
				{
					COMM_LOG("youtube",LOG_ERROR,"%s","sSwapIndex is empty");
					goto SWAP;   
				}
				COMM_LOG("youtube",LOG_DEBUG,"nSwapIndex=%d",nSwapIndex);
				if(nSwapIndex<(int)(vDecryptIndex.size()))
				{//function Pk(a,b){var c=a[0];a[0]=a[b%a.length];a[b]=c;return a}
					int nTmp=0;
					nTmp=vDecryptIndex[0];
					vDecryptIndex[0]=vDecryptIndex[nSwapIndex%vDecryptIndex.size()];
					vDecryptIndex[nSwapIndex]=nTmp;
					COMM_LOG("youtube",LOG_DEBUG,"%s","");
				}else{
					COMM_LOG("youtube",LOG_ERROR,"%s","sSwapIndex is out of range");
					goto SWAP;         
				}
			}else{
				COMM_LOG("youtube",LOG_DEBUG,"%s","Can't find )");
				goto SWAP;         
			}
		}else{
			COMM_LOG("youtube",LOG_DEBUG,"%s","Can't find (");
			goto SWAP;         
		}
#if 0
	}
	else{
		if (nRc == PCRE_ERROR_NOMATCH) printf("Sorry, no match ...\n");
		else printf("Matching error %d\n", nRc);
		goto SWAP;		
	}
#endif
	ret=GXCORE_SUCCESS;

SWAP:

	//if(NULL!=pPcre)pcre_free(pPcre);pPcre=NULL; // 编译正则表达式re 释放内存		
	return ret;
}

status_t YtbDecrypt::Splice(string sSplice,vector<unsigned int> &vDecryptIndex)
{
	status_t ret = GXCORE_ERROR;
	const char *pTmp=NULL;
	string sPattern = "(";
	string sSpliceIndex;
	int nSpliceIndex = 0;
	string::size_type nStart, nEnd;

	if(sSplice.empty())
		return ret;

	nStart = sSplice.find(sPattern);
	if(nStart < sSplice.length())
	{
		nStart += sPattern.length();
		nEnd = sSplice.find(")", nStart);
		if(nEnd < sSplice.length())
		{
			sSpliceIndex = sSplice.substr(nStart, (nEnd-nStart));

			if(sSpliceIndex.find(",") >= sSpliceIndex.length())//a,18
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","sSpliceIndex is empty");
				goto SPLICE;            
			}
			pTmp = strtok ((char *)sSpliceIndex.c_str(),","); //分割字符串
			pTmp = strtok(NULL,",");

			if(NULL==pTmp)
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","sSpliceIndex is empty");
				goto SPLICE;              
			}
			if(0 > (nSpliceIndex=atoi(pTmp)))
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","sSpliceIndex is empty");
				goto SPLICE;   
			}
			COMM_LOG("youtube",LOG_DEBUG,"nSpliceIndex=%d",nSpliceIndex);

			if(nSpliceIndex < (int)(vDecryptIndex.size()))
			{
				vDecryptIndex.erase(vDecryptIndex.begin(), (vDecryptIndex.begin()+nSpliceIndex));
				COMM_LOG("youtube",LOG_DEBUG,"%s","");
			}
			else
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","sSpliceIndex is out of range");
				goto SPLICE;         
			}
		}else{
			COMM_LOG("youtube",LOG_DEBUG,"%s","Can't find )");
			goto SPLICE;         
		}
	}else{
		COMM_LOG("youtube",LOG_DEBUG,"%s","Can't find (");
		goto SPLICE;         
	}
	ret=GXCORE_SUCCESS;

SPLICE:
	return ret;
}

#if 0
status_t YtbDecrypt::Resf(vector<string>  vArgs,vector<string>  vCode,vector<unsigned int> &vDecryptIndex)
{//注：vArgs中的内容暂时未用到,以后如有较智能点的算法,可能会用的到
	//we should develop interpret about this simple javascript later
	//cross-compile mozjs had failed 
	status_t ret=GXCORE_ERROR;

	if(false==bInitStatus)return ret;
	if(vArgs.size()==1)
	{//注：此解析方式较youtube-dl中不够灵活,但youtube-dl中也不能够完全能根据代码逆向解析出来
		//function Ok(a){a=a.split("");a=Pk(a,32);a=a.reverse();a=a.slice(3);a=a.reverse();
		//a=a.slice(1);a=a.reverse();a=Pk(a,19);a=Pk(a,24);a=a.slice(3);return a.join("")}
		for(unsigned int i=0;i<vCode.size();i++)
		{
			printf("----vCode[%u]=%s---\n",i,vCode[i].c_str());
			if(vCode[i].find("split")<vCode[i].length())
			{
				COMM_LOG("youtube",LOG_DEBUG,"%s","");
				continue;
			}else if(vCode[i]==string("a=a.reverse()"))
			{
				COMM_LOG("youtube",LOG_DEBUG,"%s","");
				reverse(vDecryptIndex.begin(),vDecryptIndex.end());
			}else if(vCode[i]==string("return a.join(\"\")"))
			{
				COMM_LOG("youtube",LOG_DEBUG,"%s","");
				continue;
			}else if(vCode[i].find("slice")<vCode[i].length())
			{//slice
				if(GXCORE_SUCCESS==Slice(vCode[i],vDecryptIndex))
				{
					COMM_LOG("youtube",LOG_DEBUG,"%s","");
					continue;
				}else
				{
					COMM_LOG("youtube",LOG_DEBUG,"Slice %s is error",vCode[i].c_str());
					return ret;         
				}

			}else if(vCode[i].find("var ")<vCode[i].length())
			{
				if(i+2>vCode.size())
				{
					COMM_LOG("youtube",LOG_ERROR,"%s is error",vCode[i].c_str());
					return ret;         
				}
				if(vCode[i+1].find("%a.length")<vCode[i+1].length() && vCode[i+2].find("]=b")<vCode[i+2].length())
				{
					string::size_type nStart,nEnd;
					string sPattern= "a[";  
					int nIndex=0;
					nStart=vCode[i+2].find(sPattern);
					if(nStart<vCode[i+2].length())
					{
						nStart=nStart+sPattern.length();
						nEnd=vCode[i+2].find("]=b",nStart);
						if(nEnd<vCode[i+2].length())
						{
							string sIndex=vCode[i+2].substr(nStart,nEnd-nStart);
							if(0>(nIndex=atoi(sIndex.c_str())))
							{
								COMM_LOG("youtube",LOG_ERROR,"%s","sIndex is empty");
								return ret;   
							}
							if(nIndex<(int)(vDecryptIndex.size()))
							{//function Pk(a,b){var c=a[0];a[0]=a[b%a.length];a[b]=c;return a}
								int nTmp=0;
								nTmp=vDecryptIndex[0];
								vDecryptIndex[0]=vDecryptIndex[nIndex%vDecryptIndex.size()];
								vDecryptIndex[nIndex]=nTmp;
								i=i+2;//jump two
								COMM_LOG("youtube",LOG_DEBUG,"%s","");
							}else
							{
								COMM_LOG("youtube",LOG_ERROR,"%s","sSwapIndex is out of range");
								return ret;         
							}

						}else
						{
							COMM_LOG("youtube",LOG_ERROR,"nStart=%d,nEnd=%d,%s",nStart,nEnd,"not find proper array index");
							return ret;
						}           
					}else
					{
						COMM_LOG("youtube",LOG_ERROR,"%s","not find array index");
						return ret;
					}      


					continue;
				}else
				{
					COMM_LOG("youtube",LOG_ERROR,"%s and %s are error",vCode[i+1].c_str(),vCode[i+2].c_str());
					return ret;         
				}
			}
			else if(vCode[i].find(",")<vCode[i].length())
			{//swap
				if(GXCORE_SUCCESS==Swap(vCode[i],vDecryptIndex))
				{
					COMM_LOG("youtube",LOG_DEBUG,"%s","");
					continue;
				}else
				{
					COMM_LOG("youtube",LOG_DEBUG,"Swap %s is error",vCode[i].c_str());
					return ret;         
				}                
			}else
			{
				COMM_LOG("youtube",LOG_DEBUG,"%s is not recognise",vCode[i].c_str());
				return ret;                
			}
		}
	}
	//    else if(vArgs.size()==2)//注：此比较固定,仅是个swap功能,基本不发生变化
	//    {//function Pk(a,b){var c=a[0];a[0]=a[b%a.length];a[b]=c;return a}
	//        
	//    }
	else
	{
		COMM_LOG("youtube",LOG_DEBUG,"%s","vArgs.size is error");
		return ret;
	}


	ret=GXCORE_SUCCESS;
	return ret;
}
#endif

status_t YtbDecrypt::Resf(vector<string> vArgs, vector<string> vCode, vector<unsigned int> &vDecryptIndex, vector<string> vSubTitle, vector<string> vSubFun, string FunVarName)
{//注：vArgs中的内容暂时未用到,以后如有较智能点的算法,可能会用的到
	//we should develop interpret about this simple javascript later
	//cross-compile mozjs had failed 
	status_t ret=GXCORE_ERROR;
	string sPattern;

	if(false==bInitStatus)return ret;
	if(vArgs.size()==1)
	{//注：此解析方式较youtube-dl中不够灵活,但youtube-dl中也不能够完全能根据代码逆向解析出来
	//以下代码为示例，主代码内容的名称和顺序一直会改变，但对应的子函数中的功能不变，所以需要将子函数的名称和对应的功能提取，然后在主函数中匹配名字，就知道相应的功能。
	//var lo={j1:function(a,b){var c=a[0];a[0]=a[b%a.length];a[b]=c},Fe:function(a,b){a.splice(0,b)},ub:function(a){a.reverse()}};function mo(a){a=a.split("");lo.Fe(a,2);lo.ub(a,61);lo.Fe(a,3);lo.ub(a,36);lo.j1(a,41);lo.Fe(a,1);lo.ub(a,36);lo.Fe(a,1);lo.ub(a,64);return a.join("")};

		for(unsigned int i=0;i<vCode.size();i++)
		{
			printf("----vCode[%u]=  %s---\n",i,vCode[i].c_str());
			if(vCode[i].find("split")<vCode[i].length())
			{
				COMM_LOG("youtube",LOG_DEBUG,"%s","");
				continue;
			}
			else if(vCode[i]==string("return a.join(\"\")"))
			{
				COMM_LOG("youtube",LOG_DEBUG,"%s","");
				continue;
			}
			else
			{
				for(vector<string>::size_type j=0; j<vSubTitle.size(); ++j)
				{
					sPattern = FunVarName + "." + vSubTitle[j];
					if(vCode[i].find(sPattern) < vCode[i].length())
					{
						COMM_LOG("youtube",LOG_DEBUG,"sPattren = %s %s",sPattern.c_str(), "");
						if(vSubFun[j].compare(YTB_SWAP) == 0)
						{
							COMM_LOG("youtube",LOG_DEBUG,"%s","Swap.....");
							if(GXCORE_SUCCESS != Swap(vCode[i],vDecryptIndex))
							{
								COMM_LOG("youtube",LOG_DEBUG,"Swap %s is error",vCode[i].c_str());
								return ret;         
							}
						}
						else if(vSubFun[j].compare(YTB_SPLICE) == 0)
						{
							COMM_LOG("youtube",LOG_DEBUG,"%s","splice....");
							if(GXCORE_SUCCESS != Splice(vCode[i], vDecryptIndex))
							{
								COMM_LOG("youtube",LOG_DEBUG,"Split %s is error",vCode[i].c_str());
								return ret;
							}
						}
						else if(vSubFun[j].compare(YTB_REVERSE) == 0)
						{
							COMM_LOG("youtube",LOG_DEBUG,"%s","reverse...");
							reverse(vDecryptIndex.begin(),vDecryptIndex.end());
						}
#if 0
						else if(vSubFun[j].compare(YTB_SLICE) == 0)
						{
							COMM_LOG("youtube",LOG_DEBUG,"%s","slice...");
							//do not set function, if need, set
						}
#endif
						else
						{;}
						break;
					}
					if(j == (vSubTitle.size()-1)) //如果函数名字没有与子函数名列表中的名字匹配，则返回出错。
					{
						COMM_LOG("youtube",LOG_DEBUG,"%s is not recognise",vCode[i].c_str());
						return ret;                
					}
				}
			}
		}
	}
	else
	{
		COMM_LOG("youtube",LOG_DEBUG,"%s","vArgs.size is error");
		return ret;
	}

	ret=GXCORE_SUCCESS;
	return ret;
}

status_t YtbDecrypt::ExtractFunctionQuick(string &sJsContent,string sFunName,vector<string>  &vArgs,vector<string>  &vCode)
{
#define ENTRY_FUNBODY_LEN_MAX (200)  //假定函数体最大值
	status_t ret=GXCORE_ERROR;

	string::size_type nStart,nEnd;
	string sPattern,sFunBody; 
	const char *pTmp=NULL;  
	if(sJsContent.empty())return ret;

	sPattern="function "+sFunName+"(a){";

	nStart=sJsContent.find(sPattern);
	if(nStart<sJsContent.length())
	{
		nStart=nStart+sPattern.length();
		nEnd=sJsContent.find("}",nStart);
		if(nEnd<sJsContent.length())
		{
			sFunBody=sJsContent.substr(nStart,nEnd-nStart);
			if(sFunBody.length()>ENTRY_FUNBODY_LEN_MAX)
			{
				COMM_LOG("youtube",LOG_ERROR,"%s","sEntryFunName.length is too long");
				goto GETENTRYFUNNAMEQUICK;                
			}

			pTmp= strtok ((char *)sFunBody.c_str(),";"); //分割字符串
			while(pTmp)
			{
				printf("---2pTmp: %s\n", pTmp);
				vCode.push_back(pTmp);//在容器尾部加入一个数据
				pTmp = strtok(NULL,";"); //指向下一个指针
			}  
			vArgs.push_back("a");           
		}else
		{
			COMM_LOG("youtube",LOG_ERROR,"%s","not find proper sEntryFunName");
			goto GETENTRYFUNNAMEQUICK;
		}           
	}else
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","not find signature");
		goto GETENTRYFUNNAMEQUICK;
	}      

	COMM_LOG("youtube",LOG_DEBUG,"%s","find sEntryFunName ...");
	ret=GXCORE_SUCCESS;

GETENTRYFUNNAMEQUICK:	
	return ret;
}

YtbDecrypt* YtbDecrypt::GetInstance()
{
	if(lInstance==0)
	{    
		if(0==(lInstance=new YtbDecrypt()))
		{
			COMM_LOG("youtube",LOG_ERROR,"%s","YtbDecrypt instance is error"); 
		}
	}    
	return lInstance;
}

status_t YtbDecrypt::ExtractFunction(string &sJsContent,string sFunName,vector<string>  &vArgs,vector<string>  &vCode)
{
	status_t ret=GXCORE_ERROR;
	pcre  *pPcre=NULL;
	const char *pcError=NULL;
	int  nErrOffset;
	int  nOutVector[30]={0};
	int  nRc=0; 
	const char *pTmp=NULL;
	string sPattern;

	//snprintf(sPatternTmp,sizeof(sPatternTmp),"function %s\((?P<args>[a-z,]+)\){(?P<code>[^}]+)}",StringTrim(sFunName).c_str());
	sPattern="function "+StringTrim(sFunName)+"\\((?P<args>[a-z,]+)\\){(?P<code>[^}]+)}";
	//sPattern="function Ok\\((?P<args>[a-z,]+)\\){(?P<code>[^}]+)}";
	if(sJsContent.empty())return ret;
	pPcre = pcre_compile(sPattern.c_str(),       // pattern, 输入参数,将要被编译的字符串形式的正则表达式
			0,            // options, 输入参数,用来指定编译时的一些选项
			&pcError,       // errptr, 输出参数,用来输出错误信息
			&nErrOffset,   // erroffset, 输出参数,pattern中出错位置的偏移量
			NULL);        // tableptr, 输入参数,用来指定字符表,一般情况用NULL	
	if (pPcre == NULL) {                 //如果编译失败,返回错误信息
		COMM_LOG("youtube",LOG_ERROR,"PCRE compilation failed at offset %d: %s", nErrOffset, pcError);
		return ret;
	}	
	nRc = pcre_exec(pPcre,            // code, 输入参数,用pcre_compile编译好的正则表达结构的指针
			NULL,          // extra, 输入参数,用来向pcre_exec传一些额外的数据信息的结构的指针
			sJsContent.c_str(),           // subject, 输入参数,要被用来匹配的字符串
			sJsContent.length(),  // length, 输入参数, 要被用来匹配的字符串的指针
			0,             // startoffset, 输入参数,用来指定subject从什么位置开始被匹配的偏移量
			0,             // options, 输入参数, 用来指定匹配过程中的一些选项
			nOutVector,       // ovector, 输出参数,用来返回匹配位置偏移量的数组
			ARRAY_LEN(nOutVector));    // ovecsize, 输入参数, 用来返回匹配位置偏移量的数组的最大大小
	// 返回值：匹配成功返回非负数,没有匹配返回负数

	if(nRc==3)
	{
		string sArgs,sCode;
		int nSubLength=0;
		nSubLength = nOutVector[3] - nOutVector[2];
		sArgs=sJsContent.substr(nOutVector[2],nSubLength);
		if(sArgs.empty())
		{
			COMM_LOG("youtube",LOG_ERROR,"%s","sArgs is empty");
			goto EXTRACTFUNCTION;            
		}

		pTmp= strtok ((char *)sArgs.c_str(),","); //分割字符串
		while(pTmp)
		{
			printf("---1pTmp: %s\n", pTmp);
			vArgs.push_back(pTmp);//在容器尾部加入一个数据
			pTmp = strtok(NULL,","); //指向下一个指针
		}        
		nSubLength = nOutVector[5] - nOutVector[4];
		sCode=sJsContent.substr(nOutVector[4],nSubLength);
		if(sCode.empty())
		{
			COMM_LOG("youtube",LOG_ERROR,"%s","sCode is empty");
			goto EXTRACTFUNCTION;            
		}
		//printf("sCode: %.*s\n", nSubLength, sCode);
		pTmp= strtok ((char *)sCode.c_str(),";"); //分割字符串
		while(pTmp)
		{
			printf("---2pTmp: %s\n", pTmp);
			vCode.push_back(pTmp);//在容器尾部加入一个数据

			pTmp = strtok(NULL,";"); //指向下一个指针
		}    
	}else{
		if (nRc == PCRE_ERROR_NOMATCH) COMM_LOG("youtube",LOG_ERROR,"%s","Sorry, no match ...");
		else COMM_LOG("youtube",LOG_ERROR,"Matching error %d", nRc);
		goto EXTRACTFUNCTION;		
	}
	COMM_LOG("youtube",LOG_DEBUG,"%s","OK, has matched ...");
	ret=GXCORE_SUCCESS;

EXTRACTFUNCTION:

	if(NULL!=pPcre)pcre_free(pPcre);pPcre=NULL; // 编译正则表达式re 释放内存		
	return ret;
}

//寻找到解析函数中的变量名
status_t YtbDecrypt::GetFunVarNameQuick(string &sJsContent, string sFunName, string &sFunVarName)
{
#define FUNCTION_VAR_NAME_LEN_MAX (20)  //假定入口函数最大值
	status_t ret = GXCORE_ERROR;
	string::size_type nStart, nEnd;
	string sPattern = "function " + sFunName;   

	if(sJsContent.empty())
		return ret;

	nStart = sJsContent.find(sPattern);
	if(nStart < sJsContent.length())
	{
		nStart = sJsContent.find(";",nStart);
		nStart += 1;
		if(nStart < sJsContent.length())
		{
			nEnd = sJsContent.find(".", nStart);
			if(nEnd < sJsContent.length())
			{
				sFunVarName = sJsContent.substr(nStart, (nEnd-nStart));
				if(sFunVarName.empty())
				{
					COMM_LOG("youtube",LOG_ERROR,"%s","sFunVarName's is empty");
				}
				else if(sFunVarName.length() > FUNCTION_VAR_NAME_LEN_MAX)
				{
					COMM_LOG("youtube",LOG_ERROR,"%s","sFunVarName's length is too long");
					sFunVarName = "";
				}
				else
				{
					COMM_LOG("youtube",LOG_ERROR,"sFunVarName: %s", sFunVarName.c_str());
					ret = GXCORE_SUCCESS;
				}
			}else{
				COMM_LOG("youtube",LOG_ERROR,"%s","sFunVarName can't find End");
			}
		}else{
			COMM_LOG("youtube",LOG_ERROR,"%s","sFunVarName can't find var name start");
		}
	}else{
		COMM_LOG("youtube",LOG_ERROR,"%s","sFunVarName can't find function name");
	}

	return ret;
}

//根据找到的解析函数中变量名，寻找变量中的各个子函数的名称和对应的功能
//分别保存在vSubFunName, vSubFun中, 一一对应。
//可以识别函数有：交换(swap)函数，reverse函数，splice函数，slice函数
status_t YtbDecrypt::GetSubFunNameQuick(string &sJsContent, string sFunVarName, vector<string> &vSubFunName, vector<string> &vSubFun)
{
#define FUNCTION_VAR_NAME_LEN_MAX (20)  //假定入口函数最大值
	status_t ret = GXCORE_ERROR;
	string::size_type nStart, nEnd, nVarCodeEnd;
	string pTmp, subFunCode;  
	string sPattern = "var " + sFunVarName + "={";   

	if(sJsContent.empty())
		return ret;

	nStart = sJsContent.find(sPattern);
	if(nStart < sJsContent.length())
	{
		nStart = sJsContent.find("{", nStart);
		nStart += 1;
		if(nStart < sJsContent.length())
		{
			nVarCodeEnd = sJsContent.find("};", nStart);
			if(nVarCodeEnd < sJsContent.length())
			{
				nEnd = sJsContent.find(":function", nStart);
				while(nEnd < nVarCodeEnd)
				{
					pTmp = sJsContent.substr(nStart, (nEnd-nStart));
					if(pTmp.empty())
					{
						COMM_LOG("youtube",LOG_ERROR,"%s", "Can't get sub function title");
						break;
					}
					vSubFunName.push_back(pTmp);//在容器尾部加入一个数据
					COMM_LOG("youtube",LOG_ERROR,"subfunname: %s", pTmp.c_str());

					nStart = sJsContent.find("{", nEnd);//找到子函数的开头
					nStart += 1;
					nEnd = sJsContent.find("}", nStart);//找到子函数的结尾处 
					//识别改子函数对应的函数意义，
					//可以识别函数有：交换(swap)函数，reverse函数，splice函数，slice函数
					//搜寻子函数中有没有已上可是别函数的关键字
					subFunCode = sJsContent.substr(nStart, (nEnd-nStart));
					if(pTmp.empty())
					{
						COMM_LOG("youtube",LOG_ERROR,"%s", "Can't get sub function content");
						break;
					}
					if(subFunCode.find("var ") < subFunCode.length())
					{
						pTmp = YTB_SWAP;
						vSubFun.push_back(pTmp);
						COMM_LOG("youtube",LOG_ERROR,"subfun: %s", pTmp.c_str());
						ret = GXCORE_SUCCESS;
					}
					else if(subFunCode.find(".reverse") < subFunCode.length())
					{
						pTmp = YTB_REVERSE;
						vSubFun.push_back(pTmp);
						COMM_LOG("youtube",LOG_ERROR,"subfun: %s", pTmp.c_str());
						ret = GXCORE_SUCCESS;
					}
					else if(subFunCode.find(".splice") < subFunCode.length())
					{
						pTmp = YTB_SPLICE;
						vSubFun.push_back(pTmp);
						COMM_LOG("youtube",LOG_ERROR,"subfun: %s", pTmp.c_str());
						ret = GXCORE_SUCCESS;
					}
					else if(subFunCode.find("slice") < subFunCode.length())
					{
						pTmp = YTB_SLICE;
						vSubFun.push_back(pTmp);
						COMM_LOG("youtube",LOG_ERROR,"subfun: %s", pTmp.c_str());
						ret = GXCORE_SUCCESS;
					}
					else
					{
						COMM_LOG("youtube",LOG_ERROR,"%s %s",subFunCode.c_str(),"is unknow function.");
						ret = GXCORE_ERROR;
						break;
					}

					//寻找下一个子函数名
					nStart = nEnd + 2;
					nEnd = sJsContent.find(":function", nEnd);
				}
			}else{
				COMM_LOG("youtube",LOG_ERROR,"%s", "Var function can't find the end }.");
			}
		}else{
			COMM_LOG("youtube",LOG_ERROR,"%s", "Var function can't find the start {.");
		}
	}else{
		COMM_LOG("youtube",LOG_ERROR,"%s", "Can't find the Var function name.");
	}

	return ret;
}

status_t YtbDecrypt::GetEntryFunNameQuick(string &sJsContent, string &sEntryFunName)
{
#define ENTRY_FUNNAME_LEN_MAX (20)  //假定入口函数最大值
	status_t ret=GXCORE_ERROR;

	string::size_type nStart,nEnd;
	string sPattern= "signature=";   
	if(sJsContent.empty())return ret;
	nStart=sJsContent.find(sPattern);
	if(nStart<sJsContent.length())
	{
		nStart=nStart+sPattern.length();
		nEnd=sJsContent.find("(",nStart);
		if(nEnd<sJsContent.length())
		{
			sEntryFunName=sJsContent.substr(nStart,nEnd-nStart);
			if(sEntryFunName.length()>ENTRY_FUNNAME_LEN_MAX)
			{
				COMM_LOG("youtube",LOG_ERROR,"nStart=%d,nEnd=%d,%s",nStart,nEnd,"sEntryFunName.length is too long");
				sEntryFunName="";
				goto GETENTRYFUNNAMEQUICK;                
			}
		}else
		{
			COMM_LOG("youtube",LOG_ERROR,"nStart=%d,nEnd=%d,%s",nStart,nEnd,"not find proper sEntryFunName");
			goto GETENTRYFUNNAMEQUICK;
		}           
	}else
	{
		COMM_LOG("youtube",LOG_ERROR,"%s","not find signature");
		goto GETENTRYFUNNAMEQUICK;
	}      

	COMM_LOG("youtube",LOG_DEBUG,"%s","find sEntryFunName ...");
	ret=GXCORE_SUCCESS;

GETENTRYFUNNAMEQUICK:	
	return ret;
}
status_t YtbDecrypt::GetEntryFunName(string &sJsContent,string &sEntryFunName)
{
	status_t ret=GXCORE_ERROR;
	pcre  *pPcre=NULL;
	const char *pcError=NULL;
	int  nErrOffset;
	int  nOutVector[8]={0};
	int  nRc=0;
	char  sPattern [] = "signature=([a-zA-Z]+)";   
	if(sJsContent.empty())return ret;
	pPcre = pcre_compile(sPattern,       // pattern, 输入参数,将要被编译的字符串形式的正则表达式
			0,            // options, 输入参数,用来指定编译时的一些选项
			&pcError,       // errptr, 输出参数,用来输出错误信息
			&nErrOffset,   // erroffset, 输出参数,pattern中出错位置的偏移量
			NULL);        // tableptr, 输入参数,用来指定字符表,一般情况用NULL	
	if (pPcre == NULL) {                 //如果编译失败,返回错误信息
		COMM_LOG("youtube",LOG_ERROR,"PCRE compilation failed at offset %d: %s/n", nErrOffset, pcError);
		return ret;
	}	
	nRc = pcre_exec(pPcre,            // code, 输入参数,用pcre_compile编译好的正则表达结构的指针
			NULL,          // extra, 输入参数,用来向pcre_exec传一些额外的数据信息的结构的指针
			sJsContent.c_str(),           // subject, 输入参数,要被用来匹配的字符串
			sJsContent.length(),  // length, 输入参数, 要被用来匹配的字符串的指针
			0,             // startoffset, 输入参数,用来指定subject从什么位置开始被匹配的偏移量
			0,             // options, 输入参数, 用来指定匹配过程中的一些选项
			nOutVector,       // ovector, 输出参数,用来返回匹配位置偏移量的数组
			ARRAY_LEN(nOutVector));    // ovecsize, 输入参数, 用来返回匹配位置偏移量的数组的最大大小
	// 返回值：匹配成功返回非负数,没有匹配返回负数

	//printf("/nOK, has matched .../n/n");   //没有出错,已经匹配
	if(nRc==2)
	{
		int nSubLength = nOutVector[3] - nOutVector[2];
		sEntryFunName=sJsContent.substr(nOutVector[2],nSubLength);
		if(sEntryFunName.empty())
		{
			COMM_LOG("youtube",LOG_ERROR,"%s","sEntryFunName is empty");
			goto GETENTRYFUNNAME;
		}
	}else{
		if (nRc == PCRE_ERROR_NOMATCH) printf("Sorry, no match ...\n");
		else printf("Matching error %d\n", nRc);
		goto GETENTRYFUNNAME;		
	}
	COMM_LOG("youtube",LOG_DEBUG,"%s","OK, has matched ...");
	ret=GXCORE_SUCCESS;

GETENTRYFUNNAME:

	if(NULL!=pPcre)pcre_free(pPcre);pPcre=NULL; // 编译正则表达式re 释放内存		
	return ret;
}


#endif
