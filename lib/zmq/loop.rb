# encoding: utf-8

require "set"
require "forwardable"

class ZMQ::Loop

  # The ZMQ::Loop class provides an event-driven reactor pattern. The reactor handlers ZMQ::Socket instances and once-off
  # or repeated timers. Its resolution is 1 msec. It uses a tickless timer to reduce CPU interrupts in inactive processes.

  class << self
    extend Forwardable
    def_delegators :instance, :context, :stop, :running?, :verbose=, :register_timer, :cancel_timer, :register_socket, :remove_socket
    private
    attr_accessor :instance
  end

  # Start the reactor. Takes control of the thread and returns when :
  #
  # * the 0MQ context is terminated or
  # * the process is interrupted or
  # * any event handler raises an error
  # * any event handler returns false
  #
  # ZMQ::Loop.run do # or ZMQ::Loop.run(&proc)
  #   ZL.add_oneshot_timer(0.2){ ZL.stop }
  # end
  #
  def self.run(proc = nil, &blk)
    self.instance = ZMQ::Loop.new
    (proc || blk).call
    instance.start
  end

  # A higher level API for socket bind and loop registration.
  #
  # ZMQ::Loop.run do
  #   ZL.bind(pub, "inproc://fanout", Producer)
  # end
  #
  def self.bind(socket, address, handler = ZMQ::DefaultHandler, *args)
    attach(socket, :bind, address, handler, *args)
  end

  # A higher level API for socket bind and loop registration.
  #
  # ZMQ::Loop.run do
  #   ZL.bind(pub, "inproc://fanout", Producer)
  #   ZL.connect(sub, "inproc://fanout", Consumer)
  # end
  #
  def self.connect(socket, address, handler = ZMQ::DefaultHandler, *args)
    attach(socket, :connect, address, handler, *args)
  end

  # Registers a given ZMQ::Socket instance for readable events notification.
  #
  # ZMQ::Loop.run do
  #   ZL.register_readable(sub, "inproc://fanout", Consumer)
  # end
  #
  def self.register_readable(socket, handler = ZMQ::DefaultHandler, *args)
    socket.handler = handler.new(socket, *args) if handler
    assert_handler_for_event(socket, :on_readable)
    instance.register_socket(socket, ZMQ::POLLIN)
  end

  # Registers a given ZMQ::Socket instance for writable events notification.
  #
  # ZMQ::Loop.run do
  #   ZL.register_writable(pub, "inproc://fanout", Producer)
  # end
  #
  def self.register_writable(socket, handler = ZMQ::DefaultHandler, *args)
    socket.handler = handler.new(socket, *args) if handler
    assert_handler_for_event(socket, :on_writable)
    instance.register_socket(socket, ZMQ::POLLOUT)
  end

  # Registers a oneshot timer with the event loop.
  #
  # ZMQ::Loop.run do
  #   ZL.add_oneshot_timer(0.2){ :work } # Fires once after 0.2s
  # end
  #
  def self.add_oneshot_timer(delay, p = nil, &blk)
    add_timer(delay, 1, p, &blk)
  end

  # Registers a periodic timer with the event loop.
  #
  # ZMQ::Loop.run do
  #   ZL.add_oneshot_timer(0.2){ :work } # Fires every 0.2s
  # end
  #
  def self.add_periodic_timer(delay, p = nil, &blk)
    add_timer(delay, 0, p, &blk)
  end

  # Lower level interface for timer registration
  #
  # ZMQ::Loop.run do
  #  timer = ZL.add_timer(0.1, 5){ :work } # Fires 5 times at 0.1s intervals
  # end
  #
  def self.add_timer(delay, times, p = nil, &blk)
    timer = ZMQ::Timer.new(delay, times, p, &blk)
    instance.register_timer(timer)
    timer
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