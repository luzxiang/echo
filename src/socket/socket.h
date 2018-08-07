/*
 * socket.h
 *
 *  Created on: 2018年7月25日
 *      Author: root
 */

#ifndef SOCKET_SOCKET_H_
#define SOCKET_SOCKET_H_
#include <deque>

void socket_create_server(int);

class Socket{
private:
	Socket();
	~Socket();
	std::deque <string> SendBuf;
	std::deque <string> RecvBuf;
public:
	int set_socketopt(int);
	int socket_create(int);
	int socket_accept(int);
	int select_send(int);
	void select_recv(int);
	void socket_create_server(int);

	int socket_select();
};


#endif /* SOCKET_SOCKET_H_ */

















