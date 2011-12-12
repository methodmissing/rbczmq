# encoding: utf-8

class ZMQ::DefaultHandler < ZMQ::Handler

  # A default / blanket socket callback handler for when a socket is registered on a ZMQ::Loop instance.
  #
  # XXX: Likely a massive fail for some socket pairs as a default - look into removing this.

  def on_readable
    p socket.recv_nonblock
  end

  def on_writable
    socket.send("")
  end
end