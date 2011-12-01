#include <rbczmq_ext.h>

void rb_czmq_free_message(zmq_message_wrapper *message)
{
    errno = 0;
    zmsg_destroy(&(message->message));
    message->flags |= ZMQ_FRAME_RECYCLED;
    message->message = NULL;
}

static void rb_czmq_free_message_gc(void *ptr)
{
    zmq_message_wrapper *msg = ptr;
    if (msg) {
        if (msg->message != NULL && !(msg->flags & ZMQ_MESSAGE_RECYCLED)) rb_czmq_free_message(msg);
        xfree(msg);
    }
}

VALUE rb_czmq_alloc_message(zmsg_t *message)
{
    VALUE message_obj;
    zmq_message_wrapper *m = NULL;
    errno = 0;
    message_obj = Data_Make_Struct(rb_cZmqMessage, zmq_message_wrapper, 0, rb_czmq_free_message_gc, m);
    m->message = message;
    ZmqAssertObjOnAlloc(m->message, m);
    m->flags = 0;
    rb_obj_call_init(message_obj, 0, NULL);
    return message_obj;
}

static VALUE rb_czmq_message_new(VALUE message)
{
    zmq_message_wrapper *msg = NULL;
    errno = 0;
    message = Data_Make_Struct(rb_cZmqMessage, zmq_message_wrapper, 0, rb_czmq_free_message_gc, msg);
    msg->message = zmsg_new();
    ZmqAssertObjOnAlloc(msg->message, msg);
    msg->flags = 0;
    rb_obj_call_init(message, 0, NULL);
    return message;
}

static VALUE rb_czmq_message_size(VALUE obj)
{
    ZmqGetMessage(obj);
    return INT2NUM(zmsg_size(message->message));
}

static VALUE rb_czmq_message_content_size(VALUE obj)
{
    ZmqGetMessage(obj);
    return INT2NUM(zmsg_content_size(message->message));
}

static VALUE rb_czmq_message_push(VALUE obj, VALUE frame_obj)
{
    int rc = 0;
    errno = 0;
    ZmqGetMessage(obj);
    ZmqGetFrame(frame_obj);
    rc = zmsg_push(message->message, frame->frame);
    frame->flags |= ZMQ_FRAME_MESSAGE;
    ZmqAssert(rc);
    return Qtrue;
}

static VALUE rb_czmq_message_add(VALUE obj, VALUE frame_obj)
{
    int rc = 0;
    errno = 0;
    ZmqGetMessage(obj);
    ZmqGetFrame(frame_obj);
    rc = zmsg_add(message->message, frame->frame);
    frame->flags |= ZMQ_FRAME_MESSAGE;
    ZmqAssert(rc);
    return Qtrue;
}

static VALUE rb_czmq_message_pop(VALUE obj)
{
    zframe_t *frame = NULL;
    ZmqGetMessage(obj);
    frame = zmsg_pop(message->message);
    if (frame == NULL) return Qnil;
    return rb_czmq_alloc_frame(&frame, ZMQ_FRAME_ALLOC | ZMQ_FRAME_MESSAGE);
}

static VALUE rb_czmq_message_print(VALUE obj)
{
    ZmqGetMessage(obj);
    zmsg_dump(message->message);
    return Qnil;
}

static VALUE rb_czmq_message_first(VALUE obj)
{
    zframe_t *frame = NULL;
    ZmqGetMessage(obj);
    frame = zmsg_first(message->message);
    if (frame == NULL) return Qnil;
    return rb_czmq_alloc_frame(&frame, ZMQ_FRAME_ALLOC | ZMQ_FRAME_MESSAGE);
}

static VALUE rb_czmq_message_next(VALUE obj)
{
    zframe_t *frame = NULL;
    ZmqGetMessage(obj);
    frame = zmsg_next(message->message);
    if (frame == NULL) return Qnil;
    return rb_czmq_alloc_frame(&frame, ZMQ_FRAME_ALLOC | ZMQ_FRAME_MESSAGE);
}

static VALUE rb_czmq_message_last(VALUE obj)
{
    zframe_t *frame = NULL;
    ZmqGetMessage(obj);
    frame = zmsg_last(message->message);
    if (frame == NULL) return Qnil;
    return rb_czmq_alloc_frame(&frame, ZMQ_FRAME_ALLOC | ZMQ_FRAME_MESSAGE);
}

static VALUE rb_czmq_message_remove(VALUE obj, VALUE frame_obj)
{
    ZmqGetMessage(obj);
    ZmqGetFrame(frame_obj);
    zmsg_remove(message->message, frame->frame);
    return Qnil;
}

static VALUE rb_czmq_message_pushstr(VALUE obj, VALUE str)
{
    int rc = 0;
    errno = 0;
    ZmqGetMessage(obj);
    Check_Type(str, T_STRING);
    rc = zmsg_pushstr(message->message, StringValueCStr(str));
    ZmqAssert(rc);
    return Qtrue;
}

static VALUE rb_czmq_message_addstr(VALUE obj, VALUE str)
{
    int rc = 0;
    errno = 0;
    ZmqGetMessage(obj);
    Check_Type(str, T_STRING);
    rc = zmsg_addstr(message->message, StringValueCStr(str));
    ZmqAssert(rc);
    return Qtrue;
}

static VALUE rb_czmq_message_popstr(VALUE obj)
{
    char *str = NULL;
    ZmqGetMessage(obj);
    str = zmsg_popstr(message->message);
    if (str == NULL) return Qnil;
    return rb_str_new2(str);
}

