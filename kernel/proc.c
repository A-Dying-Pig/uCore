#include "defs.h"
#include "proc.h"
#include "trap.h"
#include "riscv.h"
#include "file.h"
#include "memory_layout.h"

struct proc pool[NPROC];

__attribute__ ((aligned (16))) char kstack[NPROC][KSTACK_SIZE];
extern char trampoline[];

extern char boot_stack_top[];
struct proc* current_proc = 0;
struct proc idle;
int curr_pid = 0;

struct proc* curr_proc() {
    if(current_proc == 0)
        return &idle;
    return current_proc;
}

void
procinit(void)
{
    struct proc *p;
    for(p = pool; p < &pool[NPROC]; p++) {
        p->state = UNUSED;
        p->kstack = (uint64) kstack[p - pool];
    }
    idle.kstack = (uint64)boot_stack_top;
    idle.pid = 0;
}

int allocpid() {
    static int PID = 1;
    return PID++;
}

pagetable_t
proc_pagetable(struct proc *p)
{
    pagetable_t pagetable;

    // An empty page table.
    pagetable = uvmcreate();
    if(pagetable == 0)
        panic("");

    if(mappages(pagetable, TRAMPOLINE, PGSIZE,
                (uint64)trampoline, PTE_R | PTE_X) < 0){
        uvmfree(pagetable, 0);
        return 0;
    }

    if((p->trapframe = (struct trapframe *)kalloc()) == 0){
        panic("kalloc\n");
    }
    // map the trapframe just below TRAMPOLINE, for trampoline.S.
    if(mappages(pagetable, TRAPFRAME, PGSIZE,
                (uint64)(p->trapframe), PTE_R | PTE_W) < 0){;
        panic("");
    }

    return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmunmap(pagetable, TRAPFRAME, 1, 0);
    uvmfree(pagetable, sz);
}

static void
freeproc(struct proc *p)
{
    // if(p->mailbox_data)
    //     kfree((void*)p->mailbox_data);
    // p->mailbox_data = 0;
    // if(p->mailbox)
    //     kfree((void*)p->mailbox);
    // p->mailbox = 0;

    if(p->trapframe)
        kfree((void*)p->trapframe);
    p->trapframe = 0;
    if(p->pagetable)
        proc_freepagetable(p->pagetable, p->sz);
    p->pagetable = 0;
    p->state = UNUSED;
    for(int i = 0; i < FD_MAX; ++i) {
        if(p->files[i] != 0) {
            fileclose(p->files[i]);
            p->files[i] = 0;
        }
    }
}


struct proc* allocproc(void)
{
    struct proc *p;
    for(p = pool; p < &pool[NPROC]; p++) {
        if(p->state == UNUSED) {
            goto found;
        }
    }
    return 0;

found:
    p->pid = allocpid();
    p->state = USED;
    p->sz = 0;
    p->exit_code = -1;
    p->parent = 0;
    p->ustack = 0;
    p->pagetable = proc_pagetable(p);
    if(p->pagetable == 0){
        panic("");
    }
    memset(&p->context, 0, sizeof(p->context));
    memset((void*)p->kstack, 0, KSTACK_SIZE);
    p->context.ra = (uint64)usertrapret;
    p->context.sp = p->kstack + KSTACK_SIZE;
    return p;
}

int mailread(char* buf, int len){
    struct proc *p = curr_proc();

   if (p->mailbox->mail_counter == 0){
        warn("Mailbox is empty\n");
        return -1;
    }
    if (len <= 0){
        warn("Invalid receiving buffer length\n\n");
        return 0;
    }
    if (buf == 0){
        warn("Invalid buffer\n");
        return 0;
    }
 
    int remain_bytes =  p->mailbox_data + MAIL_BUF_LENGTH - p->mailbox->data_s,
        mail_length = p->mailbox->mail_length[p->mailbox->length_s];

    // info("Read mail length: %d\n",mail_length);
    if (len > mail_length){
        len = mail_length;
    }

    if (len <= remain_bytes){
        memmove(buf, p->mailbox->data_s, len);
    }else{
        memmove(buf, p->mailbox->data_s, remain_bytes);
        memmove(buf + remain_bytes, p->mailbox_data, len - remain_bytes);
    }
    // printf("read mail: %s, length: %d", buf, len);
    p->mailbox->data_s = (p->mailbox->data_s - p->mailbox_data + mail_length) % MAIL_BUF_LENGTH + p->mailbox_data;
    p->mailbox->length_s = (p->mailbox->length_s + 1 ) % MAX_MAIL;
    p->mailbox->mail_counter --;
    return len;
}


