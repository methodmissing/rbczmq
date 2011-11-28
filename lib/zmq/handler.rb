# encoding: utf-8

class ZMQ::Handler
  attr_reader :socket
  def initialize(socket, *args)
    @socket = socket
  end

  def on_error(exception)
    raise exception
  end

  def post_init
  end
end