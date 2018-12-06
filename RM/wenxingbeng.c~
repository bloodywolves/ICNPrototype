#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>

#include <rte_common.h>
#include <rte_common_vect.h>
#include <rte_byteorder.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_tailq.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_string_fns.h>


#include "wenxingbeng.h"
#include <rte_hash.h>


/** Create PID */
#define PID(a,b,c,d) ((uint32_t)(((a) & 0xff) << 24) | \
			(((b) & 0xff) << 16) | \
			(((c) & 0xff) << 8)  | \
			((d) & 0xff))


/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/

struct ipv4 {
        uint32_t ip_dst;
} ;

union ipv4_host {
	struct {
		uint32_t ip_dst;
		uint32_t pad0;
		uint32_t pad1;
		uint32_t pad2;
	};
	__m128i xmm;
};

struct ipv4_route {
	struct ipv4 key;
	uint8_t if_out;
};


typedef struct NID{
    uint8_t nid[16]; 
}nid_t;

struct sid{
        uint8_t nid_sid[16];
        uint8_t l_sid[20];      
};
union sid_host{
        struct {
               uint8_t nid_sid[16];
               uint8_t l_sid[20]; 
               uint8_t pad[12];
        };
        __m128i xmm[3];
};
struct sid_route {
       struct sid key;
       nid_t nid_out;
       uint8_t flag_out;
       uint32_t pid_out;
       uint32_t ipv4_out;
 
};
//--------------------------------The information about this computer--------------------------
static struct ipv4_route ipv4_route_array[] = 
{
	{{IPv4(192,168,8,2)}, 1},
	{{IPv4(172,16,17,124)}, 1},
	{{IPv4(172,16,17,0)}, 88},
	{{IPv4(211,0,0,0)}, 3},
};

		
static struct sid_route sid_route_array[] = 
{
	{{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20}},
	    { {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} },
	    0x1,
	    PID('P','I','D','2'),
	    IPv4(192,168,8,2)},
	{{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}}, 
	    { {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} },
	    0x1,
	    0x1111000B,
	    IPv4(172,16,17,1)},
	{{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2}}, 
	    { {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} },
	    0x1,
	    0x1111000C,
	    IPv4(172,16,17,2)},
	{{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3}}, 
	    { {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} },
	    0x1,
	    0x1111000D, 
	    IPv4(172,16,17,3)},
};
//-----------------------------------------------------------------------------------------

/**********************************************************************************************/
/**************************************By  WenXingBeng*****************************************/
/*******************************************************************************End************/

struct nid_ip_nid_ip_nid
{
       nid_t eid1;
       uint32_t ip1;
       uint32_t pid;
       uint32_t ip2;
       nid_t eid2;
};


//mark
//****************************************************************************************
//****************************************************************************************
//****************************************************************************************

static uint64_t PID_change_frequency=10;
#define pid_change_period 1   //  In real,pid_change_period=1/PID_change_frequency;!!
#define HSOT_IP IPv4(192,168,4,1)
#define NID {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1}
uint8_t HOST_NID[16]=NID;
static  nid_t host_nid={ NID };
static struct nid_ip_nid_ip_nid nid_ip_nid_ip_nid_array[] =
{
        {
                 { {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1} },
                 IPv4(192,168,1,3),
                 PID('P','I','D','2'),  //PID(uint32_t)
                 IPv4(192,168,3,2),
                 { {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1} },
        },
        
};
//****************************************************************************************
//****************************************************************************************
//****************************************************************************************


// ret<0/eid1<eid2   ret=0/eid1=eid2  ret>0/eid1>eid2
static inline int compare_nid(nid_t eid1,nid_t eid2)
{
        int i=0,ret=0;
        for(i=0;i<16;i++)
        {
                if(eid1.nid[i]<eid2.nid[i])
                {
                        return -1;      
                }
                else if(eid1.nid[i]>eid2.nid[i])
                {
                        return 1;       //eid1>eid2
                }
        }
        return ret;     //eid1=eid2
}




static inline int aligment_configure()
{
        int number=sizeof(nid_ip_nid_ip_nid_array)/sizeof(struct nid_ip_nid_ip_nid),j,i;
        struct nid_ip_nid_ip_nid * item=NULL;
        uint32_t ip_temp;
        for(i=0;i<number;i++)
        {
                item=&nid_ip_nid_ip_nid_array[i];
                if(compare_nid(host_nid,item->eid1)==0||compare_nid(host_nid,item->eid2)==0)
                {
                        if(compare_nid(host_nid,item->eid1)!=0)
                        {
                                ip_temp=item->ip1;
                                item->ip1=item->ip2;
                                item->ip2=ip_temp;
                                for(j=0;j<16;j++)
                                {

                                       item->eid2.nid[j]=item->eid1.nid[j]; 
                                       item->eid1.nid[j]=HOST_NID[j];
                                }
                        }
                }
        }
}

