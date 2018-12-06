#ifndef CLICK_IPBORDERROUTER_HH
#define CLICK_IPBORDERROUTER_HH
#include <click/element.hh>
#include <click/string.hh>
#include <click/hashmap.hh>
CLICK_DECLS


struct iphead
{
	uint8_t version_len;
	uint8_t type;
	uint16_t total_len;
	uint16_t id;
	uint16_t flags_offset;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t checksum;
	uint32_t s_ip;
	uint32_t d_ip;
};	

struct CoLoR_get
{
	uint8_t version_type;////�汾4λ������4λ
	uint8_t ttl;//����ʱ��
	uint16_t total_len;//�ܳ���
	uint16_t port_no1;//�˿ں�1
	uint16_t port_no2;//�˿ں�2
	uint16_t minpid;//��Сpid��̬����
	uint8_t pids_o;//pid����
	uint8_t res;//�������ֽ�
	uint8_t offset[4];//ƫ����
	uint8_t length[4];//ƫ�Ƴ���
	uint16_t content_len;//���ݳ���
	uint16_t mtu;//����䵥Ԫ
	uint16_t publickey_len;//��Կ����
	uint16_t checksum;//�����
	
	uint8_t n_sid[16];//SID��NID����
	uint8_t l_sid[20];//SID��L����
	uint8_t nid[16];//NID
	
};

struct CoLoR_data
{
	uint8_t version_type;////�汾4λ������4λ
	uint8_t ttl;//����ʱ��
	uint16_t total_len;//�ܳ���
	uint16_t port_no1;//�˿ں�1
	uint16_t port_no2;//�˿ں�2
	uint16_t minpid;//��Сpid��̬����
	uint8_t pids_o;//pid������λ������offset��־��λ
	uint8_t res;//�������ֽ�
	uint8_t offset[4];//ƫ����
	uint8_t length[4];//ƫ�Ƴ���
	uint16_t next_header;//��һ����ͷ����
	uint16_t checksum;//�����
	uint8_t reserved[4];//�������ֽ�
	uint8_t n_sid[16];//SID��NID����
	uint8_t l_sid[20];//SID��L����
	uint8_t nid[16];//NID
	
};





class IPBorderRouter : public Element { public:

  IPBorderRouter();
  ~IPBorderRouter();

  const char *class_name() const	{ return "IPBorderRouter"; }
  const char *port_count() const	{ return "1-2/1-5"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);
  HashMap<uint32_t,int>ip_nic;
  HashMap<int,uint32_t>s_ip;
  HashMap<String,uint32_t>pid_ip;
  HashMap<String,uint32_t>nid_ip;
  HashMap<String,uint32_t>pid_time;
  char pidpid[100][4];
  int pid_c_num;
  uint32_t pasttime;



};



struct color_ctl_pub
{
    uint8_t ver_type;
    uint8_t controltype;
    uint16_t total_len;
 
    uint16_t checksum;
    uint8_t sign_alg;
    uint8_t option;   //ack 1bit, flag 1 bit, kind 2 bit,p 1 bit, 3 bit reserve
 
    uint16_t series_number_ack;
    uint16_t series_number_send;

    uint16_t item;
    uint16_t MTU;

    uint8_t EID[16];
    
    uint16_t public_key_len;
    uint16_t resevered;
   
    uint8_t public_key[49];
    //uint8_t pad[3];  

    uint8_t signature[55];
    //uint8_t pad[1];

};

struct pid_item
{
	char pre_pid[4];
	char cur_pid[4];
	uint32_t source_ip;
	uint32_t dest_ip;
	uint32_t time;

};


class PushIPBorderRouter : public Element { public:

  PushIPBorderRouter();
  ~PushIPBorderRouter();

  const char *class_name() const	{ return "PushIPBorderRouter"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return PUSH; }

  void push(int, Packet *);

};


class PullIPBorderRouter : public Element { public:

  PullIPBorderRouter();
  ~PullIPBorderRouter();

  const char *class_name() const	{ return "PullIPBorderRouter"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return PULL; }

  Packet *pull(int);

};

CLICK_ENDDECLS
#endif
