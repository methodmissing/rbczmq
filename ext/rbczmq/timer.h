#ifndef RBCZMQ_TIMER_H
#define RBCZMQ_TIMER_H

typedef struct {
    size_t delay;
    size_t times;
    Bool cancelled;
    VALUE callback;
} zmq_timer_wrapper;

#define ZmqAssertTimer(obj) ZmqAssertType(obj, rb_cZmqTimer, "ZMQ::Timer")
#define ZmqGetTimer(obj) \
    zmq_timer_wrapper *timer = NULL; \
    ZmqAssertTimer(obj); \
    Data_Get_Struct(obj, zmq_timer_wrapper, timer); \
    if (!timer) rb_raise(rb_eTypeError, "uninitialized ZMQ timer!");

VALUE rb_czmq_timer_s_new(int argc, VALUE *argv, VALUE timer);

void _init_rb_czmq_timer();

#endif

