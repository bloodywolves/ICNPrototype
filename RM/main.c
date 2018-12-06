/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>

#include <rte_timer.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
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

#include "main.h"

#include "trap.h"
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/
#include "wenxingbeng.c"
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************End************/

#define APP_LOOKUP_EXACT_MATCH          0
#define APP_LOOKUP_LPM                  1
#define DO_RFC_1812_CHECKS

#ifndef APP_LOOKUP_METHOD
#define APP_LOOKUP_METHOD             APP_LOOKUP_EXACT_MATCH
#endif

/*
 *  When set to zero, simple forwaring path is eanbled.
 *  When set to one, optimized forwarding path is enabled.
 *  Note that LPM optimisation path uses SSE4.1 instructions.
 */

#if ((APP_LOOKUP_METHOD == APP_LOOKUP_LPM) && !defined(__SSE4_1__))
#define ENABLE_MULTI_BUFFER_OPTIMIZE	0
#else
#define ENABLE_MULTI_BUFFER_OPTIMIZE	1
#endif

#if (APP_LOOKUP_METHOD == APP_LOOKUP_EXACT_MATCH)
#include <rte_hash.h>
#elif (APP_LOOKUP_METHOD == APP_LOOKUP_LPM)
#include <rte_lpm.h>
#include <rte_lpm6.h>
#else
#error "APP_LOOKUP_METHOD set to incorrect value"
#endif

#ifndef IPv6_BYTES
#define IPv6_BYTES_FMT "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"\
                       "%02x%02x:%02x%02x:%02x%02x:%02x%02x"
#define IPv6_BYTES(addr) \
	addr[0],  addr[1], addr[2],  addr[3], \
	addr[4],  addr[5], addr[6],  addr[7], \
	addr[8],  addr[9], addr[10], addr[11],\
	addr[12], addr[13],addr[14], addr[15]
#endif


#define RTE_LOGTYPE_L3FWD RTE_LOGTYPE_USER1

#define MAX_JUMBO_PKT_LEN  9600

#define IPV6_ADDR_LEN 16

#define MEMPOOL_CACHE_SIZE 256

#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

/*
 * This expression is used to calculate the number of mbufs needed depending on user input, taking
 *  into account memory for rx and tx hardware rings, cache per lcore and mtable per port per lcore.
 *  RTE_MAX is used to ensure that NB_MBUF never goes below a minimum value of 8192
 */

#define NB_MBUF RTE_MAX	(																	\
				(nb_ports*nb_rx_queue*RTE_TEST_RX_DESC_DEFAULT +							\
				nb_ports*nb_lcores*MAX_PKT_BURST +											\
				nb_ports*n_tx_queue*RTE_TEST_TX_DESC_DEFAULT +								\
				nb_lcores*MEMPOOL_CACHE_SIZE),												\
				(unsigned)8192)

/*
 * RX and TX Prefetch, Host, and Write-back threshold values should be
 * carefully set for optimal performance. Consult the network
 * controller's datasheet and supporting DPDK documentation for guidance
 * on how these parameters should be set.
 */
#define RX_PTHRESH 8 /**< Default values of RX prefetch threshold reg. */
#define RX_HTHRESH 8 /**< Default values of RX host threshold reg. */
#define RX_WTHRESH 4 /**< Default values of RX write-back threshold reg. */

/*
 * These default values are optimized for use with the Intel(R) 82599 10 GbE
 * Controller and the DPDK ixgbe PMD. Consider using other values for other
 * network controllers and/or network drivers.
 */
#define TX_PTHRESH 36 /**< Default values of TX prefetch threshold reg. */
#define TX_HTHRESH 0  /**< Default values of TX host threshold reg. */
#define TX_WTHRESH 0  /**< Default values of TX write-back threshold reg. */

#define MAX_PKT_BURST     32
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */

/*
 * Try to avoid TX buffering if we have at least MAX_TX_BURST packets to send.
 */
#define	MAX_TX_BURST	(MAX_PKT_BURST / 2)

#define NB_SOCKETS 8

/* Configure how many packets ahead to prefetch, when reading packets */
#define PREFETCH_OFFSET	3

/* Used to mark destination port as 'invalid'. */
#define	BAD_PORT	((uint16_t)-1)

#define FWDSTEP	4

/*
 * Configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 128
#define RTE_TEST_TX_DESC_DEFAULT 512
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

/* ethernet addresses of ports */
static struct ether_addr ports_eth_addr[RTE_MAX_ETHPORTS];

static __m128i val_eth[RTE_MAX_ETHPORTS];

/* replace first 12B of the ethernet header. */
#define	MASK_ETH	0x3f

/* mask of enabled ports */
static uint32_t enabled_port_mask = 0;
static int promiscuous_on = 1; /**< Ports set in promiscuous mode off by default. */
static int numa_on = 1; /**< NUMA is enabled by default. */

#if (APP_LOOKUP_METHOD == APP_LOOKUP_EXACT_MATCH)
static int ipv6 = 0; /**< ipv6 is false by default. */
#endif

struct mbuf_table {
	uint16_t len;
	struct rte_mbuf *m_table[MAX_PKT_BURST];
};

struct lcore_rx_queue {
	uint8_t port_id;
	uint8_t queue_id;
} __rte_cache_aligned;

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT RTE_MAX_ETHPORTS
#define MAX_RX_QUEUE_PER_PORT 128

#define MAX_LCORE_PARAMS 1024
struct lcore_params {
	uint8_t port_id;
	uint8_t queue_id;
	uint8_t lcore_id;
} __rte_cache_aligned;

static struct lcore_params lcore_params_array[MAX_LCORE_PARAMS];
static struct lcore_params lcore_params_array_default[] = {
	{0, 0, 2},
	{0, 1, 2},
	{0, 2, 2},
	{1, 0, 2},
	{1, 1, 2},
	{1, 2, 2},
	{2, 0, 2},
	{3, 0, 3},
	{3, 1, 3},
};

static struct lcore_params * lcore_params = lcore_params_array_default;
static uint16_t nb_lcore_params = sizeof(lcore_params_array_default) /
				sizeof(lcore_params_array_default[0]);

