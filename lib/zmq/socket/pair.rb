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

class ZMQ::Socket::Pair

# == ZMQ::Socket::Pair
#
# A socket of type ZMQ::Socket::Pair can only be connected to a single peer at any one time. No message routing or filtering
# is performed on messages sent over a ZMQ::Socket::Pair socket.
#
# When a ZMQ::Socket::Pair socket enters an exceptional state due to having reached the high water mark for the connected
# peer, or if no peer is connected, then any ZMQ::Socket#send operations on the socket shall block until the peer becomes
# available for sending; messages are not discarded.
#
# === Summary of ZMQ::Socket::Pair characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Pair
# [Direction] Bidirectional
# [Send/receive pattern] Unrestricted
# [Incoming routing strategy] N/A
# [Outgoing routing strategy] N/A
# [ZMQ::Socket#hwm option action] Block

  TYPE_STR = "PAIR"

  def type
    ZMQ::PAIR
  end
end