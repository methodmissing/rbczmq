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

#define ZmqRaiseSysError() { \
        printf("Sys error location: %s:%d\n", __FILE__,__LINE__); \
        rb_sys_fail(zmq_strerror(zmq_errno())); \
    }
#define ZmqAssertSysError() if (zmq_errno() != 0 && zmq_errno() != EAGAIN) ZmqRaiseSysError();
#define ZmqAssert(rc) \
    if (rc == -1) { \
        ZmqAssertSysError(); \
        if (rc == ENOMEM) rb_memerror(); \
        return Qfalse; \
    }
#define ZmqAssertObjOnAlloc(obj, wrapper) \
    if (obj == NULL) { \
        xfree(wrapper); \
        ZmqAssertSysError(); \
        rb_memerror(); \
    }
#define ZmqAssertType(obj, type, desc) \
    if (!rb_obj_is_kind_of(obj,type)) \
        rb_raise(rb_eTypeError, "wrong argument type %s (expected %s): %s", rb_obj_classname(obj), desc, RSTRING_PTR(rb_obj_as_string(obj)));

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
extern VALUE rb_cZmqPoller;
extern VALUE rb_cZmqPollitem;

extern st_table *frames_map;

extern VALUE intern_call;
extern VALUE intern_readable;
extern VALUE intern_writable;
extern VALUE intern_error;

#include <context.h>
#include <socket.h>
#include <frame.h>
#include <message.h>
#include <loop.h>
#include <timer.h>
#include <poller.h>
#include <pollitem.h>

static inline char *rb_czmq_formatted_current_time()
{
    time_t curtime = time (NULL);
    struct tm *loctime = localtime(&curtime);
    char *formatted;
    formatted = (char*)xmalloc(20);
    strftime(formatted, 20, "%y-%m-%d %H:%M:%S ", loctime);
    return formatted;
}

struct nogvl_device_args {
    int type;
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    zctx_t *ctx;
#endif
    zmq_sock_wrapper *in;
    zmq_sock_wrapper *out;
    int rc;
};
typedef struct nogvl_device_args nogvl_device_args_t;

#endif
