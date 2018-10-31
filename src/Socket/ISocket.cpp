/*
 * Socket.cpp
 *
 *  Created on: 2018年8月29日
 *      Author: luzxiang
 */ 

#include <functional>
#include "../log/log.h"
#include "ISocket.h" 
using namespace std;

void ISocket::SetSockOpt(void)
{
#ifdef OS_LINUX
    int b_on = 0;       //设置为阻塞模式
    ioctl(fd, FIONBIO, &b_on);

	int reuse0=1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse0 , sizeof(reuse0));

    int rLen = 8*1024 * 1024;   
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*) &rLen, sizeof(rLen));
    //发送缓冲区
    int wLen = 8*1024 * 1024;     
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
#endif
}
/*******************************************************************************
 * Function     : Socket::Socket
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
ISocket::ISocket(void)
{
	rFifo = new sockt_fifo_st(R_FIFO_BUF_LEN_);
	wFifo = new sockt_fifo_st(W_FIFO_BUF_LEN_);

    fd = -1;
    lastAlivedTime = time(NULL);
    SktThdIsStart = false;
    IsConnected = false;         //连接状态

	wbuffer = new char[RW_SOCKT_BUF_MAXLEN_];//(char*)malloc(RWBUF_LEN_);
	rbuffer = new char[RW_SOCKT_BUF_MAXLEN_];//(char*)malloc(RWBUF_LEN_); 
    memset(wbuffer,0,RW_SOCKT_BUF_MAXLEN_);
    memset(rbuffer,0,RW_SOCKT_BUF_MAXLEN_);
    memset(&SvrAddr,0,sizeof(SvrAddr));

	Read = std::bind(&recv, _1, _2, _3, _4);
	Send = std::bind(&send, _1, _2, _3, _4);

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
ISocket::ISocket(char*ip, int port)
{
	ISocket();
	rFifo = new sockt_fifo_st(R_FIFO_BUF_LEN_);
	wFifo = new sockt_fifo_st(W_FIFO_BUF_LEN_); 

    fd = -1;
    lastAlivedTime = time(NULL);
    SktThdIsStart = false;
    IsConnected = false;         //连接状态
	wbuffer = new char[RW_SOCKT_BUF_MAXLEN_];//(char*)malloc(RWBUF_LEN_);
	rbuffer = new char[RW_SOCKT_BUF_MAXLEN_];//(char*)malloc(RWBUF_LEN_);
    memset(wbuffer,0,RW_SOCKT_BUF_MAXLEN_); 
    memset(rbuffer,0,RW_SOCKT_BUF_MAXLEN_); 
    memset(&SvrAddr,0,sizeof(SvrAddr));

	Read = std::bind(&recv, _1, _2, _3, _4);
	Send = std::bind(&send, _1, _2, _3, _4);

    Port = port;
    strncpy(Ip,ip,sizeof(Ip));
}
/*******************************************************************************
 * Function     : Socket::GetFd
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int ISocket::GetFd(void)
{
	return this->fd;
}
/*******************************************************************************
 * Function     : Socket::~Socket
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
ISocket::~ISocket(void)
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
bool ISocket::Start(void)
{
	if(0 != Connect())
		return false;
	SktThread = std::thread(&ISocket::Selecting,this);
	return true;
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
void ISocket::Stop(void)
{
    this->Release();
}
/*******************************************************************************
 * Function     : Socket::Release
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void ISocket::Release(void)
{
	SLEEP_US(100);
    SktThdIsStart = false;
    this->Close();
    LOG_DEBUG("wPthread.joinable()");
	SLEEP_US(10);
    if(SktThread.joinable())
    {
    	LOG_DEBUG("wPthread.join()");
    	SktThread.join();
    }
	SLEEP_US(10);
	if(rFifo != nullptr)
	{
		LOG_DEBUG("free(rFifo)");
		rFifo->Release();
		delete rFifo;//free(rFifo);
	}
	if(wFifo != nullptr)
	{
		LOG_DEBUG("free(wFifo)");
		wFifo->Release();
		delete wFifo;//free(wFifo);
	}
	if(wbuffer != nullptr)
	{
		delete[] wbuffer;//free(wbuf);
	}
	if(rbuffer != nullptr)
	{
		delete[] rbuffer;//free(rbuf);
		rbuffer = nullptr;
	}
    this->fd = -1;
}

/*******************************************************************************
 * Function     : Close
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void ISocket::Close(void)
{
    IsConnected = false;
    if (fd > 0)
	{
		close(fd);
	    fd = -1;
	}
}
/*******************************************************************************
 * Function     : Connect
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --Encrypt
 *******************************************************************************/