static inline void print_nid_ip_nid_ip_nid()
{
        uint8_t number=sizeof(nid_ip_nid_ip_nid_array)/sizeof(struct nid_ip_nid_ip_nid);
        uint8_t i=0,j=0;
        printf("[From %s]number=%d\n",__func__,number);
        struct nid_ip_nid_ip_nid * item=NULL;
        for(i=0;i<number;i++)
        {
                item=&nid_ip_nid_ip_nid_array[i];
                printf("  #%d#  ",i+1);
                for(j=0;j<16;j++)
                {
                        if(j<15)printf("%d:",item->eid1.nid[j]);
                        else printf("%d  ",item->eid1.nid[j]);
                }
                
                uint32_t ip_temp=rte_cpu_to_be_32(item->ip1);
                uint8_t * wxb_ip=(uint8_t *)(&ip_temp);
	        printf("[%d.",*(wxb_ip++) );
                printf("%d.",*(wxb_ip++) );
                printf("%d.",*(wxb_ip++) );
                printf("%d]",*(wxb_ip++) );
                
                printf("-- %d --",item->pid);
                
                ip_temp=rte_cpu_to_be_32(item->ip2);
                wxb_ip=(uint8_t *)(&ip_temp);
	        printf("[%d.",*(wxb_ip++) );
                printf("%d.",*(wxb_ip++) );
                printf("%d.",*(wxb_ip++) );
                printf("%d] ",*(wxb_ip++) );
                
                
                for(j=0;j<16;j++)
                {
                        if(j<15)printf("%d:",item->eid2.nid[j]);
                        else printf("%d  ",item->eid2.nid[j]);
                }
                
                if(compare_nid(host_nid,item->eid1)==0||compare_nid(host_nid,item->eid2)==0)
                {
                        printf("***USED in this RM!***\n");
                }
                else
                {
                        printf("\n");
                }
        }
}




/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/

#define NB_SOCKETS_wxb 8

static struct rte_hash *sid_lookup_struct_wxb[NB_SOCKETS_wxb];
static struct rte_hash *ipv4_lookup_struct_wxb[NB_SOCKETS_wxb];


/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************End************/

/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/


#ifdef RTE_MACHINE_CPUFLAG_SSE4_2
#include <rte_hash_crc.h>
#define DEFAULT_HASH_FUNC       rte_hash_crc
#else
#include <rte_jhash.h>
#define DEFAULT_HASH_FUNC       rte_jhash
#endif

static inline uint32_t
ipv4_wxb_hash_crc(const void *data, __rte_unused uint32_t data_len,uint32_t init_val)
{
	const union ipv4_host *k;
	k=data;
	const uint8_t * wxb_ip=data;
	printf("[From %s]ip_dst=%d.",__func__,*(wxb_ip++));
	printf("%d.",*(wxb_ip++));
	printf("%d.",*(wxb_ip++));
	printf("%d\n",*(wxb_ip++));
	
#ifdef RTE_MACHINE_CPUFLAG_SSE4_2
	init_val = rte_hash_crc_4byte(k->ip_dst, init_val);
	init_val = rte_hash_crc_4byte(k->pad0, init_val);
	init_val = rte_hash_crc_4byte(k->pad1, init_val);
	init_val = rte_hash_crc_4byte(k->pad2, init_val);
#else /* RTE_MACHINE_CPUFLAG_SSE4_2 */
	init_val = rte_jhash_1word(k->ip_dst, init_val);
	init_val = rte_jhash_1word(k->pad0, init_val);
	init_val = rte_jhash_1word(k->pad1, init_val);
	init_val = rte_jhash_1word(k->pad2, init_val);	
#endif /* RTE_MACHINE_CPUFLAG_SSE4_2 */
	return (init_val);
}

static inline uint32_t
sid_hash_crc(const void *data, __rte_unused uint32_t data_len,uint32_t init_val)
{
	int cycle=0;
	int i=0;
	const uint32_t * temp=(const uint32_t *)data;
	cycle=(sizeof(union sid_host)/4);
#ifdef RTE_MACHINE_CPUFLAG_SSE4_2
        for(i=0;i<cycle;i++)
        {
              init_val = rte_hash_crc_4byte(*temp, init_val);
              temp++;
        }
#else /* RTE_MACHINE_CPUFLAG_SSE4_2 */
	for(i=0;i<cycle;i++)
        {
              init_val = rte_jhash_1word(*temp, init_val);
              temp++;
        }
#endif /* RTE_MACHINE_CPUFLAG_SSE4_2 */
	return (init_val);
}

