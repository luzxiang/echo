/*
 * socketClient.cpp
 *
 *  Created on: 2017年11月2日
 *      Author: root
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "../log/log.h"
//客户端
int socket_send(int fd, const char *pdata, const size_t len)
{
    fd_set writefd;
    size_t offset = 0;
    int sendLen = 0;

    while(offset < len)
    {
        FD_ZERO(&writefd);
        FD_SET(fd, &writefd);
        int ret = select(fd + 1, 0, &writefd, 0, NULL);
        if(ret == -1)
		{
			perror("select error\n");
			return -1;
		}
        sendLen = send(fd, pdata + offset, len - offset, 0);
        if(sendLen == -1)
        {
        	perror("send error");
        	return -1;
        }
        offset += sendLen;
    }
	return offset;
}

int socket_recv(int fd, char *pdata, int len)
{
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 300;

	int perLen = len;

	int recvLen = 0;
	fd_set readfd;
	FD_ZERO(&readfd);
	FD_SET(fd,&readfd);

	int offset = 0;

	while(select(fd + 1, &readfd, 0, 0, &timeout) > 0)
	{
		recvLen = recv(fd, pdata + offset, perLen, 0);
		if(recvLen > 0)
		{
			offset += recvLen;
			printf("正常处理数据...\n");
			//set the flag that data have received and Analyse
			if(offset >= len)
			{
				printf("跳出接收循环...\n");
				break;
			}
		}
		else if((recvLen < 0) &&(errno == EAGAIN||errno == EWOULDBLOCK||errno == EINTR)) //这几种错误码，认为连接是正常的，继续接收
		{
			printf("继续接收数据...\n");
			continue;//继续接收数据
		}
		else if( recvLen  == 0)
		{
			printf("跳出接收循环...\n");
			break;//跳出接收循环
		}
		else
		{
			printf("跳出接收循环...\n");
			break;//跳出接收循环
		}
	}
	return offset;
}

int tcp_client(char *ip,int port)
{
    int client_sockfd;
    int len;
    struct sockaddr_in remote_addr; 			//服务器端网络地址结构体
    char sendbuf[BUFSIZ];  						//数据传送的缓冲区
    char recvbuf[BUFSIZ];  						//数据传送的缓冲区
    memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
    remote_addr.sin_family=AF_INET; 			//设置为IP通信
    remote_addr.sin_addr.s_addr=inet_addr(ip);	//服务器IP地址
    remote_addr.sin_port=htons(port); 			//服务器端口号

    LOG_INFO("client working...");
    /*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/
    if((client_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
    {
        perror("socket");
        return -1;
    }

    /*将套接字绑定到服务器的网络地址上*/
    if(connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)
    {
        perror("connect");
        return -1;
    }
    LOG_INFO("connected to server");
    len=socket_recv(client_sockfd,recvbuf,sizeof(recvbuf));//接收服务器端信息
    recvbuf[len]='\0';
    LOG_INFO("%s\n",recvbuf); //打印服务器端信息
    /*循环的发送接收信息并打印接收信息--recv返回接收到的字节数，send返回发送的字节数*/
    while(1)
    {
    	LOG_INFO("send msg to server >");
        scanf("%s",sendbuf);
        if(0 == strcmp(sendbuf,"quit"))
            break;
        len=socket_send(client_sockfd,sendbuf,strlen(sendbuf));
        len=socket_recv(client_sockfd,recvbuf,sizeof(recvbuf));
        recvbuf[len]='\0';
        LOG_INFO("recv msg from server::%s\n",recvbuf);
        fflush(stdout);
        memset(recvbuf,0,sizeof(recvbuf));
    }
    close(client_sockfd);//关闭套接字
    return 0;
}
















