#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

int epoll_fd;

int main()
{
	int server_sockfd = -1;
	int client_sockfd = -1;
	int client_len = 0;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	char Recv_buf[1024];
	char Send_buf[1024];
	int recv_len;

	epoll_fd = epoll_create1(0);

	if(epoll_fd)

	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(6677);

	bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	listen(server_sockfd, 20);
	//忽略子进程停止和退出信号，第二个参数为ignore
	signal(SIGCHLD, SIG_IGN);

	while(1)
	{
		printf("waiting for client\n");
		client_len = sizeof(client_addr);
		client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_len);

		//创建子进程处理新的连接
		if(fork() == 0)
		{
			recv_len = read(client_sockfd, Recv_buf, sizeof(Recv_buf));
			printf("recv from client: %s\n", Recv_buf);
			sprintf(Send_buf, "yep %c!", Recv_buf[recv_len - 1 ]);
			write(client_sockfd, Send_buf, strlen(Send_buf));
			close(client_sockfd);
			exit(0);
		}
		else
		{
			//关闭父进程中的client_sockfd，交由子进程处理
			close(client_sockfd);
		}
	}

	return 0;
}