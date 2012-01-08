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
static VALUE intern_readable;
static VALUE intern_writable;
static VALUE intern_error;

/*
 * :nodoc:
 *  Wraps rb_funcall to support callbacks with or without callbacks.
 *
*/
static VALUE rb_czmq_callback0(VALUE *args)
{
   return (NIL_P(args[2])) ? rb_funcall(args[0], args[1], 0) : rb_funcall(args[0], args[1], args[2], 1);
}

/*
 * :nodoc:
 *  Callback for when the reactor started. Invoked from a a oneshot timer that fires after 1ms and updates the running
 *  member on the loop struct.
 *
*/
ZMQ_NOINLINE static int rb_czmq_loop_started_callback(ZMQ_UNUSED zloop_t *loop, ZMQ_UNUSED zmq_pollitem_t *item, void *arg)
{
    zmq_loop_wrapper *loop_wrapper = arg;
    loop_wrapper->running = TRUE;
    return 0;
}

/*
 * :nodoc:
 *  Callback to signal / break the reactor loop. Triggered when we invoke ZMQ::Loop#stop. The -1 return will trickle down
 *  to libczmq and stop the event loop.
 *
*/
ZMQ_NOINLINE static int rb_czmq_loop_breaker_callback(ZMQ_UNUSED zloop_t *loop, ZMQ_UNUSED zmq_pollitem_t *item, void *arg)
{
    zmq_loop_wrapper *loop_wrapper = arg;
    loop_wrapper->running = FALSE;
    return -1;
}

/*
 * :nodoc:
 *  Wraps calls back into the Ruby VM with rb_protect and properly bubbles up an errors to the user.
 *
*/
ZMQ_NOINLINE static int rb_czmq_callback(zloop_t *loop, VALUE *args)
{
    int status;
    volatile VALUE ret;
    status = 0;
    ret = rb_protect((VALUE(*)(VALUE))rb_czmq_callback0, (VALUE)args, &status);
    if (status) {
        zloop_timer(loop, 1, 1, rb_czmq_loop_breaker_callback, (void *)args[0]);
        if (NIL_P(rb_errinfo())) {
            rb_jump_tag(status);
        } else {
            rb_funcall(args[0], intern_error, 1, rb_errinfo());
            return 0;
        }
    } else if (ret == Qfalse) {
        zloop_timer(loop, 1, 1, rb_czmq_loop_breaker_callback, (void *)args[0]);
        return -1;
    }
    return 0;
}

/*
 * :nodoc:
 *  Low level callback for when timers registered with the reactor fires. This calls back into the Ruby VM.
 *
*/
ZMQ_NOINLINE static int rb_czmq_loop_timer_callback(zloop_t *loop, ZMQ_UNUSED zmq_pollitem_t *item, void *cb)
{
    int rc;
    VALUE args[3];
    ZmqGetTimer((VALUE)cb);
    if (timer->cancelled == TRUE) {
       zloop_timer_end(loop, (void *)cb);
       return 0;
    }
    args[0] = (VALUE)cb;
    args[1] = intern_call;
    args[2] = Qnil;
    rc = rb_czmq_callback(loop, args);
    return rc;
}

/*
 * :nodoc:
 *  Low level callback for handling socket activity. This calls back into the Ruby VM. We special case ZMQ_POLLERR
 *  by invoking the error callback on the handler registered for this socket.
 *
*/
ZMQ_NOINLINE static int rb_czmq_loop_socket_callback(zloop_t *loop, zmq_pollitem_t *item, void *arg)
{
    int ret_r = 0;
    int ret_w = 0;
    int ret_e = 0;
    VALUE args[3];
    zmq_sock_wrapper *socket = arg;
    args[0] = (VALUE)socket->handler;
    args[2] = Qnil;
    if (item->revents & ZMQ_POLLIN) {
        args[1] = intern_readable;
        ret_r = rb_czmq_callback(loop, args);
    }
    if (item->revents & ZMQ_POLLOUT) {
        args[1] = intern_writable;
        ret_w = rb_czmq_callback(loop, args);
    }
    if (item->revents & ZMQ_POLLERR) {
        args[1] = intern_error;
        args[2] = rb_exc_new2(rb_eZmqError, zmq_strerror(zmq_errno()));
        ret_e = rb_czmq_callback(loop, args);
    }
    if (ret_r == -1 || ret_w == -1 || ret_e == -1) return -1;
    return 0;
}

