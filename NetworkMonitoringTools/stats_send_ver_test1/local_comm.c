#include "main.h"

struct _timer_info* parse_timer_message(int* recv_type, struct _timer_info* head, struct PORT_INFO* port_info)
{
	int i;
	int* count;
	struct TIMER_MESSAGE* timer_message;

	count = (int*)(recv_type + 1);
	timer_message = (struct TIMER_MESSAGE*)(count + 1);

	for(i = 0; i < (*count); i++)
	{

		printf("ip = %s\n\t", timer_message->ip);
		
		if(timer_message->flag)
		{
			printf("end time : ");
		}
		else
		{
			printf("start time : ");
		}

		printf("%d/%d/%d,%d:%d:%d\n", timer_message->t.tm_year + 1900, timer_message->t.tm_mon + 1, timer_message->t.tm_mday, timer_message->t.tm_hour, timer_message->t.tm_min, timer_message->t.tm_sec);
//		timer_message++;

		head = set_child_timer(head, timer_message, port_info);
		timer_message++;
	}

	return head;
}

union _VALUE* fetch_inform_data(struct INFORM_GET_TAIL* inform_get_tail, char* command_element, int* port_num)
{
	union _VALUE* _value;

	_value = (union _VALUE*)malloc(sizeof(union _VALUE));
	_value->l = -1;

	if(*port_num < 0)
	{
		return _value;
	}

	printf("fetch_inform_data\n");
/*
    printf("inform element: %s\n", command_element);
	printf("content_type = %d\n", inform_get_tail->content_type);
	printf("start time: %d/%d/%d,%d:%d:%d\n", inform_get_tail->start.tm_year, inform_get_tail->start.tm_mon, inform_get_tail->start.tm_mday, inform_get_tail->start.tm_hour, inform_get_tail->start.tm_min, inform_get_tail->start.tm_sec);
	printf("end time: %d/%d/%d,%d:%d:%d\n", inform_get_tail->end.tm_year, inform_get_tail->end.tm_mon, inform_get_tail->end.tm_mday, inform_get_tail->end.tm_hour, inform_get_tail->end.tm_min, inform_get_tail->end.tm_sec);
*/

	if(inform_get_tail->content_type)
	{
//		_value = mysql_fetch_duration_stats(command_element, inform_get_tail, port_num, _value);
	}
	else
	{
//		_value = mysql_fetch_point_stats(command_element, &inform_get_tail->start, port_num, _value);
	}

	return _value;
}

void* parse_inform_message(int* recv_type)
{
	static struct option inform_element[] = 
	{
		{"_total_packets",},
		{"_rx_packets",},
		{"_tx_packets",},
		{"_total_bytes",},
		{"_rx_bytes",},
		{"_tx_bytes",},
		{"_rtt_avg",},
		{"_packet_loss"}
	};

	void* send_buf;
	void* ptr;
	struct INFORM_GET_HEAD* inform_get_head;
	struct INFORM_GET_MIDDLE* inform_get_middle;
	struct INFORM_GET_TAIL* inform_get_tail;
	union _VALUE* _value;
	int i;
	int count = 0;
	int j;
	uint32_t request;
	char command_element[128];
	int port_num;

	send_buf = (void*)calloc(1280, sizeof(char));
	inform_get_head = (struct INFORM_GET_HEAD*)(recv_type + 1);
	memcpy(send_buf, inform_get_head, sizeof(struct INFORM_GET_HEAD));
	ptr = send_buf + sizeof(struct INFORM_GET_HEAD);	
	inform_get_middle = (struct INFORM_GET_MIDDLE*)(inform_get_head + 1);
	memcpy(ptr, inform_get_middle, sizeof(struct INFORM_GET_MIDDLE));
	ptr += sizeof(struct INFORM_GET_MIDDLE);
	
	printf("port_count = %d\n", inform_get_head->count);
	//请求的端口个数
	for(i = 0; i < inform_get_head->count; i++)
	{
		printf("port_ip : %s\n", inform_get_middle->ip);
		//port_num = mysql_get_port_num(inform_get_middle->ip);

		printf("port_num = %d\n", port_num);

		if(port_num < 0)
		{
			printf("get port number failed\n");	
		}

		j = 0;
		request = inform_get_middle->request;
		printf("%d\n", request);
		inform_get_tail = (struct INFORM_GET_TAIL*)(inform_get_middle + 1);

		//请求的内容个数
		while(request)
		{
			if(request & 1)
			{
				memcpy(ptr, inform_get_tail, sizeof(struct INFORM_GET_TAIL));
				ptr += sizeof(struct INFORM_GET_TAIL);	
				strcpy(command_element, inform_element[j].name);
				_value = fetch_inform_data(inform_get_tail, command_element, &port_num);
				//输入实际值，若没有获取正确的值，则输入值为-1,判断工作交给RM
				memcpy(ptr, _value, sizeof(union _VALUE));
				ptr += sizeof(union _VALUE);
				free(_value);

				inform_get_tail++;
				count++;
			}

			request >>= 1;
			j++;
		}

		inform_get_middle = (struct INFORM_GET_MIDDLE*)inform_get_tail;
		memcpy(ptr, inform_get_middle, sizeof(struct INFORM_GET_MIDDLE));
		ptr += sizeof(struct INFORM_GET_MIDDLE);		
	}	

	return send_buf;
}

void* local_listen(int* recvfd, struct sockaddr_in* sin, struct _timer_info* timer_info, struct PORT_INFO* port_info, int* type)
{
	char recv_buf[1024];
	int recv_len;
	int* recv_type;
    struct _timer_info* head;
    int sin_len;
    int* ret_ack;
    void* inform_data;

//    timer_info = (struct _timer_info*)malloc(sizeof(struct _timer_info));
//    timer_info = NULL;
    sin_len = sizeof(struct sockaddr_in);
    head = timer_info;
	recv_len = recvfrom(*recvfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)sin, &sin_len);
	recv_type = (int*)recv_buf;
	printf("recv_packet with type %d\n", *recv_type);

	switch(*recv_type)
	{
		//收到RM发送的定时操作端口请求
		case 1:{
				*type = 1;
				head = parse_timer_message(recv_type, head, port_info);
				return head;
			}
		//收到RM发送的inform-ack(data)
		case 2:{
				*type = 2;
				ret_ack = (int*)(recv_type + 1);
				return ret_ack;
			}
		//收到RM发送的inform-get信息
		case 3:{
				*type = 3;
				inform_data = parse_inform_message(recv_type);
				return inform_data;
			}
		default:break;
	}
	
	return NULL;
}