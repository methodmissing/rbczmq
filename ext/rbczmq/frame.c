#include "rbczmq_ext.h"
static VALUE intern_data;


/*
 * :nodoc:
 *  Coerce a zframe instance to a native Ruby object.
 *
*/
VALUE rb_czmq_alloc_frame(zframe_t *frame)
{
    VALUE frame_obj;
    zmq_frame_wrapper* f;
    frame_obj = Data_Make_Struct(rb_cZmqFrame, zmq_frame_wrapper, 0, rb_czmq_free_frame_gc, f);
    f->frame = frame;
    f->flags = ZMQ_FRAME_OWNED;
    f->message = NULL;
    rb_obj_call_init(frame_obj, 0, NULL);
    return frame_obj;
}

/*
 * :nodoc:
 *  Free all resources for a frame - invoked by the lower level ZMQ::Frame#destroy as well as the GC callback. We also
 *  do a symbol table lookup here to ensure we don't double free a struct that's already been recycled from within
 *  libczmq.
 *
*/
void rb_czmq_free_frame(zmq_frame_wrapper *frame)
{
    if (frame && frame->frame && (frame->flags & ZMQ_FRAME_OWNED) != 0) {
        zframe_destroy(&frame->frame);
        frame->flags &= ~ZMQ_FRAME_OWNED;
    }
}

/*
 * :nodoc:
 *  GC free callback
 *
*/
void rb_czmq_free_frame_gc(void *ptr)
{
    zmq_frame_wrapper *frame = (zmq_frame_wrapper *)ptr;
    rb_czmq_free_frame(frame);
}

/*
 *  call-seq:
 *     ZMQ::Frame.new    =>  ZMQ::Frame
 *     ZMQ::Frame.new("data")    =>  ZMQ::Frame
 *
 *  Creates a new ZMQ::Frame instance. Can be initialized with or without data. A frame corresponds to one zmq_msg_t.
 *
 * === Examples
 *     ZMQ::Frame.new    =>  ZMQ::Frame
 *     ZMQ::Frame.new("data")    =>  ZMQ::Frame
 *
*/

static VALUE rb_czmq_frame_s_new(int argc, VALUE *argv, VALUE frame)
{
    VALUE data;
    errno = 0;
    zframe_t *fr;
    rb_scan_args(argc, argv, "01", &data);
    if (NIL_P(data)) {
        fr = zframe_new(NULL, 0);
    } else {
        Check_Type(data, T_STRING);
        fr = zframe_new(RSTRING_PTR(data), (size_t)RSTRING_LEN(data));
    }
    if (fr == NULL) {
        ZmqAssertSysError();
        rb_memerror();
    }
    return rb_czmq_alloc_frame(fr);
}

/*
 *  call-seq:
 *     frame.destroy    =>  nil
 *
 *  Explicitly destroys an allocated ZMQ::Frame instance. Useful for manual memory management, otherwise the GC
 *  will take the same action if a frame object is not reachable anymore on the next GC cycle. This is
 *  a lower level API.
 *
 * === Examples
 *     frame = ZMQ::Frame.new("data")    =>  ZMQ::Frame
 *     frame.destroy     =>  nil
 *
*/

static VALUE rb_czmq_frame_destroy(VALUE obj)
{
    ZmqGetFrame(obj);
    ZmqAssertFrameOwned(frame);
    rb_czmq_free_frame(frame);
    return Qnil;
}

/*
 *  call-seq:
 *     frame.size    =>  Fixnum
 *
 *  Returns the size of the frame.
 *
 * === Examples
 *     frame = ZMQ::Frame.new("data")    =>  ZMQ::Frame
 *     frame.size     =>  4
 *
*/

static VALUE rb_czmq_frame_size(VALUE obj)
{
    size_t size;
    ZmqGetFrame(obj);
    ZmqReturnNilUnlessFrameOwned(frame);
    size = zframe_size(frame->frame);
    return LONG2FIX(size);
}

/*
 *  call-seq:
 *     frame.data    =>  String
 *
 *  Returns the data represented by the frame as a string.
 *
 * === Examples
 *     frame = ZMQ::Frame.new("data")    =>  ZMQ::Frame
 *     frame.data     =>  "data"
 *
*/

