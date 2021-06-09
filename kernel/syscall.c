#include "defs.h"
#include "syscall_ids.h"
#include "trap.h"
#include "proc.h"
#include "time.h"

#define min(a, b) a < b ? a : b;

uint64 sys_write(int fd, char *addr, uint len) {
    if (fd != 1)
        return -1;
    if ((uint64)addr < 0x1000)
        return -1;
    struct proc *p = curr_proc();
    char str[200];
    int size = copyinstr(p->pagetable, str, (uint64) addr, MIN(len, 200));
    printf("size = %d\n", size);
    for(int i = 0; i < size; ++i) {
        console_putchar(str[i]);
    }
    return size;
}

uint64 sys_exit(int code) {
    exit(code);
    return 0;
}

uint64 sys_sched_yield() {
    yield();
    return 0;
}

uint64 sys_get_time(TimeVal *ts, int tz){
    ts->sec=get_cycle()/25000000;
    ts->usec=get_cycle()/25;
    return 0;
}

long long sys_set_priority(long long p){
    return set_priority(p);
}

long long sys_mmap(void* start, unsigned long long len, int port){
    pagetable_t pg = curr_proc()->pagetable;
    uint64 a;
    int flag=0;

    if(len == 0){
        printf("len=0\n");
        return 0;
    }
    if(len > 1048576000){
        printf("len长于1G\n");
        return -1;
    }
    if((uint64)start%0x1000 != 0){
        printf("不对齐\n");
        return -1;
    }
    if((port & 0x7) == 0){
        printf("port=0\n");
        return -1;
    }
    if((port & (~0x7)) != 0){
        printf("port前几位！=0\n");
        return -1;
    }
    for(a = (uint64)start; a < (uint64)start + len; a = a + 4096){
        if(useraddr(pg,a) != 0){
            printf("有的頁不爲空\n");
            return -1;
        }
        flag = mappages(pg,a,4096,(uint64)kalloc(),(port<<1)|(1<<4));
        if (flag<0){
            printf("分配内存失敗\n");
            return -1;
        }
    }
    return (((len) + 4095) & ~(4095));
}

int sys_munmap(void* start, unsigned long long len){
    pagetable_t pg = curr_proc()->pagetable;
    uint64 a;

    if(len == 0){
        printf("len=0");
        return 0;
    }
    if(len > 1048576000){
        printf("len长于1G");
        return -1;
    }
    if((uint64)start%0x1000 != 0){
        printf("不对齐");
        return -1;
    }
    for(a = (uint64)start; a < (uint64)start + len; a = a + 4096){
        if(useraddr(pg,a) == 0){
            return -1;
        }
    }
    
    uvmunmap(pg,(uint64)start,len/4096,0);
   
    return (((len) + 4095) & ~(4095));
}

void syscall() {
    struct trapframe *trapframe = curr_proc()->trapframe;
    int id = trapframe->a7, ret;
    uint64 args[6] = {trapframe->a0, trapframe->a1, trapframe->a2, trapframe->a3, trapframe->a4, trapframe->a5};
    printf("syscall %d args:%p %p %p %p %p %p\n", id, args[0], args[1], args[2], args[3], args[4], args[5]);
    switch (id) {
        case SYS_write:
            ret = sys_write(args[0], (char *) args[1], args[2]);
            printf("\n");
            break;
        case SYS_exit:
            ret = sys_exit(args[0]);
            break;
        case SYS_sched_yield:
            ret = sys_sched_yield();
            break;
        case SYS_gettimeofday:
            ret = sys_get_time((TimeVal *)args[0],args[1]);
            break;
        case SYS_setpriority:
            ret = sys_set_priority(args[0]);
            break;
        case SYS_mmap:
            ret = sys_mmap((void *)args[0],args[1],args[2]);
            break;
        case SYS_munmap:
            ret = sys_munmap((void*)args[0],args[1]);
            break;
        default:
            ret = -1;
            printf("unknown syscall %d\n", id);
    }
    trapframe->a0 = ret;
    printf("syscall ret %d\n", ret);
}
