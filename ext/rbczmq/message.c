#include "rbczmq_ext.h"

/*
 * :nodoc:
 *  Free all resources for a message - invoked by the lower level ZMQ::Message#destroy as well as the GC callback.
 *
*/
void rb_czmq_free_message(zmq_message_wrapper *message)
{
    errno = 0;

    if (message->message != NULL && message->flags & ZMQ_MESSAGE_OWNED) {
        zmsg_destroy(&message->message);
        message->flags &= ~ZMQ_MESSAGE_OWNED;
    }
    message->message = NULL;

    if (message->frames) {
        zlist_destroy(&message->frames);
    }
}

/*
 * :nodoc:
 *  GC free callback
 *
*/
static void rb_czmq_free_message_gc(void *ptr)
{
    zmq_message_wrapper *msg = (zmq_message_wrapper *)ptr;
    if (msg) {
        rb_czmq_free_message(msg);
        xfree(msg);
    }
}

/*
 * :nodoc:
 * mark the owned ZMQ::Frame objects so that they are not collected.
*/
void rb_czmq_mark_message(zmq_message_wrapper *message)
{
    if (message->frames) {
        VALUE frame = (VALUE)zlist_first(message->frames);
        while (frame) {
            rb_gc_mark(frame);
            frame = (VALUE)zlist_next(message->frames);
        }
    }
}

/*
 * :nodoc:
 *  Coerce a zmsg instance to a native Ruby object.
 *
*/
VALUE rb_czmq_alloc_message(zmsg_t *message)
{
    VALUE message_obj;
    zmq_message_wrapper *m = NULL;
    errno = 0;
    message_obj = Data_Make_Struct(rb_cZmqMessage, zmq_message_wrapper, rb_czmq_mark_message, rb_czmq_free_message_gc, m);
    m->message = message;
    ZmqAssertObjOnAlloc(m->message, m);
    m->flags = ZMQ_MESSAGE_OWNED;
    m->frames = zlist_new();

    /* create ZMQ::Frame objects for all frames in the message and
       link the ruby objects to this message. */
    zframe_t* zframe = zmsg_first(message);
    while (zframe) {
        VALUE frame_object = rb_czmq_alloc_frame(zframe);
        ZmqGetFrame(frame_object);
        frame->flags &= ~ZMQ_FRAME_OWNED;
        frame->message = m;
        zlist_append(m->frames, (void*)frame_object);
        
        zframe = zmsg_next(message);
    }

    rb_obj_call_init(message_obj, 0, NULL);
    return message_obj;
}

/*
 *  call-seq:
 *     ZMQ::Message.new    =>  ZMQ::Message
 *
 *  Creates an empty ZMQ::Message instance.
 *
 * === Examples
 *     ZMQ::Message.new    =>  ZMQ::Message
 *
*/

static VALUE rb_czmq_message_new(VALUE message)
{
    zmsg_t* msg = zmsg_new();
    if (msg == NULL) {
        rb_raise(rb_eZmqError, "Failed to allocate message object (zmsg_new).");
    }
    return rb_czmq_alloc_message(msg);
}

/*
 *  call-seq:
 *     msg.size    =>  Fixnum
 *
 *  Returns the size of a given ZMQ::Message instance - the number of frames.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.size    =>   0
 *     msg.pushstr "frame"  =>  true
 *     msg.size    =>   1
 *
*/

static VALUE rb_czmq_message_size(VALUE obj)
{
    ZmqGetMessage(obj);
    ZmqReturnNilUnlessOwned(message);
    return INT2NUM(zmsg_size(message->message));
}

/*
 *  call-seq:
 *     msg.content_size    =>  Fixnum
 *
 *  Returns the content size of a given ZMQ::Message instance - the sum of each frame's length.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.content_size    =>   0
 *     msg.pushstr "frame"  =>  true
 *     msg.content_size    =>   5
 *
*/

static VALUE rb_czmq_message_content_size(VALUE obj)
{
    ZmqGetMessage(obj);
    ZmqReturnNilUnlessOwned(message);
    return INT2NUM(zmsg_content_size(message->message));
}

