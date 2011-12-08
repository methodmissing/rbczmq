# encoding: utf-8

class ZMQ::DefaultHandler < ZMQ::Handler
  def on_readable
    p socket.recv_nonblock
  end

  def on_writable
    socket.send("")
  end
end