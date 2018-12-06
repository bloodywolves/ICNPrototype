#include <click/config.h>
#include "color_get_new.hh"
#include <click/args.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include <click/ipaddress.hh>
#include <clicknet/ip.h>
#include <clicknet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
//#define Random(x) (rand() % x)
#define alpha 1.2
//#define min 72
//#define max 1500
//1k到1Gbit的长度不定的Data包,这里是以字节为单位

#define min 100
#define max 4e9

CLICK_DECLS

color_get_new::color_get_new(){}

color_get_new::~color_get_new(){}

int color_get_new::configure(Vector<String> &conf, ErrorHandler *errh)
{
	return 0;
}
int color_get_new::initialize(ErrorHandler *)
{
  
  srand((unsigned)time(NULL));
  i = 0;
  char filename1[]="/home/flow/time_deal/flow1_1_yh2.txt";
  FILE *fp;
  long lsize;
  size_t result;
  fp=fopen(filename1,"rb");
  if(fp==NULL){printf("file open error\n");exit(1);} 
    
  fseek(fp,0,SEEK_END);
  lsize=ftell(fp);
  rewind(fp);
  buffer = (char *)malloc(sizeof(char)*lsize);
  if(buffer==NULL)
  {
    fputs("Memory error",stderr);
    exit(2);
  }
  result = fread(buffer,1,lsize,fp);
  if(result!=lsize)
  {
    fputs("Reading error",stderr);
    exit(3);
  }
  fclose(fp);

  char filename2[]="/home/pc-1/ipprefix";
  FILE *fp2;
  long lsize2;
  size_t result2;
  fp2=fopen(filename2,"rb");
  if(fp2==NULL){printf("file open error\n");exit(1);} 
    
  fseek(fp2,0,SEEK_END);
  lsize2=ftell(fp2);
  rewind(fp2);
  buffer2 = (char *)malloc(sizeof(char)*lsize2);
  if(buffer2==NULL)
  {
    fputs("Memory error",stderr);
    exit(2);
  }
  result2 = fread(buffer2,1,lsize2,fp2);
  if(result2!=lsize2)
  {
    fputs("Reading error",stderr);
    exit(3);
  }
  fclose(fp2);

  return 0;
}

double random_exp(double lambda)
{
  double pv=0.0;
  pv = (double)(rand()%100)/100;
  while(pv==0)
  {
   pv = (double)(rand() %100)/100;
  }
  pv = (-1 / lambda)*log(1-pv);
  return pv;
}

Packet *color_get_new::smaction(Packet *p)
{

    
    WritablePacket *pp = p->push(96);

    struct get_header_new h;

    h.version_type = 0xa0;
    h.ttl = 255;
    h.total_len = htons(96);

    h.port_src = 0;
    h.port_dst = 0;

    h.minimal_PID_CP = htons(10);
    h.PIDs = 0;
    h.offsets_RES = 0;

    h.offset = 0;
    h.length = 0;
   
    h.content_len = 0;
    h.mtu = htons(1500);

    h.publickey_len = 0;
    h.checksum = 0;
     
    char sid[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,100};
    memcpy(h.l_EID, sid, 20);

    char nid[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10};
    memcpy(h.nid_EID, nid, 16);
    memcpy(h.nid,nid,16);
        
    memset(h.publickey,0,4);

    char buf[1000];
    for(i=0;*buffer!='\n';i++,buffer++)
      {    
        buf[i]= *buffer;
      }
    if(*buffer=='\n') buffer++;
    
    char *word;
    for(i=0,word=strtok(buf," ");word!=NULL;word=strtok(NULL," "),i++)
      {

          if(i==0){h.cc_packets=strtol(word,NULL,10);}
          if(i==1){h.cc_flow_size=strtol(word,NULL,10);}
	  if(i==2){h.cc_flow_time=10;}
//          if(i==2){h.cc_flow_time=strtol(word,NULL,10);}
      }
 //printf("%d %d %d\n",h.cc_packets,h.cc_flow_size,h.cc_flow_time);

    char buf2[1000];
    int hang=rand()%97375+1;
    char* middle=buffer2;
    if(hang==1)
    {
     for(i=0;*middle!='\n';i++,middle++)
     {
      buf2[i]=*middle;
     }
    }
    else{
      while(hang!=1)
        {
           while(*middle!='\n')
            {
              middle++;
            }
           hang--;
           if(*middle=='\n')middle++;
        }
       for(i=0;*middle!='\n';i++,middle++)
         {
           buf2[i]=*middle;
        }
    }
   
    char *word2;
    char ip_ss[4];

    for(i=0,word2=strtok(buf2," ");word2!=NULL;word2=strtok(NULL," "),i++)
      {
          if(i==0){ip_ss[0]=atoi(word2);}
          if(i==1){ip_ss[1]=atoi(word2);}
          if(i==2){ip_ss[2]=atoi(word2);}
      }

	struct click_ip hh;
	hh.ip_v = 4;
	hh.ip_hl = 5;
	hh.ip_tos = 0;
	hh.ip_len = htons(116);
	hh.ip_id = 0;
	hh.ip_off = 0;
	hh.ip_ttl = 255;
	hh.ip_p = 10;
	hh.ip_sum = 0;
	ip_ss[3]=rand()%256;
	hh.ip_src.s_addr = IPAddress((const unsigned char*)ip_ss).addr();
	hh.ip_dst.s_addr = IPAddress((const unsigned char*)ip_ss).addr();
	hh.ip_sum = click_in_cksum((const unsigned char *)&hh, 20);

	memcpy(pp->data(), &h, 96);
        pp=pp->push(20);
	memcpy(pp->data(), &hh, 20); 

   // struct timeval in_time;
   // FILE *fppp;
   // char filename[]="/home/DOF_CLIENT";
   // fppp=fopen(filename,"a+");
   // gettimeofday(&in_time,NULL);
   // fprintf(fppp,"time:%d.%d\n",in_time.tv_sec,in_time.tv_usec);
   // fclose(fppp);
       // output(0).push(pp);
	return pp;
}

void color_get_new::push(int, Packet *p)
{
		if (Packet *q = smaction(p))
		output(0).push(q);
}

Packet *color_get_new::pull(int)
{
	if (Packet *p = input(0).pull())
		return smaction(p);
	else
		return 0;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(color_get_new)



