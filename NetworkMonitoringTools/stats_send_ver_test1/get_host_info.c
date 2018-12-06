#include "main.h"

int get_host_info(struct HOST_INFO* host_info, char* host_name)
{
//写入设备名称
	strcpy(host_info->host_name, host_name);
	
//设备系统信息
	int s;
	struct utsname u_name;

	s = uname(&u_name);

	if (s < 0)
	{
   		printf("uname()failed\n");
   		return -1 ;
	}

	host_info->u_name = u_name;
	
	return 0;
}

int get_port_index(struct PORT_INFO* port_info, struct HOST_INFO* host_info, char* dir_name)
{
//获取本机端口数量及每个端口基本信息
	struct ifconf ifc;
	struct ifreq buf[64];				
	int fd;
	int interfaceNum = 0;	
	int i = 0;
	char broadAddr[32];
	char subnetMask[32];
	uint8_t dev_mac[7];
	char mac[18];
	FILE *f_port_info;
	char file_name[200];

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		close(fd);
		perror("socket conf create failed");
		return -1;
	}

	memset(&file_name,0,sizeof(file_name));
	memset(&broadAddr,0,sizeof(broadAddr));
	memset(&subnetMask,0,sizeof(subnetMask));
	memset(&mac,0,sizeof(mac));

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = (caddr_t)buf;

	if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
	{
     	interfaceNum = ifc.ifc_len / sizeof(struct ifreq);	
		
		while(interfaceNum-- > 0)
		{
			memset(&dev_mac,0,sizeof(dev_mac));
	                memset(&mac,0,sizeof(mac));
		//获取接口mac地址
			if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[interfaceNum])))
             		{
		   		memcpy(dev_mac,buf[interfaceNum].ifr_hwaddr.sa_data,6);
             		//若mac地址为00:00:00:00:00:00(本地回环)，则不继续接下来的工作
				snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
					 (unsigned char)dev_mac[0],
                            		 (unsigned char)dev_mac[1],
                            		 (unsigned char)dev_mac[2],
                            		 (unsigned char)dev_mac[3],
                            		 (unsigned char)dev_mac[4],
                            		 (unsigned char)dev_mac[5]);
			
				if(strncmp(mac,"00:00:00:00:00:00",17) == 0)
				{	
					continue;	
				}
				
				memcpy(&port_info->dev_info.dev_mac,dev_mac,6);
								
			}
            		else
            		{
               			printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                		close(fd);
                		return -1;
            		}
		//获取接口名称
			strcpy(port_info->dev_info.dev_name,buf[interfaceNum].ifr_name);
		
		//获取接口IP地址
			if (!ioctl(fd, SIOCGIFADDR, (char *)&buf[interfaceNum]))
            		{
				strcpy(port_info->dev_info.dev_ip,(char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_addr))->sin_addr));
            		}
            		else
            		{
                		printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                		close(fd);
                		return -1;
            		}
	
		//获取广播地址	
			if (!ioctl(fd, SIOCGIFBRDADDR, &buf[interfaceNum]))
            		{
                		snprintf(broadAddr, sizeof(broadAddr), "%s",
				(char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_broadaddr))->sin_addr));
		   		memcpy(&port_info->dev_info.dev_broad_addr,broadAddr,sizeof(broadAddr));
            		}
            		else
            		{
                		printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                		close(fd);
                		return -1;
            		}
		//获取子网掩码
			if (!ioctl(fd, SIOCGIFNETMASK, &buf[interfaceNum]))
            		{
                		snprintf(subnetMask, sizeof(subnetMask), "%s",
                            	(char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_netmask))->sin_addr));
		   		memcpy(&port_info->dev_info.dev_subnet_mask,subnetMask,sizeof(subnetMask));
            		}
            		else
            		{
                		printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                		close(fd);
               			return -1;
			}
		//获取接口MTU
			if(!ioctl(fd,SIOCGIFMTU,&buf[interfaceNum]))
			{
				port_info->dev_info.mtu = buf[interfaceNum].ifr_mtu;	
			}
			else
            		{
                		printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                		close(fd);
               			return -1;
			}
		//获取网卡参数
			get_adapter(&fd,port_info);

			snprintf(file_name,sizeof(file_name),"%s/log/port_%s",dir_name,port_info->dev_info.dev_name);

			//输出端口信息到文件
			f_port_info = fopen(file_name,"w+");
				
			if(f_port_info == NULL)	
			{
				perror("cannot open input file");
			}
				
			dump_dev_info(&port_info->dev_info, f_port_info);		
			fclose(f_port_info);
			port_info++;
			i++;
		}	
	}
	else
	{
		perror("ioctl error");
		close(fd);
		return -1;
	}

	host_info->port_count = i;
	close(fd);

	return 0;
}

void print_state_list(struct PORT_STATE_INDEX *head)
{
	struct PORT_STATE_INDEX *p;
	int i = 0;

	for(p = head;p != NULL;p = p->next)
	{
		printf("%d\t", i);
		printf("%s\n", p->dev_name);
		i++;
	}
}

