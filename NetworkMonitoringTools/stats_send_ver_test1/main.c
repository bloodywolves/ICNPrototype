#include "main.h"

//设备名称
char host_name[32];
//目录名称
char dir_name[100];
//子进程id
pid_t port_child_pid[64];
int child_count = 0;
//定义设备信息结构体
struct HOST_INFO host_info;
struct PORT_INFO port_info[64];
struct PORT_STATE_INDEX* port_state_index;
struct PORT_INDEX* port_index;
static struct PORT_INDEX share_struct;

//主定时器结构体
struct _timer_manage timer_manage;

int port_child_proc(int* i)
{
	char file_name[200];
        int counter;

//创建端口子进程
	port_child_pid[*i] = fork();
//子进程程序
	if(port_child_pid[*i] == 0)
	{	
		printf("father %d , child NO:%d = %d\n",getppid(),*i,getpid());
	//用于输出该端口信息的文件名
		memset(&file_name,0,sizeof(file_name));
		snprintf(file_name,sizeof(file_name),"%s/log/port_%s",dir_name,port_info[*i].dev_info.dev_name);

	//获取端口动态数据	
		get_port_info(&port_info[*i], file_name, i);
		printf("port %s exit\n",port_info[*i].dev_info.dev_name);
		exit(1);
	}
	else if(port_child_pid[*i] < 0)
	{
		perror("fork create");
		exit(-1);
	}

	child_count++;
	return 0;
}

int port_kill_proc(int* i)
{
	int ret = -1;

	while(ret)
	{
		ret = kill(port_child_pid[*i], SIGKILL);
	}

	port_child_pid[*i] = 0;
//	waitpid(port_child_pid[*i], NULL, 0);
	child_count--;
	return 0;
}

