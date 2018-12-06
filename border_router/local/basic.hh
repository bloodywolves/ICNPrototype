/*Basic Click Model*/
#ifndef BASIC_HH
#define BASIC_HH
#include <click/element.hh>

CLICK_DECLS
class Basic : public Element { public:

	struct packet{
		uint32_t sequence;
		uint8_t data[996];
	} pkg;

	Basic();
	~Basic();
	const char *class_name() const	{ return "Basic"; }
  	const char *port_count() const	{ return "1/1"; }
  	const char *processing() const 	{ return PUSH; }

  	void push(int port, Packet *packet);

};
CLICK_ENDDECLS
#endif