/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************End************/

/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/

#ifdef RTE_ARCH_X86_64
/* default to 4 million hash entries (approx) */
#define L3FWD_HASH_ENTRIES		1024*1024*4
#else
/* 32-bit has less address-space for hugepage memory, limit to 1M entries */
#define L3FWD_HASH_ENTRIES		1024*1024*1
#endif
#define HASH_ENTRY_NUMBER_DEFAULT	4

static uint8_t ipv4_out_if[L3FWD_HASH_ENTRIES] __rte_cache_aligned;
static uint32_t sid_out_ipv4[L3FWD_HASH_ENTRIES] __rte_cache_aligned;
static uint32_t sid_out_pid[L3FWD_HASH_ENTRIES] __rte_cache_aligned;
static uint8_t sid_out_flag[L3FWD_HASH_ENTRIES] __rte_cache_aligned;
static nid_t sid_out_nid[L3FWD_HASH_ENTRIES] __rte_cache_aligned;

static __m128i mask02;

static inline uint8_t
get_ipv4_dst_port_wxb(void *ipv4_hdr, uint8_t portid, struct rte_hash  * ipv4_lookup_struct_wxb)
{
	int ret = 0;
	union ipv4_host key;
	
	ipv4_hdr = (uint8_t *)ipv4_hdr + offsetof(struct ipv4_hdr, dst_addr);
	__m128i data = _mm_loadu_si128((__m128i*)(ipv4_hdr));
	/* Get 5 tuple: dst port, src port, dst IP address, src IP address and protocol */
	key.xmm = _mm_and_si128(data, mask02);
	/* Find destination port */
	ret = rte_hash_lookup(ipv4_lookup_struct_wxb, (const void *)&key);
	return (uint8_t)((ret < 0)? portid : ipv4_out_if[ret]);
}

static inline uint32_t
get_sid_dst_ipv4(void *ipv4_hdr, struct rte_hash  * sid_lookup_struct_wxb)
{
	int ret = 0;
	int i=0;
	uint32_t input_ipv4=0;
	struct ipv4_hdr * temp_ipv4_hdr=(struct ipv4_hdr *)ipv4_hdr;

	union sid_host key;
	
	CoLoR_get_t * get_hdr=( CoLoR_get_t* )((uint8_t *)ipv4_hdr+sizeof(struct ipv4_hdr));
	
	input_ipv4=temp_ipv4_hdr->src_addr;
		
	for(i=0; i<16 ;i++)
	{
	key.nid_sid[i]=get_hdr->nid_sid[i];
	}
	for(i=0; i<20 ;i++)
	{
	key.l_sid[i]=get_hdr->l_sid[i];
	}
	for(i=0; i<12 ;i++)
	{
	key.pad[i]=0;
	}
	
	printf("[From %s]key.nid_sid=",__func__);
        for(i=0 ;i<16 ;i++)
        {
        printf("%2X",key.nid_sid[i]);
        }
        printf("\n");
                
        printf("[From %s]key.l_sid=",__func__);
        for(i=0 ;i<20 ;i++)
        {
            printf("%2X",key.l_sid[i]);
        }
        printf("\n");
	

	ret = rte_hash_lookup(sid_lookup_struct_wxb, (const void *)&key);
	return (uint32_t)((ret < 0)? input_ipv4 : sid_out_ipv4[ret]);
}

static inline uint32_t
get_sid_dst_pid(void *ipv4_hdr, struct rte_hash  * sid_lookup_struct_wxb)
{
	int ret = 0;
	int i=0;
	uint32_t defualt_pid=0;

	union sid_host key;
	
	CoLoR_get_t * get_hdr=( CoLoR_get_t* )((uint8_t *)ipv4_hdr+sizeof(struct ipv4_hdr));
	
		
	for(i=0; i<16 ;i++)
	{
	key.nid_sid[i]=get_hdr->nid_sid[i];
	}
	for(i=0; i<20 ;i++)
	{
	key.l_sid[i]=get_hdr->l_sid[i];
	}
	for(i=0; i<12 ;i++)
	{
	key.pad[i]=0;
	}

	ret = rte_hash_lookup(sid_lookup_struct_wxb, (const void *)&key);
	return (uint32_t)((ret < 0)? defualt_pid : sid_out_pid[ret]);
}



