#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <algorithm>
#include "log.h"

#ifndef OS_WIN_LOG
#ifndef OS_LINUX_LOG
//#define  OS_WIN_LOG		//在WIN平台下编译
#ifndef OS_WIN_LOG
#define  OS_LINUX_LOG      //在Linux平台下编译
#endif
#endif
#endif

#ifdef OS_LINUX_LOG
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <dirent.h>
#include <fstream>
/*******************************************************************************
 * Function     : GetAnsiCodeByType
 * Input        :
 * Return       :
 * Description  : Get the Ansi Code type
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
string GetAnsiCodeByType(int type)
{
    string color;
    switch (type)
    {
        case LOGERROR: //閿欒淇℃伅锛氶珮浜�佸墠鏅櫧鑹层�佽儗鏅孩鑹�
            color = ANSI("1;37;41"); //HLIGHT()FGROUND(WHITE)BGROUND(RED);
            break;
        case LOGWARN: //璀﹀憡淇℃伅锛氶珮浜�佸墠鏅粍鑹�
            color = ANSI("1;33"); //HLIGHT()FGROUND(YELLOW);
            break;
        case LOGINFO: //涓氬姟淇℃伅锛氬叧闂墍鏈夊睘鎬э紝鎭㈠榛樿
            color = NONE();
            break;
        case LOGDEBUG: //璋冭瘯淇℃伅锛氶珮浜�佸墠鏅繁缁胯壊
            color = ANSI("1;36"); //HLIGHT()FGROUND(CYAN);
            break;
        case LOGTRACE: //杩借釜淇℃伅锛氶珮浜�佸墠鏅传鑹�
            color = ANSI("1;35"); //HLIGHT()FGROUND(PURPLE);
            break;
        default: //鍏抽棴鎵�鏈夊睘鎬э紝鎭㈠榛樿
            color = NONE();
            break;
    }
    return color;
}


/*******************************************************************************
 * Function     : get_str_time
 * Input        :
 * Return       :
 * Description  : 鑾峰彇鏍囧噯鏃堕棿瀛楃涓�
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
string get_str_time(time_t bte = 0)
{
    time_t tne = 0;
    char tmp[64] = { 0 };

    tne = (0 < bte) ? (bte) : (time(0));
    strftime(tmp, sizeof(tmp), "%H:%M:%S", localtime(&tne));
    return string(tmp);
}

#define DATETIME "%04d%02d%02d-%02d:%02d:%02d"

Log* Log::pLog = NULL;
//鍙�氳繃鎵嬪姩杈撳叆绋嬪簭鍚姩鍙傛暟鎵撳紑鏈湴鏂囦欢鏃ュ織
static bool tofile = false; //榛樿涓嶈褰曞埌鏈湴鏂囦欢
static bool toterminal = true; //
const unsigned int LOG_PATH_LEN = 1024 * 2;
const unsigned int LOG_FILE_NAME_LEN = 510;

//Log鏋勯�犲嚱鏁�
Log::Log(const char* logname)
{
    pthread_mutex_init(&this->pMtx, 0);
    this->name = (char*) malloc(64);
    memset(this->name, 0, 64);
    strcpy(this->name, logname);

    this->dir = (char*) malloc(1024);
    this->path = (char*) malloc(1024);
    this->strDateTime = (char*) malloc(sizeof(DATETIME));
    this->modelDir = (char*) malloc(1024);

    memset(this->dir, 0, 1024);
    memset(this->path, 0, 1024);
    memset(this->modelDir, 0, 1024);
    memset(this->strDateTime, 0, sizeof(DATETIME));
    memset(&this->filetime, 0, sizeof(this->filetime));

    this->level = LOGINFO;
    this->thLogPid = -1;
    this->fileSize = -1;
    pthread_mutex_init(&this->pMtx,NULL);//=PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_init(&this->pCond,NULL);//= PTHREAD_COND_INITIALIZER;
}
/*******************************************************************************
 * Function     : ~Log
 * Input        :
 * Return       :
 * Description  : Log鏋愭瀯鍑芥暟
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
Log::~Log()
{
    if ( NULL != this->name)
    {
        free(this->name);
        this->name = NULL;
    }
    if ( NULL != this->dir)
    {
        free(this->dir);
        this->dir = NULL;
    }
    if ( NULL != this->path)
    {
        free(this->path);
        this->path = NULL;
    }
    if ( NULL != this->strDateTime)
    {
        free(this->strDateTime);
        this->strDateTime = NULL;
    }
    if ( NULL != this->modelDir)
    {
        free(this->modelDir);
        this->modelDir = NULL;
    }
    pthread_mutex_destroy(&this->pMtx);
    pthread_cond_destroy(&this->pCond);
}
/*******************************************************************************
 * Function     : Log::GetInstance
 * Input        :
 * Return       :
 * Description  : 鏃ュ織绫诲崟渚嬪疄渚嬭幏鍙�
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
Log* Log::GetInstance()
{
    if (NULL == Log::pLog)
    {
        Log::pLog = new Log("");
    }
    return Log::pLog;
}

/*******************************************************************************
 * Function     : Log::Level_Set
 * Input        :
 * Return       :
 * Description  : set the log type and set the log where to
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
void Log::Level_Set(int argc, char* argv[])
{
//	typedef struct{
//		struct option opt;
//		const char *commit;
//	}new_option;
//
//	new_option argv_options[] = {
//			{{"log", 1, NULL, 0},"设置日志级别"},//--log info
//	        {{"help",  0, NULL, 'h' }, "打印帮助"},//--help / -h
//	        {{0, 0, 0, 0 }, 0}
//	};

    const char TYPENUM = 6;
    const char TONUM = 3;
    string loghead("log:");
    string logtype[TYPENUM] = { "error", "warn", "info", "debug", "trace", "file" };
    string logto[TONUM] = { "tofile", "toterminal", "tonull" };
    string argvs;

    strcpy(this->name, basename(argv[0])); //璁剧疆鏃ュ織鍚�
    snprintf(this->modelDir, 1024, "%s/%s", LOGROOTPATH, this->name); //妯″潡鏃ュ織鎵�鍦ㄧ洰褰�

    for (int i = 0; i < argc; i++)
    {
        argvs = argv[i];
        transform(argvs.begin(), argvs.end(), argvs.begin(), ::tolower);
        if (0 != argvs.compare(0, loghead.length(), loghead))
        {
        	continue;
        }
		//find the log type
		for (int j = 0; j < TYPENUM; j++)
		{
			string typestr = ":" + logtype[j];
			if (argvs.find(typestr) != string::npos)
			{
				level = j;
				tofile = (level != TYPENUM - 1)?(true):(false);
				toterminal = true;
				CreatePthread();
				break;
			}
		}
		//find the log where to output
		for (int j = 0; j < TONUM; j++)
		{
			string tostr = ":" + logto[j];
			if (argvs.find(tostr) != string::npos)
			{
				tofile = false;
				toterminal = false;
				tofile = (j == 0)?(true):(false);
				toterminal = (j == 1)?(true):(false);
			}
		}
		break;
    }
}
/*******************************************************************************
 * Function     : create_multi_dir
 * Input        :
 * Return       :
 * Description  : 閫掑綊鍒涘缓鐩綍
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
int Create_multi_dir(const char *path)
{
    if ( NULL == path)
    {
        return -1;
    }
    int len = 0;
    len = strlen(path);
    char *dir_path = (char*) malloc(len + 2);

    dir_path[len] = '\0';
    strncpy(dir_path, path, len);
    if ('/' != dir_path[len] && '\0' == dir_path[len])
    {
        dir_path[len] = '/';
        dir_path[len + 1] = '\0';
        len += 1;
    }
    for (int i = 0; i < len; i++)
    {
        if (dir_path[i] == '/' && i > 0)
        {
            dir_path[i] = '\0';
            if (access(dir_path, F_OK) < 0)
            {
                if (mkdir(dir_path, 0755) < 0)
                {
                    free(dir_path);
                    return -1;
                }
            }
            dir_path[i] = '/';
        }
    }
    free(dir_path);
    dir_path = NULL;
    return 0;
}
/*******************************************************************************
 * Function     : ReDeleteFile
 * Input        :
 * Return       :
 * Description  : delete the file from the fold
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
bool ReDeleteFile(char *path, bool bDelTopDir)
{
    if (NULL == path)
        return false;
    DIR *dir = NULL;
    struct dirent *s_dir;
    struct stat file_stat;
    char curfile[LOG_PATH_LEN + LOG_FILE_NAME_LEN] = { 0 };
    char strPath[LOG_PATH_LEN] = { 0 };

    snprintf(strPath, sizeof(strPath), "%s", path);
    int len = strlen(path);
    if (strPath[len - 1] != '/')
    {
        strPath[len] = '/';
        strPath[len + 1] = 0;
    }
    if ((dir = opendir(strPath)) == NULL)   //PWD List
    {
        perror("opendir()");
        return -1;
    }
    while ((s_dir = readdir(dir)) != NULL)
    {
        if ((strcmp(s_dir->d_name, ".") == 0) || (strcmp(s_dir->d_name, "..") == 0))
            continue;
        sprintf(curfile, "%s%s", strPath, s_dir->d_name);
        stat(curfile, &file_stat);
        if (S_ISDIR(file_stat.st_mode))      //濡傛灉鏄洰褰�
        {
            ReDeleteFile(curfile, bDelTopDir);	//閫掑綊鍒犻櫎鐩綍涓嬬殑鏂囦欢
            remove(curfile);					//鐩綍娓呯┖鍚庯紝鍒犻櫎璇ョ洰褰�
        }
        else					//鏄枃浠�
        {
            remove(curfile);	//鍒犻櫎鏂囦欢
        }
    }
    closedir(dir);
    if (bDelTopDir)	//鏈�缁堢洰褰曡娓呯┖浜嗭紝鍒犻櫎璇ョ洰褰�
    {
        remove(path);	//鍒犻櫎鏈�椤跺眰鐨勭洰褰�
    }
    return true;
}

/*******************************************************************************
 * Function     : clear_log_before_nday
 * Input        :
 * Return       :
 * Description  : 鍒犻櫎nday澶╁墠鐨勬棩蹇楁枃浠�
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
int clear_log_before_nday(const char*path, struct tm nowtime, unsigned int nday)
{
    char logPath[2 * 1024] = { 0 };
    unsigned int keyday = 0;
    int year = nowtime.tm_year + 1900;
    int mon = nowtime.tm_mon + 1;
    int day = nowtime.tm_mday;

    snprintf(logPath, sizeof(logPath), "%s/", path);
    if (access(logPath, F_OK) != 0)
        return -1;
    for (int y = year; y >= 2012; --y)
    {
        memset(logPath, 0, sizeof(logPath));
        snprintf(logPath, sizeof(logPath), "%s/%04d/", path, y);
        if (access(logPath, F_OK) != 0)
            continue;
        if (keyday >= nday)
        {
            ReDeleteFile(logPath, 1);
        }
        for (int m = mon; m > 0; --m)
        {
            memset(logPath, 0, sizeof(logPath));
            snprintf(logPath, sizeof(logPath), "%s/%04d/%02d/", path, y, m);
            if (access(logPath, F_OK) != 0)
                continue;
            if (keyday >= nday)
            {
                ReDeleteFile(logPath, 1);
            }
            for (int d = day; d > 0; --d)
            {
                memset(logPath, 0, sizeof(logPath));
                snprintf(logPath, sizeof(logPath), "%s/%04d/%02d/%02d/", path, y, m, d);
                if (access(logPath, F_OK) != 0)
                    continue;
                if (keyday < nday)
                {
                    keyday += 1;
                    continue;
                }
                ReDeleteFile(logPath, 1);
            }
            day = (day == nowtime.tm_mday)?(31):(day);
        }
        mon = (mon == nowtime.tm_mon + 1)?(12):(mon);
    }
    return 0;
}

/*******************************************************************************
 * Function     : Symlink
 * Input        :
 * Return       :
 * Description  : TODO
 * Created on   : 2018骞�2鏈�13鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
int Log::loglink_update(void)
{
    string oldpath = this->path;
    string linklog(LOGROOTPATH);	// = this->modelDir;

    linklog += "/";
    linklog += this->name;
    linklog += ".log";

    unlink(linklog.c_str());
    return symlink(oldpath.c_str(), linklog.c_str());
}

/*******************************************************************************
 * Function     : Log::Handle
 * Input        :
 * Return       :
 * Description  : 鏃ュ織鍒嗘枃浠跺ぇ灏忔帶鍒�
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
bool Log::Handle(void)
{
    struct timeb tb;
    struct tm *nowtime;
    ftime(&tb);
    nowtime = localtime(&tb.time);

    //璺ㄥぉ鏃讹紝闇�瑕侀噸鏂扮敓鎴愬伐浣滅洰褰�
    if ((this->fileSize == -1) || (this->filetime.tm_hour != nowtime->tm_hour && nowtime->tm_min == 0))
    {
        UpdateLogFileDateTime(nowtime);
        EndCurLogFile();
        //闇�瑕佽缃笅涓�涓棩蹇楁枃浠跺睘鎬�
        SetNextLogFileAttrib(nowtime);
        CreateFile(this->path);
        loglink_update();

        clear_log_before_nday(this->modelDir, this->filetime, LOG_KEEP_NDAY);

        this->fileSize = 0;
    }
    CheckLogPath();
    return true;
}

/*******************************************************************************
 * Function     : sig_handler
 * Input        :
 * Return       :
 * Description  : 淇″彿澶勭悊鍑芥暟
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
//static void sig_handler( int sig )
//{
//	if( SIGKILL == sig )
//	{
//		DEBUG_LOG("static void sig_handler( int sig )");
//		pthread_exit(NULL);
//	}
//}
/*******************************************************************************
 * Function     : LogProc
 * Input        :
 * Return       :
 * Description  :
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
void* LogProc(void *p)
{
    Log *plog = (Log*) p;
    list<LogMsg_st>::iterator it;
    pthread_mutex_lock(&plog->pMtx);

    while (1)
    {
        while (plog->logList.size() == 0)
        {
            pthread_cond_wait(&plog->pCond,&plog->pMtx);
        }
		if (false == plog->Handle())
		{
			continue;
		}
		it = plog->logList.begin();
		for (int i = 0; plog->logList.end() != it; it = plog->logList.begin())
		{
			plog->Write((*it).str);		//鍐欏埌鏈湴鏃ュ織鏂囦欢
			plog->logList.pop_front();
			if (++i == 10)
			{
				break;
			}
		}
    }
    pthread_mutex_unlock(&plog->pMtx);//临界区数据操作完毕，释放互斥锁
    return NULL;
}
/*******************************************************************************
 * Function     : CreatePthread
 * Input        :
 * Return       :
 * Description  : create the thread for log grogram
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
void Log::CreatePthread(void)
{
    pthread_attr_t attr;
    sched_param param;
    int newprio = 90;
    pthread_attr_init(&attr);
    pthread_attr_getschedparam(&attr, &param);
    param.sched_priority = newprio;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&this->thLogPid, &attr, LogProc, (void*) this);
}

/*******************************************************************************
 * Function     : Log::Push
 * Input        :
 * Return       :
 * Description  : 鏃ュ織鏍煎紡鍖栧叆鍙傞噸缁勫苟鍘嬪叆list
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
void Log::Push(int logtype, const char* logmsg, ...)
{
    if (NULL == logmsg)
        return;
    if (!toterminal && !tofile && logtype != LOGFILE)
        return;

    char _logmsg[LOGMSGLEN] = { 0 };
    LogMsg_st LogBuf;
    memset(LogBuf.str, 0, sizeof(LogBuf.str));

    va_list args;
    va_start(args, logmsg);
    vsprintf(_logmsg, logmsg, args);			//灏嗘棩蹇楀唴瀹逛粠鍙傛暟涓嬁鍑烘潵
    va_end(args);
    //鎺у埗鍙版棩蹇椾笉鏀惧湪绾跨▼涓紝
    snprintf(LogBuf.str, sizeof(LogBuf.str), "%s"
            "[%s] %s."
            "%s", GetAnsiCodeByType(logtype).c_str(),	//鏍规嵁鏃ュ織绾у埆璁剧疆ANSI灞炴��
            get_str_time(0).c_str(), _logmsg,
            NONE()								//鎭㈠ANSI灞炴��
            );
    if (toterminal)
    {
        puts(LogBuf.str);           //puts杈撳嚭瀛楃涓插苟鎹㈣
    }
    if (tofile || logtype == LOGFILE)
    {
        pthread_mutex_lock(&this->pMtx);
        this->logList.push_back(LogBuf);
        pthread_cond_signal(&this->pCond);
        pthread_mutex_unlock(&this->pMtx);
    }
}
/*******************************************************************************
 * Function     : Log::CreateFile
 * Input        :
 * Return       :
 * Description  :
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
bool Log::CreateFile(string filename)
{
    fstream file;
    file.open(filename.c_str(), ios::out);
    if (!file)
        return false;
    file.close();
    return true;
}

/*******************************************************************************
 * Function     : Log::Write
 * Input        :
 * Return       :
 * Description  : 鏃ュ織鏁版嵁鏍煎紡鍖栧啓鍏ユ枃浠�
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
void Log::Write(string logstr)
{
    if ("" == logstr)
    {
        return;
    }
    FILE* flog = fopen(this->path, "at+");
    if ( NULL == flog)
    {
        return;
    }
    fputs(logstr.c_str(), flog);
    fputs("\r\n", flog);           //fputs涓嶄細鑷姩鎹㈣
    fflush(flog);
    fclose(flog);
    this->fileSize += logstr.size();
}

/*******************************************************************************
 * Function     : Log::EndCurLogFile
 * Input        :
 * Return       :
 * Description  : 缁撴潫褰撳墠鏃ュ織锛堜负鏃ュ織鏂囦欢鏃堕棿娈靛鍔犵粨鏉熸椂闂寸偣锛�
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
bool Log::EndCurLogFile(void)
{
    char logdir[1024] = { 0 };
    if (access(this->path, F_OK) != 0)
        return false;
    sprintf(logdir, "%s~%02d:%02d:%02d", this->path, this->filetime.tm_hour, this->filetime.tm_min, this->filetime.tm_sec);
    if (0 != rename(this->path, logdir))           //寰�褰撳墠鏃ュ織鏂囦欢鍚嶆渶鍚庡鍔犵粨鏉熸椂闂寸偣
        return false;
    return true;
}

/*******************************************************************************
 * Function     : Log::UpdateLogFileDateTime
 * Input        :
 * Return       :
 * Description  : 鏇存柊鏃堕棿锛岀敤鏉ョ粨鏉熷拰鍒涘缓鏃ュ織鏂囦欢
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
void Log::UpdateLogFileDateTime(struct tm *nowtime)
{
    memcpy(&this->filetime, nowtime, sizeof(filetime));
    memset(this->strDateTime, 0, sizeof(DATETIME));
    sprintf(this->strDateTime, "%04d%02d%02d-%02d:%02d:%02d", this->filetime.tm_year + 1900, this->filetime.tm_mon + 1,
            this->filetime.tm_mday, this->filetime.tm_hour, this->filetime.tm_min, this->filetime.tm_sec);
}

/*******************************************************************************
 * Function     : Log::SetNextLogFileAttrib
 * Input        :
 * Return       :
 * Description  : 鏇存柊鏃ュ織璺緞锛堝寘鎷枃浠跺悕锛�
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
void Log::SetNextLogFileAttrib(struct tm *nowtime)
{
    memset(this->dir, 0, 1024);
    memset(this->path, 0, 1024);

    sprintf(this->dir, "%s/%s/%04d/%02d/%02d/", LOGROOTPATH, this->name, nowtime->tm_year + 1900, nowtime->tm_mon + 1,
            nowtime->tm_mday);
    sprintf(this->path, "%s%s.log.%s", this->dir, this->name, this->strDateTime);
}

/*******************************************************************************
 * Function     : Log::CheckLogPath
 * Input        :
 * Return       :
 * Description  : 妫�娴嬫棩蹇楃洰褰曠殑瀹屾暣鎬�
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
bool Log::CheckLogPath()
{
    if (this->dir == NULL)
        return false;
    if (0 != access(this->dir, 0))
    {
        if (0 != Create_multi_dir(this->dir))
        {
            return false;
        }
    }
    return true;
}

/*******************************************************************************
 * Function     : Log::Wait
 * Input        :
 * Return       :
 * Description  : 绛夊緟鏃ュ織瀹屾垚璁板綍
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
void Log::Wait()
{
    while (0 < this->logList.size())
    {
        usleep(1000);
    }
    //pthread_kill(thlogPid,SIGKILL);
}
#else/*OS_Win*/

