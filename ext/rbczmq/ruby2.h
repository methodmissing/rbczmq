#ifndef RBCZMQ_RUBY2_H
#define RBCZMQ_RUBY2_H

#ifdef HAVE_RUBY_THREAD_H
#include <ruby/thread.h>
#endif

#ifdef HAVE_RB_THREAD_BLOCKING_REGION
#define rb_thread_call_without_gvl(func, data1, ubf, data2) \
  rb_thread_blocking_region((rb_blocking_function_t *)func, data1, ubf, data2)
#endif

#endif
