#ifndef RBCZMQ_RUBINIUS_H
#define RBCZMQ_RUBINIUS_H

#define RSTRING_NOT_MODIFIED

#ifdef HAVE_RUBY_ENCODING_H
#include <ruby/st.h>
#include <ruby/encoding.h>
#include <ruby/io.h>
extern rb_encoding *binary_encoding;
#define ZmqEncode(str) rb_enc_associate(str, binary_encoding)
#else
#include "st.h"
#define ZmqEncode(str) str
#endif

#define TRAP_BEG
#define TRAP_END

#define ZMQ_DEFAULT_SOCKET_TIMEOUT Qnil

#define THREAD_PASS rb_thread_schedule();

#endif
