#include <click/config.h>
#include "shuai.hh"
#include <sys/timeb.h>
#include <sys/time.h>
#include <stdlib.h> 
#include <string.h>
CLICK_DECLS

Shuai :: Shuai()
{
	finalsize=0;
}

Shuai :: ~Shuai()
{

}

void Shuai :: push(int port, Packet *packet)
{
	char ip[4];
	struct pac{
		//IPAddress addr;
		int pid;
		char content[100];
	} pkg;
	ip[0]=rand()%223+1;
	ip[1]=rand()%256;
	ip[2]=rand()%256;
	ip[3]=rand()%256;
	IPAddress addr = IPAddress( (const unsigned char * )ip );
	pkg.pid=rand()%5000;
	memset(pkg.content, 1, 100);
	//char *haha=(char *)packet->data();
	//memcpy(haha, (char *)&pkg, 200);
	packet->kill();
	WritablePacket * newpkg=Packet :: make ((const char *)&pkg, sizeof(pkg));
	newpkg->set_dst_ip_anno (addr);
	int pkglength=newpkg->length();
	printf("shuai bi jiali is %d\n", pkglength);
	struct timeval start;
	finalsize++;
	output(0).push(newpkg);
	gettimeofday(&start,NULL);
	long timeuse=1000000*start.tv_sec+start.tv_usec;
	printf("shuai bi jiali (in) time is %ld\n", timeuse);
	printf("fa chu de bao is %d\n", finalsize);

}

CLICK_ENDDECLS
EXPORT_ELEMENT(Shuai)
ELEMENT_MT_SAFE(Shuai)