static inline void
update_IP(void *ipv4_hdr, uint32_t dst_addr)
{
        struct ipv4_hdr * temp_ipv4_hdr=(struct ipv4_hdr *)ipv4_hdr;
        temp_ipv4_hdr->src_addr=rte_cpu_to_be_32( HSOT_IP );
        temp_ipv4_hdr->dst_addr=dst_addr;
	return;

}

static inline void
update_PID(struct rte_mbuf *m, uint32_t pid)
{
        m->pkt.data_len+=sizeof(pid);
        m->pkt.pkt_len+=sizeof(pid);

        CoLoR_get_t * get_hdr=(CoLoR_get_t *)(rte_pktmbuf_mtod(m, unsigned char *) +sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr));
        uint8_t PIDs= get_hdr->PIDs;
        printf("[From %s]PIDs=%d\n",__func__,PIDs);
        struct ipv4_hdr *ipv4_hdr=(struct ipv4_hdr *)(rte_pktmbuf_mtod(m, unsigned char *) +sizeof(struct ether_hdr));
        printf("[From %s]ipv4_hdr->total_length=%d\n",__func__,rte_cpu_to_be_16(ipv4_hdr->total_length));
        ipv4_hdr->total_length=rte_cpu_to_be_16(  rte_cpu_to_be_16(ipv4_hdr->total_length)+sizeof(pid)   );
        uint32_t * pid_add=(uint32_t *)(rte_pktmbuf_mtod(m, unsigned char *) +sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr)+sizeof(CoLoR_get_t)+sizeof(pid)*PIDs);
        *(pid_add)=pid;
	get_hdr->PIDs++;
	get_hdr->total_len=rte_cpu_to_be_16( rte_cpu_to_be_16( get_hdr->total_len)+sizeof(pid) );
        return;
}

static inline uint8_t
get_sid_dst_flag(void *ipv4_hdr, struct rte_hash  * sid_lookup_struct_wxb)
{
	int ret = 0;
	int i=0;
	uint8_t defualt_flag=0;

	union sid_host key;
	
	CoLoR_get_t * get_hdr=( CoLoR_get_t* )((uint8_t *)ipv4_hdr+sizeof(struct ipv4_hdr));
	
		
	for(i=0; i<16 ;i++)
	{
	key.nid_sid[i]=get_hdr->nid_sid[i];
	}
	for(i=0; i<20 ;i++)
	{
	key.l_sid[i]=get_hdr->l_sid[i];
	}
	for(i=0; i<12 ;i++)
	{
	key.pad[i]=0;
	}

	ret = rte_hash_lookup(sid_lookup_struct_wxb, (const void *)&key);
	return (uint32_t)((ret < 0)? defualt_flag : sid_out_flag[ret]);
}

