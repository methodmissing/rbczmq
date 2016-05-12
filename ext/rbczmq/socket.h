#ifndef RBCZMQ_SOCKET_H
#define RBCZMQ_SOCKET_H

#define ZMQ_SOCKET_DESTROYED 0x01

/* Connection states */

#define ZMQ_SOCKET_PENDING 0x01
#define ZMQ_SOCKET_BOUND 0x02
#define ZMQ_SOCKET_CONNECTED 0x04
#define ZMQ_SOCKET_DISCONNECTED 0x08

typedef struct {
    zctx_t *ctx;
    void *socket;
    void *ctx_wrapper; // zmq_ctx_wrapper - can't be defined yet, circular header includes.
    int flags;
    bool verbose;
    int state;
    VALUE endpoints;
    VALUE thread;
    VALUE context;
    VALUE monitor_endpoint;
    VALUE monitor_handler;
    VALUE monitor_thread;
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
    { assertion }; \
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
    long length;
    bool read;
};

struct nogvl_send_frame_args {
    zmq_sock_wrapper *socket;
    zframe_t *frame;
    int flags;
    bool read;
};

struct nogvl_send_message_args {
    zmq_sock_wrapper *socket;
    zmsg_t *message;
    bool read;
};

struct nogvl_recv_args {
    zmq_sock_wrapper *socket;
    zmq_msg_t message;
};

struct nogvl_socket_poll_args {
    zmq_sock_wrapper *socket;
    int timeout;
};

struct nogvl_monitor_recv_args {
    void *socket;
    zmq_msg_t msg_event;
    zmq_msg_t msg_endpoint;
    zmq_sock_wrapper *monitored_socket_wrapper;
};

struct nogvl_conn_args {
    zmq_sock_wrapper *socket;
    char *endpoint;
};

extern VALUE intern_on_connected;
extern VALUE intern_on_connect_delayed;
extern VALUE intern_on_connect_retried;
extern VALUE intern_on_listening;
extern VALUE intern_on_bind_failed;
extern VALUE intern_on_accepted;
extern VALUE intern_on_accept_failed;
extern VALUE intern_on_closed;
extern VALUE intern_on_close_failed;
extern VALUE intern_on_disconnected;

void _init_rb_czmq_socket();
VALUE rb_czmq_nogvl_zsocket_destroy(void *ptr);

#if (ZMQ_VERSION_MAJOR == 4 && ZMQ_VERSION_MINOR >= 1)
typedef struct {
    uint16_t event;  // id of the event as bitfield
    int32_t  value; // value is either error code, fd or reconnect interval
} zmq_event_t;
#endif

#endif
