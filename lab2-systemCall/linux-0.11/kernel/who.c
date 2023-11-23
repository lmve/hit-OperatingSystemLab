/*
 *  kernel/who.c
 *
 *  (c) lm     2023
 *  system_call lab
 */
#include <asm/segment.h>
#include <errno.h>
#include <linux/kernel.h>

#define NAMELIMIT  23        /* define the limit of name */
char username[NAMELIMIT+1];  /* argument buffer */

int sys_iam(const char* name)
{
	/*
	 * copy name from user to kernel,limit size 23
	 * if name's length over size return = -1 && errno = EINVAL 
	 *
	 */
	unsigned int len = 0;
	int i = 0;
	int res = -1;
	printk("now we are in kernel space\n");
	while(get_fs_byte(name+len) != '\0')
		len++;           /* count size*/
	if(len <= NAMELIMIT){
		printk("Copying from user to kernel ...\n");
		for(i=0; i<len; i++){
			username[i] = get_fs_byte(name+i);
		}
		printk("DOWN!\n");
		username[i] = '\0';
		printk("%s\n",username);
		res = len;

	}else{
		printk("ERROR, user name over limit size %d\n",23);
		res = -(EINVAL);
	}
	return res;
}


int sys_whoami(char* name,unsigned int size)
{

	/*
	 * copy name from kernel to user,friendly to do this job.
	 * if size simaller than size, return -1 && errno = EINVAL
	 */
	unsigned int len = 0;
	int i = 0;
	int res = -1;
	printk("kernel whoami\n");
	while(username[len] != '\0')
		len++;
	if(len < size){
		printk("Copying from kernel to user...\n");
		for(i=0; i<len; i++){
			put_fs_byte(username[i],name+i);
		}
		printk("Down!\n");
		put_fs_byte('\0',name+i);
		res = len;
	}else{
		printk("ERROR, kernel's name length is longer then %d\n",size);
		res = -(EINVAL);
	}
	return res;
}



