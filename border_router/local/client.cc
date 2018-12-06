/*
 * client.{cc,hh} -- do-nothing element
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <iostream>
#include <string>
#include <click/config.h>
#include "client.hh"
using namespace std;
CLICK_DECLS

Client::Client()
{
}

Client::~Client()
{
}

Packet *
Client::simple_action(Packet *p)
{
  return p;
}

void
Client::push(int port, Packet *packet)
{
	WritablePacket *q=packet->uniqueify();
	if(port==0)
	{
		cout<<"port0 packet"<<endl;
	//	output(1).push(q);
	//}
	//else if(port==1)
	//{
		//printf("%s\n",packet->data());
		cout<<"old packet  "<<q->data()<<endl;
		cout<<"old length="<<q->length()<<endl;
		char a[20];
		memcpy(a,(const char*)q->data(),q->length());
		cout<<a[0]<<endl;
		int outinterface=a[0]-'0';
		cout<<"outinterface="<<outinterface<<endl;

		q->pull(4);
		cout<<"new packet  "<<q->data()<<endl;
		output(outinterface).push(q);
	}


/*
	if     (strncmp((const char *)packet->data(),"ABC",3) == 0)
		output(0).push(packet);
	else if(strncmp((const char *)packet->data(),"DEF",3) == 0)
		output(1).push(packet);
	else if(strncmp((const char *)packet->data(),"GHI",3) == 0)
		output(2).push(packet);
	else if(strncmp((const char *)packet->data(),"JKL",3) == 0)
		output(3).push(packet);
*/
}

PushClient::PushClient()
{
}

PushClient::~PushClient()
{
}

void
PushClient::push(int, Packet *p)
{
  //output(0).push(p);
	output(2).push(p);
}

PullClient::PullClient()
{
}

PullClient::~PullClient()
{
}

Packet *
PullClient::pull(int)
{
  return input(0).pull();
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Client)
EXPORT_ELEMENT(PushClient)
EXPORT_ELEMENT(PullClient)
ELEMENT_MT_SAFE(Client)
ELEMENT_MT_SAFE(PushClient)
ELEMENT_MT_SAFE(PullClient)
