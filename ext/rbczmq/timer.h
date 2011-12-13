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

#ifndef RBCZMQ_TIMER_H
#define RBCZMQ_TIMER_H

typedef struct {
    size_t delay;
    size_t times;
    Bool cancelled;
    VALUE callback;
} zmq_timer_wrapper;

#define ZmqAssertTimer(obj) ZmqAssertType(obj, rb_cZmqTimer, "ZMQ::Timer")
#define ZmqGetTimer(obj) \
    zmq_timer_wrapper *timer = NULL; \
    ZmqAssertTimer(obj); \
    Data_Get_Struct(obj, zmq_timer_wrapper, timer); \
    if (!timer) rb_raise(rb_eTypeError, "uninitialized ZMQ timer!");

VALUE rb_czmq_timer_s_new(int argc, VALUE *argv, VALUE timer);

void _init_rb_czmq_timer();

#endif

