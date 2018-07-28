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
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_FD_NUM 3

/*******************************************************
 * Function     : setnonblock
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzxiang
 * Notes        : --
 ******************************************************/
void setnonblock(int fd)
{
	int flag = fcntl(fd,F_GETFL,0);
	if(flag == -1)
	{
		printf("get fcntl error!!!\n");
		return ;
	}
	int ret = fcntl(fd,F_SETFL,flag | O_NONBLOCK);
	if(ret == -1)
	{
		printf("set fcntl error!!!\n");
		return ;
	}
}
/*******************************************************************************
 * Function     : set_socket
 * Description  : TODO
 * Input        :
 * Return       :
 * Author       : luzx
 * Notes        : --
 *******************************************************************************/
int set_socket(int fd)
{
	setnonblock(fd);
	//Linux环境下，须如下定义：
	struct timeval timeout = {3,0};
	//设置发送超时
	setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,(char *)&timeout,sizeof(struct timeval));
	//设置接收超时
	setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));
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

	set_socket(fd);
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(fd, (struct sockaddr*)&addr,sizeof(addr)) == -1)
	{
		printf("bind error!!!");
		return -1;
	}
	if(listen(fd, 20) == -1)
	{
		printf("listen error!!!");
		return -1;
	}
	return fd;
}

/*******************************************************
 * Function     : socket_accept
 * Description  : accept
 * Input        :
 * Return       :
 * Author       : luzxiang
 * Notes        : --
 ******************************************************/
void socket_accept(int fd)
{
	int i =0;
	int max_fd = fd;

	int clients[MAX_FD_NUM] = {-1};
	int client_fd ;
	fd_set readfds;
	unsigned int len = 0;

	while(1)
	{
		FD_ZERO(&readfds);
		FD_SET(fd,&readfds);

		for(i = 0; i < MAX_FD_NUM; ++i)
		{
			if(clients[i] != -1)
			{
				FD_SET(clients[i],&readfds);
			}
			max_fd = (max_fd < clients[i])?(clients[i]):(max_fd);
		}
		int ret = select(max_fd + 1, &readfds, NULL,NULL,NULL);
//		printf("select = %d\n",ret);
		if(ret == -1)
		{
			printf("select error!!!");
			return;
		}
		if(FD_ISSET(fd, &readfds))
		{
			struct sockaddr_in client_addr;
			memset(&client_addr, 0, sizeof(client_addr));
			len = sizeof(client_addr);
			printf("accept client:\n");
			client_fd = accept(fd, (struct sockaddr*)&client_addr, &len);

			if(client_fd == -1)
			{
				printf("accept error!!!");
				break;
			}
			for(i = 0; i < MAX_FD_NUM; ++i)
			{
				if(clients[i] == -1)
				{
					clients[i] = client_fd;
					printf("%s :%d\n",inet_ntoa(client_addr.sin_addr),client_addr.sin_port);
					break;
				}
			}
			continue;
		}
		//
		for(i = 0; i < MAX_FD_NUM; ++i)
		{
			if(FD_ISSET(clients[i], &readfds))
			{
				char recvbuf[1024] = {0};
				recv(clients[i], recvbuf, sizeof(recvbuf), 0);
				printf("recv:%s\n",recvbuf);
				char sendbuf[1024] = {'\0'};
				sprintf(sendbuf,"[%s]",recvbuf);
				if(send(clients[i],sendbuf,strlen(sendbuf),0)<0)
				{
					perror("write");
					return ;
				}
			}
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
void socket_server(int port)
{
	int fd = socket_create(port);
	if(fd == -1)
	{
		printf("socket_create error!!!");
		return ;
	}
	socket_accept(fd);
}
















