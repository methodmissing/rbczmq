#ifndef RBCZMQ_MESSAGE_H
#define RBCZMQ_MESSAGE_H

/* This flag indicates that the wrapped zmsg_t is owned by ruby
   and can be freed when the ZMQ::Message object is garbage collected */
#define ZMQ_MESSAGE_OWNED 0x01

typedef struct {
    zmsg_t  *message;
    int flags;
    /* a zlist of frame wrapper objects for the frames in this message */
    zlist_t *frames;
} zmq_message_wrapper;

#define ZmqAssertMessage(obj) ZmqAssertType(obj, rb_cZmqMessage, "ZMQ::Message")
#define ZmqGetMessage(obj) \
    zmq_message_wrapper *message = NULL; \
    ZmqAssertMessage(obj); \
    Data_Get_Struct(obj, zmq_message_wrapper, message); \
    if (!message) rb_raise(rb_eTypeError, "uninitialized ZMQ message!");

    /*
    if (message->flags & ZMQ_MESSAGE_DESTROYED) rb_raise(rb_eZmqError, "ZMQ::Message instance %p has been destroyed by the ZMQ framework", (void *)obj);
    */

#define ZmqReturnNilUnlessOwned(wrapper) if (wrapper && (wrapper->flags & ZMQ_MESSAGE_OWNED) == 0) { \
    return Qnil; \
}

#define ZmqAssertMessageOwned(wrapper) if (wrapper && (wrapper->flags & ZMQ_MESSAGE_OWNED) == 0) { \
    rb_raise(rb_eTypeError, "Cannot modify a message that is gone."); \
}

VALUE rb_czmq_alloc_message(zmsg_t *message);
void rb_czmq_free_message(zmq_message_wrapper *message);
void rb_czmq_mark_message(zmq_message_wrapper *message);

void _init_rb_czmq_message();

#endif