static struct rte_eth_conf port_conf = {
	.rxmode = {
		.mq_mode = ETH_MQ_RX_RSS,
		.max_rx_pkt_len = ETHER_MAX_LEN,
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 1, /**< IP checksum offload enabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 0, /**< CRC stripped by hardware */
	},
	.rx_adv_conf = {
		.rss_conf = {
			.rss_key = NULL,
			.rss_hf = ETH_RSS_IP,
		},
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

static const struct rte_eth_rxconf rx_conf = {
	.rx_thresh = {
		.pthresh = RX_PTHRESH,
		.hthresh = RX_HTHRESH,
		.wthresh = RX_WTHRESH,
	},
	.rx_free_thresh = 32,
};

static struct rte_eth_txconf tx_conf = {
	.tx_thresh = {
		.pthresh = TX_PTHRESH,
		.hthresh = TX_HTHRESH,
		.wthresh = TX_WTHRESH,
	},
	.tx_free_thresh = 0, /* Use PMD default values */
	.tx_rs_thresh = 0, /* Use PMD default values */
	.txq_flags = (ETH_TXQ_FLAGS_NOMULTSEGS |
			ETH_TXQ_FLAGS_NOVLANOFFL |
			ETH_TXQ_FLAGS_NOXSUMSCTP |
			ETH_TXQ_FLAGS_NOXSUMUDP |
			ETH_TXQ_FLAGS_NOXSUMTCP)

};

static struct rte_mempool * pktmbuf_pool[NB_SOCKETS];

#if (APP_LOOKUP_METHOD == APP_LOOKUP_EXACT_MATCH)

#ifdef RTE_MACHINE_CPUFLAG_SSE4_2
#include <rte_hash_crc.h>
#define DEFAULT_HASH_FUNC       rte_hash_crc
#else
#include <rte_jhash.h>
#define DEFAULT_HASH_FUNC       rte_jhash
#endif

struct ipv4_5tuple {
        uint32_t ip_dst;
        uint32_t ip_src;
        uint16_t port_dst;
        uint16_t port_src;
        uint8_t  proto;
} __attribute__((__packed__));

union ipv4_5tuple_host {
	struct {
		uint8_t  pad0;
		uint8_t  proto;
		uint16_t pad1;
		uint32_t ip_src;
		uint32_t ip_dst;
		uint16_t port_src;
		uint16_t port_dst;
	};
	__m128i xmm;
};


#define XMM_NUM_IN_IPV6_5TUPLE 3

struct ipv6_5tuple {
        uint8_t  ip_dst[IPV6_ADDR_LEN];
        uint8_t  ip_src[IPV6_ADDR_LEN];
        uint16_t port_dst;
        uint16_t port_src;
        uint8_t  proto;
} __attribute__((__packed__));

union ipv6_5tuple_host {
	struct {
		uint16_t pad0;
		uint8_t  proto;
		uint8_t  pad1;
		uint8_t  ip_src[IPV6_ADDR_LEN];
		uint8_t  ip_dst[IPV6_ADDR_LEN];
		uint16_t port_src;
		uint16_t port_dst;
		uint64_t reserve;
	};
	__m128i xmm[XMM_NUM_IN_IPV6_5TUPLE];
};

struct ipv4_l3fwd_route {
	struct ipv4_5tuple key;
	uint8_t if_out;
};


struct ipv6_l3fwd_route {
	struct ipv6_5tuple key;
	uint8_t if_out;
};


/*
static struct ipv4_l3fwd_route ipv4_l3fwd_route_array[] = {
	{{IPv4(10,10,1,1), IPv4(10,10,0,1),     1,  0, IPPROTO_UDP}, 1},
	{{IPv4(10,10,0,1), IPv4(10,10,1,1),     0,  1, IPPROTO_UDP}, 0},
	{{IPv4(111,0,0,0), IPv4(100,30,0,1),  101, 11, IPPROTO_TCP}, 2},
	{{IPv4(211,0,0,0), IPv4(200,40,0,1),  102, 12, IPPROTO_TCP}, 3},
};
*/

static struct ipv6_l3fwd_route ipv6_l3fwd_route_array[] = {
	{{
	{0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x02, 0x1e, 0x67, 0xff, 0xfe, 0, 0, 0},
	{0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x02, 0x1b, 0x21, 0xff, 0xfe, 0x91, 0x38, 0x05},
	101, 11, IPPROTO_TCP}, 0},

	{{
	{0xfe, 0x90, 0, 0, 0, 0, 0, 0, 0x02, 0x1e, 0x67, 0xff, 0xfe, 0, 0, 0},
	{0xfe, 0x90, 0, 0, 0, 0, 0, 0, 0x02, 0x1b, 0x21, 0xff, 0xfe, 0x91, 0x38, 0x05},
	102, 12, IPPROTO_TCP}, 1},

	{{
	{0xfe, 0xa0, 0, 0, 0, 0, 0, 0, 0x02, 0x1e, 0x67, 0xff, 0xfe, 0, 0, 0},
	{0xfe, 0xa0, 0, 0, 0, 0, 0, 0, 0x02, 0x1b, 0x21, 0xff, 0xfe, 0x91, 0x38, 0x05},
	101, 11, IPPROTO_TCP}, 2},

	{{
	{0xfe, 0xb0, 0, 0, 0, 0, 0, 0, 0x02, 0x1e, 0x67, 0xff, 0xfe, 0, 0, 0},
	{0xfe, 0xb0, 0, 0, 0, 0, 0, 0, 0x02, 0x1b, 0x21, 0xff, 0xfe, 0x91, 0x38, 0x05},
	102, 12, IPPROTO_TCP}, 3},
};

typedef struct rte_hash lookup_struct_t;
static lookup_struct_t *ipv6_l3fwd_lookup_struct[NB_SOCKETS];

#ifdef RTE_ARCH_X86_64
/* default to 4 million hash entries (approx) */
#define L3FWD_HASH_ENTRIES		1024*1024*4
#else
/* 32-bit has less address-space for hugepage memory, limit to 1M entries */
#define L3FWD_HASH_ENTRIES		1024*1024*1
#endif
#define HASH_ENTRY_NUMBER_DEFAULT	4

static uint32_t hash_entry_number = HASH_ENTRY_NUMBER_DEFAULT;


static inline uint32_t
ipv6_hash_crc(const void *data, __rte_unused uint32_t data_len, uint32_t init_val)
{
	const union ipv6_5tuple_host *k;
	uint32_t t;
	const uint32_t *p;
#ifdef RTE_MACHINE_CPUFLAG_SSE4_2
	const uint32_t  *ip_src0, *ip_src1, *ip_src2, *ip_src3;
	const uint32_t  *ip_dst0, *ip_dst1, *ip_dst2, *ip_dst3;
#endif /* RTE_MACHINE_CPUFLAG_SSE4_2 */

	k = data;
	t = k->proto;
	p = (const uint32_t *)&k->port_src;

#ifdef RTE_MACHINE_CPUFLAG_SSE4_2
	ip_src0 = (const uint32_t *) k->ip_src;
	ip_src1 = (const uint32_t *)(k->ip_src+4);
	ip_src2 = (const uint32_t *)(k->ip_src+8);
	ip_src3 = (const uint32_t *)(k->ip_src+12);
	ip_dst0 = (const uint32_t *) k->ip_dst;
	ip_dst1 = (const uint32_t *)(k->ip_dst+4);
	ip_dst2 = (const uint32_t *)(k->ip_dst+8);
	ip_dst3 = (const uint32_t *)(k->ip_dst+12);
	init_val = rte_hash_crc_4byte(t, init_val);
	init_val = rte_hash_crc_4byte(*ip_src0, init_val);
	init_val = rte_hash_crc_4byte(*ip_src1, init_val);
	init_val = rte_hash_crc_4byte(*ip_src2, init_val);
	init_val = rte_hash_crc_4byte(*ip_src3, init_val);
	init_val = rte_hash_crc_4byte(*ip_dst0, init_val);
	init_val = rte_hash_crc_4byte(*ip_dst1, init_val);
	init_val = rte_hash_crc_4byte(*ip_dst2, init_val);
	init_val = rte_hash_crc_4byte(*ip_dst3, init_val);
	init_val = rte_hash_crc_4byte(*p, init_val);
#else /* RTE_MACHINE_CPUFLAG_SSE4_2 */
	init_val = rte_jhash_1word(t, init_val);
	init_val = rte_jhash(k->ip_src, sizeof(uint8_t) * IPV6_ADDR_LEN, init_val);
	init_val = rte_jhash(k->ip_dst, sizeof(uint8_t) * IPV6_ADDR_LEN, init_val);
	init_val = rte_jhash_1word(*p, init_val);
#endif /* RTE_MACHINE_CPUFLAG_SSE4_2 */
	return (init_val);
}

#define IPV4_L3FWD_NUM_ROUTES \
	(sizeof(ipv4_l3fwd_route_array) / sizeof(ipv4_l3fwd_route_array[0]))

#define IPV6_L3FWD_NUM_ROUTES \
	(sizeof(ipv6_l3fwd_route_array) / sizeof(ipv6_l3fwd_route_array[0]))

static uint8_t ipv4_l3fwd_out_if[L3FWD_HASH_ENTRIES] __rte_cache_aligned;
static uint8_t ipv6_l3fwd_out_if[L3FWD_HASH_ENTRIES] __rte_cache_aligned;

#endif

#if (APP_LOOKUP_METHOD == APP_LOOKUP_LPM)

struct ipv4_l3fwd_route {
	uint32_t ip;
	uint8_t  depth;
	uint8_t  if_out;
};

struct ipv6_l3fwd_route {
	uint8_t ip[16];
	uint8_t  depth;
	uint8_t  if_out;
};

static struct ipv4_l3fwd_route ipv4_l3fwd_route_array[] = {   //IPv4 
	{IPv4(10,10,1,1), 24, 1},
	{IPv4(10,10,0,1), 24, 0},
	{IPv4(3,1,1,0), 24, 2},
	{IPv4(4,1,1,0), 24, 3},
	{IPv4(5,1,1,0), 24, 4},
	{IPv4(6,1,1,0), 24, 5},
	{IPv4(7,1,1,0), 24, 6},
	{IPv4(8,1,1,0), 24, 7},
};

static struct ipv6_l3fwd_route ipv6_l3fwd_route_array[] = {
	{{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 0},
	{{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 1},
	{{3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 2},
	{{4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 3},
	{{5,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 4},
	{{6,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 5},
	{{7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 6},
	{{8,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 7},
};

#define IPV4_L3FWD_NUM_ROUTES \
	(sizeof(ipv4_l3fwd_route_array) / sizeof(ipv4_l3fwd_route_array[0]))
#define IPV6_L3FWD_NUM_ROUTES \
	(sizeof(ipv6_l3fwd_route_array) / sizeof(ipv6_l3fwd_route_array[0]))

#define IPV4_L3FWD_LPM_MAX_RULES         1024
#define IPV6_L3FWD_LPM_MAX_RULES         1024
#define IPV6_L3FWD_LPM_NUMBER_TBL8S (1 << 16)

typedef struct rte_lpm lookup_struct_t;
typedef struct rte_lpm6 lookup6_struct_t;
static lookup6_struct_t *ipv6_l3fwd_lookup_struct[NB_SOCKETS];
#endif

struct lcore_conf {
	uint16_t n_rx_queue;
	struct lcore_rx_queue rx_queue_list[MAX_RX_QUEUE_PER_LCORE];
	uint16_t tx_queue_id[RTE_MAX_ETHPORTS];
	struct mbuf_table tx_mbufs[RTE_MAX_ETHPORTS];
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/
	lookup_struct_t * sid_lookup_struct_wxb;
	lookup_struct_t * ipv4_lookup_struct_wxb;
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************END************/

#if (APP_LOOKUP_METHOD == APP_LOOKUP_LPM)
	lookup6_struct_t * ipv6_lookup_struct;
#else
	lookup_struct_t * ipv6_lookup_struct;
#endif
} __rte_cache_aligned;

static struct lcore_conf lcore_conf[RTE_MAX_LCORE];

/* Send burst of packets on an output interface */
static inline int
send_burst(struct lcore_conf *qconf, uint16_t n, uint8_t port)
{
	struct rte_mbuf **m_table;
	int ret;
	uint16_t queueid;

	queueid = qconf->tx_queue_id[port];
	m_table = (struct rte_mbuf **)qconf->tx_mbufs[port].m_table;

	ret = rte_eth_tx_burst(port, queueid, m_table, n);
	if (unlikely(ret < n)) {
		do {
			rte_pktmbuf_free(m_table[ret]);
		} while (++ret < n);
	}

	return 0;
}

/* Enqueue a single packet, and send burst if queue is filled */
static inline int
send_single_packet(struct rte_mbuf *m, uint8_t port)
{
	uint32_t lcore_id;
	uint16_t len;
	struct lcore_conf *qconf;

	lcore_id = rte_lcore_id();

	qconf = &lcore_conf[lcore_id];
	len = qconf->tx_mbufs[port].len;
	qconf->tx_mbufs[port].m_table[len] = m;
	len++;

	/* enough pkts to be sent */
	if (unlikely(len == MAX_PKT_BURST)) {
		send_burst(qconf, MAX_PKT_BURST, port);
		len = 0;
	}

	qconf->tx_mbufs[port].len = len;
	return 0;
}

static inline __attribute__((always_inline)) void
send_packetsx4(struct lcore_conf *qconf, uint8_t port,
	struct rte_mbuf *m[], uint32_t num)
{
	uint32_t len, j, n;

	len = qconf->tx_mbufs[port].len;

	/*
	 * If TX buffer for that queue is empty, and we have enough packets,
	 * then send them straightway.
	 */
	if (num >= MAX_TX_BURST && len == 0) {
		n = rte_eth_tx_burst(port, qconf->tx_queue_id[port], m, num);
		if (unlikely(n < num)) {
			do {
				rte_pktmbuf_free(m[n]);
			} while (++n < num);
		}
		return;
	}

	/*
	 * Put packets into TX buffer for that queue.
	 */

	n = len + num;
	n = (n > MAX_PKT_BURST) ? MAX_PKT_BURST - len : num;

	j = 0;
	switch (n % FWDSTEP) {
	while (j < n) {
	case 0:
		qconf->tx_mbufs[port].m_table[len + j] = m[j];
		j++;
	case 3:
		qconf->tx_mbufs[port].m_table[len + j] = m[j];
		j++;
	case 2:
		qconf->tx_mbufs[port].m_table[len + j] = m[j];
		j++;
	case 1:
		qconf->tx_mbufs[port].m_table[len + j] = m[j];
		j++;
	}
	}

	len += n;

	/* enough pkts to be sent */
	if (unlikely(len == MAX_PKT_BURST)) {

		send_burst(qconf, MAX_PKT_BURST, port);

		/* copy rest of the packets into the TX buffer. */
		len = num - n;
		j = 0;
		switch (len % FWDSTEP) {
		while (j < len) {
		case 0:
			qconf->tx_mbufs[port].m_table[j] = m[n + j];
			j++;
		case 3:
			qconf->tx_mbufs[port].m_table[j] = m[n + j];
			j++;
		case 2:
			qconf->tx_mbufs[port].m_table[j] = m[n + j];
			j++;
		case 1:
			qconf->tx_mbufs[port].m_table[j] = m[n + j];
			j++;
		}
		}
	}

	qconf->tx_mbufs[port].len = len;
}

#ifdef DO_RFC_1812_CHECKS
static inline int
is_valid_ipv4_pkt(struct ipv4_hdr *pkt, uint32_t link_len)
{
	/* From http://www.rfc-editor.org/rfc/rfc1812.txt section 5.2.2 */
	/*
	 * 1. The packet length reported by the Link Layer must be large
	 * enough to hold the minimum length legal IP datagram (20 bytes).
	 */
	if (link_len < sizeof(struct ipv4_hdr))
		return -1;

	/* 2. The IP checksum must be correct. */
	/* this is checked in H/W */

	/*
	 * 3. The IP version number must be 4. If the version number is not 4
	 * then the packet may be another version of IP, such as IPng or
	 * ST-II.
	 */
	if (((pkt->version_ihl) >> 4) != 4)
		return -3;
	/*
	 * 4. The IP header length field must be large enough to hold the
	 * minimum length legal IP datagram (20 bytes = 5 words).
	 */
	if ((pkt->version_ihl & 0xf) < 5)
		return -4;

	/*
	 * 5. The IP total length field must be large enough to hold the IP
	 * datagram header, whose length is specified in the IP header length
	 * field.
	 */
	if (rte_cpu_to_be_16(pkt->total_length) < sizeof(struct ipv4_hdr))
		return -5;

	return 0;
}
#endif

#if (APP_LOOKUP_METHOD == APP_LOOKUP_EXACT_MATCH)

static __m128i mask01;
static __m128i mask02;
//static __m128i mask03;
static __m128i mask1;
static __m128i mask2;


static inline uint8_t
get_ipv6_dst_port(void *ipv6_hdr,  uint8_t portid, lookup_struct_t * ipv6_l3fwd_lookup_struct)
{
	int ret = 0;
	union ipv6_5tuple_host key;

	ipv6_hdr = (uint8_t *)ipv6_hdr + offsetof(struct ipv6_hdr, payload_len);
	__m128i data0 = _mm_loadu_si128((__m128i*)(ipv6_hdr));
	__m128i data1 = _mm_loadu_si128((__m128i*)(((uint8_t*)ipv6_hdr)+sizeof(__m128i)));
	__m128i data2 = _mm_loadu_si128((__m128i*)(((uint8_t*)ipv6_hdr)+sizeof(__m128i)+sizeof(__m128i)));
	
	key.xmm[0] = _mm_and_si128(data0, mask1);// Get part of 5 tuple: src IP address lower 96 bits and protocol 
	key.xmm[1] = data1;                      // Get part of 5 tuple: dst IP address lower 96 bits and src IP address higher 32 bits
	key.xmm[2] = _mm_and_si128(data2, mask2);// Get part of 5 tuple: dst port and src port and dst IP address higher 32 bits 

	/* Find destination port */
	ret = rte_hash_lookup(ipv6_l3fwd_lookup_struct, (const void *)&key);
	return (uint8_t)((ret < 0)? portid : ipv6_l3fwd_out_if[ret]);
}
#endif

#if (APP_LOOKUP_METHOD == APP_LOOKUP_LPM)

#endif

#if ((APP_LOOKUP_METHOD == APP_LOOKUP_EXACT_MATCH) && \
	(ENABLE_MULTI_BUFFER_OPTIMIZE == 1))
static inline void l3fwd_simple_forward(struct rte_mbuf *m, uint8_t portid, struct lcore_conf *qconf);

#define MASK_ALL_PKTS    0xf
#define EXECLUDE_1ST_PKT 0xe
#define EXECLUDE_2ND_PKT 0xd
#define EXECLUDE_3RD_PKT 0xb
#define EXECLUDE_4TH_PKT 0x7
/*
static inline void
simple_ipv4_fwd_4pkts(struct rte_mbuf* m[4], uint8_t portid, struct lcore_conf *qconf)
{
	struct ether_hdr *eth_hdr[4];
	struct ipv4_hdr *ipv4_hdr[4];
	void *d_addr_bytes[4];
	uint8_t dst_port[4];
	int32_t ret[4];
	union ipv4_5tuple_host key[4];
	//__m128i data[4];

	eth_hdr[0] = rte_pktmbuf_mtod(m[0], struct ether_hdr *);
	eth_hdr[1] = rte_pktmbuf_mtod(m[1], struct ether_hdr *);
	eth_hdr[2] = rte_pktmbuf_mtod(m[2], struct ether_hdr *);
	eth_hdr[3] = rte_pktmbuf_mtod(m[3], struct ether_hdr *);

	// Handle IPv4 headers.
	ipv4_hdr[0] = (struct ipv4_hdr *)(rte_pktmbuf_mtod(m[0], unsigned char *) +
			sizeof(struct ether_hdr));
	ipv4_hdr[1] = (struct ipv4_hdr *)(rte_pktmbuf_mtod(m[1], unsigned char *) +
			sizeof(struct ether_hdr));
	ipv4_hdr[2] = (struct ipv4_hdr *)(rte_pktmbuf_mtod(m[2], unsigned char *) +
			sizeof(struct ether_hdr));
	ipv4_hdr[3] = (struct ipv4_hdr *)(rte_pktmbuf_mtod(m[3], unsigned char *) +
			sizeof(struct ether_hdr));

#ifdef DO_RFC_1812_CHECKS
	// Check to make sure the packet is valid (RFC1812) 
	uint8_t valid_mask = MASK_ALL_PKTS;
	if (is_valid_ipv4_pkt(ipv4_hdr[0], m[0]->pkt.pkt_len) < 0) {
		rte_pktmbuf_free(m[0]);
		valid_mask &= EXECLUDE_1ST_PKT;
	}
	if (is_valid_ipv4_pkt(ipv4_hdr[1], m[1]->pkt.pkt_len) < 0) {
		rte_pktmbuf_free(m[1]);
		valid_mask &= EXECLUDE_2ND_PKT;
	}
	if (is_valid_ipv4_pkt(ipv4_hdr[2], m[2]->pkt.pkt_len) < 0) {
		rte_pktmbuf_free(m[2]);
		valid_mask &= EXECLUDE_3RD_PKT;
	}
	if (is_valid_ipv4_pkt(ipv4_hdr[3], m[3]->pkt.pkt_len) < 0) {
		rte_pktmbuf_free(m[3]);
		valid_mask &= EXECLUDE_4TH_PKT;
	}
	if (unlikely(valid_mask != MASK_ALL_PKTS)) {
		if (valid_mask == 0){
			return;
		} else {
			uint8_t i = 0;
			for (i = 0; i < 4; i++) {
				if ((0x1 << i) & valid_mask) {
					l3fwd_simple_forward(m[i], portid, qconf);
				}
			}
			return;
		}
	}
#endif // End of #ifdef DO_RFC_1812_CHECKS

	data[0] = _mm_loadu_si128((__m128i*)(rte_pktmbuf_mtod(m[0], unsigned char *) +
		sizeof(struct ether_hdr) + offsetof(struct ipv4_hdr, time_to_live)));
	data[1] = _mm_loadu_si128((__m128i*)(rte_pktmbuf_mtod(m[1], unsigned char *) +
		sizeof(struct ether_hdr) + offsetof(struct ipv4_hdr, time_to_live)));
	data[2] = _mm_loadu_si128((__m128i*)(rte_pktmbuf_mtod(m[2], unsigned char *) +
		sizeof(struct ether_hdr) + offsetof(struct ipv4_hdr, time_to_live)));
	data[3] = _mm_loadu_si128((__m128i*)(rte_pktmbuf_mtod(m[3], unsigned char *) +
		sizeof(struct ether_hdr) + offsetof(struct ipv4_hdr, time_to_live)));

	//key[0].xmm = _mm_and_si128(data[0], mask0);
	//key[1].xmm = _mm_and_si128(data[1], mask0);
	//key[2].xmm = _mm_and_si128(data[2], mask0);
	//key[3].xmm = _mm_and_si128(data[3], mask0);

	const void *key_array[4] = {&key[0], &key[1], &key[2],&key[3]};
	rte_hash_lookup_multi(qconf->ipv4_lookup_struct, &key_array[0], 4, ret);
	dst_port[0] = (uint8_t) ((ret[0] < 0) ? portid : ipv4_l3fwd_out_if[ret[0]]);
	dst_port[1] = (uint8_t) ((ret[1] < 0) ? portid : ipv4_l3fwd_out_if[ret[1]]);
	dst_port[2] = (uint8_t) ((ret[2] < 0) ? portid : ipv4_l3fwd_out_if[ret[2]]);
	dst_port[3] = (uint8_t) ((ret[3] < 0) ? portid : ipv4_l3fwd_out_if[ret[3]]);

	if (dst_port[0] >= RTE_MAX_ETHPORTS || (enabled_port_mask & 1 << dst_port[0]) == 0)
		dst_port[0] = portid;
	if (dst_port[1] >= RTE_MAX_ETHPORTS || (enabled_port_mask & 1 << dst_port[1]) == 0)
		dst_port[1] = portid;
	if (dst_port[2] >= RTE_MAX_ETHPORTS || (enabled_port_mask & 1 << dst_port[2]) == 0)
		dst_port[2] = portid;
	if (dst_port[3] >= RTE_MAX_ETHPORTS || (enabled_port_mask & 1 << dst_port[3]) == 0)
		dst_port[3] = portid;

	// 02:00:00:00:00:xx 
	d_addr_bytes[0] = &eth_hdr[0]->d_addr.addr_bytes[0];
	d_addr_bytes[1] = &eth_hdr[1]->d_addr.addr_bytes[0];
	d_addr_bytes[2] = &eth_hdr[2]->d_addr.addr_bytes[0];
	d_addr_bytes[3] = &eth_hdr[3]->d_addr.addr_bytes[0];
	*((uint64_t *)d_addr_bytes[0]) = 0x000000000002 + ((uint64_t)dst_port[0] << 40);
	*((uint64_t *)d_addr_bytes[1]) = 0x000000000002 + ((uint64_t)dst_port[1] << 40);
	*((uint64_t *)d_addr_bytes[2]) = 0x000000000002 + ((uint64_t)dst_port[2] << 40);
	*((uint64_t *)d_addr_bytes[3]) = 0x000000000002 + ((uint64_t)dst_port[3] << 40);

#ifdef DO_RFC_1812_CHECKS
	// Update time to live and header checksum 
	--(ipv4_hdr[0]->time_to_live);
	--(ipv4_hdr[1]->time_to_live);
	--(ipv4_hdr[2]->time_to_live);
	--(ipv4_hdr[3]->time_to_live);
	++(ipv4_hdr[0]->hdr_checksum);
	++(ipv4_hdr[1]->hdr_checksum);
	++(ipv4_hdr[2]->hdr_checksum);
	++(ipv4_hdr[3]->hdr_checksum);
#endif

	// src addr 
	ether_addr_copy(&ports_eth_addr[dst_port[0]], &eth_hdr[0]->s_addr);
	ether_addr_copy(&ports_eth_addr[dst_port[1]], &eth_hdr[1]->s_addr);
	ether_addr_copy(&ports_eth_addr[dst_port[2]], &eth_hdr[2]->s_addr);
	ether_addr_copy(&ports_eth_addr[dst_port[3]], &eth_hdr[3]->s_addr);

	send_single_packet(m[0], (uint8_t)dst_port[0]);
	send_single_packet(m[1], (uint8_t)dst_port[1]);
	send_single_packet(m[2], (uint8_t)dst_port[2]);
	send_single_packet(m[3], (uint8_t)dst_port[3]);

}
*/
static inline void get_ipv6_5tuple(struct rte_mbuf* m0, __m128i mask0, __m128i mask1,
				 union ipv6_5tuple_host * key)
{
        __m128i tmpdata0 = _mm_loadu_si128((__m128i*)(rte_pktmbuf_mtod(m0, unsigned char *)
			+ sizeof(struct ether_hdr) + offsetof(struct ipv6_hdr, payload_len)));
        __m128i tmpdata1 = _mm_loadu_si128((__m128i*)(rte_pktmbuf_mtod(m0, unsigned char *)
			+ sizeof(struct ether_hdr) + offsetof(struct ipv6_hdr, payload_len)
			+  sizeof(__m128i)));
        __m128i tmpdata2 = _mm_loadu_si128((__m128i*)(rte_pktmbuf_mtod(m0, unsigned char *)
			+ sizeof(struct ether_hdr) + offsetof(struct ipv6_hdr, payload_len)
			+ sizeof(__m128i) + sizeof(__m128i)));
        key->xmm[0] = _mm_and_si128(tmpdata0, mask0);
        key->xmm[1] = tmpdata1;
        key->xmm[2] = _mm_and_si128(tmpdata2, mask1);
	return;
}
/*
static inline void
simple_ipv6_fwd_4pkts(struct rte_mbuf* m[4], uint8_t portid, struct lcore_conf *qconf)
{
	struct ether_hdr *eth_hdr[4];
	__attribute__((unused)) struct ipv6_hdr *ipv6_hdr[4];
	void *d_addr_bytes[4];
	uint8_t dst_port[4];
	int32_t ret[4];
	union ipv6_5tuple_host key[4];

	eth_hdr[0] = rte_pktmbuf_mtod(m[0], struct ether_hdr *);
	eth_hdr[1] = rte_pktmbuf_mtod(m[1], struct ether_hdr *);
	eth_hdr[2] = rte_pktmbuf_mtod(m[2], struct ether_hdr *);
	eth_hdr[3] = rte_pktmbuf_mtod(m[3], struct ether_hdr *);

	// Handle IPv6 headers.
	ipv6_hdr[0] = (struct ipv6_hdr *)(rte_pktmbuf_mtod(m[0], unsigned char *) +
			sizeof(struct ether_hdr));
	ipv6_hdr[1] = (struct ipv6_hdr *)(rte_pktmbuf_mtod(m[1], unsigned char *) +
			sizeof(struct ether_hdr));
	ipv6_hdr[2] = (struct ipv6_hdr *)(rte_pktmbuf_mtod(m[2], unsigned char *) +
			sizeof(struct ether_hdr));
	ipv6_hdr[3] = (struct ipv6_hdr *)(rte_pktmbuf_mtod(m[3], unsigned char *) +
			sizeof(struct ether_hdr));

	get_ipv6_5tuple(m[0], mask1, mask2, &key[0]);
	get_ipv6_5tuple(m[1], mask1, mask2, &key[1]);
	get_ipv6_5tuple(m[2], mask1, mask2, &key[2]);
	get_ipv6_5tuple(m[3], mask1, mask2, &key[3]);

	const void *key_array[4] = {&key[0], &key[1], &key[2],&key[3]};
	rte_hash_lookup_multi(qconf->ipv6_lookup_struct, &key_array[0], 4, ret);
	dst_port[0] = (uint8_t) ((ret[0] < 0)? portid:ipv6_l3fwd_out_if[ret[0]]);
	dst_port[1] = (uint8_t) ((ret[1] < 0)? portid:ipv6_l3fwd_out_if[ret[1]]);
	dst_port[2] = (uint8_t) ((ret[2] < 0)? portid:ipv6_l3fwd_out_if[ret[2]]);
	dst_port[3] = (uint8_t) ((ret[3] < 0)? portid:ipv6_l3fwd_out_if[ret[3]]);

	if (dst_port[0] >= RTE_MAX_ETHPORTS || (enabled_port_mask & 1 << dst_port[0]) == 0)
		dst_port[0] = portid;
	if (dst_port[1] >= RTE_MAX_ETHPORTS || (enabled_port_mask & 1 << dst_port[1]) == 0)
		dst_port[1] = portid;
	if (dst_port[2] >= RTE_MAX_ETHPORTS || (enabled_port_mask & 1 << dst_port[2]) == 0)
		dst_port[2] = portid;
	if (dst_port[3] >= RTE_MAX_ETHPORTS || (enabled_port_mask & 1 << dst_port[3]) == 0)
		dst_port[3] = portid;

	// 02:00:00:00:00:xx 
	d_addr_bytes[0] = &eth_hdr[0]->d_addr.addr_bytes[0];
	d_addr_bytes[1] = &eth_hdr[1]->d_addr.addr_bytes[0];
	d_addr_bytes[2] = &eth_hdr[2]->d_addr.addr_bytes[0];
	d_addr_bytes[3] = &eth_hdr[3]->d_addr.addr_bytes[0];
	*((uint64_t *)d_addr_bytes[0]) = 0x000000000002 + ((uint64_t)dst_port[0] << 40);
	*((uint64_t *)d_addr_bytes[1]) = 0x000000000002 + ((uint64_t)dst_port[1] << 40);
	*((uint64_t *)d_addr_bytes[2]) = 0x000000000002 + ((uint64_t)dst_port[2] << 40);
	*((uint64_t *)d_addr_bytes[3]) = 0x000000000002 + ((uint64_t)dst_port[3] << 40);

	// src addr 
	ether_addr_copy(&ports_eth_addr[dst_port[0]], &eth_hdr[0]->s_addr);
	ether_addr_copy(&ports_eth_addr[dst_port[1]], &eth_hdr[1]->s_addr);
	ether_addr_copy(&ports_eth_addr[dst_port[2]], &eth_hdr[2]->s_addr);
	ether_addr_copy(&ports_eth_addr[dst_port[3]], &eth_hdr[3]->s_addr);

	send_single_packet(m[0], (uint8_t)dst_port[0]);
	send_single_packet(m[1], (uint8_t)dst_port[1]);
	send_single_packet(m[2], (uint8_t)dst_port[2]);
	send_single_packet(m[3], (uint8_t)dst_port[3]);

}
*/
#endif 

int compare_sid_nid(uint8_t *sid1,uint8_t *sid2,int t)                                              
{          
  int i;                                                                                            
  for(i=0;i<t;i++) 
  {        
    if(sid1[i]==sid2[i])
    continue;
    else return 0;                                                                                  
   }       
  return 1;
}  

long long int pc3_port=0;
long long int pc4_port=0;
long long int pc5_port=0;

int
get_trap_message(struct rte_mbuf *m,uint8_t portid)
{ 
    struct ether_hdr *eth_hdr;
    eth_hdr = rte_pktmbuf_mtod(m,struct ether_hdr *);   //这里可能没m,而是burst
    struct  trap_get_header *trap_head = (struct trap_get_header *)(rte_pktmbuf_mtod(m, unsigned char *));

    uint8_t trap_hello_sid[20];
    uint8_t trap_hello_sid2[20];
    uint8_t trap_hello_nid_3[16];
    uint8_t trap_hello_nid_4[16];
    uint8_t trap_hello_nid_5[16];
    
    int i; 

    for(i=0;i<16;i++){
      if(i==0)trap_hello_nid_3[i]='p';
      else if(i==1)trap_hello_nid_3[i]='c';
      else if(i==2)trap_hello_nid_3[i]='-';
      else if(i==3) trap_hello_nid_3[i]='3';
      else trap_hello_nid_3[i]=0;
    }
    for(i=0;i<16;i++){
      if(i==0)trap_hello_nid_4[i]='p';
      else if(i==1)trap_hello_nid_4[i]='c';
      else if(i==2)trap_hello_nid_4[i]='-';
      else if(i==3) trap_hello_nid_4[i]='4';
      else trap_hello_nid_4[i]=0;
    }
    for(i=0;i<16;i++){
      if(i==0)trap_hello_nid_5[i]='p';
      else if(i==1)trap_hello_nid_5[i]='c';
      else if(i==2)trap_hello_nid_5[i]='-';
      else if(i==3) trap_hello_nid_5[i]='5';
      else trap_hello_nid_5[i]=0;
    }
    for(i=0;i<20;i++){
      if(i<19){trap_hello_sid[i]=0;
               trap_hello_sid2[i]=0;}
      else if(i==19) {trap_hello_sid[i]=2;
                      trap_hello_sid2[i]=3;}
     }
  if(trap_head->version_type==0xa0)
  {
    if(compare_sid_nid(trap_hello_sid,trap_head->sid,20)==1)
    {
      trap_head->sid[19]=3;
      //看是哪个边界路由器发送的,已经修改了nid，把消息重新发送到边界路由器,可以收到返回的trap_data,现在就是解析了。
    if(compare_sid_nid(trap_hello_nid_3,trap_head->nid,16)==1)
      {
       //printf("It's nid is PC-3\n");
       ether_addr_copy(&ports_eth_addr[0], &eth_hdr->s_addr);
       send_single_packet(m, portid);
      }
    else if(compare_sid_nid(trap_hello_nid_4,trap_head->nid,16)==1)
      {
       //printf("It's nid is PC-4\n"); 
       ether_addr_copy(&ports_eth_addr[1], &eth_hdr->s_addr);
       send_single_packet(m, portid);
      }
    else if(compare_sid_nid(trap_hello_nid_5,trap_head->nid,16)==1)
      {
       //printf("It's nid is PC-5\n"); 
       ether_addr_copy(&ports_eth_addr[2], &eth_hdr->s_addr);
       send_single_packet(m, portid);
      }
    }//end if trap nid
  }//end if trap get
    return 0;
}

int data_trap_message(struct rte_mbuf *m)
{
/*
  FILE *fp,*fp2,*fp3;
  char filename1[]="/home/GZLL/traffic_pc3";
  char filename2[]="/home/GZLL/traffic_pc4";
  char filename3[]="/home/GZLL/traffic_pc5";
*/
  struct timeval now_time;

  uint8_t trap_hello_sid[20];
  uint8_t trap_hello_sid2[20];
  uint8_t trap_hello_nid_3[16];
  uint8_t trap_hello_nid_4[16];
  uint8_t trap_hello_nid_5[16];
  int i;
  for(i=0;i<16;i++){
      if(i==0)trap_hello_nid_3[i]='p';
      else if(i==1)trap_hello_nid_3[i]='c';
      else if(i==2)trap_hello_nid_3[i]='-';
      else if(i==3) trap_hello_nid_3[i]='3';
      else trap_hello_nid_3[i]=0;
    }
    for(i=0;i<16;i++){
      if(i==0)trap_hello_nid_4[i]='p';
      else if(i==1)trap_hello_nid_4[i]='c';
      else if(i==2)trap_hello_nid_4[i]='-';
      else if(i==3) trap_hello_nid_4[i]='4';
      else trap_hello_nid_4[i]=0;
    }
    for(i=0;i<16;i++){
      if(i==0)trap_hello_nid_5[i]='p';
      else if(i==1)trap_hello_nid_5[i]='c';
      else if(i==2)trap_hello_nid_5[i]='-';
      else if(i==3) trap_hello_nid_5[i]='5';
      else trap_hello_nid_5[i]=0;
    }
    for(i=0;i<20;i++){
      if(i<19){trap_hello_sid[i]=0;
               trap_hello_sid2[i]=0;}
      else if(i==19) {trap_hello_sid[i]=2;
                      trap_hello_sid2[i]=3;}
     }

  struct trap_data_header *trap_data=(struct trap_data_header *)(rte_pktmbuf_mtod(m,unsigned char *));
   if((compare_sid_nid(trap_hello_nid_3,trap_data->nid,16))&&(compare_sid_nid(trap_hello_sid2,trap_data->sid,20)))
     {
      //printf("pc-3 data\n");
      pc3_port = trap_data->port1;
      //printf("pc-3 port1=%lld\n",pc3_port);
   //   gettimeofday(&now_time,NULL);
  //    fp=fopen(filename1,"a");
    //  fprintf(fp,"port1=%lld port2=%lld time:%ld.%ld\n",strNum2,strNum1,now_time.tv_sec,now_time.tv_usec);
     // fclose(fp);
      rte_pktmbuf_free(m);
     }
    else if((compare_sid_nid(trap_hello_nid_4,trap_data->nid,16))&&(compare_sid_nid(trap_hello_sid2,trap_data->sid,20)))
     {
      //printf("pc-4 data\n");
     // gettimeofday(&now_time,NULL);
      pc4_port = trap_data->port1;
      //printf("pc4_port1=%lld\n",pc4_port);
     // fp2=fopen(filename2,"w+");
     // fprintf(fp2,"port1=%lld port2=%lld time:%ld.%ld\n",strNum2,strNum1,now_time.tv_sec,now_time.tv_usec);
     // fclose(fp2);
      rte_pktmbuf_free(m);
     }
     else if((compare_sid_nid(trap_hello_nid_5,trap_data->nid,16))&&(compare_sid_nid(trap_hello_sid2,trap_data->sid,20)))
     {
     // printf("pc-5 data\n");
      //gettimeofday(&now_time,NULL);
      pc5_port=trap_data->port1;
     // printf("pc5_port1=%lld\n",pc5_port);
      //fp3=fopen(filename3,"w+");
      //fprintf(fp3,"port1=%lld port2=%lld time:%ld.%ld\n",strNum2,strNum1,now_time.tv_sec,now_time.tv_usec);
      //fclose(fp3);
      rte_pktmbuf_free(m);
      }
 return 0;
}

uint32_t probility_pid()
{
  uint32_t pid_add;
  int i = rand()%99 +1;
  char pid[4];
  int j;
  for(j=0;j<4;j++)
   {if(j==0) pid[j]='P';
    else if(j==1)pid[j]='I';
    else if(j==2)pid[j]='D';
    else if(j==3)
         {
          if(i<=33)  pid[j]='0'+1;
          else if(i<=66 && i>33) pid[j]='0'+2;
          else if(i<=99 && i>66) pid[j]='0'+3;
         }
    }
   memcpy(&pid_add,pid,4);
   return pid_add;
}

uint32_t probility_pid046()
{
  uint32_t pid_add;
  int i = rand()%100 +1;
  char pid[4];
  int j;
  for(j=0;j<4;j++)
   {if(j==0) pid[j]='P';
    else if(j==1)pid[j]='I';
    else if(j==2)pid[j]='D';
    else if(j==3)
         {
          if(i<=40)  pid[j]='0'+2;
          //else if(i<=80 && i>60) pid[j]='0'+2;
          else if(i<=100 && i>40) pid[j]='0'+3;
         }
    }
   memcpy(&pid_add,pid,4);
   return pid_add;
}
uint32_t probility_pid532()
{
  uint32_t pid_add;
  int i = rand()%100 +1;
  char pid[4];
  int j;
  for(j=0;j<4;j++)
   {if(j==0) pid[j]='P';
    else if(j==1)pid[j]='I';
    else if(j==2)pid[j]='D';
    else if(j==3)
         {
          if(i<=50)  pid[j]='0'+1;
          else if(i<=80 && i>50) pid[j]='0'+2;
          else if(i<=100 && i>80) pid[j]='0'+3;
         }
    }
   memcpy(&pid_add,pid,4);
   return pid_add;
}


uint32_t random_pid()
{
  uint32_t pid_add;
  srand(time(NULL));
  int i = rand()%3+1;
  char pid[4];
  int j;
  for(j=0;j<4;j++)
   {if(j==0) pid[j]='P';
    else if(j==1)pid[j]='I';
    else if(j==2)pid[j]='D';
    else if(j==3)pid[j]='0'+i;
    };
   memcpy(&pid_add,pid,4);
  
   return pid_add;
}

/**
   已经知道流量和SBD
   最大值最小，均衡
**/
uint32_t max_min_utility(uint32_t cc_flow_size)
{
  uint32_t pid_add;
  int k;
  if((pc3_port<pc4_port)&&(pc3_port<pc5_port))
  k=1;
  else if((pc4_port<pc3_port)&&(pc4_port<pc5_port))
  k=2;
  else if((pc5_port<pc3_port)&&(pc5_port<pc4_port))
  k=3;
  else 
      {
       srand(time(NULL));
       k = rand()%3+1;
     }


  if(k==1)
    pc3_port=pc3_port+cc_flow_size;
  else if(k==2)
    pc4_port=pc4_port+cc_flow_size;
  else if(k==3)
    pc5_port=pc5_port+cc_flow_size;
   
  //printf("k=%d\n",k);
  char pid[4];
  int j;
  for(j=0;j<4;j++)
   {if(j==0) pid[j]='P';
    else if(j==1)pid[j]='I';
    else if(j==2)pid[j]='D';
    else if(j==3)pid[j]='0'+k;
    };
   memcpy(&pid_add,pid,4);
   return pid_add;
}


int k2=0;
int flag2=0;
uint32_t lunxun()
{
uint32_t pid_add;
int k;
if(k2==0 && flag2==0)    {k=1;k2=1;}
else if(k2==1 && flag2==0){k=2;flag2=1;}
else if(k2==1 && flag2==1){k=3;k2=0;flag2=0;}

char pid[4];
int j;
for(j=0;j<4;j++)
   {
    if(j==0) pid[j]='P';
    else if(j==1)pid[j]='I';
    else if(j==2)pid[j]='D';
    else if(j==3)pid[j]='0'+k;   //轮询时为K,此处为了测试数据做了修改，2016.12.2
    };
memcpy(&pid_add,pid,4);
return pid_add;
}

int k3=0;
uint32_t lunxun2()
{
 uint32_t pid_add;
 int k;
 if(k3==0) {k=2;k3=1;}
 else if (k2=1){k=3;k3=0;}

  char pid[4];
  int j;
  for(j=0;j<4;j++)
   {
    if(j==0) pid[j]='P';
    else if(j==1)pid[j]='I';
    else if(j==2)pid[j]='D';
    else if(j==3)pid[j]='0'+k;   //轮询时为K,此处为了测试数据做了修改，2016.12.2
    };
  memcpy(&pid_add,pid,4);
  return pid_add;
}

static struct itimerval oldtv;
int biaozhi = 0;
void set_timer()
{
  struct itimerval itv;
  itv.it_interval.tv_sec = 1000;
  itv.it_interval.tv_usec=0;
  itv.it_value.tv_sec=1000;
  itv.it_value.tv_usec=0;
  setitimer(ITIMER_REAL, &itv, &oldtv);
}
void signal_handler(int m)
{
 if(biaozhi==0)
  biaozhi=1;
 else if(biaozhi==1)
  biaozhi=0;
 printf("biaozhi=%d\n",biaozhi);
}

static inline int
ADD_PID(struct rte_mbuf *m)
{
  struct CoLoR_get *color_get = (struct CoLoR_get *)(rte_pktmbuf_mtod(m,unsigned char *)+sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr));
  color_get->total_len = m->pkt.data_len-sizeof(struct ipv4_hdr)-sizeof(struct ether_hdr);

  uint32_t *pid_add = (uint32_t *)((uint8_t *)color_get + color_get->total_len);
  uint32_t pid_xuan;
  //if(biaozhi==0)
  //pid_xuan = probility_pid532();
  //else if(biaozhi==1)
  //{
  // pid_xuan = probility_pid046();
  //}

  pid_xuan = lunxun();
	printf("%c\n",   *((char*)&pid_xuan+3)   );
	getchar();
  //pid_xuan = max_min_utility(color_get->cc_flow_size);
 
  //  if(color_get->cc_flow_size >= 10000000)
  //  pid_xuan=lunxun();
  //  else
  //  pid_xuan=probility_pid();
 
  //pid_xuan=dongtai(color_get->cc_flow_size);
 //pid_xuan=probility_pid();
 // printf("pid_xuan：%x\n",pid_xuan);
  memcpy(pid_add,&pid_xuan,4);

  color_get->total_len=rte_cpu_to_be_16(color_get->total_len+4);
  m->pkt.data_len+=4;
  m->pkt.pkt_len+=4;
  char pid1[4]={'P','I','D','1'};
  char pid2[4]={'P','I','D','2'};
  char pid3[4]={'P','I','D','3'};
  uint32_t pid1_d,pid2_d,pid3_d;
  memcpy(&pid1_d,pid1,4);
  memcpy(&pid2_d,pid2,4);
  memcpy(&pid3_d,pid3,4);
  if(memcmp(pid_add,pid1,4)==0) 
  return 1;
  else if(memcmp(pid_add,pid2,4)==0)
  return 2;
  else if (memcmp(pid_add,pid3,4)==0)
  return 3;
  else return 0;
}

static void
get_packets_forward(struct rte_mbuf *m,uint8_t portid,struct lcore_conf *qconf)
{
    struct ether_hdr *eth_hdr;
    struct ipv4_hdr *ipv4_hdr;
    int ret;
    uint8_t port_id;
    eth_hdr = rte_pktmbuf_mtod(m,struct ether_hdr *);
    ipv4_hdr = (struct ipv4_hdr *)(rte_pktmbuf_mtod(m,unsigned char *) + sizeof(struct ether_hdr));
    ipv4_hdr->total_length +=4;

    #ifdef DO_RFC_1812_CHECKS
		/* Check to make sure the packet is valid (RFC1812) */
		if (is_valid_ipv4_pkt(ipv4_hdr, m->pkt.pkt_len) < 0) {
			rte_pktmbuf_free(m);
			return;
		}
    #endif
  
    CoLoR_get_t * get_hdr;
    get_hdr = (CoLoR_get_t *)(rte_pktmbuf_mtod(m,unsigned char *) + sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr));
    get_hdr->total_len+=4;
    get_hdr->PIDs+=1;
    ret = ADD_PID(m);
    if(ret == 0) {printf("ADD PID wrong!\n");}
    else if (ret==1) {port_id = 1;ipv4_hdr->src_addr=htonl(IPv4(192,168,2,1));ipv4_hdr->dst_addr=htonl(IPv4(192,168,2,2));}
    else if (ret==2) {port_id=2;ipv4_hdr->src_addr=htonl(IPv4(192,168,3,1));ipv4_hdr->dst_addr=htonl(IPv4(192,168,3,2));}
    else if (ret==3){port_id=3;ipv4_hdr->src_addr=htonl(IPv4(192,168,4,1));ipv4_hdr->dst_addr=htonl(IPv4(192,168,4,2));}
    #ifdef DO_RFC_1812_CHECKS
		--(ipv4_hdr->time_to_live);
		++(ipv4_hdr->hdr_checksum);
    #endif
    ether_addr_copy(&ports_eth_addr[port_id], &eth_hdr->s_addr);
    //send_single_packet(m,port_id);
   ret = rte_eth_tx_burst(port_id,0,&m,1);
 
}
static inline __attribute__((always_inline)) void
l3fwd_simple_forward(struct rte_mbuf *m, uint8_t portid, struct lcore_conf *qconf)
{
	struct ether_hdr *eth_hdr;
	struct ipv4_hdr *ipv4_hdr;
	void *d_addr_bytes;
	uint8_t dst_port;

	eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);

	if (m->ol_flags & PKT_RX_IPV4_HDR) {
		/* Handle IPv4 headers.*/
		ipv4_hdr = (struct ipv4_hdr *)(rte_pktmbuf_mtod(m, unsigned char *) +
				sizeof(struct ether_hdr));


/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/
	
//--------------------------Print the IP address------------------------------------------------     
		int i=0;     
		CoLoR_get_t * get_hdr;

                printf("[From %s]",__func__);
		print_ip_address_wxb(ipv4_hdr);
                
                get_hdr = (CoLoR_get_t *)(rte_pktmbuf_mtod(m, unsigned char *) +sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr));	
                printf("[From %s]nid_sid=",__func__);
                for(i=0 ;i<16 ;i++)
                {
                     printf("%2X",get_hdr->nid_sid[i]);
                }
                printf("\n");
                
                printf("[From %s]l_sid=",__func__);
                for(i=0 ;i<20 ;i++)
                {
                     printf("%2X",get_hdr->l_sid[i]);
                }
                printf("\n");
                
                uint32_t sid_ip=get_sid_dst_ipv4(ipv4_hdr,qconf->sid_lookup_struct_wxb);
                
                update_IP(ipv4_hdr,sid_ip);
                
                printf("[From %s]",__func__);
		print_ip_address_wxb(ipv4_hdr);
                
                uint32_t sid_pid=get_sid_dst_pid(ipv4_hdr,qconf->sid_lookup_struct_wxb);
                printf("[From %s]PID=%X\n",__func__,sid_pid);
  
                uint8_t sid_flag;
                sid_flag=get_sid_dst_flag(ipv4_hdr,qconf->sid_lookup_struct_wxb);
                printf("[From %s]Flag=%X\n",__func__,sid_flag);
 
                nid_t sid_nid;
                sid_nid=get_sid_dst_nid(ipv4_hdr,qconf->sid_lookup_struct_wxb);
                printf("[From %s]NID=",__func__);
                for(i=0;i<16;i++)
                {
                  printf("%2X",sid_nid.nid[i]);
                }
                printf("\n");
                
                printf("[From %s]Total length=%d\n",__func__,m->pkt.data_len);
                if(sid_flag)
                {
                   update_PID(m,sid_pid);
                }
                
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************End************/             


#ifdef DO_RFC_1812_CHECKS
		/* Check to make sure the packet is valid (RFC1812) */
		if (is_valid_ipv4_pkt(ipv4_hdr, m->pkt.pkt_len) < 0) {
			rte_pktmbuf_free(m);
			return;
		}
#endif

/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/
		dst_port = get_ipv4_dst_port_wxb(ipv4_hdr, portid, qconf->ipv4_lookup_struct_wxb);	
		printf("[From %s]dst_port=%d\n",__func__,dst_port);	
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************End************/

		if (dst_port >= RTE_MAX_ETHPORTS ||
				(enabled_port_mask & 1 << dst_port) == 0)
			dst_port = portid;

		/* 02:00:00:00:00:xx */
		d_addr_bytes = &eth_hdr->d_addr.addr_bytes[0];
		*((uint64_t *)d_addr_bytes) = ETHER_LOCAL_ADMIN_ADDR +
			((uint64_t)dst_port << 40);

#ifdef DO_RFC_1812_CHECKS
		/* Update time to live and header checksum */
		--(ipv4_hdr->time_to_live);
		++(ipv4_hdr->hdr_checksum);
#endif

		/* src addr */
		ether_addr_copy(&ports_eth_addr[dst_port], &eth_hdr->s_addr);

		send_single_packet(m, dst_port);

	} else {
		/* Handle IPv6 headers.*/
		struct ipv6_hdr *ipv6_hdr;

		ipv6_hdr = (struct ipv6_hdr *)(rte_pktmbuf_mtod(m, unsigned char *) +
				sizeof(struct ether_hdr));

		dst_port = get_ipv6_dst_port(ipv6_hdr, portid, qconf->ipv6_lookup_struct);

		if (dst_port >= RTE_MAX_ETHPORTS || (enabled_port_mask & 1 << dst_port) == 0)
			dst_port = portid;

		/* 02:00:00:00:00:xx */
		d_addr_bytes = &eth_hdr->d_addr.addr_bytes[0];
		*((uint64_t *)d_addr_bytes) = ETHER_LOCAL_ADMIN_ADDR +
			((uint64_t)dst_port << 40);

		/* src addr */
		ether_addr_copy(&ports_eth_addr[dst_port], &eth_hdr->s_addr);

		send_single_packet(m, dst_port);
	}

}

#ifdef DO_RFC_1812_CHECKS

#define	IPV4_MIN_VER_IHL	0x45
#define	IPV4_MAX_VER_IHL	0x4f
#define	IPV4_MAX_VER_IHL_DIFF	(IPV4_MAX_VER_IHL - IPV4_MIN_VER_IHL)

/* Minimum value of IPV4 total length (20B) in network byte order. */
#define	IPV4_MIN_LEN_BE	(sizeof(struct ipv4_hdr) << 8)

/*
 * From http://www.rfc-editor.org/rfc/rfc1812.txt section 5.2.2:
 * - The IP version number must be 4.
 * - The IP header length field must be large enough to hold the
 *    minimum length legal IP datagram (20 bytes = 5 words).
 * - The IP total length field must be large enough to hold the IP
 *   datagram header, whose length is specified in the IP header length
 *   field.
 * If we encounter invalid IPV4 packet, then set destination port for it
 * to BAD_PORT value.
 */
static inline __attribute__((always_inline)) void
rfc1812_process(struct ipv4_hdr *ipv4_hdr, uint16_t *dp, uint32_t flags)
{
	uint8_t ihl;

	if ((flags & PKT_RX_IPV4_HDR) != 0) {

		ihl = ipv4_hdr->version_ihl - IPV4_MIN_VER_IHL;

		ipv4_hdr->time_to_live--;
		ipv4_hdr->hdr_checksum++;

		if (ihl > IPV4_MAX_VER_IHL_DIFF ||
				((uint8_t)ipv4_hdr->total_length == 0 &&
				ipv4_hdr->total_length < IPV4_MIN_LEN_BE)) {
			dp[0] = BAD_PORT;
		}
	}
}

#else
#define	rfc1812_process(mb, dp)	do { } while (0)
#endif /* DO_RFC_1812_CHECKS */


#if ((APP_LOOKUP_METHOD == APP_LOOKUP_LPM) && \
	(ENABLE_MULTI_BUFFER_OPTIMIZE == 1))

#endif /* APP_LOOKUP_METHOD */

/****************
用来进行数据包封装，主要是封装
color数据包的控制包 

加了校验和的三个函数;

11月2号成功发送数据包，11月3号周二将代码构造成函数，并添加定时功能。
*****************/
#define GET_PACKET 0xA0
#define DATA_PACKET 0xA1
#define CONTROL_PACKET 0xA3

#define CONTROL_PACKET_PID 0x10
#define CONTROL_PACKET_PID_ANNOUNCE 129

struct ipHdr_s{
    uint8_t        vl;	        /* Version and header length */
    uint8_t        tos;         /* Type of Service */
    uint16_t       tlen;        /* total length */
    uint16_t       ident;       /* identification */
    uint16_t       ffrag;       /* Flags and Fragment offset */
    uint8_t        ttl;         /* Time to Live */
    uint8_t        proto;       /* Protocol */
    uint16_t       cksum;       /* Header checksum */
    uint32_t       src;         /* source IP address */
    uint32_t       dst;         /* destination IP address */
} __attribute__((__packed__));

#define OPTION_FLAG 		0x40
#define OPTION_KIND_MASK 		0x30
#define OPTION_KIND_MASK_REVERSE 	0xCF
#define OPTION_KIND0_change_request	0x00
#define OPTION_KIND1_a			0x10
#define OPTION_KIND2_b			0x20
#define OPTION_KIND3_ack		0x30

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
    uint8_t signature[55];
}__attribute__((__packed__));
//共计：140个字节

/**********************line_chain of pids*************************/
#define MAX_PID_NUMBER 20
struct pid_info
{
  uint8_t EID[16];
  uint8_t ID;
  uint32_t pid_change_time;
  uint32_t number_pid;
  uint32_t pid[MAX_PID_NUMBER];
}__attribute__((__packed__));

typedef struct pid_info_table *link;  //link为节点指针

struct pid_info_table
{
   struct pid_info item;    //数据域
   link next;               //链域
}__attribute__((__packed__));
link head = NULL;    //现在head还是NULL,等初始化完成后head就成了所需要的链表的首部了。

//此函数的参数pid_number请注意
link make_node(struct pid_info *item){
   link p = (struct pid_info_table *)  malloc(sizeof(struct pid_info_table));//2 is to make program correcetly! 
   
   p->item = *item;
   p->next = NULL;
   return p;
}

//释放节点p,注意释放之后链表会断掉，需要重新连接
void free_node(link p){
  free(p);
}

//此函数用来获取链表p，pos位置的item，将item返回给p_item指针,正确返回1,错误返回-1
int get_item(link p, uint8_t pos, struct pid_info *p_item)
{
  link chain;
  uint8_t i;
  
  chain = p;
  i = 0;
  
  while(chain!=NULL && i<pos) //将指针移动到要返回元素的位置
   {
      chain = chain->next;
      i++;
   }
   if ((chain==NULL)||(i>pos)){
    return -1;
   }
   *p_item = chain->item;
   return 1;
}

//此函数用于查找依据EID查找 PID，输入EID数组的首地址，返回指针p
link search(link p, uint8_t *EID){
   link chain;
   chain = p;
   uint8_t EID_key[16];
   memcpy(EID_key,EID,16);
   
   if(chain==NULL){
    printf("链表为空！\n");
    return NULL;
   }

   while((chain->next!=NULL) &&  ((chain->item.EID[0]!=EID_key[0])||
                                  (chain->item.EID[1]!=EID_key[1])||
                                  (chain->item.EID[2]!=EID_key[2])||
                                  (chain->item.EID[3]!=EID_key[3])||
                                  (chain->item.EID[4]!=EID_key[4])||
                                  (chain->item.EID[5]!=EID_key[5])||
                                  (chain->item.EID[6]!=EID_key[6])||
                                  (chain->item.EID[7]!=EID_key[7])||
                                  (chain->item.EID[8]!=EID_key[8])||
                                  (chain->item.EID[9]!=EID_key[9])||
                                  (chain->item.EID[10]!=EID_key[10])||
                                  (chain->item.EID[11]!=EID_key[11])||
                                  (chain->item.EID[12]!=EID_key[12])||
                                  (chain->item.EID[13]!=EID_key[13])||
                                  (chain->item.EID[14]!=EID_key[14])||
                                  (chain->item.EID[15]!=EID_key[15]))
        )
        {
           chain = chain->next;   //查找匹配的EID，注意，此处还可以采用递归的方法来做
         }
    if(chain != NULL && (chain->item.EID[0]!=EID_key[0])||
                        (chain->item.EID[1]!=EID_key[1])||
                        (chain->item.EID[2]!=EID_key[2])||
                        (chain->item.EID[3]!=EID_key[3])||
                        (chain->item.EID[4]!=EID_key[4])||
                        (chain->item.EID[5]!=EID_key[5])||
                        (chain->item.EID[6]!=EID_key[6])||
                        (chain->item.EID[7]!=EID_key[7])||
                        (chain->item.EID[8]!=EID_key[8])||
                        (chain->item.EID[9]!=EID_key[9])||
                        (chain->item.EID[10]!=EID_key[10])||
                        (chain->item.EID[11]!=EID_key[11])||
                        (chain->item.EID[12]!=EID_key[12])||
                        (chain->item.EID[13]!=EID_key[13])||
                        (chain->item.EID[14]!=EID_key[14])||
                        (chain->item.EID[15]!=EID_key[15]))
       {
         printf("[From %s]在链表中未找到EID\n",__func__);
         return NULL;
       }
     if((chain->item.EID[0]==EID_key[0])&&  
        (chain->item.EID[1]==EID_key[1])&&
        (chain->item.EID[2]==EID_key[2])&&
        (chain->item.EID[3]==EID_key[3])&&
        (chain->item.EID[4]==EID_key[4])&&
        (chain->item.EID[5]==EID_key[5])&&
        (chain->item.EID[6]==EID_key[6])&&
        (chain->item.EID[7]==EID_key[7])&&
        (chain->item.EID[8]==EID_key[8])&&
        (chain->item.EID[9]==EID_key[9])&&
        (chain->item.EID[10]==EID_key[10])&&
        (chain->item.EID[11]==EID_key[11])&&
        (chain->item.EID[12]==EID_key[12])&&
        (chain->item.EID[13]==EID_key[13])&&
        (chain->item.EID[14]==EID_key[14])&&
        (chain->item.EID[15]==EID_key[15]))
        { printf("[From %s]找到EID，已返回\n",__func__);}
      
     return chain;      //返回找到的节点
}


static inline void
print_configuration_init_pid_change(link head)
{
	uint8_t wxb_k;
	link AS=head;
	printf("[From %s]\n",__func__);  
	while(AS!=NULL)
	{
	        printf("  #ID%d# ",AS->item.ID);
   		printf(" -->Neighhor EID=");  
		for(wxb_k=0;wxb_k<16;wxb_k++)
		{
			if(wxb_k<15)printf("%2X:",AS->item.EID[wxb_k]);
			else printf("%2X",AS->item.EID[wxb_k]);
		}
		printf("|Number=%d",AS->item.number_pid);
		printf("|Time=%d|",AS->item.pid_change_time);
		for(wxb_k=0;wxb_k<AS->item.number_pid;wxb_k++)
		{
			printf(" pid[%d]=%d",wxb_k,AS->item.pid[wxb_k]);
		}
		printf("\n");	
		AS=AS->next;
	}
}

struct pid_info_table * head_wxb=NULL;
 
static int
read_configuration_init_pid_change_wxb()
{
        int number=sizeof(nid_ip_nid_ip_nid_array)/sizeof(struct nid_ip_nid_ip_nid);
        int i,j,k;
        struct pid_info_table * AS=NULL;
        struct nid_ip_nid_ip_nid * item=NULL;
        struct pid_info_table * last_point=NULL;
        static int counter=0;
        for(i=0;i<number;i++)
        {
                item=&nid_ip_nid_ip_nid_array[i];
                if(compare_nid(host_nid,item->eid1)==0)
                {       
                        AS=search(head_wxb,(uint8_t *)item->eid2.nid);
                        if(AS!=NULL)
                        {
                                AS->item.pid[AS->item.number_pid]=item->pid;
                                AS->item.number_pid++;                    
                        }
                        else if(head_wxb==NULL)//This is the first node in this chain!
                        {
                                link p=(struct pid_info_table *) malloc(sizeof (struct pid_info_table));
                                for(j=0;j<16;j++)
                                {
                                        p->item.EID[j]=item->eid2.nid[j];
                                }
                                head_wxb=p;
                                last_point=p;
                                p->item.number_pid=1;
                                p->item.pid_change_time=30;
                                p->item.pid[0]=item->pid;
                                p->item.ID=counter++;
                                p->next=NULL;
                        }else
                        {
                                link p=(struct pid_info_table *) malloc(sizeof (struct pid_info_table));
                                for(j=0;j<16;j++)
                                {
                                        p->item.EID[j]=item->eid2.nid[j];
                                }
                                last_point->next=p;
                                last_point=p;
                                p->item.number_pid=1;
                                p->item.pid_change_time=30;
                                p->item.pid[0]=item->pid;
                                p->item.ID=counter++;
                                p->next=NULL;         
                        }
                      
                }
        }
        print_configuration_init_pid_change(head_wxb);
        return 0;
}

//此处备注，因为众多参数还是需要通过配置文件初始化的。
//所以需要写一个读取文件初始化的程序。
//配置文件第一行写相邻域的个数，第二行写域间PID的个数，不同数目之间用","分隔，排序默认从小到大。
//第三行用来存储timer的值，"，"分割
//第四行写每个域的EID，用",和:"分离.EID是16个字节，这个是比较麻烦的，因为只能用数组来存储EID
//第五行行往后用来配置每个域的PID，用","分隔
//用来读取配置文件
static int
read_configuration_init_pid_change()
{
	uint8_t AS_number,j;
	uint8_t h=0;
	uint8_t pid_number[100];
	uint32_t timer[100];
	int i;
	uint8_t *word;
	uint8_t *EID_pointer[1600];
	uint8_t EID[1600];
	char *outer_ptr=NULL;
	char *inner_ptr=NULL;

	uint16_t SIZE = 1000;
	uint8_t buf[SIZE];
	uint8_t *buff;
	uint32_t pids[100];
	
	FILE *fp = NULL;
	fp = fopen("as_configuration.txt","r");
	//后续的维护者可以把这里的路径改为宏定义#define Conf_path "...."

	if(fp == NULL) 
	{
		printf("[From %s]Can't locate the file!\n",__func__); 
		return -1;
	}   //出错返回-1
	else
	{
		printf("[From %s]Reading file Successfully!\n",__func__); 
	}

	int line_counter=0;
	while(!feof(fp))
  	{
		char * temp_p = fgets(buf,SIZE-1,fp);
		line_counter++;
		if(line_counter==1)  
		{
			AS_number = atoi(buf);    
			//直接提取AS的个数，存储到AS_number里
			printf("[From %s] AS_number=%d\n",__func__,AS_number);
		}
		else if(line_counter==2)
		{
			//这个还是很巧妙的
			for(i=0,j=0,word=strtok(buf,",");word!=NULL;word=strtok(NULL,","),i++)  
			{
				printf("[From %s] <To AS %d>PID's number=%s\n",__func__,i,word);
				pid_number[i]=atoi(word);    
				//用来读取每个域PID的个数,提取出来后存入pid_number里，并统计所有的PID的数目。
          			j+=atoi(word);
         		}       
        		//初始化四字节的数组用来缓存一下PID。  
        	}
   		else if(line_counter==3)
   		{
			for(word=strtok(buf,","),i=0;word!=NULL;word=strtok(NULL,","),i++)
			{
				timer[i]=atoi(word);          //提取timer,
			}     
		}
  		else if (line_counter==4)
		{              
  		 //提取EID，注意EID是16个字节，处理起来比较烦，首先根据","分离出不同的EID，然后根据":"分离出以以字节为单位的EID，所有的EID最终存储到一个数组中
			buff = buf;
			while((EID_pointer[h]=strtok_r(buff,",",&outer_ptr))!=NULL)
			{
				//printf("[From %s]outer_ptr=%s\n",__func__,outer_ptr);
				buff=EID_pointer[h];
				while((EID_pointer[h]=strtok_r(buff,":",&inner_ptr))!=NULL)
				{
					h++;
					buff=NULL;
					//printf("[From %s]inner_ptr=%s\n",__func__,inner_ptr);
				}
				buff=NULL;
			}
			int jjj;
			for(jjj=0;jjj<h;jjj++)
			{
				EID[jjj]=atoi(EID_pointer[jjj]);
				printf("[From %s]EID[%d]=%d\n",__func__,jjj,EID[jjj]);
			}         
			//将EID按照字节提取到了EID的数组汇总。使用的时候比较麻烦，需要memcpy
		}
		else if(line_counter>4 &&line_counter <= (4+ AS_number) )
		{
			static uint8_t hh=0;
			for(word=strtok(buf,",");word!=NULL;word=strtok(NULL,","),hh++)
			{
				pids[hh]=atoi(word);  
				printf("[From %s]pids[%d]=%d\n",__func__,hh,pids[hh]);    
				//将PID提取到pid的数组中。注意没有按照不同域来，因为不知道域的数目，无法初始化未知个数组
			}
		}
  	}

 	fclose(fp);
	/**init PID_chains，现在有了数据，需要初始化一个链表存储这些数据。为每一个AS开创一个node，将item填入 **/
	struct pid_info *pin[AS_number];   
	//初始化AS_number个pid_info,这样写其实是有问题的。
	int k1,k2=0,k3=0;
	for(i=0;i<AS_number;i++)
	{
		pin[i] = (struct pid_info *)malloc(sizeof(struct pid_info)*2); //2 is to make program correcetly! 
		//这里利用弹性数组，很巧妙的分配了内存。避免了内存的浪费
		for(k1=0;k1<16;k1++,k2++)
		{
			pin[i]->EID[k1]=EID[k2];             
			//应该是对的，很有可能出问题K1(1-16);k2可以一直增长
		}
		pin[i]->pid_change_time=timer[i];     
		//直接将timer的值传给
		pin[i]->number_pid=pid_number[i];
		for(k1=0;k1<pid_number[i];k1++,k3++)
		{
			pin[i]->pid[k1]=pids[k3];   
		}
	}

	//建立连接关系
	link p[AS_number];
	for(i=0;i<AS_number;i++)
	{
		p[i]=make_node(pin[i]);
	
		if(i==0) 
		{
			head=p[i];
		}
		else if(i<(AS_number-1))
		{
			p[i-1]->next = p[i];
		}	
		else if(i==(AS_number-1)) 
		{
			p[i-1]->next=p[i];
			p[i]->next=NULL;
		}
	}

	//print_configuration_init_pid_change(head);	
	return 0;
}



static inline uint32_t  
 __rte_raw_cksum(const void *buf, size_t len, uint32_t sum)  
 {  
  /* workaround gcc strict-aliasing warning */  
  uintptr_t ptr = (uintptr_t)buf;  
  const uint16_t *u16 = (const uint16_t *)ptr;  
   
  while (len >= (sizeof(*u16) * 4)) {  
  sum += u16[0];  
  sum += u16[1];  
  sum += u16[2];  
  sum += u16[3];  
  len -= sizeof(*u16) * 4;  
  u16 += 4;  
  }  
  while (len >= sizeof(*u16)) {  
  sum += *u16;  
  len -= sizeof(*u16);  
  u16 += 1;  
  }  
   
  /* if length is in odd bytes */  
  if (len == 1)  
  sum += *((const uint8_t *)u16);  
   
  return sum;  
 }

static inline uint16_t  
__rte_raw_cksum_reduce(uint32_t sum)  
{  
 sum = ((sum & 0xffff0000) >> 16) + (sum & 0xffff);  
 sum = ((sum & 0xffff0000) >> 16) + (sum & 0xffff);  
 return (uint16_t)sum;  
}  
  
static inline uint16_t  
rte_raw_cksum(const void *buf, size_t len)  
{  
 uint32_t sum;  
  
 sum = __rte_raw_cksum(buf, len, 0);  
 return __rte_raw_cksum_reduce(sum);  
}  
  
static inline uint16_t  
rte_ipv4_cksum(const struct ipv4_hdr *ipv4_hdr)  
{  
 uint16_t cksum;  
 cksum = rte_raw_cksum(ipv4_hdr, sizeof(struct ipv4_hdr));  
 return (cksum == 0xffff) ? cksum : ~cksum;  
}

static int
pktgen_ctor_ether_header(struct ether_hdr * eth,struct rte_mbuf * m)
{
  struct ether_hdr * ether_header = eth;
  int i;
  uint8_t addr1[6]={00,0x16,0x31,0xfe,0xe6,0x90};
  uint8_t addr2[6]={00,0x16,0x31,0xfe,0xe6,0x91};
  for(i=0;i<6;i++)
   ether_header->d_addr.addr_bytes[i] = addr2[i];
  for(i=0;i<6;i++)
   ether_header->s_addr.addr_bytes[i] = addr1[i];
  ether_header->ether_type=0x0008;
  
  memcpy(m->pkt.data,ether_header,14);
  return sizeof(struct ether_hdr);
}

static int
pktgen_ctor_ip_header(struct ipv4_hdr * ip,struct rte_mbuf *m)
{
  struct ipv4_hdr * ip_header=ip;
  ip_header->version_ihl=0x45;
  ip_header->type_of_service=0;
  ip_header->total_length=0;
  ip_header->packet_id=0;
  ip_header->fragment_offset=0;
  ip_header->time_to_live=4;
  ip_header->next_proto_id=10;
  ip_header->hdr_checksum=0;
  ip_header->src_addr=htonl(IPv4(192,168,2,1));
  ip_header->dst_addr=htonl(IPv4(192,168,2,2));// This is a temporary IP which will be updated after!
  memcpy(m->pkt.data+sizeof(struct ether_hdr),ip_header,sizeof(struct ipv4_hdr));
  return sizeof(struct ipv4_hdr);
}


//定时器超时后用来封装一个pid_change_request发送到对面的RM
//ccp为传递进来的指针参数用来存储封装好的数据包，m为创建的mbuf,kinda应该是设计用来区分协商包的种类的|根据kinda来
//封装不同的数据|协商过程12的数据包，数据字段尾部有，其实是可以根据item的长度算出来的。item标示的是pid的条目数,timer的内存做了加1处理
//可以设计一个结构体，一次性传递很多参数进来
static int
pktgen_ctor_colorctl_pub(struct color_ctl_pub * ccp,struct rte_mbuf *m,uint8_t kind,uint16_t item)
{
 struct color_ctl_pub *pid_change=ccp;
 pid_change->ver_type = CONTROL_PACKET;
 pid_change->controltype =CONTROL_PACKET_PID;    //16标识PID协商的数据包
 if((kind == 0) || (kind == 3))
 pid_change->total_len = sizeof(struct color_ctl_pub);
 else if((kind == 1) || (kind == 2))
 pid_change->total_len = sizeof(struct color_ctl_pub) + 4*(item+1); 

 pid_change->checksum = 0;
 pid_change->sign_alg = 0;
 
        uint8_t option=0;
        option=option|OPTION_FLAG;
        switch(kind)
        {
	        case 0: 
		        option=option|OPTION_KIND0_change_request;
		        break;
	        case 1: 
		        option=option|OPTION_KIND1_a;
		        break;
		
	        case 2: 
		        option=option|OPTION_KIND2_b;
		        break;
	        case 3: 
		        option=option|OPTION_KIND3_ack;
		        break;
        }
 
 pid_change->option =option;
 	printf("[From %s]option=%2X\n",__func__,pid_change->option);
 
 
/*********
      ack1|flag1|kind2|P1|R3|
PIDCR  0     1     0   0   0
*********/

 pid_change->series_number_ack = 0;
 pid_change->series_number_send = 0;

 pid_change->item = item;
 pid_change->MTU = htons(1500);

	int i;
	for(i=0;i<16;i++)
	{ 
		pid_change->EID[i] = HOST_NID[i];
  	}
/*
*随便定一个NID标识本RM，这里设置的是1,见上面的程序
*/
  pid_change->public_key_len = htons(49);
  pid_change->resevered = 0;

 for(i=0;i<49;i++){
 pid_change->public_key[i] = 0;
 }
 for(i=0;i<55;i++){
 pid_change->signature[i] = 0;
 }
 
 memcpy(m->pkt.data+14+20,pid_change,sizeof(struct color_ctl_pub));
 
 return sizeof(struct color_ctl_pub);
}

//封装一个流量比例协商包的控制包的包头
static int
pktgen_ctor_colorctl_traffic_pub(struct color_ctl_pub * ccp, struct rte_mbuf *m)
{
 struct color_ctl_pub *pid_change = ccp;
 pid_change->ver_type = CONTROL_PACKET;
 pid_change->controltype = 0x23;
 pid_change->total_len = sizeof(struct color_ctl_pub) + 3;  //3个uint8_t，添加对应的百分比
 pid_change->checksum = 0;
 pid_change->sign_alg = 0;
 pid_change->option = 0;
 pid_change->series_number_ack = 0;
 pid_change->series_number_send = 0;
 pid_change->item = 4;
 pid_change->MTU = htons(1500);
 int i ;
 for(i=0;i<16;i++)
   {
     if (i!=15)
     pid_change->EID[i] = 0;
     else 
     pid_change->EID[i] = 1;
    }
  pid_change->public_key[i]=htons(49);
  pid_change->resevered = 0;
  for (i=0;i<49;i++){
   pid_change->public_key[i]=0;
  }
  for(i=0;i<55;i++){
  pid_change->signature[i]=0;
  }
  memcpy(m->pkt.data+14+20,pid_change,sizeof(struct color_ctl_pub));
  uint8_t traffic_proportion[3];
  for(i=0;i<3;i++)
   {
    if(i==1) {traffic_proportion[i]=10;}
    else if (i==2) {traffic_proportion[i]=20;}
    else if (i==3) {traffic_proportion[i]=30;}
   }
  memcpy(m->pkt.data+14+20+sizeof(struct color_ctl_pub),traffic_proportion,3);
  return pid_change->total_len;
}

//封装一个ether_header,ip_header,color控制包公共部分的消息
static int 
pkt_colorctl_setup(unsigned lcore_id,struct rte_mbuf *m,uint8_t kind,uint8_t item)
{
    
  int ret=0;
    
  struct ether_hdr ethh;
  ret=pktgen_ctor_ether_header(&ethh,m);
  

  struct ipv4_hdr iph;
  ret+=pktgen_ctor_ip_header(&iph,m);

  struct color_ctl_pub ctl_pub;
  //pktgen_ctor_colorctl_pub(struct color_ctl_pub * ccp,struct mbuf *m,uint8_t kind,uint16_t item)
  ret+=pktgen_ctor_colorctl_pub(&ctl_pub,m,kind,item);  //pid_change_request没有PID的部分,need to add!

  return ret;  //返回值为数据包大小
}
/*
在main_loop里面加的函数有点多，首先需要使用一个lcore加定时器用来实现周期性的发送协商消息。其次，当每个端口的接收队列中收到协商数据包后需要执行相应的处理流程。
因此需要添加的主要有两个部分，一部分是周期性的协商数据包，一部分是收到处理消息后判断，然后处理。
*/

//封装一个ether_header,ip_header,color控制包，协商流量比例的包头
static int
pkt_colorctl_traffic_setup(unsigned lcore_id,struct rte_mbuf *m)
{
 int ret = 0;
 struct ether_hdr ethh;
 ret=pktgen_ctor_ether_header(&ethh,m);
 struct ipv4_hdr iph;
 ret+=pktgen_ctor_ip_header(&iph,m);
 struct color_ctl_pub ctl_pub;
 ret+=pktgen_ctor_colorctl_traffic_pub(&ctl_pub,m);
 return ret;
}

//0-ip  1-pid
static inline uint32_t find_ip_or_pid(link AS,int type)
{
        int i;
        nid_t nid_temp;
        int number=sizeof(nid_ip_nid_ip_nid_array)/sizeof(struct nid_ip_nid_ip_nid);
        struct nid_ip_nid_ip_nid * item=NULL;
        for(i=0;i<16;i++)
        {     
                 nid_temp.nid[i]=AS->item.EID[i];
        }

        for(i=0;i<number;i++)
        {
                item=&nid_ip_nid_ip_nid_array[i];
                if(compare_nid(host_nid,item->eid1)==0&&compare_nid(nid_temp,item->eid2)==0)
                {
                   if(type==0)    return rte_cpu_to_be_32(item->ip1);
                   else if(type==1) return rte_cpu_to_be_32(item->pid);
                }
        }
}

static inline void
ADD_PID_and_IP(struct rte_mbuf *m, uint32_t pid,uint32_t ip)
{
        struct color_ctl_pub * ctl_hdr=(struct color_ctl_pub *)(rte_pktmbuf_mtod(m, unsigned char *) +sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr));     
        ctl_hdr->total_len=m->pkt.data_len-sizeof(struct ipv4_hdr)-sizeof(struct ether_hdr);
        struct ipv4_hdr * ipv4_hdr = (struct ipv4_hdr *)(rte_pktmbuf_mtod(m, unsigned char *) + sizeof(struct ether_hdr)); 
        ipv4_hdr->dst_addr=ip;  
        uint32_t * pid_add=(uint32_t *)( (uint8_t *)ctl_hdr+ctl_hdr->total_len);
        *(pid_add)=pid;

	ctl_hdr->total_len=rte_cpu_to_be_16(  ctl_hdr->total_len+sizeof(pid) );

	m->pkt.data_len+=sizeof(pid);
        m->pkt.pkt_len+=sizeof(pid);
        return;
}

//可以完善成根据参数发送想要的数据包
static uint8_t
send_pid_change_request(unsigned lcore_id,uint8_t port_id_xs,uint8_t queue_id_xs,uint32_t pid,uint32_t ip)
{
        int socketid;
        int ret;
        if (numa_on) socketid = rte_lcore_to_socket_id(lcore_id);
        else socketid = 0;

        struct rte_mbuf *m = rte_pktmbuf_alloc(pktmbuf_pool[socketid]);
        if (unlikely(m==NULL)) printf("Alloc mbuf error!\n");
        //else printf("[From %s]Creating mbuf success!\n",__func__);

        ret = pkt_colorctl_setup(lcore_id,m,0,0);

        m->pkt.nb_segs=1;
        m->pkt.next=NULL;
        m->pkt.pkt_len=ret;
        m->pkt.data_len=ret;

        printf("[From %s]pkt.data_len=%d\n",__func__,m->pkt.data_len+4);
        ADD_PID_and_IP(m,pid,ip); 
         ret = rte_eth_tx_burst(port_id_xs,queue_id_xs,&m,1);   //指定端口发送数据包,现在设计的是从port0发送。
         if (unlikely(ret < 1)) 
         {
            printf("\n[From %s]Send Error!Please check your configure.\n",__func__);
            rte_pktmbuf_free(m);
        }
        rte_pktmbuf_free(m);
       /*
        FILE *fp = NULL;
        fp = fopen("pumpking.txt","a+");
        if(fp!=NULL)
        {
	        printf("[From %s]OPEN FILE successfully!\n",__func__);
	        uint64_t cur_tsc = rte_rdtsc();
	        fprintf(fp,"\n%lu ",cur_tsc);
        }
        else
        {
	        printf("[From %s]NOT FOUND THE FILE!\n",__func__);
        }
        fclose(fp);
        */
 return ret; //返回值判断状态
}

//发送流量协商消息
static uint8_t
send_link_traffic_proportion(unsigned lcore_id,uint8_t port_id_xs,uint8_t queue_id_xs,uint32_t pid,uint32_t ip)
{
   int socketid;
   int ret;
   if (numa_on) socketid = rte_lcore_to_socket_id(lcore_id);
   else socketid = 0;
   
   struct rte_mbuf *m = rte_pktmbuf_alloc(pktmbuf_pool[socketid]);
   if (unlikely(m==NULL))printf("Alloc mbuf error!\n");
   //else printf("[From%s]Creating mbuf success!\n",__func__);
   
   ret = pkt_colorctl_traffic_setup(lcore_id,m);
   
   m->pkt.nb_segs=1;
   m->pkt.next=NULL;
   m->pkt.pkt_len=ret;
   m->pkt.data_len=ret;

 //  printf("[From %s]pkt.data_len=%d\n",__func__,m->pkt.data_len+4);
   ADD_PID_and_IP(m,pid,ip);
   ret = rte_eth_tx_burst(port_id_xs,queue_id_xs,&m,1);
   
   if (unlikely(ret<1))
    {
      printf("\n[From%s]Send Error！Please check your configure.\n",__func__);
      rte_pktmbuf_free(m);
    }
    rte_pktmbuf_free(m);
    /*
    FILE *fp =NULL;
    fp= fopen("Time_of_TE_Negotiation_Packets.txt","a+");
    if (fp!=NULL)
     {
       printf("[From %s]OPEN FILE successfully!\n",__func__);
       uint64_t cur_tsc = rte_rdtsc();
       fprintf(fp,"%lu\n",cur_tsc);
      }
     else {
      printf("[From %s]Not found the file!\n",__func__);
     }
     fclose(fp);
    */
}
//发送pid_a消息
static uint8_t
send_pid_a(unsigned lcore_id,uint32_t *pids,uint32_t *timer,uint16_t pid_number,uint8_t port_id_xs,uint8_t queue_id_xs,uint32_t pid,uint32_t ip)
{
  int socketid;
  int ret;
  if (numa_on) socketid = rte_lcore_to_socket_id(lcore_id);
  else socketid = 0;
   
  struct rte_mbuf *m = rte_pktmbuf_alloc(pktmbuf_pool[socketid]);
  if (unlikely(m==NULL)) printf("Alloc mubf error!\n");
  else printf("[From %s]creating mbuf success !\n",__func__);
  
  /**pkt_colorctl_setup(unsigned lcore_id,struct mbuf *m,uint8_t kind, uint8_t item)*/
  ret = pkt_colorctl_setup(lcore_id,m,1,pid_number);
  if(ret!=0)
    {
     /*ret不为0，说明封装数据包成功*/
     memcpy(m->pkt.data+sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr)+sizeof(struct color_ctl_pub),timer,sizeof(uint32_t));   
     //封装Timer
     memcpy(m->pkt.data+sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr)+sizeof(struct color_ctl_pub)+sizeof(uint32_t),pids,pid_number*sizeof(uint32_t));  //封装pids
     ret = ret + sizeof(uint32_t) + pid_number*sizeof(uint32_t);
     }

