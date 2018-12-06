/*Basic Router Model*/
#ifndef BASICROUTER_HH
#define BASICROUTER_HH
#include <click/element.hh>

CLICK_DECLS
class BasicRouter : public Element { public:

	uint8_t type;
	uint8_t seq;
	uint8_t resend;

	BasicRouter();
	~BasicRouter();

	const char *class_name() const	{ return "BasicRouter"; }
  	const char *port_count() const	{ return "2/2"; }
  	const char *processing() const 	{ return PUSH; }

  	void push(int port, Packet *packet);

};
CLICK_ENDDECLS
#endif


