#include "main.h"
#include "getnet.h"

//测试专用时间输出程序
void print_time(void)
{
	char tstr[1024];
    time_t t;
    
    t = time(NULL);
    strcpy(tstr, ctime(&t));
    tstr[strlen(tstr)-1] = '\0';
    printf("\n%s\n",tstr);
}

//测试专用动态网包计数器输出程序
void test_dump_stats_info(struct STATS_INFO *stats_info)
{
	printf("total packets : %lld\n",stats_info->total_packets);
	printf("total bytes : %lld\n",stats_info->total_bytes);
	printf("packets send : %lld\n",stats_info->tx_packets);
	printf("bytes send : %lld\n",stats_info->tx_bytes);
	printf("packets receive : %lld\n",stats_info->rx_packets);
	printf("bytes receive : %lld\n",stats_info->rx_bytes);
	printf("tx_packets_ps : %f\n",stats_info->tx_packets_ps);	
	printf("rx_packets_ps : %f\n",stats_info->rx_packets_ps);
	printf("tx_bytes_ps : %f\n",stats_info->tx_bytes_ps);
	printf("rx_bytes_ps : %f\n",stats_info->rx_bytes_ps);

	printf("\n\n----------------------------------------------------------------------\n\n");
}

//打印定时器信息
void print_timer_manage(struct _timer_info* head)
{
    struct _timer_info* p;

    p = head;
    
    printf("id\telapse\ttime\n");

    while(p != NULL)
    {
        printf("%d\t%d\t%s\n", p->timer_id, p->elapse, asctime(&p->t));
        p = p->next;
    }
}

//文件时间输出程序
void fprint_time(FILE *p)
{
	char tstr[1024];
    time_t t;
    
    t = time(NULL);
    strcpy(tstr, ctime(&t));
    tstr[strlen(tstr)-1] = '\0';
    fprintf(p, "%s\n", tstr);
}

//解析设备系统信息
void dump_host_info(struct HOST_INFO *host_info,FILE *p)
{
//	mysql_insert_host(host_info);

	fprintf(p,"host name : %s\n",host_info->host_name);
	fprintf(p,"sysname : %s\n",host_info->u_name.sysname);
	fprintf(p,"nodename : %s\n",host_info->u_name.nodename);
	fprintf(p,"release : %s\n",host_info->u_name.release);
	fprintf(p,"version : %s\n",host_info->u_name.version);
	fprintf(p,"machin : %s\n",host_info->u_name.machine);
	fprintf(p,"domainname : %s\n",host_info->u_name.__domainname);
	fprintf(p,"port amount : %d\n",host_info->port_count);

	fprintf(p,"\n\n----------------------------------------------------------------------\n\n");

}

//网络端口具体参数解析函数
/*********************************************************************************************************************/

void dump_supported(struct ADAPTER *ep,FILE *p)
{
	u_int32_t mask = ep->supported;
	int did1 = 0;

//支持的接口类型
	fprintf(p, "	Supported ports: [ ");
	if (mask & SUPPORTED_TP)
	{
		did1++;
		fprintf(p, "TP ");
	}
	if (mask & SUPPORTED_AUI)
	{
		did1++;
		fprintf(p, "AUI ");
	}
	if (mask & SUPPORTED_BNC)
	{
		did1++;
		fprintf(p, "BNC ");
	}	
	if (mask & SUPPORTED_MII)
	{
		did1++;
		fprintf(p, "MII ");
	}
	if (mask & SUPPORTED_FIBRE)
	{
		did1++;
		fprintf(p, "FIBRE ");
	}
	fprintf(p, "]\n");

//网卡支持的链路模式
	fprintf(p, "	Supported link modes:   ");
	did1 = 0;
	if (mask & SUPPORTED_10baseT_Half) {
		did1++; fprintf(p, "10baseT/Half ");
	}
	if (mask & SUPPORTED_10baseT_Full) {
		did1++; fprintf(p, "10baseT/Full ");
	}
	if (did1 && (mask & (SUPPORTED_100baseT_Half|SUPPORTED_100baseT_Full))) {
		fprintf(p, "\n");
		fprintf(p, "	                        ");
	}
	if (mask & SUPPORTED_100baseT_Half) {
		did1++; fprintf(p, "100baseT/Half ");
	}
	if (mask & SUPPORTED_100baseT_Full) {
		did1++; fprintf(p, "100baseT/Full ");
	}
	if (did1 && (mask & (SUPPORTED_1000baseT_Half|SUPPORTED_1000baseT_Full))) {
		fprintf(p, "\n");
		fprintf(p, "	                        ");
	}
	if (mask & SUPPORTED_1000baseT_Half) {
		did1++; fprintf(p, "1000baseT/Half ");
	}
	if (mask & SUPPORTED_1000baseT_Full) {
		did1++; fprintf(p, "1000baseT/Full ");
	}
	if (mask & SUPPORTED_10000baseT_Full)
	{
		did1++; fprintf(p, "\n");
		did1++; fprintf(p, "10000baseT/Full");
	}
	if(did1 == 0)
	{
		fprintf(p,"Not reported");
	}
	fprintf(p, "\n");

//网卡是否支持自动协商
	fprintf(p, "	Supports auto-negotiation: ");
	if (mask & SUPPORTED_Autoneg)
		fprintf(p, "Yes\n");
	else
		fprintf(p, "No\n");
}

