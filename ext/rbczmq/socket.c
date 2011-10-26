#include <rbczmq_ext.h>

int rb_czmq_get_sock_fd(void *sock)
{
    int fd;
#ifdef ZMQ_FD
    fd = zsockopt_fd(sock);
#else
    fd = -1;
#endif
    return fd;
}

void rb_czmq_free_sock(zmq_sock_wrapper *sock)
{
    if (sock->ctx) {
        zsocket_destroy(sock->ctx, sock->socket);
        sock->socket = NULL;
    }
}

void rb_czmq_mark_sock(void *ptr)
{
    zmq_sock_wrapper *sock = ptr;
    if (sock){
        rb_gc_mark(sock->handler);
        rb_gc_mark(sock->endpoint);
    }
}

void rb_czmq_free_sock_gc(void *ptr)
{
    zmq_sock_wrapper *sock = ptr;
    if (sock){
        /*rb_czmq_free_sock(sock);*/
        ZmqDebugf("rb_czmq_free_sock_gc %p, %p", sock, sock->socket);
        xfree(sock);
    }
}

static VALUE rb_czmq_socket_close(VALUE obj)
{
    GetZmqSocket(obj);
    rb_czmq_free_sock(sock);
    return Qnil;
}

static VALUE rb_czmq_socket_type(VALUE obj)
{
    GetZmqSocket(obj);
    return rb_const_get_at(rb_mZmq, rb_intern(zsocket_type_str(sock->socket)));
}

static VALUE rb_czmq_socket_type_str(VALUE obj)
{
    GetZmqSocket(obj);
    return rb_str_new2(zsocket_type_str(sock->socket));
}

static VALUE rb_czmq_socket_endpoint(VALUE obj)
{
    GetZmqSocket(obj);
    return sock->endpoint;
}

static VALUE rb_czmq_socket_state(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2NUM(sock->state);
}

static VALUE rb_czmq_socket_fd(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2NUM(sock->fd);
}

VALUE rb_czmq_nogvl_socket_bind(void *ptr) {
    int rc;
    struct nogvl_conn_args *args = ptr;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    rc = zsocket_bind(args->socket, args->endpoint);
#else
    ZmqBlockingWrite(rc = zsocket_bind(args->socket, args->endpoint), args->fd);
#endif
    return (VALUE)rc;
}

VALUE rb_czmq_nogvl_socket_connect(void *ptr) {
    int rc;
    struct nogvl_conn_args *args = ptr;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    rc = zsocket_connect(args->socket, args->endpoint);
#else
    ZmqBlockingWrite(rc = zsocket_connect(args->socket, args->endpoint), args->fd);
#endif
    return (rc == -1) ? Qfalse : Qtrue;
}

static VALUE rb_czmq_socket_bind(VALUE obj, VALUE endpoint)
{
    struct nogvl_conn_args args;
    int rc;
    GetZmqSocket(obj);
    Check_Type(endpoint, T_STRING);
    args.socket = sock->socket;
    args.endpoint = StringValueCStr(endpoint);
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    args.fd = sock->fd;
#endif
    rc = (int)rb_thread_blocking_region(rb_czmq_nogvl_socket_bind, (void *)&args, RUBY_UBF_IO, 0);
    if (rc == -1) ZmqRaiseError;
    if (sock->verbose)
        zclock_log ("I: %s socket %p: bound \"%s\"", zsocket_type_str(sock->socket), obj, StringValueCStr(endpoint));
    sock->state = ZMQ_SOCKET_BOUND;
    sock->endpoint = rb_str_dup(endpoint);
    return INT2FIX(rc);
}

static VALUE rb_czmq_socket_connect(VALUE obj, VALUE endpoint)
{
    struct nogvl_conn_args args;
    GetZmqSocket(obj);
    Check_Type(endpoint, T_STRING);
    args.socket = sock->socket;
    args.endpoint = StringValueCStr(endpoint);
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    args.fd = sock->fd;
#endif
    if (rb_thread_blocking_region(rb_czmq_nogvl_socket_connect, (void *)&args, RUBY_UBF_IO, 0) == Qfalse) ZmqRaiseError;
    if (sock->verbose)
        zclock_log ("I: %s socket %p: connected \"%s\"", zsocket_type_str(sock->socket), obj, StringValueCStr(endpoint));
    sock->state = ZMQ_SOCKET_CONNECTED;
    sock->endpoint = rb_str_dup(endpoint);
    return Qtrue;
}

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
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    rc = zstr_send(args->socket, args->msg);
#else
    ZmqBlockingWrite(rc = zstr_send(args->socket, args->msg), args->fd);
#endif
    return (rc == -1) ? Qfalse : Qtrue;
}

