#include "main.h"

static struct _timer_info* delete_repete_timer(struct _timer_info* p)
{
    struct _timer_info* ret;
    //第一个元素
    if(p->pre == NULL)
    {
        //只有一个元素
        if(p->next == NULL)
        {
            free(p);
            return NULL;
        }
        else
        {
            p->next->pre = NULL;
            ret = p->next;
            free(p);
            return ret;
        }
    }
    //中间元素
    else if(p->next != NULL)
    {
        p->next->pre = p->pre;
        p->pre->next = p->next;
        ret = p->next;
        free(p);
        return ret;
    }
    //末尾元素
    else
    {
        p->pre->next = NULL;
        free(p);
        return NULL;
    }
}

//升序插入子定时器，并且删除重复的定时器
static struct _timer_info* insert_a_timer(struct _timer_info* head, struct _timer_info* pnew)
{
    struct _timer_info* p;
    struct _timer_info* p_end;//末尾判定指针
    time_t t;
    int elapse;
    int insert_flag = 0;
    int repete_flag = 0;
    int i;
    
    t = time(NULL);
    elapse = difftime(pnew->time, t);

    if(elapse < 0)
    {
        printf("the time is past\n");
        return head;
    }

    //空链表直接插入
    if(head == NULL)
    {
        head = pnew;
        pnew->pre = NULL;
        pnew->next = NULL;
        pnew->elapse = difftime(pnew->time, t);
        return head;
    }

    //判断头节点是否重复
    if(head->timer_id == pnew->timer_id)
    {
        printf("%d match %d\n", head->timer_id, pnew->timer_id);
        if(head->flag == pnew->flag)
        {
            head = delete_repete_timer(head);
            printf("delete_timer first\n");
            repete_flag = 1;

            if(head == NULL)
            {
                head = pnew;
                pnew->pre = NULL;
                pnew->next = NULL;
                pnew->elapse = difftime(pnew->time, t);
                return head;
            }
        }
    }

    p = head;
    elapse = difftime(p->time, pnew->time);

    //第一个元素
    if(elapse >= 0)
    {
        head = pnew;
        pnew->pre = NULL;
        pnew->next = p;
        p->pre = pnew;
        pnew->elapse = difftime(pnew->time, t);
        p->elapse = elapse;
        insert_flag = 1;
    }

    p_end = p;
    p = p->next;

    while(p != NULL)
    {
        if(insert_flag == 1 && repete_flag == 1)
        {
            return head;
        }
        else if(!repete_flag)
        {
            if(p->timer_id == pnew->timer_id)
            {
                if(p->flag == pnew->flag)
                {
                    p = delete_repete_timer(p);
                    printf("delete middle\n");
                    repete_flag = 1;

                    if(p == NULL)
                    {   
                        break;
                    }
                }
            }
        }

        //如果元素还没被插入过，就进行判定前插操作
        if(!insert_flag)
        {
            elapse = difftime(p->time, pnew->time);

            if(elapse >= 0)
            {
                p->pre->next = pnew;
                pnew->next = p;
                pnew->pre = p->pre;
                p->pre = pnew;
                p->elapse = elapse;
                pnew->elapse = difftime(pnew->time, pnew->pre->time);
                insert_flag = 1;
            }
        }

        p_end = p;
        p = p->next;
    }

    if(!insert_flag)
    {
        //末尾插入
        p = pnew;
        pnew->pre = p_end;
        p_end->next = pnew;
        pnew->elapse = difftime(pnew->time, p_end->time);
        pnew->next = NULL;
    }

    return head;
}

//清除子定时器
static struct _timer_info* del_a_timer(struct _timer_info* head, int value)
{
    struct _timer_info* p;

    p = head;

    if(head == NULL)
    {
        printf("no timer %d\n", value);
        return NULL;
    }

    //删除第一个元素
    if(p->timer_id == value)
    {
        if(p->next == NULL)
        {
            printf("delete the timer %d\n", value);
            return NULL;
        }

        head = p->next;
        head->pre = NULL;
        free(p);
        printf("delete the timer %d\n", value);
        return head;
    }

    if(p->next != NULL)
    {
        p = p->next;
    }

    //删除中间元素
    while(p->next != NULL)
    {
        if(p->timer_id == value)
        {
            p->next->pre = p->pre;
            p->pre->next = p->next;
            free(p);
            printf("delete the timer %d\n", value);
            return head;
        }

        p = p->next;
    }

    //删除最后一个元素
    if(p->timer_id == value)
    {
        p->pre->next = NULL;
        free(p);
        printf("delete the timer %d\n", value);
        return head;
    }

    printf("no timer %d\n", value);
    return head;
}

//获取定时器id
static int set_timer_id(struct TIMER_MESSAGE* timer_message, struct PORT_INFO* port_info)
{
	int i = 0;

	while(port_info->port_type)
	{
		if(strcmp(timer_message->ip, port_info->dev_info.dev_ip) == 0)
		{
			return i;
		}

		i++;
		port_info++;
	}

	return -1;
}

//设定子定时器执行启动函数
struct _timer_info* timer_proc_launch(struct _timer_info* timer_info, int* num)
{
	print_time();
    printf("timer_proc%d is launched.\n", *num);
    port_child_proc(num);
    timer_info = del_a_timer(timer_info, *num);

    if(timer_info == NULL)
    {
        return NULL;
    }

    return timer_info;
}

//设定子定时器执行终止函数
struct _timer_info* timer_proc_end(struct _timer_info* timer_info, int* num)
{
	print_time();
    printf("timer_proc%d is end.\n", *num);
    port_kill_proc(num);
    timer_info = del_a_timer(timer_info, *num);

    if(timer_info == NULL)
    {
        return NULL;
    }

    return timer_info;
}

//设定子定时器
struct _timer_info* set_child_timer(struct _timer_info* timer_info, struct TIMER_MESSAGE* timer_message, struct PORT_INFO *port_info)
{
    struct _timer_info* pnew;

   	pnew = (struct _timer_info*)malloc(sizeof(struct _timer_info));

   	pnew->timer_id = set_timer_id(timer_message, port_info);
    pnew->flag = timer_message->flag;

   	if(pnew->timer_id == -1)
   	{
   		printf("error: no such timer ip\n");
   		return timer_info;
   	}

	if(pnew->flag)
	{
		pnew->timer_proc = timer_proc_end;
	}
	else
	{
		pnew->timer_proc = timer_proc_launch;
	}

	pnew->t = timer_message->t;
    pnew->time = mktime(&pnew->t);
    printf("ready to insert a timer %d\n", pnew->timer_id);
    timer_info = insert_a_timer(timer_info, pnew);

    return timer_info;
}