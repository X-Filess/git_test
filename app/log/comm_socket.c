#ifdef LINUX_OS

#include <stdio.h>
#include "comm_socket.h"
#define MINIUPNPC_IGNORE_EINTR

#define PRINT_SOCKET_ERROR(x) perror(x)


#define MINIUPNPC_GET_SRC_ADDR (1)


#define closesocket close

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define USE_GETHOSTBYNAME
#undef USE_GETHOSTBYNAME
#define DEBUG
#define MINIUPNPC_SET_SOCKET_TIMEOUT

//#define MIN(x,y) (((x)<(y))?(x):(y))


static void *
miniwget3(const char * host,
        unsigned short port, const char * path,
        int * size, char * addr_str, int addr_str_len,
        const char * httpversion, unsigned int scope_id);
/* miniwget2() :
 *  * Call miniwget3(); retry with HTTP/1.1 if 1.0 fails. */
static void *
miniwget2(const char * host,
        unsigned short port, const char * path,
        int * size, char * addr_str, int addr_str_len,
        unsigned int scope_id);



bool WgetWriteToFile(char *pUrl,char *pSavePath)
{
    bool bRet=false;

    FILE* pFile=NULL;
    int nSize=0;
    char *pBuf=NULL;

    if(NULL==pUrl || NULL==pSavePath)
    {
        return bRet;
    }
    //printf("pUrl=%s,pSavePath=%s\n",pUrl,pSavePath);
    if(NULL==(pBuf=miniwget(pUrl,&nSize,0)))
    {
        printf("--miniwget failed.--\n");
        return bRet;
    }
    //printf("pBuf=%s,nSize=%d\n",pBuf,nSize);
    if(NULL==(pFile = fopen(pSavePath, "wb" )))
    {
        printf("--fopen pSavePath failed.--\n");
        return bRet;
    }
    if(1!=fwrite(pBuf,nSize,1,pFile))
    {
        printf("--fwrite failed.--\n");
        goto WGETWRITETOFILE;
    }
    bRet=true;
WGETWRITETOFILE:
    fclose(pFile);
    free(pBuf);

    return bRet;
}




/* connecthostport()
 *  * return a socket connected (TCP) to the host and port
 *   * or -1 in case of error */
int connecthostport(const char * host, unsigned short port, unsigned int scope_id)
{
    int s, n;
#ifdef USE_GETHOSTBYNAME
    struct sockaddr_in dest;
    struct hostent *hp;
#else /* #ifdef USE_GETHOSTBYNAME */
    char tmp_host[MAXHOSTNAMELEN+1];
    char port_str[8];
    struct addrinfo *ai, *p;
    struct addrinfo hints;
#endif /* #ifdef USE_GETHOSTBYNAME */
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
    struct timeval timeout;
#endif /* #ifdef MINIUPNPC_SET_SOCKET_TIMEOUT */

#ifdef USE_GETHOSTBYNAME
    hp = gethostbyname(host);
    if(hp == NULL)
    {
        herror(host);
        return -1;
    }
    memcpy(&dest.sin_addr, hp->h_addr, sizeof(dest.sin_addr));
    memset(dest.sin_zero, 0, sizeof(dest.sin_zero));
    s = socket(PF_INET, SOCK_STREAM, 0);
    if(s < 0)
    {
        PRINT_SOCKET_ERROR("socket");
        return -1;
    }
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
    /* setting a 3 seconds timeout for the connect() call */
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0)
    {
        PRINT_SOCKET_ERROR("setsockopt");
    }
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    if(setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)) < 0)
    {
        PRINT_SOCKET_ERROR("setsockopt");
    }
