#ifndef CLICK_SERVER_HH
#define CLICK_SERVER_HH
#include <click/element.hh>

CLICK_DECLS

class Server : public Element { public:

	int PacketCount;
	int PacketLength;
	struct timeval Time;

	Server();
	~Server();

	const char *class_name() const	{ return "Server"; }
	const char *port_count() const	{ return "1/1"; }
  	const char *processing() const 	{ return PUSH; }

	void push(int port, Packet *packet);
};

CLICK_ENDDECLS
#endif
