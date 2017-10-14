#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LINUX_OS
#include <sys/socket.h>
#include <sys/types.h>

//#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>

#endif
#include <time.h>
#include <errno.h>
//#include <signal.h>
#include <unistd.h>


#include "comm_socket.h"
#include "comm_log.h"



static LogModuleType lstLogModule[LOG_MODULE_COUNT_MAX]={//最多有
{"gcyin",LOG_ERROR},\
{"youtube",LOG_DEBUG},\


};
static log_level_e leConditionDefault=LOG_INFO;//默认的条件等级，若module name 不存在时或为空时，使用此条件
static log_level_map_s ls_log_map_arr[]={\
	{LOG_DEBUG,"DEBUG"},\
	{LOG_WARN,"WARN"},\
	{LOG_INFO,"INFO"},\
	{LOG_ERROR,"ERROR"},\
	{LOG_FATAL,"FATAL"},\
};
#define LOG_LEVEL_NAME_MAP_COUNT_MAX		(ARRAY_LEN(ls_log_map_arr))
static LogOutPutType leOutPutType=LOG_OUT_PUT_CONSOLE;
static LogOutPutType lePreOutPutType=LOG_OUT_PUT_CONSOLE;
static char lsPreDiskSelPath[50]={0};
static bool lbLogHadInit=false;
#ifdef LINUX_OS
static bool lbLogElfHadRun=false;
#endif
typedef unsigned char GXxmlChar;
#define LOG_INIT_FILE_PATH	"/usr/share/app/ncconfig.xml"
#define ROOT_STR         (GXxmlChar*)"globalconfig"
#define LOG_STR        (GXxmlChar*)"commlog"
#define MODULE_STR        (GXxmlChar*)"modulename"
#define CONDITION_STR        (GXxmlChar*)"levelcondition"
#define ITEM_STR        (GXxmlChar*)"item"

#ifndef LINUX_OS
static int LogBufHandle(char *pcBuffer);
#endif

void CommLogSetOutPutType(LogOutPutType eOutPutType)
{
	leOutPutType=eOutPutType;
}
void CommLogSetDefaultCondition(log_level_e eConditon)
{
	leConditionDefault=eConditon;
}
void CommLogSetModule(const char *pcModuleName,log_level_e  eConditon)
{
	if(NULL==pcModuleName || strlen(pcModuleName)<1)return;
	int i=0;
	for(i=0;i<LOG_MODULE_COUNT_MAX;i++)
	{
		if(strlen(lstLogModule[i].sModuleName)==0)
		{//如果找到第一个没有使用过的
			snprintf(lstLogModule[i].sModuleName,sizeof(lstLogModule[i].sModuleName),"%s",pcModuleName);
			lstLogModule[i].eCondition=eConditon;
			break;
		}	
		if(strcasecmp(pcModuleName,lstLogModule[i].sModuleName)==0)
		{
			lstLogModule[i].eCondition=eConditon;
		}
	}
}

