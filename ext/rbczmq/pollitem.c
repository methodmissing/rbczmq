#include <rbczmq_ext.h>

/*
 * :nodoc:
 *  GC mark callback
 *
*/
void rb_czmq_mark_pollitem(void *ptr)
{
    zmq_pollitem_wrapper *pollitem = (zmq_pollitem_wrapper *)ptr;
    if (ptr) {
        rb_gc_mark(pollitem->socket);
        rb_gc_mark(pollitem->io);
        rb_gc_mark(pollitem->events);
    }
}

/*
 * :nodoc:
 *  GC free callback
 *
*/
void rb_czmq_free_pollitem_gc(void *ptr)
{
    zmq_pollitem_wrapper *pollitem = (zmq_pollitem_wrapper *)ptr;
    if (ptr) {
        xfree(pollitem->item);
        xfree(pollitem);
    }
}

static VALUE rb_czmq_pollitem_s_new(int argc, VALUE *argv, VALUE obj)
{
    VALUE pollable, events;
    int evts;
    zmq_pollitem_wrapper *pollitem = NULL;
    rb_scan_args(argc, argv, "11", &pollable, &events);
    /* XXX: default to ZMQ_POLLIN ? */
    if (NIL_P(events)) events = INT2NUM((ZMQ_POLLIN | ZMQ_POLLOUT));
    Check_Type(events, T_FIXNUM);
    evts = NUM2INT(events);
    if (evts != ZMQ_POLLIN && evts != ZMQ_POLLOUT)
        rb_raise(rb_eZmqError, "invalid socket event: Only ZMQ::POLLIN and ZMQ::POLLOUT events are supported!");

    /* XXX: Cleanup allocated struct on any failures below */
    obj = Data_Make_Struct(rb_cZmqPollitem, zmq_pollitem_wrapper, rb_czmq_mark_pollitem, rb_czmq_free_pollitem_gc, pollitem);
    pollitem->events = events;
    /* XXX: Assert allocation of zmq_pollitem struct */
    pollitem->item = ALLOC(zmq_pollitem_t);
    pollitem->item->events = evts;
    if (rb_obj_is_kind_of(pollable, rb_cZmqSocket)) {
       GetZmqSocket(pollable);
       ZmqAssertSocketNotPending(sock, "socket in a pending state (not bound or connected) and thus cannot be registered as a poll item!");
       pollitem->socket = pollable;
       pollitem->io = Qnil;
       pollitem->item->socket = sock->socket;
    } else if (rb_obj_is_kind_of(pollable, rb_cIO)) {
       pollitem->io = pollable;
       pollitem->socket = Qnil;
       pollitem->item->fd = rb_funcall(pollable, rb_intern("fileno"), 0);
   /* XXX: handle coercion of other I/O like objects as well ? respond_to?(:fileno) ? */
    } else {
       rb_raise(rb_eTypeError, "wrong pollable type %s (%s). Only objects of type ZMQ::Socket and IO supported.", rb_obj_classname(pollable), RSTRING_PTR(rb_obj_as_string(pollable)));
    }
    rb_obj_call_init(obj, 0, NULL);
    return obj;
}

VALUE rb_czmq_pollitem_pollable(VALUE obj)
{
    ZmqGetPollitem(obj);
    if (NIL_P(pollitem->socket)) return pollitem->io;
    return pollitem->socket;
}

void _init_rb_czmq_pollitem()
{
    rb_cZmqPollitem = rb_define_class_under(rb_mZmq, "Pollitem", rb_cObject);

    rb_define_singleton_method(rb_cZmqPollitem, "new", rb_czmq_pollitem_s_new, -1);
    rb_define_method(rb_cZmqPollitem, "pollable", rb_czmq_pollitem_pollable, 0);
}