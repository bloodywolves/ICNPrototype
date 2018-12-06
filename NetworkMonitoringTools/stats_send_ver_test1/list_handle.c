#include "main.h"

struct DEST_INFO *create_dest_list(void)
{
	printf("create a new ip list\n");
	struct DEST_INFO *head;
	head = (struct DEST_INFO *)malloc(sizeof(struct DEST_INFO));
	head = NULL;
	return head;
}

struct DEST_INFO *add_dest_node(struct DEST_INFO *head,struct DEST_INFO *pnew)
{
	struct DEST_INFO *p;
	p = head;

//if it's a blank list,set the pnew as head node
	if(head == NULL)
	{
		printf("it's a blank ip list\n");
		head = pnew;
		pnew->linkloss_trap_flag = 0;
		pnew->linkdelay_trap_flag = 0;
		pnew->next = NULL;
	}
	else
	{
	//while the element is not the last element
		while(p->next != NULL)
		{
			if(strcmp(p->dest_ip,pnew->dest_ip) == 0)
			{
				printf("the ip is repeated\n");
				return head;
			}

			p = p->next;
		}

		//judge the last element
		if(strcmp(p->dest_ip,pnew->dest_ip) != 0)
		{
			printf("insert a new ip\n");
			p->next = pnew;
			pnew->linkloss_trap_flag = 0;
			pnew->linkdelay_trap_flag = 0;
			pnew->next = NULL;
		}
	}
	return head;
}

void display_dest_list(struct DEST_INFO *head)
{
	struct DEST_INFO *p;
	for(p = head;p != NULL;p = p->next)
	{
		printf("%s\n",p->dest_ip);
	}
}

struct PORT_STATE_INDEX* create_state_list(void)
{
	printf("create a new device list\n");
	struct PORT_STATE_INDEX *head;
	head = (struct PORT_STATE_INDEX *)malloc(sizeof(struct PORT_STATE_INDEX));
	head = NULL;
	return head;
}

struct PORT_STATE_INDEX *add_state_node(struct PORT_STATE_INDEX *head,struct PORT_STATE_INDEX *pnew)
{
	struct PORT_STATE_INDEX *p;
	p = head;
	int repetition = 0;

//if it's a blank list,set the pnew as head node
	if(head == NULL)
	{
		printf("it's a blank device list\n");
		head = pnew;
		pnew->trap_level = 0;
		pnew->trap_state = 0;
		pnew->next = NULL;
	}
	else
	{
		while(p->next != NULL)
		{
			if(strcmp(p->dev_name,pnew->dev_name) == 0)
			{
				repetition = 1;
				break;
			}
			p = p->next;
		}

		while(repetition == 0)
		{
			if(strcmp(p->dev_name,pnew->dev_name) == 0)
			{
				printf("the device is repeated\n");
				break;
			}

			printf("insert a new device\n");
			p->next = pnew;
			pnew->trap_level = 0;
			pnew->trap_state = 0;
			pnew->next = NULL;
			repetition = 1;
		}
	}
	return head;
}

void display_state_list(struct PORT_STATE_INDEX *head,struct PORT_INDEX *share_struct)
{
	struct PORT_STATE_INDEX *p;
	int i = 0;

	for(p = head;p != NULL;p = p->next)
	{
		share_struct->port_count++;
		strcpy(share_struct->port[i].dev_name,p->dev_name);
		printf("%s\n",share_struct->port[i].dev_name);
		i++;
	}
}

struct PORT_INDEX_INFO *search_port_index(struct PORT_INDEX *port,char *dev_name)
{
	struct PORT_INDEX_INFO *p;
	int i;
//	printf("the device name to match is %s\n",dev_name);
	for(i = 0;i < port->port_count;i++)
	{
		if(strcmp(port->port[i].dev_name,dev_name) == 0)
		{
			p = &port->port[i];
			return p;
		}
	}
	
	return NULL;
}


