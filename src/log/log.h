/*
 * log.h
 *
 *  Created on: 2017-11-17
 *      Author: luzxiang
 */

#ifndef LOG_H_
#define LOG_H_

#ifndef OS_WIN_LOG
#ifndef OS_LINUX_LOG

//#define  OS_WIN_LOG		//在WIN平台下编译

#ifndef OS_WIN_LOG
#define  OS_LINUX_LOG      //在Linux平台下编译
#endif

#endif
#endif

/* 
 * 在main函数的第一句调用LOG_FILE_EN(argc,argv)
 * 不带任何日志参数启动程序时，默认启用Info日志级别，并只将日志打印到terminal
 * 
 * log:error   启用error级别的日志，并保存到文件
 * log:warn    启用error、warn级别的日志，并启用文件记录
 * log:info    启用error、warn、info级别的日志，并启用文件记录
 * log:debug   启用error、warn、info、debug级别的日志，并启用文件记录
 * log:trace   启用error、warn、info、debug、trace级别的日志，并启用文件记录
 * log:file    启用error、warn、info、debug、trace、file级别的日志，但只将file级别的日志保存到文件
 *
 * log:xxx:toterminal	只将小于等于xxx级别的日志打印到terminal（默认）
 * log:xxx:tofile		只将小于等于xxx级别的日志保存到文件
 * log:xxx:tonull 		不记录也不显示任何日志
 */

#include <string.h>
#include <iostream> 
using namespace std;
/*日志的存放的根目录*/
#ifdef OS_LINUX_LOG
#include "../log/color.h"
#define LOGROOTPATH "/var/log/"
/*只保留日志的天数，N天前的将被清理*/
#define LOG_KEEP_NDAY (3)
#else //OS_WIN
#define LOGROOTPATH "D:\\log\\"
#endif

#define LOGERROR   (0) 				//系统发生了严重的错误, 必须马上进行处理, 否则系统将无法继续运行.
#define LOGWARN    (LOGERROR + 1) 	//系统能继续运行, 但是必须引起关注.
#define LOGINFO    (LOGWARN + 1) 	//重要的业务逻辑处理完成.
#define LOGDEBUG   (LOGINFO + 1)    //主要给开发人员看.
#define LOGTRACE   (LOGDEBUG + 1)   //系统详细信息, 主要给开发人员用.
#define LOGFILE    (LOGTRACE + 1)   //强制<单独>将日志信息写入文件.

#define LOGUSLEEP  	(100)			//日志的刷新频率
#define LOGMSGLEN	(2*1024)       //单条日志信息的最大字节
#define LOGBUFLEN 	(LOGMSGLEN+100) //每条日志信息+时间的最大字节

#define LOGFILESIZE	(100*1024*1024)     //单个日志文件的最大字节

typedef struct
{
    int type;
    char str[LOGBUFLEN];
} LogMsg_st;


#ifdef OS_WIN_LOG
extern int level;
extern void Level_Set(int argc, char* argv[]);
extern void Push(int logtype, const char* logmsg, ...);

#define BASENAME(path) (string(path).substr(string(path).find_last_of('\\')+1)).c_str()

#define LOG_LEVEL_SET(argc,argv)do{\
		Level_Set((argc),(argv));\
	}while(0)
/* *****************************************
 * ***   对日志的不同类型的功能进行定义      ***
 * *****************************************/

#define LOG_ERROR(fmt,...) do{\
        if(level >= LOGERROR)\
            Push(LOGERROR,"%s|%d|%s|ERROR| " fmt,BASENAME(__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__);\
    }while(0)

#define LOG_WARN(fmt,...) do{\
        if(level >= LOGWARN)\
            Push(LOGWARN,"%s|%d|%s|WARN| " fmt,BASENAME(__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__);\
        }while(0)

#define LOG_INFO(fmt,...) do{\
        if(level >= LOGINFO)\
            Push(LOGINFO,"%s|%d|%s|INFO| " fmt,BASENAME(__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__);\
    }while(0)

#define LOG_DEBUG(fmt,...) do{\
        if(level >= LOGDEBUG)\
            Push(LOGDEBUG,"%s|%d|%s|DEBUG| " fmt,BASENAME(__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__);\
    }while(0)

#define LOG_WATCH(args) do{\
        if(level >= LOGDEBUG)\
			std::cout<<BASENAME(__FILE__)<<"|"<<(__LINE__)<<"|"<<__FUNCTION__<<"|WATCH "<<#args<<"="<<(args)<<endl;\
    }while(0)