   m->pkt.nb_segs=1;
   m->pkt.next=NULL;
   m->pkt.pkt_len=ret;
   m->pkt.data_len=ret;

  ADD_PID_and_IP(m,pid,ip);
  printf("[From %s]pktgen pid_a message, length is %d\n",__func__,ret+4);
 
  ret = 0;
  ret = rte_eth_tx_burst(port_id_xs,queue_id_xs,&m,1); 

  if (unlikely(ret<1))
  {
    printf("Send Error!Please check your configure.\n");
  }
    rte_pktmbuf_free(m);
  return ret;
}
//发送pid_b消息
static uint8_t
send_pid_b(unsigned lcore_id,uint32_t *pids,uint32_t *timer,uint16_t pid_number,uint8_t port_id_xs,uint8_t queue_id_xs,uint32_t pid,uint32_t ip)
{
  int socketid;
  int ret;
  if (numa_on) socketid = rte_lcore_to_socket_id(lcore_id);
  else socketid = 0;
   
  struct rte_mbuf *m = rte_pktmbuf_alloc(pktmbuf_pool[socketid]);
  if (unlikely(m==NULL)) printf("Alloc mubf error!\n");
  else printf("[From %s]creating mbuf success !\n",__func__);
  
  /**pkt_colorctl_setup(unsigned lcore_id,struct mbuf *m,uint8_t kind, uint8_t item)*/
  ret = pkt_colorctl_setup(lcore_id,m,2,pid_number);
  if(ret!=0)
    {
     /*ret不为0，说明封装数据包成功*/
     memcpy(m->pkt.data+sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr)+sizeof(struct color_ctl_pub),timer,sizeof(uint32_t));   
     //封装Timer
     memcpy(m->pkt.data+sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr)+sizeof(struct color_ctl_pub)+sizeof(uint32_t),pids,pid_number*sizeof(uint32_t));  //封装pids
     ret = ret + sizeof(uint32_t) + pid_number*sizeof(uint32_t);
     }

   m->pkt.nb_segs=1;
   m->pkt.next=NULL;
   m->pkt.pkt_len=ret;
   m->pkt.data_len=ret;

  printf("[From %s]pktgen pid_b message, length is %d\n",__func__,ret+4);

  ADD_PID_and_IP(m,pid,ip);

  ret = 0;
  ret = rte_eth_tx_burst(port_id_xs,queue_id_xs,&m,1);  

  if (unlikely(ret<1))
  {
    printf("Send Error!Please check your configure.\n");
   }
    rte_pktmbuf_free(m);
  return ret;
}

