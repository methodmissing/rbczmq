# encoding: utf-8

class ZMQ::Poller

  # API sugar to poll non-blocking. Returns immediately if there's no items in a ready state.
  #
  def poll_nonblock
    poll(0)
  end

  # API sugar for registering a ZMQ::Socket or IO for readability
  #
  def register_readable(pollable)
    register ZMQ::Pollitem.new(pollable, ZMQ::POLLIN)
  end

  # API sugar for registering a ZMQ::Socket or IO for writability
  #
  def register_writable(pollable)
    register ZMQ::Pollitem.new(pollable, ZMQ::POLLOUT)
  end
end