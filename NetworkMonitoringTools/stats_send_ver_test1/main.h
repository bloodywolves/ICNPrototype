#ifndef _MAIN_H
#define _MAIN_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/sockios.h>
#include <malloc.h>
#include <memory.h>
//#include <mysql/mysql.h>
#include <netdb.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netpacket/packet.h>
#include <pthread.h>
#include <semaphore.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

// 统计时间间隔，单位：s
#define	INTERVAL	10
//最大记录时间，单位：s
#define MAX_REC_TIME	60 * 60
//数据库最大记录数
#define MAX_REC	MAX_REC_TIME / INTERVAL
//RM地址
#define RM_IP	"211.71.67.107"
//带宽利用率门限,单位：%
#define BAND_THRES_HIGH	0.0006
#define BAND_THRES_LOW 0.0001
//链路丢包率门限，单位：%
#define LINK_THRES_LOSS 75
//链路时延门限，单位：ms
#define LINK_THRES_DELAY 1
//本地请求监听端口
#define LOCAL_PORT 6677

//extern struct PORT_STATE_INDEX *port_state_index;
extern struct PORT_INDEX *port_index;
extern char host_name[32];
//静态信息部分
/**************************************************************************************************/

//网络端口具体参数
struct ADAPTER
{
	uint32_t	supported;	/* Features this interface supports */
	uint32_t	advertising;	/* Features this interface advertises */
	uint16_t	speed;		/* The forced speed, 10Mb, 100Mb, gigabit */
	int	duplex;		/* Duplex, half or full */
	int	port;		/* Which connector port */
	int	transceiver;	/* Which tranceiver to use */
	uint8_t	autoneg;	/* Enable or disable autonegotiation */	

};

//每个网络端口的静态信息
struct DEV_INFO
{
	char dev_name[16]; //网口名称
	uint8_t dev_mac[7];//mac地址
	char dev_ip[16];//ip地址
	char dev_broad_addr[32];//广播地址
	char dev_subnet_mask[32];//子网掩码
	int mtu;//接口MTU	
	struct ADAPTER adapter;//网络端口具体参数
};

//动态信息部分
/**************************************************************************************************/

//链路时延测试信息
struct LINK_STATS
{
	float packet_loss;//实际测试丢包率，单位：百分比
	float rtt_avg;//实际测试平均时延，单位：ms
	float rtt_min;//实际测试最小时延，单位：ms
	float rtt_max;//实际测试最大时延，单位：ms
	float rtt_mdev;//实际测试平均抖动值，单位：ms
};

//通信对端链路信息
struct DEST_INFO
{
	int linkloss_trap_flag;//判断是否汇报过链路丢包trap消息
	int linkdelay_trap_flag;//判断是否汇报过链路时延trap消息
	char dest_ip[16];
	struct LINK_STATS link_stats;
	struct DEST_INFO *next;
};

//get包计数器
struct GET_PACK_COUNT
{
	long long tx_packets;
	long long rx_packets;
	long long total_packets;//端口流经的总包数
	long long tx_bytes;
	long long rx_bytes;
	long long total_bytes;//端口流经的总字节数
};

//data包计数器
struct DATA_PACK_COUNT
{
	long long tx_packets;
	long long rx_packets;
	long long total_packets;//端口流经的总包数
	long long tx_bytes;
	long long rx_bytes;
	long long total_bytes;//端口流经的总字节数
};

//各网卡收发数据统计信息
struct STATS_INFO
{
	long long total_packets;//端口流经的总包数
	long long total_bytes;//端口流经的总字节数
	long long tx_packets;//发包数
	long long rx_packets;//收包数
	long long tx_bytes;//发送字节数
	long long rx_bytes;//接收字节数
	double total_packets_ps;//平均每秒流经总包数
	double tx_packets_ps;//平均每秒发包数
	double rx_packets_ps;//平均每秒收包数	
	double total_bytes_ps;//平均每秒流经总字节数
	double tx_bytes_ps;//平均每秒发送字节数
	double rx_bytes_ps;//平均每秒接收字节数
	struct GET_PACK_COUNT get_pack_count;//get包计数器
	struct DATA_PACK_COUNT data_pack_count;//data包计数器
};

//端口效率计算结果
struct EFFICIENCY
{
	float bandwidth;//带宽利用率，单位：%
};

/**************************************************************************************************/

//网络端口状态信息
struct PORT_INFO
{
	struct DEV_INFO dev_info;
	int port_type;//1，该端口连接本域路由器；2,该端口连接其他域边界路由器；3,该端口连接本域主机
	int link_detect;//是否检测到链路，是则为1
	struct STATS_INFO stats_info;	
	struct EFFICIENCY efficiency;
	struct DEST_INFO *dest_info;
};

//设备主信息
struct HOST_INFO
{
	char host_name[32];//本机标识名
	struct utsname u_name;//本机系统信息
	int port_count;//本机网络端口数量
};

//trap state of each port connecting to the network(for list)
struct PORT_STATE_INDEX
{
	char dev_name[16];
	int trap_level;//trap level:0 for no trap;1 for normal trap;2 for connection trap
	uint32_t trap_state;//trap state:according to trap_type in trap_message
	struct PORT_STATE_INDEX *next;
};

struct PORT_INDEX_INFO
{
	char dev_name[16];
	int trap_level;//trap level:0 for no trap;1 for normal trap;2 for connection trap
	uint32_t trap_state;//trap state:according to trap_type in trap_message
};

struct PORT_INDEX
{
//	sem_t mutex;
	pthread_rwlock_t lock;
	char host_name[32];
	char dir_name[100];
	int port_count;
	struct PORT_INDEX_INFO port[64];
};

/**************************************************************************************************/

//端口定时器信息
struct TIMER_MESSAGE
{
	char ip[16];
	int flag;
	struct tm t;
};

struct _timer_info
{
    int timer_id; /* 定时器编号 */
    int flag;/* 定时器类型判定，启动子进程为0,终止为1 */
    int elapse; /* 相对前一个子定时器的剩余时间 */
    time_t time;/* 自系统时间来已经经过的时间(1970年00:00:00) */
    struct tm t;/* 绝对时间 */
    struct _timer_info* (* timer_proc) (struct _timer_info* timer_info, int* num);/* 启动时间函数入口 */
    struct _timer_info* pre;
    struct _timer_info* next;
    /*
    int (* timer_proc) (void *arg, int arg_len);
    char func_arg[MAX_FUNC_ARG_LEN];
    int arg_len;
    */
};

struct _timer_manage
{
    void (* old_sigfunc)(int);
    void (* new_sigfunc)(int);
    struct itimerval value, ovalue;
    struct _timer_info* timer_info;
};

struct _TM
{
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
};

struct INFORM_GET_HEAD
{
	char host_name[32];
	int count;
};

struct INFORM_GET_MIDDLE
{
	char ip[16];
	/*总包数（1<<0），进包数（1<<1），出包数（1<<2），总流量(1<<3），
	进流量（1<<4），出流量（1<<5），时延(1<<6)，丢包率（1<<7）， 带宽利用率（1<<8）*/
	uint32_t request;
};

struct INFORM_GET_TAIL
{
	//某个时间点（0）,某段时间的均值（1）,某段时间的峰值（2）,某段时间的谷值（3）
	int content_type;
	struct _TM start;
	struct _TM end;
};

union _VALUE
{
	long long l;
	double f;
};

#endif