static uint8_t
send_ack(unsigned lcore_id,uint8_t port_id_xs,uint8_t queue_id_xs,uint32_t pid,uint32_t ip)
{
 int socketid;
 int ret;
 if (numa_on) socketid = rte_lcore_to_socket_id(lcore_id);
 else socketid = 0;

 struct rte_mbuf *m = rte_pktmbuf_alloc(pktmbuf_pool[socketid]);
 if (unlikely(m==NULL)) printf("Alloc mbuf error!\n");
 else printf("[From %s]creating mbuf success!\n",__func__);
 
 ret = pkt_colorctl_setup(lcore_id,m,3,0);
 m->pkt.nb_segs=1;
 m->pkt.next=NULL;
 m->pkt.pkt_len = ret;
 m->pkt.data_len = ret;
 ADD_PID_and_IP(m,pid,ip);
 ret = rte_eth_tx_burst(port_id_xs,queue_id_xs,&m,1); 
 if(unlikely(ret<1))
  {
   printf("Send Error!Please check your configure.\n");
  }
  rte_pktmbuf_free(m);
  return ret;
}

int counter=0;

static inline void update_nid_ip_nid_ip_nid(link AS)
{
        int i;
        int counter=0;
        nid_t nid_temp;
        uint8_t number=sizeof(nid_ip_nid_ip_nid_array)/sizeof(struct nid_ip_nid_ip_nid);
        struct nid_ip_nid_ip_nid * item=NULL;
        for(i=0;i<16;i++)
        {
                nid_temp.nid[i]=AS->item.EID[i];
        }
        for(i=0;i<number;i++)
        {
                item=&nid_ip_nid_ip_nid_array[i];
                if(compare_nid(item->eid1,host_nid)==0 &&compare_nid(item->eid2,nid_temp)==0)
                {
                        item->pid=AS->item.pid[counter++];
                }
        }
}

