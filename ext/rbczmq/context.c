#include "rbczmq_ext.h"

static VALUE intern_zctx_process;
static zmutex_t* context_mutex = NULL; /* this can only be used outside of Ruby GVL to avoid deadlocks */

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
    zmutex_lock(context_mutex);

    errno = 0;
    zmq_ctx_wrapper *ctx = ptr;
    if (ctx->pid == getpid()) {
        /* Only actually destroy the context if we are the process that created it. */
        /* This may need to be changed when ZeroMQ's support for forking is improved. */
        zctx_destroy(&ctx->ctx);
    }
    ctx->flags |= ZMQ_CONTEXT_DESTROYED;

    zmutex_unlock(context_mutex);

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

    // destroy sockets. This duplicates czmq's context shutdown which destroys sockets,
    // but we need to process the list of Ruby ZMQ::Socket objects' data objects to mark
    // them as closed so that they cannot be double destroyed.
    zmq_sock_wrapper* socket;
    while ((socket = zlist_pop(ctx->sockets))) {
        rb_czmq_context_destroy_socket(socket);
    }

    // finally, shutdown the context.
    rb_thread_blocking_region(rb_czmq_nogvl_zctx_destroy, ctx, RUBY_UBF_IO, 0);

    ctx->ctx = NULL;
    rb_hash_aset(ctx_map, ctx->pidValue, Qnil);
    zlist_destroy(&ctx->sockets);
}

/*
 * :nodoc:
 *  Destroy the socket while the GIL is released - may block depending on socket linger value.
 *
*/
VALUE rb_czmq_nogvl_zsocket_destroy(void *ptr)
{
    zmutex_lock(context_mutex);

    zmq_sock_wrapper *sock = ptr;
    errno = 0;
    // zclock_log ("I: %s socket %p, context %p: destroy", zsocket_type_str(sock->socket), sock, sock->ctx);
    zsocket_destroy(sock->ctx, sock->socket);

    zmutex_unlock(context_mutex);
    return Qnil;
}

/*
 * :nodoc:
 *
 * Interal use: Close a socket from the context being destroyed or garbage collected, or
 * from the socket being closed, or garbage collected.
 *
 * The order of these events cannot be determined, so this function has to be idempotent
 * about this. The close socket call to CZMQ must happen only once or it will abort the
 * application.
 */
void rb_czmq_context_destroy_socket(zmq_sock_wrapper* socket)
{
    if (socket == NULL || socket->context == NULL) {
        return;
    }

    if (socket && socket->context == Qnil) {
        // A socket with a context object of Qnil is created by ZMQ::Beacon#new.
        // zbeacon is responsible for closing this socket in its own context, we will simply mark
        // it as destroyed and let the ZMQ::Beacon object do the clean up when its internal
        // context is destroyed.
    }
    else if (socket->ctx && socket->ctx_wrapper && !(socket->flags & ZMQ_SOCKET_DESTROYED)) {
        zmq_ctx_wrapper *ctx = (zmq_ctx_wrapper *)socket->ctx_wrapper;
        if (ctx) {
            // remove from the list of socket objects created by this context object.
            zlist_remove(ctx->sockets, socket);

            if (socket->socket) {
                rb_thread_blocking_region(rb_czmq_nogvl_zsocket_destroy, socket, RUBY_UBF_IO, 0);
            }
        }
    }

    socket->flags |= ZMQ_SOCKET_DESTROYED;
    socket->ctx = NULL;
    socket->socket = NULL;
    socket->state = ZMQ_SOCKET_DISCONNECTED;
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
 *  GC mark callback
 *
*/
static void rb_czmq_mark_ctx_gc(void *ptr)
{
    zmq_ctx_wrapper *ctx = (zmq_ctx_wrapper *)ptr;
    if (ctx) {
        rb_gc_mark(ctx->pidValue);
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
    zsys_handler_reset(); // restore ruby signal handlers.
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
    context = Data_Make_Struct(rb_cZmqContext, zmq_ctx_wrapper, rb_czmq_mark_ctx_gc, rb_czmq_free_ctx_gc, ctx);
    ctx->ctx = (zctx_t*)rb_thread_blocking_region(rb_czmq_nogvl_zctx_new, NULL, RUBY_UBF_IO, 0);
    ZmqAssertObjOnAlloc(ctx->ctx, ctx);
    ctx->flags = 0;
    ctx->pid = getpid();
    ctx->pidValue = get_pid();
    ctx->sockets = zlist_new();
    rb_obj_call_init(context, 0, NULL);
    rb_hash_aset(ctx_map, ctx->pidValue, context);
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
    ZmqAssertContextPidMatches(ctx);
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
    ZmqAssertContextPidMatches(ctx);
    Check_Type(threads, T_FIXNUM);
    iothreads = FIX2INT(threads);
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
    ZmqAssertContextPidMatches(ctx);
    Check_Type(linger, T_FIXNUM);
    msecs = FIX2INT(linger);
    if (msecs < 0) rb_raise(rb_eZmqError, "negative linger / timeout values is not supported.");
    zctx_set_linger(ctx->ctx, msecs);
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
    zmutex_lock(context_mutex);
    VALUE result = (VALUE)zsocket_new(args->ctx, args->type);
    zmutex_unlock(context_mutex);
    return result;
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
        case ZMQ_STREAM: return rb_cZmqStreamSocket;
                         break;
        default: rb_raise(rb_eZmqError, "ZMQ socket type %d not supported!", socket_type);
                 break;
    }
}

VALUE rb_czmq_socket_alloc(VALUE context, zctx_t *zctx, void *s)
{
    VALUE socket;
    zmq_sock_wrapper *sock = NULL;
    socket = Data_Make_Struct(rb_czmq_ctx_socket_klass(zsocket_type(s)), zmq_sock_wrapper, rb_czmq_mark_sock, rb_czmq_free_sock_gc, sock);
    sock->socket = s;
    ZmqAssertObjOnAlloc(sock->socket, sock);
    sock->flags = 0;
    sock->ctx = zctx; // czmq context
    if (context == Qnil) {
        sock->ctx_wrapper = NULL;
    } else {
        ZmqGetContext(context);
        sock->ctx_wrapper = ctx; // rbczmq ZMQ::Context wrapped data struct
    }
    sock->verbose = false;
    sock->state = ZMQ_SOCKET_PENDING;
    sock->endpoints = rb_ary_new();
    sock->thread = rb_thread_current();
    sock->context = context;
    sock->monitor_endpoint = Qnil;
    sock->monitor_handler = Qnil;
    sock->monitor_thread = Qnil;
    rb_obj_call_init(socket, 0, NULL);
    return socket;
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
    int socket_type;
    struct nogvl_socket_args args;
    errno = 0;
    ZmqGetContext(obj);
    ZmqAssertContextPidMatches(ctx);
    if (TYPE(type) != T_FIXNUM && TYPE(type) != T_SYMBOL) rb_raise(rb_eTypeError, "wrong socket type %s (expected Fixnum or Symbol)", RSTRING_PTR(rb_obj_as_string(type)));
    socket_type = FIX2INT((SYMBOL_P(type)) ? rb_const_get_at(rb_mZmq, rb_to_id(type)) : type);

    args.ctx = ctx->ctx;
    args.type = socket_type;
    VALUE socket_object = rb_czmq_socket_alloc(obj, ctx->ctx, (void*)rb_thread_blocking_region(rb_czmq_nogvl_socket_new, (void *)&args, RUBY_UBF_IO, 0));
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(socket_object);
    zlist_push(ctx->sockets, sock);
    return socket_object;
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

    context_mutex = zmutex_new();
}
