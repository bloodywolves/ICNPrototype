#ifndef CLICK_DROP_HH
#define CLICK_DROP_HH
#include <click/element.hh>
CLICK_DECLS

/*
=c
Null

=s basictransfer
null element: passes packets unchanged

=d
Just passes packets along without doing anything else.

=a
PushNull, PullNull
*/

class Drop : public Element { public:

  Drop();
  ~Drop();

  const char *class_name() const	{ return "Drop"; }
  const char *port_count() const	{ return "1/0"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};

CLICK_ENDDECLS
#endif
