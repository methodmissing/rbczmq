# encoding: utf-8

class ZMQ::Poller

  # API sugar to poll non-blocking. Returns immediately if there's no sockets in a ready state.
  #
  def poll_nonblock
    poll(0)
  end

  # API sugar for registering a socket for readability
  #
  def register_readable(socket)
    register ZMQ::Pollitem.new(socket, ZMQ::POLLIN)
  end

  # API sugar for registering a socket for writability
  #
  def register_writable(socket)
    register ZMQ::Pollitem.new(socket, ZMQ::POLLOUT)
  end
end