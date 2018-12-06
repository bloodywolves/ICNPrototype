#include "main.h"

#define	PACKET_SIZE	4096
#define	MAX_NO_PACKETS	5

//设定通信对端
//#define PEER	"172.168.1.2"
//设定超时重传时间
#define	TIMEOUT	500000

static struct LINK_STATS link_stats;
static char *addr[1];
static char sendpacket[PACKET_SIZE];
static char recvpacket[PACKET_SIZE];
static int sockfd,datalen = 56;
static int nsend = 0, nreceived = 0;
static double temp_rtt[MAX_NO_PACKETS];
static double all_time = 0;
static double min = 0;
static double max = 0;
static double avg = 0;
static double mdev = 0;

static struct sockaddr_in dest_addr;
static struct sockaddr_in from;
static struct timeval tvrecv;
static pid_t pid;
static int recv_flag;

//定时器部分
/***************************************************************************************************/
static struct sigaction tact;
static struct itimerval tact_value;

/*
//超时操作
static void prompt_info(int signo)

{
	recv_flag = 1;
}

// 建立信号处理机制

static void init_sigaction(void (*sig_func)())
{

//信号到了要执行的任务处理函数为prompt_info

tact.sa_handler = sig_func;

tact.sa_flags = 0;

//初始化信号集

sigemptyset(&tact.sa_mask);

//建立信号处理机制

sigaction(SIGALRM, &tact, NULL);

}

static void init_time(struct timeval time)

{

	//设定初始时间计数

	tact_value.it_value = time;

	//设定执行任务的时间间隔
	//tact_value.it_interval = tact_value.it_value;

	tact_value.it_interval.tv_sec = 0;
	tact_value.it_interval.tv_usec = 0;


	//设置计时器ITIMER_REAL

	setitimer(ITIMER_REAL, &tact_value, NULL);

}

static void reset_time(void)

{

	//设定初始时间计数

	tact_value.it_value.tv_sec = 0;
	tact_value.it_value.tv_usec = 0;

	//设定执行任务的时间间隔
	//tact_value.it_interval = tact_value.it_value;

	tact_value.it_interval.tv_sec = 0;
	tact_value.it_interval.tv_usec = 0;


	//设置计时器ITIMER_REAL

	setitimer(ITIMER_REAL, &tact_value, NULL);

}
*/

/***************************************************************************************************/
//计算rtt最小、大值，平均值，算术平均数差
static void computer_rtt(void)
{
	double sum_avg = 0;
	int i;
	min = max = temp_rtt[0];

//	printf("temp_rtt = %lf\n",temp_rtt[0]);

	if(nreceived)
	{
		avg = all_time/nreceived;
	}

	for(i=0; i<nreceived; i++){
		
		if(temp_rtt[i] < min)
			min = temp_rtt[i];
		else if(temp_rtt[i] > max)
			max = temp_rtt[i];

		if((temp_rtt[i]-avg) < 0)
			sum_avg += avg - temp_rtt[i];
		else
			sum_avg += temp_rtt[i] - avg; 
		}
	if(nreceived)
	{	
			mdev = sum_avg/nreceived;
	}
}

//统计值初始化
static void init_statistics(void)
{
	min=0;
	avg=0;
	max=0;
	mdev=0;
	nsend=0;
	nreceived=0;
	all_time=0;
	memset(temp_rtt,0,sizeof(temp_rtt));
}

//统计数据函数
static int statistics(void)
{
	computer_rtt();		//计算rtt
	close(sockfd);
	return 0;
}

//检验和算法
static unsigned short cal_chksum(unsigned short *addr,int len)
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short check_sum = 0;

	while(nleft>1)		//ICMP包头以字（2字节）为单位累加
	{
		sum += *w++;
		nleft -= 2;
	}

	if(nleft == 1)		//ICMP为奇数字节时，转换最后一个字节，继续累加
	{
		*(unsigned char *)(&check_sum) = *(unsigned char *)w;
		sum += check_sum;
	}
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	check_sum = ~sum;	//取反得到校验和
	return check_sum;
}

