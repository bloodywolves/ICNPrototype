#include "main.h"
#include "getnet.h"
#include "trap.h"
// 接收缓冲区大小
#define	RCV_BUF_SIZE     1024 * 5

//临时结构体，用于存储网包数据
static struct STATS_INFO stats_info;
// 接收缓冲区
static int g_iRecvBufSize = RCV_BUF_SIZE;
static char g_acRecvBuf[RCV_BUF_SIZE];
static unsigned int size_received;

// 物理网卡接口,需要根据具体情况修改 
static uint8_t local_mac[7];
static uint8_t local_ip[5];

//线程互斥变量
static pthread_mutex_t mutex;

//物理网卡混杂模式属性操作 
static int ethdump_setPromisc(char *pcIfName, int fd, int iFlags)
{
    int iRet = -1;
    struct ifreq stIfr;

    // 获取接口属性标志位 
    strcpy(stIfr.ifr_name, pcIfName);
    iRet = ioctl(fd, SIOCGIFFLAGS, &stIfr);
    if (0 > iRet)
    {
        perror("[Error]Get Interface Flags");   
        return -1;
    }
   
    if (0 == iFlags)
    {
        // 取消混杂模式 
        stIfr.ifr_flags &= ~IFF_PROMISC;
    }
    else
    {
        // 设置为混杂模式 
        stIfr.ifr_flags |= IFF_PROMISC;
    }

    iRet = ioctl(fd, SIOCSIFFLAGS, &stIfr);
    if (0 > iRet)
    {
        perror("[Error]Set Interface Flags");
        return -1;
    }
   
    return 0;
}


// Init L2 Socket 
static int ethdump_initSocket(char *g_szIfName)
{
    int iRet = -1;
    int fd = -1;
    struct ifreq stIf;
    struct sockaddr_ll stLocal;
  
    memset(&stLocal,0,sizeof(stLocal));

// 创建SOCKET 
    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (0 > fd)
    {
        perror("[Error]Initinate L2 raw socket");
      	close(fd);
	return -1;
    }
   
    // 网卡混杂模式设置 
    ethdump_setPromisc(g_szIfName, fd, 1);

    // 设置SOCKET选项 
    iRet = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &g_iRecvBufSize,sizeof(int));
    if (0 > iRet)
    {
        perror("[Error]Set socket option");
        close(fd);
        return -1;
    }
   
    // 获取物理网卡接口索引 
    strcpy(stIf.ifr_name, g_szIfName);
    iRet = ioctl(fd, SIOCGIFINDEX, &stIf);
    if(0 > iRet)	
    {
        perror("[Error]Ioctl operation");
        close(fd);
        return -1;
    }

    // 绑定物理网卡 
    stLocal.sll_family = PF_PACKET;
    stLocal.sll_ifindex = stIf.ifr_ifindex;
    stLocal.sll_protocol = htons(ETH_P_ALL);
    iRet = bind(fd, (struct sockaddr *)&stLocal, sizeof(stLocal));

    if (0 > iRet)
    {
        perror("[Error]Bind the interface");
        close(fd);
        return -1;
    }
   
    return fd;   
}

//获取本机网卡相应信息
static int GetLocalMac ( const char *device,char *mac,char *ip )
{
	int sockfd;
	struct ifreq req;
	struct sockaddr_in * sin;
	
	if ( ( sockfd = socket ( PF_INET,SOCK_DGRAM,0 ) ) ==-1 )
	{
		fprintf ( stderr,"Sock Error:%s\n\a",strerror ( errno ) );
		close(sockfd);
		return ( -1 );
	}
	
	memset ( &req,0,sizeof ( req ) );
	strcpy ( req.ifr_name,device );

	if ( ioctl ( sockfd,SIOCGIFHWADDR, ( char * ) &req ) ==-1 )
	{
		fprintf ( stderr,"ioctl SIOCGIFHWADDR:%s\n\a",strerror ( errno ) );
		close ( sockfd );
		return ( -1 );
	}
	memcpy ( mac,req.ifr_hwaddr.sa_data,6 );
	
	req.ifr_addr.sa_family = PF_INET;
	if ( ioctl ( sockfd,SIOCGIFADDR, ( char * ) &req ) ==-1 )
	{
		fprintf ( stderr,"ioctl SIOCGIFADDR:%s\n\a",strerror ( errno ) );
		close ( sockfd );
		return ( -1 );
	}
	sin = ( struct sockaddr_in * ) &req.ifr_addr;
	memcpy ( ip, ( char * ) &sin->sin_addr,4 );
	close(sockfd);
	return 0;
}