#endif /* #ifdef MINIUPNPC_SET_SOCKET_TIMEOUT */
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    n = connect(s, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
#ifdef MINIUPNPC_IGNORE_EINTR
    while(n < 0 && errno == EINTR)
    {
        socklen_t len;
        fd_set wset;
        int err;
        FD_ZERO(&wset);
        FD_SET(s, &wset);
        if((n = select(s + 1, NULL, &wset, NULL, NULL)) == -1 && errno == EINTR)
            continue;
        /*len = 0;*/
        /*n = getpeername(s, NULL, &len);*/
        len = sizeof(err);
        if(getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
            PRINT_SOCKET_ERROR("getsockopt");
            closesocket(s);
            return -1;
        }
        if(err != 0) {
            errno = err;
            n = -1;
        }
    }
#endif /* #ifdef MINIUPNPC_IGNORE_EINTR */
    if(n<0)
    {
        PRINT_SOCKET_ERROR("connect");
        closesocket(s);
        return -1;
    }
#else /* #ifdef USE_GETHOSTBYNAME */
    /* use getaddrinfo() instead of gethostbyname() */
    memset(&hints, 0, sizeof(hints));
    /* hints.ai_flags = AI_ADDRCONFIG; */
#ifdef AI_NUMERICSERV
    hints.ai_flags = AI_NUMERICSERV;
#endif
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC; /* AF_INET, AF_INET6 or AF_UNSPEC */
    /* hints.ai_protocol = IPPROTO_TCP; */
    snprintf(port_str, sizeof(port_str), "%hu", port);
    if(host[0] == '[')
    {
        /* literal ip v6 address */
        int i, j;
        for(i = 0, j = 1; host[j] && (host[j] != ']') && i < MAXHOSTNAMELEN; i++, j++)
        {
            tmp_host[i] = host[j];
            if(0 == memcmp(host+j, "%25", 3))   /* %25 is just url encoding for '%' */
                j+=2;                           /* skip "25" */
        }
        tmp_host[i] = '\0';
    }
    else
    {
        strncpy(tmp_host, host, MAXHOSTNAMELEN);
    }
    tmp_host[MAXHOSTNAMELEN] = '\0';
    n = getaddrinfo(tmp_host, port_str, &hints, &ai);
    if(n != 0)
    {
        fprintf(stderr, "getaddrinfo() error : %s\n", gai_strerror(n));
        return -1;
    }
    s = -1;
    for(p = ai; p; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(s < 0)
            continue;
        if(p->ai_addr->sa_family == AF_INET6 && scope_id > 0) {
            struct sockaddr_in6 * addr6 = (struct sockaddr_in6 *)p->ai_addr;
            addr6->sin6_scope_id = scope_id;
        }
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
        /* setting a 3 seconds timeout for the connect() call */
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0)
        {
            PRINT_SOCKET_ERROR("setsockopt");
        }
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        if(setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)) < 0)
        {
            PRINT_SOCKET_ERROR("setsockopt");
        }
#endif /* #ifdef MINIUPNPC_SET_SOCKET_TIMEOUT */
        n = connect(s, p->ai_addr, p->ai_addrlen);
#ifdef MINIUPNPC_IGNORE_EINTR
        while(n < 0 && errno == EINTR)
        {
            socklen_t len;
            fd_set wset;
            int err;
            FD_ZERO(&wset);
            FD_SET(s, &wset);
            if((n = select(s + 1, NULL, &wset, NULL, NULL)) == -1 && errno == EINTR)
                continue;
            /*len = 0;*/
            /*n = getpeername(s, NULL, &len);*/
            len = sizeof(err);
            if(getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                PRINT_SOCKET_ERROR("getsockopt");
                closesocket(s);
                freeaddrinfo(ai);
                return -1;
            }
            if(err != 0) {
                errno = err;
                n = -1;
            }
        }
#endif /* #ifdef MINIUPNPC_IGNORE_EINTR */
        if(n < 0)
        {
            closesocket(s);
            continue;
        }
        else
        {
            break;
        }
    }
    freeaddrinfo(ai);
    if(s < 0)
    {
        PRINT_SOCKET_ERROR("socket");
        return -1;
    }
    if(n < 0)
    {
        PRINT_SOCKET_ERROR("connect");
        return -1;
    }
