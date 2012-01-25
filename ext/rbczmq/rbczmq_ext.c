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

#include <rbczmq_ext.h>

VALUE rb_mZmq;
VALUE rb_eZmqError;
VALUE rb_cZmqContext;

VALUE rb_cZmqSocket;
VALUE rb_cZmqPubSocket;
VALUE rb_cZmqSubSocket;
VALUE rb_cZmqPushSocket;
VALUE rb_cZmqPullSocket;
VALUE rb_cZmqRouterSocket;
VALUE rb_cZmqDealerSocket;
VALUE rb_cZmqRepSocket;
VALUE rb_cZmqReqSocket;
VALUE rb_cZmqPairSocket;

VALUE rb_cZmqFrame;
VALUE rb_cZmqMessage;
VALUE rb_cZmqLoop;
VALUE rb_cZmqTimer;

st_table *frames_map = NULL;

#ifdef HAVE_RUBY_ENCODING_H
rb_encoding *binary_encoding;
#endif

/*
 *  call-seq:
 *     ZMQ.interrupted?    =>  boolean
 *
 *  Returns true if the process was interrupted by signal.
 *
 * === Examples
 *     ZMQ.interrupted?    =>  boolean
 *
*/

static VALUE rb_czmq_m_interrupted_p(ZMQ_UNUSED VALUE obj)
{
    return (zctx_interrupted == TRUE) ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *     ZMQ.version    =>  Array
 *
 *  Returns the libzmq version linked against.
 *
 * === Examples
 *     ZMQ.version    =>  [2,1,11]
 *
*/

static VALUE rb_czmq_m_version(ZMQ_UNUSED VALUE obj)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    return rb_ary_new3(3, INT2NUM(major), INT2NUM(minor), INT2NUM(patch));
}

/*
 *  call-seq:
 *     ZMQ.now    =>  Fixnum
 *
 *  Returns a current timestamp as a Fixnum
 *
 * === Examples
 *     ZMQ.now    =>  1323206405148
 *
*/

static VALUE rb_czmq_m_now(ZMQ_UNUSED VALUE obj)
{
    return INT2NUM(zclock_time());
}

/*
 *  call-seq:
 *     ZMQ.log("msg")    =>  nil
 *
 *  Logs a timestamped message to stdout.
 *
 * === Examples
 *     ZMQ.log("msg")    =>  nil # 11-12-06 21:20:55 msg
 *
*/

static VALUE rb_czmq_m_log(ZMQ_UNUSED VALUE obj, VALUE msg)
{
    Check_Type(msg, T_STRING);
    zclock_log(StringValueCStr(msg));
    return Qnil;
}

/*
 *  call-seq:
 *     ZMQ.error    =>  ZMQ::Error
 *
 *  Returns the last known ZMQ error (if any) as a ZMQ::Error instance.
 *
 * === Examples
 *     ZMQ.error    =>  ZMQ::Error or nil
 *
*/

static VALUE rb_czmq_m_error(ZMQ_UNUSED VALUE obj)
{
    int err;
    err = zmq_errno();
    if (err == 0) return Qnil;
    return rb_exc_new2(rb_eZmqError, zmq_strerror(zmq_errno()));
}

/*
 * :nodoc:
 *  Spawn an attached thread for Ruby 1.8 to not block the VM.
 *
*/
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
static void rb_czmq_one_eight_device(void *ptr, zctx_t *ctx, void *pipe)
{
    struct nogvl_device_args *args = ptr;
    zmq_sock_wrapper *in = args->in;
    zmq_sock_wrapper *out = args->out;
    args->rc = zmq_device(args->type, in->socket, out->socket);
}
#endif

/*
 * :nodoc:
 *  Setup and start a device while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_device(void *ptr)
{
    struct nogvl_device_args *args = ptr;
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    int critical;
    nogvl_device_args_t *thargs = NULL;
    void *pipe;
    int fd, rc;
    fd_set fdset;
    fd_set *rfds = NULL;
    fd_set *wfds = NULL;
    fd_set *efds = NULL;
    FD_ZERO(&fdset);
#endif
    errno = 0;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    zmq_sock_wrapper *in = args->in;
    zmq_sock_wrapper *out = args->out;
    return (VALUE)zmq_device(args->type, in->socket, out->socket);
#else
    /* ruby overwrites the main OS thread's C stack when context switching to another ruby thread */
    thargs = ALLOC(nogvl_device_args_t);
    if (thargs == NULL) return (VALUE)-1;
    MEMCPY(thargs, args, nogvl_device_args_t, 1);
    thargs->rc = 0;
    /* spawn an attached thread to avoid syscalls blocking the whole VM */
    critical = rb_thread_critical;
    rb_thread_critical = Qtrue;
    pipe = zthread_fork(thargs->ctx, rb_czmq_one_eight_device, (void *)thargs);
    rb_thread_critical = critical;
    if (pipe == NULL) {
        xfree(thargs);
        return (VALUE)-1;
    }
    fd = zsockopt_fd(pipe);
    FD_SET(fd, &fdset);
    rfds = &fdset;
    /* if our pipe's FD not readable anymore, we know zmq_device returned and our thread died */
    for(;;) {
        rc = rb_thread_select(fd + 1, rfds, wfds, efds, NULL);
        if (rc == 1) break;
    }
    rc = thargs->rc;
    xfree(thargs);
    return (VALUE)rc;
#endif
}