struct announce_item
{
        uint32_t pid_original;
        uint32_t pid_current;
        uint32_t ip_inside;
        uint32_t ip_outside;
        uint32_t time_to_live;
}__attribute__((__packed__));

//Tell border router the PID informations.
static uint8_t
send_ip_nid_ip(unsigned lcore_id,uint8_t port_id_xs,uint8_t queue_id_xs,struct announce_item *item,uint32_t ip)
{
  int socketid;
  if (numa_on) socketid = rte_lcore_to_socket_id(lcore_id);
  else socketid = 0;
   
  struct rte_mbuf *m = rte_pktmbuf_alloc(pktmbuf_pool[socketid]);
  if (unlikely(m==NULL)) printf("Alloc mubf error!\n");



  int ret=0;  
  struct ether_hdr ethh;
  ret=pktgen_ctor_ether_header(&ethh,m);
  struct ipv4_hdr iph;
  ret+=pktgen_ctor_ip_header(&iph,m);
  struct color_ctl_pub ctl_pub;
  ctl_pub.ver_type = CONTROL_PACKET;
  ctl_pub.controltype =CONTROL_PACKET_PID_ANNOUNCE;
  ctl_pub.item=rte_cpu_to_be_16( 1 );

  int i;
  for(i=0;i<16;i++)
  { 
    ctl_pub.EID[i] = HOST_NID[i];
  }


  memcpy(m->pkt.data+sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr),&ctl_pub,sizeof(struct color_ctl_pub));
  ret+=sizeof(struct color_ctl_pub);
  memcpy(m->pkt.data+sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr)+sizeof(struct color_ctl_pub),item,sizeof(struct announce_item));
  ret+=sizeof(struct announce_item);

   m->pkt.nb_segs=1;
   m->pkt.next=NULL;
   m->pkt.pkt_len=ret;
   m->pkt.data_len=ret;

  ADD_PID_and_IP(m,0,ip);     //This function is more than what we need because the IP should not be added.
  
  printf("[From %s]pktgen ip_nid_ip message, length is %d\n",__func__,ret+4);
 
  ret = 0;
  ret = rte_eth_tx_burst(port_id_xs,queue_id_xs,&m,1); 

  if (unlikely(ret<1))
  {
    printf("Send Error!Please check your configure.\n");
  }
    rte_pktmbuf_free(m);
  return ret;
}

