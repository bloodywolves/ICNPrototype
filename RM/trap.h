#ifndef TRAP_H
#define TRAP_H 

struct trap_get_header
  {
        uint8_t ether_dhost[6]; //目地硬件地址
        uint8_t ether_shost[6]; //源硬件地址
        uint16_t ether_type; //网络类型

        uint8_t version_type;//版本号和类型号
        uint8_t ttl;//生存时间
        uint16_t total_len;//总长度

        uint16_t port_no;//端口号
        uint16_t checksum;//校验和

        uint8_t sid_len;//SID长度
        uint8_t nid_len;//NID长度
        uint8_t pid_n;//PID数量
        uint8_t options_static;//static options

        uint32_t OFFSET;//the offset position
        uint32_t LENGTH;//the offset length
        uint8_t options_reserved[8];

        uint16_t publickey_len;//publickey length
        uint16_t mtu;//transmission MTU

        uint8_t sid[20];//SID
        uint8_t nid[16];//NID
  
        char trap_nid[16];
        char port_ip[16];
  };

struct trap_data_header
{
           uint8_t ether_dhost[6]; //目地硬件地址
           uint8_t ether_shost[6]; //源硬件地址
           uint16_t ether_type; //网络类型


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
             
           uint32_t trap_type[2];     
             
           long long int port1;
           long long int port2;

};
#endif 