static int share_handle(void)
{
	int fd;
	pthread_rwlockattr_t attr;

//clean the memory region
	shm_unlink(host_name);
//create the share region
	fd = shm_open(host_name,O_RDWR | O_CREAT |O_EXCL,0777);
	ftruncate(fd,sizeof(struct PORT_INDEX));
//write to the share region
	write(fd,&share_struct,sizeof(struct PORT_INDEX));
//map the share region
	port_index = mmap(NULL,sizeof(struct PORT_INDEX),PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
	close(fd);
//set the mutex of the port_index
//	sem_init(&port_index->mutex,1,1);
//set the read&write lock
	pthread_rwlockattr_init(&attr);
	pthread_rwlockattr_setpshared(&attr,PTHREAD_PROCESS_SHARED);
	pthread_rwlock_init(&port_index->lock,&attr);

	printf("end of malloc\n");
	return 0;
}

void sig_func(int signo)
{
    int id;
    char tstr[200];
    int i;
    time_t t;

    t = time(NULL);

    if(timer_manage.timer_info != NULL)
    {
        timer_manage.timer_info->elapse = difftime(timer_manage.timer_info->time, t);

        //容错
        while(timer_manage.timer_info->elapse < 0)
        {
        	printf("timer error : %ds", timer_manage.timer_info->elapse);
       		id = timer_manage.timer_info->timer_id;
			timer_manage.timer_info = timer_manage.timer_info->timer_proc(timer_manage.timer_info, &id);

			//定时器已空
			if(timer_manage.timer_info == NULL)
			{
				break;
			}
			
			timer_manage.timer_info->elapse = difftime(timer_manage.timer_info->time, t);
        }

        while(timer_manage.timer_info->elapse == 0)
        {
			id = timer_manage.timer_info->timer_id;
			timer_manage.timer_info = timer_manage.timer_info->timer_proc(timer_manage.timer_info, &id);

			//定时器已空
			if(timer_manage.timer_info == NULL)
			{
				break;
			}
        }
    }
}

//设定主定时器
/* success, return 0; failed, return -1 */
int init_mul_timer(struct _timer_manage *timer_manage)
{
    int ret;
    
    if( (timer_manage->old_sigfunc = signal(SIGALRM, sig_func)) == SIG_ERR)
    {
        return (-1);
    }
    timer_manage->new_sigfunc = sig_func;
    
    timer_manage->value.it_value.tv_sec = 1;
    timer_manage->value.it_value.tv_usec = 0;
    timer_manage->value.it_interval.tv_sec = 1;
    timer_manage->value.it_interval.tv_usec = 0;
    ret = setitimer(ITIMER_REAL, &timer_manage->value, &timer_manage->ovalue); 
    
    return (ret);
}

//清除主定时器
int destroy_mul_timer(struct _timer_manage *timer_manage)
{
    int ret;
    
    if( (signal(SIGALRM, timer_manage->old_sigfunc)) == SIG_ERR)
    {
        return (-1);
    }

    memset(timer_manage, 0, sizeof(struct _timer_manage));
    ret = setitimer(ITIMER_REAL, &timer_manage->ovalue, &timer_manage->value);
    if(ret < 0)
    {
        return (-1);
    } 
    
    printf("destroy multi timer\n");
    return ret;
}

static void exit_proc(int sig)
{
	pthread_rwlock_destroy(&port_index->lock);
	shm_unlink(host_name);
	destroy_mul_timer(&timer_manage);

	printf("\nthe program exit\n");
	exit(0);
}

//本地端口监听线程
static void* thread_local_listen(void* arg)
{
	struct _timer_manage* timer_manage;
	int comm_fd, sin_len;
	struct sockaddr_in sin;
	int ret;
	int type = 0;
	void* local_ret;
	int send_len = 1280;

	comm_fd = socket(AF_INET, SOCK_DGRAM, 0);

	if(comm_fd == -1)
	{
		perror("create local socket failed");
		pthread_exit(NULL);
	}

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_port = htons(LOCAL_PORT);
	sin_len = sizeof(sin);

	ret = bind(comm_fd, (struct sockaddr*)&sin, sizeof(sin));

	if(ret == -1)
	{
		perror("bind failed");
		pthread_exit(NULL);
	}

	timer_manage = (struct _timer_manage*)arg;

	while(1)
	{
		local_ret = local_listen(&comm_fd, &sin, timer_manage->timer_info, port_info, &type);
		
		switch(type)
		{
			case 1:{
					timer_manage->timer_info = local_ret;
					print_time();
					print_timer_manage(timer_manage->timer_info);
					break;	
				}
			case 3:{
					sendto(comm_fd, local_ret, send_len, 0, (struct sockaddr*)&sin, sizeof(sin));
					free(local_ret);
					break;
				}
			default:break;
		}	
	}

	close(comm_fd);
	pthread_exit(NULL);
}

int main(int argc,char *argv[])
{
	//获取用户输入信息
	int ret;
	char file_name[200];
	char command[200];
	FILE *cstream;
	int opt;
	
	struct option longopts[] = {
								{"help",0,0,'h'}	
								};
	
	memset(&dir_name,'\0',sizeof(dir_name));
	memset(&host_name,'\0',sizeof(host_name));

	while((opt = getopt_long(argc,argv,"n:d:h",longopts,NULL)) != -1) 
    {   
        switch(opt)
        {   
        //set the host name
			case 'n':{
						strcpy(host_name,optarg);
						//printf("optind = %d\n",optind);
						break;
					}
		//set the directory name
			case 'd':{
						strcpy(dir_name,optarg);
						break;
					}
		//help option
			case 'h':{
						printf("Usage : ./module_stats -n [the host name]\n\t\t\t-d [the user directory]\n");
						exit(0);
					}   
		}   
	}   

	if(argc < 3)
	{
		printf("Usage : ./module_stats -n [the host name]\n\t\t\t-d [the user directory]\n");
		exit(0);
	}
	else if(host_name[0] == '\0')
	{
		printf("Usage : at least input the host name by\n\t./module_stats -n [the host name]\n");
		exit(0);
	}
	
	if(dir_name[0] == '\0')
	{
		printf("default user directory\n");		
	//set the directory name
		sprintf(dir_name,"/home",host_name);
	}

//create the directory
	sprintf(command,"mkdir %s/log",dir_name);

	if((cstream = popen(command,"r")) == NULL)
	{
		perror("create the directory error");
	}

	pclose(cstream);

/*
	ret = mkdir("/home/kamiuck/myworkspace/mininet-host/",0777);
	if(ret != 0)
	{
		perror("create the directory error");
	}
*/

	memset(&host_info,0,sizeof(host_info));
	memset(&port_info,0,sizeof(port_info));
	memset(&share_struct,0,sizeof(share_struct));

//获取设备信息	
	ret = get_host_info(&host_info, host_name);

	if(ret < 0)
	{
		perror("get host information error!");
	}

//获取网口数量及各网卡基本信息
	ret = get_port_index(port_info, &host_info, dir_name);

	if(ret < 0)
	{
		perror("get port information error!");
	}

	sprintf(file_name,"%s/log/host_info",dir_name);

//输出设备信息
	FILE *f_host_info;

	f_host_info = fopen(file_name,"w+");

	if(f_host_info == NULL)
	{
		perror("cannot open input host_info file");
		exit(0);
	}

	dump_host_info(&host_info,f_host_info);
	fclose(f_host_info);

	//时间同步
	/*
	char rm_ip[32];
	strcpy(rm_ip,RM_IP);
	ntp_sync(rm_ip);
	*/

//get the dest ip for link test,and build the port_state_index
//	printf("begin to fetch the port.lst\n");
//create a null port_state_index list
	//port_state_index = get_dest_ip(dir_name, port_info, &host_info);

	strncpy(share_struct.host_name,host_name,sizeof(host_name));
	strncpy(share_struct.dir_name,dir_name,sizeof(dir_name));
/*
	printf("port_state_index :\n");
	display_state_list(port_state_index,&share_struct);
*/
//create the pipe file to share the port_state_index
	/*
	char fifo_fd[100];
	sprintf(fifo_fd,"%s/log/fifo",dir_name);
	if(mkfifo(fifo_fd,0666) < 0 && (errno != EEXIST))
	{
		perror("create fifo error");
	}
*/
	memset(&port_child_pid,0,sizeof(port_child_pid));

	share_handle();

//创建端口数据库
	/*
	int count;
	count = host_info.port_count;

	do
	{
		count--;
//		mysql_create_port_database(&port_info[count], &count);
	}
	while(count > 0);
*/
//定时器部分
	memset(&timer_manage, 0, sizeof(struct _timer_manage));
    ret = init_mul_timer(&timer_manage);

	timer_manage.timer_info = (struct _timer_info*)malloc(sizeof(struct _timer_info));
    timer_manage.timer_info = NULL;

	pthread_t pthread_local_listen;

	if(pthread_create(&pthread_local_listen, NULL, thread_local_listen, &timer_manage) != 0)
	{	
		perror("Creation of parse thread failed.");
		return -1;
	}	

	int test;

	for(test = 0; test < host_info.port_count; test++)
	{
		port_child_proc(&test);
	}

	signal(SIGINT,exit_proc);
//保持父进程运行,并监控子进程运行状态
	int status_child;
	int i;
	pid_t pid_child;

//	signal(SIGCHLD,sub_quit);
	while(1)
	{
		printf("child count : %d\n", child_count);
		while(!child_count);

		pid_child = waitpid(-1, &status_child, WUNTRACED);
		child_count--;
	//判断子进程是否暂停执行，终止暂停的进程并恢复
		if(WIFSTOPPED(status_child))
		{
			printf("child stoped with status %d\n", WSTOPSIG(status_child));
			for(i = 0; i < 64; i++)
			{
				if(port_child_pid[i])
				{
					if(port_child_pid[i] == pid_child)
					{
						port_kill_proc(&i);
						waitpid(port_child_pid[i], NULL, 0);
						port_child_proc(&i);
						break;
					}
				}
			}
		}
	//判断是否正常退出
		else if(WIFEXITED(status_child))
		{
			printf("normal exit\n");

			for(i = 0; i < 64; i++)
			{
				if(port_child_pid[i])
				{
					if(port_child_pid[i] == pid_child)
					{
						port_child_pid[i] = 0;
						break;
					}
				}
			}
		}
	//判断是否由于信号退出,对父进程主动杀死的子进程不予理会
		else if(WIFSIGNALED(status_child) && WTERMSIG(status_child) == 9)
		{
			printf("child killed\n");
		}
	//对异常退出的子进程进行恢复
		else
		{
			printf("abnormal exit with status :%d\n", WEXITSTATUS(status_child));
			
			for(i = 0; i < 64; i++)
			{
				if(port_child_pid[i])
				{
					if(port_child_pid[i] == pid_child)
					{
						port_child_proc(&i);
						break;
					}
				}
			}
		}
	} 

//	sem_destroy(&port_index->mutex);
	pthread_rwlock_destroy(&port_index->lock);
	shm_unlink(host_name);
	destroy_mul_timer(&timer_manage);

	printf("\nthe program exit\n");

	exit(ret);
}
