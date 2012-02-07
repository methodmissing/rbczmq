# encoding: utf-8

# Prefer compiled Rubinius bytecode in .rbx/
ENV["RBXOPT"] = "-Xrbc.db"

begin
require "zmq/rbczmq_ext"
rescue LoadError
  require "rbczmq_ext"
end

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

  # Sugaring for creating new poll items
  #
  # ZMQ::Pollitem(STDIN, ZMQ::POLLIN)  =>  ZMQ::Pollitem
  #
  def self.Pollitem(pollable, events = nil)
    ZMQ::Pollitem.new(pollable, events)
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

  # API sugaring: IO.select compatible API, but for ZMQ sockets.
  #
  def self.select(read = [], write = [], error = [], timeout = nil)
    poller = ZMQ::Poller.new
    read.each{|s| poller.register_readable(s) } if read
    write.each{|s| poller.register_writable(s) } if write
    ready = poller.poll(timeout)
    [poller.readables, poller.writables, []] if ready
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
require "zmq/poller"