// 解析Ethernet帧首部 
static int ethdump_parseEthHead(const struct ether_header *pstEthHead)
{	

	if (NULL == pstEthHead)
   	{
        	return -1;
	}

//锁定互斥锁
	pthread_mutex_lock(&mutex);		
	stats_info.total_packets++;
	stats_info.total_bytes+= size_received;
	
	if(strncmp((char*)pstEthHead->ether_shost,(char*)local_mac,6)==0)
	{
		stats_info.tx_packets++;
		stats_info.tx_bytes += size_received;
	//解开互斥锁
		pthread_mutex_unlock(&mutex);
		return 2;
	}
	else
//	if(strncmp((char*)pstEthHead->ether_dhost,(char*)local_mac,6)==0)
	{
		stats_info.rx_packets++;
		stats_info.rx_bytes += size_received;
	//解开互斥锁
		pthread_mutex_unlock(&mutex);
		return 1;
	}	

//解开互斥锁
	pthread_mutex_unlock(&mutex);
    return 0;   
}

static void *thread_parse_trap(void *arg)
{
    char nid[16];
    int ret = 0;
	struct TRAP_HELLO_PKG *pstTrapGet = NULL;
	struct TRAP_MESSAGE trap_message;

	memset(&trap_message,0,sizeof(struct TRAP_MESSAGE));
	memset(nid,'\0',sizeof(nid));

	pstTrapGet = (struct TRAP_HELLO_PKG *)arg;

/*
	char test_char[16];
	strncpy(test_char,(char *)arg,sizeof(test_char));
	printf("tset_char is %s\n",test_char);
*/
	if(strcmp(pstTrapGet->trap_sbd.trap_nid,port_index->host_name) == 0)
	{
		//printf("it's a get trap request\n");

		memcpy((char *)nid,(char *)pstTrapGet->trap_head.nid,16);
		trap_message.trap_sbd = pstTrapGet->trap_sbd;
		read_trap_file(&trap_message);
		ret = send_trap_data(&trap_message,nid);
	}
    //关闭线程
	free(arg);
	pthread_exit(NULL);	
}

