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

void _init_rb_czmq_beacon();

#endif