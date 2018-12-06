#ifndef CLICK_COLOR_GET_NEW_HH
#define CLICK_COLOR_GET_NEW_HH

#include <click/element.hh>

CLICK_DECLS

class color_get_new : public Element
{
	public:	
				
		color_get_new();
		~color_get_new();

		const char *class_name() const { return "color_get_new"; }
		const char *port_count() const { return PORTS_1_1; }
	
		int configure(Vector<String> &, ErrorHandler *);
		bool can_live_reconfigure() const { return true; }
	
		struct get_header_new
        {
	    uint8_t version_type;
            uint8_t ttl;
            uint16_t total_len;
           
            uint16_t port_src;
            uint16_t port_dst;

            uint16_t minimal_PID_CP;
            uint8_t PIDs;
            uint8_t offsets_RES;

            uint32_t offset;

            uint32_t length;

            uint16_t content_len;
            uint16_t mtu;

            uint16_t publickey_len;
            uint16_t checksum;
            
            uint8_t nid_EID[16];
            uint8_t l_EID[20];
            uint8_t nid[16];
            uint32_t cc_packets;
            uint32_t cc_flow_size;
            uint32_t cc_flow_time;
            uint8_t publickey[4];
        };//96 Bytes total without ether_head without ip
                                        
		int i;
                char *buffer;
                char *buffer2;
		Packet *smaction(Packet *);
		void push(int, Packet *);
		Packet *pull(int);
		
		int initialize(ErrorHandler *);
};

CLICK_ENDDECLS
#endif
	
