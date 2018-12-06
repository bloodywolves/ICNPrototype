#ifndef LONGPACKET_HH
#define LONGPACKET_HH
#include <click/element.hh>

CLICK_DECLS

class Longpacket : public Element { public:

	int PacketCount;
	int PacketLength;
	struct timeval Time;

	Longpacket();
	~Longpacket();

	const char *class_name() const	{ return "Longpacket"; }
  	const char *port_count() const	{ return "1/1"; }
  	const char *processing() const 	{ return PUSH; }

  	void push(int port, Packet *packet);
};

CLICK_ENDDECLS
#endif
