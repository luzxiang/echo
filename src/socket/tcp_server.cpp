#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#include "../log/log.h"
void *create_tcpserver_thread(void*p)
{

    return NULL;
}

//服务器端
int tcp_server(int port)
{
    int server_sockfd;//服务器端套接字
    int client_sockfd;//客户端套接字
    int len;
    struct sockaddr_in my_addr;   //服务器网络地址结构体
    struct sockaddr_in remote_addr; //客户端网络地址结构体
    unsigned int sin_size;
    char recvbuf[BUFSIZ] = {0};  //数据传送的缓冲区
    char sendbuf[BUFSIZ] = {0};  //数据传送的缓冲区

    memset(&my_addr,0,sizeof(my_addr)); //数据初始化--清零
    my_addr.sin_family = AF_INET; //设置为IP通信
    my_addr.sin_addr.s_addr = INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
    my_addr.sin_port = htons(port); //服务器端口号

    LOG_DEBUG("server working...");
    /*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
    if((server_sockfd = socket(PF_INET,SOCK_STREAM,0)) < 0)
    {
        perror("socket");
        return 1;
    }
    LOG_DEBUG("socket create sucessful");
        /*将套接字绑定到服务器的网络地址上*/
    if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))<0)
    {
        perror("bind");
        return 1;
    }
    LOG_DEBUG("bind  sucessful");

    /*监听连接请求--监听队列长度为5*/
    listen(server_sockfd,5);

    sin_size = sizeof(struct sockaddr_in);
    /*等待客户端连接请求到达*/
    LOG_INFO("waiting client connect ...");
    if((client_sockfd = accept(server_sockfd,(sockaddr *)&remote_addr,&sin_size))<0)
    {
        perror("accept");
        return 1;
    }
    LOG_INFO("accept client %s :%d",inet_ntoa(remote_addr.sin_addr),remote_addr.sin_port);
    len = send(client_sockfd,"Welcome to my server.",21,0);//发送欢迎信息

    /*接收客户端的数据并将其发送给客户端--recv返回接收到的字节数，send返回发送的字节数*/
    while((len = recv(client_sockfd,recvbuf,BUFSIZ,0))>0)
    {
        recvbuf[len] = '\0';
        LOG_INFO("client:%s",recvbuf);
        sprintf(sendbuf,"[%s]",recvbuf);
//        sprintf(sendbuf,"have get you mes.%s",recvbuf);
//        LOG_INFO("ready to send mesg to client.");
        if(send(client_sockfd,sendbuf,strlen(sendbuf),0)<0)
        {
            perror("write");
            return 1;
        }
//        LOG_INFO("have send back to client");
    }
    LOG_INFO("client_sockfd");
    close(client_sockfd);
    LOG_INFO("server_sockfd");
    close(server_sockfd);
        return 0;
}
