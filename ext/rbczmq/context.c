#include "rbczmq_ext.h"

static VALUE intern_zctx_process;

static VALUE rb_czmq_ctx_set_iothreads(VALUE context, VALUE io_threads);

static VALUE get_pid()
{
    rb_secure(2);
    return INT2NUM(getpid());
}

/*
 * :nodoc:
 *  Destroy the context while the GIL is released - zctx_destroy also closes it's list of sockets and thus may block
 *  depending on socket linger values.
 *
*/
static VALUE rb_czmq_nogvl_zctx_destroy(void *ptr)
{
    errno = 0;
    zmq_ctx_wrapper *ctx = ptr;
    zctx_destroy(&ctx->ctx);
    ctx->flags |= ZMQ_CONTEXT_DESTROYED;
    return Qnil;
}

/*
 * :nodoc:
 *  Free all resources for a context - invoked by the lower level ZMQ::Context#destroy as well as the GC callback
 *
*/
static void rb_czmq_free_ctx(zmq_ctx_wrapper *ctx)
{
    VALUE ctx_map;
    ctx_map = rb_ivar_get(rb_mZmq, intern_zctx_process);
    rb_thread_blocking_region(rb_czmq_nogvl_zctx_destroy, ctx, RUBY_UBF_IO, 0);
    ctx->ctx = NULL;
    rb_hash_aset(ctx_map, get_pid(), Qnil);
}

/*
 * :nodoc:
 *  GC free callback
 *
*/
static void rb_czmq_free_ctx_gc(void *ptr)
{
    zmq_ctx_wrapper *ctx = (zmq_ctx_wrapper *)ptr;
    if (ctx) {
        if (ctx->ctx != NULL && !(ctx->flags & ZMQ_CONTEXT_DESTROYED)) rb_czmq_free_ctx(ctx);
        xfree(ctx);
    }
}

/*
 * :nodoc:
 *  Creates a new context while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_zctx_new(ZMQ_UNUSED void *ptr)
{
    errno = 0;
    zctx_t *ctx = NULL;
    ctx = zctx_new();
    zctx_set_linger(ctx, 1);
    return (VALUE)ctx;
}

/*
 *  call-seq:
 *     ZMQ::Context.new    =>  ZMQ::Context
 *     ZMQ::Context.new(1)    =>  ZMQ::Context
 *
 *  Returns a handle to a new ZMQ context. A single context per process is supported in order to guarantee stability across
 *  all Ruby implementations. A context should be passed as an argument to any Ruby threads. Optionally a context can be
 *  initialized with an I/O threads value (default: 1) - there should be no need to fiddle with this.
 *
 * === Examples
 *     ZMQ::Context.new    =>  ZMQ::Context
 *     ZMQ::Context.new(1)    =>  ZMQ::Context
 *
*/

static VALUE rb_czmq_ctx_s_new(int argc, VALUE *argv, VALUE context)
{
    VALUE ctx_map;
    VALUE io_threads;
    zmq_ctx_wrapper *ctx = NULL;
    rb_scan_args(argc, argv, "01", &io_threads);
    ctx_map = rb_ivar_get(rb_mZmq, intern_zctx_process);
    if (!NIL_P(rb_hash_aref(ctx_map, get_pid()))) rb_raise(rb_eZmqError, "single ZMQ context per process allowed");
    context = Data_Make_Struct(rb_cZmqContext, zmq_ctx_wrapper, 0, rb_czmq_free_ctx_gc, ctx);
    ctx->ctx = (zctx_t*)rb_thread_blocking_region(rb_czmq_nogvl_zctx_new, NULL, RUBY_UBF_IO, 0);
    ZmqAssertObjOnAlloc(ctx->ctx, ctx);
    ctx->flags = 0;
    rb_obj_call_init(context, 0, NULL);
    rb_hash_aset(ctx_map, get_pid(), context);
    if (!NIL_P(io_threads)) rb_czmq_ctx_set_iothreads(context, io_threads);
    return context;
}

/*
 *  call-seq:
 *     ctx.destroy    =>  nil
 *
 *  Destroy a ZMQ context and all sockets in it. Useful for manual memory management, otherwise the GC
 *  will take the same action if a context object is not reachable anymore on the next GC cycle. This is
 *  a lower level API.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     ctx.destroy     =>   nil
 *
*/

