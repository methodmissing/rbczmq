/*
 *
 * Author:: Lourens Naudé
 * Homepage::  http://github.com/methodmissing/rbczmq
 * Date:: 20111213
 *
 *----------------------------------------------------------------------------
 *
 * Copyright (C) 2011 by Lourens Naudé. All Rights Reserved.
 * Email: lourens at methodmissing dot com
 *
 * (The MIT License)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *---------------------------------------------------------------------------
 *
 */

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
    if (rc != 0) { \
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

extern st_table *frames_map;

#include <context.h>
#include <socket.h>
#include <frame.h>
#include <message.h>
#include <loop.h>
#include <timer.h>

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
