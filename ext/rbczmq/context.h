#ifndef RBCZMQ_CONTEXT_H
#define RBCZMQ_CONTEXT_H

#define ZMQ_CONTEXT_DESTROYED 0x01

typedef struct {
    zctx_t *ctx;
    int flags;
} zmq_ctx_wrapper;

#define ZmqAssertContext(obj) ZmqAssertType(obj, rb_cZmqContext, "ZMQ::Context")
#define ZmqGetContext(obj) \
    zmq_ctx_wrapper *ctx = NULL; \
    ZmqAssertContext(obj); \
    Data_Get_Struct(obj, zmq_ctx_wrapper, ctx); \
    if (!ctx) rb_raise(rb_eTypeError, "uninitialized ZMQ context!"); \
    if (ctx->flags & ZMQ_CONTEXT_DESTROYED) rb_raise(rb_eZmqError, "ZMQ::Context instance %p has been destroyed by the ZMQ framework", (void *)obj);

struct nogvl_socket_args {
    zctx_t *ctx;
    int type;
};

void _init_rb_czmq_context();

#endif