//设置ICMP报头
static int pack(int pack_no)
{
	int i,packsize;
	struct icmp *icmp;
	struct timeval *tval;
	icmp = (struct icmp*)sendpacket;
	icmp->icmp_type = ICMP_ECHO;	//ICMP_ECHO类型的类型号为0
	icmp->icmp_code = 0;
	icmp->icmp_cksum = 0;
	icmp->icmp_seq = pack_no;	//发送的数据报编号
	icmp->icmp_id = pid;

	packsize = 8 + datalen;		//数据报大小为64字节
	tval = (struct timeval *)icmp->icmp_data;
	gettimeofday(tval,NULL);		//记录发送时间
	//校验算法
	icmp->icmp_cksum =  cal_chksum((unsigned short *)icmp,packsize);	
	return packsize;
}

//发送ICMP报文
static void send_packet(void)
{
	int packetsize;
		nsend++;
		packetsize = pack(nsend);	//设置ICMP报头
		//发送数据报
		if(sendto(sockfd,sendpacket,packetsize,0,
			(struct sockaddr *)&dest_addr,sizeof(dest_addr)) < 0)
		{
			perror("sendto error");
		}
}

//两个timeval相减
static void tv_sub(struct timeval *recvtime,struct timeval *sendtime)
{
	long sec = recvtime->tv_sec - sendtime->tv_sec;
	long usec = recvtime->tv_usec - sendtime->tv_usec;
	if(usec >= 0){
		recvtime->tv_sec = sec;
		recvtime->tv_usec = usec;
	}else{
		recvtime->tv_sec = sec - 1;
		recvtime->tv_usec = 0-usec;
	}
}

//剥去ICMP报头
static int unpack(char *buf,int len)
{
	int i;
	int iphdrlen;		//ip头长度
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;
	double rtt;


	ip = (struct ip *)buf;
	iphdrlen = ip->ip_hl << 2;	//求IP报文头长度，即IP报头长度乘4
	icmp = (struct icmp *)(buf + iphdrlen);	//越过IP头，指向ICMP报头
	len -= iphdrlen;	//ICMP报头及数据报的总长度
	if(len < 8)		//小于ICMP报头的长度则不合理
	{
		perror("receive icmp length error");
		return 0;
	}
	//确保所接收的是所发的ICMP的回应
	if((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == pid)&&(icmp->icmp_seq == nsend))
	{
		tvsend = (struct timeval *)icmp->icmp_data;
		tv_sub(&tvrecv,tvsend);	//接收和发送的时间差
		
		//以毫秒为单位计算rtt
		rtt = tvrecv.tv_sec*1000.0 + tvrecv.tv_usec/1000.0;
		temp_rtt[nreceived] = rtt;
		
		all_time += rtt;	//总时间
		nreceived++;		
		return 1;
	}
	else return 0;
}


//接受所有ICMP报文
static int recv_packet(void)
{
	int ret = 0;
	int recv_size,fromlen;
	extern int error;
	struct timeval timeout;

	fromlen = sizeof(from);
	recv_flag = 0;

	timeout.tv_sec = 0;
	timeout.tv_usec = TIMEOUT;

//select描述符
	fd_set fds;
	int maxfdp;
/*	
	init_sigaction(prompt_info);
	init_time(timeout);

	while(recv_flag == 0)
	{
*/		
//清空文件描述符集
	FD_ZERO(&fds);
//添加文件描述符
	FD_SET(sockfd,&fds);
	maxfdp = sockfd + 1;
//	printf("time set is %lu s . %lu us\n",timeout.tv_sec,timeout.tv_usec);	
	while(recv_flag == 0)
	{
	//	printf("set select\n");
		recv_flag = select(maxfdp,&fds,NULL,NULL,&timeout);

		switch(recv_flag)
		{
			case -1:{
						perror("select thread error");
						return -1;
						break;
					}
			case 0:{
						printf("receive timeout\n");
						recv_flag = 1;
						break;
					}
			default:{
					//sockfd是否可读？
						if(FD_ISSET(sockfd,&fds))	
						{
						//接收数据报(设定recvfrom阻塞接收)
							if((recv_size = recvfrom(sockfd,recvpacket,sizeof(recvpacket),0,(struct sockaddr *)&from,&fromlen)) < 0)
							{
								perror("recvfrom error");			
							}
							gettimeofday(&tvrecv,NULL);		//记录接收时间
							recv_flag=unpack(recvpacket,recv_size);		//剥去ICMP报头,若收到的包是本机所发的icmp回应，则返回1，否则返回0
						}
						break;
					}
		}
	}
//		if(recv_size > 0)
//		{

//		}		
/*
	//清除定时器
	reset_time();
*/
/*
		//t//
		print_time();
		printf("block is relieve : recv_flag = %d\n",recv_flag);
		printf("time left %lu s . %lu us\n",timeout.tv_sec,timeout.tv_usec);	
*/
//	printf("end of receiving\n");
	return ret;
}

