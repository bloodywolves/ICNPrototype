/*Basic Router Model*/
#ifndef ROUTERFOUR_HH
#define ROUTERFOUR_HH
#include <click/element.hh>

CLICK_DECLS
class RouterFour : public Element { public:

	uint8_t type;
	uint8_t seq;
	uint8_t resend;
	uint8_t num;

	RouterFour();
	~RouterFour();

	const char *class_name() const	{ return "RouterFour"; }
  	const char *port_count() const	{ return "4/4"; }
  	const char *processing() const 	{ return PUSH; }

  	void push(int port, Packet *packet);

};
CLICK_ENDDECLS
#endif


