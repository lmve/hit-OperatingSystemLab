# 进程追踪
## 实验要求
1. 基于模板 process.c 编写多进程的样本程序，实现如下功能：
   所有子进程都并行运行，每个子进程的实际运行时间一般不超过 30 秒；
   父进程向标准输出打印所有子进程的 id，并在所有子进程都退出后才退出；
2. 在 Linux0.11 上实现进程运行轨迹的跟踪。 
   基本任务是在内核中维护一个日志文件 /var/process.log，把从操作系统启动到系统关机过程中所有进程的运行轨迹都记录在这一 log 文件中。
3. 在修改过的 0.11 上运行样本程序，通过分析 log 文件，统计该程序建立的所有进程的等待时间、完成时间（周转间）和运行时间，然后计算平均等待时间，平均完成时间和吞吐量。可以自己编写统计程序，也可以使用 python 脚本——stat_log.py（在 /home/teacher/ 目录下） ——进行统计。
4. 修改 0.11 进程调度的时间片，然后再运行同样的样本程序，统计同样的时间数据，和原有的情况对比体会不同时间片带来的差异。
## 实验过程记录
### log 文件
- 此文件用于记录操作系统从启动到关机过程中所有进程的运行轨迹。
- path = /var/process.log
- 格式为 pid    state    time
  >pid 是进程id
  state 可以是新建(N)、就绪(J)、运行(R)、阻塞(W)、退出(E) 中的一种
  time 表示 state 切换时间，此时间不是物理时间而是系统的滴答时间
### process.c 
- 此文件用于模拟用户创建的不同进程
- function cpuio_bound(int last, int cpu_time, int io_time); 用于模拟不同的用户态进程
  >此函数按照参数占用cpu/io时间
  last: 进程实际占用cpu和io时间，不含在就绪队列中的时间，>= 0;
  cpu_time: 一次连续占用cpu的时间
  io_time: 一次连续占用io的时间
  if (last > cpu_time + io_time)多次调度
  所有时间单位均为秒
- process 通过系统调用fork创建多个进程，每个子进程通过调用cpuio_bound来模拟实际工作中的差异化工作，父进程等待所有子进程完成后才退出。
- 系统调用
  >1、fork()用于创建新的进程
  此系统调用将创建一个与原来进程几乎完全相同的进程，子进程默认情况下是完全复制父进程的数据段、代码段，但如果初始参数或传入变量不同，可以让它们做不同的工作。
  返回值:
  为负    进程创建失败
  为正    表明当前是父进程
  为零    表明当前是子进程
  2、wait()主动阻塞的系统调用（为等待所有子进程退出）
```
  process.c 具体实现
    /*
    *  oslab3/process.c
    *
    *  All child processes are parallel, and not over 30 sec
    *  father process print id of all child, and wait they exit
    *  (c) 2023 lucky ma 
    */
    #include <stdio.h>
    #include <unistd.h>
    #include <time.h>
    #include <sys/times.h>
    #include <sys/types.h>    
    #define HZ	100   
    void cpuio_bound(int last, int cpu_time, int io_time);
    int main(int argc, char* argv[])
    {
    	pid_t n_proc[10];
    	int i;
    	for(i=0;i<10;i++)
    	{
    		n_proc[i] = fork(); /* create child process */
    	        if(n_proc[i] == 0)  /* successfully */
    		{
    			cpuio_bound(20, 2*i, 20-2*i);/* hand out task */
    			return 0;
    		}
    		else if (n_proc[i] < 0 )
    		{/*failed to create  */
    		        printf("Failed to fork child process %d\n",i+1);
    			return -1;
    		}
    	}
    
    	/* print child id */ 
    	for(i=0; i<10; i++){
    		printf("Child PID: %d\n",n_proc[i]);
    	}
    
    	/* wait for child */
    	wait(&i);
    	return 0;
    }   
    /*
     * Task simulation
     * last sum of cpu & i/o time,    >= 0
     * cpu_time  once use cpu's time  >= 0
     * io_time   once i/o task's time >= 0
     * if (last > cpu plus io) it's means run many times 
     */
    
    void cpuio_bound(int last, int cpu_time, int io_time)
    {
    	struct tms start_time, current_time;
    	clock_t utime, stime;
    	int sleep_time;
    
    	while (last > 0)
    	{
    		/* CPU Burst */
    		times(&start_time);
    		do
    		{
    			times(&current_time);
    			utime = current_time.tms_utime - start_time.tms_utime;
    			stime = current_time.tms_stime - start_time.tms_stime;
    		} while ( ( (utime + stime) / HZ )  < cpu_time );
    		last -= cpu_time;
    
    		if (last <= 0 )
    			break;
    
    		/* IO Burst */
    		sleep_time=0;
    		while (sleep_time < io_time)
    		{
    			sleep(1);
    			sleep_time++;
    		}
    		last -= sleep_time;
    	}
    }
```
### log 文件何时打开
  为了尽早开始记录，因该在内核启动时就打开log文件，即操作系统转移到用户态执行时，也就是创建一号进程init()之前。
  ```
  >init/main.c
  ...
  move_to_user_mode(); /* 在此之后打开log文件 */
  if(!fork()){
    $~ ~$ init();
  }
  ...  
  move_to_user_mode();
/***************添加开始***************/
setup((void *) &drive_info);
// 建立文件描述符0和/dev/tty0的关联
(void) open("/dev/tty0",O_RDWR,0);
//文件描述符1也和/dev/tty0关联
(void) dup(0);
// 文件描述符2也和/dev/tty0关联
(void) dup(0);
(void) open("/var/process.log",O_CREAT|O_TRUNC|O_WRONLY,0666);
/***************添加结束***************/
if (!fork()) {        /* we count on this going ok */
    init();
}
//……
```
文件描述符0、1、2 分别是标准输入stdin、标准输出stdout、标准错误stderr。
前四句原本在init()中初始化，现在提前了，需要将init()中的注释掉。

