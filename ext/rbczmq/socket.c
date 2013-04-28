#include "rbczmq_ext.h"

VALUE intern_on_connected;
VALUE intern_on_connect_delayed;
VALUE intern_on_connect_retried;
VALUE intern_on_listening;
VALUE intern_on_bind_failed;
VALUE intern_on_accepted;
VALUE intern_on_accept_failed;
VALUE intern_on_closed;
VALUE intern_on_close_failed;
VALUE intern_on_disconnected;

/*
 * :nodoc:
 *  Destroy the socket while the GIL is released - may block depending on socket linger value.
 *
*/
static VALUE rb_czmq_nogvl_zsocket_destroy(void *ptr)
{
    zmq_sock_wrapper *sock = ptr;
    errno = 0;
    zsocket_destroy(sock->ctx, sock->socket);
    return Qnil;
}

/*
 * :nodoc:
 *  Free all resources for a socket - invoked by the lower level ZMQ::Socket#destroy as well as the GC callback (some
 *  regressions here still though).
 *
*/
void rb_czmq_free_sock(zmq_sock_wrapper *sock)
{
    if (sock->ctx) {
        rb_thread_blocking_region(rb_czmq_nogvl_zsocket_destroy, sock, RUBY_UBF_IO, 0);
        if (zmq_errno() == ENOTSOCK) ZmqRaiseSysError();
        sock->socket = NULL;
        sock->flags |= ZMQ_SOCKET_DESTROYED;
    }
}

/*
 * :nodoc:
 *  GC mark callback
 *
*/
void rb_czmq_mark_sock(void *ptr)
{
    zmq_sock_wrapper *sock = (zmq_sock_wrapper *)ptr;
    if (sock){
        if (sock->verbose)
            zclock_log ("I: %s socket %p, context %p: GC mark", zsocket_type_str(sock->socket), sock, sock->ctx);
        rb_gc_mark(sock->endpoints);
        rb_gc_mark(sock->thread);
        rb_gc_mark(sock->context);
        rb_gc_mark(sock->monitor_endpoint);
        rb_gc_mark(sock->monitor_handler);
        rb_gc_mark(sock->monitor_thread);
    }
}

/*
 * :nodoc:
 *  GC free callback
 *
*/
void rb_czmq_free_sock_gc(void *ptr)
{
    zmq_sock_wrapper *sock = (zmq_sock_wrapper *)ptr;
    if (sock){
        if (sock->verbose)
            zclock_log ("I: %s socket %p, context %p: GC free", zsocket_type_str(sock->socket), sock, sock->ctx);
/*
        XXX: cyclic dependency
        #4  0x0000000100712524 in zsocket_set_linger (linger=1, socket=<value temporarily unavailable, due to optimizations>) at zsocket.c:288
        if (sock->socket != NULL && !(sock->flags & ZMQ_SOCKET_DESTROYED)) rb_czmq_free_sock(sock);
*/
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
        zlist_destroy(&(sock->str_buffer));
        zlist_destroy(&(sock->frame_buffer));
        zlist_destroy(&(sock->msg_buffer));
#endif
        xfree(sock);
    }
}

/*
 *  call-seq:
 *     sock.close   =>  nil
 *
 *  Closes a socket. The GC will take the same action if a socket object is not reachable anymore on the next GC cycle.
 *  This is a lower level API.
 *
 * === Examples
 *     sock.close    =>  nil
 *
*/

static VALUE rb_czmq_socket_close(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    /* This is useless for production / real use cases as we can't query the state again OR assume
    anything about the underlying connection. Merely doing the right thing. */
    sock->state = ZMQ_SOCKET_PENDING;
    rb_czmq_free_sock(sock);
    return Qnil;
}

/*
 *  call-seq:
 *     sock.endpoints   =>  Array of Strings
 *
 *  Returns the endpoints this socket is currently connected to, if any.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:PUSH)
 *     sock.endpoints    =>   []
 *     sock.bind("inproc://test")
 *     sock.endpoints    =>  ["inproc://test"]
 *
*/

static VALUE rb_czmq_socket_endpoints(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return sock->endpoints;
}

/*
 *  call-seq:
 *     sock.state   =>  String
 *
 *  Returns the current socket state, one of ZMQ::Socket::PENDING, ZMQ::Socket::BOUND or ZMQ::Socket::CONNECTED
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:PUSH)
 *     sock.state       =>   ZMQ::Socket::PENDING
 *     sock.bind("inproc://test")
 *     sock.state       =>   ZMQ::Socket::BOUND
 *
*/

static VALUE rb_czmq_socket_state(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(sock->state);
}

/*
 *  call-seq:
 *     sock.fd   =>  Fixnum
 *
 *  Returns a file descriptor reference for integrating this socket with an externel event loop or multiplexor.
 *  Edge-triggered notification of I/O state changes.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:PUSH)
 *     sock.fd       =>   -1
 *     sock.bind("inproc://test")
 *     sock.fd       =>   4
 *
*/

static VALUE rb_czmq_socket_fd(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    if (sock->state == ZMQ_SOCKET_PENDING) return INT2NUM(-1);
    return INT2NUM(zsocket_fd(sock->socket));
}

/*
 * :nodoc:
 *  Binds to an endpoint while the GIL is released.
 *
*/
VALUE rb_czmq_nogvl_socket_bind(void *ptr)
{
    int rc;
    struct nogvl_conn_args *args = ptr;
    errno = 0;
    zmq_sock_wrapper *socket = args->socket;
    rc = zsocket_bind(socket->socket, args->endpoint);
    return (VALUE)rc;
}

/*
 * :nodoc:
 *  Connects to an endpoint while the GIL is released.
 *
*/
VALUE rb_czmq_nogvl_socket_connect(void *ptr)
{
    int rc;
    struct nogvl_conn_args *args = ptr;
    errno = 0;
    zmq_sock_wrapper *socket = args->socket;
    rc = zsocket_connect(socket->socket, args->endpoint);
    return (VALUE)rc;
}

/*
 *  call-seq:
 *     sock.bind("inproc://test")   =>  Fixnum
 *
 *  Binds to a given endpoint. When the port number is '*', attempts to bind to a free port. Always returns the port
 *  number on success.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:PUSH)
 *     sock.bind("tcp://localhost:*")    =>  5432
 *
*/

