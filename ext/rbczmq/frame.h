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

#ifndef RBCZMQ_FRAME_H
#define RBCZMQ_FRAME_H

#define ZmqAssertFrame(obj) ZmqAssertType(obj, rb_cZmqFrame, "ZMQ::Frame")
#define ZmqGetFrame(obj) \
    zframe_t *frame = NULL; \
    ZmqAssertFrame(obj); \
    Data_Get_Struct(obj, zframe_t, frame); \
    if (!frame) rb_raise(rb_eTypeError, "uninitialized ZMQ frame!"); \
    if (!(st_lookup(frames_map, (st_data_t)frame, 0))) rb_raise(rb_eZmqError, "object %p has been destroyed by the ZMQ framework", (void *)obj);

#define ZmqRegisterFrame(fr) \
    zframe_freefn((fr), rb_czmq_frame_freed); \
    st_insert(frames_map, (st_data_t)(fr), (st_data_t)0);

void rb_czmq_free_frame(zframe_t *frame);
void rb_czmq_free_frame_gc(void *ptr);
void rb_czmq_frame_freed(zframe_t *frame);

VALUE rb_czmq_alloc_frame(zframe_t *frame);

void _init_rb_czmq_frame();

#endif
