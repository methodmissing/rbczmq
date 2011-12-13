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

class ZMQ::Socket::Sub

# == ZMQ::Socket::Sub
#
# A socket of type ZMQ::Socket::Sub is used by a subscriber to subscribe to data distributed by a publisher. Initially a
# ZMQ::Socket::Sub socket is not subscribed to any messages, use ZMQ::Socket#subscribe to specify which messages to
# subscribe to. The ZMQ::Socket#send function is not implemented for this socket type.
#
# === Summary of ZMQ::Socket::Sub characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Pub
# [Direction] Unidirectional
# [Send/receive pattern] Receive only
# [Incoming routing strategy] Fair-queued
# [Outgoing routing strategy] N/A
# [ZMQ::Socket#hwm option action] Drop

  TYPE_STR = "SUB"

  def type
    ZMQ::SUB
  end

  include ZMQ::DownstreamSocket
end