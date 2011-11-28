# encoding: utf-8

require "set"
require "forwardable"

class ZMQ::Loop
  class << self
    extend Forwardable
    def_delegators :instance, :context, :stop, :running?, :verbose=, :register_timer, :cancel_timer, :register_socket, :remove_socket
    private
    attr_accessor :instance
  end

  def self.run(proc = nil, &blk)
    self.instance = ZMQ::Loop.new
    (proc || blk).call
    instance.start
  end

  def self.bind(socket, address, handler = ZMQ::DefaultHandler, *args)
    attach(socket, :bind, address, handler, *args)
  end

  def self.connect(socket, address, handler = ZMQ::DefaultHandler, *args)
    attach(socket, :connect, address, handler, *args)
  end

  def self.register_readable(socket, handler = ZMQ::DefaultHandler, *args)
    socket.handler = handler.new(socket, *args) if handler
    assert_handler_for_event(socket, :on_readable)
    instance.register_socket(socket, ZMQ::POLLIN)
  end

  def self.register_writable(socket, handler = ZMQ::DefaultHandler, *args)
    socket.handler = handler.new(socket, *args) if handler
    assert_handler_for_event(socket, :on_writable)
    instance.register_socket(socket, ZMQ::POLLOUT)
  end

  def self.add_timer(delay, times, p = nil, &blk)
    timer = ZMQ::Timer.new(delay, times, p, &blk)
    instance.register_timer(timer)
    timer
  end

  def self.add_oneshot_timer(delay, p = nil, &blk)
    add_timer(delay, 1, p, &blk)
  end

  def self.add_periodic_timer(delay, p = nil, &blk)
    add_timer(delay, 0, p, &blk)
  end

  private
  def self.attach(socket, action, address, handler, *args)
    ret = socket.__send__(action, address)
    register_readable(socket, handler, args) if socket.poll_readable?
    register_writable(socket, handler, args) if socket.poll_writable?
    ret
  end

  def self.assert_handler_for_event(socket, cb)
    unless socket.handler.respond_to?(cb)
      socket.handler = nil
      raise ZMQ::Error, "Socket #{socket.type_str}'s handler #{socket.handler.class} expected to implement an #{cb} callback!"
    end
  end
end

ZL = ZMQ::Loop