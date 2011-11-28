#include <rbczmq_ext.h>

static VALUE intern_data;

VALUE rb_czmq_alloc_frame(zframe_t **frame, int flags)
{
    VALUE frame_obj;
    zmq_frame_wrapper *fr = NULL;
    frame_obj = Data_Make_Struct(rb_cZmqFrame, zmq_frame_wrapper, 0, rb_czmq_free_frame_gc, fr);
    fr->frame = *frame;
    fr->flags = flags;
    rb_obj_call_init(frame_obj, 0, NULL);
    return frame_obj;
}

void rb_czmq_free_frame(zmq_frame_wrapper *frame)
{
    zframe_destroy(&(frame->frame));
    frame->frame = NULL;
    frame->flags |= ZMQ_FRAME_RECYCLED;
}

void rb_czmq_free_frame_gc(void *ptr)
{
    zmq_frame_wrapper *frame = ptr;
    if (frame) {
        if (frame->frame != NULL && !(frame->flags & (ZMQ_FRAME_RECYCLED | ZMQ_FRAME_MESSAGE))) rb_czmq_free_frame(frame);
        xfree(frame);
    }
}

static VALUE rb_czmq_frame_s_new(int argc, VALUE *argv, VALUE frame)
{
    VALUE data;
    zmq_frame_wrapper *fr = NULL;
    rb_scan_args(argc, argv, "01", &data);
    frame = Data_Make_Struct(rb_cZmqFrame, zmq_frame_wrapper, 0, rb_czmq_free_frame_gc, fr);
    if (NIL_P(data)) {
        fr->frame = zframe_new(NULL, 0);
    } else {
        Check_Type(data, T_STRING);
        fr->frame = zframe_new(RSTRING_PTR(data), (size_t)RSTRING_LEN(data));
    }
    fr->flags = ZMQ_FRAME_NEW;
    ZmqAssertObjOnAlloc(fr->frame, fr);
    rb_obj_call_init(frame, 0, NULL);
    return frame;
}

static VALUE rb_czmq_frame_destroy(VALUE obj)
{
    ZmqGetFrame(obj);
    rb_czmq_free_frame(frame);
    return Qnil;
}

static VALUE rb_czmq_frame_size(VALUE obj)
{
    size_t size;
    ZmqGetFrame(obj);
    size = zframe_size(frame->frame);
    return LONG2FIX(size);
}

static VALUE rb_czmq_frame_data(VALUE obj)
{
    size_t size;
    ZmqGetFrame(obj);
    size = zframe_size(frame->frame);
    return ZmqEncode(rb_str_new((char *)zframe_data(frame->frame), (long)size));
}

static VALUE rb_czmq_frame_to_s(VALUE obj)
{
    return rb_funcall(obj, intern_data, 0, 0);
}

static VALUE rb_czmq_frame_strhex(VALUE obj)
{
    ZmqGetFrame(obj);
    return rb_str_new2(zframe_strhex(frame->frame));
}

static VALUE rb_czmq_frame_dup(VALUE obj)
{
    VALUE dup;
    zmq_frame_wrapper *dup_fr = NULL;
    ZmqGetFrame(obj);
    dup = Data_Make_Struct(rb_cZmqFrame, zmq_frame_wrapper, 0, rb_czmq_free_frame_gc, dup_fr);
    dup_fr->frame = zframe_dup(frame->frame);
    dup_fr->flags = ZMQ_FRAME_DUP;
    ZmqAssertObjOnAlloc(dup_fr->frame, dup_fr);
    rb_obj_call_init(dup, 0, NULL);
    return dup;
}

static VALUE rb_czmq_frame_data_equals_p(VALUE obj, VALUE data)
{
    ZmqGetFrame(obj);
    Check_Type(data, T_STRING);
    return (zframe_streq(frame->frame, RSTRING_PTR(data)) == TRUE) ? Qtrue : Qfalse;
}

static VALUE rb_czmq_frame_more_p(VALUE obj)
{
    ZmqGetFrame(obj);
    return (zframe_more(frame->frame) == ZFRAME_MORE) ? Qtrue : Qfalse;
}