static VALUE rb_czmq_socket_bind(VALUE obj, VALUE endpoint)
{
    struct nogvl_conn_args args;
    int rc;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    Check_Type(endpoint, T_STRING);
    args.socket = sock;
    args.endpoint = StringValueCStr(endpoint);
    rc = (int)rb_thread_blocking_region(rb_czmq_nogvl_socket_bind, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    if (sock->verbose)
        zclock_log ("I: %s socket %p: bound \"%s\"", zsocket_type_str(sock->socket), obj, StringValueCStr(endpoint));
    sock->state = ZMQ_SOCKET_BOUND;
    rb_ary_push(sock->endpoints, rb_str_new4(endpoint));
    return INT2NUM(rc);
}

/*
 *  call-seq:
 *     sock.connect("tcp://localhost:3456")   =>  boolean
 *
 *  Attempts to connect to a given endpoint.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     rep = ctx.socket(:REP)
 *     port = rep.bind("tcp://localhost:*")    =>  5432
 *     req = ctx.socket(:REQ)
 *     req.connect("tcp://localhost:#{port}")   => true
 *
*/

static VALUE rb_czmq_socket_connect(VALUE obj, VALUE endpoint)
{
    struct nogvl_conn_args args;
    int rc;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    Check_Type(endpoint, T_STRING);
    args.socket = sock;
    args.endpoint = StringValueCStr(endpoint);
    rc = (int)rb_thread_blocking_region(rb_czmq_nogvl_socket_connect, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    if (sock->verbose)
        zclock_log ("I: %s socket %p: connected \"%s\"", zsocket_type_str(sock->socket), obj, StringValueCStr(endpoint));
    sock->state = ZMQ_SOCKET_CONNECTED;
    rb_ary_push(sock->endpoints, rb_str_new4(endpoint));
    return Qtrue;
}

/*
 *  call-seq:
 *     sock.verbose = true   =>  nil
 *
 *  Let this socket be verbose - dumps a lot of data to stdout for debugging.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.verbose = true    =>  nil
 *
*/

static VALUE rb_czmq_socket_set_verbose(VALUE obj, VALUE level)
{
    bool vlevel;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    vlevel = (level == Qtrue) ? true : false;
    sock->verbose = vlevel;
    return Qnil;
}

/*
 * :nodoc:
 *  Sends a raw string while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_zstr_send(void *ptr)
{
    struct nogvl_send_args *args = ptr;
    errno = 0;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    return (VALUE)zstr_send(socket->socket, args->msg);
#else
    if (rb_thread_alone()) return (VALUE)zstr_send(socket->socket, args->msg);
try_writable:
    if ((zsocket_events(socket->socket) & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
        return (VALUE)zstr_send(socket->socket, args->msg);
    } else {
        rb_thread_wait_fd(zsocket_fd(socket->socket));
        if (zsocket_sndtimeo(socket->socket) != -1)
            return (VALUE)zstr_send(socket->socket, args->msg);
        goto try_writable;
    }
#endif
}

/*
 * :nodoc:
 *  Sends a raw string with the multi flag set while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_zstr_sendm(void *ptr)
{
    struct nogvl_send_args *args = ptr;
    errno = 0;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    return (VALUE)zstr_sendm(socket->socket, args->msg);
#else
    if (rb_thread_alone()) return (VALUE)zstr_sendm(socket->socket, args->msg);
try_writable:
    if ((zsocket_events(socket->socket) & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
        return (VALUE)zstr_sendm(socket->socket, args->msg);
    } else {
        if (zsocket_sndtimeo(socket->socket) != -1)
            return (VALUE)zstr_sendm(socket->socket, args->msg);
        rb_thread_wait_fd(zsocket_fd(socket->socket));
        goto try_writable;
    }
#endif
}

/*
 *  call-seq:
 *     sock.send("message")  =>  boolean
 *
 *  Sends a string to this ZMQ socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REQ)
 *     sock.connect("inproc://test")
 *     sock.send("message")    =>  true
 *
*/

static VALUE rb_czmq_socket_send(VALUE obj, VALUE msg)
{
    int rc;
    struct nogvl_send_args args;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqAssertSocketNotPending(sock, "can only send on a bound or connected socket!");
    ZmqSockGuardCrossThread(sock);
    Check_Type(msg, T_STRING);
    args.socket = sock;
    args.msg = StringValueCStr(msg);
    rc = (int)rb_thread_blocking_region(rb_czmq_nogvl_zstr_send, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    if (sock->verbose)
        zclock_log ("I: %s socket %p: send \"%s\"", zsocket_type_str(sock->socket), obj, StringValueCStr(msg));
    return Qtrue;
}

/*
 *  call-seq:
 *     sock.sendm("message")  =>  boolean
 *
 *  Sends a string to this ZMQ socket, with a more flag set.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REQ)
 *     sock.connect("inproc://test")
 *     sock.sendm("mes")    =>  true
 *     sock.sendm("sage")    =>  true
 *
*/

static VALUE rb_czmq_socket_sendm(VALUE obj, VALUE msg)
{
    int rc;
    struct nogvl_send_args args;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqAssertSocketNotPending(sock, "can only send on a bound or connected socket!");
    ZmqSockGuardCrossThread(sock);
    Check_Type(msg, T_STRING);
    args.socket = sock;
    args.msg = StringValueCStr(msg);
    rc = (int)rb_thread_blocking_region(rb_czmq_nogvl_zstr_sendm, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    if (sock->verbose)
        zclock_log ("I: %s socket %p: sendm \"%s\"", zsocket_type_str(sock->socket), sock->socket, StringValueCStr(msg));
    return Qtrue;
}

/*
 * :nodoc:
 *  Receives a raw string while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_recv(void *ptr)
{
    struct nogvl_recv_args *args = ptr;
    errno = 0;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    return (VALUE)zstr_recv(socket->socket);
#else
    if (zlist_size(socket->str_buffer) != 0)
       return (VALUE)zlist_pop(socket->str_buffer);
try_readable:
    if ((zsocket_events(socket->socket) & ZMQ_POLLIN) == ZMQ_POLLIN) {
        do {
            zlist_append(socket->str_buffer, zstr_recv_nowait(socket->socket));
        } while (zmq_errno() != EAGAIN && zmq_errno() != EINTR);
        return (VALUE)zlist_pop(socket->str_buffer);
     } else {
        rb_thread_wait_fd(zsocket_fd(socket->socket));
        if (zsocket_rcvtimeo(socket->socket) != -1)
            return zstr_recv(socket->socket);
        goto try_readable;
     }
#endif
}

/*
 *  call-seq:
 *     sock.recv =>  String or nil
 *
 *  Receive a string from this ZMQ socket. May block depending on the socket type.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.bind("inproc://test")
 *     sock.recv    =>  "message"
 *
*/

static VALUE rb_czmq_socket_recv(VALUE obj)
{
    char *str = NULL;
    struct nogvl_recv_args args;
    VALUE result = Qnil;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqAssertSocketNotPending(sock, "can only receive on a bound or connected socket!");
    ZmqSockGuardCrossThread(sock);
    args.socket = sock;
    str = (char *)rb_thread_blocking_region(rb_czmq_nogvl_recv, (void *)&args, RUBY_UBF_IO, 0);
    if (str == NULL) return result;
    ZmqAssertSysError();
    if (sock->verbose)
        zclock_log ("I: %s socket %p: recv \"%s\"", zsocket_type_str(sock->socket), sock->socket, str);
    result = ZmqEncode(rb_str_new2(str));
    free(str);
    return result;
}

/*
 *  call-seq:
 *     sock.recv_nonblock =>  String or nil
 *
 *  Receive a string from this ZMQ socket. Does not block.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.bind("inproc://test")
 *     sock.recv_nonblock    =>  "message"
 *
*/

static VALUE rb_czmq_socket_recv_nonblock(VALUE obj)
{
    char *str = NULL;
    errno = 0;
    VALUE result = Qnil;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqAssertSocketNotPending(sock, "can only receive on a bound or connected socket!");
    ZmqSockGuardCrossThread(sock);
    str = zstr_recv_nowait(sock->socket);
    if (str == NULL) return result;
    ZmqAssertSysError();
    if (sock->verbose)
        zclock_log ("I: %s socket %p: recv_nonblock \"%s\"", zsocket_type_str(sock->socket), sock->socket, str);
    result = ZmqEncode(rb_str_new2(str));
    free(str);
    return result;
}

/*
 * :nodoc:
 *  Sends a frame while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_send_frame(void *ptr)
{
    struct nogvl_send_frame_args *args = ptr;
    errno = 0;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    return (VALUE)zframe_send(&(args->frame), socket->socket, args->flags);
#else
    if (rb_thread_alone()) return (VALUE)zframe_send(&(args->frame), socket->socket, args->flags);
try_writable:
    if ((zsocket_events(socket->socket) & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
        return (VALUE)zframe_send(&(args->frame), socket->socket, args->flags);
    } else {
        rb_thread_wait_fd(zsocket_fd(socket->socket));
        goto try_writable;
    }
#endif
}

/*
 *  call-seq:
 *     sock.send_frame(frame) =>  nil
 *
 *  Sends a ZMQ::Frame instance to this socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.bind("inproc://test")
 *     frame = ZMQ::Frame("frame")
 *     sock.send_frame(frame)    =>  nil
 *     frame = ZMQ::Frame("multi")
 *     sock.send_frame(frame, ZMQ::Frame::MORE)
 *
*/

static VALUE rb_czmq_socket_send_frame(int argc, VALUE *argv, VALUE obj)
{
    struct nogvl_send_frame_args args;
    VALUE frame_obj;
    VALUE flags;
    char print_prefix[255];
    char *cur_time = NULL;
    zframe_t *print_frame = NULL;
    int rc, flgs;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqAssertSocketNotPending(sock, "can only send on a bound or connected socket!");
    ZmqSockGuardCrossThread(sock);
    rb_scan_args(argc, argv, "11", &frame_obj, &flags);
    ZmqGetFrame(frame_obj);

    if (NIL_P(flags)) {
        flgs = 0;
    } else {
        if (SYMBOL_P(flags)) flags = rb_const_get_at(rb_cZmqFrame, rb_to_id(flags));
        Check_Type(flags, T_FIXNUM);
        flgs = FIX2INT(flags);
    }

    if (sock->verbose) {
        cur_time = rb_czmq_formatted_current_time();
        print_frame = (flgs & ZFRAME_REUSE) ? frame : zframe_dup(frame);
    }
    args.socket = sock;
    args.frame = frame;
    args.flags = flgs;
    rc = (int)rb_thread_blocking_region(rb_czmq_nogvl_send_frame, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    if (sock->verbose) ZmqDumpFrame("send_frame", print_frame);
    return Qtrue;
}

/*
 * :nodoc:
 *  Sends a message while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_send_message(void *ptr)
{
    struct nogvl_send_message_args *args = ptr;
    zmq_sock_wrapper *socket = args->socket;
    errno = 0;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    zmsg_send(&(args->message), socket->socket);
#else
    if (rb_thread_alone()) {
        zmsg_send(&(args->message), socket->socket);
        return Qnil;
    }
try_writable:
    if ((zsocket_events(socket->socket) & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
        zmsg_send(&(args->message), socket->socket);
    } else {
        rb_thread_wait_fd(zsocket_fd(socket->socket));
        goto try_writable;
    }
#endif
    return Qnil;
}

/*
 *  call-seq:
 *     sock.send_message(msg) =>  nil
 *
 *  Sends a ZMQ::Message instance to this socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.bind("inproc://test")
 *     msg = ZMQ::Message.new
 *     msg.push ZMQ::Frame("header")
 *     sock.send_message(msg)   =>  nil
 *
*/

static VALUE rb_czmq_socket_send_message(VALUE obj, VALUE message_obj)
{
    struct nogvl_send_message_args args;
    zmsg_t *print_message = NULL;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqAssertSocketNotPending(sock, "can only send on a bound or connected socket!");
    ZmqSockGuardCrossThread(sock);
    ZmqGetMessage(message_obj);
    if (sock->verbose) print_message = zmsg_dup(message->message);
    args.socket = sock;
    args.message = message->message;
    rb_thread_blocking_region(rb_czmq_nogvl_send_message, (void *)&args, RUBY_UBF_IO, 0);
    message->flags |= ZMQ_MESSAGE_DESTROYED;
    if (sock->verbose) ZmqDumpMessage("send_message", print_message);
    return Qnil;
}

/*
 * :nodoc:
 *  Receives a frame while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_recv_frame(void *ptr)
{
    struct nogvl_recv_args *args = ptr;
    errno = 0;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    return (VALUE)zframe_recv(socket->socket);
#else
    if (zlist_size(socket->frame_buffer) != 0)
       return (VALUE)zlist_pop(socket->frame_buffer);
try_readable:
    if ((zsocket_events(socket->socket) & ZMQ_POLLIN) == ZMQ_POLLIN) {
        do {
            zlist_append(socket->frame_buffer, zframe_recv_nowait(socket->socket));
        } while (zmq_errno() != EAGAIN && zmq_errno() != EINTR);
        return (VALUE)zlist_pop(socket->frame_buffer);
     } else {
        rb_thread_wait_fd(zsocket_fd(socket->socket));
        goto try_readable;
     }
#endif
}

/*
 *  call-seq:
 *     sock.recv_frame =>  ZMQ::Frame or nil
 *
 *  Receives a ZMQ frame from this socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.bind("inproc://test")
 *     sock.recv_frame  =>  ZMQ::Frame or nil
 *
*/

static VALUE rb_czmq_socket_recv_frame(VALUE obj)
{
    zframe_t *frame = NULL;
    struct nogvl_recv_args args;
    char print_prefix[255];
    char *cur_time = NULL;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqAssertSocketNotPending(sock, "can only receive on a bound or connected socket!");
    ZmqSockGuardCrossThread(sock);
    args.socket = sock;
    frame = (zframe_t *)rb_thread_blocking_region(rb_czmq_nogvl_recv_frame, (void *)&args, RUBY_UBF_IO, 0);
    if (frame == NULL) return Qnil;
    if (sock->verbose) {
        cur_time = rb_czmq_formatted_current_time();
        ZmqDumpFrame("recv_frame", frame);
    }
    return rb_czmq_alloc_frame(frame);
}

/*
 *  call-seq:
 *     sock.recv_frame_nonblock =>  ZMQ::Frame or nil
 *
 *  Receives a ZMQ frame from this socket. Does not block
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.bind("inproc://test")
 *     sock.recv_frame_nonblock  =>  ZMQ::Frame or nil
 *
*/

static VALUE rb_czmq_socket_recv_frame_nonblock(VALUE obj)
{
    zframe_t *frame = NULL;
    char print_prefix[255];
    char *cur_time = NULL;
    errno = 0;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqAssertSocketNotPending(sock, "can only receive on a bound or connected socket!");
    ZmqSockGuardCrossThread(sock);
    frame = zframe_recv_nowait(sock->socket);
    if (frame == NULL) return Qnil;
    if (sock->verbose) {
        cur_time = rb_czmq_formatted_current_time();
        ZmqDumpFrame("recv_frame_nonblock", frame);
    }
    return rb_czmq_alloc_frame(frame);
}

/*
 * :nodoc:
 *  Receives a message while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_recv_message(void *ptr)
{
    struct nogvl_recv_args *args = ptr;
    errno = 0;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    return (VALUE)zmsg_recv(socket->socket);
#else
    if (zlist_size(socket->msg_buffer) != 0)
       return (VALUE)zlist_pop(socket->msg_buffer);
try_readable:
    if ((zsocket_events(socket->socket) & ZMQ_POLLIN) == ZMQ_POLLIN) {
        do {
            zlist_append(socket->msg_buffer, zmsg_recv(socket->socket));
        } while (zmq_errno() != EAGAIN && zmq_errno() != EINTR);
        return (VALUE)zlist_pop(socket->msg_buffer);
     } else {
        rb_thread_wait_fd(zsocket_fd(socket->socket));
        goto try_readable;
     }
#endif
}

/*
 *  call-seq:
 *     sock.recv_message =>  ZMQ::Message or nil
 *
 *  Receives a ZMQ message from this socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.bind("inproc://test")
 *     sock.recv_message  =>  ZMQ::Message or nil
 *
*/

static VALUE rb_czmq_socket_recv_message(VALUE obj)
{
    zmsg_t *message = NULL;
    struct nogvl_recv_args args;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqAssertSocketNotPending(sock, "can only receive on a bound or connected socket!");
    ZmqSockGuardCrossThread(sock);
    args.socket = sock;
    message = (zmsg_t *)rb_thread_blocking_region(rb_czmq_nogvl_recv_message, (void *)&args, RUBY_UBF_IO, 0);
    if (message == NULL) return Qnil;
    if (sock->verbose) ZmqDumpMessage("recv_message", message);
    return rb_czmq_alloc_message(message);
}

#if ZMQ_VERSION_MAJOR == 2

/*
 *  call-seq:
 *     sock.hwm =>  Fixnum
 *
 *  Returns the socket HWM (High Water Mark) value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.hwm  =>  0
 *
*/

static VALUE rb_czmq_socket_opt_hwm(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_hwm(sock->socket));
}

/*
 *  call-seq:
 *     sock.hwm = 100 =>  nil
 *
 *  Sets the socket HWM (High Water Mark() value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.hwm = 100  =>  nil
 *     sock.hwm  =>  100
 *
*/

static VALUE rb_czmq_socket_set_opt_hwm(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_hwm, "HWM", value);
}

/*
 *  call-seq:
 *     sock.swap =>  Fixnum
 *
 *  Returns the socket SWAP value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.swap  =>  0
 *
*/

static VALUE rb_czmq_socket_opt_swap(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_swap(sock->socket));
}

/*
 *  call-seq:
 *     sock.swap = 100 =>  nil
 *
 *  Sets the socket SWAP value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.swap = 100  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_swap(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_swap, "SWAP", value);
}
#endif

#if ZMQ_VERSION_MAJOR == 3
/*
 *  call-seq:
 *     sock.sndhwm =>  Fixnum
 *
 *  Returns the socket send HWM (High Water Mark) value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.sndhwm  =>  0
 *
*/

static VALUE rb_czmq_socket_opt_sndhwm(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_sndhwm(sock->socket));
}

/*
 *  call-seq:
 *     sock.sndhwm = 100 =>  nil
 *
 *  Sets the socket send HWM (High Water Mark() value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.sndhwm = 100  =>  nil
 *     sock.sndhwm  =>  100
 *
*/

static VALUE rb_czmq_socket_set_opt_sndhwm(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_sndhwm, "SNDHWM", value);
}

