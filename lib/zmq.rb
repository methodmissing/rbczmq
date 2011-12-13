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

# Prefer compiled Rubinius bytecode in .rbx/
ENV["RBXOPT"] = "-Xrbc.db"

require "zmq/rbczmq_ext"
require 'zmq/version' unless defined? ZMQ::VERSION

module ZMQ
  # Sugaring for creating new ZMQ frames
  #
  # ZMQ::Frame("frame")  =>  ZMQ::Frame
  #
  def self.Frame(data = nil)
    ZMQ::Frame.new(data)
  end

  # Sugaring for creating new ZMQ messages
  #
  # ZMQ::Message("one", "two", "three")  =>  ZMQ::Message
  #
  def self.Message(*parts)
    m = ZMQ::Message.new
    parts.each{|p| m.addstr(p) }
    m
  end

  # Returns the ZMQ context for this process, if any
  #
  def self.context
    @__zmq_ctx_process[Process.pid]
  end

  # Higher level loop API.
  #
  # XXX: Handle cases where context is nil
  #
  def self.loop
    @loop ||= ZMQ::Loop.new(context)
  end

  autoload :Handler, 'zmq/handler'
  autoload :DefaultHandler, 'zmq/default_handler'
end

require "zmq/context"
require "zmq/socket"
require "zmq/loop"
require "zmq/timer"
require "zmq/frame"
require "zmq/message"