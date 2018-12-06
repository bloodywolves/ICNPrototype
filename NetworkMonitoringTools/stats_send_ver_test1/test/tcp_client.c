#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	int sockfd = -1;
	int len = 0;
	struct sockaddr_in addr;
	int ret;
	char Send_buf[1024];
	char Recv_buf[1024];
	int i = 0;

	addr.sin_family = AF_INET;
	//服务器的地址和端口
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(6677);
	len = sizeof(addr);

	while(1)
	{
		//sleep(2);
		memset(Recv_buf, '\0', sizeof(Recv_buf));	
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		ret = connect(sockfd, (struct sockaddr*)&addr, len);
		
		if(ret == -1)
		{
			perror("client connect error");
			continue;
		}

		sprintf(Send_buf, "hello %d", i);
		i++;
		write(sockfd, Send_buf, strlen(Send_buf));
		//获取服务器端回送
		read(sockfd, Recv_buf, sizeof(Recv_buf));
		printf("data from server: %s\n", Recv_buf);
		close(sockfd);
	}

	return 0;
}