/*
 *  call-seq:
 *     sock.rcvhwm =>  Fixnum
 *
 *  Returns the socket receive HWM (High Water Mark) value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.sndhwm  =>  0
 *
*/

static VALUE rb_czmq_socket_opt_rcvhwm(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_rcvhwm(sock->socket));
}

/*
 *  call-seq:
 *     sock.rcvhwm = 100 =>  nil
 *
 *  Sets the socket receive HWM (High Water Mark() value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.rcvhwm = 100  =>  nil
 *     sock.rcvhwm  =>  100
 *
*/

static VALUE rb_czmq_socket_set_opt_rcvhwm(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_rcvhwm, "RCVHWM", value);
}
#endif

/*
 *  call-seq:
 *     sock.affinity =>  Fixnum
 *
 *  Returns the socket AFFINITY value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.affinity  =>  0
 *
*/

static VALUE rb_czmq_socket_opt_affinity(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_affinity(sock->socket));
}

/*
 *  call-seq:
 *     sock.affinity = 1 =>  nil
 *
 *  Sets the socket AFFINITY value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.affinity = 1  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_affinity(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_affinity, "AFFINITY", value);
}

/*
 *  call-seq:
 *     sock.rate =>  Fixnum
 *
 *  Returns the socket RATE value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.rate  =>  40000
 *
*/