int get_link_stats(char *dev_name,struct DEST_INFO *dest_info)
{
	int start_flag = 1;
	int iRet = -1;
	int end_flag = 1;//收包结束标识
	struct hostent *host;
	struct protoent *protocol;
	unsigned long inaddr = 0;
	int size = 50 * 1024;
	int error = -1;
	struct ifreq stIf;

	memset(temp_rtt,0,sizeof(temp_rtt));

	addr[0] = dest_info->dest_ip;
	//不是ICMP协议
	if((protocol = getprotobyname("icmp")) == NULL)
	{
		perror("getprotobyname");
		return -1;
	}
	memset(&link_stats,0,sizeof(link_stats));

while(start_flag)
{
	//生成使用ICMP的原始套接字，只有root才能生成
	if((sockfd = socket(AF_INET,SOCK_RAW,protocol->p_proto)) < 0)
	{
		perror("socket error");
		close(sockfd);
		return -1;
	}

	//扩大套接字的接收缓存区导50K，这样做是为了减小接收缓存区溢出的可能性，若无意中ping一个广播地址或多播地址，将会引来大量的应答
	setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));
	
	//绑定物理网卡
	strcpy(stIf.ifr_ifrn.ifrn_name,dev_name);

	if(setsockopt(sockfd,SOL_SOCKET,SO_BINDTODEVICE,(char *)&stIf,sizeof(stIf)) < 0)
	{
		perror("[Error] bind failed");
		close(sockfd);
		return -1;
	}
	
	//回收root权限，设置当前权限
	setuid(getuid());

	bzero(&dest_addr,sizeof(dest_addr));	//初始化
	dest_addr.sin_family = AF_INET;		//套接字域是AF_INET(网络套接字)

	//判断主机名是否是IP地址
	if(inet_addr(addr[0]) == INADDR_NONE)
	{
		if((host = gethostbyname(addr[0])) == NULL)	//是主机名
		{
			perror("gethostbyname error");
			close(sockfd);
			return -1;
		}
		memcpy((char *)&dest_addr.sin_addr,host->h_addr,host->h_length);
	}
	else{ //是IP地址
		dest_addr.sin_addr.s_addr = inet_addr(addr[0]);
	}

	pid = getpid();	

	while(end_flag)
	{
		//t//
//		sleep(1);
		send_packet();		//发送ICMP报文
		recv_packet();		
		if(nsend >= MAX_NO_PACKETS)
		{
			end_flag=0;		
		}
	}
	
	statistics();	
	
	if(max<=(TIMEOUT/1000))
	{
		link_stats.packet_loss = (float)(nsend-nreceived)/nsend*100;
		link_stats.rtt_avg = avg;
		link_stats.rtt_min = min;
		link_stats.rtt_max = max;
		link_stats.rtt_mdev = mdev;
		start_flag = 0;			
	}
	init_statistics();
}		
	//读取链路状态信息
	dest_info->link_stats = link_stats;

//	printf("end of link test\n");
	
/*	
	//清空信号集合
	sigset_t sig1;
	sigaddset(&sig1,SIGALRM);
	sigemptyset(&sig1);
*/	
	return 0;

}