static VALUE rb_czmq_frame_data(VALUE obj)
{
    size_t size;
    ZmqGetFrame(obj);
    ZmqReturnNilUnlessFrameOwned(frame);
    size = zframe_size(frame->frame);
    return ZmqEncode(rb_str_new((char *)zframe_data(frame->frame), (long)size));
}

/*
 *  call-seq:
 *     frame.to_s    =>  String
 *
 *  Returns a String representation of this frame.
 *
 * === Examples
 *     frame = ZMQ::Frame.new("data")    =>  ZMQ::Frame
 *     frame.to_s     =>  "data"
 *
*/

static VALUE rb_czmq_frame_to_s(VALUE obj)
{
    return rb_funcall(obj, intern_data, 0, 0);
}

/*
 *  call-seq:
 *     frame.strhex    =>  String
 *
 *  Returns a printable hex representation of this frame
 *
 * === Examples
 *     frame = ZMQ::Frame.new("message")    =>  ZMQ::Frame
 *     frame.strhex     =>  "6D657373616765"
 *
*/

static VALUE rb_czmq_frame_strhex(VALUE obj)
{
    ZmqGetFrame(obj);
    ZmqReturnNilUnlessFrameOwned(frame);
    return rb_str_new2(zframe_strhex(frame->frame));
}

/*
 *  call-seq:
 *     frame.dup    =>  ZMQ::Frame
 *
 *  Returns a copy of the current frame
 *
 * === Examples
 *     frame = ZMQ::Frame.new("message")    =>  ZMQ::Frame
 *     frame.dup     =>  ZMQ::Frame
 *
*/

static VALUE rb_czmq_frame_dup(VALUE obj)
{
    zframe_t *dup_fr = NULL;
    ZmqGetFrame(obj);
    ZmqAssertFrameOwned(frame);
    dup_fr = zframe_dup(frame->frame);
    if (dup_fr == NULL) {
        ZmqAssertSysError();
        rb_memerror();
    }
    return rb_czmq_alloc_frame(dup_fr);
}

/*
 *  call-seq:
 *     frame.data_equals?("data")    =>  boolean
 *
 *  Determines if the current frame's payload matches a given string
 *
 * === Examples
 *     frame = ZMQ::Frame.new("message")    =>  ZMQ::Frame
 *     frame.data_equals?("message")     =>  true
 *
*/