struct PORT_STATE_INDEX* get_dest_ip(char *dir_name, struct PORT_INFO* port_info, struct HOST_INFO* host_info)
{
	struct DEST_INFO *pnew;
	struct PORT_STATE_INDEX* port_state_index;
	struct PORT_STATE_INDEX *pnew1;
	struct PORT_INFO* p;
	FILE *f_port_lst;
	char file_name[200];
	char ch;
	char string[1024];
	char dest_ip[16];
	int i;
	int j;
	int k;
	char trash_buf[32];
	char dev_name[16];
//flag to mark if the port dest ip list is created(0 for undone;1 for done)
	int flag_list[64];
//flag to mark if the device is found
	int flag_find = 0;

	memset(&flag_list,0,sizeof(flag_list));

	sprintf(file_name,"%s/port.lst",dir_name);
	printf("get the %s\n",file_name);
	f_port_lst = fopen(file_name,"r+");

	if(f_port_lst == NULL)
	{
		perror("cannot open file");
		return NULL;
	}

	port_state_index = create_state_list();

	while(1)
	{
		p = port_info;
		i = 0;
		fgets(string,1024,f_port_lst);

		printf("%s",string);

	//存放端口类型标志位（字符型）
		ch = string[0];

		i = i + 2;

		while(string[i] != '\t')
		{
			dev_name[i-2] = string[i];
			i++;
		}

		dev_name[i-2] = '\0';
		i++;

		printf("dev_name is %s\n",dev_name);

	//查找端口IP
		for(j = 0;j < host_info->port_count;j++)
		{
			if(strcmp(dev_name,p->dev_info.dev_name) == 0)
			{			
				flag_find = 1;
				p->port_type = (int)ch - 48;
				printf("match the device %s with type %d\n",dev_name,p->port_type);
				k = 0;

				if(p->port_type == 1)
				{
					pnew1 = (struct PORT_STATE_INDEX *)malloc(sizeof(struct PORT_STATE_INDEX));
					strcpy(pnew1->dev_name,dev_name);
					port_state_index = add_state_node(port_state_index,pnew1);
				}

				while(string[i] != '\n')
				{					
					dest_ip[k] = string[i];
					k++;
					i++;
				}

				dest_ip[k] = '\0';
				printf("dest ip is : %s\n",dest_ip);

			//apply for the space of pnew
				pnew = (struct DEST_INFO *)malloc(sizeof(struct DEST_INFO));
				strcpy(pnew->dest_ip,dest_ip);

				if(flag_list[j] == 0)
				{
					p->dest_info = create_dest_list();
					flag_list[j] = 1;
				}

				p->dest_info = add_dest_node(p->dest_info,pnew);
				break;
			}

			p++;
		}

		flag_find = 0;
		getc(f_port_lst);

		if(feof(f_port_lst))
		{
			break;
		}
		else
		{
			fseek(f_port_lst,-1,SEEK_CUR);
		}
	}
/*
	ch = fgetc(f_port_lst);

	while(ch != EOF)
	{
		if(ch == '\t')
		{
			dev_name[i] = '\0';
			i = 0;
		//search the source ip(linear search)
			for(j = 0;j < host_info.port_count;j++)
			{
				if(strcmp(dev_name,port_info[j].dev_info.dev_name) == 0)
				{
					printf("match the device %s\n",dev_name);
					flag_find = 1;
					ch = getc(f_port_lst);

				//get the dest ip
					while(ch != '\n')
					{
						if(ch == EOF)
						{
							break;
						}
						dest_ip[i] = ch;
						i++;
						ch = getc(f_port_lst);						
					}

					dest_ip[i] = '\0';
					i = 0;
					printf("dest ip is : %s\n",dest_ip);

				//apply for the space of pnew
					pnew = (struct DEST_INFO *)malloc(sizeof(struct DEST_INFO));
					strcpy(pnew->dest_ip,dest_ip);
					
					pnew1 = (struct PORT_STATE_INDEX *)malloc(sizeof(struct PORT_STATE_INDEX));
					strcpy(pnew1->dev_name,dev_name);

				//judge if the list have been create
					port_state_index = add_state_node(port_state_index,pnew1);
					
					if(flag_list[j] == 0)
					{
						port_info[j].dest_info = create_dest_list(pnew);
						flag_list[j] = 1;

						break;
					}	
					else
					{
						port_info[j].dest_info = add_dest_node(port_info[j].dest_info,pnew);

						break;
					}
				}
			}

		//	free(pnew);
		//	free(pnew1);

			if(flag_find)
			{
				flag_find = 0;
			}
			else
			{
				perror("no such device in this equipment");
				fgets(trash_buf,sizeof(trash_buf),f_port_lst);
			}
		}
		else
		{
			dev_name[i] = ch;
			i++;
		}		
		ch = fgetc(f_port_lst);
	}
*/
	fclose(f_port_lst);

	printf("test port_state_index : \n");
	print_state_list(port_state_index);

	return port_state_index;
}