static void rb_czmq_loop_stop0(zmq_loop_wrapper *loop);

/*
 * :nodoc:
 *  Free all resources for a reactor loop - invoked by the lower level ZMQ::Loop#destroy as well as the GC callback
 *
*/
static void rb_czmq_free_loop(zmq_loop_wrapper *loop)
{
    rb_czmq_loop_stop0(loop);
    zloop_destroy(&(loop->loop));
    loop->loop = NULL;
    loop->flags |= ZMQ_LOOP_DESTROYED;
}

/*
 * :nodoc:
 *  GC free callback
 *
*/
static void rb_czmq_free_loop_gc(void *ptr)
{
    zmq_loop_wrapper *loop = (zmq_loop_wrapper *)ptr;
    if (loop) {
        if (loop->loop != NULL && !(loop->flags & ZMQ_LOOP_DESTROYED)) rb_czmq_free_loop(loop);
        xfree(loop);
    }
}

/*
 *  call-seq:
 *     ZMQ::Loop.new    =>  ZMQ::Loop
 *
 *  Creates a new reactor instance. Several loops per process is supported for the lower level API.
 *
 * === Examples
 *     ZMQ::Loop.new    =>   ZMQ::Loop
 *
*/

static VALUE rb_czmq_loop_new(VALUE loop)
{
    zmq_loop_wrapper *lp = NULL;
    errno = 0;
    loop = Data_Make_Struct(rb_cZmqLoop, zmq_loop_wrapper, 0, rb_czmq_free_loop_gc, lp);
    lp->loop = zloop_new();
    ZmqAssertObjOnAlloc(lp->loop, lp);
    lp->flags = 0;
    lp->running = FALSE;
    lp->verbose = FALSE;
    rb_obj_call_init(loop, 0, NULL);
    return loop;
}

/*
 *  call-seq:
 *     loop.start    =>  Fixnum
 *
 *  Creates a new reactor instance and blocks the caller until the process is interrupted, the context terminates or the
 *  loop's explicitly stopped via callback. Returns 0 if interrupted and -1 when stopped via a handler.
 *
 * === Examples
 *     loop = ZMQ::Loop.new    =>   ZMQ::Loop
 *     loop.start    =>   Fixnum
 *
*/

static VALUE rb_czmq_loop_start(VALUE obj)
{
    int rc;
    errno = 0;
    ZmqGetLoop(obj);
    THREAD_PASS;
    zloop_timer(loop->loop, 1, 1, rb_czmq_loop_started_callback, loop);
    rc = zloop_start(loop->loop);
    if (rc > 0) rb_raise(rb_eZmqError, "internal event loop error!");
    return INT2NUM(rc);
}

/*
 *  call-seq:
 *     loop.running?    =>  boolean
 *
 *  Predicate that returns true if the reactor is currently running.
 *
 * === Examples
 *     loop = ZMQ::Loop.new    =>   ZMQ::Loop
 *     loop.running?    =>   false
 *
*/

static VALUE rb_czmq_loop_running_p(VALUE obj)
{
    ZmqGetLoop(obj);
    return (loop->running == TRUE) ? Qtrue : Qfalse;
}

/*
 * :nodoc:
 *  Registers a oneshot timer that'll fire after 1msec to signal a loop break.
 *
*/
static void rb_czmq_loop_stop0(zmq_loop_wrapper *loop)
{
    zloop_timer(loop->loop, 1, 1, rb_czmq_loop_breaker_callback, loop);
}

