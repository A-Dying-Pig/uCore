#include "defs.h"
#include "syscall_ids.h"
#include "trap.h"
#include "proc.h"
#include "time.h"

#define min(a, b) a < b ? a : b;

uint64 sys_write(int fd, char *str, uint len) {
    if (fd != 1)
        return -1;
    if(str<(char *)0x80400000)
        return -1;
    if(str+len>(char *)0x80400000+0x20000*9)
        return -1;
    int size = min(strlen(str), len);
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

void syscall() {
    struct trapframe *trapframe = curr_proc()->trapframe;
    int id = trapframe->a7, ret;
    //printf("syscall %d\n", id);
    uint64 args[7] = {trapframe->a0, trapframe->a1, trapframe->a2, trapframe->a3, trapframe->a4, trapframe->a5, trapframe->a6};
    switch (id) {
        case SYS_write:
            ret = sys_write(args[0], (char *) args[1], args[2]);
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
        default:
            ret = -1;
            printf("unknown syscall %d\n", id);
    }
    trapframe->a0 = ret;
    //printf("syscall ret %d\n", ret);
}
