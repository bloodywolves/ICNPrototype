#include "main.h"
/*总包数（1<<0），进包数（1<<1），出包数（1<<2），总流量(1<<3），
进流量（1<<4），出流量（1<<5），时延(1<<6)，丢包率（1<<7）*/
#define TOTAL_PACKETS	(1 << 0)
#define RX_PACKETS		(1 << 1)
#define TX_PACKETS		(1 << 2)
#define TOTAL_BYTES		(1 << 3)
#define RX_BYTES		(1 << 4)
#define TX_BYTES		(1 << 5)
#define RTT_AVG			(1 << 6)
#define PACKETS_LOSS	(1 << 7)
#define BAND_WIDTH		(1 << 8)

char _ip[] = "172.168.6.1";
char _information[] = "rtt_avg";
char _type[] = "peak";
char _start_time[] = "2015/6/10,21:15:15";
char _end_time[] = "2015/6/10,21:16:15";

int send_message(void* send_buf, int space_remain)
{
	int sendfd;
	int send_len;
	struct sockaddr_in addr;
	struct INFORM_GET_HEAD* p;
	int* send_type;

	send_type = (int*)send_buf;
//	parse_inform_message(send_type);
	
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
   	send_len = 1024 - space_remain;

   	sendto(sendfd, send_buf, send_len, 0, (struct sockaddr*)&addr, sizeof(addr));
   	printf("send_buf\n");

   	close(sendfd);
	return 0;
}

int set_request(uint32_t* request, char* element)
{
	if(strcasecmp(element, "TOTAL_PACKETS") == 0)
	{
		*request = *request ^ TOTAL_PACKETS;
	}
	else if(strcasecmp(element, "RX_PACKETS") == 0)
	{
		*request = *request ^ RX_PACKETS;
	}
	else if(strcasecmp(element, "TX_PACKETS") == 0)
	{
		*request = *request ^ TX_PACKETS;
	}
	else if(strcasecmp(element, "TOTAL_BYTES") == 0)
	{
		*request = *request ^ TOTAL_BYTES;
	}
	else if(strcasecmp(element, "RX_BYTES") == 0)
	{
		*request = *request ^ RX_BYTES;
	}
	else if(strcasecmp(element, "TX_BYTES") == 0)
	{
		*request = *request ^ TX_BYTES;
	}
	else if(strcasecmp(element, "RTT_AVG") == 0)
	{
		*request = *request ^ RTT_AVG;
	}
	else if(strcasecmp(element, "PACKETS_LOSS") == 0)
	{
		*request = *request ^ PACKETS_LOSS;
	}
	else
	{
		printf("no such information %s\n", element);
		return 0;
	}

	return 1;
}


struct _TM insert_time(char* str)
{
	int num = 0;
	int i = 0;
	struct _TM t;

    //获取年
    while(str[i] != '/')
    {
        num = num * 10 + (int)str[i] - 48;
        i++;
    }

    t.tm_year = num;
    num = 0;
    i++;       
    // printf("year: %d\n", t.tm_year);

    //获取月
    while(str[i] != '/')
    {
        num = num * 10 + (int)str[i] - 48;
        i++;
    }

    t.tm_mon = num;
    num = 0;
    i++;
    // printf("mon: %d\n", t.tm_mon);

    //获取日
    while(str[i] != ',')
    {
        num = num * 10 + (int)str[i] - 48;
        i++;
    }

    t.tm_mday = num;
    num = 0;
    i++;
    // printf("day: %d\n", t.tm_mday);

    //获取时
    while(str[i] != ':')
    {
        num = num * 10 + (int)str[i] - 48;
        i++;
    }

    t.tm_hour = num;
    num = 0;
    i++;
    // printf("hour: %d\n", t.tm_hour);

    //获取分
    while(str[i] != ':')
    {
        num = num * 10 + (int)str[i] - 48;
        i++;
    }

    t.tm_min = num;
    num = 0;
    i++;
    // printf("min: %d\n", t.tm_min);

    //获取秒
    while(str[i] != '\0')
    {
        num = num * 10 + (int)str[i] - 48;
        i++;
    }

    t.tm_sec = num;
    // printf("sec: %d\n", t.tm_sec);

    return t;
}


void* insert_inform_get_tail(int count, struct INFORM_GET_TAIL* inform_get_tail, void* ptr)
{
	char tstr[1024];
	//标记插入的是起始时间(0),还是结束时间(1)
	int type;
	printf("insert inform get tail\n");

	while(count--)
	{
		/*
		printf("what kind of information you want to get, you can choose the information below\n\t\
			point: 某个时间点的数据\n\t\
			average: 某段时间的均值\n\t\
			peak: 某段时间的峰值\n\t\
			nadir：某段时间的谷值\n");*/
		//gets(tstr);
		strcpy(tstr, _type);

		if(strcmp(tstr, "point") == 0)
		{
			type = 0;
			inform_get_tail->content_type = 0;
		}
		else
		{
			type = 1;
			if(strcmp(tstr, "average") == 0)
			{
				inform_get_tail->content_type = 1;
			}
			else if(strcmp(tstr, "peak") == 0)
			{
				inform_get_tail->content_type = 2;
			}
			else if(strcmp(tstr, "nadir") == 0)
			{
				inform_get_tail->content_type = 3;
			}
			else
			{
				printf("no such information %s\n", tstr);
				//count++;
				continue;
			}
		}

		strcpy(tstr, _start_time);
		inform_get_tail->start = insert_time(tstr);

		if(type)
		{
			strcpy(tstr, _end_time);
			inform_get_tail->end = insert_time(tstr);		
		}

		memcpy(ptr, inform_get_tail, sizeof(struct INFORM_GET_TAIL));
		ptr += sizeof(struct INFORM_GET_TAIL);
		memset(inform_get_tail, 0, sizeof(struct INFORM_GET_TAIL));
		
	}

	return ptr;
}

