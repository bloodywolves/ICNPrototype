#include "main.h"
#include "trap.h"
/*
//udp发送模式
static int send_trap_message(struct TRAP_MESSAGE *trap_message)
{
	//socket发送trap信息
	int socket_stats_sender;
	struct sockaddr_in address;
	struct TRAP_MESSAGE stats_buf;
	
	stats_buf = *trap_message;
	bzero(&address,sizeof(address));
	address.sin_family=AF_INET;
	address.sin_addr.s_addr=inet_addr(RM_ADDRESS);
	address.sin_port=htons(TRAP_PORT);
	
	if((socket_stats_sender=socket(AF_INET,SOCK_DGRAM,0))<0)
	{
		perror("creat socket_stats_sender failed");
		exit(0);
	}
	sendto(socket_stats_sender,&stats_buf,sizeof(stats_buf),0,(struct sockaddr *)&address,sizeof(address));
	printf("send the message successfully\n\n\n\n");
	close(socket_stats_sender);
	return(EXIT_SUCCESS);
	return 0;
}
*/

int read_trap_file(struct TRAP_MESSAGE *trap_message)
{
	struct TRAP_MESSAGE buf;
	char file_fd[200];
	int ret = 0;
	struct flock lock;
	int read_fd;

	memset(&lock,0,sizeof(lock));
	memset(&buf,0,sizeof(buf));
	sprintf(file_fd,"%s/log/temp_trap_%s",port_index->dir_name,trap_message->trap_sbd.port_ip);

	lock.l_type = F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	read_fd = open(file_fd,O_RDONLY);

	if(read_fd < 0)
	{
		printf("open read %s failed\n",file_fd);
		return -1;
	}

	ret = fcntl(read_fd,F_SETLKW,&lock);
	
	if(ret == -1)
	{
		printf("%s set the read lock failed\n",file_fd);
		return ret;
	}

	ret = read(read_fd,&buf,sizeof(struct TRAP_MESSAGE));
	
	if(ret != sizeof(struct TRAP_MESSAGE))
	{
		printf("read %s failed\n",file_fd);
		return ret;
	}

	lock.l_type = F_UNLCK;	
	fcntl(read_fd,F_SETLKW,&lock);
	close(read_fd);

	trap_message->trap_sbd = buf.trap_sbd;
	trap_message->trap_type = buf.trap_type;

	return 0;
}

int write_trap_file(struct TRAP_MESSAGE *trap_message)
{
	char file_fd[200];
	int write_fd;
	struct TRAP_MESSAGE buf;
	int ret = 0;
	struct flock lock;

	memset(&buf,0,sizeof(buf));
	memset(&lock,0,sizeof(lock));

	buf.trap_type = trap_message->trap_type;
	buf.trap_sbd = trap_message->trap_sbd;

	sprintf(file_fd,"%s/log/temp_trap_%s",port_index->dir_name,trap_message->trap_sbd.port_ip);

//创建trap内容文件
	write_fd = open(file_fd,O_WRONLY | O_CREAT, 0777);
	
	if(write_fd < 0)
	{
		printf("open write %s failed\n",file_fd);
		return -1;
	}

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	
	ret = fcntl(write_fd,F_SETLKW,&lock);
	
	if(ret == -1)
	{
		printf("%s : set the write lock failed\n",file_fd);
		return ret;
	}

	ret = write(write_fd,&buf,sizeof(struct TRAP_MESSAGE));

	if(ret != sizeof(struct TRAP_MESSAGE))
	{
		printf("write to %s failed\n",file_fd);
		return ret;
	}

	lock.l_type = F_UNLCK;
	fcntl(write_fd,F_SETLKW,&lock);
	close(write_fd);

	return 0;
}

