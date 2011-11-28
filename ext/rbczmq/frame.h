#ifndef RBCZMQ_FRAME_H
#define RBCZMQ_FRAME_H

#define ZMQ_FRAME_RECYCLED 2
#define ZMQ_FRAME_MESSAGE 4
#define ZMQ_FRAME_NEW 8
#define ZMQ_FRAME_ALLOC 16
#define ZMQ_FRAME_DUP 32

typedef struct {
    zframe_t  *frame;
    int flags;
} zmq_frame_wrapper;

#define ZmqAssertFrame(obj) ZmqAssertType(obj, rb_cZmqFrame, "ZMQ::Frame")
#define ZmqGetFrame(obj) \
    zmq_frame_wrapper *frame = NULL; \
    ZmqAssertFrame(obj); \
    Data_Get_Struct(obj, zmq_frame_wrapper, frame); \
    if (!frame) rb_raise(rb_eTypeError, "uninitialized ZMQ frame!");

void rb_czmq_free_frame(zmq_frame_wrapper *frame);
void rb_czmq_free_frame_gc(void *ptr);

VALUE rb_czmq_alloc_frame(zframe_t **frame, int flags);

void _init_rb_czmq_frame();

#endif
