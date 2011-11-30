#ifndef RBCZMQ_EXT_H
#define RBCZMQ_EXT_H

#include <czmq.h>
#include "ruby.h"

/* Compiler specific */

#if defined(__GNUC__) && (__GNUC__ >= 3)
#define ZMQ_UNUSED __attribute__ ((unused))
#define ZMQ_NOINLINE __attribute__ ((noinline))
#else
#define ZMQ_UNUSED
#define ZMQ_NOINLINE
#endif

#include <rbczmq_prelude.h>

#define ZmqRaiseSysError rb_sys_fail(zmq_strerror(zmq_errno()))
/* What about EINVAL ? */
#define ZmqAssert(rc) \
    if (rc != 0) { \
        if (zmq_errno() != 0) ZmqRaiseSysError; \
        if (rc == ENOMEM) rb_memerror(); \
        return Qfalse; \
    }
#define ZmqAssertObjOnAlloc(obj, wrapper) \
    if (obj == NULL) { \
        xfree(wrapper); \
        rb_memerror(); \
    }
#define ZmqAssertType(obj, type, desc) \
    if (!rb_obj_is_kind_of(obj,type)) \
        rb_raise(rb_eTypeError, "wrong argument type %s (expected %s)", rb_obj_classname(obj), desc);

extern VALUE rb_mZmq;
extern VALUE rb_eZmqError;
extern VALUE rb_cZmqContext;

extern VALUE rb_cZmqSocket;
extern VALUE rb_cZmqPubSocket;
extern VALUE rb_cZmqSubSocket;
extern VALUE rb_cZmqPushSocket;
extern VALUE rb_cZmqPullSocket;
extern VALUE rb_cZmqRouterSocket;
extern VALUE rb_cZmqDealerSocket;
extern VALUE rb_cZmqRepSocket;
extern VALUE rb_cZmqReqSocket;
extern VALUE rb_cZmqPairSocket;

extern VALUE rb_cZmqFrame;
extern VALUE rb_cZmqMessage;
extern VALUE rb_cZmqLoop;
extern VALUE rb_cZmqTimer;

#include <context.h>
#include <socket.h>
#include <frame.h>
#include <message.h>
#include <loop.h>
#include <timer.h>

static VALUE
get_pid()
{
    rb_secure(2);
    return INT2FIX(getpid());
}

static inline char *rb_czmq_formatted_current_time()
{
    time_t curtime = time (NULL);
    struct tm *loctime = localtime(&curtime);
    char *formatted;
    formatted = (char*)xmalloc(20);
    strftime(formatted, 20, "%y-%m-%d %H:%M:%S ", loctime);
    return formatted;
}

#endif