/*
 *  call-seq:
 *     ZMQ.device(type, sock1, sock2)    =>  nil
 *
 *  Setup and run a ZMQ device. These are intermediate (and stateless) components that reduce complexity in larger
 *  topologies. They generally connect a set of frontend sockets to a set of backend sockets. Devices also block
 *  the current thread of execution as they're like a dedicated "proxy", shuffling messages from one component /
 *  network to another.
 *
 * === Examples
 *     ZMQ.device(ZMQ::QUEUE, sock1, sock2)    =>  ZMQ::Error or nil
 *
*/

static VALUE rb_czmq_m_device(ZMQ_UNUSED VALUE obj, VALUE type, VALUE insock, VALUE outsock)
{
    int rc;
    struct nogvl_device_args args;
    zmq_sock_wrapper *in = NULL;
    zmq_sock_wrapper *out = NULL;
    Check_Type(type, T_FIXNUM);
    ZmqAssertSocket(insock);
    ZmqAssertSocket(outsock);

    Data_Get_Struct(insock, zmq_sock_wrapper, in); \
    if (!in) rb_raise(rb_eTypeError, "uninitialized ZMQ socket!"); \
    if (in->flags & ZMQ_SOCKET_DESTROYED) rb_raise(rb_eZmqError, "object %p has been destroyed by the ZMQ framework", (void *)insock);
    ZmqSockGuardCrossThread(in);
    ZmqAssertSocketNotPending(in, "can only use a bound or connected frontend socket in a device!");

    Data_Get_Struct(outsock, zmq_sock_wrapper, out); \
    if (!out) rb_raise(rb_eTypeError, "uninitialized ZMQ socket!"); \
    if (out->flags & ZMQ_SOCKET_DESTROYED) rb_raise(rb_eZmqError, "object %p has been destroyed by the ZMQ framework", (void *)outsock);
    ZmqSockGuardCrossThread(out);
    ZmqAssertSocketNotPending(out, "can only use a bound or connected backend socket in a device!");

    args.type = FIX2INT(type);
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    args.ctx = in->ctx;
#endif
    args.in = in;
    args.out = out;

    rc = (int)rb_thread_blocking_region(rb_czmq_nogvl_device, (void *)&args, RUBY_UBF_IO, 0);
    if (rc == -1 && zmq_errno() == EINVAL) rb_raise(rb_eZmqError, "Device type %s not supported!", RSTRING_PTR(rb_obj_as_string(type)));
    if (rc == -1 && zmq_errno() == EFAULT) rb_raise(rb_eZmqError, "Frontend (%s) or backend (%s) socket is invalid!", RSTRING_PTR(rb_obj_as_string(insock)), RSTRING_PTR(rb_obj_as_string(outsock)));
    return INT2NUM(rc);
}

void Init_rbczmq_ext()
{
    frames_map = st_init_numtable();

#ifdef HAVE_RUBY_ENCODING_H
    binary_encoding = rb_enc_find("binary");
#endif

    rb_mZmq = rb_define_module("ZMQ");

    rb_eZmqError = rb_define_class_under(rb_mZmq, "Error", rb_eStandardError);

    rb_define_module_function(rb_mZmq, "interrupted?", rb_czmq_m_interrupted_p, 0);
    rb_define_module_function(rb_mZmq, "version", rb_czmq_m_version, 0);
    rb_define_module_function(rb_mZmq, "now", rb_czmq_m_now, 0);
    rb_define_module_function(rb_mZmq, "log", rb_czmq_m_log, 1);
    rb_define_module_function(rb_mZmq, "error", rb_czmq_m_error, 0);
    rb_define_module_function(rb_mZmq, "device", rb_czmq_m_device, 3);

    rb_define_const(rb_mZmq, "POLLIN", INT2NUM(ZMQ_POLLIN));
    rb_define_const(rb_mZmq, "POLLOUT", INT2NUM(ZMQ_POLLOUT));
    rb_define_const(rb_mZmq, "POLLERR", INT2NUM(ZMQ_POLLERR));

    rb_define_const(rb_mZmq, "PAIR", INT2NUM(ZMQ_PAIR));
    rb_define_const(rb_mZmq, "SUB", INT2NUM(ZMQ_SUB));
    rb_define_const(rb_mZmq, "PUB", INT2NUM(ZMQ_PUB));
    rb_define_const(rb_mZmq, "REQ", INT2NUM(ZMQ_REQ));
    rb_define_const(rb_mZmq, "REP", INT2NUM(ZMQ_REP));
    rb_define_const(rb_mZmq, "DEALER", INT2NUM(ZMQ_DEALER));
    rb_define_const(rb_mZmq, "ROUTER", INT2NUM(ZMQ_ROUTER));
    rb_define_const(rb_mZmq, "PUSH", INT2NUM(ZMQ_PUSH));
    rb_define_const(rb_mZmq, "PULL", INT2NUM(ZMQ_PULL));

    rb_define_const(rb_mZmq, "EFSM", INT2NUM(EFSM));
    rb_define_const(rb_mZmq, "ENOCOMPATPROTO", INT2NUM(ENOCOMPATPROTO));
    rb_define_const(rb_mZmq, "ETERM", INT2NUM(ETERM));
    rb_define_const(rb_mZmq, "EMTHREAD", INT2NUM(EMTHREAD));

    rb_define_const(rb_mZmq, "STREAMER", INT2NUM(ZMQ_STREAMER));
    rb_define_const(rb_mZmq, "FORWARDER", INT2NUM(ZMQ_FORWARDER));
    rb_define_const(rb_mZmq, "QUEUE", INT2NUM(ZMQ_QUEUE));

    _init_rb_czmq_context();
    _init_rb_czmq_socket();
    _init_rb_czmq_frame();
    _init_rb_czmq_message();
    _init_rb_czmq_timer();
    _init_rb_czmq_loop();
}