static VALUE rb_czmq_ctx_destroy(VALUE obj)
{
    ZmqGetContext(obj);
    rb_czmq_free_ctx(ctx);
    return Qnil;
}

/*
 *  call-seq:
 *     ctx.iothreads = 2    =>  nil
 *
 *  Raises default I/O threads from 1 - there should be no need to fiddle with this.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     ctx.iothreads = 2    =>   nil
 *
*/

static VALUE rb_czmq_ctx_set_iothreads(VALUE obj, VALUE threads)
{
    int iothreads;
    errno = 0;
    ZmqGetContext(obj);
    Check_Type(threads, T_FIXNUM);
    iothreads = FIX2INT(threads);
    if (iothreads > 1) rb_warn("You probably don't want to spawn more than 1 I/O thread per ZMQ context.");
    if (iothreads < 0) rb_raise(rb_eZmqError, "negative I/O threads count is not supported.");
    zctx_set_iothreads(ctx->ctx, iothreads);
    if (zmq_errno() == EINVAL) ZmqRaiseSysError();
    return Qnil;
}

/*
 *  call-seq:
 *     ctx.linger = 100    =>  nil
 *
 *  Set msecs to flush sockets when closing them. A high value may block / pause the application on socket close. This
 *  binding defaults to a linger value of 1 msec set for all sockets, which is important for the reactor implementation
 *  in ZMQ::Loop to avoid stalling the event loop.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     ctx.linger = 100    =>   nil
 *
*/

static VALUE rb_czmq_ctx_set_linger(VALUE obj, VALUE linger)
{
    errno = 0;
    int msecs;
    ZmqGetContext(obj);
    Check_Type(linger, T_FIXNUM);
    msecs = FIX2INT(linger);
    if (msecs < 0) rb_raise(rb_eZmqError, "negative linger / timeout values is not supported.");
    zctx_set_linger(ctx->ctx, msecs);
    return Qnil;
}

/*
 *  call-seq:
 *     ctx.hwm    =>  Fixnum
 *
 *  Returns High Water Mark (HWM) option used for native thread creation (non-Ruby threads, ahead of time API addition)
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     ctx.hwm    =>   1
 *
*/

static VALUE rb_czmq_ctx_hwm(VALUE obj)
{
    errno = 0;
    int wm;
    ZmqGetContext(obj);
    rb_warn("Deprecated method ZMQ::Context#hwm, does nothing - to be removed after 2013/05/14");
    return INT2FIX(zctx_hwm(ctx->ctx));
}

/*
 *  call-seq:
 *     ctx.hwm = 100    =>  nil
 *
 *  Sets the High Water Mark (HWM) option used for native thread creation (non-Ruby threads, ahead of time API addition)
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     ctx.hwm = 100    =>   nil
 *
*/

static VALUE rb_czmq_ctx_set_hwm(VALUE obj, VALUE hwm)
{
    errno = 0;
    int wm;
    ZmqGetContext(obj);
    Check_Type(hwm, T_FIXNUM);
    rb_warn("Deprecated method ZMQ::Context#hwm=, does nothing - to be removed after 2013/05/14");
    wm = FIX2INT(hwm);
    if (wm < 0) rb_raise(rb_eZmqError, "negative HWM values is not supported.");
    zctx_set_hwm(ctx->ctx, wm);
    return Qnil;
}

/*
 * :nodoc:
 *  Creates a new socket while the GIL is released.
 *
*/
VALUE rb_czmq_nogvl_socket_new(void *ptr)
{
    errno = 0;
    struct nogvl_socket_args *args = ptr;
    return (VALUE)zsocket_new(args->ctx, args->type);
}

