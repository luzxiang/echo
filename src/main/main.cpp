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
#include "../Socket/ISocket.h"
#include "../Socket/Server.h"
using namespace std;
#include <assert.h>
const int BUF_LEN = 1024 * 2;

void socket_client(char *ip,int port)
{
	char *buf = new char[BUF_LEN];
	ISocket *skt = new ISocket(ip, port);
	skt->Start();
	while(std::cin.getline(buf,BUF_LEN))
	{
		skt->wPut(buf, strlen(buf) + 1, 1);
		std::cout<<">>";
	}
	skt->Release();
	delete [] buf;
	getchar();
}
void socket_test(char mode = 's')
{
    char ip[] = "127.0.0.1";//"10.194.10.1";//"66.154.108.47";//
    int port = 4433;

    if(mode == 's')
    {
    	Server *server = new Server(ip, port, 1024);
    	server->Start();
    	getchar();
    }
    else if(mode == 'c')
    {
        LOG_INFO("ready to connect server:%s...",ip);
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


