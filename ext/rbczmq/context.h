#ifndef RBCZMQ_CONTEXT_H
#define RBCZMQ_CONTEXT_H

typedef struct {
    zctx_t *ctx;
} zmq_ctx_wrapper;

#define ZmqAssertContext(obj) ZmqAssertType(obj, rb_cZmqContext, "ZMQ::Context")
#define ZmqGetContext(obj) \
    zmq_ctx_wrapper *ctx = NULL; \
    ZmqAssertContext(obj); \
    Data_Get_Struct(obj, zmq_ctx_wrapper, ctx); \
    if (!ctx) rb_raise(rb_eTypeError, "uninitialized ZMQ context!");

void _init_rb_czmq_context();

#endif
