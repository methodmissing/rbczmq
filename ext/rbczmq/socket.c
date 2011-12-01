#include <rbczmq_ext.h>

static VALUE rb_czmq_nogvl_zsocket_destroy(void *ptr)
{
    zmq_sock_wrapper *sock = ptr;
    zsocket_destroy(sock->ctx, sock->socket);
    return Qnil;
}

void rb_czmq_free_sock(zmq_sock_wrapper *sock)
{
    if (sock->ctx) {
        rb_thread_blocking_region(rb_czmq_nogvl_zsocket_destroy, sock, RUBY_UBF_IO, 0);
        if (zmq_errno() == ENOTSOCK) ZmqRaiseSysError();
        sock->socket = NULL;
    }
}

void rb_czmq_mark_sock(void *ptr)
{
    zmq_sock_wrapper *sock = ptr;
    if (sock){
        rb_gc_mark(sock->handler);
        rb_gc_mark(sock->endpoint);
        rb_gc_mark(sock->thread);
        rb_gc_mark(sock->recv_timeout);
        rb_gc_mark(sock->send_timeout);
    }
}

void rb_czmq_free_sock_gc(void *ptr)
{
    zmq_sock_wrapper *sock = ptr;
    if (sock){
        /*if (sock->socket != NULL) rb_czmq_free_sock(sock);*/
        xfree(sock);
    }
}

/*
 *  call-seq:
 *     sock.close   =>  nil
 *
 *  Closes a socket.
 *
 * === Examples
 *     sock.close    =>  nil
 *
*/

static VALUE rb_czmq_socket_close(VALUE obj)
{
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    rb_czmq_free_sock(sock);
    return Qnil;
}

/*
 *  call-seq:
 *     sock.endpoint   =>  String
 *
 *  Closes a socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:PUSH)
 *     sock.endpoint    =>   nil
 *     sock.bind("inproc://test")
 *     sock.endpoint    =>  "inproc://test"
 *
*/

static VALUE rb_czmq_socket_endpoint(VALUE obj)
{
    GetZmqSocket(obj);
    return sock->endpoint;
}

/*
 *  call-seq:
 *     sock.state   =>  String
 *
 *  Closes a socket.
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
    GetZmqSocket(obj);
    return INT2NUM(sock->state);
}

/*
 *  call-seq:
 *     sock.fd   =>  Fixnum
 *
 *  Closes a socket.
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
    GetZmqSocket(obj);
    if (sock->state == ZMQ_SOCKET_PENDING) return INT2NUM(-1);
    return INT2NUM(zsockopt_fd(sock->socket));
}

VALUE rb_czmq_nogvl_socket_bind(void *ptr) {
    int rc;
    struct nogvl_conn_args *args = ptr;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    rc = zsocket_bind(socket->socket, args->endpoint);
#else
    ZmqBlockingWrite(((rc = zsocket_bind(socket->socket, args->endpoint)) == -1), socket);
#endif
    return (VALUE)rc;
}

VALUE rb_czmq_nogvl_socket_connect(void *ptr) {
    int rc;
    struct nogvl_conn_args *args = ptr;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    rc = zsocket_connect(socket->socket, args->endpoint);
#else
    ZmqBlockingWrite(((rc = zsocket_connect(socket->socket, args->endpoint)) == -1), socket);
#endif
    return (VALUE)rc;
}

/*
 *  call-seq:
 *     sock.bind("inproc://test")   =>  Fixnum
 *
 *  Closes a socket.
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
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    Check_Type(endpoint, T_STRING);
    args.socket = sock;
    args.endpoint = StringValueCStr(endpoint);
    rc = (int)rb_thread_blocking_region(rb_czmq_nogvl_socket_bind, (void *)&args, RUBY_UBF_IO, 0);
    if (rc == -1) ZmqRaiseSysError();
    if (sock->verbose)
        zclock_log ("I: %s socket %p: bound \"%s\"", zsocket_type_str(sock->socket), obj, StringValueCStr(endpoint));
    sock->state |= ZMQ_SOCKET_BOUND;
    sock->endpoint = rb_str_dup(endpoint);
    return INT2FIX(rc);
}

/*
 *  call-seq:
 *     sock.connect("tcp://localhost:3456")   =>  boolean
 *
 *  Closes a socket.
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
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    Check_Type(endpoint, T_STRING);
    args.socket = sock;
    args.endpoint = StringValueCStr(endpoint);
    rc = (int)rb_thread_blocking_region(rb_czmq_nogvl_socket_connect, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    if (sock->verbose)
        zclock_log ("I: %s socket %p: connected \"%s\"", zsocket_type_str(sock->socket), obj, StringValueCStr(endpoint));
    sock->state |= ZMQ_SOCKET_CONNECTED;
    sock->endpoint = rb_str_dup(endpoint);
    return Qtrue;
}

/*
 *  call-seq:
 *     sock.verbose = true   =>  nil
 *
 *  Closes a socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.verbose = true    =>  nil
 *
*/

