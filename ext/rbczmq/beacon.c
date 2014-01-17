#include "rbczmq_ext.h"

/*
 * :nodoc:
 *  Destroy the beacon while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_beacon_destroy(void *ptr)
{
    zmq_beacon_wrapper *beacon = ptr;
    if (beacon->beacon) {
        zbeacon_destroy(&beacon->beacon);
        beacon->beacon = NULL;
    }
    return Qnil;
}

/*
 * :nodoc:
 *  GC free callback
 *
*/
static void rb_czmq_free_beacon_gc(void *ptr)
{
    zmq_beacon_wrapper *beacon = (zmq_beacon_wrapper *)ptr;
    if (beacon) {
        rb_thread_blocking_region(rb_czmq_nogvl_beacon_destroy, beacon, RUBY_UBF_IO, 0);
        xfree(beacon);
    }
}

/*
 * :nodoc:
 *  Allocate a beacon while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_new_beacon(void *ptr)
{
    int port = (int)ptr;
    return (VALUE)zbeacon_new(port);
}

/*
 *  call-seq:
 *     ZMQ::Beacon.new(9999)   =>  ZMQ::Beacon
 *
 *  Create a new beacon.
 *
 * === Examples
 *     ZMQ::Beacon.new(9999)    =>  ZMQ::Beacon
 *
*/
static VALUE rb_czmq_beacon_s_new(VALUE beacon, VALUE port)
{
    zmq_beacon_wrapper *bcn = NULL;
    int prt;
    Check_Type(port, T_FIXNUM);
    beacon = Data_Make_Struct(rb_cZmqBeacon, zmq_beacon_wrapper, 0, rb_czmq_free_beacon_gc, bcn);
    prt = FIX2INT(port);
    bcn->beacon = (zbeacon_t*)rb_thread_blocking_region(rb_czmq_nogvl_new_beacon, (void *)&prt, RUBY_UBF_IO, 0);
    ZmqAssertObjOnAlloc(bcn->beacon, bcn);
    rb_obj_call_init(beacon, 0, NULL);
    return beacon;
}

/*
 *  call-seq:
 *     beacon.destroy   =>  nil
 *
 *  Stop broadcasting a beacon. The GC will take the same action if a beacon object is not reachable anymore on the next
 *  GC cycle. This is a lower level API.
 *
 * === Examples
 *     beacon.destroy    =>  nil
 *
*/
static VALUE rb_czmq_beacon_destroy(VALUE obj)
{
    GetZmqBeacon(obj);
    rb_thread_blocking_region(rb_czmq_nogvl_beacon_destroy, beacon, RUBY_UBF_IO, 0);
    return Qnil;
}

/*
 *  call-seq:
 *     beacon.hostname   =>  String
 *
 *  Returns the beacon's IP address
 *
 * === Examples
 *     beacon.hostname   =>  "127.0.0.1"
 *
*/
static VALUE rb_czmq_beacon_hostname(VALUE obj)
{
    GetZmqBeacon(obj);
    return rb_str_new2(zbeacon_hostname(beacon->beacon));
}

/*
 * :nodoc:
 *  Set the beacon broadcast interval while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_set_interval(void *ptr)
{
    struct nogvl_beacon_interval_args *args = ptr;
    zmq_beacon_wrapper *beacon = args->beacon;
    zbeacon_set_interval(beacon->beacon, args->interval);
    return Qnil;
}

/*
 *  call-seq:
 *     beacon.interval = 100   =>  nil
 *
 *  Sets the broadcast interval in milliseconds
 *
 * === Examples
 *     beacon.interval = 100   =>  nil
 *
*/
static VALUE rb_czmq_beacon_set_interval(VALUE obj, VALUE interval)
{
    struct nogvl_beacon_interval_args args;
    GetZmqBeacon(obj);
    Check_Type(interval, T_FIXNUM);
    args.beacon = beacon;
    args.interval = FIX2INT(interval);
    rb_thread_blocking_region(rb_czmq_nogvl_set_interval, (void *)&args, RUBY_UBF_IO, 0);
    return Qnil;
}

/*
 * :nodoc:
 *  Filter beacons while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_noecho(void *ptr)
{
    zmq_beacon_wrapper *beacon = ptr;
    zbeacon_noecho(beacon->beacon);
    return Qnil;
}

/*
 *  call-seq:
 *     beacon.noecho   =>  nil
 *
 *  Filter out any beacon that looks exactly like ours
 *
 * === Examples
 *     beacon.noecho   =>  nil
 *
*/
static VALUE rb_czmq_beacon_noecho(VALUE obj)
{
    GetZmqBeacon(obj);
    rb_thread_blocking_region(rb_czmq_nogvl_noecho, (void *)beacon, RUBY_UBF_IO, 0);
    return Qnil;
}