#endif /* #ifdef USE_GETHOSTBYNAME */
    return s;
}


    void *
miniwget(const char * url, int * size, unsigned int scope_id)
{
    unsigned short port;
    char * path;
    /* protocol://host:port/chemin */
    char hostname[MAXHOSTNAMELEN+1];
    *size = 0;
    if(!parseURL(url, hostname, &port, &path, &scope_id))
        return NULL;
#ifdef DEBUG
    printf("parsed url : hostname='%s' port=%hu path='%s' scope_id=%u\n",
            hostname, port, path, scope_id);
#endif
    return miniwget2(hostname, port, path, size, 0, 0, scope_id);
}

    void *
miniwget_getaddr(const char * url, int * size,
        char * addr, int addrlen, unsigned int scope_id)
{
    unsigned short port;
    char * path;
    /* protocol://host:port/path */
    char hostname[MAXHOSTNAMELEN+1];
    *size = 0;
    if(addr)
        addr[0] = '\0';
    if(!parseURL(url, hostname, &port, &path, &scope_id))
        return NULL;
#ifdef DEBUG
    printf("parsed url : hostname='%s' port=%hu path='%s' scope_id=%u\n",
            hostname, port, path, scope_id);
#endif
    return miniwget2(hostname, port, path, size, addr, addrlen, scope_id);
}




/* miniwget2() :
 *  * Call miniwget3(); retry with HTTP/1.1 if 1.0 fails. */
    static void *
miniwget2(const char * host,
        unsigned short port, const char * path,
        int * size, char * addr_str, int addr_str_len,
        unsigned int scope_id)
{
    char * respbuffer;

#if 1
    respbuffer = miniwget3(host, port, path, size,
            addr_str, addr_str_len, "1.1", scope_id);
#else
    respbuffer = miniwget3(host, port, path, size,
            addr_str, addr_str_len, "1.0", scope_id);
    if (*size == 0)
    {
#ifdef DEBUG
        printf("Retrying with HTTP/1.1\n");
#endif
        free(respbuffer);
        respbuffer = miniwget3(host, port, path, size,
                addr_str, addr_str_len, "1.1", scope_id);
    }
#endif
    return respbuffer;
}


/* parseURL()
 *  * arguments :
 *   *   url :      source string not modified
 *    *   hostname : hostname destination string (size of MAXHOSTNAMELEN+1)
 *     *   port :     port (destination)
 *      *   path :     pointer to the path part of the URL
 *       *
 *        * Return values :
 *         *    0 - Failure
 *          *    1 - Success         */
    int