// 数据帧解析函数 
static int ethdump_parseFrame(const char *pcFrameData)
{ 
	struct ether_header *pstEthHead = NULL;
	uint8_t *head_type = NULL;
	struct TRAP_HEADER *pstColorGet;

//判定该包为收包或发包，1 for 收;2 for 发
	int rsflag = 0;
//	struct ip *pstIpHead = NULL;

// Ethnet帧头解析 
	pstEthHead = (struct ether_header*)g_acRecvBuf;
	rsflag = ethdump_parseEthHead(pstEthHead);


	if (0 > rsflag)
	{
		return rsflag;
	}

//ip包头解析,+1的作用是去掉了mac帧头以后再接着往下取了一个uint8_t	
	head_type = (uint8_t*)(pstEthHead + 1);

	if(*head_type == 161)
	{
	//判断为data包
	//锁定互斥锁
		pthread_mutex_lock(&mutex);
	//	printf("it's a data packet\n");
		stats_info.data_pack_count.total_packets++;
		stats_info.data_pack_count.total_bytes+= size_received;
		
		if(rsflag == 1)
		{
			stats_info.data_pack_count.rx_packets++;
			stats_info.data_pack_count.rx_bytes+= size_received;
		}
		else if(rsflag == 2)
		{
			stats_info.data_pack_count.tx_packets++;
			stats_info.data_pack_count.tx_bytes+= size_received;			
		}
	//解开互斥锁
		pthread_mutex_unlock(&mutex);
	}
	else if(*head_type == 160)
	{
	//判断为get包
	//锁定互斥锁
		pthread_mutex_lock(&mutex);
	//	printf("it's a get packet\n");
		stats_info.get_pack_count.total_packets++;
		stats_info.get_pack_count.total_bytes+= size_received;

		if(rsflag == 1)
		{
			stats_info.get_pack_count.rx_packets++;
			stats_info.get_pack_count.rx_bytes+= size_received;
		}
		else if(rsflag == 2)
		{
			stats_info.get_pack_count.tx_packets++;
			stats_info.get_pack_count.tx_bytes+= size_received;			
		}
	//解开互斥锁
		pthread_mutex_unlock(&mutex);

	//提取color-ge包头，这里不+1的原因是struct TRAP_HELLO_PKG中本身就包括了mac帧头了，所以取的时候就连着mac帧头一块取了
		pstColorGet = (struct TRAP_HEADER *)(pstEthHead);

		if(pstColorGet->sid[19] == 3)
		{
			struct TRAP_HELLO_PKG *pstTrapGet = NULL;
			struct TRAP_HELLO_PKG *trap_message_buf;
			trap_message_buf = (struct TRAP_HELLO_PKG *)malloc(sizeof(struct TRAP_HELLO_PKG));
			pstTrapGet = (struct TRAP_HELLO_PKG *)(pstEthHead);		
			memcpy(trap_message_buf,pstTrapGet,sizeof(struct TRAP_HELLO_PKG));

		//创建子线程进行get包头解析
			pthread_t pthread_parse_trap;

			if(pthread_create(&pthread_parse_trap, NULL, thread_parse_trap, trap_message_buf) != 0)
			{	
				perror("Creation of parse thread failed.");
				return -1;
			}	
		//	parse_trap_get_pkg(pstColorGet);
		}
	}	
/*
//判断有ip包头    
	else if(*head_type == 69)
	{
	//取IP包头
		pstIpHead = (struct ip*)(pstEthHead + 1);
	//取color协议头字段
		head_type = (uint8_t*)(pstIpHead + 1);
				
	}
*/
	return 0;	

}

// 捕获网卡数据帧 
static void ethdump_startCapture(const int fd)
{
    socklen_t stFromLen = 0;
   
    // 循环监听 
    while(1)
    {
        // 清空接收缓冲区 
        memset(g_acRecvBuf, 0, RCV_BUF_SIZE);

        // 接收数据帧 
        size_received = recvfrom(fd, g_acRecvBuf, g_iRecvBufSize, 0, NULL, &stFromLen);
        if (0 > size_received)
        {
            continue;
        }
	

// 解析数据帧 
    ethdump_parseFrame(g_acRecvBuf);	
	
/*
	//t//
	print_time();
	printf("capture thread : receive packet size %d\n",size_received);
	test_dump_stats_info(&stats_info);
*/
    }
}

//获取网包数据   

static void *thread_capture(void *arg)
{
    int iRet = -1;
    int fd   = -1;

	char dev_name[16];

	memset(dev_name,0,sizeof(dev_name));

	strcpy(dev_name,(char *)arg);
		   
    // 初始化SOCKET 
    fd = ethdump_initSocket(dev_name);

    if(0 > fd)
    {
	printf("fail to init capture_socket : %s\n",dev_name);
	close(fd);
        exit(0);
    }
	GetLocalMac(dev_name,local_mac,local_ip);

    // 捕获数据包 
	ethdump_startCapture(fd);
	printf("capture thread of %s exit\n",dev_name);   
    // 关闭SOCKET 
	close(fd);
    //关闭线程
	free(arg);
	pthread_exit(NULL);
	
}

