/*
 * Socket.cpp
 *
 *  Created on: 2018年8月29日
 *      Author: luzxiang
 */
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <string.h>
#include "Socket.h"
#include <chrono>
#include <thread>


void Socket::SetSockOpt(void)
{
    int b_on = 0;       //设置为阻塞模式
    ioctl(fd, FIONBIO, &b_on);

    int rBufLen = 256 * 1024;       //设置为32K
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*) &rBufLen, sizeof(rBufLen));
    //发送缓冲区
    int wBufLen = 256 * 1024;       //设置为32K
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*) &wBufLen, sizeof(wBufLen));
    int timeout = 5*1000; //5秒
    //发送时限
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout));
    //接收时限
    timeout = 5*1000; //5秒
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
    // 禁用Nagle算法
    int nodelay = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
}
/*******************************************************************************
 * Function     : Socket::Socket
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
Socket::Socket(void)
{
	rBuf = new FIFO(R_BUF_LEN_);
	wBuf = new FIFO(W_BUF_LEN_);

    fd = -1;
    Encrypt = 0;
    lastAlivedTime = 0;
    rThdIsStart = false;
    wThdIsStart = false;
    IsConnected = false;         //连接状态
    memset(&wbuf,0,sizeof(wbuf));
    memset(&rbuf,0,sizeof(rbuf));

    Port = 0;
    memset(Ip,0,sizeof(Ip));
}
/*******************************************************************************
 * Function     : Socket:Sockt
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
Socket::Socket(char*ip, int port)
{
	rBuf = new FIFO(R_BUF_LEN_);
	wBuf = new FIFO(W_BUF_LEN_);

    fd = -1;
    Encrypt = 0;
    lastAlivedTime = 0;
    rThdIsStart = false;
    wThdIsStart = false;
    IsConnected = false;         //连接状态
    memset(&SvrAddr,0,sizeof(SvrAddr));
    memset(&wbuf,0,sizeof(wbuf));
    memset(&rbuf,0,sizeof(rbuf));
    Port = port;
    strncpy(Ip,ip,sizeof(Ip));
}

/*******************************************************************************
 * Function     : Socket::~Socket
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
Socket::~Socket(void)
{
	Release();
}
/*******************************************************************************
 * Function     : Socket::wLength
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int Socket::wBufLen(void)
{
	return this->wBuf->Len();
}

/*******************************************************************************
 * Function     : Socket::rLength
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int Socket::rBufLen(void)
{
	return this->rBuf->Len();
}

/*******************************************************************************
 * Function     : Socket::Start
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void Socket::Start(void)
{
	Connect();
	rPthread = std::thread(&Socket::OnWrite,this);
	wPthread = std::thread(&Socket::OnRead,this);
}
//
/*******************************************************************************
 * Function     : Socket::Stop
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void Socket::Stop(void)
{
}
/*******************************************************************************
 * Function     : Socket::Release
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void Socket::Release(void)
{
	std::this_thread::sleep_for(std::chrono::microseconds(100));
    rThdIsStart = false;
    wThdIsStart = false;
    this->Close();
	printf("rPthread.joinable()\n");
    if(rPthread.joinable())
    {
    	printf("rPthread.join()\n");
    	rPthread.join();
    }
	printf("wPthread.joinable()\n");
    if(wPthread.joinable())
    {
    	printf("wPthread.join()\n");
    	wPthread.join();
    }
	std::this_thread::sleep_for(std::chrono::microseconds(10));
	if(rBuf != nullptr)
	{
		printf("free(rBuf)\n");
		free(rBuf);
	}
	if(wBuf != nullptr)
	{
		printf("free(wBuf)\n");
		free(wBuf);
	}
}

/*******************************************************************************
 * Function     : Close
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void Socket::Close(void)
{
    IsConnected = false;
	if(fd != -1)
	{
		close(fd);
	    fd = -1;
	}
}
//
/*******************************************************************************
 * Function     : Connect
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int Socket::Connect(void)
{
    bzero(&SvrAddr, sizeof(SvrAddr));
    SvrAddr.sin_family = AF_INET;
    SvrAddr.sin_port = htons(Port);

    if (inet_pton(SvrAddr.sin_family, Ip, &SvrAddr.sin_addr) == 0)
    {
        printf("Invalid address.\n");
        return - 1;
    }
	fd = socket(SvrAddr.sin_family, SOCK_STREAM, 0);
    if (fd <= 0)
	{
		printf("create socket: %s\n",strerror(errno));
		return -1;
	}
	else
	{
		printf("socket created successful\n");
	}
	// 设置属性
    SetSockOpt();
    if(connect(fd, (struct sockaddr*) &SvrAddr, sizeof(SvrAddr)) == -1)
	{
		printf("%s\n", strerror(errno));
		return -1;
	}
    this->IsConnected = true;
	return 0;
}

/*******************************************************************************
 * Function     : ReConnect
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int Socket::ReConnect(void)
{
	this->Close();
	return this->Connect();
}

/*******************************************************************************
 * Function     : Socket::send
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int Socket::Writing(const char *buf, const size_t len)
{
    if (!this->IsConnected || buf == nullptr)
        return -1;
    fd_set writefd;
    size_t offset = 0;
//    timeval timeout;
//    timeout.tv_sec = 2;
//    timeout.tv_usec = 0;

    int wLen = 0;
    while (offset < len)
    {
        FD_ZERO(&writefd);
        FD_SET(fd, &writefd);
        int nRet = select(fd + 1, 0, &writefd, 0, NULL);//&timeout);
        if(nRet == -1)
        {
        	printf("select error: %s\n", strerror(errno));
        	return -1;
        }
        if (Encrypt == 0 || Encrypt == 2) //数据不加密
        {
            if (fd > 0)
        		wLen = send(fd, buf + offset, len - offset, 0);
        }
//        else if (nEncrypt == 1 && m_pSsl)  //数据ssl加密
//        {
//             nSendLen = SSL_write(m_pSsl,cmd + offset, len - offset);
//        }
        if(wLen == -1)
        {
        	printf("send error: %s\n", strerror(errno));
        	return -1;
        }
        offset += wLen;
    }
    return offset;
}
/*******************************************************************************
 * Function     : Socket::OnWrite
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void Socket::OnWrite(void)
{
	printf("Start Thread of OnWrite...\n");
	int onwlen = 0;
	char *buf = (char*)malloc(sizeof(this->wbuf));
	struct timespec tout;
	memset(&tout, 0, sizeof(tout));
	tout.tv_nsec = 0;

	wThdIsStart = true;
    while(wThdIsStart){
        pthread_mutex_lock(&this->wBuf->mtx);
        while(this->wBuf->Empty() && wThdIsStart)
        {
        	tout.tv_sec = time(0) + 3;
        	pthread_cond_timedwait(&this->wBuf->cv, &this->wBuf->mtx,&tout);
        }
        onwlen = this->wBuf->Get(buf,sizeof(this->wbuf));
        pthread_mutex_unlock(&this->wBuf->mtx);
        if(wThdIsStart)
        {
        	Writing(buf,onwlen);
        }
    }
    printf("Finish Thread of OnWrite...\n");
}
/*******************************************************************************
 * Function     : Socket::OnRead
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void Socket::OnRead(void)
{
	printf("Start Thread of OnRead...\n");
	rThdIsStart = true;
    while(rThdIsStart){
    	Reading();
    }
    printf("Finish Thread of OnRead...\n");
}
/*******************************************************************************
 * Function     : Socket::GetLastAlivedTime
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int Socket::GetLastAlivedTime(void)
{
	return this->lastAlivedTime;
}

/*******************************************************************************
 * Function     : Socket::read
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int Socket::Reading(void)
{
    if (this->IsConnected == false)
    {
    	printf("socket is disconnected!!!\n");
    	std::this_thread::sleep_for(std::chrono::seconds(3));
    	return this->ReConnect();
    }
    int nLen = 0;
    timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(this->fd, &fdset);

    if (select(this->fd + 1, &fdset, 0, 0, &timeout) > 0)
    {
        if (Encrypt == 0 || Encrypt == 2)
        {
        	memset(this->rbuf,0,sizeof(this->rbuf));
		    nLen = recv(this->fd, this->rbuf, sizeof(this->rbuf), 0);
		}
//        else if (nEncrypt == 1 && m_pSsl != NULL)
//        {
//		    nLen = SSL_read(m_pSsl, buffer, sizeof(buffer));
//		}
        if (nLen > 0)
        {
        	OnReceive(this->rbuf,nLen);
        }
        else//SOCKE 异常（已退出）
        {
        	printf("send error: %s\n", strerror(errno));
        }
    }
	return 0;
}
/*******************************************************************************
 * Function     : Socket::OnReceive
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void Socket::OnReceive(char *buf, int len)
{
	lastAlivedTime = time(0);
	this->rBuf->Put(buf,len);
}
/*******************************************************************************
 * Function     : Socket::Get
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int Socket::Get(char *buf, unsigned int len)
{
	return this->rBuf->Get(buf,len);
}
/*******************************************************************************
 * Function     : Socket::Put
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int Socket::Put(const char *buf, unsigned int len)
{
	int plen = 0;
    pthread_mutex_lock(&this->wBuf->mtx);//需要操作head这个临界资源，先加锁，
    plen = this->wBuf->Put(buf,len);
    pthread_mutex_unlock(&this->wBuf->mtx);//解锁
    pthread_cond_signal(&this->wBuf->cv);
	return plen;
}