static VALUE rb_czmq_nogvl_zstr_sendm(void *ptr) {
    int rc;
    struct nogvl_send_args *args = ptr;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    rc = zstr_sendm(args->socket, args->msg);
#else
    ZmqBlockingWrite(rc = zstr_sendm(args->socket, args->msg), args->fd);
#endif
    return (rc == -1) ? Qfalse : Qtrue;
}

static VALUE rb_czmq_socket_send(VALUE obj, VALUE msg)
{
    struct nogvl_send_args args;
    GetZmqSocket(obj);
    Check_Type(msg, T_STRING);
    args.socket = sock->socket;
    args.msg = StringValueCStr(msg);
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    args.fd = sock->fd;
#endif
    if (rb_thread_blocking_region(rb_czmq_nogvl_zstr_send, (void *)&args, RUBY_UBF_IO, 0) == Qfalse) ZmqRaiseError;
    if (sock->verbose)
        zclock_log ("I: %s socket %p: send \"%s\"", zsocket_type_str(sock->socket), obj, StringValueCStr(msg));
    return Qtrue;
}

static VALUE rb_czmq_socket_sendm(VALUE obj, VALUE msg)
{
    struct nogvl_send_args args;
    GetZmqSocket(obj);
    Check_Type(msg, T_STRING);
    args.socket = sock->socket;
    args.msg = StringValueCStr(msg);
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    args.fd = sock->fd;
#endif
    if (rb_thread_blocking_region(rb_czmq_nogvl_zstr_sendm, (void *)&args, RUBY_UBF_IO, 0) == Qfalse) ZmqRaiseError;
    if (sock->verbose)
        zclock_log ("I: %s socket %p: sendm \"%s\"", zsocket_type_str(sock->socket), sock->socket, StringValueCStr(msg));
    return Qtrue;
}

static VALUE rb_czmq_nogvl_recv(void *ptr) {
    char *str = NULL;
    struct nogvl_recv_args *args = ptr;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    str = zstr_recv(args->socket);
#else
    int rc = 0;
    ZmqBlockingRead(str = zstr_recv(args->socket), args->fd);
#endif
    return (VALUE)str;
}

static VALUE rb_czmq_socket_recv(VALUE obj)
{
    char *str = NULL;
    struct nogvl_recv_args args;
    int rc;
    GetZmqSocket(obj);
    rc = 0;
    args.socket = sock->socket;
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    args.fd = sock->fd;
#endif
    str = (char *)rb_thread_blocking_region(rb_czmq_nogvl_recv, (void *)&args, RUBY_UBF_IO, 0);
    if (str == NULL) return Qnil;
    if (sock->verbose)
        zclock_log ("I: %s socket %p: recv \"%s\"", zsocket_type_str(sock->socket), sock->socket, str);
    return ZmqEncode(rb_str_new2(str));
}

static VALUE rb_czmq_socket_recv_nonblock(VALUE obj)
{
    char *str = NULL;
    GetZmqSocket(obj);
    str = zstr_recv_nowait(sock->socket);
    if (str == NULL) return Qnil;
    if (sock->verbose)
        zclock_log ("I: %s socket %p: recv_nonblock \"%s\"", zsocket_type_str(sock->socket), sock->socket, str);
    return ZmqEncode(rb_str_new2(str));
}

static VALUE rb_czmq_nogvl_send_frame(void *ptr) {
    struct nogvl_send_frame_args *args = ptr;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    zframe_send(&(args->frame), args->socket, args->flags);
#else
    int rc = 0;
    ZmqBlockingWrite(zframe_send(&(args->frame), args->socket, args->flags), args->fd);
#endif
    return Qnil;
}

static VALUE rb_czmq_socket_send_frame(int argc, VALUE *argv, VALUE obj)
{
    struct nogvl_send_frame_args args;
    VALUE frame_obj;
    VALUE flags;
    char print_prefix[255];
    char *cur_time = NULL;
    zframe_t *print_frame = NULL;
    GetZmqSocket(obj);
    rb_scan_args(argc, argv, "11", &frame_obj, &flags);
    ZmqGetFrame(frame_obj);
    if (NIL_P(flags)) flags = INT2FIX(0);
    if (SYMBOL_P(flags)) flags = rb_const_get_at(rb_cZmqFrame, rb_to_id(flags));
    if (sock->verbose) {
        cur_time = rb_czmq_formatted_current_time();
        print_frame = zframe_dup(frame->frame);
    }
    args.socket = sock->socket;
    args.frame = frame->frame;
    args.flags = FIX2INT(flags);
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    args.fd = sock->fd;
#endif
    rb_thread_blocking_region(rb_czmq_nogvl_send_frame, (void *)&args, RUBY_UBF_IO, 0);
    if (sock->verbose) ZmqDumpFrame("send_frame", print_frame);
    return Qnil;
}

