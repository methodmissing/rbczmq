#include <rbczmq_ext.h>

VALUE rb_mZmq;
VALUE rb_eZmqError;
VALUE rb_cZmqContext;
VALUE rb_cZmqSocket;
VALUE rb_cZmqFrame;
VALUE rb_cZmqMessage;
VALUE rb_cZmqLoop;
VALUE rb_cZmqTimer;

#ifdef HAVE_RUBY_ENCODING_H
rb_encoding *binary_encoding;
#endif

static VALUE rb_czmq_m_interrupted_p(ZMQ_UNUSED VALUE obj)
{
    return (zctx_interrupted == TRUE) ? Qtrue : Qfalse;
}

static VALUE rb_czmq_m_version(ZMQ_UNUSED VALUE obj)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    return rb_ary_new3(3, INT2NUM (major), INT2NUM (minor), INT2NUM (patch));
}

static VALUE rb_czmq_m_now(ZMQ_UNUSED VALUE obj)
{
    return INT2NUM(zclock_time());
}

static VALUE rb_czmq_m_log(ZMQ_UNUSED VALUE obj, VALUE msg)
{
    Check_Type(msg, T_STRING);
    zclock_log(StringValueCStr(msg));
    return Qnil;
}

static VALUE rb_czmq_m_error(ZMQ_UNUSED VALUE obj)
{
    int err;
    err = zmq_errno();
    if (err == 0) return rb_str_buf_new(0);
    return rb_str_new2(zmq_strerror(err));
}

extern void _init_rbczmq_context();

void Init_rbczmq_ext()
{

#ifdef HAVE_RUBY_ENCODING_H
    binary_encoding = rb_enc_find("binary");
#endif

    rb_mZmq = rb_define_module("ZMQ");

    rb_eZmqError = rb_define_class_under(rb_mZmq, "Error", rb_eStandardError);

    rb_define_module_function(rb_mZmq, "interrupted?", rb_czmq_m_interrupted_p, 0);
    rb_define_module_function(rb_mZmq, "version", rb_czmq_m_version, 0);
    rb_define_module_function(rb_mZmq, "now", rb_czmq_m_now, 0);
    rb_define_module_function(rb_mZmq, "log", rb_czmq_m_log, 1);
    rb_define_module_function(rb_mZmq, "error", rb_czmq_m_error, 0);

    rb_define_const(rb_mZmq, "POLLIN", INT2NUM(ZMQ_POLLIN));
    rb_define_const(rb_mZmq, "POLLOUT", INT2NUM(ZMQ_POLLOUT));
    rb_define_const(rb_mZmq, "POLLERR", INT2NUM(ZMQ_POLLERR));

    rb_define_const(rb_mZmq, "PAIR", INT2NUM(ZMQ_PAIR));
    rb_define_const(rb_mZmq, "SUB", INT2NUM(ZMQ_SUB));
    rb_define_const(rb_mZmq, "PUB", INT2NUM(ZMQ_PUB));
    rb_define_const(rb_mZmq, "REQ", INT2NUM(ZMQ_REQ));
    rb_define_const(rb_mZmq, "REP", INT2NUM(ZMQ_REP));
    rb_define_const(rb_mZmq, "DEALER", INT2NUM(ZMQ_DEALER));
    rb_define_const(rb_mZmq, "ROUTER", INT2NUM(ZMQ_ROUTER));
    rb_define_const(rb_mZmq, "PUSH", INT2NUM(ZMQ_PUSH));
    rb_define_const(rb_mZmq, "PULL", INT2NUM(ZMQ_PULL));

    _init_rb_czmq_context();
    _init_rb_czmq_socket();
    _init_rb_czmq_frame();
    _init_rb_czmq_message();
    _init_rb_czmq_timer();
    _init_rb_czmq_loop();
}
