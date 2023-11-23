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