#include <Windows.h>
#include <time.h>

int level = LOGINFO;
/*******************************************************************************
 * Function     : Log::Level_Set
 * Input        :
 * Return       :
 * Description  : set the log type and set the log where to
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
void Level_Set(int argc, char* argv[])
{
    const char TYPENUM = 6;
    const char TONUM = 3;
    string loghead("log:");
    string logtype[TYPENUM] = { "error", "warn", "info", "debug", "trace", "file" };
    string logto[TONUM] = { "tofile", "toterminal", "tonull" };
    string argvs;

    for (int i = 0; i < argc; i++)
    {
        argvs = argv[i];
        transform(argvs.begin(), argvs.end(), argvs.begin(), ::tolower);
        if (0 != argvs.compare(0, loghead.length(), loghead))
        	continue;
		//find the log type
		for (int j = 0; j < TYPENUM; j++)
		{
			string typestr = ":";
			typestr.append(logtype[j]);
			if (argvs.find(typestr) != string::npos)
			{
				level = j;
				break;
			}
		}
		break;
    }
}
/*******************************************************************************
 * Function     : Log::Push
 * Input        :
 * Return       :
 * Description  : 鏃ュ織鏍煎紡鍖栧叆鍙傞噸缁勫苟鍘嬪叆list
 * Created on   : 2018骞�2鏈�11鏃�
 * Author       : luzxiang
 * Notes        : --
 *******************************************************************************/
void Push(int logtype, const char* logmsg, ...)
{
    if (NULL == logmsg)
        return;
    char _logmsg[LOGMSGLEN] = { 0 };
    LogMsg_st LogBuf;
    memset(LogBuf.str, 0, sizeof(LogBuf.str));

    va_list args;
    va_start(args, logmsg);
    vsprintf_s(_logmsg, logmsg, args);			//灏嗘棩蹇楀唴瀹逛粠鍙傛暟涓嬁鍑烘潵
    va_end(args);

	 time_t the_time;
     time(&the_time);

	 char strtime[50]={0};
	 tm* t_tm = localtime(&the_time);
	 _snprintf_s(strtime,sizeof(strtime),"%02d:%02d:%02d",t_tm->tm_hour,t_tm->tm_min,t_tm->tm_sec);

    _snprintf_s(LogBuf.str, sizeof(LogBuf.str),
            "[%s] %s.\n",
            strtime, _logmsg								//鎭㈠ANSI灞炴��
            );

	puts(LogBuf.str);
	//std::cout<<get_str_time(0).c_str()<<_logmsg<<endl;
}

#endif
