#ifndef RBCZMQ_MESSAGE_H
#define RBCZMQ_MESSAGE_H

typedef struct {
    zmsg_t  *message;
} zmq_message_wrapper;

#define ZmqAssertMessage(obj) ZmqAssertType(obj, rb_cZmqMessage, "ZMQ::Message")
#define ZmqGetMessage(obj) \
    zmq_message_wrapper *message = NULL; \
    ZmqAssertMessage(obj); \
    Data_Get_Struct(obj, zmq_message_wrapper, message); \
    if (!message) rb_raise(rb_eTypeError, "uninitialized ZMQ message!");

VALUE rb_czmq_alloc_message(zmsg_t *message);

void _init_rb_czmq_message();

#endif
