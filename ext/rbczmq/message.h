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

#ifndef RBCZMQ_MESSAGE_H
#define RBCZMQ_MESSAGE_H

#define ZMQ_MESSAGE_DESTROYED 0x01

typedef struct {
    zmsg_t  *message;
    int flags;
} zmq_message_wrapper;

#define ZmqAssertMessage(obj) ZmqAssertType(obj, rb_cZmqMessage, "ZMQ::Message")
#define ZmqGetMessage(obj) \
    zmq_message_wrapper *message = NULL; \
    ZmqAssertMessage(obj); \
    Data_Get_Struct(obj, zmq_message_wrapper, message); \
    if (!message) rb_raise(rb_eTypeError, "uninitialized ZMQ message!"); \
    if (message->flags & ZMQ_MESSAGE_DESTROYED) rb_raise(rb_eZmqError, "object %p has been destroyed by the ZMQ framework", (void *)obj);

VALUE rb_czmq_alloc_message(zmsg_t *message);
void rb_czmq_free_message(zmq_message_wrapper *message);

void _init_rb_czmq_message();

#endif
