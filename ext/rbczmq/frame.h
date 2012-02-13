#ifndef RBCZMQ_FRAME_H
#define RBCZMQ_FRAME_H

#define ZmqAssertFrame(obj) ZmqAssertType(obj, rb_cZmqFrame, "ZMQ::Frame")
#define ZmqGetFrame(obj) \
    zframe_t *frame = NULL; \
    ZmqAssertFrame(obj); \
    Data_Get_Struct(obj, zframe_t, frame); \
    if (!frame) rb_raise(rb_eTypeError, "uninitialized ZMQ frame!"); \
    if (!(st_lookup(frames_map, (st_data_t)frame, 0))) rb_raise(rb_eZmqError, "ZMQ::Frame instance %p has been destroyed by the ZMQ framework", (void *)obj);

#define ZmqRegisterFrame(fr) \
    zframe_freefn((fr), rb_czmq_frame_freed); \
    st_insert(frames_map, (st_data_t)(fr), (st_data_t)0);

void rb_czmq_free_frame(zframe_t *frame);
void rb_czmq_free_frame_gc(void *ptr);
void rb_czmq_frame_freed(zframe_t *frame);

VALUE rb_czmq_alloc_frame(zframe_t *frame);

void _init_rb_czmq_frame();

#endif