//网卡通告状态
void dump_advertised(struct ADAPTER *ep,FILE *p)
{
	u_int32_t mask = ep->advertising;
	int did1;

	fprintf(p, "	Advertised link modes:  ");
	did1 = 0;
	if (mask & ADVERTISED_10baseT_Half) {
		did1++; fprintf(p, "10baseT/Half ");
	}
	if (mask & ADVERTISED_10baseT_Full) {
		did1++; fprintf(p, "10baseT/Full ");
	}
	if (did1 && (mask & (ADVERTISED_100baseT_Half|ADVERTISED_100baseT_Full))) {
		fprintf(p, "\n");
		fprintf(p, "	                        ");
	}
	if (mask & ADVERTISED_100baseT_Half) {
		did1++; fprintf(p, "100baseT/Half ");
	}
	if (mask & ADVERTISED_100baseT_Full) {
		did1++; fprintf(p, "100baseT/Full ");
	}
	if (did1 && (mask & (ADVERTISED_1000baseT_Half|ADVERTISED_1000baseT_Full))) {
		fprintf(p, "\n");
		fprintf(p, "	                        ");
	}
	if (mask & ADVERTISED_1000baseT_Half) {
		did1++; fprintf(p, "1000baseT/Half ");
	}
	if (mask & ADVERTISED_1000baseT_Full) {
		did1++; fprintf(p, "1000baseT/Full ");
	}
	if (mask & ADVERTISED_10000baseT_Full)
	{
		did1++; fprintf(p, "\n");
		fprintf(p, "10000baseT/Full ");
	}
	if (did1 == 0)
		 fprintf(p, "Not reported");
	fprintf(p, "\n");

	fprintf(p, "	Advertised auto-negotiation: ");
	if (mask & ADVERTISED_Autoneg)
		fprintf(p, "Yes\n");
	else
		fprintf(p, "No\n");
}

//网卡当前限速
int dump_speed(struct ADAPTER *ep,FILE *p)
{	
	int ret = 0;
	fprintf(p, "	Speed: ");
	switch (ep->speed) {
	case SPEED_10:
		{
			ret = 10;
			fprintf(p, "10Mb/s\n");
			break;
		}
	case SPEED_100:
		{
			ret = 100;
			fprintf(p, "100Mb/s\n");
			break;
		}
	case SPEED_1000:
		{
			ret = 1000;
			fprintf(p, "1000Mb/s\n");
			break;
		}
	case SPEED_10000:
		{
			ret = 10000;
			fprintf(p, "10000Mb/s\n");
			break;
		}
	default:
		fprintf(p, "Unknown! (%i)\n", ep->speed);
		break;
	};
	
	return ret;
}

int fdump_speed(struct ADAPTER *ep)
{	
	int ret = 0;
	
	switch (ep->speed) {
	case SPEED_10:
		{
			ret = 10;
			break;
		}
	case SPEED_100:
		{
			ret = 100;
			break;
		}
	case SPEED_1000:
		{
			ret = 1000;
			break;
		}
	case SPEED_10000:
		{
			ret = 10000;
			break;
		}
	default:
		printf("Unknown Speed! (%i)\n", ep->speed);
		break;
	};
	
	return ret;
}

