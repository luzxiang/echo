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

void Server::Waite(void)
{
	struct epoll_event ev, events[Ev_Num];
	ssize_t n = 0;
	const int MAXLINE = 1024;
	char line[MAXLINE];
	int tout = 1000 * 10;//ms
	socklen_t clilen;
	struct sockaddr_in clientaddr;
	int connfd, sockfd, evs = 0;
	SktThdIsStart = true;
	while (SktThdIsStart)
	{
		evs = epoll_wait(epfd, events, Ev_Num, tout);//等待epoll事件的发生
		LOG_DEBUG("%d",evs);
		for(int i = 0; i < evs; i++)//处理所发生的所有事件
		{
			LOG_INFO("events[i].data.fd,%d",events[i].data.fd);
			if (events[i].data.fd == listenfd)	//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
			{
				connfd = accept(listenfd, (sockaddr *) &clientaddr, &clilen);
				if (connfd < 0)
				{
					perror("connfd<0");
					exit(1);
				}
				//setnonblocking(connfd);
				LOG_INFO("accapt a connection from %s" ,inet_ntoa(clientaddr.sin_addr));
				//设置用于读操作的文件描述符
				ev.data.fd = connfd;
				//设置用于注测的读操作事件
				ev.events = EPOLLIN | EPOLLET;
				//ev.events=EPOLLIN;
				//注册ev
				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
			}
			else if (events[i].events & EPOLLIN)	//如果是已经连接的用户，并且收到数据，那么进行读入。
			{
				LOG_INFO("EPOLLIN");
				if ((sockfd = events[i].data.fd) < 0)
					continue;
				if ((n = Read(sockfd, line, MAXLINE, 0)) < 0)
				{
					if (errno == ECONNRESET)
					{
						close(sockfd);
						events[i].data.fd = -1;
					}
					else
						std::cout << "readline error" << std::endl;
				}
				else if (n == 0)
				{
					close(sockfd);
					events[i].data.fd = -1;
				}
				line[n] = '\0';
				LOG_WATCH(line);
				//设置用于写操作的文件描述符
				ev.data.fd = sockfd;
				//设置用于注测的写操作事件
				ev.events = EPOLLOUT | EPOLLET;
				//修改sockfd上要处理的事件为EPOLLOUT
				epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
			}
			else if (events[i].events & EPOLLOUT) // 如果有数据发送
			{
				LOG_INFO("EPOLLOUT");
				sockfd = events[i].data.fd;
				Send(sockfd, line, n, 0);
				//设置用于读操作的文件描述符
				ev.data.fd = sockfd;
				//设置用于注测的读操作事件
				ev.events = EPOLLIN | EPOLLET;
				//修改sockfd上要处理的事件为EPOLIN
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
			}
		}
	}
}


