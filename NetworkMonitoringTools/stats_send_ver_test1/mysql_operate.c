#include "main.h"

int mysql_insert_host(struct HOST_INFO* host_info)
{
	MYSQL my_connection;
	char command[1024];

	mysql_init(&my_connection);

	if(mysql_real_connect(&my_connection, "localhost", "switch", "bjtungirc", NULL, 0, NULL, 0))
	{
		sprintf(command, "drop database stats_%s", host_name);
		mysql_query(&my_connection, command);
		sprintf(command, "create database stats_%s", host_name);

		if(mysql_query(&my_connection, command))
		{
			fprintf(stderr, "create database error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return -1;
		}

		sprintf(command, "use stats_%s", host_name);

		if(mysql_query(&my_connection, command))
		{
			fprintf(stderr, "use database error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return -1;
		}

		sprintf(command, "create table host_info(host_name varchar(%d), sysname varchar(%d),\
		 nodename varchar(%d), release_ver varchar(%d), version varchar(%d), machine varchar(%d), domain_name varchar(%d), port_amount int)",\
		32, _UTSNAME_SYSNAME_LENGTH, _UTSNAME_NODENAME_LENGTH, _UTSNAME_RELEASE_LENGTH, _UTSNAME_VERSION_LENGTH, _UTSNAME_MACHINE_LENGTH, _UTSNAME_DOMAIN_LENGTH);
	
		if(mysql_query(&my_connection, command))
		{
			fprintf(stderr, "create host_table error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return -1;
		}

		sprintf(command, "insert into host_info(host_name, sysname, nodename, release_ver, version, machine, domain_name, port_amount) \
			values('%s', '%s', '%s', '%s', '%s', '%s', '%s', %d)", \
			host_info->host_name, host_info->u_name.sysname, host_info->u_name.nodename, host_info->u_name.release, \
			host_info->u_name.version, host_info->u_name.machine, host_info->u_name.__domainname, host_info->port_count);

		if(mysql_query(&my_connection, command))
		{
			fprintf(stderr, "insert into host_table error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return -1;
		}

		sprintf(command, "create table port_index(_port_num int, _port_ip varchar(16))");

		if(mysql_query(&my_connection, command))
		{
			fprintf(stderr, "create table port_index error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return -1;
		}
	}
	else
	{
		printf("connect to mysql_host failed\n");
		mysql_close(&my_connection);
		return -1;
	}

	mysql_close(&my_connection);
	return 0;
}

int mysql_create_port_database(struct PORT_INFO* port_info, int* i)
{
	MYSQL my_connection;
	char command[1024];
	char database_name[50];
	char dev_mac[18];
	struct DEV_INFO* dev_info;

	dev_info = &port_info->dev_info;
	sprintf(database_name, "stats_%s", host_name);
	mysql_init(&my_connection);

	if(mysql_real_connect(&my_connection, "localhost", "switch", "bjtungirc", database_name, 0, NULL, 0))
	{
		sprintf(command, "drop table port_info_%d", *i);
		mysql_query(&my_connection, command);
		sprintf(command, "drop table port_stats_%d", *i);
		mysql_query(&my_connection, command);		

		sprintf(command, "create table port_info_%d(_dev_name varchar(16), _port_type smallint, _mac char(18), _ip varchar(16), _broad_addr varchar(16), _subnet_mask varchar(16), \
			_mtu int, _support int, _advertise int, _speed smallint, _duplex int, _port int, _transceiver int, _autoneg int)", *i);

		if(mysql_query(&my_connection, command))
		{
			fprintf(stderr, "create port_info_%s table error %d: %s\n", dev_info->dev_name, mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return -1;
		}

		snprintf(dev_mac, sizeof(dev_mac), "%02x:%02x:%02x:%02x:%02x:%02x\n",
               (unsigned char)dev_info->dev_mac[0],
               (unsigned char)dev_info->dev_mac[1],
               (unsigned char)dev_info->dev_mac[2],
               (unsigned char)dev_info->dev_mac[3],
               (unsigned char)dev_info->dev_mac[4],
               (unsigned char)dev_info->dev_mac[5]);

		sprintf(command, "insert into port_info_%d(_dev_name, _port_type, _mac, _ip, _broad_addr, _subnet_mask, _mtu, _support, _advertise, _speed, _duplex, _port, _transceiver, _autoneg) \
			values('%s', %d, '%s', '%s', '%s', '%s', %d, %d, %d, %d, %d, %d, %d, %d)", *i, dev_info->dev_name, port_info->port_type, \
			dev_mac, dev_info->dev_ip, dev_info->dev_broad_addr, dev_info->dev_subnet_mask, dev_info->mtu, \
			dev_info->adapter.supported, dev_info->adapter.advertising, dev_info->adapter.speed, \
			dev_info->adapter.duplex, dev_info->adapter.port, dev_info->adapter.transceiver, dev_info->adapter.autoneg);

		if(mysql_query(&my_connection, command))
		{
			fprintf(stderr, "insert port_info_%s table error %d: %s\n", dev_info->dev_name, mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return -1;
		}

		sprintf(command, "insert into port_index(_port_num, _port_ip) \
			values(%d, '%s')", *i, dev_info->dev_ip);

		if(mysql_query(&my_connection, command))
		{
			fprintf(stderr, "%s insert port_index table error %d: %s\n", dev_info->dev_name, mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return -1;
		}

		sprintf(command, "create table port_stats_%d(_time_id int, _date date, _time time, _connected smallint, _bandwidth float, \
			_total_packets bigint, _total_bytes bigint, _tx_packets bigint, _tx_bytes bigint, \
			_rx_packets bigint, _rx_bytes bigint, _total_packets_ps double, _total_bytes_ps double, _tx_packets_ps double, _tx_bytes_ps double, \
			_rx_packets_ps double, _rx_bytes_ps double, _packet_loss float, _rtt_avg float, _rtt_min float, _rtt_max float, _rtt_mdev float)", *i);

		if(mysql_query(&my_connection, command))
		{
			fprintf(stderr, "create table port_stats_%s table error %d: %s\n", port_info->dev_info.dev_name, mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return -1;
		}			
	}
	else
	{
		printf("connect to %s failed\n", database_name);
		mysql_close(&my_connection);
		return -1;
	}

	mysql_close(&my_connection);
	return 0;
}

int mysql_insert_port_stats(struct PORT_INFO* port_info, int* i)
{
	static int time_id = 0;
	MYSQL my_connection;
	char command[1024];
	char database_name[50];

	sprintf(database_name, "stats_%s", host_name);
	mysql_init(&my_connection);

	if(mysql_real_connect(&my_connection, "localhost", "switch", "bjtungirc", database_name, 0, NULL, 0))
	{
		if(time_id >= MAX_REC)
		{
			time_id = 0;
		}

		sprintf(command, "delete from port_stats_%d where _time_id = %d", *i, time_id);
		mysql_real_query(&my_connection, command, strlen(command));
		sprintf(command, "insert into port_stats_%d(_time_id, _date, _time, _connected, _bandwidth, \
			_total_packets, _total_bytes, _tx_packets, _tx_bytes, _rx_packets, _rx_bytes, \
			_total_packets_ps, _total_bytes_ps, _tx_packets_ps, _tx_bytes_ps, _rx_packets_ps, _rx_bytes_ps, \
			_packet_loss, _rtt_avg, _rtt_min, _rtt_max, _rtt_mdev) \
		values(%d, current_date(), current_time(), %d, %f, %lld, %lld, %lld, %lld, %lld, %lld, %lf, %lf, %lf, %lf, %lf, %lf, %f, %f, %f, %f, %f)", \
		*i, time_id, port_info->link_detect, port_info->efficiency.bandwidth, port_info->stats_info.total_packets, port_info->stats_info.total_bytes, \
		port_info->stats_info.tx_packets, port_info->stats_info.tx_bytes, port_info->stats_info.rx_packets, port_info->stats_info.rx_bytes, \
		port_info->stats_info.total_packets_ps, port_info->stats_info.total_bytes_ps, port_info->stats_info.tx_packets_ps, port_info->stats_info.tx_bytes_ps, \
		port_info->stats_info.rx_packets_ps, port_info->stats_info.rx_bytes_ps, \
		port_info->dest_info->link_stats.packet_loss, \
		port_info->dest_info->link_stats.rtt_avg, port_info->dest_info->link_stats.rtt_min, port_info->dest_info->link_stats.rtt_max, port_info->dest_info->link_stats.rtt_mdev);

		if(mysql_real_query(&my_connection, command, strlen(command)))
		{
			fprintf(stderr, "insert into port_stats_%s table error %d: %s\n", port_info->dev_info.dev_name, mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return -1;
		}
	}
	else
	{
		printf("connect to %s failed\n", database_name);
		mysql_close(&my_connection);
		return -1;
	}

	mysql_close(&my_connection);
	time_id++;
	return 0;
}

int mysql_get_port_num(char* ip)
{
	MYSQL_RES* result;
	MYSQL_ROW row;
	MYSQL my_connection;
	char command[1024];
	char database_name[50];
	int ret;

	result = (MYSQL_RES*)malloc(sizeof(MYSQL_RES));
	sprintf(database_name, "stats_%s", host_name);
	mysql_init(&my_connection);

	if(mysql_real_connect(&my_connection, "localhost", "switch", "bjtungirc", database_name, 0, NULL, 0))
	{
		sprintf(command, "select _port_num from port_index where _port_ip = '%s'", ip);
		
		if(mysql_real_query(&my_connection, command, strlen(command)))
		{
			fprintf(stderr, "%s get port_num error %d: %s\n", ip, mysql_errno(&my_connection), mysql_error(&my_connection));
			free(result);
			mysql_close(&my_connection);
			return -1;
		}

		result = mysql_store_result(&my_connection);

		if(result == NULL)
		{
			fprintf(stderr, "%s get port_num error %d: %s\n", ip, mysql_errno(&my_connection), mysql_error(&my_connection));
			free(result);
			mysql_close(&my_connection);
			return -1;
		}
		else
		{			
			row = mysql_fetch_row(result);

			if(row)
			{
				ret = atoi(*row);
			}
			else
			{
				printf("no such ip in port_index\n", ip);
				ret = -1;
			}
		}		
	}
	else
	{
		printf("connect to %s failed\n", database_name);
		free(result);
		mysql_close(&my_connection);
		return -1;
	}

	free(result);
	mysql_close(&my_connection);	
	return ret;	
}

void* a_to_num(char* input, int* type)
{
	long l = 0;
	double f = 0.0;
	float power = 1.0;
	void* ret;

	ret = (void*)malloc(8);

	while(*input != '\0')
	{
		l = l * 10 + (int)(*input) - 48;
		input++;

		if(*input == '.')
		{
			input++;
			*type = 1;
			break;
		}
	}

	if(*type)
	{
		while(*input != '\0')
		{
			f = f * 10 + (int)(*input) - 48;
			power *= 10.0; 
			input++;
		}

		f = f / power;
		f += l;
		memcpy(ret, &f, sizeof(double));
	}
	else
	{
		memcpy(ret, &l, sizeof(long));
	}

	return ret;
}

union _VALUE* mysql_fetch_point_stats(char* command_element, struct _TM* start, int* i, union _VALUE* _value)
{
	MYSQL_RES* result;
	MYSQL_ROW row;
	MYSQL my_connection;
	char command[1024];
	char database_name[50];
	char start_date[16];
	char _time1[16];
	char _time2[16];
	char _time3[16];
	int type = 0;
	void* ret_num;
	char input[16];

	sprintf(start_date, "%d-%d-%d", start->tm_year, start->tm_mon, start->tm_mday);
	
//设定时间偏差值(1s内)
	//请求的时间
	sprintf(_time2, "%d:%d:%d", start->tm_hour, start->tm_min, start->tm_sec);

	//请求时间前一秒
	if(start->tm_sec)
	{
		if(start->tm_min)
		{
			sprintf(_time1, "%d:59:59", start->tm_hour - 1);
		}
		else
		{
			sprintf(_time1, "%d:%d:59", start->tm_hour, start->tm_min - 1);
		}
	}
	else
	{
		sprintf(_time1, "%d:%d:%d", start->tm_hour, start->tm_min, start->tm_sec - 1);
	}

	//请求时间后一秒
	if(start->tm_sec == 59)
	{
		if(start->tm_min == 59)
		{
			sprintf(_time3, "%d:0:0", start->tm_hour + 1);
		}
		else
		{
			sprintf(_time3, "%d:%d:0", start->tm_hour, start->tm_min + 1);
		}
	}
	else
	{
		sprintf(_time3, "%d:%d:%d", start->tm_hour, start->tm_min, start->tm_sec + 1);
	}

	sprintf(database_name, "stats_%s", host_name);
	mysql_init(&my_connection);

	if(mysql_real_connect(&my_connection, "localhost", "switch", "bjtungirc", database_name, 0, NULL, 0))
	{
		sprintf(command, "select %s from port_stats_%d where _date = '%s' and (_time = '%s' or _time = '%s' or _time = '%s')", \
			command_element, *i, start_date, _time1, _time2, _time3);

		if(mysql_real_query(&my_connection, command, strlen(command)))
		{
			fprintf(stderr, "get %s error %d: %s\n", command_element, mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return _value;
		}
		else
		{
			result = mysql_store_result(&my_connection);

			if(result == NULL)
			{
				fprintf(stderr, "get %s result error %d: %s\n", command_element, \
					mysql_errno(&my_connection), mysql_error(&my_connection));
				mysql_close(&my_connection);
				return _value;
			}
			else
			{
				
				row = mysql_fetch_row(result);

				if(row)
				{
					printf("%s\n", *row);
					snprintf(input, sizeof(input), "%s", *row);
					ret_num = a_to_num(input, &type);

					if(type)
					{
						_value->f = *(double*)ret_num;
						printf("value = %lf\n", _value->f);
					}
					else
					{
						_value->l = *(long*)ret_num;
						printf("value = %lld\n", _value->l);
					}

					free(ret_num);
				}
				else
				{
					printf("no %s at %s\n", command_element, _time2);
				}
			}
		}
	}
	else
	{
		printf("connect to %s failed\n", database_name);
		mysql_close(&my_connection);
		return _value;
	}

	mysql_close(&my_connection);
	return _value;
}

union _VALUE* mysql_fetch_duration_stats(char* command_element, struct INFORM_GET_TAIL* inform_get_tail, int* i, union _VALUE* _value)
{
	MYSQL_RES* result;
	MYSQL_ROW row;
	MYSQL my_connection;
	char sub_command[32];
	char command[1024];
	char database_name[50];
	char start_date[16];
	char end_date[16];
	char start_time[16];
	char end_time[16];
	int type = 0;
	void* ret_num;
	char input[16];

	switch(inform_get_tail->content_type)
	{
		case 1:sprintf(sub_command, "avg(%s)", command_element);break;
		case 2:sprintf(sub_command, "max(%s)", command_element);break;
		case 3:sprintf(sub_command, "min(%s)", command_element);break;
		default:printf("no such content_type\n");return _value;
	}

	sprintf(start_date, "%d-%d-%d", inform_get_tail->start.tm_year, inform_get_tail->start.tm_mon, inform_get_tail->start.tm_mday);
	sprintf(end_date, "%d-%d-%d", inform_get_tail->end.tm_year, inform_get_tail->end.tm_mon, inform_get_tail->end.tm_mday);	
	//请求起始时间
	sprintf(start_time, "%d:%d:%d", inform_get_tail->start.tm_hour, inform_get_tail->start.tm_min, inform_get_tail->start.tm_sec);
	//请求结束时间
	sprintf(end_time, "%d:%d:%d", inform_get_tail->end.tm_hour, inform_get_tail->end.tm_min, inform_get_tail->end.tm_sec);	

	sprintf(database_name, "stats_%s", host_name);
	mysql_init(&my_connection);

	if(mysql_real_connect(&my_connection, "localhost", "switch", "bjtungirc", database_name, 0, NULL, 0))
	{
		if(strcmp(start_date, end_date) == 0)
		{
			sprintf(command, "select %s from port_stats_%d where _date = '%s' and (_time >= '%s' and _time <= '%s')", \
				sub_command, *i, start_date, start_time, end_time);
		}
		else
		{
			sprintf(command, "select %s from port_stats_%d where (_date = '%s' and _time >= '%s') or (_date = '%s' and _time <= '%s')", \
				sub_command, *i, start_date, start_time, end_date, end_time);
		}

		if(mysql_real_query(&my_connection, command, strlen(command)))
		{
			fprintf(stderr, "get %s error %d: %s\n", command_element, mysql_errno(&my_connection), mysql_error(&my_connection));
			mysql_close(&my_connection);
			return _value;
		}
		else
		{
			result = mysql_store_result(&my_connection);

			if(result == NULL)
			{
				fprintf(stderr, "get %s result error %d: %s\n", command_element, \
					mysql_errno(&my_connection), mysql_error(&my_connection));
				mysql_close(&my_connection);
				return _value;
			}
			else
			{
				
				row = mysql_fetch_row(result);

				if(row)
				{
					snprintf(input, sizeof(input), "%s", *row);
					ret_num = a_to_num(input, &type);

					if(type)
					{
						_value->f = *(double*)ret_num;
						printf("value = %lf\n", _value->f);
					}
					else
					{
						_value->l = *(long*)ret_num;
						printf("value = %lld\n", _value->l);
					}

					free(ret_num);
				}
				else
				{
					printf("no %s from %s,%s to %s,%s\n", sub_command, start_date, start_time, end_date, end_time);
				}
			}
		}
	}
	else
	{
		printf("connect to %s failed\n", database_name);
		mysql_close(&my_connection);
		return _value;
	}

	mysql_close(&my_connection);
	return _value;
}


/*
int main()
{
	MYSQL my_connection;
	int res;
	char command[1024];
	char ip[16];
	char mac[13];

	mysql_init(&my_connection);

	if(mysql_real_connect(&my_connection, "localhost", "switch", "cgnb1314", NULL, 0, NULL, 0))
	{
		printf("connect to mysql success\n");
		
		res = mysql_query(&my_connection, "drop database stats");
		res = mysql_query(&my_connection, "create database stats");
	
		if(res)
		{
			fprintf(stderr, "create database error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
			return -1;					
		}

		res = mysql_query(&my_connection, "use stats");
	
		if(res)
		{
			fprintf(stderr, "use database error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
			return -1;					
		}

		res = mysql_query(&my_connection, "create table eth0(ip varchar(16), mac varchar(13))");

		if(res)
		{
			//忽略表已存在的情况
			if(mysql_errno(&my_connection) != 1050)
			{
				fprintf(stderr, "create error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
				return -1;
			}
			else
			{
				if(mysql_query(&my_connection, "alter table eth0 change mac mac varchar(13)"))
				{
					fprintf(stderr, "change error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
					return -1;					
				}
			}
		}

		strcpy(ip, "192.168.78.101");
		strcpy(mac, "902b347abf53");
		sprintf(command, "insert into eth0(ip, mac) values('%s', '%s')", ip, mac);

		res = mysql_query(&my_connection, command);

		if(res)
		{
			fprintf(stderr, "insert error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
			return -1;			
		}
	}
	else
	{
		printf("connect to mysql failed\n");
		return -1;
	}

	while(1);
	mysql_close(&my_connection);

	return 0;
}
*/