//网卡当前工作模式
void dump_duplex(struct ADAPTER *ep,FILE *p)
{
	fprintf(p, "	Duplex: ");
	switch (ep->duplex) {
	case 1:
		fprintf(p, "Half\n");
		break;
	case 2:
		fprintf(p, "Full\n");
		break;
	default:
		fprintf(p, "Unknown! (%i)\n", ep->duplex);
		break;
	};
}

//网卡当前接口类型
void dump_port(struct ADAPTER *ep,FILE *p)
{
	fprintf(p, "	Port: ");
	switch (ep->port) {
	case 1:
		fprintf(p, "Twisted Pair\n");
		break;
	case 2:
		fprintf(p, "AUI\n");
		break;
	case 3:
		fprintf(p, "BNC\n");
		break;
	case 4:
		fprintf(p, "MII\n");
		break;
	case 5:
		fprintf(p, "FIBRE\n");
		break;
	default:
		fprintf(p, "Unknown! (%i)\n", ep->port);
		break;
	};
}

//网口收发器
void dump_transceiver(struct ADAPTER *ep,FILE *p)
{
	fprintf(p, "	Transceiver: ");
	switch (ep->transceiver) {
	case 1:
		fprintf(p, "internal\n");
		break;
	case 2:
		fprintf(p, "external\n");
		break;
	default:
		fprintf(p, "Unknown!\n");
		break;
	};
}

//网口自动协商
void dump_autoneg(struct ADAPTER *ep,FILE *p)
{
	fprintf(p, "	Auto-negotiation: %s\n",
		(ep->autoneg == AUTONEG_DISABLE) ?
		"off" : "on");
}
/**************************************************************************************************/

//解析网卡具体参数
void dump_adapter(struct ADAPTER *adapter,FILE *p)
{

		//网卡状态解析	
		dump_supported(adapter,p);
		dump_advertised(adapter,p);
		dump_speed(adapter,p);
		dump_duplex(adapter,p);
		dump_port(adapter,p);
		dump_transceiver(adapter,p);
		dump_autoneg(adapter,p);
}

//解析网卡基本信息
void dump_dev_info(struct DEV_INFO *dev_info,FILE *p, int* i)
{
	fprintf(p,"dev_name : %s\n",dev_info->dev_name);
	fprintf(p,"dev_mac : %02x:%02x:%02x:%02x:%02x:%02x\n",
               (unsigned char)dev_info->dev_mac[0],
               (unsigned char)dev_info->dev_mac[1],
               (unsigned char)dev_info->dev_mac[2],
               (unsigned char)dev_info->dev_mac[3],
               (unsigned char)dev_info->dev_mac[4],
               (unsigned char)dev_info->dev_mac[5]);

	fprintf(p,"dev_ip : %s\n",dev_info->dev_ip);
	fprintf(p,"dev_broad_addr : %s\n",dev_info->dev_broad_addr);
	fprintf(p,"dev_subnet_mask : %s\n",dev_info->dev_subnet_mask);
	fprintf(p,"MTU : %d\n",dev_info->mtu);

	struct ADAPTER adapter;
	adapter = dev_info->adapter;
	dump_adapter(&adapter,p);
	fprintf(p,"\n\n----------------------------------------------------------------------\n\n");
}

//各端口动态信息
/**************************************************************************************************/

//解析链路测试时延信息
void dump_link_stats(struct DEST_INFO *dest_info,FILE *p)
{

	struct DEST_INFO *pd;
	struct LINK_STATS link_stats;
	for(pd = dest_info;pd != NULL;pd = pd->next)
	{
		link_stats = pd->link_stats;
		fprintf(p,"dest ip : %s\n",pd->dest_ip);
		fprintf(p,"\n\n");
		
		fprintf(p,"packet loss : %f\n",link_stats.packet_loss);
		fprintf(p,"avg rtt : %f\n",link_stats.rtt_avg);
		fprintf(p,"min rtt : %f\n",link_stats.rtt_min);
		fprintf(p,"max rtt : %f\n",link_stats.rtt_max);
		fprintf(p,"mdev rtt : %f\n",link_stats.rtt_mdev);
		fprintf(p,"\n-------------------------------------------------------------\n\n");
	}	
}