parseURL(const char * url,
        char * hostname, unsigned short * port,
        char * * path, unsigned int * scope_id)
{
    char * p1, *p2, *p3;
    if(!url)
        return 0;
    p1 = strstr(url, "://");
    if(!p1)
        return 0;
    p1 += 3;
    if(  (url[0]!='h') || (url[1]!='t')
            ||(url[2]!='t') || (url[3]!='p'))
        return 0;
    memset(hostname, 0, MAXHOSTNAMELEN + 1);
    if(*p1 == '[')
    {
        /* IP v6 : http://[2a00:1450:8002::6a]/path/abc */
        char * scope;
        scope = strchr(p1, '%');
        p2 = strchr(p1, ']');
        if(p2 && scope && scope < p2 && scope_id) {
            /* parse scope */
#ifdef IF_NAMESIZE
            char tmp[IF_NAMESIZE];
            int l;
            scope++;
            /* "%25" is just '%' in URL encoding */
            if(scope[0] == '2' && scope[1] == '5')
                scope += 2; /* skip "25" */
            l = p2 - scope;
            if(l >= IF_NAMESIZE)
                l = IF_NAMESIZE - 1;
            memcpy(tmp, scope, l);
            tmp[l] = '\0';
            *scope_id = if_nametoindex(tmp);
            if(*scope_id == 0) {
                *scope_id = (unsigned int)strtoul(tmp, NULL, 10);
            }
#else
            /* under windows, scope is numerical */
            char tmp[8];
            int l;
            scope++;
            /* "%25" is just '%' in URL encoding */
            if(scope[0] == '2' && scope[1] == '5')
                scope += 2; /* skip "25" */
            l = p2 - scope;
            if(l >= sizeof(tmp))
                l = sizeof(tmp) - 1;
            memcpy(tmp, scope, l);
            tmp[l] = '\0';
            *scope_id = (unsigned int)strtoul(tmp, NULL, 10);
#endif
        }
        p3 = strchr(p1, '/');
        if(p2 && p3)
        {
            p2++;
            strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p2-p1)));
            if(*p2 == ':')
            {
                *port = 0;
                p2++;
                while( (*p2 >= '0') && (*p2 <= '9'))
                {
                    *port *= 10;
                    *port += (unsigned short)(*p2 - '0');
                    p2++;
                }
            }
            else
            {
                *port = 80;
            }
            *path = p3;
            return 1;
        }
    }
    p2 = strchr(p1, ':');
    p3 = strchr(p1, '/');
    if(!p3)
        return 0;
    if(!p2 || (p2>p3))
    {
        strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p3-p1)));
        *port = 80;
    }
    else
    {
        strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p2-p1)));
        *port = 0;
        p2++;
        while( (*p2 >= '0') && (*p2 <= '9'))
        {
            *port *= 10;
            *port += (unsigned short)(*p2 - '0');
            p2++;
        }
    }
    *path = p3;
    return 1;
}




/*
 *  * Read a HTTP response from a socket.
 *   * Process Content-Length and Transfer-encoding headers.
 *    * return a pointer to the content buffer, which length is saved
 *     * to the length parameter.parameter */
    void *
