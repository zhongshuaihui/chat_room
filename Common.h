#ifndef  CHATROOM_COMMON_H
#define CHATROOM_COMMON_H

#include <iostream>
#include <list>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

// 服务器IP地址
#define SERVER_IP "127.0.0.1"
// 服务器端口号
#define SERVER_PORT 8889
// epoll最大监听数量
#define EPOLL_SIZE 100
// 缓冲区大小
#define BUF_SIZE 1024
// 新用户到来
#define WELCOME_MESSAGE "Welcome to the Night City! There are %d person in the chat room. Your ID is: Client %d"
// 消息前缀
#define MESSAGE "(Public) Client %d >> %s"
#define PRIVATE_MESSAGE "(Private) Client %d >> %s"
#define PRIVATE_ERROR_MESSAGE "Client %d is not in the chat room."
// 退出聊天室
#define EXIT "EXIT"
// 当你是唯一一人时发出提醒
#define CAUTION "You are the only person in the chat room!"

// 设置socket为非阻塞模式
static int setnonblocking(int fd){
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

// 向epollfd注册新的fd，enable_ET表示是否开启ET模式
static void addfd(int epollfd, int fd, bool enable_ET){
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN;
	if(enable_ET){
		event.events |= EPOLLET;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
	printf("fd added to epoll!\n\n");
}

struct Msg
{
	int type; // 0代表公共消息，1代表私密消息
	int fromID;
	int toID;
	char content[BUF_SIZE];
	
};


#endif 