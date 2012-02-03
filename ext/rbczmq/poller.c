#include <rbczmq_ext.h>

/*
 * :nodoc:
 *  GC mark callback
 *
*/
static void rb_czmq_mark_poller(void *ptr)
{
    zmq_poll_wrapper *poller =  (zmq_poll_wrapper *)ptr;
    if (poller) {
        rb_gc_mark(poller->pollables);
        rb_gc_mark(poller->readables);
        rb_gc_mark(poller->writables);
    }
}

/*
 * :nodoc:
 *  GC free callback
 *                  
*/
static void rb_czmq_free_poller_gc(void *ptr)
{
    zmq_poll_wrapper *poller = (zmq_poll_wrapper *)ptr;
    if (poller) {
        xfree(poller->pollset);
        xfree(poller);
    }
}

/*
 * :nodoc:
 *  Rebuild the pollset from the sockets registered with this poller
 *
*/
int rb_czmq_poller_rebuild_pollset(zmq_poll_wrapper *poller)
{
    VALUE pollable;
    int rebuilt;
    xfree(poller->pollset);
    poller->pollset = NULL;
    poller->pollset = ALLOC_N(zmq_pollitem_t, poller->poll_size);
    if (!poller->pollset) return -1;
    for (rebuilt = 0; rebuilt < poller->poll_size; rebuilt++) {
        pollable = rb_ary_entry(poller->pollables, (long)rebuilt);
        ZmqGetPollitem(pollable);
        poller->pollset[rebuilt] = *pollitem->item;
    }
    poller->dirty = FALSE;
    return 0;
}

/*
 * :nodoc:
 *  Rebuild the readable and writable arrays if any spoll items are in a ready state
 *
*/
int rb_czmq_poller_rebuild_selectables(zmq_poll_wrapper *poller)
{
    VALUE pollable;
    int rebuilt;
    rb_ary_clear(poller->readables);
    rb_ary_clear(poller->writables);
    for (rebuilt = 0; rebuilt < poller->poll_size; rebuilt++) {
        zmq_pollitem_t item = poller->pollset[rebuilt];
        pollable = rb_ary_entry(poller->pollables, (long)rebuilt);
        ZmqGetPollitem(pollable);
        if (item.revents & ZMQ_POLLIN)
            rb_ary_push(poller->readables, rb_czmq_pollitem_pollable(pollable));
        if (item.revents & ZMQ_POLLOUT)
            rb_ary_push(poller->writables, rb_czmq_pollitem_pollable(pollable));
    }
    return 0;
}

/*
 *  call-seq:
 *     ZMQ::Poller.new    =>  ZMQ::Poller
 *
 *  Initializes a new ZMQ::Poller instance.
 *
 * === Examples
 *
 *     ZMQ::Poller.new    =>  ZMQ::Poller
 *
*/
VALUE rb_czmq_poller_new(VALUE obj)
{
    zmq_poll_wrapper *poller = NULL;
    obj = Data_Make_Struct(rb_cZmqPoller, zmq_poll_wrapper, rb_czmq_mark_poller, rb_czmq_free_poller_gc, poller);
    poller->pollset = NULL;
    poller->pollables = rb_ary_new();
    poller->readables = rb_ary_new();
    poller->writables = rb_ary_new();
    poller->dirty = FALSE;
    rb_obj_call_init(obj, 0, NULL);
    return obj;
}

/*
 *  call-seq:
 *     poller.poll(1)    =>  Fixnum
 *
 *  Multiplexes input/output events in a level-triggered fashion over a set of registered sockets.
 *
 * === Examples
 *
 *  Supported timeout values :
 *
 *  -1  : block until any sockets are ready (no timeout)
 *   0  : non-blocking poll
 *   1  : block for up to 1 second (1000ms)
 *  0.1 : block for up to 0.1 seconds (100ms)
 *
 *     poller = ZMQ::Poller.new             =>  ZMQ::Poller
 *     poller.register(req)                 =>  true
 *     poller.poll(1)                       =>  Fixnum
 *
*/
VALUE rb_czmq_poller_poll(int argc, VALUE *argv, VALUE obj)
{
    VALUE tmout;
    size_t timeout;
    int rc;
    ZmqGetPoller(obj);
    rb_scan_args(argc, argv, "01", &tmout);
    if (NIL_P(tmout)) tmout = INT2NUM(0);
    if (TYPE(tmout) != T_FIXNUM && TYPE(tmout) != T_FLOAT) rb_raise(rb_eTypeError, "wrong timeout type %s (expected Fixnum or Float)", RSTRING_PTR(rb_obj_as_string(tmout)));
    if (poller->poll_size == 0) return INT2NUM(0);
    if (poller->dirty == TRUE) {
        rc = rb_czmq_poller_rebuild_pollset(poller);
        if (rc == -1) rb_raise(rb_eZmqError, "failed in rebuilding the pollset!");
    }
    timeout = (size_t)(((TYPE(tmout) == T_FIXNUM) ? FIX2LONG(tmout) : RFLOAT_VALUE(tmout)) * 1000); 
    if (timeout < 0) timeout = -1;
    rc = zmq_poll(poller->pollset, poller->poll_size, (long)timeout);
    ZmqAssert(rc);
    if (rc == 0) {
        rb_ary_clear(poller->readables);
        rb_ary_clear(poller->writables);
    } else {
        rb_czmq_poller_rebuild_selectables(poller);
    }
    return INT2NUM(rc);
}

