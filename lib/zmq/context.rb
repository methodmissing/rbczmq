# encoding: utf-8

class ZMQ::Context
  def bind(sock_type, endpoint)
    s = socket(sock_type)
    s.bind(endpoint)
    s
  end

  def connect(sock_type, endpoint)
    s = socket(sock_type)
    s.connect(endpoint)
    s
  end
end