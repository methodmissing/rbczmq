# encoding: utf-8

class ZMQ::Handler
  attr_reader :socket
  def initialize(socket, *args)
    @socket = socket
  end

  # Callback for error conditions such as socket errors on poll and exceptions raised in callbacks. Receives an exception
  # instance as argument and raises by default.
  #
  # handler.on_error(err)  =>  raise
  #
  def on_error(exception)
    raise exception
  end

  # Stub for on connect / bind callbacks
  #
  # XXX: viable to support this ?
  #
  def post_init
  end
end