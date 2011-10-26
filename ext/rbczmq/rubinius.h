#ifndef RBCZMQ_RUBINIUS_H
#define RBCZMQ_RUBINIUS_H

#define RSTRING_NOT_MODIFIED

/* XXX */
#define ZmqEncode(str) str

#define TRAP_BEG
#define TRAP_END

#define THREAD_PASS rb_thread_schedule();

#define ZmqBlockingRead(fcall, fd) fcall;
#define ZmqBlockingWrite(fcall, fd) fcall;

#endif
