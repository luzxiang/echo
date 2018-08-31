//============================================================================
// Name        : cpp.cpp
// Author      :
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/stat.h>
#include <pthread.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <thread>
using std::this_thread::sleep_for;
using std::chrono::seconds;

#include "main.h"
#include "../log/log.h"
#include "../socket/tcp_select_server.h"
#include "../socket/Socket.h"
using namespace std;

void socket_client(char *ip,int port)
{

	Socket *skt = new Socket(ip, port);
	skt->Start();
	char recv[1000] = {0};
	std::thread show([&]{
		while(1)
		{
	        usleep(1000);
	        memset(recv,0,sizeof(recv));
	        if(skt->Get(recv,sizeof(recv)) > 0)
	        {
	        	LOG_INFO("[recv]:%s",recv);
				fflush(stdout);
	        }
		}
	});
	show.detach();
	char buf[1024*1024] = {0};
	unsigned int sumLen = 0;
	int len = 0;
	FILE *file = fopen("./data","rb");
	unsigned int start = time(0);
	LOG_WATCH(time(0));
	while ((len = fread(buf, 1, sizeof(buf), file)) > 0) //读磁盘文件
	{
		while(skt->Put(buf,len) == 0)
		{
			usleep(1000);
		}
		sumLen += len;
	}
	LOG_WATCH(skt->wFreeLen());
	LOG_WATCH(skt->wBufLen());
	LOG_WATCH(time(0)-start);
	LOG_WATCH(sumLen);
//	for(int i = 0; i <= 100000; i++)
//	{
//		sprintf(buf,"msg1234567890abcdefghijklmnopqrstuvwxyz(%d)",i);
//		len = strlen(buf);

//		sumLen += len;
//		while(skt->Put(buf,len) == 0)
//		{
//			usleep(100);
//		}
//	}
	while(std::cin.getline(buf,sizeof(buf)))
	{
		len = strlen(buf);
		skt->Put(buf,len);
	}
	getchar();
	skt->Release();
}
void socket_test(char mode = 's')
{
    char ip[] = "10.194.10.1";//127.0.0.1";//"66.154.108.47";//
    int port = 4433;

    if(mode == 's')
    {
    	socket_create_server(port);
    }
    else if(mode == 'c')
    {
        LOG_INFO("ready to connect server:%s...",ip);
//        tcp_client(ip,port);
        socket_client(ip,port);
    }
}
int main(int argc, char*argv[])
{
	LOG_LEVEL_SET(argc, argv);
	
	int m = [](int x) { return [](int y) { return y * 2; }(x)+6; }(5);
	std::cout<<"m:"<<m<<std::endl;//输出m:16 
	std::cout << "n:" << [](int x, int y) { return x + y; }(5, 4) << std::endl;            //输出n:9

	socket_test(argv[1][0]);
	getchar();
//    sys_getopt_long(argc, argv,::basename(argv[0]));
}


