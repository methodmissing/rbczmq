#ifndef RBCZMQ_RUBY18_H
#define RBCZMQ_RUBY18_H

#define THREAD_PASS rb_thread_schedule();
#define ZmqEncode(str) str
#include "rubyio.h"
#include "rubysig.h"
#ifndef RSTRING_PTR
#define RSTRING_PTR(str) RSTRING(str)->ptr
#endif
#ifndef RSTRING_LEN
#define RSTRING_LEN(s) (RSTRING(s)->len)
#endif

/*
 * partial emulation of the 1.9 rb_thread_blocking_region under 1.8,
 * this is enough for dealing with blocking I/O functions in the
 * presence of threads.
 */

#define RUBY_UBF_IO ((rb_unblock_function_t *)-1)
typedef void rb_unblock_function_t(void *);
typedef VALUE rb_blocking_function_t(void *);
static VALUE
rb_thread_blocking_region(
  rb_blocking_function_t *func, void *data1,
  ZMQ_UNUSED rb_unblock_function_t *ubf,
  ZMQ_UNUSED void *data2)
{
  VALUE rv;
  TRAP_BEG;
  rv = func(data1);
  TRAP_END;
  return rv;
}

struct timeval rb_time_interval _((VALUE));

extern VALUE rb_eZmqError;

#ifdef ZMQ_FD
#define ZmqBlockingRead(fcall, sock) \
    retry_read: \
      if (!rb_thread_alone() && !(zsockopt_events((sock)->socket) & ZMQ_POLLIN)) \
          rb_thread_wait_fd(zsockopt_fd((sock)->socket)); \
      if ((fcall)) { \
          if (rb_io_wait_readable(zsockopt_fd((sock)->socket)) == Qtrue) { \
              goto retry_read; \
          } else { \
              ZmqRaiseSysError(); \
          } \
      }
#define ZmqBlockingWrite(fcall, sock) \
    retry_write: \
      if (!rb_thread_alone() && !(zsockopt_events((sock)->socket) & ZMQ_POLLOUT)) \
          rb_thread_fd_writable(zsockopt_fd((sock)->socket)); \
      if ((fcall)) { \
          if (rb_io_wait_writable(zsockopt_fd((sock)->socket)) == Qtrue) { \
              goto retry_write; \
          } else { \
              ZmqRaiseSysError(); \
          } \
      }
#else
#define ZmqBlockingRead(fcall, socket) fcall
#define ZmqBlockingWrite(fcall, socket) fcall
#endif

#define rb_errinfo() ruby_errinfo

#endif
