# encoding: utf-8

class ZMQ::Poller
  def poll_nonblock
    poll(0)
  end

  def register_readable(socket)
    register socket, ZMQ::POLLIN
  end

  def register_writable(socket)
    register socket, ZMQ::POLLOUT
  end
end