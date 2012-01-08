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

class ZMQ::Socket
  def self.unsupported_api(*methods)
    methods.each do |m|
      class_eval <<-"evl", __FILE__, __LINE__
        def #{m}(*args); raise(ZMQ::Error, "API #{m} not supported for #{const_get(:TYPE_STR)} sockets!");  end
      evl
    end
  end

  def self.handle_fsm_errors(error, *methods)
    methods.each do |m|
      class_eval <<-"evl", __FILE__, __LINE__
        def #{m}(*args);
          super
        rescue SystemCallError => e
          raise(ZMQ::Error, "#{error} Please assert that you're not sending / receiving out of band data when using the REQ / REP socket pairs.") if e.errno == ZMQ::EFSM
          raise
        end
      evl
    end
  end

  # Determines if there are one or more messages to read from this socket. Should be used in conjunction with the
  # ZMQ_FD socket option for edge-triggered notifications.
  #
  # socket.readable? => true
  #
  def readable?
    (events & ZMQ::POLLIN) == ZMQ::POLLIN
  end

  # Determines if this socket is in a writable state. Should be used in conjunction with the ZMQ_FD socket option for
  # edge-triggered notifications.
  #
  # socket.writable? => true
  #
  def writable?
    (events & ZMQ::POLLOUT) == ZMQ::POLLOUT
  end

  # Generates a string representation of this socket type
  #
  # socket = ctx.socket(:PUB)
  # socket.type_str => "PUB"
  #
  def type_str
    self.class.const_get(:TYPE_STR)
  end

  # Generates a string representation of the current socket state
  #
  # socket = ctx.bind(:PUB, "tcp://127.0.0.1:5000")
  # socket.to_s => "PUB socket bound to tcp://127.0.0.1:5000"
  #
  def to_s
    case state
    when BOUND
      "#{type_str} socket bound to #{endpoint}"
    when CONNECTED
      "#{type_str} socket connected to #{endpoint}"
    else
      "#{type_str} socket"
    end
  end

  # Poll all sockets for readbable states by default
  def poll_readable?
    true
  end

  # Poll all sockets for writable states by default
  def poll_writable?
    true
  end
end

module ZMQ::DownstreamSocket
  # An interface for sockets that can only receive (read) data
  #
  # === Behavior
  #
  # [Disabled methods] ZMQ::Socket#bind, ZMQ::Socket#send, ZMQ::Socket#sendm, ZMQ::Socket#send_frame,
  #                    ZMQ::Socket#send_message
  # [Socket types] ZMQ::Socket::Pull, ZMQ::Socket::Sub

  def self.included(sock)
    sock.unsupported_api :bind, :send, :sendm, :send_frame, :send_message
  end

  # Upstream sockets should never be polled for writable states
  def poll_writable?
    false
  end
end

module ZMQ::UpstreamSocket
  # An interface for sockets that can only send (write) data
  #
  # === Behavior
  #
  # [Disabled methods] ZMQ::Socket#connect, ZMQ::Socket#recv, ZMQ::Socket#recv_nonblock, ZMQ::Socket#recv_frame,
  #                    ZMQ::Socket#recv_frame_nonblock, ZMQ::Socket#recv_message
  # [Socket types] ZMQ::Socket::Push, ZMQ::Socket::Pub

  def self.included(sock)
    sock.unsupported_api :connect, :recv, :recv_nonblock, :recv_frame, :recv_frame_nonblock, :recv_message
  end

  # Upstream sockets should never be polled for readable states
  def poll_readable?
    false
  end
end

require "zmq/socket/pub"
require "zmq/socket/sub"
require "zmq/socket/push"
require "zmq/socket/pull"
require "zmq/socket/pair"
require "zmq/socket/req"
require "zmq/socket/rep"
require "zmq/socket/router"
require "zmq/socket/dealer"
