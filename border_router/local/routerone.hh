/*Basic Router Model*/
#ifndef ROUTERONE_HH
#define ROUTERONE_HH
#include <click/element.hh>

CLICK_DECLS
class RouterOne : public Element { public:

	uint8_t type;
	uint8_t seq;
	uint8_t resend;
	uint8_t num;

	RouterOne();
	~RouterOne();

	const char *class_name() const	{ return "RouterOne"; }
  	const char *port_count() const	{ return "3/3"; }
  	const char *processing() const 	{ return PUSH; }

  	void push(int port, Packet *packet);

};
CLICK_ENDDECLS
#endif


