/*
 * Socket.h
 *
 *  Created on: 2018年8月29日
 *      Author: luzxiang
 */

#ifndef SOCKET_ISOCKET_H_
#define SOCKET_ISOCKET_H_
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h> 
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h> 
#include <netinet/ip.h> 
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <unistd.h>

#include <algorithm>  
#include "../FIFO/FIFO.h"
#include "../log/log.h"

using namespace std::placeholders;
#include <chrono>
#include <thread>
#define SLEEP_FOR 	std::this_thread::sleep_for
#define SLEEP_NS(x) SLEEP_FOR(std::chrono::nanoseconds((int64_t)(x)))//ns
#define SLEEP_US(x) SLEEP_FOR(std::chrono::microseconds((int64_t)(x)))//us
#define SLEEP_MS(x) SLEEP_FOR(std::chrono::milliseconds((int64_t)(x)))//ms
#define SLEEP_S(x) 	SLEEP_FOR(std::chrono::seconds((int64_t)(x)))//seconds

#define IP_LEN_ 				(48)
#define RW_SOCKT_BUF_MINLEN_	(4*1024*1024U)
#define RW_SOCKT_BUF_MAXLEN_	(6*1024*1024U)
#define RW_SOCKT_BUF_LEN_		(RW_SOCKT_BUF_MINLEN_ + 1024)//为了能把双向同步时的任务请求包(大约4M)发过去，这里需要再加1024
#define R_FIFO_BUF_LEN_			(16*1024*1024U)//这里必须是2的幂次方，因为用到了重构的fifo
#define W_FIFO_BUF_LEN_			(16*1024*1024U)//这里必须是2的幂次方，因为用到了重构的fifo

class ISocket{
protected:
    int fd;
	char *wbuffer;//从FIFO取出数据到这里，然后从这里发到Socket
	char *rbuffer;//从Socket取出数据到这里，然后从这里给到FIFO
	int pfds[2];//guandao fd
    sockt_fifo_st* wFifo;
    sockt_fifo_st* rFifo;
	std::thread SktThread;  	//发送数据线程
    bool SktThdIsStart;
    bool IsConnected;         	//<连接状态
    unsigned int lastAlivedTime;
    char Ip[IP_LEN_];
    unsigned int Port;
    struct sockaddr_in SvrAddr;//IPV4socket 地址信息

	std::function<int(int , char *, size_t , int )> Read;
	std::function<int(int , const char *, size_t , int )> Send;

	ISocket(void);
	virtual int Selecting(void);
	virtual int Writing(void);
	virtual int Reading(void);
	virtual void Close(void);
    virtual int Connect(void);
    virtual void SetSockOpt(void);
	int OnReceive(char *buf, unsigned int len);

	unsigned int wGet(char *buf, unsigned int len);
	unsigned int wGet(char *buf, unsigned int len, unsigned int tout_s);
	unsigned int wGet(char *buf, unsigned int len, unsigned int tout_s, unsigned int tout_msec);

	unsigned int rPut(const char *buf, unsigned int len);
	unsigned int rPut(const char *buf, unsigned int len, unsigned int tout_s);
	unsigned int rPut(const char *buf, unsigned int len, unsigned int tout_s, unsigned int tout_msec);
private:
	unsigned int Get(sockt_fifo_st *fifo,char *buf, unsigned int len, unsigned int tout_s, long int tout_msec);
	virtual unsigned int Put(sockt_fifo_st *fifo,const char *buf, unsigned int len, unsigned int tout_s, long int tout_ms);
public:
	ISocket(char*ip, int port);
	virtual ~ISocket(void);
	void Release(void);
	int GetFd(void);
	void Stop(void);
	bool Start(void);
    int ReConnect(void);
	bool IsAlived(void);
	unsigned int GetWBufLen();
	unsigned int GetRBufLen();
	unsigned int GetLastAlivedTime(void);

	unsigned int rGet(char *buf, unsigned int len);
	unsigned int rGet(char *buf, unsigned int len, unsigned int tout_s);
	unsigned int rGet(char *buf, unsigned int len, unsigned int tout_s, unsigned int tout_msec);

	unsigned int wPut(const char *buf, unsigned int len);
	unsigned int wPut(const char *buf, unsigned int len, unsigned int tout_s);
	unsigned int wPut(const char *buf, unsigned int len, unsigned int tout_s, unsigned int tout_msec);

};


#endif /* SOCKET_SOCKET_H_ */
