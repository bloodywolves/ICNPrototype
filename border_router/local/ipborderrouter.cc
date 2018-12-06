#include <click/config.h>
#include "ipborderrouter.hh"
#include <sys/time.h> 
#include <time.h>
CLICK_DECLS

IPBorderRouter::IPBorderRouter()
{
/*---------------------------------------*/
ip_nic.insert(0x20b0a0a,1);
ip_nic.insert(0x103a8c0,2);




/*---------------------------------------*/
s_ip.insert(1,0x10b0a0a);
s_ip.insert(2,0x203a8c0);



/*---------------------------------------*/

pid_ip.insert("PID2",0x20b0a0a);

/*---------------------------------------*/
nid_ip.insert("d3rm",0x103a8c0);



/*---------------------------------------*/
pid_c_num=0;
pasttime=0;
}

IPBorderRouter::~IPBorderRouter()
{
}

Packet *
IPBorderRouter::simple_action(Packet *p)
{
  return p;
}

void
IPBorderRouter::push(int port, Packet *packet)
{
	time_t rawtime;
	time(&rawtime);
/*	if(pasttime<(uint32_t)(rawtime))
	{	
		
		int pid_num;
		String pid_time_key;
		for(pid_num=0;pid_num<100;pid_num++)
		{	
			char pidpidkey[100];
			memset(pidpidkey,0,100);
			memcpy(pidpidkey,pidpid[pid_num],4);
			pid_time_key=pidpidkey;
			uint32_t pidtime=pid_time.find(pid_time_key);
			if(pidtime!=0)
			printf("deadline=%ld,current time=%ld\n",pidtime,rawtime);
			if(pidtime<rawtime&&pidtime!=0)
			{
				pid_time.remove(pid_time_key);
				pid_ip.remove(pid_time_key);
			}
			
		}
		pasttime=(uint32_t)(rawtime);
	}
	
*/
	//struct timeval start, end;
	//gettimeofday( &start, NULL );
	struct iphead ip;
	memcpy(&ip,(unsigned char *)packet->data()+14,20);
	if(ip.d_ip==s_ip.find(1)||ip.d_ip==s_ip.find(2))
        // if(1)
	{
		unsigned char version[4];
		memset(version,0,4);
		memcpy(version,packet->data()+34,4);
		if(version[0]==160&&port==0)
		{	

			struct CoLoR_get head;
			memcpy(&head,(unsigned char *)packet->data()+34,80);;
			int n=(int)ntohs(head.total_len);
			unsigned char pid[100];
			memset(pid,0,100);
			memcpy(pid,packet->data()+n+30,4);
			String pid_key=(char *)pid;
			ip.d_ip=pid_ip.find(pid_key);
			ip.s_ip=s_ip.find(ip_nic.find(ip.d_ip));
			memcpy((unsigned char *)packet->data()+14,&ip,20);
			output(ip_nic.find(ip.d_ip)).push(packet);
			//printf("get from inside,length=%d,pids=%d,pid=%s,outport=eth%d\n",n,(int)head.pids_o,pid,ip_nic.find(ip.d_ip)-1);
		}
		else if(version[0]==160&&port==1)
		{
			ip.d_ip=nid_ip.find("d3rm");
			ip.s_ip=s_ip.find(ip_nic.find(ip.d_ip));
			memcpy((unsigned char *)packet->data()+14,&ip,20);
			output(ip_nic.find(ip.d_ip)).push(packet);
			printf("get from outside,outport=eth%d\n",ip_nic.find(ip.d_ip)-1);
		}


		else if(version[0]==161&&port==0)
		{
			struct CoLoR_data head;
			memcpy(&head,(unsigned char *)packet->data()+34,80);
			int pids=(int)head.pids_o;
			printf("%d\n",pids);
			unsigned char pid[4];
			if(pids!=0)
			{	
				head.pids_o=pids-1;
				head.total_len=htons((int)(ntohs(head.total_len))-4);
				unsigned char newhead[110+4*pids];
				memcpy(newhead,packet->data(),110+4*pids);
				memcpy(pid,packet->data()+110+4*pids,4);
				packet->pull(4);
				memcpy((unsigned char *)packet->data(),newhead,110+4*pids);
				memcpy((unsigned char *)packet->data()+34,&head,80);
				String pid_key=(char *)pid;
				ip.d_ip=pid_ip.find(pid_key);
				ip.s_ip=s_ip.find(ip_nic.find(ip.d_ip));
				memcpy((unsigned char *)packet->data()+14,&ip,20);
				output(ip_nic.find(ip.d_ip)).push(packet);
				printf("data from inside,pids=%d,pid=%s,outport=eth%d\n",pids,pid,ip_nic.find(ip.d_ip)-1);
			}	
			else
				{
					
				}

		}
		else if(version[0]==161&&port==1)
		{
			struct CoLoR_data head;
			memcpy(&head,(unsigned char *)packet->data()+34,80);
			int pids=(int)head.pids_o;
			printf("%d\n",pids);
			unsigned char pid[4];
			memset(pid,0,4);
			if(pids!=0)
			{
				memcpy(pid,packet->data()+110+4*pids,4);
				String pid_key=(char *)pid;
				ip.d_ip=pid_ip.find(pid_key);
				ip.s_ip=s_ip.find(ip_nic.find(ip.d_ip));
				memcpy((unsigned char *)packet->data()+14,&ip,20);
				output(ip_nic.find(ip.d_ip)).push(packet);
				printf("data form outside,pids=%d,outport=eth%d\n",pids,ip_nic.find(ip.d_ip)-1);
			}
			else
			{
				unsigned char nid[16];
				memset(nid,0,16);
				memcpy(nid,packet->data()+98,16);
				String nid_key=(char *)nid;
				ip.d_ip=nid_ip.find(nid_key);
				ip.s_ip=s_ip.find(ip_nic.find(ip.d_ip));
				memcpy((unsigned char *)packet->data()+14,&ip,20);
				output(ip_nic.find(ip.d_ip)).push(packet);
				printf("data from outside,pids=%d,outport=eth%d\n",pids,ip_nic.find(ip.d_ip)-1);
				printf("%s\n",nid);
			}

		}


		else if(version[0]==163)
		{
			

			struct color_ctl_pub head;
			memcpy(&head,(unsigned char *)packet->data()+34,140);
			if(head.controltype==129)
			{
				int items=(int)(ntohs(head.item));
				struct pid_item dpid;
				memset(dpid.cur_pid,0,4);
				if(items=!0)
				{	
					
					items--;
					memcpy(&dpid,(unsigned char *)packet->data()+174+items*20,20);
					char linshi[100];
					memset(linshi,0,100);
					memcpy(linshi,dpid.cur_pid,4);


					memcpy(pidpid[pid_c_num],dpid.cur_pid,4);
					if(pid_c_num==99)
					pid_c_num=0;
					else
					pid_c_num++;


					String pid_key=(char *)linshi;
					if(dpid.source_ip==s_ip.find(1))
					pid_ip.insert(pid_key,dpid.dest_ip);
					else
					pid_ip.insert(pid_key,dpid.source_ip);
					time(&rawtime);
					uint32_t time_v=htonl(dpid.time)+(uint32_t)(rawtime);
					pid_time.insert(pid_key,time_v);
					//gettimeofday( &end, NULL );
					printf("Control packet,cur_pid=%s,source_ip=%lu,dest_ip=%lu,last time=%d\n",linshi,dpid.source_ip,dpid.dest_ip,htonl(dpid.time));
					//printf("PID table size=%d\n",pid_ip.size());				
		   			//printf("start : %d.%d\n", start.tv_sec, start.tv_usec);
   					//printf("end   : %d.%d\n", end.tv_sec, end.tv_usec);
					//output(1).push(packet);
				}
			}
			if((head.controltype==16||head.controltype==35)&&port==0)
			{
				int n=(int)ntohs(head.total_len);
				unsigned char pid[100];
				memset(pid,0,100);
				memcpy(pid,packet->data()+n+30,4);
				String pid_key=(char *)pid;
				ip.d_ip=pid_ip.find(pid_key);
				ip.s_ip=s_ip.find(ip_nic.find(ip.d_ip));
				memcpy((unsigned char *)packet->data()+14,&ip,20);
				output(ip_nic.find(ip.d_ip)).push(packet);
				printf("Control from inside,length=%d,pid=%s,outport=eth%d\n",n,pid,ip_nic.find(ip.d_ip)-1);
			
			

			}
			if((head.controltype==16||head.controltype==35)&&port==1)
			{
				packet->take(8);
				packet->put(4);
				head.total_len=ntohs(ntohs(head.total_len)-4);
				memcpy((unsigned char *)packet->data()+34,&head,140);
				ip.d_ip=nid_ip.find("d3rm");
				ip.s_ip=s_ip.find(ip_nic.find(ip.d_ip));
				memcpy((unsigned char *)packet->data()+14,&ip,20);
				output(ip_nic.find(ip.d_ip)).push(packet);
				printf("Control from outside,outport=eth%d\n",ip_nic.find(ip.d_ip)-1);
			}
			

		}

	
	}
	else
	{	
		if(ip_nic.find(ip.d_ip)>=1&&ip_nic.find(ip.d_ip)<=4)
		{	output(ip_nic.find(ip.d_ip)).push(packet);
			printf("NOT my packet,outport is eth%d\n",ip_nic.find(ip.d_ip)-1);
		}
		else
		{	output(0).push(packet);
			//printf("IP is wrong,discard!\n");
		}
	}
}

PushIPBorderRouter::PushIPBorderRouter()
{
}

PushIPBorderRouter::~PushIPBorderRouter()
{
}

void
PushIPBorderRouter::push(int, Packet *p)
{
  //output(0).push(p);
	output(2).push(p);
}

PullIPBorderRouter::PullIPBorderRouter()
{
}

PullIPBorderRouter::~PullIPBorderRouter()
{
}

Packet *
PullIPBorderRouter::pull(int)
{
  return input(0).pull();
}

CLICK_ENDDECLS
EXPORT_ELEMENT(IPBorderRouter)
EXPORT_ELEMENT(PushIPBorderRouter)
EXPORT_ELEMENT(PullIPBorderRouter)
ELEMENT_MT_SAFE(IPBorderRouter)
ELEMENT_MT_SAFE(PushIPBorderRouter)
ELEMENT_MT_SAFE(PullIPBorderRouter)
