/*
 * Socket.h
 *
 *  Created on: 2018年8月29日
 *      Author: luzxiang
 */

#ifndef SOCKET_SOCKET_H_
#define SOCKET_SOCKET_H_
#include <thread>
#include <netinet/in.h>
#include "../fifo/fifo.h"

#define IP_LEN_ 			(48)
#define R_FIFO_BUF_LEN_		(32*1024*1024)//
#define W_FIFO_BUF_LEN_		(32*1024*1024)//

#define RWBUF_LEN_    	    (4*1024*1024)

typedef struct sockt_fifo{
	FIFO *Buf;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
    //数据写入缓冲区
    unsigned int Put(const char *buf, unsigned int len)
    {
    	len = Buf->Put(buf, len);
    	pthread_cond_signal(&this->cv);
    	return len;
    }
    void lock(void)
    {
    	pthread_mutex_lock(&this->mtx);
    }
    unsigned int timewait(unsigned int tout_s)
    {
    	struct timespec tout;
    	memset(&tout, 0, sizeof(tout));
    	tout.tv_nsec = 0;
    	tout.tv_sec = time(0) + tout_s;
    	pthread_cond_timedwait(&this->cv, &this->mtx,&tout);
    	return 0;
    }
    void unlock(void)
    {
    	pthread_mutex_unlock(&this->mtx);
    }
    //从缓冲给读取数据
    unsigned int Get(char *buf, unsigned int len) const
    {
    	return Buf->Get(buf, len);
    }
    unsigned int Free(void) const
    {
    	return Buf->size - Buf->Len();
    }
    bool HaveFree(unsigned int size) const
    {
    	return (Buf->size - Buf->Len() >= size);
    }
    //返回缓冲区中数据长度
    unsigned int Len() const{
    	return Buf->Len();
    }
    bool Empty() const
    {
    	return Buf->Empty();
    }
	sockt_fifo(unsigned int size = 1024)
	{
		Buf = new FIFO(size);
	    mtx = PTHREAD_MUTEX_INITIALIZER;
	    cv = PTHREAD_COND_INITIALIZER;
	}
	sockt_fifo(char *buf, unsigned int size)
	{
		Buf = new FIFO(buf, size);
	    mtx = PTHREAD_MUTEX_INITIALIZER;
	    cv = PTHREAD_COND_INITIALIZER;
	}
	~sockt_fifo()
	{
		if(Buf){
			delete Buf;
		}
	}
}sockt_fifo_st;

class Socket{
private:
    int fd;
	char *wbuf;
	char *rbuf;
    sockt_fifo_st* wFifo;
    sockt_fifo_st* rFifo;
	std::thread rPthread;  //接收数据线程
	std::thread wPthread;  //发送数据线程
    bool wThdIsStart;
    bool rThdIsStart;
    unsigned int lastAlivedTime;
public:
    int Encrypt;
    int IsConnected;         //<连接状态
    char Ip[IP_LEN_];
    unsigned int Port;
    struct sockaddr_in SvrAddr;//IPV4socket 地址信息

private:
	void Stop(void);
    void Close(void);
    int Connect(void);
    void OnWrite(void);
    int ReConnect(void);
    void SetSockOpt(void);
	void OnRead(void);
	int Reading(void);
	int Writing(const char *buf, const size_t len);
	int OnReceive(char *buf, unsigned int len);
public:
	Socket(void);
	Socket(char*ip, int port);
	~Socket(void);
	void Start(void);
	void Release(void);
	unsigned int GetLastAlivedTime(void);
	unsigned int Get(char *buf, unsigned int len);
	unsigned int Put(const char *buf, unsigned int len);
};


#endif /* SOCKET_SOCKET_H_ */
