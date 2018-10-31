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
#include "../log/log.h"
#include <chrono>
#include <thread>
using namespace std;

void Socket::SetSockOpt(void)
{
    int b_on = 0;       //设置为阻塞模式
    ioctl(fd, FIONBIO, &b_on);

	int reuse0=1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse0 , sizeof(reuse0));

    int rLen = 8*1024 * 1024;       //设置为32K
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*) &rLen, sizeof(rLen));
    //发送缓冲区
    int wLen = 8*1024 * 1024;       //设置为32K
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*) &wLen, sizeof(wLen));
    int tout = 5*1000; //5秒
    //发送时限
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &tout, sizeof(tout));
    //接收时限
    tout = 5*1000; //5秒
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tout, sizeof(tout));
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
	rFifo = new sockt_fifo_st(R_FIFO_BUF_LEN_);
	wFifo = new sockt_fifo_st(W_FIFO_BUF_LEN_);

    fd = -1;
    Encrypt = 0;
    lastAlivedTime = 0;
    rThdIsStart = false;
    wThdIsStart = false;
    IsConnected = false;         //连接状态

	wbuf = new char[RWBUF_LEN_];//(char*)malloc(RWBUF_LEN_);
	rbuf = new char[RWBUF_LEN_];//(char*)malloc(RWBUF_LEN_);
    memset(wbuf,0,RWBUF_LEN_);
    memset(rbuf,0,RWBUF_LEN_);

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
	rFifo = new sockt_fifo_st(R_FIFO_BUF_LEN_);
	wFifo = new sockt_fifo_st(W_FIFO_BUF_LEN_);

    fd = -1;
    Encrypt = 0;
    lastAlivedTime = 0;
    rThdIsStart = false;
    wThdIsStart = false;
    IsConnected = false;         //连接状态
    memset(&SvrAddr,0,sizeof(SvrAddr));
	wbuf = new char[RWBUF_LEN_];//(char*)malloc(RWBUF_LEN_);
	rbuf = new char[RWBUF_LEN_];//(char*)malloc(RWBUF_LEN_);
    memset(wbuf,0,RWBUF_LEN_);
    memset(rbuf,0,RWBUF_LEN_);
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
	LOG_INFO("rPthread.joinable()");
    if(rPthread.joinable())
    {
    	LOG_INFO("rPthread.join()");
    	rPthread.join();
    }
	LOG_INFO("wPthread.joinable()");
    if(wPthread.joinable())
    {
    	LOG_INFO("wPthread.join()");
    	wPthread.join();
    }
	std::this_thread::sleep_for(std::chrono::microseconds(10));
	if(rFifo != nullptr)
	{
		LOG_INFO("free(rFifo)");
		delete rFifo;//free(rFifo);
	}
	if(wFifo != nullptr)
	{
		LOG_INFO("free(wFifo)");
		delete wFifo;//free(wFifo);
	}
	if(wbuf != nullptr)
	{
		delete[] wbuf;//free(wbuf);
	}
	if(rbuf != nullptr)
	{
		delete[] rbuf;//free(rbuf);
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
    if (fd <= 0)
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
        LOG_INFO("Invalid address.");
        return - 1;
    }
	fd = socket(SvrAddr.sin_family, SOCK_STREAM, 0);
    if (fd <= 0)
	{
		LOG_INFO("socket error: %s",strerror(errno));
		return -1;
	}
	else
	{
		LOG_INFO("socket created successful");
	}
	// 设置属性
    SetSockOpt();
    if(connect(fd, (struct sockaddr*) &SvrAddr, sizeof(SvrAddr)) == -1)
	{
		LOG_INFO("connect error:%s", strerror(errno));
		return -1;
	}
    this->IsConnected = true;
	LOG_INFO("connect successful!!!\n");
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
    if (buf == nullptr)
        return -1;
    fd_set wfds;
    size_t offset = 0;
    timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    int wLen = 0;
    int nRet = 0;
    while (offset < len)
    {
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);
        nRet = select(fd + 1, 0, &wfds, 0, &timeout);
        if(nRet == 0)//time out
        {
        	continue;
        }else if(nRet < 0)
        {
        	LOG_WARN("select=%d error: %s",nRet, strerror(errno));
        	continue;
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
        	LOG_WARN("send error: %s", strerror(errno));
        	this->Close();
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
	LOG_INFO("Start Thread of OnWrite...");
	int onwlen = 0;

	wThdIsStart = true;
	this->wFifo->lock();
    while(wThdIsStart){
        while(this->wFifo->Empty() && wThdIsStart)
        {
        	this->wFifo->timewait(3);
        	if (!this->IsConnected)
        		break;
        }
    	if (this->IsConnected){
			onwlen = this->wFifo->Get(wbuf,sizeof(this->wbuf));
        	Writing(wbuf,onwlen);
        }else{
        	LOG_WARN("socket is disconnected!!!");
        	std::this_thread::sleep_for(std::chrono::seconds(1));
        	continue;
        }
    }
    this->wFifo->unlock();
    LOG_INFO("Finish Thread of OnWrite...");
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
	LOG_INFO("Start Thread of OnRead...");
	rThdIsStart = true;
    while(rThdIsStart){
        if (!this->IsConnected)
        {
        	std::this_thread::sleep_for(std::chrono::seconds(3));
        	this->ReConnect();
        	continue;
        }
    	Reading();
    }
    LOG_INFO("Finish Thread of OnRead...");
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
    int nLen = 0;
    timeval tout;
    tout.tv_sec = 2;
    tout.tv_usec = 0;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(this->fd, &rfds);

    int rfree = std::min(this->rFifo->Free(), (unsigned int)(RWBUF_LEN_));

    if (select(this->fd + 1, &rfds, 0, 0, &tout) > 0)
    {
        if (Encrypt == 0 || Encrypt == 2)
        {
        	memset(this->rbuf,0,RWBUF_LEN_);
		    nLen = recv(this->fd, this->rbuf, rfree, 0);
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
        	LOG_INFO("send error: %s", strerror(errno));
        	this->Close();
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
int Socket::OnReceive(char *buf,unsigned int len)
{
	lastAlivedTime = time(0);
	if(this->rFifo->HaveFree(len))
	{
		return this->rFifo->Put(buf,len);
	}
	return 0;
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
	return this->rFifo->Buf->Get(buf,len);
}
/*******************************************************************************
 * Function     : Socket::Get
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int Socket::Get(char *buf, unsigned int len,long int tout_s, long int tout_nsec)
{
	this->rFifo->lock();
	this->rFifo->timewait(tout_s, tout_nsec);
	this->rFifo->unlock();
	return this->rFifo->Empty() ? (0):(this->rFifo->Buf->Get(buf,len));
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
	if(this->wFifo->HaveFree(len))
		return this->wFifo->Put(buf,len);
	else
		return 0;
}