/*
 *  call-seq:
 *     msg.push(frame)    =>  Boolean
 *
 *  Push frame to the front of the message, before all other frames. Message takes ownership of the frame and will
 *  destroy it when message is sent.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.push ZMQ::Frame(test)    =>   true
 *     msg.size     =>   1
 *
*/

static VALUE rb_czmq_message_push(VALUE obj, VALUE frame_obj)
{
    int rc = 0;
    errno = 0;
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);
    ZmqGetFrame(frame_obj);
    ZmqAssertFrameOwnedNoMessage(frame);

    rc = zmsg_push(message->message, frame->frame);
    ZmqAssert(rc);
    frame->message = message;
    frame->flags &= ~ZMQ_FRAME_OWNED;
    zlist_push(message->frames, (void*)frame_obj);
    return Qtrue;
}

/*
 *  call-seq:
 *     msg.add(frame)    =>  Boolean
 *
 *  Add frame to the end of the message, after all other frames. Message takes ownership of the frame and will
 *  destroy it when message is sent.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.add ZMQ::Frame(test)    =>   true
 *     msg.size     =>   1
 *
*/

static VALUE rb_czmq_message_add(VALUE obj, VALUE frame_obj)
{
    int rc = 0;
    errno = 0;
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);
    ZmqGetFrame(frame_obj);
    ZmqAssertFrameOwnedNoMessage(frame);

    rc = zmsg_add(message->message, frame->frame);
    ZmqAssert(rc);
    frame->message = message;
    frame->flags &= ~ZMQ_FRAME_OWNED;
    zlist_append(message->frames, (void*)frame_obj);
    return Qtrue;
}

/*
 *  call-seq:
 *     msg.pop    =>  ZMQ::Frame or nil
 *
 *  Remove first frame from message, if any. Returns a ZMQ::Frame instance or nil. Caller now owns the frame.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.pushstr "test"    =>   true
 *     msg.pop    =>   ZMQ::Frame
 *     msg.size    =>   1
 *     msg.pop    =>   nil
 *
*/

static VALUE rb_czmq_message_pop(VALUE obj)
{
    zframe_t *zframe = NULL;
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);
    zframe = zmsg_pop(message->message); /* we now own the frame */
    if (zframe == NULL) return Qnil;
    VALUE frame_obj = (VALUE)zlist_pop(message->frames);
    /* detach frame from message, it is now owned by the ruby object */
    ZmqGetFrame(frame_obj);
    frame->flags |= ZMQ_FRAME_OWNED;
    frame->message = NULL;
    return frame_obj ? frame_obj : Qnil;
}

/*
 *  call-seq:
 *     msg.print    =>  nil
 *
 *  Dumps the first 10 frames of the message to stderr for debugging.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.pushstr "test"    =>   true
 *     msg.print    =>   nil
 *
*/

static VALUE rb_czmq_message_print(VALUE obj)
{
    ZmqGetMessage(obj);
    ZmqReturnNilUnlessOwned(message);
    zmsg_dump(message->message);
    return Qnil;
}

/*
 *  call-seq:
 *     msg.first    =>  ZMQ::Frame or nil
 *
 *  Resets the cursor to the first message frame, if any.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.pushstr "test"    =>   true
 *     msg.first    =>   ZMQ::Frame
 *     msg.first    =>   nil
 *
*/

static VALUE rb_czmq_message_first(VALUE obj)
{
    ZmqGetMessage(obj);
    ZmqReturnNilUnlessOwned(message);
    VALUE frame_obj = (VALUE)zlist_first(message->frames);
    return frame_obj ? frame_obj : Qnil;
}

/*
 *  call-seq:
 *     msg.next    =>  ZMQ::Frame or nil
 *
 *  Returns the next message frame or nil if there aren't anymore. Advances the frames cursor and can thus be used for
 *  iteration.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.pushstr "test"    =>   true
 *     msg.next    =>   ZMQ::Frame
 *     msg.next    =>   nil
 *
*/

static VALUE rb_czmq_message_next(VALUE obj)
{
    ZmqGetMessage(obj);
    ZmqReturnNilUnlessOwned(message);
    VALUE frame_obj = (VALUE)zlist_next(message->frames);
    return frame_obj ? frame_obj : Qnil;
}

