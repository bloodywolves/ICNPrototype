/*  jiangjiali shi ge da shuai bi*/
#ifndef SHUAI_HH
#define SHUAI _HH
#include <click/element.hh>

CLICK_DECLS
class Shuai : public Element { public:
	int finalsize;
	Shuai();
	~Shuai();
	const char *class_name() const	{ return "Shuai"; }
  	const char *port_count() const	{ return "1/1"; }
  	const char *processing() const 	{ return PUSH; }

  	void push(int port, Packet *packet);

};
CLICK_ENDDECLS
#endif