static VALUE rb_czmq_socket_opt_rate(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_rate(sock->socket));
}

/*
 *  call-seq:
 *     sock.rate = 50000 =>  nil
 *
 *  Sets the socket RATE value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.rate = 50000  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_rate(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_rate, "RATE", value);
}

/*
 *  call-seq:
 *     sock.recovery_ivl =>  Fixnum
 *
 *  Returns the socket RECOVERY_IVL value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.recovery_ivl  =>  10
 *
*/

static VALUE rb_czmq_socket_opt_recovery_ivl(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_recovery_ivl(sock->socket));
}

/*
 *  call-seq:
 *     sock.recovery_ivl = 20 =>  nil
 *
 *  Sets the socket RECOVERY_IVL value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.recovery_ivl = 20  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_recovery_ivl(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_recovery_ivl, "RECOVERY_IVL", value);
}

#if ZMQ_VERSION_MAJOR == 2
/*
 *  call-seq:
 *     sock.recovery_ivl_msec =>  Fixnum
 *
 *  Returns the socket RECOVERY_IVL_MSEC value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.recovery_ivl_msec  =>  -1
 *
*/

static VALUE rb_czmq_socket_opt_recovery_ivl_msec(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_recovery_ivl_msec(sock->socket));
}

/*
 *  call-seq:
 *     sock.recovery_ivl_msec = 20 =>  nil
 *
 *  Sets the socket RECOVERY_IVL_MSEC value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.recovery_ivl_msec = 20  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_recovery_ivl_msec(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_recovery_ivl_msec, "RECOVERY_IVL_MSEC", value);
}

/*
 *  call-seq:
 *     sock.mcast_loop? =>  boolean
 *
 *  Returns the socket MCAST_LOOP status.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.mcast_loop?  =>  true
 *
*/

