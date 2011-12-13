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

class ZMQ::Socket::Dealer

# == ZMQ::Socket::Dealer
#
# A socket of type ZMQ::Socket::Dealer is an advanced pattern used for extending request/reply sockets. Each message sent is
# load-balanced among all connected peers, and each message received is fair-queued from all connected peers.
#
# Previously this socket was called ZMQ_XREQ and that name remains available for backwards compatibility.
#
# When a ZMQ::Socket::Dealer socket enters an exceptional state due to having reached the high water mark for all peers, or if
# there are no peers at all, then any ZMQ::Socket#send operations on the socket shall block until the exceptional state ends
# or at least one peer becomes available for sending; messages are not discarded.
#
# When a ZMQ::Socket::Dealer socket is connected to a ZMQ::Socket::Rep socket each message sent must consist of an empty message part,
# the delimiter, followed by one or more body parts.
#
# === Summary of ZMQ::Socket::Dealer characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Router, ZMQ::Socket::Request, ZMQ::Socket::Reply
# [Direction] Bidirectional
# [Send/receive pattern] Unrestricted
# [Outgoing routing strategy] Load-balanced
# [Incoming routing strategy] Fair-queued
# [ZMQ_HWM option action] Block

  TYPE_STR = "DEALER"

  def type
    ZMQ::DEALER
  end
end