//将计数器信息存入结构体中
static int record_counter(struct PORT_INFO *port_info)
{
	stats_info.total_packets_ps = stats_info.total_packets/(float)INTERVAL;	
	stats_info.tx_packets_ps = stats_info.tx_packets/(float)INTERVAL;
	stats_info.rx_packets_ps = stats_info.rx_packets/(float)INTERVAL;
	stats_info.total_bytes_ps = stats_info.total_bytes/(float)INTERVAL;
	stats_info.tx_bytes_ps = stats_info.tx_bytes/(float)INTERVAL;
	stats_info.rx_bytes_ps = stats_info.rx_bytes/(float)INTERVAL;

	port_info->stats_info = stats_info;
}

static int check_port(char *dev_name)
{
	int ret = 0;
	struct ifreq ifr;
	struct ethtool_value edata;
	int fd;

	memset(&edata,0,sizeof(edata));
	
	memset(&ifr,0,sizeof(ifr));
	strcpy(ifr.ifr_name,dev_name);
	
	if((fd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		close(fd);
		perror("socket check create failed");
		return -1;
	}

	edata.cmd = ETHTOOL_GLINK;
	ifr.ifr_data = (caddr_t)&edata;
	
	if(ioctl(fd,SIOCETHTOOL,&ifr) == 0)
	{
		if(edata.data)
		{
			ret = 1;
		}
		else
		{
			ret = 0;	
		}
	}	
	else if(errno != EOPNOTSUPP)
	{
		close(fd);
		perror("cannot get link status");
		return 0;
	}

	return ret;
}

static int trap_send_proc(struct PORT_TRAP_STATE *pts)
{
	struct TRAP_MESSAGE trap_message;
	int ret = 0;

	memset(&trap_message,0,sizeof(struct TRAP_MESSAGE));

	strncpy(trap_message.trap_sbd.trap_nid,port_index->host_name,16);
	strncpy(trap_message.trap_sbd.port_ip,pts->port_ip,16);
	trap_message.trap_type = pts->trap_state;

	ret = write_trap_file(&trap_message);

	if(ret != 0)
	{
		return ret;
	}
        char strip[]={"10.10.11.1"};
        if(strcmp(trap_message.trap_sbd.port_ip,strip))
	ret = send_trap_hello(&trap_message);

	pts->trap_flag = 0;

	return ret;
}

int get_port_info(struct PORT_INFO *port_info, char *file_name, int* port_id)
{
	char *dev_name;
	int check_flag = 0;
	FILE *f_port_info;
	int bandwidth = 0;
	float efficiency_bandwidth = 0;
	int bandwidth_trap_flag = 0;//判断是否发出过带宽利用率的trap消息
	char dev_ip[16];
	struct DEST_INFO *dest_info;//链路测试时用于取地址的指针
	int ret;
	int rewrite_flag;//设定数据库重写次数
//mark the trap state of this port
	struct PORT_INDEX_INFO *port;

//该端口trap状态
	static struct PORT_TRAP_STATE pts;

	uint32_t port_trap_state;
	int port_comm = 0;//if this port is a communication port:1 for yes;0 for no
	//t//
	/*
	struct timeval action_start;
	struct timeval action_end;
	int time_diff;
	*/
	dev_name = (char *)malloc(16);

	memset(&dev_ip,0,sizeof(dev_ip));
	memset(&pts,0,sizeof(struct PORT_TRAP_STATE));
	
	strcpy(dev_ip,port_info->dev_info.dev_ip);
	strncpy(pts.port_ip,dev_ip,16);	
	memcpy(dev_name,port_info->dev_info.dev_name,16);	
//用于输出该端口信息的文件名
//	snprintf(file_name,sizeof(file_name),"log/port_info_%d",*num);
	
//	port = (struct PORT_STATE_INDEX *)malloc(sizeof(struct PORT_STATE_INDEX));
//	port = port_state_index;
/*
	pthread_rwlock_rdlock(&port_index->lock);
	port = search_port_index(port_index,dev_name);
	pthread_rwlock_unlock(&port_index->lock);

	if(port != NULL)
	{
		port_comm = 1;
		printf("%s is a communication port\n",port->dev_name);
	}
	else
	{
		port_comm = 0;
	}
*/
	pthread_mutex_init(&mutex,NULL);		
	pthread_t pthread_capture;

	if(pthread_create(&pthread_capture, NULL, thread_capture, dev_name)!=0)
	{	
		printf("%s capture thread :\t",file_name);
		perror("Creation of capture thread failed.");
		return -1;
	}	

	while(1)
	{

	//锁定互斥锁
		pthread_mutex_lock(&mutex);
	//重置计数器
		memset(&stats_info,0,sizeof(stats_info));
	//解开互斥锁
		pthread_mutex_unlock(&mutex);
		
		sleep(INTERVAL);
	//锁定互斥锁
		pthread_mutex_lock(&mutex);
	//读取计数器	
		record_counter(port_info);
	//解开互斥锁
		pthread_mutex_unlock(&mutex);

	//检测网口通断状态
		check_flag = check_port(dev_name);
	
	//连通情况下	
		if(check_flag)
		{
		//输出端口速率
			bandwidth = fdump_speed(&port_info->dev_info.adapter);
		
			if(bandwidth)
			{
				efficiency_bandwidth = port_info->stats_info.total_bytes_ps*8/bandwidth/10000;//除10000为Mb换算后再乘百分比换算的结果
				port_info->efficiency.bandwidth = efficiency_bandwidth;	


				switch(bandwidth_trap_flag)
				{
				//尚未汇报过带宽trap消息
					case 0:{
								if(efficiency_bandwidth > BAND_THRES_HIGH)
								{
									port_trap_state = TRAP_BANDWIDTH_HIGH;
									trap_port_set(&pts,&port_trap_state);
									bandwidth_trap_flag = 1;

									if(port_comm)
									{
										pthread_rwlock_wrlock(&port_index->lock);
										trap_state_mark(port,&port_trap_state);
										pthread_rwlock_unlock(&port_index->lock);
									}								
								}
								else if(efficiency_bandwidth < BAND_THRES_LOW)
								{
									port_trap_state = TRAP_BANDWIDTH_LOW;
									trap_port_set(&pts,&port_trap_state);
									bandwidth_trap_flag = 2;
								}
								break;
							}
				//已经汇报过带宽过高trap消息
					case 1:{
								if(efficiency_bandwidth < BAND_THRES_HIGH)
								{
									port_trap_state = TRAP_BANDWIDTH_HIGH;
									trap_port_set(&pts,&port_trap_state);
									bandwidth_trap_flag = 0;

									if(port_comm)
									{
										pthread_rwlock_wrlock(&port_index->lock);
										trap_state_mark(port,&port_trap_state);
										pthread_rwlock_unlock(&port_index->lock);
									}

									if(efficiency_bandwidth < BAND_THRES_LOW)
									{
										port_trap_state = TRAP_BANDWIDTH_LOW;
										trap_port_set(&pts,&port_trap_state);
										bandwidth_trap_flag = 2;
									}
								}
								break;
							}
				//已经汇报过带宽过低trap消息
					case 2:{
								if(efficiency_bandwidth > BAND_THRES_LOW)
								{
									port_trap_state = TRAP_BANDWIDTH_LOW;
									trap_port_set(&pts,&port_trap_state);
									bandwidth_trap_flag = 0;

									if(efficiency_bandwidth >BAND_THRES_HIGH)
									{
										port_trap_state = TRAP_BANDWIDTH_HIGH;
										trap_port_set(&pts,&port_trap_state);
										bandwidth_trap_flag = 1;

										if(port_comm)
										{											
											pthread_rwlock_wrlock(&port_index->lock);
											trap_state_mark(port,&port_trap_state);
											pthread_rwlock_unlock(&port_index->lock);
										}								
									}
								}
								break;
							}	
				}
			}
			else
			{
				perror("get bandwidth error");
			}		
			
		//链路时延测试
			/*
			for(dest_info = port_info->dest_info;dest_info != NULL;dest_info = dest_info->next)
			{	 
			//获取链路状态信息
				get_link_stats(dev_name,dest_info);

				switch(dest_info->linkloss_trap_flag)
				{
				//尚未汇报过链路丢包trap
					case 0:{
								if(dest_info->link_stats.packet_loss > LINK_THRES_LOSS)
								{
									if(dest_info->link_stats.packet_loss == 100)
									{
										port_trap_state = TRAP_LINKCONNECT;
										trap_port_set(&pts,&port_trap_state);
										dest_info->linkloss_trap_flag = 1;

										if(port_comm)
										{											
											pthread_rwlock_wrlock(&port_index->lock);
											trap_state_mark(port,&port_trap_state);
											pthread_rwlock_unlock(&port_index->lock);
										}					
									}		
									else
									{
										port_trap_state = TRAP_LINKLOSS;
										trap_port_set(&pts,&port_trap_state);
										dest_info->linkloss_trap_flag = 2;

										if(port_comm)
										{											
											pthread_rwlock_wrlock(&port_index->lock);
											trap_state_mark(port,&port_trap_state);
											pthread_rwlock_unlock(&port_index->lock);
										}
									}
								}
								break;
							}
				//汇报过链路断开trap
					case 1:{
								if(dest_info->link_stats.packet_loss < 100)
								{
									port_trap_state = TRAP_LINKCONNECT;
									trap_port_set(&pts,&port_trap_state);
									dest_info->linkloss_trap_flag = 0;

									if(port_comm)
									{										
										pthread_rwlock_wrlock(&port_index->lock);
										trap_state_mark(port,&port_trap_state);
										pthread_rwlock_unlock(&port_index->lock);
									}

									if(dest_info->link_stats.packet_loss > LINK_THRES_LOSS)
									{
										port_trap_state = TRAP_LINKLOSS;
										trap_port_set(&pts,&port_trap_state);
										dest_info->linkloss_trap_flag = 2;

										if(port_comm)
										{											
											pthread_rwlock_wrlock(&port_index->lock);
											trap_state_mark(port,&port_trap_state);
											pthread_rwlock_unlock(&port_index->lock);
										}									
									}
								}
								break;
							}
				//汇报过链路丢包率trap
					case 2:{
								if(dest_info->link_stats.packet_loss == 100)
								{
									port_trap_state = TRAP_LINKLOSS;
									trap_port_set(&pts,&port_trap_state);

									port_trap_state = TRAP_LINKCONNECT;
									trap_port_set(&pts,&port_trap_state);
									dest_info->linkloss_trap_flag = 1;

									if(port_comm)
									{									
										pthread_rwlock_wrlock(&port_index->lock);
										trap_state_mark(port,&port_trap_state);
										pthread_rwlock_unlock(&port_index->lock);
									}
								}
								else if(dest_info->link_stats.packet_loss < LINK_THRES_LOSS)
								{
									port_trap_state = TRAP_LINKLOSS;
									trap_port_set(&pts,&port_trap_state);
									dest_info->linkloss_trap_flag = 0;

									if(port_comm)
									{									
										pthread_rwlock_wrlock(&port_index->lock);
										trap_state_mark(port,&port_trap_state);
										pthread_rwlock_unlock(&port_index->lock);
									}
								}
								break;
							}
				}

				switch(dest_info->linkdelay_trap_flag)
				{
				//尚未汇报过链路时延过大trap信息
					case 0:{
								if(dest_info->link_stats.rtt_min > LINK_THRES_DELAY)
								{
									port_trap_state = TRAP_LINKDELAY;
									trap_port_set(&pts,&port_trap_state);
									dest_info->linkdelay_trap_flag = 1;

									if(port_comm)
									{										
										pthread_rwlock_wrlock(&port_index->lock);
										trap_state_mark(port,&port_trap_state);
										pthread_rwlock_unlock(&port_index->lock);
									}
								}
								break;
							}
				//已经汇报过链路时延过大trap信息
					case 1:{
								if(dest_info->link_stats.rtt_min < LINK_THRES_DELAY)
								{
									port_trap_state = TRAP_LINKDELAY;
									trap_port_set(&pts,&port_trap_state);
									dest_info->linkdelay_trap_flag = 0;

									if(port_comm)
									{										
										pthread_rwlock_wrlock(&port_index->lock);
										trap_state_mark(port,&port_trap_state);
										pthread_rwlock_unlock(&port_index->lock);
									}
								}
								break;
							}
				}
			}
			*/
			//gettimeofday(&action_start, NULL);
		//输出端口信息到文件
			f_port_info = fopen(file_name,"a+");
			
			if(f_port_info == NULL)
			{
				perror("cannot open input file");
			}
                        if(*port_id==0)
                        {
                          FILE *fp;
                          fp=fopen("/home/pc-4/stats_send_ver_test1/traffic_temp","w");
                          fprintf(fp,"%lld\n",port_info->stats_info.total_bytes);
                          fclose(fp);
                        }
			dump_port_info(port_info,f_port_info);		
			fclose(f_port_info);

			//gettimeofday(&action_end, NULL);
			//time_diff = (action_end.tv_sec - action_start.tv_sec) * 1000000 + action_end.tv_usec - action_start.tv_usec;
			//printf("write to file use: %d us\n", time_diff);

/*
			rewrite_flag = 3;
		//写入数据库
			do
			{
				//gettimeofday(&action_start, NULL);

				ret = mysql_insert_port_stats(port_info, port_id);				
				rewrite_flag--;
				//gettimeofday(&action_end, NULL);
				//time_diff = (action_end.tv_sec - action_start.tv_sec) * 1000000 + action_end.tv_usec - action_start.tv_usec;
				//printf("write to mysql use: %d us\n", time_diff);

				if(rewrite_flag == 0)
				{
					printf("re-insert into mysql\n");
					break;
				}
			}
			while(ret);
*/

//			if(pts.trap_flag == 1)
//			{
				trap_send_proc(&pts);
//			}

		}
	//断开情况下
		else
		{
			port_trap_state = TRAP_CONNECTION;
			trap_port_set(&pts,&port_trap_state);
			
			if(port_comm)
			{				
				pthread_rwlock_wrlock(&port_index->lock);
				trap_state_mark(port,&port_trap_state);
				pthread_rwlock_unlock(&port_index->lock);
			}

			trap_send_proc(&pts);

			port_info->link_detect = check_flag;
			
			f_port_info = fopen(file_name,"w");
			
			if(f_port_info == NULL)
			{
				perror("cannot open input file");
			}
                        /* 
                        if(*port_id==0)
                        TTB2=port_info->stats_info.total_bytes; //eth1
                        else if (*port_id==1)
                        TTB1=port_info->stats_info.total_bytes; //eth0
                        */
			dump_port_info(port_info,f_port_info);		
			fclose(f_port_info);

			rewrite_flag = 3;
			//写入数据库
			do
			{
			//	ret = mysql_insert_port_stats(port_info, port_id);				

				if(rewrite_flag == 0)
				{
					printf("re-insert into mysql\n");
					break;
				}
			}
			while(ret);

			while(!check_flag)
			{
				sleep(1);
				check_flag = check_port(dev_name);	
			}

			port_trap_state = TRAP_CONNECTION;
			trap_port_set(&pts,&port_trap_state);
			
			if(port_comm)
			{			
				pthread_rwlock_wrlock(&port_index->lock);
				trap_state_mark(port,&port_trap_state);
				pthread_rwlock_unlock(&port_index->lock);
			}

			trap_send_proc(&pts);

			port_info->link_detect = check_flag;
		}
	}
	printf("fork of %s exit\n",dev_name);
	exit(1);
}

