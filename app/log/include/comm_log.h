#ifndef COMM_LOG_H
#define COMM_LOG_H

#ifdef LINUX_OS
#include <sys/un.h>
#endif
#include "app_utility.h"
#include "tree.h"
#include "parser.h"
#include "app_default_params.h"



#define LOG_FILE_USER_DEFINED_NAME	"stblog.txt" /* ÃÃÃÃ­Â³ÃÃÃ²ÃÂ±ÃÃÂ¶Â¨ÃÃ¥ÃÂ»Â¸Ã¶ÃÃÃÂ¾Â´Ã¦Â´Â¢ÃÃÂ¼Ã¾,Â³Â¤Â¶ÃÃÃÃÃÃÂª50 */
#define LOG_FILE_NAME_LEN_MAX	(20)
#define LOG_FILE_PATH_LEN_MAX	(50)/*  ÃÃÃÂ¾ ÃÃÂ¼Ã¾ÂµÃÃÂ«ÃÂ·Â¾Â¶Â³Â¤Â¶Ã     */

#define LOG_MODULE_NAME_LEN_MAX	(20)/*   ÃÂ£Â¿Ã©ÃÃ»ÃÃÂµÃÃÃ®Â´Ã³ÃÂµ  */

#define LOG_MODULE_COUNT_MAX	(50)/*    ÃÂ£Â¿Ã©ÃÃ®Â´Ã³ÃÃ½   */

#define LOG_FILE_DEFAULT_NAME	"stblog.txt"
#define LOG_FILE_DEFAULT_RAM_PATH	"/var/log/"
#define LOG_FILE_DEFAULT_FLASH_PATH	"/home/gx/"

#define UNIX_DOMAIN_DIR	"/tmp/unixdomain"
#define UNIX_PATH_LOG	"log"
#define UNIX_DOMAIN_PATH_LEN_MAX	(50)

#define LOG_RECORD_SIZE_MAX	(2048)/*  ÃÂ»ÃÃÂ¼ÃÃÂ¼ÃÃÂµÃÂ´Ã³ÃÂ¡(Â°Ã¼ÂºÂ¬Â¿ÃÃÃÃÃ)   */
#define LOG_BUFFER_SIZE_MAX	(1536)/* ÃÂªÂ¼ÃÃÂ¼ÂµÃÃÃÂ·Ã»Â´Â®ÂµÃÂ´Ã³ÃÂ¡  */
#define LOG_TIME_SIZE_MAX	(30)/*    ÃÃÃÂ¾ÃÂ±Â¼Ã¤ÂµÃÂ´Ã³ÃÂ¡  */
#define LOG_FUN_INFO_SIZE_MAX	(100)/*    ÃÃÃÂ¾ÃÂ±Â¼Ã¤ÂµÃÂ´Ã³ÃÂ¡  */

#define LOG_FILE_SIZE_MAX	(64*1024) /*    64KB  */


#ifdef LINUX_OS

typedef enum
{
	LOG_OUT_PUT_CONSOLE=0,/* Â´Â°Â¿ÃÃÃ¤Â³Ã¶   */
	LOG_OUT_PUT_DISK,
	LOG_OUT_PUT_FLASH,
	LOG_OUT_PUT_SOCKET,/* ÃÃÂ½ÃÃÃÃÃÂ¼Ã¾ÃÃ¤Â³Ã¶,ÃÂµÂ¼ÃÃÃÃÃ¤Â³Ã¶ÂµÂ½disk or flashÃÃÂ£Â¬ÂºÃ³ÃÃ¸Â¿ÃÂ¸Ã¹Â¾ÃÃÃ¨ÃÂªÂ¶ÃÂ´ÃÂ½Ã¸ÃÃÃÂ©ÃÂ¹ */
/*  ÃÃ§Â¹Ã»ÃÃ¤Â³Ã¶ÃÃ ÃÃÃÃÃÃÂ½ÃÃÃÂ£Â¬ÃÂ¬ÃÃÃÃÃÂ´ÃÃ«ÂµÂ½flashÃÃ,ÃÃ´Â´ÃÃÃÂ²Â»Â´Ã¦ÃÃÂ£Â¬ÃÃ²ÃÂ²ÃÃÃÂ´ÃÃ«ÂµÂ½flashÃÃÂ£Â¬ecosÃÂ¬Â´Ã  */	
	LOG_OUT_PUT_NULL,/*  Â²Â»ÃÃ¶ÃÃÂºÃÂ´Â¦ÃÃ­  */
}LogOutPutType;
#else

typedef enum
{
	LOG_OUT_PUT_CONSOLE=0,
	LOG_OUT_PUT_DISK,
	LOG_OUT_PUT_FLASH,
	LOG_OUT_PUT_NULL,/*  Â²Â»ÃÃ¶ÃÃÂºÃÂ´Â¦ÃÃ­  */
}LogOutPutType;
#endif


typedef enum
{
        LOG_DEBUG=0,/* ÃÂ»Â°Ã£ÃÃÂµÃ·ÃÃÂ¼Â¶Â±Ã°   */
        LOG_WARN,/* Â¾Â¯Â¸Ã¦Â¼Â¶Â±Ã°,  */
        LOG_INFO,/* ÃÃ®ÃÃÃÃÂ»Â§Â¿ÃÃÃÂ¿Â´ÂµÂ½ÂµÃÃÃÃÂ¢Â¼Â¶Â±Ã°  */
        LOG_ERROR,
        LOG_FATAL,
        LOG_MAX,
}log_level_e;