getHTTPResponse(int s, int * size)
{
    char buf[2048];
    int n;
    int endofheaders = 0;
    int chunked = 0;
    int content_length = -1;
    unsigned int chunksize = 0;
    unsigned int bytestocopy = 0;
    /* buffers : */
    char * header_buf;
    unsigned int header_buf_len = 2048;
    unsigned int header_buf_used = 0;
    char * content_buf;
    unsigned int content_buf_len = 2048;
    unsigned int content_buf_used = 0;
    char chunksize_buf[32];
    unsigned int chunksize_buf_index;

    header_buf = GxCore_Malloc(header_buf_len);
    content_buf = GxCore_Malloc(content_buf_len);
    chunksize_buf[0] = '\0';
    chunksize_buf_index = 0;
    while((n = receivedata(s, buf, 2048, 5000, NULL)) > 0)
    {
        if(endofheaders == 0)
        {
            int i;
            int linestart=0;
            int colon=0;
            int valuestart=0;
            if(header_buf_used + n > header_buf_len) {
                header_buf = realloc(header_buf, header_buf_used + n);
                header_buf_len = header_buf_used + n;
            }
            memcpy(header_buf + header_buf_used, buf, n);
            header_buf_used += n;
            /* search for CR LF CR LF (end of headers)
             *              * recognize also LF LF */
            i = 0;
            while(i < ((int)header_buf_used-1) && (endofheaders == 0)) {
                if(header_buf[i] == '\r') {
                    i++;
                    if(header_buf[i] == '\n') {
                        i++;
                        if(i < (int)header_buf_used && header_buf[i] == '\r') {
                            i++;
                            if(i < (int)header_buf_used && header_buf[i] == '\n') {
                                endofheaders = i+1;
                            }
                        }
                    }
                } else if(header_buf[i] == '\n') {
                    i++;
                    if(header_buf[i] == '\n') {
                        endofheaders = i+1;
                    }
                }
                i++;
            }
            if(endofheaders == 0)
                continue;
            /* parse header lines */
            for(i = 0; i < endofheaders - 1; i++) {
                if(colon <= linestart && header_buf[i]==':')
                {
                    colon = i;
                    while(i < (endofheaders-1)
                            && (header_buf[i+1] == ' ' || header_buf[i+1] == '\t'))
                        i++;
                    valuestart = i + 1;
                }
                /* detecting end of line */
                else if(header_buf[i]=='\r' || header_buf[i]=='\n')
                {
                    if(colon > linestart && valuestart > colon)
                    {
#ifdef DEBUG
                        printf("header='%.*s', value='%.*s'\n",
                                colon-linestart, header_buf+linestart,
                                i-valuestart, header_buf+valuestart);
#endif
                        if(0==strncasecmp(header_buf+linestart, "content-length", colon-linestart))
                        {
                            content_length = atoi(header_buf+valuestart);
#ifdef DEBUG
                            printf("Content-Length: %d\n", content_length);
#endif
                        }
                        else if(0==strncasecmp(header_buf+linestart, "transfer-encoding", colon-linestart)
                                && 0==strncasecmp(header_buf+valuestart, "chunked", 7))
                        {
#ifdef DEBUG
                            printf("chunked transfer-encoding!\n");
#endif
                            chunked = 1;
                        }
                    }
                    while((i < (int)header_buf_used) && (header_buf[i]=='\r' || header_buf[i] == '\n'))
                        i++;
                    linestart = i;
                    colon = linestart;
                    valuestart = 0;
                }
            }

            /* copy the remaining of the received data back to buf */
            n = header_buf_used - endofheaders;
            memcpy(buf, header_buf + endofheaders, n);
            /* if(headers) */
        }
        if(endofheaders)
        {
            /* content */
            if(chunked)
            {
                int i = 0;
                while(i < n)
                {
                    if(chunksize == 0)
                    {
                        /* reading chunk size */
                        if(chunksize_buf_index == 0) {
                            /* skipping any leading CR LF */
                            if(i<n && buf[i] == '\r') i++;
                            if(i<n && buf[i] == '\n') i++;
                        }
                        while(i<n && isxdigit(buf[i])
                                && chunksize_buf_index < (sizeof(chunksize_buf)-1))
                        {
                            chunksize_buf[chunksize_buf_index++] = buf[i];
                            chunksize_buf[chunksize_buf_index] = '\0';
                            i++;
                        }
                        while(i<n && buf[i] != '\r' && buf[i] != '\n')
                            i++; /* discarding chunk-extension */
                        if(i<n && buf[i] == '\r') i++;
                        if(i<n && buf[i] == '\n') {
                            unsigned int j;
                            for(j = 0; j < chunksize_buf_index; j++) {
                                if(chunksize_buf[j] >= '0'
                                        && chunksize_buf[j] <= '9')
                                    chunksize = (chunksize << 4) + (chunksize_buf[j] - '0');
                                else
                                    chunksize = (chunksize << 4) + ((chunksize_buf[j] | 32) - 'a' + 10);
                            }
                            chunksize_buf[0] = '\0';
                            chunksize_buf_index = 0;
                            i++;
                        } else {
                            /* not finished to get chunksize */
                            continue;
                        }
#ifdef DEBUG
                        printf("chunksize = %u (%x)\n", chunksize, chunksize);
#endif
                        if(chunksize == 0)
                        {
#ifdef DEBUG
                            printf("end of HTTP content - %d %d\n", i, n);
                            /*printf("'%.*s'\n", n-i, buf+i);*/
#endif
                            goto end_of_stream;
                        }
                    }
                    bytestocopy = ((int)chunksize < (n - i))?chunksize:(unsigned int)(n - i);
                    if((content_buf_used + bytestocopy) > content_buf_len)
                    {
                        if(content_length >= (int)(content_buf_used + bytestocopy)) {
                            content_buf_len = content_length;
                        } else {
                            content_buf_len = content_buf_used + bytestocopy;
                        }
                        content_buf = (char *)realloc((void *)content_buf,
                                content_buf_len);
                    }
                    memcpy(content_buf + content_buf_used, buf + i, bytestocopy);
                    content_buf_used += bytestocopy;
                    i += bytestocopy;
                    chunksize -= bytestocopy;
                }
            }
            else
            {
                /* not chunked */
                if(content_length > 0
                        && (int)(content_buf_used + n) > content_length) {
                    /* skipping additional bytes */
                    n = content_length - content_buf_used;
                }
                if(content_buf_used + n > content_buf_len)
                {
                    if(content_length >= (int)(content_buf_used + n)) {
                        content_buf_len = content_length;
                    } else {
                        content_buf_len = content_buf_used + n;
                    }
                    content_buf = (char *)realloc((void *)content_buf,
                            content_buf_len);
                }
                memcpy(content_buf + content_buf_used, buf, n);
                content_buf_used += n;
            }
        }
        /* use the Content-Length header value if available */
        if(content_length > 0 && (int)content_buf_used >= content_length)
        {
#ifdef DEBUG
            printf("End of HTTP content\n");
#endif
            break;
        }
    }
end_of_stream:
    free(header_buf); header_buf = NULL;
    *size = content_buf_used;
    if(content_buf_used == 0)
    {
        free(content_buf);
        content_buf = NULL;
    }
    return content_buf;
}





