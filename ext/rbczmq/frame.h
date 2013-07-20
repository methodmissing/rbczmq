#ifndef RBCZMQ_FRAME_H
#define RBCZMQ_FRAME_H

#include "message.h"

/* This flag indicates that the wrapped zframe_t is owned by ruby
   and can be freed when the ZMQ::Frame object is garbage collected */
#define ZMQ_FRAME_OWNED 0x01

typedef struct {
    /* The czmq frame object. This is only valid if the frame is owned
       by ruby, or the frame has been added to a message and the message
       is owned by ruby. */
    zframe_t *frame;

    /* The ruby ZMQ::Message object this frame is attached to, or NULL */
    zmq_message_wrapper* message;

    int flags;
} zmq_frame_wrapper;

#define ZmqAssertFrame(obj) ZmqAssertType(obj, rb_cZmqFrame, "ZMQ::Frame")
#define ZmqGetFrame(obj) \
    zmq_frame_wrapper *frame = NULL; \
    ZmqAssertFrame(obj); \
    Data_Get_Struct(obj, zmq_frame_wrapper, frame); \
    if (!frame) rb_raise(rb_eTypeError, "uninitialized ZMQ frame!");

#define ZmqAssertFrameOwned(wrapper) if (!(\
      (wrapper->flags & ZMQ_FRAME_OWNED) != 0 || \
      (wrapper->message != NULL && (wrapper->message->flags & ZMQ_MESSAGE_OWNED) != 0 ) \
    )) { \
        rb_raise(rb_eZmqError, "Cannot access frame that belongs to another message or is gone."); \
    }

#define ZmqAssertFrameOwnedNoMessage(wrapper) if (!(\
      (wrapper->flags & ZMQ_FRAME_OWNED) != 0 \
    )) { \
        rb_raise(rb_eZmqError, "Cannot access frame that belongs to another message or is gone."); \
    }

#define ZmqReturnNilUnlessFrameOwned(wrapper) if (!(\
      (wrapper->flags & ZMQ_FRAME_OWNED) != 0 || \
      (wrapper->message != NULL && (wrapper->message->flags & ZMQ_MESSAGE_OWNED) != 0 ) \
    )) { \
        return Qnil; \
    }

void rb_czmq_free_frame(zmq_frame_wrapper *frame);
void rb_czmq_free_frame_gc(void *ptr);

VALUE rb_czmq_alloc_frame(zframe_t *frame);

void _init_rb_czmq_frame();

#endif
