#include <click/config.h>
#include "basic.hh"
#include <sys/timeb.h>
#include <sys/time.h>
#include <stdlib.h> 
#include <string.h>
CLICK_DECLS

Basic :: Basic()
{

}

Basic :: ~Basic()
{

}

void Basic :: push(int port, Packet *packet)
{
	//清除触发包
	packet->kill();

	//创建新包
	WritablePacket * newpkg=Packet :: make ((const char *)&pkg, sizeof(pkg));

	//发包
	output(0).push(newpkg);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Basic)
ELEMENT_MT_SAFE(Basic)