void dump_get_pack(struct GET_PACK_COUNT *get_pack_count,FILE*p)
{
	fprintf(p,"get packets count\n\n");
	fprintf(p,"total packets : %lld\n",get_pack_count->total_packets);
	fprintf(p,"packets send : %lld\n",get_pack_count->tx_packets);	
	fprintf(p,"packets recieve : %lld\n",get_pack_count->rx_packets);
	fprintf(p,"total bytes : %lld\n",get_pack_count->total_bytes);
	fprintf(p,"bytes send : %lld\n",get_pack_count->tx_bytes);
	fprintf(p,"bytes receive : %lld\n",get_pack_count->rx_bytes);
}

void dump_data_pack(struct DATA_PACK_COUNT *data_pack_count,FILE *p)
{
	fprintf(p,"data packets count\n\n");
	fprintf(p,"total packets : %lld\n",data_pack_count->total_packets);
	fprintf(p,"packets send : %lld\n",data_pack_count->tx_packets);	
	fprintf(p,"packets receive : %lld\n",data_pack_count->rx_packets);
	fprintf(p,"total bytes : %lld\n",data_pack_count->total_bytes);
	fprintf(p,"bytes send : %lld\n",data_pack_count->tx_bytes);
	fprintf(p,"bytes receive : %lld\n",data_pack_count->rx_bytes);
}

//解析各网卡收发数据统计信息
void dump_stats_info(struct STATS_INFO *stats_info,FILE *p)
{
	/*
	fprintf(p,"total packets : %lld\n",stats_info->total_packets);
	*/
	fprintf(p,"%lld\n",stats_info->total_bytes);
	/*
	fprintf(p,"packets send : %lld\n",stats_info->tx_packets);
	fprintf(p,"bytes send : %lld\n",stats_info->tx_bytes);
	fprintf(p,"packets receive : %lld\n",stats_info->rx_packets);
	fprintf(p,"bytes receive : %lld\n",stats_info->rx_bytes);
	fprintf(p,"total_packets_ps : %f\n",stats_info->total_packets_ps);	
	fprintf(p,"tx_packets_ps : %f\n",stats_info->tx_packets_ps);	
	fprintf(p,"rx_packets_ps : %f\n",stats_info->rx_packets_ps);
	fprintf(p,"total_bytes_ps : %f\n",stats_info->total_bytes_ps);
	fprintf(p,"tx_bytes_ps : %f\n",stats_info->tx_bytes_ps);
	fprintf(p,"rx_bytes_ps : %f\n",stats_info->rx_bytes_ps);

	fprintf(p,"\n\n\n");

	struct GET_PACK_COUNT get_pack_count;
	get_pack_count =  stats_info->get_pack_count;
	dump_get_pack(&get_pack_count,p);

	fprintf(p,"\n\n\n");

	struct DATA_PACK_COUNT data_pack_count;
	data_pack_count =  stats_info->data_pack_count;
	dump_data_pack(&data_pack_count,p);

	fprintf(p,"\n\n\n");
	*/	
}
/**************************************************************************************************/

//解析端口效率信息
void dump_efficiency(struct EFFICIENCY *efficiency,FILE *p)
{
	fprintf(p,"the efficiency of the bandwidth is : %f \n",efficiency->bandwidth);
}

//解析本机端口信息
void dump_port_info(struct PORT_INFO *port_info,FILE*p)
{
/*
	struct DEV_INFO dev_info;
	dev_info = port_info->dev_info;
	dump_dev_info(&port_info->dev_info,p);
*/
/*	
	fprint_time(p);
	fprintf(p,"\n");
	
	fprintf(p, "	Link detected: %s\n",port_info->link_detect ? "yes":"no");
	fprintf(p,"\n");
*/		
	struct STATS_INFO stats_info;
	stats_info = port_info->stats_info;
	dump_stats_info(&stats_info,p);
/*	
	struct EFFICIENCY efficiency;
	efficiency = port_info->efficiency;
	dump_efficiency(&efficiency,p);

	fprintf(p,"\n\n----------------------------------------------------------------------\n\n");

	struct DEST_INFO *dest_info;
	dest_info = port_info->dest_info;
	dump_link_stats(dest_info,p);
*/
//	fprintf(p,"\n\n----------------------------------------------------------------------\n\n");
}