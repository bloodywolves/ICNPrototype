#ifndef CLICK_SENDDATA_HH
#define CLICK_SENDDATA_HH
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

class SendData : public Element { public:

  SendData();
  ~SendData();

  const char *class_name() const	{ return "SendData"; }
  const char *port_count() const	{ return "0/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};

/*
=c
PushNull

=s basictransfer
push-only null element

=d
Responds to each pushed packet by pushing it unchanged out its first output.

=a
Null, PullNull
*/

class PushSendData : public Element { public:

  PushSendData();
  ~PushSendData();

  const char *class_name() const	{ return "PushSendData"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return PUSH; }

  void push(int, Packet *);

};









/*
=c
PullNull

=s basictransfer
pull-only null element

=d
Responds to each pull request by pulling a packet from its input and returning
that packet unchanged.

=a
Null, PushNull */

class PullSendData : public Element { public:

  PullSendData();
  ~PullSendData();

  const char *class_name() const	{ return "PullSendData"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return PULL; }

  Packet *pull(int);

};

CLICK_ENDDECLS
#endif
