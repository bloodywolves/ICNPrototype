#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <resolv.h>
#include <signal.h>
#include <getopt.h>

#define Rec_Port "eth1"
#define Send_Port "eth1"
#define PHYSICALPORTlength 30
char Physicalport[30];
#define Send_Bufsize 8162000
#define Rec_Bufsize 1024


struct argc {
   uint32_t flow_packets;
   uint32_t flow_size;
   uint32_t flow_time;
   struct data_header *data;
};
int thread_num;
struct get_header
        {
            uint8_t version_type;
            uint8_t ttl;
            uint16_t total_len;

            uint16_t port_src;
            uint16_t port_dst;

            uint16_t minimal_PID_CP;
            uint8_t PIDs;
            uint8_t offsets_RES;

            uint32_t offset;

            uint32_t length;

            uint16_t content_len;
            uint16_t mtu;

            uint16_t publickey_len;
            uint16_t checksum;

            uint8_t nid_EID[16];
            uint8_t l_EID[20];
            uint8_t nid[16];
            uint32_t cc_flow_packets;
            uint32_t cc_flow_size;
            uint32_t cc_flow_time;
            uint8_t publickey[4];
            uint32_t pid;
        };

struct data_header
        {
            uint8_t version_type;
            uint8_t ttl;
            uint16_t total_len;

            uint16_t port_src;
            uint16_t port_dst;

            uint16_t minimal_pid_p;
            uint8_t  PIDs;
            uint8_t  RES;

            uint32_t offset;
            uint32_t length;

            uint16_t next_header;
            uint16_t check_sum;

            uint32_t reserved;

            uint8_t nid_eid[16];
            uint8_t l_eid[20];

            uint8_t NID[16];
            uint32_t pid;
        };

static void * send_data_packets(void *argv)
{
 struct argc B; 
 memcpy(&B,argv,sizeof(struct argc));

 if(B.flow_time<0)
 {
   thread_num=thread_num-1;
   pthread_detach(pthread_self());
   pthread_exit(NULL);
  }

 int sockfd; 
 if((sockfd=socket(PF_PACKET,SOCK_PACKET,htons(ETH_P_ALL)))==-1)
 {
  printf("socket error!\n");
  printf("errnu%d\n",errno);
  exit(0);
 }

 struct sockaddr to;
 memset(&to,0,sizeof(to));
 memcpy(Physicalport,Send_Port,PHYSICALPORTlength);
 strcpy(to.sa_data,Physicalport);

 uint8_t sbuf[Send_Bufsize];
 memset(sbuf,0,sizeof(sbuf));

 int lastpsize = B.flow_size % B.flow_packets;
 int psize = B.flow_size / B.flow_packets;
 int duration = B.flow_time / B.flow_packets;
 int len = 14 + sizeof(struct data_header) + psize;
 int len2= 14 + sizeof(struct data_header) + lastpsize;
 int j=0;
 if(psize>1514)
 {
  psize=1300;
  lastpsize = B.flow_size % 1514;
  if(lastpsize==0) B.flow_packets=B.flow_size/1514;
  else B.flow_packets=B.flow_size/1514 + 1;
  len = 1400;
  len2 = 14+sizeof(struct data_header) + lastpsize;
 }
 if(lastpsize>1514) lastpsize =1514;
 if(len2>1400) len2=1400;
 if(len>1400) len=1400;
 uint8_t mac[14]={255,255,255,255,255,255,255,255,255,255,255,255,8,0};
 uint8_t *sbuf2=sbuf+14+sizeof(struct data_header);
 memcpy(sbuf,mac,14);
 memcpy(sbuf+14,B.data,sizeof(struct data_header));
 memset(sbuf2,1,psize);
 int i,pklen;
 
 if(duration == 0)
  {
   if(B.flow_packets>1) 
    {
    duration = 1;
    B.flow_time=1;
    }
  }
if(duration>60000000) duration=60000000;
 if(lastpsize == 0)
 {
  for(i=0;i<B.flow_packets;i++)
   { 
     pklen = sendto(sockfd,sbuf,len,0,&to,sizeof(to));
     if(pklen==-1)
     {
      printf("errno%d \n",errno);
      printf("Sendto Error!\n");
      printf("B %d %d %d\n",B.flow_packets,B.flow_size,B.flow_time);
 //     printf("psize %d lastpsize %d duration %d\n",psize,lastpsize,duration);
      close(sockfd);
      exit(1);
     }
     //usleep(1);
     if(duration!=0 && duration>10000)usleep(1000);
     else if(duration!=0 && duration<10000) usleep(1);
   }
 }
 else 
 {
   for(i=0;i<B.flow_packets-1;i++)
   { 
    pklen = sendto(sockfd,sbuf,len,0,&to,sizeof(to));
    if(pklen==-1)
    {
     printf("errno%d \n",errno);
     printf("Sendto Error2 pklen=%d len=%d\n",pklen,len);
     printf("B %d %d %d\n",B.flow_packets,B.flow_size,B.flow_time);
    // printf("psize %d lastpsize %d duration %d\n",psize,lastpsize,duration);
     close(sockfd);
     exit(1);
    }
    //usleep(1);
    if(duration!=0 && duration>10000) usleep(1000);
    else if(duration !=0 && duration <=10000) usleep(1);
   }
   if(pklen=sendto(sockfd,sbuf,len2,0,&to,sizeof(to))==-1)
   {
    printf("errno%d \n",errno);
    printf("Sendto error3\n");
    printf("B %d %d %d\n",B.flow_packets,B.flow_size,B.flow_time);
   // printf("psize %d lastpsize %d duration %d\n",psize,lastpsize,duration);
    close(sockfd);
    exit(1);
   }
 }
 close(sockfd);
 thread_num=thread_num-1;
 pthread_detach(pthread_self());
 pthread_exit(0);
 }