/* miniwget3() :
 *  * do all the work.
 *   * Return NULL if something failed. */
    static void *
miniwget3(const char * host,
        unsigned short port, const char * path,
        int * size, char * addr_str, int addr_str_len,
        const char * httpversion, unsigned int scope_id)
{
    char buf[2048];
    int s;
    int n;
    int len;
    int sent;
    void * content;

    *size = 0;
    s = connecthostport(host, port, scope_id);
    if(s < 0)
        return NULL;

    /* get address for caller ! */
    if(addr_str)
    {
        struct sockaddr_storage saddr;
        socklen_t saddrlen;

        saddrlen = sizeof(saddr);
        if(getsockname(s, (struct sockaddr *)&saddr, &saddrlen) < 0)
        {
            perror("getsockname");
        }
        else
        {
            /* getnameinfo return ip v6 address with the scope identifier
             *              * such as : 2a01:e35:8b2b:7330::%4281128194 */
            n = getnameinfo((const struct sockaddr *)&saddr, saddrlen,
                    addr_str, addr_str_len,
                    NULL, 0,
                    NI_NUMERICHOST | NI_NUMERICSERV);
            if(n != 0) {
                fprintf(stderr, "getnameinfo() failed : %s\n", gai_strerror(n));
            }
        }
#ifdef DEBUG
        printf("address miniwget : %s\n", addr_str);
#endif
    }
    len = snprintf(buf, sizeof(buf),
            "GET %s HTTP/%s\r\n"
            "Host: %s:%d\r\n"
            "Connection: Close\r\n"
            //"User-Agent: " OS_STRING ", UPnP/1.0, MiniUPnPc/" MINIUPNPC_VERSION_STRING "\r\n"

            "\r\n",
            path, httpversion, host, port);
    sent = 0;
    /* sending the HTTP request */
    while(sent < len)
    {
        n = send(s, buf+sent, len-sent, 0);
        if(n < 0)
        {
            perror("send");
            closesocket(s);
            return NULL;
        }
        else
        {
            sent += n;
        }
    }
    content = getHTTPResponse(s, size);
    closesocket(s);
    return content;
}



    int
