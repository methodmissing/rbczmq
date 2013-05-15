#ifndef RBCZMQ_RUBY19_H
#define RBCZMQ_RUBY19_H

#include <ruby/encoding.h>
#include <ruby/io.h>
extern rb_encoding *binary_encoding;
#define ZmqEncode(str) rb_enc_associate(str, binary_encoding)
#ifndef THREAD_PASS
#define THREAD_PASS rb_thread_schedule();
#endif

#endif
