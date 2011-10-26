# encoding: utf-8

require "set"

class ZMQ::Loop
  READABLES = Set[ZMQ::SUB, ZMQ::PULL, ZMQ::ROUTER, ZMQ::DEALER, ZMQ::REP, ZMQ::REQ, ZMQ::PAIR]
  WRITABLES = Set[ZMQ::PUB, ZMQ::PUSH, ZMQ::ROUTER, ZMQ::DEALER, ZMQ::REP, ZMQ::REQ, ZMQ::PAIR]

  def self.run(ctx, proc = nil, &blk)
    lp = new(ctx)
    (proc || blk).call(lp)
    lp.start()
  end

  def bind(socket, address, handler = ZMQ::DefaultHandler)
    attach(socket, :bind, address, handler)
  end

  def connect(socket, address, handler = ZMQ::DefaultHandler)
    attach(socket, :connect, address, handler)
  end

  def register_readable(socket, handler = nil)
    assert_handler_for_event(socket, :on_readable)
    register_socket(socket, ZMQ::POLLIN)
  end

  def register_writable(socket, handler = nil)
    assert_handler_for_event(socket, :on_writable)
    register_socket(socket, ZMQ::POLLOUT)
  end

  def add_timer(delay, times, p = nil, &blk)
    timer = ZMQ::Timer.new(delay, times, p, &blk)
    register_timer(timer)
    timer
  end

  def add_oneshot_timer(delay, p = nil, &blk)
    add_timer(delay, 1, p, &blk)
  end

  def add_periodic_timer(delay, p = nil, &blk)
    add_timer(delay, 0, p, &blk)
  end

  private
  def attach(socket, action, address, handler)
    socket.handler = handler.new(socket) if handler
    ret = socket.__send__(action, address)
    register_readable(socket) if READABLES.include?(socket.type)
    register_writable(socket) if WRITABLES.include?(socket.type)
    ret
  end

  def assert_handler_for_event(socket, cb)
    unless socket.handler.respond_to?(cb)
      raise ZMQ::Error, "Socket #{socket.type_str}'s handler #{socket.handler.class} expected to implement an #{cb} callback!"
    end
  end
end

ZL = ZMQ::Loop