int mailwrite(int pid, void* buf, int len){
    struct proc *p;
    for(p = pool; p < &pool[NPROC]; p++) {
        if(p->state != UNUSED && p->pid == pid) {
            goto found;
        }
    }
    warn("Sending mail to a non-existing process\n");
    return 0;
found:
    if (p->mailbox->mail_counter == MAX_MAIL){
        warn("Mailbox is full\n");
        return -1;
    }
    if (len < 0){
        warn("Invalid mail length\n\n");
        return 0;
    }
    if (len == 0){
        warn("Send length 0 mail\n");
        return 0;
    }
    if (buf == 0){
        warn("Invalid buffer\n");
        return 0;
    }

    char * buffer = (char *)buf;
    if (len > MAX_MAIL_LENGTH){
        warn("Trucate mail\n");
        len = MAX_MAIL_LENGTH;
        buffer[len] = '\0';
    }
    printf("sending mail: %s, length: %d \n", buffer,len);
    p->mailbox->mail_counter ++;
    p->mailbox->mail_length[p->mailbox->length_e] = len;
    p->mailbox->length_e = (p->mailbox->length_e + 1 ) % MAX_MAIL;
    int remain_bytes = p->mailbox_data + MAIL_BUF_LENGTH - p->mailbox->data_e;
    if (len <= remain_bytes){
        memmove(p->mailbox->data_e, buffer, len);
    }else{
        memmove(p->mailbox->data_e, buffer, remain_bytes);
        memmove(p->mailbox_data, buffer + remain_bytes, len - remain_bytes);
    }
    p->mailbox->data_e = (p->mailbox->data_e - p->mailbox_data + len) % MAIL_BUF_LENGTH + p->mailbox_data;
    return len;
}



void
scheduler(void)
{
    struct proc *p;
    for(;;){
        int all_done = 1;
        for(p = pool; p < &pool[NPROC]; p++) {
            if(p->state == RUNNABLE) {
                all_done = 0;
                p->state = RUNNING;
                current_proc = p;
                curr_pid = p->pid;
                // info("switch to next proc %d\n", p->pid);
                swtch(&idle.context, &p->context);
            }
        }
        if(all_done)
            panic("all apps over\n");
    }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
    struct proc *p = curr_proc();
    if(p->state == RUNNING)
        panic("sched running");
    swtch(&p->context, &idle.context);
}

// Give up the CPU for one scheduling round.
void yield(void)
{
    current_proc->state = RUNNABLE;
    sched();
}

int
fork(void)
{
    int pid;
    struct proc *np;
    struct proc *p = curr_proc();
    // Allocate process.
    if((np = allocproc()) == 0){
        panic("allocproc\n");
    }
    // Copy user memory from parent to child.
    if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
        panic("uvmcopy\n");
    }
    np->sz = p->sz;

    // copy saved user registers.
    *(np->trapframe) = *(p->trapframe);

    for(int i = 0; i < FD_MAX; ++i)
        if(p->files[i] != 0 && p->files[i]->type != FD_NONE) {
            p->files[i]->ref++;
            np->files[i] = p->files[i];
        }

    // Cause fork to return 0 in the child.
    np->trapframe->a0 = 0;
    pid = np->pid;
    np->parent = p;
    np->state = RUNNABLE;

    //mail box
    np->mailbox_data = p->mailbox_data;
    np->mailbox = p->mailbox;

    return pid;
}

int exec(char* name) {
    int id = get_id_by_name(name);
    if(id < 0)
        return -1;
    struct proc *p = curr_proc();
    proc_freepagetable(p->pagetable, p->sz);
    p->sz = 0;
    p->pagetable = proc_pagetable(p);
    if(p->pagetable == 0){
        panic("");
    }
    loader(id, p);
    return 0;
}

int
wait(int pid, int* code)
{
    struct proc *np;
    int havekids;
    struct proc *p = curr_proc();

    for(;;){
        // Scan through table looking for exited children.
        havekids = 0;
        for(np = pool; np < &pool[NPROC]; np++){
            if(np->state != UNUSED && np->parent == p && (pid <= 0 || np->pid == pid)){
                havekids = 1;
                if(np->state == ZOMBIE){
                    // Found one.
                    np->state = UNUSED;
                    pid = np->pid;
                    *code = np->exit_code;
                    return pid;
                }
            }
        }
        if(!havekids){
            return -1;
        }
        p->state = RUNNABLE;
        sched();
    }
}

void exit(int code) {
    struct proc *p = curr_proc();
    p->exit_code = code;
    info("proc %d exit with %d\n", p->pid, code);
    freeproc(p);
    if(p->parent != 0) {
        trace("wait for parent to clean\n");
        p->state = ZOMBIE;
    }
    sched();
}

int fdalloc(struct file* f) {
    struct proc* p = curr_proc();
    // fd = 0 is reserved for stdio/stdout
    for(int i = 1; i < FD_MAX; ++i) {
        if(p->files[i] == 0) {
            p->files[i] = f;
            return i;
        }
    }
    return -1;
}