static VALUE rb_czmq_nogvl_send_message(void *ptr) {
    struct nogvl_send_message_args *args = ptr;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    zmsg_send(&(args->message), args->socket);
#else
    int rc = 0;
    ZmqBlockingWrite(zmsg_send(&(args->message), args->socket), args->fd);
#endif
    return Qnil;
}

static VALUE rb_czmq_socket_send_message(VALUE obj, VALUE message_obj)
{
    struct nogvl_send_message_args args;
    zmsg_t *print_message = NULL;
    GetZmqSocket(obj);
    ZmqGetMessage(message_obj);
    if (sock->verbose) print_message = zmsg_dup(message->message);
    args.socket = sock->socket;
    args.message = message->message;
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    args.fd = sock->fd;
#endif
    rb_thread_blocking_region(rb_czmq_nogvl_send_message, (void *)&args, RUBY_UBF_IO, 0);
    if (sock->verbose) ZmqDumpMessage("send_message", print_message);
    return Qnil;
}

static VALUE rb_czmq_nogvl_recv_frame(void *ptr) {
    zframe_t *frame = NULL;
    struct nogvl_recv_args *args = ptr;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    frame = zframe_recv(args->socket);
#else
    int rc = 0;
    ZmqBlockingRead(frame = zframe_recv(args->socket), args->fd);
#endif
    return (VALUE)frame;
}

static VALUE rb_czmq_socket_recv_frame(VALUE obj)
{
    int rc;
    zframe_t *frame = NULL;
    struct nogvl_recv_args args;
    char print_prefix[255];
    char *cur_time;
    GetZmqSocket(obj);
    rc = 0;
    args.socket = sock->socket;
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    args.fd = sock->fd;
#endif
    frame = (zframe_t *)rb_thread_blocking_region(rb_czmq_nogvl_recv_frame, (void *)&args, RUBY_UBF_IO, 0);
    if (frame == NULL) return Qnil;
    if (sock->verbose) {
        cur_time = rb_czmq_formatted_current_time();
        ZmqDumpFrame("recv_frame", frame);
    }
    return rb_czmq_alloc_frame(frame);
}

static VALUE rb_czmq_socket_recv_frame_nonblock(VALUE obj)
{
    zframe_t *frame = NULL;
    char print_prefix[255];
    char *cur_time;
    GetZmqSocket(obj);
    frame = zframe_recv_nowait(sock->socket);
    if (frame == NULL) return Qnil;
    if (sock->verbose) {
        cur_time = rb_czmq_formatted_current_time();
        ZmqDumpFrame("recv_frame_nonblock", frame);
    }
    return rb_czmq_alloc_frame(frame);
}

static VALUE rb_czmq_nogvl_recv_message(void *ptr) {
    zmsg_t *message = NULL;
    struct nogvl_recv_args *args = ptr;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    message = zmsg_recv(args->socket);
#else
    int rc = 0;
    ZmqBlockingRead(message = zmsg_recv(args->socket), args->fd);
#endif
    return (VALUE)message;
}

static VALUE rb_czmq_socket_recv_message(VALUE obj)
{
    int rc;
    zmsg_t *message = NULL;
    struct nogvl_recv_args args;
    GetZmqSocket(obj);
    rc = 0;
    args.socket = sock->socket;
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    args.fd = sock->fd;
#endif
    message = (zmsg_t *)rb_thread_blocking_region(rb_czmq_nogvl_recv_message, (void *)&args, RUBY_UBF_IO, 0);
    if (message == NULL) return Qnil;
    if (sock->verbose) ZmqDumpMessage("recv_message", message);
    return rb_czmq_alloc_message(message);
}

static VALUE rb_czmq_socket_set_handler(VALUE obj, VALUE handler)
{
    GetZmqSocket(obj);
    sock->handler = handler;
    return Qnil;
}

static VALUE rb_czmq_socket_handler(VALUE obj)
{
    GetZmqSocket(obj);
    return sock->handler;
}