/*
 *  call-seq:
 *     loop.stop    =>  nil
 *
 *  Stops the reactor loop. ZMQ::Loop#start will return a -1 status code as this can only be called via a handler.
 *
 * === Examples
 *     loop = ZMQ::Loop.new    =>   ZMQ::Loop
 *     loop.add_timer(1){ loop.stop }
 *     loop.start    =>   -1
 *
*/

static VALUE rb_czmq_loop_stop(VALUE obj)
{
    ZmqGetLoop(obj);
    if (loop->running == FALSE) rb_raise(rb_eZmqError, "event loop not running!");
    rb_czmq_loop_stop0(loop);
    return Qnil;
}

/*
 *  call-seq:
 *     loop.destroy    =>  nil
 *
 *  Explicitly destroys a reactor instance. Useful for manual memory management, otherwise the GC
 *  will take the same action if a message object is not reachable anymore on the next GC cycle. This is
 *  a lower level API.
 *
 * === Examples
 *     loop = ZMQ::Loop.new    =>   ZMQ::Loop
 *     loop.destroy   =>    nil
 *
*/

static VALUE rb_czmq_loop_destroy(VALUE obj)
{
    ZmqGetLoop(obj);
    rb_czmq_free_loop(loop);
    return Qnil;
}

/*
 *  call-seq:
 *     loop.verbose = true    =>  nil
 *
 *  Logs reactor activity to stdout - useful for debugging, but can be quite noisy with lots of activity.
 *
 * === Examples
 *     loop = ZMQ::Loop.new    =>   ZMQ::Loop
 *     loop.verbose = true   =>    nil
 *
*/

static VALUE rb_czmq_loop_set_verbose(VALUE obj, VALUE level)
{
    Bool vlevel;
    ZmqGetLoop(obj);
    vlevel = (level == Qtrue) ? TRUE : FALSE;
    zloop_set_verbose(loop->loop, vlevel);
    loop->verbose = vlevel;
    return Qnil;
}

/*
 *  call-seq:
 *     loop.register_socket(sock, ZMQ::POLLIN)    =>  true
 *
 *  Registers a socket for read or write events. Only ZMQ::POLLIN and ZMQ::POLLOUT events are supported. All sockets are
 *  implicitly registered for ZMQ::POLLERR events.
 *
 * === Examples
 *     loop = ZMQ::Loop.new    =>   ZMQ::Loop
 *     loop.register_socket(sock, ZMQ::POLLIN)   =>   true
 *
*/

static VALUE rb_czmq_loop_register_socket(VALUE obj, VALUE socket, VALUE event)
{
    int rc, evt;
    zmq_pollitem_t *pollitem = NULL;
    errno = 0;
    ZmqGetLoop(obj);
    GetZmqSocket(socket);
    ZmqAssertSocketNotPending(sock, "socket in a pending state (not bound or connected) and cannot be registered with the event loop!");
    ZmqSockGuardCrossThread(sock);
    Check_Type(event, T_FIXNUM);
    evt = FIX2INT(event);
    if (evt != ZMQ_POLLIN && evt != ZMQ_POLLOUT)
        rb_raise(rb_eZmqError, "invalid socket event: Only ZMQ::POLLIN and ZMQ::POLLOUT events are supported!");
    pollitem = ruby_xmalloc(sizeof(zmq_pollitem_t));
    pollitem->socket = sock->socket;
    pollitem->events |= ZMQ_POLLERR | evt;
    rc = zloop_poller(loop->loop, pollitem, rb_czmq_loop_socket_callback, (void *)sock);
    ZmqAssert(rc);
    /* Do not block on socket close */
    zsockopt_set_linger(sock->socket, 1);
   /* Let socket be verbose if loop is verbose */
    if (loop->verbose == TRUE) sock->verbose = loop->verbose;
    return Qtrue;
}

