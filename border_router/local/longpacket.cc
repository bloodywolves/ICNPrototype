#include <click/config.h>
#include "longpacket.hh"
#include <sys/timeb.h>
#include <sys/time.h>
#include <stdlib.h> 
#include <string.h>
CLICK_DECLS

Longpacket :: Longpacket()
{
	PacketCount=0;
}

Longpacket :: ~Longpacket()
{

}

void Longpacket :: push(int port, Packet *packet)
{
	packet->kill();
	char c[1000000]={'A'};
	WritablePacket * newpkg=Packet :: make (c,1000000);

	gettimeofday(&Time,NULL);
	long timeuse=1000000*Time.tv_sec+Time.tv_usec;
	printf("Sent Time         : %ld us\n", timeuse);
	PacketCount++;
	printf("Sent Packets Num  : %d\n", PacketCount);
	PacketLength=newpkg->length();
	printf("Sent Packet length: %d\n", PacketLength);

	output(0).push(newpkg);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Longpacket)
ELEMENT_MT_SAFE(Longpacket)
