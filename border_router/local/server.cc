/*
 * server.{cc,hh} -- do-nothing element
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
#include <click/config.h>
#include "server.hh"
CLICK_DECLS

Server::Server()
{
	PacketCount=0;
}

Server::~Server()
{
}

void
Server::push(int port, Packet *packet)
{
	gettimeofday(&Time,NULL);
	long timeuse=1000000*Time.tv_sec+Time.tv_usec;
	printf("Recv Time         : %ld us\n", timeuse);
	PacketCount++;
	printf("Recv Packets Num  : %d\n", PacketCount);
	PacketLength=packet->length();
	printf("Recv Packet length: %d\n", PacketLength);
/*
	packet->push(4);
	WritablePacket *q=packet->uniqueify();
	
	memcpy((unsigned char*)q->data(),"3abc",4);
	output(0).push(q);
*/
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Server)
ELEMENT_MT_SAFE(Server)
