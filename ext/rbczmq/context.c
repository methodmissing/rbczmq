#include <rbczmq_ext.h>

static VALUE intern_zctx_process;

static void rb_czmq_free_ctx(zmq_ctx_wrapper *ctx)
{
    TRAP_BEG;
    zctx_destroy(&ctx->ctx);
    TRAP_END;
    ctx->ctx = NULL;
    rb_ivar_set(rb_mZmq, intern_zctx_process, Qnil);
}

static void rb_czmq_free_ctx_gc(void *ptr)
{
    zmq_ctx_wrapper *ctx = ptr;
    ZmqDebug("rb_czmq_free_ctx_gc");
    if (ctx) {
        rb_czmq_free_ctx(ctx);
        xfree(ctx);
    }
}

static VALUE rb_czmq_ctx_new(VALUE context)
{
    zmq_ctx_wrapper *ctx = NULL;
    if (!NIL_P(rb_ivar_get(rb_mZmq, intern_zctx_process))) rb_raise(rb_eZmqError, "single ZMQ context per process allowed");
    context = Data_Make_Struct(rb_cZmqContext, zmq_ctx_wrapper, 0, rb_czmq_free_ctx_gc, ctx);
    ctx->ctx = zctx_new();
    ZmqAssertObjOnAlloc(ctx->ctx, ctx);
    rb_ivar_set(rb_mZmq, intern_zctx_process, context);
    rb_obj_call_init(context, 0, NULL);
    return context;
}

static VALUE rb_czmq_ctx_destroy(VALUE obj)
{
    ZmqGetContext(obj);
    rb_czmq_free_ctx(ctx);
    return Qnil;
}

static VALUE rb_czmq_ctx_set_iothreads(VALUE obj, VALUE threads)
{
    ZmqGetContext(obj);
    Check_Type(threads, T_FIXNUM);
    rb_warn("You probably don't want to spawn more than 1 IO thread per ZMQ context.");
    zctx_set_iothreads(ctx->ctx, FIX2INT(threads));
    return Qnil;
}

static VALUE rb_czmq_ctx_set_linger(VALUE obj, VALUE linger)
{
    ZmqGetContext(obj);
    Check_Type(linger, T_FIXNUM);
   /* XXX: boundary checks */
    zctx_set_linger(ctx->ctx, FIX2INT(linger));
    return Qnil;
}

static VALUE rb_czmq_ctx_socket(VALUE obj, VALUE type)
{
    VALUE socket, socket_type;
    zmq_sock_wrapper *sock = NULL;
    ZmqGetContext(obj);
    if (TYPE(type) != T_FIXNUM && TYPE(type) != T_SYMBOL) rb_raise(rb_eTypeError, "wrong socket type %s (expected Fixnum or Symbol)", RSTRING_PTR(rb_obj_as_string(type)));
    socket_type = (SYMBOL_P(type)) ? rb_const_get_at(rb_mZmq, rb_to_id(type)) : type;

    socket = Data_Make_Struct(rb_cZmqSocket, zmq_sock_wrapper, rb_czmq_mark_sock, rb_czmq_free_sock_gc, sock);
    TRAP_BEG;
    sock->socket = zsocket_new(ctx->ctx, FIX2INT(socket_type));
    TRAP_END;
    ZmqAssertObjOnAlloc(sock->socket, sock);
    sock->handler = Qnil;
    sock->ctx = ctx->ctx;
    sock->fd = rb_czmq_get_sock_fd(sock->socket);
    sock->verbose = FALSE;
    sock->state = ZMQ_SOCKET_PENDING;
    sock->endpoint = Qnil;
    rb_obj_call_init(socket, 0, NULL);
    return socket;
}

void _init_rb_czmq_context() {
    intern_zctx_process = rb_intern("@__zmq_ctx_process");

    rb_cZmqContext = rb_define_class_under(rb_mZmq, "Context", rb_cObject);

    rb_define_alloc_func(rb_cZmqContext, rb_czmq_ctx_new);
    rb_define_method(rb_cZmqContext, "destroy", rb_czmq_ctx_destroy, 0);
    rb_define_method(rb_cZmqContext, "iothreads=", rb_czmq_ctx_set_iothreads, 1);
    rb_define_method(rb_cZmqContext, "linger=", rb_czmq_ctx_set_linger, 1);
    rb_define_method(rb_cZmqContext, "socket", rb_czmq_ctx_socket, 1);
}
