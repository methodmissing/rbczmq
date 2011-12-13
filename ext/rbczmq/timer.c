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

#include <rbczmq_ext.h>

static VALUE intern_call;

static void rb_czmq_mark_timer(void *ptr)
{
    zmq_timer_wrapper *timer = ptr;
    zclock_log ("I: timer %p: GC mark", timer);
    rb_gc_mark(timer->callback);
}

static void rb_czmq_free_timer_gc(void *ptr)
{
    zmq_timer_wrapper *timer = ptr;
    zclock_log ("I: timer %p: GC free", timer);
    if (timer) xfree(timer);
}

/*
 *  call-seq:
 *     ZMQ::Timer.new(1, 2){ :fired }    =>  ZMQ::Timer
 *
 *  Initializes a new ZMQ::Timer instance.
 *
 * === Examples
 *     ZMQ::Timer.new(1, 2){ :fired }    =>  ZMQ::Timer
 *
*/

VALUE rb_czmq_timer_s_new(int argc, VALUE *argv, VALUE timer)
{
    VALUE delay, times, proc, callback;
    size_t timer_delay;
    zmq_timer_wrapper *tr = NULL;
    rb_scan_args(argc, argv, "21&", &delay, &times, &proc, &callback);
    if (NIL_P(proc) && NIL_P(callback)) rb_raise(rb_eArgError, "no callback given!");
    if (NIL_P(proc)) {
        rb_need_block();
    } else {
        callback = proc;
    }
    if (TYPE(delay) != T_FIXNUM && TYPE(delay) != T_FLOAT) rb_raise(rb_eTypeError, "wrong delay type %s (expected Fixnum or Float)", RSTRING_PTR(rb_obj_as_string(delay)));
    Check_Type(times, T_FIXNUM);
    timer_delay = (size_t)(((TYPE(delay) == T_FIXNUM) ? FIX2LONG(delay) : RFLOAT_VALUE(delay)) * 1000); 
    timer = Data_Make_Struct(rb_cZmqTimer, zmq_timer_wrapper, rb_czmq_mark_timer, rb_czmq_free_timer_gc, tr);
    tr->cancelled = FALSE;
    tr->delay = timer_delay;
    tr->times = FIX2INT(times);
    tr->callback = callback;
    rb_obj_call_init(timer, 0, NULL);
    return timer;
}

/*
 *  call-seq:
 *     timer.fire   =>  ??
 *
 *  Fires a timer.
 *
 * === Examples
 *     timer = ZMQ::Timer.new(1, 2){|arg| :fired }    =>  ZMQ::Timer
 *     timer.fire(arg)
 *
*/

static VALUE rb_czmq_timer_fire(VALUE obj, VALUE args)
{
    ZmqGetTimer(obj);
    if (timer->cancelled == TRUE) rb_raise(rb_eZmqError, "cannot fire timer, already cancelled!");
    return rb_apply(timer->callback, intern_call, args); 
}

/*
 *  call-seq:
 *     timer.cancel   =>  nil
 *
 *  Fires a timer.
 *
 * === Examples
 *     timer = ZMQ::Timer.new(1, 2){|arg| :fired }    =>  ZMQ::Timer
 *     timer.cancel   => nil
 *
*/

static VALUE rb_czmq_timer_cancel(VALUE obj)
{
    ZmqGetTimer(obj);
    timer->cancelled = TRUE;
    return Qnil; 
}

void _init_rb_czmq_timer() {
    intern_call = rb_intern("call");

    rb_cZmqTimer = rb_define_class_under(rb_mZmq, "Timer", rb_cObject);

    rb_define_singleton_method(rb_cZmqTimer, "new", rb_czmq_timer_s_new, -1);
    rb_define_method(rb_cZmqTimer, "fire", rb_czmq_timer_fire, -2);
    rb_define_alias(rb_cZmqTimer, "call", "fire");
    rb_define_method(rb_cZmqTimer, "cancel", rb_czmq_timer_cancel, 0);
}

