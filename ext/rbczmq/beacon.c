#include "rbczmq_ext.h"

static void rb_czmq_free_beacon_gc(void *ptr)
{
    zmq_beacon_wrapper *beacon = (zmq_beacon_wrapper *)ptr;
    if (beacon) {
        xfree(beacon);
    }
}

static VALUE rb_czmq_beacon_s_new(VALUE beacon, VALUE port)
{
    zmq_beacon_wrapper *bcn = NULL;
    Check_Type(port, T_FIXNUM);
    beacon = Data_Make_Struct(rb_cZmqBeacon, zmq_beacon_wrapper, 0, rb_czmq_free_beacon_gc, bcn);
    bcn->beacon = (zbeacon_t*)zbeacon_new(FIX2INT(port));
    ZmqAssertObjOnAlloc(bcn->beacon, bcn);
    rb_obj_call_init(beacon, 0, NULL);
    return beacon;
}

static VALUE rb_czmq_beacon_destroy(VALUE obj)
{
    GetZmqBeacon(obj);
    zbeacon_destroy(&beacon->beacon);
    return Qnil;
}

static VALUE rb_czmq_beacon_hostname(VALUE obj)
{
    GetZmqBeacon(obj);
    return rb_str_new2(zbeacon_hostname(beacon->beacon));
}

static VALUE rb_czmq_beacon_set_interval(VALUE obj, VALUE interval)
{
    Check_Type(interval, T_FIXNUM);
    GetZmqBeacon(obj);
    zbeacon_set_interval(beacon->beacon, FIX2INT(interval));
    return Qnil;
}

static VALUE rb_czmq_beacon_noecho(VALUE obj)
{
    GetZmqBeacon(obj);
    zbeacon_noecho(beacon->beacon);
    return Qnil;
}

static VALUE rb_czmq_beacon_publish(VALUE obj, VALUE transmit)
{
    GetZmqBeacon(obj);
    Check_Type(transmit, T_STRING);
    zbeacon_publish(beacon->beacon, (byte *)StringValueCStr(transmit), RSTRING_LEN(transmit));
    return Qnil;
}

static VALUE rb_czmq_beacon_silence(VALUE obj)
{
    GetZmqBeacon(obj);
    zbeacon_silence(beacon->beacon);
    return Qnil;
}

static VALUE rb_czmq_beacon_subscribe(VALUE obj, VALUE filter)
{
    GetZmqBeacon(obj);
    Check_Type(filter, T_STRING);
    zbeacon_publish(beacon->beacon, (byte *)StringValueCStr(filter), RSTRING_LEN(filter));
    return Qnil;
}

static VALUE rb_czmq_beacon_unsubscribe(VALUE obj)
{
    GetZmqBeacon(obj);
    zbeacon_unsubscribe(beacon->beacon);
    return Qnil;
}

static VALUE rb_czmq_beacon_pipe(VALUE obj)
{
    void *socket;
    GetZmqBeacon(obj);
    socket = zbeacon_pipe(beacon->beacon);
    // TODO: Return ZMQ::Socket instance + plug in with GC
    return Qnil;
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