receivedata(int socket,
        char * data, int length,
        int timeout, unsigned int * scope_id)
{
#if MINIUPNPC_GET_SRC_ADDR
#ifdef DEBUG
    /* to shut up valgrind about uninit value */
    struct sockaddr_storage src_addr = {0};
#else
    struct sockaddr_storage src_addr;
#endif
    socklen_t src_addr_len = sizeof(src_addr);
#endif
    int n;
    /* using poll */
    struct pollfd fds[1]; /* for the poll */
#ifdef MINIUPNPC_IGNORE_EINTR
    do {
#endif
        fds[0].fd = socket;
        fds[0].events = POLLIN;
        n = poll(fds, 1, timeout);
#ifdef MINIUPNPC_IGNORE_EINTR
    } while(n < 0 && errno == EINTR);
#endif
    if(n < 0) {
        PRINT_SOCKET_ERROR("poll");
        return -1;
    } else if(n == 0) {
        /* timeout */
        return 0;
    }
#if MINIUPNPC_GET_SRC_ADDR
    n = recvfrom(socket, data, length, 0,
            (struct sockaddr *)&src_addr, &src_addr_len);
#else
    n = recv(socket, data, length, 0);
#endif
    if(n<0) {
        PRINT_SOCKET_ERROR("recv");
    }
#if MINIUPNPC_GET_SRC_ADDR
    if (src_addr.ss_family == AF_INET6) {
        const struct sockaddr_in6 * src_addr6 = (struct sockaddr_in6 *)&src_addr;
#ifdef DEBUG
        printf("scope_id=%u\n", src_addr6->sin6_scope_id);
#endif
        if(scope_id)
            *scope_id = src_addr6->sin6_scope_id;
    }
#endif
    return n;
}


bool Socket(int nFamily,int nType,int nProtocol,int *pnOutSock)
{
    bool bRet=true;

    if(NULL==pnOutSock)
    {
        perror("pnOutSock is null\n");
        return false;
    }

    if((*pnOutSock=socket(nFamily,nType,nProtocol))<0)
    {
        perror("socket  create error\n");
        return false;
    }

    return bRet;
}

bool Bind(int nSocket,const struct sockaddr *pAddress,int nAddrLen)
{
    bool bRet=true;

   if(bind(nSocket,pAddress,nAddrLen)<0)
    {
        perror("bind socket is error\n");
        return false;
    }

    return bRet;
}


bool Listen(int nSocket,int nBackLog)
{
    bool bRet=true;
    char *pStr=NULL;

    if(NULL!=(pStr=getenv("LISTENQ")))
        nSocket=atoi(pStr);

    if(listen(nSocket,nBackLog)<0)
    {
        perror("listen socket is error\n");
        return false;
    }

    return bRet;
}

bool Accept(int nSocket,struct sockaddr *pAddress,int *pnAddrLen,int *pnOutSock)
{
    bool bRet=true;
    socklen_t addr_len = 0;

    if((pAddress == NULL) || (pnAddrLen == NULL) || (pnOutSock == NULL))
        return false;

    addr_len = *pnAddrLen;
    if((*pnOutSock=accept(nSocket,pAddress,&addr_len))<0)
    {
        perror("accept socket is error\n");
        return false;
    }

    return bRet;
}

bool InetPton(int nFamily,const char *pIpSrc,void *pDst)
{
    bool bRet=true;

    if(NULL==pIpSrc || NULL ==pDst || strlen(pIpSrc)<8)
    {
        perror("param is error\n");
        return false;
    }

    if(inet_pton(nFamily,pIpSrc,pDst)<0)
    {
        perror("InetPton is error\n");
        return false;
    }

    return bRet;
}

bool InetNtop(int nFamily,const void *pIpSrc,char *pcDst,unsigned int unDstLen)
{
    bool bRet=true;

    if(NULL==pIpSrc || NULL ==pcDst)
    {
        perror("param is error\n");
        return false;
    }

    if(NULL==inet_ntop(nFamily,pIpSrc,pcDst,unDstLen))
    {
        perror("InetPton is error\n");
        return false;
    }

    return bRet;
}