#define LOG_TRACE(fmt,...) do{\
        if(level >= LOGTRACE)\
            Push(LOGTRACE,"%s|%d|%s|TRACE| " fmt,BASENAME(__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__);\
    }while(0)

#else

#include <list>
#include <pthread.h>
#include <sys/timeb.h>
class Log;
extern "C++" const char *basename (const char *__filename);

#define LOG_LEVEL_SET(argc,argv) do{\
		Log::GetInstance()->Level_Set((argc),(argv));\
	}while(0)
/* *****************************************
 * ***   对日志的不同类型的功能进行定义      ***
 * *****************************************/
#define LOG_ERROR(fmt,...) do{\
        if(Log::GetInstance()->level >= LOGERROR)\
            Log::GetInstance()->Push(LOGERROR,"%s|%d|%s|ERROR| " fmt,basename(__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__);\
    }while(0)

#define LOG_WARN(fmt,...) do{\
        if(Log::GetInstance()->level >= LOGWARN)\
            Log::GetInstance()->Push(LOGWARN,"%s|%d|%s|WARN| " fmt,basename(__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__);\
        }while(0)

#define LOG_INFO(fmt,...) do{\
        if(Log::GetInstance()->level >= LOGINFO)\
            Log::GetInstance()->Push(LOGINFO,"%s|%d|%s|INFO| " fmt,basename(__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__);\
    }while(0)

#define LOG_ADD(word) do{\
        if(Log::GetInstance()->level >= LOGINFO){\
			std::cout<<(word);\
			fflush(stdout);\
        }\
    }while(0)

#define LOG_DEBUG(fmt,...) \
	do{\
        if(Log::GetInstance()->level >= LOGDEBUG)\
            Log::GetInstance()->Push(LOGDEBUG,"%s|%d|%s|DEBUG| " fmt,basename(__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__);\
    }while(0)

#define LOG_WATCH(args) do{\
        if(Log::GetInstance()->level >= LOGDEBUG)\
			std::cout<<ANSI("1;36")<<basename(__FILE__)<<"|"<<__LINE__<<"|"<<__FUNCTION__<<"|WATCH "<<#args<<"="<<UNLINE()<<(args)<<NONE()<<endl;\
    }while(0)

#define LOG_TRACE(fmt,...) do{\
        if(Log::GetInstance()->level >= LOGTRACE)\
            Log::GetInstance()->Push(LOGTRACE,"%s|%d|%s|TRACE| " fmt,basename(__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__);\
    }while(0)

#define LOG_FILE(fmt,...) do{\
        if(Log::GetInstance()->level >= LOGFILE)\
            Log::GetInstance()->Push(LOGFILE,"%s|%d|%s|FILE| " fmt,basename(__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__);\
    }while(0)

class Log
{
private:
    static Log* pLog;
    pthread_t thLogPid;           	//日志线程pid
    char* modelDir;          		//模块日志所在目录
    char* strDateTime;          	//日志系统当前时间
    char* name;                 	//当前日志文件名
    char* dir;                  	//当前日志文件所在目录
    char* path;                 	//当前日志文件路径（包括了文件名）
    int fileSize;             	    //记录日志文件大小
    struct tm filetime;			  	//记录当前日志的时间

    Log(const char* logname);
    ~Log();

    void Wait();
    void UpdateLogFileDateTime(struct tm *nowtime);	//更新日志系统日期和时间
    bool EndCurLogFile();         	//结束当前日志文件（添加日志结束时间）
    bool CheckLogPath();         	//确保日志工作目录的安全性
    void SetNextLogFileAttrib(struct tm *nowtime);  	//设置下一个日志文件的属性（文件名、保存目录）
    void CreatePthread(void);
    int loglink_update(void);

public:
    unsigned int level;
    list<LogMsg_st> logList;      	//日志缓存容器
    pthread_mutex_t pMtx;    	//日志线程互斥锁
    pthread_cond_t pCond;    	//日志线程互斥锁
    void Write(string logstr);      //在线程里调用，将缓存容器的日志信息写入日志文件
    bool CreateFile( string filename);
    bool Handle();                  //日志属性的控制（目录安全性、大小限制）
    static Log* GetInstance();              //日志类的单例模式(确保有且仅有一个日志实例)
    void Push(int logtype, const char* logmsg, ...);     //将日志信息压入缓存容器最后
    void Level_Set(int argc, char* argv[]);

protected:
};
#endif/*OS_LINUX*/
#endif /* LOG_H_ */

