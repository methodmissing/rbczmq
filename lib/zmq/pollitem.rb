# encoding: utf-8

class ZMQ::Pollitem
  # API that allows poll items to send data regardless of the underlying pollable item type (ZMQ::Socket or IO).
  #
  def send(*args)
    case pollable
    when BasicSocket
      pollable.send(*args)
    when IO
      pollable.write_nonblock(*args)
    when ZMQ::Socket
      pollable.send(*args)
    end
  end

  # API that allows poll items to recv data regardless of the underlying pollable item type (ZMQ::Socket or IO).
  #
  def recv
    case pollable
    when BasicSocket
      # XXX assumed page size
      pollable.recv_nonblock(4096)
    when IO
      # XXX assumed page size
      pollable.read_nonblock(4096)
    when ZMQ::Socket
      pollable.recv_nonblock
    end
  end
end