bool Connect(int nSocket,const struct sockaddr *pAddress,int nAddrLen)
{
    bool bRet=true;

    if(nSocket<0)
    {
        perror("nSocket is error\n");
        return false;
    }

    if(connect(nSocket,pAddress,nAddrLen)<0)
    {
        perror("connect socket is error\n");
        return false;
    }

    return bRet;
}

int CreateUnixSocket(char *pc_path)
{
    int n_sockfd = -1;
    int nOn = 0;
    struct sockaddr_un st_ser_addr;

    if(NULL==pc_path)goto UNIXSOCK;

    unlink(pc_path);
    memset(&st_ser_addr,0,sizeof(st_ser_addr));
    st_ser_addr.sun_family = AF_UNIX;
    strcpy(st_ser_addr.sun_path,pc_path);

    n_sockfd = socket(AF_UNIX,SOCK_DGRAM,0);
    if(n_sockfd < 0 )
    {
        perror("socket error");
        goto UNIXSOCK;
    }
    nOn = 1;
    setsockopt(n_sockfd, SOL_SOCKET, SO_REUSEADDR, &nOn, sizeof(nOn) );

    if(bind(n_sockfd,(struct sockaddr *)&st_ser_addr,sizeof(st_ser_addr)) < 0)
    {
        perror("bind error");
        close(n_sockfd);
        n_sockfd=-1;
        goto UNIXSOCK;
    }
    //printf("Bind is ok\n");


UNIXSOCK:

    return n_sockfd;
}

#if 0/* BEGIN: Deleted by yingc, 2013/12/16 */
/*  将一个结构体发送达到域套接字中  */
bool SendToUnixSocket(char *pc_path,char *pc_buf,int nBufLen)
{
    bool bRet=false;
    int n_sockfd=-1;
    struct sockaddr_un st_ser_addr;

    if(NULL==pc_path)goto SENDTOUNIX;
    if(NULL==pc_buf)goto SENDTOUNIX;
    if(nBufLen<=0)goto SENDTOUNIX;

    memset(&st_ser_addr,0,sizeof(st_ser_addr));
    st_ser_addr.sun_family = AF_UNIX;
    strcpy(st_ser_addr.sun_path,pc_path);

    n_sockfd = socket(AF_UNIX,SOCK_DGRAM,0);
    if(n_sockfd < 0 )
    {
        perror("socket error");
        goto SENDTOUNIX;
    }

    if((sendto(n_sockfd,pc_buf,nBufLen,0,(struct sockaddr*)&st_ser_addr,sizeof(st_ser_addr))) < 0)
    {
        perror("bind error");
        close(n_sockfd);
        goto SENDTOUNIX;
    }
    bRet=true;

SENDTOUNIX:
    close(n_sockfd);
    return bRet;
}
#endif/* END:   Deleted by yingc, 2013/12/16   PN: */



int SendToUnixSocket(char *pc_path,char *pc_buf,unsigned int unBufLen)
{
    int ret = -1;
    int n_sockfd=-1;
    struct sockaddr_un st_ser_addr;

    if(NULL==pc_path)goto SENDTOUNIX;
    if(NULL==pc_buf)goto SENDTOUNIX;

    memset(&st_ser_addr,0,sizeof(st_ser_addr));
    st_ser_addr.sun_family = AF_UNIX;
    strcpy(st_ser_addr.sun_path,pc_path);

    n_sockfd = socket(AF_UNIX,SOCK_DGRAM,0);
    if(n_sockfd < 0 )
    {
        perror("socket error");
        goto SENDTOUNIX;
    }

    if((ret=sendto(n_sockfd,pc_buf,unBufLen,0,(struct sockaddr*)&st_ser_addr,sizeof(st_ser_addr))) < 0)
    {
        perror("sendto error");
        //close(n_sockfd);
        goto SENDTOUNIX;
    }


SENDTOUNIX:
    close(n_sockfd);
    return ret;
}





#endif











