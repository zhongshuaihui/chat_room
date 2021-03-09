#ifndef CHATROOM_SERVER_H
#define CHATROOM_SERVER_H

#include "Common.h"
using namespace std;

class Server
{
public:
	Server();
	void Start();
	void Close_Server();

private:
	void HandleMessage(int fd, int &ret);
	int listener;
	int epollfd;
	list<int> clients_list;
	epoll_event events[EPOLL_SIZE];
	
};

#endif