/*
 *  call-seq:
 *     msg.last    =>  ZMQ::Frame or nil
 *
 *  Resets the cursor to the last message frame, if any.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.pushstr "test"    =>   true
 *     msg.last    =>   ZMQ::Frame
 *     msg.last    =>   nil
 *
*/

static VALUE rb_czmq_message_last(VALUE obj)
{
    ZmqGetMessage(obj);
    ZmqReturnNilUnlessOwned(message);
    VALUE frame_obj = (VALUE)zlist_last(message->frames);
    return frame_obj ? frame_obj : Qnil;
}

/*
 *  call-seq:
 *     msg.remove(frame)    =>  nil
 *
 *  Removes the given frame from the message's frame list if present. Does not destroy the frame itself.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     frame = ZMQ::Frame("test")    =>  ZMQ::Frame
 *     msg.push(frame)    =>   true
 *     msg.size     =>   1
 *     msg.remove(frame)    =>   nil
 *     msg.size     =>   0
 *
*/

static VALUE rb_czmq_message_remove(VALUE obj, VALUE frame_obj)
{
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);
    ZmqGetFrame(frame_obj);

    /* remove from message and our list of frame objects */
    zmsg_remove(message->message, frame->frame);
    zlist_remove(message->frames, (void*)frame_obj);

    /* removing from message does not destroy the frame,
       therefore, we take ownership of the frame at this point */
    frame->message = NULL;
    frame->flags |= ZMQ_FRAME_OWNED;

    return Qnil;
}

/*
 *  call-seq:
 *     msg.pushstr(frame)    =>  Boolean
 *
 *  Push a string as a new frame to the front of the message, before all other frames.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.pushstr "test"    =>   true
 *
*/

static VALUE rb_czmq_message_pushstr(VALUE obj, VALUE str)
{
    int rc = 0;
    errno = 0;
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);
    Check_Type(str, T_STRING);
    rc = zmsg_pushmem(message->message, StringValueCStr(str), RSTRING_LEN(str));
    ZmqAssert(rc);

    /* keep zlist of frame ruby objects in sync with message's frame list */
    zframe_t* zframe = zmsg_first(message->message);
    VALUE frame_object = rb_czmq_alloc_frame(zframe);
    ZmqGetFrame(frame_object);
    frame->flags &= ~ZMQ_FRAME_OWNED;
    frame->message = message;
    zlist_push(message->frames, (void*)frame_object);

    return Qtrue;
}

/*
 *  call-seq:
 *     msg.addstr(frame)    =>  Boolean
 *
 *  Push a string as a new frame to the end of the message, after all other frames.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.addstr "test"    =>   true
 *
*/

static VALUE rb_czmq_message_addstr(VALUE obj, VALUE str)
{
    int rc = 0;
    errno = 0;
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);
    Check_Type(str, T_STRING);
    rc = zmsg_addmem(message->message, StringValueCStr(str), RSTRING_LEN(str));
    ZmqAssert(rc);

    /* keep zlist of frame ruby objects in sync with message's frame list */
    zframe_t* zframe = zmsg_last(message->message);
    VALUE frame_object = rb_czmq_alloc_frame(zframe);
    ZmqGetFrame(frame_object);
    frame->flags &= ~ZMQ_FRAME_OWNED;
    frame->message = message;
    zlist_append(message->frames, (void*)frame_object);

    return Qtrue;
}

/*
 *  call-seq:
 *     msg.popstr    =>  String or nil
 *
 *  Pop frame off front of message, return as a new string.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.addstr "test"    =>   true
 *     msg.popstr    =>   "test"
 *
*/

static VALUE rb_czmq_message_popstr(VALUE obj)
{
    char *str = NULL;
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);
    str = zmsg_popstr(message->message);
    if (str == NULL) return Qnil;

    /* destroys the frame, keep frame objects list in sync: */
    zlist_pop(message->frames);

    return rb_str_new2(str);
}

/*
 *  call-seq:
 *     msg.wrap(frame)    =>  nil
 *
 *  Push frame plus empty frame to front of message, before the first frame. Message takes ownership of frame, will
 *  destroy it when message is sent.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.wrap ZMQ::Frame("test")    =>   nil
 *     msg.size     =>   2
 *
*/

