## 系统调用
#### 实验任务
在 Linux 0.11 上添加两个系统调用，并编写两个简单的应用程序测试它们。
（1）iam()
第一个系统调用是 iam()，其原型为：
```
   int iam( const char* name);
```

完成的功能是将字符串参数 name 的内容拷贝到内核中保存下来。要求 name 的长度不能超过 23 个字符。返回值是拷贝的字符数。如果 name 的字符个数超过了 23，则返回 “-1”，并置 errno 为 EINVAL。在 kernal/who.c 中实现此系统调用。
（2）whoami()
第二个系统调用是 whoami()，其原型为：
```
int whoami(char* name, unsigned int size);
```
它将内核中由 iam() 保存的名字拷贝到 name 指向的用户地址空间中，同时确保不会对 name 越界访存（name 的大小由 size 说明）。返回值是拷贝的字符分区 操作系统 的第23 页   （name 的大小由 size 说明）。返回值是拷贝的字符数。如果 size 小于需要的空间，则返回“-1”，并置 errno 为 EINVAL。也是在 kernal/who.c 中实现。
（3）测试程序
运行添加过新系统调用的 Linux 0.11，在其环境下编
写两个测试程序 iam.c 和 whoami.c。最终的运行结
果是：
$ ./iam lizhijun
 $ ./whoami
 lizhijun

#### 系统调用剖析
Linux 系统调用有两种，基于glibc（GNU发布的libc库版本）syscall()**间接方式**,以及废弃的宏——_syscall[0-6]**直接方式**
本次实验采用宏的方式实现系统调用
- 具体分析
  _syscall 是系统调用“族”，所以图中用_syscallX来表示它们，其中的X表示系统调用中的参数个数，
其原型是_syscallX(type,name,type1,arg1,type2,arg2,…)。_syscallX 是用宏来实现的，根据系统调用中参数
个数、类型及返回值的不同，这里共有 7 个不同的宏，分别是_syscall[0-6]，因此，对于参数个数不同的
系统调用，需要调用不同的宏来完成。图中对参数已有介绍，type 是系统调用的返回值类型，name 是系
统调用名称（字符串），最后通过宏替换会转换成数值型的子功能号，typeN和argN配对出现，分别表示
参数的类型及变量名。
```
#define _syscall3(type, name, type1, arg1, type2, arg2, type3, arg3) \ 
2 type name(type1 arg1, type2 arg2, type3 arg3) { \ 
3   long __res; \ 
4  
__asm__ volatile ("push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx" \ 
5             : "=a" (__res) \ 
6             : "0" (__NR_##name),"ri" ((long)(arg1)),"c" ((long)(arg2)), \ 
7               "d" ((long)(arg3)) : "memory"); \ 
8   
__syscall_return(type,__res); \ 
9 } 
```
'3' 代表有三个参数，{type,name}系统调用返回值类型、系统调用名（最终宏展开为系统调用号）{type,arg...}是参数类型及参数名。
一般Linux系统下的Linux中的系统调用是用寄存器来传递参数的，这些参数需要按照从左到右的顺序依次存到不同的通用寄存器（除esp）中。其中，寄存器 eax用来保存子功能号，ebx保存第1个参数，ecx 保存第2个参数，edx保存第3个参数，esi保存第4个参数，edi保存第5个参数。传递参数还可以用栈（内存）。
- 小结
宏_syscall 和库函数syscall 相比，syscall 实现更灵活，对用户来说任何参数个数的系统调用都统一用一种形式，用户只要记住syscall就可以了，而宏_syscall的实现比较死板，针对每种参数个数的系统调用都要有单独的形式，因此支持的参数数量必然有限，而且用户要记住 7 种形式_syscall[0-6]，调用时除了输入实参外，还要输入实参的类型，确实有些麻烦，此外这个宏会引发安全漏洞

 #### 实验简介
- 操作系统实现系统调用的过程：
>应用程序调用库函数（API）；
API 将系统调用号存入 EAX，然后通过中断调用使系统进入内核态；
内核中的中断处理函数根据系统调用号，调用对应的内核函数（系统调用）；
系统调用完成相应功能，将返回值存入 EAX，返回到中断处理函数；
中断处理函数返回到 API 中；
API 将 EAX 返回给应用程序。

- 从lib/close.c API 入手
```
#define __LIBRARY__
#include <unistd.h>
_syscall1(int,close,int,fd)
```
_syscall1 是一个宏，定义在 include/unistd.h中
```
#define _syscall(type,name,atype,a) \
type name(atype a) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
    : "=a" (__res) \
    : "0" (__NR_##name),"b" ((long)(a))); \
if (__res >= 0) \
    return (type) __res; \
errno = - __res; \
return -1; \
}
```
将 _syscall1(int,close,int,fd) 进行宏展开，得到：
```
int close(int fd)
{
    long __res;
    __asm__ volatile ("int $0x80"
        : "=a" (__res)
        : "0" (__NR_close),"b" ((long) (fd)));
    if (__res >= 0)
        return (int) __res;
    errno = -__res;
    return -1;
}
```
先将宏__NR_close存入EAX,将参数fd存入EBX，然后调用0x80中断进入内核。调用返回后，从EAX取出返回值，存入__res,通过对__res的判断决定返回什么样的值给API的调用者。
``` include/unistd.h ```中定义了系统调用号
例如：
``` #define __NR_close    6```
据此接口函数可设计为：
``` 
    __syscall1(int,iam,const char*, name);
    __syscall2(int,whoami,char*,name,unsigned int,size);
```
- 从 int 0x80 进入内核
通过```set_system_gate``` 设置IDT，将system_call函数地址写入 0x80对应的中断描述符中，在触发80号中断后自动调用sysyem_call.
在system_call.s中添加新的系统调用
- **实现系统调用**
```
sys_iam()
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

```
```
sys_whoami()
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
```
- 编写用户态测试程序
  ```iam()、whoami()```
*iam()*
```
#define __LIBRARY__
#include <unistd.h> 
#include <errno.h>
#include <asm/segment.h> 
#include <linux/kernel.h>
_syscall1(int, iam, const char*, name);
   
int main(int argc, char *argv[])
{
    /*调用系统调用iam()*/
    iam(argv[1]);
    return 0;
}
```
*whoami*
```
/*
 *  whoami.c
 *  lm    2023
 *  */
#define __LIBRARY__
#include <unistd.h> 
#include <errno.h>
#include <asm/segment.h> 
#include <linux/kernel.h>
#include <stdio.h>
   
_syscall2(int, whoami,char *,name,unsigned int,size);
   
int main(int argc, char *argv[])
{
    char username[64] = {0};
    /*调用系统调用whoami()*/
    whoami(username, 24);
    printf("%s\n", username);
    return 0;
}
```
- **实验过程**
  1. 添加系统调用号 include/unistd.h
  2. 修改系统调用总数, kernel/system_call.s
  3. 为新增的系统调用添加系统调用名并维护系统调用表 include/linux/sys.h
  4. 编写系统调用内核实现代码，kernel/who.c(两个系统调用功能类似，写在同一文件中，即sys_iam()与sys_whoami()，ps:还需添加必要的头文件等)
  5.  修改 Makefile （将新加入的两个系统调用一同编译）kernel/Makefile
  6. 编译内核  make
  7. 将用户态测试程序复制到linux-0.11下编译测试即可
  
  

 



  

  

