# encoding: utf-8

class ZMQ::Handler

  # The ZMQ::Pollitem instance wrapped by this handler.
  attr_reader :pollitem

  # A ZMQ::Pollitem instance is compulsary on init, with support for optional arguments if a subclasses do require them.
  #
  # pub = ctx.bind(:PUB, "tcp://127.0.0.1:5000") # lower level API
  # item = ZMQ::Pollitem.new(pub)
  # item.handler = ZMQ::Handler.new(pub)
  #
  # class ProducerHandler < ZMQ::Handler
  #   def initialize(item, producer)
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
  def initialize(pollitem, *args)
    raise TypeError.new("#{pollitem.inspect} is not a valid ZMQ::Pollitem instance") unless ZMQ::Pollitem === pollitem
    @pollitem = pollitem
  end

  # Callback invoked from ZMQ::Loop handlers when the pollable item is ready for reading. Subclasses are expected to implement
  # this contract as the default just raises NotImplementedError. It's reccommended to read in a non-blocking manner
  # from within this callback.
  #
  # def on_readable
  #   msgs << recv
  # end
  #
  def on_readable
    raise NotImplementedError, "ZMQ handlers are expected to implement an #on_readable contract"
  end

  # Callback invoked from ZMQ::Loop handlers when the pollable item is ready for writing. Subclasses are expected to implement
  # this contract as the default just raises NotImplementedError. It's reccommended to write data out as fast as possible
  # from within this callback.
  #
  # def on_writable
  #   send buffer.shift
  # end
  #
  def on_writable
    raise NotImplementedError, "ZMQ handlers are expected to implement an #on_writable contract"
  end

  # Callback for error conditions such as pollable item errors on poll and exceptions raised in callbacks. Receives an exception
  # instance as argument and raises by default.
  #
  # handler.on_error(err)  =>  raise
  #
  def on_error(exception)
    raise exception
  end

  # API that allows handlers to send data regardless of the underlying pollable item type (ZMQ::Socket or IO).
  #
  def send(*args)
    pollitem.send(*args)
  end

  # API that allows handlers to receive data regardless of the underlying pollable item type (ZMQ::Socket or IO).
  #
  def recv
    pollitem.recv
  end
end