static VALUE rb_czmq_message_wrap(VALUE obj, VALUE frame_obj)
{
    int rc = 0;
    errno = 0;
    ZmqGetMessage(obj);
    ZmqGetFrame(frame_obj);
    rc = zmsg_wrap(message->message, frame->frame);
    frame->flags |= ZMQ_FRAME_MESSAGE;
    ZmqAssert(rc);
    return Qtrue;
}

static VALUE rb_czmq_message_unwrap(VALUE obj)
{
    zframe_t *frame = NULL;
    ZmqGetMessage(obj);
    frame = zmsg_unwrap(message->message);
    if (frame == NULL) return Qnil;
    return rb_czmq_alloc_frame(&frame, ZMQ_FRAME_ALLOC);
}

static VALUE rb_czmq_message_dup(VALUE obj)
{
    VALUE dup;
    zmq_message_wrapper *dup_msg = NULL;
    errno = 0;
    ZmqGetMessage(obj);
    dup = Data_Make_Struct(rb_cZmqMessage, zmq_message_wrapper, 0, rb_czmq_free_message_gc, dup_msg);
    dup_msg->message = zmsg_dup(message->message);
    ZmqAssertObjOnAlloc(dup_msg->message, dup_msg);
    dup_msg->flags = message->flags;
    rb_obj_call_init(dup, 0, NULL);
    return dup;
}

static VALUE rb_czmq_message_destroy(VALUE obj)
{
    ZmqGetMessage(obj);
    rb_czmq_free_message(message);
    return Qnil;
}

static VALUE rb_czmq_message_encode(VALUE obj)
{
    byte *buff;
    size_t buff_size;
    ZmqGetMessage(obj);
    buff_size = zmsg_encode(message->message, &buff);
    return rb_str_new((char *)buff, buff_size);
}

static VALUE rb_czmq_message_s_decode(ZMQ_UNUSED VALUE obj, VALUE buffer)
{
    zmsg_t * m = NULL;
    m = zmsg_decode((byte *)StringValueCStr(buffer), RSTRING_LEN(buffer));
    if (m == NULL) return Qnil;
    return rb_czmq_alloc_message(m);
}

static VALUE rb_czmq_message_eql_p(VALUE obj, VALUE other_message)
{
    zmq_message_wrapper *other = NULL;
    byte *buff = NULL;
    size_t buff_size;
    byte *other_buff = NULL;
    size_t other_buff_size;
    ZmqGetMessage(obj);
    ZmqAssertMessage(other_message);
    Data_Get_Struct(other_message, zmq_message_wrapper, other);
    if (!other) rb_raise(rb_eTypeError, "uninitialized ZMQ message!");

    if (zmsg_size(message->message) != zmsg_size(other->message)) return Qfalse;
    if (zmsg_content_size(message->message) != zmsg_content_size(other->message)) return Qfalse;

    buff_size = zmsg_encode(message->message, &buff);
    other_buff_size = zmsg_encode(other->message, &other_buff);
    if (buff_size != other_buff_size) return Qfalse;
    if (strncmp((const char*)buff, (const char*)other_buff, buff_size) != 0) return Qfalse;
    return Qtrue;
}

static VALUE rb_czmq_message_equals(VALUE obj, VALUE other_message)
{
    if (obj == other_message) return Qtrue;
    return rb_czmq_message_eql_p(obj, other_message);
}

void _init_rb_czmq_message() {
    rb_cZmqMessage = rb_define_class_under(rb_mZmq, "Message", rb_cObject);

    rb_define_singleton_method(rb_cZmqMessage, "decode", rb_czmq_message_s_decode, 1);

    rb_define_alloc_func(rb_cZmqMessage, rb_czmq_message_new);
    rb_define_method(rb_cZmqMessage, "size", rb_czmq_message_size, 0);
    rb_define_method(rb_cZmqMessage, "content_size", rb_czmq_message_content_size, 0);
    rb_define_method(rb_cZmqMessage, "push", rb_czmq_message_push, 1);
    rb_define_method(rb_cZmqMessage, "pop", rb_czmq_message_pop, 0);
    rb_define_method(rb_cZmqMessage, "add", rb_czmq_message_add, 1);
    rb_define_method(rb_cZmqMessage, "wrap", rb_czmq_message_wrap, 1);
    rb_define_method(rb_cZmqMessage, "unwrap", rb_czmq_message_unwrap, 0);
    rb_define_method(rb_cZmqMessage, "print", rb_czmq_message_print, 0);
    rb_define_method(rb_cZmqMessage, "first", rb_czmq_message_first, 0);
    rb_define_method(rb_cZmqMessage, "next", rb_czmq_message_next, 0);
    rb_define_method(rb_cZmqMessage, "last", rb_czmq_message_last, 0);
    rb_define_method(rb_cZmqMessage, "remove", rb_czmq_message_remove, 1);
    rb_define_method(rb_cZmqMessage, "pushstr", rb_czmq_message_pushstr, 1);
    rb_define_method(rb_cZmqMessage, "popstr", rb_czmq_message_popstr, 0);
    rb_define_method(rb_cZmqMessage, "addstr", rb_czmq_message_addstr, 1);
    rb_define_method(rb_cZmqMessage, "dup", rb_czmq_message_dup, 0);
    rb_define_method(rb_cZmqMessage, "destroy", rb_czmq_message_destroy, 0);
    rb_define_method(rb_cZmqMessage, "encode", rb_czmq_message_encode, 0);
    rb_define_method(rb_cZmqMessage, "eql?", rb_czmq_message_equals, 1);
    rb_define_method(rb_cZmqMessage, "==", rb_czmq_message_equals, 1);
}
