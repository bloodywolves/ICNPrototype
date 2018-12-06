#include <click/config.h>
#include "trap_rm.hh"
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


CLICK_DECLS

trap_rm::trap_rm()
{
}

trap_rm::~trap_rm()
{
}

int trap_rm::configure(Vector<String> &conf, ErrorHandler *errh)
{
	return 0;
}

int trap_rm::initialize(ErrorHandler *)
{
        ss = 1;
	mm = 0;
	i=0;
        
    
}
/*
int compare_sid_nid(uint8_t *sid1,uint8_t *sid2,int t)
{ 
  int i;
  for(i=0;i<t;i++)
  {
    if(sid1[i]==sid2[i])
    continue;
    else return 0;
   }
  return 1;
}

int max(int a[],int n)
{
   int i;
   int iMax;
   iMax = a[0];
   for(i=1;i<n;i++)
    {
     if (iMax<a[i])
     iMax=a[i];
    }
   return iMax; 
}
*/
Packet *trap_rm::smaction(Packet *p)
{
    
    struct timeval in_time;
   /*
    uint8_t trap_hello_sid[20];
    uint8_t trap_hello_sid2[20];
    uint8_t trap_hello_nid_3[16];
    uint8_t trap_hello_nid_4[16];
    uint8_t trap_hello_nid_5[16];
    uint8_t trap_hello_nid_6[16];

    FILE *fp,*fp2,*fp3,*fp4;
    char filename1[]="/home/traffic_pc3";
    char filename2[]="/home/traffic_pc4";
    char filename3[]="/home/traffic_pc5";
    char filename4[]="/home/traffic_pc6";
    long int strNum1;
    long int strNum2;
    struct timeval now_time;


    for(i=0;i<16;i++){
      if(i==0)trap_hello_nid_3[i]='p';
      else if(i==1)trap_hello_nid_3[i]='c';
      else if(i==2)trap_hello_nid_3[i]='-';
      else if(i==3) trap_hello_nid_3[i]='3';
      else trap_hello_nid_3[i]=0;
    }
    for(i=0;i<16;i++){
      if(i==0)trap_hello_nid_4[i]='p';
      else if(i==1)trap_hello_nid_4[i]='c';
      else if(i==2)trap_hello_nid_4[i]='-';
      else if(i==3) trap_hello_nid_4[i]='4';
      else trap_hello_nid_4[i]=0;
    }
    for(i=0;i<16;i++){
      if(i==0)trap_hello_nid_5[i]='p';
      else if(i==1)trap_hello_nid_5[i]='c';
      else if(i==2)trap_hello_nid_5[i]='-';
      else if(i==3) trap_hello_nid_5[i]='5';
      else trap_hello_nid_5[i]=0;
    }
    for(i=0;i<16;i++){
      if(i==0)trap_hello_nid_6[i]='p';
      else if(i==1)trap_hello_nid_6[i]='c';
      else if(i==2)trap_hello_nid_6[i]='-';
      else if(i==3) trap_hello_nid_6[i]='6';
      else trap_hello_nid_6[i]=0;
    }

    for(i=0;i<20;i++){
      if(i<19){trap_hello_sid[i]=0;
               trap_hello_sid2[i]=0;}
      else if(i==19) {trap_hello_sid[i]=2;
                      trap_hello_sid2[i]=3;}
    }
    */
    struct trap_get_header *trap_head=(struct trap_get_header *)p->data();
 //   printf("%x\n",trap_head->version_type);
    if(trap_head->version_type==0xa0)
    {
    char filename[]="/home/DOF_FAILURE"; 
    FILE *fp;
    fp=fopen(filename,"a+");

     int a=1;
     gettimeofday(&in_time,NULL);
     fprintf(fp,"time:%d.%d\n",in_time.tv_sec,in_time.tv_usec);
     fclose(fp);
    }
   
    if(trap_head->version_type==0xa0) 
   {
    //printf("it's a get\n");
    
    if(compare_sid_nid(trap_hello_sid,trap_head->sid,20)==1)
    {
     //这是sid2,说明是感知的请求消息
     //printf("sid is 2\n");
     trap_head->sid[19]=3;
     
    //看是哪个边界路由器发送的,已经修改了nid，把消息重新发送到边界路由器,可以收到返回的trap_data,现在就是解析了。
    if(compare_sid_nid(trap_hello_nid_3,trap_head->nid,16)==1)
      {
       printf("It's nid is PC-3\n");
       output(0).push(p);
      }
    else if(compare_sid_nid(trap_hello_nid_4,trap_head->nid,16)==1)
      {
       printf("It's nid is PC-4\n");
       output(0).push(p);
      }
    else if(compare_sid_nid(trap_hello_nid_5,trap_head->nid,16)==1)
      {
       printf("It's nid is PC-5\n"); 
       output(0).push(p);
      }
    else if(compare_sid_nid(trap_hello_nid_6,trap_head->nid,16)==1)
      {
       printf("It's nid is PC-6\n"); 
       output(0).push(p);
      }
    }
   }
   else if(trap_head->version_type==0xa1) 
   {
     //printf("its a data packet!\n");
     struct data_header *trap_data =(struct data_header *)p->data();
     if((compare_sid_nid(trap_hello_nid_3,trap_data->nid,16))&&(compare_sid_nid(trap_hello_sid2,trap_data->sid,20)))
     {
      printf("pc-3 data\n");
      //for(i=0;i<10;i++){
      strNum1=trap_data->port1;
      strNum2=trap_data->port2;
      //}
      //int a[10];
      //int maxi;
      //memset(a,1,10);
      //maxi=max(a,10);
      gettimeofday(&now_time,NULL);
      fp=fopen(filename1,"a+");
      //fprintf(fp,"port1=%ld port2=%ld time:%d.%d\n",strNum2,strNum1,now_time.tv_sec,now_time.tv_usec);
      fprintf(fp,"%d.%d   %d.%d\n",in_time.tv_sec,in_time.tv_usec,now_time.tv_sec,now_time.tv_usec);
      fclose(fp);
      p->kill();
      }
     else if((compare_sid_nid(trap_hello_nid_4,trap_data->nid,16))&&(compare_sid_nid(trap_hello_sid2,trap_data->sid,20)))
     {
      printf("pc-4 data\n");
      gettimeofday(&now_time,NULL);
      strNum1=trap_data->port1;
      strNum2=trap_data->port2;

      //fp2=fopen(filename2,"a+");
      //fprintf(fp2,"port1=%ld port2=%ld time:%d.%d\n",strNum2,strNum1,now_time.tv_sec,now_time.tv_usec);
      //fclose(fp2);
      p->kill();
     }
     else if((compare_sid_nid(trap_hello_nid_5,trap_data->nid,16))&&(compare_sid_nid(trap_hello_sid2,trap_data->sid,20)))
     {
      printf("pc-5 data\n");
      gettimeofday(&now_time,NULL);
      strNum1=trap_data->port1;
      strNum2=trap_data->port2;

      fp3=fopen(filename3,"a+");
      fprintf(fp3,"port1=%ld port2=%ld time:%d.%d\n",strNum2,strNum1,now_time.tv_sec,now_time.tv_usec);
      fclose(fp3);
      p->kill();
      }
     else if((compare_sid_nid(trap_hello_nid_6,trap_data->nid,16))&&(compare_sid_nid(trap_hello_sid2,trap_data->sid,20)))
     {
      printf("pc-6 data\n");
      strNum1=trap_data->port1;
      strNum2=trap_data->port2;
      gettimeofday(&now_time,NULL);
      fp4=fopen(filename4,"a+");
      fprintf(fp4,"port1=%ld port2=%ld time:%d.%d\n",strNum2,strNum1,now_time.tv_sec,now_time.tv_usec);
      fclose(fp4);
      p->kill();

      }
   } 
 
    return 0;
    
}

void trap_rm::push(int, Packet *p)
{
		if (Packet *q = smaction(p))
		output(0).push(q);
}

Packet *trap_rm::pull(int)
{
	if (Packet *p = input(0).pull())
		return smaction(p);
	else
		return 0;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(trap_rm)



