#ifndef CHATROOM_CLIENT_H
#define CHATROOM_CLIENT_H

#include "Common.h"
using namespace std;

class Client
{
public:
	Client();
	void Start();
	void Close_Client();

private:
	int sock;
	int pid;
	int epollfd;
	int pipe_fd[2];
	bool atWork;
	epoll_event events[EPOLL_SIZE];

	Msg msg;
	char send_buff[BUF_SIZE];
	char recv_buff[BUF_SIZE];
	
};

#endif