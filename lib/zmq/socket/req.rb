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

class ZMQ::Socket::Req

# == ZMQ::Socket::Req
#
# A socket of type ZMQ::Socket::Req is used by a client to send requests to and receive replies from a service. This socket type allows
# only an alternating sequence of ZMQ::Socket#send and subsequent ZMQ::Socket#recv calls. Each request sent is load-balanced
# among all services, and each reply received is matched with the last issued request.
#
# When a ZMQ::Socket::Req socket enters an exceptional state due to having reached the high water mark for all services, or if there
# are no services at all, then any ZMQ::Socket#send operations on the socket shall block until the exceptional state ends or at
# least one service becomes available for sending; messages are not discarded.
#
# === Summary of ZMQ::Socket::Req characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Rep
# [Direction] Bidirectional
# [Send/receive pattern] Send, Receive, Send, Receive, …
# [Outgoing routing strategy] Load-balanced
# [Incoming routing strategy] Last peer
# [ZMQ::Socket#hwm option action] Block

  TYPE_STR = "REQ"

  def type
    ZMQ::REQ
  end

  def send_frame(frame, flags = 0)
    raise ZMQ::Error, "cannot send multiple frames on REP sockets" if flags == ZMQ::Frame::MORE
    super
  end

  unsupported_api :bind, :sendm
end