static VALUE rb_czmq_socket_opt_mcast_loop(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return (zsocket_mcast_loop(sock->socket) == 1) ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *     sock.mcast_loop = false =>  nil
 *
 *  Sets the socket MCAST_LOOP value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.mcast_loop = false  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_mcast_loop(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetBooleanSockOpt(obj, zsocket_set_mcast_loop, "MCAST_LOOP", value);
}
#endif

/*
 *  call-seq:
 *     sock.sndbuf =>  Fixnum
 *
 *  Returns the socket SNDBUF value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.sndbuf  =>  0
 *
*/

static VALUE rb_czmq_socket_opt_sndbuf(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_sndbuf(sock->socket));
}

/*
 *  call-seq:
 *     sock.sndbuf = 1000 =>  nil
 *
 *  Sets the socket SNDBUF value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.sndbuf = 1000  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_sndbuf(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_sndbuf, "SNDBUF", value);
}

/*
 *  call-seq:
 *     sock.rcvbuf =>  Fixnum
 *
 *  Returns the socket RCVBUF value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.rcvbuf  =>  0
 *
*/

static VALUE rb_czmq_socket_opt_rcvbuf(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_rcvbuf(sock->socket));
}

/*
 *  call-seq:
 *     sock.rcvbuf = 1000 =>  nil
 *
 *  Sets the socket RCVBUF value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.rcvbuf = 1000  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_rcvbuf(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_rcvbuf, "RCVBUF", value);
}

/*
 *  call-seq:
 *     sock.linger =>  Fixnum
 *
 *  Returns the socket LINGER value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.linger  =>  -1
 *
*/

static VALUE rb_czmq_socket_opt_linger(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_linger(sock->socket));
}

/*
 *  call-seq:
 *     sock.linger = 1000 =>  nil
 *
 *  Sets the socket LINGER value in ms.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.linger = 1000  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_linger(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_linger, "LINGER", value);
}

/*
 *  call-seq:
 *     sock.backlog =>  Fixnum
 *
 *  Returns the socket BACKLOG value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.backlog  =>  100
 *
*/

static VALUE rb_czmq_socket_opt_backlog(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_backlog(sock->socket));
}

/*
 *  call-seq:
 *     sock.backlog = 200 =>  nil
 *
 *  Sets the socket BACKLOG value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.backlog = 200  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_backlog(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_backlog, "BACKLOG", value);
}

/*
 *  call-seq:
 *     sock.reconnect_ivl =>  Fixnum
 *
 *  Returns the socket RECONNECT_IVL value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.reconnect_ivl  =>  100
 *
*/

static VALUE rb_czmq_socket_opt_reconnect_ivl(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_reconnect_ivl(sock->socket));
}

/*
 *  call-seq:
 *     sock.reconnect_ivl = 200 =>  nil
 *
 *  Sets the socket RECONNECT_IVL value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.reconnect_ivl = 200  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_reconnect_ivl(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_reconnect_ivl, "RECONNECT_IVL", value);
}

/*
 *  call-seq:
 *     sock.reconnect_ivl_max =>  Fixnum
 *
 *  Returns the socket RECONNECT_IVL_MAX value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.reconnect_ivl_max  =>  0
 *
*/

static VALUE rb_czmq_socket_opt_reconnect_ivl_max(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_reconnect_ivl_max(sock->socket));
}

