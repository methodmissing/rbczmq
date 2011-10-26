# encoding: utf-8

require "zmq/rbczmq_ext"
require 'zmq/version' unless defined? ZMQ::VERSION

module ZMQ
  def self.Frame(data = nil)
    ZMQ::Frame.new(data)
  end

  def self.context
    @__zmq_ctx_process
  end

  def self.loop
    @loop ||= ZMQ::Loop.new(context)
  end

  autoload :Handler, 'zmq/handler'
  autoload :DefaultHandler, 'zmq/default_handler'
end

require "zmq/socket"
require "zmq/loop"
require "zmq/timer"
require "zmq/frame"
require "zmq/message"