void main()
{ 
 pthread_attr_t attr;
 pthread_attr_init(&attr);
 pthread_attr_setdetachstate(&attr,1);
 thread_num=0;
 struct argc A;
 int s_s;
 char rbuf[Rec_Bufsize];
 memset(rbuf,0,Rec_Bufsize); 

 pthread_t thread_do;

 struct data_header dh;
 dh.version_type=0xa1;
 dh.ttl=255;
 dh.total_len=0;
 dh.port_src=0;
 dh.port_dst=0;
 dh.minimal_pid_p=0;
 dh.PIDs=1;
 dh.RES=0;
 dh.offset=0;
 dh.length=0;
 dh.next_header=0;
 dh.check_sum = 0;
 dh.reserved =0;
 char nid[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
 memcpy(dh.nid_eid,nid,16);
 char sid[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,100};
 memcpy(dh.l_eid,sid,20);
 memcpy(dh.NID,nid,16);
 dh.pid=1;

 struct sockaddr from;
 memset(&from,0,sizeof(from));
 memcpy(Physicalport,Rec_Port,PHYSICALPORTlength);
 strcpy(from.sa_data,Physicalport);

 if((s_s=socket(PF_PACKET,SOCK_PACKET,htons(ETH_P_ALL)))==-1)
 {
  printf("socket error!\n");
  exit(0);
 }
 while(1){
 int lenr = recvfrom(s_s,rbuf,sizeof(rbuf),0,NULL,NULL);
 if(lenr<0)
  {
   printf("recv error!\n");
   close(s_s);
   exit(1);
  }
  struct get_header *get_h = (struct get_header *)(rbuf+14+20);
  if(get_h->version_type==0xa0)
  {
  A.flow_packets=get_h->cc_flow_packets;
  A.flow_size=get_h->cc_flow_size;
  A.flow_time=get_h->cc_flow_time;
  A.data = &dh;
 // printf("%d %d %d\n",A.flow_packets,A.flow_size,A.flow_time);
  if((get_h->cc_flow_time==0) && (get_h->cc_flow_packets>10)) {printf("data error\n");exit(1);}
if(get_h->cc_flow_time<0){printf("data error2\n");exit(1);}
 //  if(thread_num<32000) 
 //  {
    int err = pthread_create(&thread_do,&attr,send_data_packets,(void *)&A);
    thread_num=thread_num+1; 
    if(err!=0)
     {
      printf("create thread udp failed  err=%d/n",err);
      exit(1);
     }
    //printf("thread_num%d\n",thread_num);
  // }
  }//end get
  else continue;
 }//end while
}//end main
