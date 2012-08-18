#ifndef RBCZMQ_SOCKET_H
#define RBCZMQ_SOCKET_H

#define ZMQ_SOCKET_DESTROYED 0x01

/* Connection states */

#define ZMQ_SOCKET_PENDING 0x01
#define ZMQ_SOCKET_BOUND 0x02
#define ZMQ_SOCKET_CONNECTED 0x04

typedef struct {
    zctx_t *ctx;
    void *socket;
    int flags;
    Bool verbose;
    int state;
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    zlist_t *str_buffer;
    zlist_t *frame_buffer;
    zlist_t *msg_buffer;
#endif
    VALUE endpoints;
    VALUE thread;
} zmq_sock_wrapper;

#define ZmqAssertSocket(obj) ZmqAssertType(obj, rb_cZmqSocket, "ZMQ::Socket")
#define GetZmqSocket(obj) \
    ZmqAssertSocket(obj); \
    Data_Get_Struct(obj, zmq_sock_wrapper, sock); \
    if (!sock) rb_raise(rb_eTypeError, "uninitialized ZMQ socket!"); \
    if (sock->flags & ZMQ_SOCKET_DESTROYED) rb_raise(rb_eZmqError, "ZMQ::Socket instance %p has been destroyed by the ZMQ framework", (void *)obj);

#define ZmqDumpFrame(method, frame) \
  do { \
      sprintf(print_prefix, "%sI: %s socket %p: %s", cur_time, zsocket_type_str(sock->socket), (void *)sock->socket, method); \
      zframe_print((frame), print_prefix); \
      xfree(cur_time); \
  } while(0)

#define ZmqDumpMessage(method, message) \
  do { \
      zclock_log("I: %s socket %p: %s", zsocket_type_str(sock->socket), (void *)sock->socket, method); \
      zmsg_dump((message)); \
  } while(0)

#define ZmqSockGuardCrossThread(sock) \
  if ((sock)->thread != rb_thread_current()) \
      rb_raise(rb_eZmqError, "Cross thread violation for %s socket %p: created in thread %p, invoked on thread %p", zsocket_type_str((sock)->socket), (void *)(sock), (void *)(sock)->thread, (void *)rb_thread_current());

#define ZmqAssertSockOptFor(sock_type) \
    if (zsocket_type(sock->socket) != sock_type) \
        rb_raise(rb_eZmqError, "Socket option not supported on a %s socket!", zsocket_type_str(sock->socket));

#define CheckBoolean(arg) \
    if (TYPE((arg)) != T_TRUE && TYPE((arg)) != T_FALSE) \
        rb_raise(rb_eTypeError, "wrong argument %s (expected true or false)", RSTRING_PTR(rb_obj_as_string((arg))));

#define ZmqSetSockOpt(obj, opt, desc, value) \
    int val; \
    GetZmqSocket(obj); \
    ZmqSockGuardCrossThread(sock); \
    Check_Type(value, T_FIXNUM); \
    val = FIX2INT(value); \
    (opt)(sock->socket, val); \
    if (sock->verbose) \
        zclock_log ("I: %s socket %p: set option \"%s\" %d", zsocket_type_str(sock->socket), (void *)obj, (desc),  val); \
    return Qnil;

#define ZmqSetStringSockOpt(obj, opt, desc, value, assertion) \
    char *val; \
    GetZmqSocket(obj); \
    ZmqSockGuardCrossThread(sock); \
    Check_Type(value, T_STRING); \
    (assertion); \
    val = StringValueCStr(value); \
    (opt)(sock->socket, val); \
    if (sock->verbose) \
        zclock_log ("I: %s socket %p: set option \"%s\" \"%s\"", zsocket_type_str(sock->socket), (void *)obj, (desc),  val); \
    return Qnil;

#define ZmqSetBooleanSockOpt(obj, opt, desc, value) \
    int val; \
    GetZmqSocket(obj); \
    ZmqSockGuardCrossThread(sock); \
    CheckBoolean(value); \
    val = (value == Qtrue) ? 1 : 0; \
    (opt)(sock->socket, val); \
    if (sock->verbose) \
        zclock_log ("I: %s socket %p: set option \"%s\" %d", zsocket_type_str(sock->socket), (void *)obj, (desc),  val); \
    return Qnil;

#define ZmqAssertSocketNotPending(sock, msg) \
    if (!((sock)->state & (ZMQ_SOCKET_BOUND | ZMQ_SOCKET_CONNECTED))) \
        rb_raise(rb_eZmqError, msg);

void rb_czmq_free_sock(zmq_sock_wrapper *sock);

void rb_czmq_mark_sock(void *ptr);
void rb_czmq_free_sock_gc(void *ptr);

struct nogvl_send_args {
    zmq_sock_wrapper *socket;
    const char *msg;
    Bool read;
};

struct nogvl_send_frame_args {
    zmq_sock_wrapper *socket;
    zframe_t *frame;
    int flags;
    Bool read;
};

struct nogvl_send_message_args {
    zmq_sock_wrapper *socket;
    zmsg_t *message;
    Bool read;
};

struct nogvl_recv_args {
    zmq_sock_wrapper *socket;
};

struct nogvl_conn_args {
    zmq_sock_wrapper *socket;
    char *endpoint;
};

void _init_rb_czmq_socket();

#endif
