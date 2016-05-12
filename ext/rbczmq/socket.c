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
 *  GC mark callback
 *
*/
void rb_czmq_mark_sock(void *ptr)
{
    zmq_sock_wrapper *sock = (zmq_sock_wrapper *)ptr;
    if (sock && sock->ctx){
        if (sock->verbose) {
            zclock_log ("I: %s socket %p, context %p: GC mark", sock->flags & ZMQ_SOCKET_DESTROYED ? "(closed)" : zsocket_type_str(sock->socket), sock, sock->ctx);
        }
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
    if (sock) {
        if (sock->verbose) {
            zclock_log ("I: %s socket %p, context %p: GC free", sock->flags & ZMQ_SOCKET_DESTROYED ? "(closed)" : zsocket_type_str(sock->socket), sock, sock ? sock->ctx : NULL);
        }
/*
        XXX: cyclic dependency
        #4  0x0000000100712524 in zsocket_set_linger (linger=1, socket=<value temporarily unavailable, due to optimizations>) at zsocket.c:288
        if (sock->socket != NULL && !(sock->flags & ZMQ_SOCKET_DESTROYED)) rb_czmq_free_sock(sock);

        SOLVED:

        The above occurs when a socket is not explicitly destroyed and the socket happens to be garbage
        collected *after* the context has been destroyed. When the context is destroyed, all sockets
        in the context are closed internally by czmq. However, the rbczmq ZMQ::Socket object still
        thinks it is alive and will destroy again.

        The new socket destroy method 'rb_czmq_context_destroy_socket' is idempotent and can safely
        be called after either context or socket has been destroyed.
*/

        rb_czmq_context_destroy_socket(sock);
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
    sock->state = ZMQ_SOCKET_DISCONNECTED;
    rb_czmq_context_destroy_socket(sock);
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
    if (sock->state == ZMQ_SOCKET_PENDING || sock->state == ZMQ_SOCKET_DISCONNECTED) return INT2NUM(-1);
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
    rc = zsocket_bind(socket->socket, "%s", args->endpoint);
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
    rc = zsocket_connect(socket->socket, "%s", args->endpoint);
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
    rc = (int)rb_thread_call_without_gvl(rb_czmq_nogvl_socket_bind, (void *)&args, RUBY_UBF_IO, 0);
    /* ZmqAssert will return false on any non-zero return code. Bind returns the port number */
    if (rc < 0) {
        ZmqAssert(rc);
    }
    /* get the endpoint name with any ephemeral ports filled in. */
    char* endpoint_string = zsocket_last_endpoint(sock->socket);
    ZmqAssert(endpoint_string != NULL);
    if (sock->verbose)
        zclock_log ("I: %s socket %p: bound \"%s\"", zsocket_type_str(sock->socket), obj, endpoint_string);
    sock->state = ZMQ_SOCKET_BOUND;
    rb_ary_push(sock->endpoints, rb_str_new_cstr(endpoint_string));
    free(endpoint_string);
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
    rc = (int)rb_thread_call_without_gvl(rb_czmq_nogvl_socket_connect, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    /* get the endpoint name with any ephemeral ports filled in. */
    char* endpoint_string = zsocket_last_endpoint(sock->socket);
    ZmqAssert(endpoint_string != NULL);
    if (sock->verbose)
        zclock_log ("I: %s socket %p: connected \"%s\"", zsocket_type_str(sock->socket), obj, endpoint_string);
    sock->state = ZMQ_SOCKET_CONNECTED;
    rb_ary_push(sock->endpoints, rb_str_new_cstr(endpoint_string));
    free(endpoint_string);
    return Qtrue;
}

/*
 * :nodoc:
 *  Disconnects from an endpoint while the GIL is released.
 *
*/
VALUE rb_czmq_nogvl_socket_disconnect(void *ptr)
{
    int rc;
    struct nogvl_conn_args *args = ptr;
    errno = 0;
    zmq_sock_wrapper *socket = args->socket;
    rc = zsocket_disconnect(socket->socket, "%s", args->endpoint);
    return (VALUE)rc;
}

/*
 *  call-seq:
 *     sock.disconnect("tcp://localhost:3456")   =>  boolean
 *
 *  Attempts to disconnect from a given endpoint.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     rep = ctx.socket(:REP)
 *     port = rep.bind("tcp://localhost:*")    =>  5432
 *     req = ctx.socket(:REQ)
 *     req.connect("tcp://localhost:#{port}")   => true
 *     req.disconnect("tcp://localhost:#{port}")   => true
 *
*/

static VALUE rb_czmq_socket_disconnect(VALUE obj, VALUE endpoint)
{
    struct nogvl_conn_args args;
    int rc;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    Check_Type(endpoint, T_STRING);
    args.socket = sock;
    args.endpoint = StringValueCStr(endpoint);
    rc = (int)rb_thread_call_without_gvl(rb_czmq_nogvl_socket_disconnect, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    if (sock->verbose)
        zclock_log ("I: %s socket %p: disconnected \"%s\"", zsocket_type_str(sock->socket), obj, StringValueCStr(endpoint));
    rb_ary_delete(sock->endpoints, endpoint);
    long endpoint_count = RARRAY_LEN(sock->endpoints);
    if (endpoint_count == 0) {
        sock->state = ZMQ_SOCKET_DISCONNECTED;
    }
    return Qtrue;
}

/*
 * :nodoc:
 *  Unbinds from an endpoint while the GIL is released.
 *
*/
VALUE rb_czmq_nogvl_socket_unbind(void *ptr)
{
    int rc;
    struct nogvl_conn_args *args = ptr;
    errno = 0;
    zmq_sock_wrapper *socket = args->socket;
    rc = zsocket_unbind(socket->socket, "%s", args->endpoint);
    return (VALUE)rc;
}


/*
 *  call-seq:
 *     sock.disconnect("tcp://localhost:3456")   =>  boolean
 *
 *  Attempts to disconnect from a given endpoint.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     rep = ctx.socket(:REP)
 *     port = rep.bind("tcp://localhost:*")    =>  5432
 *     req = ctx.socket(:REQ)
 *     req.connect("tcp://localhost:#{port}")   => true
 *     rep.unbind("tcp://localhost:#{port}")   => true
 *
*/

static VALUE rb_czmq_socket_unbind(VALUE obj, VALUE endpoint)
{
    struct nogvl_conn_args args;
    int rc;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    Check_Type(endpoint, T_STRING);
    args.socket = sock;
    args.endpoint = StringValueCStr(endpoint);
    rc = (int)rb_thread_call_without_gvl(rb_czmq_nogvl_socket_unbind, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    if (sock->verbose)
        zclock_log ("I: %s socket %p: unbound \"%s\"", zsocket_type_str(sock->socket), obj, StringValueCStr(endpoint));
    rb_ary_delete(sock->endpoints, endpoint);
    long endpoint_count = RARRAY_LEN(sock->endpoints);
    if (endpoint_count == 0) {
        sock->state = ZMQ_SOCKET_DISCONNECTED;
    }
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
 *
 * Based on czmq `s_send_string` to send a C string. We need to be able to support
 * strings that contain null bytes, so we cannot use zstr_send as it is intended for
 * null terminated C strings.
 */
static int rb_czmq_nogvl_zstr_send_internal(struct nogvl_send_args *args, int flags)
{
    errno = 0;
    zmq_sock_wrapper *socket = args->socket;

    zmq_msg_t message;
    zmq_msg_init_size(&message, args->length);
    memcpy(zmq_msg_data(&message), args->msg, args->length);
    int rc = zmq_sendmsg(socket->socket, &message, flags);
    return (rc == -1? -1: 0);
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
    int rc = rb_czmq_nogvl_zstr_send_internal(args, 0);
    return (VALUE)rc;
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
    int rc = rb_czmq_nogvl_zstr_send_internal(args, ZMQ_SNDMORE);
    return (VALUE)rc;
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
    args.socket = sock;
    StringValue(msg);
    Check_Type(msg, T_STRING);
    args.msg = RSTRING_PTR(msg);
    args.length = RSTRING_LEN(msg);
    rc = (int)rb_thread_call_without_gvl(rb_czmq_nogvl_zstr_send, (void *)&args, RUBY_UBF_IO, 0);
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
    args.socket = sock;
    StringValue(msg);
    Check_Type(msg, T_STRING);
    args.msg = RSTRING_PTR(msg);
    args.length = RSTRING_LEN(msg);
    rc = (int)rb_thread_call_without_gvl(rb_czmq_nogvl_zstr_sendm, (void *)&args, RUBY_UBF_IO, 0);
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

    // implement similar to zstr_recv, except that the copy to the string is done
    // after we return with the GIL so that the ruby string object can be created
    // from the zmq message buffer in a single copy.
    assert (socket->socket);
    int rc = zmq_recvmsg(socket->socket, &args->message, 0);
    return (VALUE)rc;
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
    zmq_msg_init(&args.message);

    int rc = (int)rb_thread_call_without_gvl(rb_czmq_nogvl_recv, (void *)&args, RUBY_UBF_IO, 0);
    if (rc < 0) {
        zmq_msg_close(&args.message);
        return Qnil;
    }
    ZmqAssertSysError();
    if (sock->verbose)
        zclock_log ("I: %s socket %p: recv \"%s\"", zsocket_type_str(sock->socket), sock->socket, str);

    result = rb_str_new(zmq_msg_data(&args.message), zmq_msg_size(&args.message));
    zmq_msg_close(&args.message);

    result = ZmqEncode(result);
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
    struct nogvl_recv_args args;
    errno = 0;
    VALUE result = Qnil;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ZmqAssertSocketNotPending(sock, "can only receive on a bound or connected socket!");
    ZmqSockGuardCrossThread(sock);

    zmq_msg_init(&args.message);

    int rc = zmq_recvmsg(sock->socket, &args.message, ZMQ_DONTWAIT);
    if (rc < 0) {
        zmq_msg_close(&args.message);
        return Qnil;
    }
    ZmqAssertSysError();

    result = rb_str_new(zmq_msg_data(&args.message), zmq_msg_size(&args.message));
    zmq_msg_close(&args.message);

    if (sock->verbose) {
        zclock_log ("I: %s socket %p: recv \"%s\"", zsocket_type_str(sock->socket), sock->socket, StringValueCStr(result));
    }

    result = ZmqEncode(result);
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
    return (VALUE)zframe_send(&(args->frame), socket->socket, args->flags);
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
    ZmqAssertFrameOwnedNoMessage(frame);

    if (NIL_P(flags)) {
        flgs = 0;
    } else {
        if (SYMBOL_P(flags)) flags = rb_const_get_at(rb_cZmqFrame, rb_to_id(flags));
        Check_Type(flags, T_FIXNUM);
        flgs = FIX2INT(flags);
    }

    if (sock->verbose) {
        cur_time = rb_czmq_formatted_current_time();
        print_frame = (flgs & ZFRAME_REUSE) ? frame->frame : zframe_dup(frame->frame);
    }
    args.socket = sock;
    args.frame = frame->frame;
    args.flags = flgs;
    rc = (int)rb_thread_call_without_gvl(rb_czmq_nogvl_send_frame, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    if ((flgs & ZFRAME_REUSE) == 0) {
        /* frame has been destroyed, clear the owns flag */
        frame->flags &= ~ZMQ_FRAME_OWNED;
    }
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
    zmsg_send(&(args->message), socket->socket);
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
    ZmqAssertMessageOwned(message);
    if (sock->verbose) print_message = zmsg_dup(message->message);
    args.socket = sock;
    args.message = message->message;
    rb_thread_call_without_gvl(rb_czmq_nogvl_send_message, (void *)&args, RUBY_UBF_IO, 0);
    message->flags &= ~ZMQ_MESSAGE_OWNED;
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
    return (VALUE)zframe_recv(socket->socket);
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
    frame = (zframe_t *)rb_thread_call_without_gvl(rb_czmq_nogvl_recv_frame, (void *)&args, RUBY_UBF_IO, 0);
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
    return (VALUE)zmsg_recv(socket->socket);
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
    message = (zmsg_t *)rb_thread_call_without_gvl(rb_czmq_nogvl_recv_message, (void *)&args, RUBY_UBF_IO, 0);
    if (message == NULL) return Qnil;
    if (sock->verbose) ZmqDumpMessage("recv_message", message);
    return rb_czmq_alloc_message(message);
}

/*
 * :nodoc:
 *  Poll for input while the GIL is released.
 *
*/
static VALUE rb_czmq_nogvl_poll(void *ptr)
{
    struct nogvl_socket_poll_args *args = ptr;
    zmq_sock_wrapper *socket = args->socket;
    return (VALUE)zsocket_poll(socket->socket, args->timeout);
}

/*
 *  call-seq:
 *     sock.poll(100) =>  Boolean
 *
 *  Poll for input events on the socket. Returns true if there is input, otherwise false.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.bind("inproc://test")
 *     sock.poll(100)  =>  true
 *
*/

static VALUE rb_czmq_socket_poll(VALUE obj, VALUE timeout)
{
    bool readable;
    struct nogvl_socket_poll_args args;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    Check_Type(timeout, T_FIXNUM);
    ZmqSockGuardCrossThread(sock);
    args.socket = sock;
    args.timeout = FIX2INT(timeout);
    readable = (bool)rb_thread_call_without_gvl(rb_czmq_nogvl_poll, (void *)&args, RUBY_UBF_IO, 0);
    return (readable == true) ? Qtrue : Qfalse;
}

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

/*
 *  call-seq:
 *     sock.maxmsgsize =>  Fixnum
 *
 *  Returns the socket MAXMSGSIZE value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.maxmsgsize  =>  10
 *
*/

static VALUE rb_czmq_socket_opt_maxmsgsize(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_maxmsgsize(sock->socket));
}

/*
 *  call-seq:
 *     sock.maxmsgsize = 20 =>  nil
 *
 *  Sets the socket MAXMSGSIZE value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.maxmsgsize = 20  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_maxmsgsize(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_maxmsgsize, "MAXMSGSIZE", value);
}

/*
 *  call-seq:
 *     sock.multicast_hops =>  Fixnum
 *
 *  Returns the socket MULTICAST_HOPS value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.multicast_hops  =>  10
 *
*/

static VALUE rb_czmq_socket_opt_multicast_hops(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    return INT2NUM(zsocket_multicast_hops(sock->socket));
}

/*
 *  call-seq:
 *     sock.multicast_hops = 20 =>  nil
 *
 *  Sets the socket MULTICAST_HOPS value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.multicast_hops = 20  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_multicast_hops(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetSockOpt(obj, zsocket_set_multicast_hops, "MULTICAST_HOPS", value);
}

/*
 *  call-seq:
 *     sock.ipv4only =>  Boolean
 *
 *  Returns the socket IPV4ONLY value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.ipv4only  =>  false
 *
*/

static VALUE rb_czmq_socket_opt_ipv4only(VALUE obj)
{
    int ipv4only;
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    ipv4only = zsocket_ipv4only(sock->socket);
    return (ipv4only == 0 ? Qfalse : Qtrue);
}

/*
 *  call-seq:
 *     sock.ipv4only = true =>  nil
 *
 *  Sets the socket IPV4ONLY value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.ipv4only = true  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_ipv4only(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetBooleanSockOpt(obj, zsocket_set_ipv4only, "IPV4ONLY", value);
}

/*
 *  call-seq:
 *     sock.delay_attach_on_connect = true =>  nil
 *
 *  Sets the socket DELAY_ATTACH_ON_CONNECT value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.delay_attach_on_connect = true  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_delay_attach_on_connect(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetBooleanSockOpt(obj, zsocket_set_delay_attach_on_connect, "DELAY_ATTACH_ON_CONNECT", value);
}

/*
 *  call-seq:
 *     sock.router_mandatory = true =>  nil
 *
 *  Sets the socket ROUTER_MANDATORY value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.router_mandatory = true  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_router_mandatory(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetBooleanSockOpt(obj, zsocket_set_router_mandatory, "ROUTER_MANDATORY", value);
}

/*
 *  call-seq:
 *     sock.router_raw= true =>  nil
 *
 *  Sets the socket ROUTER_RAW value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.router_raw = true  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_router_raw(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetBooleanSockOpt(obj, zsocket_set_router_raw, "ROUTER_RAW", value);
}

/*
 *  call-seq:
 *     sock.xpub_verbose = true =>  nil
 *
 *  Sets the socket XPUB_VERBOSE value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.xpub_verbose = true  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_xpub_verbose(VALUE obj, VALUE value)
{
    zmq_sock_wrapper *sock = NULL;
    ZmqSetBooleanSockOpt(obj, zsocket_set_xpub_verbose, "XPUB_VERBOSE", value);
}

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
    // return INT2NUM(zsocket_linger(sock->socket));
    // TODO: how to get the linger value in ZMQ4/CZMQ2?
    return INT2NUM(-1);
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


/*
 *  call-seq:
 *     port = sock.bind('tcp://0.0.0.0:*')  =>  41415
 *     sock.last_endpoint  =>  "tcp://0.0.0.0:41415"
 *
 *  Gets the last endpoint that this socket connected or bound to.
 *
*/

static VALUE rb_czmq_socket_opt_last_endpoint(VALUE obj)
{
    zmq_sock_wrapper *sock = NULL;
    GetZmqSocket(obj);
    char* endpoint_string = zsocket_last_endpoint(sock->socket);
    VALUE result = rb_str_new_cstr(endpoint_string);
    if (endpoint_string != NULL) {
        free(endpoint_string);
    }
    return result;
}

/*
 *  call-seq:
 *     sock.stream_notify = false =>  nil
 *
 *  Sets the socket stream_notify value.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:STREAM)
 *     sock.stream_notify = false  =>  nil
 *
*/

static VALUE rb_czmq_socket_set_opt_stream_notify(VALUE obj, VALUE value)
{
    int rc, optval;
    zmq_sock_wrapper *sock = NULL;

    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    CheckBoolean(value);
    optval = (value == Qtrue) ? 1 : 0;

    rc = zmq_setsockopt(sock->socket, ZMQ_STREAM_NOTIFY, &optval, sizeof(optval));
    ZmqAssert(rc);

    if (sock->verbose)
        zclock_log ("I: %s socket %p: set option \"STREAM_NOTIFY\" %d",
                    zsocket_type_str(sock->socket), (void *)obj, optval);

    return Qnil;
}

/*
 * :nodoc:
 *  Receives a monitoring event message while the GIL is released.
 *
*/

static VALUE rb_czmq_nogvl_monitor_recv(void *ptr)
{
    struct nogvl_monitor_recv_args *args = ptr;
    int rc;
    /* if the socket being monitored has been destroyed, simply return
       an error condition. The monitoring thread will then exit. */
    if (args->monitored_socket_wrapper->flags & ZMQ_SOCKET_DESTROYED)
    {
        return (VALUE)-1;
    }
    rc = zmq_recvmsg (args->socket, &args->msg_event, 0);
    if (rc == 0 && zmq_msg_more(&args->msg_event))
    {
        rc = zmq_recvmsg (args->socket, &args->msg_endpoint, 0);
    }
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

    rb_thread_schedule();

    args.monitored_socket_wrapper = sock;
    args.socket = s;

    while (1) {
        zmq_msg_init (&args.msg_event);
        zmq_msg_init (&args.msg_endpoint);
        rc = (int)rb_thread_call_without_gvl(rb_czmq_nogvl_monitor_recv, (void *)&args, RUBY_UBF_IO, 0);
        if (rc == -1 && (zmq_errno() == ETERM || zmq_errno() == ENOTSOCK || zmq_errno() == EINTR)) break;
        if (rc == -1 && (sock->flags & ZMQ_SOCKET_DESTROYED)) break;
        assert (rc != -1);
        memcpy (&event, zmq_msg_data (&args.msg_event), sizeof (event));
        // zmq_msg_data(&msg2), zmq_msg_size(&msg2)

        // copy endpoint into ruby string.
        VALUE endpoint_str = rb_str_new(zmq_msg_data(&args.msg_endpoint), zmq_msg_size(&args.msg_endpoint));
        VALUE method = Qnil;

        switch (event.event) {
        case ZMQ_EVENT_CONNECTED: method = intern_on_connected; break;
        case ZMQ_EVENT_CONNECT_DELAYED: method = intern_on_connect_delayed; break;
        case ZMQ_EVENT_CONNECT_RETRIED: method = intern_on_connect_retried; break;
        case ZMQ_EVENT_LISTENING: method = intern_on_listening; break;
        case ZMQ_EVENT_BIND_FAILED: method = intern_on_bind_failed; break;
        case ZMQ_EVENT_ACCEPTED: method = intern_on_accepted; break;
        case ZMQ_EVENT_ACCEPT_FAILED: method = intern_on_accept_failed; break;
        case ZMQ_EVENT_CLOSE_FAILED: method = intern_on_close_failed; break;
        case ZMQ_EVENT_CLOSED: method = intern_on_closed; break;
        case ZMQ_EVENT_DISCONNECTED: method = intern_on_disconnected; break;
        }

        if (method != Qnil) {
            rb_funcall(sock->monitor_handler, method, 2, endpoint_str, INT2FIX(event.value));
        }

        /* once the socket is marked as destroyed, it appears to be not safe to call receive on it. */
        if (sock->flags & ZMQ_SOCKET_DESTROYED)
        {
            break;
        }

        zmq_msg_close(&args.msg_event);
        zmq_msg_close(&args.msg_endpoint);
    }
    /* leave the socket to be closed when the context is destroyed on the main/other thread.
        This additional thread is likely to be terminated by termination, interrupt or the
        end of the application's normal executing when the context is destroyed.
       */
    /* zmq_close (s); */
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
        return Qtrue;
    } else {
        return Qfalse;
    }
}

void _init_rb_czmq_socket()
{
    rb_cZmqSocket = rb_define_class_under(rb_mZmq, "Socket", rb_cObject);
    rb_cZmqPubSocket = rb_define_class_under(rb_cZmqSocket, "Pub", rb_cZmqSocket);
    rb_cZmqSubSocket = rb_define_class_under(rb_cZmqSocket, "Sub", rb_cZmqSocket);
    rb_cZmqPushSocket = rb_define_class_under(rb_cZmqSocket, "Push", rb_cZmqSocket);
    rb_cZmqPullSocket = rb_define_class_under(rb_cZmqSocket, "Pull", rb_cZmqSocket);
    rb_cZmqRouterSocket = rb_define_class_under(rb_cZmqSocket, "Router", rb_cZmqSocket);
    rb_cZmqDealerSocket = rb_define_class_under(rb_cZmqSocket, "Dealer", rb_cZmqSocket);
    rb_cZmqRepSocket = rb_define_class_under(rb_cZmqSocket, "Rep", rb_cZmqSocket);
    rb_cZmqReqSocket = rb_define_class_under(rb_cZmqSocket, "Req", rb_cZmqSocket);
    rb_cZmqPairSocket = rb_define_class_under(rb_cZmqSocket, "Pair", rb_cZmqSocket);
    rb_cZmqXPubSocket = rb_define_class_under(rb_cZmqSocket, "XPub", rb_cZmqSocket);
    rb_cZmqXSubSocket = rb_define_class_under(rb_cZmqSocket, "XSub", rb_cZmqSocket);
    rb_cZmqStreamSocket = rb_define_class_under(rb_cZmqSocket, "Stream", rb_cZmqSocket);

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
    rb_define_const(rb_cZmqSocket, "DISCONNECTED", INT2NUM(ZMQ_SOCKET_DISCONNECTED));

    rb_define_method(rb_cZmqSocket, "close", rb_czmq_socket_close, 0);
    rb_define_method(rb_cZmqSocket, "endpoints", rb_czmq_socket_endpoints, 0);
    rb_define_method(rb_cZmqSocket, "state", rb_czmq_socket_state, 0);
    rb_define_method(rb_cZmqSocket, "real_bind", rb_czmq_socket_bind, 1);
    rb_define_method(rb_cZmqSocket, "real_connect", rb_czmq_socket_connect, 1);
    rb_define_method(rb_cZmqSocket, "disconnect", rb_czmq_socket_disconnect, 1);
    rb_define_method(rb_cZmqSocket, "unbind", rb_czmq_socket_unbind, 1);
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
    rb_define_method(rb_cZmqSocket, "poll", rb_czmq_socket_poll, 1);

    rb_define_method(rb_cZmqSocket, "sndhwm", rb_czmq_socket_opt_sndhwm, 0);
    rb_define_method(rb_cZmqSocket, "sndhwm=", rb_czmq_socket_set_opt_sndhwm, 1);
    rb_define_method(rb_cZmqSocket, "rcvhwm", rb_czmq_socket_opt_rcvhwm, 0);
    rb_define_method(rb_cZmqSocket, "rcvhwm=", rb_czmq_socket_set_opt_rcvhwm, 1);
    rb_define_method(rb_cZmqSocket, "affinity", rb_czmq_socket_opt_affinity, 0);
    rb_define_method(rb_cZmqSocket, "affinity=", rb_czmq_socket_set_opt_affinity, 1);
    rb_define_method(rb_cZmqSocket, "rate", rb_czmq_socket_opt_rate, 0);
    rb_define_method(rb_cZmqSocket, "rate=", rb_czmq_socket_set_opt_rate, 1);
    rb_define_method(rb_cZmqSocket, "recovery_ivl", rb_czmq_socket_opt_recovery_ivl, 0);
    rb_define_method(rb_cZmqSocket, "recovery_ivl=", rb_czmq_socket_set_opt_recovery_ivl, 1);
    rb_define_method(rb_cZmqSocket, "maxmsgsize", rb_czmq_socket_opt_maxmsgsize, 0);
    rb_define_method(rb_cZmqSocket, "maxmsgsize=", rb_czmq_socket_set_opt_maxmsgsize, 1);
    rb_define_method(rb_cZmqSocket, "multicast_hops", rb_czmq_socket_opt_multicast_hops, 0);
    rb_define_method(rb_cZmqSocket, "multicast_hops=", rb_czmq_socket_set_opt_multicast_hops, 1);
    rb_define_method(rb_cZmqSocket, "ipv4only?", rb_czmq_socket_opt_ipv4only, 0);
    rb_define_method(rb_cZmqSocket, "ipv4only=", rb_czmq_socket_set_opt_ipv4only, 1);
    rb_define_method(rb_cZmqSocket, "delay_attach_on_connect=", rb_czmq_socket_set_opt_delay_attach_on_connect, 1);
    rb_define_method(rb_cZmqSocket, "router_mandatory=", rb_czmq_socket_set_opt_router_mandatory, 1);
    rb_define_method(rb_cZmqSocket, "router_raw=", rb_czmq_socket_set_opt_router_raw, 1);
    rb_define_method(rb_cZmqSocket, "xpub_verbose=", rb_czmq_socket_set_opt_xpub_verbose, 1);
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
    rb_define_method(rb_cZmqSocket, "last_endpoint", rb_czmq_socket_opt_last_endpoint, 0);
    rb_define_method(rb_cZmqStreamSocket, "stream_notify=", rb_czmq_socket_set_opt_stream_notify, 1);
}
