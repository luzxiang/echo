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
	char buf[255] = {0};

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
	for(int i = 0; i <= 50000; i++)
	{
		sprintf(buf,"mesg%d",i);
		skt->Put(buf,strlen(buf));
	}
	while(std::cin.getline(buf,sizeof(buf)))
	{
		skt->Put(buf,strlen(buf));
	}
	getchar();
	skt->Release();
}
void socket_test(char mode = 's')
{
    char ip[] = "127.0.0.1";//"66.154.108.47";//
    int port = 4141;

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


