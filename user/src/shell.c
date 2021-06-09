#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


const unsigned char LF = 0x0a;
const unsigned char CR = 0x0d;
const unsigned char DL = 0x7f;
const unsigned char BS = 0x08;
const unsigned char VB = 0x7c;

char line[10][100] = {};
int cmd_idx = 0;

int top[10] = {0,0,0,0,0,0,0,0,0,0};

void push(char c) {
    line[cmd_idx][top[cmd_idx]++] = c;
}
void push2(char c, int idx) {
    line[idx][top[idx]++] = c;
}

void pop() {
    --top[cmd_idx];
}

int is_empty(int idx) {
    return top[idx] == 0;
}

void clear() {
    for (int i = 0; i<= cmd_idx; i++)
        top[cmd_idx] = 0;
    cmd_idx = 0;
}

int main() {
    printf("C user shell\n");
    printf(">> ");
    while (1) {
        char c = getchar();
        switch (c) {
            case LF:
            case CR:
                printf("\n");
                if (cmd_idx == 0){
                    if (!is_empty(0)) {
                        push('\0');
                        int pid = fork();
                        if (pid == 0) {
                            // child process
                            if (exec(line[0]) < 0) {
                                printf("no such program\n");
                                exit(0);
                            }
                            panic("unreachable!");
                        } else {
                            int xstate = 0;
                            int exit_pid = 0;
                            exit_pid = wait(pid, &xstate);
                            assert(pid == exit_pid, -1);
                            printf("Shell: Process %d exited with code %d\n", pid, xstate);
                        }
                        clear();
                    }
                }else{
                    if (!is_empty(0) && !is_empty(1)){
                        push2('\0',0);
                        push2('\0',1);
                        // pipe
                        uint64 fd[2];
                        if(pipe(fd) < 0){
                            panic("pipe failed\n");
                        }
                        int pid1 = fork();
                        if (pid1 == 0){
                            rpc_out(fd[1]);
                            if (exec(line[0]) < 0) {
                                printf("no such program\n");
                                exit(0);
                            }
                        }
                        int pid2 = fork();
                        if (pid2 == 0){
                            rpc_in(fd[0]);
                            if (exec(line[1]) < 0) {
                                printf("no such program\n");
                                exit(0);
                            }
                        }
                        int xstate = 0;
                        int exit_pid = 0;
                        exit_pid = wait(pid1, &xstate);
                        assert(pid1 == exit_pid, -1);
                        printf("Shell: Process %d exited with code %d\n", pid1, xstate);
                        exit_pid = wait(pid2, &xstate);
                        assert(pid2 == exit_pid, -1);
                        printf("Shell: Process %d exited with code %d\n", pid2, xstate);
                    }
                    clear();
                }
                
                printf(">> ");
                break;
            case BS:
            case DL:
                if (!is_empty(cmd_idx)) {
                    putchar(BS);
                    printf(" ");
                    putchar(BS);
                    pop();
                }
                break;
            case VB:
                if (cmd_idx == 0){
                    cmd_idx ++;
                }else{
                    printf("Shell: only support TWO piped programs\n");
                    clear();
                    printf(">> ");
                }
                putchar('|');
                break;
            default:
                putchar(c);
                push(c);
                break;
        }
    }
    return 0;
}
