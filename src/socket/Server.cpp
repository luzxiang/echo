/*
 * Socket.cpp
 *
 *  Created on: 2018年8月29日
 *      Author: luzxiang
 */
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <functional>
#include "../log/log.h"
#include "Server.h"
using namespace std;

void Server::SetSockOpt(void)
{
#ifdef OS_LINUX
    int b_on = 0;       //设置为阻塞模式
    ioctl(listenfd, FIONBIO, &b_on);

	int reuse0=1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse0 , sizeof(reuse0));

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (const void *)&reuse0 , sizeof(reuse0));

    int rLen = 8*1024 * 1024;
    setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (const char*) &rLen, sizeof(rLen));
    //发送缓冲区
    int wLen = 8*1024 * 1024;
    setsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, (const char*) &wLen, sizeof(wLen));
    int tout = 5*1000; //5秒
    //发送时限
    setsockopt(listenfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &tout, sizeof(tout));
    //接收时限
    tout = 5*1000; //5秒
    setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tout, sizeof(tout));
    // 禁用Nagle算法
    int nodelay = 1;
    setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
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
Server::Server(void)
{
	epfd = -1;
	Ev_Num = 0;
	listenfd = -1;
    lastAlivedTime = time(NULL);
    SktThdIsStart = false;
    IsConnected = false;         //连接状态

	wFifo = new sockt_fifo_st(W_FIFO_BUF_LEN_);
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
Server::Server(char*ip, int port, int num)
{
	Server();

	listenfd = -1;
	epfd = -1;
	Ev_Num = num;
    lastAlivedTime = time(NULL);
    SktThdIsStart = false;
    IsConnected = false;         //连接状态
	wFifo = new sockt_fifo_st(W_FIFO_BUF_LEN_);
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
 * Function     : Socket::~Socket
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
Server::~Server(void)
{
	this->Release();
}

/*******************************************************************************
 * Function     : Socket::Start
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
bool Server::Start(void)
{
	if(0 != Create())
		return false;
	SktThread = std::thread(&Server::Waite,this);
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
void Server::Stop(void)
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
void Server::Release(void)
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
	if(wbuffer != nullptr)
	{
		delete[] wbuffer;//free(wbuf);
	}
	if(rbuffer != nullptr)
	{
		delete[] rbuffer;//free(rbuf);
		rbuffer = nullptr;
	}
#include <sys/epoll.h>
    this->listenfd = -1;
}

/*******************************************************************************
 * Function     : Close
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void Server::Close(void)
{
    IsConnected = false;
    if (listenfd > 0)
	{
		close(listenfd);
		listenfd = -1;
	}
}

/*******************************************************************************
 * Function     : Socket::GetLastAlivedTime
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int Server::GetLastAlivedTime(void)
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
bool Server::IsAlived(void)
{
	return this->IsConnected;
}
/*******************************************************************************
 * Function     : setnonblocking
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void Server::setnonblocking(int sock)
{
    int opts;
    opts = fcntl(sock,F_GETFL);
    if(opts<0)
    {
        perror("fcntl(sock,GETFL)");
        exit(1);
    }
    opts = opts | O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts) < 0)
    {
        perror("fcntl(sock,SETFL,opts)");
        exit(1);
    }
}
/*******************************************************************************
 * Function     : Create
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int Server::Create(void)
{
	//声明epoll_event结构体的变量,ev用于注册事件,数组用于回传要处理的事件
	struct epoll_event ev;
	//生成用于处理accept的epoll专用的文件描述符
	epfd = epoll_create(Ev_Num);
	struct sockaddr_in serveraddr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	SetSockOpt();
	//把socket设置为非阻塞方式
//	setnonblocking(listenfd);
	//设置与要处理的事件相关的文件描述符
	ev.data.fd = listenfd;
	//设置要处理的事件类型
	ev.events = EPOLLIN|EPOLLET;
	//ev.events = EPOLLIN;
	//注册epoll事件

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(Port);
	bind(listenfd,(sockaddr *)&serveraddr, sizeof(serveraddr));
	listen(listenfd, Ev_Num);//在没有被accept之前，最大支持多少个连接请求
	LOG_INFO("Socket is create successful");
	epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);
	return 0;
}
/*******************************************************************************
 * Function     : Close
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void Server::Close(struct epoll_event ev)
{
	auto iev = std::find_if(Clients.begin(),Clients.end(),[&](const Client_St* c){
		return c->ev.data.fd == ev.data.fd;
	});

	LOG_WATCH(Clients.size());
	LOG_WARN("client[%s:%d] is closed", (*iev)->ip.c_str(), (*iev)->port);
	Clients.erase(iev);
	close(ev.data.fd);
	LOG_WATCH(Clients.size());
	//修改sockfd上要处理的事件为EPOLLOUT
	epoll_ctl(epfd,EPOLL_CTL_DEL,ev.data.fd,&ev);
	ev.data.fd = -1;
}

/*******************************************************************************
 * Function     : Waite
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void Server::Waite(void)
{
	struct epoll_event ev, events[Ev_Num];
	const int MAXLINE = 1024;
	char readbuf[MAXLINE]={0};
	char sendbuf[MAXLINE]={0};
	ssize_t readlen = 0;
	ssize_t sendlen = 0;
	int tout = 1000 * 10;//ms
	struct sockaddr_in clientaddr;
    socklen_t clilen = sizeof(struct sockaddr);
	int connfd, evs = 0;
	SktThdIsStart = true;
	while (SktThdIsStart)
	{
		evs = epoll_wait(epfd, events, Ev_Num, tout);//等待epoll事件的发生
		LOG_DEBUG("%d",evs);
		for(int i = 0; i < evs; i++)//处理所发生的所有事件
		{
			if (events[i].data.fd == listenfd)	//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
			{
				memset(&clientaddr, 0, sizeof(clientaddr));
				connfd = accept4(listenfd, (sockaddr *) &clientaddr, &clilen, SOCK_CLOEXEC);
				if (connfd < 0)
				{
					perror("connfd<0");
					exit(1);
				}
				//setnonblocking(connfd);
				ev.data.fd = connfd;//设置用于读操作的文件描述符
				ev.events = EPOLLIN | EPOLLET;//设置用于注测的读操作事件
				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
				this->Clients.insert(new Client_St(ev, inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port));
				LOG_INFO("accapt a connection from %s:%d"
						,inet_ntoa(clientaddr.sin_addr)
						,clientaddr.sin_port);
			}
			else if (events[i].events & EPOLLIN)	//如果是已经连接的用户，并且收到数据，那么进行读入。
			{
				if ((events[i].data.fd) < 0)
					continue;
				readlen = Read(events[i].data.fd, readbuf, MAXLINE, 0);
				if (readlen == 0 || (errno == ECONNRESET))
				{
					Close(events[i]);
					continue;
				}else if(readlen < 0)
				{
					std::cout << "read line error" << std::endl;
					continue;
				}
				readbuf[readlen] = '\0';
				this->wPut(readbuf, readlen, 0, 100);
				auto iev = std::find_if(Clients.begin(),Clients.end(),[&](const Client_St* c){
					return c->ev.data.fd == events[i].data.fd;
				});
				LOG_INFO("client[%s:%d,%d] %s",(*iev)->ip.c_str(), (*iev)->port, (*iev)->ev.data.fd, readbuf);
				ev.data.fd = events[i].data.fd;//设置用于写操作的文件描述符
				ev.events = EPOLLOUT | EPOLLET;//设置用于注测的写操作事件
				epoll_ctl(epfd,EPOLL_CTL_MOD, events[i].data.fd, &ev);//修改sockfd上要处理的事件为EPOLLOUT
			}
			else if (events[i].events & EPOLLOUT) //如果有数据发送
			{
				sendlen = this->wGet(sendbuf,sizeof(sendbuf),0,10);
				Send(events[i].data.fd, sendbuf, sendlen, 0);

				ev.data.fd = events[i].data.fd;//设置用于读操作的文件描述符
				ev.events = EPOLLIN | EPOLLET;//设置用于注测的读操作事件
				epoll_ctl(epfd, EPOLL_CTL_MOD, events[i].data.fd, &ev);//修改sockfd上要处理的事件为EPOLIN
			}
		}
	}
}

/*******************************************************************************
 * Function     : GetWBufLen
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int Server::GetWBufLen()
{
	return wFifo->Len();
}
/*******************************************************************************
 * Function     : Socket::Get
 * Description  : 从待写FiFo缓存里获取数据
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int Server::wGet(char *buf, unsigned int len)
{
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
unsigned int Server::wGet(char *buf, unsigned int len, unsigned int tout_s)
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
unsigned int Server::wGet(char *buf, unsigned int len, unsigned int tout_s, unsigned int tout_msec)
{
	if(len == 0) return 0;
	return this->Get(this->wFifo, buf, len, tout_s, tout_msec);
}
/*******************************************************************************
 * Function     : Socket::Get
 * Description  : 从指定的FiFo缓存里获取数据
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int Server::Get(sockt_fifo_st *fifo,char *buf, unsigned int len, unsigned int tout_s, long int tout_msec)
{
	if(tout_msec < 0) tout_msec = 0;
	if(fifo->Empty())
	{
		mtxlock lck(fifo->mtx);
		fifo->cv.wait_for(lck, std::chrono::milliseconds(tout_s*1000+tout_msec));
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
unsigned int Server::wPut(const char *buf, unsigned int len)
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
unsigned int Server::wPut(const char *buf, unsigned int len, unsigned int tout_s)
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
unsigned int Server::wPut(const char *buf, unsigned int len, unsigned int tout_s, unsigned int tout_ms)
{
	return this->Put(this->wFifo, buf, len, tout_s, tout_ms);
}

/*******************************************************************************
 * Function     : Socket::Put
 * Description  : 往指定FiFo缓存里推入数据,等待处理
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
unsigned int Server::Put(sockt_fifo_st *fifo,const char *buf, unsigned int len, unsigned int tout_s, long int tout_ms)
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

