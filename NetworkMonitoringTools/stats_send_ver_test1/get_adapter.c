#include "main.h"
#include "getnet.h"

#ifndef SIOCETHTOOL
#define SIOCETHTOOL 0x8946
#endif
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static int link_detect = 0;

//网卡当前工作模式
static int dump_duplex(struct ethtool_cmd *ep)
{
	switch (ep->duplex) {
	case DUPLEX_HALF:
		return 1;
	case DUPLEX_FULL:
		return 2;
	default:
		break;
	};
	return 0;
}

//网卡当前接口类型
static int dump_port(struct ethtool_cmd *ep)
{
	switch (ep->port) {
	case PORT_TP:
		return 1;
	case PORT_AUI:
		return 2;
	case PORT_BNC:
		return 3;
	case PORT_MII:
		return 4;		
	case PORT_FIBRE:
		return 5;
	default:
		break;
	};
	
	return 0;
}

//网口收发器
static int dump_transceiver(struct ethtool_cmd *ep)
{
	switch (ep->transceiver) {
	case XCVR_INTERNAL:
		return 1;
	case XCVR_EXTERNAL:
		return 2;
	default:
		break;
	};	
	return 0;
}

static int dump_ecmd(struct ethtool_cmd *ep,struct ADAPTER *adapter)
{

	adapter->supported = ep->supported;
	adapter->advertising = ep->advertising;
	adapter->speed = ep->speed;
	adapter->duplex = dump_duplex(ep);
	adapter->port = dump_port(ep);
	adapter->transceiver = dump_transceiver(ep);
	adapter->autoneg = ep->autoneg;

	return 0;
}

static int do_gset(int fd, struct ifreq *ifr,struct ADAPTER *adapter)
{
	int err;
	struct ethtool_cmd ecmd;
	struct ethtool_wolinfo wolinfo;
	struct ethtool_value edata;
	int allfail = 1;

	memset(&ecmd,0,sizeof(ecmd));
	memset(&wolinfo,0,sizeof(wolinfo));
	memset(&edata,0,sizeof(edata));
	
	ecmd.cmd = ETHTOOL_GSET;
	ifr->ifr_data = (caddr_t)&ecmd;
	err = ioctl(fd, SIOCETHTOOL, ifr);
	if (err == 0) {
		//填充静态信息
		err = dump_ecmd(&ecmd,adapter);
		if(err){return err;}
		allfail = 0;
	} else if (errno != EOPNOTSUPP) {
		perror("Cannot get device settings");
	}

	edata.cmd = ETHTOOL_GLINK;
	ifr->ifr_data = (caddr_t)&edata;
	err = ioctl(fd, SIOCETHTOOL, ifr);
	if (err == 0) {
		if(edata.data)
		{	
			link_detect = 1;
		}		
		else{link_detect = 0;}
		allfail = 0;
	} else if (errno != EOPNOTSUPP) {
		perror("Cannot get link status");
	}

	if (allfail) {
		fprintf(stdout, "No data available\n");
		return 75;
	}	
	return 0;
}

int get_adapter(int *fd,struct PORT_INFO *port_info)
{

/* http://topic.csdn.net/u/20070104/12/e57086ff-1a48-477b-b672-91e4ba3b6da4.html
ifreq结构定义在/usr/include\net/if.h，用来配置ip地址，激活接口，配置MTU等接口信息的。
其中包含了一个接口的名字和具体内容——（是个共用体，有可能是IP地址，广播地址，子网掩码，MAC号，MTU或其他内容）。
ifreq包含在ifconf结构中。而ifconf结构通常是用来保存所有接口的信息的。
*/
	int ret;
	struct ifreq ifr; // 接口请求结构
	struct ADAPTER adapter;

/* Setup our control structures. */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, port_info->dev_info.dev_name);
	
	ret = do_gset(*fd,&ifr,&adapter);
	port_info->dev_info.adapter = adapter;
	port_info->link_detect = link_detect;
	return ret;
}
