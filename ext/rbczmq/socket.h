#ifndef RBCZMQ_SOCKET_H
#define RBCZMQ_SOCKET_H

#define ZMQ_SOCKET_PENDING 0
#define ZMQ_SOCKET_BOUND 1
#define ZMQ_SOCKET_CONNECTED 2

typedef struct {
    zctx_t *ctx;
    void *socket;
    int fd;
    Bool verbose;
    int state;
    VALUE handler;
    VALUE endpoint;
} zmq_sock_wrapper;

#define ZmqAssertSocket(obj) ZmqAssertType(obj, rb_cZmqSocket, "ZMQ::Socket")
#define GetZmqSocket(obj) \
    zmq_sock_wrapper *sock = NULL; \
    ZmqAssertSocket(obj); \
    Data_Get_Struct(obj, zmq_sock_wrapper, sock); \
    if (!sock) rb_raise(rb_eTypeError, "uninitialized ZMQ socket!");

#define ZmqDumpFrame(method, frame) \
  do { \
      sprintf(print_prefix, "%sI: %s socket %p: %s", cur_time, zsocket_type_str(sock->socket), sock->socket, method); \
      zframe_print((frame), print_prefix); \
      xfree(cur_time); \
  } while(0)

#define ZmqDumpMessage(method, message) \
  do { \
      zclock_log("I: %s socket %p: %s", zsocket_type_str(sock->socket), sock->socket, method); \
      zmsg_dump((message)); \
  } while(0)

#define AssertSockOptFor(sock_type) \
    if (zsockopt_type(sock->socket) != sock_type) \
        rb_raise(rb_eZmqError, "Socket option not supported on a %s socket!", zsocket_type_str(sock->socket));

#define CheckBoolean(arg) \
   if (TYPE((arg)) != T_TRUE && TYPE((arg)) != T_FALSE) \
       rb_raise(rb_eTypeError, "wrong argument %s (expected true or false)", RSTRING_PTR(rb_obj_as_string((arg))));

int rb_czmq_get_sock_fd(void *sock);
void rb_czmq_free_sock(zmq_sock_wrapper *sock);

void rb_czmq_mark_sock(void *ptr);
void rb_czmq_free_sock_gc(void *ptr);

void _init_rb_czmq_socket();

#endif
