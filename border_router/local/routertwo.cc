/*Basic Router Model*/
#include <click/config.h>
#include "routertwo.hh"
#include <sys/timeb.h>
#include <sys/time.h>
#include <stdlib.h> 
#include <string.h>
CLICK_DECLS

RouterTwo :: RouterTwo()
{
	type=0;
	seq=0;
	resend=0;
	num=0;
}

RouterTwo :: ~RouterTwo()
{

}

void RouterTwo :: push(int port, Packet *packet)
{
	//主要结构：双网口基础路由器（网桥），双向无条件转发，提供基本的跨节点连通性
	//主要功能：判断类型与入口方向是否吻合（0-0，1-1），吻合则向另一侧转发，不吻合则丢包
	type  =*(packet->data());
	seq   =*(packet->data()+1);
	resend=*(packet->data()+2);
	num   =*(packet->data()+3);

	if     (port==0&&type==0)
	{
		//printf("[Error] Data packet from Interest side.\n");
		packet->kill();
	}
	else if((port==1||port==2||port==3)&&type==1)
	{
		//printf("[Error] Interest packet from Data side.\n");
		packet->kill();
	}
	else if((port==1||port==2||port==3)&&type==0)
	{
		output(0).push(packet);
	}
	else if(port==0&&type==1)
	{
		if(num==1)
			output(1).push(packet);
		else if(num==2)
			output(1).push(packet);
		else if(num==3)
			output(2).push(packet);
		else if(num==4)
			output(3).push(packet);
	}
}

CLICK_ENDDECLS
EXPORT_ELEMENT(RouterTwo)
ELEMENT_MT_SAFE(RouterTwo)
