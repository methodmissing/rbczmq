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

class ZMQ::Socket::Pull

# == ZMQ::Socket::Pull
#
# A socket of type ZMQ::Socket::Pull is used by a pipeline node to receive messages from upstream pipeline nodes. Messages
# are fair-queued from among all connected upstream nodes. The ZMQ::Socket#send function is not implemented for this
# socket type.
#
# Deprecated alias: ZMQ_UPSTREAM.
#
# === Summary of ZMQ::Socket::Pull characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Push
# [Direction] Unidirectional
# [Send/receive pattern] Receive only
# [Incoming routing strategy] Fair-queued
# [Outgoing routing strategy] N/A
# [ZMQ::Socket#hwm option action] N/A

  TYPE_STR = "PULL"

  def type
    ZMQ::PULL
  end

  include ZMQ::DownstreamSocket
end