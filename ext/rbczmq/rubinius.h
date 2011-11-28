#ifndef RBCZMQ_RUBINIUS_H
#define RBCZMQ_RUBINIUS_H

#define RSTRING_NOT_MODIFIED

/* XXX */
#define ZmqEncode(str) str

#define TRAP_BEG
#define TRAP_END

#define ZMQ_DEFAULT_SOCKET_TIMEOUT Qnil

#define THREAD_PASS rb_thread_schedule();

#endif
