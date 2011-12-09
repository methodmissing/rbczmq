# encoding: utf-8

class ZMQ::Handler
  attr_reader :socket
  def initialize(socket, *args)
    raise TypeError.new("#{socket.inspect} is not a valid ZMQ::Socket instance") unless ZMQ::Socket === socket
    @socket = socket
  end

  def on_readable
    raise NotImplementedError, "ZMQ handlers are expected to implement an #on_readable contract"
  end

  def on_writable
    raise NotImplementedError, "ZMQ handlers are expected to implement an #on_writable contract"
  end

  # Callback for error conditions such as socket errors on poll and exceptions raised in callbacks. Receives an exception
  # instance as argument and raises by default.
  #
  # handler.on_error(err)  =>  raise
  #
  def on_error(exception)
    raise exception
  end
end