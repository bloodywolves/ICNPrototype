/*Basic Router Model*/
#ifndef ROUTERTWO_HH
#define ROUTERTWO_HH
#include <click/element.hh>

CLICK_DECLS
class RouterTwo : public Element { public:

	uint8_t type;
	uint8_t seq;
	uint8_t resend;
	uint8_t num;

	RouterTwo();
	~RouterTwo();

	const char *class_name() const	{ return "RouterTwo"; }
  	const char *port_count() const	{ return "4/4"; }
  	const char *processing() const 	{ return PUSH; }

  	void push(int port, Packet *packet);

};
CLICK_ENDDECLS
#endif


