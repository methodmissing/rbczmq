#include "rbczmq_ext.h"

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
        rb_gc_mark(pollitem->handler);
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

/*
 *  call-seq:
 *     ZMQ::Pollitem.new(io, ZMQ:POLLIN)    =>  ZMQ::Pollitem
 *
 *  A generic poll item that supports Ruby I/O objects as well as native ZMQ sockets. Poll items are primarily used
 *  for registering pollable entities with ZMQ::Poller and ZMQ::Loop instances. If no events given, we default to
 *  observing both readable and writable state.
 *
 * === Examples
 *
 *  Supported pollable types :
 *
 *  IO           : any Ruby I/O object (must respond to #fileno as well)
 *  ZMQ::Socket  : native ZMQ socket
 *
 *  Supported events :
 *
 *  ZMQ::POLLIN  : register for readable events
 *  ZMQ::POLLOUT : register for writable events
 *
 *     ZMQ::Pollitem.new(io)                    =>  ZMQ::Pollitem
 *     ZMQ::Pollitem.new(io, ZMQ:POLLIN)        =>  ZMQ::Pollitem
 *     ZMQ::Pollitem.new(socket, ZMQ:POLLOUT)   =>  ZMQ::Pollitem
 *
*/
static VALUE rb_czmq_pollitem_s_new(int argc, VALUE *argv, VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    VALUE pollable, events;
    int evts;
    zmq_pollitem_wrapper *pollitem = NULL;
    rb_scan_args(argc, argv, "11", &pollable, &events);
    if (NIL_P(events)) events = INT2NUM((ZMQ_POLLIN | ZMQ_POLLOUT));
    Check_Type(events, T_FIXNUM);
    evts = NUM2INT(events);
    if (!(evts & ZMQ_POLLIN) && !(evts & ZMQ_POLLOUT))
        rb_raise(rb_eZmqError, "invalid socket event: Only ZMQ::POLLIN and ZMQ::POLLOUT events are supported!");

    obj = Data_Make_Struct(rb_cZmqPollitem, zmq_pollitem_wrapper, rb_czmq_mark_pollitem, rb_czmq_free_pollitem_gc, pollitem);
    pollitem->events = events;
    pollitem->handler = Qnil;
    pollitem->item = ALLOC(zmq_pollitem_t);
    ZmqAssertObjOnAlloc(pollitem->item, pollitem);
    pollitem->item->events = evts;
    if (rb_obj_is_kind_of(pollable, rb_cZmqSocket)) {
       GetZmqSocket(pollable);
       ZmqAssertSocketNotPending(sock, "socket in a pending state (not bound or connected) and thus cannot be registered as a poll item!");
       ZmqSockGuardCrossThread(sock);
       pollitem->socket = pollable;
       pollitem->io = Qnil;
       pollitem->item->fd = 0;
       pollitem->item->socket = sock->socket;
       /* Do not block on socket close */
       // zsockopt_set_linger(sock->socket, 1);
    } else if (rb_obj_is_kind_of(pollable, rb_cIO)) {
       pollitem->io = pollable;
       pollitem->socket = Qnil;
       pollitem->item->socket = NULL;
       pollitem->item->fd = NUM2INT(rb_funcall(pollable, rb_intern("fileno"), 0));
   /* XXX: handle coercion of other I/O like objects as well ? respond_to?(:fileno) ? */
    } else {
       rb_raise(rb_eTypeError, "wrong pollable type %s (%s). Only objects of type ZMQ::Socket and IO supported.", rb_obj_classname(pollable), RSTRING_PTR(rb_obj_as_string(pollable)));
    }
    rb_obj_call_init(obj, 0, NULL);
    return obj;
}

/*
 *  call-seq:
 *     ZMQ::Pollitem.coerce(sock) =>  ZMQ::Pollitem
 *
 *  Attempts to coerce a ZMQ::Socket or IO to a ZMQ::Pollitem instance
 *
 * === Examples
 *
 *     ZMQ::Pollitem.coerce(sock)   =>  ZMQ::Pollitem
 *     ZMQ::Pollitem.coerce(STDOUT) =>  IO
 *
*/

VALUE rb_czmq_pollitem_s_coerce(ZMQ_UNUSED VALUE obj, VALUE pollable)
{
    VALUE pollitem;
    VALUE args[1];
    if (rb_obj_is_kind_of(pollable, rb_cZmqPollitem)) return pollable;
    args[0] = pollable;
    pollitem = Qnil;
    return rb_czmq_pollitem_s_new(1, args, pollitem);
}

