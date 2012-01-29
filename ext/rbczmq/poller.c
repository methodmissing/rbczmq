#include <rbczmq_ext.h>

#ifdef RUBINIUS
/* Rubinius does not include this symbol table mark utility function */
static int mark_entry(ID key, VALUE value)
{
    rb_gc_mark(value);
    return ST_CONTINUE;
}

void rb_mark_tbl(st_table *tbl)
{
    if (!tbl) return;
    st_foreach(tbl, mark_entry);
}
#endif

/*
 * :nodoc:
 *  GC mark callback
 *
*/
static void rb_czmq_mark_poller(void *ptr)
{
    zmq_poll_wrapper *poller =  (zmq_poll_wrapper *)ptr;
    if (poller) {
        rb_mark_tbl(poller->socket_map);
        rb_gc_mark(poller->sockets);
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
        st_free_table(poller->socket_map);
        xfree(poller->pollset);
        xfree(poller);
    }
}

/*
 * :nodoc:
 *  Generate a pollset item for a single poll socket
 *
*/
static int rb_czmq_poller_rebuild_pollset_i(VALUE key, VALUE value, VALUE *args)
{
    zmq_poll_wrapper *poller = (zmq_poll_wrapper *)args;
    GetZmqSocket(key);
    poller->pollset[poller->rebuilt].socket = sock->socket;
    poller->pollset[poller->rebuilt].events = NUM2INT(value);
    poller->rebuilt++;
    return ST_CONTINUE;
}

/*
 * :nodoc:
 *  Rebuild the pollset from the sockets registered with this poller
 *
*/
int rb_czmq_poller_rebuild_pollset(zmq_poll_wrapper *poller)
{
    xfree(poller->pollset);
    poller->pollset = NULL;
    poller->pollset = ALLOC_N(zmq_pollitem_t, poller->poll_size);
    if (!poller->pollset) return -1;

    poller->rebuilt = 0;
    rb_hash_foreach(poller->sockets, rb_czmq_poller_rebuild_pollset_i, (st_data_t)poller);
    return 0;
}

/*
 * :nodoc:
 *  Maps a raw socket pointer back to a Ruby objects
 *
*/
VALUE rb_czmq_poller_map_socket(zmq_poll_wrapper *poller, void *sock)
{
    VALUE socket;
    st_lookup(poller->socket_map, (st_data_t)sock, &socket);
    return socket;
}

/*
 * :nodoc:
 *  Rebuild the readable and writable arrays if any sockets are in a ready state
 *
*/
int rb_czmq_poller_rebuild_selectables(zmq_poll_wrapper *poller)
{
    int rebuilt;
    rb_ary_clear(poller->readables);
    rb_ary_clear(poller->writables);
    for (rebuilt = 0; rebuilt < poller->poll_size; rebuilt++) {
        zmq_pollitem_t item = poller->pollset[rebuilt];
        if (item.revents & ZMQ_POLLIN)
            rb_ary_push(poller->readables, rb_czmq_poller_map_socket(poller, item.socket));
        if (item.revents & ZMQ_POLLOUT)
            rb_ary_push(poller->writables, rb_czmq_poller_map_socket(poller, item.socket));
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
    poller->sockets = rb_hash_new();
    poller->readables = rb_ary_new();
    poller->writables = rb_ary_new();
    poller->socket_map = st_init_numtable();
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
 *     poller.register(req, ZMQ::POLLIN)    =>  true
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
    if (poller->dirty == TRUE) rb_czmq_poller_rebuild_pollset(poller);
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
 *     poller.register(sock, ZMQ::POLLIN)    =>  boolean
 *
 *  Registers a socket for a particular I/O event (ZMQ::POLLIN or ZMQ::POLLOUT).
 *
 * === Examples
 *
 *  Supported events :
 *
 *  ZMQ::POLLIN   : readable state
 *  ZMQ::POLLOUT  : writable state
 *
 *     poller = ZMQ::Poller.new             =>  ZMQ::Poller
 *     poller.register(req, ZMQ::POLLIN)    =>  true
 *
*/
VALUE rb_czmq_poller_register(int argc, VALUE *argv, VALUE obj)
{
    VALUE socket, events, revents;
    ZmqGetPoller(obj);
    rb_scan_args(argc, argv, "11", &socket, &events);
    GetZmqSocket(socket);
    ZmqAssertSocketNotPending(sock, "socket in a pending state (not bound or connected) and thus cannot be registered with a poller!");
    ZmqSockGuardCrossThread(sock);
    if (NIL_P(events)) {
        events = INT2NUM(ZMQ_POLLIN | ZMQ_POLLOUT);
    } else {
        Check_Type(events, T_FIXNUM);
    }
    if (events == INT2NUM(0)) return Qfalse;
    revents = rb_hash_aref(poller->sockets, socket);
    if (NIL_P(revents)) {
        rb_hash_aset(poller->sockets, socket, events);
    } else {
        rb_hash_aset(poller->sockets, socket, INT2NUM(NUM2INT(revents) | NUM2INT(events)));
    }
    st_insert(poller->socket_map, (st_data_t)sock->socket, (st_data_t)socket);
    poller->poll_size++;
    poller->dirty = TRUE;
    return Qtrue;
}

/*
 *  call-seq:
 *     poller.remove(sock)    =>  boolean
 *
 *  Removes a socket from this poller. Deregisters the socket for *any* previously registered events.
 *
 * === Examples
 *
 *     poller = ZMQ::Poller.new             =>  ZMQ::Poller
 *     poller.register(req, ZMQ::POLLIN)    =>  true
 *     poller.remove(req)                   =>  true
 *
*/
VALUE rb_czmq_poller_remove(VALUE obj, VALUE socket)
{
    VALUE ret;
    ZmqGetPoller(obj);
    GetZmqSocket(socket);
    ZmqSockGuardCrossThread(sock);
    ret = rb_hash_delete(poller->sockets, socket);
    if (!NIL_P(ret)) {
        poller->poll_size--;
        poller->dirty = TRUE;
        st_delete(poller->socket_map, (st_data_t*)&sock->socket, 0);
        return Qtrue;
    }
    return Qfalse;
}

/*
 *  call-seq:
 *     poller.readables    =>  Array
 *
 *  All sockets in a readable state after the last poll.
 *
 * === Examples
 *
 *     poller = ZMQ::Poller.new             =>  ZMQ::Poller
 *     poller.register(req, ZMQ::POLLIN)    =>  true
 *     poller.poll(1)                       =>  1
 *     poller.readables                     =>  [req]
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
 *  All sockets in a writable state after the last poll.
 *
 * === Examples
 *
 *     poller = ZMQ::Poller.new             =>  ZMQ::Poller
 *     poller.register(req, ZMQ::POLLOUT)   =>  true
 *     poller.poll(1)                       =>  1
 *     poller.writables                     =>  [req]
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
    rb_define_method(rb_cZmqPoller, "register", rb_czmq_poller_register, -1);
    rb_define_method(rb_cZmqPoller, "remove", rb_czmq_poller_remove, 1);
    rb_define_method(rb_cZmqPoller, "readables", rb_czmq_poller_readables, 0);
    rb_define_method(rb_cZmqPoller, "writables", rb_czmq_poller_writables, 0);
}