static VALUE rb_czmq_message_wrap(VALUE obj, VALUE frame_obj)
{
    errno = 0;
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);

    {
        ZmqGetFrame(frame_obj);
        ZmqAssertFrameOwned(frame);
        zmsg_wrap(message->message, frame->frame);
        frame->flags &= ~ZMQ_FRAME_OWNED;
        frame->message = message;
    }

    /* keep frame objects list in sync. Two frames have been added. frame_obj from above
       and a new one for the empty frame. */
    {
        zmsg_first(message->message);
        zframe_t* empty_frame = zmsg_next(message->message);

        VALUE empty_frame_object = rb_czmq_alloc_frame(empty_frame);
        ZmqGetFrame(empty_frame_object);
        frame->flags &= ~ZMQ_FRAME_OWNED;
        frame->message = message;

        zlist_push(message->frames, (void*)empty_frame_object);
        zlist_push(message->frames, (void*)frame_obj);
    }
    return Qnil;
}

/*
 *  call-seq:
 *     msg.unwrap   =>  ZMQ::Frame or nil
 *
 *  Pop frame off front of message, caller now owns frame. If next frame is empty, pops and destroys that empty frame.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     frame = ZMQ::Frame("test")     =>   ZMQ::Frame
 *     msg.wrap ZMQ::Frame("test")    =>   nil
 *     msg.size     =>   2
 *     msg.unwrap     =>   frame
 *     msg.size     =>   0
 *
*/

static VALUE rb_czmq_message_unwrap(VALUE obj)
{
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);

    /* reimplemented the zmsg_unwrap function for simpler logic: */
    zframe_t *zframe = zmsg_pop(message->message);
    VALUE frame_obj = 0;
    if (zframe != NULL) {
        frame_obj = (VALUE)zlist_pop(message->frames);
    }

    zframe_t *empty = zmsg_first(message->message);
    if (zframe_size(empty) == 0) {
        empty = zmsg_pop(message->message);
        zframe_destroy (&empty);
        zlist_pop(message->frames);
    }
    
    {
        ZmqGetFrame(frame_obj);
        frame->message = NULL;
        frame->flags |= ZMQ_FRAME_OWNED;
    }
    return frame_obj ? frame_obj : Qnil;
}

/*
 *  call-seq:
 *     msg.dup    =>  ZMQ::Message
 *
 *  Creates a copy of this message
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.dup    =>   ZMQ::Message
 *
*/

static VALUE rb_czmq_message_dup(VALUE obj)
{
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);

    zmsg_t* dup_msg = zmsg_dup(message->message);
    return rb_czmq_alloc_message(dup_msg);
}

/*
 *  call-seq:
 *     msg.destroy    =>  nil
 *
 *  Destroys a ZMQ::Message instance and all the frames it contains. Useful for manual memory management, otherwise the GC
 *  will take the same action if a message object is not reachable anymore on the next GC cycle. This is
 *  a lower level API.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.destroy    =>   nil
 *
*/

static VALUE rb_czmq_message_destroy(VALUE obj)
{
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);
    rb_czmq_free_message(message);
    return Qnil;
}

/*
 *  call-seq:
 *     msg.encode    =>  string
 *
 *  Encodes the message to a new buffer.
 *
 * === Examples
 *     msg = ZMQ::Message.new    =>  ZMQ::Message
 *     msg.pushstr "body"
 *     msg.pushstr "header"
 *     msg.encode     =>   "\006header\004body"
 *
*/

static VALUE rb_czmq_message_encode(VALUE obj)
{
    byte *buff;
    size_t buff_size;
    ZmqGetMessage(obj);
    ZmqReturnNilUnlessOwned(message);
    buff_size = zmsg_encode(message->message, &buff);
    return rb_str_new((char *)buff, buff_size);
}

/*
 *  call-seq:
 *     ZMQ::Message.decode("\006header\004body")    =>  ZMQ::Message
 *
 *  Decode a buffer into a new message. Returns nil if the buffer is not properly formatted.
 *
 * === Examples
 *     msg = ZMQ::Message.decode("\006header\004body")
 *     msg.popstr     =>   "header"
 *     msg.popstr     =>   "body"
 *
*/

