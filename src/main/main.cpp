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
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <thread>

#include "main.h"
#include "../log/log.h"
#include "../socket/tcp_server.h"
#include "../socket/tcp_client.h"
#include "../socket/socket.h"
using namespace std;

void tcpip_test(char mode = 's')
{
    char ip[] = "127.0.0.1";//"66.154.108.47";//
    int port = 4141;
    LOG_INFO("server(0) or client(1)>>");

    if(mode == 's')   //1:Client
    {
//        tcp_server(port);
        socket_server(port);
    }
    else if(mode == 'c')//2.Server
    {
        LOG_INFO("ready to connect server:%s...",ip);
        tcp_client(ip,port);
    }
}

int getArgValue(int argc,const char* argv[],string&value,const char* key,const char *c = ":")
{
	size_t pos = 0;
	string str_key(key);
	string str_argv;
	string str_value;

	str_key.append(c);
	for (int i = 0; i < argc; i++)
	{
		str_argv.clear();
		str_argv.append(argv[i]);
		if (str_argv.find(str_key) != string::npos)
		{
			pos = str_argv.find_first_not_of(str_key);
			str_value.clear();
			str_value.append(str_argv.substr(pos));
			value.append(str_argv.substr(pos).c_str());
			break;
		}
	}
	return 0;
} 
int main(int argc,char*argv[])
{
	LOG_LEVEL_SET(argc, argv);
	
	int m = [](int x) { return [](int y) { return y * 2; }(x)+6; }(5);
	std::cout<<"m:"<<m<<std::endl;//输出m:16 
	std::cout << "n:" << [](int x, int y) { return x + y; }(5, 4) << std::endl;            //输出n:9

	tcpip_test(argv[1][0]);
}


