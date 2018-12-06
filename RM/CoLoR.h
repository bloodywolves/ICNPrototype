#ifndef _CoLoR_H_
#define _CoLoR_H_ 

//#define CONTENTLEN 4
#define PUBKEYLEN 4


typedef struct CoLoR_get
{
	uint8_t version_type;	//�汾4λ������4λ
	uint8_t ttl;		//����ʱ��
	uint16_t total_len;	//�ܳ���

	uint16_t port_src;	//Դ�˿ں�
	uint16_t port_dst;	//Ŀ�Ķ˿ں�
	uint16_t minmal_PID_CP; //pid�ı������
	uint8_t PIDs;		//PID����Ŀ
	uint8_t Offest_RES;     //λ����ȡOffset
	uint32_t offset;	//ƫ����
	uint32_t length;	//ƫ�Ƴ���
	uint16_t content_len;	//��Կ����
	uint16_t mtu;		//����䵥Ԫ
	uint16_t publickey_len;	//��Կ����
	uint16_t checksum;	//�����
	uint8_t nid_sid[16];	//NID part of an SID������Ϊ16�ֽ�
	uint8_t l_sid[20]; 	//SID�ĳ���Ϊ20�ֽ�
	uint8_t nid[16];	//NID�ĳ���Ϊ16�ֽ�
       //	uint8_t content[CONTENTLEN];	// Content characteristics
        uint32_t cc_flow_packets;
        uint32_t cc_flow_size;
        uint32_t cc_flow_duration;
	uint8_t publickey[PUBKEYLEN];	//��Կ
} CoLoR_get_t;
#endif
