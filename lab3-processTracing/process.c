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