static inline void announce_ip_nid_ip(link AS)
{
        struct announce_item announce;
        nid_t nid_temp;
        int number_pid=AS->item.number_pid;
        uint8_t number=sizeof(nid_ip_nid_ip_nid_array)/sizeof(struct nid_ip_nid_ip_nid);
        struct nid_ip_nid_ip_nid * item=NULL;
        struct nid_ip_nid_ip_nid * item_j=NULL;
        int i=0,counter=0,j=0;
        for(i=0;i<16;i++)
        {     
                 nid_temp.nid[i]=AS->item.EID[i];
        }
        for(i=0;i<number;i++)
        {
                item=&nid_ip_nid_ip_nid_array[i];
                if(compare_nid(item->eid1,host_nid)==0 &&compare_nid(item->eid2,nid_temp)==0 && AS->item.pid[counter]==item->pid)
                {
                        counter++;
                        printf("[From %s] announce PID=%d\n",__func__,item->pid);
                        
                        announce.time_to_live=  rte_cpu_to_be_32( pid_change_period*2 );
                        announce.ip_inside=     rte_cpu_to_be_32( item->ip1 );
                        announce.pid_current=   rte_cpu_to_be_32( item->pid );
                        announce.ip_outside=    rte_cpu_to_be_32( item->ip2 );
                        
                        for(j=0;j<number;j++)
                        {
                                item_j=&nid_ip_nid_ip_nid_array[j];
                                if(compare_nid(item_j->eid1,host_nid)==0)
                                {
                                        send_ip_nid_ip(5,0,0,&announce,rte_cpu_to_be_32(item_j->ip1) );        
                                }
                        }
                        
                        if(counter>=number_pid)  //The index is form the zero!!
                                break;  //The process of the searching is over!pp
                }
        }    
}

static struct rte_timer timer0;
/* timer0 callback */
static void
timer0_cb(__attribute__((unused)) struct rte_timer *tim,
	  __attribute__((unused)) void *arg)
{
          link AS=head_wxb;
          int i,ii;
          nid_t nid_temp;
          int number=sizeof(nid_ip_nid_ip_nid_array)/sizeof(struct nid_ip_nid_ip_nid);
          struct nid_ip_nid_ip_nid * item=NULL;
          uint8_t * wxb_ip=NULL;
          uint32_t temp_ip=0;
          static int negotiation_cycle=0;
          negotiation_cycle+=1;
          
          /***negotiation traffic proportion***/
          //send_link_traffic_proportion(5,2,0,rte_cpu_to_be_32(PID('P','I','D','2')),rte_cpu_to_be_32(IPv4(192,168,3,2)));

          while(AS!=NULL)
          {
                for(ii=0;ii<16;ii++)
                {     
                         nid_temp.nid[ii]=AS->item.EID[ii];
                }
                
                for(i=0;i<number;i++)
                {
                        item=&nid_ip_nid_ip_nid_array[i];
                        if(compare_nid(host_nid,item->eid1)==0&&compare_nid(nid_temp,item->eid2)==0 && compare_nid(item->eid1,item->eid2)<=0)
                        {
                                /*printf("[From %s]\n",__func__);
                                for(ii=0;ii<16;ii++)
                                {       if(ii==0) printf("    To #%d# %d",AS->item.ID,AS->item.EID[ii]);
                                        else if(ii==15) printf(":%d\n",AS->item.EID[ii]);
                                        else printf(":%d",AS->item.EID[ii]);
                                }
                                printf("    The PID negotiation Sponsor. [Path] pid=%d",item->pid);
                                temp_ip=rte_cpu_to_be_32( item->ip1);
                                wxb_ip=(uint8_t *)(&temp_ip);
	                        printf("   IP=%d.",*(wxb_ip++) );
                                printf("%d.",*(wxb_ip++) );
                                printf("%d.",*(wxb_ip++) );
                                printf("%d \n",*(wxb_ip++) );
                                */
                                //send_pid_change_request(5,0,0,rte_cpu_to_be_32(item->pid),rte_cpu_to_be_32(item->ip1));
                                //counter+=1;
                                //printf("[From %s]Alread send pid_change_request! negotiation_cycle=%d  No.%d \n\n",__func__,negotiation_cycle,counter);
                                break;
                        }
                }
                
                AS=AS->next;
          }
}

//定义全局变量，traffic_proportion
int traffic_proportion[4];

/* main processing loop */
static int
main_loop(__attribute__((unused)) void *dummy)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	unsigned lcore_id;
	uint64_t prev_tsc, diff_tsc, cur_tsc;
	int i, j, nb_rx;
	uint8_t portid, queueid;
	struct lcore_conf *qconf;
	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) /
		US_PER_S * BURST_TX_DRAIN_US;

#if ((APP_LOOKUP_METHOD == APP_LOOKUP_LPM) && \
	(ENABLE_MULTI_BUFFER_OPTIMIZE == 1))
	int32_t k;
	uint16_t dst_port[MAX_PKT_BURST];
	__m128i dip[MAX_PKT_BURST / FWDSTEP];
	uint32_t flag[MAX_PKT_BURST / FWDSTEP];
#endif

	prev_tsc = 0;

	lcore_id = rte_lcore_id();
	qconf = &lcore_conf[lcore_id];

	if (qconf->n_rx_queue == 0) {
		RTE_LOG(INFO, L3FWD, "lcore %u has nothing to do\n", lcore_id);
		return 0;
	}

	RTE_LOG(INFO, L3FWD, "entering main loop on lcore %u\n", lcore_id);

	for (i = 0; i < qconf->n_rx_queue; i++) {

		portid = qconf->rx_queue_list[i].port_id;
		queueid = qconf->rx_queue_list[i].queue_id;
		RTE_LOG(INFO, L3FWD, " -- lcoreid=%u portid=%hhu rxqueueid=%hhu\n", lcore_id,
			portid, queueid);
	}
        /****定时发送消息*/
        uint64_t hz;
        int PID_run_lcore=5;
        if(lcore_id==PID_run_lcore)
        {
                //signal(SIGALRM,signal_handler);
	        rte_timer_subsystem_init();
                rte_timer_init(&timer0);
                hz = rte_get_timer_hz();
	        lcore_id = rte_lcore_id();
                rte_timer_reset(&timer0, 100*hz, PERIODICAL, lcore_id, timer0_cb, NULL);
        }
        uint64_t cur_tsc2=0;
        uint64_t prev_tsc2=0;
        uint64_t diff_tsc2=0;
/*****************/

       while(1){
		
		cur_tsc = rte_rdtsc();

		/*
		 * TX burst queue drain
		 */
		diff_tsc = cur_tsc - prev_tsc;
		if (unlikely(diff_tsc > drain_tsc)) {

			/*
			 * This could be optimized (use queueid instead of
			 * portid), but it is not called so often
			 */
			for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
				if (qconf->tx_mbufs[portid].len == 0)
					continue;
				send_burst(qconf,
					qconf->tx_mbufs[portid].len,
					portid);
				qconf->tx_mbufs[portid].len = 0;
			}

			prev_tsc = cur_tsc;
		}
		
		if(lcore_id==PID_run_lcore)
                {
		
		        hz = rte_get_timer_hz();
                        cur_tsc2 = rte_rdtsc();
		        diff_tsc2 = cur_tsc2 - prev_tsc2;
		        if (diff_tsc2 > hz/1000) {
			        rte_timer_manage();
			        prev_tsc2 = cur_tsc2;
		        }       
                }

		/*
		 * Read packet from RX queues
		 */
		for (i = 0; i < qconf->n_rx_queue; ++i)
                {
			portid = qconf->rx_queue_list[i].port_id;
			queueid = qconf->rx_queue_list[i].queue_id;
			nb_rx = rte_eth_rx_burst(portid, queueid, pkts_burst,MAX_PKT_BURST);
 /**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/
			static uint32_t wxb_packets_counter=0;	
			if(nb_rx>0)
			{
			     wxb_packets_counter=wxb_packets_counter+nb_rx;
			     //printf("\n[%s]:The DPDK receives %d packets once and %d paclets totally!! \n",__func__,nb_rx,wxb_packets_counter);
			}
			else
			{
			     continue;
			}
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************END************/	
#if (ENABLE_MULTI_BUFFER_OPTIMIZE == 1)
#if (APP_LOOKUP_METHOD == APP_LOOKUP_EXACT_MATCH)
			{
				/*
				 * Send nb_rx - nb_rx%4 packets
				 * in groups of 4.
				 */
				/*int32_t n = RTE_ALIGN_FLOOR(nb_rx, 4);
				for (j = 0; j < n ; j+=4) 
                                {
					*uint32_t ol_flag = pkts_burst[j]->ol_flags
							& pkts_burst[j+1]->ol_flags
							& pkts_burst[j+2]->ol_flags
							& pkts_burst[j+3]->ol_flags;
					if (ol_flag & PKT_RX_IPV4_HDR ) {
						simple_ipv4_fwd_4pkts(&pkts_burst[j],
									portid, qconf);
					} else if (ol_flag & PKT_RX_IPV6_HDR) {
						simple_ipv6_fwd_4pkts(&pkts_burst[j],
									portid, qconf);
					} else {
                                        
                                              
						l3fwd_simple_forward(pkts_burst[j],
									portid, qconf);
						l3fwd_simple_forward(pkts_burst[j+1],
									portid, qconf);
						l3fwd_simple_forward(pkts_burst[j+2],
									portid, qconf);
						l3fwd_simple_forward(pkts_burst[j+3],
									portid, qconf);
					}
				}*/
			for (j=0 ; j < nb_rx ; j++) 
			{
                         /***********************收到了数据包*********************************/
                         /***********************处理协商消息*********************************/
                         unsigned lcore_id_xs;
                         lcore_id_xs = rte_lcore_id();
                         uint8_t port_id_xs,queue_id_xs;
                         port_id_xs = portid;
                         queue_id_xs = queueid;

                         struct ether_hdr *eth_hdr;
                         struct color_ctl_pub *color_public_header;   
                         eth_hdr = rte_pktmbuf_mtod(pkts_burst[j],struct ether_hdr *);   //这里可能没m,而是burst
  			 CoLoR_get_t * get_hdr = (CoLoR_get_t *)(rte_pktmbuf_mtod(pkts_burst[j], unsigned char *) +sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr));	
                         /*******************trap*********************/
                         struct  trap_get_header *trap_head = (struct trap_get_header *)(rte_pktmbuf_mtod(pkts_burst[j], unsigned char *));
			 if(trap_head->version_type==0xa0)
                         {
                            get_trap_message(pkts_burst[j],portid);
                         }
                         else if(trap_head->version_type==0xa1)
                         {
                            data_trap_message(pkts_burst[j]);
                          }
                         struct ipv4_hdr *ipv4_hdr;
                         if(pkts_burst[j]->ol_flags & PKT_RX_IPV4_HDR)
                         {
                          ipv4_hdr = rte_pktmbuf_mtod(pkts_burst[j],struct ipv4_hdr *)+sizeof(struct ether_hdr);
                 //需要添加判断是控制包还是get包,如果是控制包，还要进一步判断是协商pid包还是注册包，然后做相印的处理,controltype 9表示pid_change_request
		 uint8_t ver_type;
		 uint8_t EID_tiqu[16];
		 uint32_t pid_huancun[100];
		 uint32_t time_huancun;
		 uint32_t time_new;
		 uint32_t pid_new[100];
		 uint32_t time_huancun_pid_a;
		 uint32_t pid_huancun_pid_a[10][100]; //There can be 10 neighbors and 100 pids each!
		 link AS;
		 int sys=0;
                 ver_type=get_hdr->version_type;
	        // printf("[From %s]ver_type=%2X\n",__func__,ver_type);
	         uint32_t pid_path,ip_path;
                 if(ver_type==GET_PACKET)
                 {
                    //printf("[From %s]Tt's a get packet!\n",__func__);
		    //l3fwd_simple_forward(pkts_burst[j],portid, qconf);
                   /*
                    FILE *fp;
                    char filename[]="/home/DOF_RM";
                    fp=fopen(filename,"a+");
                    struct timeval in_time;
                    gettimeofday(&in_time,NULL);
                    fprintf(fp,"time:%d.%d\n",in_time.tv_sec,in_time.tv_usec);
                    fclose(fp);
                   */
                    get_packets_forward(pkts_burst[j],portid,qconf);
                 }
                 else if(ver_type==CONTROL_PACKET)
                 {
                    printf("[From %s]It's a control packet!\n",__func__);
                    color_public_header =(struct color_ctl_pub *)(rte_pktmbuf_mtod(pkts_burst[j],unsigned char *)+sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr));
                    memcpy(EID_tiqu,color_public_header->EID,16);  //提取一下EID
                    
                    printf("[From %s]Source'EID=",__func__);               
                    int wxb_k=0;
                    for(wxb_k=0;wxb_k<16;wxb_k++)
                    {
                    	if(wxb_k<15)printf("%2X:",EID_tiqu[wxb_k]);
                    	else printf("%2X",EID_tiqu[wxb_k]);
                    }
                    printf("\n");
                    
                    if(color_public_header->controltype==CONTROL_PACKET_PID)
                    {
                         printf("[From %s]PID_optionKind=%2X\n",__func__,color_public_header->option&OPTION_KIND_MASK);
                         if((color_public_header->option&OPTION_KIND_MASK)==OPTION_KIND0_change_request)
                         {

                             printf("[From %s]It's a pid_change_request!\n",__func__);
                            /***如果收到pid_change_request，需要提取EID，然后判断是哪一个域发来的，根据EID查找链表，提取PID列表，然后根据当前的PID和PID生成算法生成下一个时间段的PID，并且依据设立Timer的算法，生成一个Timer的值。然后将新的PID和Timer值封装入pid_a消息，修改IP地址和mac地址，修改校验和，将数据包发送出去。由于目前PID的生成算法还没有详细的思路，所以只能按照不严谨的方式来处理。***/
                             AS = search(head_wxb,(uint8_t *)EID_tiqu); //从head开始搜寻链表,返回的是查到的节点的指针，类型是link，就是pid_info_table *


                             //AS->pid就是目前所使用的PID序列
                             if(AS==NULL) 
                             {
                             	printf("[%s]opps, cant find this AS in our AS-pid chain!\n",__func__);
                             	//exit(0);
                             }
                             else 
                             {
                                printf("[From %s]Conecting EID=",__func__);      
                                for(wxb_k=0;wxb_k<16;wxb_k++)
                                {
                                 if(wxb_k<15)printf("%2X:",EID_tiqu[wxb_k]);
                                        else printf("%2X",AS->item.EID[wxb_k]);
                                }
                                printf("     Number_pid=%d",AS->item.number_pid);
                                for(wxb_k=0;wxb_k<AS->item.number_pid;wxb_k++)
                                {
                                        printf(" pid[%d]%d",wxb_k,AS->item.pid[wxb_k]);
                                }
                                printf("\n");
                             	printf("[%s]Find this AS in our AS-pid chain!\n",__func__);
                                int i,number;
                                uint32_t pid_fa[AS->item.number_pid];

                                uint32_t time_fa;

                                srand((unsigned)time(NULL));   //生成随机数
                                number=(rand()%101); //随机数范围1-100  
                                //读取PIDs
                                for(i=0;i<AS->item.number_pid;i++)
                                {
                                        pid_huancun[i] = AS->item.pid[i];//当前PID
                                        printf("[From %s]pid_huancun[%d]=%d   ",__func__,i,pid_huancun[i]);
                                        pid_new[i] =( pid_huancun[i]+number)%100;  //生成算法就是加一个随机数,之后等理论研究到位后再修改。
                                        pid_fa[i] = htonl(pid_new[i]);
                                        printf("pid_new[%d]=%d\n",i,pid_new[i]);
                                }
                                time_huancun = AS->item.pid_change_time;
                                time_new = (AS->item.pid_change_time + number)%100;
                                printf("[From %s]time_huancun=%d  time_new=%d \n",__func__,time_huancun,time_new);

                                time_fa = htonl(time_new);  
                                //先把当前的PID读取出来，生成新的pid,存到pid_huancun里
                                //封装并发送pid_a消息
                                //函数参数：send_pid_a(unsigned lcored_id,uint32_t *pids,uint32_t *timer,uint16_t pid_number,uint8_t portid, uint8_t queueid)                            
                                pid_path=find_ip_or_pid(AS,1);
                                ip_path=find_ip_or_pid(AS,0);
                                send_pid_a(lcore_id_xs,pid_fa,&time_fa,AS->item.number_pid,port_id_xs,queue_id_xs,pid_path,ip_path);  
                             }
                             
     
                        }
                      else if((color_public_header->option&OPTION_KIND_MASK)==OPTION_KIND1_a)
                     {
	    
                          printf("[From %s]it is a pid_a message!\n",__func__);
                         /***如果收到pid_a消息，提取EID，判断是哪一个域发来的，读取消息中封装的PID序列和Timer的值***/ 
                          AS = search(head_wxb,(uint8_t *)EID_tiqu); 
                          uint32_t pid_fa[AS->item.number_pid];
                          uint32_t time_fa;
                          if(AS==NULL) 
                          {
                          	printf("[From %s]opps,cant find this AS in our AS-pid chain!\n",__func__);
                          	//exit(0);
                          }
                          else
                          {

                                  printf("[From %s]find this AS in our AS-pid chain!\n",__func__);
                                  int i, number;
                                  srand((unsigned)time(NULL));
                                  number=(rand()%101)+1;
                                  int ID=AS->item.ID;
                                  memcpy(&time_huancun_pid_a,(uint8_t *)color_public_header+sizeof(struct color_ctl_pub),4); 
                                  time_huancun_pid_a = ntohl(time_huancun_pid_a);
                                  printf("[From %s]time_huancun_pid_a=%d\n",__func__,time_huancun_pid_a);
                                  memcpy(pid_huancun_pid_a[ID],(uint8_t *)color_public_header+sizeof(struct color_ctl_pub)+4,AS->item.number_pid*4);  
    
                                for(i=0;i<AS->item.number_pid;i++)
		                {
			                pid_huancun_pid_a[ID][i] = ntohl(pid_huancun_pid_a[ID][i]); 
        
        	                }                 
                         
                                uint8_t number2=sizeof(nid_ip_nid_ip_nid_array)/sizeof(struct nid_ip_nid_ip_nid);
                                struct nid_ip_nid_ip_nid * item=NULL;
                                int different=1,jj;
                                while(different)
                                {
  
                                        different=0;
                                        for(i=0;i<number2;i++)
			                {
			                        item=&nid_ip_nid_ip_nid_array[i];
                                                for(jj=0;jj<AS->item.number_pid;jj++)
			                        { 
			                               if(pid_huancun_pid_a[ID][jj] - item->pid==0)
			                               {
			                                     //sys=system("clear");
			                                     printf("[From %s]!!!%d==%d   random_number=%d\n"  ,__func__,pid_huancun_pid_a[ID][j],item->pid,number);
			                                     different=1;   
			                               } 
			                        }  
			                }   
	 
			                if(different==1)
		                        {
		                              for(jj=0;jj<AS->item.number_pid;jj++)
		                              {
		                                      pid_huancun_pid_a[ID][jj]=pid_huancun_pid_a[ID][jj]+number;
		                              }   
		                        }  
                                }            
                                for(i=0;i<AS->item.number_pid;i++)
		                {
			                 pid_fa[i] = ntohl(pid_huancun_pid_a[ID][i]); 
			                 printf("[From %s]pid_huancun_pid_a[ID][%d]=%d\n",__func__,i,pid_huancun_pid_a[ID][i]);
        
        	                }   			        
			        time_fa = htonl(time_huancun_pid_a); 
			        //send_pid_b(unsigned lcore_id,uint32_t *pids,uint32_t *timer,uint16_t pid_number,uint8_t port_id_xs,uint8_t queue_id_xs)
			        pid_path=find_ip_or_pid(AS,1);
                                ip_path=find_ip_or_pid(AS,0);
			        send_pid_b(lcore_id_xs,pid_fa,&time_fa,AS->item.number_pid,port_id_xs,queue_id_xs,pid_path,ip_path);

                          } 
			
                        }
			else if((color_public_header->option&OPTION_KIND_MASK)==OPTION_KIND2_b)
                        {
                            printf("[From %s]it is a pid_b message!\n",__func__);
                         /***如果收到pid_a消息，提取EID，判断是哪一个域发来的，读取消息中封装的PID序列和Timer的值***/ 
                          AS = search(head_wxb,(uint8_t *)EID_tiqu);
                          if(AS==NULL) 
                          {
                          	printf("[From %s]opps,cant find this AS in our AS-pid chain!\n",__func__);
                          	//exit(0);
                          }
                          else 
                          {
                                   printf("[From %s]find this AS in our AS-pid chain!\n",__func__);
                                  int i, number;
                                  srand((unsigned)time(NULL));
                                  number=(rand()%101)+1;
                                  uint32_t PID_tiqu[AS->item.number_pid];
			          uint32_t time_tiqu;
                                  memcpy(&time_tiqu,(uint8_t *)color_public_header+sizeof(struct color_ctl_pub),4); 
                                  time_tiqu = ntohl(time_tiqu);
                                  printf("[From %s]time_tiqu=%d\n",__func__,time_tiqu);
                                  memcpy(PID_tiqu,(uint8_t *)color_public_header+sizeof(struct color_ctl_pub)+4,AS->item.number_pid*4);  
                                  
                                  uint8_t ACK_YES=1;
                                  int jj;
                                  link AASS;
                                  AASS=head_wxb;
                                  
                                for(i=0;i<AS->item.number_pid;i++)
		                {
			                PID_tiqu[i] = ntohl(PID_tiqu[i]); 
        
        	                } 
                                        while(AASS!=NULL)
                                        {
                                                for(i=0;i< AASS->item.number_pid;i++)
				                {

					                for(jj=0;jj< AS->item.number_pid;jj++)
				                        {
					                        printf("[From %s]PID_tiqu[%d]=%d %d\n",__func__,i,PID_tiqu[jj],AASS->item.pid[i]);
					                        if(PID_tiqu[jj]==AASS->item.pid[i])
					                        {
						                        ACK_YES=0;
					                        }
				                       }
				                } 
                                                AASS=AASS->next;
                                        }
	
				        if(ACK_YES)//Update the information!!
				        {
				                pid_path=find_ip_or_pid(AS,1);
                                                ip_path=find_ip_or_pid(AS,0);
					        send_ack(lcore_id_xs,port_id_xs,queue_id_xs,pid_path,ip_path);
		                         	AS->item.pid_change_time = time_tiqu;
		                          	for(i=0;i<AS->item.number_pid;i++)
		                         	{
		                          		AS->item.pid[i] = PID_tiqu[i];
		                          	}
		                          	update_nid_ip_nid_ip_nid(AS);
		                          	print_configuration_init_pid_change(head_wxb); 
					        print_nid_ip_nid_ip_nid();
					        announce_ip_nid_ip(AS);
				        }
				        else
				        {
					          uint32_t pid_fa[AS->item.number_pid];
				                  uint32_t time_fa;
					          for(i=0;i<AS->item.number_pid;i++)
				                     {
				                         pid_huancun[i] =PID_tiqu[i];//当前PID
				                         printf("[From %s]PID_tiqu[%d]=%d   ",__func__,i,pid_huancun[i]);
				                         pid_new[i] = (PID_tiqu[i]+number)%100;  //生成算法就是加一个随机数,之后等理论研究到位后再修改。
				                         pid_fa[i] = htonl(pid_new[i]);
				                         printf("pid_new[%d]=%d\n",i,pid_new[i]);
				                     }
					             time_huancun = time_tiqu;
					             time_new = (time_tiqu + number)%100;
					             printf("[From %s]time_tiqu=%d  time_new=%d \n",__func__,time_huancun,time_new);
					               
					             time_fa = htonl(time_new);   
					             pid_path=find_ip_or_pid(AS,1);
                                                     ip_path=find_ip_or_pid(AS,0);   
				                     send_pid_a(lcore_id_xs,pid_fa,&time_fa,AS->item.number_pid,port_id_xs,queue_id_xs,pid_path,ip_path);  
				        } 
                                        
                          }
                         
                        }
		        else if((color_public_header->option&OPTION_KIND_MASK)==OPTION_KIND3_ack)
		        {
		                  printf("[From %s]it is an ack!\n",__func__);
		                  AS = search(head_wxb,(uint8_t *)EID_tiqu);
		                  int ID=AS->item.ID;
		                  if(AS==NULL) 
		                  {
		                  	printf("opps,cant find this AS in our AS-pid chain!\n");
		                  	//exit(0);
		                  }
		                  else printf("find this AS in our AS-pid chain!\n");
		                  AS->item.pid_change_time = time_huancun_pid_a;
		                  for(i=0;i<AS->item.number_pid;i++)
		                  {
		                  	AS->item.pid[i] = pid_huancun_pid_a[ID][i];
		                  }
					sys=system("clear");
					//print_configuration_init_pid_change(head); 
					update_nid_ip_nid_ip_nid(AS);
					print_configuration_init_pid_change(head_wxb); 
					print_nid_ip_nid_ip_nid();
					announce_ip_nid_ip(AS);

				FILE *fp = NULL;
				fp = fopen("pumpking.txt","a+");
				if(fp!=NULL)
				{
					printf("[From %s]OPEN FILE successfully!\n",__func__);
					uint64_t cur_tsc = rte_rdtsc();
					fprintf(fp," %lu",cur_tsc);
				}
				else
				{
					printf("[From %s]NOT FOUND THE FILE!\n",__func__);
				}
				fclose(fp);

		          }
                    }//end if 是协商包                          
                    else if (color_public_header->controltype==0x23){
                    //是协商流量比例的包。提取出来存下来就好。
                    memcpy(traffic_proportion,color_public_header+140,4);
                    int counter;
                    printf("Traffic Proportion is:");
                    for(counter=0;counter<4;counter++)
                       {
                        printf("%3d",traffic_proportion[counter]);
                        }
                   }                   
                   rte_pktmbuf_free(pkts_burst[j]);	  //BUG!!!!  The packet is no more than 206!  
                //   system("clear");
                //   fflush(stdin);
                //    print_configuration_init_pid_change(head);  //If there are two threads in one machine, this printf() function can work badly!!!
                //     sleep(10);
                  }//end if 是控制包
                 /*******************************************************/
				   }
		         	}
                          }
                          
