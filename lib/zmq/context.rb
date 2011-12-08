# encoding: utf-8

class ZMQ::Context
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