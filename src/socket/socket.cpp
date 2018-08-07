/*
 * socket.cpp
 *
 *  Created on: 2018年7月24日
 *      Author: luzxiang
 */
#include <sys/types.h>
#include <dirent.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <string>
#include <deque>
#include <map>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <iostream>
using namespace std;

#include "../log/log.h"

#define MAX_FD_NUM 3

struct ClientConnected{
	int fd;
	struct sockaddr_in addr;
	std::deque <string> SendBuf;//all data wait to send
	std::deque <string> RecvBuf;//all data have receive
};//created

std::map<int, ClientConnected> g_Clients;
//ClientConnected g_Clients[MAX_FD_NUM];
struct sockaddr_in g_sevrAddr;
/*******************************************************************************
 * Function     : set_socket
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
static int set_socketopt(int fd)
{
	int b_on = 0;
    ioctl(fd, FIONBIO, &b_on);
	//Linux环境下，须如下定义：
	struct timeval timeout = {3,0};
	//设置发送超时
	setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,(char *)&timeout,sizeof(struct timeval));
	//设置接收超时
	setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));
	//set recv buf size
	int recv_buf_size = 1*1024*1024;
	setsockopt(fd,SOL_SOCKET,SO_RCVBUF,(char *)&recv_buf_size,sizeof(recv_buf_size));
	//set send buf size
	int send_buf_size = 1*1024*1024;
	setsockopt(fd,SOL_SOCKET,SO_SNDBUF,(char *)&send_buf_size,sizeof(send_buf_size));

	return 0;
}

/*******************************************************
 * Function     : socket_create
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzxiang
 * Notes        : --
 ******************************************************/
int socket_create(int port)
{
	int fd = socket(AF_INET,SOCK_STREAM, 0);
	if(fd == -1)
	{
		printf("socket error\n");
		return -1;
	}

	memset(&g_sevrAddr, 0, sizeof(g_sevrAddr));
	g_sevrAddr.sin_family = AF_INET;
	g_sevrAddr.sin_port = htons(port);
	g_sevrAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(fd, (struct sockaddr*)&g_sevrAddr,sizeof(g_sevrAddr)) == -1)
	{
		printf("bind error!!!");
		return -1;
	}

	set_socketopt(fd);

	if(listen(fd, 20) == -1)
	{
		printf("listen error!!!");
		return -1;
	}
	return fd;
}

/*******************************************************************************
 * Function     : socket_accept
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int socket_accept(int sfd)
{
	int cfd ;
	unsigned int len = 0;
	struct sockaddr_in cliAddr;

	len = sizeof(cliAddr);
	memset(&cliAddr, 0, len);
	printf("accept client:\n");
	cfd = accept(sfd, (struct sockaddr*)&cliAddr, &len);

	if(cfd == -1)
	{
		printf("accept error!!!\n");
		return -1;
	}

//	for(int i = 0; i < MAX_FD_NUM; ++i)
//	for(auto c: g_Clients)
	g_Clients[cfd].fd = cfd;
	int size = g_Clients.size();
	printf("cfd=%d g_Clients.size=%d\n",cfd,size);
	memcpy(&g_Clients[cfd].addr,&cliAddr,sizeof(g_Clients[cfd].addr));
	printf("%s :%d\n",inet_ntoa(g_Clients[cfd].addr.sin_addr),g_Clients[cfd].addr.sin_port);
	return 0;
}

/*******************************************************************************
 * Function     : select_send
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int select_send(int fd)
{
	int ret = 0;
	int slen = 0;
	int max_fd = fd;
	unsigned int sumlen = 0;

	fd_set wfds;

	std::deque <string>::iterator msg = g_Clients[0].RecvBuf.begin();
	for(;msg != g_Clients[0].RecvBuf.end();msg = g_Clients[0].RecvBuf.begin())
	{
		sumlen = 0;
		while(sumlen < msg->length())
		{
			FD_ZERO(&wfds);
			FD_SET(fd,&wfds);

			ret = select(max_fd + 1,NULL,&wfds,NULL,NULL);
			if(ret == -1)
			{
				printf("select error!!!");
				return -1;
			}
			//
			if(FD_ISSET(fd, &wfds))
			{
				slen = send(fd,msg->data(),msg->length(),0);
				if(slen < 0)
				{
					perror("write");
					return -1;
				}
				sumlen += slen;
			}
		}
		g_Clients[0].RecvBuf.pop_front();
	}
//	g_RecvBuf.clear();
	return 0;
}

/*******************************************************
 * Function     : socket_accept
 * Description  : accept
 * Input        :
 * Return       :
 * Author       : luzxiang
 * Notes        : --
 ******************************************************/
void select_recv(int fd)
{
	int i =0;
	int ret = 0;
	int max_fd = fd;

	fd_set rfds;

	while(1)
	{
		FD_ZERO(&rfds);
		FD_SET(fd,&rfds);

		for(i = 0; i < MAX_FD_NUM; ++i)
		{
			if(g_Clients[i].fd != -1)
			{
				FD_SET(g_Clients[i].fd,&rfds);
			}
			max_fd = (max_fd < g_Clients[i].fd)?(g_Clients[i].fd):(max_fd);
		}
		ret = select(max_fd + 1, &rfds, NULL,NULL,NULL);
//		printf("select = %d\n",ret);
		if(ret == -1)
		{
			printf("select error!!!");
			break;
		}
		if(FD_ISSET(fd, &rfds))
		{
			socket_accept(fd);
			continue;
		}
		//
		for(i = 0; ret && i < MAX_FD_NUM; ++i)
		{
			if(!FD_ISSET(g_Clients[i].fd, &rfds))
				continue;
			--ret;
			char recvbuf[1024] = {0};
			if(0 == recv(g_Clients[i].fd, recvbuf, sizeof(recvbuf), 0))
			{
				close(g_Clients[i].fd);
				g_Clients[i].fd = -1;
				printf("socket closed!!!\n");
				continue;
			}
			g_Clients[0].RecvBuf.push_back(recvbuf);
			printf("recv:%s\n",recvbuf);
			select_send(g_Clients[i].fd);
		}
	}
	close(fd);
}


/*******************************************************************************
 * Function     : server
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
void socket_create_server(int port)
{
	int fd = socket_create(port);
	if(fd == -1)
	{
		printf("socket_create error!!!");
		return ;
	}
	select_recv(fd);
}
















