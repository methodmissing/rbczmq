#include <rbczmq_ext.h>

static VALUE intern_zctx_process;

static VALUE rb_czmq_nogvl_zctx_destroy(void *ptr)
{
    zmq_ctx_wrapper *ctx = ptr;
    zctx_destroy(&ctx->ctx);
    return Qnil;
}

static void rb_czmq_free_ctx(zmq_ctx_wrapper *ctx)
{
    VALUE ctx_map;
    rb_thread_blocking_region(rb_czmq_nogvl_zctx_destroy, ctx, RUBY_UBF_IO, 0);
    ctx->ctx = NULL;
    ctx_map = rb_ivar_get(rb_mZmq, intern_zctx_process);
    rb_hash_aset(ctx_map, get_pid(), Qnil);
}

static void rb_czmq_free_ctx_gc(void *ptr)
{
    zmq_ctx_wrapper *ctx = ptr;
    if (ctx) {
        rb_czmq_free_ctx(ctx);
        xfree(ctx);
    }
}

static VALUE rb_czmq_nogvl_zctx_new(ZMQ_UNUSED void *ptr)
{
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
 *  Returns a handle to a new ZMQ context. Optional I/O threads argument.
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
    rb_hash_aset(ctx_map, get_pid(), context);
    rb_obj_call_init(context, 0, NULL);
    if (!NIL_P(io_threads)) rb_czmq_ctx_set_iothreads(context, io_threads);
    return context;
}

/*
 *  call-seq:
 *     ctx.destroy    =>  nil
 *
 *  Destroy a ZMQ context and all sockets in it.
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
 *  Raises default I/O threads from 1.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     ctx.iothreads = 2    =>   nil
 *
*/

static VALUE rb_czmq_ctx_set_iothreads(VALUE obj, VALUE threads)
{
    int iothreads;
    ZmqGetContext(obj);
    Check_Type(threads, T_FIXNUM);
    iothreads = FIX2INT(threads);
    if (iothreads > 1) rb_warn("You probably don't want to spawn more than 1 I/O thread per ZMQ context.");
    zctx_set_iothreads(ctx->ctx, iothreads);
    if (zmq_errno() == EINVAL) ZmqRaiseSysError();
    return Qnil;
}

/*
 *  call-seq:
 *     ctx.linger = 100    =>  nil
 *
 *  Set msecs to flush sockets when closing them
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     ctx.linger = 100    =>   nil
 *
*/

static VALUE rb_czmq_ctx_set_linger(VALUE obj, VALUE linger)
{
    ZmqGetContext(obj);
    Check_Type(linger, T_FIXNUM);
   /* XXX: boundary checks */
    zctx_set_linger(ctx->ctx, FIX2INT(linger));
    return Qnil;
}

VALUE rb_czmq_nogvl_socket_new(void *ptr) {
    struct nogvl_socket_args *args = ptr;
    return (VALUE)zsocket_new(args->ctx, args->type);
}

static inline VALUE rb_czmq_ctx_socket_klass(int socket_type) {
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
        default: rb_raise(rb_eZmqError, "ZMQ socket type %d not supported!", socket_type);
                 break;
    }
}

/*
 *  call-seq:
 *     ctx.socket(:PUSH)    =>  ZMQ::Socket
 *
 *  Creates a socket within this ZMQ context.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     ctx.socket(:PUSH)    =>   ZMQ::Socket
 *
*/

static VALUE rb_czmq_ctx_socket(VALUE obj, VALUE type)
{
    VALUE socket;
    int socket_type;
    struct nogvl_socket_args args;
    zmq_sock_wrapper *sock = NULL;
    ZmqGetContext(obj);
    if (TYPE(type) != T_FIXNUM && TYPE(type) != T_SYMBOL) rb_raise(rb_eTypeError, "wrong socket type %s (expected Fixnum or Symbol)", RSTRING_PTR(rb_obj_as_string(type)));
    socket_type = FIX2INT((SYMBOL_P(type)) ? rb_const_get_at(rb_mZmq, rb_to_id(type)) : type);

    socket = Data_Make_Struct(rb_czmq_ctx_socket_klass(socket_type), zmq_sock_wrapper, rb_czmq_mark_sock, rb_czmq_free_sock_gc, sock);
    args.ctx = ctx->ctx;
    args.type = socket_type;
    sock->socket = (void*)rb_thread_blocking_region(rb_czmq_nogvl_socket_new, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssertObjOnAlloc(sock->socket, sock);
    sock->handler = Qnil;
    sock->ctx = ctx->ctx;
    sock->verbose = FALSE;
    sock->state = ZMQ_SOCKET_PENDING;
    sock->endpoint = Qnil;
    sock->thread = rb_thread_current();
    sock->recv_timeout = ZMQ_SOCKET_DEFAULT_TIMEOUT;
    sock->send_timeout = ZMQ_SOCKET_DEFAULT_TIMEOUT;
    rb_obj_call_init(socket, 0, NULL);
    return socket;
}

void _init_rb_czmq_context() {
    intern_zctx_process = rb_intern("@__zmq_ctx_process");
    rb_ivar_set(rb_mZmq, intern_zctx_process, rb_hash_new());

    rb_cZmqContext = rb_define_class_under(rb_mZmq, "Context", rb_cObject);

    rb_define_singleton_method(rb_cZmqContext, "new", rb_czmq_ctx_s_new, -1);
    rb_define_method(rb_cZmqContext, "destroy", rb_czmq_ctx_destroy, 0);
    rb_define_method(rb_cZmqContext, "iothreads=", rb_czmq_ctx_set_iothreads, 1);
    rb_define_method(rb_cZmqContext, "linger=", rb_czmq_ctx_set_linger, 1);
    rb_define_method(rb_cZmqContext, "socket", rb_czmq_ctx_socket, 1);
}
