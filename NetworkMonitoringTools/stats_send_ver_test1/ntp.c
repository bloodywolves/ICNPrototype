#include "main.h"

//3600s*24h*(365days*70years+17days)
#define From00to70 0x83aa7e80U

#define NTP_PORT 123

typedef struct NTP_PACKET
{
	uint8_t li_vn_mode;
	uint8_t stratum;
	uint8_t poll;
	uint8_t precision;
	uint32_t root_delay;
	uint32_t root_dispersion;
	uint8_t def_id[4];
	uint32_t ref_time_stamp_high;
	uint32_t ref_time_stamp_low;
	uint32_t ori_time_stamp_high;
	uint32_t ori_time_stamp_low;
	uint32_t recv_time_stamp_high;
	uint32_t recv_time_stamp_low;
	uint32_t tran_time_stamp_high;
	uint32_t tran_time_stamp_low;
}NTP_PACKET;

NTP_PACKET ntp_pack,new_pack;

long long first_time_stamp,final_time_stamp;
long long dif_time,delay_time;

static void NTP_Init(void)
{
	bzero(&ntp_pack,sizeof(NTP_PACKET));
	ntp_pack.li_vn_mode = 0x1b;
	first_time_stamp = From00to70 + time(NULL);
//	printf("first time = %lld\n",first_time_stamp);
	ntp_pack.ori_time_stamp_high = ntohl(first_time_stamp);
}

int ntp_sync(char *rm_ip)
{
	fd_set insetl;
	int sockfd;
	struct timeval tv,tv1;
	struct timezone tz;
	struct sockaddr_in addr;
	char ntp_server[32];
	int syn_flag = 1;

	strcpy(ntp_server,rm_ip);
//	printf("ntp server : %s\n",ntp_server);

	if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		perror("create ntp socket error");
		exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(NTP_PORT);
	addr.sin_addr.s_addr = inet_addr(ntp_server);
	bzero(&(addr.sin_zero),8);

	tv.tv_sec = 10;
	tv.tv_usec = 0;

	FD_ZERO(&insetl);
	FD_SET(sockfd,&insetl);

	NTP_Init();

	if((sendto(sockfd,&ntp_pack,sizeof(NTP_PACKET),0,(struct sockaddr *)&addr,sizeof(struct sockaddr))) < 0)
	{
		perror("send to ntp server failed");
		exit(1);
	}

	if(select(sockfd+1,&insetl,NULL,NULL,&tv) < 0)
	{
		perror("ntp select error");
		exit(1);
	}
	else
	{
		if(FD_ISSET(sockfd,&insetl))
		{
			if(recv(sockfd,&new_pack,sizeof(NTP_PACKET),0) < 0)
			{
				perror("ntp recv error");
				exit(1);
			}
			else
			{
				syn_flag = 0;
			}
		}
	}

	if(syn_flag)
	{
		printf("time sync error\n");
		return 0;
	}

	final_time_stamp = time(NULL) + From00to70;
//	printf("final time = %lld\n",final_time_stamp);

	new_pack.root_delay = ntohl(new_pack.root_delay);
	new_pack.root_dispersion = ntohl(new_pack.root_dispersion);
	new_pack.ref_time_stamp_high = ntohl(new_pack.ref_time_stamp_high);
	new_pack.ref_time_stamp_low = ntohl(new_pack.ref_time_stamp_low);
	new_pack.ori_time_stamp_high = ntohl(new_pack.ori_time_stamp_high);
	new_pack.ori_time_stamp_low = ntohl(new_pack.ori_time_stamp_low);
	new_pack.recv_time_stamp_high = ntohl(new_pack.recv_time_stamp_high);
	new_pack.recv_time_stamp_low = ntohl(new_pack.recv_time_stamp_low);
	new_pack.tran_time_stamp_high = ntohl(new_pack.tran_time_stamp_high);
	new_pack.tran_time_stamp_low = ntohl(new_pack.tran_time_stamp_low);

	//client和server的时间差 = ((T2-T1) + (T3-T4))/2
	dif_time = ((new_pack.recv_time_stamp_high - first_time_stamp) + (new_pack.tran_time_stamp_high - final_time_stamp))>>1;
//	printf("dif time = %lld\n",dif_time);
	//延时
	delay_time = ((new_pack.recv_time_stamp_high - first_time_stamp) - (new_pack.tran_time_stamp_high - final_time_stamp))>>1;
//	printf("delay time = %lld\n",delay_time);
	//实际时间戳
	tv1.tv_sec = time(NULL) + dif_time + delay_time;
	tv1.tv_usec = 0;

	settimeofday(&tv1,NULL);
	system("hwclock -w");
	system("date");
	return 0;
}