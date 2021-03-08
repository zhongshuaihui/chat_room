#include "Client.h"
using namespace std;

Client::Client(){
	sockaddr_in server_address;
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);
	server_address.sin_port = htons(SERVER_PORT);

	sock = 0;
	pid = 0;
	atWork = true;
	epollfd = 0;

	cout << " Connect server: " << SERVER_IP << " : " << SERVER_PORT << endl;

	sock = socket(PF_INET, SOCK_STREAM,0);
	assert(sock >= 0);

	if(connect(sock, (struct sockaddr*) &server_address, sizeof(server_address)) < 0 ){
		perror("connect error");
		exit(-1);
	}

	if(pipe(pipe_fd) < 0){
		perror("pipe error");
		exit(-1);
	}

	epollfd = epoll_create(5);
	assert(epollfd != -1);

	addfd(epollfd, sock, true);
	addfd(epollfd, pipe_fd[0], true);

}

void Client::Close_Client(){
	// 父进程
	if(pid){
		close(pipe_fd[0]);
		close(epollfd);
	}
	else{
		close(pipe_fd[1]);
	}
}

void Client::Start(){
	pid = fork();
	assert(pid != -1);

	// 子进程
	if(pid == 0){
		// 关闭管道的读端
		close(pipe_fd[0]);
		cout<<"Please input 'exit' to exit the chat room."<<endl;
		cout<<"\\ + ClientId to private chat."<<endl;

		while(atWork){
			memset(msg.content, 0, sizeof(msg.content));
			fgets(msg.content, BUF_SIZE, stdin);
			// 输入EXIT，strncasecmp会自动忽略大小写
			if(strncasecmp(msg.content, EXIT, strlen(EXIT)) == 0){
				atWork = false;
			}
			// 将文本写进管道
			else{
				memset(send_buff, 0, BUF_SIZE);
				memcpy(send_buff, &msg, sizeof(msg));
				int ret = write(pipe_fd[1], send_buff, sizeof(send_buff));
				assert(ret != -1);
			}
		}
	}
	// 父进程
	else{
		// 关闭管道的写端
		close(pipe_fd[1]);
		while(atWork){
			int ret = epoll_wait(epollfd, events, 2, -1);
			if(ret < 0){
				perror("Epoll failure\n");
				break;
			}

			// 循环遍历就绪事件
			for(int i = 0; i < ret; i++){
				int sockfd = events[i].data.fd;
				memset(recv_buff, 0, sizeof(recv_buff));

				// 连接socket有数据返回
				if(sockfd == sock){
					int len = recv(sock, recv_buff, BUF_SIZE, 0);
					memset(&msg, 0, sizeof(msg));
					memcpy(&msg, recv_buff, sizeof(msg));
					// 连接关闭
					if(len == 0){
						cout<<"Server closed connection: "<<sock<<endl;
						close(sock);
						atWork = false;
					}
					// 输出返回值，即他人的消息
					else{
						cout << msg.content << endl;
					}
				}
				// 管道读端就绪
				else{
					int len = read(pipe_fd[0], recv_buff, BUF_SIZE);
					assert(ret != -1);
					// EOF
					if(len == 0){
						atWork = false;
					}
					// 发送数据
					else{
						if(send(sock, recv_buff, sizeof(recv_buff), 0) < 0){
							perror("Send error");
						} 
					}
				}
			}
		}
	}
	Close_Client();
}