static VALUE rb_czmq_frame_eql_p(VALUE obj, VALUE other_frame)
{
    zmq_frame_wrapper *other = NULL;
    ZmqGetFrame(obj);
    ZmqAssertFrame(other_frame);
    Data_Get_Struct(other_frame, zmq_frame_wrapper, other);
    if (!other) rb_raise(rb_eTypeError, "uninitialized ZMQ frame!");
    return (zframe_eq(frame->frame, other->frame)) ? Qtrue : Qfalse;
}

static VALUE rb_czmq_frame_equals(VALUE obj, VALUE other_frame)
{
    if (obj == other_frame) return Qtrue;
    return rb_czmq_frame_eql_p(obj, other_frame);
}

static VALUE rb_czmq_frame_cmp(VALUE obj, VALUE other_frame)
{
    long diff;
    zmq_frame_wrapper *other = NULL;
    if (obj == other_frame) return INT2FIX(0);
    ZmqGetFrame(obj);
    ZmqAssertFrame(other_frame);
    Data_Get_Struct(other_frame, zmq_frame_wrapper, other);
    if (!other) rb_raise(rb_eTypeError, "uninitialized ZMQ frame!");
    diff = (zframe_size(frame->frame) - zframe_size(other->frame));
    if (diff == 0) return INT2FIX(0);
    if (diff > 0) return INT2FIX(1);
    return INT2FIX(-1);
}

static VALUE rb_czmq_frame_print(int argc, VALUE *argv, VALUE obj)
{
    VALUE prefix;
    char *print_prefix = NULL;
    ZmqGetFrame(obj);
    rb_scan_args(argc, argv, "01", &prefix);
    print_prefix = NIL_P(prefix) ? "" : RSTRING_PTR(prefix);
    zframe_print(frame->frame, (char *)print_prefix);
    return Qnil;
}

static VALUE rb_czmq_frame_reset(VALUE obj, VALUE data)
{
    ZmqGetFrame(obj);
    Check_Type(data, T_STRING);
    zframe_reset(frame->frame, (char *)RSTRING_PTR(data), (size_t)RSTRING_LEN(data));
    return Qnil;
}

void _init_rb_czmq_frame() {
    intern_data = rb_intern("data");

    rb_cZmqFrame = rb_define_class_under(rb_mZmq, "Frame", rb_cObject);

    rb_define_const(rb_cZmqFrame, "MORE", INT2NUM(ZFRAME_MORE));
    rb_define_const(rb_cZmqFrame, "REUSE", INT2NUM(ZFRAME_REUSE));

    rb_define_singleton_method(rb_cZmqFrame, "new", rb_czmq_frame_s_new, -1);
    rb_define_method(rb_cZmqFrame, "destroy", rb_czmq_frame_destroy, 0);
    rb_define_method(rb_cZmqFrame, "size", rb_czmq_frame_size, 0);
    rb_define_method(rb_cZmqFrame, "dup", rb_czmq_frame_dup, 0);
    rb_define_method(rb_cZmqFrame, "data", rb_czmq_frame_data, 0);
    rb_define_method(rb_cZmqFrame, "to_s", rb_czmq_frame_to_s, 0);
    rb_define_method(rb_cZmqFrame, "to_str", rb_czmq_frame_to_s, 0);
    rb_define_method(rb_cZmqFrame, "strhex", rb_czmq_frame_strhex, 0);
    rb_define_method(rb_cZmqFrame, "data_equals?", rb_czmq_frame_data_equals_p, 1);
    rb_define_method(rb_cZmqFrame, "more?", rb_czmq_frame_more_p, 0);
    rb_define_method(rb_cZmqFrame, "eql?", rb_czmq_frame_eql_p, 1);
    rb_define_method(rb_cZmqFrame, "==", rb_czmq_frame_equals, 1);
    rb_define_method(rb_cZmqFrame, "<=>", rb_czmq_frame_cmp, 1);
    rb_define_method(rb_cZmqFrame, "print", rb_czmq_frame_print, -1);
    rb_define_alias(rb_cZmqFrame, "dump", "print");
    rb_define_method(rb_cZmqFrame, "reset", rb_czmq_frame_reset, 1);
}
