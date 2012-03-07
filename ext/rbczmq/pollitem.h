#ifndef RBCZMQ_POLLITEM_H
#define RBCZMQ_POLLITEM_H

typedef struct {
    VALUE socket;
    VALUE io;
    VALUE events;
    VALUE handler;
    zmq_pollitem_t *item;
} zmq_pollitem_wrapper;

#define ZmqAssertPollitem(obj) ZmqAssertType(obj, rb_cZmqPollitem, "ZMQ::Pollitem")
#define ZmqGetPollitem(obj) \
    zmq_pollitem_wrapper *pollitem = NULL; \
    ZmqAssertPollitem(obj); \
    Data_Get_Struct(obj, zmq_pollitem_wrapper, pollitem); \
    if (!pollitem) rb_raise(rb_eTypeError, "uninitialized ZMQ pollitem!");

#define ZmqAssertHandler(obj, pollitem, handler, callback) \
    if (!rb_respond_to(handler, (callback))) \
        rb_raise(rb_eZmqError, "Pollable entity %s's handler %s expected to implement an %s callback!", RSTRING_PTR(rb_obj_as_string(rb_czmq_pollitem_pollable((obj)))), rb_obj_classname(handler), rb_id2name((callback)));

VALUE rb_czmq_pollitem_coerce(VALUE pollable);
VALUE rb_czmq_pollitem_pollable(VALUE obj);
VALUE rb_czmq_pollitem_events(VALUE obj);

void _init_rb_czmq_pollitem();

#endif