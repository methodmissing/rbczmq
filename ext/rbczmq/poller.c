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
        /* XXX: mark VALUEs in poller->socket_map as well ?*/
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
        xfree(poller);
    }
}

int rb_czmq_poller_rebuild(zmq_poll_wrapper *poller)
{
    VALUE socket;
    int rebuilt;
    xfree(poller->pollset);
    poller->pollset = NULL;
    poller->pollset = ALLOC_N(zmq_pollitem_t, poller->poll_size);
    if (!poller->pollset) return -1;

    rebuilt = 0;
    socket = rb_ary_entry(poller->sockets, 0);
    if (NIL_P(socket)) return 0;
    while (rebuilt != poller->poll_size) {
        GetZmqSocket(socket);
        poller->pollset[rebuilt].socket = sock->socket;
        poller->pollset[rebuilt].events = sock->poll_events;
        rebuilt++;
        socket = rb_ary_entry(poller->sockets, rebuilt);
    }
    return 0;
}

VALUE rb_czmq_poller_map_socket(zmq_poll_wrapper *poller, void *sock)
{
    VALUE socket;
    st_lookup(poller->socket_map, (st_data_t)sock, &socket);
    return socket;
}
int rb_czmq_poller_rebuild_pollset(zmq_poll_wrapper *poller)
{
    VALUE socket;
    int rebuilt;
    xfree(poller->pollset);
    poller->pollset = NULL;
    poller->pollset = ALLOC_N(zmq_pollitem_t, poller->poll_size);
    if (!poller->pollset) return -1;

    rebuilt = 0;
    socket = rb_ary_entry(poller->sockets, 0);
    if (NIL_P(socket)) return 0;
    for (rebuilt = 0; rebuilt <= poller->poll_size; rebuilt++) {
        socket = rb_ary_entry(poller->sockets, 0);
        GetZmqSocket(socket);
        poller->pollset[rebuilt].socket = sock->socket;
        poller->pollset[rebuilt].events = sock->poll_events;
    }
    return 0;
}

int rb_czmq_poller_rebuild_selectables(zmq_poll_wrapper *poller)
{
    int rebuilt;
    rb_ary_clear(poller->readables);
    rb_ary_clear(poller->writables);
    for (rebuilt = 0; rebuilt <= poller->poll_size; rebuilt++) {
        zmq_pollitem_t item = poller->pollset[rebuilt];
        if (item.revents & ZMQ_POLLIN)
            rb_ary_push(poller->readables, rb_czmq_poller_map_socket(poller, item.socket));
        if (item.revents & ZMQ_POLLOUT)
            rb_ary_push(poller->writables, rb_czmq_poller_map_socket(poller, item.socket));
    }
    return 0;
}

VALUE rb_czmq_poller_new(VALUE obj)
{
    zmq_poll_wrapper *poller = NULL;
    obj = Data_Make_Struct(rb_cZmqPoller, zmq_poll_wrapper, rb_czmq_mark_poller, rb_czmq_free_poller_gc, poller);
    poller->pollset = NULL;
    poller->sockets = rb_ary_new();
    poller->readables = rb_ary_new();
    poller->writables = rb_ary_new();
    poller->socket_map = st_init_numtable();
    poller->dirty = FALSE;
    rb_obj_call_init(obj, 0, NULL);
    return obj;
}

VALUE rb_czmq_poller_poll(int argc, VALUE *argv, VALUE obj)
{
    VALUE tmout;
    size_t timeout;
    int rc;
    ZmqGetPoller(obj);
    rb_scan_args(argc, argv, "01", &tmout);
    if (NIL_P(tmout)) tmout = INT2NUM(0);
    if (TYPE(tmout) != T_FIXNUM && TYPE(tmout) != T_FLOAT) rb_raise(rb_eTypeError, "wrong timeout type %s (expected Fixnum or Float)", RSTRING_PTR(rb_obj_as_string(tmout)));
    if (poller->dirty == TRUE) rb_czmq_poller_rebuild_pollset(poller);
    if (poller->poll_size == 0) return INT2NUM(0);
    timeout = (size_t)(((TYPE(tmout) == T_FIXNUM) ? FIX2LONG(tmout) : RFLOAT_VALUE(tmout)) * 1000); 
    rc = zmq_poll(poller->pollset, poller->poll_size, timeout);
    ZmqAssert(rc);
    if (rc != 0) rb_czmq_poller_rebuild_selectables(poller);
    return INT2NUM(rc);
}

VALUE rb_czmq_poller_register(int argc, VALUE *argv, VALUE obj)
{
    VALUE socket, evts;
    int events;
    ZmqGetPoller(obj);
    rb_scan_args(argc, argv, "11", &socket, &evts);
    GetZmqSocket(socket);
    sock->poll_events = 0;
    if (NIL_P(evts)) {
        events = ZMQ_POLLIN | ZMQ_POLLOUT;
    } else {
        Check_Type(evts, T_FIXNUM);
        events = NUM2INT(evts);
    }
    if (events == 0) return Qfalse;
    sock->poll_events = events;
    rb_ary_push(poller->sockets, socket);
    st_insert(poller->socket_map, (st_data_t)sock->socket, (st_data_t)socket);
    poller->poll_size++;
    poller->dirty = TRUE;
    return Qtrue;
}

VALUE rb_czmq_poller_remove(VALUE obj, VALUE socket)
{
    VALUE ret;
    ZmqGetPoller(obj);
    GetZmqSocket(socket);
    ret = rb_ary_delete(poller->sockets, socket);
    if (!NIL_P(ret)) {
        poller->poll_size--;
        poller->dirty = TRUE;
        st_delete(poller->socket_map, (st_data_t*)&sock->socket, 0);
        return Qtrue;
    }
    return Qfalse;
}

VALUE rb_czmq_poller_readables(VALUE obj)
{
    ZmqGetPoller(obj);
    return poller->readables;
}

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