#ifndef RBCZMQ_BEACON_H
#define RBCZMQ_BEACON_H

typedef struct {
    zbeacon_t *beacon;
} zmq_beacon_wrapper;

#define ZmqAssertBeacon(obj) ZmqAssertType(obj, rb_cZmqBeacon, "ZMQ::Beacon")
#define GetZmqBeacon(obj) \
    zmq_beacon_wrapper *beacon = NULL; \
    ZmqAssertBeacon(obj); \
    Data_Get_Struct(obj, zmq_beacon_wrapper, beacon); \
    if (!beacon) rb_raise(rb_eTypeError, "uninitialized ZMQ beacon!!"); \

struct nogvl_beacon_destroy_args {
    zmq_beacon_wrapper *beacon;
};

struct nogvl_beacon_interval_args {
    zmq_beacon_wrapper *beacon;
    int interval;
};

struct nogvl_beacon_publish_args {
    zmq_beacon_wrapper *beacon;
    char *transmit;
    int length;
};

struct nogvl_beacon_subscribe_args {
    zmq_beacon_wrapper *beacon;
    char *filter;
    int length;
};

void _init_rb_czmq_beacon();

#endif