static int choose_port(struct PORT_INDEX_INFO *p)
{
	int i;

	p->trap_level = 2;

	for(i = 0;i < port_index->port_count;i++)
	{
		printf("%s:\n\ttrap level = %d\n\ttrap state = 0x%x\n",port_index->port[i].dev_name,port_index->port[i].trap_level,port_index->port[i].trap_state);
		if(port_index->port[i].trap_level < p->trap_level)
		{
			p->trap_level = port_index->port[i].trap_level;
			strcpy(p->dev_name,port_index->port[i].dev_name);
		}
	}
	
	return 0;
}

static int get_port_mac(struct ifreq *ifc,char *dev_name)
{
	int sockfd;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("%s create mac socket failed\n",dev_name);
		close(sockfd);
		return -1;
	}

	strncpy(ifc->ifr_name,dev_name,16);

	if(!ioctl(sockfd,SIOCGIFHWADDR,ifc))
	{
		close(sockfd);
		return 0;
	}
}

int send_trap_data(struct TRAP_MESSAGE *trap_message,char *nid)
{
	int sockfd;
	struct TRAP_DATA_PKG trap_data_pkg;
	uint8_t source_mac[7] = {255,255,255,255,255,255};
	uint8_t dest_mac[7] = {255,255,255,255,255,255};
	struct in_addr ip_src;
	struct in_addr ip_dst;
	struct sockaddr sa;
	int ret;
	struct ifreq ifc;
 

    FILE *fp1;
    char fileName1[] = "/home/pc-4/stats_send_ver_test1/traffic_temp";
    char strNum[100] = { '\0' };
    if((fp1 = fopen(fileName1, "r"))==NULL)
    {
        printf("open file error!");
        return -1;
    }
    fgets(strNum,100,fp1);
    fclose(fp1);

//struct PORT_INDEX_INFO trap_port;
/*
	pthread_rwlock_rdlock(&port_index->lock);
	choose_port(&trap_port);
	pthread_rwlock_unlock(&port_index->lock);
*/
	//trap_port.dev_name = TRAP_PORT;
	get_port_mac(&ifc,TRAP_PORT);

	memcpy((char *)source_mac,(char *)ifc.ifr_hwaddr.sa_data,6);

	if((sockfd = socket(PF_PACKET,SOCK_PACKET,htons(ETH_P_ALL))) == -1 )
	{
		perror("trap data socket error");
		return -1;
	}

	memset(&trap_data_pkg,0,sizeof(trap_data_pkg));

	memcpy((char*)trap_data_pkg.trap_data_head.ether_dhost,(char*)dest_mac,6);
	memcpy((char*)trap_data_pkg.trap_data_head.ether_shost,(char*)source_mac,6);
	trap_data_pkg.trap_data_head.ether_type = htons ( 0x0800 );
/*
        trap_data_pkg.trap_data_head.iphead[0] = 69;
	trap_data_pkg.trap_data_head.iphead[1] = 0; 
	trap_data_pkg.trap_data_head.iphead[2] = 0; 
	trap_data_pkg.trap_data_head.iphead[3] = 88;
	trap_data_pkg.trap_data_head.iphead[8] = 255; 
	trap_data_pkg.trap_data_head.iphead[9] = 10;
	trap_data_pkg.trap_data_head.iphead[10] = 174; 
	trap_data_pkg.trap_data_head.iphead[11] = 71;

	ip_src.s_addr = inet_addr("192.168.70.1");
	memcpy(trap_data_pkg.trap_data_head.iphead+12,&ip_src.s_addr,sizeof(ip_src.s_addr));
	ip_dst.s_addr = inet_addr("192.168.70.2");
	memcpy(trap_data_pkg.trap_data_head.iphead+16,&ip_dst.s_addr,sizeof(ip_dst.s_addr));
*/
	trap_data_pkg.trap_data_head.version_type = 161;
	trap_data_pkg.trap_data_head.ttl = 255;
	trap_data_pkg.trap_data_head.total_len = htons(68);

    trap_data_pkg.trap_data_head.port_no = 0;
    trap_data_pkg.trap_data_head.checksum = 0;

    trap_data_pkg.trap_data_head.sid_len = 20;
    trap_data_pkg.trap_data_head.nid_len = 16;
    trap_data_pkg.trap_data_head.pid_n = 0;
    trap_data_pkg.trap_data_head.options_static = 0;

    trap_data_pkg.trap_data_head.OFFSET = 0;
    trap_data_pkg.trap_data_head.LENGTH = 0;

    memset(&trap_data_pkg.trap_data_head.options_reserved,0,sizeof(trap_data_pkg.trap_data_head.options_reserved));
    trap_data_pkg.trap_data_head.sid[19] = 3;
    memcpy((char*)trap_data_pkg.trap_data_head.nid,(char*)nid,16);

	trap_data_pkg.trap_message.trap_type = trap_message->trap_type;
	//trap_data_pkg.trap_message.trap_type = 1;
	trap_data_pkg.trap_message.trap_sbd = trap_message->trap_sbd;

        trap_data_pkg.trap_message.port1_data = strtoll(strNum,NULL,10);
        //printf("port1=%lld\n",trap_data_pkg.trap_message.port1_data);
        //trap_data_pkg.trap_message.port1_data = 1;
        //trap_data_pkg.trap_message.port2_data= TTB3; //eth1 10.10.10.1
        trap_data_pkg.trap_message.port2_data= 3;


	memset(&sa,'\0',sizeof(sa));
    strcpy(sa.sa_data,TRAP_PORT);

//    printf("send data-trap-update by the port '%s'\n\n",sa.sa_data);
//    printf("trap nid : %s\n",trap_data_pkg.trap_message.trap_sbd.trap_nid);
//    printf("trap node = %s\n",trap_data_pkg.trap_message.trap_sbd.port_ip);
//    printf("trap type = 0x%x\n\n",trap_data_pkg.trap_message.trap_type);

   	ret = sendto(sockfd,&trap_data_pkg,sizeof(trap_data_pkg),0,&sa,sizeof(sa));

   	if(ret != sizeof(trap_data_pkg))
    {
        fprintf ( stderr,"Send Trap Data Error:%s\n\a",strerror ( errno ) );
        close(sockfd);
        return 0;
    }

    close(sockfd);
    return ret;
}