/*
 * :nodoc:
 *  Broadcast beacon while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_publish(void *ptr)
{
    struct nogvl_beacon_publish_args *args = ptr;
    zmq_beacon_wrapper *beacon = args->beacon;
    zbeacon_publish(beacon->beacon, (byte *)args->transmit ,args->length);
    return Qnil;
}

/*
 *  call-seq:
 *     beacon.publish("address")   =>  nil
 *
 *  Start broadcasting beacon to peers.
 *
 * === Examples
 *     beacon.publish("address")   =>  nil
 *
*/
static VALUE rb_czmq_beacon_publish(VALUE obj, VALUE transmit)
{
    struct nogvl_beacon_publish_args args;
    GetZmqBeacon(obj);
    Check_Type(transmit, T_STRING);
    args.beacon = beacon;
    args.transmit = RSTRING_PTR(transmit);
    args.length = (int)RSTRING_LEN(transmit);
    rb_thread_blocking_region(rb_czmq_nogvl_publish, (void *)&args, RUBY_UBF_IO, 0);
    return Qnil;
}

/*
 * :nodoc:
 *  Silence beacon while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_silence(void *ptr)
{
    zmq_beacon_wrapper *beacon = ptr;
    zbeacon_silence(beacon->beacon);
    return Qnil;
}

/*
 *  call-seq:
 *     beacon.silence   =>  nil
 *
 *  Stop broadcasting beacons to peers.
 *
 * === Examples
 *     beacon.silence   =>  nil
 *
*/
static VALUE rb_czmq_beacon_silence(VALUE obj)
{
    GetZmqBeacon(obj);
    rb_thread_blocking_region(rb_czmq_nogvl_silence, (void *)beacon, RUBY_UBF_IO, 0);
    return Qnil;
}

/*
 * :nodoc:
 *  Start listening to peers while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_subscribe(void *ptr)
{
    struct nogvl_beacon_subscribe_args *args = ptr;
    zmq_beacon_wrapper *beacon = args->beacon;
    zbeacon_subscribe(beacon->beacon, (byte *)args->filter ,args->length);
    return Qnil;
}

/*
 *  call-seq:
 *     beacon.subscribe("channel")   =>  nil
 *
 *  Start listening to other peers.
 *
 * === Examples
 *     beacon.subscribe("channel")   =>  nil
 *
*/
static VALUE rb_czmq_beacon_subscribe(VALUE obj, VALUE filter)
{
    struct nogvl_beacon_subscribe_args args;
    GetZmqBeacon(obj);
    args.beacon = beacon;
    if (NIL_P(filter)) {
        args.filter = NULL;
        args.length = 0;
    } else {
        Check_Type(filter, T_STRING);
        args.filter = RSTRING_PTR(filter);
        args.length = (int)RSTRING_LEN(filter);
    }
    rb_thread_blocking_region(rb_czmq_nogvl_subscribe, (void *)&args, RUBY_UBF_IO, 0);
    return Qnil;
}

/*
 * :nodoc:
 *  Stop listening to peers while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_unsubscribe(void *ptr)
{
    zmq_beacon_wrapper *beacon = ptr;
    zbeacon_unsubscribe(beacon->beacon);
    return Qnil;
}

/*
 *  call-seq:
 *     beacon.unsubscribe   =>  nil
 *
 *  Stop broadcasting beacons to peers.
 *
 * === Examples
 *     beacon.unsubscribe   =>  nil
 *
*/
static VALUE rb_czmq_beacon_unsubscribe(VALUE obj)
{
    GetZmqBeacon(obj);
    rb_thread_blocking_region(rb_czmq_nogvl_unsubscribe, (void *)beacon, RUBY_UBF_IO, 0);
    return Qnil;
}

static VALUE rb_czmq_beacon_pipe(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    VALUE socket;
    GetZmqBeacon(obj);
    socket = rb_czmq_socket_alloc(Qnil, NULL, zbeacon_socket(beacon->beacon));
    GetZmqSocket(socket);
    sock->state = ZMQ_SOCKET_BOUND;
    return socket;
}

void _init_rb_czmq_beacon()
{
    rb_cZmqBeacon = rb_define_class_under(rb_mZmq, "Beacon", rb_cObject);

    rb_define_singleton_method(rb_cZmqBeacon, "new", rb_czmq_beacon_s_new, 1);
    rb_define_method(rb_cZmqBeacon, "destroy", rb_czmq_beacon_destroy, 0);
    rb_define_method(rb_cZmqBeacon, "hostname", rb_czmq_beacon_hostname, 0);
    rb_define_method(rb_cZmqBeacon, "interval=", rb_czmq_beacon_set_interval, 1);
    rb_define_method(rb_cZmqBeacon, "noecho", rb_czmq_beacon_noecho, 0);
    rb_define_method(rb_cZmqBeacon, "publish", rb_czmq_beacon_publish, 1);
    rb_define_method(rb_cZmqBeacon, "silence", rb_czmq_beacon_silence, 0);
    rb_define_method(rb_cZmqBeacon, "subscribe", rb_czmq_beacon_subscribe, 1);
    rb_define_method(rb_cZmqBeacon, "unsubscribe", rb_czmq_beacon_unsubscribe, 0);
    rb_define_method(rb_cZmqBeacon, "pipe", rb_czmq_beacon_pipe, 0);
}
