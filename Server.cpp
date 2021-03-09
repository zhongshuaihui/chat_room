#include "Server.h"
using namespace std;

Server::Server(){
	cout << "Server start..." << endl;
	listener = 0;
	epollfd = 0;

	// 初始化socket
	sockaddr_in server_address;
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);
	server_address.sin_port = htons(SERVER_PORT);

	listener = socket(PF_INET, SOCK_STREAM, 0);
	assert(listener >= 0);

	// 命名socket
	int ret = bind(listener, (struct sockaddr*) &server_address, sizeof(server_address));
	assert(ret != -1);

	// 监听socket
	ret = listen(listener, 5);
	assert(ret != -1);

	// 初始化epoll
	epollfd = epoll_create(EPOLL_SIZE);
	assert(epollfd != -1);
	addfd(epollfd, listener, true);

}

void Server::Start(){
	while(1){
		// 调用epoll_wait()
		int ret = epoll_wait(epollfd, events, EPOLL_SIZE, -1);
		if(ret < 0){
			perror("Epoll failure\n");
			break;
		}

		cout << "Epoll number = " << ret << endl;

		// 循环遍历就绪事件
		for(int i = 0; i < ret; i++){
			int sockfd = events[i].data.fd;

			// 监听socket就绪，表示有新连接
			if(sockfd == listener){
				sockaddr_in client_address;
				socklen_t client_add_len = sizeof(client_address);
				// 接受连接
				int clientfd = accept(listener, (struct sockaddr*) &client_address, &client_add_len);
				assert(clientfd != -1);

				// 使用pipe和splice()获取用户名

				cout << "New client connection from: " << inet_ntoa(client_address.sin_addr)
					 << ":" << ntohs(client_address.sin_port) << ", clientID = " << clientfd << endl;

				// 将新socket放入注册表
				addfd(epollfd, clientfd, true);
				// 将新client放入用户表中
				clients_list.push_back(clientfd);

				cout << "Add new client: clientID = " << clientfd << " to epoll" << endl;
				cout << "Now there are " << clients_list.size() << " clients in the chat room" << endl;

				Msg msg;
				char reply[BUF_SIZE];
				memset(&msg, 0, sizeof(msg));
				bzero(reply, BUF_SIZE);
				sprintf(msg.content, WELCOME_MESSAGE, clients_list.size(), clientfd);
				memcpy(reply, &msg, sizeof(msg));
				// 发送ack
				int ret = send(clientfd, reply, BUF_SIZE, 0);
				if(ret < 0){
					perror("Can't send ACK");
					Close_Server();
					exit(-1);
				}
			}
			else if(events[i].events & EPOLLIN){
				// 利用多线程处理IO
				int m = 0;
				thread new_thread(&Server::HandleMessage, this, sockfd, ref(m));
				new_thread.detach();
				// 收到消息，进行消息处理
				// int ret = HandleMessage(sockfd);
				if(m < 0){
					perror("Can't send message");
					Close_Server();
					exit(-1);
				}
			}
			else{
				perror("Something wrong.\n");
				Close_Server();
				exit(-1);
			}
		} 
	}
	Close_Server();
}

void Server::Close_Server(){
	close(listener);
	close(epollfd);
}

void Server::HandleMessage(int clientfd, int &ret){
	char recv_buff[BUF_SIZE];
	char send_buff[BUF_SIZE];
	Msg msg;

	cout << "Read from clientID = " << clientfd << endl;
	// 接收消息
	int len = recv(clientfd, recv_buff, BUF_SIZE, 0);
	memset(&msg, 0, sizeof(msg));
	memcpy(&msg, recv_buff, sizeof(msg));

	// 设置msg
	msg.fromID = clientfd;
	if(msg.content[0] == '\\' && isdigit(msg.content[1])){
		msg.type = 1;
		msg.toID = msg.content[1] - '0';
		memcpy(msg.content, msg.content+2, sizeof(msg.content));
	}
	else
		msg.type = 0;

	// 断开连接
	if(len == 0){
		close(clientfd);

		clients_list.remove(clientfd);
		cout<<"ClientID=" << clientfd << " closed." << endl
			<< "Now we have " << clients_list.size() << " in our chat room"
			<< endl <<endl;
	}
	else{
		// 当仅有一人时，不转发消息
		if(clients_list.size() == 1){
			memcpy(&msg.content, CAUTION, sizeof(msg.content));
            bzero(send_buff, BUF_SIZE);
            memcpy(send_buff, &msg, sizeof(msg));
            send(clientfd, send_buff, sizeof(send_buff), 0);
            ret = len;
		}
		// 公共消息，转发给除本人以外的所有人
		char format_message[BUF_SIZE];
		if(msg.type == 0){
			sprintf(format_message, MESSAGE, clientfd, msg.content);
			memcpy(msg.content, format_message, BUF_SIZE);
			list<int>::iterator it;
			for(it = clients_list.begin(); it != clients_list.end(); it++){
				if(*it != clientfd ){
					bzero(send_buff, BUF_SIZE);
					memcpy(send_buff, &msg,sizeof(msg));
					if(send(*it, send_buff, sizeof(send_buff),0) < 0 ){
						ret = -1;
					}
				}
			}
		}
		// 当是私有信息时
		else if(msg.type == 1){
			bool private_offline = true;
			sprintf(format_message, PRIVATE_MESSAGE, clientfd, msg.content);
			memcpy(msg.content, format_message, BUF_SIZE);

			list<int>::iterator it;
			for(it = clients_list.begin(); it != clients_list.end(); it++){
				// 找到目标主机
				if(*it == msg.toID){
					private_offline = false;
					bzero(send_buff, BUF_SIZE);
					memcpy(send_buff, &msg, sizeof(msg));
					if(send(*it, send_buff, sizeof(send_buff), 0) < 0 ){
						ret = -1;
					}
				}
			}

			// 未找到目标主机
			if(private_offline){
				sprintf(format_message, PRIVATE_ERROR_MESSAGE, msg.toID);
				memcpy(msg.content, format_message, BUF_SIZE);
				bzero(send_buff, BUF_SIZE);
				memcpy(send_buff, &msg, sizeof(msg));
				if(send(msg.fromID, send_buff, sizeof(send_buff),0) < 0 ){
					ret = -1;
				}
			}
			
		}
	}
	ret = len;
}