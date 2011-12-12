# encoding: utf-8

class ZMQ::Handler

  # The ZMQ::Socket instance wrapped by this handler.
  attr_reader :socket

  # A ZMQ::Socket instance is compulsary on init, with support for optional arguments if a subclasses do require them.
  #
  # pub = ctx.bind(:PUB, "tcp://127.0.0.1:5000") # lower level API
  # pub.handler = ZMQ::Handler.new(pub)
  #
  # class ProducerHandler < ZMQ::Handler
  #   def initialize(socket, producer)
  #     super
  #     @producer = producer
  #   end
  #
  #   def on_writable
  #     @producer.work
  #   end
  # end
  #
  # ZMQ::Loop.bind(:PUB, "tcp://127.0.0.1:5000", ProducerHandler, producer) # higher level API
  #
  def initialize(socket, *args)
    raise TypeError.new("#{socket.inspect} is not a valid ZMQ::Socket instance") unless ZMQ::Socket === socket
    @socket = socket
  end

  # Callback invoked from ZMQ::Loop handlers when the socket is ready for reading. Subclasses are expected to implement
  # this contract as the default just raises NotImplementedError. It's reccommended to read in a non-blocking manner
  # from within this callback.
  #
  # def on_readable
  #   msgs << socket.recv_nonblock
  # end
  #
  def on_readable
    raise NotImplementedError, "ZMQ handlers are expected to implement an #on_readable contract"
  end

  # Callback invoked from ZMQ::Loop handlers when the socket is ready for writing. Subclasses are expected to implement
  # this contract as the default just raises NotImplementedError. It's reccommended to write data out as fast as possible
  # from within this callback.
  #
  # def on_writable
  #   socket.send buffer.shift
  # end
  #
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