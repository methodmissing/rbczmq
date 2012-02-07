#ifndef RBCZMQ_MESSAGE_H
#define RBCZMQ_MESSAGE_H

#define ZMQ_MESSAGE_DESTROYED 0x01

typedef struct {
    zmsg_t  *message;
    int flags;
} zmq_message_wrapper;

#define ZmqAssertMessage(obj) ZmqAssertType(obj, rb_cZmqMessage, "ZMQ::Message")
#define ZmqGetMessage(obj) \
    zmq_message_wrapper *message = NULL; \
    ZmqAssertMessage(obj); \
    Data_Get_Struct(obj, zmq_message_wrapper, message); \
    if (!message) rb_raise(rb_eTypeError, "uninitialized ZMQ message!"); \
    if (message->flags & ZMQ_MESSAGE_DESTROYED) rb_raise(rb_eZmqError, "object %p has been destroyed by the ZMQ framework", (void *)obj);

VALUE rb_czmq_alloc_message(zmsg_t *message);
void rb_czmq_free_message(zmq_message_wrapper *message);

void _init_rb_czmq_message();

#endif