int main(int argc, char** argv)
{
	char itstr[1024];
	int itnum;
	int space_remain = 1024;
	int ret = 0;
	int n, i, j, count, port_count = 0;
	char element[16];
	void* send_buf;
	void* ptr;
	int* send_type;
	struct INFORM_GET_HEAD* modify_head;
	struct INFORM_GET_HEAD* inform_get_head;
	struct INFORM_GET_MIDDLE* inform_get_middle;
	struct INFORM_GET_TAIL* inform_get_tail;
	char host_name[32];
	int head_type = 3;

	send_buf = (void*)calloc(1024, sizeof(char));
	inform_get_head = (struct INFORM_GET_HEAD*)malloc(sizeof(struct INFORM_GET_HEAD));
	inform_get_middle = (struct INFORM_GET_MIDDLE*)malloc(sizeof(struct INFORM_GET_MIDDLE));
	inform_get_tail = (struct INFORM_GET_TAIL*)malloc(sizeof(struct INFORM_GET_TAIL));

	fprintf(stdout, "input the switch you want to inform\n");
	gets(itstr);
	strcpy(inform_get_head->host_name, itstr);
	fprintf(stdout, "how many ports you want to inform\n");
	gets(itstr);
	itnum = atoi(itstr);
	n = itnum;
	inform_get_head->count = itnum;

	//封装头部类型
	memcpy(send_buf, &head_type, sizeof(int));
	ptr = send_buf + sizeof(int);
	memcpy(ptr, inform_get_head, sizeof(struct INFORM_GET_HEAD));
	ptr += sizeof(struct INFORM_GET_HEAD);
	space_remain -= (sizeof(struct INFORM_GET_HEAD) + sizeof(int));

	while(n--)
	{
		port_count++;
		memset(inform_get_middle, 0, sizeof(struct INFORM_GET_MIDDLE));
	//	printf("input the port ip you want to inform\n");
	//	gets(itstr);
		strcpy(itstr, _ip);
		strcpy(inform_get_middle->ip, itstr);
	/*
		printf("input all the information you want to get, separate by space. you can choose infromation below, case insensitive\t\n\
			TOTAL_PACKETS\t\n\
			RX_PACKETS\t\n\
			TX_PACKETS\t\n\
			TOTAL_BYTES\t\n\
			RX_BYTES\t\n\
			TX_BYTES\t\n\
			RTT_AVG\t\n\
			PACKETS_LOSS\n");
	*/
		//gets(itstr);
		strcpy(itstr, _information);
		i = 0;
		j = 0;
		count = 0;

		while(itstr[j] != '\0')
		{
			if(itstr[j] == ' ')
			{
				element[i] = '\0';
				ret = set_request(&inform_get_middle->request, element);
				count += ret;
				i = 0;
				j++;
				continue;
			}

			element[i] = itstr[j];	
			i++;
			j++;
		}

		element[i] = '\0';
		ret = set_request(&inform_get_middle->request, element);
		inform_get_middle->request = inform_get_middle->request;
		count += ret;
		space_remain -= (sizeof(struct INFORM_GET_MIDDLE) + count * sizeof(struct INFORM_GET_TAIL));
		printf("space remain %d\n", space_remain);

		if(space_remain < 0)
		{
			send_type = (int*)send_buf;
			modify_head = (struct INFORM_GET_HEAD*)(send_type + 1);
			modify_head->count = port_count - 1;
			inform_get_head->count -= modify_head->count;
			space_remain += (sizeof(struct INFORM_GET_MIDDLE) + count * sizeof(struct INFORM_GET_TAIL));
			send_message(send_buf, space_remain);
			memset(send_buf, 0, sizeof(send_buf));
			port_count = 1;
			space_remain = 1024;

			//封装头部类型
			memcpy(send_buf, &head_type, sizeof(int));
			ptr = send_buf + sizeof(int);
			memcpy(ptr, inform_get_head, sizeof(struct INFORM_GET_HEAD));
			ptr += sizeof(struct INFORM_GET_HEAD);
			memcpy(ptr, inform_get_middle, sizeof(struct INFORM_GET_MIDDLE));
			ptr += sizeof(struct INFORM_GET_MIDDLE);
			ptr = insert_inform_get_tail(count, inform_get_tail, ptr);
			space_remain -= (sizeof(struct INFORM_GET_HEAD) + sizeof(struct INFORM_GET_MIDDLE) + count * sizeof(struct INFORM_GET_TAIL) + sizeof(int));
		}
		else
		{
			memcpy(ptr, inform_get_middle, sizeof(struct INFORM_GET_MIDDLE));
			ptr += sizeof(struct INFORM_GET_MIDDLE);
			ptr = insert_inform_get_tail(count, inform_get_tail, ptr);
		}
	}

	send_message(send_buf, space_remain);
	return 0;
}