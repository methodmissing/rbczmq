# encoding: utf-8

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