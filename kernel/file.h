#ifndef __FILE_H__
#define __FILE_H__

#include "types.h"

// pipe.h
#define PIPESIZE 512
#define MAX_MAIL 16
#define MAX_MAIL_LENGTH 256
#define MAIL_BUF_LENGTH (MAX_MAIL * MAX_MAIL_LENGTH)

struct pipe {
    char data[PIPESIZE];
    uint nread;     // number of bytes read
    uint nwrite;    // number of bytes written
    int readopen;   // read fd is still open
    int writeopen;  // write fd is still open
};

// file.h
struct file {
    enum { FD_NONE = 0, FD_PIPE} type;
    int ref; // reference count
    char readable;
    char writable;
    struct pipe *pipe; // FD_PIPE
    uint off;          // FD_INODE
};


struct mailbox{
    int mail_counter;
    char * data_s;
    char * data_e;
    int mail_length[MAX_MAIL];
    int length_s;
    int length_e;
};


extern struct file filepool[128 * 16];

#endif //!__FILE_H__