/*
 *  call-seq:
 *     sock.reconnect_ivl_max = 5 =>  nil
 *
 *  Sets the socket RECONNECT_IVL_MAX value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.reconnect_ivl_max = 5  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_reconnect_ivl_max(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_reconnect_ivl_max, "RECONNECT_IVL_MAX", value);
}

/*
 *  call-seq:
 *     sock.identity = "anonymous" =>  nil
 *
 *  Sets the socket IDENTITY value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.identity = "anonymous"  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_identity(VALUE obj, VALUE value)
{
    char *val;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    Check_Type(value, T_STRING);
    if (RSTRING_LEN(value) == 0) rb_raise(rb_eZmqError, "socket identity cannot be empty.");
    if (RSTRING_LEN(value) > 255) rb_raise(rb_eZmqError, "maximum socket identity is 255 chars.");
    val = StringValueCStr(value);
    zsocket_set_identity(sock->socket, val);
    if (sock->verbose)
        zclock_log ("I: %s socket %p: set option \"IDENTITY\" \"%s\"", zsocket_type_str(sock->socket), obj, val);
    return Qnil;
}

/*
 *  call-seq:
 *     sock.subscribe "ruby" =>  nil
 *
 *  Subscribes this SUB socket to a topic.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:SUB)
 *     sock.subscribe "ruby"  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_subscribe(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetStringSockOpt(obj, zsocket_set_subscribe, "SUBSCRIBE", value, {
       ZmqAssertSockOptFor(ZMQ_SUB)
    });
}

/*
 *  call-seq:
 *     sock.unsubscribe "ruby" =>  nil
 *
 *  Unsubscribes this SUB socket from a topic.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:SUB)
 *     sock.unsubscribe "ruby"  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_unsubscribe(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetStringSockOpt(obj, zsocket_set_unsubscribe, "UNSUBSCRIBE", value, {
       ZmqAssertSockOptFor(ZMQ_SUB)
    });
}

/*
 *  call-seq:
 *     sock.rcvmore =>  boolean
 *
 *  Query if there's more messages to receive.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:SUB)
 *     sock.rcvmore =>  true
 *
*/

static VALUE rb_czmq_socket_opt_rcvmore(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return (zsocket_rcvmore(sock->socket) == 1) ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *     sock.events =>  Fixnum
 *
 *  Query if this socket is in a readable or writable state.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:SUB)
 *     sock.events => ZMQ::POLLIN
 *
*/

static VALUE rb_czmq_socket_opt_events(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_events(sock->socket));
}

/*
 *  call-seq:
 *     sock.rcvtimeo =>  Fixnum
 *
 *  Returns the socket RCVTIMEO value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.rcvtimeo  =>  -1
 *
*/

static VALUE rb_czmq_socket_opt_rcvtimeo(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_rcvtimeo(sock->socket));
}

/*
 *  call-seq:
 *     sock.rcvtimeout = 200 =>  nil
 *
 *  Sets the socket RCVTIMEO value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.rcvtimeo = 200  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_rcvtimeo(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_rcvtimeo, "RCVTIMEO", value);
}

/*
 *  call-seq:
 *     sock.sndtimeo =>  Fixnum
 *
 *  Returns the socket SNDTIMEO value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.sndtimeo  =>  -1
 *
*/

static VALUE rb_czmq_socket_opt_sndtimeo(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_sndtimeo(sock->socket));
}

/*
 *  call-seq:
 *     sock.sndtimeout = 200 =>  nil
 *
 *  Sets the socket SNDTIMEO value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.sndtimeo = 200  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_sndtimeo(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_sndtimeo, "SNDTIMEO", value);
}

#if defined (ZMQ_ROUTER_RAW)
/*
 *  call-seq:
 *     sock.raw = true =>  nil
 *
 *  Define this as a RAW socket - applicable to ROUTER sockets only.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:ROUTER)
 *     sock.router = 200  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_raw(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetBooleanSockOpt(obj, zsocket_set_router_raw, "ROUTER_RAW", value);
}
#endif

/*
 * :nodoc:
 *  Receives a monitoring event message while the GIL is released.
 *
*/

static VALUE rb_czmq_nogvl_monitor_recv(void *ptr)
{
    struct nogvl_monitor_recv_args *args = ptr;
    int rc;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    rc = zmq_recvmsg (args->socket, &args->msg, 0);
#else
    int fd;
    size_t option_len = sizeof (int);
    zmq_getsockopt (args->socket, ZMQ_FD, &fd, &option_len);
try_readable:
    if ((zsocket_events(args->socket) & ZMQ_POLLIN) == ZMQ_POLLIN) {
        rc = zmq_recvmsg (args->socket, &args->msg, 0);
     } else {
        rb_thread_wait_fd(fd);
        goto try_readable;
     }
#endif
    return (VALUE)rc;
}

/*
 * :nodoc:
 *  Runs with the context of a new Ruby thread and spawns a PAIR socket for handling monitoring events
 *
*/

static VALUE rb_czmq_socket_monitor_thread(void *arg)
{
    zmq_event_t event;
    struct nogvl_monitor_recv_args args;
    int rc;
    zmq_sock_wrapper *sock = (zmq_sock_wrapper *)arg;

    void *s = zsocket_new (sock->ctx, ZMQ_PAIR);
    assert (s);

    rc = zmq_connect (s, StringValueCStr(sock->monitor_endpoint));
    assert (rc == 0);

    THREAD_PASS;

    while (1) {
        args.socket = s;
        zmq_msg_init (&args.msg);
        rc = (int)rb_thread_blocking_region(rb_czmq_nogvl_monitor_recv, (void *)&args, RUBY_UBF_IO, 0);
        if (rc == -1 && (zmq_errno() == ETERM || zmq_errno() == ENOTSOCK || zmq_errno() == EINTR)) break;
        assert (rc != -1);
        memcpy (&event, zmq_msg_data (&args.msg), sizeof (event));
        switch (event.event) {
        case ZMQ_EVENT_CONNECTED: rb_funcall(sock->monitor_handler, intern_on_connected, 2, rb_str_new2(event.data.connected.addr), INT2FIX(event.data.connected.fd));
                                  break;
        case ZMQ_EVENT_CONNECT_DELAYED: rb_funcall(sock->monitor_handler, intern_on_connect_delayed, 2, rb_str_new2(event.data.connect_delayed.addr), INT2FIX(event.data.connect_delayed.err));
                                        break;
        case ZMQ_EVENT_CONNECT_RETRIED: rb_funcall(sock->monitor_handler, intern_on_connect_retried, 2, rb_str_new2(event.data.connect_retried.addr), INT2FIX(event.data.connect_retried.interval));
                                        break;
        case ZMQ_EVENT_LISTENING: rb_funcall(sock->monitor_handler, intern_on_listening, 2, rb_str_new2(event.data.listening.addr), INT2FIX(event.data.listening.fd));
                                  break;
        case ZMQ_EVENT_BIND_FAILED: rb_funcall(sock->monitor_handler, intern_on_bind_failed, 2, rb_str_new2(event.data.bind_failed.addr), INT2FIX(event.data.bind_failed.err));
                                    break;
        case ZMQ_EVENT_ACCEPTED: rb_funcall(sock->monitor_handler, intern_on_accepted, 2, rb_str_new2(event.data.accepted.addr), INT2FIX(event.data.accepted.fd));
                                 break;
        case ZMQ_EVENT_ACCEPT_FAILED: rb_funcall(sock->monitor_handler, intern_on_accept_failed, 2, rb_str_new2(event.data.accept_failed.addr), INT2FIX(event.data.accept_failed.err));
                                      break;
        case ZMQ_EVENT_CLOSE_FAILED: rb_funcall(sock->monitor_handler, intern_on_close_failed, 2, rb_str_new2(event.data.close_failed.addr), INT2FIX(event.data.close_failed.err));
                                     break;
        case ZMQ_EVENT_CLOSED: rb_funcall(sock->monitor_handler, intern_on_closed, 2, rb_str_new2(event.data.closed.addr), INT2FIX(event.data.closed.fd));
                               break;
        case ZMQ_EVENT_DISCONNECTED: rb_funcall(sock->monitor_handler, intern_on_disconnected, 2, rb_str_new2(event.data.disconnected.addr), INT2FIX(event.data.disconnected.fd));
                                     break;
        }
    }
    zmq_close (s);
    return Qnil;
}