int ISocket::Connect(void)
{
	bzero(&SvrAddr , sizeof(SvrAddr));
    SvrAddr.sin_family = AF_INET;
    SvrAddr.sin_port = htons(Port);

    if (inet_pton(SvrAddr.sin_family, Ip, &SvrAddr.sin_addr) == 0)
    {
        LOG_WARN("Invalid address.");
        return - 1;
    }
	fd = socket(SvrAddr.sin_family, SOCK_STREAM, 0);
    if (fd <= 0)
	{
    	LOG_WARN("socket error: %s",strerror(errno));
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
		LOG_WARN("connect error:%s", strerror(errno));
		return -1;
	}
	LOG_INFO("connect to server:%s successful!!!\n",this->Ip);
	this->IsConnected = true;
	memset(pfds,0,sizeof(pfds));
	int ret = pipe(pfds);
	if(ret < 0)
	{
		LOG_WARN("[errno=%d]%s",errno,strerror(errno));
	}
	LOG_WATCH(pfds[0]);
	LOG_WATCH(pfds[1]);
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
int ISocket::ReConnect(void)
{
	this->Close();
	return this->Connect();
}
/*******************************************************************************
 * Function     : ISocket::Selecting
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int ISocket::Selecting(void)
{
    timeval tout;
    //reading
    fd_set rfds;
	FD_ZERO(&rfds);
    //writing
    fd_set wfds;
	FD_ZERO(&wfds);

    int nRet = 0;
    SktThdIsStart = true;
    char buffer[100] = {0};
    int maxfd = std::max<int>(pfds[0],fd);
    while(SktThdIsStart)
    {
        tout.tv_sec = 2;
        tout.tv_usec = 0;
	    FD_ZERO(&rfds);
	    FD_SET(this->fd, &rfds);
	    FD_SET(this->pfds[0], &rfds);
		if(this->wFifo->Len() > 0)
		{
		    FD_ZERO(&wfds);
	    	FD_SET(this->fd, &wfds);
	        tout.tv_sec = 0;
		}

		nRet = select(maxfd + 1, &rfds, &wfds, 0, &tout);
	    if(!SktThdIsStart)break;
        if (!this->IsConnected) {
        	LOG_WARN("Socket is disConnected, reConnect......\n");
        	SLEEP_S(1);
        	if (!this->IsConnected) this->ReConnect();
        	maxfd = std::max<int>(pfds[0],fd);
        	continue;
        }
		if(nRet < 0){
			LOG_WARN("select=%d error: %s",nRet, strerror(errno));
			continue;
		}else if(nRet == 0){
//			continue;
		}
		if(FD_ISSET(pfds[0],&rfds)) {
			read(pfds[0], buffer, sizeof(buffer));
		}
		if(FD_ISSET(fd,&rfds))
		{
			LOG_DEBUG("reading\n");
			Reading();
		}
		if(this->wFifo->Len() > 0)
		{
			LOG_DEBUG("writing\n");
			Writing();
		}
    }
	return 0;
}
/*******************************************************************************
 * Function     : Socket::send
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int ISocket::Writing(void)
{
	unsigned int glen = std::min<int>(this->wFifo->Len(), RW_SOCKT_BUF_LEN_);
    int onwlen = this->wGet(this->wbuffer, glen, 3);
    if(onwlen <= 0)
    	return 0;
    int wlen = this->Send(fd, this->wbuffer, onwlen, 0);
	if(wlen == -1) {
		LOG_WARN("send error: %s", strerror(errno));
		this->Close();
	}else{
		this->lastAlivedTime = time(NULL);
	}
    return wlen;
}
/*******************************************************************************
 * Function     : Socket::read
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int ISocket::Reading(void)
{
    int rfree = std::min<int>(this->rFifo->HaveFree(), (RW_SOCKT_BUF_MAXLEN_));
    if(rfree <= 0)
    	return -1;
    int nLen = 0;
	memset(this->rbuffer,0,rfree);
	nLen = Read(this->fd, this->rbuffer, rfree, 0);
	if (nLen > 0) {
//		OnReceive(this->rbuffer, nLen);
		LOG_WATCH(this->rbuffer);
	}
	//返回值<0时
	else if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {//认为连接是正常的，继续接收
		LOG_INFO("errno[%d] error: %s", errno, strerror(errno));
	}else{//SOCKE 异常（已退出）
		LOG_WATCH(this->fd);
		LOG_WATCH(rfree);
		LOG_INFO("errno=%d, error:%s",errno, strerror(errno));
		this->Close();
	}
	return 0;
}
/*******************************************************************************
 * Function     : GetWBufLen
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::GetWBufLen()
{
	return wFifo->Len();
}
/*******************************************************************************
 * Function     : GetRBufLen
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::GetRBufLen()
{
	return rFifo->Len();
}
/*******************************************************************************
 * Function     : Socket::OnReceive
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int ISocket::OnReceive(char *buf,unsigned int len)
{
	lastAlivedTime = time(NULL);
	return rPut(buf, len, 30);
}
/*******************************************************************************
 * Function     : Socket::GetLastAlivedTime
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::GetLastAlivedTime(void)
{
	return this->lastAlivedTime;
}
/*******************************************************************************
 * Function     : bool IsAlived(void)
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
bool ISocket::IsAlived(void)
{
	return this->IsConnected;
}
/*******************************************************************************
 * Function     : Socket::Get
 * Description  : 从待写FiFo缓存里获取数据
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::wGet(char *buf, unsigned int len)
{
	LOG_TRACE("rFifo->Len()=%d, rFifo->HaveFree()=%d",this->rFifo->Len(),this->rFifo->HaveFree());
	return this->Get(this->wFifo, buf,len,0, 0);
}
/*******************************************************************************
 * Function     : Socket::Get
 * Description  : 从待写FiFo缓存里获取数据
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::wGet(char *buf, unsigned int len, unsigned int tout_s)
{
	return this->Get(this->wFifo, buf, len, tout_s, 0);
}
/*******************************************************************************
 * Function     : Socket::Get
 * Description  : 从待写FiFo缓存里获取数据
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::wGet(char *buf, unsigned int len, unsigned int tout_s, unsigned int tout_msec)
{
	if(len == 0) return 0;
	return this->Get(this->wFifo, buf, len, tout_s, tout_msec);
}
/*******************************************************************************
 * Function     : Socket::Get
 * Description  : 从已读FiFo缓存里获取数据
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::rGet(char *buf, unsigned int len)
{
	if(len == 0) return 0;
	LOG_TRACE("rFifo->Len()=%d, rFifo->HaveFree()=%d",this->rFifo->Len(),this->rFifo->HaveFree());
	return this->Get(this->rFifo, buf, len, 0, 0);
}
/*******************************************************************************
 * Function     : Socket::Get
 * Description  : 从已读FiFo缓存里获取数据
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::rGet(char *buf, unsigned int len, unsigned int tout_s)
{
	return this->Get(this->rFifo, buf, len, tout_s, 0);
}
/*******************************************************************************
 * Function     : Socket::Get
 * Description  : 从已读FiFo缓存里获取数据
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::rGet(char *buf, unsigned int len, unsigned int tout_s, unsigned int tout_msec)
{
	return this->Get(this->rFifo, buf,len,tout_s, tout_msec);
}

/*******************************************************************************
 * Function     : Socket::Get
 * Description  : 从指定的FiFo缓存里获取数据
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::Get(sockt_fifo_st *fifo,char *buf, unsigned int len, unsigned int tout_s, long int tout_msec)
{
	if(tout_msec < 0) tout_msec = 0;
	if(fifo->Empty())
	{
		LOG_TRACE("rFifo->lock()");
		mtxlock lck(fifo->mtx);
		LOG_TRACE("rFifo->timewait()");
		fifo->cv.wait_for(lck, std::chrono::milliseconds(tout_s*1000+tout_msec));
		LOG_TRACE("rFifo->unlock()");
	}
	if(fifo->Empty()) return 0;
	return fifo->Get(buf,len);
}
/*******************************************************************************
 * Function     : Socket::Put
 * Description  : 往待写FiFo缓存里推入数据,等待发送
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::wPut(const char *buf, unsigned int len)
{
	return this->Put(this->wFifo, buf,len,0, 0);
}
/*******************************************************************************
 * Function     : Socket::Put
 * Description  : 往待写FiFo缓存里推入数据,等待发送
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::wPut(const char *buf, unsigned int len, unsigned int tout_s)
{
	return this->Put(this->wFifo, buf, len, tout_s, 0);
}
/*******************************************************************************
 * Function     : Socket::Put
 * Description  : 往待写FiFo缓存里推入数据,等待发送
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::wPut(const char *buf, unsigned int len, unsigned int tout_s, unsigned int tout_ms)
{
	return this->Put(this->wFifo, buf, len, tout_s, tout_ms);
}

/*******************************************************************************
 * Function     : Socket::Put
 * Description  : 往已读FiFo缓存里推入数据,等待处理
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::rPut(const char *buf, unsigned int len)
{
	return this->Put(this->rFifo, buf, len, 0, 0);
}
/*******************************************************************************
 * Function     : Socket::Put
 * Description  : 往已读FiFo缓存里推入数据,等待处理
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::rPut(const char *buf, unsigned int len, unsigned int tout_s)
{
	return this->Put(this->rFifo, buf, len, tout_s, 0);
}
/*******************************************************************************
 * Function     : Socket::Put
 * Description  : 往已读FiFo缓存里推入数据,等待处理
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::rPut(const char *buf, unsigned int len, unsigned int tout_s, unsigned int tout_ms)
{
	return this->Put(this->rFifo, buf, len, tout_s, tout_ms);
}
/*******************************************************************************
 * Function     : Socket::Put
 * Description  : 往指定FiFo缓存里推入数据,等待处理
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int ISocket::Put(sockt_fifo_st *fifo,const char *buf, unsigned int len, unsigned int tout_s, long int tout_ms)
{
	int plen = 0;
	int perwait_ms = 1;
	tout_ms += tout_s * 1000;
	if(tout_ms < 0) tout_ms = 0;
	while(tout_ms >= 0)
	{
		if(fifo->HaveFree(len))
		{
			plen = fifo->Put(buf,len);
			write(pfds[1], "read please", 1);
			break;
		}
		SLEEP_US(perwait_ms*1000);
		tout_ms -= perwait_ms;
	}
//	LOG_WARN("Failure to push, time out:%ld ms!!!",tout_ms);
	return plen;
}