#elif (APP_LOOKUP_METHOD == APP_LOOKUP_LPM)
                        
			k = RTE_ALIGN_FLOOR(nb_rx, FWDSTEP);
			for (j = 0; j != k; j += FWDSTEP) {
				processx4_step1(&pkts_burst[j],
					&dip[j / FWDSTEP],
					&flag[j / FWDSTEP]);
			}

			k = RTE_ALIGN_FLOOR(nb_rx, FWDSTEP);
			for (j = 0; j != k; j += FWDSTEP) {
				processx4_step2(qconf, dip[j / FWDSTEP],
					flag[j / FWDSTEP], portid,
					&pkts_burst[j], &dst_port[j]);
			}

			k = RTE_ALIGN_FLOOR(nb_rx, FWDSTEP);
			for (j = 0; j != k; j += FWDSTEP) {
				processx4_step3(&pkts_burst[j], &dst_port[j]);
			}

			/* Process up to last 3 packets one by one. */
			switch (nb_rx % FWDSTEP) {
			case 3:
				process_packet(qconf, pkts_burst[j],
					dst_port + j, portid);
				j++;
			case 2:
				process_packet(qconf, pkts_burst[j],
					dst_port + j, portid);
				j++;
			case 1:
				process_packet(qconf, pkts_burst[j],
					dst_port + j, portid);
				j++;
			}

			/*
			 * Send packets out, through destination port.
			 * Try to group packets with the same destination port.
			 * If destination port for the packet equals BAD_PORT,
			 * then free the packet without sending it out.
			 */
			for (j = 0; j < nb_rx; j = k) {

				uint16_t cn, pn = dst_port[j];

				k = j;
				do {
					cn = dst_port[k];
				} while (cn != BAD_PORT && pn == cn &&
						++k < nb_rx);

				send_packetsx4(qconf, pn, pkts_burst + j,
					k - j);

				if (cn == BAD_PORT) {
					rte_pktmbuf_free(pkts_burst[k]);
					k += 1;
				}
			}

#endif /* APP_LOOKUP_METHOD */
#else /* ENABLE_MULTI_BUFFER_OPTIMIZE == 0 */
                       /*
                        if((wxb_print_time++==10000000))
                        {
                           printf("Lcore[%u]Forwarding[%lu0,000,000]times:ENABLE_MULTI_BUFFER_OPTIMIZE == 0\n",lcore_id,++wxb_print_counter);
                           wxb_print_time=0;
                        }
                        */
			/* Prefetch first packets */
			for (j = 0; j < PREFETCH_OFFSET && j < nb_rx; j++) 
                        {
				rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j], void *));
			}

			/* Prefetch and forward already prefetched packets */
			for (j = 0; j < (nb_rx - PREFETCH_OFFSET); j++)
                        {
				rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j + PREFETCH_OFFSET], void *));
				l3fwd_simple_forward(pkts_burst[j], portid,qconf);
			}

			/* Forward remaining prefetched packets */
			for (; j < nb_rx; j++)
                        {
				l3fwd_simple_forward(pkts_burst[j], portid,qconf);
			}
                      
#endif /* ENABLE_MULTI_BUFFER_OPTIMIZE */

		}
	}//end while
}

static int
check_lcore_params(void)
{
	uint8_t queue, lcore;
	uint16_t i;
	int socketid;

	for (i = 0; i < nb_lcore_params; ++i) {
		queue = lcore_params[i].queue_id;
		if (queue >= MAX_RX_QUEUE_PER_PORT) {
			printf("invalid queue number: %hhu\n", queue);
			return -1;
		}
		lcore = lcore_params[i].lcore_id;
		if (!rte_lcore_is_enabled(lcore)) {
			printf("error: lcore %hhu is not enabled in lcore mask\n", lcore);
			return -1;
		}
		if ((socketid = rte_lcore_to_socket_id(lcore) != 0) &&
			(numa_on == 0)) {
			printf("warning: lcore %hhu is on socket %d with numa off \n",
				lcore, socketid);
		}
	}
	return 0;
}

static int
check_port_config(const unsigned nb_ports)
{
	unsigned portid;
	uint16_t i;

	for (i = 0; i < nb_lcore_params; ++i) {
		portid = lcore_params[i].port_id;
		if ((enabled_port_mask & (1 << portid)) == 0) {
			printf("port %u is not enabled in port mask\n", portid);
			return -1;
		}
		if (portid >= nb_ports) {
			printf("port %u is not present on the board\n", portid);
			return -1;
		}
	}
	return 0;
}

static uint8_t
get_port_n_rx_queues(const uint8_t port)
{
	int queue = -1;
	uint16_t i;

	for (i = 0; i < nb_lcore_params; ++i) {
		if (lcore_params[i].port_id == port && lcore_params[i].queue_id > queue)
			queue = lcore_params[i].queue_id;
	}
	return (uint8_t)(++queue);
}

static int
init_lcore_rx_queues(void)
{
	uint16_t i, nb_rx_queue;
	uint8_t lcore;

	for (i = 0; i < nb_lcore_params; ++i) {
		lcore = lcore_params[i].lcore_id;
		nb_rx_queue = lcore_conf[lcore].n_rx_queue;
		if (nb_rx_queue >= MAX_RX_QUEUE_PER_LCORE) {
			printf("error: too many queues (%u) for lcore: %u\n",
				(unsigned)nb_rx_queue + 1, (unsigned)lcore);
			return -1;
		} else {
			lcore_conf[lcore].rx_queue_list[nb_rx_queue].port_id =
				lcore_params[i].port_id;
			lcore_conf[lcore].rx_queue_list[nb_rx_queue].queue_id =
				lcore_params[i].queue_id;
			lcore_conf[lcore].n_rx_queue++;
		}
	}
	return 0;
}

/* display usage */
static void
print_usage(const char *prgname)
{
	printf ("%s [EAL options] -- -p PORTMASK -P"
		"  [--config (port,queue,lcore)[,(port,queue,lcore]]"
		"  [--enable-jumbo [--max-pkt-len PKTLEN]]\n"
		"  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
		"  -P : enable promiscuous mode\n"
		"  --config (port,queue,lcore): rx queues configuration\n"
		"  --no-numa: optional, disable numa awareness\n"
		"  --ipv6: optional, specify it if running ipv6 packets\n"
		"  --enable-jumbo: enable jumbo frame"
		" which max packet len is PKTLEN in decimal (64-9600)\n"
		"  --hash-entry-num: specify the hash entry number in hexadecimal to be setup\n",
		prgname);
}