/*
 *  call-seq:
 *     sock.monitor("inproc://monitoring", callback, events) =>  nil
 *
 *  Registers a monitoring callback for this socket
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     rep = ctx.socket(:REP)
 *     rep.monitor("inproc://monitoring.rep", RepMonitor)
 *     req = ctx.socket(:REQ)
 *     req.monitor("inproc://monitoring.req", ReqMonitor, ZMQ_EVENT_DISCONNECTED)
 *     rep.bind("tcp://127.0.0.1:5331")
 *     rep.bind("tcp://127.0.0.1:5332")
 *
*/

static VALUE rb_czmq_socket_monitor(int argc, VALUE *argv, VALUE obj)
{
    VALUE endpoint;
    VALUE handler;
    VALUE events;
    int rc;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    rb_scan_args(argc, argv, "12", &endpoint, &handler, &events);
    Check_Type(endpoint, T_STRING);
    if (NIL_P(events))
        events = rb_const_get_at(rb_mZmq, rb_intern("EVENT_ALL"));
    if (NIL_P(handler)) {
        handler = rb_class_new_instance(0, NULL, rb_const_get_at(rb_mZmq, rb_intern("Monitor")));
    }
    Check_Type(events, T_FIXNUM);
    rc = zmq_socket_monitor(sock->socket, StringValueCStr(endpoint), NUM2INT(events));
    if (rc == 0) {
        sock->monitor_endpoint = endpoint;
        sock->monitor_handler = handler;
        sock->monitor_thread = rb_thread_create(rb_czmq_socket_monitor_thread, (void*)sock);
        rb_thread_run(sock->monitor_thread);
        return Qtrue;
    } else {
        return Qfalse;
    }
}

