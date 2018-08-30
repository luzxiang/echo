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
#define R_BUF_LEN_			(2*1024*1024)
#define W_BUF_LEN_			(2*1024*1024)

class Socket{
private:
    int fd;
	FIFO* wBuf;
	FIFO* rBuf;
	char wbuf[1024 * 16];
	char rbuf[1024 * 16];
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
	void OnReceive(char *buf, int len);
public:
	Socket(void);
	Socket(char*ip, int port);
	~Socket(void);
	void Start(void);
	void Release(void);
    unsigned int rBufLen(void);
    unsigned int wBufLen(void);
	unsigned int GetLastAlivedTime(void);
	unsigned int Get(char *buf, unsigned int len);
	unsigned int Put(const char *buf, unsigned int len);
};


#endif /* SOCKET_SOCKET_H_ */
