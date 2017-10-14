#ifndef APP_SOCKET_H
#define APP_SOCKET_H
#ifdef LINUX_OS

#include <sys/un.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
//#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/un.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/select.h>
#include <net/if.h>
#include <poll.h>
#include <errno.h>
#if 0
typedef int bool;
#define false 	(0)
#define true	(1)
#endif

__BEGIN_DECLS

int CreateUnixSocket(char *pc_path);
int SendToUnixSocket(char *pc_path,char *pc_buf,unsigned int unBufLen);


bool Socket(int nFamily,int nType,int nProtocol,int *pnOutSock);
bool Bind(int nSocket,const struct sockaddr *pAddress,int nAddrLen);
bool Listen(int nSocket,int nBackLog);
bool Accept(int nSocket,struct sockaddr *pAddress,int *pnAddrLen,int *pnOutSock);

bool InetPton(int nFamily,const char *pIpSrc,void *pDst);
bool InetNtop(int nFamily,const void *pIpSrc,char *pcDst,unsigned int unDstLen);
bool Connect(int nSocket,const struct sockaddr *pAddress,int nAddrLen);



    int
receivedata(int socket,
        char * data, int length,
        int timeout, unsigned int * scope_id);


int connecthostport(const char * host, unsigned short port,
        unsigned int scope_id);
void *
getHTTPResponse(int s, int * size);
/* parseURL()
 * arguments :
 *   url :      source string not modified
 *   hostname : hostname destination string (size of MAXHOSTNAMELEN+1)
 *   port :     port (destination)
 *   path :     pointer to the path part of the URL
 *
 * Return values :
 *    0 - Failure
 *    1 - Success         */
int
parseURL(const char * url,
        char * hostname, unsigned short * port,
        char * * path, unsigned int * scope_id);
void *
miniwget(const char * url, int * size, unsigned int scope_id);
void *
miniwget_getaddr(const char * url, int * size,
        char * addr, int addrlen, unsigned int scope_id);



bool WgetWriteToFile(char *pUrl,char *pSavePath);
__END_DECLS
#endif 

#endif 