/*
 * :nodoc:
 *  Maps a Ruby class to a ZMQ socket type.
 *
*/
static inline VALUE rb_czmq_ctx_socket_klass(int socket_type)
{
    switch (socket_type) {
        case ZMQ_PUB: return rb_cZmqPubSocket;
                      break;
        case ZMQ_SUB: return rb_cZmqSubSocket;
                      break;
        case ZMQ_PUSH: return rb_cZmqPushSocket;
                       break;
        case ZMQ_PULL: return rb_cZmqPullSocket;
                       break;
        case ZMQ_PAIR: return rb_cZmqPairSocket;
                       break;
        case ZMQ_REQ: return rb_cZmqReqSocket;
                      break;
        case ZMQ_REP: return rb_cZmqRepSocket;
                      break;
        case ZMQ_ROUTER: return rb_cZmqRouterSocket;
                         break;
        case ZMQ_DEALER: return rb_cZmqDealerSocket;
                         break;
        case ZMQ_XPUB: return rb_cZmqXPubSocket;
                      break;
        case ZMQ_XSUB: return rb_cZmqXSubSocket;
                      break;
        default: rb_raise(rb_eZmqError, "ZMQ socket type %d not supported!", socket_type);
                 break;
    }
}

/*
 *  call-seq:
 *     ctx.socket(:PUSH)    =>  ZMQ::Socket
 *     ctx.socket(ZMQ::PUSH)    =>  ZMQ::Socket
 *
 *  Creates a socket within this ZMQ context. This is the only API exposed for creating sockets - they're always spawned off
 *  a context. Sockets also track state of the current Ruby thread they're created in to ensure they always only ever do work
 *  on the thread they were spawned on.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     ctx.socket(:PUSH)    =>   ZMQ::Socket
 *     ctx.socket(ZMQ::PUSH)    =>  ZMQ::Socket
 *
*/

static VALUE rb_czmq_ctx_socket(VALUE obj, VALUE type)
{
    VALUE socket;
    int socket_type;
    struct nogvl_socket_args args;
    zmq_sock_wrapper *sock = NULL;
    errno = 0;
    ZmqGetContext(obj);
    if (TYPE(type) != T_FIXNUM && TYPE(type) != T_SYMBOL) rb_raise(rb_eTypeError, "wrong socket type %s (expected Fixnum or Symbol)", RSTRING_PTR(rb_obj_as_string(type)));
    socket_type = FIX2INT((SYMBOL_P(type)) ? rb_const_get_at(rb_mZmq, rb_to_id(type)) : type);

    socket = Data_Make_Struct(rb_czmq_ctx_socket_klass(socket_type), zmq_sock_wrapper, rb_czmq_mark_sock, rb_czmq_free_sock_gc, sock);
    args.ctx = ctx->ctx;
    args.type = socket_type;
    sock->socket = (void*)rb_thread_blocking_region(rb_czmq_nogvl_socket_new, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssertObjOnAlloc(sock->socket, sock);
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    sock->str_buffer = zlist_new();
    sock->frame_buffer = zlist_new();
    sock->msg_buffer = zlist_new();
#endif
    sock->flags = 0;
    sock->ctx = ctx->ctx;
    sock->verbose = false;
    sock->state = ZMQ_SOCKET_PENDING;
    sock->endpoints = rb_ary_new();
    sock->thread = rb_thread_current();
    sock->context = obj;
    sock->monitor_endpoint = Qnil;
    sock->monitor_handler = Qnil;
    sock->monitor_thread = Qnil;
    rb_obj_call_init(socket, 0, NULL);
    return socket;
}

void _init_rb_czmq_context()
{
    intern_zctx_process = rb_intern("@__zmq_ctx_process");
    rb_ivar_set(rb_mZmq, intern_zctx_process, rb_hash_new());

    rb_cZmqContext = rb_define_class_under(rb_mZmq, "Context", rb_cObject);

    rb_define_singleton_method(rb_cZmqContext, "new", rb_czmq_ctx_s_new, -1);
    rb_define_method(rb_cZmqContext, "destroy", rb_czmq_ctx_destroy, 0);
    rb_define_method(rb_cZmqContext, "iothreads=", rb_czmq_ctx_set_iothreads, 1);
    rb_define_method(rb_cZmqContext, "linger=", rb_czmq_ctx_set_linger, 1);
    rb_define_method(rb_cZmqContext, "socket", rb_czmq_ctx_socket, 1);
    rb_define_method(rb_cZmqContext, "hwm", rb_czmq_ctx_hwm, 0);
    rb_define_method(rb_cZmqContext, "hwm=", rb_czmq_ctx_set_hwm, 1);
}
