#ifndef RBCZMQ_POLLER_H
#define RBCZMQ_POLLER_H

typedef struct {
    VALUE pollables;
    VALUE readables;
    VALUE writables;
    zmq_pollitem_t *pollset;
    int poll_size;
    bool dirty;
    bool verbose;
} zmq_poll_wrapper;

#define ZmqAssertPoller(obj) ZmqAssertType(obj, rb_cZmqPoller, "ZMQ::Poller")
#define ZmqGetPoller(obj) \
    zmq_poll_wrapper *poller = NULL; \
    ZmqAssertPoller(obj); \
    Data_Get_Struct(obj, zmq_poll_wrapper, poller); \
    if (!poller) rb_raise(rb_eTypeError, "uninitialized ZMQ poller!");

struct nogvl_poll_args {
    zmq_pollitem_t *items;
    int nitems;
    long timeout;
};

void _init_rb_czmq_poller();

#endif