/*
 *  call-seq:
 *     poller.register(pollitem)    =>  boolean
 *
 *  Registers a poll item for a particular I/O event (ZMQ::POLLIN or ZMQ::POLLOUT) with this poller instance.
 *  ZMQ::Socket or Ruby IO instances will automatically be coerced to ZMQ::Pollitem instances with the default
 *  events mask (ZMQ::POLLIN | ZMQ::POLLOUT)
 *
 * === Examples
 *
 *  Supported events :
 *
 *  ZMQ::POLLIN   : readable state
 *  ZMQ::POLLOUT  : writable state
 *
 *     poller = ZMQ::Poller.new                              =>  ZMQ::Poller
 *     poller.register(ZMQ::Pollitem.new(req, ZMQ::POLLIN))  =>  true
 *
 *     poller.register(pub_socket)                           =>  true
 *     poller.register(STDIN)                                =>  true
 *
*/
VALUE rb_czmq_poller_register(VALUE obj, VALUE pollable)
{
    ZmqGetPoller(obj);
    pollable = rb_czmq_pollitem_coerce(pollable);
    ZmqGetPollitem(pollable);
    rb_ary_push(poller->pollables, pollable);
    poller->poll_size++;
    poller->dirty = TRUE;
    return Qtrue;
}

/*
 *  call-seq:
 *     poller.remove(pollitem)    =>  boolean
 *
 *  Removes a poll item from this poller. Deregisters the socket for *any* previously registered events.
 *  Note that we match on both poll items as well as pollable entities for all registered poll items.
 *
 * === Examples
 *
 *     poller = ZMQ::Poller.new             =>  ZMQ::Poller
 *     poller.register(req)                 =>  true
 *     poller.remove(req)                   =>  true
 *
*/
VALUE rb_czmq_poller_remove(VALUE obj, VALUE pollable)
{
    int pos;
    VALUE rpollable;
    ZmqGetPoller(obj);
    pollable = rb_czmq_pollitem_coerce(pollable);
    ZmqGetPollitem(pollable);
    for (pos = 0; pos < poller->poll_size; pos++) {
        rpollable = rb_ary_entry(poller->pollables, (long)pos);
        if (pollable == rpollable || rb_czmq_pollitem_pollable(pollable) == rb_czmq_pollitem_pollable(rpollable)) {
            rb_ary_delete(poller->pollables, rpollable);
            poller->poll_size--;
            poller->dirty = TRUE;
            return Qtrue;
        }
    }
    return Qfalse;
}

/*
 *  call-seq:
 *     poller.readables    =>  Array
 *
 *  All poll items in a readable state after the last poll.
 *
 * === Examples
 *
 *     poller = ZMQ::Poller.new                          =>  ZMQ::Poller
 *     poller.register(ZMQ::Pollitem(req, ZMQ::POLLIN))  =>  true
 *     poller.poll(1)                                    =>  1
 *     poller.readables                                  =>  [req]
 *
*/
VALUE rb_czmq_poller_readables(VALUE obj)
{
    ZmqGetPoller(obj);
    return poller->readables;
}

/*
 *  call-seq:
 *     poller.writables    =>  Array
 *
 *  All poll items in a writable state after the last poll.
 *
 * === Examples
 *
 *     poller = ZMQ::Poller.new                           =>  ZMQ::Poller
 *     poller.register(ZMQ::Pollitem(req, ZMQ::POLLOUT))  =>  true
 *     poller.poll(1)                                     =>  1
 *     poller.writables                                   =>  [req]
 *
*/
VALUE rb_czmq_poller_writables(VALUE obj)
{
    ZmqGetPoller(obj);
    return poller->writables;
}

void _init_rb_czmq_poller()
{
    rb_cZmqPoller = rb_define_class_under(rb_mZmq, "Poller", rb_cObject);

    rb_define_alloc_func(rb_cZmqPoller, rb_czmq_poller_new);
    rb_define_method(rb_cZmqPoller, "poll", rb_czmq_poller_poll, -1);
    rb_define_method(rb_cZmqPoller, "register", rb_czmq_poller_register, 1);
    rb_define_method(rb_cZmqPoller, "remove", rb_czmq_poller_remove, 1);
    rb_define_method(rb_cZmqPoller, "readables", rb_czmq_poller_readables, 0);
    rb_define_method(rb_cZmqPoller, "writables", rb_czmq_poller_writables, 0);
}
