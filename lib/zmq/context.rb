# encoding: utf-8

class ZMQ::Context

  # Before using any ØMQ library functions the caller must initialise a ØMQ context.
  #
  # The context manages open sockets and automatically closes these before termination. Other responsibilities include :
  #
  # * A simple way to set the linger timeout on sockets
  # * C onfigure contexts for a number of I/O threads
  # * Sets-up signal (interrrupt) handling for the process.

  # Sugaring for spawning a new socket and bind to a given endpoint
  #
  # ctx.bind(:PUB, "tcp://127.0.0.1:5000")
  #
  def bind(sock_type, endpoint)
    s = socket(sock_type)
    s.bind(endpoint)
    s
  end

  # Sugaring for spawning a new socket and connect to a given endpoint
  #
  # ctx.connect(:SUB, "tcp://127.0.0.1:5000")
  #
  def connect(sock_type, endpoint)
    s = socket(sock_type)
    s.connect(endpoint)
    s
  end
end