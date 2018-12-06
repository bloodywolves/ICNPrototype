#include "main.h"

struct SEND_BUF
{
    int type;
	int count;
	struct TIMER_MESSAGE timer_message[64];
};

struct SEND_BUF send_buf;

static int get_timer_message(FILE *fp)
{
	int i, num, len, j;
	char str[1024];
	char* ip;

	j = 0;

	while(1)
    {
        i = 0;
        num = 0;
        fgets(str, 1024, fp);

        printf("%s", str);
        len = strlen(str);

        //获取启动终止标识
        while(str[i] != ' ')
        {
            num = num * 10 + (int)str[i] - 48;
            i++;
        }

        send_buf.timer_message[j].flag = num;
        num = 0;
        i++;
        printf("flag: %d\n", send_buf.timer_message[j].flag);

        //获取端口ip
        while(str[i] != ' ')
        {
        	send_buf.timer_message[j].ip[i - 2] = str[i];
        	i++;
        }

        send_buf.timer_message[j].ip[i - 2] = '\0';
        i++;
        printf("ip: %s\n", send_buf.timer_message[j].ip);

        //获取年
        while(str[i] != '/')
        {
            num = num * 10 + (int)str[i] - 48;
            i++;
        }

        send_buf.timer_message[j].t.tm_year = num - 1900;
        num = 0;
        i++;       
        printf("year: %d\n", send_buf.timer_message[j].t.tm_year);

        //获取月
        while(str[i] != '/')
        {
            num = num * 10 + (int)str[i] - 48;
            i++;
        }

        send_buf.timer_message[j].t.tm_mon = num - 1;
        num = 0;
        i++;
        printf("mon: %d\n", send_buf.timer_message[j].t.tm_mon);

        //获取日
        while(str[i] != ',')
        {
            num = num * 10 + (int)str[i] - 48;
            i++;
        }

        send_buf.timer_message[j].t.tm_mday = num;
        num = 0;
        i++;
        printf("day: %d\n", send_buf.timer_message[j].t.tm_mday);

        //获取时
        while(str[i] != ':')
        {
            num = num * 10 + (int)str[i] - 48;
            i++;
        }

        send_buf.timer_message[j].t.tm_hour = num;
        num = 0;
        i++;
        printf("hour: %d\n", send_buf.timer_message[j].t.tm_hour);

        //获取分
        while(str[i] != ':')
        {
            num = num * 10 + (int)str[i] - 48;
            i++;
        }

        send_buf.timer_message[j].t.tm_min = num;
        num = 0;
        i++;
        printf("min: %d\n", send_buf.timer_message[j].t.tm_min);

        //获取秒
        while(str[i] != '\n')
        {
            num = num * 10 + (int)str[i] - 48;
            i++;

            if(i >= len)
            {
                break;
            }
        }

        send_buf.timer_message[j].t.tm_sec = num;
        num = 0;
        printf("sec: %d\n", send_buf.timer_message[j].t.tm_sec);

        fgetc(fp);
        if(feof(fp))
        {
            break;
        }

        fseek(fp,-1,SEEK_CUR);
        j++;
    }

    j++;
    return j;

}

int main(int argc, char **argv)
{
	int sendfd;
	int ret, send_len;
	struct sockaddr_in addr;

	FILE *fp;

	memset(&send_buf, 0, sizeof(send_buf));
	fp = fopen("timer.lst","r+");

    send_buf.type = 1;
	send_buf.count = get_timer_message(fp);

    fclose(fp);
    printf("number of timer is %d\n", send_buf.count);

   	sendfd = socket(AF_INET, SOCK_DGRAM, 0);

   	if(sendfd < 0)
   	{
   		perror("create socket failed");
   		return -1;
   	}

   	bzero(&addr, sizeof(addr));
   	addr.sin_family = AF_INET;
   	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
   	addr.sin_port = htons(LOCAL_PORT);
   	send_len = sizeof(int) + send_buf.count * sizeof(struct TIMER_MESSAGE);

   	sendto(sendfd, &send_buf, send_len, 0, (struct sockaddr*)&addr, sizeof(addr));

   	close(sendfd);

    return 0;
}