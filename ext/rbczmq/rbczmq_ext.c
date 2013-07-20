#include "rbczmq_ext.h"

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
VALUE rb_cZmqXPubSocket;
VALUE rb_cZmqXSubSocket;

VALUE rb_cZmqFrame;
VALUE rb_cZmqMessage;
VALUE rb_cZmqLoop;
VALUE rb_cZmqTimer;
VALUE rb_cZmqPoller;
VALUE rb_cZmqPollitem;
VALUE rb_cZmqBeacon;

VALUE intern_call;
VALUE intern_readable;
VALUE intern_writable;
VALUE intern_error;

rb_encoding *binary_encoding;

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
    return (zctx_interrupted == true) ? Qtrue : Qfalse;
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
 *  call-seq:
 *     ZMQ.errno    =>  Fixnum
 *
 *  Returns the last known ZMQ errno (if any) as a Fixnum.
 *
 * === Examples
 *     ZMQ.errno    =>  0
 *
*/

static VALUE rb_czmq_m_errno(ZMQ_UNUSED VALUE obj)
{
    return INT2NUM(zmq_errno());
}

/*
 *  call-seq:
 *     ZMQ.interrupted!    =>  nil
 *
 *  Callback for Ruby signal handlers for terminating blocking functions and the reactor loop in libczmq.
 *
 * === Examples
 *     ZMQ.interrupted!    =>  nil
 *
*/

static VALUE rb_czmq_m_interrupted_bang(ZMQ_UNUSED VALUE obj)
{
    zctx_interrupted = 1;
    return Qnil;
}

/*
 * :nodoc:
 *  Runs the ZeroMQ proxy with the GVL released.
 *
*/
static VALUE rb_czmq_m_proxy_nogvl(void* args)
{
	void** sockets = (void**)args;
	return (VALUE)zmq_proxy(sockets[0], sockets[1], sockets[2]);
}

/*
 *  call-seq:
 *     ZMQ.proxy(frontend, backend, capture = nil) => nil
 *
 * Run the ZMQ proxy method echoing messages received from front end socket to back end and vice versa,
 * copying messages to the capture socket if provided. This method does not return unless the application
 * is interrupted with a signal.
 *
 * @see http://api.zeromq.org/3-2:zmq-proxy
 *
 * === Examples
 *     context = ZMQ::Context.new
 *     frontend = context.socket(ZMQ::ROUTER)
 *     frontend.bind("tcp://127.0.0.1:5555")
 *     backend = context.socket(ZMQ::DEALER)
 *     backend.bind("tcp://127.0.0.1:5556")
 *     ZMQ.proxy(frontend, backend) => -1 when interrupted
*/
static VALUE rb_czmq_m_proxy(int argc, VALUE *argv, ZMQ_UNUSED VALUE klass)
{
    zmq_sock_wrapper *sock = NULL;
    VALUE frontend, backend, capture;
    void *sockets[3];
    int rc;

    rb_scan_args(argc, argv, "21", &frontend, &backend, &capture);

    GetZmqSocket(frontend);
    sockets[0] = sock->socket;

    GetZmqSocket(backend);
    sockets[1] = sock->socket;

    if (!NIL_P(capture))
    {
        GetZmqSocket(capture);
        sockets[2] = sock->socket;
    }
    else
    {
        sockets[2] = NULL;
    }

    rc = (int)rb_thread_blocking_region(rb_czmq_m_proxy_nogvl, (void *)sockets, RUBY_UBF_IO, 0);

    // int result = zmq_proxy(frontend_socket, backend_socket, capture_socket);
    return INT2NUM(rc);
}

void Init_rbczmq_ext()
{
    intern_call = rb_intern("call");
    intern_readable = rb_intern("on_readable");
    intern_writable = rb_intern("on_writable");
    intern_error = rb_intern("on_error");

    binary_encoding = rb_enc_find("binary");

    rb_mZmq = rb_define_module("ZMQ");

    rb_eZmqError = rb_define_class_under(rb_mZmq, "Error", rb_eStandardError);

    rb_define_module_function(rb_mZmq, "interrupted?", rb_czmq_m_interrupted_p, 0);
    rb_define_module_function(rb_mZmq, "version", rb_czmq_m_version, 0);
    rb_define_module_function(rb_mZmq, "now", rb_czmq_m_now, 0);
    rb_define_module_function(rb_mZmq, "log", rb_czmq_m_log, 1);
    rb_define_module_function(rb_mZmq, "error", rb_czmq_m_error, 0);
    rb_define_module_function(rb_mZmq, "errno", rb_czmq_m_errno, 0);
    rb_define_module_function(rb_mZmq, "interrupted!", rb_czmq_m_interrupted_bang, 0);
    rb_define_module_function(rb_mZmq, "proxy", rb_czmq_m_proxy, -1);

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
    rb_define_const(rb_mZmq, "XSUB", INT2NUM(ZMQ_XSUB));
    rb_define_const(rb_mZmq, "XPUB", INT2NUM(ZMQ_XPUB));

    rb_define_const(rb_mZmq, "EFSM", INT2NUM(EFSM));
    rb_define_const(rb_mZmq, "ENOCOMPATPROTO", INT2NUM(ENOCOMPATPROTO));
    rb_define_const(rb_mZmq, "ETERM", INT2NUM(ETERM));
    rb_define_const(rb_mZmq, "EMTHREAD", INT2NUM(EMTHREAD));

    rb_define_const(rb_mZmq, "EVENT_CONNECTED", INT2NUM(ZMQ_EVENT_CONNECTED));
    rb_define_const(rb_mZmq, "EVENT_CONNECT_DELAYED", INT2NUM(ZMQ_EVENT_CONNECT_DELAYED));
    rb_define_const(rb_mZmq, "EVENT_CONNECT_RETRIED", INT2NUM(ZMQ_EVENT_CONNECT_RETRIED));
    rb_define_const(rb_mZmq, "EVENT_LISTENING", INT2NUM(ZMQ_EVENT_LISTENING));
    rb_define_const(rb_mZmq, "EVENT_BIND_FAILED", INT2NUM(ZMQ_EVENT_BIND_FAILED));
    rb_define_const(rb_mZmq, "EVENT_ACCEPTED", INT2NUM(ZMQ_EVENT_ACCEPTED));
    rb_define_const(rb_mZmq, "EVENT_ACCEPT_FAILED", INT2NUM(ZMQ_EVENT_ACCEPT_FAILED));
    rb_define_const(rb_mZmq, "EVENT_CLOSED", INT2NUM(ZMQ_EVENT_CLOSED));
    rb_define_const(rb_mZmq, "EVENT_CLOSE_FAILED", INT2NUM(ZMQ_EVENT_CLOSE_FAILED));
    rb_define_const(rb_mZmq, "EVENT_DISCONNECTED", INT2NUM(ZMQ_EVENT_DISCONNECTED));
    rb_define_const(rb_mZmq, "EVENT_ALL", INT2NUM(ZMQ_EVENT_ALL));

    _init_rb_czmq_context();
    _init_rb_czmq_socket();
    _init_rb_czmq_frame();
    _init_rb_czmq_message();
    _init_rb_czmq_timer();
    _init_rb_czmq_loop();
    _init_rb_czmq_poller();
    _init_rb_czmq_pollitem();
    _init_rb_czmq_beacon();
}