/*
 *  call-seq:
 *     loop.remove_socket(sock)    =>  nil
 *
 *  Removes a previously registered socket from the reactor loop.
 *
 * === Examples
 *     loop = ZMQ::Loop.new    =>   ZMQ::Loop
 *     loop.verbose = true   =>    nil
 *
*/

static VALUE rb_czmq_loop_remove_socket(VALUE obj, VALUE socket)
{
    zmq_pollitem_t *pollitem = NULL;
    errno = 0;
    ZmqGetLoop(obj);
    GetZmqSocket(socket);
    ZmqAssertSocketNotPending(sock, "socket in a pending state (not bound or connected) and cannot be unregistered from the event loop!");
    ZmqSockGuardCrossThread(sock);
    pollitem = ruby_xmalloc(sizeof(zmq_pollitem_t));
    pollitem->socket = sock->socket;
    pollitem->events |= ZMQ_POLLIN | ZMQ_POLLOUT | ZMQ_POLLERR;
    zloop_poller_end(loop->loop, pollitem);
    return Qnil;
}

/*
 *  call-seq:
 *     loop.register_timer(timer)    =>  true
 *
 *  Registers a ZMQ::Timer instance with the reactor.
 *
 * === Examples
 *     loop = ZMQ::Loop.new    =>   ZMQ::Loop
 *     timer = ZMQ::Timer.new(1, 2){ :fired }
 *     loop.register_timer(timer)   =>   true
 *
*/

static VALUE rb_czmq_loop_register_timer(VALUE obj, VALUE tm)
{
    int rc;
    errno = 0;
    ZmqGetLoop(obj);
    ZmqGetTimer(tm);
    rc = zloop_timer(loop->loop, timer->delay, timer->times, rb_czmq_loop_timer_callback, (void *)tm);
    ZmqAssert(rc);
    return Qtrue;
}

/*
 *  call-seq:
 *     loop.cancel_timer(timer)    =>  nil
 *
 *  Cancels a ZMQ::Timer instance previously registered with the reactor.
 *
 * === Examples
 *     loop = ZMQ::Loop.new    =>   ZMQ::Loop
 *     timer = ZMQ::Timer.new(1, 2){ :fired }
 *     loop.register_timer(timer)   =>   true
 *     loop.cancel_timer(timer)   =>   nil
 *
*/

static VALUE rb_czmq_loop_cancel_timer(VALUE obj, VALUE tm)
{
    int rc;
    errno = 0;
    ZmqGetLoop(obj);
    ZmqGetTimer(tm);
    rc = zloop_timer_end(loop->loop, (void *)tm);
    ZmqAssert(rc);
    return Qtrue;
}

void _init_rb_czmq_loop() {
    intern_call = rb_intern("call");
    intern_readable = rb_intern("on_readable");
    intern_writable = rb_intern("on_writable");
    intern_error = rb_intern("on_error");

    rb_cZmqLoop = rb_define_class_under(rb_mZmq, "Loop", rb_cObject);

    rb_define_alloc_func(rb_cZmqLoop, rb_czmq_loop_new);
    rb_define_method(rb_cZmqLoop, "start", rb_czmq_loop_start, 0);
    rb_define_method(rb_cZmqLoop, "stop", rb_czmq_loop_stop, 0);
    rb_define_method(rb_cZmqLoop, "running?", rb_czmq_loop_running_p, 0);
    rb_define_method(rb_cZmqLoop, "destroy", rb_czmq_loop_destroy, 0);
    rb_define_method(rb_cZmqLoop, "verbose=", rb_czmq_loop_set_verbose, 1);
    rb_define_method(rb_cZmqLoop, "register_socket", rb_czmq_loop_register_socket, 2);
    rb_define_method(rb_cZmqLoop, "remove_socket", rb_czmq_loop_remove_socket, 1);
    rb_define_method(rb_cZmqLoop, "register_timer", rb_czmq_loop_register_timer, 1);
    rb_define_method(rb_cZmqLoop, "cancel_timer", rb_czmq_loop_cancel_timer, 1);
}
