#ifndef CLICK_TRAP_RM_HH
#define CLICK_TRAP_RM_HH

#include <click/element.hh>

CLICK_DECLS

class trap_rm : public Element
{
	public:	
				
		trap_rm();
		~trap_rm();

		const char *class_name() const { return "trap_rm"; }
		const char *port_count() const { return PORTS_1_1; }
	
		int configure(Vector<String> &, ErrorHandler *);
		bool can_live_reconfigure() const { return true; }
	
		struct trap_get_header
        {
            uint8_t ether_dhost[6];
            uint8_t ether_shost[6];
            uint16_t ether_type;
           
            uint8_t ip[20]; 
          
	    uint8_t version_type;
            uint8_t ttl;
            uint16_t total_len;
           
            uint16_t port_no;
            uint16_t checksum;
            
            uint8_t sid_len;
            uint8_t nid_len;
            uint8_t pid_n;
            uint8_t reserved;

            uint32_t offset;
            uint32_t length;
            uint32_t Reserved[2];

            uint16_t pk_len;
	    uint16_t mtu;

            uint8_t sid[20];
            uint8_t nid[16];
			
            char trap_nid[16];
	    char port_ip[16];

        };

        struct data_header
        {
           uint8_t ether_dhost[6];
	   uint8_t ether_shost[6];
           uint16_t ether_type;
		
           uint8_t version_type;
           uint8_t ttl;
           uint16_t total_len;
             
           uint16_t port_no;
           uint16_t checksum;

           uint8_t sid_len;
           uint8_t nid_len;
           uint8_t pid_n;
           uint8_t tos;

           uint32_t offset;
           uint32_t length;

           uint32_t reserved[2];
			
           uint8_t sig_alg;
           uint8_t if_hash_cache;
           uint16_t opt;
			
           uint8_t sid[20];
           uint8_t nid[16];
			
           char trap_nid[16];
           char port_ip[16];
			
           long long int port1;
           long long int port2;
            
        };
                                        
		uint32_t ss;
		uint32_t mm;
		int i;
		Packet *smaction(Packet *);
		void push(int, Packet *);
		Packet *pull(int);
		
		int initialize(ErrorHandler *);
};

CLICK_ENDDECLS
#endif
	
