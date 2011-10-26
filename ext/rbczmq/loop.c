#include <rbczmq_ext.h>

static VALUE intern_call;
static VALUE intern_readable;
static VALUE intern_writable;
static VALUE intern_error;

static VALUE rb_czmq_callback0(VALUE *args)
{
   return (NIL_P(args[2])) ? rb_funcall(args[0], args[1], 0) : rb_funcall(args[0], args[1], args[2], 1);
}

ZMQ_NOINLINE static int rb_czmq_loop_started_callback(ZMQ_UNUSED zloop_t *loop, ZMQ_UNUSED zmq_pollitem_t *item, void *arg)
{
    zmq_loop_wrapper *loop_wrapper = arg;
    loop_wrapper->running = TRUE;
    return 0;
}

ZMQ_NOINLINE static int rb_czmq_loop_breaker_callback(ZMQ_UNUSED zloop_t *loop, ZMQ_UNUSED zmq_pollitem_t *item, void *arg)
{
    zmq_loop_wrapper *loop_wrapper = arg;
    loop_wrapper->running = FALSE;
    return -1;
}

ZMQ_NOINLINE static int rb_czmq_callback(zloop_t *loop, VALUE *args)
{
    int status;
    volatile VALUE ret;
    status = 0;
    ret = rb_protect((VALUE(*)(VALUE))rb_czmq_callback0, (VALUE)args, &status);
    if (status) {
        zloop_timer(loop, 1, 1, rb_czmq_loop_breaker_callback, NULL);
        if (NIL_P(rb_errinfo())) {
            rb_jump_tag(status);
        } else {
            rb_funcall(args[0], intern_error, 1, rb_errinfo());
            return 0;
        }
    } else if (ret == Qfalse) {
        zloop_timer(loop, 1, 1, rb_czmq_loop_breaker_callback, NULL);
        return -1;
    }
    return 0;
}

ZMQ_NOINLINE static int rb_czmq_loop_timer_callback(zloop_t *loop, ZMQ_UNUSED zmq_pollitem_t *item, void *cb)
{
    int rc;
    VALUE args[3];
    args[0] = (VALUE)cb;
    args[1] = intern_call;
    args[2] = Qnil;
    rc = rb_czmq_callback(loop, args);
    return rc;
}

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
static void rb_czmq_free_loop(zmq_loop_wrapper *loop)
{
    if (loop->ctx) {
        rb_czmq_loop_stop0(loop);
        zloop_destroy(&loop->loop);
        loop->loop = NULL;
    }
}

static void rb_czmq_free_loop_gc(void *ptr)
{
    zmq_loop_wrapper *loop = ptr;
    if (loop) {
        ZmqDebugf("rb_czmq_free_loop_gc %p, %p", loop, loop->loop);
        rb_czmq_free_loop(loop);
        xfree(loop);
    }
}

static VALUE rb_czmq_loop_s_new(VALUE loop, VALUE context)
{
    zmq_loop_wrapper *lp = NULL;
    ZmqGetContext(context);
    loop = Data_Make_Struct(rb_cZmqLoop, zmq_loop_wrapper, 0, rb_czmq_free_loop_gc, lp);
    lp->loop = zloop_new();
    if (lp->loop == NULL) {
        xfree(lp);
        ZmqRaiseError;
    }
    lp->running = FALSE;
    lp->ctx = ctx->ctx;
    rb_obj_call_init(loop, 0, NULL);
    return loop;
}

static VALUE rb_czmq_loop_start(VALUE obj)
{
    int rc;
    ZmqGetLoop(obj);
    THREAD_PASS;
    zloop_timer(loop->loop, 1, 1, rb_czmq_loop_started_callback, loop);
    TRAP_BEG;
    rc = zloop_start(loop->loop);
    TRAP_END;
    return INT2NUM(rc);
}

static VALUE rb_czmq_loop_running_p(VALUE obj)
{
    ZmqGetLoop(obj);
    return (loop->running == TRUE) ? Qtrue : Qfalse;
}

static void rb_czmq_loop_stop0(zmq_loop_wrapper *loop)
{
    zloop_timer(loop->loop, 1, 1, rb_czmq_loop_breaker_callback, loop);
}

static VALUE rb_czmq_loop_stop(VALUE obj)
{
    ZmqGetLoop(obj);
    if (loop->running == FALSE) rb_raise(rb_eZmqError, "event loop not running!");
    rb_czmq_loop_stop0(loop);
    return Qnil;
}

static VALUE rb_czmq_loop_destroy(VALUE obj)
{
    ZmqGetLoop(obj);
    rb_czmq_free_loop(loop);
    return Qnil;
}

static VALUE rb_czmq_loop_set_verbose(VALUE obj, VALUE level)
{
    Bool vlevel;
    ZmqGetLoop(obj);
    vlevel = (level == Qtrue) ? TRUE : FALSE;
    zloop_set_verbose(loop->loop, vlevel);
    return Qnil;
}

static VALUE rb_czmq_loop_register_socket(VALUE obj, VALUE socket, VALUE event)
{
    int rc;
    zmq_pollitem_t *pollitem = NULL;
    ZmqGetLoop(obj);
    GetZmqSocket(socket);
    if (sock->state == ZMQ_SOCKET_PENDING)
        rb_raise(rb_eZmqError, "socket in a pending state (not bound or connected) cannot be registered with the event loop!");
    Check_Type(event, T_FIXNUM);
    pollitem = ruby_xmalloc(sizeof(zmq_pollitem_t));
    pollitem->socket = sock->socket;
    pollitem->events |= ZMQ_POLLERR | FIX2INT(event);
    rc = zloop_poller(loop->loop, pollitem, rb_czmq_loop_socket_callback, (void *)sock);
    ZmqAssert(rc);
    /* Do not block on socket close */
    zsockopt_set_linger(sock->socket, 1);
    return Qtrue;
}

static VALUE rb_czmq_loop_remove_socket(VALUE obj, VALUE socket)
{
    zmq_pollitem_t *pollitem = NULL;
    ZmqGetLoop(obj);
    GetZmqSocket(socket);
    pollitem = ruby_xmalloc(sizeof(zmq_pollitem_t));
    pollitem->socket = sock->socket;
    pollitem->events |= ZMQ_POLLIN | ZMQ_POLLOUT | ZMQ_POLLERR;
    zloop_poller_end(loop->loop, pollitem);
    return Qnil;
}

static VALUE rb_czmq_loop_register_timer(VALUE obj, VALUE tm)
{
    int rc;
    ZmqGetLoop(obj);
    ZmqGetTimer(tm);
    rc = zloop_timer(loop->loop, timer->delay, timer->times, rb_czmq_loop_timer_callback, (void *)tm);
    ZmqAssert(rc);
    return Qtrue;
}

static VALUE rb_czmq_loop_cancel_timer(VALUE obj, VALUE timer)
{
    ZmqGetLoop(obj);
    zloop_timer_end(loop->loop, (void *)timer);
    return Qnil;
}

void _init_rb_czmq_loop() {
    intern_call = rb_intern("call");
    intern_readable = rb_intern("on_readable");
    intern_writable = rb_intern("on_writable");
    intern_error = rb_intern("on_error");

    rb_cZmqLoop = rb_define_class_under(rb_mZmq, "Loop", rb_cObject);

    rb_define_singleton_method(rb_cZmqLoop, "new", rb_czmq_loop_s_new, 1);
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
