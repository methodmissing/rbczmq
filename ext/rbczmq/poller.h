#ifndef RBCZMQ_POLLER_H
#define RBCZMQ_POLLER_H

typedef struct {
    VALUE sockets;
    VALUE readables;
    VALUE writables;
    st_table * socket_map;
    zmq_pollitem_t *pollset;
    int poll_size;
    Bool dirty;
} zmq_poll_wrapper;

typedef struct {
    VALUE socket;
    zmq_pollitem_t *pollitem;
} zmq_poll_item_wrapper;

#define ZmqAssertPoller(obj) ZmqAssertType(obj, rb_cZmqPoller, "ZMQ::Poller")
#define ZmqGetPoller(obj) \
    zmq_poll_wrapper *poller = NULL; \
    ZmqAssertPoller(obj); \
    Data_Get_Struct(obj, zmq_poll_wrapper, poller); \
    if (!poller) rb_raise(rb_eTypeError, "uninitialized ZMQ poller!");

void _init_rb_czmq_poller();

#endif