static int parse_max_pkt_len(const char *pktlen)
{
	char *end = NULL;
	unsigned long len;

	/* parse decimal string */
	len = strtoul(pktlen, &end, 10);
	if ((pktlen[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;

	if (len == 0)
		return -1;

	return len;
}

static int
parse_portmask(const char *portmask)
{
	char *end = NULL;
	unsigned long pm;

	/* parse hexadecimal string */
	pm = strtoul(portmask, &end, 16);
	if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;

	if (pm == 0)
		return -1;

	return pm;
}

#if (APP_LOOKUP_METHOD == APP_LOOKUP_EXACT_MATCH)
static int
parse_hash_entry_number(const char *hash_entry_num)
{
	char *end = NULL;
	unsigned long hash_en;
	/* parse hexadecimal string */
	hash_en = strtoul(hash_entry_num, &end, 16);
	if ((hash_entry_num[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;

	if (hash_en == 0)
		return -1;

	return hash_en;
}
#endif

static int
parse_config(const char *q_arg)
{
	char s[256];
	const char *p, *p0 = q_arg;
	char *end;
	enum fieldnames {
		FLD_PORT = 0,
		FLD_QUEUE,
		FLD_LCORE,
		_NUM_FLD
	};
	unsigned long int_fld[_NUM_FLD];
	char *str_fld[_NUM_FLD];
	int i;
	unsigned size;

	nb_lcore_params = 0;

	while ((p = strchr(p0,'(')) != NULL) {
		++p;
		if((p0 = strchr(p,')')) == NULL)
			return -1;

		size = p0 - p;
		if(size >= sizeof(s))
			return -1;

		snprintf(s, sizeof(s), "%.*s", size, p);
		if (rte_strsplit(s, sizeof(s), str_fld, _NUM_FLD, ',') != _NUM_FLD)
			return -1;
		for (i = 0; i < _NUM_FLD; i++){
			errno = 0;
			int_fld[i] = strtoul(str_fld[i], &end, 0);
			if (errno != 0 || end == str_fld[i] || int_fld[i] > 255)
				return -1;
		}
		if (nb_lcore_params >= MAX_LCORE_PARAMS) {
			printf("exceeded max number of lcore params: %hu\n",
				nb_lcore_params);
			return -1;
		}
		lcore_params_array[nb_lcore_params].port_id = (uint8_t)int_fld[FLD_PORT];
		lcore_params_array[nb_lcore_params].queue_id = (uint8_t)int_fld[FLD_QUEUE];
		lcore_params_array[nb_lcore_params].lcore_id = (uint8_t)int_fld[FLD_LCORE];
		++nb_lcore_params;
	}
	lcore_params = lcore_params_array;
	return 0;
}

#define CMD_LINE_OPT_CONFIG "config"
#define CMD_LINE_OPT_NO_NUMA "no-numa"
#define CMD_LINE_OPT_IPV6 "ipv6"
#define CMD_LINE_OPT_ENABLE_JUMBO "enable-jumbo"
#define CMD_LINE_OPT_HASH_ENTRY_NUM "hash-entry-num"

/* Parse the argument given in the command line of the application */
static int
parse_args(int argc, char **argv)
{
	int opt, ret;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];
	static struct option lgopts[] = {
		{CMD_LINE_OPT_CONFIG, 1, 0, 0},
		{CMD_LINE_OPT_NO_NUMA, 0, 0, 0},
		{CMD_LINE_OPT_IPV6, 0, 0, 0},
		{CMD_LINE_OPT_ENABLE_JUMBO, 0, 0, 0},
		{CMD_LINE_OPT_HASH_ENTRY_NUM, 1, 0, 0},
		{NULL, 0, 0, 0}
	};

	argvopt = argv;

	while ((opt = getopt_long(argc, argvopt, "p:P",
				lgopts, &option_index)) != EOF) {

		switch (opt) {
		/* portmask */
		case 'p':
			enabled_port_mask = parse_portmask(optarg);
			if (enabled_port_mask == 0) {
				printf("invalid portmask\n");
				print_usage(prgname);
				return -1;
			}
			break;
		case 'P':
			printf("Promiscuous mode selected\n");
			promiscuous_on = 1;
			break;

		/* long options */
		case 0:
			if (!strncmp(lgopts[option_index].name, CMD_LINE_OPT_CONFIG,
				sizeof (CMD_LINE_OPT_CONFIG))) {
				ret = parse_config(optarg);
				if (ret) {
					printf("invalid config\n");
					print_usage(prgname);
					return -1;
				}
			}

			if (!strncmp(lgopts[option_index].name, CMD_LINE_OPT_NO_NUMA,
				sizeof(CMD_LINE_OPT_NO_NUMA))) {
				printf("numa is disabled \n");
				numa_on = 0;
			}

#if (APP_LOOKUP_METHOD == APP_LOOKUP_EXACT_MATCH)
			if (!strncmp(lgopts[option_index].name, CMD_LINE_OPT_IPV6,
				sizeof(CMD_LINE_OPT_IPV6))) {
				printf("ipv6 is specified \n");
				ipv6 = 1;
			}
#endif

			if (!strncmp(lgopts[option_index].name, CMD_LINE_OPT_ENABLE_JUMBO,
				sizeof (CMD_LINE_OPT_ENABLE_JUMBO))) {
				struct option lenopts = {"max-pkt-len", required_argument, 0, 0};

				printf("jumbo frame is enabled - disabling simple TX path\n");
				port_conf.rxmode.jumbo_frame = 1;
				tx_conf.txq_flags = 0;

				/* if no max-pkt-len set, use the default value ETHER_MAX_LEN */
				if (0 == getopt_long(argc, argvopt, "", &lenopts, &option_index)) {
					ret = parse_max_pkt_len(optarg);
					if ((ret < 64) || (ret > MAX_JUMBO_PKT_LEN)){
						printf("invalid packet length\n");
						print_usage(prgname);
						return -1;
					}
					port_conf.rxmode.max_rx_pkt_len = ret;
				}
				printf("set jumbo frame max packet length to %u\n",
						(unsigned int)port_conf.rxmode.max_rx_pkt_len);
			}
#if (APP_LOOKUP_METHOD == APP_LOOKUP_EXACT_MATCH)
			if (!strncmp(lgopts[option_index].name, CMD_LINE_OPT_HASH_ENTRY_NUM,
				sizeof(CMD_LINE_OPT_HASH_ENTRY_NUM))) {
				ret = parse_hash_entry_number(optarg);
				if ((ret > 0) && (ret <= L3FWD_HASH_ENTRIES)) {
					hash_entry_number = ret;
				} else {
					printf("invalid hash entry number\n");
					print_usage(prgname);
					return -1;
				}
			}
#endif
			break;

		default:
			print_usage(prgname);
			return -1;
		}
	}

	if (optind >= 0)
		argv[optind-1] = prgname;

	ret = optind-1;
	optind = 0; /* reset getopt lib */
	return ret;
}

static void
print_ethaddr(const char *name, const struct ether_addr *eth_addr)
{
	printf ("%s%02X:%02X:%02X:%02X:%02X:%02X", name,
		eth_addr->addr_bytes[0],
		eth_addr->addr_bytes[1],
		eth_addr->addr_bytes[2],
		eth_addr->addr_bytes[3],
		eth_addr->addr_bytes[4],
		eth_addr->addr_bytes[5]);
}

#if (APP_LOOKUP_METHOD == APP_LOOKUP_EXACT_MATCH)


static void convert_ipv6_5tuple(struct ipv6_5tuple* key1,
                union ipv6_5tuple_host* key2)
{
	uint32_t i;
	for (i = 0; i < 16; i++)
	{
		key2->ip_dst[i] = key1->ip_dst[i];
		key2->ip_src[i] = key1->ip_src[i];
	}
	key2->port_dst = rte_cpu_to_be_16(key1->port_dst);
	key2->port_src = rte_cpu_to_be_16(key1->port_src);
	key2->proto = key1->proto;
	key2->pad0 = 0;
	key2->pad1 = 0;
	key2->reserve = 0;
	return;
}

#define BYTE_VALUE_MAX 256
#define ALL_32_BITS 0xffffffff
#define BIT_8_TO_15 0x0000ff00
#define ZERO_32_BITS 0x00000000


#define BIT_16_TO_23 0x00ff0000
static inline void
populate_ipv6_few_flow_into_table(const struct rte_hash* h)
{
	uint32_t i;
	int32_t ret;
	uint32_t array_len = sizeof(ipv6_l3fwd_route_array)/sizeof(ipv6_l3fwd_route_array[0]);

	mask1 = _mm_set_epi32(ALL_32_BITS, ALL_32_BITS, ALL_32_BITS, BIT_16_TO_23);
	mask2 = _mm_set_epi32(0, 0, ALL_32_BITS, ALL_32_BITS);
	for (i = 0; i < array_len; i++) {
		struct ipv6_l3fwd_route entry;
		union ipv6_5tuple_host newkey;
		entry = ipv6_l3fwd_route_array[i];
		convert_ipv6_5tuple(&entry.key, &newkey);
		ret = rte_hash_add_key (h, (void *) &newkey);
		if (ret < 0) {
			rte_exit(EXIT_FAILURE, "Unable to add entry %" PRIu32
				" to the l3fwd hash.\n", i);
		}
		ipv6_l3fwd_out_if[ret] = entry.if_out;
	}
	printf("Hash: Adding 0x%" PRIx32 "keys\n", array_len);
}

#define NUMBER_PORT_USED 4
/*
static inline void
populate_ipv4_many_flow_into_table(const struct rte_hash* h,
                unsigned int nr_flow)
{
	unsigned i;
	mask0 = _mm_set_epi32(ALL_32_BITS, ALL_32_BITS, ALL_32_BITS, BIT_8_TO_15);
	for (i = 0; i < nr_flow; i++) {
		struct ipv4_l3fwd_route entry;
		union ipv4_5tuple_host newkey;
		uint8_t a = (uint8_t) ((i/NUMBER_PORT_USED)%BYTE_VALUE_MAX);
		uint8_t b = (uint8_t) (((i/NUMBER_PORT_USED)/BYTE_VALUE_MAX)%BYTE_VALUE_MAX);
		uint8_t c = (uint8_t) ((i/NUMBER_PORT_USED)/(BYTE_VALUE_MAX*BYTE_VALUE_MAX));
		// Create the ipv4 exact match flow 
		memset(&entry, 0, sizeof(entry));
		switch (i & (NUMBER_PORT_USED -1)) {
		case 0:
			entry = ipv4_l3fwd_route_array[0];
			entry.key.ip_dst = IPv4(101,c,b,a);
			break;
		case 1:
			entry = ipv4_l3fwd_route_array[1];
			entry.key.ip_dst = IPv4(201,c,b,a);
			break;
		case 2:
			entry = ipv4_l3fwd_route_array[2];
			entry.key.ip_dst = IPv4(111,c,b,a);
			break;
		case 3:
			entry = ipv4_l3fwd_route_array[3];
			entry.key.ip_dst = IPv4(211,c,b,a);
			break;
		};
		convert_ipv4_5tuple(&entry.key, &newkey);
		int32_t ret = rte_hash_add_key(h,(void *) &newkey);
		if (ret < 0) {
			rte_exit(EXIT_FAILURE, "Unable to add entry %u\n", i);
		}
		ipv4_l3fwd_out_if[ret] = (uint8_t) entry.if_out;

	}
	printf("Hash: Adding 0x%x keys\n", nr_flow);
}
*/
static inline void
populate_ipv6_many_flow_into_table(const struct rte_hash* h,
                unsigned int nr_flow)
{
	unsigned i;
	mask1 = _mm_set_epi32(ALL_32_BITS, ALL_32_BITS, ALL_32_BITS, BIT_16_TO_23);
	mask2 = _mm_set_epi32(0, 0, ALL_32_BITS, ALL_32_BITS);
	for (i = 0; i < nr_flow; i++) {
		struct ipv6_l3fwd_route entry;
		union ipv6_5tuple_host newkey;
		uint8_t a = (uint8_t) ((i/NUMBER_PORT_USED)%BYTE_VALUE_MAX);
		uint8_t b = (uint8_t) (((i/NUMBER_PORT_USED)/BYTE_VALUE_MAX)%BYTE_VALUE_MAX);
		uint8_t c = (uint8_t) ((i/NUMBER_PORT_USED)/(BYTE_VALUE_MAX*BYTE_VALUE_MAX));
		/* Create the ipv6 exact match flow */
		memset(&entry, 0, sizeof(entry));
		switch (i & (NUMBER_PORT_USED - 1)) {
		case 0: entry = ipv6_l3fwd_route_array[0]; break;
		case 1: entry = ipv6_l3fwd_route_array[1]; break;
		case 2: entry = ipv6_l3fwd_route_array[2]; break;
		case 3: entry = ipv6_l3fwd_route_array[3]; break;
		};
		entry.key.ip_dst[13] = c;
		entry.key.ip_dst[14] = b;
		entry.key.ip_dst[15] = a;
		convert_ipv6_5tuple(&entry.key, &newkey);
		int32_t ret = rte_hash_add_key(h,(void *) &newkey);
		if (ret < 0) {
			rte_exit(EXIT_FAILURE, "Unable to add entry %u\n", i);
		}
		ipv6_l3fwd_out_if[ret] = (uint8_t) entry.if_out;

	}
	printf("Hash: Adding 0x%x keys\n", nr_flow);
}

static void
setup_hash(int socketid)
{
    struct rte_hash_parameters ipv6_l3fwd_hash_params = {
        .name = NULL,
        .entries = L3FWD_HASH_ENTRIES,
        .bucket_entries = 4,
        .key_len = sizeof(union ipv6_5tuple_host),
        .hash_func = ipv6_hash_crc,
        .hash_func_init_val = 0,
    };

    char s[64];

		/* create ipv6 hash */
	snprintf(s, sizeof(s), "ipv6_l3fwd_hash_%d", socketid);
	ipv6_l3fwd_hash_params.name = s;
	ipv6_l3fwd_hash_params.socket_id = socketid;
	ipv6_l3fwd_lookup_struct[socketid] = rte_hash_create(&ipv6_l3fwd_hash_params);
	if (ipv6_l3fwd_lookup_struct[socketid] == NULL)
		rte_exit(EXIT_FAILURE, "Unable to create the l3fwd hash on "
				"socket %d\n", socketid);

	if (hash_entry_number != HASH_ENTRY_NUMBER_DEFAULT) 
        {
		/* For testing hash matching with a large number of flows we
		 * generate millions of IP 5-tuples with an incremented dst
		 * address to initialize the hash table. */
		if (ipv6 == 0) 
                {
			/* populate the ipv4 hash */
			//populate_ipv4_many_flow_into_table(get_l3fwd_lookup_struct[socketid], hash_entry_number);
		} else {
			/* populate the ipv6 hash */
			populate_ipv6_many_flow_into_table(
				ipv6_l3fwd_lookup_struct[socketid], hash_entry_number);
		}
	}
        else 
        {
		/* Use data in ipv4/ipv6 l3fwd lookup table directly to initialize the hash table */
		if (ipv6 == 0) 
                {
			/* populate the ipv4 hash */
			
		} else {
			/* populate the ipv6 hash */
			populate_ipv6_few_flow_into_table(ipv6_l3fwd_lookup_struct[socketid]);
		}
	}
}
#endif

#if (APP_LOOKUP_METHOD == APP_LOOKUP_LPM)

#endif

static int
init_mem(unsigned nb_mbuf)
{
	struct lcore_conf *qconf;
	int socketid;
	unsigned lcore_id;
	char s[64];

	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) 
        {
		if (rte_lcore_is_enabled(lcore_id) == 0)
			continue;

		if (numa_on)
			socketid = rte_lcore_to_socket_id(lcore_id);
		else
			socketid = 0;

		if (socketid >= NB_SOCKETS) {
			rte_exit(EXIT_FAILURE, "Socket %d of lcore %u is out of range %d\n",
				socketid, lcore_id, NB_SOCKETS);
		}
		if (pktmbuf_pool[socketid] == NULL) {
			snprintf(s, sizeof(s), "mbuf_pool_%d", socketid);
			pktmbuf_pool[socketid] =
				rte_mempool_create(s, nb_mbuf, MBUF_SIZE, MEMPOOL_CACHE_SIZE,
					sizeof(struct rte_pktmbuf_pool_private),
					rte_pktmbuf_pool_init, NULL,
					rte_pktmbuf_init, NULL,
					socketid, 0);
			if (pktmbuf_pool[socketid] == NULL)
				rte_exit(EXIT_FAILURE,
						"Cannot init mbuf pool on socket %d\n", socketid);
			else
				printf("Allocated mbuf pool on socket %d\n", socketid);

#if (APP_LOOKUP_METHOD == APP_LOOKUP_LPM)
//			setup_lpm(socketid);
#else
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/
			setup_hash_wxb(socketid);
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/
			setup_hash(socketid);
#endif
		}
		qconf = &lcore_conf[lcore_id];
		qconf->ipv6_lookup_struct = ipv6_l3fwd_lookup_struct[socketid];
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************Start**********/
		qconf->ipv4_lookup_struct_wxb = ipv4_lookup_struct_wxb[socketid];
		qconf->sid_lookup_struct_wxb=sid_lookup_struct_wxb[socketid];
/**********************************************************************************************/
/***************************************By WenXingBeng*****************************************/
/*******************************************************************************End************/

	}
	return 0;
}

/* Check the link status of all ports in up to 9s, and print them finally */
static void
check_all_ports_link_status(uint8_t port_num, uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
	uint8_t portid, count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		all_ports_up = 1;
		for (portid = 0; portid < port_num; portid++) {
			if ((port_mask & (1 << portid)) == 0)
				continue;
			memset(&link, 0, sizeof(link));
			rte_eth_link_get_nowait(portid, &link);
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf("Port %d Link Up - speed %u "
						"Mbps - %s\n", (uint8_t)portid,
						(unsigned)link.link_speed,
				(link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
					("full-duplex") : ("half-duplex\n"));
				else
					printf("Port %d Link Down\n",
						(uint8_t)portid);
				continue;
			}
			/* clear all_ports_up flag if any link down */
			if (link.link_status == 0) {
				all_ports_up = 0;
				break;
			}
		}
		/* after finally printing all link status, get out */
		if (print_flag == 1)
			break;

		if (all_ports_up == 0) {
			printf(".");
			fflush(stdout);
			rte_delay_ms(CHECK_INTERVAL);
		}

		/* set the print_flag if all ports up or timeout */
		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
			print_flag = 1;
			printf("done\n");
		}
	}
}



int
MAIN(int argc, char **argv)
{        
	int ret;
        struct lcore_conf *qconf;
        struct rte_eth_dev_info dev_info;
        struct rte_eth_txconf *txconf;
        	
	unsigned nb_ports;
	uint16_t queueid;
	unsigned lcore_id;
	uint32_t n_tx_queue, nb_lcores;
	uint8_t portid, nb_rx_queue, queue, socketid;

       /*定时改变选路策略****/
       signal(SIGALRM,signal_handler);
       set_timer();
       /******/

        /*******/
        //read_configuration_init_pid_change();
        print_nid_ip_nid_ip_nid();
        aligment_configure();
        print_nid_ip_nid_ip_nid();
        read_configuration_init_pid_change_wxb();
        
	/* init EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL parameters\n");
	argc -= ret;
	argv += ret;

	/* parse application arguments (after the EAL ones) */
	ret = parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid L3FWD parameters\n");

	if (check_lcore_params() < 0)
		rte_exit(EXIT_FAILURE, "check_lcore_params failed\n");

	ret = init_lcore_rx_queues();
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "init_lcore_rx_queues failed\n");


	if (rte_eal_pci_probe() < 0)
		rte_exit(EXIT_FAILURE, "Cannot probe PCI\n");

	nb_ports = rte_eth_dev_count();
	if (nb_ports > RTE_MAX_ETHPORTS)
		nb_ports = RTE_MAX_ETHPORTS;

	if (check_port_config(nb_ports) < 0)
		rte_exit(EXIT_FAILURE, "check_port_config failed\n");

	nb_lcores = rte_lcore_count();



        
        
	/* initialize all ports */
	for (portid = 0; portid < nb_ports; portid++) {
		/* skip ports that are not enabled */
		if ((enabled_port_mask & (1 << portid)) == 0) {
			printf("\nSkipping disabled port %d\n", portid);
			continue;
		}

		/* init port */
		printf("Initializing port %d ... ", portid );
		fflush(stdout);

		nb_rx_queue = get_port_n_rx_queues(portid);
		n_tx_queue = nb_lcores;
		if (n_tx_queue > MAX_TX_QUEUE_PER_PORT)
			n_tx_queue = MAX_TX_QUEUE_PER_PORT;
		printf("Creating queues: nb_rxq=%d nb_txq=%u... ",
			nb_rx_queue, (unsigned)n_tx_queue );
		ret = rte_eth_dev_configure(portid, nb_rx_queue,
					(uint16_t)n_tx_queue, &port_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%d\n",
				ret, portid);

		rte_eth_macaddr_get(portid, &ports_eth_addr[portid]);
		print_ethaddr(" Address:", &ports_eth_addr[portid]);
		printf(", ");

		/*
		 * prepare dst and src MACs for each port.
		 */
		*(uint64_t *)(val_eth + portid) =
			ETHER_LOCAL_ADMIN_ADDR + ((uint64_t)portid << 40);
		ether_addr_copy(&ports_eth_addr[portid],
			(struct ether_addr *)(val_eth + portid) + 1);

		/* init memory */
		ret = init_mem(NB_MBUF);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "init_mem failed\n");

		/* init one TX queue per couple (lcore,port) */
		queueid = 0;
		for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
			if (rte_lcore_is_enabled(lcore_id) == 0)
				continue;

			if (numa_on)
				socketid = (uint8_t)rte_lcore_to_socket_id(lcore_id);
			else
				socketid = 0;

			printf("txq=%u,%d,%d ", lcore_id, queueid, socketid);
			fflush(stdout);
			ret = rte_eth_tx_queue_setup(portid, queueid, nb_txd,
						     socketid, &tx_conf);
			if (ret < 0)
				rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup: err=%d, "
					"port=%d\n", ret, portid);

			qconf = &lcore_conf[lcore_id];
			qconf->tx_queue_id[portid] = queueid;
			queueid++;
		}
		printf("\n");
	}

	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
		if (rte_lcore_is_enabled(lcore_id) == 0)
			continue;
		qconf = &lcore_conf[lcore_id];
		printf("\nInitializing rx queues on lcore %u ... ", lcore_id );
		fflush(stdout);
		/* init RX queues */
		for(queue = 0; queue < qconf->n_rx_queue; ++queue) {
			portid = qconf->rx_queue_list[queue].port_id;
			queueid = qconf->rx_queue_list[queue].queue_id;

			if (numa_on)
				socketid = (uint8_t)rte_lcore_to_socket_id(lcore_id);
			else
				socketid = 0;

			printf("rxq=%d,%d,%d ", portid, queueid, socketid);
			fflush(stdout);

			ret = rte_eth_rx_queue_setup(portid, queueid, nb_rxd,
				        socketid, &rx_conf, pktmbuf_pool[socketid]);
			if (ret < 0)
				rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup: err=%d,"
						"port=%d\n", ret, portid);
		}
	}

	printf("\n");

	/* start ports */
	for (portid = 0; portid < nb_ports; portid++) {
		if ((enabled_port_mask & (1 << portid)) == 0) {
			continue;
		}
		/* Start device */
		ret = rte_eth_dev_start(portid);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_dev_start: err=%d, port=%d\n",
				ret, portid);

		/*
		 * If enabled, put device in promiscuous mode.
		 * This allows IO forwarding mode to forward packets
		 * to itself through 2 cross-connected  ports of the
		 * target machine.
		 */
		if (promiscuous_on)
			rte_eth_promiscuous_enable(portid);
	}

        /*uint64_t hz=0;
        FILE *fp = NULL;
	fp = fopen("pumpking.txt","w+");
	if(fp!=NULL)
	{
	
		printf("[From %s]OPEN FILE successfully!\n",__func__);
		hz= rte_get_timer_hz();
		fprintf(fp,"hz=%lu\n",hz);	
	}
	else
	{
		printf("[From %s]NOT FOUND THE FILE!\n",__func__);
	}
	fclose(fp);
        */

        /*******/
	check_all_ports_link_status((uint8_t)nb_ports, enabled_port_mask);

	/* launch per-lcore init on every lcore */
	rte_eal_mp_remote_launch(main_loop, NULL, CALL_MASTER);
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (rte_eal_wait_lcore(lcore_id) < 0)
			return -1;
	}

	return 0;

}
