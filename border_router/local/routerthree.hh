/*Basic Router Model*/
#ifndef ROUTERTHREE_HH
#define ROUTERTHREE_HH
#include <click/element.hh>

CLICK_DECLS
class RouterThree : public Element { public:

	uint8_t type;
	uint8_t seq;
	uint8_t resend;
	uint8_t num;

	RouterThree();
	~RouterThree();

	const char *class_name() const	{ return "RouterThree"; }
  	const char *port_count() const	{ return "4/4"; }
  	const char *processing() const 	{ return PUSH; }

  	void push(int port, Packet *packet);

};
CLICK_ENDDECLS
#endif