bool CommLogInit()
{
	bool bRet=false;
	GXxmlDocPtr doc=NULL;
	GXxmlChar *pModuleValue=NULL,*pConditionValue=NULL;
	GXxmlNodePtr group=NULL,xndTmp=NULL;
	int nConditionValue=0;	
	if(false==lbLogHadInit)
	{
		doc = GXxmlParseFile(LOG_INIT_FILE_PATH);
		if (NULL == doc)
		{
			printf( "[comm_log]GXxml parse error.\n"); 
			#ifndef LINUX_OS
			
				lbLogHadInit=true;
			#endif
			goto       LOGINIT;                                                                      
		}                                                                                                         
		                                                                  
		// root                                                                                                   
		GXxmlNodePtr root = doc->root;                                                                            
		if (root == NULL)                                                                                         
		{                                                                                                         
			printf( "[comm_log]GXxml empty\n");                                                                    
			goto       LOGINIT;                                                                                 
		}                                                                                                         
		                                                                  
		if (0 !=  GXxmlStrcmp(root->name, ROOT_STR))                                                               
		{                                                                                                         
			goto       LOGINIT;                                                                                     
		}                                                                                                         
		                                                                  
		// group                                                                                                  
		group = root->childs;                                                                                                                                         
		while (NULL != group)                                                                                     
		{  
			if(0==GXxmlStrcmp(group->name,LOG_STR))
			{
				xndTmp=group->childs;
				while(NULL!=xndTmp)
				{
					if(0==GXxmlStrcmp(xndTmp->name,ITEM_STR))
					{
						pModuleValue = GXxmlGetProp(xndTmp, MODULE_STR); 
						pConditionValue = GXxmlGetProp(xndTmp, CONDITION_STR);   
						if(pModuleValue!=NULL && pConditionValue!=NULL)
						{
							nConditionValue=atoi((const char *)pConditionValue);
							if(nConditionValue<LOG_MAX && nConditionValue>=LOG_DEBUG)
							{
								printf("set module---\n");
								CommLogSetModule((const char *)pModuleValue,nConditionValue);
							}
						}
						if(pModuleValue!=NULL)
						{
							GxCore_Free(pModuleValue);                                                                                   
							pModuleValue = NULL;  
						}
						if(pConditionValue!=NULL)
						{
							GxCore_Free(pConditionValue);                                                                                   
							pConditionValue = NULL;  
						}
					}
					xndTmp=xndTmp->next;
				}
				break;
			}else
			{
				group = group->next;   
			}		                                        
		}
		lbLogHadInit=true;
	}

	bRet=true;
LOGINIT:
	if(doc)
	{
		GXxmlFreeDoc(doc);
		doc=NULL;
	}
	if(pModuleValue)
	{
		GxCore_Free(pModuleValue);                                                                                   
		pModuleValue = NULL;  
	}
	if(pConditionValue)
	{
		GxCore_Free(pConditionValue);                                                                                   
		pConditionValue = NULL;  
	}
	return bRet;
}
static log_level_e CommLogGetModCondition(const char *pcModuleName)
{
	if(NULL==pcModuleName || strlen(pcModuleName)<1)return leConditionDefault;
	int i=0;
	for(i=0;i<LOG_MODULE_COUNT_MAX;i++)
	{	
		if(strcasecmp(pcModuleName,lstLogModule[i].sModuleName)==0)
		{
			return lstLogModule[i].eCondition;
		}
	}

	return leConditionDefault;
}


#if 0/* BEGIN: Deleted by yingc, 2013/8/5 */
int main()
{
	int ret=0;
	int n_sockfd=-1,n_read_len=-1,n_addr_len=-1;
	char s_recv_buf[LOG_BUFFER_SIZE_MAX] = {0};
	char s_unix_path[UNIX_DOMAIN_PATH_LEN_MAX]={0};
	struct sockaddr_un st_cli_addr;

	if(access(UNIX_DOMAIN_DIR,0)<0)/* 如果不存在则进行创建  */
	{
		if(mkdir(UNIX_DOMAIN_DIR,S_IRWXU)<0)
		{
			printf("mkdir is error\n");
			return ret;
		}
	}

	snprintf(s_unix_path,UNIX_DOMAIN_PATH_LEN_MAX,"%s/%s",UNIX_DOMAIN_DIR,UNIX_PATH_LOG);
	if(0>(n_sockfd=CreateUnixSocket(s_unix_path)))
	{
		printf("CreateUnixSocket is error\n");
	}
	n_addr_len=sizeof(st_cli_addr);

	while(1)
	{
		n_read_len=-1;
		memset(&st_cli_addr,0,sizeof(st_cli_addr));	

		n_read_len=recvfrom(n_sockfd,s_recv_buf,LOG_BUFFER_SIZE_MAX,0,(struct sockaddr*)&st_cli_addr,&n_addr_len);
		if(n_read_len<0)
		{

		}
		else if(n_read_len>0)
		{
			printf("Recv: %s\n",s_recv_buf);
		}
		else
		{
			printf("-----n_read_len=%d---\n",n_read_len);
			continue;
		}
		
	}
	printf("--gcyin:--file=%s,func=%s,line=%d---connect is success----\n",__FILE__,__FUNCTION__,__LINE__);

	return ret;
}
#endif/* END:   Deleted by yingc, 2013/8/5   PN: */
#if 0

int NcLogHandle(log_level_e e_condition,log_level_e e_level,const char *pc_file,const char *pc_fun,int n_line,char *pc_fmt, ...)
{



}
#endif

