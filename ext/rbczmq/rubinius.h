#ifndef RBCZMQ_RUBINIUS_H
#define RBCZMQ_RUBINIUS_H

#define RSTRING_NOT_MODIFIED

#include <ruby/st.h>
#include <ruby/encoding.h>
#include <ruby/io.h>
extern rb_encoding *binary_encoding;
#define ZmqEncode(str) rb_enc_associate(str, binary_encoding)

#define ZMQ_DEFAULT_SOCKET_TIMEOUT Qnil

#define THREAD_PASS rb_thread_schedule();

#endif