static VALUE rb_czmq_socket_opt_hwm(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_hwm(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_hwm(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_hwm, "HWM", value);
}

static VALUE rb_czmq_socket_opt_swap(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_swap(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_swap(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_swap, "SWAP", value);
}

static VALUE rb_czmq_socket_opt_affinity(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_affinity(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_affinity(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_affinity, "AFFINITY", value);
}

static VALUE rb_czmq_socket_opt_rate(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_rate(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_rate(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_rate, "RATE", value);
}

static VALUE rb_czmq_socket_opt_recovery_ivl(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_recovery_ivl(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_recovery_ivl(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_recovery_ivl, "RECOVERY_IVL", value);
}

static VALUE rb_czmq_socket_opt_recovery_ivl_msec(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_recovery_ivl_msec(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_recovery_ivl_msec(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_recovery_ivl_msec, "RECOVERY_IVL_MSEC", value);
}

static VALUE rb_czmq_socket_opt_mcast_loop(VALUE obj)
{
    GetZmqSocket(obj);
    return (zsockopt_mcast_loop(sock->socket) == 1) ? Qtrue : Qfalse;
}

static VALUE rb_czmq_socket_set_opt_mcast_loop(VALUE obj, VALUE value)
{
    ZmqSetBooleanSockOpt(obj, zsockopt_set_mcast_loop, "MCAST_LOOP", value);
}

static VALUE rb_czmq_socket_opt_sndbuf(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_sndbuf(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_sndbuf(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_sndbuf, "SNDBUF", value);
}

static VALUE rb_czmq_socket_opt_rcvbuf(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_rcvbuf(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_rcvbuf(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_rcvbuf, "RCVBUF", value);
}

static VALUE rb_czmq_socket_opt_linger(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_linger(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_linger(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_linger, "LINGER", value);
}

static VALUE rb_czmq_socket_opt_backlog(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_backlog(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_backlog(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_backlog, "BACKLOG", value);
}

static VALUE rb_czmq_socket_opt_reconnect_ivl(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_reconnect_ivl(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_reconnect_ivl(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_reconnect_ivl, "RECONNECT_IVL", value);
}

static VALUE rb_czmq_socket_opt_reconnect_ivl_max(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_reconnect_ivl_max(sock->socket));
}

static VALUE rb_czmq_socket_set_opt_reconnect_ivl_max(VALUE obj, VALUE value)
{
    ZmqSetSockOpt(obj, zsockopt_set_reconnect_ivl_max, "RECONNECT_IVL_MAX", value);
}

static VALUE rb_czmq_socket_set_opt_identity(VALUE obj, VALUE value)
{
    ZmqSetStringSockOpt(obj, zsockopt_set_identity, "IDENTITY", value, "");
}

static VALUE rb_czmq_socket_set_opt_subscribe(VALUE obj, VALUE value)
{
    ZmqSetStringSockOpt(obj, zsockopt_set_subscribe, "SUBSCRIBE", value, {
       ZmqAssertSockOptFor(ZMQ_SUB)
    });
}

static VALUE rb_czmq_socket_set_opt_unsubscribe(VALUE obj, VALUE value)
{
    ZmqSetStringSockOpt(obj, zsockopt_set_unsubscribe, "UNSUBSCRIBE", value, {
       ZmqAssertSockOptFor(ZMQ_SUB)
    });
}

static VALUE rb_czmq_socket_opt_rcvmore(VALUE obj)
{
    GetZmqSocket(obj);
    return (zsockopt_rcvmore(sock->socket) == 1) ? Qtrue : Qfalse;
}

static VALUE rb_czmq_socket_opt_events(VALUE obj)
{
    GetZmqSocket(obj);
    return INT2FIX(zsockopt_events(sock->socket));
}

void _init_rb_czmq_socket() {
    rb_cZmqSocket = rb_define_class_under(rb_mZmq, "Socket", rb_cObject);

    rb_define_const(rb_cZmqSocket, "PENDING", INT2NUM(ZMQ_SOCKET_PENDING));
    rb_define_const(rb_cZmqSocket, "BOUND", INT2NUM(ZMQ_SOCKET_BOUND));
    rb_define_const(rb_cZmqSocket, "CONNECTED", INT2NUM(ZMQ_SOCKET_CONNECTED));

    rb_define_method(rb_cZmqSocket, "close", rb_czmq_socket_close, 0);
    rb_define_method(rb_cZmqSocket, "type", rb_czmq_socket_type, 0);
    rb_define_method(rb_cZmqSocket, "type_str", rb_czmq_socket_type_str, 0);
    rb_define_method(rb_cZmqSocket, "endpoint", rb_czmq_socket_endpoint, 0);
    rb_define_method(rb_cZmqSocket, "state", rb_czmq_socket_state, 0);
    rb_define_method(rb_cZmqSocket, "bind", rb_czmq_socket_bind, 1);
    rb_define_method(rb_cZmqSocket, "connect", rb_czmq_socket_connect, 1);
    rb_define_method(rb_cZmqSocket, "fd", rb_czmq_socket_fd, 0);
    rb_define_alias(rb_cZmqSocket, "to_i", "fd");
    rb_define_method(rb_cZmqSocket, "fd", rb_czmq_socket_fd, 0);
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
