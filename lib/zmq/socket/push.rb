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

class ZMQ::Socket::Push

# == ZMQ::Socket::Push
#
# A socket of type ZMQ::Socket::Push is used by a pipeline node to send messages to downstream pipeline nodes. Messages are
# load-balanced to all connected downstream nodes. The ZMQ::Socket#recv function is not implemented for this socket type.
#
# When a ZMQ::Socket::Push socket enters an exceptional state due to having reached the high water mark for all downstream
# nodes, or if there are no downstream nodes at all, then any ZMQ::Socket#send operations on the socket shall block until
# the exceptional state ends or at least one downstream node becomes available for sending; messages are not discarded.
#
# Deprecated alias: ZMQ_DOWNSTREAM.
#
# === Summary of ZMQ::Socket::Push characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Pull
# [Direction] Unidirectional
# [Send/receive pattern] Send only
# [Incoming routing strategy] N/A
# [Outgoing routing strategy] Load-balanced
# [ZMQ::Socket#hwm option action] Block

  TYPE_STR = "PUSH"

  def type
    ZMQ::PUSH
  end

  include ZMQ::UpstreamSocket
end