typedef struct
{
	log_level_e    e_level;//
	char        s_level[10];
}log_level_map_s;



#define SKY_TAB_NAME_MAX   (50)/* tab Â±ÃªÃÂ©ÂµÃÃÃ®Â´Ã³ÃÂµ  */
typedef struct
{
	log_level_e    e_level;//
	char        s_filename[LOG_FILE_NAME_LEN_MAX];//Â´Ã¦Â´Â¢ÃÃÂ¼Ã¾ÃÃ»(Â³ÃÃÃ²ÃÂ±Â¿ÃÃÃÂ¶Â¨ÃÃ¥ÃÃÃÂ¾Â´Ã¦Â´Â¢ÃÃÂ¼Ã¾ÃÃ»)
	char        s_time[LOG_TIME_SIZE_MAX];//Â¼ÃÃÂ¼ÂµÃÃÂ±Â¼Ã¤(ÃÂ±ÃÃ¸Â¸Ã¹Â¾ÃÂ°Ã¥ÃÃÂµÃÃÃ¨ÃÃ)
	char        s_funinfo[LOG_FUN_INFO_SIZE_MAX];//file fun line
	char        s_buffer[LOG_BUFFER_SIZE_MAX];
}log_record_s;//ä¸­ææ¾ç¤º

typedef enum
{/* ÃÃÃÂ¾ÃÂ¬ÃÃÃÃÂ´Ã¦Â´Â¢ÃÃramfsÃÃÂ£Â¬ÂµÂ±ÃÂµÃÂ³Â·Â¢ÃÃÃÃ ÃÂ¦ÃÃÂºÃÃÂ±Â»Ã²Â·Â¢ÃÃSAVE_FLASHÂ¿ÃÃÃÃÃÃÂ±Â²ÃÂ±Â£Â´Ã¦ÂµÂ½flashÃÃ  */
        LOG_CONTROL_SAVE_DISK=1,/* ÃÃÃÂ¾Â±Â£Â´Ã¦ÃÂ«ÃÂ·Â¾Â¶Â£Â¬Â¿ÃÃÃÃÂ¸Â¶Â¨ÂµÂ½Â¾ÃÃÃ¥ÃÂ³Â¸Ã¶ÃÃÃÃÂ£Â¬Â´ÃÃÂ±ÃÃ´Â¼Ã¬Â²Ã¢Â²Â»ÂµÂ½ÃÃÃÃÃÂ± Â£Â¬ÃÃ²ÃÃÂ¶Â¯Â±Â£Â´Ã¦ÂµÂ½ÃÂ­ÃÂ´ramfsÃÃ  */
        LOG_CONTROL_SAVE_FLASH=2,/* Â½Â«ramfsÃÃÂµÃÃÃÃÂ¾ÃÂ¢Â¼Â´Â±Â£Â´Ã¦ÂµÂ½flashÃÃÂ£Â¬ÃÃ´ÃÃÂ¾Â­ÃÃÂ¶Â¨ÂµÂ½Â±Â£Â´Ã¦ÂµÂ½ÃÃÃÃÂ£Â¬ÃÃ²Â´ÃÂ²ÃÃÃ·ÃÃÃÂ§  */
}LogControlType;/*  Â¿ÃÃÃÃÃÃÃ ÃÃ   */

typedef struct
{
    LogControlType    eControlType;//ÃÃ ÂµÂ±ÃÃkey
    char        sControlValue[50];//ÃÃ ÂµÂ±ÃÃvalue
}LogControlRecord;/* Â¿ÃÃÃÃÃÂ¼ÃÃÂ¼  */

typedef struct
{
	char sModuleName[LOG_MODULE_NAME_LEN_MAX];//ÃÂ£Â¿Ã©ÂµÃÃÃ»ÃÃ(Â´Ã³ÃÂ¡ÃÂ´ignore)
	log_level_e    eCondition;//ÃÃÂ·Ã±ÃÃ¤Â³Ã¶ÃÃµÂ¼Ã¾ÂµÃÂ¼Â¶ 
}LogModuleType;

__BEGIN_DECLS

//int LogHandle(log_level_e e_condition,log_level_e e_level,const char *pc_file,const char *pc_fun,int n_line,char *pc_fmt, ...);



//#define NCLOG(level_condition,level,args...) NcLogHandle(level_condition,level,__FILE__,__FUNCTION__,__LINE__,args)

//extern void comm_log(const char * pcModuleName,log_level_e level,char *args,...);
int LogHandle(const char * pcModuleName,log_level_e e_level,const char *pc_file,const char *pc_fun,int n_line,const char *pc_fmt, ...);
#define COMM_LOG(pcModuleName,level,args...) LogHandle(pcModuleName,level,__FILE__,__FUNCTION__,__LINE__,args)
void CommLogSetModule(const char *pcModuleName,log_level_e  eConditon);
void CommLogSetOutPutType(LogOutPutType eOutPutType);
void CommLogSetDefaultCondition(log_level_e eConditon);

__END_DECLS
#endif 