static inline nid_t
get_sid_dst_nid(void *ipv4_hdr, struct rte_hash  * sid_lookup_struct_wxb)
{
	int ret = 0;
	int i=0;
	nid_t defualt_nid={{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

	union sid_host key;
	
	CoLoR_get_t * get_hdr=( CoLoR_get_t* )((uint8_t *)ipv4_hdr+sizeof(struct ipv4_hdr));
	
		
	for(i=0; i<16 ;i++)
	{
	key.nid_sid[i]=get_hdr->nid_sid[i];
	}
	for(i=0; i<20 ;i++)
	{
	key.l_sid[i]=get_hdr->l_sid[i];
	}
	for(i=0; i<12 ;i++)
	{
	key.pad[i]=0;
	}

	ret = rte_hash_lookup(sid_lookup_struct_wxb, (const void *)&key);
	return (nid_t)((ret < 0)? defualt_nid : sid_out_nid[ret]);
}

/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************End************/

/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/
static void convert_ipv4(struct ipv4* key1,union ipv4_host* key2)
{
	key2->ip_dst = rte_cpu_to_be_32(key1->ip_dst);
	key2->pad0 = 0;
	key2->pad1 = 0;
	key2->pad2 = 0;
	return;
}

static void convert_sid(struct sid * key1,union sid_host* key2)
{
        int i=0;
        for(i=0;i<16;i++)
        {
            key2->nid_sid[i]=key1->nid_sid[i];
        }
        for(i=0;i<20;i++)
        {
            key2->l_sid[i]=key1->l_sid[i];
        }
        for(i=0;i<12;i++)
        {
            key2->pad[i]=0;
        }
	return;
}
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************End************/

/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/

static inline void
populate_ipv4_few_flow_into_table_wxb(const struct rte_hash* h)
{
	uint32_t i;
	int32_t ret;
	uint32_t array_len = sizeof(ipv4_route_array)/sizeof(ipv4_route_array[0]); 

	mask02 = _mm_set_epi32(0 ,0, 0, 0XFFFFFFFF);
	for (i = 0; i < array_len; i++) 
	{
		struct ipv4_route  entry;
		union ipv4_host newkey;
		entry = ipv4_route_array[i];
		convert_ipv4(&entry.key, &newkey);
		ret = rte_hash_add_key (h,(void *) &newkey);
		if (ret < 0) 
		{
			rte_exit(EXIT_FAILURE, "Unable to add entry %u to the"
                                "l3fwd hash.\n", i);
		}
		ipv4_out_if[ret] = entry.if_out;
	}
	printf("Hash: Adding 0x%x keys\n", array_len);
}


static inline void
populate_sid_few_flow_into_table(const struct rte_hash* h)
{
	uint32_t i;
	int32_t ret;
	uint32_t array_len = sizeof(sid_route_array)/sizeof(sid_route_array[0]); 

	for (i = 0; i < array_len; i++) 
	{
		struct sid_route  entry;
		union sid_host newkey;
		entry = sid_route_array[i];
		convert_sid(&entry.key, &newkey);
		
		ret = rte_hash_add_key (h,(void *) &newkey);
		if (ret < 0) 
		{
			rte_exit(EXIT_FAILURE, "Unable to add entry %u to the"
                                "l3fwd hash.\n", i);
		}
		sid_out_ipv4[ret] = rte_cpu_to_be_32(entry.ipv4_out);//!!!!!!!!!!!!!!rte_cpu_to_be_32
		sid_out_pid[ret] = entry.pid_out;
		sid_out_flag[ret] = entry.flag_out;
		sid_out_nid[ret] = entry.nid_out;
	}
	printf("[From populate_sid_few_flow_into_table]Hash: Adding 0x%x keys\n", array_len);
}

/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************End************/


static inline void 
setup_hash_wxb(int socketid)
{
    
    	char s[64];
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/

    struct rte_hash_parameters ipv4_hash_params = {
        .name = NULL,
        .entries = L3FWD_HASH_ENTRIES,
        .bucket_entries = 4,
        .key_len = sizeof(union ipv4_host),
        .hash_func = ipv4_wxb_hash_crc,  
        .hash_func_init_val = 0,
    };
    
	// create ipv4 hash 
	snprintf(s, sizeof(s), "ipv4_hash_%d", socketid);
	ipv4_hash_params.name = s;
	ipv4_hash_params.socket_id = socketid;
	ipv4_lookup_struct_wxb[socketid] = rte_hash_create(&ipv4_hash_params);
	if (ipv4_lookup_struct_wxb[socketid] == NULL)
		rte_exit(EXIT_FAILURE, "Unable to create the ipv4_hash hash on "
				"socket %d\n", socketid);	
	populate_ipv4_few_flow_into_table_wxb(ipv4_lookup_struct_wxb[socketid]);
				
        // create sid hash 

    struct rte_hash_parameters sid_hash_params = {
        .name = NULL,
        .entries = L3FWD_HASH_ENTRIES,
        .bucket_entries = 4,
        .key_len = sizeof(union sid_host),
        .hash_func = sid_hash_crc,
        .hash_func_init_val = 0,
    };

	snprintf(s, sizeof(s), "sid_hash_%d", socketid);
	sid_hash_params.name = s;
	sid_hash_params.socket_id = socketid;
	sid_lookup_struct_wxb[socketid] = rte_hash_create(&sid_hash_params);
	if (sid_lookup_struct_wxb[socketid] == NULL)
		rte_exit(EXIT_FAILURE, "Unable to create the sid_hash hash on "
				"socket %d\n", socketid);
	populate_sid_few_flow_into_table(sid_lookup_struct_wxb[socketid]);		

/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************End************/
}

static inline void
print_ip_address_wxb(struct ipv4_hdr *ipv4_hdr)
{
        uint8_t * wxb_ip=(uint8_t *)(& ipv4_hdr->src_addr);
	printf("Source IP=%d.",*(wxb_ip++) );
        printf("%d.",*(wxb_ip++) );
        printf("%d.",*(wxb_ip++) );
        printf("%d -->>",*(wxb_ip++) );

        wxb_ip=(uint8_t *)(& ipv4_hdr->dst_addr);
	printf("Destination IP=%d.",*(wxb_ip++) );
        printf("%d.",*(wxb_ip++) );
        printf("%d.",*(wxb_ip++) );
        printf("%d \n",*(wxb_ip++) );
}