static VALUE rb_czmq_frame_data_equals_p(VALUE obj, VALUE data)
{
    ZmqGetFrame(obj);
    ZmqAssertFrameOwned(frame);
    Check_Type(data, T_STRING);
    return (zframe_streq(frame->frame, RSTRING_PTR(data)) == true) ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *     frame.more?    =>  boolean
 *
 *  Determines if the current frame is part of a multipart message.
 *
 * === Examples
 *     frame = ZMQ::Frame.new("message")    =>  ZMQ::Frame
 *     frame.more?     =>  false
 *
*/

static VALUE rb_czmq_frame_more_p(VALUE obj)
{
    ZmqGetFrame(obj);
    ZmqReturnNilUnlessFrameOwned(frame);
    return (zframe_more(frame->frame) == ZFRAME_MORE) ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *     frame.eql?(other)    =>  boolean
 *
 *  Determines if the current frame is equal to another (identical size and data).
 *
 * === Examples
 *     frame = ZMQ::Frame.new("message")    =>  ZMQ::Frame
 *     other_frame = ZMQ::Frame.new("other")
 *     frame.eql?(other_frame)     =>  false
 *
*/

static VALUE rb_czmq_frame_eql_p(VALUE obj, VALUE other_frame)
{
    zmq_frame_wrapper *other = NULL;
    ZmqGetFrame(obj);
    ZmqAssertFrameOwned(frame);
    ZmqAssertFrame(other_frame);
    Data_Get_Struct(other_frame, zmq_frame_wrapper, other);
    if (!other || !other->frame) rb_raise(rb_eTypeError, "uninitialized ZMQ frame!"); \
    ZmqAssertFrameOwned(other);
    return (zframe_eq(frame->frame, other->frame)) ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *     frame == other    =>  boolean
 *
 *  Determines if the current frame is equal to another (identical size and data).
 *
 * === Examples
 *     frame = ZMQ::Frame.new("message")    =>  ZMQ::Frame
 *     other_frame = ZMQ::Frame.new("other")
 *     frame == other_frame     =>  false
 *     frame == frame      =>  true
 *
*/

static VALUE rb_czmq_frame_equals(VALUE obj, VALUE other_frame)
{
    if (obj == other_frame) return Qtrue;
    return rb_czmq_frame_eql_p(obj, other_frame);
}

/*
 *  call-seq:
 *     frame <=> other    =>  boolean
 *
 *  Comparable support for ZMQ::Frame instances.
 *
 * === Examples
 *     frame = ZMQ::Frame.new("message")    =>  ZMQ::Frame
 *     other_frame = ZMQ::Frame.new("other")
 *     frame > other_frame     =>  true
 *
*/

static VALUE rb_czmq_frame_cmp(VALUE obj, VALUE other_frame)
{
    long diff;
    zmq_frame_wrapper *other = NULL;
    if (obj == other_frame) return INT2NUM(0);
    ZmqGetFrame(obj);
    ZmqAssertFrameOwned(frame);
    ZmqAssertFrame(other_frame);
    Data_Get_Struct(other_frame, zmq_frame_wrapper, other);
    if (!other || !other->frame) rb_raise(rb_eTypeError, "uninitialized ZMQ frame!");
    ZmqAssertFrameOwned(frame);
    diff = (zframe_size(frame->frame) - zframe_size(other->frame));
    if (diff == 0) return INT2NUM(0);
    if (diff > 0) return INT2NUM(1);
    return INT2NUM(-1);
}

/*
 *  call-seq:
 *     frame.print    =>  nil
 *
 *  Dumps out frame contents to stderr
 *
 * === Examples
 *     frame = ZMQ::Frame.new("message")    =>  ZMQ::Frame
 *     frame.print  =>  nil
 *
*/

static VALUE rb_czmq_frame_print(int argc, VALUE *argv, VALUE obj)
{
    VALUE prefix;
    const char *print_prefix = NULL;
    ZmqGetFrame(obj);
    ZmqAssertFrameOwned(frame);
    rb_scan_args(argc, argv, "01", &prefix);
    if (NIL_P(prefix)) {
        print_prefix = "";
    } else {
        Check_Type(prefix, T_STRING);
        print_prefix = RSTRING_PTR(prefix);
    }
    zframe_print(frame->frame, (char *)print_prefix);
    return Qnil;
}

/*
 *  call-seq:
 *     frame.reset("new")   =>  nil
 *
 *  Sets new content for this frame
 *
 * === Examples
 *     frame = ZMQ::Frame.new("message")    =>  ZMQ::Frame
 *     frame.reset("new")   =>  nil
 *     frame.data   =>   "new"
 *
*/

static VALUE rb_czmq_frame_reset(VALUE obj, VALUE data)
{
    errno = 0;
    ZmqGetFrame(obj);
    ZmqAssertFrameOwned(frame);
    Check_Type(data, T_STRING);
    zframe_reset(frame->frame, (char *)RSTRING_PTR(data), (size_t)RSTRING_LEN(data));
    ZmqAssertSysError();
    return Qnil;
}

/*
 * call-seq:
 *    frame.gone?   #=> false
 *    frame.destroy # or send
 *    frame.gone?   #=> true
 *
 * Return boolean indicating if the ZMQ::Frame is gone (sent or destroyed). If the message is
 * gone, accessor methods will return nil and methods requiring data or methods that mutate
 * the message will raise an exception.
*/

static VALUE rb_czmq_frame_gone(VALUE obj)
{
    ZmqGetFrame(obj);
    return (frame->flags & ZMQ_FRAME_OWNED) ||
           (frame->message != NULL && (frame->message->flags & ZMQ_MESSAGE_OWNED))
               ? Qfalse : Qtrue;
}


void _init_rb_czmq_frame()
{
    intern_data = rb_intern("data");

    rb_cZmqFrame = rb_define_class_under(rb_mZmq, "Frame", rb_cObject);

    rb_define_const(rb_cZmqFrame, "MORE", INT2NUM(ZFRAME_MORE));
    rb_define_const(rb_cZmqFrame, "REUSE", INT2NUM(ZFRAME_REUSE));
    rb_define_const(rb_cZmqFrame, "DONTWAIT", INT2NUM(ZFRAME_DONTWAIT));

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
    rb_define_method(rb_cZmqFrame, "gone?", rb_czmq_frame_gone, 0);
}
