# encoding: utf-8

#--
#
# Author:: Lourens Naudé
# Homepage::  http://github.com/methodmissing/rbczmq
# Date:: 20111213
#
#----------------------------------------------------------------------------
#
# Copyright (C) 2011 by Lourens Naudé. All Rights Reserved.
# Email: lourens at methodmissing dot com
#
# (The MIT License)
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# 'Software'), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
#---------------------------------------------------------------------------
#
#

ctx = ZMQ::Context.new
req = ctx.socket(:REQ)
req.connect(Runner::ENDPOINT)

msg = Runner.payload

messages, start_time = 0, nil
while (case Runner.encoding
       when :string
         req.send(msg)
       when :frame
         req.send_frame(ZMQ::Frame(msg))
       when :message
         m =  ZMQ::Message.new
         m.pushstr "header"
         m.pushstr msg
         m.pushstr "body"
         req.send_message(m)
       end) do
  start_time ||= Time.now
  messages += 1
  case Runner.encoding
    when :string
      req.recv
    when :frame
      req.recv_frame
    when :message
      req.recv_message
    end
  break if messages == Runner.msg_count
end

Runner.stats(start_time)