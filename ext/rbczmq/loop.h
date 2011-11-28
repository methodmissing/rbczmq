#ifndef RBCZMQ_LOOP_H
#define RBCZMQ_LOOP_H

typedef struct {
    zloop_t  *loop;
    Bool running;
} zmq_loop_wrapper;

#define ZmqGetLoop(obj) \
    zmq_loop_wrapper *loop = NULL; \
    Data_Get_Struct(obj, zmq_loop_wrapper, loop); \
    if (!loop) rb_raise(rb_eTypeError, "uninitialized ZMQ loop!");

void _init_rb_czmq_loop();

#endif