/*
 * :nodoc:
 *  Attempt to coerce an object to a ZMQ::Pollitem instance
 *
*/
VALUE rb_czmq_pollitem_coerce(VALUE pollable)
{
    return rb_czmq_pollitem_s_coerce(Qnil, pollable);
}

/*
 *  call-seq:
 *     pollitem.pollable   =>  IO or ZMQ::Socket
 *
 *  Returns the pollable entity for this poll item.
 *
 * === Examples
 *
 *     item = ZMQ::Pollitem.new(STDOUT)         =>  ZMQ::Pollitem
 *     item.pollable                            =>  STDOUT
 *
 *     item = ZMQ::Pollitem.new(pub_sock)       =>  ZMQ::Pollitem
 *     item.pollable                            =>  ZMQ::Socket
 *
*/

VALUE rb_czmq_pollitem_pollable(VALUE obj)
{
    ZmqGetPollitem(obj);
    if (NIL_P(pollitem->socket)) return pollitem->io;
    return pollitem->socket;
}

/*
 *  call-seq:
 *     pollitem.events   =>  Fixnum
 *
 *  Returns the I/O events the pollable entity is interested in.
 *
 * === Examples
 *
 *     item = ZMQ::Pollitem.new(sock, ZMQ::POLLIN)   =>  ZMQ::Pollitem
 *     item.events                                   =>  ZMQ::POLLIN
 *
*/

VALUE rb_czmq_pollitem_events(VALUE obj)
{
    ZmqGetPollitem(obj);
    return pollitem->events;
}

/*
 *  call-seq:
 *     pollitem.handler =>  Object or nil
 *
 *  Returns the callback handler currently associated with this poll item.
 *
 * === Examples
 *     item = ZMQ::Pollitem.new(sock)
 *     item.handler = MyFrameHandler   => nil
 *     item.handler                    => MyFrameHandler
 *
*/

VALUE rb_czmq_pollitem_handler(VALUE obj)
{
    ZmqGetPollitem(obj);
    return pollitem->handler;
}

/*
 *  call-seq:
 *     pollitem.handler = MyFrameHandler =>  nil
 *
 *  Associates a callback handler with this poll item.
 *
 * === Examples
 *     item = ZMQ::Pollitem.new(sock)
 *     item.handler = MyFrameHandler => nil
 *
*/

VALUE rb_czmq_pollitem_handler_equals(VALUE obj, VALUE handler)
{
    ZmqGetPollitem(obj);
    ZmqAssertHandler(obj, pollitem, handler, intern_error);
    ZmqAssertHandler(obj, pollitem, handler, intern_readable);
    ZmqAssertHandler(obj, pollitem, handler, intern_writable);
    pollitem->handler = handler;
    return Qnil;
}

/*
 *  call-seq:
 *     pollitem.verbose = true    =>  nil
 *
 *  Logs pollitem activity to stdout - useful for debugging, but can be quite noisy with lots of activity. Only applicable
 *  to pollable items of type ZMQ::Socket.
 *
 * === Examples
 *     item = ZMQ::Pollitem.new(sock)    =>   ZMQ::Pollitem
 *     item.verbose = true   =>    nil
 *
*/

VALUE rb_czmq_pollitem_set_verbose(VALUE obj, VALUE level)
{
    bool vlevel;
    zmq_sock_wrapper *sock = NULL;
    ZmqGetPollitem(obj);
    vlevel = (level == Qtrue) ? true : false;
    if (rb_obj_is_kind_of(pollitem->socket, rb_cZmqSocket)) {
        GetZmqSocket(pollitem->socket);
        sock->verbose = vlevel;
    }
    return Qnil;
}

void _init_rb_czmq_pollitem()
{
    rb_cZmqPollitem = rb_define_class_under(rb_mZmq, "Pollitem", rb_cObject);

    rb_define_singleton_method(rb_cZmqPollitem, "new", rb_czmq_pollitem_s_new, -1);
    rb_define_singleton_method(rb_cZmqPollitem, "coerce", rb_czmq_pollitem_s_coerce, 1);
    rb_define_method(rb_cZmqPollitem, "pollable", rb_czmq_pollitem_pollable, 0);
    rb_define_method(rb_cZmqPollitem, "events", rb_czmq_pollitem_events, 0);
    rb_define_method(rb_cZmqPollitem, "handler", rb_czmq_pollitem_handler, 0);
    rb_define_method(rb_cZmqPollitem, "handler=", rb_czmq_pollitem_handler_equals, 1);
    rb_define_method(rb_cZmqPollitem, "verbose=", rb_czmq_pollitem_set_verbose, 1);
}
