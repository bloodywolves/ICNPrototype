#ifndef CLICK_CLIENT_HH
#define CLICK_CLIENT_HH
#include <click/element.hh>
#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <fstream>

#include <arpa/inet.h>
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

class Client : public Element { public:

  Client();
  ~Client();

  const char *class_name() const	{ return "Client"; }
  const char *port_count() const	{ return "1/4"; }

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

class PushClient : public Element { public:

  PushClient();
  ~PushClient();

  const char *class_name() const	{ return "PushClient"; }
  const char *port_count() const	{ return "1/6"; }
  //const char *processing() const	{ return PUSH; }

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

class PullClient : public Element { public:

  PullClient();
  ~PullClient();

  const char *class_name() const	{ return "PullClient"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return PULL; }

  Packet *pull(int);

};

CLICK_ENDDECLS
#endif
