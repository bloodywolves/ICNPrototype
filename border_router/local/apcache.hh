/*APcache Click Model*/
#ifndef APCACHE_HH
#define APCACHE_HH
#include <click/element.hh>

CLICK_DECLS
class APcache : public Element { public:


	struct packet{
		uint8_t data[1000];
	} pkg;

	uint8_t type;
	uint8_t seq;

	int interest;
	int data;
	int incache;

	APcache();
	~APcache();
	const char *class_name() const	{ return "APcache"; }
  	const char *port_count() const	{ return "2/2"; }
  	const char *processing() const 	{ return PUSH; }

  	void push(int port, Packet *packet);

};
CLICK_ENDDECLS
#endif