int LogHandle(const char * pcModuleName,log_level_e e_level,const char *pc_file,const char *pc_fun,int n_line,const char *pc_fmt, ...)
{
	int ret = -1;
	int n_len_tmp=0,i=0,n_level_found=0;
	char *pc_buff=NULL,*pc_record=NULL,*pc_time=NULL;
	int nOutPutTypeChangeFlag=0;
	char *pcResult=NULL,*pcResultConfirm=NULL;	
	time_t t_time_tmp;
	va_list list;
	log_level_e e_condition;
	//char s_levelname[10]={0};
	char s_unix_path[UNIX_DOMAIN_PATH_LEN_MAX]={0};
	char sPartition[50]={0};

	if(false==CommLogInit())
	{
		printf("CommLogInit failed\n");
	}
	GxBus_ConfigGetInt(LOG_OUT_PUT_TYPE, (int32_t *)(&leOutPutType),LOG_OUT_PUT_CONSOLE);
	GxBus_ConfigGet(LOG_DISK_SELECT, sPartition, sizeof(sPartition), "tmp");
	if(leOutPutType!=lePreOutPutType)
	{
		lePreOutPutType=leOutPutType;
		nOutPutTypeChangeFlag=1;
	}	
	if(strcmp(lsPreDiskSelPath,sPartition))
	{
		snprintf(lsPreDiskSelPath,sizeof(lsPreDiskSelPath),"%s",sPartition);
		nOutPutTypeChangeFlag=1;
	}
	if(LOG_OUT_PUT_NULL==leOutPutType)goto LOGHANDLE;

	snprintf(s_unix_path,UNIX_DOMAIN_PATH_LEN_MAX,"%s/%s",UNIX_DOMAIN_DIR,UNIX_PATH_LOG);	

	e_condition=CommLogGetModCondition(pcModuleName);
	if(e_level<e_condition)goto LOGHANDLE;/* 条件可更改,如果不够这个级别则不进行记录  */
	
	pc_time=app_time_to_format_str(get_display_time_by_timezone(time(&t_time_tmp)));
	if(NULL==pc_time)
	{
		printf("--pc_time  get is error--\n");
		goto LOGHANDLE;
	}
	if(NULL==(pc_buff=(GxCore_Malloc(LOG_BUFFER_SIZE_MAX))))
	{
		printf("--pc_buff  GxCore_Malloc is error--\n");
		goto LOGHANDLE;
	}
	memset(pc_buff,0,LOG_BUFFER_SIZE_MAX);
	va_start(list, pc_fmt); 
	vsnprintf(pc_buff, LOG_BUFFER_SIZE_MAX,pc_fmt, list); 
	va_end(list); 
	if(NULL==(pc_record=(GxCore_Malloc(LOG_RECORD_SIZE_MAX))))
	{
		printf("--pc_record  GxCore_Malloc is error--\n");
		goto LOGHANDLE;
	}
	memset(pc_record,0,LOG_RECORD_SIZE_MAX);

	if(leOutPutType!=LOG_OUT_PUT_CONSOLE)
	{
		n_len_tmp+=snprintf(pc_record+n_len_tmp,LOG_RECORD_SIZE_MAX-n_len_tmp,"[%s]",LOG_FILE_DEFAULT_NAME);
	}
	
	n_len_tmp+=snprintf(pc_record+n_len_tmp,LOG_RECORD_SIZE_MAX-n_len_tmp,"[[%s]]",pc_time);
	
	for(i=0;i<LOG_LEVEL_NAME_MAP_COUNT_MAX;i++)
	{
		if(ls_log_map_arr[i].e_level==e_level)
		{
			n_len_tmp+=snprintf(pc_record+n_len_tmp,LOG_RECORD_SIZE_MAX-n_len_tmp," [%s]",ls_log_map_arr[i].s_level);
			n_level_found=1;
			break;
		}
	}
	if(n_level_found==0)
	{
		printf("--n_level_found   is not found--\n");
		goto LOGHANDLE;
	}
	if(NULL==pcModuleName)
	{
		n_len_tmp+=snprintf(pc_record+n_len_tmp,LOG_RECORD_SIZE_MAX-n_len_tmp," [%s]","NULL");
	}else
	{
		if(strlen(pcModuleName)<=LOG_MODULE_NAME_LEN_MAX)
		{
			n_len_tmp+=snprintf(pc_record+n_len_tmp,LOG_RECORD_SIZE_MAX-n_len_tmp," [%s]",pcModuleName);
		}else
		{
			printf("--pcModuleName   is error--\n");
			goto LOGHANDLE;
		}
	}

	n_len_tmp+=snprintf(pc_record+n_len_tmp,LOG_RECORD_SIZE_MAX-n_len_tmp," [%s |%s |%d]",pc_file,pc_fun,n_line);
	n_len_tmp+=snprintf(pc_record+n_len_tmp,LOG_RECORD_SIZE_MAX-n_len_tmp," [%s]\n",pc_buff);

#ifdef LINUX_OS
	//if(leOutPutType==LOG_OUT_PUT_SOCKET)
	if(leOutPutType!=LOG_OUT_PUT_CONSOLE)
	{/*  注:当是linux系统时,日志由log.elf统一进行处理*/
	/*    */

		if(lbLogElfHadRun==false)
		{
			
			char sCmdTmp[50]={0};
			if(0==ExecuteCmd("ps",&pcResult) && NULL!=pcResult)
			{//注意此处的函数调用，防止递归调用
#define COMM_LOG_DAEMON_NAME "commlogd"
				if(strstr(pcResult,COMM_LOG_DAEMON_NAME))
				{
					lbLogElfHadRun=true;
				}else
				{
					snprintf(sCmdTmp,sizeof(sCmdTmp),"%s &",	COMM_LOG_DAEMON_NAME);
					system(sCmdTmp);
					usleep(200);//防止正确执行后ps  无法获取正确结果,同时让守护准备好
					if(0==ExecuteCmd("ps",&pcResultConfirm) && NULL!=pcResultConfirm)
					{//确认守护是否执行
						//printf("pcResultConfirm=%s\n",pcResultConfirm);
						if(strstr(pcResultConfirm,COMM_LOG_DAEMON_NAME))
						{
							lbLogElfHadRun=true;
						}else{
							//说明守护不存在，提示打印
							printf("[comm log]: %s is not exist!!!!",COMM_LOG_DAEMON_NAME);
							printf("%s",pc_record);
							goto LOGHANDLE;
						}
						
					}else
					{
						printf("2ExecuteCmd(\"ps\",&pcResult)  error\n");
						printf("%s",pc_record);
						goto LOGHANDLE;					
					}
					
				}
				
			}else
			{
				printf("ExecuteCmd(\"ps\",&pcResult)  error\n");
				printf("%s",pc_record);
				goto LOGHANDLE;
			}
		}
		if(nOutPutTypeChangeFlag)//若发生改变则发送相应的控制字
		{
			char sLogUnixPath[50]={0};
			LogControlRecord tmp={0};
			if(leOutPutType==LOG_OUT_PUT_DISK)
			{
				tmp.eControlType=LOG_CONTROL_SAVE_DISK;
				GxBus_ConfigGet(LOG_DISK_SELECT, tmp.sControlValue, sizeof(tmp.sControlValue), "/media/sda1");
			}else
			{
				tmp.eControlType=LOG_CONTROL_SAVE_FLASH;
			}
				
			//snprintf(tmp.sControlValue,sizeof(tmp.sControlValue),"%s","/media/sda1");
			snprintf(sLogUnixPath,sizeof(sLogUnixPath),"%s/%s",UNIX_DOMAIN_DIR,UNIX_PATH_LOG);
			SendToUnixSocket(sLogUnixPath,(char *)(&tmp),sizeof(LogControlRecord));
		}
		if(0>(ret=SendToUnixSocket(s_unix_path,pc_record,strlen(pc_record))))
		{//此处最好使用静态连接
			//printf("--SendToUnixSocket   is error--\n");
			goto LOGHANDLE;
		}
	}
	else
	{//说明是直接打到串口输出
		printf("%s",pc_record);
	}
#else
	LogBufHandle(pc_record);
	//printf("%s",pc_record);
#endif

LOGHANDLE:

	if(NULL!=pc_buff)GxCore_Free(pc_buff);
	if(NULL!=pc_record)GxCore_Free(pc_record);
	if(NULL!=pc_time)GxCore_Free(pc_time);
	pc_record=NULL;
	pc_buff=NULL;
	pc_time=NULL;
	if(NULL!=pcResultConfirm)GxCore_Free(pcResultConfirm);
	if(NULL!=pcResult)GxCore_Free(pcResult);

	
	return ret;
}



