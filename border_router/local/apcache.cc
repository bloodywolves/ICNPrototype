#include <click/config.h>
#include "apcache.hh"
#include <sys/timeb.h>
#include <sys/time.h>
#include <stdlib.h> 
#include <string.h>
CLICK_DECLS

APcache :: APcache()
{
	incache=0;
}

APcache :: ~APcache()
{

}

void APcache :: push(int port, Packet *packet)
{


	type=*packet->data();
	seq=*(packet->data()+1);


	if(type==0)
	{
		interest=1;
		data=0;
	}
	else if(type==1)
	{
		interest=0;
		data=1;
	}
	else
	{
		interest=0;
		data=0;
	}


	if     (port==0&&data==1)
	{
		//printf("[Error] Data packet from Interest side.\n");
		packet->kill();
		//output(2).push(packet);
	}
	else if(port==1&&data==1)
	{
		//printf("[Report] Data packet.\n");
		output(0).push(packet);
	}
	else if(port==0&&interest==1)
	{
		if(incache==0)
		{
			//printf("[Report] Interest packet.\n");
			output(1).push(packet);
		}
		else if(incache==1)
		{
			//创建新包
			WritablePacket * newpkg=Packet :: make ((const char *)&pkg, sizeof(pkg));
			pkg.data[0]=type;
			pkg.data[1]=seq;
			//清除旧包
			packet->kill();

			output(0).push(newpkg);
		}
	}
	else if(port==1&&interest==1)
	{
		//printf("[Error] Interest packet from Data side.\n");
		packet->kill();
		//output(2).push(packet);
	}
}

CLICK_ENDDECLS
EXPORT_ELEMENT(APcache)
ELEMENT_MT_SAFE(APcache)
