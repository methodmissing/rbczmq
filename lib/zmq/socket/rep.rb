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

class ZMQ::Socket::Rep

# == ZMQ::Socket::Rep
#
# A socket of type ZMQ::Socket::Rep is used by a service to receive requests from and send replies to a client. This socket type
# allows only an alternating sequence of ZMQ::Socket#recv and subsequent ZMQ::Socket#send calls. Each request received is
# fair-queued from among all clients, and each reply sent is routed to the client that issued the last request. If the
# original requester doesn't exist any more the reply is silently discarded.
#
# When a ZMQ::Socket::Rep socket enters an exceptional state due to having reached the high water mark for a client, then any replies
# sent to the client in question shall be dropped until the exceptional state ends.
#
# === Summary of ZMQ::Socket#rep characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Rep
# [Direction] Bidirectional
# [Send/receive pattern] Receive, Send, Receive, Send, …
# [Incoming routing strategy] Fair-queued
# [Outgoing routing strategy] Last peer
# [ZMQ::Socket#hwm option action] Drop

  TYPE_STR = "REP"

  def type
    ZMQ::REP
  end

  def send_frame(frame, flags = 0)
    raise ZMQ::Error, "cannot send multiple frames on REP sockets" if flags == ZMQ::Frame::MORE
    super
  end

  unsupported_api :connect, :sendm
end