#if 0/* BEGIN: Deleted by yingc, 2013/12/12 */
inline void comm_log(const char * pcModuleName,log_level_e level,char *args,...)
{
     LogHandle(pcModuleName,level,__FILE__,__FUNCTION__,__LINE__,args);
}
#endif/* END:   Deleted by yingc, 2013/12/12   PN: */

#ifndef LINUX_OS
static int LogBufHandle(char *pcBuffer)
{
	int ret = -1;
	uint32_t unFileSize=0,unBufSize=0;
	char sFilePath[50]={0};
	GxFileInfo fileInfo={0};
	handle_t fileHandle=-1;
	char *pcBuf=NULL;
	char sDiskPath[50]={0};

	if(NULL==pcBuffer)
	{
		printf("--pcBuffer  is NULL--\n");
		goto LOGBUFHANDLE;
	}
	if(leOutPutType==LOG_OUT_PUT_DISK)
	{
		GxBus_ConfigGet(LOG_DISK_SELECT, sDiskPath, sizeof(sDiskPath), "tmp");
		snprintf(sFilePath,sizeof(sFilePath),"%s/%s",sDiskPath,LOG_FILE_DEFAULT_NAME);
	}else if(leOutPutType==LOG_OUT_PUT_FLASH)
	{
		snprintf(sFilePath,sizeof(sFilePath),"%s%s",LOG_FILE_DEFAULT_FLASH_PATH,LOG_FILE_DEFAULT_NAME);
	}else
	{
		printf("%s",pcBuffer);
		goto LOGBUFHANDLE;
	}
	

	if(GXCORE_FILE_EXIST!=GxCore_FileExists(sFilePath))
	{
		if(0>=(fileHandle=GxCore_Open(sFilePath,"a+")))
		{//说明很有可能是路径不存在，此时直接打到串口上
			if(leOutPutType==LOG_OUT_PUT_DISK)
			{//说明disk可能已拔出，重新设置输出方式
				GxBus_ConfigSetInt(LOG_OUT_PUT_TYPE, LOG_OUT_PUT_CONSOLE);
			}
			printf("--fopen(%s,\"a+\")  is error--\n",sFilePath);
			printf("%s",pcBuffer);
			goto LOGBUFHANDLE;
		}
		GxCore_Close(fileHandle);
		fileHandle=-1;	
		unFileSize=0;
	}else
	{
		GxCore_GetFileInfo(sFilePath,&fileInfo);
		//printf("sFilePath=%s,fileInfo.type=%d\n",sFilePath,fileInfo.type);
		if(fileInfo.type==GX_FILE_REGULAR)
		{
			unFileSize=fileInfo.size_by_bytes;
		}else
		{
			printf("inner error,LogBufHandle\n");
			printf("%s",pcBuffer);
			goto LOGBUFHANDLE;
		}
	}
	unBufSize=strlen(pcBuffer)+1;

	if(unBufSize>=LOG_FILE_SIZE_MAX)
	{/*  如果要写入的内容超出文件的最大值，则直接返回不写入 */
		printf("--unBufSize>=LOG_FILE_SIZE_MAX  is error--\n");
		goto LOGBUFHANDLE;
	}

	if(unFileSize+unBufSize>=LOG_FILE_SIZE_MAX)/* 超出范围则进行截断 */
	{
		
		if(NULL==(pcBuf=GxCore_Malloc(unFileSize+1)))
		{
			printf("--GxCore_Malloc  is error--\n");
			goto LOGBUFHANDLE;
		}else
		{
			memset(pcBuf,0,unFileSize+1);
			if(0>=(fileHandle=GxCore_Open(sFilePath,"a+")))
			{
				printf("--fopen(%s,\"a+\")  is error--\n",sFilePath);
				goto LOGBUFHANDLE;
			}else
			{
				if(GxCore_ReadAt(fileHandle,pcBuf,unFileSize,1,unFileSize/2)<0)
				{
					printf("--GxCore_Read  is error--\n");
					printf("%s",pcBuffer);
					goto LOGBUFHANDLE;
				}
				GxCore_Close(fileHandle);
				fileHandle=-1;
			}			
			if(0>=(fileHandle=GxCore_Open(sFilePath,"w+")))
			{//直接覆盖，注，此处处理不同于log daemon，没有对一些截断细节进行处理
			//若必要，可参照log守护进程
				printf("--fopen(%s,\"w+\")  is error--\n",sFilePath);
				goto LOGBUFHANDLE;
			}else
			{
				if(GxCore_Write(fileHandle,pcBuf,strlen(pcBuf),1)<0)
				{
					printf("--GxCore_Write  is error--\n");
					printf("%s",pcBuffer);
				}
				GxCore_Sync(fileHandle);
				GxCore_Close(fileHandle);
				fileHandle=-1;
			}
		}

	}

	//截断后以及没有超出范围的情况下可以直接写入文件
	if(0>=(fileHandle=GxCore_Open(sFilePath,"a+")))
	{
		printf("--fopen(%s,\"a+\")  is error--\n",sFilePath);
		goto LOGBUFHANDLE;
	}else
	{
		if(GxCore_Write(fileHandle,pcBuffer,strlen(pcBuffer),1)<0)
		{
			printf("--GxCore_Write  is error--\n");
			printf("%s",pcBuffer);
		}
	}
	GxCore_Sync(fileHandle);
	GxCore_Close(fileHandle);
	fileHandle=-1;


//printf("--gcyin:--file=%s,func=%s,line=%d-------\n",__FILE__,__FUNCTION__,__LINE__);
LOGBUFHANDLE:
	if(-1!=fileHandle)
	{
		GxCore_Sync(fileHandle);
		GxCore_Close(fileHandle);
	}
	if(NULL!=pcBuf)GxCore_Free(pcBuf);

	return ret;
}
#endif
