static VALUE rb_czmq_socket_set_verbose(VALUE obj, VALUE level)
{
    Bool vlevel;
    GetZmqSocket(obj);
    vlevel = (level == Qtrue) ? TRUE : FALSE;
    sock->verbose = vlevel;
    return Qnil;
}

static VALUE rb_czmq_nogvl_zstr_send(void *ptr) {
    int rc;
    struct nogvl_send_args *args = ptr;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    rc = zstr_send(socket->socket, args->msg);
#else
    ZmqBlockingWrite(((rc = zstr_send(socket->socket, args->msg)) == -1), socket);
#endif
    return (VALUE)rc;
}

static VALUE rb_czmq_nogvl_zstr_sendm(void *ptr) {
    int rc;
    struct nogvl_send_args *args = ptr;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    rc = zstr_sendm(socket->socket, args->msg);
#else
    ZmqBlockingWrite(((rc = zstr_sendm(socket->socket, args->msg)) == -1), socket);
#endif
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
    GetZmqSocket(obj);
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
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    Check_Type(msg, T_STRING);
    args.socket = sock;
    args.msg = StringValueCStr(msg);
    rc = rb_thread_blocking_region(rb_czmq_nogvl_zstr_sendm, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    if (sock->verbose)
        zclock_log ("I: %s socket %p: sendm \"%s\"", zsocket_type_str(sock->socket), sock->socket, StringValueCStr(msg));
    return Qtrue;
}

static VALUE rb_czmq_nogvl_recv(void *ptr) {
    char *str = NULL;
    struct nogvl_recv_args *args = ptr;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    str = zstr_recv(socket->socket);
#else
    ZmqBlockingRead(((str = zstr_recv(socket->socket)) == NULL), socket);
#endif
    return (VALUE)str;
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
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    args.socket = sock;
    str = (char *)rb_thread_blocking_region(rb_czmq_nogvl_recv, (void *)&args, RUBY_UBF_IO, 0);
    if (str == NULL) return Qnil;
    ZmqAssertSysError();
    if (sock->verbose)
        zclock_log ("I: %s socket %p: recv \"%s\"", zsocket_type_str(sock->socket), sock->socket, str);
    return ZmqEncode(rb_str_new2(str));
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
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    str = zstr_recv_nowait(sock->socket);
    if (str == NULL) return Qnil;
    ZmqAssertSysError();
    if (sock->verbose)
        zclock_log ("I: %s socket %p: recv_nonblock \"%s\"", zsocket_type_str(sock->socket), sock->socket, str);
    return ZmqEncode(rb_str_new2(str));
}

static VALUE rb_czmq_nogvl_send_frame(void *ptr) {
    int rc;
    struct nogvl_send_frame_args *args = ptr;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    rc = zframe_send(&(args->frame), socket->socket, args->flags);
#else
    ZmqBlockingWrite((rc = zframe_send(&(args->frame), socket->socket, args->flags) == -1), socket);
#endif
    return (VALUE)rc;
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
    int rc;
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    rb_scan_args(argc, argv, "11", &frame_obj, &flags);
    ZmqGetFrame(frame_obj);
    if (NIL_P(flags)) flags = INT2FIX(0);
    if (SYMBOL_P(flags)) flags = rb_const_get_at(rb_cZmqFrame, rb_to_id(flags));

   if (sock->verbose) {
        cur_time = rb_czmq_formatted_current_time();
        print_frame = zframe_dup(frame->frame);
    }
    args.socket = sock;
    args.frame = frame->frame;
    args.flags = FIX2INT(flags);
    rc = (int)rb_thread_blocking_region(rb_czmq_nogvl_send_frame, (void *)&args, RUBY_UBF_IO, 0);
    ZmqAssert(rc);
    frame->flags |= ZMQ_FRAME_RECYCLED;
    if (sock->verbose) ZmqDumpFrame("send_frame", print_frame);
    return Qtrue;
}

int wrap_zmsg_send(zmsg_t **m, void *socket) {
    zmsg_send(m, socket);
    return 0;
}

static VALUE rb_czmq_nogvl_send_message(void *ptr) {
    struct nogvl_send_message_args *args = ptr;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    zmsg_send(&(args->message), socket->socket);
#else
    ZmqBlockingWrite((wrap_zmsg_send(&(args->message), socket->socket) != 0), socket);
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
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    ZmqGetMessage(message_obj);
    if (sock->verbose) print_message = zmsg_dup(message->message);
    args.socket = sock;
    args.message = message->message;
    rb_thread_blocking_region(rb_czmq_nogvl_send_message, (void *)&args, RUBY_UBF_IO, 0);
    message->flags |= ZMQ_MESSAGE_RECYCLED;
    if (sock->verbose) ZmqDumpMessage("send_message", print_message);
    return Qnil;
}

static VALUE rb_czmq_nogvl_recv_frame(void *ptr) {
    zframe_t *frame = NULL;
    struct nogvl_recv_args *args = ptr;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    frame = zframe_recv(socket->socket);
#else
    ZmqBlockingRead(((frame = zframe_recv(socket->socket)) == NULL), socket);
#endif
    return (VALUE)frame;
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
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    args.socket = sock;
    frame = (zframe_t *)rb_thread_blocking_region(rb_czmq_nogvl_recv_frame, (void *)&args, RUBY_UBF_IO, 0);
    if (frame == NULL) return Qnil;
    if (sock->verbose) {
        cur_time = rb_czmq_formatted_current_time();
        ZmqDumpFrame("recv_frame", frame);
    }
    return rb_czmq_alloc_frame(&frame, ZMQ_FRAME_ALLOC);
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
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    frame = zframe_recv_nowait(sock->socket);
    if (frame == NULL) return Qnil;
    if (sock->verbose) {
        cur_time = rb_czmq_formatted_current_time();
        ZmqDumpFrame("recv_frame_nonblock", frame);
    }
    return rb_czmq_alloc_frame(&frame, ZMQ_FRAME_ALLOC);
}

static VALUE rb_czmq_nogvl_recv_message(void *ptr) {
    zmsg_t *message = NULL;
    struct nogvl_recv_args *args = ptr;
    zmq_sock_wrapper *socket = args->socket;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    message = zmsg_recv(socket->socket);
#else
    ZmqBlockingRead(((message = zmsg_recv(socket->socket)) == NULL), socket);
#endif
    return (VALUE)message;
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
    GetZmqSocket(obj);
    ZmqSockGuardCrossThread(sock);
    args.socket = sock;
    message = (zmsg_t *)rb_thread_blocking_region(rb_czmq_nogvl_recv_message, (void *)&args, RUBY_UBF_IO, 0);
    if (message == NULL) return Qnil;
    if (sock->verbose) ZmqDumpMessage("recv_message", message);
    return rb_czmq_alloc_message(message);
}

/*
 *  call-seq:
 *     sock.handler = MyFrameHandler =>  nil
 *
 *  Associates a callback handler with this socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.handler = MyFrameHandler => nil
 *
*/

static VALUE rb_czmq_socket_set_handler(VALUE obj, VALUE handler)
{
    GetZmqSocket(obj);
    sock->handler = handler;
    return Qnil;
}

/*
 *  call-seq:
 *     sock.handler =>  Object or nil
 *
 *  Returns the callback handler currently associated with this socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.handler = MyFrameHandler
 *     sock.handler   =>   MyFrameHandler
 *
*/

static VALUE rb_czmq_socket_handler(VALUE obj)
{
    GetZmqSocket(obj);
    return sock->handler;
}

/*
 *  call-seq:
 *     sock.recv_timeout = 5 =>  nil
 *
 *  Sets a receive timeout for this socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.recv_timeout = 5 => nil
 *
*/

static VALUE rb_czmq_socket_set_recv_timeout(VALUE obj, VALUE timeout)
{
    GetZmqSocket(obj);
    if (TYPE(timeout) != T_FIXNUM && TYPE(timeout) != T_FLOAT) rb_raise(rb_eTypeError, "wrong timeout type %s (expected Fixnum or Float)", RSTRING_PTR(rb_obj_as_string(timeout)));
    sock->recv_timeout = timeout;
    return Qnil;
}

/*
 *  call-seq:
 *     sock.recv_timeout =>  Fixnum or nil
 *
 *  Returns the recv timeout currently associated with this socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.recv_timeout = 5
 *     sock.recv_timeout   =>   5
 *
*/

static VALUE rb_czmq_socket_recv_timeout(VALUE obj)
{
    GetZmqSocket(obj);
    return sock->recv_timeout;
}

/*
 *  call-seq:
 *     sock.send_timeout = 5 =>  nil
 *
 *  Sets a send timeout for this socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.send_timeout = 5 => nil
 *
*/

static VALUE rb_czmq_socket_set_send_timeout(VALUE obj, VALUE timeout)
{
    GetZmqSocket(obj);
    if (TYPE(timeout) != T_FIXNUM && TYPE(timeout) != T_FLOAT) rb_raise(rb_eTypeError, "wrong timeout type %s (expected Fixnum or Float)", RSTRING_PTR(rb_obj_as_string(timeout)));
    sock->send_timeout = timeout;
    return Qnil;
}

/*
 *  call-seq:
 *     sock.send_timeout =>  Fixnum or nil
 *
 *  Returns the send timeout currently associated with this socket.
 *
 * === Examples
 *     ctx = ZMQ::Context.new
 *     sock = ctx.socket(:REP)
 *     sock.send_timeout = 5
 *     sock.send_timeout   =>   5
 *
*/

static VALUE rb_czmq_socket_send_timeout(VALUE obj)
{
    GetZmqSocket(obj);
    return sock->send_timeout;
}

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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_hwm(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_hwm, "HWM", value);
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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_swap(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_swap, "SWAP", value);
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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_affinity(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_affinity, "AFFINITY", value);
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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_rate(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_rate, "RATE", value);
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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_recovery_ivl(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_recovery_ivl, "RECOVERY_IVL", value);
}

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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_recovery_ivl_msec(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_recovery_ivl_msec, "RECOVERY_IVL_MSEC", value);
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
    GetZmqSocket(obj);
    return (zsockopt_mcast_loop(sock->socket) == 1) ? Qtrue : Qfalse;
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
    ZmqSetBooleanSockOpt(obj, zsockopt_set_mcast_loop, "MCAST_LOOP", value);
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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_sndbuf(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_sndbuf, "SNDBUF", value);
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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_rcvbuf(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_rcvbuf, "RCVBUF", value);
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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_linger(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_linger, "LINGER", value);
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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_backlog(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_backlog, "BACKLOG", value);
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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_reconnect_ivl(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_reconnect_ivl, "RECONNECT_IVL", value);
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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_reconnect_ivl_max(sock->socket));
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
    ZmqSetSockOpt(obj, zsockopt_set_reconnect_ivl_max, "RECONNECT_IVL_MAX", value);
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
    ZmqSetStringSockOpt(obj, zsockopt_set_identity, "IDENTITY", value, "");
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
    ZmqSetStringSockOpt(obj, zsockopt_set_subscribe, "SUBSCRIBE", value, {
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
    ZmqSetStringSockOpt(obj, zsockopt_set_unsubscribe, "UNSUBSCRIBE", value, {
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
    GetZmqSocket(obj);
    return (zsockopt_rcvmore(sock->socket) == 1) ? Qtrue : Qfalse;
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
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_events(sock->socket));
}

void _init_rb_czmq_socket() {
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

    rb_define_const(rb_cZmqSocket, "PENDING", INT2NUM(ZMQ_SOCKET_PENDING));
    rb_define_const(rb_cZmqSocket, "BOUND", INT2NUM(ZMQ_SOCKET_BOUND));
    rb_define_const(rb_cZmqSocket, "CONNECTED", INT2NUM(ZMQ_SOCKET_CONNECTED));

    rb_define_method(rb_cZmqSocket, "close", rb_czmq_socket_close, 0);
    rb_define_method(rb_cZmqSocket, "endpoint", rb_czmq_socket_endpoint, 0);
    rb_define_method(rb_cZmqSocket, "state", rb_czmq_socket_state, 0);
    rb_define_method(rb_cZmqSocket, "bind", rb_czmq_socket_bind, 1);
    rb_define_method(rb_cZmqSocket, "connect", rb_czmq_socket_connect, 1);
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
    rb_define_method(rb_cZmqSocket, "handler=", rb_czmq_socket_set_handler, 1);
    rb_define_method(rb_cZmqSocket, "handler", rb_czmq_socket_handler, 0);
    rb_define_method(rb_cZmqSocket, "recv_timeout=", rb_czmq_socket_set_recv_timeout, 1);
    rb_define_method(rb_cZmqSocket, "recv_timeout", rb_czmq_socket_recv_timeout, 0);
    rb_define_method(rb_cZmqSocket, "send_timeout=", rb_czmq_socket_set_send_timeout, 1);
    rb_define_method(rb_cZmqSocket, "send_timeout", rb_czmq_socket_send_timeout, 0);

    rb_define_method(rb_cZmqSocket, "hwm", rb_czmq_socket_opt_hwm, 0);
    rb_define_method(rb_cZmqSocket, "hwm=", rb_czmq_socket_set_opt_hwm, 1);
    rb_define_method(rb_cZmqSocket, "swap", rb_czmq_socket_opt_swap, 0);
    rb_define_method(rb_cZmqSocket, "swap=", rb_czmq_socket_set_opt_swap, 1);
    rb_define_method(rb_cZmqSocket, "affinity", rb_czmq_socket_opt_affinity, 0);
    rb_define_method(rb_cZmqSocket, "affinity=", rb_czmq_socket_set_opt_affinity, 1);
    rb_define_method(rb_cZmqSocket, "rate", rb_czmq_socket_opt_rate, 0);
    rb_define_method(rb_cZmqSocket, "rate=", rb_czmq_socket_set_opt_rate, 1);
    rb_define_method(rb_cZmqSocket, "recovery_ivl", rb_czmq_socket_opt_recovery_ivl, 0);
    rb_define_method(rb_cZmqSocket, "recovery_ivl=", rb_czmq_socket_set_opt_recovery_ivl, 1);
    rb_define_method(rb_cZmqSocket, "recovery_ivl_msec", rb_czmq_socket_opt_recovery_ivl_msec, 0);
    rb_define_method(rb_cZmqSocket, "recovery_ivl_msec=", rb_czmq_socket_set_opt_recovery_ivl_msec, 1);
    rb_define_method(rb_cZmqSocket, "mcast_loop?", rb_czmq_socket_opt_mcast_loop, 0);
    rb_define_method(rb_cZmqSocket, "mcast_loop=", rb_czmq_socket_set_opt_mcast_loop, 1);
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
}
