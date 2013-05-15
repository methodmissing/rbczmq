#include "rbczmq_ext.h"

/*
 * :nodoc:
 *  Wraps rb_funcall to support callbacks with or without callbacks.
 *
*/
static VALUE rb_czmq_callback0(VALUE *args)
{
   return (NIL_P(args[2])) ? rb_funcall(args[0], args[1], 0) : rb_funcall(args[0], args[1], 1, args[2]);
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
    loop_wrapper->running = true;
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
    loop_wrapper->running = false;
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

/* data structure used to pass czmq parameters across to callbacks with ruby GVL */
struct _rb_czmq_callback_invocation
{
	int result;
	zloop_t *loop;
	VALUE* args;
	
	// used by poll item callback
	zmq_pollitem_t *item;
	void* arg;
};

/*
 * :nodoc:
 *  Low level callback for when timers registered with the reactor fires. This calls back into the Ruby VM.
 *
 * This function is called from rb_thread_call_with_gvl and is executed with the ruby GVL held.
*/
ZMQ_NOINLINE static void* rb_czmq_loop_timer_callback_with_gvl(void* data)
{
	struct _rb_czmq_callback_invocation* invocation = (struct _rb_czmq_callback_invocation*)data;
	zloop_t* loop = invocation->loop;
	zmq_pollitem_t *item = invocation->item;
	void* cb = invocation->arg;

    VALUE args[3];
    ZmqGetTimer((VALUE)cb);
    if (timer->cancelled == true) {
		zloop_timer_end(loop, (void *)cb);
		invocation->result = 0;
    } else {
	    args[0] = (VALUE)cb;
	    args[1] = intern_call;
	    args[2] = Qnil;
	
	    invocation->result = rb_czmq_callback(loop, args);
	}
    return NULL;
}


/*
 * :nodoc:
 *  Low level callback for when timers registered with the reactor fires. This calls back into the Ruby VM.
 *
 * This is the CZMQ callback, which grabs the ruby GVL and calls the with GVL callback above.
*/
ZMQ_NOINLINE static int rb_czmq_loop_timer_callback(zloop_t *loop, ZMQ_UNUSED zmq_pollitem_t *item, void *cb)
{
	struct _rb_czmq_callback_invocation invocation;
	invocation.result = -1;
	invocation.loop = loop;
	invocation.item = item;
	invocation.args = NULL;
	invocation.arg = cb;
	rb_thread_call_with_gvl(rb_czmq_loop_timer_callback_with_gvl, &invocation);
	return invocation.result;
}

/*
 * :nodoc:
 *  Low level callback for handling socket activity. This calls back into the Ruby VM. We special case ZMQ_POLLERR
 *  by invoking the error callback on the handler registered for this socket.
 *
 * This function is called from rb_thread_call_with_gvl and is executed with the ruby GVL held.
*/
ZMQ_NOINLINE static void* rb_czmq_loop_pollitem_callback_with_gvl(void* data)
{
	struct _rb_czmq_callback_invocation *invocation = (struct _rb_czmq_callback_invocation *)data;
	zloop_t *loop = invocation->loop;
	zmq_pollitem_t *item = invocation->item;
	void *arg = invocation->arg;
	
    int ret_r = 0;
    int ret_w = 0;
    int ret_e = 0;
    VALUE args[3];
    zmq_pollitem_wrapper *pollitem = (zmq_pollitem_wrapper *)arg;
    args[0] = (VALUE)pollitem->handler;
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
    if (ret_r == -1 || ret_w == -1 || ret_e == -1) {
		invocation->result = -1;
	} else {
		invocation->result = 0;
	}
	return NULL;
}

/*
 * :nodoc:
 *  Low level callback for handling socket activity. This calls back into the Ruby VM. We special case ZMQ_POLLERR
 *  by invoking the error callback on the handler registered for this socket.
 *
 * This is the CZMQ callback, which grabs the ruby GVL and calls the with GVL callback above.
*/
ZMQ_NOINLINE static int rb_czmq_loop_pollitem_callback(zloop_t *loop, zmq_pollitem_t *item, void *arg)
{
	struct _rb_czmq_callback_invocation invocation;
	invocation.result = -1;
	invocation.loop = loop;
	invocation.item = item;
	invocation.arg = arg;
	invocation.args = NULL;
	rb_thread_call_with_gvl(rb_czmq_loop_pollitem_callback_with_gvl, (void*)&invocation);
	return invocation.result;
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
    lp->running = false;
    lp->verbose = false;
    rb_obj_call_init(loop, 0, NULL);
    return loop;
}

/*
 * :nodoc:
 *  Run the zloop without holding the GVL. 
 *
*/
static VALUE rb_czmq_loop_start_nogvl(void *ptr)
{
	zmq_loop_wrapper *loop = (zmq_loop_wrapper *)ptr;
	return (VALUE)zloop_start(loop->loop);
}

/*
 * :nodoc:
 * unblocking function: called by ruby when our zloop is running without
 * gvl to tell it to stop and return back to ruby.
*/
static void rb_czmq_loop_start_ubf(void* arg)
{
    // this flag is set when an interrupt / signal would kill
    // an application using czmq and allows loops to end gracefully.
    // This will terminate all loops and pools in all threads.

    // Without setting this, the CZMQ loop does not terminate when
    // the user hits CTRL-C.
    zctx_interrupted = true;

    // do same as 
    zmq_loop_wrapper *loop_wrapper = arg;
    loop_wrapper->running = false;
}

/*
 *  call-seq:
 *     loop.start    =>  Fixnum
 *
 *  Creates a new reactor instance and blocks the caller until the process is interrupted, the context terminates or the
 *  loop's explicitly stopped via callback. Returns 0 if interrupted and -1 when stopped via a handler.
 * === Examples
 *
 *     loop = ZMQ::Loop.new    =>   ZMQ::Loop
 *     loop.start    =>   Fixnum
 *
*/

static VALUE rb_czmq_loop_start(VALUE obj)
{
    int rc;
    errno = 0;
    ZmqGetLoop(obj);
    rb_thread_schedule();
    zloop_timer(loop->loop, 1, 1, rb_czmq_loop_started_callback, loop);

    rc = (int)rb_thread_blocking_region(rb_czmq_loop_start_nogvl, (void *)loop, rb_czmq_loop_start_ubf, (void*)loop);
	
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
    return (loop->running == true) ? Qtrue : Qfalse;
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
    if (loop->running == false) rb_raise(rb_eZmqError, "event loop not running!");
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
    bool vlevel;
    ZmqGetLoop(obj);
    vlevel = (level == Qtrue) ? true : false;
    zloop_set_verbose(loop->loop, vlevel);
    loop->verbose = vlevel;
    return Qnil;
}

/*
 *  call-seq:
 *     loop.register(item)                 =>  true
 *
 *  Registers a poll item with the reactor. Only ZMQ::POLLIN and ZMQ::POLLOUT events are supported.
 *
 * === Examples
 *     loop = ZMQ::Loop.new                        =>   ZMQ::Loop
 *     item = ZMQ::Pollitem.new(sock, ZMQ::POLLIN) =>   ZMQ::Pollitem
 *     loop.register(item)                         =>   true
 *
*/

static VALUE rb_czmq_loop_register(VALUE obj, VALUE pollable)
{
    int rc;
    errno = 0;
    ZmqGetLoop(obj);
    pollable = rb_czmq_pollitem_coerce(pollable);
    ZmqGetPollitem(pollable);
    rc = zloop_poller(loop->loop, pollitem->item, rb_czmq_loop_pollitem_callback, (void *)pollitem);
    ZmqAssert(rc);
    /* Let pollable be verbose if loop is verbose */
    if (loop->verbose == true) rb_czmq_pollitem_set_verbose(pollable, Qtrue);
    return Qtrue;
}

/*
 *  call-seq:
 *     loop.remove(item)    =>  nil
 *
 *  Removes a previously registered poll item from the reactor loop.
 *
 * === Examples
 *     loop = ZMQ::Loop.new                        =>   ZMQ::Loop
 *     item = ZMQ::Pollitem.new(sock, ZMQ::POLLIN) =>   ZMQ::Pollitem
 *     loop.register(item)                         =>   true
 *     loop.remove(item)                           =>   nil
 *
*/

static VALUE rb_czmq_loop_remove(VALUE obj, VALUE pollable)
{
    errno = 0;
    ZmqGetLoop(obj);
    pollable = rb_czmq_pollitem_coerce(pollable);
    ZmqGetPollitem(pollable);
    zloop_poller_end(loop->loop, pollitem->item);
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

void _init_rb_czmq_loop()
{
    rb_cZmqLoop = rb_define_class_under(rb_mZmq, "Loop", rb_cObject);

    rb_define_alloc_func(rb_cZmqLoop, rb_czmq_loop_new);
    rb_define_method(rb_cZmqLoop, "start", rb_czmq_loop_start, 0);
    rb_define_method(rb_cZmqLoop, "stop", rb_czmq_loop_stop, 0);
    rb_define_method(rb_cZmqLoop, "running?", rb_czmq_loop_running_p, 0);
    rb_define_method(rb_cZmqLoop, "destroy", rb_czmq_loop_destroy, 0);
    rb_define_method(rb_cZmqLoop, "verbose=", rb_czmq_loop_set_verbose, 1);
    rb_define_method(rb_cZmqLoop, "register", rb_czmq_loop_register, 1);
    rb_define_method(rb_cZmqLoop, "remove", rb_czmq_loop_remove, 1);
    rb_define_method(rb_cZmqLoop, "register_timer", rb_czmq_loop_register_timer, 1);
    rb_define_method(rb_cZmqLoop, "cancel_timer", rb_czmq_loop_cancel_timer, 1);
}