static VALUE rb_czmq_message_s_decode(ZMQ_UNUSED VALUE obj, VALUE buffer)
{
    zmsg_t * m = NULL;
    Check_Type(buffer, T_STRING);
    m = zmsg_decode((byte *)StringValueCStr(buffer), RSTRING_LEN(buffer));
    if (m == NULL) return Qnil;
    return rb_czmq_alloc_message(m);
}

/*
 *  call-seq:
 *     msg.eql?(other)    =>  boolean
 *
 *  Determines if a message equals another. True if size, content size and serialized representation is equal.
 *
 * === Examples
 *     msg = ZMQ::Message("header", "body")
 *     other = ZMQ::Message("header", "body")
 *     msg.eql?(other)    =>   true
 *
*/

static VALUE rb_czmq_message_eql_p(VALUE obj, VALUE other_message)
{
    zmq_message_wrapper *other = NULL;
    byte *buff = NULL;
    size_t buff_size;
    byte *other_buff = NULL;
    size_t other_buff_size;
    ZmqGetMessage(obj);
    ZmqAssertMessage(other_message);
    ZmqAssertMessageOwned(message);
    Data_Get_Struct(other_message, zmq_message_wrapper, other);
    if (!other) rb_raise(rb_eTypeError, "uninitialized ZMQ message!");
    ZmqAssertMessageOwned(other);

    if (zmsg_size(message->message) != zmsg_size(other->message)) return Qfalse;
    if (zmsg_content_size(message->message) != zmsg_content_size(other->message)) return Qfalse;

    buff_size = zmsg_encode(message->message, &buff);
    other_buff_size = zmsg_encode(other->message, &other_buff);
    if (buff_size != other_buff_size) return Qfalse;
    if (strncmp((const char*)buff, (const char*)other_buff, buff_size) != 0) return Qfalse;
    return Qtrue;
}

/*
 *  call-seq:
 *     msg == other    =>  boolean
 *
 *  Determines if a message equals another. True if size, content size and serialized representation is equal.
 *
 * === Examples
 *     msg = ZMQ::Message("header", "body")
 *     other = ZMQ::Message("header", "body")
 *     msg == other    =>   true
 *
*/

static VALUE rb_czmq_message_equals(VALUE obj, VALUE other_message)
{
    if (obj == other_message) return Qtrue;
    return rb_czmq_message_eql_p(obj, other_message);
}

/*
 *  call-seq:
 *     msg.to_a    =>  Array
 *
 *  Returns an Array of all frames this message is composed of.
 *
 * === Examples
 *     ZMQ::Message.new.to_a                 =>   []
 *     msg = ZMQ::Message("header", "body")  =>   ZMQ::Message
 *     msg.to_a                              =>   [ZMQ::Frame("header"), ZMQ::Frame("body")]
 *
*/

static VALUE rb_czmq_message_to_a(VALUE obj)
{
    VALUE ary;
    ZmqGetMessage(obj);
    ZmqAssertMessageOwned(message);
    ary = rb_ary_new2(zlist_size(message->frames));
    VALUE frame_obj = (VALUE)zlist_first(message->frames);
    while (frame_obj) {
        rb_ary_push(ary, frame_obj);
        frame_obj = (VALUE)zlist_next(message->frames);
    }
    return ary;
}

/*
 * call-seq:
 *    msg.gone?   #=> false
 *    msg.destroy # or send
 *    msg.gone?   #=> true
 *
 * Return boolean indicating if the ZMQ::Message is gone (sent or destroyed). If the message is
 * gone, accessor methods will return nil and methods requiring data or methods that mutate
 * the message will raise an exception.
*/

static VALUE rb_czmq_message_gone(VALUE obj)
{
    ZmqGetMessage(obj);
    return (message->flags & ZMQ_MESSAGE_OWNED) ? Qfalse : Qtrue;
}

void _init_rb_czmq_message()
{
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
    rb_define_method(rb_cZmqMessage, "to_a", rb_czmq_message_to_a, 0);
    rb_define_method(rb_cZmqMessage, "gone?", rb_czmq_message_gone, 0);
}