void _init_rb_czmq_socket()
{
    rb_cZmqSocket = rb_define_class_under(rb_mZmq, "Socket", rb_cObject);
    rb_cZmqPubSocket = rb_define_class_under(rb_cZmqSocket, "Pub", rb_cZmqSocket);
    rb_cZmqSubSocket = rb_define_class_under(rb_cZmqSocket, "Sub", rb_cZmqSocket);;
    rb_cZmqPushSocket = rb_define_class_under(rb_cZmqSocket, "Push", rb_cZmqSocket);;
    rb_cZmqPullSocket = rb_define_class_under(rb_cZmqSocket, "Pull", rb_cZmqSocket);;
    rb_cZmqRouterSocket = rb_define_class_under(rb_cZmqSocket, "Router", rb_cZmqSocket);;
    rb_cZmqDealerSocket = rb_define_class_under(rb_cZmqSocket, "Dealer", rb_cZmqSocket);;
    rb_cZmqRepSocket = rb_define_class_under(rb_cZmqSocket, "Rep", rb_cZmqSocket);;
    rb_cZmqReqSocket = rb_define_class_under(rb_cZmqSocket, "Req", rb_cZmqSocket);;
    rb_cZmqPairSocket = rb_define_class_under(rb_cZmqSocket, "Pair", rb_cZmqSocket);;
    rb_cZmqXPubSocket = rb_define_class_under(rb_cZmqSocket, "XPub", rb_cZmqSocket);
    rb_cZmqXSubSocket = rb_define_class_under(rb_cZmqSocket, "XSub", rb_cZmqSocket);;

    intern_on_connected = rb_intern("on_connected");
    intern_on_connect_delayed = rb_intern("on_connect_delayed");
    intern_on_connect_retried = rb_intern("on_connect_retried");
    intern_on_listening = rb_intern("on_listening");
    intern_on_bind_failed = rb_intern("on_bind_failed");
    intern_on_accepted = rb_intern("on_accepted");
    intern_on_accept_failed = rb_intern("on_accept_failed");
    intern_on_closed = rb_intern("on_closed");
    intern_on_close_failed = rb_intern("on_close_failed");
    intern_on_disconnected = rb_intern("on_disconnected");

    rb_define_const(rb_cZmqSocket, "PENDING", INT2NUM(ZMQ_SOCKET_PENDING));
    rb_define_const(rb_cZmqSocket, "BOUND", INT2NUM(ZMQ_SOCKET_BOUND));
    rb_define_const(rb_cZmqSocket, "CONNECTED", INT2NUM(ZMQ_SOCKET_CONNECTED));

    rb_define_method(rb_cZmqSocket, "close", rb_czmq_socket_close, 0);
    rb_define_method(rb_cZmqSocket, "endpoints", rb_czmq_socket_endpoints, 0);
    rb_define_method(rb_cZmqSocket, "state", rb_czmq_socket_state, 0);
    rb_define_method(rb_cZmqSocket, "real_bind", rb_czmq_socket_bind, 1);
    rb_define_method(rb_cZmqSocket, "real_connect", rb_czmq_socket_connect, 1);
    rb_define_method(rb_cZmqSocket, "fd", rb_czmq_socket_fd, 0);
    rb_define_alias(rb_cZmqSocket, "to_i", "fd");
    rb_define_method(rb_cZmqSocket, "verbose=", rb_czmq_socket_set_verbose, 1);
    rb_define_method(rb_cZmqSocket, "send", rb_czmq_socket_send, 1);
    rb_define_method(rb_cZmqSocket, "sendm", rb_czmq_socket_sendm, 1);
    rb_define_method(rb_cZmqSocket, "send_frame", rb_czmq_socket_send_frame, -1);
    rb_define_method(rb_cZmqSocket, "send_message", rb_czmq_socket_send_message, 1);
    rb_define_method(rb_cZmqSocket, "recv", rb_czmq_socket_recv, 0);
    rb_define_method(rb_cZmqSocket, "recv_nonblock", rb_czmq_socket_recv_nonblock, 0);
    rb_define_method(rb_cZmqSocket, "recv_frame", rb_czmq_socket_recv_frame, 0);
    rb_define_method(rb_cZmqSocket, "recv_frame_nonblock", rb_czmq_socket_recv_frame_nonblock, 0);
    rb_define_method(rb_cZmqSocket, "recv_message", rb_czmq_socket_recv_message, 0);

#if ZMQ_VERSION_MAJOR == 2
    rb_define_method(rb_cZmqSocket, "swap", rb_czmq_socket_opt_swap, 0);
    rb_define_method(rb_cZmqSocket, "swap=", rb_czmq_socket_set_opt_swap, 1);
    rb_define_method(rb_cZmqSocket, "recovery_ivl_msec", rb_czmq_socket_opt_recovery_ivl_msec, 0);
    rb_define_method(rb_cZmqSocket, "recovery_ivl_msec=", rb_czmq_socket_set_opt_recovery_ivl_msec, 1);
    rb_define_method(rb_cZmqSocket, "mcast_loop?", rb_czmq_socket_opt_mcast_loop, 0);
    rb_define_method(rb_cZmqSocket, "mcast_loop=", rb_czmq_socket_set_opt_mcast_loop, 1);
    rb_define_method(rb_cZmqSocket, "hwm", rb_czmq_socket_opt_hwm, 0);
    rb_define_method(rb_cZmqSocket, "hwm=", rb_czmq_socket_set_opt_hwm, 1);
#endif

#if ZMQ_VERSION_MAJOR == 3
    rb_define_method(rb_cZmqSocket, "sndhwm", rb_czmq_socket_opt_sndhwm, 0);
    rb_define_method(rb_cZmqSocket, "sndhwm=", rb_czmq_socket_set_opt_sndhwm, 1);
    rb_define_method(rb_cZmqSocket, "rcvhwm", rb_czmq_socket_opt_rcvhwm, 0);
    rb_define_method(rb_cZmqSocket, "rcvhwm=", rb_czmq_socket_set_opt_rcvhwm, 1);

#if defined (ZMQ_ROUTER_RAW)
    rb_define_method(rb_cZmqSocket, "raw=", rb_czmq_socket_set_opt_raw, 1);
#endif
#endif

    rb_define_method(rb_cZmqSocket, "affinity", rb_czmq_socket_opt_affinity, 0);
    rb_define_method(rb_cZmqSocket, "affinity=", rb_czmq_socket_set_opt_affinity, 1);
    rb_define_method(rb_cZmqSocket, "rate", rb_czmq_socket_opt_rate, 0);
    rb_define_method(rb_cZmqSocket, "rate=", rb_czmq_socket_set_opt_rate, 1);
    rb_define_method(rb_cZmqSocket, "recovery_ivl", rb_czmq_socket_opt_recovery_ivl, 0);
    rb_define_method(rb_cZmqSocket, "recovery_ivl=", rb_czmq_socket_set_opt_recovery_ivl, 1);
    rb_define_method(rb_cZmqSocket, "sndbuf", rb_czmq_socket_opt_sndbuf, 0);
    rb_define_method(rb_cZmqSocket, "sndbuf=", rb_czmq_socket_set_opt_sndbuf, 1);
    rb_define_method(rb_cZmqSocket, "rcvbuf", rb_czmq_socket_opt_rcvbuf, 0);
    rb_define_method(rb_cZmqSocket, "rcvbuf=", rb_czmq_socket_set_opt_rcvbuf, 1);
    rb_define_method(rb_cZmqSocket, "linger", rb_czmq_socket_opt_linger, 0);
    rb_define_method(rb_cZmqSocket, "linger=", rb_czmq_socket_set_opt_linger, 1);
    rb_define_method(rb_cZmqSocket, "backlog", rb_czmq_socket_opt_backlog, 0);
    rb_define_method(rb_cZmqSocket, "backlog=", rb_czmq_socket_set_opt_backlog, 1);
    rb_define_method(rb_cZmqSocket, "reconnect_ivl", rb_czmq_socket_opt_reconnect_ivl, 0);
    rb_define_method(rb_cZmqSocket, "reconnect_ivl=", rb_czmq_socket_set_opt_reconnect_ivl, 1);
    rb_define_method(rb_cZmqSocket, "reconnect_ivl_max", rb_czmq_socket_opt_reconnect_ivl_max, 0);
    rb_define_method(rb_cZmqSocket, "reconnect_ivl_max=", rb_czmq_socket_set_opt_reconnect_ivl_max, 1);
    rb_define_method(rb_cZmqSocket, "identity=", rb_czmq_socket_set_opt_identity, 1);
    rb_define_method(rb_cZmqSocket, "subscribe", rb_czmq_socket_set_opt_subscribe, 1);
    rb_define_method(rb_cZmqSocket, "unsubscribe", rb_czmq_socket_set_opt_unsubscribe, 1);
    rb_define_method(rb_cZmqSocket, "rcvmore?", rb_czmq_socket_opt_rcvmore, 0);
    rb_define_method(rb_cZmqSocket, "events", rb_czmq_socket_opt_events, 0);
    rb_define_method(rb_cZmqSocket, "rcvtimeo", rb_czmq_socket_opt_rcvtimeo, 0);
    rb_define_method(rb_cZmqSocket, "rcvtimeo=", rb_czmq_socket_set_opt_rcvtimeo, 1);
    rb_define_method(rb_cZmqSocket, "sndtimeo", rb_czmq_socket_opt_sndtimeo, 0);
    rb_define_method(rb_cZmqSocket, "sndtimeo=", rb_czmq_socket_set_opt_sndtimeo, 1);
    rb_define_method(rb_cZmqSocket, "monitor", rb_czmq_socket_monitor, -1);
}
