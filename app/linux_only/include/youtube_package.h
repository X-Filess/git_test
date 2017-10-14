#ifndef __YOUTUBE_PACKAGE_H__
#define __YOUTUBE_PACKAGE_H__

#include "app_tools.h"
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

#include <pcre.h>
#include "app_curl.h"
#include "comm_log.h"
#include "comm_socket.h"
#include "jansson.h"


#define YTB_WATCH_URL_FORMATS "https://www.youtube.com/watch?v=%s&gl=US&hl=en&has_verified=1"
#define YTB_DECRYPT_VIDEOID_HTTP_PATH ("https://gdata.youtube.com/feeds/api/standardfeeds/most_popular?v=2&max-results=1")
#define YTB_DECRYPT_VIDEOID_SAVE_PATH  "/tmp/youtube/decrypt_videoid.xml"
#define YTB_WATCH_FILE_PATH		"/tmp/youtube/youtube_watch"
#define YTB_WATCH_INNER_FILE_PATH		"/tmp/youtube/youtube_watch_inner"
#define YTB_DECRYPTED_JS_FILE_PATH		"/tmp/youtube/youtube_decrypt.js"
#define YTB_DECRYPTED_SWF_FILE_PATH		"/tmp/youtube/youtube_decrypt.swf"

#define YTB_WATCH_STUDY_FILE_PATH		"/tmp/youtube/youtube_watch_study"
#define YTB_DECRYPTED_JS_STUDY_FILE_PATH		"/tmp/youtube/youtube_decrypt_study.js"



__BEGIN_DECLS


status_t YtbGetUrlByVideoId(const char *pcVideoId,char *pcUrlOut);


class YtbDecrypt
{
#define DECRYPT_INDEX_SAVE_FILE_PATH ("/home/gx/youtube_decrypt_index.txt")
#define DECRYPT_UPDATE_TIME_DEFAULT (30)//unit is minute
#define DECRYPT_VIDEO_ID_LEN_MAX    (128)
#define VIDEO_ID_TIMER_VECTOR_SIZE_MAX    (6)
#define FUZZY_SIG_LEN_MIN (79)
#define FUZZY_SIG_LEN_MAX (93)
public:
    virtual ~YtbDecrypt();
    
    virtual status_t GetDecryptSignature(const string &sDecryptVideoId,const string &sEncryptSig,string &sDecryptSig);
    bool GetInitStatus();
    //bool GetIsCached();
    
    static YtbDecrypt *GetInstance();

protected:

private:
    //enum{VIDEO_ID_TIMER_VECTOR_SIZE_MAX=10};//vector最大值为10,若超出则将前一个删除掉
    class GarbageCollect // 它的唯一工作就是在析构函数中删除CSingleton的实例 
    {
    public:
        ~GarbageCollect()
        { 
            if (YtbDecrypt::lInstance)
            {
                delete YtbDecrypt::lInstance;
                YtbDecrypt::lInstance=NULL;
            }
        }
    };
    static GarbageCollect lGarbage; 
    //js file decrypt
    YtbDecrypt();
    
    status_t ExtractFunction(string &sJsContent,string sFunName,vector<string>  &vArgs,vector<string>  &vCode);
    status_t ExtractFunctionQuick(string &sJsContent,string sFunName,vector<string>  &vArgs,vector<string>  &vCode);
	status_t Resf(vector<string> vArgs, vector<string> vCode, vector<unsigned int> &vDecryptIndex, vector<string> vSubTitle, vector<string> vSubFun, string FunVarName);
    //status_t Resf(vector<string>  vArgs,vector<string>  vCode,vector<unsigned int> &vDecryptIndex);
    status_t GetEntryFunName(string &sJsContent,string &sEntryFunName);
    status_t GetEntryFunNameQuick(string &sJsContent,string &sEntryFunName);
	status_t GetFunVarNameQuick(string &sJsContent, string sFunName, string &sFunVarName);
	status_t GetSubFunNameQuick(string &sJsContent, string sFunVarName, vector<string> &vSubFunName, vector<string> &vSubFun);
    //status_t GenInnerDecryptIndexVector();
    status_t Slice(string sSlice,vector<unsigned int> &vDecryptIndex);
    status_t Swap(string sSwap,vector<unsigned int> &vDecryptIndex);
    status_t Splice(string sSplice,vector<unsigned int> &vDecryptIndex);
    status_t GetDecryptFileContent(string &sServerFilePath,string &sContent,const string &sFileSavePath=(YTB_DECRYPTED_JS_FILE_PATH));//swf or js
    status_t GetWatchFileContent(string &sWatchContent);
    //status_t GetWatchFileContent(const string &sDecryptVideoId,string &sWatchContent);
    status_t GetWatchFileContent(const string &sDecryptVideoId,string &sWatchContent,const string &sFileSavePath=(YTB_WATCH_INNER_FILE_PATH));
    status_t GetPlayerConfigQuick(const string &sWatchContent,string &sJsonPlayerConfig);
    status_t GetDecryptFileServerPath(const string &sJsonPlayerConfig,string &sServerFilePath);
    //status_t GenDecryptIndexVector();
    status_t GetEncryptSigQuick(const string &sJsonPlayerConfig,string &sEncryptSig);

    status_t SaveDecryptIndexVector();
    status_t LoadDecryptIndexVector();  
    status_t SaveVideoIdVector();
    status_t LoadVideoIdVector();    
    //string sJsContent;
    
    //string sDecryptVideoId;
    
    
    map<unsigned int,vector<unsigned int> > m_mDecryptIndex;
    //map<unsigned int,vector<unsigned int>> m_mDecryptIndexUsed;//目前在用的
    //vector<unsigned int> vDecryptIndex;//用于定时器获取新的
    //vector<unsigned int> vDecryptIndexUsed;//目前在用的
    vector<string> m_vVideoIdTimer;
    vector<string> m_vVideoIdTimerTmp;//加锁控制,赋值给m_vVideoIdTimer
    string m_sVideoIdStudy;//同一时刻只有一个，将要学习的线程，优先级高于update线程，学习完后，加入到m_vVideoIdTimer
    unsigned int m_unVideoIdStudyLen;
    unsigned int unTimeInterval;
    bool bInitStatus;
    //bool bHasCached;
    //bool bNeedUpdate;
    
    pthread_mutex_t m_MutexVideoIdStudy;
    pthread_mutex_t m_MutexVideoIdTimer;
    pthread_mutex_t m_MutexDecryptIndexMap;
    pthread_mutex_t m_MutexDecryptFile;
    friend void *GetDecryptIndexThread(void *pArg);
    
//    pthread_mutex_t m_MutexStudyAndTimer;
//    pthread_cond_t m_CondStudyAndTimer;
      
    friend void *StudyDecryptIndexThread(void *pArg);//有且仅有一个study线程
    
    static YtbDecrypt* lInstance;
};

inline bool YtbDecrypt::GetInitStatus()
{
    return bInitStatus;
}

__END_DECLS

#endif
