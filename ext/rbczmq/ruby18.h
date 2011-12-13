/*
 *
 * Author:: Lourens Naudé
 * Homepage::  http://github.com/methodmissing/rbczmq
 * Date:: 20111213
 *
 *----------------------------------------------------------------------------
 *
 * Copyright (C) 2011 by Lourens Naudé. All Rights Reserved.
 * Email: lourens at methodmissing dot com
 *
 * (The MIT License)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *---------------------------------------------------------------------------
 *
 */

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

#define rb_errinfo() ruby_errinfo

#endif
