#ifndef RBCZMQ_LOOP_H
#define RBCZMQ_LOOP_H

typedef struct {
    zloop_t  *loop;
    Bool verbose;
    Bool running;
} zmq_loop_wrapper;

#define ZmqAssertLoop(obj) ZmqAssertType(obj, rb_cZmqLoop, "ZMQ::Loop")
#define ZmqGetLoop(obj) \
    zmq_loop_wrapper *loop = NULL; \
    ZmqAssertLoop(obj); \
    Data_Get_Struct(obj, zmq_loop_wrapper, loop); \
    if (!loop) rb_raise(rb_eTypeError, "uninitialized ZMQ loop!");

void _init_rb_czmq_loop();

#endif
