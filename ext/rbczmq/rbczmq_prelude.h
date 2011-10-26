#ifndef RBCZMQ_PRELUDE_H
#define RBCZMQ_PRELUDE_H

#ifndef RFLOAT_VALUE
#define RFLOAT_VALUE(v) (RFLOAT(v)->value)
#endif

#ifdef RUBINIUS
#include <rubinius.h>
#else
#ifdef RUBY_VM
#include <ruby19.h>
#else
#include <ruby18.h>
#endif
#endif

struct nogvl_send_args {
    void *socket;
    const char *msg;
    Bool read;
    int fd;
};

struct nogvl_send_frame_args {
    void *socket;
    zframe_t *frame;
    int flags;
    Bool read;
    int fd;
};

struct nogvl_send_message_args {
    void *socket;
    zmsg_t *message;
    Bool read;
    int fd;
};

struct nogvl_recv_args {
    void *socket;
    int fd;
};

struct nogvl_conn_args {
    void *socket;
     char *endpoint;
    int fd;
};

#endif
