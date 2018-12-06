#ifndef _TRAP_H
#define _TRAP_H

//define if the port is up/down. 1 for up; 0 for down
#define TRAP_CONNECTION	(1 << 0)
//define if the bandwidth efficiency is too high,or normal 
#define TRAP_BANDWIDTH_HIGH	(1 << 1)
//define if the bandwidth efficiency is too low,or normal
#define TRAP_BANDWIDTH_LOW	(1 << 2)
//define if the link A and B is connected
#define TRAP_LINKCONNECT	(1 << 3)
//define if the packet loss of link between A and B is too much or not
#define TRAP_LINKLOSS	(1 << 4)
//define if the delay of link between A and B is too high or not
#define TRAP_LINKDELAY	(1 << 5)
//the port connect to RM
#define TRAP_PORT	"eth2"

//mark the trap is recovered
/************************************************************/
/*
//define if the port is up/down. 1 for up; 0 for down
#define RE_TRAP_CONNECTION	(0 << 0)
//define if the bandwidth efficiency is too high,too low,or normal 
#define RE_TRAP_BANDWIDTH	(0 << 1 )
//define if the link A and B is connected
#define RE_TRAP_LINKCONNECT	(0 << 2)
//define if the packet loss of link between A and B is too much or not
#define RE_TRAP_LINKLOSS	(0 << 3)
//define if the delay of link between A and B is too high or not
#define RE_TRAP_LINKDELAY	(0 << 4)
*/
/*************************************************************/
struct TRAP_DATA_HEADER
{
//ethernet头
	uint8_t ether_dhost[6]; //目地硬件地址
	uint8_t ether_shost[6]; //源硬件地址
	uint16_t ether_type; //网络类型
  
 //       uint8_t iphead[20];

	//CoLoR-Data头
	uint8_t version_type;////版本4位，类型4位
	uint8_t ttl;//生存时间
	uint16_t total_len;//总长度

	uint16_t port_no;//端口号
	uint16_t checksum;//检验和

	uint8_t sid_len;//SID长度
	uint8_t nid_len;//NID长度
	uint8_t pid_n;//PID数量
	uint8_t options_static;//固定首部选项

	int OFFSET;//the offset position
	int LENGTH;//the offset length

	uint8_t options_reserved[8];

	uint8_t signature_algorithm;//签名算法
	uint8_t if_hash_cache;//是否哈希4位，是否缓存4位
	uint16_t options_dynamic;//可变首部选项

	uint8_t sid[20];//SID
	uint8_t nid[16];//NID
};

struct TRAP_HEADER
{
//以太网帧头
	uint8_t ether_dhost[6]; //目的MAC
	uint8_t ether_shost[6]; //源MAC
	uint16_t ether_type; //协议类型

	//uint8_t iphead[20];//ip包头部

// color协议get包首部内容
	uint8_t version_type;//版本号和类型号
	uint8_t ttl;//生存时间
	uint16_t total_len;//总长度

	uint16_t port_no;//端口号
	uint16_t checksum;//校验和
    
	uint8_t sid_len;//SID长度
	uint8_t nid_len;//NID长度
	uint8_t pid_n;//PID数量
	uint8_t options_static;//static options

	int OFFSET;//the offset position
	int LENGTH;//the offset length

	uint8_t options_reserved[8];

	uint16_t publickey_len;//publickey length
	uint16_t mtu;//transmission MTU

	uint8_t sid[20];//SID
	uint8_t nid[16];//NID
};
/*
union TRAP_INFO
{
	float info_f;
	int info_l; 
};
*/
struct TRAP_NODE
{
	char source_ip[16];
	char dest_ip[16];
};

struct TRAP_SBD
{
	char trap_nid[16];
	char port_ip[16];
};

//trap消息
struct TRAP_MESSAGE
{
	struct TRAP_SBD trap_sbd;
	uint32_t trap_type;//消息类型
        long long int port1_data;
        long long int port2_data;
};

//get-trap-hello
struct TRAP_HELLO_PKG
{
	struct TRAP_HEADER trap_head;
	struct TRAP_SBD trap_sbd;
};

//data-trap-update
struct TRAP_DATA_PKG
{
	struct TRAP_DATA_HEADER trap_data_head;
	struct TRAP_MESSAGE trap_message;
};

struct PORT_TRAP_STATE
{
	int trap_flag;//判断当前感知周期中是否需要发送trap消息
	char port_ip[16];
	uint32_t trap_state;
};
#endif