### 实现写文件的函数fprintk
- 为什么要实现此系统调用？
  > 进程的调度是在内核态下，而此时write()功能失效，就像printf()只能在用户态下使用，内核态下需要使用printk()是一样的道理。参考printk()、sys_write()。
  代码如下（功能与printk()相似，放在kernel/printk.c中）：
  ```
  #include <linux/sched.h>
  #include <sys/stat.h>/* adding fprint to kernel space */
  static char logbuf[1024];
  int fprintk(int fd,const char *fmt, ...){
        va_list args;
        int count;
        struct file *file;
        struct m_inode *inode;
        va_start(args,fmt);
        count = vsprintf(logbuf, fmt, args);
        va_end(args);
        if(fd < 3)   /* if output to stdout | stderr , just calling sys_write */
        {
                __asm__("push %%fs\n\t"
                                       "push %%ds\n\t"
                        "pop %%fs\n\t"
                        "pushl %0\n\t"
                        "pushl $logbuf\n\t"
                        "pushl %1\n\t"
                        "call sys_write\n\t"
                        "addl $8,%%esp\n\t"
                        "popl %0\n\t"
                        "pop %%fs"
                        ::"r" (count), "r" (fd)
                        :"ax","cx","dx");
        }
        else
        {
                if(!(file = task[0]->filp[fd])) /*get file handle from process-0 file description list */
                return 0;
                inode = file->f_inode;
                __asm__("push %%fs\n\t"
                        "push %%ds\n\t"
                        "pop %%fs\n\t"
                        "pushl %0\n\t"
                        "pushl $logbuf\n\t"
                        "pushl %1\n\t"
                        "pushl %2\n\t"
                        "call file_write\n\t"
                        "addl $12, %%esp\n\t"
                        "popl %0\n\t"
                        "pop %%fs"
                        ::"r" (count), "r" (file), "r" (inode)
                        :"ax", "cx", "dx");
        }
        return count;
 }
 ```
 jiffies 为滴答次数
 记录了从开机到当前时间的时钟中断发生的次数，在kernel/sched.c中定义。

### 状态切换
- 找到状态切换点、并记录在log文件中。
- 在此要熟悉进程从创建到退出的全过程。
- linux-0.11 支持四种状态转移：就绪到运行、运行到就绪、运行到睡眠、睡眠到就绪。以及新建与退出。
- fork.c、sched.c、exit.c
1. 记录创建的新进程
  > 从fork()开始追踪、fork功能由内核中的sys_fork()实现，由该函数在kernel.system_call.s中实现可知，由copy_process()实现进程创建
```
  sys_fork:
        call find_empty_process
        testl %eax,%eax
        js 1f
        push %gs
        pushl %esi
        pushl %edi
        pushl %ebp
        pushl %eax
        call copy_process
        addl $20,%esp
1:      ret
```
  > 创建好进程后就可以记录了**新建（N）**