//原始套接字封装发送(mac以上封装)
int send_trap_hello(struct TRAP_MESSAGE *trap_message)
{
	int sockfd;
	struct TRAP_HELLO_PKG trap_hello_pkg;
	uint8_t source_mac[7] = {255,255,255,255,255,255};
	uint8_t dest_mac[7] = {255,255,255,255,255,255};

	struct in_addr ip_src;
 	struct in_addr ip_dst;
	struct sockaddr sa;
	int ret;
	struct ifreq ifc;
	char mac[18];

//	struct PORT_INDEX_INFO trap_port;
/*
	pthread_rwlock_rdlock(&port_index->lock);
	choose_port(&trap_port);
	pthread_rwlock_unlock(&port_index->lock);
*/
	get_port_mac(&ifc,TRAP_PORT);

	memcpy((char *)source_mac,(char *)ifc.ifr_hwaddr.sa_data,6);

	if((sockfd = socket(PF_PACKET,SOCK_PACKET,htons(ETH_P_ALL))) == -1 )
	{
		perror("trap hello socket error");
		return -1;
	}

	memset(&trap_hello_pkg,0,sizeof(trap_hello_pkg));

	memcpy((char*)trap_hello_pkg.trap_head.ether_dhost,(char*)dest_mac,6);
	memcpy((char*)trap_hello_pkg.trap_head.ether_shost,(char*)source_mac,6);
	trap_hello_pkg.trap_head.ether_type = htons ( 0x0800 );
/*
        trap_hello_pkg.trap_head.iphead[0] = 69;
	trap_hello_pkg.trap_head.iphead[1] = 0; 
	trap_hello_pkg.trap_head.iphead[2] = 0; 
	trap_hello_pkg.trap_head.iphead[3] = htons(88);
	trap_hello_pkg.trap_head.iphead[8] = 255; 
	trap_hello_pkg.trap_head.iphead[9] = 10;
	trap_hello_pkg.trap_head.iphead[10] = 174; 
	trap_hello_pkg.trap_head.iphead[11] = 71;

	ip_src.s_addr = inet_addr("192.168.70.1");
	memcpy(trap_hello_pkg.trap_head.iphead+12,&ip_src.s_addr,sizeof(ip_src.s_addr));
	ip_dst.s_addr = inet_addr("192.168.70.2");
	memcpy(trap_hello_pkg.trap_head.iphead+16,&ip_dst.s_addr,sizeof(ip_dst.s_addr));
*/
	trap_hello_pkg.trap_head.version_type = 160;
	trap_hello_pkg.trap_head.ttl = 255;
	trap_hello_pkg.trap_head.total_len = htons(68);

        trap_hello_pkg.trap_head.port_no = 0;
        trap_hello_pkg.trap_head.checksum = 0;

        trap_hello_pkg.trap_head.sid_len = 20;
        trap_hello_pkg.trap_head.nid_len = 16;
        trap_hello_pkg.trap_head.pid_n = 0;
        trap_hello_pkg.trap_head.options_static = 0;

        trap_hello_pkg.trap_head.OFFSET = 0;
        trap_hello_pkg.trap_head.LENGTH = 0;

    memset(&trap_hello_pkg.trap_head.options_reserved,0,sizeof(trap_hello_pkg.trap_head.options_reserved));

    trap_hello_pkg.trap_head.publickey_len = htons(16);
    trap_hello_pkg.trap_head.mtu = htons(1500);

    memset(&trap_hello_pkg.trap_head.sid,0,sizeof(trap_hello_pkg.trap_head.sid));
//t//测试用，实际发送时的sid填的是2  
    trap_hello_pkg.trap_head.sid[19] = 2;//标识是一个包含trap信息的get包

    memset(&trap_hello_pkg.trap_head.nid,0,sizeof(trap_hello_pkg.trap_head.nid));
    memcpy((char*)trap_hello_pkg.trap_head.nid,(char*)trap_message->trap_sbd.trap_nid,16);//trap端口地址

    trap_hello_pkg.trap_sbd = trap_message->trap_sbd;

   	memset(&sa,'\0',sizeof(sa));
    strcpy(sa.sa_data,TRAP_PORT);
 /*   printf("send trap-hello message by the port '%s'\n\n",sa.sa_data);
    printf("trap nid : %s\n",trap_hello_pkg.trap_sbd.trap_nid);
    printf("trap node = %s\n",trap_hello_pkg.trap_sbd.port_ip);
    printf("trap type = 0x%x\n\n",trap_message->trap_type);
*/
    ret = sendto(sockfd,&trap_hello_pkg,sizeof(trap_hello_pkg),0,&sa,sizeof(sa));

   	if(ret != sizeof(trap_hello_pkg))
    {
        fprintf ( stderr,"Send Trap Error:%s\n\a",strerror ( errno ) );
        close(sockfd);
        return 0;
    }

    close(sockfd);
    return ret;
}

int trap_port_set(struct PORT_TRAP_STATE *pts,uint32_t *state)
{
	int ret;
	pts->trap_flag = 1;
	pts->trap_state = pts->trap_state ^ (*state);
}

int trap_state_mark(struct PORT_INDEX_INFO *port,uint32_t *state)
{
//	printf("0x%x\n",*state);
	port->trap_state = port->trap_state ^ (*state);
//	printf("%s's trap state = 0x%x\n",port->dev_name,port->trap_state);
	
	if(port->trap_state & TRAP_CONNECTION || port->trap_state & TRAP_LINKCONNECT)
	{
		port->trap_level = 2;
	}
	else if(port->trap_state == 0)
	{
		port->trap_level = 0;
	}
	else
	{
		port->trap_level = 1;
	}

//	printf("%s's trap level = %d\n",port->dev_name,port->trap_level);

	return 0;
}
