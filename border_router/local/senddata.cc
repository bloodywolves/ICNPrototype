#include <iostream>
#include <click/config.h>
#include "senddata.hh"
using namespace std;
CLICK_DECLS

SendData::SendData()
{
}

SendData::~SendData()
{
}

Packet *
SendData::simple_action(Packet *p)
{
  return p;
}

void
SendData::push(int port, Packet *packet)
{
		WritablePacket *q=packet->uniqueify();
		q->push(4);

		char dada[4]={'A','B','C'};

		memcpy((unsigned char *)q->data(),&dada,3);
		cout<<"success"<<endl;

		output(0).push(q);
	
	
}

PushSendData::PushSendData()
{
}

PushSendData::~PushSendData()
{
}

void
PushSendData::push(int, Packet *p)
{
  //output(0).push(p);
	output(2).push(p);
}

PullSendData::PullSendData()
{
}

PullSendData::~PullSendData()
{
}

Packet *
PullSendData::pull(int)
{
  return input(0).pull();
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SendData)
EXPORT_ELEMENT(PushSendData)
EXPORT_ELEMENT(PullSendData)
ELEMENT_MT_SAFE(SendData)
ELEMENT_MT_SAFE(PushSendData)
ELEMENT_MT_SAFE(PullSendData)