```
        p->leader = 0;          /* process leadership doesn't inherit */
        p->utime = p->stime = 0;
        p->cutime = p->cstime = 0;
        p->start_time = jiffies;
        /* * * * * now the new process is be created * * * * * * */
        /* write down the message */
        fprintk(3,"%d\t%c\t%d\n",p->pid,'N', jiffies);
        /* work is down */
```
    即：记录好创建时间后就可以写入log文件

  > 之后新进程将转为**就绪态（J）**
```
          p->state = TASK_RUNNING;        /* do this last, just in case */
        /*
         * write down the change for every new process
         * new to ready
         * lm           2023
         */
        fprintk(3,"%d\tJ\t%d\n",p->pid,jiffies);
```
    即：当前进程转为就绪态就可以写入log文件
2. 修改sched.c
> 位于kernel/sched.c 文件中的sleep_on()与interrutible_sleep_on()会让当前进程进入睡眠状态。
```
   	tmp = *p;
	*p = current;
	current->state = TASK_UNINTERRUPTIBLE;  /*set the sleep process to non interruptible*/
	/* process's state was changed */
	fprintk(3, "%ld\t%c\t%ld\n", current->pid, 'W', jiffies);
	/* the end */

	schedule();
    	if (tmp){
		tmp->state=0;
	        /* process's state was changed  already wake up */
		fprintk(3, "%ld\t%c\t%ld\n", tmp->pid, 'J', jiffies);
		/* print message end in 3-processTack */
	}
```
```
    interrutible_sleep_on()
   	tmp=*p;
	*p=current;
repeat:	current->state = TASK_INTERRUPTIBLE;
	/* process's state was changed */
        fprintk(3, "%ld\t%c\t%ld\n", current->pid, 'W', jiffies);
        /* the end */
	schedule();

	if (*p && *p != current) {
		(**p).state=0;
		goto repeat;
       		/* process's state was changed  already wake up */
                fprintk(3, "%ld\t%c\t%ld\n", tmp->pid, 'J', jiffies);
                /* print message end in 3-processTack */
	}
```
```  
   	if (tmp){
		tmp->state=0;
		/* process's state was changed  already wake up */
                fprintk(3, "%ld\t%c\t%ld\n", tmp->pid, 'J', jiffies);
                /* print message end in 3-processTack */
	}
```
```
    就绪与运行态的转换是通过schedule()
   		 /* state is ready */
		 fprintk(3, "%ld\t%c\t%ld\n", (*p)->pid, 'J', jiffies);
		 /* the end */
         /* those code add by lm before switch_to*/
	      if (task[next]->pid != current->pid) {
		    if (current->state == TASK_RUNNING) {
			  fprintk(3, "%ld\t%c\t%ld\n", current->pid, 'J', jiffies);
		}
		  fprintk(3, "%ld\t%c\t%ld\n", task[next]->pid, 'R', jiffies);
	}

```
```
    主动睡眠 sys_pause() 和 sys_waitpid()
   	current->state = TASK_INTERRUPTIBLE;
	/* father should wait son finish his task */
	fprintk(3, "%ld\t%c\t%ld\n", current->pid, 'W', jiffies);
	/* end  */

    睡眠到就绪态依靠 wake_up()
   		(**p).state=0;
		/* process is wake up */
		fprintk(3, "%ld\t%c\t%ld\n", (*p)->pid, 'J', jiffies);
		/* the end */
```
3. exit.c
   当一个进程运行结束或中途停止，那么内核需要释放此进程所占用的资源。当用户程序调用exit()时会执行内核函数do_exit()，此函数首先释放进程代码段、数据段占用的内存页面，关闭进程打开的所有文件等操作。
```
   	if (current->leader)
		kill_session();
	current->state = TASK_ZOMBIE;
	/* process exit */
	fprintk(3, "%ld\t%c\t%ld\n", current->pid, 'E', jiffies);
	/* end */
	current->exit_code = code;
	tell_father(current->father);
	schedule();
	return (-1);	/* just to suppress warnings */

  	if (flag) {
		if (options & WNOHANG)
			return 0;
		current->state=TASK_INTERRUPTIBLE;
		/*change to sleep */
		fprintk(3, "%ld\t%c\t%ld\n", current->pid, 'W', jiffies);
		/* end */
		schedule();
   }
```



   


                               





